#ifndef INC_DRIVER_ADC_H_
#define INC_DRIVER_ADC_H_

#include "stm32f411xx.h"

#define ADC_CH1             (1U<<0)
#define ADC_SEQ_LEN_1       0x00
#define ADC_CR2_ADCON       (1U<<0)
#define ADC_CR2_CONT        (1U<<1)
#define ADC_CR2_SWSTART     (1U<<30)
#define ADC_SR_EOC          (1U<<1)

void adc_pa1_init(void);
void adc_start_conversion(void);
uint32_t adc_read(void);

#endif /* INC_DRIVER_ADC_H_ */