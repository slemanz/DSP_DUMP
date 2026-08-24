#include "probe.h"
#include "driver_systick.h"

/*
 * This chapter measures the machine rather than a signal, so the graph carries
 * numbers about the machine. g_mhz is whatever the core is running at, g_cycles
 * is what a fixed piece of work costs in cycles, g_ms is what those cycles come
 * to in time, and g_gain is the ratio against the slowest configuration.
 *
 * Keeping cycles and milliseconds on separate traces is the point of the
 * chapter: raising the clock moves one of them and not the other.
 *
 * They are globals because the data sampling window only accepts expressions
 * whose operands are static variables.
 */
volatile float32_t g_mhz;
volatile float32_t g_cycles;
volatile float32_t g_ms;
volatile float32_t g_gain;

void probe_reset(void)
{
    g_mhz    = 0.0f;
    g_cycles = 0.0f;
    g_ms     = 0.0f;
    g_gain   = 0.0f;
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