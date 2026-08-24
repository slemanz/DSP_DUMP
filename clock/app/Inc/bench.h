#ifndef INC_BENCH_H_
#define INC_BENCH_H_

#include "arm_math.h"

/*
 * One fixed piece of arithmetic, reused by every app that measures something.
 * 256 samples through 64 taps is 16384 multiply accumulates, which is small
 * enough to fit the cycle counter at 100 MHz and big enough that the loop, not
 * the call around it, is what gets measured.
 */
#define BENCH_LEN       256U
#define BENCH_TAPS      64U
#define BENCH_MACS      (BENCH_LEN * BENCH_TAPS)

void     bench_prepare(void);
uint32_t bench_cycles(void);
float32_t bench_checksum(void);

#endif /* INC_BENCH_H_ */