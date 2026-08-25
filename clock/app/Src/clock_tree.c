/*
 * The whole clock tree, read back out of the registers.
 *
 * Nothing here is remembered from the configuration function. Every number is
 * decoded from RCC and FLASH at the moment it is printed, which is the only way
 * to be sure the chip agrees with the intent. A constant in a header that says
 * 100000000 is a claim; this is a measurement of what was actually programmed.
 *
 * That is also why clock_get is written this way. The uart baud divider and the
 * SysTick reload both call it, so the moment the clock changes they follow, and
 * there is no list of places to remember to edit.
 *
 *     make clock_tree && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "driver_clock.h"
#include "driver_flash.h"
#include "driver_gpio.h"
#include "driver_systick.h"

static const char *source_name(void)
{
    switch (RCC->CFGR & CFGR_SWS_MSK)
    {
        case CFGR_SWS_HSI: return "HSI, the internal RC";
        case CFGR_SWS_HSE: return "HSE, the crystal";
        case CFGR_SWS_PLL: return "PLL";
        default:           return "reserved";
    }
}

static void print_line(const char *what, uint32_t hz, uint32_t limit)
{
    printf("%-16s %10lu Hz", what, (unsigned long)hz);

    if (limit != 0U)
    {
        printf("   limit %lu, %s", (unsigned long)limit,
               (hz <= limit) ? "ok" : "OVER");
    }

    printf("\r\n");
}

int main(void)
{
    config_app();
    probe_reset();

    uint32_t pll = RCC->PLLCFGR;
    uint32_t m   = (pll & PLLCFGR_M_MSK) >> PLLCFGR_M_POS;
    uint32_t n   = (pll & PLLCFGR_N_MSK) >> PLLCFGR_N_POS;
    uint32_t p   = ((((pll & PLLCFGR_P_MSK) >> PLLCFGR_P_POS) + 1U) * 2U);
    uint32_t in  = ((pll & PLLCFGR_SRC_HSE) != 0U) ? HSE_VALUE_HZ : HSI_VALUE_HZ;

    printf("\r\nsystem clock source: %s\r\n\r\n", source_name());

    printf("crystal on the board  %lu Hz, HSEBYP %lu (0 is a crystal, 1 is a"
           " clock signal)\r\n",
           (unsigned long)HSE_VALUE_HZ,
           (unsigned long)((RCC->CR & CR_HSEBYP) ? 1U : 0U));

    printf("\r\npll:  %lu / %lu = %lu Hz into the vco\r\n",
           (unsigned long)in, (unsigned long)m, (unsigned long)(in / m));
    printf("      times %lu = %lu Hz out of the vco\r\n",
           (unsigned long)n, (unsigned long)((in / m) * n));
    printf("      over %lu = %lu Hz\r\n\r\n",
           (unsigned long)p, (unsigned long)(((in / m) * n) / p));

    print_line("SYSCLK",      clock_get(),        SYSCLK_MAX_HZ);
    print_line("HCLK",        clock_hclk(),       SYSCLK_MAX_HZ);
    print_line("PCLK1",       clock_pclk1(),      PCLK1_MAX_HZ);
    print_line("PCLK2",       clock_pclk2(),      PCLK2_MAX_HZ);
    print_line("TIM on APB1", clock_timer_pclk1(), 0U);
    print_line("TIM on APB2", clock_timer_pclk2(), 0U);

    printf("\r\nregulator VOS   %lu   (3 is scale 1, the only one good for"
           " 100 MHz)\r\n",
           (unsigned long)((PWR->CR & PWR_CR_VOS_MSK) >> 14));
    printf("flash latency   %lu wait states\r\n",
           (unsigned long)flash_get_latency());
    printf("prefetch %lu  icache %lu  dcache %lu\r\n",
           (unsigned long)((FLASH->ACR & FLASH_ACR_PRFTEN) ? 1U : 0U),
           (unsigned long)((FLASH->ACR & FLASH_ACR_ICEN) ? 1U : 0U),
           (unsigned long)((FLASH->ACR & FLASH_ACR_DCEN) ? 1U : 0U));

    printf("\r\nthis text arriving unmangled is itself a check: the baud"
           " divider\r\n");
    printf("was computed from clock_pclk1(), so garbage here would mean the"
           " tree above is wrong\r\n");

    led_init(GPIOB, GPIO_PIN_NO_5);

    uint64_t last = ticks_get();

    while (1)
    {
        if ((ticks_get() - last) >= 500U)
        {
            GPIO_ToggleOutputPin(GPIOB, GPIO_PIN_NO_5);
            last = ticks_get();
        }

        g_mhz = (float32_t)clock_get() / 1.0e6f;
        probe_step();
    }
}