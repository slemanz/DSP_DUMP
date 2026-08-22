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

## Where the Taps Are Made

The kernels are designed in Python and written straight into C by
[`tools/design.py`](tools/design.py). Nothing is copied by hand.

```
python3 tools/design.py            print the design summary
python3 tools/design.py --write    write the four generated files
```

`scipy.signal.firwin` is the windowed design, `firls` is least squares and
`remez` is equiripple, which are the same three choices MATLAB's filter designer
offers in a dropdown. Naming them in a script rather than clicking them in a
dialog is the difference between a filter that can be regenerated and one that
cannot, and [`tools/plot.py`](tools/plot.py) is the dialog's other half: taps,
response and step response for any kernel by name.

One detail is worth stating because it is a common and expensive mistake.
`firwin` takes the cutoff in Hz once `fs` is given. MATLAB's `fir1` takes it as
a fraction of the Nyquist rate, which is $f_s/2$ and not $f_s$, so a cutoff
handed to `fir1` as `fc/fs` lands at half the frequency that was intended. It is
the kind of error a forgiving test signal hides completely, which is why
[`fir_python`](app/Src/fir_python.c) measures where the cutoff actually ended up
instead of trusting the request.

The cheapest check of all is in the summary's `sum` column. The taps of a low
pass add to 1, and the taps of a high pass or a band pass add to 0, because that
sum is the filter's gain at DC. A single mistyped tap shows up there.

## Turning a Filter Around

Negate every tap and add one to the middle one. That is the whole operation, and
it turns a low pass into the high pass that completes it.

It works because negating and adding an impulse subtracts the kernel from that
impulse, and an impulse passes everything unchanged, so what is left passes
exactly what the kernel discarded. The two kernels add back to the impulse, and
that is testable rather than decorative:

```
low + high as kernels, against one impulse: 0.000000000
low + high as outputs, against the input: 0.000000715
```

[`fir_inversion`](app/Src/fir_inversion.c) sends the signal through both halves
and adds the results, and gets the input back, delayed by half the kernel and
otherwise untouched. Nothing was lost and nothing was invented.

The gain table from the same app has one row worth pausing on. At the cutoff
both filters read 0.50 and sum to 1. That is why the design puts the -6 dB point
on the cutoff rather than -3 dB: complementary halves have to add to one, not to
one in power.

## Delay Is Not a Defect

Every kernel here is symmetric, and a symmetric kernel of $M$ taps delays its
output by $(M-1)/2$ samples and does nothing else to the shape. That is linear
phase: every frequency is held up by the same amount of time, so the waveform
arrives late but intact.

The convolution chapter saw this as constant lateness in a plot. Here it is a
design quantity with a price tag, sitting in the same table as the transition
width, because a 201 tap filter at 2 kHz is 50 ms of delay and there are jobs
where that is the number that decides the design.

## Reading the Result

[`fir_separate`](app/Src/fir_separate.c) is the payoff. The test signal is 10 Hz
plus 100 Hz plus 500 Hz at equal amplitude, and 200 samples at 2 kHz holds
exactly 1, 10 and 50 whole periods of them, so nothing leaks and every number
means what it looks like. Three kernels are aimed at it, and the table is nine
gains:

```
kernel          10 Hz    100 Hz    500 Hz
lp_50         1.00087   0.00123   0.00050
bp_50_300     0.00290   1.00205   0.00115
hp_300        0.00000   0.00108   0.99909
```

The diagonal is 1 and everything else is about 0.001, which is 60 dB down. On
the debugger each output is streamed next to the tone it was supposed to
recover, and the two lie on top of each other.

## What the Library Adds

`arm_fir_f32` computes the same thing, and
[`fir_cmsis`](app/Src/fir_cmsis.c) is about the two things that have to be right
before the numbers agree.

The coefficients go in time reversed, `h[N-1]` first. Every kernel in this
chapter is symmetric, so reversing changes nothing and the mistake stays
invisible; the app uses a deliberately lopsided three tap kernel, where an
impulse comes back as `0.20 0.30 0.50` instead of `0.50 0.30 0.20`.

The output is `blockSize` points, not `blockSize + numTaps - 1`. The overhang
does not vanish, it is kept in a state buffer to be prepended to whatever
arrives next, and that is the entire reason the function wants an instance
rather than a pair of arrays. Running the signal through in one call of 200 and
in two calls of 100 gives the same answer to the bit, which is what a filter
that never stops requires and what the next chapter is built on.

## Watching the Signals in Ozone

A filter has a signal side and a kernel side, so the debugger carries both:

| probe | carries |
| --- | --- |
| `g_x` | the signal going in, or the frequency being swept |
| `g_y` | the signal coming out |
| `g_h` | the kernel itself |
| `g_mag` | that same kernel as a frequency response, in dB |
| `g_ref` | whatever the output is being held against |

They are declared in [`probe.c`](app/Src/probe.c) and wired into the Data
Sampling window by [`app.jdebug`](workspaces/app.jdebug), so every app opens the
same way. Every app calls `probe_reset` first, including the ones that never
stream anything, because the linker runs with `--gc-sections` and a probe that
no app in the build mentions is dropped from the image.

`STEP_MS` is 100 rather than a few milliseconds. The sampler cannot keep up at
the faster rate, and what it does instead of complaining is miss points, which
arrives on the graph as a distorted signal and reads as a broken filter.

## Apps

Each app is a self-contained `main` that prints its numbers once and then
streams the probes, so `make monitor` catches the output and the Timeline shows
the signals. Two of them are all table and worth reading on the terminal rather
than in the debugger.

0. [The simplest kernel](app/Src/fir_smoothing.c): a kernel of ones, measured as
   a noise reducer and then as a frequency separator, winning the first outright
   and losing the second. The noise falls by $\sqrt{M}$ as the theory says, and
   the stopband stays at -13 dB no matter how many taps are spent on it.
1. [What the window is for](app/Src/fir_windows.c): the same sinc at the same
   cutoff and the same length, tapered three ways, with the rejection and the
   transition width computed on the chip. Both columns move and they move
   opposite each other.
2. [What the length is for](app/Src/fir_length.c): the same sinc through 31, 101
   and 201 taps, where the transition halves each time and the rejection does
   not budge, alongside the delay and the multiplies that the extra taps cost.
3. [Turning a filter around](app/Src/fir_inversion.c): spectral inversion in one
   line, checked by addition rather than by looking, with the two kernels
   summing to a single impulse exactly and the two outputs summing back to the
   input.
4. [Pulling three tones apart](app/Src/fir_separate.c): a low pass, a band pass
   and a high pass aimed at a signal holding whole numbers of periods of 10, 100
   and 500 Hz, reported as a table of nine gains and drawn as each recovered
   tone lying on top of the original.
5. [Agreeing with the design tool](app/Src/fir_python.c): the same input through
   the same taps on the target and in numpy, compared point by point against a
   reference the design script wrote out, and the cutoff measured rather than
   assumed.
6. [The library filter](app/Src/fir_cmsis.c): `arm_fir_f32` with its coefficients
   the right way round, its shorter output explained, and the same signal handed
   over in one block and in two giving the same answer, which is the state
   buffer earning its place.