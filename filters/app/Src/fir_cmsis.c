/*
 * arm_fir_f32, and the two things about it that are not obvious.
 *
 * The first is the coefficient order. arm_fir_f32 wants the taps time
 * reversed, h[N-1] first and h[0] last. Every kernel in this module is
 * symmetric, so reversing it changes nothing and the mistake never shows. The
 * check below uses a deliberately lopsided kernel, where it shows immediately.
 *
 * The second is the output length. fir_apply and arm_conv_f32 return
 * srcLen + hLen - 1 points, because the last input sample still drops a whole
 * copy of the kernel into the output. arm_fir_f32 returns exactly blockSize
 * points and keeps the overhang in a state buffer, ready to be prepended to
 * whatever arrives next. That is the entire reason it takes an instance rather
 * than just arrays, and it is what lets a filter run forever on a signal that
 * never ends.
 *
 * The last table is that claim tested: the same signal in one call, and then in
 * two halves, giving the same answer to the bit.
 *
 *     make fir_cmsis && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "filters.h"
#include "kernels.h"
#include "testsig.h"

#define TAPS        LP_50_LEN
#define BLOCK       SIG_LEN
#define HALF        (SIG_LEN / 2U)
#define OUT_LEN     (SIG_LEN + TAPS - 1U)
#define LOPSIDED    3U

static float32_t reversed[TAPS];
static float32_t state[TAPS + BLOCK - 1U];
static float32_t whole[BLOCK];
static float32_t halves[BLOCK];
static float32_t direct[OUT_LEN];

int main(void)
{
    config_app();
    probe_reset();

    arm_fir_instance_f32 fir;
    uint32_t n;

    /* an impulse through a kernel that is not symmetric, so order matters */
    float32_t lopsided[LOPSIDED] = { 0.5f, 0.3f, 0.2f };
    float32_t impulse[8] = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    float32_t flipped[LOPSIDED];
    float32_t small_state[LOPSIDED + 8U - 1U];
    float32_t got[8];

    printf("\r\nkernel {0.50, 0.30, 0.20} hit with a single impulse\r\n\r\n");

    arm_fir_init_f32(&fir, LOPSIDED, lopsided, small_state, 8U);
    arm_fir_f32(&fir, impulse, got, 8U);
    printf("  taps as written    %.2f %.2f %.2f\r\n",
           (double)got[0], (double)got[1], (double)got[2]);

    for (n = 0U; n < LOPSIDED; n++)
    {
        flipped[n] = lopsided[LOPSIDED - 1U - n];
    }

    arm_fir_init_f32(&fir, LOPSIDED, flipped, small_state, 8U);
    arm_fir_f32(&fir, impulse, got, 8U);
    printf("  taps time reversed %.2f %.2f %.2f\r\n",
           (double)got[0], (double)got[1], (double)got[2]);
    printf("\r\nonly the second one is the kernel. a symmetric kernel hides"
           " this completely.\r\n");

    /* now the real one, reversed as required */
    for (n = 0U; n < TAPS; n++)
    {
        reversed[n] = lp_50[TAPS - 1U - n];
    }

    arm_fir_init_f32(&fir, TAPS, reversed, state, BLOCK);
    arm_fir_f32(&fir, sig_3tone, whole, BLOCK);

    fir_apply(sig_3tone, SIG_LEN, lp_50, TAPS, direct);

    float32_t gap_direct = 0.0f;

    for (n = 0U; n < BLOCK; n++)
    {
        float32_t gap = fabsf(whole[n] - direct[n]);

        if (gap > gap_direct)
        {
            gap_direct = gap;
        }
    }

    /* the same signal, handed over in two pieces */
    arm_fir_init_f32(&fir, TAPS, reversed, state, HALF);
    arm_fir_f32(&fir, &sig_3tone[0],    &halves[0],    HALF);
    arm_fir_f32(&fir, &sig_3tone[HALF], &halves[HALF], HALF);

    float32_t gap_split = 0.0f;

    for (n = 0U; n < BLOCK; n++)
    {
        float32_t gap = fabsf(halves[n] - whole[n]);

        if (gap > gap_split)
        {
            gap_split = gap;
        }
    }

    printf("\r\n%-34s %8s\r\n", "", "points");
    printf("%-34s %8u\r\n", "fir_apply and arm_conv_f32", (unsigned)OUT_LEN);
    printf("%-34s %8u\r\n", "arm_fir_f32", (unsigned)BLOCK);
    printf("%-34s %8u\r\n", "the difference, held in state",
           (unsigned)(TAPS - 1U));

    printf("\r\narm_fir_f32 against fir_apply over %u points: %.9f\r\n",
           (unsigned)BLOCK, (double)gap_direct);
    printf("one call of %u against two calls of %u:      %.9f\r\n",
           (unsigned)BLOCK, (unsigned)HALF, (double)gap_split);
    printf("\r\nthe state buffer is what makes the second line zero\r\n");

    while (1)
    {
        for (n = 0U; n < BLOCK; n++)
        {
            g_x   = sig_3tone[n];
            g_y   = whole[n];
            g_ref = direct[n];
            g_mag = whole[n] - halves[n];
            g_h   = (n < TAPS) ? reversed[n] : 0.0f;
            probe_step();
        }
    }
}