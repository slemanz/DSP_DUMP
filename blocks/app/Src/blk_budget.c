/*
 * Does it fit?
 *
 * The acquisition chapter ended by measuring how much of a sample period the
 * handler used just to store a number. This is the same question with the
 * filter added, and it has a definite answer for any given rate and tap count.
 *
 *     cycles available per sample = HCLK / fs
 *
 * That is the budget. If the filter costs more than that, no amount of
 * arranging helps and something has to give: fewer taps, a lower rate, a
 * cheaper representation, or a faster clock. Which of those is the subject of
 * the next two chapters.
 *
 * The filter is timed here rather than estimated, per sample streaming and per
 * block, so the table is measurement and not arithmetic.
 *
 *     make blk_budget && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "stream.h"
#include "testsig.h"
#include "kernels.h"
#include "driver_clock.h"
#include "driver_systick.h"

#define TAPS        LP_31_LEN
#define BLOCK       32U
#define REPEATS     8U

static const uint32_t rates[] = { 1000U, 8000U, 44100U, 96000U };

static float32_t out[SIG_LEN];
static float32_t reversed[TAPS];
static float32_t state[TAPS + BLOCK - 1U];
static stream_fir_t fir;

int main(void)
{
    config_app();
    probe_reset();

    uint32_t hclk = clock_get();

    /* one sample at a time, averaged over the whole signal a few times */
    stream_init(&fir, lp_31, TAPS);
    cycles_start();

    for (uint32_t r = 0U; r < REPEATS; r++)
    {
        for (uint32_t n = 0U; n < SIG_LEN; n++)
        {
            out[n] = stream_step(&fir, sig_3tone[n]);
        }
    }

    uint32_t stream_cycles = cycles_read() / (SIG_LEN * REPEATS);
    systick_init(TICK_HZ);

    /* and in blocks, through the library */
    for (uint32_t i = 0U; i < TAPS; i++)
    {
        reversed[i] = lp_31[TAPS - 1U - i];
    }

    arm_fir_instance_f32 inst;
    arm_fir_init_f32(&inst, TAPS, reversed, state, BLOCK);

    cycles_start();

    for (uint32_t r = 0U; r < REPEATS; r++)
    {
        for (uint32_t b = 0U; b < (SIG_LEN / BLOCK); b++)
        {
            arm_fir_f32(&inst, &sig_3tone[b * BLOCK], &out[b * BLOCK], BLOCK);
        }
    }

    uint32_t block_cycles = cycles_read() / (SIG_LEN * REPEATS);
    systick_init(TICK_HZ);

    printf("\r\ncore at %lu Hz, %lu taps\r\n\r\n", (unsigned long)hclk,
           (unsigned long)TAPS);

    printf("  %-34s %8lu cycles\r\n", "stream_step, per sample",
           (unsigned long)stream_cycles);
    printf("  %-34s %8lu cycles\r\n", "arm_fir_f32, per sample",
           (unsigned long)block_cycles);
    printf("  %-34s %8.2f\r\n", "cycles per multiply accumulate, block",
           (double)((float32_t)block_cycles / (float32_t)TAPS));

    printf("\r\n%10s %14s %12s %12s %s\r\n",
           "fs", "budget", "streaming", "block", "verdict");

    for (uint32_t k = 0U; k < ARRAY_LEN(rates); k++)
    {
        uint32_t budget = hclk / rates[k];

        printf("%10lu %14lu %11lu%% %11lu%% %s\r\n",
               (unsigned long)rates[k], (unsigned long)budget,
               (unsigned long)(stream_cycles * 100U / budget),
               (unsigned long)(block_cycles * 100U / budget),
               (block_cycles < budget) ? "fits" : "DOES NOT FIT");

        g_block = (float32_t)budget;
    }

    printf("\r\nthe budget column is the whole period, and everything else the"
           " chip\r\nhas to do comes out of the same place: the handler"
           " itself, the\r\nconversion, whatever the main loop wanted.\r\n");

    printf("\r\nwhere the percentages get close to a hundred the answer is not"
           " to\r\noptimise the loop. it is that the design is wrong at that"
           " rate, and\r\nthe things that move it are fewer taps, a cheaper"
           " number format, or\r\ninstructions that do more than one multiply"
           " at a time.\r\n");

    while (1)
    {
        for (uint32_t n = 0U; n < SIG_LEN; n++)
        {
            g_in     = sig_3tone[n];
            g_stream = out[n];
            probe_step();
        }
    }
}