# Convolution

The previous chapter ended by taking a signal apart into impulses, sending each
piece through a system, and adding the pieces back up. It stopped one step short
of saying how. Convolution is that step, and it turns out to be the only step
there is: for a linear, shift invariant system, knowing what it does to a single
impulse is knowing what it does to everything.

That is a strong claim, so it is worth stating plainly. A system that satisfies
the three properties from the last chapter is fully described by 29 numbers, or
3, or however long its response to one impulse happens to be. Not by its code,
not by its circuit. By those numbers. Convolution is how you get from them to
an output.

The examples here build the operation from five samples by hand, watch it fill
in one impulse at a time on the debugger, use it to strip a 15 kHz tone out of a
signal, and then measure what the same arithmetic costs written three ways.

## What Convolution Relates

Convolution takes two signals and produces a third. In this chapter the two
going in are the input signal and the impulse response, and the one coming out
is the output signal:

$ x[n] * h[n] = y[n] $

The asterisk is the operation, not multiplication. If $x$ has $N$ points and $h$
has $M$, then $y$ has

$ N + M - 1 $

points, and that is not a convention. The last sample of the input still gets to
drop a full copy of the impulse response into the output, and that copy runs
$M - 1$ samples past where the input ended. The output is longer because the
system is still ringing after the input stops.

The operation is commutative, associative and distributive. Commutative is the
one that surprises people, because the two signals have such different jobs in
the story: one is the data and the other is the system. The arithmetic does not
know that. `arm_conv_f32` names its arguments `pSrcA` and `pSrcB`, and that
naming is the honest one.

## The Delta Function and the Impulse Response

The delta function, written $\delta[n]$, is a signal that is 1 at sample 0 and 0
everywhere else. It is called the unit impulse.

Feed it to a system and whatever comes out is the impulse response, written
$h[n]$. Two different systems give two different impulse responses, and that is
the whole point: the impulse response is the system's fingerprint.

Any single non-zero sample is a scaled and shifted delta. A signal that is $-3$
at sample 8 and zero elsewhere is

$ -3 \, \delta[n - 8] $

which is worth reading slowly, because the two numbers in it are the two things
convolution does to the impulse response: the 8 is where the copy lands, and the
$-3$ is how tall it is.

The same object goes by different names depending on who is holding it. In
filter design it is the filter kernel or the convolution kernel. In image
processing it is the point spread function.

## Scale, Shift, Add

Here is the entire operation, on five input samples and three taps, printed by
[`conv_by_hand`](app/Src/conv_by_hand.c):

```
        n      0      1      2      3      4      5      6
x[0]=+1.0  1.000  0.500 -0.250      .      .      .      .
x[1]=+2.0      .  2.000  1.000 -0.500      .      .      .
x[2]=+0.0      .      .  0.000  0.000 -0.000      .      .
x[3]=-1.0      .      .      . -1.000 -0.500  0.250      .
x[4]=+0.5      .      .      .      .  0.500  0.250 -0.125
         -------------------------------------------------
        y  1.000  2.500  0.750 -1.500  0.000  0.500 -0.125
```

Each row is one input sample turned into a copy of the kernel, scaled by that
sample's value and laid down starting at that sample's position. The bottom row
is the columns added up. Everything else in this chapter is that table with more
numbers in it.

Three things fall out of the picture without any further argument. The output
runs to $n = 6$ because the last copy starts at 4 and occupies three slots. The
whole thing costs $5 \times 3$ multiply accumulates and there is no hidden work.
And the totals multiply out,

$ \sum y = \left( \sum x \right) \left( \sum h \right) $

because every input sample is spread across the taps and none of it is lost.
That last one is why a kernel whose taps sum to 1 leaves a signal's average
where it found it, and it is the reason the low pass kernel used here is scaled
that way.

## Two Ways to Write the Same Loop

The table can be built along its rows or down its columns, and both appear in
[`conv.c`](app/Src/conv.c).

Along the rows, walk the input and throw copies forward:

```c
for (uint32_t i = 0; i < srcALen; i++)
{
    for (uint32_t j = 0; j < srcBLen; j++)
    {
        pDst[i + j] += pSrcA[i] * pSrcB[j];
    }
}
```

Down the columns, walk the output and reach back for whatever can still get
there:

$ y[i] = \sum_{j=0}^{M-1} h[j] \, x[i - j] $

The first is the picture. The second is the equation, and it is the one that
matters in practice, because each output point is finished when its column is,
which means a real time filter can produce a sample as soon as the input for it
has arrived. The first form has to see the whole input before any output is
final.

They do not agree to the last bit. [`conv_algorithm`](app/Src/conv_algorithm.c)
measures the gap at around $10^{-7}$ on a 32 point signal. Neither loop is
wrong. They add the same products in a different order, and float addition is
not associative, so the order leaves rounding behind. The same thing shows up
between $x * h$ and $h * x$ in [`conv_cmsis`](app/Src/conv_cmsis.c), which is a
good reminder that commutative is a statement about arithmetic, not about
floats.