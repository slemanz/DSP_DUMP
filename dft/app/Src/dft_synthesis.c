/*
 * A square wave, put back together one frequency at a time.
 *
 * The chapter opens by claiming that any periodic signal is a sum of properly
 * chosen sinusoids. A square wave is the least likely looking candidate for
 * that claim: it is flat, then it jumps. This app takes its transform and adds
 * the bins back one at a time, streaming the running total.
 *
 * The first pass is a single sine, and it already overshoots the corners by
 * 27%. Adding harmonics pulls the flat parts flatter and the overshoot down,
 * and then something worth noticing happens: at the twelfth harmonic the peak
 * lands on 1.0000 and the reconstruction is exact.
 *
 * That is not the textbook picture, where the overshoot at a step never
 * disappears no matter how many harmonics are added. It disappears here because
 * this square wave was sampled. Its period is 48 samples, so it has exactly
 * twelve harmonics below half the sampling rate and there are no more to add.
 * The ringing the textbook is talking about lives between the samples, where
 * this signal has nothing to say.
 *
 * Bins that hold nothing are added and skipped without a pass, because a square
 * wave carries only odd harmonics and watching the even ones contribute zero
 * gets old.
 *
 *     make dft_synthesis && make load && make debug
 */
#include <stdio.h>
#include <math.h>
#include "config.h"
#include "signals.h"
#include "probe.h"
#include "dft.h"

#define N               192U
#define BINS            DFT_BINS(N)
#define SHOW_ABOVE      1.0f    // a bin quieter than this changed nothing visible

static float32_t x[N];
static float32_t re[BINS];
static float32_t im[BINS];
static float32_t mag[BINS];
static float32_t running[N];

int main(void)
{
    float32_t peak;
    uint32_t index;
    uint32_t harmonics;

    config_app();
    probe_reset();

    /* a 1 kHz square at 48 kHz, so 24 samples high and 24 low */
    for (uint32_t n = 0; n < N; n++)
    {
        x[n] = ((n % 48U) < 24U) ? 1.0f : -1.0f;
    }

    dft_forward(x, N, re, im);
    dft_magnitude(re, im, N, mag);

    printf("\r\na 1 kHz square wave, %lu samples, rebuilt one frequency at a time\r\n",
           (unsigned long)N);

    while (1)
    {
        arm_fill_f32(0.0f, running, N);
        harmonics = 0;

        for (uint32_t k = 0; k < BINS; k++)
        {
            float32_t scale = ((k == 0U) || (k == (BINS - 1U))) ?
                              (float32_t)N : ((float32_t)N / 2.0f);
            float32_t a = re[k] / scale;
            float32_t b = im[k] / scale;

            for (uint32_t i = 0; i < N; i++)
            {
                float32_t angle = TWO_PI * (float32_t)k * (float32_t)i / (float32_t)N;

                running[i] += (a * cosf(angle)) + (b * sinf(angle));
            }

            if (mag[k] < SHOW_ABOVE)
            {
                continue;
            }

            harmonics++;
            arm_absmax_f32(running, N, &peak, &index);

            printf("%2lu harmonic(s), up to %5.0f Hz, peak %.4f, overshoot %5.2f%%\r\n",
                   (unsigned long)harmonics, (double)BIN_HZ(k, N, SAMPLE_RATE_HZ),
                   peak, (double)(100.0f * (peak - 1.0f)));

            for (uint32_t i = 0; i < N; i++)
            {
                g_x       = x[i];
                g_mag     = (i < BINS) ? mag[i] : 0.0f;
                g_rebuilt = running[i];

                probe_step();
            }
        }
    }
}