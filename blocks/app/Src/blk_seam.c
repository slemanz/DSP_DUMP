/*
 * Closing the seam.
 *
 * Convolving each block on its own restarts the filter at every boundary, and
 * what that loses is the first numTaps - 1 outputs of each block: the ones that
 * needed samples from before the block began.
 *
 * There are two standard repairs and they are the same repair.
 *
 * Overlap and save is the explicit one. Hand the filter the last numTaps - 1
 * samples of the previous block along with the new ones, convolve the longer
 * thing, and throw away the outputs that the overlap already covered. Nothing
 * is hidden and the arithmetic is visible.
 *
 * A state buffer is the tidy one. arm_fir_f32 keeps that overlap inside its
 * instance, so the caller passes only the new samples and gets back exactly as
 * many outputs, with the join handled where it cannot be forgotten. That is why
 * the function takes an instance, and it is what the filters chapter was
 * pointing at when one call of 200 matched two calls of 100.
 *
 * All three are run below against the streaming answer.
 *
 *     make blk_seam && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "stream.h"
#include "testsig.h"
#include "kernels.h"

#define TAPS        LP_31_LEN
#define BLOCK       32U
#define BLOCKS      (SIG_LEN / BLOCK)
#define OVERLAP     (TAPS - 1U)
#define LONG_LEN    (BLOCK + OVERLAP)
#define SCRATCH     (LONG_LEN + TAPS - 1U)

static float32_t reference[SIG_LEN];
static float32_t saved[SIG_LEN];
static float32_t stated[SIG_LEN];
static float32_t longer[LONG_LEN];
static float32_t scratch[SCRATCH];
static float32_t reversed[TAPS];
static float32_t state[TAPS + BLOCK - 1U];
static stream_fir_t fir;

static float32_t worst_gap(const float32_t *pA, const float32_t *pB,
                           uint32_t len)
{
    float32_t worst = 0.0f;

    for (uint32_t n = 0U; n < len; n++)
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

    stream_init(&fir, lp_31, TAPS);

    for (uint32_t n = 0U; n < SIG_LEN; n++)
    {
        reference[n] = stream_step(&fir, sig_3tone[n]);
    }

    /* overlap and save, spelled out */
    for (uint32_t i = 0U; i < LONG_LEN; i++)
    {
        longer[i] = 0.0f;
    }

    for (uint32_t b = 0U; b < BLOCKS; b++)
    {
        for (uint32_t i = 0U; i < BLOCK; i++)
        {
            longer[OVERLAP + i] = sig_3tone[(b * BLOCK) + i];
        }

        arm_conv_f32(longer, LONG_LEN, lp_31, TAPS, scratch);

        /* the first OVERLAP outputs are the ones the overlap already paid for */
        for (uint32_t i = 0U; i < BLOCK; i++)
        {
            saved[(b * BLOCK) + i] = scratch[OVERLAP + i];
        }

        /* carry the tail of this block into the front of the next */
        for (uint32_t i = 0U; i < OVERLAP; i++)
        {
            longer[i] = longer[BLOCK + i];
        }
    }

    /* the same thing, with the library keeping the overlap */
    for (uint32_t i = 0U; i < TAPS; i++)
    {
        reversed[i] = lp_31[TAPS - 1U - i];
    }

    arm_fir_instance_f32 inst;
    arm_fir_init_f32(&inst, TAPS, reversed, state, BLOCK);

    for (uint32_t b = 0U; b < BLOCKS; b++)
    {
        arm_fir_f32(&inst, &sig_3tone[b * BLOCK], &stated[b * BLOCK], BLOCK);
    }

    printf("\r\n%lu samples, %lu blocks of %lu, %lu taps\r\n\r\n",
           (unsigned long)SIG_LEN, (unsigned long)BLOCKS,
           (unsigned long)BLOCK, (unsigned long)TAPS);

    printf("  %-34s %12s %10s\r\n", "", "worst gap", "carried");
    printf("  %-34s %12.9f %10lu\r\n", "overlap and save",
           (double)worst_gap(saved, reference, SIG_LEN),
           (unsigned long)OVERLAP);
    printf("  %-34s %12.9f %10lu\r\n", "arm_fir_f32 state buffer",
           (double)worst_gap(stated, reference, SIG_LEN),
           (unsigned long)(TAPS - 1U));
    printf("  %-34s %12.9f %10lu\r\n", "the two against each other",
           (double)worst_gap(saved, stated, SIG_LEN), 0UL);

    printf("\r\nboth are the streaming answer. they are the same idea written"
           " twice:\r\nsomething has to remember the last %lu samples across"
           " the boundary,\r\nand the only question is whether you do it or the"
           " library does.\r\n", (unsigned long)OVERLAP);

    printf("\r\nthe state buffer arm_fir_f32 wants is numTaps + blockSize - 1"
           "\r\nfloats, which is %lu here. that is what it is for.\r\n",
           (unsigned long)(TAPS + BLOCK - 1U));

    while (1)
    {
        for (uint32_t n = 0U; n < SIG_LEN; n++)
        {
            g_in     = sig_3tone[n];
            g_stream = reference[n];
            g_block  = stated[n];
            g_gap    = stated[n] - reference[n];
            probe_step();
        }
    }
}