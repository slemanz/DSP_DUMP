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