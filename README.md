# DSP Dump

Digital signal processing used to be the business of dedicated processors,
because ordinary microcontrollers were not fast enough to run it in real time.
That changed with the digital signal controller: a microcontroller carrying the
arithmetic hardware a DSP has, a floating point unit, SIMD instructions and a
single cycle multiply accumulate, without giving up the peripherals and the
on-chip flash that make an MCU practical. The Arm Cortex-M4 in the STM32F411
used throughout this repository is one of them.

Every chapter here is a bare-metal project that builds, flashes and runs on that
chip. The algorithms are written twice: once by hand, so the arithmetic stays
visible, and once through CMSIS-DSP, the library of routines Arm publishes for
these cores, so the cost of the optimized version can be measured against the
obvious one.

## Contents

In reading order:

- [Signal Statistics and Noise](statistics): the mean, the variance and the
  standard deviation of a signal, what each one measures, and how noise is
  quantified with them.
- [Quantization and the Sampling Theorem](sampling): turning a continuous
  signal into samples, what the ADC's finite resolution costs, and how fast the
  sampling has to be to keep the signal intact.
- [Linear Systems and Superposition](superposition): the properties that make a
  system linear, and the decomposition they allow.
- [Convolution](convolution): the operation that turns an impulse response into
  the output of a system for any input.
- [Discrete Fourier Transform](dft): moving a signal between the time domain
  and the frequency domain.
- [Digital Filter Design](filters): where the impulse response comes from, and
  how the frequency response reports what was chosen.
- [Clock Tree and Flash Latency](clock): running the core at 100 MHz instead of
  16, and what the memory in front of it costs at that speed.
- [Analog Input and the Sampling Clock](acquisition): reading a real sensor, and
  driving the ADC from a timer so the sampling rate is a number and not a guess.
- [Streaming and Block Processing](blocks): the two ways to hand one sample at a
  time to an algorithm that wants a whole array.
- [Fixed Point and Q Notation](fixed_point): representing fractions in integers,
  what saturation is for, and what the speed is bought with.
- [Optimization Strategies](optimization): one loop taken down a ladder of
  compiler flags, hand transformations and SIMD instructions, measured at every
  rung.
- [The CMSIS-DSP Library](cmsis): what is in it, which routine answers which
  question, and the conventions that are shared across all of them.
- [Template](template): the bare-metal STM32F411 project skeleton the examples
  are built on, with drivers, linker and build wired up.

The first five chapters are the arithmetic, worked on signals that are already
in memory. The rest is the machine those chapters run on: its clock, its
converter, the shape data arrives in, and what can be traded to make it faster.
The CMSIS-DSP archive they all link against lives in [`lib/`](lib) and is shared
rather than copied per module.

---

Built against **CMSIS-DSP 1.16.2** and **CMSIS 6.3**.
