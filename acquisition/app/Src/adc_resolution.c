/*
 * Twelve bits, and how to buy a few more.
 *
 * The converter divides its reference into 4096 steps, so one count is
 * 3300/4096 mV, about 806 uV, and no amount of care gets a reading finer than
 * that from a single conversion.
 *
 * From more than one conversion it does. Averaging N samples of a steady
 * voltage divides the noise on it by sqrt(N), which is the same statement the
 * moving average made about a signal in the filters chapter and the same
 * sqrt(N) the statistics chapter measured. Four times the samples is one extra
 * bit, so 4^n samples buys n bits, and the price is that the effective rate
 * drops by the same factor.
 *
 * There is a catch that sounds like a joke and is not: this only works if
 * there is noise. A perfectly clean DC voltage sitting inside one code
 * averages to that code forever and no averaging will find the part below it.
 * The noise is what carries the information about where inside the code the
 * signal really is, so a little of it is a requirement rather than a nuisance.
 * A floating pin has plenty; a bench supply may not.
 *
 *     make adc_resolution && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "driver_adc.h"
#include "driver_clock.h"

#define SAMPLES         1024U

static float32_t buf[SAMPLES];

static const uint32_t groups[] = { 1U, 4U, 16U, 64U, 256U };

int main(void)
{
    config_app();
    probe_reset();

    adc_pa1_init();
    adc_free_running();

    printf("\r\n%lu bit converter over %lu mV\r\n", 12UL,
           (unsigned long)ADC_VREF_MV);
    printf("one count is %lu uV\r\n\r\n",
           (unsigned long)(ADC_VREF_MV * 1000U / ADC_FULL_SCALE));

    for (uint32_t i = 0U; i < SAMPLES; i++)
    {
        buf[i] = (float32_t)adc_read();
    }

    float32_t mean;
    float32_t sigma;

    arm_mean_f32(buf, SAMPLES, &mean);
    arm_std_f32(buf, SAMPLES, &sigma);

    printf("%lu raw samples of PA1\r\n", (unsigned long)SAMPLES);
    printf("  mean  %10.3f counts, %8.1f mV\r\n", (double)mean,
           (double)(mean * (float32_t)ADC_VREF_MV
                    / (float32_t)ADC_FULL_SCALE));
    printf("  sigma %10.3f counts\r\n\r\n", (double)sigma);

    if (sigma < 0.3f)
    {
        printf("that is under a third of a count, so there is almost nothing"
               " to average.\r\nthe table below will barely move. touch the pin"
               " or leave it floating\r\nand run it again to see the other"
               " case.\r\n\r\n");
    }

    printf("%8s %12s %12s %14s %12s\r\n",
           "average", "sigma", "vs sqrt(N)", "extra bits", "rate left");

    for (uint32_t k = 0U; k < ARRAY_LEN(groups); k++)
    {
        uint32_t n = groups[k];
        uint32_t out = SAMPLES / n;

        for (uint32_t i = 0U; i < out; i++)
        {
            float32_t sum = 0.0f;

            for (uint32_t j = 0U; j < n; j++)
            {
                sum += buf[(i * n) + j];
            }

            buf[i + SAMPLES - out] = sum / (float32_t)n;
        }

        float32_t s;
        arm_std_f32(&buf[SAMPLES - out], out, &s);

        printf("%8lu %12.4f %12.2f %14.2f %11lu%%\r\n",
               (unsigned long)n, (double)s,
               (double)(sigma / (s > 0.0f ? s : 1.0e-6f)),
               (double)(logf((float32_t)n) / logf(4.0f)),
               (unsigned long)(100U / n));

        g_jitter = s;
    }

    printf("\r\nthe sigma column should fall like sqrt(N) and the rate column"
           " is what\r\nit cost. there is no free lunch here, only a rate you"
           " may not need.\r\n");

    g_rate = mean;

    while (1)
    {
        uint32_t v = adc_read();

        g_raw   = (float32_t)v;
        g_volts = (float32_t)v * (float32_t)ADC_VREF_MV
                  / (float32_t)ADC_FULL_SCALE / 1000.0f;
        probe_step();
    }
}