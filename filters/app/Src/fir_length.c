/*
 * The other knob.
 *
 * The previous app held the length fixed and changed the window. This one
 * holds the window fixed and changes the length, and the interesting part is
 * the column that does not move.
 *
 * More taps make the transition narrower and leave the stopband exactly where
 * the window put it. So the two choices do not interact: pick the window from
 * how much rejection is needed, pick the length from how sharp the corner has
 * to be, and neither answer changes the other.
 *
 * What length costs is arithmetic and delay. Both are on the table.
 *
 *     make fir_length && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "filters.h"
#include "kernels.h"
#include "testsig.h"

#define FC_HZ       200.0f
#define SWEEP       201U

static const float32_t *const kernel[] = { lp_31, lp_hamming, lp_201 };
static const uint32_t taps[] = { LP_31_LEN, LP_HAMMING_LEN, LP_201_LEN };

static float32_t stopband(const float32_t *pH, uint32_t hLen)
{
    float32_t prev = 1.0f;
    float32_t worst = 0.0f;
    uint8_t   past_null = 0U;

    for (float32_t f = FC_HZ; f < (float32_t)TESTSIG_FS_HZ / 2.0f; f += 0.5f)
    {
        float32_t g = fir_gain(pH, hLen, f, (float32_t)TESTSIG_FS_HZ);

        if ((past_null == 0U) && (g >= prev))
        {
            past_null = 1U;
        }

        if ((past_null != 0U) && (g > worst))
        {
            worst = g;
        }

        prev = g;
    }

    return fir_db(worst);
}

static float32_t transition(const float32_t *pH, uint32_t hLen)
{
    float32_t lo = 0.0f;

    for (float32_t f = 0.0f; f < (float32_t)TESTSIG_FS_HZ / 2.0f; f += 0.5f)
    {
        float32_t g = fir_gain(pH, hLen, f, (float32_t)TESTSIG_FS_HZ);

        if (g >= 0.99f)
        {
            lo = f;
        }

        if ((f > lo) && (g <= 0.01f))
        {
            return f - lo;
        }
    }

    return 0.0f;
}

int main(void)
{
    config_app();
    probe_reset();

}