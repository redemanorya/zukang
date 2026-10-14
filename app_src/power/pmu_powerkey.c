#include "app_config.h"
#include "pmu_powerkey.h"
#include "pmu_interface.h"
#include "timeout.h"
#include "watchdog.h"
#include "clk.h"
#include "irqn.h"
#include "debug.h"

void PMU_PowerKey8SResetSet(void)
{
	DBG("PK INIT\r\n");
	PMU_PowerKeyEnable();

	PMU_PowerKeyModeSet(HARD_MODE);

	PMU_PowerKeyActiveLevelSet(HIGH_INDICATE_POWERON);
	PMU_PowerKeyShortPressTrigMaxCntSet(25);  //8ms*powerkey_trg_s_max_cnt[5:0]+ 4ms*powerkey_noise_filter_max_cnt[4:0]
												//25*8 + 16*4 = 264ms
	PMU_PowerKeyResetTrigMaxCntSet(0x8);   //1.024s*powerkey_trg_rst_max_cnt[3:0]+ powerkey_noise_filter_max_cnt[4:0]*4ms

	PMU_PowerLongResetModeSet(LONGR_RST_MODE_TIMEOUT);

	PMU_PowerKeyStateClear();
	PMU_PowerKeyShortPressStateClear();
	PMU_PowerKeyLongPressStateClear();
}

void PMU_PowerKey8sResetEnable()
{
    PMU_PowerKeyModeSet(SOFT_MODE);
    PMU_PowerKeyActiveLevelSet(LOW_INDICATE_POWERON);
    PMU_PowerKeyLongOrShortPressSet(POWERKEY_SHORT_PRESS_MODE);
    PMU_PowerLongResetModeSet(LONGR_RST_MODE_TIMEOUT);

    PMU_PowerKeyStateClear();
    PMU_PowerKeyShortPressStateClear();
    PMU_PowerKeyLongPressStateClear();

    PMU_PowerKeyEnable();
    APP_DBG("8sReset Enable\r\n");
}

void PMU_PowerKey8sResetDisable()
{
    PMU_PowerKeyDisable();
    APP_DBG("8sReset Disable\r\n");
}

#ifdef CFG_IDLE_MODE_POWER_KEY
bool GetPowerKeyVaildState(void)
{
	switch(POWERKEY_MODE)
	{
	case POWERKEY_MODE_BYPASS:
		break;
	case POWERKEY_MODE_PUSH_BUTTON:
		return 1;
		break;
	case POWERKEY_MODE_SLIDE_SWITCH_LPD://硬开关高唤醒
		return (!PMU_PowerKeyPinStateGet());
		break;
	case POWERKEY_MODE_SLIDE_SWITCH_HPD://硬开关低唤醒
		return (PMU_PowerKeyPinStateGet());
		break;
	default:
		break;
	}
	return 0;
}

#if (POWERKEY_MODE == POWERKEY_MODE_SLIDE_SWITCH_LPD) || (POWERKEY_MODE == POWERKEY_MODE_SLIDE_SWITCH_HPD)
#include "app_message.h"
#define POWER_KEY_JITTER_CNT		50
#define POWER_KEY_WAIT_RELEASE		0xffff
static uint32_t CntTimer = POWER_KEY_JITTER_CNT;
uint16_t GetSystemPowerKeyMsg(void)
{
	switch(POWERKEY_MODE)
	{
	case POWERKEY_MODE_BYPASS:
		break;
	case POWERKEY_MODE_PUSH_BUTTON:
		break;
	case POWERKEY_MODE_SLIDE_SWITCH_LPD://硬开关高唤醒
	case POWERKEY_MODE_SLIDE_SWITCH_HPD://硬开关低唤醒
		if(GetPowerKeyVaildState())
		{
			if(CntTimer > 0 && CntTimer != POWER_KEY_WAIT_RELEASE)
				CntTimer--;
			if(CntTimer == 0)
			{
				CntTimer = POWER_KEY_WAIT_RELEASE;
				return MSG_POWERDOWN;
			}
		}
		else
		{
			CntTimer = POWER_KEY_JITTER_CNT;
		}
		break;
	default:
		break;
	}
	return MSG_NONE;
}
#endif

void SystemPowerDown(void)
{
	switch(POWERKEY_MODE)
	{
	case POWERKEY_MODE_BYPASS:
		PMU_PowerKeyDisable();
		break;
	case POWERKEY_MODE_PUSH_BUTTON:
#if 0//Sam,20260701
		PMU_PowerKeyModeSet(SOFT_MODE);
		PMU_PowerKeyActiveLevelSet(LOW_INDICATE_POWERON);
		//PMU_PowerKeyLongOrShortPressSet(POWERKEY_SHORT_PRESS_MODE);
		PMU_PowerKeyLongOrShortPressSet(POWERKEY_LONG_PRESS_MODE);
		PMU_PowerKeyShortPressTrigMaxCntSet(0xff);
		PMU_PowerKeyLongPressTrigMaxCntSet(0xff);//约4s
		PMU_PowerKeyNoiseFilterMaxCntSet(0xff);

        PMU_PowerKeyStateClear();
		PMU_PowerKeyShortPressStateClear();
		PMU_PowerKeyLongPressStateClear();
        
        PMU_PowerKeyEnable();//配置完后再enable Powerkey
#endif
        SystemPowerKeyInit(E_PWRKEY_MODE_PUSHBUTTON, E_KEYDETTIME_2048MS);
        PMU_PowerKeyStateClear();
		PMU_PowerKeyShortPressStateClear();
		PMU_PowerKeyLongPressStateClear();

        //Sam，20260317，移值于G1
		PMU_PowerkeyPullup22K();
		PMU_PowerkeySarADCDis();
		// 关中断
		GIE_DISABLE();
		//切时钟到RC 12M
		SpiFlashInit(80000000, MODE_1BIT, 0, FSHC_RC_CLK_MODE);
		Clock_SysClkSelect(RC_CLK_MODE);
		Clock_APllClose();
#if !defined(CFG_RES_RTC_EN)
		Clock_HOSCDisable(); // 若有RTC应用并且RTC所用时钟是HOSC，则不关闭HOSC，即24M晶振
#endif
        PMU_SystemPowerDown();
        break;
	case POWERKEY_MODE_SLIDE_SWITCH_LPD://硬开关高唤醒
		PMU_PowerKeyStateClear();
		PMU_PowerKeyShortPressStateClear();
		PMU_PowerKeyLongPressStateClear();

		PMU_PowerKeyEnable();
		PMU_PowerKeyModeSet(HARD_MODE);
		PMU_PowerKeyHardModeSet(LEVEL_TRIGGER);
		PMU_PowerKeyActiveLevelSet(HIGH_INDICATE_POWERON);
		PMU_PowerKeyLongOrShortPressSet(POWERKEY_SHORT_PRESS_MODE);
		PMU_PowerKeyShortPressTrigMaxCntSet(0xff);
		PMU_PowerKeyLongPressTrigMaxCntSet(20);
		PMU_PowerKeyNoiseFilterMaxCntSet(0xff);

		//Sam，20260317，移值于G1
//		PMU_PowerkeyPullup22K();
		PMU_PowerkeySarADCDis();
		// 关中断
		GIE_DISABLE();
		//切时钟到RC 12M
		SpiFlashInit(80000000, MODE_1BIT, 0, FSHC_RC_CLK_MODE);
		Clock_SysClkSelect(RC_CLK_MODE);
		Clock_APllClose();
#if !defined(CFG_RES_RTC_EN)
		Clock_HOSCDisable(); // 若有RTC应用并且RTC所用时钟是HOSC，则不关闭HOSC，即24M晶振
#endif

		PMU_SystemPowerDown();
		break;
	case POWERKEY_MODE_SLIDE_SWITCH_HPD://硬开关低唤醒
		PMU_PowerKeyStateClear();
		PMU_PowerKeyShortPressStateClear();
		PMU_PowerKeyLongPressStateClear();

		PMU_PowerKeyEnable();
		PMU_PowerKeyModeSet(HARD_MODE);
		PMU_PowerKeyHardModeSet(LEVEL_TRIGGER);
		PMU_PowerKeyActiveLevelSet(LOW_INDICATE_POWERON);
		PMU_PowerKeyLongOrShortPressSet(POWERKEY_SHORT_PRESS_MODE);
		PMU_PowerKeyShortPressTrigMaxCntSet(0xff);
		PMU_PowerKeyLongPressTrigMaxCntSet(20);
		PMU_PowerKeyNoiseFilterMaxCntSet(0xff);

		//Sam，20260317，移值于G1
//		PMU_PowerkeyPullup22K();
		PMU_PowerkeySarADCDis();
		// 关中断
		GIE_DISABLE();
		//切时钟到RC 12M
		SpiFlashInit(80000000, MODE_1BIT, 0, FSHC_RC_CLK_MODE);
		Clock_SysClkSelect(RC_CLK_MODE);
		Clock_APllClose();
#if !defined(CFG_RES_RTC_EN)
		Clock_HOSCDisable(); // 若有RTC应用并且RTC所用时钟是HOSC，则不关闭HOSC，即24M晶振
#endif

		PMU_SystemPowerDown();
		break;
	default:
		break;
	}
}


void SystemPowerKeyIdleModeInit(void)
{


	switch(POWERKEY_MODE)
	{
	case POWERKEY_MODE_BYPASS:
		break;
	case POWERKEY_MODE_PUSH_BUTTON:
		#ifndef POWERKEY_FIRST_ENTER_POWERDOWN
		if(!PMU_PowerupEventGet())
			break;
		#endif
		PMU_PowerupEventClr();
#if 0
		uint32_t cnt = 2500;
		cnt = 1000;	//按需求调整长按开机的时间
		while(cnt--)
		{
//			APP_DBG("%d",PMU_PowerKeyPinStateGet());
			WDG_Feed();
			if(PMU_PowerKeyPinStateGet())
			{
				SystemPowerDown();
			}
			DelayMs(1);
		}
#endif
		break;
	case POWERKEY_MODE_SLIDE_SWITCH_LPD://硬开关高唤醒
	case POWERKEY_MODE_SLIDE_SWITCH_HPD://硬开关低唤醒
//		APP_DBG("=========%d \n",PMU_PowerKeyPinStateGet());
		if(GetPowerKeyVaildState())
		{
			SystemPowerDown();
		}
		break;
	default:
		break;
	}
}
#endif

