/*
 * The ring buffer, and the slot it does not have.
 *
 * Two pointers walk round an array. The write pointer advances on a put, the
 * read pointer on a get, and the buffer is empty when they are equal. That last
 * rule is what makes the structure work without a separate counter, and it is
 * also what costs a slot: the state where the pointers are equal has already
 * been spent on meaning empty, so it cannot also mean full. An array of 64
 * holds 63.
 *
 * Losing one slot out of 64 does not matter. Not knowing about it does, and the
 * next app is what happens when you do not.
 *
 *     make blk_fifo && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "fifo.h"

int main(void)
{
    config_app();
    probe_reset();

    fifo_init();

    printf("\r\nan array of %lu\r\n\r\n", (unsigned long)FIFO_SIZE);

    uint32_t stored = 0U;

    while (fifo_put((fifo_item_t)(stored + 1U)) == FIFO_OK)
    {
        stored++;
    }

    printf("  %-28s %6lu\r\n", "puts accepted", (unsigned long)stored);
    printf("  %-28s %6lu\r\n", "the array is", (unsigned long)FIFO_SIZE);
    printf("  %-28s %6lu\r\n", "what fifo_count says", (unsigned long)fifo_count());
    printf("  %-28s %6lu\r\n", "slot lost to the empty test", (unsigned long)(FIFO_SIZE - stored));

    fifo_item_t item = 0U;
    uint32_t    taken = 0U;
    uint8_t     ok = 1U;

    while (fifo_get(&item) == FIFO_OK)
    {
        taken++;

        if (item != (fifo_item_t)taken)
        {
            ok = 0U;
        }
    }

    printf("\r\n  %-28s %6lu\r\n", "gets that returned data", (unsigned long)taken);
    printf("  %-28s %6s\r\n", "came back in order", ok ? "yes" : "no");

    /* now go round the end, several times, to show the wrap is not special */
    fifo_init();

    uint32_t cycles = 0U;
    ok = 1U;

    for (uint32_t n = 0U; n < (FIFO_SIZE * 5U); n++)
    {
        if (fifo_put((fifo_item_t)n) != FIFO_OK)
        {
            ok = 0U;
        }

        if (fifo_get(&item) != FIFO_OK)
        {
            ok = 0U;
        }

        if (item != (fifo_item_t)n)
        {
            ok = 0U;
        }

        cycles++;
    }

    printf("\r\n  %-28s %6lu\r\n", "one in one out, repeated", (unsigned long)cycles);
    printf("  %-28s %6s\r\n", "every item came back right", ok ? "yes" : "no");
    printf("  %-28s %6lu\r\n", "left in the fifo", (unsigned long)fifo_count());

    printf("\r\nso the wrap is not a special case, it is the ordinary case"
           " arriving\r\nevery %lu items. what is special is the one slot,"
           " and it is special\r\nbecause it is silent.\r\n",
           (unsigned long)FIFO_SIZE);

    while (1)
    {
        g_in = (float32_t)fifo_capacity();
        probe_step();
    }
}