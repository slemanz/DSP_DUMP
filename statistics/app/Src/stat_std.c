/*
 * The standard deviation is the square root of the variance, which puts it
 * back in the units of the signal itself. For a zero-mean sine of amplitude A
 * it settles at A/sqrt(2), the same 0.707 factor that turns a peak amplitude
 * into an RMS value, so the 5 Hz unit sine is a result that can be checked by
 * hand.
 *
 *     make stat_std && make load && make monitor
 */
#include <stdio.h>
#include <math.h>
#include "config.h"
#include "arm_math.h"
#include "signals.h"

static float32_t signal_mean(const float32_t *sig_src_arr, uint32_t sig_length);
static float32_t signal_variance(const float32_t *sig_src_arr, float32_t sig_mean, uint32_t sig_length);
static float32_t signal_std(float32_t sig_variance);

static void report(const char *name, const float32_t *sig_src_arr, uint32_t sig_length);

int main(void)
{
    config_app();

    report("1 kHz + 15 kHz", input_signal_f32_1kHz_15kHz, KHZ1_15_SIG_LEN);
    report("5 Hz sine", _5hz_signal, HZ_5_SIG_LEN);

    printf("\r\nA/sqrt(2) for a unit sine: %f\r\n", 1.0f / sqrtf(2.0f));

    while(1)
    {

    }
}

static void report(const char *name, const float32_t *sig_src_arr, uint32_t sig_length)
{
    float32_t mean = signal_mean(sig_src_arr, sig_length);
    float32_t variance = signal_variance(sig_src_arr, mean, sig_length);
    float32_t std = signal_std(variance);

    printf("\r\n%s, %u samples\r\n", name, (unsigned)sig_length);
    printf("  mean:     %f\r\n", mean);
    printf("  variance: %f\r\n", variance);
    printf("  std:      %f\r\n", std);
    printf("  std^2:    %f\r\n", std * std);
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
        sum += diff * diff;
    }

    return sum/(float32_t)(sig_length - 1);
}

// sqrtf, not sqrt: the FPU is single precision, and sqrt would promote to double
static float32_t signal_std(float32_t sig_variance)
{
    return sqrtf(sig_variance);
}