#ifndef INC_DFT_H_
#define INC_DFT_H_

#include "arm_math.h"

#define TWO_PI          6.28318531f

/* An N point time domain signal becomes two frequency domain signals of
 * N/2 + 1 points each. N has to be even for that halving to mean anything. */
#define DFT_BINS(n)     ((n) / 2U + 1U)

/* Which frequency bin k stands for, in Hz, given the rate the signal was
 * sampled at. Bin N/2 sits at half the sampling rate, which is where the
 * sampling theorem said the highest recoverable frequency would be. */
#define BIN_HZ(k, n, fs)    ((float32_t)(k) * (float32_t)(fs) / (float32_t)(n))

void dft_forward(const float32_t *pSrc, uint32_t len, float32_t *pReX, float32_t *pImX);

void dft_inverse(const float32_t *pReX, const float32_t *pImX, uint32_t len, float32_t *pDst);

void dft_magnitude(const float32_t *pReX, const float32_t *pImX, uint32_t len, float32_t *pMag);

#endif /* INC_DFT_H_ */