#include "driver_clock.h"
#include "driver_flash.h"

/*
 * AHB prescaler encoding. Anything under 8 means no division, and note that
 * there is no divide by 32: the table steps 16, 64.
 */
static const uint8_t ahb_shift[16] =
{
    0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,
    1U, 2U, 3U, 4U, 6U, 7U, 8U, 9U,
};

/* APB prescaler encoding, same idea, four entries of no division */
static const uint8_t apb_shift[8] =
{
    0U, 0U, 0U, 0U, 1U, 2U, 3U, 4U,
};

/*
 * Raises the core to 100 MHz off the external crystal.
 *
 * The order is not free. The regulator has to be told first, because 100 MHz is
 * outside what the reset default supports. The flash has to be told before the
 * clock goes up, because a memory that needs three wait states and is given
 * none returns nonsense. APB1 has to be divided before the switch, because it
 * tops out at 50 MHz. Only then is the PLL allowed to become the system clock.
 *
 * Coming back down runs in the opposite order, and clock_config_hsi does that.
 *
 * Returns 0 and leaves the chip on the internal oscillator if the crystal never
 * starts, which is what a missing or cracked crystal looks like. Waiting for it
 * forever would be the same fault with no way to see it.
 */
uint8_t clock_config_100mhz(void)
{
    uint32_t tries;

    /* the regulator, before anything asks for the frequency it allows */
    RCC->APB1ENR |= APB1ENR_PWREN;
    PWR->CR = (PWR->CR & ~PWR_CR_VOS_MSK) | PWR_CR_VOS_SCALE1;

    /* the crystal */
    RCC->CR &= ~CR_HSEBYP;              /* a crystal, not a clock signal */
    RCC->CR |= CR_HSEON;

    for (tries = CLOCK_STARTUP_TRIES; tries > 0U; tries--)
    {
        if ((RCC->CR & CR_HSERDY) != 0U)
        {
            break;
        }
    }

    if (tries == 0U)
    {
        RCC->CR &= ~CR_HSEON;
        return 0U;
    }

    /* the memory, before the clock it has to keep up with */
    flash_set_latency(FLASH_LATENCY_100MHZ);
    flash_art_config(ENABLE, ENABLE, ENABLE);

    /* the buses, before the frequency that would take APB1 over its limit */
    RCC->CFGR = (RCC->CFGR & ~(CFGR_HPRE_MSK | CFGR_PPRE1_MSK | CFGR_PPRE2_MSK))
                | CFGR_HPRE_DIV1 | CFGR_PPRE1_DIV2 | CFGR_PPRE2_DIV1;

    /* the pll itself */
    RCC->PLLCFGR = ((PLL_M << PLLCFGR_M_POS) & PLLCFGR_M_MSK)
                 | ((PLL_N << PLLCFGR_N_POS) & PLLCFGR_N_MSK)
                 | ((((PLL_P / 2U) - 1U) << PLLCFGR_P_POS) & PLLCFGR_P_MSK)
                 | ((PLL_Q << PLLCFGR_Q_POS) & PLLCFGR_Q_MSK)
                 | PLLCFGR_SRC_HSE;

    RCC->CR |= CR_PLLON;

    for (tries = CLOCK_STARTUP_TRIES; tries > 0U; tries--)
    {
        if ((RCC->CR & CR_PLLRDY) != 0U)
        {
            break;
        }
    }

    if (tries == 0U)
    {
        RCC->CR &= ~(CR_PLLON | CR_HSEON);
        return 0U;
    }

    /* and only now the switch */
    RCC->CFGR = (RCC->CFGR & ~CFGR_SW_MSK) | CFGR_SW_PLL;

    while ((RCC->CFGR & CFGR_SWS_MSK) != CFGR_SWS_PLL)
    {
    }

    return 1U;
}

/* back to the internal oscillator, unwinding in the opposite order */
void clock_config_hsi(void)
{
    RCC->CR |= CR_HSION;

    while ((RCC->CR & CR_HSIRDY) == 0U)
    {
    }

    RCC->CFGR = (RCC->CFGR & ~CFGR_SW_MSK) | CFGR_SW_HSI;

    while ((RCC->CFGR & CFGR_SWS_MSK) != CFGR_SWS_HSI)
    {
    }

    RCC->CR &= ~(CR_PLLON | CR_HSEON);

    /* the frequency is down, so the wait states can come down after it */
    RCC->CFGR &= ~(CFGR_HPRE_MSK | CFGR_PPRE1_MSK | CFGR_PPRE2_MSK);
    flash_set_latency(FLASH_LATENCY_16MHZ);
}

/*
 * What the system clock actually is, worked out from the registers.
 *
 * This is the whole reason the uart baud rate and the systick reload keep
 * working after the clock changes: they both ask this function, and this
 * function cannot be out of date because it has nothing to remember.
 */
uint32_t clock_get(void)
{
    uint32_t cfgr = RCC->CFGR & CFGR_SWS_MSK;

    if (cfgr == CFGR_SWS_HSE)
    {
        return HSE_VALUE_HZ;
    }

    if (cfgr == CFGR_SWS_PLL)
    {
        uint32_t pll = RCC->PLLCFGR;
        uint32_t m   = (pll & PLLCFGR_M_MSK) >> PLLCFGR_M_POS;
        uint32_t n   = (pll & PLLCFGR_N_MSK) >> PLLCFGR_N_POS;
        uint32_t p   = ((((pll & PLLCFGR_P_MSK) >> PLLCFGR_P_POS) + 1U) * 2U);
        uint32_t in  = ((pll & PLLCFGR_SRC_HSE) != 0U) ? HSE_VALUE_HZ
                                                       : HSI_VALUE_HZ;

        /* 64 bit on the way through, so an m that does not divide cleanly
           still lands on the right number */
        return (uint32_t)(((uint64_t)in * (uint64_t)n) / ((uint64_t)m * p));
    }

    return HSI_VALUE_HZ;
}

uint32_t clock_hclk(void)
{
    return clock_get() >> ahb_shift[(RCC->CFGR & CFGR_HPRE_MSK) >> 4];
}

uint32_t clock_pclk1(void)
{
    return clock_hclk() >> apb_shift[(RCC->CFGR & CFGR_PPRE1_MSK) >> 10];
}

uint32_t clock_pclk2(void)
{
    return clock_hclk() >> apb_shift[(RCC->CFGR & CFGR_PPRE2_MSK) >> 13];
}

/*
 * Timers on an APB bus do not run at that bus's clock unless the bus is
 * undivided. The moment the prescaler is anything but 1, the timer clock is
 * doubled back up. It is a real multiplier in the clock tree and not a
 * rounding: it exists so that dividing the bus for the peripherals does not
 * also halve the resolution of the timers.
 */
uint32_t clock_timer_pclk1(void)
{
    uint32_t pre = (RCC->CFGR & CFGR_PPRE1_MSK) >> 10;

    return (apb_shift[pre] == 0U) ? clock_pclk1() : (clock_pclk1() * 2U);
}

uint32_t clock_timer_pclk2(void)
{
    uint32_t pre = (RCC->CFGR & CFGR_PPRE2_MSK) >> 13;

    return (apb_shift[pre] == 0U) ? clock_pclk2() : (clock_pclk2() * 2U);
}