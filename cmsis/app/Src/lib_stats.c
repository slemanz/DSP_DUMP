/*
 * The statistics group, used for the questions it is actually for.
 *
 * These are the cheapest routines in the library and the ones most worth
 * reaching for, because they answer questions about a signal that would
 * otherwise be answered by looking at a plot and guessing.
 *
 * There is one naming trap in the group and it is worth meeting before it
 * costs an afternoon. arm_power_f32 does not return power in the sense the word
 * usually carries. It returns the sum of the squares, not the mean of them, so
 * it grows with the length of the block. The mean of the squares is what
 * arm_rms_f32 squares back down from.
 *
 *     make lib_stats && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "testsig.h"

#define CLIP_AT     0.6f

static float32_t clipped[SIG_LEN];

int main(void)
{
    config_app();
    probe_reset();

    float32_t mean;
    float32_t rms;
    float32_t sd;
    float32_t var;
    float32_t power;
    float32_t peak;
    float32_t trough;
    float32_t absmax;
    uint32_t  where;
    uint32_t  where_min;
    uint32_t  where_abs;

    arm_mean_f32(sig, SIG_LEN, &mean);
    arm_rms_f32(sig, SIG_LEN, &rms);
    arm_std_f32(sig, SIG_LEN, &sd);
    arm_var_f32(sig, SIG_LEN, &var);
    arm_power_f32(sig, SIG_LEN, &power);
    arm_max_f32(sig, SIG_LEN, &peak, &where);
    arm_min_f32(sig, SIG_LEN, &trough, &where_min);
    arm_absmax_f32(sig, SIG_LEN, &absmax, &where_abs);

    printf("\r\n%lu samples\r\n\r\n", (unsigned long)SIG_LEN);

    printf("  %-16s %12.6f   %s\r\n", "arm_mean_f32", (double)mean,
           "the dc offset");
    printf("  %-16s %12.6f   %s\r\n", "arm_rms_f32", (double)rms,
           "size, ignoring sign");
    printf("  %-16s %12.6f   %s\r\n", "arm_std_f32", (double)sd,
           "size of the varying part");
    printf("  %-16s %12.6f   %s\r\n", "arm_var_f32", (double)var,
           "the same, squared");
    printf("  %-16s %12.6f   %s\r\n", "arm_power_f32", (double)power,
           "SUM of squares, not the mean");
    printf("  %-16s %12.6f   at %lu\r\n", "arm_max_f32", (double)peak,
           (unsigned long)where);
    printf("  %-16s %12.6f   at %lu\r\n", "arm_min_f32", (double)trough,
           (unsigned long)where_min);
    printf("  %-16s %12.6f   at %lu\r\n", "arm_absmax_f32", (double)absmax,
           (unsigned long)where_abs);

    printf("\r\nthe trap in that list:\r\n");
    printf("  power / len   %12.6f\r\n", (double)(power / (float32_t)SIG_LEN));
    printf("  rms squared   %12.6f\r\n", (double)(rms * rms));
    printf("\r\nthose two agree, which is the definition arm_rms_f32 uses."
           " arm_power_f32\r\nskips the division, so it is not comparable"
           " between block lengths.\r\n");

    printf("\r\nand a second trap in the same group, found by checking the"
           " identity\r\nthe statistics chapter established:\r\n\r\n");
    printf("  %-34s %12.6f\r\n", "rms^2 - mean^2, the variance",
           (double)((rms * rms) - (mean * mean)));
    printf("  %-34s %12.6f\r\n", "arm_var_f32 returned", (double)var);
    printf("  %-34s %12.6f\r\n", "the first, times N/(N-1)",
           (double)(((rms * rms) - (mean * mean))
                    * (float32_t)SIG_LEN / (float32_t)(SIG_LEN - 1U)));

    printf("\r\narm_var_f32 divides by N-1, not N: right when the block is a"
           " sample of\r\nsomething larger, wrong when the block is the whole"
           " signal.\r\n");

    /* a real question: is this signal clipping */
    arm_clip_f32(sig, clipped, -CLIP_AT, CLIP_AT, SIG_LEN);

    uint32_t touched = 0U;

    for (uint32_t n = 0U; n < SIG_LEN; n++)
    {
        if (fabsf(clipped[n]) >= (CLIP_AT - 1.0e-6f))
        {
            touched++;
        }
    }

    float32_t clipped_rms;
    arm_rms_f32(clipped, SIG_LEN, &clipped_rms);

    printf("\r\nusing them for something: run the signal into a rail at %.2f"
           "\r\n\r\n", (double)CLIP_AT);
    printf("  %-26s %10lu of %lu\r\n", "samples sitting on the rail",
           (unsigned long)touched, (unsigned long)SIG_LEN);
    printf("  %-26s %10.2f %%\r\n", "which is",
           (double)(100.0f * (float32_t)touched / (float32_t)SIG_LEN));
    printf("  %-26s %10.6f\r\n", "rms before", (double)rms);
    printf("  %-26s %10.6f\r\n", "rms after", (double)clipped_rms);

    printf("\r\ncounting samples at the rail is the cheapest clipping"
           " detector there is:\r\none comparison per sample, worth running"
           " before anything else is believed.\r\n");

    while (1)
    {
        for (uint32_t n = 0U; n < SIG_LEN; n++)
        {
            g_in  = sig[n];
            g_out = clipped[n];
            g_ref = rms;
            g_gap = clipped[n] - sig[n];
            probe_step();
        }
    }
}