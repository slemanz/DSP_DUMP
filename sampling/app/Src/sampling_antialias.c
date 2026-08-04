/*
 * The anti aliasing filter. A signal carrying 5 Hz and 15 Hz is sampled at
 * 20 Hz, which is fine for the first component and hopeless for the second:
 * the 15 Hz folds onto 5 Hz and lands on top of the data. A one pole low pass
 * with its cutoff at 5 Hz runs ahead of the converter and pulls the offending
 * component down 10 dB before it ever reaches the sample and hold.
 *
 * The filter here runs in software over the oversampled model, which is a
 * stand in for the resistor and capacitor that would sit ahead of the ADC pin.
 * On real hardware the filter has to be analog. Once a signal is aliased there
 * is no software that can separate it again.
 *
 *     make sampling_antialias && make load && make monitor
 */
#include <stdio.h>
#include <math.h>
#include "config.h"
#include "arm_math.h"
#include "driver_systick.h"
#include "probe.h"

#define FC_HZ           5.0f
#define FS_HZ           20U
#define LOW_HZ          5.0f
#define HIGH_HZ         15.0f

static float32_t rc_alpha(float32_t fc_hz);
static float32_t rc_gain(float32_t alpha, float32_t f_hz);

int main(void)
{
    config_app();

    float32_t alpha = rc_alpha(FC_HZ);
    static const float32_t probe_hz[] = { 1.0f, 2.0f, 5.0f, 10.0f, 15.0f, 20.0f };

    printf("\r\none pole low pass, fc %.1f Hz, alpha %.6f\r\n", FC_HZ, alpha);
    printf("%8s %12s %10s %12s %10s\r\n", "f (Hz)", "ideal", "dB", "measured", "dB");

    for (uint32_t k = 0; k < ARRAY_LEN(probe_hz); k++)
    {
        float32_t f = probe_hz[k];
        float32_t ideal = 1.0f / sqrtf(1.0f + ((f / FC_HZ) * (f / FC_HZ)));
        float32_t measured = rc_gain(alpha, f);

        printf("%8.0f %12.6f %10.3f %12.6f %10.3f\r\n", f,
               ideal, 20.0f * log10f(ideal), measured, 20.0f * log10f(measured));
    }

    printf("\r\nstreaming: %.0f Hz + %.0f Hz sampled at %lu Hz\r\n",
           LOW_HZ, HIGH_HZ, (unsigned long)FS_HZ);

    uint32_t steps_per_sample = MODEL_HZ / FS_HZ;
    uint32_t n = 0;
    float32_t filtered = 0.0f;
    float32_t held_raw = 0.0f;
    float32_t held_filtered = 0.0f;

    while (1)
    {
        float32_t t = (float32_t)n / (float32_t)MODEL_HZ;
        float32_t raw = 0.5f * (sinf(TWO_PI * LOW_HZ * t) + sinf(TWO_PI * HIGH_HZ * t));

        filtered += alpha * (raw - filtered);

        if ((n % steps_per_sample) == 0U)
        {
            held_raw = raw;
            held_filtered = filtered;
        }

        g_analog = raw;
        g_sampled = held_filtered;
        g_error = held_raw - held_filtered;

        n++;
        probe_step();
    }
}

// the discrete form of an RC low pass, with RC = 1 / (2 pi fc)
static float32_t rc_alpha(float32_t fc_hz)
{
    float32_t dt = 1.0f / (float32_t)MODEL_HZ;
    float32_t rc = 1.0f / (TWO_PI * fc_hz);

    return dt / (rc+dt);
}

// peak output over the second half of a six period burst, so the run in settles
static float32_t rc_gain(float32_t alpha, float32_t f_hz)
{
    float32_t dt = 1.0f / (float32_t)MODEL_HZ;
    uint32_t total = (uint32_t)(6.0f / f_hz / dt);
    float32_t y = 0.0f;
    float32_t peak = 0.0f;

    for(uint32_t i = 0; i < total; i++)
    {
        float32_t x = sinf(TWO_PI * f_hz * (float32_t)i * dt);

        y += alpha * (x - y);

        if((i > (total / 2U)) && (fabsf(y) > peak))
        {
            peak = fabsf(y);
        }
    }

    return peak;
}