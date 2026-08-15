/*
 * The transform on a real signal, and the one table that matters: which
 * frequencies are in it and how much of each.
 *
 * INPUT_SEL picks what goes in. Change it, rebuild, and look again. The three
 * shapes are chosen to get harder: one frequency, then two, then a square wave,
 * which is a signal nobody would describe in terms of sinusoids and which turns
 * out to be nothing but sinusoids.
 *
 * The window is 192 samples, which is exactly four periods of 1 kHz at 48 kHz.
 * That number is not casual, and dft_leakage is about what happens when it is
 * chosen carelessly.
 *
 *     make dft_spectrum && make load && make debug
 */
#include <stdio.h>
#include <math.h>
#include "config.h"
#include "signals.h"
#include "probe.h"
#include "dft.h"

#define N           192U            // four periods of the 1 kHz component
#define BINS        DFT_BINS(N)
#define BIN_WIDTH   (BIN_HZ(1U, N, SAMPLE_RATE_HZ))
#define REPORT_ABOVE    1.0f        // bins quieter than this are not worth a row

/************************************************************
*                          THE KNOB                         *
*************************************************************/

#define INPUT_SEL   0U      // 0 one sine | 1 two sines | 2 square wave

/************************************************************/

static float32_t x[N];
static float32_t re[BINS];
static float32_t im[BINS];
static float32_t mag[BINS];

static const char *build_input(void)
{
    const char *name;

    switch (INPUT_SEL)
    {
        case 0U:
            for (uint32_t n = 0; n < N; n++)
            {
                x[n] = sinf(TWO_PI * 1000.0f * (float32_t)n / (float32_t)SAMPLE_RATE_HZ);
            }
            name = "one 1 kHz sine";
            break;
            break;

            case 2U:
                /* a 1 kHz square, so 48 samples high then 48 low */
                for (uint32_t n = 0; n < N; n++)
                {
                    x[n] = ((n % 48U) < 24U) ? 1.0f : -1.0f;
                }
                name = "a 1 kHz square wave";
                break;
        
        default:
            arm_copy_f32(input_signal_f32_1kHz_15kHz, x, N);
            name = "1 kHz plus 15 kHz at half amplitude";
            break;
    }

    return name;
}

int main(void)
{
    const char *name;

    config_app();
    probe_reset();

    name = build_input();

    dft_forward(x, N, re, im);
    dft_magnitude(re, im, N, mag);

    printf("\r\n%s, %lu samples at %lu Hz\r\n", name, (unsigned long)N, (unsigned long)SAMPLE_RATE_HZ);
    printf("%lu bins, %.1f Hz apart, reaching up to %.0f Hz\r\n\r\n", (unsigned long)BINS, BIN_WIDTH, BIN_HZ(BINS - 1U, N, SAMPLE_RATE_HZ));

    printf("%5s %9s %10s %10s %10s\r\n", "k", "Hz", "ReX", "ImX", "magnitude");

    for (uint32_t k = 0; k < BINS; k++)
    {
        if (mag[k] >= REPORT_ABOVE)
        {
            printf("%5lu %9.0f %10.3f %10.3f %10.3f\r\n", (unsigned long)k,
                   (double)BIN_HZ(k, N, SAMPLE_RATE_HZ), re[k], im[k], mag[k]);
        }
    }

    /* Worth reading the ReX column before moving on. These signals are built
     * from sines, so the cosine half of the answer is zero and every bit of the
     * amplitude sits in ImX. Anything that treats ReX alone as the magnitude
     * reports an empty spectrum for a signal that is plainly not empty. */
    printf("\r\nmagnitude is sqrt(ReX*ReX + ImX*ImX), never ReX on its own\r\n");

    while(1)
    {
        for (uint32_t k = 0; k < N; k++)
        {
            g_x   = x[k];
            g_re  = (k < BINS) ? re[k] : 0.0f;
            g_im  = (k < BINS) ? im[k] : 0.0f;
            g_mag = (k < BINS) ? mag[k] : 0.0f;

            probe_step();
        }
    }
}