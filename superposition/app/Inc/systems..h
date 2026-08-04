#ifndef INC_SYSTEMS_H_
#define INC_SYSTEMS_H_

#include "arm_math.h"

#define CLIP_LIMIT      0.6f
#define CARRIER_LEN     32U

typedef void (*system_fn)(const float32_t *pSrc, float32_t *pDst, uint32_t len);

void system_fir(const float32_t *pSrc, float32_t *pDst, uint32_t len);
void system_clip(const float32_t *pSrc, float32_t *pDst, uint32_t len);
void system_square(const float32_t *pSrc, float32_t *pDst, uint32_t len);
void system_modulate(const float32_t *pSrc, float32_t *pDst, uint32_t len);

#endif /* INC_SYSTEMS_H_ */