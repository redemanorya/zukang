//driver、lib
#include <string.h>
#include "irqn.h"
#include "reset.h"
#include "rtc.h"
#include "watchdog.h"
#include "powercontroller.h"
#include "pmu_interface.h"

//application
#include "main_task.h"
#include "remind_sound.h"
#include "audio_effect.h"
#include "deepsleep.h"
#include "audio_vol.h"
//Port#include "ctrlvars.h"
#include "bt_config.h"
//Port#include "bb_api.h"
#include "bt_manager.h"
#include "bt_stack_service.h"
#include "audio_core_service.h"
#include "pmu_powerkey.h"
#include "audio_effect.h"
#ifdef DUAL_CORE_EN
#include "dualcore_api.h"
#include "dualcore_dsp.h"
#endif

#define MAIN_APP_TASK_SLEEP_PRIO		6 //进入deepsleep 需要相对其他task最高优先级。

#ifdef CFG_APP_IDLE_MODE_EN

#define IDLE_NOT_REQUIRED_MODE	(BIT(ModeIdle) | BIT(ModeBtHfPlay))	

#ifdef CFG_FUNC_REMIND_SOUND_EN
#define CFG_FUNC_REMIND_DEEPSLEEP		//Deepsleep 前播放(sys)提示音
#define CFG_FUNC_REMIND_WAKEUP			//Deepsleep 唤醒以后播放提示音
#endif

extern bool ModeCommonInit(void);
extern void SystemTimerInit(void);

#if defined(CFG_APP_IDLE_MODE_EN)&&defined(CFG_FUNC_REMIND_SOUND_EN)
extern volatile uint32_t gIdleRemindSoundTimeOutTimer;
#endif

#define POWER_ON_IDLE				0
#define NEED_POWER_ON				1
#define WAIT_POWER_ON_REMIND_SOUND	2
#define ENTER_POWER_ON				3

static struct
{
	uint16_t	AutoPowerOnState 	:	2;

#if	defined(CFG_IDLE_MODE_POWER_KEY)
	uint16_t	PowerKeyWakeUpCheckFlag 	: 	1; //上电/powerkey唤醒检查
#endif
#ifdef  CFG_FUNC_REMIND_SOUND_EN
	uint16_t	RemindSoundFlag 	:	1;
	uint16_t	DeepSleepFlag 	:	1;
#endif		
	uint16_t	FristPowerOnFlag 	:	1;
#if(BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT)
	uint16_t	LeSinkModeFlag 	:	1;
#endif
	SysModeNumber	SavePrevMode;
}IdleMode;

void PowerOnRemindSound(void)
{
#ifdef  CFG_FUNC_REMIND_SOUND_EN
	RemindSoundClearPlay();
#if !defined(CFG_FUNC_REMIND_WAKEUP) && defined(CFG_IDLE_MODE_DEEP_SLEEP)
	if(IdleMode.DeepSleepFlag)
	{
		IdleMode.DeepSleepFlag = FALSE;
		return; 		
	}
#endif	
	IdleMode.RemindSoundFlag = RemindSoundServiceItemRequest(SOUND_REMIND_KAIJI, REMIND_ATTR_NEED_CLEAR_INTTERRUPT_PLAY|REMIND_PRIO_SYS);
	IdleMode.DeepSleepFlag = FALSE;
	gIdleRemindSoundTimeOutTimer = 0;
#endif
}

void PowerDownRemindSound(void)
{
#ifdef  CFG_FUNC_REMIND_SOUND_EN

#if !defined(CFG_FUNC_REMIND_DEEPSLEEP) && defined(CFG_IDLE_MODE_DEEP_SLEEP)
	if(SoftFlagGet(SoftFlagIdleModeEnterSleep))
	{
		return;			
	}
#endif	
	IdleMode.RemindSoundFlag = RemindSoundServiceItemRequest(SOUND_REMIND_GUANJI, REMIND_ATTR_NEED_CLEAR_INTTERRUPT_PLAY|REMIND_PRIO_SYS);
	gIdleRemindSoundTimeOutTimer = 0;
	RemindSoundItemRequestDisable();
#endif
}

bool GetPowerRemindSoundPlayEnd(void)
{
#ifdef  CFG_FUNC_REMIND_SOUND_EN
	if(IdleMode.RemindSoundFlag)
	{
		if(!RemindSoundIsPlay() || gIdleRemindSoundTimeOutTimer > 2000)
			IdleMode.RemindSoundFlag = FALSE;
		return TRUE;
	}
#endif
	return FALSE;
}

void IdleModeConfig(void)
{
	IdleMode.AutoPowerOnState	= NEED_POWER_ON;
	PMU_PowerKeyDisable();
#ifdef CFG_IDLE_MODE_POWER_KEY
	SystemPowerKeyIdleModeInit();
#ifdef POWER_UP_GPIOC
	SystemGPIOCPowerUpSet(GPIOC_POWERON_PIN, GPIOC_PIN_MODE);
#endif
#endif
#ifdef  CFG_FUNC_REMIND_SOUND_EN
	IdleMode.RemindSoundFlag 	= FALSE;
	IdleMode.DeepSleepFlag = FALSE;
#endif
	IdleMode.FristPowerOnFlag = TRUE;
}

bool IdleModeInit(void)
{
	if(!ModeCommonInit())
	{
		ModeCommonDeinit();
		return FALSE;
	}
	//Core Process
	//AudioCoreProcessConfig((void*)AudioNoAppProcess);
#ifdef CFG_FUNC_ROBOEFFECT_EN
	AudioCoreProcessConfig((void*)AudioMusicProcess);
#else
	AudioCoreProcessConfig((void*)AudioBypassProcess);
#endif

#ifdef CFG_FUNC_ROBOEFFECT_EN
#ifdef DUAL_CORE_EN
	DualCoreDspAddrInit();
#endif

	AudioEffect_Params_Sync(&EffectParambin);
#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
	AudioEffect_Params_Sync(&EffectParambin2);
#endif
#endif

#if BT_TWS_SUPPORT
	tws_link_disconnect();
	SetBtDeviceConnState(BtAccessModeNotAccessible);//防止因本地状态导致可见性设置失败
#endif
	DBG("Idle Mode Init\n");
#if (defined(CFG_APP_BT_MODE_EN) && (BT_HFP_SUPPORT))
	if((GetA2dpState(BtCurIndex_Get()) >= BT_A2DP_STATE_CONNECTED)
		|| (GetHfpState(BtCurIndex_Get()) >= BT_HFP_STATE_CONNECTED)
		|| (GetAvrcpState(BtCurIndex_Get()) >= BT_AVRCP_STATE_CONNECTED))
	{
		//手动断开
		BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_DISCONNECT_DEV_CTRL);
	}
#endif	

#ifdef  CFG_FUNC_REMIND_SOUND_EN	
	if(!IdleMode.FristPowerOnFlag)
	{
		PowerDownRemindSound();
	}
#endif	
	DBG("Idle Mode run\n");
#ifdef CFG_APP_BT_MODE_EN
	if((!IdleMode.FristPowerOnFlag) && sys_parameter.bt_BackgroundType == BT_BACKGROUND_POWER_ON)
		BtFastPowerOff();

	#if BT_HFP_SUPPORT
	extern void SetsBtHfModeEnterFlag(uint32_t flag);
	SetsBtHfModeEnterFlag(0);
	#endif
#endif
	IdleMode.FristPowerOnFlag = FALSE;
	return TRUE;
}


bool IdleModeDeinit(void)
{

	DBG("Idle Mode Deinit\n");

#ifndef AUDIO_STREAM_UNINTERRUPT
	if(IsAudioPlayerMute() == FALSE)
	{
		HardWareMuteOrUnMute();
	}	
	
	PauseAudioCore();
	//注意：AudioCore父任务调整到mainApp下，此处只关闭AudioCore通道，不关闭任务
	AudioCoreProcessConfig((void*)AudioNoAppProcess);
	ModeCommonDeinit();//通路全部释放
#endif

#ifdef	CFG_IDLE_MODE_POWER_KEY
	SoftFlagDeregister(SoftFlagIdleModeEnterPowerDown);
#endif				
#ifdef CFG_IDLE_MODE_DEEP_SLEEP
	SoftFlagDeregister(SoftFlagIdleModeEnterSleep);
#endif	
#ifdef CFG_SOFT_POWER_KEY_EN
	SoftFlagDeregister(SoftFlagIdleModeEnterSoftPower);
#endif
#ifdef CFG_APP_BT_MODE_EN
	BtStackServiceWaitClear();
	if(sys_parameter.bt_BackgroundType != BT_BACKGROUND_DISABLE)
	{
		BtStackServiceMsgSend(MSG_BTSTACK_RUN_START);
		if(sys_parameter.bt_BackgroundType == BT_BACKGROUND_POWER_ON)
			BtFastPowerOn();
		osTaskDelay(2);//等待BT设置完可见性
	}
#endif
	return TRUE;
}

void SendEnterIdleModeMsg(void)
{
	MessageContext		msgSend;
	
	msgSend.msgId = MSG_ENTER_IDLE_MODE;
	MessageSend(GetMainMessageHandle(), &msgSend);
#if(BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT)
	if(GetSystemMode() == ModeLeAudioSinkPlay)
	{
		CommonMsgProccess(MSG_LE_BIS_SNK_STOP);
		IdleMode.LeSinkModeFlag = 1;
	}
#endif
}

void SendQuitIdleModeMsg(void)
{
	MessageContext		msgSend;
	
	msgSend.msgId = MSG_QUIT_IDLE_MODE;
	MessageSend(GetMainMessageHandle(), &msgSend);	
}

extern bool GetRemindSoundItemDisable(void);
extern void IdlePrevModeSet(SysModeNumber mode);
void IdleModeRun(uint16_t msgId)
{
#ifdef CFG_IDLE_MODE_POWER_KEY
	if(SoftFlagGet(SoftFlagIdleModeEnterPowerDown)
#ifdef	CFG_FUNC_REMIND_SOUND_EN
	 && (!GetPowerRemindSoundPlayEnd())
#endif
	 )
	{
		extern bool GetPowerKeyVaildState(void);
		SoftFlagDeregister(SoftFlagIdleModeEnterPowerDown);
		if(GetPowerKeyVaildState())
		{
			SystemPowerDown();
		}
		else
		{
			PMU_PowerKeyStateClear();
			PMU_PowerKeyShortPressStateClear();
			PMU_PowerKeyLongPressStateClear();
			IdleMode.AutoPowerOnState = NEED_POWER_ON;
		}
	}
#endif	

#ifdef CFG_IDLE_MODE_DEEP_SLEEP
	if(SoftFlagGet(SoftFlagIdleModeEnterSleep)
#ifdef	CFG_FUNC_REMIND_SOUND_EN
		&& (!GetPowerRemindSoundPlayEnd())
#endif
	)
	{

		SoftFlagDeregister(SoftFlagIdleModeEnterSleep);
	#ifdef  CFG_FUNC_REMIND_SOUND_EN
		IdleMode.DeepSleepFlag = TRUE;
	#endif	
		PauseAudioCore();
		UBaseType_t pri = uxTaskPriorityGet(NULL);
#if defined(CFG_APP_BT_MODE_EN)
		if(sys_parameter.bt_BackgroundType != BT_BACKGROUND_DISABLE)
		{
			osTaskDelay(50);
			//bb reset
			BtResetAndKill(TRUE);
		}
#endif
		vTaskPrioritySet(NULL, MAIN_APP_TASK_SLEEP_PRIO);//设定最高优先级
		
 		NVIC_DisableIRQ(TMR2_IRQn);

#ifdef CFG_FUNC_DEBUG_USE_TIMER
		extern void PrintfAllInBuffer(void);
		PrintfAllInBuffer();
#endif
	
		DeepSleeping();
		
		SystemTimerInit();

#ifdef CFG_FUNC_DEBUG_USE_TIMER
		{
			void UartLogUseTimer(bool enable);
			UartLogUseTimer(1);
		}
#endif

#if defined(CFG_APP_BT_MODE_EN)
		if(sys_parameter.bt_BackgroundType != BT_BACKGROUND_DISABLE)
		{
			WDG_Feed();
			// BtStackServiceStart();
           BtPowerOn();
           WDG_Feed();
		}
#endif


		vTaskPrioritySet(NULL, pri);
		AudioCoreServiceResume();
		osTaskDelay(10);// for printf 
	
		IdleMode.AutoPowerOnState = NEED_POWER_ON;
	}
#endif	

#ifdef CFG_SOFT_POWER_KEY_EN
	if(SoftFlagGet(SoftFlagIdleModeEnterSoftPower)
#ifdef	CFG_FUNC_REMIND_SOUND_EN
		&& (!GetPowerRemindSoundPlayEnd())
#endif
	)
	{
		extern void SoftKeyPowerOff(void);
		SoftFlagDeregister(SoftFlagIdleModeEnterSoftPower);
		//SoftKeyPowerOff();
		IdleMode.AutoPowerOnState = POWER_ON_IDLE;
	}
#endif
	switch(IdleMode.AutoPowerOnState)
	{
		case NEED_POWER_ON:
			PowerOnRemindSound();
			IdleMode.AutoPowerOnState = WAIT_POWER_ON_REMIND_SOUND;
			break;
		case WAIT_POWER_ON_REMIND_SOUND:
			#ifdef  CFG_FUNC_REMIND_SOUND_EN
			if(!GetPowerRemindSoundPlayEnd())
			#endif
			{
				osTaskDelay(100);
				IdleMode.AutoPowerOnState = ENTER_POWER_ON;	
			}
			break;
		case ENTER_POWER_ON:
			IdleMode.AutoPowerOnState = POWER_ON_IDLE;
			SendQuitIdleModeMsg();
			break;
		case POWER_ON_IDLE:
			break;
		default:
			IdleMode.AutoPowerOnState = POWER_ON_IDLE;
			break;
	}

	switch(msgId)
	{
		case MSG_POWER:
		case MSG_POWERDOWN:
		case MSG_DEEPSLEEP:

#ifdef CFG_SOFT_POWER_KEY_EN
		case MSG_SOFT_POWER:
#endif
#ifdef CFG_IDLE_MODE_POWER_KEY
			if(SoftFlagGet(SoftFlagIdleModeEnterPowerDown)
#ifdef	CFG_FUNC_REMIND_SOUND_EN
			 && (!GetPowerRemindSoundPlayEnd())
#endif
			 )
				break;
#endif
			if(IdleMode.AutoPowerOnState == POWER_ON_IDLE && (!GetPowerRemindSoundPlayEnd()))
				IdleMode.AutoPowerOnState = NEED_POWER_ON;
			break;
		case MSG_DEVICE_SERVICE_U_DISK_IN:	//插U盘开机
//			IdlePrevModeSet(ModeUDiskAudioPlay);
#ifdef CFG_FUNC_REMIND_SOUND_EN
			if(GetRemindSoundItemDisable())
				IdleMode.AutoPowerOnState = NEED_POWER_ON;	//播放开机提示音，然后进入模式
			else if(RemindSoundIsPlay())	//已经在播放提示音，等待播放完
				IdleMode.AutoPowerOnState = WAIT_POWER_ON_REMIND_SOUND;
			else
#endif
				IdleMode.AutoPowerOnState = ENTER_POWER_ON;	//直接进入模式，不需要播放开机提示音
			break;
		case MSG_DEVICE_SERVICE_CARD_IN:
			break;
		case MSG_DEVICE_SERVICE_USB_FS_DEVICE_IN://插声卡开机
		case MSG_DEVICE_SERVICE_USB_DEVICE_IN://插声卡开机
//			IdlePrevModeSet(ModeUsbDevicePlay);
#ifdef CFG_FUNC_REMIND_SOUND_EN
			if(GetRemindSoundItemDisable())
				IdleMode.AutoPowerOnState = NEED_POWER_ON;	//播放开机提示音，然后进入模式
			else if(RemindSoundIsPlay())	//已经在播放提示音，等待播放完
				IdleMode.AutoPowerOnState = WAIT_POWER_ON_REMIND_SOUND;
			else
#endif
				IdleMode.AutoPowerOnState = ENTER_POWER_ON;	//直接进入模式，不需要播放开机提示音
			break;
		default:
			CommonMsgProccess(msgId);
			break;
	}

}


void IdlePrevModeSet(SysModeNumber mode)
{
	IdleMode.SavePrevMode = mode;
}
SysModeNumber IdlePrevModeGet(void)
{
	return IdleMode.SavePrevMode;
}
extern osMutexId SysModeMutex;
void IdleModeEnter(void)
{
	if(GetSysModeState(ModeIdle) == ModeStateInit || GetSysModeState(ModeIdle) == ModeStateRunning )
		return;
	osMutexLock(SysModeMutex);
	if(IDLE_NOT_REQUIRED_MODE & BIT(mainAppCt.SysCurrentMode))
		IdleMode.SavePrevMode = mainAppCt.SysPrevMode;
	else
		IdleMode.SavePrevMode = mainAppCt.SysCurrentMode;

#if BT_TWS_SUPPORT
	if(IdleMode.SavePrevMode == ModeTwsSlavePlay)
	{
		IdleMode.SavePrevMode = mainAppCt.SysPrevMode;
	}
#endif

	if(GetSysModeState(ModeIdle) == ModeStateSusend)
	{
		SetSysModeState(ModeIdle,ModeStateReady);
	}
	SysModeEnter(ModeIdle);
	osMutexUnlock(SysModeMutex);
	APP_DBG("enter idle mode\n");
}

void IdleModeExit(void)
{
	if(IDLE_NOT_REQUIRED_MODE & BIT(IdleMode.SavePrevMode))
		IdleMode.SavePrevMode = ModeBtAudioPlay;
#if BT_TWS_SUPPORT
	if(IdleMode.SavePrevMode == ModeTwsSlavePlay)
	{
		IdleMode.SavePrevMode = ModeBtAudioPlay;
	}
#endif
#if(BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT)
	if(IdleMode.LeSinkModeFlag)
	{
		IdleMode.LeSinkModeFlag = 0;
		if(GetSysModeState(ModeLeAudioSinkPlay) == ModeStateSusend)
		{
			SetSysModeState(ModeLeAudioSinkPlay,ModeStateReady);
		}
		IdleMode.SavePrevMode = ModeLeAudioSinkPlay;
		//snk 启动
		CommonMsgProccess(MSG_LE_BIS_SNK_START);
	}
#endif
	osMutexLock(SysModeMutex);
	SysModeEnter(IdleMode.SavePrevMode);
	osMutexUnlock(SysModeMutex);
	APP_DBG("exit idle mode\n");
}

#else

void IdlePrevModeSet(SysModeNumber mode)
{
	mode = mode;
}

void IdleModeEnter(void)
{

}

void IdleModeExit(void)
{

}

#endif


