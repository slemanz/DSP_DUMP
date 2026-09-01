#include "probe.h"
#include "driver_systick.h"

/*
 * The chapter is one signal carried in three number formats, so the graph shows
 * all three on the same axis with the gap underneath. Everything is streamed as
 * a float, including the fixed point traces: they are converted back before
 * they reach the probe, because what is being compared is the value each format
 * managed to hold, not the integer it held it in.
 *
 * They are globals because the data sampling window only accepts expressions
 * whose operands are static variables.
 */
volatile float32_t g_f32;
volatile float32_t g_q31;
volatile float32_t g_q15;
volatile float32_t g_err;

void probe_reset(void)
{
    g_f32 = 0.0f;
    g_q31 = 0.0f;
    g_q15 = 0.0f;
    g_err = 0.0f;
}

/*
 * Holds the loop to one sample per STEP_MS, so the graph advances at a rate the
 * debug probe can follow. 100 ms is slow enough that the sampler keeps up; at a
 * few milliseconds it starts missing points, and a missed point looks exactly
 * like a quantisation error, which in this chapter is the last confusion you
 * want.
 */
void probe_step(void)
{
    static uint64_t next = 0;

    while (ticks_get() < next)
    {
    }

    next = ticks_get() + STEP_MS;
}
