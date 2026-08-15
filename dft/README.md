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