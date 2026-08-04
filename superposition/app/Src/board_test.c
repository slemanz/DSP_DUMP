/*
 * Board bring-up test: LEDs blink together at 500 ms, the button is reported
 * on the serial, and the potentiometer's raw ADC count is printed with it.
 *
 *     make board_test && make load && make monitor
 */

#include <stdio.h>
#include "config.h"

#include "driver_gpio.h"
#include "driver_systick.h"
#include "driver_adc.h"

#define REPORT_PERIOD_MS    500U

int main(void)
{
    config_app();

    led_init(LED_PORT, LED_1);
    led_init(LED_PORT, LED_2);
    led_init(LED_PORT, LED_3);

    // start in step, not in whatever state the port powered up in
    GPIO_WriteToOutputPin(LED_PORT, LED_1, 0);
    GPIO_WriteToOutputPin(LED_PORT, LED_2, 0);
    GPIO_WriteToOutputPin(LED_PORT, LED_3, 0);

    button_init(BUTTON_PORT, BUTTON_PIN, GPIO_MODE_IN);

    adc_pa1_init();
    adc_start_conversion();

    printf("\r\nboard test: leds PB3/PB4/PB5, button PA0, pot PA1\r\n");

    uint64_t last_report = ticks_get();
    uint8_t  press_seen = 0;

    while (1)
    {
        // polled every pass so a tap between two reports is not missed
        if (GPIO_ReadFromInputPin(BUTTON_PORT, BUTTON_PIN) == BUTTON_PRESSED)
        {
            press_seen = 1;
        }

        if ((ticks_get() - last_report) >= REPORT_PERIOD_MS)
        {
            GPIO_ToggleOutputPin(LED_PORT, LED_1);
            GPIO_ToggleOutputPin(LED_PORT, LED_2);
            GPIO_ToggleOutputPin(LED_PORT, LED_3);

            printf("pot: %4lu   button: %s\r\n",
                   (unsigned long)adc_read(),
                   press_seen ? "pressed" : "-");

            press_seen = 0;
            last_report = ticks_get();
        }
    }
}
