/*
 * The prescalers, and the multiplier hiding among them.
 *
 * The core runs at HCLK and the peripherals do not. APB1 tops out at 50 MHz on
 * this part, so at 100 MHz it has to be divided, and everything hanging off it
 * sees the divided clock. Uart baud dividers are computed from it, which is
 * exactly why a clock change that forgets the uart turns the terminal to
 * nonsense.
 *
 * Then there is the rule that is easy to lose an afternoon to. Timers on an APB
 * bus do not run at that bus's clock. When the prescaler is 1 they do, and the
 * moment it is anything else the timer clock is doubled back up. So on this
 * configuration PCLK1 is 50 MHz and TIM2 is clocked at 100.
 *
 * It is not a rounding or an approximation, it is a multiplier drawn into the
 * clock tree, put there so that slowing a bus down for its peripherals does not
 * also halve the resolution of its timers. Every timer period is computed from
 * the doubled number, and the next chapter depends on getting this right.
 *
 * The table is worked out rather than programmed. Setting APB1 to /1 while the
 * core is at 100 MHz would put the bus at twice its rating, and uart2 is on
 * that bus, so the terminal would garble halfway through this sentence.
 *
 *     make clock_buses && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "driver_clock.h"

static const uint32_t divisors[] = { 1U, 2U, 4U, 8U, 16U };

int main(void)
{
    config_app();
    probe_reset();

    uint32_t hclk = clock_hclk();

    printf("\r\nHCLK is %lu Hz\r\n\r\n", (unsigned long)hclk);

    printf("%-10s %12s %14s %9s   %s\r\n",
           "APB1 pre", "PCLK1", "TIM clock", "doubled", "bus legal");

    for (uint32_t k = 0U; k < ARRAY_LEN(divisors); k++)
    {
        uint32_t pclk = hclk / divisors[k];
        uint32_t tim  = (divisors[k] == 1U) ? pclk : (pclk * 2U);

        printf("/%-9lu %12lu %14lu %9s   %s\r\n",
               (unsigned long)divisors[k], (unsigned long)pclk,
               (unsigned long)tim, (tim == pclk) ? "no" : "yes",
               (pclk <= PCLK1_MAX_HZ) ? "ok" : "OVER 50 MHz");
    }

    printf("\r\nthe /1 row is the only one where the timer sees what the bus"
           " sees,\r\n");
    printf("and at this frequency it is also the only one that is not"
           " allowed.\r\n");

    printf("\r\nwhat this configuration is actually running:\r\n\r\n");
    printf("  %-14s %10lu Hz\r\n", "HCLK", (unsigned long)hclk);
    printf("  %-14s %10lu Hz   uart2, tim2..tim5, i2c\r\n", "PCLK1",
           (unsigned long)clock_pclk1());
    printf("  %-14s %10lu Hz   the timers on APB1\r\n", "TIM on APB1",
           (unsigned long)clock_timer_pclk1());
    printf("  %-14s %10lu Hz   uart1, uart6, spi1, adc1\r\n", "PCLK2",
           (unsigned long)clock_pclk2());
    printf("  %-14s %10lu Hz   the timers on APB2\r\n", "TIM on APB2",
           (unsigned long)clock_timer_pclk2());

    printf("\r\nso a 1 kHz period on tim2 is %lu counts, not %lu.\r\n",
           (unsigned long)(clock_timer_pclk1() / 1000U),
           (unsigned long)(clock_pclk1() / 1000U));
    printf("getting that wrong gives a timer that runs at exactly twice the"
           " rate\r\nyou asked for, which is the easiest bug in the next"
           " chapter to reach.\r\n");

    while (1)
    {
        for (uint32_t k = 0U; k < ARRAY_LEN(divisors); k++)
        {
            uint32_t pclk = hclk / divisors[k];

            g_mhz  = (float32_t)pclk / 1.0e6f;
            g_gain = (float32_t)((divisors[k] == 1U) ? pclk : (pclk * 2U))
                     / 1.0e6f;
            probe_step();
        }
    }
}