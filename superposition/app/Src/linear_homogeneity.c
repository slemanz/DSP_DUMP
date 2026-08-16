/*
 * Homogeneity: turn the input up, and the output goes up by the same factor and
 * does nothing else.
 *
 * The test everyone writes first is whether f(k*x) equals k*f(x), which is
 * true or false and teaches nothing when it is false. This one divides the
 * output back down by k instead. If the system is homogeneous, dividing by k
 * undoes multiplying by k exactly, so the normalized output is the same curve
 * at every gain and the row of numbers below is flat. Where it is not flat, the
 * way it bends says what the system did.
 *
 *     make linear_homogeneity && make load && make debug
 */
#include <stdio.h>
#include <math.h>
#include "config.h"
#include "probe.h"
#include "systems.h"

#define SIG_LEN         100U
#define SIG_HZ          5.0f
#define SIG_AMP         0.5f
#define HOLD_PASSES     3U      // passes on the graph before the gain changes

#define STREAM_SEL      3U      // which system goes to the graph, see below

static const float32_t gains[] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };

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

static float32_t x[SIG_LEN];
static float32_t scaled[SIG_LEN];
static float32_t before[SIG_LEN];
static float32_t after[SIG_LEN];
static float32_t gap[SIG_LEN];

// runs k*x through the system and divides the result back down by k, which is
// the output the system would have given at k = 1 if it were homogeneous
static void normalized(system_fn run, float32_t k, float32_t *pDst)
{
    arm_scale_f32(x, k, scaled, SIG_LEN);
    run(scaled, pDst, SIG_LEN);
    arm_scale_f32(pDst, 1.0f / k, pDst, SIG_LEN);
}

int main(void)
{
    float32_t peak;
    uint32_t index;

    config_app();
    probe_reset();

    for (uint32_t n = 0; n < SIG_LEN; n++)
    {
        x[n] = SIG_AMP * sinf(TWO_PI * SIG_HZ * (float32_t)n / (float32_t)MODEL_HZ);
    }

    printf("\r\ninput is a %.0f Hz sine of amplitude %.2f, %lu samples\r\n",
           (double)SIG_HZ, (double)SIG_AMP, (unsigned long)SIG_LEN);
    printf("each output is divided by its own k, so a flat row is homogeneous\r\n\r\n");

    printf("%12s", "k =");

    for (uint32_t g = 0; g < ARRAY_LEN(gains); g++)
    {
        printf("%9.2f", (double)gains[g]);
    }

    printf("\r\n");

    for (uint32_t s = 0; s < ARRAY_LEN(systems); s++)
    {
        printf("%12s", systems[s].name);

        for (uint32_t g = 0; g < ARRAY_LEN(gains); g++)
        {
            normalized(systems[s].run, gains[g], before);
            arm_absmax_f32(before, SIG_LEN, &peak, &index);
            printf("%9.4f", (double)peak);
        }

        printf("\r\n");
    }

    /* clip falls away because it runs out of headroom, so the louder the input
     * the smaller the share of it that comes through. square climbs in step
     * with k because its output goes as k squared and only one k is being
     * divided out. Neither row is noise, each one is the system's own law. */
    printf("\r\nflat means homogeneous. Falling means running out of room.\r\n");
    printf("Climbing in step with k means the output goes as k squared.\r\n");
    printf("\r\nstreaming %s\r\n", systems[STREAM_SEL].name);

    while (1)
    {
        for (uint32_t g = 0; g < ARRAY_LEN(gains); g++)
        {
            normalized(systems[STREAM_SEL].run, gains[g], before);
            systems[STREAM_SEL].run(x, after, SIG_LEN);
            arm_sub_f32(before, after, gap, SIG_LEN);

            for (uint32_t p = 0; p < HOLD_PASSES; p++)
            {
                for (uint32_t n = 0; n < SIG_LEN; n++)
                {
                    g_x      = x[n];
                    g_before = before[n];
                    g_after  = after[n];
                    g_gap    = gap[n];

                    probe_step();
                }
            }
        }
    }
}