#include "driver_adc.h"
#include "driver_gpio.h"
#include "driver_clock.h"

/* how many ADCCLK cycles each SMP setting holds the input for */
static const uint16_t sample_cycles[8] = { 3U, 15U, 28U, 56U, 84U, 112U,
                                           144U, 480U };

void adc_pa1_init(void)
{
    GPIO_PinConfig_t adcPin;

    adcPin.pGPIOx            = GPIOA;
    adcPin.GPIO_PinNumber    = GPIO_PIN_NO_1;
    adcPin.GPIO_PinMode      = GPIO_MODE_ANALOG;
    adcPin.GPIO_PinAltFunMode = GPIO_PIN_NO_ALTFN;
    adcPin.GPIO_PinSpeed     = GPIO_SPEED_FAST;
    adcPin.GPIO_PinPuPdControl = GPIO_NO_PUPD;
    adcPin.GPIO_PinOPType    = GPIO_OP_TYPE_PP;

    GPIO_Init(&adcPin);

    ADC1_PCLK_EN();

    /*
     * The converter has its own ceiling of 36 MHz and it is fed from PCLK2,
     * which is 100 MHz here. Divide by 2 would hand it 50 and it would not
     * complain, it would just convert badly. Divide by 4 is the fastest legal
     * setting on this clock tree.
     */
    adc_set_prescaler(ADC_PRE_DIV4);
    adc_set_sample_time(ADC_SMP_15);
    adc_set_resolution(ADC_CR1_RES_12BIT);

    ADC1->SQR3 = ADC_CH1;           /* first in the sequence is channel 1 */
    ADC1->SQR1 = ADC_SEQ_LEN_1;     /* and the sequence is one long */
    ADC1->CR2 |= ADC_CR2_ADCON;
}

/*
 * Continuous mode. The converter finishes one sample and immediately starts
 * the next, at whatever rate its own clock and sample time allow. Nothing here
 * chooses a sampling rate; it is whatever falls out.
 */
void adc_free_running(void)
{
    ADC1->CR2 &= ~(ADC_CR2_EXTEN_MSK);
    ADC1->CR2 |= ADC_CR2_CONT;
    ADC1->CR2 |= ADC_CR2_SWSTART;
}

void adc_start_conversion(void)
{
    adc_free_running();
}

uint32_t adc_read(void)
{
    while ((ADC1->SR & ADC_SR_EOC) == 0U)
    {
    }

    return ADC1->DR;            /* reading DR is what clears EOC */
}

void adc_set_prescaler(uint32_t prescaler)
{
    ADC_COMMON->CCR = (ADC_COMMON->CCR & ~ADC_CCR_ADCPRE_MSK)
                    | (prescaler & ADC_CCR_ADCPRE_MSK);
}

/* channel 1 lives in SMPR2, three bits starting at bit 3 */
void adc_set_sample_time(uint32_t smp)
{
    ADC1->SMPR2 = (ADC1->SMPR2 & ~(0x7U << 3)) | ((smp & 0x7U) << 3);
}

void adc_set_resolution(uint32_t res)
{
    ADC1->CR1 = (ADC1->CR1 & ~ADC_CR1_RES_MSK) | (res & ADC_CR1_RES_MSK);
}

/*
 * Hands the starting decision to a timer. EXTEN says an edge starts a
 * conversion and EXTSEL says which signal, and after this the CPU is not in
 * the timing path at all: the period between samples is the timer's period and
 * nothing else.
 */
void adc_set_trigger(uint32_t extsel)
{
    ADC1->CR2 &= ~ADC_CR2_CONT;
    ADC1->CR2 = (ADC1->CR2 & ~(ADC_CR2_EXTSEL_MSK | ADC_CR2_EXTEN_MSK))
              | (extsel & ADC_CR2_EXTSEL_MSK) | ADC_CR2_EXTEN_RISE;
}

uint32_t adc_clk_hz(void)
{
    uint32_t pre = ((ADC_COMMON->CCR & ADC_CCR_ADCPRE_MSK)
                    >> ADC_CCR_ADCPRE_POS);

    return clock_pclk2() / ((pre + 1U) * 2U);
}

uint32_t adc_sample_cycles(uint32_t smp)
{
    return sample_cycles[smp & 0x7U];
}

/*
 * A conversion is the time the input is held plus the time the successive
 * approximation takes, and the second part is one cycle per bit. Twelve bits
 * is twelve cycles, so dropping to 10 or 8 bits really does buy conversions,
 * two cycles at a time.
 */
uint32_t adc_conversion_cycles(void)
{
    uint32_t res  = (ADC1->CR1 & ADC_CR1_RES_MSK) >> ADC_CR1_RES_POS;
    uint32_t bits = 12U - (2U * res);
    uint32_t smp  = (ADC1->SMPR2 >> 3) & 0x7U;

    return sample_cycles[smp] + bits;
}

/*
 * Set when a conversion finished before the previous result was read, which
 * means a sample was thrown away. On a stream this is the difference between
 * a gap in the data and a gap you know about.
 */
uint8_t adc_overrun(void)
{
    if ((ADC1->SR & ADC_SR_OVR) != 0U)
    {
        ADC1->SR &= ~ADC_SR_OVR;
        return 1U;
    }

    return 0U;
}
