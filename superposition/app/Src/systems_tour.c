/*
 * The four systems this module tests are small loops, and the fastest way to
 * learn what one of them does to a signal is to send a signal through it and
 * look at both ends. Nothing is compared here and no property is tested: one
 * input goes in, one output comes out.
 *
 * Three numbers pick the experiment. INPUT_SEL chooses the shape that goes in,
 * SYSTEM_SEL chooses the system it goes through, and SIG_AMP says how tall the
 * input is. Change any of them, rebuild, and look again.
 *
 *     make systems_tour && make load && make monitor
 */
#include <stdio.h>
#include <math.h>
#include "config.h"
#include "arm_math.h"
#include "probe.h"
#include "systems.h"

#define SIG_LEN         100U
#define SIG_HZ          5.0f
#define STEP_AT         4U      // where the step rises and where the impulse sits

#define TABLE_ROWS      12U     // samples in the numeric table
#define PLOT_ROWS       SIG_LEN // samples in the ascii plot
#define PLOT_COLS       41U     // odd, so zero gets a column of its own
#define PLOT_FULL       1.2f    // amplitude that reaches the edge of the plot

/************************************************************
*                       THE THREE KNOBS                     *
*************************************************************/

#define INPUT_SEL       0U      // 0 sine | 1 step | 2 impulse | 3 ramp
#define SYSTEM_SEL      1U      // 0 fir  | 1 clip | 2 square  | 3 modulate
#define SIG_AMP         1.0f    // how tall the input is; the clipper holds at 0.6

/************************************************************/

typedef void (*input_fn)(float32_t *pDst, uint32_t len);

static void input_sine(float32_t *pDst, uint32_t len);
static void input_step(float32_t *pDst, uint32_t len);
static void input_impulse(float32_t *pDst, uint32_t len);
static void input_ramp(float32_t *pDst, uint32_t len);

static uint32_t plot_column(float32_t v);
static void plot_row(float32_t in, float32_t out);

static const struct
{
    const char *name;
    input_fn    make;
} inputs[] = {
    { "sine",    input_sine    },
    { "step",    input_step    },
    { "impulse", input_impulse },
    { "ramp",    input_ramp    },
};

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
static float32_t y[ARRAY_LEN(systems)][SIG_LEN];
static float32_t diff[SIG_LEN];

int main(void)
{
    float32_t peak_in;
    float32_t peak_out;
    uint32_t index;

    config_app();

    inputs[INPUT_SEL].make(x, SIG_LEN);

    uint32_t n = 0;

    while (1)
    {
        uint32_t i = n % SIG_LEN;

        g_input = x[i];

        n++;
        probe_step();
    }
}

static void input_sine(float32_t *pDst, uint32_t len)
{
    for(uint32_t n = 0; n < len; n++)
    {
        pDst[n] = SIG_AMP*sinf(TWO_PI*SIG_HZ*(float32_t)n/(float32_t)MODEL_HZ);
    }
}

static void input_step(float32_t *pDst, uint32_t len)
{
    for(uint32_t n = 0; n < len; n++)
    {
        pDst[n] = (n < STEP_AT) ? 0.0f : SIG_AMP;
    }
}

static void input_impulse(float32_t *pDst, uint32_t len)
{
    for(uint32_t n = 0; n < len; n++)
    {
        pDst[n] = (n == STEP_AT) ? SIG_AMP : 0.0f;
    }
}

static void input_ramp(float32_t *pDst, uint32_t len)
{
    for (uint32_t n = 0; n < len; n++)
    {
        pDst[n] = SIG_AMP * (float32_t)n / (float32_t)(len - 1U);
    }
}