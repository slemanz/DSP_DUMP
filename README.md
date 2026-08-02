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
- [Linear Systems and Superposition](embedded_c/superposition): the properties
  that make a system linear, and the decomposition they allow.
- [Discrete Fourier Transform](embedded_c/dft): moving a signal between the
  time domain and the frequency domain.
- [Template](template): the bare-metal STM32F411 project skeleton the examples
  are built on, with drivers, linker and build wired up.

The chapters still pointing into [`embedded_c/`](embedded_c) are the earlier
material, being rewritten one at a time into modules of their own at the root.
The CMSIS-DSP archive they all link against lives in [`lib/`](lib) and is shared
rather than copied per module.

---

Built against **CMSIS-DSP 1.16.2** and **CMSIS 6.3**.
