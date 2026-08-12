/*
 * How long each of the three implementations takes to convolve 320 samples
 * with 29 taps, counted in processor cycles by SysTick.
 *
 * The counter has its own cost, so the first row measures nothing at all and
 * that number is subtracted from the rest. An instrument you have not measured
 * is not an instrument.
 *
 * Every row here does the same 9280 multiply accumulates. The only thing that
 * separates them is how the work is arranged, which is what the word optimized
 * means when it is applied to CMSIS-DSP. This module builds at -O0, so the two
 * hand written loops are being measured with the compiler helping as little as
 * it is allowed to. Rebuilding at -O2 is worth doing once, to see how much of
 * the gap belongs to CMSIS-DSP and how much of it belonged to -O0.
 *
 *     make conv_timing && make load && make monitor
 */
#include <stdio.h>
#include "config.h"
#include "signals.h"
#include "driver_systick.h"
#include "driver_clock.h"
#include "probe.h"
#include "conv.h"

#define X_LEN       ((uint32_t)KHZ1_15_SIG_LEN)
#define Y_LEN       (X_LEN + LOWPASS_LEN - 1U)

static float32_t y[Y_LEN];

int main(void)
{
    uint32_t overhead;
    uint32_t scatter;
    uint32_t gather;
    uint32_t cmsis;
    float32_t ns_per_cycle;

    config_app();

    while(1)
    {

    }
}