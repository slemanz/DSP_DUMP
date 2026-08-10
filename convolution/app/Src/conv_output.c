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

    conv_scatter(input_signal_f32_1kHz_15kHz, X_LEN, lowpass_6khz, LOWPASS_LEN, y);

    printf("\r\n%lu + %lu - 1 = %lu samples out\r\n", (unsigned long)X_LEN, (unsigned long)LOWPASS_LEN, (unsigned long)Y_LEN);
    printf("the output trails the input by %lu samples\r\n", (unsigned long)GROUP_DELAY);
    printf("the first and last %lu samples are startup and tail\r\n", (unsigned long)(LOWPASS_LEN - 1U));

    /* The 1 kHz wave reaches 1.0 on its own and the 15 kHz adds another 0.5 on
     * top of it. If the peak comes back near 1.0 once the transients are
     * skipped, the fast one is gone and the slow one was left alone. */
    arm_absmax_f32(input_signal_f32_1kHz_15kHz, X_LEN, &peak_in, &index);
    arm_absmax_f32(&y[LOWPASS_LEN - 1U], X_LEN - LOWPASS_LEN, &peak_out, &index);

    printf("peak in %.4f, peak out %.4f\r\n", peak_in, peak_out);

    while(1)
    {
        /* one index for all four traces. Ozone gives every variable its own
         * track, so the signals are padded to a common length instead of
         * being wrapped to fill the same one. */
        for(uint32_t k = 0; k < Y_LEN; k++)
        {
            g_x   = (k < X_LEN) ? input_signal_f32_1kHz_15kHz[k] : 0.0f;
            g_h   = (k < LOWPASS_LEN) ? lowpass_6khz[k] : 0.0f;
            g_y   = y[k];
            g_ref = ((k >= GROUP_DELAY) && ((k - GROUP_DELAY) < X_LEN)) ?
                    input_signal_f32_1kHz_15kHz[k - GROUP_DELAY] : 0.0f;

            probe_step();
        }
    }
}