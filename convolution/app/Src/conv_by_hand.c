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

    printf("\r\n%lu input samples and %lu taps, so %lu output samples\r\n\r\n", (unsigned long)X_LEN, (unsigned long)H_LEN, (unsigned long)Y_LEN);

    printf("%9s", "n");

    for (uint32_t n = 0; n < Y_LEN; n++)
    {
        printf("%7lu", (unsigned long)n);
    }
    printf("\r\n");

    for (uint32_t i = 0; i < X_LEN; i++)
    {
        printf("x[%lu]=%+.1f", (unsigned long)i, x[i]);

        for (uint32_t n = 0; n < Y_LEN; n++)
        {
            if ((n < i) || (n >= (i + H_LEN)))
            {
                printf("      .");
            }
            else
            {
                printf("%7.3f", x[i] * h[n - i]);
            }
        }

        printf("\r\n");
    }

    conv_scatter(x, X_LEN, h, H_LEN, y);

    printf("%9s", "");

    for (uint32_t n = 0; n < Y_LEN; n++)
    {
        printf("-------");
    }

    printf("\r\n%9s", "y");

    for (uint32_t n = 0; n < Y_LEN; n++)
    {
        printf("%7.3f", y[n]);
    }

    printf("\r\n");

    /* Adding a column costs a multiply and an add per tap, so the whole table
     * is X_LEN * H_LEN multiply accumulates and nothing more. */
    printf("\r\n%lu multiply accumulates built that row\r\n", (unsigned long)(X_LEN * H_LEN));

    /* Every input sample gets spread over the taps and nothing is lost, so the
     * totals have to multiply out. This is why a kernel whose taps sum to one
     * leaves the average of a signal where it found it. */
    arm_accumulate_f32(x, X_LEN, &sum_x);
    arm_accumulate_f32(h, H_LEN, &sum_h);
    arm_accumulate_f32(y, Y_LEN, &sum_y);

    printf("sum(x) %.3f times sum(h) %.3f is %.3f, and sum(y) is %.3f\r\n", sum_x, sum_h, sum_x * sum_h, sum_y);

    arm_conv_f32(x, X_LEN, h, H_LEN, check);
    arm_sub_f32(y, check, diff, Y_LEN);
    arm_absmax_f32(diff, Y_LEN, &gap, &index);

    printf("arm_conv_f32 lands on the same row, off by %.9f\r\n", gap);

    while(1)
    {

    }
}