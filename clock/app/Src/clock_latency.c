/*
 * What the memory in front of the core costs.
 *
 * Flash does not answer at 100 MHz. It answers at something closer to 25, so
 * the core is told to wait a number of cycles before believing what is on the
 * bus. Three wait states is what a 3.3 V part needs at 100 MHz, and the
 * boundaries in the datasheet are 30, 64, 90 and 100 MHz.
 *
 * Below the required number the part is not faster, it is broken: the core
 * latches whatever happened to be on the bus and executes it. That is not
 * demonstrated here, because a demonstration of undefined behaviour teaches
 * nothing and can leave the board in a state that needs a hard reset to fix.
 * What is demonstrated is the other direction, where every extra wait state is
 * a real and measurable tax.
 *
 * The measurement runs with the prefetch and the caches switched off, on
 * purpose. Left on, they hide almost all of this and the table comes out
 * nearly flat, which is a real and useful result but it is the next app's
 * result. Here the question is what the raw memory costs, so the thing that
 * hides it is taken away.
 *
 * The workload is the same 16384 multiply accumulates in every timing app.
 * Note that the number moving is cycles, not milliseconds. The clock has not
 * changed; the core is spending more cycles standing still.
 *
 *     make clock_latency && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "bench.h"
#include "driver_clock.h"
#include "driver_flash.h"

static const uint32_t wait_states[] = { 3U, 4U, 5U, 6U, 7U };

int main(void)
{
    config_app();
    probe_reset();
    bench_prepare();

    flash_art_config(DISABLE, DISABLE, DISABLE);

    printf("\r\ncore at %lu Hz, prefetch and caches off\r\n",
           (unsigned long)clock_hclk());
    printf("%lu multiply accumulates per run\r\n\r\n",
           (unsigned long)BENCH_MACS);

    printf("%6s %12s %12s %10s\r\n", "waits", "cycles", "us", "vs 3 waits");

    uint32_t base = 0U;

    for (uint32_t k = 0U; k < ARRAY_LEN(wait_states); k++)
    {
        flash_set_latency(wait_states[k]);

        uint32_t spent = bench_cycles();

        if (k == 0U)
        {
            base = spent;
        }

        printf("%6lu %12lu %12lu %9.3f\r\n",
               (unsigned long)wait_states[k], (unsigned long)spent,
               (unsigned long)((uint64_t)spent * 1000000U / clock_hclk()),
               (double)((float32_t)spent / (float32_t)base));

        g_cycles = (float32_t)spent;
        g_gain   = (float32_t)spent / (float32_t)base;
    }

    flash_set_latency(FLASH_LATENCY_100MHZ);

    printf("\r\nchecksum %.6f\r\n", (double)bench_checksum());
    printf("\r\nthree is the smallest legal number at this frequency, so this"
           " table\r\n");
    printf("only goes the wrong way. raise wait states before raising the"
           " clock,\r\n");
    printf("lower them after lowering it, and never the other way round.\r\n");
    printf("\r\nnow turn the caches back on and run clock_cache, which is the"
           "\r\nsame axis measured from the other end.\r\n");

    flash_art_config(ENABLE, ENABLE, ENABLE);

    while (1)
    {
        for (uint32_t k = 0U; k < ARRAY_LEN(wait_states); k++)
        {
            flash_art_config(DISABLE, DISABLE, DISABLE);
            flash_set_latency(wait_states[k]);

            uint32_t spent = bench_cycles();

            g_mhz    = (float32_t)clock_hclk() / 1.0e6f;
            g_cycles = (float32_t)spent;
            g_ms     = (float32_t)spent * 1000.0f / (float32_t)clock_hclk();
            g_gain   = (float32_t)spent / (float32_t)base;

            flash_set_latency(FLASH_LATENCY_100MHZ);
            flash_art_config(ENABLE, ENABLE, ENABLE);
            probe_step();
        }
    }
}