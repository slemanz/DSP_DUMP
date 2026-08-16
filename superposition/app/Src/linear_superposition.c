/*
 * Superposition, which is the reason the other three properties are worth
 * having: a signal can be taken apart, the pieces sent through the system one
 * at a time, and the results added back to give what the whole signal would
 * have given.
 *
 * The part that is easy to miss is that the pieces are yours to choose. This
 * app splits the same signal three completely different ways, and all three
 * come back to the same output. Cutting it in half works. Taking every other
 * sample works. Twenty five pieces of one sample each works.
 *
 * That last one is impulse decomposition, and nothing above says it is special.
 * It is special for a different reason, which is what the next two apps are
 * about.
 *
 *     make linear_superposition && make load && make debug
 */
#include <stdio.h>
#include <math.h>
#include "config.h"
#include "probe.h"
#include "systems.h"

#define SIG_LEN         25U
#define HALVES          2U
#define STRIDES         2U
#define IMPULSES        SIG_LEN

static float32_t x[SIG_LEN];
static float32_t whole[SIG_LEN];
static float32_t piece[SIG_LEN];
static float32_t piece_out[SIG_LEN];
static float32_t rebuilt[SIG_LEN];
static float32_t gap[SIG_LEN];

// piece p of a split into `pieces`, written into the piece buffer. Every sample
// of the signal belongs to exactly one piece and the rest of the buffer is
// zero, so the pieces always add back up to the signal.
static void take_piece(uint32_t split, uint32_t p, uint32_t pieces)
{
    for (uint32_t n = 0; n < SIG_LEN; n++)
    {
        uint32_t owner;

        switch (split)
        {
            case 0U:  owner = (n < (SIG_LEN / 2U)) ? 0U : 1U; break;   // two halves
            case 1U:  owner = n % pieces;                     break;   // every other sample
            default:  owner = n;                              break;   // one each
        }

        piece[n] = (owner == p) ? x[n] : 0.0f;
    }
}

// runs every piece of a split through the system on its own and adds the
// results up, which is the long way round to the same place
static float32_t through_the_pieces(uint32_t split, uint32_t pieces)
{
    float32_t worst;
    uint32_t index;

    arm_fill_f32(0.0f, rebuilt, SIG_LEN);

    for (uint32_t p = 0; p < pieces; p++)
    {
        take_piece(split, p, pieces);
        system_fir(piece, piece_out, SIG_LEN);
        arm_add_f32(rebuilt, piece_out, rebuilt, SIG_LEN);
    }

    arm_sub_f32(rebuilt, whole, gap, SIG_LEN);
    arm_absmax_f32(gap, SIG_LEN, &worst, &index);

    return worst;
}

int main(void)
{
    static const struct
    {
        const char *name;
        uint32_t    pieces;
    } splits[] = {
        { "cut in half",           HALVES   },
        { "every other sample",    STRIDES  },
        { "one sample per piece",  IMPULSES },
    };

    config_app();
    probe_reset();

    for (uint32_t n = 0; n < SIG_LEN; n++)
    {
        x[n] = 0.8f * sinf(TWO_PI * (float32_t)n / 16.0f)
             * (1.0f - ((float32_t)n / 40.0f));
    }

    system_fir(x, whole, SIG_LEN);

    printf("\r\none %lu sample signal, one system, three ways to take it apart\r\n\r\n",
           (unsigned long)SIG_LEN);
    printf("%22s %8s %12s\r\n", "split", "pieces", "worst gap");

    for (uint32_t s = 0; s < ARRAY_LEN(splits); s++)
    {
        printf("%22s %8lu %12.9f\r\n", splits[s].name,
               (unsigned long)splits[s].pieces,
               (double)through_the_pieces(s, splits[s].pieces));
    }

    /* The gaps are float rounding and nothing else. Every split gives the
     * signal back, so the choice of pieces is free, and impulses are a choice
     * rather than a requirement. */
    printf("\r\nall three land on the same output, so the pieces are yours to pick\r\n");

    while (1)
    {
        for (uint32_t s = 0; s < ARRAY_LEN(splits); s++)
        {
            arm_fill_f32(0.0f, rebuilt, SIG_LEN);

            for (uint32_t p = 0; p < splits[s].pieces; p++)
            {
                take_piece(s, p, splits[s].pieces);
                system_fir(piece, piece_out, SIG_LEN);
                arm_add_f32(rebuilt, piece_out, rebuilt, SIG_LEN);
                arm_sub_f32(rebuilt, whole, gap, SIG_LEN);

                for (uint32_t n = 0; n < SIG_LEN; n++)
                {
                    g_x      = piece[n];
                    g_before = whole[n];
                    g_after  = rebuilt[n];
                    g_gap    = gap[n];

                    probe_step();
                }
            }
        }
    }
}