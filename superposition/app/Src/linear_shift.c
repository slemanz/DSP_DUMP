/*
 * Shift invariance is the third property: a shift in the input produces the
 * same shift in the output and nothing else. It is a separate question from
 * linearity, and this is where that shows. The modulator is linear, it passes
 * both of the other tests, and it still fails this one, because its gain
 * depends on where a sample sits rather than on what the sample is worth.
 *
 *     make linear_shift && make load && make monitor
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
#define STREAM_SHIFT    25U

static float32_t x[SIG_LEN];
static float32_t shifted_in[SIG_LEN];
static float32_t y_ref[SIG_LEN];
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

static const uint32_t shifts[] = { 1U, 5U, 10U, 25U, CARRIER_LEN };

static float32_t shift_error(system_fn run, uint32_t s);

int main(void)
{
    config_app();

    while(1)
    {

    }
}

static float32_t shift_error(system_fn run, uint32_t s)
{

}