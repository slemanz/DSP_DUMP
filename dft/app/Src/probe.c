#include "probe.h"
#include "driver_systick.h"

/*
 * The DFT has an input in one domain and an output in another, so the graph
 * carries both. g_x is the signal going in, g_re and g_im are the two halves of
 * what comes out, g_mag is the height of each frequency regardless of how it
 * was split between them, and g_rebuilt is the signal after a trip back.
 *
 * They are globals because the data sampling window only accepts expressions
 * whose operands are static variables.
 */
volatile float32_t g_x;
volatile float32_t g_re;
volatile float32_t g_im;
volatile float32_t g_mag;
volatile float32_t g_rebuilt;

/*
 * Every app calls this, including the two that never stream anything. The
 * linker runs with --gc-sections, so a probe no app in this build mentions is
 * dropped from the image, and then Ozone has nothing to attach that trace to
 * and quietly shows one graph fewer. Touching all five here keeps the window
 * layout the same across the whole module, and leaves the unused ones as a flat
 * line at zero rather than as a gap.
 */
void probe_reset(void)
{
    g_x       = 0.0f;
    g_re      = 0.0f;
    g_im      = 0.0f;
    g_mag     = 0.0f;
    g_rebuilt = 0.0f;
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