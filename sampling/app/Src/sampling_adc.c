/*
 * Quantization on the real converter. The potentiometer on PA1 is read at a
 * fixed rate through the 12 bit ADC, and the same reading is pushed through a
 * coarser quantizer so the two can be watched side by side. Turn the knob
 * slowly and the fine reading climbs a ramp while the coarse one climbs a
 * staircase, one tread per LSB.
 *
 *     make sampling_adc && make load && make monitor
 */
#include <stdio.h>
#include <math.h>
#include "config.h"
#include "arm_math.h"

#include "driver_adc.h"
#include "driver_systick.h"
#include "probe.h"

#define VREF            3.3f
#define ADC_BITS        12U
#define COARSE_BITS     6U
#define REPORT_MS       500U

static float32_t quantize(float32_t volts, uint32_t bits);

int main(void)
{
    config_app();

    adc_pa1_init();
    adc_start_conversion();

    printf("\r\npot on PA1, %.1f V full scale\r\n", VREF);
    printf("  %lu bit lsb: %.3f mV\r\n", (unsigned long)ADC_BITS,
           1000.0f * VREF / (float32_t)(1UL << ADC_BITS));
    printf("  %lu bit lsb: %.3f mV\r\n", (unsigned long)COARSE_BITS,
           1000.0f * VREF / (float32_t)(1UL << COARSE_BITS));

    uint64_t last_report = ticks_get();

    while(1)
    {
        uint32_t counts = adc_read();
        float32_t volts = VREF * (float32_t)counts / (float32_t)((1UL << ADC_BITS) - 1U);

        g_analog = volts;
        g_sampled = quantize(volts, COARSE_BITS);
        g_error = g_sampled - g_analog;

        if ((ticks_get() - last_report) >= REPORT_MS)
        {
            printf("counts %4lu   %.4f V   coarse %.4f V   error %+.4f V\r\n",
                   (unsigned long)counts, g_analog, g_sampled, g_error);
            last_report = ticks_get();
        }

        probe_step();
    }
}

static float32_t quantize(float32_t volts, uint32_t bits)
{
    uint32_t levels = 1UL << bits;
    float32_t lsb = VREF/(float32_t)levels;
    float32_t code = roundf(volts / lsb);

    if(code > (float32_t)(levels - 1U))
    {
        code = (float32_t)(levels - 1U);
    }

    return code * lsb;
}