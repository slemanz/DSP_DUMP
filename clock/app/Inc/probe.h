#ifndef INC_PROBE_H_
#define INC_PROBE_H_

#include "arm_math.h"

#define STEP_MS         100U

#define ARRAY_LEN(a)    (sizeof(a) / sizeof((a)[0]))

extern volatile float32_t g_mhz;
extern volatile float32_t g_cycles;
extern volatile float32_t g_ms;
extern volatile float32_t g_gain;

void probe_reset(void);
void probe_step(void);

#endif /* INC_PROBE_H_ */