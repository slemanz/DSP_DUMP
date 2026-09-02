/*
 * What happens at the end of the range, and why there are two answers.
 *
 * A q15 stops just short of +1. Adding two numbers that each sit near there
 * asks for something the format cannot hold, and the hardware will do one of
 * two completely different things depending on which instruction was used.
 *
 * Wrapping is what plain integer arithmetic does. The bits carry past the sign
 * bit and the result comes back negative. Nothing is flagged. In a filter this
 * is not a large error, it is a sign inversion at full amplitude, and it sounds
 * and looks like a fault in the hardware.
 *
 * Saturating is what the DSP instructions do. The result is clamped to the
 * largest value the format holds and stays there. That is still wrong, but it
 * is wrong in the direction the signal was already going, and a clipped peak is
 * something a person can recognise.
 *
 * This is the reason the instruction set has QADD and QSUB at all, and the
 * reason every arm_*_q15 routine in the library saturates rather than wraps.
 *
 *     make q_saturate && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"

static const float32_t pairs[][2] =
{
    { 0.5f,  0.25f },
    { 0.5f,  0.5f  },
    { 0.9f,  0.9f  },
    { -0.9f, -0.9f },
    { 0.99f, 0.99f },
};

int main(void)
{
    config_app();
    probe_reset();

    printf("\r\nq15 holds -1.000000 to +0.999969\r\n\r\n");

    printf("%9s %9s %12s %12s %12s\r\n",
           "a", "b", "wanted", "wrapped", "saturated");

    for (uint32_t k = 0U; k < ARRAY_LEN(pairs); k++)
    {
        q15_t a = (q15_t)(pairs[k][0] * Q15_ONE);
        q15_t b = (q15_t)(pairs[k][1] * Q15_ONE);

        q15_t wrapped = (q15_t)(a + b);

        q15_t sat_in_a[1] = { a };
        q15_t sat_in_b[1] = { b };
        q15_t sat_out[1];
        arm_add_q15(sat_in_a, sat_in_b, sat_out, 1U);

        printf("%9.3f %9.3f %12.6f %12.6f %12.6f\r\n",
               (double)pairs[k][0], (double)pairs[k][1],
               (double)(pairs[k][0] + pairs[k][1]),
               (double)((float32_t)wrapped / Q15_ONE),
               (double)((float32_t)sat_out[0] / Q15_ONE));

        g_f32 = pairs[k][0] + pairs[k][1];
        g_q15 = (float32_t)wrapped / Q15_ONE;
        g_q31 = (float32_t)sat_out[0] / Q15_ONE;
    }

    printf("\r\nthe 0.9 + 0.9 row: wanted 1.8, wrapped gives -0.2, the wrong"
           " sign at a\r\nfifth of the amplitude rather than an overflow that"
           " looks like one.\r\n");

    printf("\r\nthe same thing with __SSAT, which is the instruction the"
           " library is\r\nbuilt out of:\r\n\r\n");

    printf("%14s %12s %12s\r\n", "sum as int32", "__SSAT to 16", "as a"
           " fraction");

    static const int32_t sums[] = { 20000, 40000, -40000, 100000 };

    for (uint32_t k = 0U; k < ARRAY_LEN(sums); k++)
    {
        int32_t clamped = __SSAT(sums[k], 16);

        printf("%14ld %12ld %12.6f\r\n", (long)sums[k], (long)clamped,
               (double)((float32_t)clamped / Q15_ONE));
    }

    printf("\r\n__SSAT(x, 16) clamps to what a signed 16 bit word holds, one"
           " instruction,\r\none cycle, which is why saturating costs no more"
           " than wrapping.\r\n");

    while (1)
    {
        for (uint32_t k = 0U; k < ARRAY_LEN(pairs); k++)
        {
            q15_t a = (q15_t)(pairs[k][0] * Q15_ONE);
            q15_t b = (q15_t)(pairs[k][1] * Q15_ONE);

            g_f32 = pairs[k][0] + pairs[k][1];
            g_q15 = (float32_t)(q15_t)(a + b) / Q15_ONE;
            g_q31 = (float32_t)__SSAT((int32_t)a + (int32_t)b, 16) / Q15_ONE;
            g_err = g_q15 - g_f32;
            probe_step();
        }
    }
}