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

    for (uint32_t n = 0; n < SIG_LEN; n++)
    {
        x[n] = SIG_AMP * sinf(TWO_PI * SIG_HZ * (float32_t)n / (float32_t)MODEL_HZ);
    }

    printf("\r\nshift invariance: max |f(x[n-s]) - y[n-s]| over the samples\r\n");
    printf("where both are defined\r\n");
    printf("%10s", "s =");

    for (uint32_t k = 0; k < ARRAY_LEN(shifts); k++)
    {
        printf(" %11lu", (unsigned long)shifts[k]);
    }

    printf("\r\n");

    for (uint32_t i = 0; i < ARRAY_LEN(systems); i++)
    {
        printf("%10s", systems[i].name);

        for (uint32_t k = 0; k < ARRAY_LEN(shifts); k++)
        {
            printf(" %11.3e", shift_error(systems[i].run, shifts[k]));
        }

        printf("\r\n");
    }

    printf("\r\nclip and square fail the other two tests and pass this one:\r\n");
    printf("they have no memory, so nothing in them knows what n is\r\n");
    printf("modulate passes the other two and fails this one\r\n");
    printf("\r\nexcept at s = %lu, where the carrier comes back around to where\r\n", (unsigned long)CARRIER_LEN);
    printf("it started: a system that varies on a period looks shift invariant\r\n");
    printf("to any shift that is a multiple of it\r\n");

    printf("\r\nstreaming modulate with s = %lu\r\n", (unsigned long)STREAM_SHIFT);

    (void)shift_error(system_modulate, STREAM_SHIFT);

    uint32_t n = 0;

    while(1)
    {
        uint32_t i = n % SIG_LEN;

        g_input  = shifted_in[i];
        g_path_a = path_a[i];
        g_path_b = path_b[i];
        g_error  = diff[i];

        n++;
        probe_step();
    }
}

/*
 * path_a is the shifted input put through the system. path_b is the original
 * output, shifted by the same amount afterwards. The first s samples of the
 * shifted input are zero and have no counterpart in the reference, so the
 * comparison starts at n = s.
 */
static float32_t shift_error(system_fn run, uint32_t s)
{
    float32_t max;
    uint32_t index;

    run(x, y_ref, SIG_LEN);

    for(uint32_t n = 0; n < SIG_LEN; n++)
    {
        shifted_in[n] = (n < s) ? 0.0f : x[n - s];
        path_b[n]     = (n < s) ? 0.0f : y_ref[n - s];
    }

    run(shifted_in, path_a, SIG_LEN);

    arm_fill_f32(0.0f, diff, SIG_LEN);
    arm_sub_f32(&path_a[s], &path_b[s], &diff[s], SIG_LEN - s);
    arm_absmax_f32(&diff[s], SIG_LEN - s, &max, &index);

    return max;
}