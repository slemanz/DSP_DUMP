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

    while(1)
    {

    }
}