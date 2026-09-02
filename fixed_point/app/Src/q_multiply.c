/*
 * Why multiplying two fractions needs a shift.
 *
 * Adding two q15 numbers is an integer add and the result is already a q15,
 * because both operands are scaled by the same 32768 and so is the sum.
 *
 * Multiplying is not. Two numbers each scaled by 32768 produce a product scaled
 * by 32768 squared, which is a q30 sitting in a 32 bit word. To get a q15 back
 * out, shift right by 15.
 *
 *     0.5 as q15 is 16384
 *     16384 * 16384 = 268435456
 *     268435456 >> 15 = 8192, which is 0.25 as q15
 *
 * That shift is the whole content, and it is where the second half of the
 * precision goes: the product had 30 fractional bits and 15 of them were
 * thrown away to fit the answer back into the format.
 *
 * Which is also why the accumulator in a fixed point filter is wider than the
 * samples. Shifting after every multiply loses a bit each time; shifting once,
 * at the end, after everything has been added into a 64 bit register, does not.
 *
 *     make q_multiply && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"

static const float32_t pairs[][2] =
{
    { 0.5f,   0.5f   },
    { 0.25f,  0.5f   },
    { 0.9f,   0.9f   },
    { 0.001f, 0.001f },
    { -0.75f, 0.5f   },
};

int main(void)
{
    config_app();
    probe_reset();

    printf("\r\ntwo q15 operands, a q30 product, and a shift to get back\r\n\r\n");

    printf("%8s %8s %10s %10s %14s %10s %10s\r\n",
           "a", "b", "a as q15", "b as q15", "product q30", "  >> 15", "wanted");

    for (uint32_t k = 0U; k < ARRAY_LEN(pairs); k++)
    {
        q15_t a = (q15_t)(pairs[k][0] * Q15_ONE);
        q15_t b = (q15_t)(pairs[k][1] * Q15_ONE);

        int32_t product = (int32_t)a * (int32_t)b;
        q15_t   shifted = (q15_t)(product >> 15);

        printf("%8.3f %8.3f %10ld %10ld %14ld %10.6f %10.6f\r\n",
               (double)pairs[k][0], (double)pairs[k][1], (long)a, (long)b,
               (long)product, (double)((float32_t)shifted / Q15_ONE),
               (double)(pairs[k][0] * pairs[k][1]));
    }

    printf("\r\nthe library does exactly this. arm_mult_q15 on the same"
           " pairs:\r\n\r\n");
    printf("%8s %8s %12s %12s\r\n", "a", "b", "arm_mult_q15", "by hand");

    for (uint32_t k = 0U; k < ARRAY_LEN(pairs); k++)
    {
        q15_t in_a[1] = { (q15_t)(pairs[k][0] * Q15_ONE) };
        q15_t in_b[1] = { (q15_t)(pairs[k][1] * Q15_ONE) };
        q15_t out[1];

        arm_mult_q15(in_a, in_b, out, 1U);

        int32_t product = (int32_t)in_a[0] * (int32_t)in_b[0];

        printf("%8.3f %8.3f %12.6f %12.6f\r\n",
               (double)pairs[k][0], (double)pairs[k][1],
               (double)((float32_t)out[0] / Q15_ONE),
               (double)((float32_t)(q15_t)(product >> 15) / Q15_ONE));
    }

    printf("\r\nthe 0.001 row: 0.001 squared is a millionth, a q15 step is"
           " thirty\r\nmillionths, so the answer is zero, not inaccurate,"
           " gone.\r\n");

    while (1)
    {
        for (uint32_t k = 0U; k < ARRAY_LEN(pairs); k++)
        {
            q15_t a = (q15_t)(pairs[k][0] * Q15_ONE);
            q15_t b = (q15_t)(pairs[k][1] * Q15_ONE);

            g_f32 = pairs[k][0] * pairs[k][1];
            g_q15 = (float32_t)(q15_t)(((int32_t)a * (int32_t)b) >> 15)
                    / Q15_ONE;
            g_err = g_q15 - g_f32;
            probe_step();
        }
    }
}