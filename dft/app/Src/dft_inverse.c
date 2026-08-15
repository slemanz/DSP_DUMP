/*
 * The round trip. Forward into the frequency domain, back out, and a look at
 * what did not survive.
 *
 * This is the check that the two domains really do hold the same information.
 * If anything had been thrown away on the way out, it could not come back, and
 * the difference trace would show where.
 *
 * The normalization is the only part that is not obvious, and dft_by_hand is
 * where it comes from: bin 0 and bin N/2 carry N times their amplitude, every
 * other bin carries N/2 times, so undoing them means dividing by different
 * numbers at the two ends. Getting that wrong gives a signal that is the right
 * shape and the wrong size, which is a good failure to have seen once.
 *
 *     make dft_inverse && make load && make debug
 */
#include <stdio.h>
#include "config.h"
#include "signals.h"
#include "probe.h"
#include "dft.h"

#define N           192U
#define BINS        DFT_BINS(N)

static float32_t x[N];
static float32_t re[BINS];
static float32_t im[BINS];
static float32_t mag[BINS];
static float32_t rebuilt[N];
static float32_t diff[N];

int main(void)
{
    float32_t gap;
    float32_t peak;
    uint32_t index;

    config_app();
    probe_reset();

    arm_copy_f32(input_signal_f32_1kHz_15kHz, x, N);

    dft_forward(x, N, re, im);
    dft_magnitude(re, im, N, mag);
    dft_inverse(re, im, N, rebuilt);

    arm_sub_f32(rebuilt, x, diff, N);
    arm_absmax_f32(diff, N, &gap, &index);
    arm_absmax_f32(x, N, &peak, &index);

    printf("\r\n%lu samples out and %lu bins back, then %lu samples again\r\n",
           (unsigned long)N, (unsigned long)BINS, (unsigned long)N);
    printf("largest sample of the signal: %.6f\r\n", peak);
    printf("largest sample of the difference: %.6f\r\n", gap);
    printf("so the round trip kept the signal to within %.4f%% of its own height\r\n",
           (double)(100.0f * gap / peak));

    /* Two arrays of 97 rebuilt 192 samples, so 194 numbers came back from 192.
     * The two spare ones are ImX[0] and ImX[N/2], which are sums of sines that
     * are zero at every sample and so are always zero themselves. Drop them and
     * the count matches exactly, which is the packing dft_cmsis has to unpick. */
    printf("\r\n%lu samples became %lu numbers, of which ImX[0]=%.1f and ImX[%lu]=%.1f\r\n",
           (unsigned long)N, (unsigned long)(2U * BINS), im[0],
           (unsigned long)(BINS - 1U), im[BINS - 1U]);
    printf("are always zero, so the real count is %lu either way\r\n", (unsigned long)N);

    while (1)
    {
        for (uint32_t k = 0; k < N; k++)
        {
            g_x       = x[k];
            g_mag     = (k < BINS) ? mag[k] : 0.0f;
            g_rebuilt = rebuilt[k];
            g_re      = diff[k];      // the trace that should be a flat line

            probe_step();
        }
    }
}