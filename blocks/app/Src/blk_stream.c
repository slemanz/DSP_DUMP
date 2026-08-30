/*
 * A filter that never sees an array.
 *
 * The sensor hands over one sample. The filter needs the last seventeen. The
 * way out is that the filter keeps them itself, in a circular buffer inside its
 * own instance, so every call already has the window it needs and nobody has to
 * assemble one.
 *
 * This is what the lesson builds, and the interesting part is that the answer is
 * not approximately the block answer, it is the block answer. Same multiply
 * accumulate, same numbers, different bookkeeping. The check below runs the
 * whole signal through both and compares.
 *
 * The cost is that the entire filter runs inside whatever context the sample
 * arrives in, once per sample, at the sampling rate. Seventeen taps at 2 kHz is
 * fine. Two hundred taps at 48 kHz is not, and that is the argument for the
 * other arrangement.
 *
 *     make blk_stream && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "stream.h"
#include "kernels.h"
#include "testsig.h"

#define TAPS        LP_31_LEN
#define OUT_LEN     (SIG_LEN + TAPS - 1U)
#define DELAY       ((TAPS - 1U) / 2U)

static float32_t streamed[SIG_LEN];
static float32_t blocked[OUT_LEN];
static stream_fir_t fir;

int main(void)
{
    config_app();
    probe_reset();

    /* one at a time, the way a sensor delivers */
    stream_init(&fir, lp_31, TAPS);

    for (uint32_t n = 0U; n < SIG_LEN; n++)
    {
        streamed[n] = stream_step(&fir, sig_3tone[n]);
    }

    /* and all at once, the way the convolution chapter did it */
    arm_conv_f32(sig_3tone, SIG_LEN, lp_31, TAPS, blocked);

    float32_t worst = 0.0f;
    uint32_t  where = 0U;

    for (uint32_t n = 0U; n < SIG_LEN; n++)
    {
        float32_t gap = fabsf(streamed[n] - blocked[n]);

        if (gap > worst)
        {
            worst = gap;
            where = n;
        }
    }

    printf("\r\n%lu samples, %lu taps\r\n\r\n", (unsigned long)SIG_LEN,
           (unsigned long)TAPS);

    printf("  %-30s %6lu\r\n", "history the filter keeps", (unsigned long)TAPS);
    printf("  %-30s %6lu\r\n", "outputs from streaming",
           (unsigned long)SIG_LEN);
    printf("  %-30s %6lu\r\n", "outputs from arm_conv_f32",
           (unsigned long)OUT_LEN);
    printf("  %-30s %6lu\r\n", "the tail streaming never emits",
           (unsigned long)(TAPS - 1U));

    printf("\r\n  %-30s %.9f at n = %lu\r\n", "worst gap between them",
           (double)worst, (unsigned long)where);

    printf("\r\nnot approximately the same answer. the same answer. the only"
           " thing\r\nstreaming gives up is the tail, and the tail is the"
           " filter still\r\nringing after the input stopped, which a stream"
           " that has not stopped\r\ndoes not have yet.\r\n");

    printf("\r\nthe cost is where the work happens: %lu multiply accumulates"
           " inside\r\nthe sample handler, every sample, at whatever the"
           " sampling rate is.\r\n", (unsigned long)TAPS);

    while (1)
    {
        for (uint32_t n = 0U; n < SIG_LEN; n++)
        {
            g_in     = sig_3tone[n];
            g_stream = streamed[n];
            g_block  = blocked[n];
            g_gap    = streamed[n] - blocked[n];
            probe_step();
        }
    }
}