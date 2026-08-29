#include "probe.h"
#include "driver_systick.h"

/*
 * The chapter is one signal put through two different arrangements, so the
 * graph carries the input once and the two answers side by side, plus the gap
 * between them. When the arrangement is right that last trace is a flat line at
 * zero, and when it is not, the shape of what is left says which seam leaked.
 *
 * They are globals because the data sampling window only accepts expressions
 * whose operands are static variables.
 */
volatile float32_t g_in;
volatile float32_t g_stream;
volatile float32_t g_block;
volatile float32_t g_gap;

void probe_reset(void)
{
    g_in     = 0.0f;
    g_stream = 0.0f;
    g_block  = 0.0f;
    g_gap    = 0.0f;
}

/*
 * Holds the loop to one sample per STEP_MS, so the graph advances at a rate the
 * debug probe can follow. 100 ms is slow enough that the sampler keeps up; at a
 * few milliseconds it starts missing points and the traces come out distorted,
 * which reads as a seam that is not there.
 */
void probe_step(void)
{
    static uint64_t next = 0;

    while (ticks_get() < next)
    {
    }

    next = ticks_get() + STEP_MS;
}
