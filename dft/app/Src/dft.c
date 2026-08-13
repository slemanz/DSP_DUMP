#include <math.h>
#include "dft.h"

/*
 * The forward transform asks one question N/2 + 1 times: how much of this test
 * wave is in the signal? The answer is the signal multiplied by the test wave
 * point by point and added up. Where the two line up the products pile up, and
 * where they do not they cancel out. That cancelling is the whole mechanism,
 * and dft_by_hand prints it happening.
 *
 * The sine sum is taken here without a minus sign in front. Plenty of books put
 * one there. Either works as long as the inverse agrees, and dft_inverse below
 * agrees with this one.
 */
void dft_forward(const float32_t *pSrc, uint32_t len, float32_t *pReX, float32_t *pImX)
{
    uint32_t bins = DFT_BINS(len);

     for (uint32_t k = 0; k < bins; k++)
    {
        float32_t re = 0.0f;
        float32_t im = 0.0f;

        for (uint32_t i = 0; i < len; i++)
        {
            float32_t angle = TWO_PI * (float32_t)k * (float32_t)i / (float32_t)len;

            re += pSrc[i] * cosf(angle);
            im += pSrc[i] * sinf(angle);
        }

        pReX[k] = re;
        pImX[k] = im;
    }
}

/*
 * Synthesis: add the waves back up. Each bin holds a sum of N products, so it
 * has to be scaled down before it can be used as an amplitude, and the scale is
 * N/2 everywhere except the first and last bins, where it is N. dft_by_hand
 * shows where those two exceptions come from instead of asserting them.
 */
void dft_inverse(const float32_t *pReX, const float32_t *pImX, uint32_t len, float32_t *pDst)
{
    uint32_t bins = DFT_BINS(len);

    for (uint32_t i = 0; i < len; i++)
    {
        pDst[i] = 0.0f;
    }

    for (uint32_t k = 0; k < bins; k++)
    {
        float32_t scale = ((k == 0U) || (k == (bins - 1U))) ? (float32_t)len : ((float32_t)len / 2.0f);
        float32_t re = pReX[k] / scale;
        float32_t im = pImX[k] / scale;

        for (uint32_t i = 0; i < len; i++)
        {
            float32_t angle = TWO_PI * (float32_t)k * (float32_t)i / (float32_t)len;

            pDst[i] += (re * cosf(angle)) + (im * sinf(angle));
        }
    }
}

// how tall the wave at this frequency is, whether the signal carried it as a
// cosine, as a sine, or as any mixture of the two
void dft_magnitude(const float32_t *pReX, const float32_t *pImX, uint32_t len, float32_t *pMag)
{
    uint32_t bins = DFT_BINS(len);

    for (uint32_t k = 0; k < bins; k++)
    {
        pMag[k] = sqrtf((pReX[k] * pReX[k]) + (pImX[k] * pImX[k]));
    }
}