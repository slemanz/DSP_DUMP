/*
 * Picking M, N and P, and why most of the choices are illegal.
 *
 * The pll divides the input by M, multiplies by N, and divides by P. Three
 * numbers, one answer, and it looks like there is a lot of freedom. There is
 * not: the divided input has to land between 1 and 2 MHz, and the multiplied
 * result has to land between 100 and 432 MHz, so most triples that reach the
 * right frequency are not allowed to.
 *
 * The table below runs candidates for the 25 MHz crystal on this board through
 * both windows and says which ones survive. The one this module ships with is
 * marked, and so is the one the course uses, which was written for an 8 MHz
 * board and does not survive here.
 *
 * There is no arithmetic on the chip that needs doing. It is a table, and it is
 * the answer to why M is 25 and not something friendlier.
 *
 *     make clock_pll && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "driver_clock.h"

typedef struct
{
    uint32_t    in_hz;
    uint32_t    m;
    uint32_t    n;
    uint32_t    p;
    const char *note;
} candidate_t;

static const candidate_t tries[] =
{
    { 25000000U, 25U, 200U, 2U, "this board" },
    { 25000000U, 13U, 104U, 2U, "same answer, vco nearer 2 MHz" },
    { 25000000U, 25U, 192U, 2U, "96 MHz, the one that also gives usb 48" },
    { 25000000U, 12U,  96U, 2U, "vco input 2.083 MHz, over the window" },
    { 25000000U, 50U, 400U, 2U, "vco input 0.5 MHz, under the window" },
    { 25000000U,  4U, 200U, 4U, "the course values, on a 25 MHz crystal" },
    {  8000000U,  4U, 200U, 4U, "the course values, on the 8 MHz they assume" },
};

int main(void)
{
    config_app();
    probe_reset();

    printf("\r\nvco input must land in %lu..%lu Hz\r\n",
           (unsigned long)VCO_IN_MIN_HZ, (unsigned long)VCO_IN_MAX_HZ);
    printf("vco output must land in %lu..%lu Hz\r\n",
           (unsigned long)VCO_OUT_MIN_HZ, (unsigned long)VCO_OUT_MAX_HZ);
    printf("and the result must not pass %lu Hz\r\n\r\n",
           (unsigned long)SYSCLK_MAX_HZ);

    printf("%9s %4s %5s %3s %10s %11s %10s %-6s %s\r\n",
           "in", "M", "N", "P", "vco in", "vco out", "sysclk", "", "note");

    for (uint32_t k = 0U; k < ARRAY_LEN(tries); k++)
    {
        const candidate_t *c = &tries[k];

        uint32_t vco_in  = c->in_hz / c->m;
        uint32_t vco_out = (uint32_t)(((uint64_t)c->in_hz * c->n) / c->m);
        uint32_t sysclk  = vco_out / c->p;

        uint8_t ok = (vco_in  >= VCO_IN_MIN_HZ)  && (vco_in  <= VCO_IN_MAX_HZ)
                  && (vco_out >= VCO_OUT_MIN_HZ) && (vco_out <= VCO_OUT_MAX_HZ)
                  && (sysclk  <= SYSCLK_MAX_HZ);

        printf("%9lu %4lu %5lu %3lu %10lu %11lu %10lu %-6s %s\r\n",
               (unsigned long)c->in_hz, (unsigned long)c->m,
               (unsigned long)c->n, (unsigned long)c->p,
               (unsigned long)vco_in, (unsigned long)vco_out,
               (unsigned long)sysclk, ok ? "ok" : "NO", c->note);
    }

    printf("\r\nthe last two rows are the same three numbers on two different"
           " boards.\r\n");
    printf("a clock configuration is not portable, it is a property of the"
           " crystal.\r\n");

    printf("\r\nusb wants exactly 48 MHz from vco_out / Q, and Q is a whole"
           " number,\r\n");
    printf("so 200 MHz cannot feed it and 192 can, at Q = 4. that is the whole"
           "\r\nreason so much blackpill code runs at 96 rather than 100.\r\n");

    while (1)
    {
        for (uint32_t k = 0U; k < ARRAY_LEN(tries); k++)
        {
            const candidate_t *c = &tries[k];

            g_mhz = (float32_t)((uint32_t)(((uint64_t)c->in_hz * c->n)
                                           / c->m) / c->p) / 1.0e6f;
            probe_step();
        }
    }
}