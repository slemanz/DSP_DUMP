/*
 * The rung that is not on the ladder.
 *
 * Everything in this chapter so far rearranged the same arithmetic. Nothing
 * changed how many multiply accumulates there were, only how fast each one
 * went, and the whole ladder is worth whatever the last five apps measure.
 *
 * This app changes the count.
 *
 * A direct DFT is N squared multiply accumulates. An FFT is closer to N log N.
 * At 256 points that is 65536 against 2048, a factor of 32 before a single
 * instruction has been chosen, and it grows with N while everything else in
 * this chapter stays where it is.
 *
 * The DFT chapter already measured the two side by side. This one exists to put
 * the number next to the ladder, because the order those two get tried in is
 * the most consequential decision in the chapter and it is not close.
 *
 *     make opt_algorithm && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "testsig.h"
#include "driver_clock.h"
#include "driver_systick.h"

#define N           128U
#define BINS        ((N / 2U) + 1U)
#define TWO_PI      6.28318531f

static float32_t input[N];
static float32_t re[BINS];
static float32_t im[BINS];
static float32_t fft_buf[N];
static arm_rfft_fast_instance_f32 fft;

/* the transform written out, which is where the N squared comes from */
__attribute__((optimize("O2"), noinline))
static void dft_direct(const float32_t *pSrc, uint32_t len)
{
    for (uint32_t k = 0U; k < ((len / 2U) + 1U); k++)
    {
        float32_t sum_re = 0.0f;
        float32_t sum_im = 0.0f;

        for (uint32_t i = 0U; i < len; i++)
        {
            float32_t angle = TWO_PI * (float32_t)k * (float32_t)i
                              / (float32_t)len;

            sum_re += pSrc[i] * arm_cos_f32(angle);
            sum_im += pSrc[i] * arm_sin_f32(angle);
        }

        re[k] = sum_re;
        im[k] = sum_im;
    }
}

int main(void)
{
    config_app();
    probe_reset();

    for (uint32_t i = 0U; i < N; i++)
    {
        input[i] = sig[i];
    }

    arm_rfft_fast_init_128_f32(&fft);

    cycles_start();
    dft_direct(input, N);
    uint32_t direct = cycles_read();
    systick_init(TICK_HZ);

    for (uint32_t i = 0U; i < N; i++)
    {
        input[i] = sig[i];
    }

    cycles_start();
    arm_rfft_fast_f32(&fft, input, fft_buf, 0U);
    uint32_t fast = cycles_read();
    systick_init(TICK_HZ);

    printf("\r\n%lu point transform at %lu Hz\r\n\r\n", (unsigned long)N,
           (unsigned long)clock_hclk());

    printf("%-22s %14s %14s %12s\r\n", "", "operations", "cycles", "us");
    printf("%-22s %14lu %14lu %12lu\r\n", "direct, N squared",
           (unsigned long)(N * N), (unsigned long)direct,
           (unsigned long)((uint64_t)direct * 1000000U / clock_hclk()));
    printf("%-22s %14lu %14lu %12lu\r\n", "fft, N log N",
           (unsigned long)(N * 7U), (unsigned long)fast,
           (unsigned long)((uint64_t)fast * 1000000U / clock_hclk()));
    printf("%-22s %14.1f %14.1f\r\n", "ratio",
           (double)((float32_t)(N * N) / (float32_t)(N * 7U)),
           (double)((float32_t)direct / (float32_t)fast));

    printf("\r\nevery flag and instruction in this chapter moved the same"
           " arithmetic by\r\nsome factor. this moved how much arithmetic"
           " there is.\r\n");

    printf("\r\nthe gap grows with N: doubling it quadruples the direct row"
           " but only\r\ndoubles the fft row, while nothing else here changes"
           " with size.\r\n");

    printf("\r\n%8s %14s %14s %12s\r\n", "N", "N squared", "N log2 N", "ratio");

    for (uint32_t n = 64U; n <= 4096U; n *= 2U)
    {
        uint32_t bits = 0U;

        for (uint32_t v = n; v > 1U; v >>= 1U)
        {
            bits++;
        }

        printf("%8lu %14lu %14lu %12lu\r\n", (unsigned long)n,
               (unsigned long)(n * n), (unsigned long)(n * bits),
               (unsigned long)(n / bits));
    }

    printf("\r\nso the order to try things in is not a matter of taste. count"
           " the\r\noperations first, and only then start making each one"
           " cheaper.\r\n");

    g_cycles  = (float32_t)direct;
    g_speedup = (float32_t)direct / (float32_t)fast;

    while (1)
    {
        for (uint32_t k = 0U; k < BINS; k++)
        {
            g_per_mac = re[k];
            g_bytes   = im[k];
            probe_step();
        }
    }
}