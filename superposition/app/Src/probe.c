#include "probe.h"
#include "driver_systick.h"

/*
 * Every app in this module compares two routes to the same result: the signal
 * taken through the system whole, against the signal taken apart, put through
 * the system in pieces and put back together. These four values carry that
 * comparison to Ozone, and they are globals because the data sampling window
 * only accepts expressions whose operands are static variables.
 */
volatile float32_t g_input;
volatile float32_t g_path_a;
volatile float32_t g_path_b;
volatile float32_t g_error;

// holds the loop to one step per STEP_MS, so the graph advances at a rate the
// debug probe can follow
void probe_step(void)
{
    static uint64_t next = 0;

    while (ticks_get() < next)
    {
    }

    next = ticks_get() + STEP_MS;
}