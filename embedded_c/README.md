# DSP - Embedded C

- CMSIS-DSP Version: 1.16.2
- CMSIS Version: 6.3

## ARN Cortex DSP Support Features

In the past digital signal processing applications where deployed on digital
signal processors, because traditional microcontrollers were not powerful enough
to run dsp applications realistically.

**MCU vs DSP**

- MCUs are used for genereal purpose computations while DSPs are used for
real-time intensive computations.

- MCUs use low power memory types while DSPs typically have no flash memory.

**Digital Signal Controllers (DSCs)**

- MCU + DSP
- Harvard Architecture
- High Performance MAC
- Saturation Math
- SIMD Instructions for Parallel Computation
- Barrel Shifters
- Floating Point Hardware
- E.g: Arm Cortex-M4, M7, M33, M23

**DSP Support Features**

- Floating Point Unit (FPU)
- SIMD Capabilities
- MAC Capabilities
- CMSIS-DSP

**Floating Point Unit**

- The FPU is a coprocessor accesed by dedicated instructions for floating point
arithmetic in a few cycles.

| Operation | Cycle count |
| --- | --- |
| Add/subtract | 1 |
| Divide | 14 (New lib takes 80-120) |
| Multiply | 1 |
| MAC | 3 |
| Fused MAC | 3 |
| Square root | 14 |

- The FPU Consists of a grroup of control and status registers and 32 single
pricision (32-bit_ registers).

**SIMD Capabilities**

- SIMD stands for Single-Instruction-Multiple-Data
- SIMD Instructions allow multiple parallel arithmetic operations on 8 or 16 bit
data quantities
- The 8 nad 16 bit data must be packed into 32-bit

**CMSIS-DSP**

- CMSIS-DSP is a library of 61 common dsp functions published by Arm.
- These functions are optimised for running on Arm microcontrollers
    - Basic math functions
    - Transforms functions
    - Interpolation functions
    - Filters functions
    - Fast math functions
    - Complex Math functions
    - Motor Control functions
    - Matrix functions
    - Statistical functions
    - Support functions

- Data Types
    - float32_t
    - float64_t
    - q7_t
    - q15_t
    - q31_t
    - q63_t

--

1. [Quantization and Sampling](sampling)
2. [Linear Systems and Superposition](superposition)
3. [Discrete Fourier Transform](dft)