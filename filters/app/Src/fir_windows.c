/*
 * Where the taps come from, and what the window is for.
 *
 * A filter that passes everything below a cutoff and nothing above it has one
 * kernel, and that kernel is a sinc. The sinc never ends. Any kernel that fits
 * in memory is a piece cut out of it, and cutting a signal off abruptly is the
 * same act that produced the ringing in the square wave two chapters ago: the
 * cut is a step, and a step is broad in frequency.
 *
 * The window is the fix. Instead of stopping the sinc dead, taper it to zero.
 * All three kernels here are the same sinc at the same cutoff and the same
 * length, tapered three ways, and the two columns move in opposite directions.
 *
 *     make fir_windows && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "filters.h"
#include "kernels.h"
#include "testsig.h"

#define FC_HZ       200.0f
#define SWEEP       201U        /* frequency points streamed to the graph */

static const float32_t *const kernel[] = { lp_rect, lp_hamming, lp_blackman };
static const char *const name[] = { "rectangular", "hamming", "blackman" };

/*
 * The tallest ripple left after the response stops falling. firwin puts the
 * -6 dB point exactly on the cutoff, so the walk starts there, goes down to the
 * first null, and keeps the largest thing after it.
 */
static float32_t stopband(const float32_t *pH, uint32_t hLen)
{

}

/* how far it takes to get from 99% of the passband down to 1% */
static float32_t transition(const float32_t *pH, uint32_t hLen)
{
    return 0.0f;
}


int main(void)
{
    config_app();
    probe_reset();

    while(1)
    {

    }
}