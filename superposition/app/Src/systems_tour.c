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
#define TABLE_ROWS      10U     // samples in the numeric table

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

    for(uint32_t s = 0; s < ARRAY_LEN(systems); s++)
    {
        systems[s].run(x, y[s], SIG_LEN);
    }

    arm_sub_f32(y[SYSTEM_SEL], x, diff, SIG_LEN);

    printf("\r\ninput: %s at %.2f, first %lu of %lu samples\r\n", inputs[INPUT_SEL].name, SIG_AMP, (unsigned long)TABLE_ROWS, (unsigned long)SIG_LEN);
    printf("\r\n%4s %9s", "n", "x");

    for (uint32_t s = 0; s < ARRAY_LEN(systems); s++)
    {
        printf(" %9s", systems[s].name);
    }
    printf("\r\n");

    for (uint32_t n = 0; n < TABLE_ROWS; n++)
    {
        printf("%4lu %+9.4f", (unsigned long)n, x[n]);

        for (uint32_t s = 0; s < ARRAY_LEN(systems); s++)
        {
            printf(" %+9.4f", y[s][n]);
        }

        printf("\r\n");
    }

    arm_absmax_f32(x, SIG_LEN, &peak_in, &index);
    arm_absmax_f32(y[SYSTEM_SEL], SIG_LEN, &peak_out, &index);

    printf("\r\nover the whole buffer, peak in %.3f and peak out %.3f, so %s\r\n", peak_in, peak_out, systems[SYSTEM_SEL].name);
    printf("came out %.2f times as tall\r\n", peak_out / peak_in);
    printf("\r\nstreaming %s through %s to the graph\r\n", inputs[INPUT_SEL].name, systems[SYSTEM_SEL].name);

    /* one route through one system, so the fourth trace has nothing to carry */
    g_path_b = 0.0f;

    uint32_t n = 0;

    while (1)
    {
        uint32_t i = n % SIG_LEN;

        g_input = x[i];
        g_path_a = y[SYSTEM_SEL][i];
        g_error  = diff[i];

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