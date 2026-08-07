/*
 * Impulse decomposition breaks an N sample signal into N signals, each one
 * holding a single sample of the original and zeros everywhere else. Because
 * the system is linear, the outputs of those N pieces add back up to the
 * output of the whole signal, which is what superposition promises.
 *
 * The reason this one matters more than the arithmetic suggests: every piece
 * is the same impulse, only scaled and moved. So the system's response to one
 * impulse is enough to work out its response to anything, and the sum that
 * does it is the convolution.
 *
 *     make linear_impulse && make load && make monitor
 */
#include <stdio.h>
#include <math.h>
#include "config.h"
#include "arm_math.h"
#include "probe.h"
#include "systems.h"

#define SIG_LEN     25U
#define H_LEN       3U

static float32_t x[SIG_LEN];
static float32_t comp[SIG_LEN];
static float32_t comp_out[SIG_LEN];
static float32_t rebuilt[SIG_LEN];
static float32_t partial[SIG_LEN];
static float32_t y_direct[SIG_LEN];
static float32_t work[SIG_LEN];
static float32_t h[H_LEN];
static float32_t conv[SIG_LEN + H_LEN - 1U];

static float32_t max_gap(const float32_t *pA, const float32_t *pB, uint32_t len);

int main(void)
{
    config_app();
    
    while(1)
    {
        
    }
}