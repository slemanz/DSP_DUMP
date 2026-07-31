#include "config.h"

/************************************************************
*                       DRIVERS                             *
*************************************************************/
#include "driver_clock.h"
#include "driver_gpio.h"
#include "driver_uart.h"
#include "driver_systick.h"
#include "driver_fpu.h"


static const GPIO_PinConfig_t GPIO_ConfigTable[] = {

//  { GPIOC, GPIO_PIN_NO_13, GPIO_MODE_OUT,   GPIO_SPEED_LOW, GPIO_OP_TYPE_PP, GPIO_NO_PUPD,  GPIO_PIN_ALTFN_0   },    // LED
    { GPIOA, GPIO_PIN_NO_3,  GPIO_MODE_ALTFN, GPIO_SPEED_LOW, GPIO_OP_TYPE_PP, GPIO_NO_PUPD,  PA3_ALTFN_UART2_RX },    // UART2 TX
    { GPIOA, GPIO_PIN_NO_2,  GPIO_MODE_ALTFN, GPIO_SPEED_LOW, GPIO_OP_TYPE_PP, GPIO_NO_PUPD,  PA2_ALTFN_UART2_TX },    // UART2 RX
    { GPIOA, GPIO_PIN_NO_5,  GPIO_MODE_OUT,   GPIO_SPEED_LOW, GPIO_OP_TYPE_PP, GPIO_NO_PUPD,  GPIO_PIN_ALTFN_0   },    // (LED)
//  { GPIOA, GPIO_PIN_NO_6,  GPIO_MODE_IN,    GPIO_SPEED_FAST, GPIO_OP_TYPE_PP, GPIO_PIN_PU,   GPIO_PIN_ALTFN_0   },   // (BUTTON)
};

static const UART_Config_t UART_ConfigTable[ ]= {

    {UART2, UART_MODE_TXRX, UART_STD_BAUD_115200, UART_STOPBITS_1, UART_WORDLEN_8BITS, UART_PARITY_DISABLE, UART_HW_FLOW_CTRL_NONE}
};

void config_drivers(void)
{
    GPIO_Init_table(GPIO_ConfigTable, (sizeof(GPIO_ConfigTable)/sizeof(GPIO_ConfigTable[0])));
    UART_Init_table(UART_ConfigTable, (sizeof(UART_ConfigTable)/sizeof(UART_ConfigTable[0])));
    systick_init(1000);
    fpu_enable();

    UART_PeripheralControl(UART2, ENABLE);
}

/************************************************************
*                         APP                               *
*************************************************************/


void config_app(void)
{
    config_drivers();
}

// printf retarget
extern int __io_putchar(int ch)
{
    uint8_t ch_send = (uint8_t)ch;
    UART_write(UART2, &ch_send, 1);
    return ch;
}