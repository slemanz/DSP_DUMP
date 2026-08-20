#ifndef INC_FILTERS_H_
#define INC_FILTERS_H_

#include "arm_math.h"

#define TWO_PI          6.28318531f

/* how far down the graph bothers to look */
#define DB_FLOOR        -80.0f

#define ARRAY_LEN(a)    (sizeof(a) / sizeof((a)[0]))

/* the delay a symmetric kernel adds, in samples */
#define FIR_DELAY(len)  ((len) / 2U)

/* dst holds srcLen + hLen - 1 points, the same as arm_conv_f32 */
void fir_apply(const float32_t *pSrc, uint32_t srcLen,
               const float32_t *pH, uint32_t hLen, float32_t *pDst);

/* fills pH with an hLen point moving average */
void fir_moving_average(float32_t *pH, uint32_t hLen);

/* turns a low pass into the high pass that completes it */
void fir_invert(const float32_t *pH, uint32_t hLen, float32_t *pDst);

/* |H(f)|, what the kernel does to one frequency */
float32_t fir_gain(const float32_t *pH, uint32_t hLen,
                   float32_t f_hz, float32_t fs_hz);

float32_t fir_db(float32_t gain);

#endif /* INC_FILTERS_H_ */
