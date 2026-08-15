/*
 * What the transform costs, and why nobody computes it this way.
 *
 * dft_forward does N/2 + 1 bins of N products, so its cost grows with N
 * squared. The FFT reaches the same numbers in something closer to N log N by
 * noticing that the test waves it is correlating against share most of their
 * work. Doubling N roughly quadruples one and roughly doubles the other, and
 * the gap between them widens forever.
 *
 * N is 128 here and larger elsewhere in this module, for a reason worth
 * knowing: the cycle counter is 24 bits, so it can measure 1.048 s at 16 MHz
 * and no more. A direct transform of a few hundred points runs past that and
 * the counter wraps without saying so. The measurement below is checked against
 * that ceiling rather than trusted.
 *
 * The extrapolation at the end is the answer to the obvious next question. A
 * 640 point signal is not four times the work of a 160 point one.
 *
 *     make dft_timing && make load && make monitor
 */
#include <stdio.h>
#include "config.h"
#include "signals.h"
#include "driver_systick.h"
#include "driver_clock.h"
#include "probe.h"
#include "dft.h"

#define N           128U
#define BINS        DFT_BINS(N)
#define CEILING     ((uint32_t)(SYSTICK_MAX_RELOAD - (SYSTICK_MAX_RELOAD / 10U)))

static float32_t x[N];
static float32_t work[N];
static float32_t packed[N];
static float32_t re[BINS];
static float32_t im[BINS];

static arm_rfft_fast_instance_f32 fft;

int main(void)
{
    uint32_t overhead;
    uint32_t direct;
    uint32_t fast;
    float32_t ms_per_cycle;

    config_app();
    probe_reset();

    arm_copy_f32(input_signal_f32_1kHz_15kHz, x, N);
    (void)arm_rfft_fast_init_128_f32(&fft);

    ms_per_cycle = 1.0e3f / (float32_t)clock_get();

    cycles_start();
    overhead = cycles_read();

    cycles_start();
    dft_forward(x, N, re, im);
    direct = cycles_read() - overhead;

    arm_copy_f32(x, work, N);

    cycles_start();
    arm_rfft_fast_f32(&fft, work, packed, 0);
    fast = cycles_read() - overhead;

    /* the tick was stopped to free the timer, so put it back */
    systick_init(TICK_HZ);

    printf("\r\n%lu points at %lu Hz, counter costs %lu cycles\r\n",
           (unsigned long)N, (unsigned long)clock_get(), (unsigned long)overhead);

    if ((direct > CEILING) || (fast > CEILING))
    {
        printf("a measurement is near the 24 bit ceiling, treat it as a floor\r\n");
    }

    printf("\r\n%-16s %10s %9s %8s\r\n", "", "cycles", "ms", "slower");
    printf("%-16s %10lu %9.2f %7.0fx\r\n", "dft_forward", (unsigned long)direct,
           (double)((float32_t)direct * ms_per_cycle), (double)((float32_t)direct / (float32_t)fast));
    printf("%-16s %10lu %9.2f %7.0fx\r\n", "arm_rfft_fast_f32", (unsigned long)fast,
           (double)((float32_t)fast * ms_per_cycle), 1.0);

    /* Cost grows with the square, so scaling up is a matter of multiplying by
     * the ratio of the squares. This is the arithmetic behind a direct
     * transform of a few hundred points taking minutes. */
    printf("\r\nat this rate, dft_forward would need about\r\n");

    for (uint32_t len = 2U * N; len <= (16U * N); len *= 2U)
    {
        float32_t ratio = ((float32_t)len / (float32_t)N) * ((float32_t)len / (float32_t)N);

        printf("  %5lu points   %8.1f s\r\n", (unsigned long)len,
               (double)((float32_t)direct * ms_per_cycle * ratio / 1000.0f));
    }

    while (1)
    {
    }
}