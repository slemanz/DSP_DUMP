/*
 * Where the conversion time comes from, and what it puts a ceiling on.
 *
 * Three numbers decide it. PCLK2 feeds a prescaler to make ADCCLK, the input
 * is held for a chosen number of ADCCLK cycles, and then the successive
 * approximation runs at one cycle per bit. So
 *
 *     conversion = sample cycles + bits, in ADCCLK cycles
 *
 * and the fastest you can sample is one over that.
 *
 * The prescaler has a hard floor that is easy to miss. The converter is rated
 * for 36 MHz whatever the bus does, and PCLK2 here is 100, so divide by 2 hands
 * it 50 MHz. It will not refuse. It will convert, and the numbers will be
 * wrong in a way that looks like noise.
 *
 * Sample time is not a free choice either. It is how long the input has to
 * charge the sampling capacitor through whatever is driving it, so a signal
 * behind a 10k potentiometer needs a longer one than a signal from an op amp.
 * Too short does not fail, it reads low.
 *
 *     make adc_timing && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "driver_adc.h"
#include "driver_clock.h"
#include "driver_systick.h"

#define WINDOW_MS       200U

static const uint32_t prescalers[] =
{
    ADC_PRE_DIV2, ADC_PRE_DIV4, ADC_PRE_DIV6, ADC_PRE_DIV8,
};

static const uint32_t sample_settings[] =
{
    ADC_SMP_3, ADC_SMP_15, ADC_SMP_56, ADC_SMP_144, ADC_SMP_480,
};

static uint32_t measure_rate(void)
{
    uint64_t start = ticks_get();

    while (ticks_get() == start)
    {
    }

    start = ticks_get();

    uint32_t taken = 0U;

    while ((ticks_get() - start) < WINDOW_MS)
    {
        (void)adc_read();
        taken++;
    }

    return (taken * 1000U) / WINDOW_MS;
}

int main(void)
{
    config_app();
    probe_reset();

    adc_pa1_init();
    adc_free_running();

    printf("\r\nPCLK2 is %lu Hz and the converter is rated for %lu\r\n\r\n", (unsigned long)clock_pclk2(), (unsigned long)ADC_CLK_MAX_HZ);

    printf("%-10s %12s   %s\r\n", "prescaler", "ADCCLK", "legal");

    for (uint32_t k = 0U; k < ARRAY_LEN(prescalers); k++)
    {
        adc_set_prescaler(prescalers[k]);

        uint32_t clk = adc_clk_hz();

        printf("/%-9lu %12lu   %s\r\n", (unsigned long)((k + 1U) * 2U),
               (unsigned long)clk,
               (clk <= ADC_CLK_MAX_HZ) ? "ok" : "OVER 36 MHz");
    }

    adc_set_prescaler(ADC_PRE_DIV4);

    printf("\r\nat ADCCLK %lu Hz, 12 bits\r\n\r\n", (unsigned long)adc_clk_hz());
    printf("%8s %10s %10s %12s %12s\r\n", "sample", "total", "us", "max rate", "measured");

    for (uint32_t k = 0U; k < ARRAY_LEN(sample_settings); k++)
    {
        adc_set_sample_time(sample_settings[k]);
        adc_free_running();

        uint32_t total = adc_conversion_cycles();
        uint32_t rate  = adc_clk_hz() / total;

        printf("%8lu %10lu %10lu %12lu %12lu\r\n",
               (unsigned long)adc_sample_cycles(sample_settings[k]),
               (unsigned long)total,
               (unsigned long)(total * 1000000U / adc_clk_hz()),
               (unsigned long)rate, (unsigned long)measure_rate());

        g_rate = (float32_t)rate;
    }

    adc_set_sample_time(ADC_SMP_15);
    adc_free_running();

    printf("\r\nthe last two columns should track. where they do not, the"
           " reading loop\r\nis the slower of the two and the converter is"
           " waiting for it.\r\n");

    printf("\r\nfewer bits is fewer cycles, one per bit:\r\n\r\n");
    printf("%8s %10s %12s\r\n", "bits", "total", "max rate");

    static const uint32_t modes[] = { ADC_CR1_RES_12BIT, ADC_CR1_RES_10BIT,
                                      ADC_CR1_RES_8BIT,  ADC_CR1_RES_6BIT };

    for (uint32_t k = 0U; k < ARRAY_LEN(modes); k++)
    {
        adc_set_resolution(modes[k]);

        uint32_t total = adc_conversion_cycles();

        printf("%8lu %10lu %12lu\r\n", (unsigned long)(12U - (2U * k)),
               (unsigned long)total, (unsigned long)(adc_clk_hz() / total));
    }

    adc_set_resolution(ADC_CR1_RES_12BIT);
    adc_free_running();

    while (1)
    {
        uint32_t v = adc_read();

        g_raw   = (float32_t)v;
        g_volts = (float32_t)v * (float32_t)ADC_VREF_MV
                  / (float32_t)ADC_FULL_SCALE / 1000.0f;
        probe_step();
    }
}