/*
 * The optimisation you get by promising something.
 *
 * A function that takes two pointers and writes through one of them has a
 * problem the source does not show. The compiler cannot know the two pointers
 * do not overlap, so after every store through the output it has to assume the
 * input may have changed, and reload it. A whole loop of work is thrown away
 * and redone for a case that never happens.
 *
 * restrict is the promise that they do not overlap. It generates no code. It
 * removes a reload the compiler was inserting because it had no choice, and on
 * a loop that stores every iteration that reload was most of the loop.
 *
 * const is the smaller cousin and does less than people expect: it stops the
 * function writing through the pointer, but it does not promise nobody else
 * will, so it does not on its own let the compiler cache anything across a
 * store.
 *
 * The lesson's advice to group loads and stores together is the same idea seen
 * from the instruction side. A load or store on this core takes two cycles
 * alone and one when it follows another, so consecutive accesses cost n+1
 * rather than 2n. Unrolling groups them for free, which is why the previous app
 * and this one are really one topic.
 *
 *     make opt_memory && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "testsig.h"
#include "driver_clock.h"
#include "driver_systick.h"

#define REPEATS     8U
#define OUT_LEN     (SIG_LEN - TAPS + 1U)

typedef void (*filt_fn)(const float32_t *, const float32_t *, float32_t *,
                        uint32_t, uint32_t);

/* the compiler has to assume pDst may point into pSrc */
__attribute__((optimize("O2"), noinline))
static void filt_plain(const float32_t *pSrc, const float32_t *pH,
                       float32_t *pDst, uint32_t len, uint32_t taps_n)
{
    for (uint32_t n = 0U; n < len; n++)
    {
        float32_t sum = 0.0f;

        for (uint32_t i = 0U; i < taps_n; i++)
        {
            sum += pSrc[n + i] * pH[i];
        }

        pDst[n] = sum;
    }
}

/* and here it is told that it does not */
__attribute__((optimize("O2"), noinline))
static void filt_restrict(const float32_t *restrict pSrc,
                          const float32_t *restrict pH,
                          float32_t *restrict pDst,
                          uint32_t len, uint32_t taps_n)
{
    for (uint32_t n = 0U; n < len; n++)
    {
        float32_t sum = 0.0f;

        for (uint32_t i = 0U; i < taps_n; i++)
        {
            sum += pSrc[n + i] * pH[i];
        }

        pDst[n] = sum;
    }
}

/*
 * The same promise made a different way: hoist the accumulator into a local so
 * there is no store inside the inner loop at all. This is what restrict lets
 * the compiler do by itself, written out by hand so it can be seen.
 */
__attribute__((optimize("O2"), noinline))
static void filt_hoisted(const float32_t *pSrc, const float32_t *pH,
                         float32_t *pDst, uint32_t len, uint32_t taps_n)
{
    for (uint32_t n = 0U; n < len; n++)
    {
        const float32_t *pA = &pSrc[n];
        float32_t sum = 0.0f;

        for (uint32_t i = 0U; i < taps_n; i++)
        {
            sum += pA[i] * pH[i];
        }

        pDst[n] = sum;
    }
}

static float32_t out[OUT_LEN];

static const filt_fn variant[] = { filt_plain, filt_restrict, filt_hoisted };
static const char *const name[] = { "plain", "restrict", "hoisted" };

static uint32_t measure(filt_fn fn)
{
    cycles_start();

    for (uint32_t r = 0U; r < REPEATS; r++)
    {
        fn(sig, taps, out, OUT_LEN, TAPS);
    }

    uint32_t spent = cycles_read();

    systick_init(TICK_HZ);

    return spent;
}

int main(void)
{
    config_app();
    probe_reset();

    uint32_t macs = OUT_LEN * TAPS * REPEATS;

    printf("\r\nall three at -O2, %lu multiply accumulates\r\n\r\n",
           (unsigned long)macs);

    printf("%-10s %12s %14s %12s %12s\r\n",
           "version", "cycles", "per mac", "vs plain", "checksum");

    uint32_t base = 0U;

    for (uint32_t k = 0U; k < ARRAY_LEN(variant); k++)
    {
        uint32_t spent = measure(variant[k]);
        float32_t sum;

        arm_mean_f32(out, OUT_LEN, &sum);

        if (k == 0U)
        {
            base = spent;
        }

        printf("%-10s %12lu %14.2f %12.2f %12.6f\r\n", name[k],
               (unsigned long)spent,
               (double)((float32_t)spent / (float32_t)macs),
               (double)((float32_t)base / (float32_t)spent), (double)sum);

        g_cycles  = (float32_t)spent;
        g_per_mac = (float32_t)spent / (float32_t)macs;
        g_speedup = (float32_t)base / (float32_t)spent;
    }

    printf("\r\nthe checksum column is not decoration: if the three did not"
           " compute the\r\nsame thing, the speed would not matter.\r\n");

    printf("\r\nif restrict bought nothing here, that is real: this loop has"
           " no store\r\nin it, so there was no reload to remove.\r\n");

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