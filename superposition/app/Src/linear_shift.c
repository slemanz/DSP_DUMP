/*
 * Shift invariance: delay the input and the output comes out the same, just
 * later. Nothing about what the system does depends on when it happens.
 *
 * This is the property that gets quietly folded into linearity and is not part
 * of it. Two of the four systems here are nonlinear and pass this test. One is
 * perfectly linear and fails it. Linear and shift invariant are two separate
 * things and a system can be either without the other.
 *
 * One shift is not a test. The modulator's gain repeats every CARRIER_LEN
 * samples, so shifting by exactly that much lines it back up with itself and it
 * looks invariant. A system that varies with a period will pass at every
 * multiple of that period and fail everywhere else.
 *
 *     make linear_shift && make load && make debug
 */
#include <stdio.h>
#include <math.h>
#include "config.h"
#include "probe.h"
#include "systems.h"

#define SIG_LEN         100U
#define SIG_HZ          5.0f
#define SIG_AMP         0.5f
#define AGREES_BELOW    1.0e-5f
#define STREAM_SHIFT    10U

#define STREAM_SEL      3U      // the modulator, the only one with a story here

static const uint32_t shifts[] = { 1U, 5U, 10U, 25U, CARRIER_LEN };

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
static float32_t shifted_in[SIG_LEN];
static float32_t plain_out[SIG_LEN];
static float32_t before[SIG_LEN];
static float32_t after[SIG_LEN];
static float32_t gap[SIG_LEN];

/*
 * before: shift the input, then run it. after: run the input, then shift the
 * output. The first s samples of both are made up, because the signal that
 * would have been there is off the front of the buffer, so they are left out of
 * the comparison.
 */
static float32_t two_routes(system_fn run, uint32_t s)
{
    float32_t worst;
    uint32_t index;

    run(x, plain_out, SIG_LEN);

    for (uint32_t n = 0; n < SIG_LEN; n++)
    {
        shifted_in[n] = (n < s) ? 0.0f : x[n - s];
        after[n]      = (n < s) ? 0.0f : plain_out[n - s];
    }

    run(shifted_in, before, SIG_LEN);

    arm_fill_f32(0.0f, gap, SIG_LEN);
    arm_sub_f32(&before[s], &after[s], &gap[s], SIG_LEN - s);
    arm_absmax_f32(&gap[s], SIG_LEN - s, &worst, &index);

    return worst;
}

int main(void)
{
    float32_t worst;

    config_app();
    probe_reset();

    for (uint32_t n = 0; n < SIG_LEN; n++)
    {
        x[n] = SIG_AMP * sinf(TWO_PI * SIG_HZ * (float32_t)n / (float32_t)MODEL_HZ);
    }

    printf("\r\ndoes shifting before the system match shifting after?\r\n\r\n");
    printf("%12s", "s =");

    for (uint32_t i = 0; i < ARRAY_LEN(shifts); i++)
    {
        printf("%6lu", (unsigned long)shifts[i]);
    }

    printf("\r\n");

    for (uint32_t s = 0; s < ARRAY_LEN(systems); s++)
    {
        printf("%12s", systems[s].name);

        for (uint32_t i = 0; i < ARRAY_LEN(shifts); i++)
        {
            worst = two_routes(systems[s].run, shifts[i]);
            printf("%6s", (worst < AGREES_BELOW) ? "yes" : "no");
        }

        printf("\r\n");
    }

    /* The last column is the one to look at twice. It is CARRIER_LEN, the
     * length of the modulator's own cycle, and at that shift a time varying
     * system hands back a perfect result. Test at one shift and this is the
     * shift that lies to you. */
    printf("\r\nclip and square are not linear and pass. modulate is linear and fails.\r\n");
    printf("the last column is s = %lu, which is exactly the modulator's cycle.\r\n",
           (unsigned long)CARRIER_LEN);

    (void)two_routes(systems[STREAM_SEL].run, STREAM_SHIFT);
    printf("\r\nstreaming %s at s = %lu\r\n", systems[STREAM_SEL].name,
           (unsigned long)STREAM_SHIFT);

    while (1)
    {
        for (uint32_t n = 0; n < SIG_LEN; n++)
        {
            g_x      = shifted_in[n];
            g_before = before[n];
            g_after  = after[n];
            g_gap    = gap[n];

            probe_step();
        }
    }
}