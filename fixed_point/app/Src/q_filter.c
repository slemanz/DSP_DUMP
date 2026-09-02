/*
 * The same filter three ways, measuring what the narrower format costs twice
 * over: once in error, once in cycles.
 *
 * Same signal, same taps, same answer wanted. Only the number format changes,
 * and the f32 result is taken as the truth because it is the one with the most
 * bits behind it.
 *
 * Two things are worth watching. The error, which is what the format cost in
 * precision: fixed point error is a fixed size everywhere, so it is a large
 * fraction of a small sample and a small fraction of a large one. And the
 * cycles, which is what it bought back, and which are not what people expect:
 * this part has an FPU, so the f32 row is not the slow one. Where q15 wins is
 * width, not arithmetic.
 *
 * The library wants the taps time reversed, as the filter chapter found, and it
 * wants a state buffer as the block chapter found. Both of those apply to every
 * format equally.
 *
 *     make q_filter && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "testsig.h"
#include "driver_clock.h"
#include "driver_systick.h"

#define BLOCK       32U
#define BLOCKS      (SIG_LEN / BLOCK)
#define REPEATS     8U

static float32_t rev_f32[TAPS];
static q31_t     rev_q31[TAPS];
static q15_t     rev_q15[TAPS];

static float32_t state_f32[TAPS + BLOCK - 1U];
static q31_t     state_q31[TAPS + BLOCK - 1U];
static q15_t     state_q15[TAPS + BLOCK - 1U];

static q31_t     sig_q31[SIG_LEN];
static q15_t     sig_q15[SIG_LEN];

static float32_t out_f32[SIG_LEN];
static q31_t     out_q31[SIG_LEN];
static q15_t     out_q15[SIG_LEN];

static float32_t back_q31[SIG_LEN];
static float32_t back_q15[SIG_LEN];
static float32_t error[SIG_LEN];

static void report(const char *name, const float32_t *pBack, float32_t step)
{
    float32_t worst = 0.0f;
    float32_t sigma;

    for (uint32_t n = 0U; n < SIG_LEN; n++)
    {
        error[n] = pBack[n] - out_f32[n];

        if (fabsf(error[n]) > worst)
        {
            worst = fabsf(error[n]);
        }
    }

    arm_std_f32(error, SIG_LEN, &sigma);

    printf("%-8s %14.3e %14.3e %12.2f %12.1f\r\n", name, (double)worst,
           (double)sigma, (double)(worst / step),
           (double)(20.0f * log10f(0.9f / sigma)));
}

int main(void)
{
    config_app();
    probe_reset();

    for (uint32_t i = 0U; i < TAPS; i++)
    {
        rev_f32[i] = taps[TAPS - 1U - i];
    }

    arm_float_to_q31(rev_f32, rev_q31, TAPS);
    arm_float_to_q15(rev_f32, rev_q15, TAPS);
    arm_float_to_q31(sig, sig_q31, SIG_LEN);
    arm_float_to_q15(sig, sig_q15, SIG_LEN);

    arm_fir_instance_f32 f32_inst;
    arm_fir_instance_q31 q31_inst;
    arm_fir_instance_q15 q15_inst;

    arm_fir_init_f32(&f32_inst, TAPS, rev_f32, state_f32, BLOCK);
    arm_fir_init_q31(&q31_inst, TAPS, rev_q31, state_q31, BLOCK);

    arm_status ok = arm_fir_init_q15(&q15_inst, TAPS, rev_q15, state_q15,
                                     BLOCK);

    printf("\r\narm_fir_init_q15 returned %s\r\n",
           (ok == ARM_MATH_SUCCESS) ? "SUCCESS" : "ARGUMENT_ERROR");
    printf("some versions of the library only accept an even numTaps for the"
           " q15 filter.\r\nthis kernel has %lu taps.\r\n\r\n",
           (unsigned long)TAPS);

    /* one pass, for the error each format leaves behind */
    for (uint32_t b = 0U; b < BLOCKS; b++)
    {
        arm_fir_f32(&f32_inst, &sig[b * BLOCK], &out_f32[b * BLOCK], BLOCK);
        arm_fir_q31(&q31_inst, &sig_q31[b * BLOCK], &out_q31[b * BLOCK], BLOCK);
        arm_fir_q15(&q15_inst, &sig_q15[b * BLOCK], &out_q15[b * BLOCK], BLOCK);
    }

    arm_q31_to_float(out_q31, back_q31, SIG_LEN);
    arm_q15_to_float(out_q15, back_q15, SIG_LEN);

    printf("%-8s %14s %14s %12s %12s\r\n",
           "format", "worst error", "sigma", "in steps", "snr dB");

    report("q31", back_q31, 1.0f / Q31_ONE);
    report("q15", back_q15, 1.0f / Q15_ONE);

    printf("\r\na 12 bit converter delivers about 74 dB, so q15 is already"
           " ahead of it.\r\nthe q31 step count is measuring the f32"
           " reference, not q31 failing.\r\n");

    /* REPEATS passes on the same instances, for cycles */
    uint32_t spent[3];

    cycles_start();
    for (uint32_t r = 0U; r < REPEATS; r++)
    {
        for (uint32_t b = 0U; b < BLOCKS; b++)
        {
            arm_fir_f32(&f32_inst, &sig[b * BLOCK], &out_f32[b * BLOCK], BLOCK);
        }
    }
    spent[0] = cycles_read();
    systick_init(TICK_HZ);

    cycles_start();
    for (uint32_t r = 0U; r < REPEATS; r++)
    {
        for (uint32_t b = 0U; b < BLOCKS; b++)
        {
            arm_fir_q31(&q31_inst, &sig_q31[b * BLOCK], &out_q31[b * BLOCK],
                        BLOCK);
        }
    }
    spent[1] = cycles_read();
    systick_init(TICK_HZ);

    cycles_start();
    for (uint32_t r = 0U; r < REPEATS; r++)
    {
        for (uint32_t b = 0U; b < BLOCKS; b++)
        {
            arm_fir_q15(&q15_inst, &sig_q15[b * BLOCK], &out_q15[b * BLOCK],
                        BLOCK);
        }
    }
    spent[2] = cycles_read();
    systick_init(TICK_HZ);

    uint32_t samples = SIG_LEN * REPEATS;

    printf("\r\ncore at %lu Hz, %lu taps, %lu samples per pass\r\n\r\n",
           (unsigned long)clock_hclk(), (unsigned long)TAPS,
           (unsigned long)SIG_LEN);

    printf("%-8s %12s %12s %10s   %10s\r\n",
           "format", "cycles", "per sample", "vs f32", "bytes");

    static const char *const names[] = { "f32", "q31", "q15" };
    static const uint32_t width[] = { 4U, 4U, 2U };

    for (uint32_t k = 0U; k < 3U; k++)
    {
        printf("%-8s %12lu %12.2f %10.2f   %10lu\r\n", names[k],
               (unsigned long)spent[k],
               (double)((float32_t)spent[k] / (float32_t)samples),
               (double)((float32_t)spent[0] / (float32_t)spent[k]),
               (unsigned long)(width[k] * SIG_LEN));
    }

    printf("\r\nthis part has an fpu, so f32 is not the slow row here. q15"
           " wins on\r\nwidth, two samples per register and half the bytes to"
           " move.\r\n");

    while (1)
    {
        for (uint32_t n = 0U; n < SIG_LEN; n++)
        {
            g_f32 = out_f32[n];
            g_q31 = back_q31[n];
            g_q15 = back_q15[n];
            g_err = back_q15[n] - out_f32[n];
            probe_step();
        }
    }
}