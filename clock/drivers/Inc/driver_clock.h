#ifndef INC_DRIVER_CLOCK_H_
#define INC_DRIVER_CLOCK_H_

#include "stm32f411xx.h"

/*
 * The crystal soldered on the weact blackpill. It is a real crystal, so the
 * oscillator drives it and RCC_CR_HSEBYP stays clear. A board that is fed a
 * ready made square wave instead, such as a nucleo taking 8 MHz from the
 * st-link MCO pin, has to set HSEBYP, and its number here would be 8000000.
 */
#define HSE_VALUE_HZ            25000000U
#define HSI_VALUE_HZ            16000000U

/* 25 / 25 = 1 MHz into the vco, times 200 = 200 MHz, over 2 = 100 MHz out */
#define PLL_M                   25U
#define PLL_N                   200U
#define PLL_P                   2U
#define PLL_Q                   8U

#define SYSCLK_MAX_HZ           100000000U
#define PCLK1_MAX_HZ            50000000U
#define PCLK2_MAX_HZ            100000000U

/* the vco has to be fed between these, and has to land between these */
#define VCO_IN_MIN_HZ           1000000U
#define VCO_IN_MAX_HZ           2000000U
#define VCO_OUT_MIN_HZ          100000000U
#define VCO_OUT_MAX_HZ          432000000U

/* RCC_CR */
#define CR_HSION                (1U << 0)
#define CR_HSIRDY               (1U << 1)
#define CR_HSEON                (1U << 16)
#define CR_HSERDY               (1U << 17)
#define CR_HSEBYP               (1U << 18)
#define CR_PLLON                (1U << 24)
#define CR_PLLRDY               (1U << 25)

/* RCC_CFGR */
#define CFGR_SW_MSK             (0x3U << 0)
#define CFGR_SW_HSI             (0x0U << 0)
#define CFGR_SW_HSE             (0x1U << 0)
#define CFGR_SW_PLL             (0x2U << 0)
#define CFGR_SWS_MSK            (0x3U << 2)
#define CFGR_SWS_HSI            (0x0U << 2)
#define CFGR_SWS_HSE            (0x1U << 2)
#define CFGR_SWS_PLL            (0x2U << 2)
#define CFGR_HPRE_MSK           (0xFU << 4)
#define CFGR_HPRE_DIV1          (0x0U << 4)
#define CFGR_PPRE1_MSK          (0x7U << 10)
#define CFGR_PPRE1_DIV1         (0x0U << 10)
#define CFGR_PPRE1_DIV2         (0x4U << 10)
#define CFGR_PPRE2_MSK          (0x7U << 13)
#define CFGR_PPRE2_DIV1         (0x0U << 13)

/* RCC_PLLCFGR */
#define PLLCFGR_M_POS           0U
#define PLLCFGR_M_MSK           (0x3FU << PLLCFGR_M_POS)
#define PLLCFGR_N_POS           6U
#define PLLCFGR_N_MSK           (0x1FFU << PLLCFGR_N_POS)
#define PLLCFGR_P_POS           16U
#define PLLCFGR_P_MSK           (0x3U << PLLCFGR_P_POS)
#define PLLCFGR_SRC_HSE         (1U << 22)
#define PLLCFGR_Q_POS           24U
#define PLLCFGR_Q_MSK           (0xFU << PLLCFGR_Q_POS)

/* RCC_APB1ENR, and PWR_CR voltage scaling */
#define APB1ENR_PWREN           (1U << 28)
#define PWR_CR_VOS_MSK          (0x3U << 14)
#define PWR_CR_VOS_SCALE1       (0x3U << 14)

/* how long to wait for an oscillator before giving up on it */
#define CLOCK_STARTUP_TRIES     0x00050000U

uint8_t clock_config_100mhz(void);
void    clock_config_hsi(void);

/* every one of these reads the registers back rather than trusting a constant */
uint32_t clock_get(void);
uint32_t clock_hclk(void);
uint32_t clock_pclk1(void);
uint32_t clock_pclk2(void);
uint32_t clock_timer_pclk1(void);
uint32_t clock_timer_pclk2(void);

#endif /* INC_DRIVER_CLOCK_H_ */