/**
 *************************************************************************************
 * @file	adc_levels.c
 * @author	ken bu/bkd
 * @version	v0.0.1
 * @date    2019/04/24
 * @brief	 for  Sliding rheostat
 * @ maintainer: 
 * Copyright (C) Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 *************************************************************************************
 */
#include "app_config.h"
#include "app_message.h"
#include "audio_vol.h"

#ifdef CFG_ADC_LEVEL_KEY_EN
#include "main_task.h"
#include "adc_levels.h"
#include "adc.h"
//#include "config.h"
#include "timeout.h"
#include "gpio.h"
#include "debug.h"
#include "key.h"
#include "delay.h"

static uint8_t ADCLevelsScanCount        = 0;

#define MAX_ADCLEVL_LEVEL_VAL 	        4096						//电位器最大电压值:4096对应3.3v
#define DISTANCE_BETWEEN_STEP 		    5							//电位器滤波采样值://5//15//25
/*
****************************************************************
* ADC通道初始化列表
* channel: ADC采样通道
* reg_ana_en 模拟通道寄存器
* gpio
* msg 消息序号
****************************************************************
*/
const struct
{
	uint8_t 	channel;
	uint8_t 	reg_ana_en;
	uint32_t 	gpio;
	uint32_t 	msg;
} AdcChannelScanMap[] =
{
  {ADC_CHANNEL_ADC0, GPIO_A_ANA_EN, GPIOA29, MSG_ADC_LEVEL_CH1},
  {ADC_CHANNEL_ADC0, GPIO_A_ANA_EN, GPIOA30, MSG_ADC_LEVEL_CH2},
  {ADC_CHANNEL_ADC1, GPIO_A_ANA_EN, GPIOA31, MSG_ADC_LEVEL_CH3},
};

#define ADC_CHANNEL_TOTAL	(sizeof(AdcChannelScanMap)/sizeof(AdcChannelScanMap[0]))

static struct
{
	uint8_t repeat_count;	//防抖次数
	uint8_t STEP_Store;		//上一次步进值
}ADCLevels[ADC_CHANNEL_TOTAL];

void ADC_SecureSite(void);
/*
****************************************************************
* adc电位器初始化函数
*
*
****************************************************************
*/
void ADCLevelsKeyInit(void)
{
    uint8_t k;
	
    ADCLevelsScanCount = 0;
	for(k = 0; k < ADC_CHANNEL_TOTAL; k++)
	{
		ADCLevels[k].STEP_Store = 0xff;
		ADCLevels[k].repeat_count = 0;
	}

    GPIO_RegOneBitSet(AdcChannelScanMap[ADCLevelsScanCount].reg_ana_en, AdcChannelScanMap[ADCLevelsScanCount].gpio);
}
/*
****************************************************************
* adc电位器扫描处理
*
*
****************************************************************
*/
KeyScanMsg  AdcLevelKeyProcess(void)
{
	uint16_t     Val;
	uint8_t      i_count;
	uint32_t     stepNum = AudioVol_GetMaxLevel() + 1;
	if (stepNum <= 1) stepNum = 33;	// 兜底，EffectParambin未初始化时默认32+1档
	KeyScanMsg	 Msg =  {KEY_MSG_INDEX_EMPTY, KEY_UNKOWN_TYPE,ADC_LEVEL_KEY_SOURCE};

	//B6 SarADC 一个通路会复用多个GPIO,需要先使能通道再关闭通道
	GPIO_RegOneBitSet(AdcChannelScanMap[ADCLevelsScanCount].reg_ana_en, AdcChannelScanMap[ADCLevelsScanCount].gpio);
#ifdef FUNC_OS_EN
	osTaskDelay(1);
#else
	DelayMs(1);
#endif
	if((Chip_Version() == CHIP_VERSION_ECO1) || (Chip_Version() == CHIP_VERSION_ECO0))
	{
		ADC_Enable();
		osTaskDelay(1);
		Val = ADC_SingleDataGet(AdcChannelScanMap[ADCLevelsScanCount].channel);
		ADC_SecureSite();
		//DelayUs(4); 需要等待至少4us才能再次采样
	}
	else
	{
		Val = ADC_SingleDataGet(AdcChannelScanMap[ADCLevelsScanCount].channel);
	}
    GPIO_RegOneBitClear(AdcChannelScanMap[ADCLevelsScanCount].reg_ana_en, AdcChannelScanMap[ADCLevelsScanCount].gpio);

	for(i_count=0;i_count < stepNum;i_count++)
	{
		uint32_t min = MAX_ADCLEVL_LEVEL_VAL/stepNum*i_count - DISTANCE_BETWEEN_STEP;
		uint32_t max = MAX_ADCLEVL_LEVEL_VAL/stepNum*(i_count+1) + DISTANCE_BETWEEN_STEP;
		if(i_count == 0)
		{
			min = 0;
		}
		if(i_count == (stepNum-1))
		{
			max = MAX_ADCLEVL_LEVEL_VAL;
		}
		if(Val >= min && Val <= max)
		{
			break;
		}
	}

	if(i_count != stepNum)
	{
		if(i_count != ADCLevels[ADCLevelsScanCount].STEP_Store)
		{
			ADCLevels[ADCLevelsScanCount].repeat_count++;
			if(ADCLevels[ADCLevelsScanCount].repeat_count > how_many_times_have_effect)
			{
				ADCLevels[ADCLevelsScanCount].STEP_Store =  i_count;
				APP_DBG("Val = %d\n", Val);
				APP_DBG("i_count = %d\n", i_count);
				APP_DBG("ADCLevelsScanCount = %d\n", ADCLevelsScanCount);

				Msg.type = KEY_RELEASED;
				Msg.index = AdcChannelScanMap[ADCLevelsScanCount].msg + i_count;
			}
		}
		else
		{
			ADCLevels[ADCLevelsScanCount].repeat_count = 0;
		}
	}

    //GPIO_RegOneBitClear(AdcChannelScanMap[ADCLevelsScanCount].reg_ana_en, AdcChannelScanMap[ADCLevelsScanCount].gpio);
	ADCLevelsScanCount = (ADCLevelsScanCount + 1) % ADC_CHANNEL_TOTAL;
    //GPIO_RegOneBitSet(AdcChannelScanMap[ADCLevelsScanCount].reg_ana_en, AdcChannelScanMap[ADCLevelsScanCount].gpio);

	return Msg;
}
#endif

