/*
 * The whole input through the whole kernel, with the three signals lined up on
 * one time axis so the output can be read against what went in.
 *
 * Three things are visible. The fast ripple is gone, because 15 kHz is past
 * the 6 kHz cutoff. The output is late, by half the kernel, and g_ref is the
 * input pushed back by exactly that much so the two can be laid over each
 * other. The first and last few samples are wrong on purpose, because there
 * the kernel is hanging off the end of the input and part of the sum is
 * missing.
 *
 *     make conv_output && make load && make debug
 */
#include <stdio.h>
#include "config.h"
#include "signals.h"
#include "probe.h"
#include "conv.h"

#define X_LEN           ((uint32_t)KHZ1_15_SIG_LEN)
#define Y_LEN           (X_LEN + LOWPASS_LEN - 1U)
#define GROUP_DELAY     ((LOWPASS_LEN - 1U) / 2U)

static float32_t y[Y_LEN];

int main(void)
{
    float32_t peak_in;
    float32_t peak_out;
    uint32_t index;

    config_app();

    while(1)
    {

    }
}