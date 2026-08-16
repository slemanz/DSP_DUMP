#ifndef INC_CONFIG_H_
#define INC_CONFIG_H_

#include <stdint.h>

#include "driver_gpio.h"

/************************************************************
*                     DEMO BOARD WIRING                     *
*************************************************************/

#define LED_PORT            GPIOB
#define LED_1               GPIO_PIN_NO_3
#define LED_2               GPIO_PIN_NO_4
#define LED_3               GPIO_PIN_NO_5

#define BUTTON_PORT         GPIOA
#define BUTTON_PIN          GPIO_PIN_NO_0
#define BUTTON_PRESSED      0U      // pull-up, wired to GND

/* The potentiometer is on PA1, ADC1 channel 1. */

void config_app(void);
void config_interface(void);
void config_core(void);

/* Pin helpers shared by every example. */
void led_init(GPIO_RegDef_t *pGPIOx, uint8_t pin);
void button_init(GPIO_RegDef_t *pGPIOx, uint8_t pin, uint8_t mode);

#endif /* INC_CONFIG_H_ */
