/*
 * The other knob.
 *
 * The previous app held the length fixed and changed the window. This one
 * holds the window fixed and changes the length, and the interesting part is
 * the column that does not move.
 *
 * More taps make the transition narrower and leave the stopband exactly where
 * the window put it. So the two choices do not interact: pick the window from
 * how much rejection is needed, pick the length from how sharp the corner has
 * to be, and neither answer changes the other.
 *
 * What length costs is arithmetic and delay. Both are on the table.
 *
 *     make fir_length && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "filters.h"
#include "kernels.h"
#include "testsig.h"

#define FC_HZ       200.0f
#define SWEEP       201U

static const float32_t *const kernel[] = { lp_31, lp_hamming, lp_201 };
static const uint32_t taps[] = { LP_31_LEN, LP_HAMMING_LEN, LP_201_LEN };

static float32_t stopband(const float32_t *pH, uint32_t hLen)
{
    float32_t prev = 1.0f;
    float32_t worst = 0.0f;
    uint8_t   past_null = 0U;

    for (float32_t f = FC_HZ; f < (float32_t)TESTSIG_FS_HZ / 2.0f; f += 0.5f)
    {
        float32_t g = fir_gain(pH, hLen, f, (float32_t)TESTSIG_FS_HZ);

        if ((past_null == 0U) && (g >= prev))
        {
            past_null = 1U;
        }

        if ((past_null != 0U) && (g > worst))
        {
            worst = g;
        }

        prev = g;
    }

    return fir_db(worst);
}

static float32_t transition(const float32_t *pH, uint32_t hLen)
{
    float32_t lo = 0.0f;

    for (float32_t f = 0.0f; f < (float32_t)TESTSIG_FS_HZ / 2.0f; f += 0.5f)
    {
        float32_t g = fir_gain(pH, hLen, f, (float32_t)TESTSIG_FS_HZ);

        if (g >= 0.99f)
        {
            lo = f;
        }

        if ((f > lo) && (g <= 0.01f))
        {
            return f - lo;
        }
    }

    return 0.0f;
}

int main(void)
{
    config_app();
    probe_reset();

    printf("\r\nthe same hamming sinc at %.0f Hz, three lengths\r\n\r\n",
           (double)FC_HZ);
    printf("%6s %10s %12s %8s %12s\r\n",
           "taps", "stopband", "transition", "delay", "mults/sample");

    for (uint32_t k = 0U; k < ARRAY_LEN(kernel); k++)
    {
        printf("%6lu %7.1f dB %9.1f Hz %8lu %12lu\r\n",
               (unsigned long)taps[k],
               (double)stopband(kernel[k], taps[k]),
               (double)transition(kernel[k], taps[k]),
               (unsigned long)FIR_DELAY(taps[k]),
               (unsigned long)taps[k]);
    }

    printf("\r\nthe stopband column does not move. the window set it and the"
           " length cannot.\r\n");
    printf("the transition halves when the taps double, and so does nothing"
           " else.\r\n");

    while (1)
    {
        for (uint32_t k = 0U; k < ARRAY_LEN(kernel); k++)
        {
            for (uint32_t i = 0U; i < SWEEP; i++)
            {
                float32_t f = ((float32_t)TESTSIG_FS_HZ / 2.0f) *
                              (float32_t)i / (float32_t)(SWEEP - 1U);

                g_h   = (i < taps[k]) ? kernel[k][i] : 0.0f;
                g_mag = fir_db(fir_gain(kernel[k], taps[k], f,
                                        (float32_t)TESTSIG_FS_HZ));
                g_x   = f;
                probe_step();
            }
        }
    }
}