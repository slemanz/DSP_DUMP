#include "driver_clock.h"
#include "stm32f411xx.h"

#define CFGR_HPRE_MSK           (0xFU << 4)

/* AHB prescaler encoding. Anything under 8 means no division, and note that
   there is no divide by 32: the table steps 16, 64. */
static const uint8_t ahb_shift[16] =
{
    0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,
    1U, 2U, 3U, 4U, 6U, 7U, 8U, 9U,
};

uint32_t clock_get(void)
{
    return 16000000;
}

uint32_t clock_hclk(void)
{
    return clock_get() >> ahb_shift[(RCC->CFGR & CFGR_HPRE_MSK) >> 4];
}