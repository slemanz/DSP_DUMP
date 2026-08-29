/*
 * A stream that does not miss, and the question the next chapter answers.
 *
 * Everything so far took samples and looked at them one at a time. Real work
 * does not: convolution wants an array, the DFT wants a window, and every
 * CMSIS-DSP function takes a block and a length. So the samples have to arrive
 * at an exact rate and be put somewhere until there are enough of them.
 *
 * This app does the first half and measures whether the second half is even
 * possible. It runs the converter from the timer and, in the handler, does
 * nothing but store the result. Then it counts: how many samples arrived, how
 * many were lost to overrun, and how much of each period the handler used.
 *
 * That last number is the budget for the next chapter. If storing a sample
 * already uses most of the period, there is no room to filter it in there, and
 * the filtering has to happen somewhere else on a block that has already been
 * collected. Which is exactly what the next chapter is about.
 *
 *     make adc_stream && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "timer.h"
#include "driver_adc.h"
#include "driver_clock.h"
#include "driver_systick.h"

#define TARGET_HZ       2000U
#define RUN_MS          1000U
#define RING            256U

static volatile uint16_t ring[RING];
static volatile uint32_t head;
static volatile uint32_t arrived;
static volatile uint32_t dropped;
static volatile uint32_t busiest;

void TIM2_IRQHandler(void)
{
    uint32_t entered = TIM2->CNT;

    timer_clear_update(TIM2);

    if ((ADC1->SR & ADC_SR_OVR) != 0U)
    {
        ADC1->SR &= ~ADC_SR_OVR;
        dropped++;
    }

    if ((ADC1->SR & ADC_SR_EOC) != 0U)
    {
        ring[head] = (uint16_t)ADC1->DR;
        head = (head + 1U) % RING;
        arrived++;
    }

    /* how much of the period this handler used, in timer counts */
    uint32_t used = TIM2->CNT - entered;

    if (used > busiest)
    {
        busiest = used;
    }
}

int main(void)
{
    config_app();
    probe_reset();

    adc_pa1_init();

    uint32_t fs = timer_periodic_init(TIM2, TARGET_HZ);

    timer_trgo_on_update(TIM2);
    adc_set_trigger(ADC_TRIGGER_TIM2_TRGO);
    timer_interrupt_enable(TIM2, IRQ_NO_TIM2);

    printf("\r\nstreaming at %lu Hz for %lu ms\r\n", (unsigned long)fs,
           (unsigned long)RUN_MS);

    uint64_t start = ticks_get();

    while ((ticks_get() - start) < RUN_MS)
    {
    }

    uint32_t period_counts = TIM2->ARR + 1U;
    uint32_t expected = (fs * RUN_MS) / 1000U;

    printf("\r\n  %-22s %10lu\r\n", "samples expected", (unsigned long)expected);
    printf("  %-22s %10lu\r\n", "samples arrived", (unsigned long)arrived);
    printf("  %-22s %10lu\r\n", "overruns", (unsigned long)dropped);
    printf("  %-22s %10ld\r\n", "difference",
           (long)((int32_t)arrived - (int32_t)expected));

    printf("\r\nthe handler's budget:\r\n");
    printf("  %-22s %10lu counts %8lu ns\r\n", "one sample period",
           (unsigned long)period_counts,
           (unsigned long)((uint64_t)period_counts * 1000000000U
                           / clock_timer_pclk1()));
    printf("  %-22s %10lu counts %8lu ns\r\n", "worst handler seen",
           (unsigned long)busiest,
           (unsigned long)((uint64_t)busiest * 1000000000U
                           / clock_timer_pclk1()));
    printf("  %-22s %9lu%%\r\n", "of the period used",
           (unsigned long)(busiest * 100U / period_counts));

    printf("\r\nwhat is left over is the whole budget for doing something with"
           " the\r\nsample, and it is not much. that is why the next chapter"
           " collects a\r\nblock first and works on it afterwards.\r\n");

    g_rate   = (float32_t)fs;
    g_jitter = (float32_t)dropped;

    while (1)
    {
        for (uint32_t i = 0U; i < RING; i++)
        {
            g_raw   = (float32_t)ring[i];
            g_volts = (float32_t)ring[i] * (float32_t)ADC_VREF_MV
                      / (float32_t)ADC_FULL_SCALE / 1000.0f;
            probe_step();
        }
    }
}