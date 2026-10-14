/**
 **************************************************************************************
 * @file    adc_key.c
 * @brief   
 *
 * @author  pi
 * @version V1.0.0
 *
 * $Created: 2018-01-11 17:30:47$
 *
 * @Copyright (C) 2018, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

//driver、lib
#include "type.h"
#include "adc.h"
#include "clk.h"
#include "gpio.h"
#include "timeout.h"
#include "sadc_interface.h"
#include "rtos_api.h"

//application
#include "app_config.h"
#include "adc_key.h"
#include "debug.h"
#include "delay.h"

#define     NORMAL_ADKEY                (1)    ///1 = 标准的ADKEY值处理，0 = 用户自定义ADKEY值处理


#define 	ADC_KEY_SCAN_TIME			10
#define 	ADC_KEY_JITTER_TIME			30
#define 	ADC_KEY_CP_TIME				1000 	//for hold or long press
#define 	ADC_KEY_CPH_TIME			200		//for long long press
#define	    ADC_KEY_COUNT				11
#define		ADC_KEY_FULL_VAL			4096

#define		ADC_VAL(val)				(val*4096/330)

#define     NORMAL_ADKEY                (1)    ///1 = 标准的ADKEY值处理，0 = 用户自定义ADKEY值处理
#define     POWERKEY_11_ADKEY           (1)    ///1 = 标准的11个 2 = 稳定6个按键 ADKEY值处理，0 = 5个 ADKEY值处理+1个POWER KEY功能键

#if (POWERKEY_11_ADKEY == 1)
#define 	PWR_ADCKEY_COUNT			11

#define PWRKEY_0    ADC_020V
#define PWRKEY_1    ADC_040V
#define PWRKEY_2    ADC_070V
#define PWRKEY_3    ADC_110V
#define PWRKEY_4    ADC_140V
#define PWRKEY_5    ADC_170V
#define PWRKEY_6    ADC_190V
#define PWRKEY_7    ADC_220V
#define PWRKEY_8    ADC_260V
#define PWRKEY_9    ADC_290V
#define PWRKEY_10   ADC_310V

uint32_t UserPWR_11Key_Tab[]=
{
	PWRKEY_0,
	PWRKEY_1,
	PWRKEY_2,
	PWRKEY_3,
	PWRKEY_4,
	PWRKEY_5,
	PWRKEY_6,
	PWRKEY_7,
	PWRKEY_8,
	PWRKEY_9,
	PWRKEY_10,
};
#endif

#if (NORMAL_ADKEY == 1)
#define ADKEY_0    (ADC_000V+ADC_030V)/2
#define ADKEY_1    (ADC_030V+ADC_060V)/2
#define ADKEY_2    (ADC_060V+ADC_090V)/2
#define ADKEY_3    (ADC_090V+ADC_120V)/2
#define ADKEY_4    (ADC_120V+ADC_150V)/2
#define ADKEY_5    (ADC_150V+ADC_180V)/2
#define ADKEY_6    (ADC_180V+ADC_210V)/2
#define ADKEY_7    (ADC_210V+ADC_240V)/2
#define ADKEY_8    (ADC_240V+ADC_270V)/2
#define ADKEY_9    (ADC_270V+ADC_300V)/2
#define ADKEY_10   (ADC_300V+ADC_330V)/2
#define ADKEY_11   (ADC_000V+ADC_030V)/2
#define ADKEY_12   (ADC_000V+ADC_030V)/2
#define ADKEY_13   (ADC_000V+ADC_030V)/2
#define ADKEY_14   (ADC_000V+ADC_030V)/2

uint32_t UserADKey_Tab[]=
{
	ADKEY_0,
	ADKEY_1,
	ADKEY_2,
	ADKEY_3,
	ADKEY_4,
	ADKEY_5,
	ADKEY_6,
	ADKEY_7,
	ADKEY_8,
	ADKEY_9,
	ADKEY_10,
};
#endif

typedef enum _ADC_KEY_STATE
{
	ADC_KEY_STATE_IDLE,
	ADC_KEY_STATE_JITTER,
	ADC_KEY_STATE_PRESS_DOWN,
	ADC_KEY_STATE_CP

} ADC_KEY_STATE;



#ifdef CFG_RES_ADC_KEY_SCAN
TIMER			AdcKeyWaitTimer;
//TIMER			AdcKeyScanTimer;
ADC_KEY_STATE	AdcKeyState;
static 	uint8_t 	PreKeyIndex = KEY_MSG_INDEX_EMPTY;

const struct
{
	uint8_t 			channel;
	uint8_t 			key_count;
	uint8_t 			key_index_offset;
	const uint32_t * 	key_adc_val_tab;
	uint32_t			RegIndex;//GPIO 寄存器偏移量
	uint32_t			GpioIndex;//GPIO index
} AdcKeyScanMap[] =
{
#ifdef CFG_RES_POWERKEY_ADC_EN
	{ADC_CHANNEL_POWERKEY, 		PWR_ADCKEY_COUNT, 	0, 				UserPWR_11Key_Tab, 0, 0},
#endif
#ifdef CFG_RES_ADC_KEY_CH1
	{CFG_RES_ADC_KEY_CH1,	ADC_KEY_COUNT,		ADC_KEY_COUNT,	UserADKey_Tab, PORT_ANA_REG(CFG_RES_ADC_KEY_CH1_IO), GET_IO_PIN(CFG_RES_ADC_KEY_CH1_IO)},
#endif
#ifdef CFG_RES_ADC_KEY_CH2
	{CFG_RES_ADC_KEY_CH2,	ADC_KEY_COUNT,		ADC_KEY_COUNT*2,UserADKey_Tab, PORT_ANA_REG(CFG_RES_ADC_KEY_CH2_IO), GET_IO_PIN(CFG_RES_ADC_KEY_CH2_IO)},
#endif
};

void ADC_SecureSite(void);
void AdcKeyInit(void)
{
	AdcKeyState = ADC_KEY_STATE_IDLE;
	PreKeyIndex = KEY_MSG_INDEX_EMPTY;
	//TimeOutSet(&AdcKeyScanTimer, 0);

	uint16_t dadd_voltage = 330;//310;//Power_GetLDO33DConfigVoltage()/ 10;
	uint8_t i;
#if (POWERKEY_11_ADKEY == 1)
	for (i = 0; i < sizeof(UserPWR_11Key_Tab)/sizeof(UserPWR_11Key_Tab[0]);i++)
	{
		UserPWR_11Key_Tab[i] = UserPWR_11Key_Tab[i]/ dadd_voltage ;
	#ifdef CFG_ADC_KEY_DEBUG
		DBG("[%d]%d\n", i, UserPWR_11Key_Tab[i]);
	#endif
	}

#else
	for (i = 0; i < sizeof(UserPWRKey_Tab)/sizeof(UserPWRKey_Tab[0]);i++)
	{
		UserPWRKey_Tab[i] = UserPWRKey_Tab[i]/ dadd_voltage;
	#ifdef CFG_ADC_KEY_DEBUG
		DBG("[%d]%d\n", i, UserPWRKey_Tab[i]);
	#endif
	}
#endif
	for (i = 0; i < sizeof(UserADKey_Tab)/sizeof(UserADKey_Tab[0]);i++)
	{
		UserADKey_Tab[i] = UserADKey_Tab[i] / dadd_voltage;
	#ifdef CFG_ADC_KEY_DEBUG
		DBG("[%d]%d\n", i, UserADKey_Tab[i]);
	#endif
	}

//#ifdef CFG_RES_ADC_KEY_CH1
//	GPIO_RegOneBitSet(CFG_RES_ADC_KEY_CH1_ANA_EN, CFG_RES_ADC_KEY_CH1_ANA_MASK);
//#endif
//
//#ifdef CFG_RES_ADC_KEY_CH2
//	GPIO_RegOneBitSet(CFG_RES_ADC_KEY_CH2_ANA_EN, CFG_RES_ADC_KEY_CH2_ANA_MASK);
//#endif

	return;
}

uint8_t GetAdcKeyIndex(void)
{
	uint8_t 	KeyIndex = KEY_MSG_INDEX_EMPTY;
	uint8_t 	i;
	uint16_t	Val;

	for(i=0;i<sizeof(AdcKeyScanMap)/sizeof(AdcKeyScanMap[0]);i++)
	{

		if(AdcKeyScanMap[i].channel != ADC_CHANNEL_POWERKEY)
		{
			GPIO_RegOneBitSet(AdcKeyScanMap[i].RegIndex, AdcKeyScanMap[i].GpioIndex);
			if((Chip_Version() == CHIP_VERSION_ECO1) || (Chip_Version() == CHIP_VERSION_ECO0))
			{
				ADC_Enable();
				//DelayUs(10);
				osTaskDelay(1);
				Val = ADC_SingleDataGet(AdcKeyScanMap[i].channel);
				ADC_SecureSite();
				//DelayUs(10); //需要等待至少4us才能再次采样
			}
			else
			{
				Val = ADC_SingleDataGet(AdcKeyScanMap[i].channel);
			}
			GPIO_RegOneBitClear(AdcKeyScanMap[i].RegIndex, AdcKeyScanMap[i].GpioIndex);
		}
		else
		{
			if((Chip_Version() == CHIP_VERSION_ECO1) || (Chip_Version() == CHIP_VERSION_ECO0))
			{
				ADC_Enable();
				//DelayUs(10);
				osTaskDelay(1);
				Val = ADC_SingleDataGet(AdcKeyScanMap[i].channel);
				ADC_SecureSite();
				//DelayUs(10); //需要等待至少4us才能再次采样
			}
			else
			{
				Val = ADC_SingleDataGet(AdcKeyScanMap[i].channel);
			}
		}
#ifdef CFG_ADC_KEY_DEBUG
		if(Val < 4040)
		{
			DBG("val = %d\n", Val);
		}
#endif
		if(Val < (ADC_KEY_FULL_VAL - (ADC_KEY_FULL_VAL / ADC_KEY_COUNT) / 2))
		{
			if(AdcKeyScanMap[i].key_adc_val_tab)
			{
				uint8_t j;
//				for(j = 0; j < AdcKeyScanMap[i].key_count; j++)
//				{
//					DBG("%d, %d\n", i, AdcKeyScanMap[i].key_adc_val_tab[j]);
//				}
				for(j = 0; j < AdcKeyScanMap[i].key_count; j++)
				{
					if(Val < AdcKeyScanMap[i].key_adc_val_tab[j])
					{
						KeyIndex = j;
						//DBG("KeyIndex = %d, %d, %d\n", KeyIndex, Val, AdcKeyScanMap[i].key_adc_val_tab[j]);
						break;
					}
				}
			}
			else
			{
				KeyIndex = (Val + (ADC_KEY_FULL_VAL / AdcKeyScanMap[i].key_count) / 2) * AdcKeyScanMap[i].key_count / ADC_KEY_FULL_VAL;
			}

			if(KeyIndex != KEY_MSG_INDEX_EMPTY)
			{
				KeyIndex += AdcKeyScanMap[i].key_index_offset;
				break;
			}
		}
	}

	return KeyIndex;
}


/**
 * @func        AdcKeyScan
 * @brief       AdcKeyScan 按下有消抖，弹起没有消抖处理。
 * @param       void  
 * @Output      None
 * @return      AdcKeyMsg
 * @Others      
 * Record
 * 1.Date        : 20180123
 *   Author      : pi.wang
 *   Modification: Created function
*/
KeyScanMsg AdcKeyScan(void)
{
	uint8_t				KeyIndex=KEY_MSG_INDEX_EMPTY;
	KeyScanMsg Msg = {KEY_MSG_INDEX_EMPTY, KEY_UNKOWN_TYPE,ADC_KEY_SOURCE};

//	if(!IsTimeOut(&AdcKeyScanTimer))
//	{
//		return Msg;
//	}
//	TimeOutSet(&AdcKeyScanTimer, ADC_KEY_SCAN_TIME);

	KeyIndex = GetAdcKeyIndex();

	switch(AdcKeyState)
	{
		case ADC_KEY_STATE_IDLE:
			if(KeyIndex == KEY_MSG_INDEX_EMPTY)
			{
				return Msg;
			}
			PreKeyIndex = KeyIndex;
			TimeOutSet(&AdcKeyWaitTimer, ADC_KEY_JITTER_TIME);
			//APP_DBG("GOTO JITTER!\n");
			AdcKeyState = ADC_KEY_STATE_JITTER;

		case ADC_KEY_STATE_JITTER:
			if(PreKeyIndex != KeyIndex)
			{
				//APP_DBG("GOTO IDLE Because jitter!\n");
				PreKeyIndex = KEY_MSG_INDEX_EMPTY;
				AdcKeyState = ADC_KEY_STATE_IDLE;
			}
			else if(IsTimeOut(&AdcKeyWaitTimer))
			{
				//APP_DBG("GOTO PRESS_DOWN!\n");
				TimeOutSet(&AdcKeyWaitTimer, ADC_KEY_CP_TIME);
				AdcKeyState = ADC_KEY_STATE_PRESS_DOWN;
				Msg.index = PreKeyIndex;
				Msg.type = KEY_PRESSED;
			}
			break;

		case ADC_KEY_STATE_PRESS_DOWN:
			if(PreKeyIndex != KeyIndex)
			{
				//APP_DBG("key release %d\n", PreKeyIndex);
				AdcKeyState = ADC_KEY_STATE_IDLE;
				Msg.index = PreKeyIndex;
				Msg.type = KEY_RELEASED;
				PreKeyIndex = KEY_MSG_INDEX_EMPTY;
			}
			else if(IsTimeOut(&AdcKeyWaitTimer))
			{
				//return key cp value
				//APP_DBG("ADC KEY CPS!\n");
				TimeOutSet(&AdcKeyWaitTimer, ADC_KEY_CPH_TIME);
				AdcKeyState = ADC_KEY_STATE_CP;
				Msg.index = PreKeyIndex;
				Msg.type = KEY_LONG_PRESSED;
			}
			break;

		case ADC_KEY_STATE_CP:
			if(PreKeyIndex != KeyIndex)
			{
				//return key cp value
				//APP_DBG("ADC KEY CPR!\n");
				AdcKeyState = ADC_KEY_STATE_IDLE;
				Msg.index = PreKeyIndex;
				Msg.type  = KEY_LONG_RELEASED;
				PreKeyIndex = KEY_MSG_INDEX_EMPTY;
			}
			else if(IsTimeOut(&AdcKeyWaitTimer))
			{
				//return key cph value
				//APP_DBG("ADC KEY CPH!\n");
				TimeOutSet(&AdcKeyWaitTimer, ADC_KEY_CPH_TIME);
				Msg.index = PreKeyIndex;
				Msg.type  = KEY_LONG_PRESS_HOLD;
			}
			break;

		default:
			AdcKeyState = ADC_KEY_STATE_IDLE;
			PreKeyIndex = KEY_MSG_INDEX_EMPTY;
			break;
	}

	return Msg;
}

#endif //CFG_RES_ADC_KEY_SCAN


