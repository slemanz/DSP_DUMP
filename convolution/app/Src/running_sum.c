/*
 * The running sum and the first difference: what an integral and a derivative
 * become once the signal is a list of numbers instead of a curve.
 *
 * Two things are worth taking from this. They undo each other, which is the
 * discrete version of the statement that differentiating an integral gives the
 * function back. And the running sum is a convolution, with a kernel of all
 * ones, so a filter you would write as 9280 multiply accumulates can sometimes
 * be written as one add per sample instead. That second point is the door into
 * recursive filters.
 *
 * On the graph the running sum smooths and the first difference sharpens. Both
 * are doing the same thing to frequency: a slope is small on a slow wave and
 * large on a fast one, and a total is the other way round.
 *
 *     make running_sum && make load && make debug
 */
#include <stdio.h>
#include "config.h"
#include "signals.h"
#include "probe.h"
#include "conv.h"

#define X_LEN       ((uint32_t)KHZ1_15_SIG_LEN)
#define CONV_LEN    (X_LEN + X_LEN - 1U)

static float32_t sum[X_LEN];
static float32_t back[X_LEN];
static float32_t slope[X_LEN];
static float32_t ones[X_LEN];
static float32_t as_conv[CONV_LEN];
static float32_t diff[X_LEN];

int main(void)
{
    config_app();

    while(1)
    {

    }
}