/*
 * The trade, with numbers on it.
 *
 * Streaming and blocking do the same arithmetic. What separates them is when it
 * happens, and that shows up in two places: how far behind the signal the
 * answer is, and how much has to fit into one interruption.
 *
 * Latency is the block length, in samples, divided by the sampling rate. That
 * is not an implementation detail and no processor makes it smaller: the first
 * sample of a block cannot be filtered until the last one has arrived.
 *
 * The peak load is the other one. Streaming spreads the work evenly, numTaps
 * multiply accumulates every sample forever. Blocking does nothing for a whole
 * block and then does all of it at once, so the same average work arrives as a
 * spike that has to fit between two samples.
 *
 * The table is that pair for several block sizes at a fixed rate. Both columns
 * cannot be small at the same time, which is the whole content.
 *
 *     make blk_latency && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "stream.h"
#include "testsig.h"
#include "kernels.h"

#define TAPS        LP_31_LEN
#define RATE_HZ     2000U

static const uint32_t blocks[] = { 1U, 8U, 32U, 64U, 128U, 256U };

int main(void)
{
    config_app();
    probe_reset();

    printf("\r\n%lu taps at %lu Hz\r\n\r\n", (unsigned long)TAPS,
           (unsigned long)RATE_HZ);

    printf("%8s %12s %14s %14s %12s\r\n",
           "block", "latency ms", "macs/sample", "macs/burst", "bursts/s");

    for (uint32_t k = 0U; k < ARRAY_LEN(blocks); k++)
    {
        uint32_t b = blocks[k];

        printf("%8lu %12.2f %14lu %14lu %12lu\r\n",
               (unsigned long)b,
               (double)((float32_t)b * 1000.0f / (float32_t)RATE_HZ),
               (unsigned long)TAPS,
               (unsigned long)(b * TAPS),
               (unsigned long)(RATE_HZ / b));

        g_block = (float32_t)(b * TAPS);
        g_gap   = (float32_t)b * 1000.0f / (float32_t)RATE_HZ;
    }

    printf("\r\nthe third column never moves. the work per sample is the"
           " filter, and\r\nrearranging when it happens does not change how"
           " much of it there is.\r\n");

    printf("\r\nblock of 1 is streaming, and it is in the table to make that"
           " point:\r\nthese are not two techniques, they are one parameter.\r\n");

    printf("\r\nthe fourth column is what has to fit between two samples if the"
           " burst\r\nis not allowed to overrun, and it grows with the block"
           " while the\r\ntime available does not.\r\n");

    printf("\r\nso the block is chosen from whichever end is binding. a control"
           " loop\r\npicks it from the latency column. an audio path with a"
           " deadline picks\r\nit from the burst column. nothing picks it from"
           " the third one.\r\n");

    printf("\r\nthere is a third reason and it is the practical one: below"
           " some block\r\nsize the library routines lose to a plain loop,"
           " because their setup\r\ncosts more than the work. that one is"
           " measured in the cmsis chapter.\r\n");

    while (1)
    {
        for (uint32_t k = 0U; k < ARRAY_LEN(blocks); k++)
        {
            g_in    = (float32_t)blocks[k];
            g_block = (float32_t)(blocks[k] * TAPS);
            g_gap   = (float32_t)blocks[k] * 1000.0f / (float32_t)RATE_HZ;
            probe_step();
        }
    }
}