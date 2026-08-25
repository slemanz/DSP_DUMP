/*
 * What the pll actually buys, and what it does not.
 *
 * The same workload is run at 16 MHz on the internal oscillator and at 100 MHz
 * on the crystal and pll, in one session, switching between them. Two numbers
 * come back for each, and they behave completely differently:
 *
 *   cycles       stays where it is. a cycle is a unit of work, and the work
 *                did not change because the clock did.
 *   milliseconds falls by the ratio of the frequencies, because that is all a
 *                frequency is: how long a cycle lasts.
 *
 * Keeping those apart is the whole point. Raising the clock does not make the
 * code more efficient and never will; it makes the same inefficiency take less
 * time. Every measurement in this repository that is quoted in cycles is
 * therefore portable across this change, and every one quoted in milliseconds
 * is not.
 *
 * Two practical notes. The cycle counter is 24 bits and counts at HCLK, so its
 * ceiling is 1.048 s at 16 MHz and 0.168 s at 100: the faster the core, the
 * shorter the window it can measure. And the uart survives the switch only
 * because its divider is recomputed from clock_pclk1() afterwards, which is the
 * step the course does by editing a constant and rebuilding.
 *
 *     make clock_speed && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "bench.h"
#include "driver_clock.h"
#include "driver_flash.h"
#include "driver_systick.h"

static uint32_t measure(const char *what)
{
    uint32_t hclk  = clock_hclk();
    uint32_t spent = bench_cycles();
    uint32_t us    = (uint32_t)((uint64_t)spent * 1000000U / hclk);

    printf("%-12s %11lu %12lu %12lu %12lu\r\n", what,
           (unsigned long)(hclk / 1000000U), (unsigned long)spent,
           (unsigned long)us,
           (unsigned long)((uint64_t)SYSTICK_MAX_RELOAD * 1000U / hclk));

    g_mhz    = (float32_t)hclk / 1.0e6f;
    g_cycles = (float32_t)spent;
    g_ms     = (float32_t)us / 1000.0f;

    return spent;
}

int main(void)
{
    config_app();
    probe_reset();
    bench_prepare();

    printf("\r\n%lu multiply accumulates, run at both clocks\r\n\r\n",
           (unsigned long)BENCH_MACS);
    printf("%-12s %11s %12s %12s %12s\r\n",
           "clock", "MHz", "cycles", "us", "counter ms");

    clock_config_hsi();
    config_app();
    uint32_t slow = measure("HSI");

    if (clock_config_100mhz() == 0U)
    {
        config_app();
        printf("\r\nthe crystal did not start. everything below would be a"
               " lie, so stopping here.\r\n");

        while (1)
        {
            probe_step();
        }
    }

    config_app();
    uint32_t fast = measure("HSE + PLL");

    printf("\r\ncycles moved by %.3fx and time moved by %.3fx\r\n",
           (double)((float32_t)fast / (float32_t)slow),
           (double)(((float32_t)fast / 100.0f) / ((float32_t)slow / 16.0f)));

    printf("\r\nthe cycle column should be flat. if it is not, something other"
           " than\r\n");
    printf("the frequency changed too, and on this part that means the flash"
           " wait\r\nstates that had to go up with it.\r\n");

    printf("\r\nchecksum %.6f\r\n", (double)bench_checksum());

    g_gain = (float32_t)clock_hclk() / 16.0e6f;

    while (1)
    {
        clock_config_hsi();
        config_app();
        (void)measure("HSI");
        probe_step();

        if (clock_config_100mhz() != 0U)
        {
            config_app();
            (void)measure("HSE + PLL");
        }

        probe_step();
    }
}