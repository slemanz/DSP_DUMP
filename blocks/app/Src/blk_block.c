/*
 * The other arrangement: collect first, then work.
 *
 * The handler stores the sample and returns. When enough of them have arrived
 * it raises a flag, and the main loop picks up the whole block and runs the
 * filter over it once. Nothing is computed at the sampling rate any more; it is
 * computed at the block rate, which is smaller by the block length.
 *
 * What that buys is the ability to call anything. arm_conv_f32, the DFT, a
 * matrix routine, the whole library takes a pointer and a length and none of it
 * fits inside a per sample handler.
 *
 * What it costs is latency. The first sample of a block waits for the last one
 * before anything happens to it, so the answer is a block length behind the
 * signal, and no amount of processor makes that smaller.
 *
 * This app runs the same signal through in blocks and holds the result against
 * the streaming answer from the previous one. They agree except at the seams,
 * which is the next app.
 *
 *     make blk_block && make load && make monitor
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
#define SCRATCH     (BLOCK + TAPS - 1U)

static float32_t naive[SIG_LEN];
static float32_t reference[SIG_LEN];
static float32_t scratch[SCRATCH];
static stream_fir_t fir;

int main(void)
{
    config_app();
    probe_reset();

    /* the answer to beat, one sample at a time */
    stream_init(&fir, lp_31, TAPS);

    for (uint32_t n = 0U; n < SIG_LEN; n++)
    {
        reference[n] = stream_step(&fir, sig_3tone[n]);
    }

    /*
     * block by block, each one convolved on its own, which is the obvious
     * thing to do and is wrong in a way worth seeing
     */
    for (uint32_t b = 0U; b < BLOCKS; b++)
    {
        arm_conv_f32(&sig_3tone[b * BLOCK], BLOCK, lp_31, TAPS, scratch);

        for (uint32_t i = 0U; i < BLOCK; i++)
        {
            naive[(b * BLOCK) + i] = scratch[i];
        }
    }

    printf("\r\n%lu samples in %lu blocks of %lu\r\n\r\n",
           (unsigned long)SIG_LEN, (unsigned long)BLOCKS,
           (unsigned long)BLOCK);

    printf("  %-32s %8lu\r\n", "filter runs, streaming",
           (unsigned long)SIG_LEN);
    printf("  %-32s %8lu\r\n", "filter runs, block", (unsigned long)BLOCKS);
    printf("  %-32s %8lu\r\n", "multiply accumulates either way",
           (unsigned long)(SIG_LEN * TAPS));
    printf("  %-32s %8lu samples\r\n", "latency, streaming", 0UL);
    printf("  %-32s %8lu samples\r\n", "latency, block",
           (unsigned long)BLOCK);

    /* where do they disagree */
    float32_t worst = 0.0f;
    uint32_t  bad = 0U;

    for (uint32_t n = 0U; n < SIG_LEN; n++)
    {
        float32_t gap = fabsf(naive[n] - reference[n]);

        if (gap > 1.0e-5f)
        {
            bad++;
        }

        if (gap > worst)
        {
            worst = gap;
        }
    }

    printf("\r\n  %-32s %8.6f\r\n", "worst gap against streaming",
           (double)worst);
    printf("  %-32s %8lu of %lu\r\n", "samples that disagree",
           (unsigned long)bad, (unsigned long)SIG_LEN);
    printf("  %-32s %8lu\r\n", "blocks with a damaged start",
           (unsigned long)(BLOCKS - 1U));
    printf("  %-32s %8lu\r\n", "damaged samples in each",
           (unsigned long)(bad / (BLOCKS - 1U)));
    printf("  %-32s %8lu\r\n", "and taps minus one is",
           (unsigned long)(TAPS - 1U));

    printf("\r\nthe damage is exactly the first %lu samples of every block"
           " after the\r\nfirst. each block was convolved as if the signal"
           " began there, so the\r\nfilter started from silence %lu times"
           " instead of once.\r\n",
           (unsigned long)(TAPS - 1U), (unsigned long)BLOCKS);
    printf("block zero is the exception, and not because it is fine: it is"
           " the one\r\nplace where starting from silence is correct, which"
           " is what streaming\r\ndoes too.\r\n");

    printf("\r\nthat is the seam, and the next app closes it.\r\n");

    while (1)
    {
        for (uint32_t n = 0U; n < SIG_LEN; n++)
        {
            g_in     = sig_3tone[n];
            g_stream = reference[n];
            g_block  = naive[n];
            g_gap    = naive[n] - reference[n];
            probe_step();
        }
    }
}