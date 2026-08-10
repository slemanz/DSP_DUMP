/*
 * The same table as conv_by_hand, but written twice and watched as it fills in.
 *
 * conv_scatter walks the input and throws a scaled copy of the kernel forward.
 * conv_gather walks the output and reaches back for the input samples that
 * reach it. They are the same sum read from opposite ends.
 *
 * They do not agree to the last bit, and that is worth a moment. The two loops
 * add the same numbers in a different order, and float addition does not care
 * about that only in theory. The gap printed below is not a mistake in either
 * loop, it is the rounding that a different order left behind.
 *
 * On the graph, g_y is the output built from the first few input samples only,
 * and it grows by one copy of the kernel per pass until it reaches g_ref, the
 * finished result. The first pass is one scaled kernel sitting alone.
 *
 *     make conv_algorithm && make load && make debug
 */
#include <stdio.h>
#include "config.h"
#include "signals.h"
#include "probe.h"
#include "conv.h"

#define X_LEN       32U
#define Y_LEN       (X_LEN + LOWPASS_LEN - 1U)

static float32_t x[X_LEN];
static float32_t y_scatter[Y_LEN];
static float32_t y_gather[Y_LEN];
static float32_t partial[Y_LEN];
static float32_t diff[Y_LEN];

int main(void)
{
    float32_t gap;
    uint32_t index;

    config_app();

    arm_copy_f32(input_signal_f32_1kHz_15kHz, x, X_LEN);

    conv_scatter(x, X_LEN, lowpass_6khz, LOWPASS_LEN, y_scatter);
    conv_gather(x, X_LEN, lowpass_6khz, LOWPASS_LEN, y_gather);

    arm_sub_f32(y_scatter, y_gather, diff, Y_LEN);
    arm_absmax_f32(diff, Y_LEN, &gap, &index);

    printf("\r\n%lu samples through %lu taps, %lu out\r\n", (unsigned long)X_LEN, (unsigned long)LOWPASS_LEN, (unsigned long)Y_LEN);
    printf("scatter and gather differ by %.9f\r\n", gap);
    printf("streaming the output as it is built, one input sample per pass\r\n");

    while(1)
    {
        for(uint32_t taken = 1; taken <= X_LEN; taken++)
        {
            /* the finished output is longer than what the first `taken`
             * samples can reach, so clear the tail before filling the head */
            arm_fill_f32(0.0f, partial, Y_LEN);
            conv_scatter(x, taken, lowpass_6khz, LOWPASS_LEN, partial);

            for(uint32_t n = 0; n < Y_LEN; n++)
            {
                g_x   = (n < X_LEN) ? x[n] : 0.0f;
                g_h   = (n < LOWPASS_LEN) ? lowpass_6khz[n] : 0.0f;
                g_y   = partial[n];
                g_ref = y_scatter[n];

                probe_step();
            }
        }

    }
}