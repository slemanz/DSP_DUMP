/*
 * Additivity is the second property: signals pass through the system without
 * interacting. Send x1 + x2 through in one go, then send each one through on
 * its own and add the outputs. A linear system gives the same answer either
 * way, which is exactly the statement of superposition.
 *
 * What a nonlinear system leaves between the two routes is not noise. For the
 * squarer it is 2*x1*x2, a signal at frequencies that were in neither input.
 *
 *     make linear_additivity && make load && make monitor
 */
#include <stdio.h>
#include <math.h>
#include "config.h"
#include "arm_math.h"
#include "probe.h"
#include "systems.h"

#define SIG_LEN     100U
#define SIG1_HZ     5.0f
#define SIG1_AMP    0.5f
#define SIG2_HZ     15.0f
#define SIG2_AMP    0.4f

static float32_t x1[SIG_LEN];
static float32_t x2[SIG_LEN];
static float32_t sum_in[SIG_LEN];
static float32_t out1[SIG_LEN];
static float32_t out2[SIG_LEN];
static float32_t path_a[SIG_LEN];
static float32_t path_b[SIG_LEN];
static float32_t diff[SIG_LEN];
static float32_t cross[SIG_LEN];

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

static float32_t additivity_error(system_fn run);

int main(void)
{
    config_app();

    for(uint32_t n = 0; n < SIG_LEN; n++)
    {
        float32_t t = (float32_t)n/(float32_t)MODEL_HZ;

        x1[n] = SIG1_AMP*sinf(TWO_PI*SIG1_HZ*t);
        x2[n] = SIG2_AMP*sinf(TWO_PI*SIG2_HZ*t);
    }

    arm_add_f32(x1, x2, sum_in, SIG_LEN);

    printf("\r\nadditivity: max |f(x1 + x2) - (f(x1) + f(x2))|\r\n");
    printf("x1 is %.0f Hz at %.2f, x2 is %.0f Hz at %.2f\r\n\r\n", SIG1_HZ, SIG1_AMP, SIG2_HZ, SIG2_AMP);

    for(uint32_t s = 0; s < ARRAY_LEN(systems); s++)
    {
        printf("%10s %12.3e\r\n", systems[s].name, additivity_error(systems[s].run));
    }

    printf("\r\nneither input reaches the %.2f rail on its own, and their sum\r\n", CLIP_LIMIT);
    printf("does, which is what interaction between signals looks like\r\n");

    float32_t residual;
    uint32_t index;

    (void)additivity_error(system_square);

    arm_mult_f32(x1, x2, cross, SIG_LEN);
    arm_scale_f32(cross, 2.0f, cross, SIG_LEN);
    arm_sub_f32(diff, cross, diff, SIG_LEN);
    arm_absmax_f32(diff, SIG_LEN, &residual, &index);

    printf("\r\nwhat square leaves between the routes is 2*x1*x2\r\n");
    printf("max residual against that product: %.9f\r\n", residual);

    printf("\r\nstreaming square: g_error is the product of two signals that\r\n");
    printf("were never multiplied by anything\r\n");

    (void)additivity_error(system_square);

    uint32_t n = 0;

    while(1)
    {
        uint32_t i = n % SIG_LEN;

        g_input  = sum_in[i];
        g_path_a = path_a[i];
        g_path_b = path_b[i];
        g_error  = diff[i];

        n++;
        probe_step();
    }
}

/*
 * path_a takes the sum through the system in one pass, path_b takes each
 * signal through on its own and adds the outputs afterwards.
 */
static float32_t additivity_error(system_fn run)
{
    float32_t max;
    uint32_t index;

    run(sum_in, path_a, SIG_LEN);

    run(x1, out1, SIG_LEN);
    run(x2, out2, SIG_LEN);
    arm_add_f32(out1, out2, path_b, SIG_LEN);

    arm_sub_f32(path_a, path_b, diff, SIG_LEN);
    arm_absmax_f32(diff, SIG_LEN, &max, &index);

    return max;
}