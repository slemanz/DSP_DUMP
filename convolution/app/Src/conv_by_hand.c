/*
 * Convolution on numbers small enough to check with a pencil. Five input
 * samples, three taps, and every intermediate value printed.
 *
 * Each row is one input sample turned into a scaled copy of the kernel, laid
 * down starting at that sample's own position. The bottom row is the columns
 * added up. That table is the whole operation, and everything else in this
 * module is the same table with more numbers in it.
 *
 *     make conv_by_hand && make load && make monitor
 */
#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "conv.h"

#define X_LEN       5U
#define H_LEN       3U
#define Y_LEN       (X_LEN + H_LEN - 1U)

static const float32_t x[X_LEN] = { 1.0f, 2.0f, 0.0f, -1.0f, 0.5f };
static const float32_t h[H_LEN] = { 1.0f, 0.5f, -0.25f };

static float32_t y[Y_LEN];
static float32_t check[Y_LEN];
static float32_t diff[Y_LEN];

int main(void)
{
    float32_t sum_x;
    float32_t sum_h;
    float32_t sum_y;
    float32_t gap;
    uint32_t index;

    config_app();

    while(1)
    {

    }
}