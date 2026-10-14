/**
 **************************************************************************************
 * @file    optical_mode.c
 * @brief
 *
 * @author  Yangyu
 * @version V1.0.0
 *
 * $Created: 2026-1-26$
 *
 * @Copyright (C) 2026, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include <string.h>
#include "type.h"
#include "irqn.h"
#include "gpio.h"
#include "dma.h"
#include "rtos_api.h"
#include "app_message.h"
#include "debug.h"
#include "delay.h"
#include "audio_adc.h"
#include "dac.h"
#include "adc_interface.h"
#include "dac_interface.h"
#include "clk.h"
#include "audio_core_api.h"
#include "audio_core_service.h"
#include "audio_effect.h"
#include "powercontroller.h"
//#include "deepsleep.h"
#include "resampler_polyphase.h"
#include "mcu_circular_buf.h"
#include "breakpoint.h"
#include "main_task.h"
#include "remind_sound_item.h"
#include "mode_task_api.h"
#include "remind_sound.h"
#include "audio_vol.h"
#include "ctrlvars.h"
#include "reset.h"
#include "spdif_api.h"
#ifdef DUAL_CORE_EN
#include "dualcore_api.h"
#include "dualcore_dsp.h"
#endif

#ifdef CFG_APP_OPTICAL_MODE_EN
/****************************************
 * DMA CTRL 仅8通道，超出无效：
 * 常驻设备排前部,所有mode序号一致
 * *************************************/
static const uint8_t Dma0ChannelMap[16] = {
	MAP0_COMMON_FIRST
#if defined(CFG_SPDIF_OPTICAL_MODULE) && (CFG_SPDIF_OPTICAL_MODULE == SPDIF_0)
	CFG_SPDIF_OPTICAL_RX_DMA_CHANNEL,
#endif
	/*在此新增独有设备,确保不超*/
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,//8 CH-default
};

static const uint8_t Dma1ChannelMap[16] = {
	MAP1_COMMON_FIRST
#ifdef CFG_APP_I2SIN_MODE_EN
	PERIPHERAL_1_ID_I2S0_RX + 2 * CFG_RES_I2S_MODULE,
#endif
#if defined(CFG_SPDIF_OPTICAL_MODULE) && (CFG_SPDIF_OPTICAL_MODULE == SPDIF_1)
	CFG_SPDIF_OPTICAL_RX_DMA_CHANNEL,
#endif
	/*在此新增独有设备,确保不超*/
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,//8 CH-default
};

bool SpdifOpticalPlayInit(void)
{
	AudioCoreIO AudioIOSet;
	bool ret = FALSE;

	if(SpdifPlayCt != NULL)
	{
		return FALSE;
	}

	DBG("Optical play Init!\n");

	SPDIF_ModuleDisable(CFG_SPDIF_OPTICAL_MODULE);

	DMA_ChannelAllocTableSet(DMA0, (uint8_t* )Dma0ChannelMap);
	DMA_ChannelAllocTableSet(DMA1, (uint8_t* )Dma1ChannelMap);

#ifdef AUDIO_STREAM_UNINTERRUPT
	SpdifPlayCt = (SpdifPlayContext*)osPortMallocFromEnd(sizeof(SpdifPlayContext));
#else
	SpdifPlayCt = (SpdifPlayContext*)osPortMalloc(sizeof(SpdifPlayContext));
#endif
	if(SpdifPlayCt == NULL)
	{
		return FALSE;
	}
	memset(SpdifPlayCt, 0, sizeof(SpdifPlayContext));

#ifdef AUDIO_STREAM_UNINTERRUPT
	SpdifPlayCt->SpdifPcmFifo = (uint32_t *)osPortMallocFromEnd(AudioCoreFrameSizeGet(DefaultNet) * sizeof(PCM_DATA_TYPE) * 2 * 2 * 2);//end bkd
#else
	SpdifPlayCt->SpdifPcmFifo = (uint32_t *)osPortMalloc(AudioCoreFrameSizeGet(DefaultNet) * sizeof(PCM_DATA_TYPE) * 2 * 2 * 2);//end bkd
#endif
	if(SpdifPlayCt->SpdifPcmFifo == NULL)
	{
		return FALSE;
	}
	memset(SpdifPlayCt->SpdifPcmFifo, 0, AudioCoreFrameSizeGet(DefaultNet) * sizeof(PCM_DATA_TYPE) * 2 * 2 * 2);
	MCUCircular_Config(&SpdifPlayCt->SpdifPcmCircularBuf, SpdifPlayCt->SpdifPcmFifo, AudioCoreFrameSizeGet(DefaultNet) * sizeof(PCM_DATA_TYPE) * 2 * 2 * 2);
	DBG("spdif frame size %d sample / frame\n",AudioCoreFrameSizeGet(DefaultNet));


	//  (DMA buffer)
#ifdef AUDIO_STREAM_UNINTERRUPT
	SpdifPlayCt->SpdifPwcFIFO = (uint32_t*)osPortMallocFromEnd(SPDIF_FIFO_LEN);
#else
	SpdifPlayCt->SpdifPwcFIFO = (uint32_t*)osPortMalloc(SPDIF_FIFO_LEN);
#endif
	if(SpdifPlayCt->SpdifPwcFIFO == NULL)
	{
		return FALSE;
	}
	memset(SpdifPlayCt->SpdifPwcFIFO, 0, SPDIF_FIFO_LEN);

#ifdef AUDIO_STREAM_UNINTERRUPT
	SpdifPlayCt->SpdifCarry = (uint32_t *)osPortMallocFromEnd(SPDIF_CARRY_LEN);
#else
	SpdifPlayCt->SpdifCarry = (uint32_t *)osPortMalloc(SPDIF_CARRY_LEN);
#endif
	if(SpdifPlayCt->SpdifCarry == NULL)
	{
		return FALSE;
	}
	memset(SpdifPlayCt->SpdifCarry, 0, SPDIF_CARRY_LEN);

#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
#ifdef AUDIO_STREAM_UNINTERRUPT
	SpdifPlayCt->SpdifCarry24 = (uint32_t *)osPortMallocFromEnd(SPDIF_CARRY_LEN_24);
#else
	SpdifPlayCt->SpdifCarry24 = (uint32_t *)osPortMalloc(SPDIF_CARRY_LEN_24);
#endif
	if(SpdifPlayCt->SpdifCarry24 == NULL)
	{
		return FALSE;
	}
	memset(SpdifPlayCt->SpdifCarry24, 0, SPDIF_CARRY_LEN_24);
#endif
#ifdef AUDIO_STREAM_UNINTERRUPT
	if(IsResRamReallocate == TRUE)
#endif
	{
		if(!ModeCommonInit())
		{
			ModeCommonDeinit();
			return FALSE;
		}

	#ifdef CFG_FUNC_ROBOEFFECT_EN
		AudioCoreProcessConfig((AudioCoreProcessFunc)AudioMusicProcess);
	#else
		AudioCoreProcessConfig((AudioCoreProcessFunc)AudioBypassProcess);
	#endif
		//Audio init
	#ifdef DUAL_CORE_EN
		DualCoreDspAddrInit();
	#endif
	#ifdef CFG_FUNC_ROBOEFFECT_EN
		AudioEffect_Params_Sync(&EffectParambin);
	#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
		AudioEffect_Params_Sync(&EffectParambin2);
	#endif
	#endif
	}
	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
#if	defined(CFG_WIRELESS_EN) && defined(CFG_WIRELESS_OUT_ON)
	AudioIOSet.Adapt = SRC_SRA;
#elif (BT_TWS_SUPPORT)
	AudioIOSet.Adapt = SRC_SRA;//tws 下不能使用硬件微调
#else
	AudioIOSet.Adapt = SRC_SRA;//SRC_ADJUST;//SRC_SRA;
#endif
	AudioIOSet.SrcQuality = SRC_QUALITY;
	AudioIOSet.Sync = FALSE;
	AudioIOSet.Channels = 2;
	AudioIOSet.Net = AudioCoreGetNetBySourceChannelNUM(SPDIF_SOURCE_NUM);

	AudioIOSet.DataIOFunc = Spdif_Rx_DataGet;
	AudioIOSet.LenGetFunc = Spdif_Rx_DataLenGet;

	AudioIOSet.Depth = AudioCoreFrameSizeGet(AudioCoreGetNetBySourceChannelNUM(SPDIF_SOURCE_NUM)) * 2;
	AudioIOSet.LowLevelCent = 40;
	AudioIOSet.HighLevelCent = 60;

	SpdifPlayCt->SampleRate = AudioCoreMixSampleRateGet(DefaultNet);
	AudioIOSet.SampleRate = SpdifPlayCt->SampleRate;
	AudioIOSet.PcmWidth	= CFG_DEFAULT_WIDTH;

	if(!AudioCoreSourceInit(&AudioIOSet, SPDIF_SOURCE_NUM))
	{
		DBG("spdif source init error!\n");
		return FALSE;
	}

	AudioCoreSourceAdjust(SPDIF_SOURCE_NUM, TRUE);
	SPDIF0_AnalogModuleDisable();

#ifdef CFG_APP_OPTICAL_MODE_EN
//	if(GetSystemMode() == ModeOpticalAudioPlay)
	{
		//spdif config
		GPIO_PortBModeSet(SPDIF_OPTICAL_INDEX, SPDIF_OPTICAL_PORT_MODE);
	}
#endif

//#ifdef CFG_APP_COAXIAL_MODE_EN
//	if(GetSystemMode() == ModeCoaxialAudioPlay)
//	{
//		GPIO_RegBitsSet(GPIO_B_ANA_EN,SPDIF0_COAXIAL_INDEX);
//		GPIO_PortBModeSet(SPDIF_COAXIAL_INDEX, SPDIF_COAXIAL_PORT_MODE);
//		SPDIF0_AnalogModuleEnable(SPDIF_COAXIAL_PORT_ANA_INPUT, SPDIF_ANA_LEVEL_300mVpp, SPDIF_ANA_LVL);
//	}
//#endif

	memset(&SpdifPlayCt->AudioInfo, 0, sizeof(SPDIF_TYPE_STR));
	SpdifPlayCt->AudioInfo.audio_type = SPDIF_AUDIO_PCM_DATA_TYPE;

	SPDIF_ModuleRst(CFG_SPDIF_OPTICAL_MODULE);
	SPDIF_ModuleDisable(CFG_SPDIF_OPTICAL_MODULE);
	SPDIF_RXInit(CFG_SPDIF_OPTICAL_MODULE,1, 0, 0);
	//使用
	DMA_ChannelDisable(CFG_SPDIF_OPTICAL_RX_DMA_CHANNEL);
	DMA_CircularConfig(CFG_SPDIF_OPTICAL_RX_DMA_CHANNEL, SPDIF_FIFO_LEN / 2, (void*)SpdifPlayCt->SpdifPwcFIFO, SPDIF_FIFO_LEN);
	DMA_ChannelEnable(CFG_SPDIF_OPTICAL_RX_DMA_CHANNEL);

#ifdef SPDIF_DPLL_LOCK_MODE
	SPDIF_DpllLockInit();//dpll lock mode
#endif

	SPDIF_ModuleEnable(CFG_SPDIF_OPTICAL_MODULE);


#ifdef CFG_FUNC_BREAKPOINT_EN
	BackupInfoUpdata(BACKUP_SYS_INFO);
#endif

	AudioCodecHardwareSync();//update hardware config

#ifdef CFG_FUNC_REMIND_SOUND_EN
	if(!SoftFlagGet(SoftFlagSysCurModeReboot))
	{
//		if(GetSystemMode() == ModeOpticalAudioPlay)
		{
			DBG("Optical Play run\n");
			ret = RemindSoundServiceItemRequest(SOUND_REMIND_GXIANMOD, REMIND_PRIO_NORMAL);
		}
//		else
//		{
//			DBG("Coaxial Play run\n");
//			ret = RemindSoundServiceItemRequest(SOUND_REMIND_TZHOUMOD, REMIND_PRIO_NORMAL);
//		}
		if(ret == FALSE)
		{
			if(IsAudioPlayerMute() == TRUE)
				{
					HardWareMuteOrUnMute();
				}
		}
	}
#endif


	/************************solution 1***************************/
	//SpdifPlayCt->SpdifLockFlag = TRUE;
	/************************solution 2***************************/
	SpdifPlayCt->SpdifLockFlag = FALSE;
	AudioCoreSourceMute(SPDIF_SOURCE_NUM, TRUE, TRUE);
	/*************************end*********************************/

#ifndef CFG_FUNC_REMIND_SOUND_EN
	 if(IsAudioPlayerMute() == TRUE)
	 {
		 HardWareMuteOrUnMute();
	 }
#endif

	return TRUE;
}

void SpdifOpticalPlayRun(uint16_t msgId)
{
	if(msgId == MSG_EFFECTPARAM_SWITCH || msgId == MSG_EFFECTFLOW_SWITCH
			|| msgId == MSG_EFFECT_REFRESH)
	{
		SpdifPlayCt->SpdifMuteCnt = 500;
	}
	switch(msgId)
	{
		default:
			CommonMsgProccess(msgId);
			break;
	}

//#ifdef CFG_APP_COAXIAL_MODE_EN
//	if(GetSystemMode() == ModeCoaxialAudioPlay)
//	{
//		if(SPDIF_FlagStatusGet(CFG_SPDIF_OPTICAL_MODULE,SYNC_FLAG_STATUS) || (!SPDIF_FlagStatusGet(CFG_SPDIF_OPTICAL_MODULE,LOCK_FLAG_STATUS)))
//		{
//			SPDIF_RXInit(CFG_SPDIF_OPTICAL_MODULE,1, 0, 0);
//			SPDIF_ModuleEnable(CFG_SPDIF_OPTICAL_MODULE);
//		}
//	}
//#endif

	if(SpdifPlayCt->SpdifLockFlag && !SPDIF_FlagStatusGet(CFG_SPDIF_OPTICAL_MODULE,LOCK_FLAG_STATUS))
	{
		APP_DBG("SPDIF RX UNLOCK!\n");
		SpdifPlayCt->SpdifLockFlag = FALSE;
		AudioCoreSourceMute(SPDIF_SOURCE_NUM, TRUE, TRUE);
#ifdef SPDIF_DPLL_LOCK_MODE
		SpdifPlayCt->dpll_lock_cnt = 0;
		SPDIF_DpllLockDeInit();
		SPDIF_DpllLockInit();//dpll lock mode
		SpdifPlayCt->SpdifDpllLockFlag = FALSE;
#endif

		AudioCoreSourceDisable(SPDIF_SOURCE_NUM);
		AudioCoreSourceAdapterReset(SPDIF_SOURCE_NUM);
		SpdifPlayCt->SpdifMuteCnt = 20;
	}
	if(!SpdifPlayCt->SpdifLockFlag && SPDIF_FlagStatusGet(CFG_SPDIF_OPTICAL_MODULE,LOCK_FLAG_STATUS))
	{
		APP_DBG("SPDIF RX LOCK!\n");
		SpdifPlayCt->SpdifLockFlag = TRUE;
		AudioCoreSourceEnable(SPDIF_SOURCE_NUM);
		osTaskDelay(10);
#ifdef SPDIF_DPLL_LOCK_MODE
		SPDIF_DpllLockStart();

		TIMER timeout;
		TimeOutSet(&timeout,1000);
		while(!SPDIF_GetDpllLockFlag())
		{
			if(IsTimeOut(&timeout))
			{
				TimeOutSet(&timeout,1000);
				SPDIF_DpllLockDeInit();
				if(!SPDIF_FlagStatusGet(CFG_SPDIF_OPTICAL_MODULE,LOCK_FLAG_STATUS))
				{
					APP_DBG("unlock return\n");
					return ;
				}
				else
				{
					APP_DBG("reinit dpll lock\n");
					SPDIF_DpllLockInit();//dpll lock mode
					SPDIF_DpllLockStart();
				}
			}
		}

		if(SPDIF_GetDpllLockFlag())
		{
			APP_DBG("Dpll LOCK!\n");
			SPDIF_DpllLockHold();
		}
		else
		{
			APP_DBG("Dpll LOCK FAIL!\n");
			SpdifPlayCt->SpdifDpllLockFlag = FALSE;
		}
#endif
	}

#ifdef SPDIF_DPLL_LOCK_MODE
	if(SpdifPlayCt->SpdifLockFlag == TRUE)
	{
		SpdifPlayCt->dpll_lock_cnt++;
		if(SpdifPlayCt->dpll_lock_cnt>1000 && SDPIF_DpllLockHoldGet())
		{
			SpdifPlayCt->dpll_lock_cnt = 0;
			APP_DBG("REG_DPLL_NDAC_HOLD_EN\n");
			SPDIF_DpllLockHoldSet(0);//DPLL彻底跟随SPDIF0
			SpdifPlayCt->SpdifDpllLockFlag = TRUE;
		}
	}
#endif

	//监控SPDIF RX采样率是否改变
#ifdef SPDIF_DPLL_LOCK_MODE
	if(SpdifPlayCt->SpdifDpllLockFlag == TRUE)
#else
	if(SpdifPlayCt->SpdifLockFlag == TRUE)
#endif
	{
		if(SpdifPlayCt->SampleRate != SPDIF_SampleRateGet(CFG_SPDIF_OPTICAL_MODULE))
		{
			SpdifPlayCt->SampleRate = SPDIF_SampleRateGet(CFG_SPDIF_OPTICAL_MODULE);
			APP_DBG("Get SampleRate: %d\n", (int)SpdifPlayCt->SampleRate);
#ifdef CFG_AUDIO_OUT_AUTO_SAMPLE_RATE_44100_48000
			AudioOutSampleRateSet(SpdifPlayCt->SampleRate);
#endif
			AudioCoreSourceChange(SPDIF_SOURCE_NUM, 2, SpdifPlayCt->SampleRate);
			AudioCoreSourceMute(SPDIF_SOURCE_NUM, TRUE, TRUE);
			SpdifPlayCt->SpdifMuteCnt = 250;	//some devices maybe need larger or smaller
#ifdef SPDIF_DPLL_LOCK_MODE
			DpllLockReInit();
#endif
		}

		if(!SpdifPlayCt->SpdifMuteCnt && AudioCoreSourceMuteStatusGet(SPDIF_SOURCE_NUM))
		{
			APP_DBG("Unmute spdif\n");
			AudioCoreSourceUnmute(SPDIF_SOURCE_NUM, TRUE, TRUE);
		}
		else if(SpdifPlayCt->SpdifMuteCnt && !AudioCoreSourceMuteStatusGet(SPDIF_SOURCE_NUM))
		{
			APP_DBG("Mute spdif\n");
			AudioCoreSourceMute(SPDIF_SOURCE_NUM, TRUE, TRUE);
		}
		SpdifDataCarry(CFG_SPDIF_OPTICAL_RX_DMA_CHANNEL);
	}
	//任务优先级设置为4,通过发送该命令，可以提高AudioCore service有效利用率
	{
		MessageContext		msgSend;
		msgSend.msgId		= MSG_NONE;
		MessageSend(GetAudioCoreServiceMsgHandle(), &msgSend);
	}
}


bool SpdifOpticalPlayDeinit(void)
{
	APP_DBG("Optical play Deinit\n");

	if(SpdifPlayCt == NULL)
	{
		return FALSE;
	}
#ifdef AUDIO_STREAM_UNINTERRUPT
	if(IsResRamRelease == TRUE)
#endif
	{
		if(IsAudioPlayerMute() == FALSE)
		{
			HardWareMuteOrUnMute();
		}

		PauseAudioCore();
		AudioCoreProcessConfig((void*)AudioNoAppProcess);
	}
	AudioCoreSourceAdjust(SPDIF_SOURCE_NUM, FALSE);
	AudioCoreSourceDisable(SPDIF_SOURCE_NUM);//SPDIF_SOURCE_NUM
	AudioCoreSourceDeinit(SPDIF_SOURCE_NUM);

	SPDIF_ModuleDisable(CFG_SPDIF_OPTICAL_MODULE);
	DMA_InterruptFlagClear(CFG_SPDIF_OPTICAL_RX_DMA_CHANNEL, DMA_DONE_INT);
	DMA_InterruptFlagClear(CFG_SPDIF_OPTICAL_RX_DMA_CHANNEL, DMA_THRESHOLD_INT);
	DMA_InterruptFlagClear(CFG_SPDIF_OPTICAL_RX_DMA_CHANNEL, DMA_ERROR_INT);
	DMA_ChannelDisable(CFG_SPDIF_OPTICAL_RX_DMA_CHANNEL);

	SPDIF0_AnalogModuleDisable();

//#ifdef CFG_APP_COAXIAL_MODE_EN
//	GPIO_PortBModeSet(SPDIF_COAXIAL_INDEX, 0);
//#endif
#ifdef CFG_APP_OPTICAL_MODE_EN
	GPIO_PortBModeSet(SPDIF_OPTICAL_INDEX, 0);
#endif


	if(SpdifPlayCt->SpdifCarry != NULL)
	{
		osPortFree(SpdifPlayCt->SpdifCarry);
		SpdifPlayCt->SpdifCarry = NULL;
	}

#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
	if(SpdifPlayCt->SpdifCarry24 != NULL)
	{
		osPortFree(SpdifPlayCt->SpdifCarry24);
		SpdifPlayCt->SpdifCarry24 = NULL;
	}
#endif

	if(SpdifPlayCt->SpdifPcmFifo != NULL)
	{
		osPortFree(SpdifPlayCt->SpdifPcmFifo);
		SpdifPlayCt->SpdifPcmFifo = NULL;
	}

	if(SpdifPlayCt->SpdifPwcFIFO != NULL)
	{
		osPortFree(SpdifPlayCt->SpdifPwcFIFO);
		SpdifPlayCt->SpdifPwcFIFO = NULL;
	}
#ifdef AUDIO_STREAM_UNINTERRUPT
	if(IsResRamRelease == TRUE)
#endif
	{
		ModeCommonDeinit();//通路全部释放
	}

#ifdef SPDIF_DPLL_LOCK_MODE
	SPDIF_DpllLockDeInit();
#endif

	osPortFree(SpdifPlayCt);
	SpdifPlayCt = NULL;

	return TRUE;
}

#endif
