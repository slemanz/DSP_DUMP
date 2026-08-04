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

static float32_t homegeneity_error(system_fn run, float32_t k);

int main(void)
{
    config_app();

    while(1)
    {

    }
}

/*
 * Runs both routes and returns how far apart they land. path_a is the input
 * scaled first and then put through the system, path_b is the input put
 * through the system first and scaled afterwards.
 */
static float32_t homegeneity_error(system_fn run, float32_t k)
{

    return 0.0f;
}