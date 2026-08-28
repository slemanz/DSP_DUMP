/*
 * Taking the CPU out of the timing path.
 *
 * The previous app had the timer interrupt the core and the core start a
 * conversion. That works, and the average rate is exact, but the individual
 * gaps are not: the handler runs whenever the core gets round to it, and how
 * long that takes depends on what it was doing.
 *
 * Jitter is not a rounding error in a spectrum, it is noise. A sample taken
 * slightly late is a sample of the wrong part of the waveform, and on a fast
 * signal a nanosecond of lateness is a real amplitude error. It gets worse the
 * steeper the signal, which is to say the higher the frequency.
 *
 * The fix is to stop routing the decision through software. The timer's
 * rollover goes out on its TRGO line, the ADC is told to start on that edge,
 * and the two are wired together inside the chip. The core is then free to be
 * late reading the result, which costs nothing, rather than late taking the
 * sample, which costs accuracy.
 *
 * What the core can still do wrong is not read fast enough, and that has a
 * flag: OVR says a conversion finished before the last one was collected. A
 * lost sample that announces itself is a different thing from a lost sample.
 *
 *     make adc_trigger && make load && make monitor
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
#define STARVE_MS       50U

int main(void)
{
    config_app();
    probe_reset();

    adc_pa1_init();

    uint32_t got = timer_periodic_init(TIM2, TARGET_HZ);

    timer_trgo_on_update(TIM2);
    adc_set_trigger(ADC_TRIGGER_TIM2_TRGO);

    printf("\r\nTIM2 at %lu Hz, its rollover on TRGO, the ADC starting on"
           " that edge\r\n", (unsigned long)got);
    printf("nothing in software is in the timing path any more\r\n\r\n");

    printf("%-26s %12lu Hz\r\n", "timer clock",
           (unsigned long)clock_timer_pclk1());
    printf("%-26s %12lu\r\n", "PSC", (unsigned long)TIM2->PSC);
    printf("%-26s %12lu\r\n", "ARR", (unsigned long)TIM2->ARR);
    printf("%-26s %12lu ns\r\n", "one timer count",
           (unsigned long)(1000000000U / clock_timer_pclk1()));
    printf("%-26s %12lu ns\r\n", "the conversion itself",
           (unsigned long)((uint64_t)adc_conversion_cycles() * 1000000000U
                           / adc_clk_hz()));

    (void)adc_overrun();

    uint64_t start = ticks_get();
    uint32_t taken = 0U;
    uint32_t lost  = 0U;

    while ((ticks_get() - start) < WINDOW_MS)
    {
        if ((ADC1->SR & ADC_SR_EOC) != 0U)
        {
            (void)ADC1->DR;
            taken++;
        }

        lost += adc_overrun();
    }

    printf("\r\nkeeping up:\r\n");
    printf("  %-22s %10lu\r\n", "samples collected", (unsigned long)taken);
    printf("  %-22s %10lu Hz\r\n", "which is",
           (unsigned long)(taken * 1000U / WINDOW_MS));
    printf("  %-22s %10lu\r\n", "overruns", (unsigned long)lost);

    /* now deliberately stop collecting and let the converter run over itself */
    (void)adc_overrun();
    start = ticks_get();

    while ((ticks_get() - start) < STARVE_MS)
    {
    }

    printf("\r\nafter %lu ms of not reading DR at all:\r\n", (unsigned long)STARVE_MS);
    printf("  %-22s %10s\r\n", "overrun flag",
           adc_overrun() ? "set" : "clear");
    printf("\r\nthe samples in that gap are gone either way. the difference is"
           " that\r\nthis way the gap is on the record.\r\n");

    g_rate = (float32_t)got;

    while (1)
    {
        if ((ADC1->SR & ADC_SR_EOC) != 0U)
        {
            uint32_t v = ADC1->DR;

            g_raw   = (float32_t)v;
            g_volts = (float32_t)v * (float32_t)ADC_VREF_MV
                      / (float32_t)ADC_FULL_SCALE / 1000.0f;
        }

        g_jitter = (float32_t)adc_overrun();
        probe_step();
    }
}