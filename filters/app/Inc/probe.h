#ifndef INC_PROBE_H_
#define INC_PROBE_H_

#include "arm_math.h"

#define STEP_MS         100U

extern volatile float32_t g_x;
extern volatile float32_t g_y;
extern volatile float32_t g_h;
extern volatile float32_t g_mag;
extern volatile float32_t g_ref;

void probe_reset(void);
void probe_step(void);

#endif /* INC_PROBE_H_ */