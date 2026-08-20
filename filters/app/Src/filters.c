#include <math.h>
#include "filters.h"

/*
 * Convolution again, unchanged from the previous chapter and repeated here so
 * this module stands on its own. Every filter in the module is this function
 * and a different kernel.
 */
void fir_apply(const float32_t *pSrc, uint32_t srcLen,
               const float32_t *pH, uint32_t hLen, float32_t *pDst)
{
    uint32_t n;
    uint32_t i;

    for (n = 0U; n < (srcLen + hLen - 1U); n++)
    {
        pDst[n] = 0.0f;
    }

    for (i = 0U; i < srcLen; i++)
    {
        for (n = 0U; n < hLen; n++)
        {
            pDst[i + n] += pSrc[i] * pH[n];
        }
    }
}

void fir_moving_average(float32_t *pH, uint32_t hLen)
{
    uint32_t i;

    for (i = 0U; i < hLen; i++)
    {
        pH[i] = 1.0f / (float32_t)hLen;
    }
}

/*
 * Negate every tap and add one to the middle. What that does is subtract the
 * kernel from a single impulse, and an impulse passes everything, so what is
 * left passes everything the kernel did not. The two kernels add back to that
 * impulse exactly, which is the check fir_inversion prints.
 */
void fir_invert(const float32_t *pH, uint32_t hLen, float32_t *pDst)
{
    uint32_t i;

    for (i = 0U; i < hLen; i++)
    {
        pDst[i] = -pH[i];
    }

    pDst[hLen / 2U] += 1.0f;
}

/*
 * The same sum the DFT is built from, evaluated at one frequency instead of at
 * a bin. A kernel is a finite list of numbers and its response is a continuous
 * curve, so nothing here has to land on a bin and no window length is involved.
 */
float32_t fir_gain(const float32_t *pH, uint32_t hLen,
                   float32_t f_hz, float32_t fs_hz)
{
    float32_t re = 0.0f;
    float32_t im = 0.0f;
    float32_t w  = TWO_PI * f_hz / fs_hz;
    uint32_t  i;

    for (i = 0U; i < hLen; i++)
    {
        re += pH[i] * cosf(w * (float32_t)i);
        im -= pH[i] * sinf(w * (float32_t)i);
    }

    return sqrtf((re * re) + (im * im));
}

float32_t fir_db(float32_t gain)
{
    float32_t out;

    if (gain < 1.0e-6f)
    {
        return DB_FLOOR;
    }

    out = 20.0f * log10f(gain);

    return (out < DB_FLOOR) ? DB_FLOOR : out;
}