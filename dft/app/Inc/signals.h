#ifndef INC_SIGNALS_H_
#define INC_SIGNALS_H_

#include <stdint.h>
#include "arm_math.h"

#define HZ_5_SIG_LEN		301
#define KHZ1_15_SIG_LEN		320

#define SAMPLE_RATE_HZ		48000U

extern float32_t _5hz_signal[HZ_5_SIG_LEN];
extern float32_t input_signal_f32_1kHz_15kHz[KHZ1_15_SIG_LEN];

#endif /* INC_SIGNALS_H_ */