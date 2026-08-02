/*
 * The same three statistics, this time from CMSIS-DSP. The library computes
 * what the hand written versions compute, arm_var_f32 divides by N-1 as well,
 * so the two columns should agree to the last digits the float can carry.
 *
 *     make stat_cmsis && make load && make monitor
 */
#include <stdio.h>
#include <math.h>
#include "config.h"
#include "arm_math.h"
#include "signals.h"

#define SIG_LEN     KHZ1_15_SIG_LEN

static float32_t signal_mean(const float32_t *sig_src_arr, uint32_t sig_length);
static float32_t signal_variance(const float32_t *sig_src_arr, float32_t sig_mean, uint32_t sig_length);
static float32_t signal_std(float32_t sig_variance);

float32_t g_mean;
float32_t g_variance;
float32_t g_std;

float32_t g_mean_arm;
float32_t g_variance_arm;
float32_t g_std_arm;

int main(void)
{
    config_app();

    g_mean = signal_mean(input_signal_f32_1kHz_15kHz, SIG_LEN);
    g_variance = signal_variance(input_signal_f32_1kHz_15kHz, g_mean, SIG_LEN);
    g_std = signal_std(g_variance);

    arm_mean_f32(input_signal_f32_1kHz_15kHz, SIG_LEN, &g_mean_arm);
    arm_var_f32(input_signal_f32_1kHz_15kHz, SIG_LEN, &g_variance_arm);
    arm_std_f32(input_signal_f32_1kHz_15kHz, SIG_LEN, &g_std_arm);

    printf("\r\n%-10s %12s %12s %12s\r\n", "", "by hand", "cmsis", "difference");
    printf("%-10s %12f %12f %12f\r\n", "mean", g_mean, g_mean_arm, fabsf(g_mean - g_mean_arm));
    printf("%-10s %12f %12f %12f\r\n", "variance", g_variance, g_variance_arm, fabsf(g_variance - g_variance_arm));
    printf("%-10s %12f %12f %12f\r\n", "std", g_std, g_std_arm, fabsf(g_std - g_std_arm));

    while (1)
    {
    }
}

static float32_t signal_mean(const float32_t *sig_src_arr, uint32_t sig_length)
{
    float32_t sum = 0.0f;

    for (uint32_t i = 0; i < sig_length; i++)
    {
        sum += sig_src_arr[i];
    }

    return sum / (float32_t)sig_length;
}

static float32_t signal_variance(const float32_t *sig_src_arr, float32_t sig_mean, uint32_t sig_length)
{
    float32_t sum = 0.0f;

    for (uint32_t i = 0; i < sig_length; i++)
    {
        float32_t diff = sig_src_arr[i] - sig_mean;
        sum += diff * diff;
    }

    return sum / (float32_t)(sig_length - 1U);
}

static float32_t signal_std(float32_t sig_variance)
{
    return sqrtf(sig_variance);
}