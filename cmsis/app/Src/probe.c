#include "probe.h"
#include "driver_systick.h"

/*
 * This chapter is mostly about conventions, so most of what it has to say is on
 * the terminal. The graph carries the one thing worth seeing: a library routine
 * and the hand written version of the same thing, on the same axis, with the
 * gap underneath. When the conventions have been read correctly that gap is a
 * flat line at zero, and when they have not it is the shape of which convention
 * was missed.
 *
 * They are globals because the data sampling window only accepts expressions
 * whose operands are static variables.
 */
volatile float32_t g_in;
volatile float32_t g_out;
volatile float32_t g_ref;
volatile float32_t g_gap;

void probe_reset(void)
{
    g_in  = 0.0f;
    g_out = 0.0f;
    g_ref = 0.0f;
    g_gap = 0.0f;
}

/*
 * Holds the loop to one sample per STEP_MS, so the graph advances at a rate the
 * debug probe can follow. 100 ms is slow enough that the sampler keeps up; at a
 * few milliseconds it starts missing points and the traces come out distorted.
 */
void probe_step(void)
{
    static uint64_t next = 0;

    while (ticks_get() < next)
    {
    }

    next = ticks_get() + STEP_MS;
}
