# Discrete Fourier Transform

Every chapter so far has worked on signals as lists of samples taken over time.
This one changes the description without changing the signal. A run of samples
can also be written as a set of amplitudes, one for each frequency it contains,
and the two descriptions hold exactly the same information: either can be
computed from the other, and nothing is lost going either way.

The claim underneath that is stronger than it first sounds. Any periodic signal
is a sum of sinusoids at the right amplitudes, including signals that look
nothing like waves. A square wave is flat and then it jumps, and it is still
nothing but twelve sine waves added together, which is one of the apps here.

The transform that does this for sampled signals is the DFT. It is built from a
single idea repeated: multiply the signal by a test wave, add up the products,
and see what survives. Where the test wave matches something in the signal the
products pile up, and where it does not they cancel. Everything below is that
sentence at different sizes.

## Four Signals, Four Transforms

Fourier analysis comes in four versions because signals come in four kinds. A
signal is continuous or discrete, and it is periodic or it is not:

| | aperiodic | periodic |
| --- | --- | --- |
| **continuous** | Fourier Transform | Fourier Series |
| **discrete** | Discrete Time Fourier Transform | Discrete Fourier Transform |

The top row is pen and paper work: both signals run to infinity and neither can
be held in memory. The bottom left runs to infinity too, in samples rather than
in time. Only the bottom right, discrete and periodic, is a finite list of
numbers, and it is the only one a microcontroller can compute. That is the DFT,
and it is the subject of this chapter.

Each of the four has a real and a complex version. Everything here is the real
version, where all the numbers are ordinary numbers.

## What Comes Out

An $N$ point signal $x[n]$ goes in, and two signals come out:

$ x[n] \longrightarrow \mathrm{ReX}[k], \; \mathrm{ImX}[k] $

each of them $N/2 + 1$ points long. `ReX` holds the amplitude of the cosine at
each frequency and `ImX` holds the amplitude of the sine. They are called real
and imaginary for historical reasons and they are neither: every number in both
arrays is an ordinary float. Reading them as *the cosine amplitudes* and *the
sine amplitudes* is more useful and never misleading.

Time domain signals are written in lowercase and frequency domain signals in
uppercase, which is the convention the rest of this repository follows too.

Going one way is the forward DFT, also called analysis or decomposition. Coming
back is the inverse DFT, also called synthesis.

## Reading the Horizontal Axis

The frequency axis can be labelled three ways: as a bin index from 0 to $N/2$,
as a fraction of the sampling rate from 0 to 0.5, or in Hz. The bin index is
what the array gives you and it means nothing on its own, so the apps here
always print Hz beside it:

$ f_k = k \, \frac{f_s}{N} $

The axis stops at $f_s / 2$ and not at $f_s$, and the reason is the sampling
theorem from an earlier chapter. If the signal was sampled properly there is
nothing above half the sampling rate to report, so there is no bin for it. The
spacing between bins, $f_s / N$, is the resolution: a 192 point window at 48 kHz
resolves 250 Hz and cannot tell 1000 Hz from 1100 Hz.

## Finding a Frequency, and Finding Nothing

Bin $k$ is one question asked with one sum:

$ \mathrm{ReX}[k] = \sum_{i=0}^{N-1} x[i] \cos\!\left(\frac{2 \pi k i}{N}\right)
\qquad
\mathrm{ImX}[k] = \sum_{i=0}^{N-1} x[i] \sin\!\left(\frac{2 \pi k i}{N}\right) $

Every bin runs the same loop. What separates a bin that finds something from a
bin that finds nothing is only what is left after the products cancel, and
[`dft_by_hand`](app/Src/dft_by_hand.c) prints both cases on eight samples:

```
  k=1     1.500   1.207  -0.000   0.500   0.500  -0.207   0.000   0.500    4.000
  k=3     1.500  -1.207   0.000  -0.500   0.500   0.207  -0.000  -0.500    0.000
```

The second row is not smaller than the first. It is the same size, arranged in
pairs of opposite sign.

That app also settles where the odd normalization in the inverse transform comes
from. Bin 0 multiplies the signal by a wave that is 1 at every sample, so all
$N$ samples contribute. Every other bin multiplies by a wave that is negative
half the time, so only $N/2$ worth survives. A constant of 0.5 across 8 samples
therefore comes back as 4, and a cosine of amplitude 1 also comes back as 4, and
the two arrived by different routes.

## Magnitude Is Not `ReX`

How much of a frequency a signal holds is

$ \mathrm{Mag}[k] = \sqrt{\mathrm{ReX}[k]^2 + \mathrm{ImX}[k]^2} $

and never `ReX` on its own. The split between the two arrays is a statement
about phase, not about size: the same wave shifted in time moves amplitude from
one array to the other without changing how loud it is.

This matters more than it sounds. The test signal in
[`signals.c`](app/Src/signals.c) is built from two *sines*, so its `ReX` is zero
at both of its own frequencies and every bit of its amplitude sits in `ImX`.
[`dft_spectrum`](app/Src/dft_spectrum.c) prints all three columns side by side
so that is visible rather than assumed.

## The Window Decides What You See

The transform has no idea the signal continues past the window it was given. It
treats the window as one period of something that repeats forever. If the window
holds a whole number of periods, the repeats join up smoothly and each frequency
lands on exactly one bin. If it does not, the joint is a step, a step is broad in
frequency, and energy that belonged in one bin smears across its neighbours.
That smearing is called leakage.

At 48 kHz a 1 kHz wave takes 48 samples per period. A window of 192 samples
holds four of them and the spectrum is two clean spikes. A window of 320 holds
6.667, and the same signal through the same code gives this instead:

```
    k        Hz  magnitude
    7    1050.0    130.374
  100   15000.0     80.292
    6     900.0     68.341
    8    1200.0     31.335
```

The 15 kHz component is still exactly on a bin and still sharp. The 1 kHz has
spread over three bins and none of them is at 1000 Hz. On a plot that reads as
one spike and one low hill, and the natural conclusion, that the transform lost
a frequency, is wrong. [`dft_leakage`](app/Src/dft_leakage.c) runs both windows
back to back so the difference cannot be blamed on anything else.

The fix is to choose the window as a whole number of periods, or to taper its
ends so there is no step to be broad about. Window functions are the subject of
their own chapter; choosing the length is free and worth doing first.

## Round Trip

[`dft_inverse`](app/Src/dft_inverse.c) sends the signal out and brings it back:

$ x[i] = \sum_{k=0}^{N/2} \bar{\mathrm{Re}}X[k] \cos\!\left(\frac{2 \pi k i}{N}\right)
+ \sum_{k=0}^{N/2} \bar{\mathrm{Im}}X[k] \sin\!\left(\frac{2 \pi k i}{N}\right) $

where the bars mean the bins have been scaled back down, by $N$ at $k = 0$ and
$k = N/2$ and by $N/2$ everywhere else. On the two tone signal the round trip
comes back within 0.002% of the signal's own height, and the difference trace on
the debugger is a flat line.

The bookkeeping is worth one look. Two arrays of 97 points is 194 numbers, and
192 went in. Nothing was gained: $\mathrm{ImX}[0]$ and $\mathrm{ImX}[N/2]$ are
sums of sines that are zero at every sample, so they are always zero. Drop those
two and the count matches exactly, which is why CMSIS-DSP packs the whole answer
into $N$ floats.

## Building a Square Wave

[`dft_synthesis`](app/Src/dft_synthesis.c) is the opening claim made literal. It
takes a square wave, transforms it, and adds the bins back one at a time while
streaming the running total to the debugger. One harmonic is a sine. Three and
the corners appear. Twelve and it is the square wave, exactly.

```
 1 harmonic(s), up to  1000 Hz, peak 1.2714, overshoot 27.14%
 3 harmonic(s), up to  5000 Hz, peak 1.1774, overshoot 17.74%
12 harmonic(s), up to 23000 Hz, peak 1.0000, overshoot  0.00%
```

The overshoot at the corners is Gibbs, and the textbook version of Gibbs never
goes away no matter how many harmonics are added. It goes away here, and the
reason is worth carrying. This square wave was sampled. Its period is 48
samples, so it has exactly twelve harmonics below half the sampling rate and
there is no thirteenth to add. The ringing the textbook describes lives between
the samples, where a sampled signal has nothing to say.

## CMSIS-DSP and the Cost of Doing It Directly

`arm_rfft_fast_f32` computes the same spectrum, and
[`dft_cmsis`](app/Src/dft_cmsis.c) is mostly about the two things that must be
lined up before the numbers can be compared.

The packing: slot 0 holds bin 0, slot 1 holds bin $N/2$, and the rest are real
and imaginary in pairs. Reading it as a plain array of pairs puts the Nyquist
bin where bin 1 belongs.

The sign: CMSIS takes the sine sum with a minus in front and
[`dft.c`](app/Src/dft.c) does not, so `ImX` comes back negated. Neither is more
correct, and the magnitude does not notice, which is one more argument for
plotting the magnitude.

The two agree to about $10^{-4}$ on a 256 point window, and most of that gap
belongs to the direct version. It accumulates 256 float additions per bin where
the FFT reaches the same place in eight stages, so the fast one is also the more
accurate one.

[`dft_timing`](app/Src/dft_timing.c) measures what that costs. The direct
transform is $N^2$ multiply accumulates and the FFT is closer to $N \log N$, so
doubling the length roughly quadruples one and roughly doubles the other. It
runs at 128 points rather than the 192 used elsewhere, because the cycle counter
is 24 bits and can measure 1.048 s at 16 MHz and no further, and a direct
transform of a few hundred points runs past that and wraps without saying so.
The app checks its own measurement against that ceiling rather than trusting it.

One practical note that has nothing to do with the mathematics and is worth more
than most of it: `arm_rfft_fast_init_f32` pulls in twiddle tables for every
length from 32 to 4096, while `arm_rfft_fast_init_256_f32` brings only what was
asked for. Swapping one for the other took 77 kB off the image.

## Watching the Signals in Ozone

The transform has an input in one domain and an output in another, so the
debugger carries both:

| probe | carries |
| --- | --- |
| `g_x` | the signal going in |
| `g_re` | `ReX`, the cosine amplitudes |
| `g_im` | `ImX`, the sine amplitudes |
| `g_mag` | the magnitude at each frequency |
| `g_rebuilt` | the signal after a trip back |

They are declared in [`probe.c`](app/Src/probe.c) and wired into the Data
Sampling window by [`app.jdebug`](workspaces/app.jdebug), so every app opens the
same way. The frequency traces are $N/2 + 1$ points against $N$ in the time
traces, so they fill the left half of each sweep and sit at zero after, which is
the shape of the answer to why a real spectrum is half as long as its signal.

Every app calls `probe_reset` first, including the two that never stream
anything. The linker runs with `--gc-sections`, so a probe that no app in the
build mentions is dropped from the image, and Ozone then has nothing to attach
that trace to and quietly shows one graph fewer.

## Apps

Each app is a self-contained `main` that prints its numbers once and then
streams the probes, so `make monitor` catches the output and the Timeline shows
the signals. Two of them compute a table and stop, and those two are worth
reading on the terminal rather than in the debugger.

0. [The transform by hand](app/Src/dft_by_hand.c): eight samples built from a
   constant, one cosine and one sine, with the eight products behind two of the
   bins printed so a bin that finds something and a bin that finds nothing can
   be compared line by line. The scaling that the inverse transform has to undo
   falls out of the last three rows rather than being asserted.
1. [The spectrum of a real signal](app/Src/dft_spectrum.c): 192 samples through
   the transform, reported as bin number, frequency in Hz, both halves of the
   answer and the magnitude. A compile time knob swaps the input between one
   sine, two sines and a square wave, and the `ReX` column shows why magnitude
   is never one array on its own.
2. [Leakage](app/Src/dft_leakage.c): the same signal and the same code through
   a window that holds a whole number of periods and one that does not, showing
   a frequency spread across three bins with none of them at the right place.
3. [The round trip](app/Src/dft_inverse.c): forward and back, with the
   difference carried on its own trace, and the accounting that shows 194
   numbers came back from 192 without anything having been gained.
4. [Building a square wave](app/Src/dft_synthesis.c): a square wave rebuilt one
   harmonic at a time on the Timeline, with the overshoot at the corners
   reported at each step and reaching zero at the twelfth, which is as many
   harmonics as a sampled square wave has.
5. [The library transform](app/Src/dft_cmsis.c): `arm_rfft_fast_f32` next to
   the direct sum, with its packing unpicked and its sine convention flipped so
   the two can be compared bin by bin.
6. [Why the FFT exists](app/Src/dft_timing.c): the direct transform and the fast
   one timed with SysTick as a cycle counter, checked against the counter's own
   24 bit ceiling, and extrapolated out to the lengths where doing it directly
   stops being an option.