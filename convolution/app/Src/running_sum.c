/*
 * The running sum and the first difference: what an integral and a derivative
 * become once the signal is a list of numbers instead of a curve.
 *
 * Two things are worth taking from this. They undo each other, which is the
 * discrete version of the statement that differentiating an integral gives the
 * function back. And the running sum is a convolution, with a kernel of all
 * ones, so a filter you would write as 9280 multiply accumulates can sometimes
 * be written as one add per sample instead. That second point is the door into
 * recursive filters.
 *
 * On the graph the running sum smooths and the first difference sharpens. Both
 * are doing the same thing to frequency: a slope is small on a slow wave and
 * large on a fast one, and a total is the other way round.
 *
 *     make running_sum && make load && make debug
 */
#include <stdio.h>
#include "config.h"
#include "signals.h"
#include "probe.h"
#include "conv.h"

#define X_LEN       ((uint32_t)KHZ1_15_SIG_LEN)
#define CONV_LEN    (X_LEN + X_LEN - 1U)

static float32_t sum[X_LEN];
static float32_t back[X_LEN];
static float32_t slope[X_LEN];
static float32_t ones[X_LEN];
static float32_t as_conv[CONV_LEN];
static float32_t diff[X_LEN];

// each output is every input up to and including this one, added together
static void running_sum(const float32_t *pSrc, float32_t *pDst, uint32_t len)
{
    pDst[0] = pSrc[0];

    for(uint32_t n = 1; n < len; n++)
    {
        pDst[n] = pDst[n - 1U] + pSrc[n];
    }
}

// each output is the step the input took to get here
static void first_difference(const float32_t *pSrc, float32_t *pDst, uint32_t len)
{
    pDst[0] = pSrc[0];

    for(uint32_t n = 1; n < len; n++)
    {
        pDst[n] = pSrc[n] - pSrc[n - 1U];
    }
}

int main(void)
{
    float32_t gap;
    uint32_t index;

    config_app();

    running_sum(input_signal_f32_1kHz_15kHz, sum, X_LEN);
    first_difference(sum, back, X_LEN);
    first_difference(input_signal_f32_1kHz_15kHz, slope, X_LEN);

    arm_sub_f32(back, input_signal_f32_1kHz_15kHz, diff, X_LEN);
    arm_absmax_f32(diff, X_LEN, &gap, &index);
    printf("\r\nthe difference of the sum is the signal again, off by %.9f\r\n", gap);

    /* a kernel of ones as long as the signal reaches every earlier sample, so
     * the first X_LEN outputs of the convolution are the running sum */
    arm_fill_f32(1.0f, ones, X_LEN);
    arm_conv_f32(input_signal_f32_1kHz_15kHz, X_LEN, ones, X_LEN, as_conv);

    arm_sub_f32(as_conv, sum, diff, X_LEN);
    arm_absmax_f32(diff, X_LEN, &gap, &index);
    printf("convolving with %lu ones gives the same sum, off by %.9f\r\n", (unsigned long)X_LEN, gap);
    printf("one add per sample against %lu multiply accumulates\r\n", (unsigned long)(X_LEN * X_LEN));

    while(1)
    {
        for (uint32_t n = 0; n < X_LEN; n++)
        {
            g_x   = input_signal_f32_1kHz_15kHz[n];
            g_h   = 1.0f;
            g_y   = sum[n];
            g_ref = slope[n];

            probe_step();
        }
    }
}