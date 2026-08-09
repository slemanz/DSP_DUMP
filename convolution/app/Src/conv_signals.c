/*
 * Convolution needs two signals before it can produce a third, so this app
 * looks at the two it will be given and computes nothing. The input is a slow
 * 1 kHz wave with a fast 15 kHz wave riding on it. The impulse response is 29
 * numbers, and those 29 numbers are the entire system.
 *
 *     make conv_signals && make load && make debug
 */
#include <stdio.h>
#include "config.h"
#include "signals.h"
#include "probe.h"
#include "conv.h"

#define X_LEN       ((uint32_t)KHZ1_15_SIG_LEN)
#define Y_LEN       ((uint32_t)X_LEN + LOWPASS_LEN - 1U)

int main(void)
{
    float32_t sum;

    config_app();

    arm_accumulate_f32(lowpass_6khz, LOWPASS_LEN, &sum);

    printf("\r\ninput  %lu samples, 1 kHz + 15 kHz at 48 kHz\r\n", (unsigned long)X_LEN);
    printf("kernel %lu taps, summing to %.4f\r\n", (unsigned long)LOWPASS_LEN, sum);
    printf("output %lu samples once they are convolved\r\n", (unsigned long)Y_LEN);

    uint32_t i = 0;
    uint32_t j = 0;
    
    while(1)
    {
        g_x = input_signal_f32_1kHz_15kHz[i];
        g_h = lowpass_6khz[j];

        i++;
        j++;

        /* the kernel is short, so it wraps and repeats its shape while the
         * input plays once. Nothing is lined up here, only shown. */
        if (i == X_LEN)
        {
            i = 0;
        }

        if (j == LOWPASS_LEN)
        {
            j = 0;
        }

        probe_step();
    }
}