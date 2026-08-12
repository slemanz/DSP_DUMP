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

## Reading the Output

Three features of the output signal are not obvious until you have seen them,
and all three are visible in [`conv_output`](app/Src/conv_output.c).

**It is longer.** $N + M - 1$, for the reason above.

**It is late.** Every output sample is a weighted average of $M$ input samples,
centred half a kernel back, so the output trails the input by

$ \frac{M - 1}{2} $

samples, which is 14 for the 29 tap kernel used here. This is the phase shift
you see when the two are plotted together. It is the same 14 samples at every
frequency only because the taps are symmetric; an asymmetric kernel delays
different frequencies by different amounts, and that is a much worse problem
than a constant delay.

**Its ends are wrong.** For the first and last $M - 1$ samples the kernel hangs
off the end of the input and part of the sum simply does not exist. Those are
the startup and tail transients, and any measurement taken on the output has to
skip them.

## Kernels You Can Read at a Glance

[`conv_kernels`](app/Src/conv_kernels.c) runs one input through four kernels
short enough to read as sentences:

| taps | what comes out |
| --- | --- |
| `{1.0}` | the input, untouched |
| `{2.0}` | twice the input |
| `{0,0,0,0,0,0,0,0,1.0}` | the input, eight samples late |
| `{1.0, 0…0, 0.6}` | the input plus a quieter copy behind it |

The first is the delta function, and it says

$ x[n] * \delta[n] = x[n] $

so the delta function is to convolution what 0 is to addition and 1 is to
multiplication. Make it taller and you have an amplifier, shorter and you have
an attenuator:

$ x[n] * k \, \delta[n] = k \, x[n] $

Move it right and you have a delay:

$ x[n] * \delta[n - s] = x[n - s] $

Moving it left would be an advance, and no real time system can do that, because
it would mean producing output before the input arrives. Use two deltas instead
of one and you have an echo, which is the whole of that effect in one line of
coefficients.

None of this is filtering. The point is narrower and worth more: the kernel is
not a setting the system takes, the kernel is the system.

## Convolution as a Filter

The signal in [`signals.c`](app/Src/signals.c) is a 1 kHz sine with a 15 kHz
sine at half amplitude riding on it, sampled at 48 kHz:

$ x[n] = \sin\!\left(\frac{2\pi \cdot 1000 \, n}{48000}\right) + \tfrac{1}{2}\sin\!\left(\frac{2\pi \cdot 15000 \, n}{48000}\right) $

The kernel in [`conv.c`](app/Src/conv.c) is 29 taps of a sinc weighted by a
Hamming window,

$ h[i] = \mathrm{sinc}\!\left(2 f_c \left(i - \tfrac{M}{2}\right)\right) \left(0.54 - 0.46 \cos\frac{2 \pi i}{M}\right) $

with $f_c = 6000 / 48000$ and $M = 28$, then divided through by its own sum so
the taps total 1. The sinc is the ideal low pass and the window is what lets it
be cut to 29 points without falling apart. The same formula reproduces the
`firCoeffs32` array that ships with the CMSIS-DSP FIR example exactly, so these
are not arbitrary numbers.

Convolve the two and the 15 kHz component comes out at 0.12% of what it was
while the 1 kHz passes untouched. On the board the evidence is one line: the
input peaks at 1.3195, because 1.0 of slow wave plus 0.5 of fast one, and the
output peaks at 1.0020.

That is a finite impulse response filter, and it was built by choosing 29
numbers and running the loop from the previous section. Filter design, in the
end, is the question of which 29 numbers.