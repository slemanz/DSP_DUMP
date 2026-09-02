/*
 * The round trip, where the error goes, and a bit the library throws away.
 *
 * Converting a float to a q15 and back is quantisation, the same operation the
 * ADC performs and the same one the sampling chapter measured. The difference
 * is that here it happens in software and is entirely a choice.
 *
 * If the conversion rounds to the nearest code, the error is bounded by half a
 * step and spread evenly, so its standard deviation is step over sqrt(12),
 * which is the formula the quantisation chapter used.
 *
 * arm_float_to_q15 does not round. It is a cast, and a cast truncates toward
 * zero, so the error runs to a full step rather than half of one and it leans
 * toward zero rather than sitting either side of the true value. The spread
 * comes out at step over sqrt(3), which is exactly twice as large. Half a bit
 * of resolution, given away by a missing 0.5.
 *
 * That is not a bug in the library, it is one instruction cheaper and it is
 * documented behaviour. It is worth knowing because the fix is one line and
 * because half a bit is a quarter of what dropping from q31 to q15 costs.
 *
 *     make q_convert && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "testsig.h"

static q15_t     truncated[SIG_LEN];
static q15_t     rounded[SIG_LEN];
static q31_t     wide[SIG_LEN];
static float32_t back[SIG_LEN];
static float32_t error[SIG_LEN];

/* what arm_float_to_q15 would do with a 0.5 in it */
static void float_to_q15_rounded(const float32_t *pSrc, q15_t *pDst,
                                 uint32_t len)
{
    for (uint32_t n = 0U; n < len; n++)
    {
        float32_t scaled = pSrc[n] * Q15_ONE;

        pDst[n] = (q15_t)__SSAT((q31_t)((scaled >= 0.0f) ? (scaled + 0.5f)
                                                         : (scaled - 0.5f)),
                                16);
    }
}

static void report(const char *name, float32_t step)
{
    float32_t worst = 0.0f;
    float32_t sigma;
    float32_t mean;

    for (uint32_t n = 0U; n < SIG_LEN; n++)
    {
        error[n] = back[n] - sig[n];

        if (fabsf(error[n]) > worst)
        {
            worst = fabsf(error[n]);
        }
    }

    arm_std_f32(error, SIG_LEN, &sigma);
    arm_mean_f32(error, SIG_LEN, &mean);

    printf("%-22s %11.3f %12.3e %12.3f\r\n", name, (double)(worst / step),
           (double)sigma, (double)(sigma / (step / sqrtf(12.0f))));
}

int main(void)
{
    config_app();
    probe_reset();

    printf("\r\n%lu samples, peak 0.900, inside what q15 holds\r\n\r\n",
           (unsigned long)SIG_LEN);

    printf("%-22s %11s %12s %12s\r\n",
           "", "worst, steps", "sigma", "vs step/sqrt12");

    arm_float_to_q15(sig, truncated, SIG_LEN);
    arm_q15_to_float(truncated, back, SIG_LEN);
    report("arm_float_to_q15", 1.0f / Q15_ONE);

    float_to_q15_rounded(sig, rounded, SIG_LEN);
    arm_q15_to_float(rounded, back, SIG_LEN);
    report("the same, rounded", 1.0f / Q15_ONE);

    printf("\r\none step of error against half a step: the whole difference"
           " is a 0.5\r\nadded before the cast.\r\n");

    arm_float_to_q31(sig, wide, SIG_LEN);
    arm_q31_to_float(wide, back, SIG_LEN);

    float32_t worst31 = 0.0f;

    for (uint32_t n = 0U; n < SIG_LEN; n++)
    {
        float32_t gap = fabsf(back[n] - sig[n]);

        if (gap > worst31)
        {
            worst31 = gap;
        }
    }

    printf("\r\nand the same trip through q31:\r\n\r\n");
    printf("  %-24s %12.3e\r\n", "q31 step", (double)(1.0f / Q31_ONE));
    printf("  %-24s %12.3e\r\n", "worst round trip error", (double)worst31);

    printf("\r\nthat is not a statement about q31: a float32 carries 24 bits"
           " of mantissa\r\nand a q31 carries 31, so the source ran out of"
           " resolution first.\r\n");

    while (1)
    {
        for (uint32_t n = 0U; n < SIG_LEN; n++)
        {
            float32_t t = (float32_t)truncated[n] / Q15_ONE;
            float32_t r = (float32_t)rounded[n] / Q15_ONE;

            g_f32 = sig[n];
            g_q15 = t;
            g_q31 = r;
            g_err = t - sig[n];
            probe_step();
        }
    }
}