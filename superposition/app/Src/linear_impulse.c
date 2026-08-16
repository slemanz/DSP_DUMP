/*
 * Impulse decomposition, and why it is the split worth having.
 *
 * Any split works, as linear_superposition showed. This one works and costs
 * nothing to describe, because every piece is the same shape: a single sample
 * with zeros around it. Run that shape through the system once and you know
 * what the system does to all of them, since each piece is only that same
 * impulse scaled and moved.
 *
 * Those three numbers below are the entire system. Not the code, not the
 * coefficients written in system_fir, just its answer to one impulse.
 *
 * Adding up the scaled shifted copies is a named operation, and the last check
 * here is that arm_conv_f32 lands on the same output. That is the whole of the
 * next chapter, arriving as a consequence rather than a new idea.
 *
 *     make linear_impulse && make load && make debug
 */
#include <stdio.h>
#include <math.h>
#include "config.h"
#include "probe.h"
#include "systems.h"

#define SIG_LEN     25U
#define H_LEN       3U
#define SHOW        5U

static float32_t x[SIG_LEN];
static float32_t whole[SIG_LEN];
static float32_t impulse[SIG_LEN];
static float32_t h[SIG_LEN];
static float32_t piece[SIG_LEN];
static float32_t piece_out[SIG_LEN];
static float32_t rebuilt[SIG_LEN];
static float32_t gap[SIG_LEN];
static float32_t conv[SIG_LEN + SIG_LEN - 1U];

static float32_t worst_gap(const float32_t *pA, const float32_t *pB, uint32_t len)
{
    float32_t worst;
    uint32_t index;

    arm_sub_f32(pA, pB, gap, len);
    arm_absmax_f32(gap, len, &worst, &index);

    return worst;
}

int main(void)
{
    config_app();
    probe_reset();

    for (uint32_t n = 0; n < SIG_LEN; n++)
    {
        x[n] = 0.8f * sinf(TWO_PI * (float32_t)n / 16.0f)
             * (1.0f - ((float32_t)n / 40.0f));
    }

    system_fir(x, whole, SIG_LEN);

    /* ask the system one question: what does a single 1 at sample 0 become? */
    arm_fill_f32(0.0f, impulse, SIG_LEN);
    impulse[0] = 1.0f;
    system_fir(impulse, h, SIG_LEN);

    printf("\r\none impulse in, and this is what came out:\r\n\r\n  h[n] =");

    for (uint32_t n = 0; n < SHOW; n++)
    {
        printf(" %5.2f", (double)h[n]);
    }

    printf("\r\n\r\nthose %lu numbers are the system. Everything below uses only them.\r\n",
           (unsigned long)H_LEN);

    /* rebuild the output by scaling and shifting that one answer, once per
     * input sample, without calling the system again */
    arm_fill_f32(0.0f, rebuilt, SIG_LEN);

    for (uint32_t k = 0; k < SIG_LEN; k++)
    {
        for (uint32_t n = k; n < SIG_LEN; n++)
        {
            rebuilt[n] += x[k] * h[n - k];
        }
    }

    printf("\r\n%lu scaled and shifted copies of h, added up\r\n", (unsigned long)SIG_LEN);
    printf("  against one pass of the whole signal through the system: %.9f\r\n",
           (double)worst_gap(rebuilt, whole, SIG_LEN));

    arm_conv_f32(x, SIG_LEN, h, H_LEN, conv);
    printf("  against arm_conv_f32(x, h):                              %.9f\r\n",
           (double)worst_gap(conv, whole, SIG_LEN));

    printf("\r\nadding the pieces up has a name, and it is convolution\r\n");

    while (1)
    {
        arm_fill_f32(0.0f, rebuilt, SIG_LEN);

        for (uint32_t k = 0; k < SIG_LEN; k++)
        {
            arm_fill_f32(0.0f, piece, SIG_LEN);
            piece[k] = x[k];
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