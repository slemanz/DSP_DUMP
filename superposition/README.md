# Linear Systems and Superposition

The previous two chapters dealt with signals on their own: what a signal is
worth on average, how much it moves around that average, and what it costs to
turn a continuous one into samples. This chapter is about what happens when a
signal is put through something. That something is a system, and almost every
technique in the rest of digital signal processing works only if the system in
question is linear, so the first job is to be able to tell whether it is. The
runnable examples live under [`app/Src/`](app/Src/), one file per concept, and
are built on the project skeleton described under
[Template](../template/README.md).

## What a System Is

A system is a process that produces an output signal in response to an input
signal. Nothing more is meant by the word here. A filter is a system, an
amplifier is a system, and so is a length of wire with a capacitor hanging off
it.

The notation used from here on separates two things at a glance. Continuous
signals are written with parentheses, $x(t)$ and $y(t)$, and discrete ones with
square brackets, $x[n]$ and $y[n]$, so the brackets alone say the signal has
already been sampled. Lowercase letters are time domain signals and uppercase
letters are their frequency domain counterparts, which is why $x[n]$ becomes
$X[f]$ once the Fourier transform arrives.

The two domains hold the same information in different places. In the time
domain the information is in the shape of the waveform. In the frequency domain
it is in the amplitude and the phase of the sinusoids the signal is made of.
Neither one holds more than the other, and moving between them is the subject of
a later chapter.

## Homogeneity

A linear system has three properties, and the first is homogeneity: a change in
the amplitude of the input produces the same change in the amplitude of the
output. If the system turns $x[n]$ into $y[n]$, then for any constant $k$:

$$ k \, x[n] \longrightarrow k \, y[n] $$

The test that follows from this is to scale the input first and run it through,
then run the original through and scale the result afterwards, and see whether
the two agree. What makes the test worth running carefully is that a nonlinear
system can pass it by accident. An amplifier that saturates at some voltage
behaves perfectly linearly as long as the signal stays below that voltage, and a
squarer passes at $k = 1$ alone, because that is where $k$ and $k^2$ are the
same number. One value of $k$ proves nothing.

## Additivity

The second property is additivity: signals pass through the system without
interacting. If $x_1[n]$ produces $y_1[n]$ and $x_2[n]$ produces $y_2[n]$, then:

$$ x_1[n] + x_2[n] \longrightarrow y_1[n] + y_2[n] $$

The two signals come out the other side as a sum and not as a mixture. Nothing
new appears that was not in one of them already.

What a nonlinear system leaves behind when this fails is not vague. Take a
system that squares its input. The difference between the two routes is:

$$ (x_1 + x_2)^2 - (x_1^2 + x_2^2) = 2 x_1 x_2 $$

which is a product of the two signals, and a product of two sinusoids contains
frequencies that were in neither of them. That is intermodulation, and it is why
a distorting amplifier sounds dirty rather than merely loud. A saturating
amplifier shows the same thing more plainly: two signals can each stay below the
rail on their own and push past it together.

## Shift Invariance

The third property is shift invariance: a shift in the input produces the same
shift in the output and nothing else. For any shift $s$:

$$ x[n - s] \longrightarrow y[n - s] $$

The system does the same thing to a signal regardless of when the signal
arrives.

This is a separate question from the first two, though the three are usually
named together. A system with no memory can be thoroughly nonlinear and still be
shift invariant, because nothing inside it knows what $n$ is. A system that
multiplies its input by a fixed waveform is linear, passes both of the other
tests, and fails this one, because its gain depends on where a sample sits
rather than on what the sample is worth. The four systems the examples test
against make the point in one table:

| System | Homogeneity | Additivity | Shift invariance |
| --- | --- | --- | --- |
| Three tap weighted average | pass | pass | pass |
| Saturating amplifier | fail | fail | pass |
| Squarer | fail | fail | pass |
| Modulator | pass | pass | fail |

A time varying system with a period hides from a careless test. If its gain
repeats every 32 samples, a shift of 32 puts everything back where it was and
the system looks invariant. As with homogeneity, one value proves nothing.

## Superposition

Superposition is the statement that ties the properties to something useful: the
response of a linear system to a sum of signals is the sum of the responses to
each individual signal. It follows from additivity and homogeneity together, and
it is what makes a linear system tractable, because it means a hard input can be
replaced by easy inputs whose answers are already known.

Two operations come out of it. **Synthesis** adds two or more signals to form
one. **Decomposition** is the reverse, breaking one signal into components that
add back up to it. Because the system is linear, decomposing a signal, running
each component through separately and synthesizing the outputs gives exactly the
same result as running the whole signal through in one pass. The route taken is
free, which means the route can be chosen for convenience.