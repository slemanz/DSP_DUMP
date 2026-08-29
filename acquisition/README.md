# Analog Input and the Sampling Clock

Every chapter up to here worked on signals that were already in memory, and the
sampling rate was a constant in a header that nobody questioned. The DFT
reported bin $k$ as $k f_s / N$. The filter chapter designed a cutoff in Hz.
Both of those mean something only if $f_s$ is true.

This is where it becomes true, and the useful way to say it is:

**a converter with no timer still has a sampling rate. It is just not a rate
anybody chose.**

In continuous mode the ADC finishes one conversion and starts the next
immediately. That is a perfectly definite $f_s$, and it fell out of the bus
clock, the prescaler and the sample time rather than out of a decision. Get it
wrong by 12% and every frequency computed downstream is wrong by 12%, and
nothing in the numbers will say so.

## Where the Rate Comes From

Three things set it. PCLK2 feeds a prescaler to make ADCCLK, the input is held
for a chosen number of ADCCLK cycles, and then the successive approximation runs
at one cycle per bit:

$ t_{conv} = \frac{n_{sample} + n_{bits}}{f_{ADCCLK}} $

The prescaler has a floor that is easy to miss. The converter is rated for
36 MHz regardless of what the bus does, and PCLK2 is 100 MHz here:

| prescaler | ADCCLK | |
| --- | --- | --- |
| /2 | 50 MHz | over the rating |
| /4 | 25 MHz | ok |
| /6 | 16.7 MHz | ok |
| /8 | 12.5 MHz | ok |

Divide by 2 hands the converter 50 MHz and it does not refuse. It converts, and
the numbers come out wrong in a way that looks like noise. The reset value is
divide by 2, which was harmless when the core ran at 16 MHz and is not now.

Sample time is not a free choice either. It is how long the input has to charge
the sampling capacitor through whatever is driving it, so a signal behind a 10k
potentiometer needs longer than one from an op amp. Too short does not fail, it
reads low.

## Handing the Decision to a Timer

[`adc_timer`](app/Src/adc_timer.c) puts a timer in charge. It rolls over at a
chosen rate, its interrupt fires, and the handler takes one sample. From here
$f_s$ is a decision.

The whole app is really about which clock the period is computed from, because
of the doubler from the previous chapter. PCLK1 is 50 MHz and TIM2 counts at
100, so:

```
ARR from PCLK1        24999   ->  4000 Hz
ARR from TIM2 clock   49999   ->  2000 Hz
```

Exactly double, with nothing about it that looks wrong. `timer_periodic_init`
calls `clock_timer_pclk1` for this reason and returns the rate it actually
achieved rather than the one it was asked for, because the divisor has to be a
whole number and not every rate has one.

## Average Rate and Individual Gaps

An interrupt driven sample arrives when the core gets round to it, so the
average rate is exact and the individual gaps are not. That difference has a
name and a cost.

Jitter is not a rounding error in a spectrum, it is noise. A sample taken
slightly late is a sample of the wrong part of the waveform, and the steeper the
signal the bigger the amplitude error that lateness turns into, which is to say
it gets worse with frequency.

[`adc_trigger`](app/Src/adc_trigger.c) removes it by taking software out of the
timing path. The timer's rollover goes out on TRGO, the ADC starts on that edge,
and the two are wired together inside the chip. The core is then free to be late
reading the result, which costs nothing, rather than late taking the sample,
which costs accuracy.

What the core can still get wrong is not reading fast enough, and that has a
flag. `OVR` says a conversion finished before the previous result was collected.
The samples in that gap are gone either way; the difference is that this way the
gap is on the record.

## Buying Bits That Are Not There

The converter divides its reference into 4096 steps, so one count is about
806 µV and no amount of care gets a finer reading out of one conversion.

Out of many it does. Averaging $N$ samples of a steady voltage divides the noise
by $\sqrt{N}$, which is the same statement the moving average made about a
signal and the same $\sqrt{N}$ the statistics chapter measured. Four times the
samples is one more bit, so $4^n$ samples buys $n$ bits, and the price is that
the effective rate falls by the same factor.

There is a catch that sounds like a joke and is not: this only works if there is
noise. A perfectly clean DC voltage sitting inside one code averages to that
code forever, and no averaging finds the part below it. The noise is what
carries the information about where inside the code the signal really is, so a
little of it is a requirement rather than a nuisance.
[`adc_resolution`](app/Src/adc_resolution.c) measures the noise first and says
so when there is too little of it for the rest of the table to mean anything.

## The Theorem, on a Signal That Exists

The sampling chapter argued this on numbers already in memory, where the signal
was whatever the array said. [`adc_alias`](app/Src/adc_alias.c) argues it on a
square wave the chip generates on PB4 and reads back through PA1, which is the
one experiment in this repository that needs a jumper wire.

TIM3 makes the wave, TIM2 sets the sampling rate, and the rate walks down past
twice the tone and keeps going:

$ f_{alias} = \left| f - \mathrm{round}\!\left(\frac{f}{f_s}\right) f_s \right| $

Above $2f$ the shape on the graph is a square wave. Below it the shape is still
a wave, a perfectly clean one, at a frequency the signal does not contain and
never did. Nothing broke when that happened and no measurement can undo it: the
samples are correct samples of a waveform that passes through all of them, and
so does the original, and once they are taken there is nothing left to say which
it was. That is why an anti aliasing filter goes before a converter rather than
after it.

Two honest notes about that app. The row where $f_s$ is exactly twice the tone
is the theorem's boundary rather than a working rate, because sampling a wave
exactly twice per period can land on the zero crossings every time and report
nothing at all. And a square wave has harmonics: the third one sits at 1500 Hz
and folds back at every rate in the table, including the ones marked as showing
the real frequency. That is not a flaw in the demonstration, it is the same
argument arriving twice.

## The Budget for the Next Chapter

Everything above takes samples and looks at them one at a time. Real work does
not. Convolution wants an array, the DFT wants a window, and every CMSIS-DSP
function takes a block and a length.

[`adc_stream`](app/Src/adc_stream.c) runs the converter from the timer, does
nothing in the handler but store the result, and then reports how much of each
sample period that handler used. What is left over is the entire budget for
doing anything with the sample, and it is not much.

That measurement is the argument for the next chapter. If storing a sample
already uses most of the period, the filtering cannot happen there, and it has
to happen somewhere else on a block that has already been collected.

## Watching the Signals in Ozone

The signal in this chapter comes from outside the chip, so the graph carries it
raw and converted, next to the two things that decide whether it means anything:

| probe | carries |
| --- | --- |
| `g_raw` | the converter's output in counts |
| `g_volts` | the same sample in volts |
| `g_rate` | the sampling rate the app is running at |
| `g_jitter` | whatever the app is measuring about steadiness |

One thing is worth being clear about, because it is easy to misread. **The graph
is not showing the sampling rate.** The samples are taken at whatever rate the
timer sets, stored, and then paraded past the probe one per `STEP_MS`. The shape
is right and the time axis belongs to the debugger.

`STEP_MS` is 100 rather than a few milliseconds for the usual reason: the
sampler misses points at the faster rate, and a trace with holes in it reads as
an acquisition fault that is not there.

## Apps

Each app is a self-contained `main` that prints its numbers once and then
streams the probes, so `make monitor` catches the output and the Timeline shows
the traces. Everything here measures a peripheral or a pin, so unlike the
earlier chapters none of it can be checked on a host.

0. [The rate nobody chose](app/Src/adc_free.c): the converter free running, with
   the rate counted against SysTick and printed beside the rate the datasheet
   arithmetic predicts, followed by what the pin actually reads.
1. [Where the rate comes from](app/Src/adc_timing.c): prescaler, sample time and
   resolution turned into a conversion time and a ceiling, including the
   prescaler setting that hands the converter more than it is rated for.
2. [The timer decides](app/Src/adc_timer.c): a chosen rate, the period computed
   from the timer clock rather than the bus clock, and the interrupt latency
   measured as the spread between the best and worst gaps.
3. [Taking the CPU out](app/Src/adc_trigger.c): the timer's rollover starting
   conversions in hardware, with the overrun flag deliberately provoked so a
   lost sample is a reported one.
4. [Buying bits](app/Src/adc_resolution.c): the LSB, then averaging groups of
   4, 16, 64 and 256 and watching the noise fall by $\sqrt{N}$, with the app
   checking first that there is enough noise for the exercise to mean anything.
5. [Aliasing on real hardware](app/Src/adc_alias.c): a square wave generated on
   one pin and sampled through the next one at rates walking past twice its
   frequency, where the shape on the graph turns into a slow clean wave that is
   not in the signal. Needs a jumper from PB4 to PA1.
6. [Keeping up](app/Src/adc_stream.c): a continuous stream at an exact rate,
   counting what arrived, what was lost, and how much of each period the handler
   spent, which is the measurement the next chapter starts from.

