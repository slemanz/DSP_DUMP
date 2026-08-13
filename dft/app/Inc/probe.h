#ifndef INC_PROBE_H_
#define INC_PROBE_H_

#include "arm_math.h"

#define STEP_MS         100U

#define ARRAY_LEN(a)    (sizeof(a) / sizeof((a)[0]))

extern volatile float32_t g_x;
extern volatile float32_t g_re;
extern volatile float32_t g_im;
extern volatile float32_t g_mag;
extern volatile float32_t g_rebuilt;

void probe_reset(void);
void probe_step(void);

#endif /* INC_PROBE_H_ */