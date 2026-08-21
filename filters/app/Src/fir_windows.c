/*
 * Where the taps come from, and what the window is for.
 *
 * A filter that passes everything below a cutoff and nothing above it has one
 * kernel, and that kernel is a sinc. The sinc never ends. Any kernel that fits
 * in memory is a piece cut out of it, and cutting a signal off abruptly is the
 * same act that produced the ringing in the square wave two chapters ago: the
 * cut is a step, and a step is broad in frequency.
 *
 * The window is the fix. Instead of stopping the sinc dead, taper it to zero.
 * All three kernels here are the same sinc at the same cutoff and the same
 * length, tapered three ways, and the two columns move in opposite directions.
 *
 *     make fir_windows && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "filters.h"
#include "kernels.h"
#include "testsig.h"

#define FC_HZ       200.0f
#define SWEEP       201U        /* frequency points streamed to the graph */

static const float32_t *const kernel[] = { lp_rect, lp_hamming, lp_blackman };
static const char *const name[] = { "rectangular", "hamming", "blackman" };

/*
 * The tallest ripple left after the response stops falling. firwin puts the
 * -6 dB point exactly on the cutoff, so the walk starts there, goes down to the
 * first null, and keeps the largest thing after it.
 */
static float32_t stopband(const float32_t *pH, uint32_t hLen)
{
    float32_t prev = 1.0f;
    float32_t worst = 0.0f;
    uint8_t past_null = 0U;

    for(float32_t f = FC_HZ; f < (float32_t)TESTSIG_FS_HZ/2.0f; f += 0.5f)
    {
        float32_t g = fir_gain(pH, hLen, f, (float32_t)TESTSIG_FS_HZ);

        if((past_null == 0U) && (g >= prev))
        {
            past_null = 1U;
        }

        if((past_null != 0U) && (g > worst))
        {
            worst = g;
        }

        prev = g;
    }

    return fir_db(worst);
}

/* how far it takes to get from 99% of the passband down to 1% */
static float32_t transition(const float32_t *pH, uint32_t hLen)
{
    float32_t lo = 0.0f;

    for (float32_t f = 0.0f; f < (float32_t)TESTSIG_FS_HZ / 2.0f; f += 0.5f)
    {
        float32_t g = fir_gain(pH, hLen, f, (float32_t)TESTSIG_FS_HZ);

        if(g >= 0.99f)
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

    printf("\r\nthe same sinc at %.0f Hz, %u taps, tapered three ways\r\n\r\n",
           (double)FC_HZ, (unsigned)LP_RECT_LEN);
    printf("%-13s %10s %12s\r\n", "window", "stopband", "transition");

    for (uint32_t k = 0U; k < ARRAY_LEN(kernel); k++)
    {
        printf("%-13s %7.1f dB %9.1f Hz\r\n", name[k],
               (double)stopband(kernel[k], LP_RECT_LEN),
               (double)transition(kernel[k], LP_RECT_LEN));
    }

    printf("\r\ndeeper stopband, wider transition. the window buys one with"
           " the other.\r\n");

    while(1)
    {
        for (uint32_t k = 0U; k < ARRAY_LEN(kernel); k++)
        {
            for (uint32_t i = 0U; i < SWEEP; i++)
            {
                float32_t f = ((float32_t)TESTSIG_FS_HZ / 2.0f) *
                              (float32_t)i / (float32_t)(SWEEP - 1U);

                g_h   = (i < LP_RECT_LEN) ? kernel[k][i] : 0.0f;
                g_mag = fir_db(fir_gain(kernel[k], LP_RECT_LEN, f,
                                        (float32_t)TESTSIG_FS_HZ));
                g_x   = f;
                probe_step();
            }
        }
    }
}