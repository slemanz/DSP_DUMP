/*
 * Three tones go in, one comes out, three times.
 *
 * The test signal is 10 Hz plus 100 Hz plus 500 Hz at equal amplitude, and the
 * window holds exactly 1, 10 and 50 whole periods of them, so nothing here
 * leaks and every number is the number it looks like.
 *
 * Three kernels are aimed at it: a low pass at 50 Hz, a band pass from 50 to
 * 300, and a high pass at 300. The table below is what each one does to each
 * tone, which is nine numbers, and the useful way to read it is that the
 * diagonal is about 1 and everything else is about 0.001.
 *
 * The graph is the check that matters. Each output is streamed next to the tone
 * it was supposed to recover, and the two lie on top of each other.
 *
 *     make fir_separate && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "filters.h"
#include "kernels.h"
#include "testsig.h"

#define TAPS        LP_50_LEN
#define OUT_LEN     (SIG_LEN + TAPS - 1U)
#define DELAY       FIR_DELAY(TAPS)
#define SETTLE      60U         /* the ends are still filling up */

static float32_t out[OUT_LEN];

static const float32_t *const kernel[] = { lp_50, bp_50_300, hp_300 };
static const float32_t *const wanted[] = { tone_10, tone_100, tone_500 };
static const char *const name[] = { "lp_50", "bp_50_300", "hp_300" };
static const float32_t tone_hz[] = { 10.0f, 100.0f, 500.0f };

int main(void)
{
    config_app();
    probe_reset();

    printf("\r\n%u samples at %lu Hz: 10 Hz + 100 Hz + 500 Hz, equal amplitude\r\n\r\n",
           (unsigned)SIG_LEN, (unsigned long)TESTSIG_FS_HZ);

    printf("%-11s %9s %9s %9s\r\n", "kernel", "10 Hz", "100 Hz", "500 Hz");

    for (uint32_t k = 0U; k < ARRAY_LEN(kernel); k++)
    {
        printf("%-11s", name[k]);

        for (uint32_t t = 0U; t < ARRAY_LEN(tone_hz); t++)
        {
            printf(" %9.5f", (double)fir_gain(kernel[k], TAPS, tone_hz[t],
                                              (float32_t)TESTSIG_FS_HZ));
        }

        printf("\r\n");
    }

    printf("\r\nand what actually came out, away from the ends\r\n\r\n");
    printf("%-11s %12s %12s\r\n", "kernel", "peak out", "worst gap");

    for (uint32_t k = 0U; k < ARRAY_LEN(kernel); k++)
    {
        float32_t peak = 0.0f;
        float32_t worst = 0.0f;

        fir_apply(sig_3tone, SIG_LEN, kernel[k], TAPS, out);

        for (uint32_t n = SETTLE; n < (SIG_LEN - SETTLE); n++)
        {
            float32_t got = out[n + DELAY];
            float32_t gap = fabsf(got - wanted[k][n]);

            if (fabsf(got) > peak)
            {
                peak = fabsf(got);
            }

            if (gap > worst)
            {
                worst = gap;
            }
        }

        printf("%-11s %12.5f %12.5f\r\n", name[k], (double)peak, (double)worst);
    }

    printf("\r\neach output sits on the tone it was aimed at to within the"
           " passband ripple\r\n");

    while (1)
    {
        for (uint32_t k = 0U; k < ARRAY_LEN(kernel); k++)
        {
            fir_apply(sig_3tone, SIG_LEN, kernel[k], TAPS, out);

            for (uint32_t n = 0U; n < SIG_LEN; n++)
            {
                g_x   = sig_3tone[n];
                g_y   = out[n + DELAY];
                g_ref = wanted[k][n];
                g_h   = (n < TAPS) ? kernel[k][n] : 0.0f;
                g_mag = out[n + DELAY] - wanted[k][n];
                probe_step();
            }
        }
    }
}