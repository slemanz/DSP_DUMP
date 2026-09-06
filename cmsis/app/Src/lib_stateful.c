/*
 * The other half: routines that remember.
 *
 * Anything that has to see more than the block it was handed carries an
 * instance, and the pattern is always the same three steps: declare the
 * instance, init it, then call it as many times as you like.
 *
 *     arm_fir_instance_f32 fir;
 *     arm_fir_init_f32(&fir, numTaps, pCoeffs, pState, blockSize);
 *     arm_fir_f32(&fir, pSrc, pDst, blockSize);
 *
 * The state buffer is the interesting part and it is where the block chapter
 * ends up. It holds the overlap between one call and the next, so the filter
 * does not restart at every boundary, and its size is fixed by that job:
 *
 *     FIR                numTaps + blockSize - 1
 *     biquad df2T        2 per stage
 *     rfft               none, the tables are in the instance
 *
 * The buffer belongs to the caller. The library will not allocate it and does
 * not check its length, so getting the formula wrong writes past the end of an
 * array and the symptom appears somewhere else entirely.
 *
 *     make lib_stateful && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "testsig.h"

#define BLOCK       32U
#define BLOCKS      (SIG_LEN / BLOCK)
#define FFT_LEN     128U
#define STAGES      2U

static float32_t rev[TAPS];
static float32_t fir_state[TAPS + BLOCK - 1U];
static float32_t fir_out[SIG_LEN];

static float32_t biquad_state[2U * STAGES];
static float32_t biquad_out[SIG_LEN];

/* two stages of a gentle low pass, in the b0 b1 b2 a1 a2 order the library
   wants, where a1 and a2 are already negated */
static const float32_t biquad_coeffs[5U * STAGES] =
{
    0.0675f, 0.1349f, 0.0675f, 1.1430f, -0.4128f,
    0.0675f, 0.1349f, 0.0675f, 1.1430f, -0.4128f,
};

static float32_t fft_in[FFT_LEN];
static float32_t fft_out[FFT_LEN];

int main(void)
{
    config_app();
    probe_reset();

    printf("\r\ndeclare, init, call. and the buffer each one needs.\r\n\r\n");
    printf("%-30s %-34s %8s\r\n", "routine", "state buffer", "floats");

    printf("%-30s %-34s %8lu\r\n", "arm_fir_f32",
           "numTaps + blockSize - 1", (unsigned long)(TAPS + BLOCK - 1U));
    printf("%-30s %-34s %8lu\r\n", "arm_biquad_cascade_df2T_f32",
           "2 per stage", (unsigned long)(2U * STAGES));
    printf("%-30s %-34s %8lu\r\n", "arm_rfft_fast_f32",
           "none, the tables are the instance", 0UL);

    /* the fir, with its coefficients the right way round */
    for (uint32_t i = 0U; i < TAPS; i++)
    {
        rev[i] = taps[TAPS - 1U - i];
    }

    arm_fir_instance_f32 fir;
    arm_fir_init_f32(&fir, TAPS, rev, fir_state, BLOCK);

    for (uint32_t b = 0U; b < BLOCKS; b++)
    {
        arm_fir_f32(&fir, &sig[b * BLOCK], &fir_out[b * BLOCK], BLOCK);
    }

    /* the biquad, which is the same pattern with a different instance */
    arm_biquad_cascade_df2T_instance_f32 biquad;
    arm_biquad_cascade_df2T_init_f32(&biquad, STAGES, biquad_coeffs,
                                     biquad_state);

    for (uint32_t b = 0U; b < BLOCKS; b++)
    {
        arm_biquad_cascade_df2T_f32(&biquad, &sig[b * BLOCK],
                                    &biquad_out[b * BLOCK], BLOCK);
    }

    /* and the transform, whose instance is all table and no state */
    arm_rfft_fast_instance_f32 fft;
    arm_status ok = arm_rfft_fast_init_128_f32(&fft);

    for (uint32_t i = 0U; i < FFT_LEN; i++)
    {
        fft_in[i] = sig[i];
    }

    if (ok == ARM_MATH_SUCCESS)
    {
        arm_rfft_fast_f32(&fft, fft_in, fft_out, 0U);
    }

    float32_t fir_rms;
    float32_t biquad_rms;
    float32_t in_rms;

    arm_rms_f32(sig, SIG_LEN, &in_rms);
    arm_rms_f32(fir_out, SIG_LEN, &fir_rms);
    arm_rms_f32(biquad_out, SIG_LEN, &biquad_rms);

    printf("\r\nall three ran, %lu blocks of %lu\r\n\r\n",
           (unsigned long)BLOCKS, (unsigned long)BLOCK);
    printf("  %-28s %10.6f\r\n", "rms in", (double)in_rms);
    printf("  %-28s %10.6f\r\n", "rms after the fir", (double)fir_rms);
    printf("  %-28s %10.6f\r\n", "rms after the biquad", (double)biquad_rms);
    printf("  %-28s %10s\r\n", "rfft_init_128 status",
           (ok == ARM_MATH_SUCCESS) ? "SUCCESS" : "ARGUMENT_ERROR");

    printf("\r\nfir against biquad, which is the choice nobody explains:\r\n\r\n");
    printf("  %-22s %14s %14s\r\n", "", "FIR", "biquad");
    printf("  %-22s %14lu %14lu\r\n", "coefficients",
           (unsigned long)TAPS, (unsigned long)(5U * STAGES));
    printf("  %-22s %14lu %14lu\r\n", "state floats",
           (unsigned long)(TAPS + BLOCK - 1U), (unsigned long)(2U * STAGES));
    printf("  %-22s %14s %14s\r\n", "phase", "linear", "not");
    printf("  %-22s %14s %14s\r\n", "can it ring forever", "no", "yes");
    printf("  %-22s %14s %14s\r\n", "can it be unstable", "no", "yes");

    printf("\r\nthe biquad does a similar job with a sixth of the arithmetic"
           " by feeding\r\nits output back in, which is where the instability"
           " and nonlinear phase\r\ncome from.\r\n");

    printf("\r\nthe coefficient order is b0 b1 b2 a1 a2 with a1/a2 already"
           " negated;\r\nscipy returns them with the opposite sign.\r\n");

    while (1)
    {
        for (uint32_t n = 0U; n < SIG_LEN; n++)
        {
            g_in  = sig[n];
            g_out = fir_out[n];
            g_ref = biquad_out[n];
            g_gap = fir_out[n] - biquad_out[n];
            probe_step();
        }
    }
}