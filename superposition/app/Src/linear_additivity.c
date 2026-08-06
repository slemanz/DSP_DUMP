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
#define SIG2_HZ     0.4f

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

    while(1)
    {

    }
}

/*
 * path_a takes the sum through the system in one pass, path_b takes each
 * signal through on its own and adds the outputs afterwards.
 */
static float32_t additivity_error(system_fn run)
{
    float32_t max;
    float32_t index;

    run(sum_in, path_a, SIG_LEN);

    run(x1, out1, SIG_LEN);
    run(x2, out2, SIG_LEN);
    arm_add_f32(out1, out2, path_b, SIG_LEN);

    arm_sub_f32(path_a, path_b, diff, SIG_LEN);
    arm_absmax_f32(diff, SIG_LEN, &max, &index);

    return max;
}