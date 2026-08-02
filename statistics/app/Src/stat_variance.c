/*
 * The variance is the power of the fluctuation around the mean: subtract the
 * mean from every sample, square the difference, add them all up and divide by
 * N-1. Lifting the whole signal by a DC level moves the mean but leaves the
 * variance untouched, because the samples fluctuate by just as much.
 *
 *     make stat_variance && make load && make monitor
 */
#include <stdio.h>
#include "config.h"
#include "arm_math.h"
#include "signals.h"

#define SIG_LEN     KHZ1_15_SIG_LEN
#define DC_OFFSET   2.0f

static float32_t signal_mean(const float32_t *sig_src_arr, uint32_t sig_length);
static float32_t signal_variance(const float32_t *sig_src_arr, float32_t sig_mean, uint32_t sig_length);

static float32_t shifted[SIG_LEN];

float32_t g_mean;
float32_t g_variance;
float32_t g_variance_shifted;

int main(void)
{
    config_app();

    g_mean = signal_mean(input_signal_f32_1kHz_15kHz, SIG_LEN);
    g_variance = signal_variance(input_signal_f32_1kHz_15kHz, g_mean, SIG_LEN);

    for (uint32_t i = 0; i < SIG_LEN; i++)
    {
        shifted[i] = input_signal_f32_1kHz_15kHz[i] + DC_OFFSET;
    }

    g_variance_shifted = signal_variance(shifted, g_mean + DC_OFFSET, SIG_LEN);

    printf("\r\nvariance over %u samples\r\n", (unsigned)SIG_LEN);
    printf("  mean:             %f\r\n", g_mean);
    printf("  variance:         %f\r\n", g_variance);
    printf("  variance + %.1f DC: %f\r\n", DC_OFFSET, g_variance_shifted);

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

static float32_t signal_variance(const float32_t *sig_src_arr, float32_t sig_mean, uint32_t sig_length)
{
    float32_t sum = 0.0f;

    for(uint32_t i = 0; i < sig_length; i++)
    {
        float32_t diff = sig_src_arr[i] - sig_mean;
        sum += diff*diff;
    }

    return sum/(float32_t)(sig_length - 1U);
}