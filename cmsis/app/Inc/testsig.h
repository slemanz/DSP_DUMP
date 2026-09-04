#ifndef INC_TESTSIG_H_
#define INC_TESTSIG_H_

#include "arm_math.h"

#define SIG_LEN     256
#define TAPS        32

extern const float32_t sig[SIG_LEN];
extern const float32_t taps[TAPS];

#endif /* INC_TESTSIG_H_ */
