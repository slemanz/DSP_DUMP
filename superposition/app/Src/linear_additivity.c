/*
 * Additivity: two signals go through together without noticing each other.
 *
 * Add first and then run, or run each one and then add. On a linear system
 * those give the same result. The interesting part is what the difference is
 * when they do not, because it is not an error and it is not noise.
 *
 * For the squarer the leftover is exactly 2*x1*x2, and that product is itself
 * two clean waves at the sum and the difference of the two input frequencies.
 * The inputs hold 5 Hz and 15 Hz. The leftover holds 10 Hz and 20 Hz, which
 * were in neither of them. A nonlinear system does not degrade a signal, it
 * manufactures new ones.
 *
 *     make linear_additivity && make load && make debug
 */
#include <stdio.h>
#include <math.h>
#include "config.h"
#include "probe.h"
#include "systems.h"

#define SIG_LEN         100U
#define SIG1_HZ         5.0f
#define SIG1_AMP        0.5f
#define SIG2_HZ         15.0f
#define SIG2_AMP        0.4f
#define AGREES_BELOW    1.0e-5f

#define STREAM_SEL      2U      // the squarer, so the graph has something to show

static const struct
{
    const char *name;
    system_fn   run;
} systems[] = {
    { "fir",      system_fir      },
    { "clip",     system_clip     },
    { "square",   system_square   },
    { "modulate", system_modulate },
};

static float32_t x1[SIG_LEN];
static float32_t x2[SIG_LEN];
static float32_t sum_in[SIG_LEN];
static float32_t out1[SIG_LEN];
static float32_t out2[SIG_LEN];
static float32_t before[SIG_LEN];
static float32_t after[SIG_LEN];
static float32_t gap[SIG_LEN];
static float32_t predicted[SIG_LEN];
static float32_t work[SIG_LEN];

// before: add the two, then run. after: run each, then add.
static float32_t two_routes(system_fn run)
{
    float32_t worst;
    uint32_t index;

    run(sum_in, before, SIG_LEN);

    run(x1, out1, SIG_LEN);
    run(x2, out2, SIG_LEN);
    arm_add_f32(out1, out2, after, SIG_LEN);

    arm_sub_f32(before, after, gap, SIG_LEN);
    arm_absmax_f32(gap, SIG_LEN, &worst, &index);

    return worst;
}

int main(void)
{
    float32_t worst;
    float32_t match;
    uint32_t index;

    config_app();
    probe_reset();

    for (uint32_t n = 0; n < SIG_LEN; n++)
    {
        float32_t t = (float32_t)n / (float32_t)MODEL_HZ;

        x1[n] = SIG1_AMP * sinf(TWO_PI * SIG1_HZ * t);
        x2[n] = SIG2_AMP * sinf(TWO_PI * SIG2_HZ * t);
    }

    arm_add_f32(x1, x2, sum_in, SIG_LEN);

    printf("\r\n%.0f Hz at %.2f plus %.0f Hz at %.2f\r\n",
           (double)SIG1_HZ, (double)SIG1_AMP, (double)SIG2_HZ, (double)SIG2_AMP);
    printf("neither one reaches the clipper's %.2f rail on its own, together they do\r\n\r\n",
           (double)CLIP_LIMIT);

    printf("does adding before the system match adding after?\r\n");

    for (uint32_t s = 0; s < ARRAY_LEN(systems); s++)
    {
        worst = two_routes(systems[s].run);
        printf("%12s   %s\r\n", systems[s].name, (worst < AGREES_BELOW) ? "yes" : "no");
    }

    /* Build the leftover from scratch, out of nothing but the two input
     * frequencies, and see whether it lands on what the squarer produced. The
     * product of two sines is a wave at their difference and a wave at their
     * sum, both scaled by the two amplitudes multiplied together. */
    (void)two_routes(systems[STREAM_SEL].run);

    for (uint32_t n = 0; n < SIG_LEN; n++)
    {
        float32_t t = (float32_t)n / (float32_t)MODEL_HZ;

        predicted[n] = SIG1_AMP * SIG2_AMP
                     * (cosf(TWO_PI * (SIG2_HZ - SIG1_HZ) * t)
                      - cosf(TWO_PI * (SIG2_HZ + SIG1_HZ) * t));
    }

    arm_sub_f32(gap, predicted, work, SIG_LEN);
    arm_absmax_f32(work, SIG_LEN, &match, &index);

    printf("\r\nwhat the squarer left over is a signal, not an error:\r\n");
    printf("  %.2f * cos(2pi * %.0f Hz * t) - %.2f * cos(2pi * %.0f Hz * t)\r\n",
           (double)(SIG1_AMP * SIG2_AMP), (double)(SIG2_HZ - SIG1_HZ),
           (double)(SIG1_AMP * SIG2_AMP), (double)(SIG2_HZ + SIG1_HZ));
    printf("  built from scratch, it lands on the leftover to %.9f\r\n", (double)match);
    printf("  the inputs held %.0f and %.0f Hz. Neither held %.0f or %.0f.\r\n",
           (double)SIG1_HZ, (double)SIG2_HZ,
           (double)(SIG2_HZ - SIG1_HZ), (double)(SIG2_HZ + SIG1_HZ));

    while (1)
    {
        for (uint32_t n = 0; n < SIG_LEN; n++)
        {
            g_x      = sum_in[n];
            g_before = before[n];
            g_after  = after[n];
            g_gap    = gap[n];

            probe_step();
        }
    }
}