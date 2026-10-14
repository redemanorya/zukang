/**
 **************************************************************************************
 * @file    main_task.h
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
#ifndef __MAIN_TASK_H__
#define __MAIN_TASK_H__


#include "type.h"
#include "dac_interface.h"
#include "rtos_api.h"
#include "bt_config.h"
#include "audio_core_api.h"
#include "app_message.h"
#include "timeout.h"
#include "mode_task.h"
#include "mode_task_api.h"
//Port#include "flash_boot.h"
#ifdef SOFT_WACTH_DOG_ENABLE
#include "soft_watch_dog.h"
#endif


#define SoftFlagMask			0xFFFFFFFF

#ifdef CFG_RES_IR_NUMBERKEY
extern bool Number_select_flag;
extern uint16_t Number_value;
extern TIMER Number_selectTimer;
#endif

typedef struct _SysVolContext
{
	bool		MuteFlag;	//AudioCore软件mute
	int8_t	 	AudioSourceVol[AUDIO_CORE_SOURCE_MAX_NUM];	//Source增益控制step，小于等于32
	int8_t	 	AudioSinkVol[AUDIO_CORE_SINK_MAX_NUM];		//Sink增益控制step，小于等于32	
	
}SysVolContext;

typedef struct _MainAppContext
{
	xTaskHandle			taskHandle;
	MessageHandle		msgHandle;
	TaskState			state;

	SysModeNumber		SysCurrentMode;
	SysModeNumber		SysPrevMode;

/*************************mode common*************************************/
#ifdef CFG_RES_DAC_MONITOR_EN
	uint32_t			*DACFIFO;
	uint32_t			DACFIFO_LEN;
#endif

#ifdef CFG_APP_BIS_BT_DUAL_CHIP_MODE_EN
	uint32_t			*LE_AUDIO_I2S_RX_FIFO;
	uint32_t			LE_AUDIO_I2S_RX_FIFO_LEN;
#endif

#ifdef CFG_RES_AUDIO_DAC1_MIX_OUT_EN
	uint32_t			*DAC1FIFO;
	uint32_t			DAC1FIFO_LEN;
#endif

#ifdef CFG_RES_AUDIO_DAC2_MIX_OUT_EN
	uint32_t			*DAC2FIFO;
	uint32_t			DAC2FIFO_LEN;
#endif

#ifdef CFG_I2S0_MIX_OUT_EN
	uint32_t			*I2S0_MIX_TX_FIFO;
	uint32_t			I2S0_MIX_TX_FIFO_LEN;
#endif

#ifdef CFG_I2S0_MIX_IN_EN
	uint32_t			*I2S0_MIX_RX_FIFO;
	uint32_t			I2S0_MIX_RX_FIFO_LEN;
#endif

#ifdef CFG_I2S1_MIX_OUT_EN
	uint32_t			*I2S1_MIX_TX_FIFO;
	uint32_t			I2S1_MIX_TX_FIFO_LEN;
#endif
#ifdef CFG_I2S1_MIX_IN_EN
	uint32_t			*I2S1_MIX_RX_FIFO;
	uint32_t			I2S1_MIX_RX_FIFO_LEN;
#endif

#ifdef CFG_I2S2_MIX_OUT_EN
	uint32_t			*I2S2_MIX_TX_FIFO;
	uint32_t			I2S2_MIX_TX_FIFO_LEN;
#endif
#ifdef CFG_I2S2_MIX_IN_EN
	uint32_t			*I2S2_MIX_RX_FIFO;
	uint32_t			I2S2_MIX_RX_FIFO_LEN;
#endif

#ifdef CFG_RES_AUDIO_SPDIF0OUT_EN
	uint32_t			*SPDIF0_OUT_FIFO;
	uint32_t			SPDIF0_OUT_FIFO_LEN;
#endif

#ifdef CFG_RES_AUDIO_SPDIF1OUT_EN
	uint32_t			*SPDIF1_OUT_FIFO;
	uint32_t			SPDIF1_OUT_FIFO_LEN;
#endif

#if (CFG_RES_MIC_SELECT) && defined(CFG_FUNC_MIC_EN)
	uint32_t			*ADCFIFO;
#endif
/******************************************************************/
#ifdef CFG_FUNC_ALARM_EN
	uint32_t 			AlarmID;//闹钟ID对应bit位
	bool				AlarmFlag;
	bool				AlarmRemindStart;//闹铃提示音开启标志
	uint32_t 			AlarmRemindCnt;//闹铃提示音循环次数
	#ifdef CFG_FUNC_SNOOZE_EN
	bool				SnoozeOn;
	uint32_t 			SnoozeCnt;// 贪睡时间计数
	#endif
#endif
	SysVolContext		gSysVol;
#ifdef CFG_APP_BT_MODE_EN
    uint8_t     HfVolume;
#endif
#if defined(CFG_FUNC_ROBOEFFECT_EN)
	uint16_t	effect_flow_index;
#ifndef ROBOEFFECT_DOUBLE_ENGINE_EN
	uint16_t	effect_param_mode_index;
#else
	uint16_t	effect_param_mode_groupindex[2];
#endif
#ifdef CFG_FUNC_MUSIC_EQ_EN
	uint8_t     EqMode;
#endif

#ifdef CFG_FUNC_MIC_TREB_BASS_EN
	uint16_t	MicBassStep;
	uint16_t	MicTrebStep;
#endif
#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
	uint16_t	MusicBassStep;
	uint16_t	MusicTrebStep;
#endif
	uint16_t	ReverbStep;
#endif

#ifdef CFG_FUNC_SILENCE_AUTO_POWER_OFF_EN
	uint32_t    Silence_Power_Off_Time;
#endif

#ifdef  CFG_APP_HDMIIN_MODE_EN
	uint8_t  	hdmiArcOnFlg;
	uint8_t     hdmiSourceMuteFlg;
	uint8_t     hdmiResetFlg;
#endif

	uint32_t SoftwareFlag;	// soft flag register
	int16_t  MicVolume;
	int16_t  MusicVolume;
#ifdef CFG_ADC_LEVEL_KEY_EN
	int16_t MicVolumeBak;
#endif

#if BT_SOURCE_SUPPORT
	uint32_t 				*sbc_encode_pcm_buf;
	uint32_t				sbc_encode_pcm_buf_len;
	MCU_CIRCULAR_CONTEXT 	sbc_encode_pcm_context;
#endif

#if (BT_TWS_SUPPORT == ENABLE)
	bool        tws_device_init_flag;
#endif
}MainAppContext;

extern MainAppContext	mainAppCt;

enum _SYS_SOFTWARE_FLAG_
{
	SoftFlagNoRemind				=	BIT(0),	//提示音故障
	SoftFlagMediaDevicePlutOut 		= 	BIT(1),
	SoftFlagMvaInCard				=	BIT(2),	//文件预搜索发现SD卡有MVA包 卡拔除时清理
	SoftFlagMvaInUDisk				=	BIT(3),	//文件预搜索发现U盘有Mva包 U盘拔除时清理
	SoftFlagDiscDelayMask			=	BIT(4), //通话模式,蓝牙断开连接,延时播放提示音,即退回到每个模式时播放
	SoftFlagWaitBtRemindEnd			=	BIT(5), //标记来电时等待提示音播放完成再进入通话状态
	SoftFlagDelayEnterBtHf			=	BIT(6), //标记延时进入通话状态
	SoftFlagBtCurPlayStateMask		=	BIT(7), //标记来电时记录当前蓝牙播放的状态
	SoftFlagBtOtaUpgradeOK			=	BIT(8),

	SoftFlagIdleModeEnterSleep		=	BIT(9),//标记进入睡眠模式
	SoftFlagIdleModeEnterPowerDown	=	BIT(10),
	SoftFlagMediaModeRead			=	BIT(11),// 进入media mode Init/Read Fatfs外设
	SoftFlagMediaNextOrPrev			=	BIT(12),// 0:next 1:prev
	SoftFlagUpgradeOK				=	BIT(13),

	SoftFlagAudioCoreSourceIsDeInit	=	BIT(14),//AudioCoreSource资源已经被删除

	SoftFlagRecording				=	BIT(15),//录音进行标记，禁止后插先播，模式切换需清理
	//标记本次deepsleep消息是否来自于TV
	SoftFlagDeepSleepMsgIsFromTV 	=	BIT(16),
	//标记进入SoftPower
	SoftFlagIdleModeEnterSoftPower 	=	BIT(17),

	//标记切换音效需要重启当前模式
	SoftFlagSysCurModeReboot 		=	BIT(18),

	SoftFlagLeAudioSrcResume 		=	BIT(19),

	SoftFlagTwsRemind				=	BIT(20), //标记tws连接成功事件 等待unmute后提示音开播
	SoftFlagMvaInUDiskFs			=	BIT(21), //文件预搜索发现U盘有Mva包 U盘拔除时清理

	SoftFlagLeAudioSrcUnMuteFlag 	=	BIT(22),

	//标记本次唤醒源是否为CEC唤醒
	SoftFlagWakeUpSouceIsCEC 		= 	BIT(23),

	SoftFlagLeAudioSrcSync 			=	BIT(24),
	SoftFlagMediaModeExitTimeOut 	= 	BIT(25), //标记退出MediaMode退出超时
};

#define SoftFlagRegister(SoftEvent)			(mainAppCt.SoftwareFlag |= SoftEvent)
#define SoftFlagDeregister(SoftEvent)		(mainAppCt.SoftwareFlag &= ~SoftEvent)
#define SoftFlagGet(SoftEvent)				(mainAppCt.SoftwareFlag & SoftEvent ? TRUE : FALSE)

// SDK默认音量级数（音量实际级数由Flash preset表决定，运行时请用AudioVol_GetMaxLevel()查看最大音量等级）
#if (BT_AVRCP_VOLUME_SYNC) && defined(CFG_APP_BT_MODE_EN)
	#define CFG_PARA_SYS_VOLUME_DEFAULT			(12)
#else
	#define CFG_PARA_SYS_VOLUME_DEFAULT			(32)
#endif

int32_t MainAppTaskStart(void);
MessageHandle GetMainMessageHandle(void);
uint32_t GetSystemMode(void);

uint32_t IsBtAudioMode(void);

uint32_t IsIdleModeReady(void);
void PowerOffMessage(void);
void BatteryLowMessage(void);

//uint8_t Get_Resampler_Polyphase(int32_t resampler);
#endif /*__MAIN_TASK_H__*/

