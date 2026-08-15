/*
 * The same spectrum from arm_rfft_fast_f32, and the two things that have to be
 * lined up before the numbers can be compared.
 *
 * First, the packing. A real FFT of N points has N/2 + 1 bins but only N
 * independent numbers, because bin 0 and bin N/2 have no sine part at all.
 * CMSIS uses that to fit the answer in an N float array: slot 0 holds bin 0,
 * slot 1 holds bin N/2, and the rest are re and im in pairs. Reading it as a
 * plain array of pairs puts the Nyquist bin where bin 1 belongs, and the
 * spectrum comes out looking wrong in a way that is hard to see.
 *
 * Second, the sign. CMSIS takes the sine sum with a minus in front and dft.c
 * does not, so ImX comes back negated. Neither is more correct. The magnitude
 * does not notice either way, which is one more reason it is the thing to plot.
 *
 * The size specific init is worth the extra name. The generic one drags in
 * twiddle tables for every length from 32 to 4096, and swapping it for this one
 * took 77 kB off the image.
 *
 *     make dft_cmsis && make load && make debug
 */
#include <stdio.h>
#include "config.h"
#include "signals.h"
#include "probe.h"
#include "dft.h"

#define N           256U
#define BINS        DFT_BINS(N)

static float32_t x[N];
static float32_t work[N];
static float32_t packed[N];
static float32_t re[BINS];
static float32_t im[BINS];
static float32_t mag[BINS];
static float32_t re_fft[BINS];
static float32_t im_fft[BINS];
static float32_t mag_fft[BINS];

static arm_rfft_fast_instance_f32 fft;

// pulls the CMSIS layout apart into the two plain arrays dft.c produces, and
// flips the sine sign on the way so the two can be put side by side
static void unpack(const float32_t *pPacked, float32_t *pReX, float32_t *pImX)
{
    pReX[0] = pPacked[0];
    pImX[0] = 0.0f;

    pReX[BINS - 1U] = pPacked[1];
    pImX[BINS - 1U] = 0.0f;

    for (uint32_t k = 1; k < (BINS - 1U); k++)
    {
        pReX[k] = pPacked[2U * k];
        pImX[k] = -pPacked[(2U * k) + 1U];
    }
}

int main(void)
{
    float32_t gap_re;
    float32_t gap_im;
    float32_t gap_mag;
    uint32_t index;

    config_app();
    probe_reset();

    arm_copy_f32(input_signal_f32_1kHz_15kHz, x, N);

    dft_forward(x, N, re, im);
    dft_magnitude(re, im, N, mag);

    if (arm_rfft_fast_init_256_f32(&fft) != ARM_MATH_SUCCESS)
    {
        printf("\r\nno FFT table for %lu points\r\n", (unsigned long)N);

        while (1)
        {
        }
    }

    /* the transform writes over its input, so it gets a copy to chew on */
    arm_copy_f32(x, work, N);
    arm_rfft_fast_f32(&fft, work, packed, 0);

    unpack(packed, re_fft, im_fft);
    dft_magnitude(re_fft, im_fft, N, mag_fft);

    arm_sub_f32(re, re_fft, work, BINS);
    arm_absmax_f32(work, BINS, &gap_re, &index);

    arm_sub_f32(im, im_fft, work, BINS);
    arm_absmax_f32(work, BINS, &gap_im, &index);

    arm_sub_f32(mag, mag_fft, work, BINS);
    arm_absmax_f32(work, BINS, &gap_mag, &index);

    printf("\r\n%lu points, %lu bins, %.1f Hz apart\r\n", (unsigned long)N,
           (unsigned long)BINS, (double)BIN_HZ(1U, N, SAMPLE_RATE_HZ));
    printf("dft_forward against arm_rfft_fast_f32, worst bin\r\n");
    printf("  ReX       %.6f\r\n", gap_re);
    printf("  ImX       %.6f\r\n", gap_im);
    printf("  magnitude %.6f\r\n", gap_mag);

    while (1)
    {
        for (uint32_t k = 0; k < N; k++)
        {
            g_x   = x[k];
            g_re  = (k < BINS) ? re[k] : 0.0f;
            g_im  = (k < BINS) ? im[k] : 0.0f;
            g_mag = (k < BINS) ? mag[k] : 0.0f;
            g_rebuilt = (k < BINS) ? mag_fft[k] : 0.0f;

            probe_step();
        }
    }
}