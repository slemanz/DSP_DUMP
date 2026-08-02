/*
 * Both formulas need the mean before they can measure the spread, which means
 * two passes over the samples and a buffer to hold them. Expanding the square
 * gives a form that needs only the running sum and the running sum of squares,
 * so a live stream can be measured one sample at a time with no buffer at all:
 *
 *     variance = (sum_of_squares - sum * sum / N) / (N - 1)
 *
 * The catch is precision. Both terms grow with the DC level while their
 * difference stays the size of the fluctuation, so a large offset leaves the
 * answer built out of the last few bits of two much larger numbers.
 *
 *     make stat_running && make load && make monitor
 */
#include <stdio.h>
#include "config.h"
#include "arm_math.h"
#include "signals.h"

#define SIG_LEN     KHZ1_15_SIG_LEN

static float32_t variance_two_pass(const float32_t *sig_src_arr, uint32_t sig_length);
static float32_t variance_one_pass(const float32_t *sig_src_arr, uint32_t sig_length);

static float32_t shifted[SIG_LEN];

int main(void)
{
    config_app();

    static const float32_t offsets[] = { 0.0f, 2.0f, 1000.0f };

    printf("\r\n%-10s %14s %14s\r\n", "dc offset", "two pass", "one pass");

    for (uint32_t k = 0; k < (sizeof(offsets) / sizeof(offsets[0])); k++)
    {
        for (uint32_t i = 0; i < SIG_LEN; i++)
        {
            shifted[i] = input_signal_f32_1kHz_15kHz[i] + offsets[k];
        }

        printf("%-10.1f %14f %14f\r\n", offsets[k],
               variance_two_pass(shifted, SIG_LEN),
               variance_one_pass(shifted, SIG_LEN));
    }

    while (1)
    {
    }
}

static float32_t variance_two_pass(const float32_t *sig_src_arr, uint32_t sig_length)
{
    float32_t sum = 0.0f;

    for (uint32_t i = 0; i < sig_length; i++)
    {
        sum += sig_src_arr[i];
    }

    float32_t mean = sum / (float32_t)sig_length;
    float32_t squares = 0.0f;

    for (uint32_t i = 0; i < sig_length; i++)
    {
        float32_t diff = sig_src_arr[i] - mean;
        squares += diff * diff;
    }

    return squares / (float32_t)(sig_length - 1U);
}

static float32_t variance_one_pass(const float32_t *sig_src_arr, uint32_t sig_length)
{
    float32_t sum = 0.0f;
    float32_t sum_of_squares = 0.0f;

    for (uint32_t i = 0; i < sig_length; i++)
    {
        sum += sig_src_arr[i];
        sum_of_squares += sig_src_arr[i] * sig_src_arr[i];
    }

    return (sum_of_squares - (sum * sum) / (float32_t)sig_length) / (float32_t)(sig_length - 1U);
}