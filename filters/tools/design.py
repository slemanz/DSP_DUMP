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

def summary():
    print(f"fs = {FS:.0f} Hz, {SIG_LEN} samples holds 1, 10 and 50 whole periods")
    print()
    print(f"{'kernel':<13}{'taps':>5}{'sum':>11}"
          f"{'10 Hz':>9}{'100 Hz':>9}{'500 Hz':>9}")
    for name, h in KERNELS.items():
        print(f"{name:<13}{len(h):>5}{h.sum():>11.6f}"
              f"{gain(h, 10):>9.5f}{gain(h, 100):>9.5f}{gain(h, 500):>9.5f}")

if __name__ == "__main__":
    summary()
    if "--write" in sys.argv:
        print()
#       write_all()