/*
 * Why a spike goes missing.
 *
 * The transform has no idea the signal continues past the window. It assumes
 * the window repeats forever, back to back. If the window holds a whole number
 * of periods, the repeats join up smoothly and the frequency lands on exactly
 * one bin. If it does not, the joint is a step, a step is broad in frequency,
 * and the energy that belonged in one bin gets smeared across its neighbours.
 *
 * At 48 kHz a 1 kHz wave takes 48 samples per period and a 15 kHz wave takes
 * 3.2. A window of 192 samples holds 4 and 60 periods of them, both whole. A
 * window of 320 holds 6.667 and 100. The 15 kHz still lands cleanly and the
 * 1 kHz does not, so a plot of that window shows one sharp spike and one low
 * hill, and it is easy to conclude the transform lost a frequency.
 *
 * The fix is the window, not the code. Nothing below is different between the
 * two runs except N.
 *
 *     make dft_leakage && make load && make debug
 */
#include <stdio.h>
#include "config.h"
#include "signals.h"
#include "probe.h"
#include "dft.h"

#define N_CLEAN     192U            // 4 and 60 whole periods
#define N_LEAKY     320U            // 6.667 and 100
#define N_MAX       N_LEAKY
#define BINS_MAX    DFT_BINS(N_MAX)
#define TOP_ROWS    4U
#define PASSES      2U

static float32_t x[N_MAX];
static float32_t re[BINS_MAX];
static float32_t im[BINS_MAX];
static float32_t mag[BINS_MAX];

// the loudest few bins, whatever they turn out to be, so nothing is assumed
// about where the energy ended up
static void report(uint32_t len)
{
    uint32_t bins = DFT_BINS(len);
    float32_t periods = (float32_t)len / 48.0f;

    dft_forward(x, len, re, im);
    dft_magnitude(re, im, len, mag);

    printf("\r\nN = %lu, which is %.3f periods of the 1 kHz component\r\n",
           (unsigned long)len, periods);
    printf("bins are %.2f Hz apart\r\n", BIN_HZ(1U, len, SAMPLE_RATE_HZ));
    printf("%5s %9s %10s\r\n", "k", "Hz", "magnitude");

    for (uint32_t shown = 0; shown < TOP_ROWS; shown++)
    {
        float32_t best = -1.0f;
        uint32_t at = 0;

        for (uint32_t k = 0; k < bins; k++)
        {
            if (mag[k] > best)
            {
                best = mag[k];
                at = k;
            }
        }

        printf("%5lu %9.1f %10.3f\r\n", (unsigned long)at,
               (double)BIN_HZ(at, len, SAMPLE_RATE_HZ), best);
        mag[at] = -1.0f;
    }
}

int main(void)
{
    config_app();
    probe_reset();

    arm_copy_f32(input_signal_f32_1kHz_15kHz, x, N_MAX);

    report(N_CLEAN);
    report(N_LEAKY);

    printf("\r\nsame signal, same code, different window\r\n");

    while (1)
    {
        const uint32_t lengths[2] = { N_CLEAN, N_LEAKY };

        for (uint32_t s = 0; s < 2U; s++)
        {
            uint32_t len = lengths[s];
            uint32_t bins = DFT_BINS(len);

            dft_forward(x, len, re, im);
            dft_magnitude(re, im, len, mag);

            printf("N = %lu\r\n", (unsigned long)len);

            for (uint32_t p = 0; p < PASSES; p++)
            {
                for (uint32_t k = 0; k < N_MAX; k++)
                {
                    g_x   = (k < len) ? x[k] : 0.0f;
                    g_mag = (k < bins) ? mag[k] : 0.0f;

                    probe_step();
                }
            }
        }
    }
}