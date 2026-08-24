#include "bench.h"
#include "driver_systick.h"

static float32_t input[BENCH_LEN];
static float32_t taps[BENCH_TAPS];
static float32_t output[BENCH_LEN + BENCH_TAPS - 1U];

void bench_prepare(void)
{
    uint32_t i;

    for (i = 0U; i < BENCH_LEN; i++)
    {
        input[i] = arm_sin_f32(0.05f * (float32_t)i);
    }

    for (i = 0U; i < BENCH_TAPS; i++)
    {
        taps[i] = 1.0f / (float32_t)BENCH_TAPS;
    }
}

/*
 * Runs the workload once and returns what it cost in core cycles.
 *
 * The count does not depend on the clock frequency. A cycle is a cycle whether
 * it lasts 62.5 ns or 10 ns, so this number stays put when the PLL comes on and
 * only the wall clock time changes. What does move it is anything that makes
 * the core wait: flash wait states, and the caches that hide them.
 *
 * cycles_start takes SysTick over, so the 1 ms tick is rebuilt on the way out.
 */
uint32_t bench_cycles(void)
{
    uint32_t i;
    uint32_t n;
    uint32_t spent;

    for (n = 0U; n < (BENCH_LEN + BENCH_TAPS - 1U); n++)
    {
        output[n] = 0.0f;
    }

    cycles_start();

    for (i = 0U; i < BENCH_LEN; i++)
    {
        for (n = 0U; n < BENCH_TAPS; n++)
        {
            output[i + n] += input[i] * taps[n];
        }
    }

    spent = cycles_read();

    systick_init(TICK_HZ);

    return spent;
}

/* keeps the optimiser honest: the result has to be looked at by someone */
float32_t bench_checksum(void)
{
    float32_t sum = 0.0f;
    uint32_t  n;

    for (n = 0U; n < (BENCH_LEN + BENCH_TAPS - 1U); n++)
    {
        sum += output[n];
    }

    return sum;
}