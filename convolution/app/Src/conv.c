#include "conv.h"

/*
 * A 29 tap low pass with its cutoff at 6 kHz for a 48 kHz sampling rate. Each
 * tap is a sample of sinc weighted by a Hamming window, and the whole set is
 * scaled so the taps sum to one, which is what fixes the gain at DC. The taps
 * are symmetric, and that is what makes the delay it adds the same 14 samples
 * at every frequency instead of a different amount at each.
 * 
 * h[i] = sinc(2 * fc * (i - 14)) * (0.54 - 0.46 * cos(2*pi*i / 28))
 */
const float32_t lowpass_6khz[LOWPASS_LEN] =
{
    -0.0018225230f, -0.0015879294f, +0.0000000000f, +0.0036977508f, +0.0080754303f,
    +0.0085302217f, -0.0000000000f, -0.0173976984f, -0.0341458607f, -0.0333591565f,
    +0.0000000000f, +0.0676308395f, +0.1522061835f, +0.2229246956f, +0.2504960933f,
    +0.2229246956f, +0.1522061835f, +0.0676308395f, +0.0000000000f, -0.0333591565f,
    -0.0341458607f, -0.0173976984f, -0.0000000000f, +0.0085302217f, +0.0080754303f,
    +0.0036977508f, +0.0000000000f, -0.0015879294f, -0.0018225230f,
};

// input side. Every input sample drops one scaled copy of the second signal
// into the output, starting where that sample sits, and the copies overlap and
// add. This is the picture from the theory, written down.
void conv_scatter(const float32_t *pSrcA, uint32_t srcALen, const float32_t *pSrcB, uint32_t srcBLen, float32_t *pDst)
{
    uint32_t dstLen = srcALen + srcBLen - 1U;

    for(uint32_t n = 0; n < dstLen; n++)
    {
        pDst[n] = 0.0f;
    }

    for(uint32_t i = 0; i < srcALen; i++)
    {
        for(uint32_t j = 0; j < srcBLen; j++)
        {
            pDst[i + j] += pSrcA[i]*pSrcB[j];
        }
    }
}

// output side. Every output sample collects the input samples that can still
// reach it, each one weighted by the tap that spans the distance. Nothing is
// accumulated across iterations, so each output point stands on its own.
void conv_gather(const float32_t *pSrcA, uint32_t srcALen, const float32_t *pSrcB, uint32_t srcBLen, float32_t *pDst)
{
    uint32_t dstLen = srcALen + srcBLen - 1U;

    for(uint32_t n = 0; n < dstLen; n++)
    {
        float32_t acc = 0.0f;

        for(uint32_t j = 0; j < srcBLen; j++)
        {
            if((n >= j) && ((n - j) < srcALen))
            {
                acc += pSrcA[n - j] * pSrcB[j];
            }
        }

        pDst[n] = acc;
    }
}