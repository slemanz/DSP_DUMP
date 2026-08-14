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
