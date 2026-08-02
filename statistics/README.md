# Signal Statistics and Noise

Before a signal can be filtered or transformed it has to be described, and the
description starts with two numbers: where the signal sits, and how much it
moves. Those are the mean and the standard deviation, and almost everything
that follows in DSP leans on them. They are also the cheapest measurements you
can make on a microcontroller, which makes them the natural place to start. The
runnable examples live under [`app/Src/`](app/Src/), one file per concept, and
are built on the project skeleton described under
[Template](../template/README.md).

## What a Signal Is

A signal describes how one parameter relates to another. That is the whole
definition, and it is why signals are drawn on two axes: the plot is the
relationship.

Signals in the real world are continuous by nature. Anything a sensor picks up,
a voltage, a temperature, a sound pressure, exists at every instant. A
microcontroller cannot hold that. Passing a continuous signal through an
analog to digital converter turns it into a discrete, digitized signal, and
from that point on every signal the firmware touches is discrete.

A signal is made of two parameters. The horizontal axis carries the
**independent variable**, which describes how and when a sample was taken, and
is often time. The vertical axis carries the **dependent variable**, which is
the actual measurement, and is a function of the independent one. The same two
go by other names: the dependent variable is also called the amplitude or the
range, and the independent variable is also called the domain.

Each digitized point of the signal is a **sample**, and the total number of
samples is denoted by **N**. All the samples together are the signal.

## The Mean

The mean is the average value of a signal. Add every sample together, divide by
the number of samples, and that is it:

$$ \mu = \dfrac{1}{N}\sum^{N-1}_{i=0} x_i $$

It is denoted by $\mu$, pronounced *mu*. In electronics the mean is the **DC
value** of the signal, and that name is worth taking literally. Add a constant
to every sample and the mean rises by exactly that constant, which is what
`stat_mean.c` shows by lifting the test signal by a known 2.0 and watching the
mean follow. Most real sensors add a DC offset of their own, so the mean is
usually the first thing measured and the first thing subtracted.

## The Standard Deviation and the Variance

The mean says nothing about how far the signal wanders from it. That is the
standard deviation: a measure of how much the signal fluctuates around its
mean. It is denoted by $\sigma$, pronounced *sigma*. Subtract the mean from
each sample, square the difference, add them all up, divide, and take the
square root:

$$ \sigma = \sqrt{\dfrac{1}{N-1} \sum^{N-1}_{i=0}(x_i - \mu)^2} $$

The power of that fluctuation is the **variance**, which is simply the same
expression without the square root:

$$ \sigma^2 = \dfrac{1}{N-1}\sum^{N-1}_{i=0} (x_i - \mu)^2 $$

Squaring is what makes the sum meaningful. Without it the deviations above and
below the mean would cancel and the total would come out near zero no matter
how violently the signal moved.

The two carry different units and are used for different things. The variance
is in units squared, which is why it reads as power, and it is what adds when
independent signals are combined. The standard deviation is back in the units
of the signal itself, which is why it is the one quoted in a datasheet and the
one compared against an amplitude.

Two properties fall out of the formula and both are worth checking on the
board. Adding a DC offset does not change either one, because every sample and
the mean move together and the distances between them are untouched, which is
what `stat_variance.c` demonstrates. And for a zero mean sine of amplitude A
the standard deviation settles at $A/\sqrt{2}$, the same 0.707 factor that
turns a peak amplitude into an RMS value: for a zero mean signal, the standard
deviation *is* the RMS value. `stat_std.c` measures the 5 Hz unit sine and
gets 0.707109 against the 0.707107 the identity predicts.

### Dividing by N-1

The divisor is $N-1$, not $N$. The samples are a finite sample of a signal
rather than the whole of it, and the mean subtracted from them was itself
estimated from those same samples, which pulls the sum of squares slightly low.
Dividing by $N-1$ corrects for it. The difference is negligible for a few
hundred samples and matters for a handful, but the choice has to be made
consistently, and CMSIS-DSP makes the same one: `arm_var_f32` divides by
`blockSize - 1`.

## Computing Them on a Cortex-M4

The Cortex-M4F has a single precision FPU, and the whole point of this module
is that the arithmetic runs in hardware. Three details decide whether it
actually does.

Use `sqrtf`, not `sqrt`. The FPU has a single precision square root
instruction, `VSQRT.F32`, that finishes in a few dozen cycles. `sqrt` takes and
returns a `double`, which the M4 has no hardware for, so the compiler converts
up, calls a software routine in the libc, and converts back down.

Square by multiplying, not with `powf`. `powf(x, 2)` is a library call that
goes through a logarithm and an exponential; `x * x` is one FPU instruction
with the same result.

And keep the constants single precision. A bare `2.0` is a `double` literal and
drags the expression around it up to double with it. Write `2.0f`.

### Two Passes or One

Both formulas need the mean before they can measure the spread, which means two
passes over the samples and a buffer to hold them. Expanding the square gives a
form that needs only a running sum and a running sum of squares:

$$ \sigma^2 = \dfrac{1}{N-1}\left(\sum^{N-1}_{i=0} x_i^2 - \dfrac{\left(\sum^{N-1}_{i=0} x_i\right)^2}{N}\right) $$

That version can measure a live stream one sample at a time with no buffer at
all, and it is the form CMSIS-DSP documents in the header of `arm_std_f32`.

The price is precision. Both terms grow with the DC level while their
difference stays the size of the fluctuation, so the answer ends up built out
of the last few bits of two much larger numbers. `stat_running.c` puts the two
side by side on the same signal: they agree at a DC offset of 2.0, and at an
offset of 1000.0 the one pass version reports 0.90 against the 0.62 the two
pass version gets, an error of nearly 50%. A `float` carries about seven
significant digits, and at the magnitude those sums reach, the smallest
representable step is already larger than the answer being looked for. The
defense, when the one pass form is the one that fits, is to remove the DC
before measuring the fluctuation.

## Using CMSIS-DSP

The library covers all three, and the statistics functions share one shape:
a pointer to the input, the number of samples, and a pointer to where the
result goes. They return nothing.

```c
float32_t mean, variance, std;

arm_mean_f32(signal, SIG_LEN, &mean);
arm_var_f32(signal, SIG_LEN, &variance);
arm_std_f32(signal, SIG_LEN, &std);
```

`arm_std_f32` is not a separate implementation: it calls `arm_var_f32` and
takes the square root of the result. `stat_cmsis.c` runs the hand written
versions and the library versions on the same signal and prints the difference
between them, which lands at zero or in the last digit the float can carry.
A last digit that disagrees is not a bug. The library does not add the samples
in the same order, and in floating point a different order of additions gives a
different final bit.

The same header carries `arm_max_f32` and `arm_min_f32`, which return the value
and its index, and `arm_max_no_idx_f32` for when the index is not wanted.

## Noise

Everything above is a description of a signal, but point it at a sensor sitting
perfectly still and it becomes a measurement of noise. The mean is the DC level
being read and the standard deviation is the noise riding on it, which is
exactly how a converter's noise is specified. `stat_noise.c` samples the
potentiometer on `PA1` and reports both, along with the peak to peak spread and
the fraction of samples falling within one standard deviation of the mean:

```
mean 2047.34  std  1.87  p-p   11  snr  1095.4  within 1 sigma 176/256
```

Peak to peak is the more intuitive number and the less useful one: it is set by
the two most extreme samples in the buffer, so a single glitch moves it, while
the standard deviation is an average over all of them. That is why datasheets
quote noise as an RMS or standard deviation figure.

The signal to noise ratio in that line is the mean divided by the standard
deviation, the definition that applies when the quantity of interest is a DC
level and everything else is noise. For an AC signal the ratio is defined
between powers instead, and is a different calculation.

The last column is the 68 percent rule. Noise that is normally distributed puts
roughly 68% of its samples within one standard deviation of the mean, 95%
within two, and 99.7% within three. Hold the potentiometer still and the count
should land near 68%. Turn it and the count collapses, because what is being
measured is no longer noise. That rule is also the basis of spike rejection: a
sample three or four deviations from the mean is unlikely enough to be treated
as a glitch rather than as data.

## Apps

Each app is a self-contained `main` that demonstrates one concept. The first
four print once and stop, so `make monitor` before resetting the board is
enough to catch the output. The fifth prints continuously.

1. [Developing the signal mean algorithm](app/Src/stat_mean.c): the mean by
   hand over 320 samples, printed alongside the mean of the same signal lifted
   by a known DC offset, to show the mean is the DC value.
2. [Developing the signal variance](app/Src/stat_variance.c): the variance by
   hand from the mean, and the same signal with a DC offset added, whose
   variance is unchanged.
3. [Developing the signal standard deviation algorithm](app/Src/stat_std.c):
   the square root of the variance with `sqrtf`, checked against $A/\sqrt{2}$
   on a unit amplitude sine.
4. [Computing the statistics using CMSIS-DSP](app/Src/stat_cmsis.c): the hand
   written results and `arm_mean_f32`, `arm_var_f32` and `arm_std_f32` in
   parallel columns, with the difference between them.
5. [Measuring real noise with the ADC](app/Src/stat_noise.c): 256 readings of
   the potentiometer on `PA1` reduced to mean, standard deviation, peak to
   peak, signal to noise ratio and the count within one standard deviation.
6. [One pass mean and variance](app/Src/stat_running.c): the buffer free form
   of the variance next to the two pass form, and the precision it loses when
   the signal carries a large DC offset.