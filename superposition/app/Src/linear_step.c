/*
 * Step decomposition breaks the same N sample signal into N signals, but the
 * pieces are steps rather than impulses: component k is zero up to k and then
 * holds a constant from k onward. The constant is the difference between two
 * neighbouring samples, which is what makes the pieces add back up to the
 * original, and what makes this decomposition a statement about how much the
 * signal changes rather than about where it sits.
 *
 * Its counterpart to the impulse response is the step response, and the two
 * carry the same information: one is the running sum of the other.
 *
 *     make linear_step && make load && make monitor
 */
#include <stdio.h>
#include <math.h>
#include "config.h"
#include "arm_math.h"
#include "probe.h"
#include "systems.h"

#define SIG_LEN     25U

static float32_t x[SIG_LEN];
static float32_t delta[SIG_LEN];
static float32_t comp[SIG_LEN];
static float32_t comp_out[SIG_LEN];
static float32_t rebuilt[SIG_LEN];
static float32_t partial[SIG_LEN];
static float32_t y_direct[SIG_LEN];
static float32_t step_res[SIG_LEN];
static float32_t work[SIG_LEN];

static void build_component(uint32_t k);
static float32_t max_gap(const float32_t *pA, const float32_t *pB, uint32_t len);

int main(void)
{
    config_app();

    while(1)
    {
        
    }
}