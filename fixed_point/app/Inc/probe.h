#ifndef INC_PROBE_H_
#define INC_PROBE_H_

#include "arm_math.h"

#define STEP_MS         100U

#define ARRAY_LEN(a)    (sizeof(a) / sizeof((a)[0]))

/* the three formats and what separates them */
#define Q7_ONE          128.0f
#define Q15_ONE         32768.0f
#define Q31_ONE         2147483648.0f

extern volatile float32_t g_f32;
extern volatile float32_t g_q31;
extern volatile float32_t g_q15;
extern volatile float32_t g_err;

void probe_reset(void);
void probe_step(void);

#endif /* INC_PROBE_H_ */
