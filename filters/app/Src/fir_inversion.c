/*
 * One line turns a low pass into the high pass that completes it.
 *
 * Negate every tap, then add one to the middle. That subtracts the kernel from
 * a single impulse, and an impulse passes everything unchanged, so what is left
 * passes everything the kernel threw away.
 *
 * The check is not the frequency response, it is addition. Send the signal
 * through both kernels and add the two outputs, and the result is the input
 * again, delayed by half the kernel and otherwise untouched. Nothing was lost
 * and nothing was invented; the two filters split the signal and the split is
 * exact.
 *
 *     make fir_inversion && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "filters.h"
#include "kernels.h"
#include "testsig.h"

#define TAPS        LP_HAMMING_LEN
#define OUT_LEN     (SIG_LEN + TAPS - 1U)
#define DELAY       FIR_DELAY(TAPS)

static float32_t highpass[TAPS];
static float32_t low_out[OUT_LEN];
static float32_t high_out[OUT_LEN];

int main(void)
{
    config_app();
    probe_reset();

    fir_invert(lp_hamming, TAPS, highpass);

    /* the two kernels should add back to a single impulse in the middle */
    float32_t worst_tap = 0.0f;

    for (uint32_t i = 0U; i < TAPS; i++)
    {
        float32_t want = (i == DELAY) ? 1.0f : 0.0f;
        float32_t gap  = fabsf((lp_hamming[i] + highpass[i]) - want);

        if (gap > worst_tap)
        {
            worst_tap = gap;
        }
    }

    fir_apply(sig_3tone, SIG_LEN, lp_hamming, TAPS, low_out);
    fir_apply(sig_3tone, SIG_LEN, highpass,   TAPS, high_out);

    /* and the two outputs should add back to the input, late by DELAY */
    float32_t worst_sum = 0.0f;

    for (uint32_t n = 0U; n < SIG_LEN; n++)
    {
        float32_t gap = fabsf((low_out[n + DELAY] + high_out[n + DELAY])
                              - sig_3tone[n]);

        if (gap > worst_sum)
        {
            worst_sum = gap;
        }
    }

    printf("\r\n%u tap low pass at 200 Hz, inverted into its own high pass\r\n\r\n",
           (unsigned)TAPS);

    printf("%8s %10s %10s %10s\r\n", "Hz", "low", "high", "sum");

    static const float32_t probe_hz[] = { 10.0f, 100.0f, 190.0f, 200.0f,
                                          210.0f, 300.0f, 500.0f, 900.0f };

    for (uint32_t k = 0U; k < ARRAY_LEN(probe_hz); k++)
    {
        float32_t lo = fir_gain(lp_hamming, TAPS, probe_hz[k],
                                (float32_t)TESTSIG_FS_HZ);
        float32_t hi = fir_gain(highpass, TAPS, probe_hz[k],
                                (float32_t)TESTSIG_FS_HZ);

        printf("%8.0f %10.5f %10.5f %10.5f\r\n",
               (double)probe_hz[k], (double)lo, (double)hi, (double)(lo + hi));
    }

    printf("\r\nlow + high as kernels, against one impulse: %.9f\r\n",
           (double)worst_tap);
    printf("low + high as outputs, against the input: %.9f\r\n",
           (double)worst_sum);
    printf("the outputs run %lu samples late, which is half the kernel\r\n",
           (unsigned long)DELAY);

    while (1)
    {
        for (uint32_t n = 0U; n < SIG_LEN; n++)
        {
            g_x   = sig_3tone[n];
            g_y   = low_out[n + DELAY];
            g_ref = high_out[n + DELAY];
            g_mag = (low_out[n + DELAY] + high_out[n + DELAY]) - sig_3tone[n];
            g_h   = (n < TAPS) ? highpass[n] : 0.0f;
            probe_step();
        }
    }
}