# Linear Systems and Superposition

A system is anything that takes a signal in and produces a signal out. Most of
them are hopeless to reason about. A small and enormously useful class is not,
and this chapter is about what puts a system in that class and what it buys you
once it is there.

The class is defined by three properties, and they are usually listed as three
separate rules to check. They are better read as one rule stated three times.
Each names an operation you might do to a signal, and each asks the same
question about it: does it matter whether the operation happens before the
system or after it?

| operation | before the system | after the system |
| --- | --- | --- |
| multiply by $k$ | $f(k \, x)$ | $k \, f(x)$ |
| add two signals | $f(x_1 + x_2)$ | $f(x_1) + f(x_2)$ |
| shift in time | $f(x[n-s])$ | $y[n-s]$ |
| break into pieces | $f(\sum x_i)$ | $\sum f(x_i)$ |

Where all four answers are *no*, the system is linear and shift invariant, and
the last row is the one that pays. It says a signal can be taken apart, the
pieces sent through one at a time, and the results added back up. Choose the
pieces well and the whole system collapses into a short list of numbers.

The apps here are built around that table. Every one of them computes the same
result by both routes and puts the two next to each other on the debugger, so a
property holding looks like two traces lying on top of each other and a flat
line underneath.

## What a System Is

A signal describes how one quantity relates to another. A system is a process
that produces an output signal in response to an input signal. That is the whole
definition and it is deliberately wide: an amplifier is a system, a filter is a
system, a length of wire is a system.

Continuous signals are written with parentheses and discrete ones with square
brackets, so $x(t)$ is continuous and $x[n]$ is sampled. Time domain signals get
lowercase letters and frequency domain signals get uppercase, a convention that
matters more once the [Discrete Fourier Transform](../dft) chapter arrives.

Four systems are used throughout, in [`systems.c`](app/Src/systems.c), and they
are chosen so that no two properties can be confused with each other:

| system | what it does | linear | shift invariant |
| --- | --- | --- | --- |
| `fir` | a three tap weighted average | yes | yes |
| `clip` | saturates beyond a limit | no | yes |
| `square` | multiplies each sample by itself | no | yes |
| `modulate` | gain that varies with the sample number | yes | no |

The bottom three exist to be counterexamples. Two of them are not linear and
still pass the shift test, and one is perfectly linear and fails it.

## Homogeneity

Turn the input up and the output goes up by the same factor, and does nothing
else:

$ k \, x[n] \longrightarrow k \, y[n] $

The usual test is whether $f(kx)$ equals $k f(x)$, which answers yes or no and
teaches nothing when the answer is no.
[`linear_homogeneity`](app/Src/linear_homogeneity.c) divides the output back
down by $k$ instead. If the system is homogeneous, dividing undoes multiplying
exactly, so the normalized output is the same curve at every gain:

```
         k =     0.25     0.50     1.00     2.00     4.00
         fir   0.4993   0.4993   0.4993   0.4993   0.4993
        clip   0.5000   0.5000   0.5000   0.3000   0.1500
      square   0.0625   0.1250   0.2500   0.5000   1.0000
    modulate   0.4755   0.4755   0.4755   0.4755   0.4755
```

A flat row is homogeneous. The two that are not flat fail in different
directions, and each direction is the system's own law showing through.

`clip` holds still and then falls away as $0.6/k$, which is its own limit
divided by the gain, so the value of the rail can be read straight off the
table. Below the rail it is exactly linear. That is headroom, and it is why a
nonlinear system tested with a small signal reports back as linear.

`square` doubles every time $k$ doubles, because its output goes as $k^2$ and
only one factor of $k$ is being divided out.

## Additivity

Two signals go through together without noticing each other:

$ x_1[n] + x_2[n] \longrightarrow y_1[n] + y_2[n] $

The lecture describes this as signals passing through *without interaction*, and
[`linear_additivity`](app/Src/linear_additivity.c) measures what the interaction
is when there is one. For the squarer the difference between the two routes is

$ (x_1 + x_2)^2 - \left( x_1^2 + x_2^2 \right) = 2 \, x_1 x_2 $

and that product of two sines is itself two clean waves, at the sum and the
difference of the input frequencies:

$ 2 A_1 A_2 \sin(\omega_1 t) \sin(\omega_2 t) = A_1 A_2 \left[ \cos(\omega_2 - \omega_1)t - \cos(\omega_2 + \omega_1)t \right] $

The app builds that expression from scratch, out of nothing but the two input
frequencies, and shows it landing on what the squarer actually produced. The
inputs hold 5 Hz and 15 Hz. The leftover holds 10 Hz and 20 Hz, which were in
neither of them.

That is the point worth keeping: a nonlinear system does not degrade a signal,
it manufactures new ones at frequencies that were never present. The effect is
called intermodulation and it is most of why nonlinearity matters in practice.

`clip` fails this test for a plainer reason. Neither input reaches its rail
alone, and their sum does, so the two signals interfered with each other by
using up the same headroom.

## Shift Invariance

Delay the input and the output arrives the same, only later:

$ x[n - s] \longrightarrow y[n - s] $

Nothing about what the system does may depend on when it happens. This is the
property most often folded into linearity, and it is not part of it, which
[`linear_shift`](app/Src/linear_shift.c) shows in four lines:

```
         s =     1     5    10    25    32
         fir   yes   yes   yes   yes   yes
        clip   yes   yes   yes   yes   yes
      square   yes   yes   yes   yes   yes
    modulate    no    no    no    no   yes
```

The two middle rows are nonlinear systems passing. The bottom row is a linear
system failing. Linear and shift invariant are independent questions and a
system can have either without the other.

The last column is the one to look at twice. It is 32 samples, exactly the
modulator's own cycle, and at that shift a time varying system hands back a
perfect result. Any system that varies with a period will pass at every multiple
of that period, so testing at a single shift is a way to certify a system that
does not deserve it.

## Superposition

Superposition is the payoff and it follows from the properties above: the
response of a linear system to a sum of signals is the sum of the responses to
each one. Take a signal apart, run the pieces through separately, add the
results, and you get what the whole signal would have given.

Breaking a signal into pieces is decomposition, and adding them back is
synthesis. The part that is easy to miss is that the pieces are yours to choose.
[`linear_superposition`](app/Src/linear_superposition.c) splits the same signal
three completely different ways and all three land on the same output:

```
                 split   pieces    worst gap
           cut in half        2  0.000000000
    every other sample        2  0.000000060
  one sample per piece       25  0.000000030
```

Cutting it in half works. Taking alternate samples works. Twenty five pieces of
one sample each works. Nothing so far says the last one is special, which is
what makes choosing it a decision rather than a rule.

## Impulse Decomposition

An impulse is a single non-zero sample in a run of zeros. Splitting an $N$ point
signal into $N$ pieces of one sample each is impulse decomposition, and it is
the split worth having because every piece has the same shape. Each one is the
same impulse, scaled by a sample value and moved to a position.

So run that one shape through the system once. Whatever comes back is the
impulse response, written $h[n]$, and it is the answer for every piece at once.
For the three tap filter here it is three numbers:

```
  h[n] =  0.50  0.30  0.20
```

Those three numbers are the system. Not the source code, not the coefficients
written inside the function, just its answer to a single impulse.
[`linear_impulse`](app/Src/linear_impulse.c) rebuilds the output from them
without calling the system again:

$ y[n] = \sum_{k} x[k] \, h[n - k] $

Adding up scaled and shifted copies of one response has a name, and the app
checks its result against `arm_conv_f32` to say so. The whole of the
[Convolution](../convolution) chapter arrives here as a consequence of
superposition rather than as a new idea.

## Step Decomposition

The other useful split. Piece $k$ is flat at zero until sample $k$ and then
holds a constant to the end, and the constant is how much the signal changed at
that sample:

$
x_k[n] =
\begin{cases}
0 & n < k \\
x[k] - x[k-1] & n \ge k
\end{cases}
$

Written that way the pieces telescope: the changes cancel in pairs and what is
left at sample $n$ is $x[n]$ exactly.

The step response is what a single unit step becomes, and
[`linear_step`](app/Src/linear_step.c) prints the reason this decomposition
earns its place:

```
    step response s[n]  0.50  0.80  1.00  1.00  1.00
       its differences  0.50  0.30  0.20  0.00  0.00
 impulse response h[n]  0.50  0.30  0.20  0.00  0.00
```

The middle row is the bottom row. The step response is the running sum of the
impulse response, so measuring either one gives the other. That is not a
curiosity, it is the method: producing a real impulse takes a tall narrow pulse
with enough energy to measure, and producing a step means closing a switch. Most
impulse responses are measured as the differences of a step response.

## Watching the Signals in Ozone

Every app computes the same result twice, so the debugger carries both routes:

| probe | carries |
| --- | --- |
| `g_x` | the signal going in |
| `g_before` | the operation done before the system |
| `g_after` | the operation done after the system |
| `g_gap` | the difference between the two |

A property holds where `g_before` and `g_after` lie on top of each other and
`g_gap` is a flat line. Where it fails, the shape of `g_gap` is the lesson: for
the squarer it is a clean periodic wave at frequencies the input never had.

The graphs worth spending time on are the last three. Each streams the output
being rebuilt one piece at a time, so `g_after` starts at zero, gains one piece
per pass, and climbs until it reaches `g_before` while `g_gap` shrinks to
nothing underneath. Superposition is not a claim there, it is something that
happens on screen.

Every app calls `probe_reset` first. The linker runs with `--gc-sections`, so a
probe no app in the build mentions is dropped from the image, and Ozone reports
that it cannot graph the expression rather than drawing an empty trace.

## Apps

Each app is a self-contained `main` that prints its table once and then streams
the probes, so `make monitor` catches the numbers and the Timeline shows the
waveforms.

0. [Systems tour](app/Src/systems_tour.c): no property and no comparison, just
   one input sent through one system so the four can be told apart before they
   are used to test anything. Three defines choose the input shape, the system
   and the amplitude; the terminal prints the four systems side by side on the
   first twelve samples and the Timeline carries the shape.
1. [Homogeneity](app/Src/linear_homogeneity.c): the four systems at five gains,
   with each output divided back down by its own gain so that a flat row means
   homogeneous. The clipper's row falls away as its rail divided by $k$ and the
   squarer's climbs in step with $k$, so each failure states the law behind it
   rather than a distance from zero.
2. [Additivity](app/Src/linear_additivity.c): two sinusoids sent through
   together and separately, with what the squarer left behind built from scratch
   as $A_1 A_2 [\cos(\omega_2-\omega_1)t - \cos(\omega_2+\omega_1)t]$ and shown
   to match, so the leftover reads as a manufactured signal rather than an
   error.
3. [Shift invariance](app/Src/linear_shift.c): the same four systems at five
   shifts as a table of yes and no, including the shift equal to the
   modulator's own period, where a time varying system looks invariant.
4. [Superposition](app/Src/linear_superposition.c): one signal split three
   different ways, cut in half, taken in alternate samples, and one sample per
   piece, with all three rebuilt through the system to the same output, so that
   impulses read as a choice rather than a requirement.
5. [Impulse decomposition](app/Src/linear_impulse.c): a single impulse sent
   through to get $h[n]$, and the whole output rebuilt from those three numbers
   alone, checked against one direct pass and against `arm_conv_f32`.
6. [Step decomposition](app/Src/linear_step.c): the same signal broken into
   steps, with the differences of the step response shown to be the impulse
   response, and the output rebuilt from the step response alone.