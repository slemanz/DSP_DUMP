# Digital Filter Design

The convolution chapter taught the operation and handed the kernel over
finished. Twenty nine numbers arrived, they were convolved with a signal, and a
15 kHz tone went away. Where those twenty nine numbers came from was never said.

This chapter is that question, and the answer starts by noticing that a kernel
has two descriptions and they are the same object. In the time domain it is a
short list of taps. In the frequency domain it is a curve saying what the filter
does to every frequency. Either one can be computed from the other, and
designing a filter means choosing which end to start from.

The curve is computed with the sum from the previous chapter. `fir_gain` in
[`filters.c`](app/Src/filters.c) is the DFT sum with one change: it is evaluated
at whatever frequency is asked for rather than at a bin. A kernel is a finite
list and its response is continuous, so there is no window length here and
nothing to leak.

## Two Knobs That Do Not Interact

Almost everything in FIR design is two tables, and the useful part is the column
that stays still.

Hold the length and the cutoff fixed and change how the kernel is tapered:

| window | stopband | transition |
| --- | --- | --- |
| rectangular | -21.2 dB | 23.5 Hz |
| hamming | -52.9 dB | 59.5 Hz |
| blackman | -75.3 dB | 81.0 Hz |

Both columns move, in opposite directions. A deeper stopband is bought with a
wider transition, every time.

Now hold the window and the cutoff fixed and change the length:

| taps | stopband | transition | delay |
| --- | --- | --- | --- |
| 31 | -51.4 dB | 201.0 Hz | 15 |
| 101 | -52.9 dB | 59.5 Hz | 50 |
| 201 | -53.5 dB | 30.0 Hz | 100 |

The stopband column does not move. The window put it at -53 dB and no amount of
length will shift it. What length buys is a narrower transition, halving each
time the taps double, and what it costs is delay and arithmetic.

So the two choices are independent. Pick the window from how much rejection the
job needs, pick the length from how sharp the corner has to be, and neither
answer disturbs the other. [`fir_windows`](app/Src/fir_windows.c) and
[`fir_length`](app/Src/fir_length.c) are those two tables computed on the chip.

## Why There Is a Window at All

A filter that passes everything below a cutoff and nothing above it has exactly
one kernel, and that kernel is a sinc. The sinc never ends.

Any kernel that fits in memory is a piece cut out of it, and cutting a signal
off abruptly is the same act that produced the ringing on the square wave two
chapters ago. The cut is a step, a step is broad in frequency, and the breadth
shows up as ripple in the stopband. Truncating the ideal filter is what makes it
non-ideal, and the rectangular row in the first table is the price: 21 dB of
rejection, which is a factor of eleven, which is not much.

The window is the repair. Instead of stopping the sinc dead, taper it to zero
over its whole length. The ripple falls away and the corner softens, and those
are the two columns.

## The Simplest Kernel, Judged Twice

Set every tap to the same number and the kernel averages the last few samples.
Nothing is simpler, and the verdict depends entirely on which question is asked.

In the time domain it wins outright. Averaging $M$ samples of noise divides the
noise's standard deviation by $\sqrt{M}$, and no other $M$ tap kernel does
better. [`fir_smoothing`](app/Src/fir_smoothing.c) measures it on a 10 Hz sine
buried in noise:

```
  taps  sigma out   measured    sqrt(M)
     3     0.1547       1.79       1.73
     5     0.1158       2.39       2.24
    11     0.0796       3.47       3.32
    21     0.0617       4.48       4.58
```

The standard deviation is the one from the statistics chapter, which could
measure noise but had no way to reduce it. This is the way.

In the frequency domain the same kernel is the worst one in the chapter. Its
stopband ripple settles at -13 dB and stays there no matter how many taps are
added, so a tone it was told to reject comes back at a fifth of its height.
Adding taps moves the first null down and does nothing at all for the rejection.

Both results are correct. The moving average is a smoother, not a separator, and
the reason the rest of this chapter exists is that separating frequencies needs
a different kernel.
