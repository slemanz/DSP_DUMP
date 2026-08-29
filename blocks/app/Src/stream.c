#include "stream.h"

/*
 * numTaps is checked against the buffer rather than trusted. The lesson's
 * version declares the history array at a compile time length and then takes
 * the kernel length as an argument, so passing a 31 tap kernel to a filter with
 * a 10 entry buffer writes past the end of it, which is exactly the crash it
 * spends a video finding. The fix there is a calloc; the fix here is that the
 * buffer is as long as the longest kernel the module uses and the function
 * refuses anything longer.
 */
void stream_init(stream_fir_t *pInst, const float32_t *pCoeffs,
                 uint32_t numTaps)
{
    uint32_t i;

    pInst->numTaps  = (numTaps > STREAM_MAX_TAPS) ? STREAM_MAX_TAPS : numTaps;
    pInst->pCoeffs  = pCoeffs;
    pInst->write_at = 0U;

    for (i = 0U; i < STREAM_MAX_TAPS; i++)
    {
        pInst->history[i] = 0.0f;
    }
}

/*
 * One sample in, one sample out, and the same multiply accumulate the block
 * version does. The only difference is where the window comes from: here it is
 * read backwards out of the ring, newest first, which is what lines the samples
 * up against the kernel the right way round.
 */
float32_t stream_step(stream_fir_t *pInst, float32_t sample)
{
    float32_t out = 0.0f;
    uint32_t  read_at;
    uint32_t  i;

    pInst->history[pInst->write_at] = sample;

    read_at = pInst->write_at;

    for (i = 0U; i < pInst->numTaps; i++)
    {
        out += pInst->pCoeffs[i] * pInst->history[read_at];

        read_at = (read_at == 0U) ? (pInst->numTaps - 1U) : (read_at - 1U);
    }

    pInst->write_at = (pInst->write_at + 1U) % pInst->numTaps;

    return out;
}
