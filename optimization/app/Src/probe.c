#include "probe.h"
#include "driver_systick.h"

/*
 * Nothing here is a signal. The whole chapter measures the same arithmetic
 * arranged differently, so the graph carries the measurement: what it cost,
 * what that is per multiply accumulate, how much better than the starting
 * point, and how much code it took.
 *
 * g_per_mac is the one to watch across apps, because it is the only number that
 * survives a change of workload. Cycles depend on how many samples there were;
 * cycles per multiply accumulate does not.
 *
 * They are globals because the data sampling window only accepts expressions
 * whose operands are static variables.
 */
volatile float32_t g_cycles;
volatile float32_t g_per_mac;
volatile float32_t g_speedup;
volatile float32_t g_bytes;

void probe_reset(void)
{
    g_cycles  = 0.0f;
    g_per_mac = 0.0f;
    g_speedup = 0.0f;
    g_bytes   = 0.0f;
}

/*
 * Holds the loop to one sample per STEP_MS, so the graph advances at a rate the
 * debug probe can follow. 100 ms is slow enough that the sampler keeps up; at a
 * few milliseconds it starts missing points and the steps come out ragged.
 */
void probe_step(void)
{
    static uint64_t next = 0;

    while (ticks_get() < next)
    {
    }

    next = ticks_get() + STEP_MS;
}
