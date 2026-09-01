/*
 * What a Q number is, and the question the lesson asks and does not answer.
 *
 * The lesson lists the fractional types as q7, q15, q31 and q63 and then says
 * "I suppose by now you're wondering why don't we just call them q8, q16, q32
 * and q64, well you'll understand that later on in this section", and later
 * never arrives.
 *
 * The answer is one bit. A q15 lives in a signed 16 bit word, and one of those
 * bits is the sign, so there are 15 left to put fraction in. The full name is
 * Q1.15: one integer bit, which is the sign, and fifteen fractional bits. The
 * short name drops the integer part because for these types it is always one.
 *
 * So the number is not an integer scaled by some factor you picked. It is a
 * fraction, it lives between -1 and just under +1, and the integer stored in
 * the word is the fraction times 2 to the power of the fractional bits.
 *
 *     make q_format && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"

int main(void)
{
    config_app();
    probe_reset();

    printf("\r\nthe same idea in three widths\r\n\r\n");

    printf("%-8s %6s %6s %8s %16s %16s\r\n",
           "type", "bits", "sign", "fraction", "step", "largest");

    printf("%-8s %6d %6d %8d %16.9f %16.9f\r\n", "q7", 8, 1, 7,
           (double)(1.0f / Q7_ONE), (double)(127.0f / Q7_ONE));
    printf("%-8s %6d %6d %8d %16.9f %16.9f\r\n", "q15", 16, 1, 15,
           (double)(1.0f / Q15_ONE), (double)(32767.0f / Q15_ONE));
    printf("%-8s %6d %6d %8d %16.12f %16.12f\r\n", "q31", 32, 1, 31,
           (double)(1.0 / (double)Q31_ONE),
           (double)(2147483647.0 / (double)Q31_ONE));

    printf("\r\nevery one of them is Q1.n: one integer bit, which is the sign,"
           " and n\r\nfractional bits. n is what the name counts, so a 16 bit"
           " word is a q15.\r\n");

    printf("\r\nthe range is not symmetric, and that is not a bug either:\r\n");
    printf("  most negative %+.9f   is exactly -1\r\n",
           (double)(-32768.0f / Q15_ONE));
    printf("  most positive %+.9f   is one step short of +1\r\n",
           (double)(32767.0f / Q15_ONE));
    printf("\r\n65536 codes, zero spends one, so one side is a step short:"
           " +1.0 is the\r\nvalue a q15 cannot hold.\r\n");

    /* what one number looks like in each */
    static const float32_t examples[] = { 0.5f, 0.1f, 0.001f, 0.9999f };

    printf("\r\n%12s %10s %12s %14s %14s\r\n",
           "value", "as q7", "as q15", "as q31", "q15 error");

    for (uint32_t k = 0U; k < ARRAY_LEN(examples); k++)
    {
        float32_t f = examples[k];
        int32_t   a = (int32_t)(f * Q7_ONE);
        int32_t   b = (int32_t)(f * Q15_ONE);
        int32_t   c = (int32_t)((double)f * (double)Q31_ONE);

        printf("%12.6f %10ld %12ld %14ld %14.3e\r\n", (double)f,
               (long)a, (long)b, (long)c,
               (double)(((float32_t)b / Q15_ONE) - f));
    }

    printf("\r\nthe q7 column is why q7 is rare. 0.001 in q7 is 0, so a filter"
           " tap that\r\nsmall simply is not there.\r\n");

    while (1)
    {
        for (uint32_t k = 0U; k < ARRAY_LEN(examples); k++)
        {
            g_f32 = examples[k];
            g_q15 = (float32_t)(int32_t)(examples[k] * Q15_ONE) / Q15_ONE;
            g_err = g_q15 - g_f32;
            probe_step();
        }
    }
}