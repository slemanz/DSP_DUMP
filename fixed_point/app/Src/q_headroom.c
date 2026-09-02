/*
 * Where a fixed point filter actually breaks.
 *
 * A filter multiplies and then adds, and the adding is the dangerous half.
 * Every individual product fits in the format, and the running total need not.
 * The worst the sum can reach is the sum of the absolute values of the taps
 * times the largest sample, so
 *
 *     headroom needed = sum |h[i]|
 *
 * For a low pass whose taps add to 1 and are all positive, that is 1, and a
 * full scale input produces a full scale output with nothing to spare. For a
 * high pass, whose taps alternate in sign, the taps add to 0 and the absolute
 * values add to considerably more than 1, so a full scale input overflows.
 *
 * There are two ways out and the library uses both. Accumulate in something
 * wider than the samples, so the intermediate total cannot overflow whatever it
 * reaches, and saturate once at the end rather than at every step. Or scale the
 * input down by the headroom before starting and scale the answer back up
 * afterwards, paying for the safety in resolution.
 *
 * The table below computes the headroom for the module's kernel and then runs
 * the sum three ways to show what each one gives back.
 *
 *     make q_headroom && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "testsig.h"

int main(void)
{
    config_app();
    probe_reset();

    float32_t sum_taps = 0.0f;
    float32_t sum_abs  = 0.0f;

    for (uint32_t i = 0U; i < TAPS; i++)
    {
        sum_taps += taps[i];
        sum_abs  += fabsf(taps[i]);
    }

    printf("\r\n%lu tap low pass\r\n\r\n", (unsigned long)TAPS);
    printf("  %-28s %10.6f\r\n", "sum of the taps", (double)sum_taps);
    printf("  %-28s %10.6f\r\n", "sum of |taps|", (double)sum_abs);
    printf("  %-28s %10.6f\r\n", "worst output for input 1.0",
           (double)sum_abs);
    printf("  %-28s %10s\r\n", "fits in q15",
           (sum_abs < 1.0f) ? "yes" : "only just");

    /* now the same question for a kernel that alternates sign */
    float32_t high_abs = 0.0f;
    float32_t high_sum = 0.0f;

    for (uint32_t i = 0U; i < TAPS; i++)
    {
        float32_t t = -taps[i];

        if (i == (TAPS / 2U))
        {
            t += 1.0f;
        }

        high_sum += t;
        high_abs += fabsf(t);
    }

    printf("\r\nthe same kernel spectrally inverted into a high pass\r\n\r\n");
    printf("  %-28s %10.6f\r\n", "sum of the taps", (double)high_sum);
    printf("  %-28s %10.6f\r\n", "sum of |taps|", (double)high_abs);
    printf("  %-28s %10s\r\n", "fits in q15",
           (high_abs <= 1.0f) ? "yes" : "NO");

    printf("\r\nthe two filters that add back to a single impulse do not share"
           " headroom:\r\nthe one whose taps cancel needs %.2f times the range"
           " of the one whose\r\ntaps add.\r\n", (double)(high_abs / sum_abs));

    /* the sum done three ways on a worst case input */
    q15_t   worst_in[TAPS];
    q15_t   q_taps[TAPS];

    for (uint32_t i = 0U; i < TAPS; i++)
    {
        float32_t t = -taps[i];

        if (i == (TAPS / 2U))
        {
            t += 1.0f;
        }

        q_taps[i]   = (q15_t)(t * Q15_ONE);
        /* the input that drives this kernel to its extreme */
        worst_in[i] = (t >= 0.0f) ? 32767 : -32768;
    }

    q15_t narrow = 0;
    q31_t medium = 0;
    q63_t wide   = 0;

    for (uint32_t i = 0U; i < TAPS; i++)
    {
        q15_t one = (q15_t)(((int32_t)q_taps[i] * (int32_t)worst_in[i]) >> 15);

        narrow = (q15_t)(narrow + one);
        medium += (q31_t)one;
        wide   += ((q63_t)q_taps[i] * (q63_t)worst_in[i]);
    }

    printf("\r\nthe worst case input for that high pass, summed three"
           " ways\r\n\r\n");
    printf("  %-34s %12.6f\r\n", "q15 accumulator, wrapping",
           (double)((float32_t)narrow / Q15_ONE));
    printf("  %-34s %12.6f\r\n", "q31 accumulator, then saturate",
           (double)((float32_t)__SSAT(medium, 16) / Q15_ONE));
    printf("  %-34s %12.6f\r\n", "q63 accumulator, one saturate",
           (double)((float32_t)__SSAT((q31_t)(wide >> 15), 16) / Q15_ONE));
    printf("  %-34s %12.6f\r\n", "what it should be", (double)high_abs);

    g_f32 = high_abs;
    g_q15 = (float32_t)narrow / Q15_ONE;
    g_q31 = (float32_t)__SSAT((q31_t)(wide >> 15), 16) / Q15_ONE;

    while (1)
    {
        for (uint32_t i = 0U; i < TAPS; i++)
        {
            g_err = taps[i];
            probe_step();
        }
    }
}