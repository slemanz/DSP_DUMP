/*
 * Step decomposition: the same signal split into steps instead of impulses.
 *
 * Piece k is flat at zero until sample k and then holds a constant from there
 * to the end, and the constant is x[k] - x[k-1], the amount the signal changed
 * at that sample. Stacking those steps rebuilds the signal, because each one
 * contributes exactly the change it is named after and the changes telescope
 * back to the original.
 *
 * The step response is what a single unit step becomes, and the first line of
 * output below is the reason this decomposition is worth knowing: the
 * differences of the step response are the impulse response. Measuring one
 * gives the other, and a step is much easier to produce in a real circuit than
 * an impulse is.
 *
 *     make linear_step && make load && make debug
 */
#include <stdio.h>
#include <math.h>
#include "config.h"
#include "probe.h"
#include "systems.h"

#define SIG_LEN     25U
#define SHOW        5U

static float32_t x[SIG_LEN];
static float32_t whole[SIG_LEN];
static float32_t delta[SIG_LEN];
static float32_t unit[SIG_LEN];
static float32_t step_res[SIG_LEN];
static float32_t impulse[SIG_LEN];
static float32_t h[SIG_LEN];
static float32_t piece[SIG_LEN];
static float32_t piece_out[SIG_LEN];
static float32_t rebuilt[SIG_LEN];
static float32_t gap[SIG_LEN];

static void show_row(const char *name, const float32_t *pSrc)
{
    printf("%22s", name);

    for (uint32_t n = 0; n < SHOW; n++)
    {
        printf(" %5.2f", (double)pSrc[n]);
    }

    printf("\r\n");
}

// piece k is flat at zero until k, then holds the change the signal made there
static void take_piece(uint32_t k)
{
    for (uint32_t n = 0; n < SIG_LEN; n++)
    {
        piece[n] = (n < k) ? 0.0f : delta[k];
    }
}

int main(void)
{
    float32_t worst;
    uint32_t index;

    config_app();
    probe_reset();

    for (uint32_t n = 0; n < SIG_LEN; n++)
    {
        x[n] = 0.8f * sinf(TWO_PI * (float32_t)n / 16.0f)
             * (1.0f - ((float32_t)n / 40.0f));
    }

    system_fir(x, whole, SIG_LEN);

    for (uint32_t n = 0; n < SIG_LEN; n++)
    {
        delta[n] = (n == 0U) ? x[0] : (x[n] - x[n - 1U]);
    }

    /* one unit step in, and one unit impulse in, so the two answers can be put
     * next to each other */
    arm_fill_f32(1.0f, unit, SIG_LEN);
    system_fir(unit, step_res, SIG_LEN);

    arm_fill_f32(0.0f, impulse, SIG_LEN);
    impulse[0] = 1.0f;
    system_fir(impulse, h, SIG_LEN);

    printf("\r\n");
    show_row("step response s[n]", step_res);

    for (uint32_t n = 0; n < SIG_LEN; n++)
    {
        gap[n] = (n == 0U) ? step_res[0] : (step_res[n] - step_res[n - 1U]);
    }

    show_row("its differences", gap);
    show_row("impulse response h[n]", h);

    printf("\r\nthe middle row is the bottom row. A step is easy to generate and\r\n");
    printf("an impulse is not, so this is usually how h[n] gets measured.\r\n");

    arm_fill_f32(0.0f, rebuilt, SIG_LEN);

    for (uint32_t k = 0; k < SIG_LEN; k++)
    {
        take_piece(k);
        system_fir(piece, piece_out, SIG_LEN);
        arm_add_f32(rebuilt, piece_out, rebuilt, SIG_LEN);
    }

    arm_sub_f32(rebuilt, whole, gap, SIG_LEN);
    arm_absmax_f32(gap, SIG_LEN, &worst, &index);
    printf("\r\n%lu steps through the system, added up: %.9f from the whole\r\n",
           (unsigned long)SIG_LEN, (double)worst);

    /* and the same output again from the step response alone, never calling
     * the system, the way linear_impulse rebuilt it from h[n] alone */
    arm_fill_f32(0.0f, rebuilt, SIG_LEN);

    for (uint32_t k = 0; k < SIG_LEN; k++)
    {
        for (uint32_t n = k; n < SIG_LEN; n++)
        {
            rebuilt[n] += delta[k] * step_res[n - k];
        }
    }

    arm_sub_f32(rebuilt, whole, gap, SIG_LEN);
    arm_absmax_f32(gap, SIG_LEN, &worst, &index);
    printf("from the step response alone, never calling the system: %.9f\r\n", (double)worst);

    while (1)
    {
        arm_fill_f32(0.0f, rebuilt, SIG_LEN);

        for (uint32_t k = 0; k < SIG_LEN; k++)
        {
            take_piece(k);
            system_fir(piece, piece_out, SIG_LEN);
            arm_add_f32(rebuilt, piece_out, rebuilt, SIG_LEN);
            arm_sub_f32(rebuilt, whole, gap, SIG_LEN);

            for (uint32_t n = 0; n < SIG_LEN; n++)
            {
                g_x      = piece[n];
                g_before = whole[n];
                g_after  = rebuilt[n];
                g_gap    = gap[n];

                probe_step();
            }
        }
    }
}