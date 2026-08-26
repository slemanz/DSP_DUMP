#ifndef INC_DRIVER_ADC_H_
#define INC_DRIVER_ADC_H_

#include "stm32f411xx.h"

/*
 * The common register block sits 0x300 past ADC1 and holds the one setting
 * that is shared between converters, which on this part is the prescaler.
 */
#define ADC1_COMMON_BASEADDR    (APB2PERIPH_BASE + 0x2300UL)
#define ADC_COMMON              ((ADC_Common_RegDef_t *)ADC1_COMMON_BASEADDR)

typedef struct
{
    __vo uint32_t CSR;      /*!< common status register,   offset 0x00 */
    __vo uint32_t CCR;      /*!< common control register,  offset 0x04 */
    __vo uint32_t CDR;      /*!< common data register,     offset 0x08 */
} ADC_Common_RegDef_t;

#define ADC_CH1             (1U << 0)
#define ADC_SEQ_LEN_1       0x00U

/* CR1 */
#define ADC_CR1_RES_POS     24U
#define ADC_CR1_RES_MSK     (0x3U << ADC_CR1_RES_POS)
#define ADC_CR1_RES_12BIT   (0x0U << ADC_CR1_RES_POS)
#define ADC_CR1_RES_10BIT   (0x1U << ADC_CR1_RES_POS)
#define ADC_CR1_RES_8BIT    (0x2U << ADC_CR1_RES_POS)
#define ADC_CR1_RES_6BIT    (0x3U << ADC_CR1_RES_POS)

/* CR2 */
#define ADC_CR2_ADCON       (1U << 0)
#define ADC_CR2_CONT        (1U << 1)
#define ADC_CR2_EOCS        (1U << 10)
#define ADC_CR2_EXTSEL_POS  24U
#define ADC_CR2_EXTSEL_MSK  (0xFU << ADC_CR2_EXTSEL_POS)
#define ADC_CR2_EXTEN_POS   28U
#define ADC_CR2_EXTEN_MSK   (0x3U << ADC_CR2_EXTEN_POS)
#define ADC_CR2_EXTEN_RISE  (0x1U << ADC_CR2_EXTEN_POS)
#define ADC_CR2_SWSTART     (1U << 30)

/* the regular trigger table in the reference manual; 0110 is tim2 trgo */
#define ADC_TRIGGER_TIM2_TRGO   (0x6U << ADC_CR2_EXTSEL_POS)

/* SR */
#define ADC_SR_EOC          (1U << 1)
#define ADC_SR_OVR          (1U << 5)

/* CCR, the prescaler off pclk2 */
#define ADC_CCR_ADCPRE_POS  16U
#define ADC_CCR_ADCPRE_MSK  (0x3U << ADC_CCR_ADCPRE_POS)
#define ADC_PRE_DIV2        (0x0U << ADC_CCR_ADCPRE_POS)
#define ADC_PRE_DIV4        (0x1U << ADC_CCR_ADCPRE_POS)
#define ADC_PRE_DIV6        (0x2U << ADC_CCR_ADCPRE_POS)
#define ADC_PRE_DIV8        (0x3U << ADC_CCR_ADCPRE_POS)

/* the converter itself has a ceiling, independent of the bus feeding it */
#define ADC_CLK_MAX_HZ      36000000U

/* SMPR2 holds channels 0 to 9, three bits each */
#define ADC_SMP_3           0U
#define ADC_SMP_15          1U
#define ADC_SMP_28          2U
#define ADC_SMP_56          3U
#define ADC_SMP_84          4U
#define ADC_SMP_112         5U
#define ADC_SMP_144         6U
#define ADC_SMP_480         7U

#define ADC_VREF_MV         3300U
#define ADC_FULL_SCALE      4096U

void     adc_pa1_init(void);
void     adc_start_conversion(void);
uint32_t adc_read(void);

void     adc_set_prescaler(uint32_t prescaler);
void     adc_set_sample_time(uint32_t smp);
void     adc_set_resolution(uint32_t res);
void     adc_set_trigger(uint32_t extsel);
void     adc_free_running(void);

uint32_t adc_clk_hz(void);
uint32_t adc_sample_cycles(uint32_t smp);
uint32_t adc_conversion_cycles(void);
uint8_t  adc_overrun(void);

#endif /* INC_DRIVER_ADC_H_ */
