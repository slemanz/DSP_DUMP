/*
 * The stateless half of the library, and the question nobody documents.
 *
 * Most of what is in CMSIS-DSP is a loop over an array with no memory between
 * calls: add these two, scale that one, take the largest. They need no
 * instance, no init and no state buffer, and they are the ones worth reaching
 * for first because there is nothing to get wrong except the arguments.
 *
 * The question that comes up immediately and is not in the reference manual is
 * whether pDst may be the same pointer as pSrc. Working in place halves the
 * memory a chain of operations needs, which on this part is the difference
 * between a block fitting in RAM and not.
 *
 * The answer is that the elementwise routines do allow it, because they touch
 * element n of the output only after reading element n of the input, and
 * nothing else. The routines whose output depends on more than one input
 * element, the filters and the transforms, do not, and the ones that need
 * scratch space say so by asking for it.
 *
 * That reasoning is more useful than the answer, because it tells you which
 * side of the line a routine you have not met yet will fall on. The app tests
 * it rather than trusting it.
 *
 *     make lib_blocks && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "testsig.h"

#define N       16U

static float32_t a[N];
static float32_t b[N];
static float32_t expected[N];
static float32_t work[N];

static uint8_t matches(const float32_t *pA, const float32_t *pB, uint32_t len)
{
    for (uint32_t i = 0U; i < len; i++)
    {
        if (fabsf(pA[i] - pB[i]) > 1.0e-6f)
        {
            return 0U;
        }
    }

    return 1U;
}

static void reset_work(void)
{
    arm_copy_f32(a, work, N);
}

int main(void)
{
    config_app();
    probe_reset();

    for (uint32_t i = 0U; i < N; i++)
    {
        a[i] = 0.1f * (float32_t)(i + 1U) - 0.8f;
        b[i] = 0.05f * (float32_t)(N - i);
    }

    printf("\r\nthe elementwise group, and whether each works in place\r\n\r\n");
    printf("%-26s %28s %8s\r\n", "call", "what it does", "in place");

    arm_add_f32(a, b, expected, N);
    reset_work();
    arm_add_f32(work, b, work, N);
    printf("%-26s %28s %8s\r\n", "arm_add_f32", "dst[i] = a[i] + b[i]",
           matches(work, expected, N) ? "yes" : "NO");

    arm_sub_f32(a, b, expected, N);
    reset_work();
    arm_sub_f32(work, b, work, N);
    printf("%-26s %28s %8s\r\n", "arm_sub_f32", "dst[i] = a[i] - b[i]",
           matches(work, expected, N) ? "yes" : "NO");

    arm_mult_f32(a, b, expected, N);
    reset_work();
    arm_mult_f32(work, b, work, N);
    printf("%-26s %28s %8s\r\n", "arm_mult_f32", "dst[i] = a[i] * b[i]",
           matches(work, expected, N) ? "yes" : "NO");

    arm_scale_f32(a, 2.5f, expected, N);
    reset_work();
    arm_scale_f32(work, 2.5f, work, N);
    printf("%-26s %28s %8s\r\n", "arm_scale_f32", "dst[i] = a[i] * k",
           matches(work, expected, N) ? "yes" : "NO");

    arm_offset_f32(a, 1.0f, expected, N);
    reset_work();
    arm_offset_f32(work, 1.0f, work, N);
    printf("%-26s %28s %8s\r\n", "arm_offset_f32", "dst[i] = a[i] + k",
           matches(work, expected, N) ? "yes" : "NO");

    arm_negate_f32(a, expected, N);
    reset_work();
    arm_negate_f32(work, work, N);
    printf("%-26s %28s %8s\r\n", "arm_negate_f32", "dst[i] = -a[i]",
           matches(work, expected, N) ? "yes" : "NO");

    arm_abs_f32(a, expected, N);
    reset_work();
    arm_abs_f32(work, work, N);
    printf("%-26s %28s %8s\r\n", "arm_abs_f32", "dst[i] = |a[i]|",
           matches(work, expected, N) ? "yes" : "NO");

    arm_clip_f32(a, expected, -0.3f, 0.3f, N);
    reset_work();
    arm_clip_f32(work, work, -0.3f, 0.3f, N);
    printf("%-26s %28s %8s\r\n", "arm_clip_f32", "dst[i] held in a range",
           matches(work, expected, N) ? "yes" : "NO");

    printf("\r\nthe rule: a routine can work in place when output n depends"
           " only on\r\ninput n. a filter cannot; a transform cannot"
           " either.\r\n");

    float32_t dot;
    arm_dot_prod_f32(a, b, N, &dot);

    printf("\r\nand the odd one out, because its result is not a block:\r\n");
    printf("  arm_dot_prod_f32(a, b, %lu, &result)  ->  %.6f\r\n",
           (unsigned long)N, (double)dot);
    printf("  that single number is a filter tap set against a window, which"
           "\r\n  is every FIR in the repository written as one call.\r\n");

    while (1)
    {
        for (uint32_t i = 0U; i < N; i++)
        {
            g_in  = a[i];
            g_out = work[i];
            g_ref = expected[i];
            g_gap = work[i] - expected[i];
            probe_step();
        }
    }
}