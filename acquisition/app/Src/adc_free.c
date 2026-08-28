/*
 * The converter on its own, and the rate nobody chose.
 *
 * Continuous mode means the ADC finishes one sample and starts the next
 * immediately. That is a sampling rate, and it is a perfectly definite number,
 * and it is not a number anybody decided: it fell out of the bus clock, the
 * prescaler and the sample time.
 *
 * Which would be a detail, except that every frequency computed anywhere in
 * this repository has an fs in it. The DFT reports bin k as k*fs/N. Get fs
 * wrong by 12% and every frequency is wrong by 12%, and nothing in the numbers
 * will say so.
 *
 * So this app measures its own rate against SysTick and prints it next to the
 * rate the datasheet arithmetic predicts. Two ways of getting at the same
 * number, which is the only kind of agreement worth anything.
 *
 *     make adc_free && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "driver_adc.h"
#include "driver_clock.h"
#include "driver_systick.h"

#define WINDOW_MS       200U
#define STATS_N         512U

int main(void)
{
    config_app();
    probe_reset();

    adc_pa1_init();
    adc_free_running();

    printf("\r\nPCLK2 %lu Hz, ADCCLK %lu Hz, %lu cycles per conversion\r\n",
           (unsigned long)clock_pclk2(), (unsigned long)adc_clk_hz(),
           (unsigned long)adc_conversion_cycles());

    uint32_t predicted = adc_clk_hz() / adc_conversion_cycles();

    /* count conversions for a fixed stretch of wall clock */
    uint64_t start = ticks_get();

    while (ticks_get() == start)
    {
    }

    start = ticks_get();

    uint32_t taken = 0U;

    while ((ticks_get() - start) < WINDOW_MS)
    {
        (void)adc_read();
        taken++;
    }

    uint32_t measured = (taken * 1000U) / WINDOW_MS;

    printf("\r\n%-24s %12lu Hz\r\n", "rate from the datasheet", (unsigned long)predicted);
    printf("%-24s %12lu Hz\r\n", "rate counted against SysTick", (unsigned long)measured);
    printf("%-24s %12ld %%\r\n", "the gap", (long)(((int32_t)measured - (int32_t)predicted) * 100 / (int32_t)predicted));

    printf("\r\nthe measured one is lower, and the missing part is this loop:"
           "\r\nreading DR and incrementing a counter is not free, and in"
           " continuous\r\nmode the converter does not wait for it.\r\n");

    /* what the signal on the pin looks like */
    uint32_t lo = 0xFFFFFFFFU;
    uint32_t hi = 0U;
    uint32_t sum = 0U;

    for (uint32_t i = 0U; i < STATS_N; i++)
    {
        uint32_t v = adc_read();

        sum += v;
        if (v < lo) { lo = v; }
        if (v > hi) { hi = v; }
    }

    uint32_t mean = sum / STATS_N;

    printf("\r\n%lu samples of whatever is on PA1\r\n\r\n",
           (unsigned long)STATS_N);
    printf("  %-10s %6lu counts %8lu mV\r\n", "min", (unsigned long)lo,
           (unsigned long)(lo * ADC_VREF_MV / ADC_FULL_SCALE));
    printf("  %-10s %6lu counts %8lu mV\r\n", "mean", (unsigned long)mean,
           (unsigned long)(mean * ADC_VREF_MV / ADC_FULL_SCALE));
    printf("  %-10s %6lu counts %8lu mV\r\n", "max", (unsigned long)hi,
           (unsigned long)(hi * ADC_VREF_MV / ADC_FULL_SCALE));
    printf("  %-10s %6lu counts\r\n", "spread", (unsigned long)(hi - lo));

    printf("\r\nan LSB is %lu uV. tie PA1 to GND and the mean goes near 0, tie"
           " it to\r\n3V3 and it goes near %lu, and a finger on the pin makes"
           " the spread jump.\r\n",
           (unsigned long)(ADC_VREF_MV * 1000U / ADC_FULL_SCALE),
           (unsigned long)(ADC_FULL_SCALE - 1U));

    g_rate = (float32_t)measured;

    while (1)
    {
        uint32_t v = adc_read();

        g_raw   = (float32_t)v;
        g_volts = (float32_t)v * (float32_t)ADC_VREF_MV
                  / (float32_t)ADC_FULL_SCALE / 1000.0f;
        probe_step();
    }
}