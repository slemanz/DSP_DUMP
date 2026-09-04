/*
 * The loop overhead, and where it goes.
 *
 * Every iteration of a loop pays for the loop as well as the work: increment
 * the counter, compare it, branch back. On this core that is about three cycles
 * of bookkeeping, and if the body is one multiply accumulate then most of the
 * loop is not doing arithmetic.
 *
 * Unrolling by n divides that overhead by n. It also gives the compiler several
 * independent multiply accumulates to interleave, which matters because the
 * floating point unit stalls one cycle when an instruction needs the result of
 * the one before it, and four accumulators in flight give it something else to
 * do while it waits.
 *
 * The returns stop. Past a certain point the overhead is already small, the
 * register file runs out, and the extra code costs more to fetch than the
 * saved branches were worth. Where that point is depends on the core, so the
 * table below is the answer for this one and not a general rule.
 *
 * All four variants compile at the same optimisation level, so what is being
 * compared is the source and not the flags.
 *
 *     make opt_unroll && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "testsig.h"
#include "driver_clock.h"
#include "driver_systick.h"

#define REPEATS     4U

typedef float32_t (*dot_fn)(const float32_t *, const float32_t *, uint32_t);

__attribute__((optimize("O2"), noinline))
static float32_t dot_1(const float32_t *pA, const float32_t *pB, uint32_t n)
{
    float32_t sum = 0.0f;

    for (uint32_t i = 0U; i < n; i++)
    {
        sum += pA[i] * pB[i];
    }

    return sum;
}

__attribute__((optimize("O2"), noinline))
static float32_t dot_2(const float32_t *pA, const float32_t *pB, uint32_t n)
{
    float32_t s0 = 0.0f;
    float32_t s1 = 0.0f;
    uint32_t  i;

    for (i = 0U; (i + 2U) <= n; i += 2U)
    {
        s0 += pA[i]      * pB[i];
        s1 += pA[i + 1U] * pB[i + 1U];
    }

    for (; i < n; i++)
    {
        s0 += pA[i] * pB[i];
    }

    return s0 + s1;
}

__attribute__((optimize("O2"), noinline))
static float32_t dot_4(const float32_t *pA, const float32_t *pB, uint32_t n)
{
    float32_t s0 = 0.0f;
    float32_t s1 = 0.0f;
    float32_t s2 = 0.0f;
    float32_t s3 = 0.0f;
    uint32_t  i;

    for (i = 0U; (i + 4U) <= n; i += 4U)
    {
        s0 += pA[i]      * pB[i];
        s1 += pA[i + 1U] * pB[i + 1U];
        s2 += pA[i + 2U] * pB[i + 2U];
        s3 += pA[i + 3U] * pB[i + 3U];
    }

    for (; i < n; i++)
    {
        s0 += pA[i] * pB[i];
    }

    return (s0 + s1) + (s2 + s3);
}

__attribute__((optimize("O2"), noinline))
static float32_t dot_8(const float32_t *pA, const float32_t *pB, uint32_t n)
{
    float32_t s0 = 0.0f;
    float32_t s1 = 0.0f;
    float32_t s2 = 0.0f;
    float32_t s3 = 0.0f;
    uint32_t  i;

    for (i = 0U; (i + 8U) <= n; i += 8U)
    {
        s0 += pA[i]      * pB[i];
        s1 += pA[i + 1U] * pB[i + 1U];
        s2 += pA[i + 2U] * pB[i + 2U];
        s3 += pA[i + 3U] * pB[i + 3U];
        s0 += pA[i + 4U] * pB[i + 4U];
        s1 += pA[i + 5U] * pB[i + 5U];
        s2 += pA[i + 6U] * pB[i + 6U];
        s3 += pA[i + 7U] * pB[i + 7U];
    }

    for (; i < n; i++)
    {
        s0 += pA[i] * pB[i];
    }

    return (s0 + s1) + (s2 + s3);
}

static const dot_fn variant[] = { dot_1, dot_2, dot_4, dot_8 };
static const char *const name[] = { "none", "by 2", "by 4", "by 8" };

static float32_t sink;

static uint32_t measure(dot_fn fn)
{
    cycles_start();

    for (uint32_t r = 0U; r < REPEATS; r++)
    {
        for (uint32_t n = 0U; n <= (SIG_LEN - TAPS); n++)
        {
            sink += fn(&sig[n], taps, TAPS);
        }
    }

    uint32_t spent = cycles_read();

    systick_init(TICK_HZ);

    return spent;
}

int main(void)
{
    config_app();
    probe_reset();

    uint32_t macs = (SIG_LEN - TAPS + 1U) * TAPS * REPEATS;

    printf("\r\nall four at -O2, %lu taps, %lu multiply accumulates\r\n\r\n",
           (unsigned long)TAPS, (unsigned long)macs);

    printf("%-8s %12s %14s %12s\r\n", "unroll", "cycles", "per mac", "vs none");

    uint32_t base = 0U;

    for (uint32_t k = 0U; k < ARRAY_LEN(variant); k++)
    {
        uint32_t spent = measure(variant[k]);

        if (k == 0U)
        {
            base = spent;
        }

        printf("%-8s %12lu %14.2f %12.2f\r\n", name[k], (unsigned long)spent,
               (double)((float32_t)spent / (float32_t)macs),
               (double)((float32_t)base / (float32_t)spent));

        g_cycles  = (float32_t)spent;
        g_per_mac = (float32_t)spent / (float32_t)macs;
        g_speedup = (float32_t)base / (float32_t)spent;
    }

    printf("\r\nchecksum %.6f\r\n", (double)sink);

    printf("\r\ntwo things happen at once: fewer branches, and several"
           " accumulators in\r\nflight covering the fpu's one cycle stall.\r\n");

    printf("\r\ns0+s1+s2+s3 is not bit for bit the same as adding in order:"
           " float add\r\nis not associative, same effect the convolution"
           " chapter measured.\r\n");

    printf("\r\nif the table stops improving before by 8, that is the answer"
           " for this\r\ncore and this loop, not a mistake.\r\n");

    while (1)
    {
        for (uint32_t k = 0U; k < ARRAY_LEN(variant); k++)
        {
            uint32_t spent = measure(variant[k]);

            g_cycles  = (float32_t)spent;
            g_per_mac = (float32_t)spent / (float32_t)macs;
            g_speedup = (float32_t)base / (float32_t)spent;
            probe_step();
        }
    }
}