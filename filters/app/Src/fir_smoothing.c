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

    uint32_t  span = NOISE_LEN - (2U * SETTLE);
    float32_t sigma_in = residual_sigma(&sig_noisy[SETTLE], span);

    printf("\r\n%u samples of a 10 Hz sine buried in noise\r\n", NOISE_LEN);
    printf("standard deviation of the noise going in: %.4f\r\n\r\n",
           (double)sigma_in);

    printf("in the time domain, where it wins\r\n");
    printf("%6s %10s %10s %10s\r\n", "taps", "sigma out", "measured", "sqrt(M)");

    for (uint32_t k = 0U; k < ARRAY_LEN(taps); k++)
    {
        uint32_t m = taps[k];

        fir_moving_average(kernel, m);
        fir_apply(sig_noisy, NOISE_LEN, kernel, m, out);

        float32_t sigma = residual_sigma(&out[SETTLE + FIR_DELAY(m)], span);

        printf("%6lu %10.4f %10.2f %10.2f\r\n",
               (unsigned long)m, (double)sigma,
               (double)(sigma_in / sigma), (double)sqrtf((float32_t)m));
    }

    printf("\r\nin the frequency domain, where it loses\r\n");
    printf("%6s %10s %10s %10s\r\n", "taps", "100 Hz", "500 Hz", "worst dB");

    for (uint32_t k = 0U; k < ARRAY_LEN(taps); k++)
    {
        uint32_t  m = taps[k];
        float32_t worst = 0.0f;

        fir_moving_average(kernel, m);

        /* walk past the first null, then keep the tallest thing after it */
        for (float32_t f = (float32_t)TESTSIG_FS_HZ / (float32_t)m;
             f < (float32_t)TESTSIG_FS_HZ / 2.0f; f += 1.0f)
        {
            float32_t g = fir_gain(kernel, m, f, (float32_t)TESTSIG_FS_HZ);

            if (g > worst)
            {
                worst = g;
            }
        }

        printf("%6lu %10.4f %10.4f %10.1f\r\n", (unsigned long)m,
               (double)fir_gain(kernel, m, 100.0f, (float32_t)TESTSIG_FS_HZ),
               (double)fir_gain(kernel, m, 500.0f, (float32_t)TESTSIG_FS_HZ),
               (double)fir_db(worst));
    }

    printf("\r\na kernel of ones has its first null at fs/M, %.1f Hz for 11 taps\r\n",
           (double)((float32_t)TESTSIG_FS_HZ / 11.0f));

    fir_moving_average(kernel, 11U);
    fir_apply(sig_noisy, NOISE_LEN, kernel, 11U, out);


    while(1)
    {
        for (uint32_t n = 0U; n < NOISE_LEN; n++)
        {
            g_x   = sig_noisy[n];
            g_y   = out[n + FIR_DELAY(11U)];
            g_ref = clean_at(n);
            g_h   = (n < 11U) ? kernel[n] : 0.0f;
            probe_step();
        }
    }
}