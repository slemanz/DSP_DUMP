/*
 * The dip at the end of every block.
 *
 * The lesson collects a block through this ring, plots it, and there is a spike
 * down to zero at the end of each one. The narration says "that dip is when we
 * reach the end of the data block" and moves on. It is not a property of block
 * processing. It is the missing slot.
 *
 * The ring holds SIZE - 1. The collecting loop puts until put fails, which
 * stores SIZE - 1 items. The reading loop then asks for SIZE of them, and the
 * last get finds the ring empty, returns a failure nobody checks, and writes
 * nothing. The destination keeps whatever was there, and the destination was
 * cleared to zero first, so the last sample of every block is exactly 0.
 *
 * A returned status that nobody reads is not a safety feature.
 *
 * Three ways out, all measured below. Ask for what it holds, declare it one
 * bigger, or stop using the pointers as the count.
 *
 *     make blk_dip && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "fifo.h"

#define BLOCK       FIFO_SIZE

static float32_t collected[BLOCK];

/* the ramp standing in for a sensor, so a wrong sample is obvious */
static fifo_item_t source(uint32_t n)
{
    return (fifo_item_t)(100U + n);
}

static void clear(void)
{
    for (uint32_t i = 0U; i < BLOCK; i++)
    {
        collected[i] = 0.0f;
    }
}

/* fill the ring the way the lesson does, until it says no */
static uint32_t fill(void)
{
    uint32_t n = 0U;

    fifo_init();

    while (fifo_put(source(n)) == FIFO_OK)
    {
        n++;
    }

    return n;
}

/*
 * Drains into the block the way the lesson does: get writes through the pointer
 * only when it succeeds, and the caller never looks at what it returned. So on
 * the failing call nothing is written and the destination keeps the zero that
 * clearing the buffer left there.
 */
static void drain_lesson_style(uint32_t want)
{
    fifo_item_t item = 0U;

    for (uint32_t i = 0U; i < want; i++)
    {
        if (fifo_get(&item) == FIFO_OK)
        {
            collected[i] = (float32_t)item;
        }
    }
}

/* the same, but stopping when the ring says it is done */
static uint32_t drain_checking_status(uint32_t want)
{
    fifo_item_t item = 0U;
    uint32_t    got = 0U;

    for (uint32_t i = 0U; i < want; i++)
    {
        if (fifo_get(&item) != FIFO_OK)
        {
            break;
        }

        collected[i] = (float32_t)item;
        got++;
    }

    return got;
}

static void report(const char *what, uint32_t len)
{
    printf("  %-26s", what);

    for (uint32_t i = len - 4U; i < len; i++)
    {
        printf(" %8.1f", (double)collected[i]);
    }

    printf("\r\n");
}

int main(void)
{
    config_app();
    probe_reset();

    uint32_t held = fill();

    printf("\r\nthe array is %lu, the ring holds %lu\r\n\r\n",
           (unsigned long)FIFO_SIZE, (unsigned long)held);

    printf("the last four samples of the block\r\n\r\n");
    printf("  %-26s %8s %8s %8s %8s\r\n", "", "n-4", "n-3", "n-2", "n-1");

    clear();
    (void)fill();
    drain_lesson_style(BLOCK);
    report("asking for 64, lesson way", BLOCK);

    clear();
    (void)fill();
    uint32_t got = drain_checking_status(BLOCK);
    report("asking for 64, checking", BLOCK);

    clear();
    (void)fill();
    drain_lesson_style(held);
    report("asking for 63", held);

    printf("\r\nthe first row ends in 0.0 and that zero is the dip. it is not"
           " a sample,\r\nit is the buffer's own cleared value showing"
           " through a get that failed.\r\n");

    printf("\r\nthe middle row checked the status and stopped at %lu of"
           " %lu, and its\r\narray is identical to the first one. reading the"
           " return value bought\r\ninformation, not correctness. the data is"
           " wrong either way; the\r\ndifference is whether the program"
           " knows.\r\n",
           (unsigned long)got, (unsigned long)BLOCK);

    printf("\r\nthe last row asked for what the ring has. no dip, no short"
           " block,\r\nnothing to explain away.\r\n");

    printf("\r\nthree ways to make the last row the only one possible:\r\n");
    printf("  1. ask for fifo_capacity() instead of FIFO_SIZE\r\n");
    printf("  2. declare the array one bigger than the block you want\r\n");
    printf("  3. keep a count and stop using pointer equality for full\r\n");
    printf("\r\nthis module takes the first, because it needs no change to the"
           " ring\r\nand it makes the lost slot part of the interface instead"
           " of a secret.\r\n");

    clear();
    (void)fill();
    drain_lesson_style(held);

    while (1)
    {
        for (uint32_t i = 0U; i < BLOCK; i++)
        {
            g_in    = (float32_t)source(i);
            g_block = collected[i];
            g_gap   = collected[i] - (float32_t)source(i);
            probe_step();
        }
    }
}