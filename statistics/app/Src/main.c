#include <stdio.h>
#include "config.h"
#include "arm_math.h"

#include "driver_systick.h"
#include "driver_gpio.h"

#include "signals.h"

#define TEST_LENGTH_SAMPLES 4

float g_in_sig_sample; // debug this


extern float _5hz_signal[HZ_5_SIG_LEN];
extern float32_t input_signal_f32_1kHz_15kHz[KHZ1_15_SIG_LEN];

static void plot_input_signal(void);
static void pseudo_dly(int dly);

int main(void)
{
    config_app();

    float32_t srcA[TEST_LENGTH_SAMPLES] = {1.0, 2.0, 3.0, 4.0};
    float32_t srcB[TEST_LENGTH_SAMPLES] = {5.0, 6.0, 7.0, 8.0};
    float32_t dst[TEST_LENGTH_SAMPLES];
    uint32_t blockSize = TEST_LENGTH_SAMPLES;

    // Perform optimized addition: dst = srcA + srcB
    arm_add_f32(srcA, srcB, dst, blockSize);
    (void)dst;
    (void)input_signal_f32_1kHz_15kHz;
    
    //uint64_t start_time = ticks_get();

    while(1)
    {
        /*
        if((ticks_get() - start_time) >= 500)
        {
            GPIO_ToggleOutputPin(GPIOA, GPIO_PIN_NO_5);
            start_time = ticks_get();
        }*/

        plot_input_signal();
    }
}


static void plot_input_signal(void)
{
	int i;
	for( i = 0; i < HZ_5_SIG_LEN; i++)
	{
		g_in_sig_sample = _5hz_signal[i];
        printf("%f\r\n", g_in_sig_sample);
		pseudo_dly(9000);
	}
}

static void pseudo_dly(int dly)
{
	for( int i = 0; i < dly; i++ ){}
}

