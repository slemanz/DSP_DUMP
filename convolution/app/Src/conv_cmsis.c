/*
 * The library call, next to the loop from conv.c.
 *
 * arm_conv_f32 takes the same five arguments in the same order, so the swap is
 * a change of name. It also calls them pSrcA and pSrcB rather than input and
 * kernel, and that naming is honest: convolution does not know which of the
 * two signals is the system. Convolving the kernel with the input gives the
 * same 348 numbers as convolving the input with the kernel, which is what
 * commutative means here.
 *
 *     make conv_cmsis && make load && make debug
 */
#include <stdio.h>
#include "config.h"
#include "signals.h"
#include "probe.h"
#include "conv.h"

#define X_LEN       ((uint32_t)KHZ1_15_SIG_LEN)
#define Y_LEN       (X_LEN + LOWPASS_LEN - 1U)

static float32_t y_mine[Y_LEN];
static float32_t y_cmsis[Y_LEN];
static float32_t y_swapped[Y_LEN];
static float32_t diff[Y_LEN];

int main(void)
{
    float32_t gap;
    uint32_t index;

    config_app();

    conv_scatter(input_signal_f32_1kHz_15kHz, X_LEN, lowpass_6khz, LOWPASS_LEN, y_mine);
    arm_conv_f32(input_signal_f32_1kHz_15kHz, X_LEN, lowpass_6khz, LOWPASS_LEN, y_cmsis);
    arm_conv_f32(lowpass_6khz, LOWPASS_LEN, input_signal_f32_1kHz_15kHz, X_LEN, y_swapped);

    arm_sub_f32(y_mine, y_cmsis, diff, Y_LEN);
    arm_absmax_f32(diff, Y_LEN, &gap, &index);
    printf("\r\nconv_scatter against arm_conv_f32: %.9f\r\n", gap);

    arm_sub_f32(y_cmsis, y_swapped, diff, Y_LEN);
    arm_absmax_f32(diff, Y_LEN, &gap, &index);
    printf("input * kernel against kernel * input: %.9f\r\n", gap);

    while(1)
    {
        for (uint32_t k = 0; k < Y_LEN; k++)
        {
            g_x   = (k < X_LEN) ? input_signal_f32_1kHz_15kHz[k] : 0.0f;
            g_h   = (k < LOWPASS_LEN) ? lowpass_6khz[k] : 0.0f;
            g_y   = y_mine[k];
            g_ref = y_cmsis[k];

            probe_step();
        }
    }
}