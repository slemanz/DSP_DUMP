/*
 * The part of the speedup nobody mentions.
 *
 * Three wait states means the flash answers at a quarter of the core's rate, so
 * a core fetching every instruction from flash would run at a quarter speed no
 * matter what the pll says. The ART accelerator is what stops that: a prefetch
 * queue that reads ahead down the straight parts, an instruction cache that
 * holds recently fetched lines, and a data cache for constants.
 *
 * The four combinations below are the same core at the same 100 MHz running
 * the same arithmetic. Everything that separates them is memory.
 *
 * This is worth knowing in the right order. The pll is the famous part of the
 * speedup and it is bounded: 16 to 100 MHz is 6.25 times, and no more exists.
 * The caches are the unglamorous part and on a flash bound loop they are worth
 * a similar factor, and they cost three bits in a register.
 *
 *     make clock_cache && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "bench.h"
#include "driver_clock.h"
#include "driver_flash.h"

typedef struct
{
    uint8_t     prefetch;
    uint8_t     icache;
    uint8_t     dcache;
    const char *name;
} art_t;

static const art_t modes[] =
{
    { DISABLE, DISABLE, DISABLE, "nothing" },
    { ENABLE,  DISABLE, DISABLE, "prefetch" },
    { DISABLE, ENABLE,  ENABLE,  "caches" },
    { ENABLE,  ENABLE,  ENABLE,  "everything" },
};

int main(void)
{
    config_app();
    probe_reset();
    bench_prepare();

    printf("\r\ncore at %lu Hz, %lu wait states, %lu multiply accumulates\r\n\r\n",
           (unsigned long)clock_hclk(), (unsigned long)flash_get_latency(),
           (unsigned long)BENCH_MACS);

    printf("%-12s %12s %12s %12s\r\n", "art", "cycles", "us", "vs nothing");

    uint32_t base = 0U;

    for (uint32_t k = 0U; k < ARRAY_LEN(modes); k++)
    {
        flash_art_config(modes[k].prefetch, modes[k].icache, modes[k].dcache);

        uint32_t spent = bench_cycles();

        if (k == 0U)
        {
            base = spent;
        }

        printf("%-12s %12lu %12lu %11.2fx\r\n", modes[k].name,
               (unsigned long)spent,
               (unsigned long)((uint64_t)spent * 1000000U / clock_hclk()),
               (double)((float32_t)base / (float32_t)spent));
    }

    flash_art_config(ENABLE, ENABLE, ENABLE);

    printf("\r\nchecksum %.6f\r\n", (double)bench_checksum());
    printf("\r\nsame clock, same instructions, same data. the whole difference"
           " is\r\nwhether the core had to wait for them.\r\n");

    while (1)
    {
        for (uint32_t k = 0U; k < ARRAY_LEN(modes); k++)
        {
            flash_art_config(modes[k].prefetch, modes[k].icache,
                             modes[k].dcache);

            uint32_t spent = bench_cycles();

            g_mhz    = (float32_t)clock_hclk() / 1.0e6f;
            g_cycles = (float32_t)spent;
            g_ms     = (float32_t)spent * 1000.0f / (float32_t)clock_hclk();
            g_gain   = (float32_t)base / (float32_t)spent;

            flash_art_config(ENABLE, ENABLE, ENABLE);
            probe_step();
        }
    }
}