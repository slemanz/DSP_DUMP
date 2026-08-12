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

    ns_per_cycle = 1.0e9f / (float32_t)clock_get();

    cycles_start();
    overhead = cycles_read();

    cycles_start();
    conv_scatter(input_signal_f32_1kHz_15kHz, X_LEN, lowpass_6khz, LOWPASS_LEN, y);
    scatter = cycles_read() - overhead;

    cycles_start();
    conv_gather(input_signal_f32_1kHz_15kHz, X_LEN, lowpass_6khz, LOWPASS_LEN, y);
    gather = cycles_read() - overhead;

    cycles_start();
    arm_conv_f32(input_signal_f32_1kHz_15kHz, X_LEN, lowpass_6khz, LOWPASS_LEN, y);
    cmsis = cycles_read() - overhead;

    /* the tick was stopped to free the timer, so put it back */
    systick_init(TICK_HZ);

    printf("\r\n%lu samples, %lu taps, %lu multiply accumulates each\r\n", (unsigned long)X_LEN, (unsigned long)LOWPASS_LEN, (unsigned long)(X_LEN * LOWPASS_LEN));
    printf("core at %lu Hz, so %.1f ns per cycle, counter costs %lu cycles\r\n\r\n", (unsigned long)clock_get(), ns_per_cycle, (unsigned long)overhead);

    printf("%-14s %10s %8s %7s\r\n", "", "cycles", "ms", "slower");
    printf("%-14s %10lu %8.3f %6.1fx\r\n", "conv_scatter", (unsigned long)scatter, (float32_t)scatter * ns_per_cycle * 1.0e-6f, (float32_t)scatter / (float32_t)cmsis);
    printf("%-14s %10lu %8.3f %6.1fx\r\n", "conv_gather", (unsigned long)gather, (float32_t)gather * ns_per_cycle * 1.0e-6f, (float32_t)gather / (float32_t)cmsis);
    printf("%-14s %10lu %8.3f %6.1fx\r\n", "arm_conv_f32", (unsigned long)cmsis, (float32_t)cmsis * ns_per_cycle * 1.0e-6f, 1.0f);

    printf("\r\n%.1f cycles per multiply accumulate in arm_conv_f32\r\n", (float32_t)cmsis / (float32_t)(X_LEN * LOWPASS_LEN));


    while(1)
    {

    }
}