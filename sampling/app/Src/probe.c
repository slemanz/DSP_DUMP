#include "probe.h"
#include "driver_systick.h"

/*
 * The three values every app in this module streams, one modeled step at a
 * time. They are globals so Ozone can graph them: the data sampling window
 * only accepts expressions whose operands are static variables.
 */
volatile float32_t g_analog;
volatile float32_t g_sampled;
volatile float32_t g_error;

// holds the loop to one modeled step per STEP_MS, so the graph advances at a
// rate the debug probe can follow
void probe_step(void)
{
    static uint64_t next = 0;

    while (ticks_get() < next)
    {
    }

    next = ticks_get() + STEP_MS;
}