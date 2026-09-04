/*
 * The first rung, and the one that is nearly free.
 *
 * The same loop appears five times below, character for character, and the only
 * difference is an attribute telling the compiler how hard to try. GCC lets a
 * function carry its own optimisation level, which is what makes this a table
 * in one binary rather than five builds to compare from memory.
 *
 * Debug builds run at -O0, and -O0 is not "C without optimisation". It is a
 * specific and deliberately naive code generator: every variable goes back to
 * the stack after every statement so a debugger can always find it. The loop
 * counter is loaded and stored on every iteration. Nothing is kept in a
 * register longer than one line of source.
 *
 * That is a fine thing to debug and a terrible thing to measure, and measuring
 * a DSP routine in a debug build is the single most common way to arrive at a
 * wrong conclusion about it.
 *
 *     make opt_levels && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "testsig.h"
#include "driver_clock.h"
#include "driver_systick.h"

#define REPEATS     4U

typedef float32_t (*dot_fn)(const float32_t *, const float32_t *, uint32_t);

__attribute__((optimize("O0"), noinline))
static float32_t dot_o0(const float32_t *pA, const float32_t *pB, uint32_t n)
{
    float32_t sum = 0.0f;

    for (uint32_t i = 0U; i < n; i++)
    {
        sum += pA[i] * pB[i];
    }

    return sum;
}

__attribute__((optimize("O1"), noinline))
static float32_t dot_o1(const float32_t *pA, const float32_t *pB, uint32_t n)
{
    float32_t sum = 0.0f;

    for (uint32_t i = 0U; i < n; i++)
    {
        sum += pA[i] * pB[i];
    }

    return sum;
}

__attribute__((optimize("O2"), noinline))
static float32_t dot_o2(const float32_t *pA, const float32_t *pB, uint32_t n)
{
    float32_t sum = 0.0f;

    for (uint32_t i = 0U; i < n; i++)
    {
        sum += pA[i] * pB[i];
    }

    return sum;
}

__attribute__((optimize("O3"), noinline))
static float32_t dot_o3(const float32_t *pA, const float32_t *pB, uint32_t n)
{
    float32_t sum = 0.0f;

    for (uint32_t i = 0U; i < n; i++)
    {
        sum += pA[i] * pB[i];
    }

    return sum;
}

__attribute__((optimize("Os"), noinline))
static float32_t dot_os(const float32_t *pA, const float32_t *pB, uint32_t n)
{
    float32_t sum = 0.0f;

    for (uint32_t i = 0U; i < n; i++)
    {
        sum += pA[i] * pB[i];
    }

    return sum;
}

static const dot_fn variant[] = { dot_o0, dot_o1, dot_o2, dot_o3, dot_os };
static const char *const name[] = { "-O0", "-O1", "-O2", "-O3", "-Os" };

static float32_t sink;

/* runs one variant over the whole signal and returns what it cost in cycles */
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

    printf("\r\ncore at %lu Hz, %lu multiply accumulates per measurement\r\n",
           (unsigned long)clock_hclk(), (unsigned long)macs);
    printf("the same loop five times, differing only in one attribute\r\n\r\n");

    printf("%-6s %12s %14s %12s\r\n", "level", "cycles", "per mac", "vs -O0");

    uint32_t base = 0U;

    for (uint32_t k = 0U; k < ARRAY_LEN(variant); k++)
    {
        uint32_t spent = measure(variant[k]);

        if (k == 0U)
        {
            base = spent;
        }

        printf("%-6s %12lu %14.2f %12.2f\r\n", name[k], (unsigned long)spent,
               (double)((float32_t)spent / (float32_t)macs),
               (double)((float32_t)base / (float32_t)spent));

        g_cycles  = (float32_t)spent;
        g_per_mac = (float32_t)spent / (float32_t)macs;
        g_speedup = (float32_t)base / (float32_t)spent;
    }

    printf("\r\nchecksum %.6f\r\n", (double)sink);

    printf("\r\nthis is the cheapest rung, and the one people skip.\r\n");

    printf("\r\n-O3 is not always the winner: it unrolls and inlines harder"
           " than -O2,\r\nand bigger code can lose more to fetching than it"
           " gains executing.\r\n");

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