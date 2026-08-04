#include <math.h>
#include "systems.h"
#include "probe.h"

/*
 * The four systems the properties get tested against. Each one takes a whole
 * buffer and keeps no state between calls, so a test is always a comparison
 * between two pure results and never depends on what ran before it.
 */

// linear and shift invariant: a three tap weighted average
void system_fir(const float32_t *pSrc, float32_t *pDst, uint32_t len)
{
    for (uint32_t n = 0; n < len; n++)
    {
        float32_t acc = 0.5f * pSrc[n];

        if (n >= 1U)
        {
            acc += 0.3f * pSrc[n - 1U];
        }

        if (n >= 2U)
        {
            acc += 0.2f * pSrc[n - 2U];
        }

        pDst[n] = acc;
    }
}

// an amplifier out of headroom: linear while the signal stays off the rail,
// nonlinear from there on
void system_clip(const float32_t *pSrc, float32_t *pDst, uint32_t len)
{
    for (uint32_t n = 0; n < len; n++)
    {
        float32_t x = pSrc[n];

        if (x > CLIP_LIMIT)
        {
            x = CLIP_LIMIT;
        }
        else if (x < -CLIP_LIMIT)
        {
            x = -CLIP_LIMIT;
        }

        pDst[n] = x;
    }
}

// nonlinear at every amplitude, with no linear range to hide in
void system_square(const float32_t *pSrc, float32_t *pDst, uint32_t len)
{
    for (uint32_t n = 0; n < len; n++)
    {
        pDst[n] = pSrc[n] * pSrc[n];
    }
}

// linear but time varying: the gain depends on where the sample sits, not on
// what the sample is worth
void system_modulate(const float32_t *pSrc, float32_t *pDst, uint32_t len)
{
    for (uint32_t n = 0; n < len; n++)
    {
        pDst[n] = pSrc[n] * cosf(TWO_PI * (float32_t)n / (float32_t)CARRIER_LEN);
    }
}