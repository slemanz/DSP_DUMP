#include "probe.h"
#include "driver_systick.h"

/*
 * Convolution relates three signals, so the graph carries three: the input, the
 * impulse response and the output. g_ref carries whatever the app is putting
 * the output next to. They are globals because the data sampling window only
 * accepts expressions whose operands are static variables.
 */
volatile float32_t g_x;
volatile float32_t g_h;
volatile float32_t g_y;
volatile float32_t g_ref;

void probe_step(void)
{
    static uint64_t next = 0;

    while (ticks_get() < next)
    {
    }

    next = ticks_get() + STEP_MS;
}