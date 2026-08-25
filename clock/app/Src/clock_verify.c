/*
 * Believing the clock is not the same as knowing it.
 *
 * Everything the previous two apps print was decoded from the same registers
 * that were written a moment earlier, so it is consistent with itself and
 * proves nothing about the crystal. If the board had a 8 MHz part fitted
 * instead of a 25 MHz one, every number would still read 100 MHz and every one
 * would be wrong by a factor of three.
 *
 * Two checks here need something outside the chip.
 *
 * The first needs a wristwatch. The led toggles every 500 SysTick ticks, and
 * SysTick was loaded from clock_hclk(), so twenty toggles is ten seconds if
 * and only if the tree is honest. Off by a factor of six and you will not need
 * the watch.
 *
 * The second needs the logic analyser. MCO1 puts a chosen clock straight onto
 * PA8 with a divider of up to 5, which is the only way to read the oscillator
 * itself rather than the chip's opinion of it. HSE over 5 is 5 MHz for a 25 MHz
 * crystal and 1.6 MHz for a 8 MHz one, and those are not easy to confuse.
 *
 *     make clock_verify && make load && make monitor
 */

#include <stdio.h>
#include "config.h"
#include "probe.h"
#include "driver_clock.h"
#include "driver_gpio.h"
#include "driver_systick.h"

#define CFGR_MCO1_MSK       (0x3U << 21)
#define CFGR_MCO1_HSI       (0x0U << 21)
#define CFGR_MCO1_HSE       (0x2U << 21)
#define CFGR_MCO1_PLL       (0x3U << 21)
#define CFGR_MCO1PRE_MSK    (0x7U << 24)
#define CFGR_MCO1PRE_DIV5   (0x7U << 24)

#define BLINK_MS            500U
#define BLINK_COUNT         20U

int main(void)
{
    config_app();
    probe_reset();

    /* PA8 as MCO1, alternate function 0 */
    GPIO_PinConfig_t mco;
    mco.pGPIOx             = GPIOA;
    mco.GPIO_PinNumber     = GPIO_PIN_NO_8;
    mco.GPIO_PinMode       = GPIO_MODE_ALTFN;
    mco.GPIO_PinSpeed      = GPIO_SPEED_FAST;
    mco.GPIO_PinOPType     = GPIO_OP_TYPE_PP;
    mco.GPIO_PinPuPdControl = GPIO_NO_PUPD;
    mco.GPIO_PinAltFunMode = GPIO_PIN_ALTFN_0;
    GPIO_Init(&mco);

    RCC->CFGR = (RCC->CFGR & ~(CFGR_MCO1_MSK | CFGR_MCO1PRE_MSK))
              | CFGR_MCO1_HSE | CFGR_MCO1PRE_DIV5;

    led_init(GPIOB, GPIO_PIN_NO_5);

    printf("\r\ntwo checks that do not come from the registers\r\n\r\n");

    printf("PA8 is carrying HSE divided by 5.\r\n");
    printf("  expect %lu Hz on the analyser\r\n",
           (unsigned long)(HSE_VALUE_HZ / 5U));
    printf("  a board with an 8 MHz part would show 1600000 instead\r\n\r\n");

    printf("the led toggles every %lu ms by SysTick, which was loaded from"
           " clock_hclk().\r\n", (unsigned long)BLINK_MS);
    printf("  time %lu toggles: expect %lu.0 seconds\r\n",
           (unsigned long)BLINK_COUNT,
           (unsigned long)(BLINK_COUNT * BLINK_MS / 1000U));
    printf("  the app prints its own elapsed count as it goes\r\n\r\n");

    printf("%8s %12s\r\n", "toggle", "ms elapsed");

    uint64_t started = ticks_get();
    uint64_t last    = started;
    uint32_t count   = 0U;

    while (1)
    {
        if ((ticks_get() - last) >= BLINK_MS)
        {
            GPIO_ToggleOutputPin(GPIOB, GPIO_PIN_NO_5);
            last = ticks_get();
            count++;

            if (count <= BLINK_COUNT)
            {
                printf("%8lu %12lu\r\n", (unsigned long)count,
                       (unsigned long)(ticks_get() - started));
            }
        }

        g_mhz = (float32_t)clock_get() / 1.0e6f;
        g_ms  = (float32_t)(ticks_get() - started);
        probe_step();
    }
}