/**
 **************************************************************************************
 * @file    hdmi_in_mode.c
 * @brief
 *
 * @author  Cecilia Wang
 * @version V1.0.0
 *
 * $Created: 2018-03-23 13:06:47$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
//driver、lib
#include <string.h>
#include "type.h"
#include "irqn.h"
#include "gpio.h"
#include "dma.h"
#include "delay.h"
#include "audio_adc.h"
#include "dac.h"
#include "adc_interface.h"
#include "dac_interface.h"
#include "timeout.h"
#include "reset.h"
#include "watchdog.h"
#include "cec.h"
#include "clk.h"
#include "mcu_circular_buf.h"
#include "powercontroller.h"

//application
#include "app_config.h"
#include "rtos_api.h"
#include "app_message.h"
#include "debug.h"
#include "audio_core_api.h"
#include "audio_core_service.h"
#include "hdmi_in_api.h"
#include "main_task.h"
#include "audio_effect.h"
#include "resampler_polyphase.h"
//#include "decoder_service.h"
//#include "remind_sound_service.h"
#include "hdmi_in_Mode.h"
//#include "deepsleep.h"
#include "breakpoint.h"
#include "ctrlvars.h"
//#include "recorder_service.h"
#include "audio_vol.h"
#include "ctrlvars.h"
#include "mode_task_api.h"
#include "remind_sound.h"
//#include "audio_adjust.h"
//#include "audio_common.h"
#ifdef DUAL_CORE_EN
#include "dualcore_api.h"
#include "dualcore_dsp.h"
#endif

extern CECInitTypeDef 	*gCecInitDef;
extern HDMIInfo         *gHdmiCt;
#ifdef  CFG_APP_HDMIIN_MODE_EN

#define HDMI_IN_PLAY_TASK_STACK_SIZE		512//1024
#define HDMI_IN_PLAY_TASK_PRIO				4
#define HDMI_IN_NUM_MESSAGE_QUEUE			10

#define PCM_REMAIN_SMAPLES					3//spdif数据丢声道时,返回数据会超过一帧128，用于缓存数据，试听感更加好

//由于采样值从32bit转换为16bit，可以使用同一个buf，否则要独立申请
#define HDMI_ARC_CARRY_LEN					(MAX_FRAME_SAMPLES * sizeof(PCM_DATA_TYPE) * 2 * 2 + PCM_REMAIN_SMAPLES * 2 * 4)// buf len for get data form dma fifo, deal
 //转采样输出buf,如果spdif转采样提升大于四倍需要加大此SPDIF_CARRY_LEN，比如输入8000以下转48000,需要缩小单次carry帧大小或调大HDMI_SRC_RESAMPLER_OUT_LEN、HDMI_ARC_PCM_FIFO_LEN
#define HDMI_SRC_RESAMPLER_OUT_LEN			(MAX_FRAME_SAMPLES * sizeof(PCM_DATA_TYPE) * 2 * 2 * 2 * 2)


typedef struct _HdmiInPlayContext
{
	uint32_t			*hdmiARCFIFO;	    //ARC的DMA循环fifo
	uint32_t            *sourceBuf_ARC;		//取ARC数据
	uint32_t            *hdmiARCCarry;
	uint32_t            *hdmiARCCarry24;
	uint32_t			*hdmiARCPcmFifo;
	MCU_CIRCULAR_CONTEXT hdmiPcmCircularBuf;

	uint32_t			hdmiSampleRate;

	uint8_t    			hdmiArcDone;
	uint8_t     		hdmiRetransCnt;
	TIMER       		hdmiMaxRespondTime;
	SPDIF_TYPE_STR  	AudioInfo;
}HdmiInPlayContext;

/****************************************
 * DMA CTRL 仅8通道，超出无效：
 * 常驻设备排前部,所有mode序号一致
 * *************************************/
static const uint8_t Dma0ChannelMap[16] = {
	MAP0_COMMON_FIRST
#if defined(CFG_HDMI_SPDIF_NUM) && (CFG_HDMI_SPDIF_NUM == HDMI_SPDIF0)
	CFG_HDMI_DMA_CHANNEL,
	HDMI_CEC_RECV_DMA_ID,
#endif

	/*在此新增独有设备,确保不超*/
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,//8 CH-default
};

static const uint8_t Dma1ChannelMap[16] = {
	MAP1_COMMON_FIRST
#ifdef CFG_APP_I2SIN_MODE_EN
	PERIPHERAL_1_ID_I2S0_RX + 2 * CFG_RES_I2S_MODULE,
#endif
#if defined(CFG_HDMI_SPDIF_NUM) && (CFG_HDMI_SPDIF_NUM == HDMI_SPDIF1)
	CFG_HDMI_DMA_CHANNEL,
#endif

	/*在此新增独有设备,确保不超*/
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,//8 CH-default
};


typedef enum __HDMI_FAST_SWITCH_ARC_STATUS
{
	FAST_SWITCH_INIT_STATUS = 0,
	FAST_SWITCH_DONE_STATUS = 1,
	FAST_SWITCH_WORK_STATUS = 2,

} HDMI_FAST_SWITCH_ARC_STATUS;

static HdmiInPlayContext*		hdmiInPlayCt;

//osMutexId			hdmiPcmFifoMutex;
bool 				HdmiSpdifLockFlag = FALSE;
bool 				unmuteLockFlag = 0;
TIMER 				unmuteLockTime;

static void HdmiARCScan(void);
static void HdmiInPlayRunning(uint16_t msgId);

bool HdmiInPlayResMalloc(uint16_t SampleLen)
{
#ifdef AUDIO_STREAM_UNINTERRUPT
	hdmiInPlayCt->hdmiARCFIFO = (uint32_t*)osPortMallocFromEnd(SampleLen * 2 * 2 * 2 * 2 * 2);
#else
	hdmiInPlayCt->hdmiARCFIFO = (uint32_t*)osPortMalloc(SampleLen * 2 * 2 * 2 * 2 * 2);
#endif
	if(hdmiInPlayCt->hdmiARCFIFO == NULL)
	{
		return FALSE;
	}
	memset(hdmiInPlayCt->hdmiARCFIFO, 0, SampleLen * 2 * 2 * 2 * 2 * 2);
#ifdef AUDIO_STREAM_UNINTERRUPT
	hdmiInPlayCt->sourceBuf_ARC = (uint32_t *)osPortMallocFromEnd(SampleLen * 2 * 2);
#else
	hdmiInPlayCt->sourceBuf_ARC = (uint32_t *)osPortMalloc(SampleLen * 2 * 2);
#endif
	if(hdmiInPlayCt->sourceBuf_ARC == NULL)
	{
		return FALSE;
	}
	memset(hdmiInPlayCt->sourceBuf_ARC, 0, SampleLen * 2 * 2);

#ifdef AUDIO_STREAM_UNINTERRUPT
	hdmiInPlayCt->hdmiARCPcmFifo = (uint32_t *)osPortMallocFromEnd(AudioCoreFrameSizeGet(DefaultNet) * sizeof(PCM_DATA_TYPE) * 2 * 2 * 2);
#else
	hdmiInPlayCt->hdmiARCPcmFifo = (uint32_t *)osPortMalloc(AudioCoreFrameSizeGet(DefaultNet) * sizeof(PCM_DATA_TYPE) * 2 * 2 * 2);
#endif
	if(hdmiInPlayCt->hdmiARCPcmFifo == NULL)
	{
		return FALSE;
	}
	MCUCircular_Config(&hdmiInPlayCt->hdmiPcmCircularBuf, hdmiInPlayCt->hdmiARCPcmFifo, AudioCoreFrameSizeGet(DefaultNet) * sizeof(PCM_DATA_TYPE) * 2 * 2 * 2);
	memset(hdmiInPlayCt->hdmiARCPcmFifo, 0, AudioCoreFrameSizeGet(DefaultNet) * sizeof(PCM_DATA_TYPE) * 2 * 2 * 2);

	return TRUE;
}


/**
 * @func        HdmiInPaly_Init
 * @brief       HdmiIn模式参数配置，资源初始化
 * @param       MessageHandle parentMsgHandle
 * @Output      None
 * @return      bool
 * @Others      任务块、Adc、Dac、AudioCore配置
 * @Others      数据流从Adc到audiocore配有函数指针，audioCore到Dac同理，由audiocoreService任务按需驱动
 * Record
 */
 bool HdmiInPlayInit(void)
{
	AudioCoreIO AudioIOSet;
//	bool ret;
	//将SPDIF时钟切换到AUPLL
	// Clock_SpdifClkSelect(APLL_CLK_MODE);
	DMA_ChannelAllocTableSet(DMA0, (uint8_t* )Dma0ChannelMap);
	DMA_ChannelAllocTableSet(DMA1, (uint8_t* )Dma1ChannelMap);
#ifdef AUDIO_STREAM_UNINTERRUPT
	if(IsResRamReallocate == TRUE)
#endif
	{
		if(!ModeCommonInit())
		{
			return FALSE;
		}
	}
#ifdef AUDIO_STREAM_UNINTERRUPT
	hdmiInPlayCt = (HdmiInPlayContext*)osPortMallocFromEnd(sizeof(HdmiInPlayContext));
#else
	hdmiInPlayCt = (HdmiInPlayContext*)osPortMalloc(sizeof(HdmiInPlayContext));
#endif
	if(hdmiInPlayCt == NULL)
	{
		return FALSE;
	}
	memset(hdmiInPlayCt, 0, sizeof(HdmiInPlayContext));

	if(!HdmiInPlayResMalloc(AudioCoreFrameSizeGet(DefaultNet)))
	{
		APP_DBG("HdmiInPlayResMalloc error\n");
		return FALSE;
	}
#ifdef AUDIO_STREAM_UNINTERRUPT
	hdmiInPlayCt->hdmiARCCarry = (uint32_t *)osPortMallocFromEnd(HDMI_ARC_CARRY_LEN);
#else
	hdmiInPlayCt->hdmiARCCarry = (uint32_t *)osPortMalloc(HDMI_ARC_CARRY_LEN);
#endif
	if(hdmiInPlayCt->hdmiARCCarry == NULL)
	{
		return FALSE;
	}
	memset(hdmiInPlayCt->hdmiARCCarry, 0, HDMI_ARC_CARRY_LEN);

#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
#ifdef AUDIO_STREAM_UNINTERRUPT
	hdmiInPlayCt->hdmiARCCarry24 = (uint32_t *)osPortMallocFromEnd(HDMI_ARC_CARRY_LEN);
#else
	hdmiInPlayCt->hdmiARCCarry24 = (uint32_t *)osPortMalloc(HDMI_ARC_CARRY_LEN);
#endif
	if(hdmiInPlayCt->hdmiARCCarry24 == NULL)
	{
		return FALSE;
	}
	memset(hdmiInPlayCt->hdmiARCCarry24, 0, HDMI_ARC_CARRY_LEN);
#endif

	memset(&hdmiInPlayCt->AudioInfo, 0, sizeof(SPDIF_TYPE_STR));
	hdmiInPlayCt->AudioInfo.audio_type = SPDIF_AUDIO_PCM_DATA_TYPE;


	HDMI_ARC_Init((uint16_t *)hdmiInPlayCt->hdmiARCFIFO, AudioCoreFrameSizeGet(DefaultNet) * 2 * 2 * 2 * 2 * 2, &hdmiInPlayCt->hdmiPcmCircularBuf);
	HDMI_CEC_DDC_Init();

#ifdef AUDIO_STREAM_UNINTERRUPT
	if(IsResRamReallocate == TRUE)
#endif
	{
#ifdef CFG_FUNC_ROBOEFFECT_EN
	AudioCoreProcessConfig((void*)AudioMusicProcess);
#else
	AudioCoreProcessConfig((void*)AudioBypassProcess);
#endif
	//HDMI_CEC_InitiateARC();

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

	AudioIOSet.Adapt = SRC_SRA;
	AudioIOSet.SrcQuality = SRC_QUALITY;
	AudioIOSet.Sync = FALSE;
	AudioIOSet.Channels = 2;
	AudioIOSet.Net = AudioCoreGetNetBySourceChannelNUM(HDMI_IN_SOURCE_NUM);

	AudioIOSet.DataIOFunc = HDMI_ARC_DataGet;
	AudioIOSet.LenGetFunc = HDMI_ARC_DataLenGet;

	AudioIOSet.Depth = AudioCoreFrameSizeGet(DefaultNet) * 2;
	AudioIOSet.LowLevelCent = 40;
	AudioIOSet.HighLevelCent = 60;
	AudioIOSet.SampleRate =  AudioCoreMixSampleRateGet(DefaultNet);
	AudioIOSet.PcmWidth	= CFG_DEFAULT_WIDTH;

	if(!AudioCoreSourceInit(&AudioIOSet, HDMI_IN_SOURCE_NUM))
	{
		DBG("hdmi source init error!\n");
		return FALSE;
	}

	AudioCoreSourceAdjust(HDMI_IN_SOURCE_NUM, TRUE);

	DMA_ChannelDisable(CFG_HDMI_DMA_CHANNEL);
	DMA_CircularConfig(CFG_HDMI_DMA_CHANNEL, AudioCoreFrameSizeGet(DefaultNet) * 2 * 2 * 2 * 2 * 2, (uint16_t *)hdmiInPlayCt->hdmiARCFIFO, AudioCoreFrameSizeGet(DefaultNet) * 2 * 2 * 2 * 2 * 2);
    DMA_ChannelEnable(CFG_HDMI_DMA_CHANNEL);

	SPDIF_ModuleEnable(CFG_HDMI_SPDIF_NUM);

#ifdef CFG_FUNC_BREAKPOINT_EN
		BackupInfoUpdata(BACKUP_SYS_INFO);
#endif

		AudioCodecHardwareSync();//update hardware config

    if ((gHdmiCt->hdmi_tv_inf.tv_type == TV_SAMSUNG_1670)
    		|| (gHdmiCt->hdmi_tv_inf.tv_type == TV_POLARIOD_010B)
			|| (gHdmiCt->hdmi_tv_inf.tv_type == TV_SAMSUNG_170F)
			|| (gHdmiCt->hdmi_tv_inf.tv_type == TV_TCL_2009)
			|| (gHdmiCt->hdmi_tv_inf.tv_type == TV_SONY_04A2)
			|| (gHdmiCt->hdmi_tv_inf.tv_type == TV_SAMSUNG_0371)
			|| (gHdmiCt->hdmi_tv_inf.tv_type == TV_SAMSUNG_5770)
			|| (gHdmiCt->hdmi_tv_inf.tv_type == TV_SONY_0571)
	   )
    {
        if(HDMI_HPD_StatusGet())
            HDMI_CEC_SetSystemAudioModeOn();
    }
	//AudioCoreSourceEnable(HDMI_IN_SOURCE_NUM);

	DBG("HdmiIn Play run\n");
#ifdef CFG_FUNC_REMIND_SOUND_EN
	if(!SoftFlagGet(SoftFlagSysCurModeReboot) && RemindSoundServiceItemRequest(SOUND_REMIND_HDMIMODE, REMIND_PRIO_NORMAL) == FALSE)
#endif
	{
		if(IsAudioPlayerMute() == TRUE)
		{
			HardWareMuteOrUnMute();
		}
	}

	HdmiSpdifLockFlag = FALSE;
	unmuteLockFlag = 0;

	return TRUE;
}

void HdmiInPlayRun(uint16_t msgId)
{
		switch(msgId)
		{
				
			default:
				//if(hdmiInPlayCt->state == TaskStateRunning)
				{
					HdmiInPlayRunning(msgId);
					if(HdmiSpdifLockFlag && !SPDIF_FlagStatusGet(CFG_HDMI_SPDIF_NUM,LOCK_FLAG_STATUS))
					{
						APP_DBG("SPDIF RX UNLOCK!\n");
						HdmiSpdifLockFlag = FALSE;
						//HDMI_CEC_DDC_Init();
						AudioCoreSourceDisable(HDMI_IN_SOURCE_NUM);
//				#ifdef CFG_FUNC_FREQ_ADJUST
//						AudioCoreSourceFreqAdjustDisable();
//				#endif
					}

					if(!HdmiSpdifLockFlag && SPDIF_FlagStatusGet(CFG_HDMI_SPDIF_NUM,LOCK_FLAG_STATUS)/* && HDMI_ARC_IsReady()*/
						// #if	defined(CFG_FUNC_REMIND_SOUND_EN)
						// && hdmiInPlayCt->IsSoundRemindDone
						// #endif
					)
					{
						if((TV_XIAOMI_XMD == gHdmiCt->hdmi_tv_inf.tv_type && HDMI_ARC_IsReady())
						|| (TV_XIAOMI_XMD != gHdmiCt->hdmi_tv_inf.tv_type))
						{
							APP_DBG("SPDIF RX LOCK!\n");
							HdmiSpdifLockFlag = TRUE;

						//AudioCoreSourceMute(APP_SOURCE_NUM, TRUE, TRUE);
						HDMISourceMute();
						osTaskDelay(20);
						AudioCoreSourceEnable(HDMI_IN_SOURCE_NUM);
						//AudioCoreSourceUnmute(APP_SOURCE_NUM, TRUE, TRUE);
						TimeOutSet(&unmuteLockTime, 500);
						unmuteLockFlag = 1;

						}
					}
					if(HdmiSpdifLockFlag == TRUE)
					{
						//硬件采样率获取，设定。
						if(hdmiInPlayCt->hdmiSampleRate != SPDIF_SampleRateGet(CFG_HDMI_SPDIF_NUM))
						{
							hdmiInPlayCt->hdmiSampleRate = SPDIF_SampleRateGet(CFG_HDMI_SPDIF_NUM);

							APP_DBG("Get samplerate: %d\n", (int)hdmiInPlayCt->hdmiSampleRate);
							AudioCoreSourceChange(HDMI_IN_SOURCE_NUM, 2, hdmiInPlayCt->hdmiSampleRate);
						}

						if(unmuteLockFlag == 1)
						{
							if(IsTimeOut(&unmuteLockTime))
							{
								unmuteLockFlag = 0;
								if(gHdmiCt->hdmi_audiomute_flag == 0)
								{
									HDMISourceUnmute();
									if(IsAudioPlayerMute() == TRUE)
									{
										HardWareMuteOrUnMute();
									}
									if(gHdmiCt->hdmiActiveReportMuteAfterInit == 1)
									{
										gHdmiCt->hdmiActiveReportMuteAfterInit = 0;
										gHdmiCt->hdmiActiveReportMuteflag = 2;
										gHdmiCt->hdmiActiveReportMuteStatus = 0;
									}
								}

							}
						}

						HdmiARCScan();
					}
					HDMI_CEC_Scan(1);

					//任务优先级设置为4,通过发送该命令，可以提高AudioCore service有效利用率
					{
						MessageContext		msgSend;
						msgSend.msgId		= MSG_NONE;
						MessageSend(GetAudioCoreServiceMsgHandle(), &msgSend);
					}
				}
				break;
		}
}

static void HdmiInPlayRunning(uint16_t msgId)
{
	if(hdmiInPlayCt->hdmiArcDone == FAST_SWITCH_WORK_STATUS)
	{
		if(IsTimeOut(&hdmiInPlayCt->hdmiMaxRespondTime))
		{
			TimeOutSet(&hdmiInPlayCt->hdmiMaxRespondTime, 200);
			msgId = MSG_HDMI_AUDIO_ARC_ONOFF;
		}
	}
	switch(msgId)
	{
		case MSG_HDMI_AUDIO_ARC_ONOFF:
			if(hdmiInPlayCt->hdmiArcDone != FAST_SWITCH_WORK_STATUS)
			{
				gHdmiCt->hdmiGiveReportStatus = 0;
				hdmiInPlayCt->hdmiRetransCnt = 0;
			}

			hdmiInPlayCt->hdmiArcDone = FAST_SWITCH_WORK_STATUS;
			TimeOutSet(&hdmiInPlayCt->hdmiMaxRespondTime, 200);
			if(gHdmiCt->hdmiGiveReportStatus == 1)
			{
				mainAppCt.hdmiArcOnFlg = !mainAppCt.hdmiArcOnFlg;
				hdmiInPlayCt->hdmiArcDone = FAST_SWITCH_DONE_STATUS;
			}
			else
			{
				if(HDMI_CEC_IsWorking() == CEC_IS_IDLE)
				{
					HDMI_ARC_OnOff(!mainAppCt.hdmiArcOnFlg);
					hdmiInPlayCt->hdmiRetransCnt ++;
				}
				else if(HDMI_CEC_IsWorking() == CEC_IS_INACTIVE)
				{
					hdmiInPlayCt->hdmiRetransCnt = 0;
					mainAppCt.hdmiArcOnFlg = !mainAppCt.hdmiArcOnFlg;
					hdmiInPlayCt->hdmiArcDone = FAST_SWITCH_DONE_STATUS;
				}
			}

			if(hdmiInPlayCt->hdmiRetransCnt >= 2)
			{
				hdmiInPlayCt->hdmiRetransCnt = 0;
				mainAppCt.hdmiArcOnFlg = !mainAppCt.hdmiArcOnFlg;
				hdmiInPlayCt->hdmiArcDone = FAST_SWITCH_DONE_STATUS;
			}
			break;

		default:
			CommonMsgProccess(msgId);
			break;
	}
}

static void HdmiARCScan(void)
{
	int32_t pcm_space;
	uint16_t spdif_len;
	int16_t pcm_len;
	int16_t *pcmBuf  = (int16_t *)hdmiInPlayCt->hdmiARCCarry;
	uint16_t cnt;

	spdif_len = DMA_CircularDataLenGet(CFG_HDMI_DMA_CHANNEL);
	pcm_space = MCUCircular_GetSpaceLen(&hdmiInPlayCt->hdmiPcmCircularBuf) - sizeof(PCM_DATA_TYPE) * 8;

#if 1//def CFG_FUNC_MIXER_SRC_EN
	pcm_space = (pcm_space * hdmiInPlayCt->hdmiSampleRate) /  AudioCoreMixSampleRateGet(DefaultNet) - sizeof(PCM_DATA_TYPE) * 8;
#endif

	if(pcm_space < sizeof(PCM_DATA_TYPE) * 8)
	{
		DBG("hdmi pcm_space err\n");
		return;
	}
	if((spdif_len >> 1) > pcm_space)
	{
		spdif_len = pcm_space * 2;
	}

	spdif_len = spdif_len /(MAX_FRAME_SAMPLES * sizeof(PCM_DATA_TYPE) * 4) * (MAX_FRAME_SAMPLES * sizeof(PCM_DATA_TYPE) * 4);
	spdif_len = spdif_len & 0xFFF8;
	if(!spdif_len)
	{
		return ;
	}

	cnt = (spdif_len /  (sizeof(PCM_DATA_TYPE) * 4)) / MAX_FRAME_SAMPLES;
	while(cnt--)
	{
#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
		DMA_CircularDataGet(CFG_HDMI_DMA_CHANNEL, pcmBuf, MAX_FRAME_SAMPLES * 16);
		pcm_len = SPDIF_SPDIFDataToPCMData(CFG_HDMI_SPDIF_NUM,(int32_t *)pcmBuf, MAX_FRAME_SAMPLES * 16, (int32_t *)hdmiInPlayCt->hdmiARCCarry24, SPDIF_WORDLTH_24BIT);

		if(pcm_len > 0)
		{
			int32_t *pcmBuf32  =  (int32_t *)hdmiInPlayCt->hdmiARCCarry24;
			uint16_t i;
			//高8位无符号位，需要移位产生
			for(i=0;i<pcm_len/4;i++)
			{
				pcmBuf32[i] <<= 8;
				pcmBuf32[i] >>= 8;
			}

			if(hdmiInPlayCt->AudioInfo.audio_type != SPDIF_AUDIO_PCM_DATA_TYPE)
			{
				return;
			}

			if(!mainAppCt.hdmiArcOnFlg)
			{
				memset(pcmBuf, 0, pcm_len);
			}
			MCUCircular_PutData(&hdmiInPlayCt->hdmiPcmCircularBuf, hdmiInPlayCt->hdmiARCCarry24, pcm_len);//注意格式转换返回值是byte
		}
#else
		DMA_CircularDataGet(CFG_HDMI_DMA_CHANNEL, pcmBuf, MAX_FRAME_SAMPLES * 8);
		pcm_len = SPDIF_SPDIFDataToPCMData(CFG_HDMI_SPDIF_NUM,(int32_t *)pcmBuf, MAX_FRAME_SAMPLES * 8, (int32_t *)pcmBuf, SPDIF_WORDLTH_16BIT);

		if(hdmiInPlayCt->AudioInfo.audio_type != SPDIF_AUDIO_PCM_DATA_TYPE)
		{
			return;
		}
		if(!mainAppCt.hdmiArcOnFlg)
		{
			memset(pcmBuf, 0, pcm_len);
		}
		MCUCircular_PutData(&hdmiInPlayCt->hdmiPcmCircularBuf, pcmBuf, pcm_len);//注意格式转换返回值是byte
#endif
	 }
}

bool HdmiInPlayDeinit(void)
{
	uint16_t timeout_time = 200;
	APP_DBG("HdmiIn Play Deinit\n");
	if(hdmiInPlayCt == NULL)
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
	}

	if(gHdmiCt->hdmi_tv_inf.tv_type != TV_SONY_047C)
	{
		if(gCecInitDef)
		{
			if((gHdmiCt->hdmi_tv_inf.tv_type != TV_POLARIOD_010B)
					&&(gHdmiCt->hdmi_tv_inf.tv_type != TV_TCL_2009)
					&& (gHdmiCt->hdmi_tv_inf.tv_type != TV_SAMSUNG_0371)
					)
			{
				hdmiInPlayCt->hdmiRetransCnt = 3;//最大重传3次
				if(gHdmiCt->hdmi_poweron_flag == -1)//休眠时，会强制将gHdmiCt->hdmi_arc_flag=0
				{
					gHdmiCt->hdmi_arc_flag = 1;//收到0xc2时，会把该标志改为0
					timeout_time = 1000;
				}
				while(hdmiInPlayCt->hdmiRetransCnt)
				{
					if(HDMI_HPD_NOT_CONNECTED_STATUS == HDMI_HPD_StatusGet())
					{
						APP_DBG("HDMI line is inactive\n");
						break;
					}
					while(HDMI_CEC_IsWorking() == CEC_IS_WORKING)
					{
						WDG_Feed();
					}

					HDMI_CEC_TerminationARC();
					TimeOutSet(&hdmiInPlayCt->hdmiMaxRespondTime, timeout_time);
					while(!IsTimeOut(&hdmiInPlayCt->hdmiMaxRespondTime))
					{
						HDMI_CEC_Scan(1);
						if(gHdmiCt->hdmi_arc_flag == 0)
						{
							APP_DBG("Terminal arc ok, resend cnt: %d\n", 3-hdmiInPlayCt->hdmiRetransCnt);
							break;
						}
						WDG_Feed();
					}
					if(gHdmiCt->hdmi_arc_flag == 0)
					{
						break;
					}
					hdmiInPlayCt->hdmiRetransCnt --;
				}

				gHdmiCt->hdmi_arc_flag = 0;
			}
			if(HDMI_HPD_NOT_CONNECTED_STATUS != HDMI_HPD_StatusGet())
			{
				while(HDMI_CEC_IsWorking() == CEC_IS_WORKING)
				{
					WDG_Feed();
				}
				HDMI_CEC_SetSystemAudioModeoff();
				while(HDMI_CEC_IsWorking() == CEC_IS_WORKING)
				{
					WDG_Feed();
				}
				HDMI_CEC_SetSystemAudioModeoff();
				while(HDMI_CEC_IsWorking() == CEC_IS_WORKING)
				{
					WDG_Feed();
				}
			}
		}
		gHdmiCt->hdmi_audiomute_flag = 0;
		gHdmiCt->hdmi_poweron_flag = 0;
	}
#ifdef AUDIO_STREAM_UNINTERRUPT
	if(IsResRamRelease == TRUE)
#endif
	{
		AudioCoreProcessConfig((void*)AudioNoAppProcess);
	}
	AudioCoreSourceDisable(HDMI_IN_SOURCE_NUM);
	AudioCoreSourceDeinit(HDMI_IN_SOURCE_NUM);

	HDMI_CEC_DDC_DeInit();
	HDMI_ARC_DeInit();

	if(hdmiInPlayCt->hdmiARCFIFO != NULL)
	{
		osPortFree(hdmiInPlayCt->hdmiARCFIFO);
		hdmiInPlayCt->hdmiARCFIFO = NULL;
	}

	if(hdmiInPlayCt->sourceBuf_ARC != NULL)
	{
		osPortFree(hdmiInPlayCt->sourceBuf_ARC);
		hdmiInPlayCt->sourceBuf_ARC = NULL;
	}

	if(hdmiInPlayCt->hdmiARCCarry != NULL)
	{
		osPortFree(hdmiInPlayCt->hdmiARCCarry);
		hdmiInPlayCt->hdmiARCCarry = NULL;
	}

#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
	if(hdmiInPlayCt->hdmiARCCarry24 != NULL)
	{
		osPortFree(hdmiInPlayCt->hdmiARCCarry24);
		hdmiInPlayCt->hdmiARCCarry24 = NULL;
	}
#endif

	if(hdmiInPlayCt->hdmiARCPcmFifo != NULL)
	{
		osPortFree(hdmiInPlayCt->hdmiARCPcmFifo);
		hdmiInPlayCt->hdmiARCPcmFifo = NULL;
	}
#ifdef AUDIO_STREAM_UNINTERRUPT
	if(IsResRamRelease == TRUE)
#endif
	{
		ModeCommonDeinit();//通路全部释放
	}
	osPortFree(hdmiInPlayCt);
	hdmiInPlayCt = NULL;

	return TRUE;
}

#endif  //CFG_APP_HDMIIN_MODE_EN
