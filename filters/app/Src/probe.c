#include "probe.h"
#include "driver_systick.h"

/*
 * A filter has a signal side and a kernel side, so the graph carries both. g_x
 * is what went in and g_y is what came out, g_h is the kernel itself and g_mag
 * is that same kernel seen as a frequency response, and g_ref is whatever the
 * app wants to hold the output against.
 *
 * They are globals because the data sampling window only accepts expressions
 * whose operands are static variables.
 */
volatile float32_t g_x;
volatile float32_t g_y;
volatile float32_t g_h;
volatile float32_t g_mag;
volatile float32_t g_ref;

/*
 * Every app calls this, including the ones that never stream anything. The
 * linker runs with --gc-sections, so a probe no app in this build mentions is
 * dropped from the image, and then Ozone has nothing to attach that trace to
 * and quietly shows one graph fewer.
 */
void probe_reset(void)
{
    g_x   = 0.0f;
    g_y   = 0.0f;
    g_h   = 0.0f;
    g_mag = 0.0f;
    g_ref = 0.0f;
}

/*
 * Holds the loop to one sample per STEP_MS, so the graph advances at a rate the
 * debug probe can follow. 100 ms is slow enough that the sampler keeps up on
 * every trace; at a few milliseconds it starts missing points and the shapes
 * come out distorted, which looks like a bug in the filter and is not.
 */
void probe_step(void)
{
    static uint64_t next = 0;

    while (ticks_get() < next)
    {
    }

    next = ticks_get() + STEP_MS;
}