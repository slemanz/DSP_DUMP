/*
 * The sampling theorem, on a signal that exists.
 *
 * The sampling chapter made this argument on numbers already in memory, where
 * the signal was whatever the array said it was. Here the signal is a square
 * wave the chip generates on PB4 and the samples come back through the pin next
 * to it, so nothing is assumed.
 *
 *     wire PB4 to PA1
 *
 * TIM3 makes the wave, TIM2 sets the sampling rate, and the app walks the
 * sampling rate down past twice the tone and keeps going. Above 2f the shape
 * on the graph is a square wave. Below it the shape is still a wave, a perfectly
 * clean one, at a frequency that is not in the signal and never was:
 *
 *     alias = |f - round(f / fs) * fs|
 *
 * Nothing is broken when this happens and no measurement can undo it. The
 * samples are correct samples of a waveform that goes through all of them, and
 * so does the original, and once they are taken there is nothing left to say
 * which one it was. That is the whole reason an anti aliasing filter goes
 * before a converter and not after it.
 *
 *     make adc_alias && make load && make monitor
 */

#include <stdio.h>
#include <math.h>
#include "config.h"
#include "probe.h"
#include "timer.h"
#include "driver_adc.h"
#include "driver_clock.h"
#include "driver_gpio.h"
#include "driver_systick.h"

#define TONE_HZ         500U
#define CAPTURE         200U

static uint16_t capture[CAPTURE];

static const uint32_t rates[] = { 8000U, 4000U, 1500U, 1100U, 1000U, 900U };

/* PB4 is TIM3 channel 1 on alternate function 2 */
static void tone_start(uint32_t hz)
{
    GPIO_PinConfig_t pin;

    pin.pGPIOx             = GPIOB;
    pin.GPIO_PinNumber     = GPIO_PIN_NO_4;
    pin.GPIO_PinMode       = GPIO_MODE_ALTFN;
    pin.GPIO_PinSpeed      = GPIO_SPEED_FAST;
    pin.GPIO_PinOPType     = GPIO_OP_TYPE_PP;
    pin.GPIO_PinPuPdControl = GPIO_NO_PUPD;
    pin.GPIO_PinAltFunMode = 2U;
    GPIO_Init(&pin);

    (void)timer_periodic_init(TIM3, hz * 2U);   /* toggle twice per period */

    TIM3->CCMR[0] = (0x3U << 4);                /* channel 1, toggle on match */
    TIM3->CCR[0]  = 0U;
    TIM3->CCER   |= (1U << 0);
    TIM3->EGR     = EGR_UG;
    TIM3->CR1    |= CR1_CEN;
}

static uint32_t alias_of(uint32_t f_hz, uint32_t fs_hz)
{
    float32_t folded = (float32_t)f_hz
                     - roundf((float32_t)f_hz / (float32_t)fs_hz)
                       * (float32_t)fs_hz;

    return (uint32_t)fabsf(folded);
}

int main(void)
{
    config_app();
    probe_reset();

    adc_pa1_init();
    tone_start(TONE_HZ);

    printf("\r\nPB4 is carrying a %lu Hz square wave. wire it to PA1.\r\n\r\n",
           (unsigned long)TONE_HZ);

    printf("%10s %10s %12s %s\r\n", "fs", "fs / 2", "apparent", "verdict");

    for (uint32_t k = 0U; k < ARRAY_LEN(rates); k++)
    {
        uint32_t fs = rates[k];
        uint32_t apparent = (fs > (2U * TONE_HZ)) ? TONE_HZ
                                                  : alias_of(TONE_HZ, fs);

        printf("%10lu %10lu %12lu %s\r\n", (unsigned long)fs,
               (unsigned long)(fs / 2U), (unsigned long)apparent,
               (fs > (2U * TONE_HZ)) ? "the real one" : "an alias");
    }

    printf("\r\nthe 1000 Hz row is exactly twice the tone, which the theorem"
           " calls the\r\nboundary and not a working rate: sampling a wave"
           " exactly twice a period\r\ncan land on the zero crossings every"
           " time and report nothing at all.\r\n");

    printf("\r\nthe graph walks the same rates in order. watch g_raw, not the"
           " numbers:\r\nthe first two look like a square wave and the last"
           " ones look like clean\r\nslow waves that the signal does not"
           " contain.\r\n");

    while (1)
    {
        for (uint32_t k = 0U; k < ARRAY_LEN(rates); k++)
        {
            uint32_t fs = timer_periodic_init(TIM2, rates[k]);

            timer_trgo_on_update(TIM2);
            adc_set_trigger(ADC_TRIGGER_TIM2_TRGO);
            (void)adc_overrun();

            for (uint32_t i = 0U; i < CAPTURE; i++)
            {
                while ((ADC1->SR & ADC_SR_EOC) == 0U)
                {
                }

                capture[i] = (uint16_t)ADC1->DR;
            }

            g_rate   = (float32_t)fs;
            g_jitter = (float32_t)((rates[k] > (2U * TONE_HZ))
                                   ? TONE_HZ : alias_of(TONE_HZ, rates[k]));

            for (uint32_t i = 0U; i < CAPTURE; i++)
            {
                g_raw   = (float32_t)capture[i];
                g_volts = (float32_t)capture[i] * (float32_t)ADC_VREF_MV
                          / (float32_t)ADC_FULL_SCALE / 1000.0f;
                probe_step();
            }
        }
    }
}