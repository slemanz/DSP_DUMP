/*
 * Two multiplies in one instruction.
 *
 * This is the rung the previous chapter was pointing at. A q15 is sixteen bits,
 * two of them fit in one register, and the instruction set has multiply
 * accumulates that take both halves at once:
 *
 *     SMLAD  Rd = Rn.bottom * Rm.bottom + Rn.top * Rm.top + Ra
 *
 * One instruction, one cycle, two multiplies and three additions. Nothing the
 * float unit has comes close, and it only exists because the data is sixteen
 * bits wide, which is the entire argument for fixed point on this core.
 *
 * The alignment condition is real and easy to miss. Reading two q15 values as
 * one 32 bit word requires the address to be a multiple of four, so the loop
 * has to start on an even sample index and the arrays have to be aligned. Get
 * it wrong and it still works on this core, slowly, because unaligned access is
 * supported and costs extra cycles.
 *
 * The accumulator is a q63 for the reason the previous chapter measured: the
 * running total of a filter overflows long before any individual product does.
 *
 *     make opt_simd && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "testsig.h"
#include "driver_clock.h"
#include "driver_systick.h"

#define REPEATS     8U

static q15_t q_sig[SIG_LEN]  __attribute__((aligned(4)));
static q15_t q_taps[TAPS]    __attribute__((aligned(4)));

/* one multiply per iteration */
__attribute__((optimize("O2"), noinline))
static q63_t dot_plain(const q15_t *pA, const q15_t *pB, uint32_t n)
{
    q63_t sum = 0;

    for (uint32_t i = 0U; i < n; i++)
    {
        sum += (q63_t)pA[i] * (q63_t)pB[i];
    }

    return sum;
}

/* two multiplies per instruction, so half the iterations */
__attribute__((optimize("O2"), noinline))
static q63_t dot_simd(const q15_t *pA, const q15_t *pB, uint32_t n)
{
    const uint32_t *pA32 = (const uint32_t *)(const void *)pA;
    const uint32_t *pB32 = (const uint32_t *)(const void *)pB;
    q63_t    sum = 0;
    uint32_t i;

    for (i = 0U; (i + 2U) <= n; i += 2U)
    {
        sum = __SMLALD(pA32[i / 2U], pB32[i / 2U], sum);
    }

    for (; i < n; i++)
    {
        sum += (q63_t)pA[i] * (q63_t)pB[i];
    }

    return sum;
}

/* four at a time, which is two instructions with nothing between them */
__attribute__((optimize("O2"), noinline))
static q63_t dot_simd4(const q15_t *pA, const q15_t *pB, uint32_t n)
{
    const uint32_t *pA32 = (const uint32_t *)(const void *)pA;
    const uint32_t *pB32 = (const uint32_t *)(const void *)pB;
    q63_t    s0 = 0;
    q63_t    s1 = 0;
    uint32_t i;

    for (i = 0U; (i + 4U) <= n; i += 4U)
    {
        s0 = __SMLALD(pA32[i / 2U],        pB32[i / 2U],        s0);
        s1 = __SMLALD(pA32[(i / 2U) + 1U], pB32[(i / 2U) + 1U], s1);
    }

    for (; i < n; i++)
    {
        s0 += (q63_t)pA[i] * (q63_t)pB[i];
    }

    return s0 + s1;
}

typedef q63_t (*dot_fn)(const q15_t *, const q15_t *, uint32_t);

static const dot_fn variant[] = { dot_plain, dot_simd, dot_simd4 };
static const char *const name[] = { "one at a time", "SMLALD", "SMLALD x2" };

static q63_t sink;

static uint32_t measure(dot_fn fn, q63_t *pResult)
{
    q63_t total = 0;

    cycles_start();

    for (uint32_t r = 0U; r < REPEATS; r++)
    {
        for (uint32_t n = 0U; n <= (SIG_LEN - TAPS); n += 2U)
        {
            total += fn(&q_sig[n], q_taps, TAPS);
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

    arm_float_to_q15(sig, q_sig, SIG_LEN);
    arm_float_to_q15(taps, q_taps, TAPS);

    uint32_t passes = ((SIG_LEN - TAPS) / 2U) + 1U;
    uint32_t macs   = passes * TAPS * REPEATS;

    printf("\r\nq15 data, %lu multiply accumulates\r\n", (unsigned long)macs);
    printf("arrays are 4 byte aligned so two samples can be read as one"
           " word\r\n\r\n");

    printf("%-16s %12s %14s %12s %s\r\n",
           "version", "cycles", "per mac", "vs plain", "same answer");

    uint32_t base = 0U;
    q63_t    truth = 0;

    for (uint32_t k = 0U; k < ARRAY_LEN(variant); k++)
    {
        q63_t answer;
        uint32_t spent = measure(variant[k], &answer);

        if (k == 0U)
        {
            base = spent;
            truth = answer;
        }

        printf("%-16s %12lu %14.2f %12.2f %s\r\n", name[k],
               (unsigned long)spent,
               (double)((float32_t)spent / (float32_t)macs),
               (double)((float32_t)base / (float32_t)spent),
               (answer == truth) ? "yes" : "NO");

        g_cycles  = (float32_t)spent;
        g_per_mac = (float32_t)spent / (float32_t)macs;
        g_speedup = (float32_t)base / (float32_t)spent;
    }

    printf("\r\nthe last column matters most: for integers, order does not"
           " change the\r\nsum, so all three give identical answers, which"
           " floats would not.\r\n");

    printf("\r\nthe compiler will not find this on its own: reading two q15 as"
           " one word\r\nis an alignment promise C does not let it assume.\r\n");

    printf("\r\nchecksum %ld\r\n", (long)(sink & 0xFFFFFFFF));

    while (1)
    {
        for (uint32_t k = 0U; k < ARRAY_LEN(variant); k++)
        {
            q63_t answer;
            uint32_t spent = measure(variant[k], &answer);

            g_cycles  = (float32_t)spent;
            g_per_mac = (float32_t)spent / (float32_t)macs;
            g_speedup = (float32_t)base / (float32_t)spent;
            probe_step();
        }
    }
}