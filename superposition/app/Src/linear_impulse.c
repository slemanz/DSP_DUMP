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

    for (uint32_t n = 0; n < SIG_LEN; n++)
    {
        x[n] = 0.8f * sinf(TWO_PI * (float32_t)n / 16.0f) * (1.0f - (float32_t)n / 40.0f);
    }

    // the impulse response: one unit impulse in, and whatever comes out
    arm_fill_f32(0.0f, comp, SIG_LEN);
    comp[0] = 1.0f;
    system_fir(comp, work, SIG_LEN);
    arm_copy_f32(work, h, H_LEN);

    printf("\r\nimpulse decomposition of a %lu sample signal\r\n", (unsigned long)SIG_LEN);
    printf("\r\nimpulse response h[n]:");

    for (uint32_t n = 0; n < 5U; n++)
    {
        printf(" %.2f", work[n]);
    }

    printf("\r\n");

    /*
     * Take the signal apart and put it straight back together, with nothing
     * in between. This only checks that the decomposition is complete.
     */
    arm_fill_f32(0.0f, rebuilt, SIG_LEN);

    for (uint32_t k = 0; k < SIG_LEN; k++)
    {
        arm_fill_f32(0.0f, comp, SIG_LEN);
        comp[k] = x[k];
        arm_add_f32(rebuilt, comp, rebuilt, SIG_LEN);
    }

    printf("\r\nsynthesis without the system, max gap: %.9f\r\n", max_gap(rebuilt, x, SIG_LEN));

    /*
     * Now the same decomposition with the system in the middle: each impulse
     * goes through on its own, and the outputs are synthesized afterwards.
     */
    system_fir(x, y_direct, SIG_LEN);
    arm_fill_f32(0.0f, rebuilt, SIG_LEN);

    for (uint32_t k = 0; k < SIG_LEN; k++)
    {
        arm_fill_f32(0.0f, comp, SIG_LEN);
        comp[k] = x[k];
        system_fir(comp, comp_out, SIG_LEN);
        arm_add_f32(rebuilt, comp_out, rebuilt, SIG_LEN);
    }

    printf("%lu impulses through the system, max gap against one pass: %.9f\r\n", (unsigned long)SIG_LEN, max_gap(rebuilt, y_direct, SIG_LEN));

    /*
     * That sum has a name. Since every component is h scaled by x[k] and
     * delayed by k, adding them is the convolution of x with h, and CMSIS
     * will do the whole thing in one call.
     */
    arm_conv_f32(x, SIG_LEN, h, H_LEN, conv);

    printf("same result from arm_conv_f32, max gap: %.9f\r\n", max_gap(conv, y_direct, SIG_LEN));
    printf("\r\nstreaming: one impulse added per pass, %lu passes to rebuild\r\n", (unsigned long)SIG_LEN);

    uint32_t n = 0;
    uint32_t added = SIG_LEN;
    
    while(1)
    {
        uint32_t i = n % SIG_LEN;
        
        if (i == 0U)
        {
            if (added >= SIG_LEN)
            {
                arm_fill_f32(0.0f, partial, SIG_LEN);
                added = 0U;
            }

            arm_fill_f32(0.0f, comp, SIG_LEN);
            comp[added] = x[added];
            system_fir(comp, comp_out, SIG_LEN);
            arm_add_f32(partial, comp_out, partial, SIG_LEN);
            added++;
        }

        g_input  = x[i];
        g_path_a = y_direct[i];
        g_path_b = partial[i];
        g_error  = partial[i] - y_direct[i];

        n++;
        probe_step();
    }
}


static float32_t max_gap(const float32_t *pA, const float32_t *pB, uint32_t len)
{
    float32_t max;
    uint32_t index;

    arm_sub_f32(pA, pB, work, len);
    arm_absmax_f32(work, len, &max, &index);

    return max;
}