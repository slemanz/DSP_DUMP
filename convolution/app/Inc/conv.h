#ifndef INC_CONV_H_
#define INC_CONV_H_

#include "arm_math.h"

#define LOWPASS_LEN     29U

extern const float32_t lowpass_6khz[LOWPASS_LEN];

/*
 * Both take their arguments in the same order as arm_conv_f32, so swapping one
 * for the library call is a change of name and nothing else.
 */
void conv_scatter(const float32_t *pSrcA, uint32_t srcALen, const float32_t *pSrcB, uint32_t srcBLen, float32_t *pDst);

void conv_gather(const float32_t *pSrcA, uint32_t srcALen, const float32_t *pSrcB, uint32_t srcBLen, float32_t *pDst);

#endif /* INC_CONV_H_ */