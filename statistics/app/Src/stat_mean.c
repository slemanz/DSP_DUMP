/*
 * The mean is the average of a signal: every sample added together, divided by
 * the number of samples. In electronics it is the DC value of the signal.
 *
 *     make stat_mean && make load && make monitor
 */
#include <stdio.h>
#include "config.h"
#include "arm_math.h"
#include "signals.h"

#define SIG_LEN     KHZ1_15_SIG_LEN
#define DC_OFFSET   2.0f

extern float32_t _5hz_signal[HZ_5_SIG_LEN];
extern float32_t input_signal_f32_1kHz_15kHz[KHZ1_15_SIG_LEN];

static float32_t signal_mean(const float32_t *sig_src_arr, uint32_t sig_length);

// the same signal lifted by a known DC level, to show the mean follows it
static float32_t shifted[SIG_LEN];

float32_t g_mean;
float32_t g_mean_shifted;

int main(void)
{
    config_app();

    g_mean = signal_mean(input_signal_f32_1kHz_15kHz, SIG_LEN);

    for(uint32_t i = 0; i < SIG_LEN; i++)
    {
        shifted[i] = input_signal_f32_1kHz_15kHz[i] + DC_OFFSET;
    }

    g_mean_shifted = signal_mean(shifted, SIG_LEN);

    printf("\r\nmean over %u samples\r\n", (unsigned)SIG_LEN);
    printf("  signal:           %f\r\n", g_mean);
    printf("  signal + %.1f DC:  %f\r\n", DC_OFFSET, g_mean_shifted);

    while(1)
    {

    }
}

static float32_t signal_mean(const float32_t *sig_src_arr, uint32_t sig_length)
{
    float32_t sum = 0.0f;

    for(uint32_t i = 0; i < sig_length; i++)
    {
        sum += sig_src_arr[i];
    }

    return sum/(float32_t)sig_length;
}