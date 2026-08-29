#ifndef INC_STREAM_H_
#define INC_STREAM_H_

#include "arm_math.h"

#define ARRAY_LEN(a)    (sizeof(a) / sizeof((a)[0]))

#define STREAM_MAX_TAPS 64U

/*
 * A filter that runs one sample at a time and never sees an array.
 *
 * The trick is that it keeps its own: the last numTaps inputs live in a
 * circular buffer inside the instance, so every call has the whole window it
 * needs without anyone having to assemble one. The kernel is passed in rather
 * than baked in, so the same instance type serves any filter.
 */
typedef struct
{
    float32_t        history[STREAM_MAX_TAPS];
    uint32_t         write_at;
    const float32_t *pCoeffs;
    uint32_t         numTaps;
} stream_fir_t;

void      stream_init(stream_fir_t *pInst, const float32_t *pCoeffs,
                      uint32_t numTaps);
float32_t stream_step(stream_fir_t *pInst, float32_t sample);

#endif /* INC_STREAM_H_ */
