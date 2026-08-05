/*
 * Homogeneity is the first property of a linear system: a change in the
 * amplitude of the input produces the same change in the amplitude of the
 * output. The test is to send k*x through the system and compare it against
 * sending x through and scaling the result afterwards. A linear system leaves
 * nothing between the two routes.
 *
 *     make linear_homogeneity && make load && make monitor
 */

#include <stdio.h>
#include <math.h>
#include "config.h"
#include "arm_math.h"
#include "probe.h"
#include "systems.h"

#define SIG_LEN         100U
#define SIG_HZ          5.0f
#define SIG_AMP         0.5f
#define STREAM_GAIN     2.0f
#define HOLD_PASSES     8U

static float32_t x[SIG_LEN];
static float32_t scaled[SIG_LEN];
static float32_t path_a[SIG_LEN];
static float32_t path_b[SIG_LEN];
static float32_t diff[SIG_LEN];

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

static const float32_t gains[] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };

static float32_t homogeneity_error(system_fn run, float32_t k);

int main(void)
{
    config_app();

    for(uint32_t n = 0; n < SIG_LEN; n++)
    {
        x[n] = SIG_AMP*sinf(TWO_PI*SIG_HZ*((float32_t)n/(float32_t)MODEL_HZ));
    }

    printf("\r\nhomogeneity: max |f(k*x) - k*f(x)| over %lu samples\r\n", (unsigned long)SIG_LEN);
    printf("%10s", "k =");

    for (uint32_t g = 0; g < ARRAY_LEN(gains); g++)
    {
        printf(" %11.2f", gains[g]);
    }
    printf("\r\n");

    for(uint32_t s = 0; s < ARRAY_LEN(systems); s++)
    {
        printf("%10s", systems[s].name);

        for (uint32_t g = 0; g < ARRAY_LEN(gains); g++)
        {
            printf(" %11.3e", homogeneity_error(systems[s].run, gains[g]));
        }
        printf("\r\n");
    }

    printf("\r\nfir and modulate are linear, so their columns are float rounding\r\n");
    printf("and nothing else\r\n");
    printf("\r\nthe signal peaks at %.2f and the clipper holds at %.2f, so clip\r\n", SIG_AMP, CLIP_LIMIT);
    printf("looks perfectly linear until k passes %.2f\r\n", CLIP_LIMIT / SIG_AMP);
    printf("square passes at k = 1 alone, where k*k and k are the same number\r\n");

    printf("\r\nstreaming k = %.1f, one system every %lu passes\r\n", STREAM_GAIN, (unsigned long)HOLD_PASSES);

    uint32_t n = 0;
    uint32_t shown = ARRAY_LEN(systems);

    while(1)
    {
        g_input = x[n % SIG_LEN];

        n++;
        probe_step();
    }
}

/*
 * Runs both routes and returns how far apart they land. path_a is the input
 * scaled first and then put through the system, path_b is the input put
 * through the system first and scaled afterwards.
 */
static float32_t homogeneity_error(system_fn run, float32_t k)
{
    float32_t max;
    uint32_t index;

    arm_scale_f32(x, k, scaled, SIG_LEN);
    run(scaled, path_a, SIG_LEN);

    run(x, path_b, SIG_LEN);
    arm_scale_f32(x, k, path_b, SIG_LEN);

    arm_sub_f32(path_a, path_b, diff, SIG_LEN);
    arm_absmax_f32(diff, SIG_LEN, &max, &index);

    return max;
}