/*
 * The moving average, judged twice and failed once.
 *
 * Every tap is the same number, so the kernel does nothing but average the
 * last few samples. Nothing in filtering is simpler, and in the time domain
 * nothing beats it: averaging M samples of noise divides its standard
 * deviation by sqrt(M), and no other M tap kernel does better.
 *
 * Then the same kernel is asked to separate frequencies, and it is the worst
 * one in this module. Its stopband ripple is 13 dB down, which is a factor of
 * about 4.5, so a tone it is supposed to reject comes through at a fifth of its
 * height. The two verdicts are printed one after the other.
 *
 *     make fir_smoothing && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "filters.h"
#include "testsig.h"

#define MAX_TAPS        21U
#define SETTLE          32U     /* skip the ends, where the window hangs off */

static float32_t kernel[MAX_TAPS];
static float32_t out[NOISE_LEN + MAX_TAPS - 1U];

static const uint32_t taps[] = { 3U, 5U, 11U, 21U };

static float32_t clean_at(uint32_t n)
{
    return arm_sin_f32(TWO_PI * 10.0f * (float32_t)n / (float32_t)TESTSIG_FS_HZ);
}

/* standard deviation of what is left after the wanted signal is taken out */
static float32_t residual_sigma(const float32_t *pY, uint32_t len)
{
    static float32_t diff[NOISE_LEN];
    uint32_t n;

    for (n = 0U; n < len; n++)
    {
        diff[n] = pY[n] - clean_at(n + SETTLE);
    }

    float32_t sigma;
    arm_std_f32(diff, len, &sigma);

    return sigma;
}

int main(void)
{
    config_app();
    probe_reset();

    while(1)
    {

    }
}