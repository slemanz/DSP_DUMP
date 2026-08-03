/*
 * Aliasing is different signals becoming indistinguishable. Sampled at 20 Hz,
 * a 15 Hz cosine and a 5 Hz cosine do not merely look alike, they produce the
 * same numbers, so nothing downstream can ever tell them apart. Anything above
 * half the sampling rate folds back below it, at |f - k*fs|.
 *
 *     make sampling_alias && make load && make monitor
 */
#include <stdio.h>
#include <math.h>
#include "config.h"
#include "arm_math.h"
#include "driver_systick.h"
#include "probe.h"

#define FS_HZ           20U
#define LOW_HZ          5.0f
#define HIGH_HZ         15.0f
#define CHECK_SAMPLES   40U

int main(void)
{
    config_app();

    printf("\r\nsampled at %lu Hz, everything folds below %.1f Hz\r\n",
           (unsigned long)FS_HZ, (float32_t)FS_HZ / 2.0f);
        
    for (uint32_t f = 1U; f <= (2U * FS_HZ) - 1U; f += 2U)
    {
        float32_t apparent = fmodf((float32_t)f, (float32_t)FS_HZ);

        if (apparent > ((float32_t)FS_HZ / 2.0f))
        {
            apparent = (float32_t)FS_HZ - apparent;
        }

        printf("  %2lu Hz reads back as %4.1f Hz%s\r\n", (unsigned long)f, apparent,
               ((float32_t)f > ((float32_t)FS_HZ / 2.0f)) ? "   alias" : "");
    }

    float32_t worst = 0.0f;

    for (uint32_t n = 0; n < CHECK_SAMPLES; n++)
    {
        float32_t t = (float32_t)n / (float32_t)FS_HZ;
        float32_t diff = fabsf(cosf(TWO_PI * HIGH_HZ * t) - cosf(TWO_PI * LOW_HZ * t));

        if (diff > worst)
        {
            worst = diff;
        }
    }

    printf("\r\nlargest gap between the %.0f Hz and %.0f Hz samples: %g\r\n",
           HIGH_HZ, LOW_HZ, worst);

    uint32_t steps_per_sample = MODEL_HZ / FS_HZ;
    uint32_t n = 0;
    float32_t held_high = 0.0f;
    float32_t held_low = 0.0f;

    while(1)
    {
        float32_t t = (float32_t)n / (float32_t)MODEL_HZ;

        if ((n % steps_per_sample) == 0U)
        {
            held_high = cosf(TWO_PI * HIGH_HZ * t);
            held_low = cosf(TWO_PI * LOW_HZ * t);
        }

        g_analog = cosf(TWO_PI * HIGH_HZ * t);
        g_sampled = held_high;
        g_error = held_high - held_low;

        n++;
        probe_step();
    }
}