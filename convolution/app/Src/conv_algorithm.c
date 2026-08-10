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

    while(1)
    {
        
    }
}