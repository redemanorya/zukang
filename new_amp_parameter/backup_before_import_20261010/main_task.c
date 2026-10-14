/**
 **************************************************************************************
 * @file    main_task.c
 * @brief   Program Entry 
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2016-6-29 13:06:47$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
//driver、lib
#include "type.h"
#include <string.h>
#include "dma.h"
#include "timer.h"
#include "irqn.h"
#include "watchdog.h"
#include "sadc_interface.h"
#include "otg_detect.h"
#include "chip_info.h"
//#include "tempCorr.h"

//applicaiton
#include "app_config.h"
#include "bt_config.h"
#include "mvwireless2_config.h"
#include "main_task.h"
//#include "deepsleep.h"
#include "breakpoint.h"
#include "remind_sound.h"
#include "debug.h"
#include "audio_vol.h"
#include "device_detect.h"
#include "ctrlvars.h"
#include "shell.h"
#include "audio_core_service.h"
#include "audio_effect_control.h"
#include "bt_stack_service.h"
//Port#include "wireless_service.h"
//Port#include "bb_api.h"
#include "bt_manager.h"
//Port#include "bt_app_ddb_info.h"
#include "bt_hf_mode.h"
#include "idle_mode.h"
//Port#include "bt_app_connect.h"
//Port#include <components/soft_watchdog/soft_watch_dog.h>
//Port#include "can_func.h"
#include "bt_app_ddb_info.h"
#include "flash_interface.h"
#include "i2s_dpll_policy.h"

#ifdef DUAL_CORE_EN
#include "dualcore_api.h"
#include "dualcore_dsp.h"
#endif
#include "components/headphone/headphone.h"
#if (BT_TWS_SUPPORT == ENABLE)
#include "bt_tws_api.h"
#include "bt_app_tws.h"
#include "bt_tws_mode.h"
#include "bt_app_tws_connect.h"
extern void tws_master_mute_send(bool MuteFlag);
#endif

MainAppContext	mainAppCt;
#ifdef CFG_RES_IR_NUMBERKEY
bool Number_select_flag = 0;
uint16_t Number_value = 0;
TIMER Number_selectTimer;
#endif


extern void CtrlVarsInit(void);
extern void report_up_grate(void);
extern volatile uint32_t gInsertEventDelayActTimer;
#if FLASH_BOOT_EN
void start_up_grate(uint32_t UpdateResource);
#endif
extern void PowerOnModeGenerate(void *BpSysInfo);
#ifdef CFG_APP_HDMIIN_MODE_EN
extern void HDMI_CEC_ActivePowerOff(uint32_t timeout_value);
extern void	HDMI_CEC_DDC_Init(void);
#endif

#define MAIN_APP_TASK_STACK_SIZE		640//512//1024
#ifdef	CFG_FUNC_OPEN_SLOW_DEVICE_TASK
#define MAIN_APP_TASK_PRIO				4
#define MAIN_APP_MSG_TIMEOUT			10	
#else
#define MAIN_APP_TASK_PRIO				3
#define MAIN_APP_MSG_TIMEOUT			1	
#endif
#define MAIN_APP_TASK_SLEEP_PRIO		6 //进入deepsleep 需要相对其他task最高优先级。
#define MAIN_NUM_MESSAGE_QUEUE			20
#define SHELL_TASK_STACK_SIZE			512//1024
#define SHELL_TASK_PRIO					2

/****************************************
 * DMA CTRL 仅8通道，超出无效：
 * 常驻设备排前部,所有mode序号一致
 * *************************************/
static const uint8_t Dma0ChannelMap[16] = {
	MAP0_COMMON_FIRST
	/*在此新增独有设备,确保不超*/
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,//8 CH-default
};

static const uint8_t Dma1ChannelMap[16] = {
	MAP1_COMMON_FIRST
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,//8 CH-default
};

static void MainAppInit(void)
{
	memset(&mainAppCt, 0, sizeof(MainAppContext));
#if FLASH_BOOT_EN
    report_up_grate();
#endif
	mainAppCt.msgHandle = MessageRegister(MAIN_NUM_MESSAGE_QUEUE);
	mainAppCt.state = TaskStateCreating;
	mainAppCt.SysCurrentMode = ModeIdle;
	mainAppCt.SysPrevMode = ModeIdle;
}

// 断点参数安全上限，防止Flash数据损坏导致异常值
#define BP_PARAM_SAFE_MAX  32

static void SysVarInit(void)
{
	int16_t i;

#ifdef CFG_FUNC_BREAKPOINT_EN
	BP_SYS_INFO *pBpSysInfo = NULL;

	pBpSysInfo = (BP_SYS_INFO *)BP_GetInfo(BP_SYS_INFO_TYPE);

#if (BT_TWS_SUPPORT == ENABLE)
	if(pBpSysInfo->CurModuleId != ModeBtAudioPlay)
	{
		tws_delay = BT_TWS_DELAY_DEINIT;
	}
	else
	{
		tws_delay = BT_TWS_DELAY_BTMODE;
	}
#endif

#ifdef CFG_DMA_RGB_LED_EN
		SetRgbLedMode(pBpSysInfo->rgb_mode);
#endif

#ifdef CFG_FUNC_ROBOEFFECT_EN
	mainAppCt.effect_flow_index = pBpSysInfo->AudioEffectParambin_flow;
#ifndef ROBOEFFECT_DOUBLE_ENGINE_EN
	mainAppCt.effect_param_mode_index = pBpSysInfo->AudioEffectParambin_paramMode;
#else
	mainAppCt.effect_param_mode_groupindex[0] = pBpSysInfo->AudioEffectParambin_paramMode[0];
	mainAppCt.effect_param_mode_groupindex[1] = pBpSysInfo->AudioEffectParambin_paramMode[1];
#endif
	APP_DBG("flow_index:%d\n", mainAppCt.effect_flow_index);
#endif

	mainAppCt.MusicVolume = pBpSysInfo->MusicVolume;
	if((mainAppCt.MusicVolume > BP_PARAM_SAFE_MAX) || (mainAppCt.MusicVolume <= 0))
	{
		mainAppCt.MusicVolume = CFG_PARA_SYS_VOLUME_DEFAULT;
	}
	APP_DBG("MusicVolume:%d,%d\n", mainAppCt.MusicVolume, pBpSysInfo->MusicVolume);
	
	mainAppCt.MicVolume = pBpSysInfo->MicVolume;
	if((mainAppCt.MicVolume > BP_PARAM_SAFE_MAX) || (mainAppCt.MicVolume <= 0))
	{
		mainAppCt.MicVolume = CFG_PARA_SYS_VOLUME_DEFAULT;
	}
	APP_DBG("MicVolume:%d,%d\n", mainAppCt.MicVolume, pBpSysInfo->MicVolume);

	#ifdef CFG_APP_BT_MODE_EN
		mainAppCt.HfVolume = pBpSysInfo->HfVolume;
		if((mainAppCt.HfVolume > BP_PARAM_SAFE_MAX) || (mainAppCt.HfVolume <= 0))
		{
			mainAppCt.HfVolume = CFG_PARA_SYS_VOLUME_DEFAULT;
		}
		APP_DBG("HfVolume:%d,%d\n", mainAppCt.HfVolume, pBpSysInfo->HfVolume);
	#endif
	
	#ifdef CFG_FUNC_MUSIC_EQ_EN
		mainAppCt.EqMode = pBpSysInfo->EqMode;
		if(mainAppCt.EqMode > EQ_MODE_VOCAL_BOOST)
		{
			mainAppCt.EqMode = EQ_MODE_FLAT;
		}
	//	EqModeSet(mainAppCt.EqMode);
		#ifdef CFG_FUNC_EQMODE_FADIN_FADOUT_EN
		mainAppCt.EqModeBak = mainAppCt.EqMode;
		mainAppCt.EqModeFadeIn = 0;
		mainAppCt.eqSwitchFlag = 0;
		#endif
		APP_DBG("EqMode:%d,%d\n", mainAppCt.EqMode, pBpSysInfo->EqMode);
	#endif

#if defined(CFG_FUNC_ROBOEFFECT_EN)
		mainAppCt.ReverbStep = pBpSysInfo->ReverbStep;
		if((mainAppCt.ReverbStep > BP_PARAM_SAFE_MAX) || (mainAppCt.ReverbStep <= 0))
		{
			mainAppCt.ReverbStep = 12;
		}
		APP_DBG("ReverbStep:%d,%d\n", mainAppCt.ReverbStep, pBpSysInfo->ReverbStep);
#endif

	#ifdef CFG_FUNC_MIC_TREB_BASS_EN
		mainAppCt.MicBassStep = pBpSysInfo->MicBassStep;
		if((mainAppCt.MicBassStep > BP_PARAM_SAFE_MAX) || (mainAppCt.MicBassStep <= 0))
		{
			mainAppCt.MicBassStep = 8;
		}
		mainAppCt.MicTrebStep = pBpSysInfo->MicTrebStep;
		if((mainAppCt.MicTrebStep > BP_PARAM_SAFE_MAX) || (mainAppCt.MicTrebStep <= 0))
		{
			mainAppCt.MicTrebStep = 8;
		}
		APP_DBG("MicTrebStep:%d,%d\n", mainAppCt.MicTrebStep, pBpSysInfo->MicTrebStep);
		APP_DBG("MicBassStep:%d,%d\n", mainAppCt.MicBassStep, pBpSysInfo->MicBassStep);
	#endif
	#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
		mainAppCt.MusicBassStep = pBpSysInfo->MusicBassStep;
		if((mainAppCt.MusicBassStep > BP_PARAM_SAFE_MAX) || (mainAppCt.MusicBassStep <= 0))
		{
			mainAppCt.MusicBassStep = 8;
		}
		mainAppCt.MusicTrebStep = pBpSysInfo->MusicTrebStep;
		if((mainAppCt.MusicTrebStep > BP_PARAM_SAFE_MAX) || (mainAppCt.MusicTrebStep <= 0))
		{
			mainAppCt.MusicTrebStep = 8;
		}
		APP_DBG("MusicTrebStep:%d,%d\n", mainAppCt.MusicTrebStep, pBpSysInfo->MusicTrebStep);
		APP_DBG("MusicBassStep:%d,%d\n", mainAppCt.MusicBassStep, pBpSysInfo->MusicBassStep);
	#endif

#else //no breakpoint
	//mainAppCt.appBackupMode = ModeBtAudioPlay;		  
		mainAppCt.MusicVolume = CFG_PARA_SYS_VOLUME_DEFAULT;
	#ifdef CFG_APP_BT_MODE_EN
		mainAppCt.HfVolume = CFG_PARA_SYS_VOLUME_DEFAULT;
	#endif
		mainAppCt.MicVolume = CFG_PARA_SYS_VOLUME_DEFAULT;
	
	#ifdef CFG_FUNC_MUSIC_EQ_EN
		mainAppCt.EqMode = EQ_MODE_FLAT;
		#ifdef CFG_FUNC_EQMODE_FADIN_FADOUT_EN
		mainAppCt.EqModeBak = mainAppCt.EqMode;
		mainAppCt.eqSwitchFlag = 0;
		#endif
	#endif
	#ifdef CFG_FUNC_ROBOEFFECT_EN
		mainAppCt.ReverbStep = 12;
	#endif
	#ifdef CFG_FUNC_MIC_TREB_BASS_EN
		mainAppCt.MicBassStep = 8;
		mainAppCt.MicTrebStep = 8;
	#endif
	#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
		mainAppCt.MusicBassStep = 8;
		mainAppCt.MusicTrebStep = 8;
	#endif

#endif//#ifdef CFG_FUNC_BREAKPOINT_EN

#ifdef CFG_APP_HDMIIN_MODE_EN
	mainAppCt.hdmiArcOnFlg = 1;
	mainAppCt.hdmiResetFlg = 0;
#endif

#if defined(CFG_APP_BT_MODE_EN) && defined(CFG_WIRELESS_OUT_ON) //CFG_WIRELESS_EN
	SetSysModeState(ModeBtAudioPlay,ModeStateSusend);
#endif

#ifdef CFG_FUNC_BREAKPOINT_EN
#ifdef CFG_APP_BT_MODE_EN
	if(SoftFlagGet(SoftFlagUpgradeOK))
	{
		pBpSysInfo->CurModuleId = ModeBtAudioPlay;
		DBG("u-disk or sd upgrade ok ,set mode to ModeBtAudioPlay \n");
	}
#endif
	PowerOnModeGenerate((void *)pBpSysInfo);
#else
	PowerOnModeGenerate(NULL);

#endif

#ifdef CFG_APP_LINEIN_MODE_EN
	/* [MOD] 这一段原先位于上面 #ifdef CFG_FUNC_BREAKPOINT_EN 的 #else 分支内,
	 * 而 CFG_FUNC_BREAKPOINT_EN 是打开的(app_config.h), 整段被预处理掉,
	 * 导致"开机强制 LineIn"实际不生效, 开机模式由断点记忆(或 SDK 默认 BT)决定。
	 * 移到 #endif 之后, 两种断点配置下都会执行。 */
	// ---- [AUX] force boot into LinePlay ----------------
	// PowerOnModeGenerate() 会把 Idle 置为 ModeStateInit,
	// 这里再复位一次, 让 SysModeInit 同时 Init Idle 和 LinePlay
	SetSysModeState(ModeIdle, ModeStateReady);
	SetSysModeState(ModeBtAudioPlay, ModeStateReady);
	SetSysModeState(ModeLineAudioPlay, ModeStateInit);
	mainAppCt.SysCurrentMode = ModeLineAudioPlay;
	mainAppCt.SysPrevMode = ModeLineAudioPlay;
	APP_DBG("!!!!!!!!!!force power on mode LinePlay!!!!!!!!!!\n");
#endif


    for(i = 0; i < AUDIO_CORE_SINK_MAX_NUM; i++)
	{
		mainAppCt.gSysVol.AudioSinkVol[i] = CFG_PARA_SYS_VOLUME_DEFAULT;
	}

	for(i = 0; i < AUDIO_CORE_SOURCE_MAX_NUM; i++)
	{
		if(i == MIC_SOURCE_NUM)
		{
			mainAppCt.gSysVol.AudioSourceVol[MIC_SOURCE_NUM] = mainAppCt.MicVolume;
		}
		else if(i == APP_SOURCE_NUM)
		{
			mainAppCt.gSysVol.AudioSourceVol[APP_SOURCE_NUM] = mainAppCt.MusicVolume;
		}
#ifdef CFG_FUNC_REMIND_SOUND_EN
		else if(i == REMIND_SOURCE_NUM)
		{
			#if CFG_PARAM_FIXED_REMIND_VOL
			mainAppCt.gSysVol.AudioSourceVol[REMIND_SOURCE_NUM] = CFG_PARAM_FIXED_REMIND_VOL;
			#else
			mainAppCt.gSysVol.AudioSourceVol[REMIND_SOURCE_NUM] = mainAppCt.MusicVolume;
			#endif
		}
#endif
		else
		{
			mainAppCt.gSysVol.AudioSourceVol[i] = CFG_PARA_SYS_VOLUME_DEFAULT;		
		}
	}
	mainAppCt.gSysVol.MuteFlag = TRUE;	
	
	#ifdef CFG_FUNC_SILENCE_AUTO_POWER_OFF_EN
	mainAppCt.Silence_Power_Off_Time = 0;
	#endif
}

void SystemTimerInit(void)
{
 	Timer_Config(TIMER2,1000,0);
 	Timer_Start(TIMER2);
 	Timer_InterruptFlagClear(TIMER2, UPDATE_INTERRUPT_SRC);
	NVIC_EnableIRQ(TMR2_IRQn);
}

static void SystemInit(void)
{
#ifndef LOSSLESS_DECODER_HIGH_RESOLUTION
	//开启高采样率解码，去掉U盘读数据API里面的延迟
	DelayMsFunc = (DelayMsFunction)osTaskDelay; //提高Os条件下驱动层延时函数精度，非OS默认使用DelayMs
#endif
	DMA_ChannelAllocTableSet(DMA0, (uint8_t* )Dma0ChannelMap);
	DMA_ChannelAllocTableSet(DMA1, (uint8_t* )Dma1ChannelMap);

#ifdef CFG_DUMP_DEBUG_EN
	DumpUartConfig(TRUE);
#endif
	SarADC_Init();
	TemperatureCorrInit();

 	SystemTimerInit();
#ifdef DUAL_CORE_EN
 	IsCore2Ready();
#endif
#ifdef CFG_FUNC_BREAKPOINT_EN
 	BP_LoadInfo();
#endif

//#if defined(CFG_FUNC_ROBOEFFECT_EN) && defined(CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH)
// 	OTG_PortSetDetectMode(0,1);
// 	//para版本没有这个API 待处理AudioEffect_CheckFlashEffectParam();
//#endif

	SysVarInit();

	// ---- [AUX] AMP init (single TWE only, low-freq WF not installed) ----
	GPIO_PortBModeSet(GPIOB2, 0);   // GPIOB2: amp power supply
	GPIO_RegOneBitSet(GPIO_B_OE, GPIOB2);
	GPIO_RegOneBitSet(GPIO_B_OUT, GPIOB2);
	amp_driver_init();
	ACM8625_Mute(0);   // [AUX] unmute TWE amp (was never unmuted)
	ACM8625_VolSet(16); // set TWE vol 0dB

#ifdef CFG_FUNC_HEADPHONE_EN
	HeadphoneInit();   // 耳机初始化
#endif
#ifdef CFG_FUNC_OXS_PROTOCOL_EN
	RL_ProtocolInit();
#endif
#ifdef FUNC_WIRELESS2_SUPPORT
	WirelessInit();
#endif
	
#ifdef CFG_FUNC_BT_OTA_EN
	SoftFlagDeregister((~(SoftFlagUpgradeOK|SoftFlagBtOtaUpgradeOK))&SoftFlagMask);
#else
	SoftFlagDeregister((~SoftFlagUpgradeOK)&SoftFlagMask);
#endif

#if (BT_TWS_SUPPORT == ENABLE)
#ifndef TWS_AUDIO_BUFFER_DYNAMIC_MALLOC
	TWS_Params_Init();
#else
	TWS_TwsEnable(0);
#endif
#endif	

#ifdef CFG_APP_HDMIIN_MODE_EN
	HDMI_CEC_DDC_Init();
#endif
	///////////////////////////////AudioCore/////////////////////////////////////////
	memset((AudioCoreContext*)&AudioCore, 0, sizeof(AudioCoreContext));
#ifdef CFG_FUNC_ROBOEFFECT_EN
	//memset((AudioEffectContext*)&AudioEffect, 0, sizeof(AudioEffectContext));
#endif
	CtrlVarsInit();//音频系统硬件变量初始化，系统变量初始化
#ifdef CFG_FUNC_I2S_DPLL_POLICY_EN
	I2SDpllPolicy_Init();
#endif

#if CFG_PARA_HOST_FS_DET || CFG_PARA_DEVICE_FS_DET
	//For OTG FS check
	OTG_FS_PortSetDetectMode(CFG_PARA_HOST_FS_DET, CFG_PARA_DEVICE_FS_DET);
#endif
#if CFG_PARA_HOST_HS_DET || CFG_PARA_DEVICE_HS_DET
	//For OTG HS check
	OTG_HS_PortSetDetectMode(CFG_PARA_HOST_HS_DET, CFG_PARA_DEVICE_HS_DET);
	NVIC_EnableIRQ(USB_HS_IRQn);
#endif
#if (defined(CFG_COMMUNICATION_BY_FS_USB)||defined(CFG_COMMUNICATION_BY_HS_USB)) && (!(CFG_PARA_HOST_FS_DET || CFG_PARA_DEVICE_FS_DET) || !(CFG_PARA_HOST_HS_DET || CFG_PARA_DEVICE_HS_DET))
	void UsbHidHardwareint(void);
	UsbHidHardwareint();//Only HID & no detect & no host
#endif

	AudioCoreServiceCreate(mainAppCt.msgHandle);
#ifdef CFG_FUNC_REMIND_SOUND_EN	
	RemindSoundInit();
#endif

#ifdef CFG_FUNC_ALARM_EN
	mainAppCt.AlarmFlag = FALSE;
#endif

#if defined(CFG_WIRELESS_EN) && defined(CFG_WIRELESS_OUT_ON)
	DBG("Wireless stack @pwr on!\n");
	WirelessServiceStart();
#elif defined(CFG_APP_BT_MODE_EN)
	//将蓝牙任务创建移至此处,以便优先申请协议栈使用的内存空间,不影响其他的任务; 开机睡眠时，蓝牙stack再次开，避免上电马上退出。
	if(sys_parameter.bt_BackgroundType != BT_BACKGROUND_DISABLE)
		BtStackServiceStart();//蓝牙设备驱动serivce 启动失败时，目前是挂起，无同步消息。
//IRKeyInit();//clk源被改？
#endif

#ifdef CFG_FUNC_CAN_DEMO_EN
	CAN_FuncInit();
#endif
	DeviceServiceInit();
#ifdef CFG_APP_BLE_MODE_EN
	BlePowerOn();
#endif
	APP_DBG("MainApp:run\n");
}

static void PublicDetect(void)
{
#ifdef CFG_FUNC_SILENCE_AUTO_POWER_OFF_EN
		switch(GetSystemMode())
		{
			// Idle,Slave,HFP,USB Audio不省电关机
			case ModeIdle:
			case ModeTwsSlavePlay:
			case ModeBtHfPlay:
			case ModeUsbDevicePlay:
			case ModeUsbFsDevicePlay:
			mainAppCt.Silence_Power_Off_Time = 0;
			break;

			// BT 连上蓝牙不关机
			case ModeBtAudioPlay:
			if(btManager.btLinkState)
				mainAppCt.Silence_Power_Off_Time = 0;
			break;

			default:
			break;
		}

        if(mainAppCt.Silence_Power_Off_Time >= SILENCE_POWER_OFF_DELAY_TIME)
		{
			mainAppCt.Silence_Power_Off_Time = 0;
			APP_DBG("Silence Power Off!!\n");
			PowerOffMessage();
        }
#endif
}

static void PublicMsgPross(MessageContext msg)
{
	switch(msg.msgId)
	{
		case MSG_AUDIO_CORE_SERVICE_CREATED:	
			APP_DBG("MainApp:AudioCore service created\n");
			AudioCoreServiceStart();
			mainAppCt.state = TaskStateReady;
			break;
		
		case MSG_AUDIO_CORE_SERVICE_STARTED:
			APP_DBG("MainApp:AudioCore service started\n");
			SysModeTaskCreat();
#ifdef FUNC_WIRELESS2_SUPPORT
			Wireless2ModeTaskCreat();
#endif
	#ifdef	CFG_FUNC_OPEN_SLOW_DEVICE_TASK
			{
				extern	void CreatSlowDeviceTask(void);
				CreatSlowDeviceTask();
			}
	#endif
	#if (BT_LE_AUDIO_SUPPORT)
			{
				extern void CreatLeAudioServiceTask(void);
				CreatLeAudioServiceTask();
			}
	#endif
			mainAppCt.state = TaskStateRunning;
			break;
#if FLASH_BOOT_EN
		case MSG_UPDATE:
			//if(SoftFlagGet(SoftFlagUpgradeOK))break;
		#ifdef CFG_FUNC_BT_OTA_EN
			//检查flashboot的BLE名称是否和当前一致，不一致则需要先更新BLE名称，再进入升级流程
			{
				extern BT_CONFIGURATION_PARAMS		*btStackConfigParams;
				BT_CONFIGURATION_PARAMS paramsCheck;
				SPI_FLASH_ERR_CODE ret=0;

				//1.read ble name from last 4k erea
				memset(&paramsCheck, 0, sizeof(BT_CONFIGURATION_PARAMS));
				ret = BtDdb_LoadLast4KBtConfigParams(&paramsCheck);
				//ret = SPiFlashReadData(BTDB_CONFIG_ADDR, (uint8_t*)&paramsCheck, sizeof(BT_CONFIGURATION_PARAMS));
				printf("flash:ble name: %s\n", paramsCheck.ble_LocalDeviceName);
				printf("local:ble name: %s\n", btStackConfigParams->ble_LocalDeviceName);

				//2.compare with current name, if not same, update it and wait for flash write done, then start bt ota grate.
				if(memcmp(paramsCheck.ble_LocalDeviceName, btStackConfigParams->ble_LocalDeviceName, BLE_NAME_SIZE) != 0)
				{
					APP_DBG("MainApp:BLE name need update for bt ota grate\n");					
#ifdef CFG_FUNC_OPEN_SLOW_DEVICE_TASK
					//send msg to slow task to update ble name, and delay to wait for the msg processed, then start bt ota grate.
					SlowDevice_MsgSend(MSG_DEVICE_BT_BLE_OTA_NAME_WRITE);
					osTaskDelay(500);
#endif
				}
				else
				{
					APP_DBG("MainApp:BLE name no need update for bt ota grate\n");
					osTaskDelay(100);
				}

			}
			start_up_grate(SysResourceBtOTA);
		#endif
			#ifdef FUNC_UPDATE_CONTROL
			APP_DBG("MainApp:UPDATE MSG\n");
			//设备经过播放，预搜索mva包登记，可升级。拔出时取消登记。
			if(SoftFlagGet(SoftFlagMvaInCard) && GetSystemMode() == ModeCardAudioPlay)
			{
				APP_DBG("MainApp:updata file exist in Card\n");
				start_up_grate(SysResourceCard);
			}
			else if(SoftFlagGet(SoftFlagMvaInUDisk) && GetSystemMode() == ModeUDiskAudioPlay)
			{
				APP_DBG("MainApp:updata file exist in Udisk\n");
				start_up_grate(SysResourceUDiskFs);
			}
			else if(SoftFlagGet(SoftFlagMvaInUDisk) && GetSystemMode() == ModeUDiskHsAudioPlay)
			{
				APP_DBG("MainApp:updata file exist in Udisk\n");
				start_up_grate(SysResourceUDisk);
			}
			#endif
			break;
#endif		

#ifdef CFG_APP_IDLE_MODE_EN		
		case MSG_POWER:
		case MSG_POWERDOWN:
		case MSG_DEEPSLEEP:
#ifdef CFG_SOFT_POWER_KEY_EN
		case MSG_SOFT_POWER:
#endif
#ifdef CFG_APP_HDMIIN_MODE_EN
			if(GetSystemMode() == ModeHdmiAudioPlay)
			{
				if(SoftFlagGet(SoftFlagDeepSleepMsgIsFromTV) == 0)//非电视端发来的关机
				{
					HDMI_CEC_ActivePowerOff(200);
				}
				SoftFlagDeregister(SoftFlagDeepSleepMsgIsFromTV);
			}
#endif
			if(GetSystemMode() != ModeIdle)
			{			
				if (msg.msgId == MSG_POWER){
					DBG("Main task MSG_POWER\n");
				}else if (msg.msgId == MSG_POWERDOWN){
					DBG("Main task MSG_POWERDOWN\n");
				}else if (msg.msgId == MSG_DEEPSLEEP){
					DBG("Main task MSG_DEEPSLEEP\n");
				}

#if (BT_LE_AUDIO_SUPPORT)
			if (!check_bis_src_is_in_idle()) 
			{
				//BIS SRC is not allowed to enter deep sleep while working
				break;
			}

			if (check_bis_sink_state())
			{
				//BIS SINK is not allowed to enter deep sleep while working
				break;
			}
#endif	
				SendEnterIdleModeMsg();
			#if (defined(CFG_APP_BT_MODE_EN) && (BT_HFP_SUPPORT))
				if(GetSystemMode() == ModeBtHfPlay)
				{
					BtHfModeExit();				
				}
				extern void SetsBtHfModeEnterFlag(uint32_t flag);
				SetsBtHfModeEnterFlag(0);
			#endif		
			
			#if BT_TWS_SUPPORT
				BtStackServiceMsgSend(MSG_BT_STACK_TWS_PAIRING_STOP);
				BtReconnectTwsStop();

				if(btManager.twsState == BT_TWS_STATE_CONNECTED)
				{
			#ifdef TWS_POWEROFF_MODE_SYNC
					if (btManager.twsRole == BT_TWS_MASTER)
					{
						void tws_send_key_msg(uint16_t msg);
						tws_send_key_msg(msg.msgId);
						osTaskDelay(10);
						btManager.twsState = BT_TWS_STATE_NONE; 	
					}
			#else
					tws_link_disconnect();
			#endif
				}
			#endif

			#ifdef	CFG_IDLE_MODE_POWER_KEY
				if(msg.msgId == MSG_POWERDOWN)
				{
					SoftFlagRegister(SoftFlagIdleModeEnterPowerDown);
				}
			#endif				
			
			#ifdef CFG_IDLE_MODE_DEEP_SLEEP
				if(msg.msgId == MSG_DEEPSLEEP)
				{					
					SoftFlagRegister(SoftFlagIdleModeEnterSleep);
				}
			#endif	
			#ifdef CFG_SOFT_POWER_KEY_EN
				if(msg.msgId == MSG_SOFT_POWER)
				{
					SoftFlagRegister(SoftFlagIdleModeEnterSoftPower);
				}
			#endif
			}
			break;
#else
#ifdef CFG_SOFT_POWER_KEY_EN
		case MSG_SOFT_POWER:
			SoftKeyPowerOff();
			break;
#endif
#endif

#ifdef CFG_APP_BT_MODE_EN
		case MSG_BT_ENTER_DUT_MODE:
			BtEnterDutModeFunc();
			break;

		case MSG_BTSTACK_DEEPSLEEP:
			APP_DBG("MSG_BTSTACK_DEEPSLEEP\n");
			break;

		case MSG_BTSTACK_BB_ERROR:
			APP_DBG("bb and bt stack reset\n");
			BtResetAndKill(FALSE);
			//reset bb and bt stack
			BtStackServiceStart();

			//发起回连
			BtStackServiceMsgSend(MSG_BTSTACK_BB_ERROR_RESTART);

			break;
#ifdef CFG_FUNC_BT_OTA_EN
		case MSG_BT_START_OTA:
			APP_DBG("\nMSG_BT_START_OTA\n");
			start_up_grate(SysResourceBtOTA);
			break;
#endif
#if (BT_HFP_SUPPORT)
			case MSG_DEVICE_SERVICE_ENTER_BTHF_MODE:
			if(GetHfpState(BtCurIndex_Get()) >= BT_HFP_STATE_CONNECTED)
			{
				BtHfModeEnter();
			}
			break;
#endif
#ifdef BT_AUTO_ENTER_PLAY_MODE
		case MSG_BT_A2DP_STREAMING:
			//播放歌曲时,有模式切换需求,则在此消息中开始进行模式切换操作
			if((GetSystemMode() != ModeBtAudioPlay)&&(GetSystemMode() != ModeBtHfPlay))
			{
				MessageContext		msgSend;
				
				APP_DBG("Enter Bt Audio Play Mode...\n");
				//ResourceRegister(AppResourceBtPlay);
				
				// Send message to main app
				msgSend.msgId		= MSG_DEVICE_SERVICE_BTPLAY_IN;
				MessageSend(GetMainMessageHandle(), &msgSend);
			}
			break;
#endif
#endif//#ifdef CFG_APP_BT_MODE_EN
#if defined(CFG_WIRELESS_EN) && (!defined(CFG_WIRELESS_OUT_ON) || defined(CFG_RF_TO_BT))
		case MSG_WIRELESS://stack  create
			if(GetWirelessServiceTaskHandle() == NULL)
			{
	#ifdef CFG_APP_BT_MODE_EN
				APP_DBG("bt stack reset\n");
				BtResetAndKill(FALSE);
	#endif
				DBG("Wireless stack!\n");
				WirelessServiceStart();
			}
			break;

		case MSG_WIRELESS_EXIT: //stack exit
			DBG("MSG_WIRELESS_EXIT\n");
			if(WirelessRFInited())
			{
				WirelessServiceKill();
				MessageContext		newmsg;
				newmsg.msgId = MSG_WIRELESS_EXITED;
				MessageSend(mainAppCt.msgHandle, &newmsg);
			}
	#ifdef CFG_APP_BT_MODE_EN
			if(sys_parameter.bt_BackgroundType != BT_BACKGROUND_DISABLE)
			{
				//reset bb and bt stack
				BtStackServiceStart();
				BtStackServiceMsgSend(MSG_BTSTACK_RUN_START);
			#ifdef 	CFG_RF_TO_BT
				if(GetSysModeState(ModeBtAudioPlay) == ModeStateSusend)
				{
					SetSysModeState(ModeBtAudioPlay,ModeStateReady);
				}
			#endif
			}
	#endif
		break;

		case MSG_WIRELESS_EXITED://maybe switch to btplay
			DBG("MSG_WIRELESS_EXITED\n");
	#if defined(CFG_RF_TO_BT)
			MessageContext		newmsg1;
			newmsg1.msgId = MSG_BT_ENTER_DUT_MODE;
			MessageSend(mainAppCt.msgHandle, &newmsg1);
	#endif
			break;
#endif//CFG_WIRELESS_EN

		default:
#if defined(CFG_WIRELESS_EN)
			if(msg.msgId > MSG_WIRELESS_EVENT && msg.msgId < MSG_WIRELESS_EVENT_END)
			{
				WirelessServiceMsgSend(msg.msgId);
			}
#endif
#if	BT_SOURCE_SUPPORT
			BtSourcePublicMsgPross(msg.msgId);
#endif
			break;
	}		
	
#if (BT_TWS_SUPPORT == ENABLE)
	tws_msg_process(msg.msgId);
#endif
}

static VMID_TEMP_TYPE TemperatureMonitor(void)
{
	static uint8_t temp_state = VMID_TEMP_NORMAL;
	static uint8_t temp_buf[10] = {0};
	static uint8_t temp_idx = 0;
	static uint8_t temp_cnt = 0;
	uint16_t temp_sum = 0;
	uint8_t temp_cur;

	temp_cur = TemperatureGet();

	if (temp_cnt < 3)
	{
		/* 冷启动：前3次连续采样 */
		temp_buf[temp_idx++] = temp_cur;
		temp_cnt++;
		if (temp_cnt == 3)
		{
			/* 3次采完，用均值填充整个缓冲区 */
			uint16_t temp_fill = ((uint16_t)temp_buf[0] + temp_buf[1] + temp_buf[2]) / 3;
			for (int i = 0; i < 10; i++)
				temp_buf[i] = (uint8_t)temp_fill;
			temp_idx = 0;
			temp_cnt = 10;
		}
	}
	else
	{
		/* 正常滑动窗口：每次更新一个位置 */
		temp_buf[temp_idx] = temp_cur;
		temp_idx = (temp_idx + 1) % 10;
	}

	/* 求和：就绪后遍历全部10个，否则用已采集样本数 */
	{
		uint8_t n = (temp_cnt == 10) ? 10 : temp_cnt;
		for (int i = 0; i < n; i++)
			temp_sum += temp_buf[i];
		uint16_t temp_avg = temp_sum / n;

		if (temp_avg >= 60)
			temp_state = VMID_TEMP_HIGH;
		else if (temp_avg < 50)
			temp_state = VMID_TEMP_NORMAL;
	}

	return temp_state;
}

static void MainAppTaskEntrance(void * param)
{
	MessageContext		msg;
	SystemInit();

#ifdef CFG_DMA_RGB_LED_EN
	Led_Dma_Proc_Init();
#endif
#ifdef SOFT_WACTH_DOG_ENABLE
	little_dog_adopt(DOG_INDEX0_MainTask);
#endif

	TIMER change_cnt;
	TimeOutSet(&change_cnt, 3);
	while(1)
	{
		if(IsTimeOut(&change_cnt))
		{
			vmid_Dynamic_change(TemperatureMonitor());
			TimeOutSet(&change_cnt, 3);
		}

		MessageRecv(mainAppCt.msgHandle, &msg, MAIN_APP_MSG_TIMEOUT);//
		PublicDetect();
		PublicMsgPross(msg);
#ifdef CFG_FUNC_I2S_DPLL_POLICY_EN
		I2SDpllPolicy_Process();
#endif
#ifdef MVAPP_UPDATA_EN
		extern void upgrade_end_reboot();
		upgrade_end_reboot();
		extern void upgrade_monitor(void);
		upgrade_monitor();
#endif

#ifdef CFG_FUNC_HEADPHONE_EN
		HeadphoneTaskPoll();
#endif
#ifdef CFG_FUNC_OXS_PROTOCOL_EN
		RL_ProtocolPoll();
#endif
#if defined(CFG_FUNC_USBDEBUG_EN) && defined(CFG_COMMUNICATION_BY_FS_USB)
		{
			extern void UsbHidLogPoll(void);
			UsbHidLogPoll();
		}
#endif

#ifdef SOFT_WACTH_DOG_ENABLE
		little_dog_feed(DOG_INDEX0_MainTask);
#endif
		WDG0_Feed();

		#ifdef AUTO_TEST_ENABLE
		extern void AutoTestMain(uint16_t test_msg);
		AutoTestMain(msg.msgId);
		#endif
#ifdef CFG_FUNC_USB_HOST_AUDIO_MIX_MODE
		extern void UsbHostHidDateProcess(void);
		UsbHostHidDateProcess();
#endif
		if(mainAppCt.state == TaskStateRunning)
		{
			DeviceServicePocess(msg.msgId);
			if(msg.msgId != MSG_NONE)
			{
				SysModeGenerate(msg.msgId);
				extern bool AudioEffect_Msg_Check(uint16_t *Msg);
				if(!AudioEffect_Msg_Check(&msg.msgId))
				{
					MessageSend(GetSysModeMsgHandle(), &msg);
				}
			}
			SysModeChangeTimeoutProcess();
		}
#ifdef LOG_ANALYSIS_DBG_SWITCH
        GPIOX_TOGGLE(3, 1);
        GPIOX_TOGGLE(5, 3);
#endif
	}
}

xTaskHandle GetMainAppTaskHandle(void)
{
	return mainAppCt.taskHandle;
}

/***************************************************************************************
 *
 * APIs
 *
 */
int32_t MainAppTaskStart(void)
{
	MainAppInit();
#ifdef CFG_FUNC_SHELL_EN
	shell_init();
	xTaskCreate(mv_shell_task, "SHELL", SHELL_TASK_STACK_SIZE, NULL, SHELL_TASK_PRIO, NULL);
#endif
	xTaskCreate(MainAppTaskEntrance, "MainApp", MAIN_APP_TASK_STACK_SIZE, NULL, MAIN_APP_TASK_PRIO, &mainAppCt.taskHandle);
	return 0;
}

MessageHandle GetMainMessageHandle(void)
{
	return mainAppCt.msgHandle;
}


uint32_t GetSystemMode(void)
{
	return mainAppCt.SysCurrentMode;
}

void SamplesFrameUpdataMsg(void)//发现帧变化，发送消息
{
	MessageContext		msgSend;
	APP_DBG("SamplesFrameUpdataMsg\n");

	msgSend.msgId		= MSG_AUDIO_CORE_FRAME_SIZE_CHANGE;
    MessageSend(mainAppCt.msgHandle, &msgSend);
}

void EffectUpdataMsg(void)
{
	MessageContext		msgSend;
	APP_DBG("EffectUpdataMsg\n");

	msgSend.msgId		= MSG_AUDIO_CORE_EFFECT_CHANGE;
	MessageSend(mainAppCt.msgHandle, &msgSend);
}

uint32_t IsBtAudioMode(void)
{
	return (GetSysModeState(ModeBtAudioPlay) == ModeStateRunning);
}

uint32_t IsBtHfMode(void)
{
	return (GetSysModeState(ModeBtHfPlay) == ModeStateRunning);
}

#if (BT_TWS_SUPPORT == ENABLE)
uint32_t IsBtTwsSlaveMode(void)
{
	return (GetSysModeState(ModeTwsSlavePlay) == ModeStateInit || GetSysModeState(ModeTwsSlavePlay) == ModeStateRunning);
}
#endif

uint32_t IsIdleModeReady(void)
{
	if(GetModeDefineState(ModeIdle))
	{
		if(GetSysModeState(ModeIdle) == ModeStateInit || GetSysModeState(ModeIdle) == ModeStateRunning )
			return 1;
	}
	return 0;
}


void PowerOffMessage(void)
{
	MessageContext		msgSend;

#if	defined(CFG_IDLE_MODE_POWER_KEY) && (POWERKEY_MODE == POWERKEY_MODE_PUSH_BUTTON)
	msgSend.msgId = MSG_POWERDOWN;
	APP_DBG("msgSend.msgId = MSG_DEVICE_SERVICE_POWERDOWN\n");
#elif defined(CFG_SOFT_POWER_KEY_EN)
	msgSend.msgId = MSG_SOFT_POWER;
	APP_DBG("msgSend.msgId = MSG_DEVICE_SERVICE_SOFT_POWEROFF\n");
#elif	defined(CFG_IDLE_MODE_DEEP_SLEEP) 
	msgSend.msgId = MSG_DEEPSLEEP;
	APP_DBG("msgSend.msgId = MSG_DEVICE_SERVICE_DEEPSLEEP\n");
#else
	msgSend.msgId = MSG_POWER;
	APP_DBG("msgSend.msgId = MSG_POWER\n");
#endif

	MessageSend(GetMainMessageHandle(), &msgSend);
}

void BatteryLowMessage(void)
{
	MessageContext		msgSend;

	APP_DBG("msgSend.msgId = MSG_DEVICE_SERVICE_BATTERY_LOW\n");
	msgSend.msgId = MSG_DEVICE_SERVICE_BATTERY_LOW;
	MessageSend(GetMainMessageHandle(), &msgSend);
}
