#ifndef INC_TIMER_H_
#define INC_TIMER_H_

#include "stm32f411xx.h"

// freq = system_freq / ((prescaler + 1) * (arr + 1))
#define PRESCALER           (10-1)
#define ARR_VALUE           (26667 - 1)

#define CR1_CEN          (1U << 0)
#define SR_UIF           (1U << 0)

#define OC4_TOGGLE       ((1U << 13) | (1U << 12))
#define OC4_PWM          (0x6 << 12)
#define CCER_CC4E        (1U << 12)


typedef struct
{
	uint32_t prescaler; 
    uint32_t auto_reload;
    uint8_t channel;  		/*!< possible modes from @TIM_CHANNEL >*/
	float initialDuty;
}TIM_Config_t;



/*
 * This is a Handle structure for a Timer
 */

typedef struct
{
	TIM_RegDef_t *pTIMx; 
	TIM_Config_t TIM_Config; 
}TIM_Handle_t;

/*
 * @TIM_CHANNEL
 * Timer possible channels
 */


#define TIM_CHANNEL1                0
#define TIM_CHANNEL2                1
#define TIM_CHANNEL3                2
#define TIM_CHANNEL4                3


void timer_PeriClockControl(TIM_RegDef_t *pTIMx, uint8_t EnorDi);

void timer_pwm_init(TIM_Handle_t *pTIMHandle);
void timer_pwm_set_duty_cycle(TIM_Handle_t *pTIMHandle, float duty_cycle);

void tim2_1hz_init(void);
void tim2_pa3_out_compare(void);
void tim2_pa3_pwm(void);

/* Added to drivers/Inc/timer.h */

#define CR1_ARPE            (1U << 7)
#define DIER_UIE            (1U << 0)
#define EGR_UG              (1U << 0)

/* CR2 master mode: what this timer sends out on its TRGO line */
#define CR2_MMS_POS         4U
#define CR2_MMS_MSK         (0x7U << CR2_MMS_POS)
#define CR2_MMS_UPDATE      (0x2U << CR2_MMS_POS)

/* returns the rate it actually achieved, which is not always the one asked for */
uint32_t timer_periodic_init(TIM_RegDef_t *pTIMx, uint32_t rate_hz);
void     timer_trgo_on_update(TIM_RegDef_t *pTIMx);
void     timer_interrupt_enable(TIM_RegDef_t *pTIMx, uint8_t irq_number);
void     timer_clear_update(TIM_RegDef_t *pTIMx);
uint32_t timer_rate_hz(TIM_RegDef_t *pTIMx);

/* the two the reference manual lists for these; the repo header stops at exti */
#define IRQ_NO_TIM2         28
#define IRQ_NO_TIM3         29

#endif