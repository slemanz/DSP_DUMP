/*
 * Four kernels short enough to read, cycled one after another so the effect of
 * each one on the same input can be watched on the graph.
 *
 *   a single 1        gives the input back untouched, so the delta function is
 *                     to convolution what 1 is to multiplication
 *   a single 2        doubles it
 *   a 1 eight late    delays it by eight samples
 *   1 and 0.6 later   adds a quieter copy behind the original
 *
 * Nothing here is filtering. The point is narrower and worth more: the kernel
 * is not a setting the system takes, the kernel is the system.
 *
 *     make conv_kernels && make load && make debug
 */
#include <stdio.h>
#include "config.h"
#include "signals.h"
#include "probe.h"
#include "conv.h"

#define X_LEN               96U     // two periods of the 1 kHz component
#define H_MAX               13U
#define Y_LEN               (X_LEN + H_MAX - 1U)
#define PASSES_PER_KERNEL   3U

static const float32_t k_identity[1]  = { 1.0f };
static const float32_t k_double[1]    = { 2.0f };
static const float32_t k_delay[9]     = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
static const float32_t k_echo[13]     = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.6f };

static const struct
{
    const char      *name;
    const float32_t *taps;
    uint32_t        len;
}kernels[] = {
    {"identity",        k_identity, ARRAY_LEN(k_identity)},
    {"double",          k_double,   ARRAY_LEN(k_double)},
    {"delay by 8",      k_delay,    ARRAY_LEN(k_delay)},
    {"echo 0.6 after 12", k_echo,   ARRAY_LEN(k_echo)}
};
    
static float32_t x[X_LEN];
static float32_t y[Y_LEN];

int main(void)
{
    config_app();

    arm_copy_f32(input_signal_f32_1kHz_15kHz, x, X_LEN);

    printf("\r\nsame input through four kernels, %lu passes each\r\n", (unsigned long)PASSES_PER_KERNEL);

    /* nothing is being compared here, so the fourth trace has nothing to carry */
    g_ref = 0.0f;

    while (1)
    {
        for(uint32_t s = 0; s < ARRAY_LEN(kernels); s++)
        {
            arm_fill_f32(0.0f, y, Y_LEN);
            conv_scatter(x, X_LEN, kernels[s].taps, kernels[s].len, y);

            printf("%s, %lu tap(s)\r\n", kernels[s].name, (unsigned long)kernels[s].len);

            for (uint32_t p = 0; p < PASSES_PER_KERNEL; p++)
            {
                for (uint32_t n = 0; n < Y_LEN; n++)
                {
                    g_x = (n < X_LEN) ? x[n] : 0.0f;
                    g_h = (n < kernels[s].len) ? kernels[s].taps[n] : 0.0f;
                    g_y = y[n];

                    probe_step();
                }
            }
        }
    }
}
