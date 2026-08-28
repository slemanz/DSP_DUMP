/*
 * Now the rate is a number somebody chose.
 *
 * A timer rolls over at a rate set by two dividers, its interrupt fires, and
 * the handler takes one sample. From here on fs is a decision rather than a
 * consequence, and every frequency computed downstream inherits it.
 *
 * The whole app is really about one line, the one that says which clock the
 * period is computed from. Timers on a divided APB bus run at twice that bus.
 * PCLK1 here is 50 MHz and TIM2 counts at 100, so a period worked out from
 * PCLK1 gives a timer running at exactly twice the rate asked for, and there is
 * nothing about it that looks wrong. The table below computes it both ways and
 * measures which one the hardware agreed with.
 *
 * The other number worth watching is the jitter. The handler runs after the
 * interrupt is taken, and how long that takes depends on what the core was
 * doing, so the gap between samples is not perfectly even. The next app takes
 * the CPU out of the path entirely.
 *
 *     make adc_timer && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "timer.h"
#include "driver_adc.h"
#include "driver_clock.h"
#include "driver_systick.h"

#define TARGET_HZ       2000U
#define WINDOW_MS       500U

static volatile uint32_t g_samples;
static volatile uint32_t g_latest;
static volatile uint32_t g_worst_gap;
static volatile uint32_t g_best_gap;

void TIM2_IRQHandler(void)
{
    static uint32_t last;

    timer_clear_update(TIM2);

    /* how far the counter got past zero before this handler read it, which is
       the interrupt latency in timer counts */
    uint32_t now = TIM2->CNT;

    if (now > g_worst_gap) { g_worst_gap = now; }
    if (now < g_best_gap)  { g_best_gap = now; }

    last = now;
    (void)last;

    ADC1->CR2 |= ADC_CR2_SWSTART;
    g_latest = adc_read();
    g_samples++;
}

int main(void)
{
    config_app();
    probe_reset();

    g_best_gap = 0xFFFFFFFFU;

    adc_pa1_init();
    ADC1->CR2 &= ~ADC_CR2_CONT;     /* one conversion per start, not a stream */

    printf("\r\nasking for %lu Hz\r\n\r\n", (unsigned long)TARGET_HZ);

    printf("%-22s %12lu Hz\r\n", "PCLK1", (unsigned long)clock_pclk1());
    printf("%-22s %12lu Hz\r\n", "TIM2 clock",
           (unsigned long)clock_timer_pclk1());
    printf("%-22s %12lu\r\n", "ARR from PCLK1",
           (unsigned long)(clock_pclk1() / TARGET_HZ - 1U));
    printf("%-22s %12lu\r\n", "ARR from TIM2 clock",
           (unsigned long)(clock_timer_pclk1() / TARGET_HZ - 1U));
    printf("%-22s %12lu Hz   <- the trap\r\n", "what the first gives",
           (unsigned long)(clock_timer_pclk1()
                           / (clock_pclk1() / TARGET_HZ)));

    uint32_t got = timer_periodic_init(TIM2, TARGET_HZ);

    printf("\r\nprogrammed PSC %lu ARR %lu, which is %lu Hz\r\n",
           (unsigned long)TIM2->PSC, (unsigned long)TIM2->ARR,
           (unsigned long)got);

    timer_interrupt_enable(TIM2, IRQ_NO_TIM2);

    uint64_t start = ticks_get();
    uint32_t at_start = g_samples;

    while ((ticks_get() - start) < WINDOW_MS)
    {
    }

    uint32_t counted = ((g_samples - at_start) * 1000U) / WINDOW_MS;

    printf("\r\n%-22s %12lu Hz\r\n", "counted over half a second",
           (unsigned long)counted);

    uint32_t counts_per_sample = TIM2->ARR + 1U;

    printf("\r\ninterrupt latency, in TIM2 counts out of %lu per sample:\r\n",
           (unsigned long)counts_per_sample);
    printf("  %-10s %8lu counts %8lu ns\r\n", "best", (unsigned long)g_best_gap,
           (unsigned long)((uint64_t)g_best_gap * 1000000000U
                           / clock_timer_pclk1()));
    printf("  %-10s %8lu counts %8lu ns\r\n", "worst",
           (unsigned long)g_worst_gap,
           (unsigned long)((uint64_t)g_worst_gap * 1000000000U
                           / clock_timer_pclk1()));
    printf("  %-10s %8lu counts, %lu ppm of the period\r\n", "spread",
           (unsigned long)(g_worst_gap - g_best_gap),
           (unsigned long)((g_worst_gap - g_best_gap) * 1000000U
                           / counts_per_sample));

    printf("\r\nthe spread is the jitter. the average rate is exact and the"
           " individual\r\ngaps are not, which is a distinction the next app"
           " removes.\r\n");

    g_rate   = (float32_t)counted;
    g_jitter = (float32_t)(g_worst_gap - g_best_gap);

    while (1)
    {
        g_raw   = (float32_t)g_latest;
        g_volts = (float32_t)g_latest * (float32_t)ADC_VREF_MV
                  / (float32_t)ADC_FULL_SCALE / 1000.0f;
        probe_step();
    }
}