/*
 * One instruction or two, and why the lesson says to prefer two.
 *
 * A multiply accumulate computes a*b + c. The floating point unit has a single
 * instruction for it, vfma.f32, and the obvious assumption is that one
 * instruction beats two.
 *
 * The lesson says the opposite, and the reason it gives is real: the addition
 * needs the result of the multiplication, so the fused instruction has a stall
 * built into it and takes three cycles rather than the one it looks like. Split
 * it into a separate multiply and add, interleave two of them, and each pair
 * can be in flight while the other stalls, which brings the average down.
 *
 * So it is not a claim about one instruction against two. It is a claim about
 * whether there is anything else to do during the stall, and it stops being
 * true the moment there is only one chain of work.
 *
 * There is also an arithmetic difference that has nothing to do with speed. The
 * fused version keeps the full precision of the product before adding and
 * rounds once; the split version rounds twice. The fused answer is the more
 * accurate one, so this rung can cost a bit of precision as well as buy cycles.
 *
 *     make opt_fma && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "testsig.h"
#include "driver_clock.h"
#include "driver_systick.h"

#define REPEATS     4U

typedef float32_t (*dot_fn)(const float32_t *, const float32_t *, uint32_t);

/* one chain, fused, nothing to interleave */
__attribute__((optimize("O2"), noinline))
static float32_t dot_fused(const float32_t *pA, const float32_t *pB,
                           uint32_t n)
{
    float32_t sum = 0.0f;

    for (uint32_t i = 0U; i < n; i++)
    {
        sum = __builtin_fmaf(pA[i], pB[i], sum);
    }

    return sum;
}

/* one chain, split, still nothing to interleave */
__attribute__((optimize("O2"), noinline))
static float32_t dot_split(const float32_t *pA, const float32_t *pB,
                           uint32_t n)
{
    float32_t sum = 0.0f;

    for (uint32_t i = 0U; i < n; i++)
    {
        float32_t product = pA[i] * pB[i];

        sum += product;
    }

    return sum;
}

/* two chains, split, each covering the other's stall */
__attribute__((optimize("O2"), noinline))
static float32_t dot_split2(const float32_t *pA, const float32_t *pB,
                            uint32_t n)
{
    float32_t s0 = 0.0f;
    float32_t s1 = 0.0f;
    uint32_t  i;

    for (i = 0U; (i + 2U) <= n; i += 2U)
    {
        float32_t p0 = pA[i]      * pB[i];
        float32_t p1 = pA[i + 1U] * pB[i + 1U];

        s0 += p0;
        s1 += p1;
    }

    for (; i < n; i++)
    {
        s0 += pA[i] * pB[i];
    }

    return s0 + s1;
}

/* two chains, fused */
__attribute__((optimize("O2"), noinline))
static float32_t dot_fused2(const float32_t *pA, const float32_t *pB,
                            uint32_t n)
{
    float32_t s0 = 0.0f;
    float32_t s1 = 0.0f;
    uint32_t  i;

    for (i = 0U; (i + 2U) <= n; i += 2U)
    {
        s0 = __builtin_fmaf(pA[i],      pB[i],      s0);
        s1 = __builtin_fmaf(pA[i + 1U], pB[i + 1U], s1);
    }

    for (; i < n; i++)
    {
        s0 = __builtin_fmaf(pA[i], pB[i], s0);
    }

    return s0 + s1;
}

static const dot_fn variant[] = { dot_fused, dot_split, dot_fused2, dot_split2 };
static const char *const name[] = { "fused x1", "split x1",
                                    "fused x2", "split x2" };

static float32_t sink;

static uint32_t measure(dot_fn fn, float32_t *pResult)
{
    float32_t total = 0.0f;

    cycles_start();

    for (uint32_t r = 0U; r < REPEATS; r++)
    {
        for (uint32_t n = 0U; n <= (SIG_LEN - TAPS); n++)
        {
            total += fn(&sig[n], taps, TAPS);
        }
    }

    uint32_t spent = cycles_read();

    systick_init(TICK_HZ);
    sink += total;
    *pResult = total;

    return spent;
}

int main(void)
{
    config_app();
    probe_reset();

    uint32_t macs = (SIG_LEN - TAPS + 1U) * TAPS * REPEATS;

    printf("\r\nall four at -O2, %lu multiply accumulates\r\n\r\n",
           (unsigned long)macs);

    printf("%-10s %12s %14s %12s %14s\r\n",
           "version", "cycles", "per mac", "vs first", "sum");

    uint32_t  base = 0U;
    float32_t fused_answer = 0.0f;

    for (uint32_t k = 0U; k < ARRAY_LEN(variant); k++)
    {
        float32_t answer;
        uint32_t spent = measure(variant[k], &answer);

        if (k == 0U)
        {
            base = spent;
            fused_answer = answer;
        }

        printf("%-10s %12lu %14.2f %12.2f %14.6f\r\n", name[k],
               (unsigned long)spent,
               (double)((float32_t)spent / (float32_t)macs),
               (double)((float32_t)base / (float32_t)spent), (double)answer);

        g_cycles  = (float32_t)spent;
        g_per_mac = (float32_t)spent / (float32_t)macs;
        g_speedup = (float32_t)base / (float32_t)spent;
    }

    printf("\r\nread the rows in pairs: fused against split at one chain, then"
           " the same\r\nquestion at two chains once there is something to fill"
           " the stall.\r\n");

    printf("\r\nthe sum column is not all the same: fused rounds once, split"
           " rounds\r\ntwice, so fused is the more accurate answer.\r\n");

    printf("\r\nthis rung is the most conditional one: if fused wins here, the"
           " stall\r\nwas already covered and the advice does not apply.\r\n");

    while (1)
    {
        for (uint32_t k = 0U; k < ARRAY_LEN(variant); k++)
        {
            float32_t answer;
            uint32_t spent = measure(variant[k], &answer);

            g_cycles  = (float32_t)spent;
            g_per_mac = (float32_t)spent / (float32_t)macs;
            g_speedup = (float32_t)base / (float32_t)spent;
            g_bytes   = answer - fused_answer;
            probe_step();
        }
    }
}