#ifndef INC_PROBE_H_
#define INC_PROBE_H_

#include "arm_math.h"

#define STEP_MS         2U
#define MODEL_HZ        500U
#define TWO_PI          6.28318531f

#define ARRAY_LEN(a)    (sizeof(a) / sizeof((a)[0]))

extern volatile float32_t g_input;
extern volatile float32_t g_path_a;
extern volatile float32_t g_path_b;
extern volatile float32_t g_error;

void probe_step(void);

#endif /* INC_PROBE_H_ */