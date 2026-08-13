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

}

int main(void)
{
    config_app();
    probe_reset();

    while(1)
    {
    }
}