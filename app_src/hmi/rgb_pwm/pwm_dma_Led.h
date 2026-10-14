#ifndef __PWM_DMA_LED_H__
#define __PWM_DMA_LED_H__
#include <string.h>
#include "type.h"
#include "gpio.h"
#include "dma.h"
#include "timeout.h"
#include "pwm.h"

#ifdef CFG_DMA_RGB_LED_EN

#define DMA_RGB_LED_NUM 	64		//RGB灯个数

#define LED_T				150//150	//LED灯珠单个bit位周期（额定为1.25us）T=LED_T/120000000,系统时钟为120M
#define RE					0			//RES信号占空比为0
#define HT					95//95  	//逻辑bit为1的占空比
#define LT					47//47		//逻辑bit为0的占空比

#define LED_BUFFER_LEN		DMA_RGB_LED_NUM*3*8+150		//通路资源初始化

typedef struct
{
	uint8_t buf[DMA_RGB_LED_NUM];//

	#ifdef CFG_MORE_GPIO_RGB_CTRL_EN
	uint8_t buf1[DMA_RGB_LED_NUM];
	#endif
}DMA_RGB_ST;

DMA_RGB_ST DMA_RGB_R;
DMA_RGB_ST DMA_RGB_G;
DMA_RGB_ST DMA_RGB_B;

typedef struct _DMA_RGB_FIFO
{
	uint8_t		r_duty;
	uint8_t		g_duty;
	uint8_t		b_duty;
	uint8_t		r_duty_add;
	uint8_t		g_duty_add;
	uint8_t		b_duty_add;
	uint8_t		rgb_duty_add1;
	uint8_t		rgb_duty_add2;
	uint8_t		rgb_add_turn;
	uint8_t		rgb_mode;

	#ifdef CFG_MORE_GPIO_RGB_CTRL_EN
	uint8_t		r_duty1;
	uint8_t		g_duty1;
	uint8_t		b_duty1;
	uint8_t		r_duty_add1;
	uint8_t		g_duty_add1;
	uint8_t		b_duty_add1;
	uint8_t		rgb1_duty_add1;
	uint8_t		rgb1_duty_add2;
	uint8_t		rgb_mode1;
	#endif
} DMA_RGB_FIFO;
DMA_RGB_FIFO DmaRgbFifo;




void Led_Dma_Proc_Init(void);
void RgbLed1MsInterrupt(void);
void RgbModeSwitch(void);
void SetRgbLedMode(uint8_t rgb_mode);
uint8_t GetRgbLedMode(void);

#endif
#endif
