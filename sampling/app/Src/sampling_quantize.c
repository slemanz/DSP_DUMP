/*
 * Quantization is the half of the conversion that discretizes the vertical
 * axis: a continuous amplitude is mapped onto one of a finite set of codes.
 * The gap between two codes is the LSB, and what the mapping throws away is
 * the quantization error, whose standard deviation is LSB/sqrt(12) as long as
 * the signal spans enough codes for that error to spread evenly.
 *
 *     make sampling_quantize && make load && make monitor
 */

#include <stdio.h>
#include <math.h>
#include "config.h"
#include "arm_math.h"
#include "driver_systick.h"
#include "probe.h"

#define SIG_HZ          5.0f
#define TABLE_LEN       512U
#define STREAM_BITS     4U

static float32_t error_buf[TABLE_LEN];

static float32_t quantize(float32_t x, uint32_t bits);

int main(void)
{
    config_app();

    printf("\r\nquantizing a sine over a -1.0 to 1.0 full scale\r\n");
    printf("%6s %12s %12s %12s %14s\r\n", "bits", "lsb", "max error", "std", "lsb/sqrt(12)");

    for (uint32_t bits = 12U; bits >= 2U; bits -= 2U)
    {
        float32_t lsb = 2.0f / (float32_t)(1UL << bits);
        float32_t max_error = 0.0f;
        float32_t std;

        for (uint32_t i = 0; i < TABLE_LEN; i++)
        {
            float32_t x = 0.999f * sinf(TWO_PI * (float32_t)i / (float32_t)TABLE_LEN);
            error_buf[i] = quantize(x, bits) - x;

            if (fabsf(error_buf[i]) > max_error)
            {
                max_error = fabsf(error_buf[i]);
            }
        }

        arm_std_f32(error_buf, TABLE_LEN, &std);

        printf("%6lu %12.6f %12.6f %12.6f %14.6f\r\n", (unsigned long)bits,
               lsb, max_error, std, lsb / sqrtf(12.0f));
    }

    printf("\r\nstreaming a %.0f Hz sine at %lu bits\r\n", SIG_HZ, (unsigned long)STREAM_BITS);

    uint32_t n = 0;

    while(1)
    {
        float32_t t = (float32_t)n / (float32_t)MODEL_HZ;
        float32_t x = sinf(TWO_PI * SIG_HZ * t);

        g_analog = x;
        g_sampled = quantize(x, STREAM_BITS);
        g_error = g_sampled - g_analog;

        n++;
        probe_step();
    }
}

// mid-tread uniform quantizer, the model of an ideal converter
static float32_t quantize(float32_t x, uint32_t bits)
{
    uint32_t levels = 1UL << bits;
    float32_t lsb = 2.0f / (float32_t)levels;
    float32_t code = roundf((x + 1.0f) / lsb);

    if (code < 0.0f)
    {
        code = 0.0f;
    }

    if (code > (float32_t)(levels - 1U))
    {
        code = (float32_t)(levels - 1U);
    }

    return -1.0f + code * lsb;
}