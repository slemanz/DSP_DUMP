#include "probe.h"
#include "driver_systick.h"

/*
 * Every property in this module is one question asked about a different
 * operation: does it matter whether the operation happens before the system or
 * after it? Scaling, adding, shifting, splitting a signal into pieces. On a
 * linear shift invariant system the answer is always no, and that is the only
 * thing the four properties are saying.
 *
 * g_before is the route where the operation happens first, g_after is the route
 * where the system happens first, and g_gap is what is left between them. The
 * property holds wherever g_gap is a flat line at zero.
 *
 * They are globals because the data sampling window only accepts expressions
 * whose operands are static variables.
 */
volatile float32_t g_x;
volatile float32_t g_before;
volatile float32_t g_after;
volatile float32_t g_gap;

/*
 * Every app calls this. The linker runs with --gc-sections, so a probe that no
 * app in the build mentions is dropped from the image, and Ozone then has
 * nothing to attach that trace to and quietly draws one graph fewer. Touching
 * all four here keeps the window the same across the module.
 */
void probe_reset(void)
{
    g_x      = 0.0f;
    g_before = 0.0f;
    g_after  = 0.0f;
    g_gap    = 0.0f;
}

// holds the loop to one sample per STEP_MS, so the graph advances at a rate the
// debug probe can follow
void probe_step(void)
{
    static uint64_t next = 0;

    while (ticks_get() < next)
    {
    }

    next = ticks_get() + STEP_MS;
}