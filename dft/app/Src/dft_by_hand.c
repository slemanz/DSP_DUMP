/*
 * The DFT on eight samples, with the arithmetic printed.
 *
 * The signal is built from three things on purpose: a constant, one cosine and
 * one sine. The transform has to find those three and report zero for
 * everything else, and watching how it reports zero is the point.
 *
 * Every bin is one question. Multiply the signal by a test wave, add up the
 * eight products, and see what is left. When the test wave matches something in
 * the signal the products keep the same sign and pile up. When it does not,
 * they come in pairs that cancel and the total collapses to zero. Nothing else
 * happens in a DFT, at any size.
 *
 *     make dft_by_hand && make load && make monitor
 */
#include <stdio.h>
#include <math.h>
#include "config.h"
#include "probe.h"
#include "dft.h"

#define N           8U
#define BINS        DFT_BINS(N)

#define DC          0.5f
#define COS_AMP     1.0f    // a cosine at 1 cycle across the window
#define COS_BIN     1U
#define SIN_AMP     0.5f    // a sine at 2 cycles across the window
#define SIN_BIN     2U

static float32_t x[N];
static float32_t re[BINS];
static float32_t im[BINS];

// prints the eight products that make one bin, so the sum can be watched
// arriving instead of just appearing
static void show_products(uint32_t k)
{
    float32_t total = 0.0f;

    printf("  k=%lu  ", (unsigned long)k);

    for(uint32_t i = 0; i < N; i++)
    {
        float32_t p = x[i] * cosf(TWO_PI * (float32_t)k * (float32_t)i / (float32_t)N);

        total += p;
        printf("%8.3f", p);
    }
    printf("%9.3f\r\n", total);
}

int main(void)
{
    config_app();
    probe_reset();

    for (uint32_t n = 0; n < N; n++)
    {
        x[n] = DC
             + (COS_AMP * cosf(TWO_PI * (float32_t)COS_BIN * (float32_t)n / (float32_t)N))
             + (SIN_AMP * sinf(TWO_PI * (float32_t)SIN_BIN * (float32_t)n / (float32_t)N));
    }

    dft_forward(x, N, re, im);

    printf("\r\nx[n] = %.1f + %.1f*cos(2pi*%lu*n/%lu) + %.1f*sin(2pi*%lu*n/%lu)\r\n\r\n",
           DC, COS_AMP, (unsigned long)COS_BIN, (unsigned long)N,
           SIN_AMP, (unsigned long)SIN_BIN, (unsigned long)N);

    printf("      n ");

    for (uint32_t n = 0; n < N; n++)
    {
        printf("%8lu", (unsigned long)n);
    }

    printf("\r\n   x[n] ");

    for (uint32_t n = 0; n < N; n++)
    {
        printf("%8.3f", x[n]);
    }

    printf("\r\n\r\nthe eight products behind two of the bins, and their totals\r\n\r\n");
    show_products(COS_BIN);
    show_products(COS_BIN + 2U);

    printf("\r\n%5s %8s %8s   %s\r\n", "k", "ReX", "ImX", "what the bin found");

    for (uint32_t k = 0; k < BINS; k++)
    {
        printf("%5lu %8.3f %8.3f", (unsigned long)k, re[k], im[k]);

        if (k == 0U)
        {
            printf("   the constant, times %lu\r\n", (unsigned long)N);
        }
        else if (k == COS_BIN)
        {
            printf("   the cosine, times %lu/2\r\n", (unsigned long)N);
        }
        else if (k == SIN_BIN)
        {
            printf("   the sine, times %lu/2\r\n", (unsigned long)N);
        }
        else
        {
            printf("   nothing\r\n");
        }
    }

    /* Bin 0 multiplies the signal by a wave that is 1 everywhere, so all N
     * samples add up. Every other bin multiplies by a wave that spends half its
     * time negative, so only N/2 worth survives. That is the whole reason the
     * inverse transform divides bin 0 by N and the rest by N/2. Bin N/2 is the
     * other special one, an alternating wave that behaves like bin 0. */
    printf("\r\n%.1f * %lu = %.1f, and %.1f * %lu/2 = %.1f, and %.1f * %lu/2 = %.1f\r\n",
           DC, (unsigned long)N, re[0],
           COS_AMP, (unsigned long)N, re[COS_BIN],
           SIN_AMP, (unsigned long)N, im[SIN_BIN]);
    printf("so undoing a bin means dividing by %lu at k=0 and k=%lu, by %lu/2 elsewhere\r\n",
           (unsigned long)N, (unsigned long)(N / 2U), (unsigned long)N);


    while(1)
    {
    }
}