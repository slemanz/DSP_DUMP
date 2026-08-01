#include "driver_adc.h"
#include "driver_gpio.h"

void adc_pa1_init(void)
{
    /****Configure the ADC GPIO Pin**/
    GPIO_PinConfig_t adcPin;

    adcPin.pGPIOx = GPIOA;
    adcPin.GPIO_PinNumber = GPIO_PIN_NO_1;
    adcPin.GPIO_PinMode = GPIO_MODE_ANALOG;
    adcPin.GPIO_PinAltFunMode = GPIO_PIN_NO_ALTFN;
    adcPin.GPIO_PinSpeed = GPIO_SPEED_FAST;
    adcPin.GPIO_PinPuPdControl = GPIO_NO_PUPD;
    adcPin.GPIO_PinOPType = GPIO_OP_TYPE_PP;

    GPIO_Init(&adcPin);

    /****Configure the ADC Module**/
    ADC1_PCLK_EN();

	ADC1->SQR3 = ADC_CH1; // Set conversion sequence start
	ADC1->SQR1 = ADC_SEQ_LEN_1; // Set conversion sequence length 
    ADC1->CR2 |= ADC_CR2_ADCON; // Enable ADC module
}

void adc_start_conversion(void)
{
	ADC1->CR2 |= ADC_CR2_CONT; // Enable continuous conversion
	ADC1->CR2 |= ADC_CR2_SWSTART; // Start ADC conversion
}

uint32_t adc_read(void)
{
	while(!(ADC1->SR & ADC_SR_EOC)){} // Wait for conversion to be complete
	return (ADC1->DR); // Read converted value*/
}