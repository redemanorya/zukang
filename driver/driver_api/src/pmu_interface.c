#include "type.h"
#include "pmu_interface.h"
#include "efuse.h"
#include "debug.h"
#include "clk.h"

/**
 * @brief     Powerkey 参数设置
 *
 * @param[in] eMode					Powerkey模式
 * @param[in] u8_spTime   			短按计时, 单位:8ms; 取值范围:9-71
 * @param[in] u8_lpTime     		长按计时, 单位:64ms; 取值范围:1-64
 * @param[in] u8_lPressRstIime		长按复位计时,单位:1.024s; 取值范围:1-15
 * @param[in] ePressMode			开机事件类型, 设置长按/短按 开机
 * @param[in] eRstMode				长按复位类型, 按键按下达到时长且松开复位/达到时长就复位
 *
 *            						时间设置应满足  TShort < TLong < TReset
 * @return    TE_PWRKEYINIT_RET
 */
static TE_PWRKEYINIT_RET SystemPowerKeySetting(TE_PWRKEY_MODE eMode, uint8_t u8_spTime, uint8_t u8_lpTime, uint8_t u8_longPressResetIime, POWERKEY_LONGORSHORT_PRESS_SEL ePressMode, POWER_LONG_RESET_MODE eRstMode)
{
	uint32_t KeyState = 0;
	//check eMode
	if (E_PWRKEY_MODE_SLIDESWITCH_LON < eMode)
	{
		return E_PWRKEYINIT_UNKNOWN_MODE;
	}

	if (E_PWRKEY_MODE_BYPASS == eMode)
	{
		PMU_PowerKeyDisable();
		return E_PWRKEYINIT_NONE;
	}

	if (!PMU_FristPowerOnFlagGet())
	{
		return E_PWRKEYINIT_NOT_FIRST_PWRON;
	}

	//check u8_spTime
	if ((u8_spTime < 9) || (u8_spTime > 71))
	{
		return E_PWRKEYINIT_SPTIME_ERR;
	}

	//check u8_lpTime
	if ((u8_lpTime < 1) || (u8_lpTime > 64))
	{
		return E_PWRKEYINIT_LPTIME_ERR;
	}

	//check u8_longPressResetIime
	if ((u8_longPressResetIime < 1) || (u8_longPressResetIime > 15))
	{
		return E_PWRKEYINIT_LPRSTTIME_ERR;
	}

	//compare u8_spTime and u8_lpTime
	if ((u8_lpTime << 3) <= u8_spTime)
	{
		return E_PWRKEYINIT_STIME_GE_LPTIME_ERR;
	}

	//compare u8_lpTime and u8_longPressResetIime
	if (((u8_longPressResetIime << 10) + 64) <= (u8_lpTime << 6) )
	{
		return E_PWRKEYINIT_LTIME_GE_RSTTIME_ERR;
	}

	if (((E_PWRKEY_MODE_SLIDESWITCH_LON == eMode)||(E_PWRKEY_MODE_SLIDESWITCH_HON == eMode)) && PMU_FristPowerOnFlagGet())
	{
		KeyState = PMU_PowerKeyPinStateGet();
	}

	//enable powerkey
	PMU_PowerKeyEnable();

	if (E_PWRKEY_MODE_PUSHBUTTON == eMode)
	{
		PMU_PowerKeyModeSet(SOFT_MODE);								//soft mode
		PMU_PowerKeyActiveLevelSet(LOW_INDICATE_POWERON);			//low level power on
	}
	else
	{
		PMU_PowerKeyModeSet(HARD_MODE);								//hard mode
		PMU_PowerKeyHardModeSet(LEVEL_TRIGGER);						//edge trigger
		if (E_PWRKEY_MODE_SLIDESWITCH_HON == eMode)
		{
			PMU_PowerKeyActiveLevelSet(HIGH_INDICATE_POWERON);			//high level power on
		}
		else
		{
			PMU_PowerKeyActiveLevelSet(LOW_INDICATE_POWERON);			//Low level power on
		}
	}

	//set poweron press mode
	PMU_PowerKeyLongOrShortPressSet(ePressMode);					//power on press mode

	//set noise filter, unit:4ms
	//Fixed 64ms
	PMU_PowerKeyNoiseFilterMaxCntSet(0x10);

	//set short press time: 8ms*powerkey_trg_s_max_cnt[5:0]+ 4ms*powerkey_noise_filter_max_cnt[4:0]
	//u8_spTime*8ms
	PMU_PowerKeyShortPressTrigMaxCntSet(u8_spTime - 8);

	//set long press time: 64ms*powerkey_trg_l_max_cnt[5:0]+ powerkey_noise_filter_max_cnt[4:0]*4ms
	//u8_lpTime*64ms
	PMU_PowerKeyLongPressTrigMaxCntSet(u8_lpTime - 1);

	//set long press reset time: 1.024s*powerkey_trg_rst_max_cnt[3:0]+ powerkey_noise_filter_max_cnt[4:0]*4ms
	//u8_longPressResetIime*1.024s + 64ms
	PMU_PowerKeyResetTrigMaxCntSet(u8_longPressResetIime);

	//set reset mode
	//LONGR_RST_MODE_TIMEOUT_KEYUP: reset release until press release if press time is longer than 8s
	//LONGR_RST_MODE_TIMEOUT: reset release when press time reached 8s no matter when press release
	PMU_PowerLongResetModeSet(eRstMode);

	if((KeyState == 1) && (E_PWRKEY_MODE_SLIDESWITCH_LON == eMode))
	{
		PMU_SystemPowerDown();
		while(1);
	}
	else if((KeyState == 0) && (E_PWRKEY_MODE_SLIDESWITCH_HON == eMode))
	{
		PMU_SystemPowerDown();
		while(1);
	}

	return E_PWRKEYINIT_OK;
}

/**
 * @brief     清除PowerKey按键状态标志
 *
 * @param     None
 * @return    None
 */
void SystemPowerKeyStateClear(void)
{
	PMU_PowerKeyStateClear();
	PMU_PowerKeyShortPressStateClear();
	PMU_PowerKeyLongPressStateClear();
}

/**
 * @brief     获取当前PowerKey设置模式
 *
 * @param     None
 * @return    TE_PWRKEY_MODE
 */
TE_PWRKEY_MODE SystemPowerKeyGetMode(void)
{
	if (PMU_PowerKeyEnGet())
	{
		if (PMU_PowerKeyModeGet() == HARD_MODE)
		{
			if (PMU_PowerKeyActiveLevelGet() == HIGH_INDICATE_POWERON)
			{
				return E_PWRKEY_MODE_SLIDESWITCH_HON;
			}
			else
			{
				return E_PWRKEY_MODE_SLIDESWITCH_LON;
			}

		}
		else
		{
			return E_PWRKEY_MODE_PUSHBUTTON;
		}
	}

	return E_PWRKEY_MODE_BYPASS;
}

/**
 * @brief     检测PowerKey短按或长按事件
 *
 * @param     ePressMode: 短/长按检测类型
 * @return    TRUE:检测到标志; FLASE:未检测到
 */
static bool SystemPowerKeyDetectEvt(POWERKEY_LONGORSHORT_PRESS_SEL ePressMode)
{
	bool bIsFlagSet = FALSE;

	switch(SystemPowerKeyGetMode())
	{
		case E_PWRKEY_MODE_SLIDESWITCH_HON:
			if ((PMU_PowerKeyPinStateGet()== FALSE)&& PMU_PowerKeyTrigStateGet())
			{
				bIsFlagSet = TRUE;
			}
			break;

		case E_PWRKEY_MODE_SLIDESWITCH_LON:
			if ((PMU_PowerKeyPinStateGet()== TRUE) && PMU_PowerKeyTrigStateGet())
			{
				bIsFlagSet = TRUE;
			}
			break;

		case E_PWRKEY_MODE_PUSHBUTTON:
			if ((FALSE == PMU_PowerKeyPinStateGet()) && (PMU_PowerKeyActiveLevelGet() == LOW_INDICATE_POWERON))
			{
				if (POWERKEY_LONG_PRESS_MODE == ePressMode)
				{
					bIsFlagSet = PMU_PowerKeyLongPressTrigStateGet();
				}
				else
				{
					bIsFlagSet = PMU_PowerKeyShortPressTrigStateGet();
				}
			}
			break;

		default:
			break;
	}

	if (bIsFlagSet)
	{
		SystemPowerKeyStateClear();
	}

	return bIsFlagSet;
}

/**
 * @brief     Powerkey初始化
 *
 * @param[in] eMode	 	Powerkey模式
 * @param     eTime     设置PUSHBUTTON模式时按键检测时长. SLIDESWITCH模式时默认为64ms
 *
 * @return    TE_PWRKEYINIT_RET
 */
TE_PWRKEYINIT_RET SystemPowerKeyInit(TE_PWRKEY_MODE eMode, TE_KEYDET_TIME eTime)
{
	uint8_t  lpTime = 0;
	uint8_t  spTime = 0;

	if (E_PWRKEY_MODE_PUSHBUTTON == eMode)
	{
		//short press time: 20*8ms = 160ms
		spTime = 20;

		switch (eTime)
		{
			case E_KEYDETTIME_512MS:
				lpTime = 8;
				break;

			case E_KEYDETTIME_1024MS:
				lpTime = 16;
				break;

			case E_KEYDETTIME_1536MS:
				lpTime = 24;
				break;

			case E_KEYDETTIME_2048MS:
				lpTime = 32;
				break;

			case E_KEYDETTIME_3072MS:
				lpTime = 48;
				break;

			case E_KEYDETTIME_4096MS:
				lpTime = 64;
				break;

			default:
				lpTime = 64;
				break;
		}
	}
	else
	{
		lpTime = 2;  //128ms
		spTime = 9;  //72ms
	}

	pmu_rc32k_calibrate();

	//DBG("lpTime = %d\n", lpTime);
	//parameter settings 参数设置
	//short press time: lpTime * 8ms
	//long presstime:   spTime * 64ms
	//long reset time:  8*1024ms + 64ms = 8256 ms
	return SystemPowerKeySetting(eMode, spTime, lpTime, 8, POWERKEY_LONG_PRESS_MODE, LONGR_RST_MODE_TIMEOUT);
}

/**
 * @brief     检测PowerKey按键事件
 *
 * @return    TRUE:检测到事件; FLASE:未检测到
 */
bool SystemPowerKeyDetect(void)
{
	return SystemPowerKeyDetectEvt(POWERKEY_LONG_PRESS_MODE);
}


/**
 * @brief     GPIOC0/1/2做为Powerup key 设置
 * @param     ePin 按键
 * @param     eType 类型
 * @return    TRUE:设置成功; FALSE:设置失败;
 */
bool SystemGPIOCPowerUpSet(GPIOC_POWERUP_PIN ePin, GPIOC_POWERUP_TYPE eType)
{
	if ((ePin <= GPIOC_POWERUP_PIN_C2) && (eType <= GPIOC_POWERUP_TYPE_BOTH_EDGE))
	{
		if (GPIOC_POWERUP_PIN_C0 == ePin)
		{
			PMU_C0GPIOCtrlSet(0, 0, 0, 1);
			PMU_GpioC0DebugModeSet(1);
			
		}
		else if (GPIOC_POWERUP_PIN_C1 == ePin)
		{
			PMU_C1GPIOCtrlSet(0, 0, 0, 1);
			PMU_GpioC1DebugModeSet(1);
		}
		else
		{
			PMU_C2GPIOCtrlSet(0, 0, 0, 1);
			PMU_GpioC2DebugModeSet(1);
		}
		PMU_GPIOCSetPowerupType(ePin, eType);
		
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


