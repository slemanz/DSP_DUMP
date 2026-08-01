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