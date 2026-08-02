/*
 * The same statistics on real samples. The potentiometer on PA1 is read into a
 * buffer and the mean gives the DC level it is set to, while the standard
 * deviation gives the noise riding on it. Hold the knob still and the mean
 * stays put while the deviation keeps reporting the noise floor of the ADC.
 *
 * The signal to noise ratio is the DC level divided by that deviation, and the
 * fraction of samples landing inside one deviation of the mean should sit near
 * 68% when the noise is normally distributed.
 *
 *     make stat_noise && make load && make monitor
 */
#include <stdio.h>
#include <math.h>
#include "config.h"
#include "arm_math.h"

#include "driver_adc.h"
#include "driver_systick.h"

#define SAMPLES         256U
#define REPORT_MS       500U

static uint32_t samples_within(const float32_t *sig_src_arr, uint32_t sig_length,
                               float32_t sig_mean, float32_t sig_std);

static float32_t buffer[SAMPLES];

int main(void)
{
    config_app();

    adc_pa1_init();
    adc_start_conversion();

    printf("\r\nstatistics of %u adc samples, pot on PA1\r\n", (unsigned)SAMPLES);

    while (1)
    {
        for (uint32_t i = 0; i < SAMPLES; i++)
        {
            buffer[i] = (float32_t)adc_read();
        }

        float32_t mean;
        float32_t std;
        float32_t max;
        float32_t min;
        uint32_t index;

        arm_mean_f32(buffer, SAMPLES, &mean);
        arm_std_f32(buffer, SAMPLES, &std);
        arm_max_f32(buffer, SAMPLES, &max, &index);
        arm_min_f32(buffer, SAMPLES, &min, &index);

        uint32_t within = samples_within(buffer, SAMPLES, mean, std);

        printf("mean %7.2f  std %5.2f  p-p %4.0f  snr %7.1f  within 1 sigma %3lu/%lu\r\n",
               mean, std, max - min, (std > 0.0f) ? (mean / std) : 0.0f,
               (unsigned long)within, (unsigned long)SAMPLES);

        ticks_delay(REPORT_MS);
    }
}

static uint32_t samples_within(const float32_t *sig_src_arr, uint32_t sig_length,
                               float32_t sig_mean, float32_t sig_std)
{
    uint32_t count = 0;

    for (uint32_t i = 0; i < sig_length; i++)
    {
        if (fabsf(sig_src_arr[i] - sig_mean) <= sig_std)
        {
            count++;
        }
    }

    return count;
}