/*
 * The sampling theorem: a signal can be reconstructed from its samples only if
 * it holds no frequency above half the sampling rate. The same 5 Hz sine is
 * run through four sampling rates, from ten times the signal down to one times
 * it, and each sample is held until the next one arrives, which is what the
 * sample and hold stage of a converter does.
 *
 *     make sampling_theorem && make load && make monitor
 */
#include <stdio.h>
#include <math.h>
#include "config.h"
#include "arm_math.h"
#include "driver_systick.h"
#include "probe.h"

#define SIG_HZ          5.0f
#define RATE_SECONDS    4U

static const uint32_t rates_hz[] = { 50U, 20U, 10U, 5U };

int main(void)
{
    config_app();

    printf("\r\na %.0f Hz sine sampled at four rates, %lu s each\r\n",
           SIG_HZ, (unsigned long)RATE_SECONDS);

    while(1)
    {
        for(uint32_t r = 0; r < ARRAY_LEN(rates_hz); r++)
        {
            uint32_t fs = rates_hz[r];
            uint32_t steps_per_sample = MODEL_HZ/fs;
            uint32_t total_steps = MODEL_HZ * RATE_SECONDS;
            float32_t held = 0.0f;

            printf("fs %2lu Hz  %.1f samples per period  %s\r\n",
                   (unsigned long)fs, (float32_t)fs / SIG_HZ,
                   ((float32_t)fs > (2.0f * SIG_HZ)) ? "above nyquist" : "below nyquist");

            for (uint32_t n = 0; n < total_steps; n++)
            {
                float32_t t = (float32_t)n/(float32_t)MODEL_HZ;
                float32_t x = sinf(TWO_PI * SIG_HZ * t);

                if((n % steps_per_sample) == 0U)
                {
                    held = x;
                }

                g_analog = x;
                g_sampled = held;
                g_error = held - x;

                probe_step();
            }
        }

    }
}