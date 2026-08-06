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