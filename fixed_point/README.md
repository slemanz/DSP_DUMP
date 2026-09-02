# Fixed Point and Q Notation

The types are called q7, q15, q31 and q63, and the obvious question is why they
are not q8, q16, q32 and q64. The answer is one bit, and it is the sign bit.

A q15 lives in a signed 16 bit word. One of those bits carries the sign, which
leaves fifteen to put fraction in. The full name is Q1.15, one integer bit and
fifteen fractional ones, and the short name counts only the fractional half
because for these types the integer half is always the sign.

So a q15 is not an integer with a scale factor someone chose. It is a fraction
between $-1$ and just under $+1$, and the integer stored in the word is that
fraction times $2^{15}$.

## What the Format Costs and What It Buys

The reason to give up precision is speed, and the exchange rate on this
particular part is not the one the textbooks assume.

The assumption is that fixed point exists for processors without a floating
point unit. This one has a floating point unit, and a single precision multiply
accumulate on it is a one cycle instruction, so floating point is not the slow
option here. On a Cortex-M0 it would be, by a lot.

What q15 wins is **width**, not arithmetic. Two samples fit in one 32 bit
register and the instruction set has multiply accumulates that take both halves
at once, which is worth roughly a factor of two when the compiler reaches for
them. And half the bytes per sample is half the buffer and half the traffic to
fetch it, which needs no special instruction at all.

What it costs has a shape worth knowing before paying it. Fixed point error is
the same size everywhere, because the step is the same size everywhere. It is a
large fraction of a small sample and a small fraction of a large one, so a
signal that spends most of its time near zero is exactly the case fixed point is
worst at. Floating point is the opposite: it keeps its precision wherever the
number happens to sit.

## Rounding, and the Half Bit the Library Gives Away

Converting a float to a q15 and back is quantisation, the same operation the ADC
performs and the same one the sampling chapter measured. If it rounds to the
nearest code, the error is bounded by half a step and its standard deviation is
$\text{step}/\sqrt{12}$.

`arm_float_to_q15` does not round. It is a cast, and a cast truncates toward
zero, so the error runs to a full step and the spread comes out at
$\text{step}/\sqrt{3}$, which is exactly twice as large:

```
                       worst, steps        sigma vs step/sqrt12
arm_float_to_q15             0.966    1.679e-05        1.906
the same, rounded            0.498    8.850e-06        1.005
```

Half a bit of resolution, given away by a missing 0.5. It is not a bug, it is
one instruction cheaper and it is documented, but it is worth knowing because
the fix is one line and because half a bit is a quarter of what dropping from
q31 to q15 costs in the first place.

There is a companion trap in the same app. A round trip through q31 comes back
apparently perfect, and that is not a statement about q31: a float32 carries 24
bits of mantissa and a q31 carries 31, so the source ran out of resolution
before the destination did. q31 is finer than the float it came from.

## The End of the Range

A q15 stops just short of $+1$. Asking for more than that gets one of two
completely different answers depending on which instruction was used, and
[`q_saturate`](app/Src/q_saturate.c) puts them side by side:

```
        a         b       wanted      wrapped    saturated
    0.500     0.500     1.000000    -1.000000     0.999969
    0.900     0.900     1.800000    -0.200012     0.999969
    0.990     0.990     1.980000    -0.020020     0.999969
```

The wrapped column is worth staring at. The answer to $0.9 + 0.9$ is not
approximately 1.8 and it is not clipped at 1.0. It is the wrong sign at a fifth
of the amplitude, and nothing is flagged.

Worse, look at the pattern going down. The closer the sum gets to needing
saturation, the **smaller** the wrapped result. On a filter output that does not
look like overflow, it looks like the signal disappearing.

Saturating is still wrong, but it is wrong in the direction the signal was
already going, and a clipped peak is something a person recognises. That is why
the instruction set has `QADD` and `SSAT` and why every `arm_*_q15` routine
saturates rather than wraps.

## Multiplying Needs a Shift

Adding two q15 numbers is an integer add and the result is already a q15,
because both operands carry the same scale and so does the sum. Multiplying is
not:

$(a \cdot 2^{15})(b \cdot 2^{15}) = ab \cdot 2^{30}$

The product is a q30 in a 32 bit word, and getting a q15 back means shifting
right by 15. That shift is where the second half of the precision goes: the
product had 30 fractional bits and 15 were discarded to fit the answer back into
the format.

[`q_multiply`](app/Src/q_multiply.c) has one row that says the rest. In q15,
$0.001 \times 0.001$ is 0. Not inaccurate, gone: the true answer is a millionth
and a q15 step is thirty millionths.

## Where It Actually Breaks

A filter multiplies and then adds, and the adding is the dangerous half. Every
product fits; the running total need not. The worst the sum can reach is

$\sum_i |h[i]| \times \max|x|$

and that quantity is not what most people expect it to be.
[`q_headroom`](app/Src/q_headroom.c) computes it for the low pass this chapter
uses and for the high pass the filter chapter made by inverting it:

```
16 tap low pass          sum |taps|   1.000000
inverted into high pass  sum |taps|   1.720030
```

Those two kernels add back to a single impulse exactly, which the filter chapter
demonstrated. They do not have the same headroom at all. The low pass has taps
that are all positive and add to one, so the sum and the sum of magnitudes
coincide. The high pass has taps that cancel to zero, so its magnitudes add to
1.72, and a full scale input overflows it.

The repair is to accumulate in something wider than the samples and saturate
once at the end rather than at every step, which is what `arm_fir_q15` does with
a q63 accumulator. The alternative is to scale the input down by the headroom
first and pay for the safety in resolution. Either way it is a decision that has
to be made, which is why a fixed point routine is not a floating point routine
with a narrower type.

## Choosing

[`q_filter`](app/Src/q_filter.c) runs the same filter in all three formats and
reports both halves of the exchange rate: the error each format left behind,
and what it cost in cycles.

```
format      worst error          sigma     in steps       snr dB
q31           1.192e-07      2.776e-08       256.00        150.2
q15           1.548e-04      8.221e-05         5.07         80.8
```

The signal to noise column is the one that decides. A 12 bit converter delivers
about 74 dB, so a q15 path at 80 is already better than the signal arriving on
it, and a q31 path at 150 is carrying seventy decibels of nothing.

The q31 row's step count needs the same care as the round trip did. 256 steps is
not q31 failing; it is the f32 reference being coarser than a q31 step. The
column is measuring the reference.

The second table in the same app times the three formats, and it answers the
question this chapter opened with: whether the exchange rate is the one people
expect. It is not. This part has a floating point unit, and a single precision
multiply accumulate on it is a one cycle instruction, so the f32 row is not the
slow one. Where q15 wins is width, not arithmetic: two samples per register and
half the bytes to move, worth roughly a factor of two when the compiler finds
the instructions for it, which is the subject of the next module.

## Watching the Signals in Ozone

The chapter is one signal carried in three formats, so the graph shows all three
on one axis with the gap underneath:

| probe | carries |
| --- | --- |
| `g_f32` | the answer in floating point, taken as the truth |
| `g_q31` | the same answer through q31 |
| `g_q15` | the same answer through q15 |
| `g_err` | what the narrow format left behind |

Everything is streamed as a float, including the fixed point traces, because
what is being compared is the value each format managed to hold rather than the
integer it held it in.

`g_err` is the trace with the lesson in it. Its size does not follow the signal,
it stays the same through the peaks and the quiet parts alike, which is fixed
point's defining property drawn rather than argued.

`STEP_MS` is 100 rather than a few milliseconds, and it matters more here than
elsewhere: a point the sampler missed looks exactly like a quantisation error,
which is the last confusion this chapter needs.

## Apps

Each app is a self-contained `main` that prints its numbers once and then
streams the probes, so `make monitor` catches the output and the Timeline shows
the traces. Everything except the timing runs without the board.

0. [What a Q number is](app/Src/q_format.c): the three widths side by side with
   their step and their range, the sign bit that makes a 16 bit word a q15, and
   why the range is one step short on the positive side.
1. [The round trip](app/Src/q_convert.c): a signal converted and brought back,
   showing that `arm_float_to_q15` truncates rather than rounds and that this
   doubles the error spread, and that a q31 round trip measures the float it
   started from rather than q31.
2. [Wrap against saturate](app/Src/q_saturate.c): sums that do not fit, done
   both ways, where the wrapped answer to $0.9 + 0.9$ is $-0.2$ and gets smaller
   the worse the overflow gets.
3. [The shift](app/Src/q_multiply.c): two q15 operands making a q30 product and
   the fifteen bits discarded to get a q15 back, with a row where the answer
   rounds to nothing at all.
4. [Headroom](app/Src/q_headroom.c): the sum of tap magnitudes as the number
   that decides whether a filter overflows, and two kernels that add to a single
   impulse needing 1.00 and 1.72 of range.
5. [Three formats, the error and the cost](app/Src/q_filter.c): the same filter
   in f32, q31 and q15 against the floating point answer, reported as steps and
   as signal to noise, then the same three timed, where the floating point row
   is not the slow one because this part has an FPU, and what q15 wins is width
   and memory rather than arithmetic.

