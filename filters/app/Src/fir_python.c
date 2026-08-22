/*
 * Closing the loop on the design tool.
 *
 * A filter designed on a laptop is worth nothing until the target agrees with
 * it. The usual way that agreement breaks is not the mathematics, it is the
 * handover: a column copied out of the wrong variable, a length that was the
 * output length and not the input length, a cutoff normalised against the
 * wrong number.
 *
 * So tools/design.py writes the taps and the test signal into C, and it also
 * writes down the answer it got for the same input through the same kernel.
 * ref_lp50 is numpy's convolve, computed in double precision on the host and
 * carried here as a constant. This app runs the convolution three ways on the
 * target and holds all three against it.
 *
 * There is nothing to see on the graph. The whole content is one column of
 * differences that should be at the size of float rounding and nothing larger.
 *
 *     make fir_python && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "filters.h"
#include "kernels.h"
#include "testsig.h"

#define TAPS        LP_50_LEN
#define OUT_LEN     (SIG_LEN + TAPS - 1U)

static float32_t mine[OUT_LEN];
static float32_t theirs[OUT_LEN];

static float32_t worst_gap(const float32_t *pA, const float32_t *pB, uint32_t len)
{
    float32_t worst = 0.0f;
    uint32_t  n;

    for (n = 0U; n < len; n++)
    {
        float32_t gap = fabsf(pA[n] - pB[n]);

        if (gap > worst)
        {
            worst = gap;
        }
    }

    return worst;
}

int main(void)
{
    config_app();
    probe_reset();

    printf("\r\n%u samples through %u taps gives %u points\r\n",
           (unsigned)SIG_LEN, (unsigned)TAPS, (unsigned)OUT_LEN);
    printf("ref_lp50 is %u points long, written by tools/design.py\r\n\r\n",
           (unsigned)REF_LP50_LEN);

    fir_apply(sig_3tone, SIG_LEN, lp_50, TAPS, mine);
    arm_conv_f32(sig_3tone, SIG_LEN, lp_50, TAPS, theirs);

    float32_t largest = 0.0f;

    for (uint32_t n = 0U; n < OUT_LEN; n++)
    {
        if (fabsf(ref_lp50[n]) > largest)
        {
            largest = fabsf(ref_lp50[n]);
        }
    }

    float32_t gap_mine   = worst_gap(mine, ref_lp50, OUT_LEN);
    float32_t gap_cmsis  = worst_gap(theirs, ref_lp50, OUT_LEN);
    float32_t gap_between = worst_gap(mine, theirs, OUT_LEN);

    printf("largest sample of the answer:      %.6f\r\n", (double)largest);
    printf("fir_apply against numpy:           %.9f\r\n", (double)gap_mine);
    printf("arm_conv_f32 against numpy:        %.9f\r\n", (double)gap_cmsis);
    printf("fir_apply against arm_conv_f32:    %.9f\r\n", (double)gap_between);
    printf("\r\nrelative to the signal, the worst of those is %.3e\r\n",
           (double)(gap_mine / largest));

    printf("\r\nthe cutoff, checked rather than trusted\r\n");
    printf("  design.py asked for 50 Hz\r\n");
    printf("  the taps give -6 dB at ");

    for (float32_t f = 1.0f; f < 200.0f; f += 0.1f)
    {
        if (fir_gain(lp_50, TAPS, f, (float32_t)TESTSIG_FS_HZ) <= 0.5f)
        {
            printf("%.1f Hz\r\n", (double)f);
            break;
        }
    }

    while (1)
    {
        for (uint32_t n = 0U; n < OUT_LEN; n++)
        {
            g_x   = (n < SIG_LEN) ? sig_3tone[n] : 0.0f;
            g_y   = mine[n];
            g_ref = ref_lp50[n];
            g_mag = mine[n] - ref_lp50[n];
            probe_step();
        }
    }
}