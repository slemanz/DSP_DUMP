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

    for (uint32_t n = 0; n < SIG_LEN; n++)
    {
        x[n] = 0.8f * sinf(TWO_PI * (float32_t)n / 16.0f) * (1.0f - (float32_t)n / 40.0f);
    }

    // the height of each step is how far the signal moved at that sample,
    // with nothing before the first one
    for (uint32_t n = 0; n < SIG_LEN; n++)
    {
        delta[n] = (n == 0U) ? x[0] : (x[n] - x[n - 1U]);
    }

    // the step response: a unit step in, and whatever comes out
    arm_fill_f32(1.0f, comp, SIG_LEN);
    system_fir(comp, step_res, SIG_LEN);

    printf("\r\nstep decomposition of a %lu sample signal\r\n", (unsigned long)SIG_LEN);
    printf("\r\nstep response s[n]:  ");

    for (uint32_t n = 0; n < 5U; n++)
    {
        printf(" %.2f", step_res[n]);
    }

    printf("\r\nits differences:     ");

    for (uint32_t n = 0; n < 5U; n++)
    {
        printf(" %.2f", (n == 0U) ? step_res[0] : (step_res[n] - step_res[n - 1U]));
    }

    printf("\r\nthose differences are the impulse response\r\n");

    /*
     * Take the signal apart and put it straight back together, with nothing
     * in between. Each component is a step, so the sum builds the signal as a
     * staircase rather than one sample at a time.
     */
    arm_fill_f32(0.0f, rebuilt, SIG_LEN);

    for (uint32_t k = 0; k < SIG_LEN; k++)
    {
        build_component(k);
        arm_add_f32(rebuilt, comp, rebuilt, SIG_LEN);
    }

    printf("\r\nsynthesis without the system, max gap: %.9f\r\n", max_gap(rebuilt, x, SIG_LEN));

    // now the same decomposition with the system in the middle
    system_fir(x, y_direct, SIG_LEN);
    arm_fill_f32(0.0f, rebuilt, SIG_LEN);

    for (uint32_t k = 0; k < SIG_LEN; k++)
    {
        build_component(k);
        system_fir(comp, comp_out, SIG_LEN);
        arm_add_f32(rebuilt, comp_out, rebuilt, SIG_LEN);
    }

    printf("%lu steps through the system, max gap against one pass: %.9f\r\n", (unsigned long)SIG_LEN, max_gap(rebuilt, y_direct, SIG_LEN));

    /*
     * The same output straight from the step response: every component is the
     * step response scaled by delta[k] and delayed by k.
     */
    arm_fill_f32(0.0f, rebuilt, SIG_LEN);

    for (uint32_t k = 0; k < SIG_LEN; k++)
    {
        for (uint32_t n = k; n < SIG_LEN; n++)
        {
            rebuilt[n] += delta[k] * step_res[n - k];
        }
    }

    printf("same result from the step response alone, max gap: %.9f\r\n", max_gap(rebuilt, y_direct, SIG_LEN));
    printf("\r\nstreaming: one step added per pass, %lu passes to rebuild\r\n", (unsigned long)SIG_LEN);

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

            build_component(added);
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

// component k is flat at zero until k, then holds delta[k] to the end
static void build_component(uint32_t k)
{
    for (uint32_t n = 0; n < SIG_LEN; n++)
    {
        comp[n] = (n < k) ? 0.0f : delta[k];
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