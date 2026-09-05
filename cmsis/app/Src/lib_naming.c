/*
 * The conventions, which are the whole library once you have them.
 *
 * CMSIS-DSP publishes several hundred functions and they are not several
 * hundred things to learn. They are a handful of rules applied consistently,
 * and once the rules are in place the reference manual becomes an index rather
 * than a course.
 *
 *     arm_<operation>_<type>
 *
 * The type suffix is the number format from the fixed point chapter: f64, f32,
 * f16, q31, q15, q7. The same operation exists in most of them and behaves the
 * same way, which means picking a format is a decision about the whole program
 * rather than about each call.
 *
 *     arm_add_f32(pSrcA, pSrcB, pDst, blockSize)
 *
 * Sources first, destination next, length last. Every stateless routine in the
 * library takes its arguments in that order, and the ones that break it, like
 * arm_dot_prod_f32, break it for a reason: their output is one number rather
 * than a block, so it moves to the end where a pointer to a scalar goes.
 *
 * Most routines return void. The ones that return arm_status are the ones that
 * can be given an impossible request, which is almost always an init.
 *
 *     make lib_naming && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "testsig.h"

#define N       8U

static float32_t f_a[N];
static float32_t f_b[N];
static float32_t f_out[N];
static q31_t     q31_a[N];
static q31_t     q31_b[N];
static q31_t     q31_out[N];
static q15_t     q15_a[N];
static q15_t     q15_b[N];
static q15_t     q15_out[N];

int main(void)
{
    config_app();
    probe_reset();

    for (uint32_t i = 0U; i < N; i++)
    {
        f_a[i] = 0.1f * (float32_t)(i + 1U);
        f_b[i] = 0.05f * (float32_t)(i + 1U);
    }

    arm_float_to_q31(f_a, q31_a, N);
    arm_float_to_q31(f_b, q31_b, N);
    arm_float_to_q15(f_a, q15_a, N);
    arm_float_to_q15(f_b, q15_b, N);

    arm_add_f32(f_a, f_b, f_out, N);
    arm_add_q31(q31_a, q31_b, q31_out, N);
    arm_add_q15(q15_a, q15_b, q15_out, N);

    printf("\r\none operation, three formats, identical shape of call\r\n\r\n");
    printf("  arm_add_f32(pSrcA, pSrcB, pDst, blockSize)\r\n");
    printf("  arm_add_q31(pSrcA, pSrcB, pDst, blockSize)\r\n");
    printf("  arm_add_q15(pSrcA, pSrcB, pDst, blockSize)\r\n\r\n");

    printf("%6s %10s %10s %14s %14s %14s\r\n",
           "i", "a", "b", "f32", "q31", "q15");

    for (uint32_t i = 0U; i < N; i++)
    {
        printf("%6lu %10.4f %10.4f %14.6f %14.6f %14.6f\r\n",
               (unsigned long)i, (double)f_a[i], (double)f_b[i],
               (double)f_out[i],
               (double)((float32_t)q31_out[i] / 2147483648.0f),
               (double)((float32_t)q15_out[i] / 32768.0f));
    }

    printf("\r\nthe last two rows: f32 passes 1.0, the other two stop there,"
           " because\r\nthat is where their format ends and the library"
           " saturates.\r\n");

    printf("\r\nthe argument order, and the two shapes it comes in\r\n\r\n");
    printf("  block out    arm_add_f32(pSrcA, pSrcB, pDst, blockSize)\r\n");
    printf("  scalar out   arm_dot_prod_f32(pSrcA, pSrcB, blockSize,"
           " pResult)\r\n");
    printf("\r\nnot an inconsistency: the length comes before the destination"
           " when the\r\ndestination is one number, because there is no block"
           " to describe.\r\n");

    float32_t dot;
    arm_dot_prod_f32(f_a, f_b, N, &dot);

    float32_t manual = 0.0f;

    for (uint32_t i = 0U; i < N; i++)
    {
        manual += f_a[i] * f_b[i];
    }

    printf("\r\n  arm_dot_prod_f32  %.6f\r\n", (double)dot);
    printf("  by hand           %.6f\r\n", (double)manual);

    printf("\r\nand the return type says whether a call can fail\r\n\r\n");
    printf("  void         it cannot be given a request it must refuse\r\n");
    printf("  arm_status   it can, and almost every one of these is an"
           " init\r\n");

    arm_rfft_fast_instance_f32 fft;

    printf("\r\n  arm_rfft_fast_init_f32(&fft, 32)  status %d\r\n",
           (int)arm_rfft_fast_init_f32(&fft, 32U));
    printf("  arm_rfft_fast_init_f32(&fft,  8)  status %d\r\n",
           (int)arm_rfft_fast_init_f32(&fft, 8U));
    printf("\r\n8 is refused because 32 is the shortest transform the library"
           " carries\r\ntables for. an init whose status is ignored is a"
           " transform that silently\r\ndoes nothing.\r\n");

    while (1)
    {
        for (uint32_t i = 0U; i < N; i++)
        {
            g_in  = f_a[i];
            g_out = f_out[i];
            g_ref = f_a[i] + f_b[i];
            g_gap = f_out[i] - (f_a[i] + f_b[i]);
            probe_step();
        }
    }
}