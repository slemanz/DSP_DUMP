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

## Impulse Decomposition

An impulse is a single nonzero point in a string of zeros. Impulse decomposition
breaks an $N$ sample signal into $N$ components, each of them $N$ samples long,
each holding one point of the original and zeros everywhere else:

$$ x_k[n] = \begin{cases} x[k] & n = k \\ 0 & n \neq k \end{cases} $$

Adding all $N$ components back together returns the original signal, which is
all the decomposition itself claims. What makes it the important one is what the
components have in common: every one of them is the same impulse, scaled by
$x[k]$ and moved to position $k$. So if the system's response to a single unit
impulse is known, and the system is linear and shift invariant, then its
response to every component is known, and superposition supplies the rest:

$$ y[n] = \sum_{k} x[k] \, h[n - k] $$

where $h[n]$ is that impulse response. This sum is the convolution, and it is
the reason a linear shift invariant system is completely described by how it
answers one impulse. CMSIS-DSP provides it as `arm_conv_f32`, and the third app
in this module checks that decomposing by hand and calling the library land on
the same numbers.

## Step Decomposition

Step decomposition breaks the same signal into $N$ components of $N$ samples,
but the components are steps rather than impulses: each one is zero up to a
point and holds a constant from there on. The constant is the difference between
two neighbouring samples:

$$ x_k[n] = \begin{cases} 0 & n < k \\ x[k] - x[k-1] & n \geq k \end{cases} $$

with $x[-1]$ taken as zero. The differences are what make the components add
back up to the original, since summing them telescopes down to $x[n]$. It also
says what this decomposition is about: it characterizes a signal by how much it
changes from sample to sample rather than by where it sits.

Its counterpart to the impulse response is the step response $s[n]$, the output
when a unit step goes in, and the two carry the same information. The step
response is the running sum of the impulse response, and the impulse response is
the first difference of the step response.

## Watching the Signals in Ozone

Every app in this module computes the same result twice, by two different
routes, and the interesting part is the gap between them. All of them therefore
stream four globals, declared in [`probe.h`](app/Inc/probe.h), which Ozone
graphs as waveforms:

| Variable | What it holds |
| --- | --- |
| `g_input` | what goes into the system |
| `g_path_a` | the result by one route |
| `g_path_b` | the result by the other |
| `g_error` | the difference between them |

While the two routes lie on top of each other and `g_error` is a flat line at
zero, the property under test holds. When `g_error` lifts off zero and takes on
a shape, that shape is what the system did wrong, and in the first three apps it
has a name: clipping, a product of two signals, a carrier that failed to follow
its signal.

Ozone's Data Sampling window reads target memory over SWD while the program
runs, using the J-Link high speed sampling interface, and the Timeline window
plots what it collects. Sampling starts on its own when the program resumes and
stops when it halts, so the whole procedure is to build, run `make debug` and
press F5. The window is set up by the project script, in `OnProjectLoad`:

```c
  Edit.SysVar (VAR_HSS_SPEED, FREQ_5_KHZ);
  Window.Show ("Data Sampling");
  Window.Add ("Data Sampling", "g_input");
  Window.Add ("Data Sampling", "g_path_a");
  Window.Add ("Data Sampling", "g_path_b");
  Window.Add ("Data Sampling", "g_error");
  Window.Show ("Timeline");
```

`Window.Add` takes the window name and an expression, which has to evaluate to a
number of eight bytes or less and whose operands have to be static variables.
That constraint is the reason the probes are file scope globals rather than
locals of `main`.

The last two apps use the Timeline for something the serial port cannot show at
all. They add one component to the reconstruction on every pass over the signal,
so `g_path_b` starts at zero and grows toward `g_path_a` while `g_error` shrinks
to nothing, and the signal can be watched being rebuilt one impulse, or one
step, at a time.

## Systems Under Test

The four systems the apps run their tests against are defined together in
[`systems.c`](app/Src/systems.c). Each one takes a whole buffer and keeps no
state between calls, so a test is always a comparison between two pure results
and never depends on what ran before it. A system holding its own history in a
`static` would make every table in this module depend on the order the tests
happened to run in.

## Apps

Each app is a self-contained `main` that prints its table once and then streams
the probes, so `make monitor` catches the numbers and the Timeline shows the
waveforms.

0. [Systems tour](app/Src/systems_tour.c): no property and no comparison, just
   one input sent through one system so the four can be told apart before they
   are used to test anything. Three defines choose the input shape, the system
   and the amplitude; the terminal prints the four systems side by side on the
   first twelve samples and the Timeline carries the shape. An impulse through
   the filter prints its three coefficients, which is the impulse response the
   fourth app is built on.
1. [Homogeneity](app/Src/linear_homogeneity.c): the four systems tested at five
   gains, showing where a nonlinear system passes by accident, then streaming a
   linear and a saturating system in alternation so the flat error and the
   shaped one can be compared in the same trace.
2. [Additivity](app/Src/linear_additivity.c): two sinusoids sent through
   together and separately, with the squarer's residual checked against
   $2 x_1 x_2$ to show that what a nonlinear system adds is a signal and not an
   error.
3. [Shift invariance](app/Src/linear_shift.c): the same four systems at five
   shifts, including the shift equal to the modulator's own period, where a time
   varying system looks invariant.
4. [Impulse decomposition](app/Src/linear_impulse.c): a 25 sample signal broken
   into 25 impulses, run through the system one at a time and synthesized,
   checked against one direct pass and against `arm_conv_f32`.
5. [Step decomposition](app/Src/linear_step.c): the same signal broken into
   steps, with the step response shown to be the running sum of the impulse
   response, and the output rebuilt from the step response alone.