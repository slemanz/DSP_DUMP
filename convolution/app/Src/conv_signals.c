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

    while(1)
    {

    }
}