#include "probe.h"
#include "driver_systick.h"

/*
 * The signal in this chapter comes from outside the chip, so the graph carries
 * it raw and converted, next to the two things that decide whether it means
 * anything: the rate it arrived at and how steady that rate was.
 *
 * They are globals because the data sampling window only accepts expressions
 * whose operands are static variables.
 */
volatile float32_t g_raw;
volatile float32_t g_volts;
volatile float32_t g_rate;
volatile float32_t g_jitter;

void probe_reset(void)
{
    g_raw    = 0.0f;
    g_volts  = 0.0f;
    g_rate   = 0.0f;
    g_jitter = 0.0f;
}

/*
 * Holds the loop to one sample per STEP_MS, so the graph advances at a rate the
 * debug probe can follow. 100 ms is slow enough that the sampler keeps up; at a
 * few milliseconds it starts missing points and the traces come out distorted,
 * which looks like an acquisition fault and is not.
 *
 * Note what this means in this chapter: the graph is not showing the sampling
 * rate. The samples are taken at the rate the timer sets and then paraded past
 * the probe slowly, one per 100 ms, so the shape is right and the time axis is
 * the debugger's, not the signal's.
 */
void probe_step(void)
{
    static uint64_t next = 0;

    while (ticks_get() < next)
    {
    }

    next = ticks_get() + STEP_MS;
}
