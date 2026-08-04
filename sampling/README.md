# Quantization and the Sampling Theorem

Everything in the previous chapter assumed the samples were already there. This
one is about where they come from, and about the two ways a converter can throw
information away before the firmware ever sees it. One of them costs a little
accuracy and is easy to budget for. The other destroys the signal outright and
cannot be undone afterwards by any amount of processing. The runnable examples
live under [`app/Src/`](app/Src/), one file per concept, and are built on the
project skeleton described under [Template](../template/README.md).

## Inside the Converter

A signal in the real world is continuous in both directions: it exists at every
instant, and at every instant it can hold any value. A converter has to
discretize both, and it does so in two separate stages.

The **sample and hold** stage discretizes the horizontal axis. It captures the
signal's value at one instant and holds it steady while the rest of the
converter works, so the independent variable stops being continuous time and
becomes a sample index.

The **quantizer** discretizes the vertical axis. It takes the held value, which
is still one of infinitely many possible voltages, and maps it onto one of a
finite set of codes.

Both stages lose something. The rest of this chapter is about what.

## Quantization

Quantization is the process of mapping continuous, infinite values onto a
smaller set of discrete ones. A converter with $N$ bits has $2^N$ codes to
spend across its full scale range, and the voltage those codes are spaced by is
the **LSB**:

$$ q = \dfrac{V_{FS}}{2^N} $$

For the 12 bit ADC on the STM32F411 running against a 3.3 V reference that
comes to 0.806 mV. Any input between two codes is reported as the nearer of the
two, so the reading carries an error of at most half an LSB, and the finite
resolution of the converter is the floor on how precise any measurement built on
it can be.

### How Much the Quantizer Loses

That error is a signal in its own right, and the tools from the previous chapter
describe it. Sweep a smooth input across the range and the error is a sawtooth
running between $-q/2$ and $+q/2$. If the signal covers many codes the error
spends equal time everywhere in that band, which makes it uniformly distributed,
and the standard deviation of a uniform distribution of width $q$ is:

$$ \sigma = \dfrac{q}{\sqrt{12}} \approx 0.29 q $$

This is the usual way quantization noise is quoted, and `sampling_quantize.c`
checks it by measuring the error with `arm_std_f32` at six different bit depths.
At 12, 10 and 8 bits the measurement tracks the formula within a few percent. At
4 and 2 bits it does not: the measured deviation comes out 0.046 against the
0.036 predicted, and 0.205 against 0.144. The formula was never wrong, its
assumption was. With only four codes the signal sits pinned to one of them for
long stretches, the error stops being uniformly spread, and the model that
produced $q/\sqrt{12}$ no longer describes it.

## Proper Sampling and the Sampling Theorem

Proper sampling is defined by what can be done afterwards: sampling is proper if
the original analog signal can be reconstructed exactly from the samples. Not
approximately, and not well enough to look right on a plot. Exactly.

The condition for it is the **sampling theorem**, also called the Nyquist
theorem: a continuous signal can be properly sampled only if it contains no
frequency component above half the sampling rate. Written with the sampling
frequency $f_s$ and the highest frequency present in the signal $f_{max}$:

$$ f_s \geq 2 f_{max} $$

Sampling at 50 Hz means the signal reaching the converter must be built entirely
from components at 25 Hz and below. Note which side of the inequality the signal
is on. The theorem is a constraint on the input, not a promise about the
sampling rate, and the practical consequence is that the input has to be
restricted before it arrives, not after.

`sampling_theorem.c` runs one 5 Hz sine through four rates and holds each sample
until the next, so the sample and hold output can be watched next to the signal
that produced it:

| Sampling rate | Samples per period | What survives |
| --- | --- | --- |
| 50 Hz | 10 | a staircase that follows the sine closely |
| 20 Hz | 4 | coarse, but still recognizably a sine |
| 10 Hz | 2 | nothing, a flat line at zero |
| 5 Hz | 1 | a constant, the sine has become DC |

### Sampling Exactly at Twice the Highest Frequency

The 10 Hz row is the interesting one, because 10 Hz is exactly twice the 5 Hz
signal and the theorem as usually stated says that is enough. What comes out is
a flat line. Every sample lands on a zero crossing, and the signal disappears
completely.

Shift the input by 90 degrees and the same rate captures +1, -1, +1, -1 at full
amplitude. So at exactly $2 f_{max}$ the result depends on the phase, which
means the reconstruction is not guaranteed and the sampling is not proper. The
condition that is actually safe is the strict one, $f_s > 2 f_{max}$, and real
systems leave considerably more margin than that so the anti aliasing filter has

## Aliasing

When the theorem is violated the signal is not merely degraded, it is
impersonated. **Aliasing** is characterized by different signals becoming
indistinguishable: a component above half the sampling rate folds back below it
and arrives disguised as a frequency that was never there. The frequency it
folds to is:

$$ f_{alias} = |f - k f_s| $$

for whichever integer $k$ brings the result below $f_s/2$. Sampled at 20 Hz, an
11 Hz input reads back as 9 Hz, a 15 Hz input reads back as 5 Hz, and a 25 Hz
input reads back as 5 Hz as well.

`sampling_alias.c` prints that folding table and then makes the point that
matters: it samples a 15 Hz cosine and a 5 Hz cosine at the same instants and
compares the two sets of numbers. The largest difference across forty samples is
on the order of $10^{-5}$, which is the rounding error of `cosf` and nothing
else. The two are not similar, they are the same data. No filter, no transform
and no amount of processing applied after the converter can separate them,
because there is nothing left to separate.

## The Signal Chain

Which is why a system that respects the theorem puts the work before the
converter. The full chain runs:

```
analog in -> anti aliasing filter -> ADC -> DSP -> DAC -> reconstruction filter -> analog out
```

The **anti aliasing filter** is an analog filter, built from real components,
sitting between the signal and the ADC pin. Its job is to remove everything
above half the sampling rate while the removal is still possible. The sampling
rate itself is set in firmware, so the two have to be designed together.

After the converter the samples are in memory and the processing happens. If an
analog output is wanted, a **DAC** turns the processed samples back into a
voltage and a second analog filter smooths it, removing the components above
half the sampling rate that the conversion reintroduces. That one is called the
**reconstruction filter**. It does the same job as the first, at the other end.

In practice most embedded work stops at the DSP stage. The filter, the converter
and the processing are the parts that always appear.

`sampling_antialias.c` demonstrates the first filter's effect, with one honest
caveat stated up front: it runs the filter in software over an oversampled
model, ahead of the point where the samples are taken. That is the position a
real RC network occupies in the circuit. Filtering in software after sampling
would accomplish nothing at all.
somewhere to roll off.

## Analog Filters

Analog filters divide into passive and active. Passive filters are built from
passive components alone, resistors, capacitors and inductors. Active filters
add an active component, usually an operational amplifier, which buys the
ability to apply gain. That matters because filtering attenuates, and a signal
that comes out of a filter too small to use has to be amplified before the next
stage.

### The Passive Low Pass

The simplest useful filter is a resistor in series with the signal and a
capacitor from the output to ground, which is why these are called RC filters.
It passes low frequencies and blocks high ones.

The mechanism is in the capacitor's behavior against frequency. At high
frequency the capacitor behaves as a short circuit, which ties the output to
ground and leaves the converter with nothing. At low frequency the capacitor
behaves as an open circuit, which is as if it were not there, leaving a plain
wire from the source to the output. High frequencies are shunted away, low
frequencies pass.

The range of frequencies that pass with no significant attenuation is the **pass
band**, the range that is attenuated is the **stop band**, and the frequency
where one gives way to the other is the **cutoff frequency**:

$$ f_c = \dfrac{1}{2 \pi R C} $$

Cutoff is defined as the point where the amplitude has dropped by 3 dB, which is
a fall to $1/\sqrt{2}$ of the input, about 71%. In power rather than amplitude
that is a fall to one half, which is where the definition comes from.

`sampling_antialias.c` prints the filter's response across six frequencies with
the ideal and the measured value side by side, and the measured column lands
within a fraction of a dB of the ideal at cutoff. Two octaves above cutoff it
has fallen to a third of the input, which is the attenuation that keeps the
15 Hz component from folding onto the 5 Hz data.

### The Passive High Pass

Swap the resistor and the capacitor and the filter inverts: low frequencies are
blocked, high frequencies pass. The mechanism inverts with it, and the cutoff
frequency is given by the same expression.

### Ideal Against Practical

An ideal filter would pass everything below cutoff untouched and everything
above it not at all, dropping to zero the instant cutoff is crossed. Drawn on a
plot it is a rectangle, which is why it is called a **brick wall** filter.

Real filters do not do this. Between the pass band and the stop band there is a
**transition band**, a range over which the amplitude falls gradually rather
than instantly. How wide it is depends on the complexity of the filter, and the
rate of fall through it is the **roll-off**. All of practical filter design is
about how much circuitry to spend narrowing that transition.

### Chebyshev, Butterworth and Bessel

The RC networks above are the teaching case. Real designs use one of three
configurations, each named after whoever worked it out and each optimizing a
different parameter. All three are built by cascading a standard active stage,
the **Sallen-Key** filter, whose component ratios decide which of the three the
result becomes.

Complexity is counted in **poles**, and more poles means more components, a
narrower transition band and a faster roll-off. There is no configuration that
is best at everything, and the reason is the trade between two performance
measures. The **frequency response** says how sharply the filter separates the
bands. The **step response** says how the filter behaves when its input jumps
from one value to another, which is what matters when the signal carries edges
rather than tones.

| Configuration | Frequency response | Step response |
| --- | --- | --- |
| Chebyshev | sharpest roll-off, at the cost of ripple in the pass band | worst, heavy overshoot and slowly decaying ringing |
| Butterworth | flattest possible pass band, no ripple, roll-off slower than Chebyshev | overshoot and ringing, less than Chebyshev |
| Bessel | worst roll-off, transition is gradual | best, no overshoot and no ringing at all |

The pattern is that a sharp cut in frequency is paid for in time. Chebyshev buys
its roll-off with ripple and ringing; Bessel gives up the roll-off entirely and
gets a clean step in return; Butterworth sits between them. Which one belongs in
front of a converter depends on whether the signal is being measured for its
spectrum or for its shape.

## Watching the Signals in Ozone

Sampling is a subject about shapes, and a serial console prints numbers. Every
app in this module therefore streams three globals, declared in
[`probe.h`](app/Inc/probe.h), which Ozone graphs as waveforms:

| Variable | What it holds |
| --- | --- |
| `g_analog` | what goes into the converter |
| `g_sampled` | what comes out of the sample and hold |
| `g_error` | what the conversion lost |

Ozone's Data Sampling window reads target memory over SWD while the program
runs, using the J-Link high speed sampling interface, and the Timeline window
plots what it collects. Sampling starts on its own when the program resumes and
stops when it halts, so the whole procedure is to build, run `make debug` and
press F5.

The window is set up by the project script, in `OnProjectLoad`:

```c
  Window.Show ("Data Sampling");
  Edit.SysVar (VAR_HSS_SPEED, 1000);
  Window.Add ("Data Sampling", "g_analog");
  Window.Add ("Data Sampling", "g_sampled");
  Window.Add ("Data Sampling", "g_error");
  Window.Show ("Timeline");
```

`VAR_HSS_SPEED` is Ozone's own sampling frequency in Hz, and 1 kHz is twice the
rate at which the apps update the probes, so no point of the waveform is missed.
`Window.Add` takes the window name and an expression, which has to evaluate to a
number of eight bytes or less and whose operands have to be static variables.
That constraint is the reason the probes are file scope globals rather than
locals of `main`.

The three modeled examples generate their continuous signal by oversampling: the
waveform is computed at 500 points per second and the sampling under study
happens at 50 Hz or below, so the 500 Hz model stands in for the continuous
signal. `sampling_adc.c` is the one that does not model anything, since there
the conversion is real.

## Apps

Each app is a self-contained `main` that demonstrates one concept, and each one
is worth watching in the Timeline rather than only on the serial port.

1. [Quantization](app/Src/sampling_quantize.c): a sine quantized at six bit
   depths, with the error's standard deviation measured against $q/\sqrt{12}$,
   then streamed at 4 bits so the staircase and its sawtooth error can be seen.
2. [The sampling theorem](app/Src/sampling_theorem.c): one 5 Hz sine sampled at
   four rates in rotation, from ten times the signal down to one times it,
   including the case of exactly twice where the signal vanishes.
3. [Aliasing](app/Src/sampling_alias.c): the folding table for a 20 Hz sampling
   rate, and a demonstration that a 15 Hz cosine and a 5 Hz cosine sampled at
   that rate produce identical numbers.
4. [Quantization on the real ADC](app/Src/sampling_adc.c): the potentiometer on
   `PA1` read through the 12 bit converter and requantized to 6, so the ramp and
   the staircase can be turned by hand.
5. [The anti aliasing filter](app/Src/sampling_antialias.c): a one pole low pass
   ahead of the sampler, its response printed against the ideal RC response, and
   the aliasing it prevents.