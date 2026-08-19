"""Designs every kernel and every test signal this module uses, and writes them
out as C. None of the generated numbers is ever typed into the firmware by hand.

    python3 tools/design.py            print the design summary
    python3 tools/design.py --write    also write the four generated files

Run it from the module root, so the paths below land in app/.

scipy's firwin takes the cutoff in Hz once fs is given, so there is no
normalised frequency to get wrong. MATLAB's fir1 takes it as a fraction of the
Nyquist rate, which is fs/2 and not fs.
"""

import sys
import numpy as np
from scipy.signal import firwin, freqz

FS = 2000.0             # sampling rate of everything in this module
SIG_LEN = 200           # 1 period of 10 Hz, 10 of 100 Hz, 50 of 500 Hz
NOISE_LEN = 600         # longer, because a standard deviation needs samples
NOISE_SIGMA = 0.30
NOISE_SEED = 7

KERNELS = {
    # the simplest kernel there is
    "ma_11":       np.ones(11) / 11,
    # one cutoff and one length, three windows
    "lp_rect":     firwin(101, 200, fs=FS, window="boxcar"),
    "lp_hamming":  firwin(101, 200, fs=FS, window="hamming"),
    "lp_blackman": firwin(101, 200, fs=FS, window="blackman"),
    # one cutoff and one window, three lengths
    "lp_31":       firwin(31,  200, fs=FS, window="hamming"),
    "lp_201":      firwin(201, 200, fs=FS, window="hamming"),
    # one kernel per tone in the test signal
    "lp_50":       firwin(101,  50, fs=FS, window="hamming"),
    "bp_50_300":   firwin(101, [50, 300], fs=FS, pass_zero=False, window="hamming"),
    "hp_300":      firwin(101, 300, fs=FS, pass_zero=False, window="hamming"),
}

def spectral_inversion(h):
    """Turns a low pass into the high pass that completes it."""
    out = -h.copy()
    out[(len(h) - 1) // 2] += 1.0
    return out


def response(h):
    freq, values = freqz(h, worN=65536, fs=FS)
    return freq, np.abs(values)


def gain(h, f_hz):
    freq, mag = response(h)
    return float(np.interp(f_hz, freq, mag))

def stopband_db(h):
    """Height of the worst ripple past the first null, in dB.

    firwin puts the -6 dB point exactly on the cutoff, so the search starts
    there, walks down to the first place the response stops falling, and takes
    the largest thing left after it.
    """
    freq, mag = response(h)
    tail = mag[np.argmax(mag <= 0.5):]
    null = np.flatnonzero(tail[1:] >= tail[:-1])[0] + 1
    return 20 * np.log10(tail[null:].max())


def transition_hz(h):
    """How far it takes to get from 99% of the passband down to 1%."""
    freq, mag = response(h)
    lo = freq[mag >= 0.99][-1]
    hi = freq[(freq > lo) & (mag <= 0.01)][0]
    return hi - lo


def signals():
    n = np.arange(SIG_LEN)
    tone_10 = np.sin(2 * np.pi * 10 * n / FS)
    tone_100 = np.sin(2 * np.pi * 100 * n / FS)
    tone_500 = np.sin(2 * np.pi * 500 * n / FS)
    three = tone_10 + tone_100 + tone_500

    m = np.arange(NOISE_LEN)
    rng = np.random.default_rng(NOISE_SEED)
    noisy = np.sin(2 * np.pi * 10 * m / FS) + rng.normal(0, NOISE_SIGMA, NOISE_LEN)

    return {
        "sig_3tone": three,
        "tone_10": tone_10,
        "tone_100": tone_100,
        "tone_500": tone_500,
        "sig_noisy": noisy,
        # what python gets for the same input through the same kernel, so the
        # firmware can be checked against it rather than eyeballed
        "ref_lp50": np.convolve(three, KERNELS["lp_50"]),
    }

def summary():
    print(f"fs = {FS:.0f} Hz, {SIG_LEN} samples holds 1, 10 and 50 whole periods")
    print()
    print(f"{'kernel':<13}{'taps':>5}{'sum':>11}"
          f"{'10 Hz':>9}{'100 Hz':>9}{'500 Hz':>9}")
    for name, h in KERNELS.items():
        print(f"{name:<13}{len(h):>5}{h.sum():>11.6f}"
              f"{gain(h, 10):>9.5f}{gain(h, 100):>9.5f}{gain(h, 500):>9.5f}")

    print()
    print("one cutoff and one length, three windows")
    print(f"{'kernel':<13}{'taps':>5}{'stopband':>11}{'transition':>12}")
    for name in ("lp_rect", "lp_hamming", "lp_blackman"):
        h = KERNELS[name]
        print(f"{name:<13}{len(h):>5}{stopband_db(h):>8.1f} dB"
              f"{transition_hz(h):>9.1f} Hz")

    print()
    print("one cutoff and one window, three lengths")
    print(f"{'kernel':<13}{'taps':>5}{'stopband':>11}{'transition':>12}{'delay':>7}")
    for name in ("lp_31", "lp_hamming", "lp_201"):
        h = KERNELS[name]
        print(f"{name:<13}{len(h):>5}{stopband_db(h):>8.1f} dB"
              f"{transition_hz(h):>9.1f} Hz{(len(h) - 1) // 2:>7}")

    lp = KERNELS["lp_hamming"]
    hp = spectral_inversion(lp)
    impulse = np.zeros(len(lp))
    impulse[(len(lp) - 1) // 2] = 1.0
    print()
    print("spectral inversion turns lp_hamming into the high pass that completes it")
    print(f"  worst gap between lp + hp and a single impulse: "
          f"{np.abs(lp + hp - impulse).max():.3e}")

def as_c_array(name, values):
    body = ""
    for i in range(0, len(values), 5):
        body += "    " + ", ".join(f"{v:+.9f}f" for v in values[i:i + 5]) + ",\n"
    return f"const float32_t {name}[{len(values)}] =\n{{\n{body}}};\n"


def write(path, text):
    with open(path, "w") as handle:
        handle.write(text)
    print(f"wrote {path}")


BANNER = ("/*\n * Generated by tools/design.py. Do not edit by hand; edit the\n"
          " * script and run it again.\n */\n\n")

def write_all():
    head = '#ifndef INC_KERNELS_H_\n#define INC_KERNELS_H_\n\n#include "arm_math.h"\n\n'
    for name, h in KERNELS.items():
        head += f"#define {(name + '_LEN').upper():<16}{len(h)}\n"
    head += "\n"
    for name in KERNELS:
        head += f"extern const float32_t {name}[{(name + '_LEN').upper()}];\n"
    head += "\n#endif /* INC_KERNELS_H_ */\n"
    write("app/Inc/kernels.h", BANNER + head)

    body = BANNER + '#include "kernels.h"\n\n'
    for name, h in KERNELS.items():
        body += as_c_array(name, h) + "\n"
    write("app/Src/kernels.c", body)

    sig = signals()
    lengths = {"sig_noisy": "NOISE_LEN", "ref_lp50": "REF_LP50_LEN"}
    head = ('#ifndef INC_TESTSIG_H_\n#define INC_TESTSIG_H_\n\n'
            '#include "arm_math.h"\n\n'
            f"#define TESTSIG_FS_HZ   {FS:.0f}U\n"
            f"#define SIG_LEN         {SIG_LEN}\n"
            f"#define NOISE_LEN       {NOISE_LEN}\n"
            f"#define REF_LP50_LEN    {len(sig['ref_lp50'])}\n\n")
    for name in sig:
        head += f"extern const float32_t {name}[{lengths.get(name, 'SIG_LEN')}];\n"
    head += "\n#endif /* INC_TESTSIG_H_ */\n"
    write("app/Inc/testsig.h", BANNER + head)

    body = BANNER + '#include "testsig.h"\n\n'
    for name, values in sig.items():
        body += as_c_array(name, values) + "\n"
    write("app/Src/testsig.c", body)


if __name__ == "__main__":
    summary()
    if "--write" in sys.argv:
        print()
        write_all()