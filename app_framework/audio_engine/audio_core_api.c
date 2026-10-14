/**
 **************************************************************************************
 * @file    audio_core.c
 * @brief   audio core 
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2016-6-29 13:06:47$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include "type.h"
#include <string.h>
#include <nds32_intrinsic.h>
#include "dma.h"
//driverapi/middleware
#include "dac_interface.h"
#include "mcu_circular_buf.h"

#include "app_config.h"
#include "debug.h"
#include "main_task.h"
#include "audio_core_service.h"
#include "effect_service.h"
#include "audio_core_api.h"
//Port#include "ctrlvars.h"
//Port#include "audio_effect.h"
#include "roboeffect_api.h"
#include "beep.h"
#include "audio_effect_parambin.h"
#include "audio_vol.h"
#ifdef CFG_APP_BT_MODE_EN
#include "bt_config.h"
#include "bt_play_api.h"
#include "bt_manager.h"
#if BT_HFP_SUPPORT
#include "bt_hf_api.h"
#endif
#endif


/**************************************************************************************************************************
 *
 *				 |***GetAdapter***|	  			 |*****MixNet Process******|			  |***SetAdapter***|
 * ************	 ******************	 **********	 ***************************  **********  ******************  **********
 *	SourceFIFO*->*SRCFIFO**SRAFIFO*->*PCMFrame*->*   Gain + Effect + Mix   *->*PCMFrame*->*SRAFIFO**SRCFIFO*->*SinkFIFO*
 * ************  ******************	 **********	 ***************************  **********  ******************  **********
 * 				 |*Context*|																	 |*Context*|
 * |*********Source PCMWidth******|  |***************MixNet PCMWidth*******************|  |*********Sink PcmWidth******|
 **************************************************************************************************************************/

AudioCoreContext		AudioCore;

void AudioCoreSourcePcmFormatConfig(uint8_t Index, uint16_t Format)
{
	if(Index < AUDIO_CORE_SOURCE_MAX_NUM)
	{
		AudioCore.AudioSource[Index].Channels = Format;
	}
}

void AudioCoreSourceEnable(uint8_t Index)
{
	if(Index < AUDIO_CORE_SOURCE_MAX_NUM)
	{
		AudioCore.AudioSource[Index].Enable = TRUE;
		AudioCoreAdjustIORenew();
		AudioCore.AudioFlowRenew = TRUE;
	}
}

void AudioCoreSourceDisable(uint8_t Index)
{
	if(Index < AUDIO_CORE_SOURCE_MAX_NUM)
	{
		AudioCore.AudioSource[Index].Enable = FALSE;
		AudioCoreAdjustIORenew();
		AudioCore.AudioFlowRenew = TRUE;
	}
}

bool AudioCoreSourceIsEnable(uint8_t Index)
{
	return AudioCore.AudioSource[Index].Enable;
}

void AudioCoreSourceMute(uint8_t Index, bool IsLeftMute, bool IsRightMute)
{
	if(IsLeftMute)
	{
		AudioCore.AudioSource[Index].LeftMuteFlag = TRUE;
	}
	if(IsRightMute)
	{
		AudioCore.AudioSource[Index].RightMuteFlag = TRUE;
	}
}

void AudioCoreSourceUnmute(uint8_t Index, bool IsLeftUnmute, bool IsRightUnmute)
{
	if(IsLeftUnmute)
	{
		AudioCore.AudioSource[Index].LeftMuteFlag = FALSE;
	}
	if(IsRightUnmute)
	{
		AudioCore.AudioSource[Index].RightMuteFlag = FALSE;
	}
}

bool AudioCoreSourceMuteStatusGet(uint8_t Index)
{
	return AudioCore.AudioSource[Index].LeftMuteFlag | AudioCore.AudioSource[Index].RightMuteFlag;
}

void AudioCoreSourceConfig(uint8_t Index, AudioCoreSource* Source)
{
	memcpy(&AudioCore.AudioSource[Index], Source, sizeof(AudioCoreSource));
}

void AudioCoreSinkEnable(uint8_t Index)
{
	AudioCore.AudioSink[Index].Enable = TRUE;
	AudioCoreAdjustIORenew();
	AudioCore.AudioFlowRenew = TRUE;
}

void AudioCoreSinkDisable(uint8_t Index)
{
	AudioCore.AudioSink[Index].Enable = FALSE;
	AudioCoreAdjustIORenew();
	AudioCore.AudioFlowRenew = TRUE;
}

bool AudioCoreSinkIsEnable(uint8_t Index)
{
	return AudioCore.AudioSink[Index].Enable;
}

void AudioCoreSinkConfig(uint8_t Index, AudioCoreSink* Sink)
{
	memcpy(&AudioCore.AudioSink[Index], Sink, sizeof(AudioCoreSink));
}


void AudioCoreProcessConfig(AudioCoreProcessFunc AudioEffectProcess)
{
	AudioCore.AudioEffectProcess = AudioEffectProcess;
}

///**
// * @func        AudioCoreConfig
// * @brief       AudioCore参数块，本地化API
// * @param       AudioCoreContext *AudioCoreCt
// * @Output      None
// * @return      None
// * @Others      外部配置的参数块，复制一份到本地
// */
//void AudioCoreConfig(AudioCoreContext *AudioCoreCt)
//{
//	memcpy(&AudioCore, AudioCoreCt, sizeof(AudioCoreContext));
//}

bool AudioCoreInit(void)
{
	int i;
	DBG("AudioCore init\n");
	for(i = 0; i < MaxNet; i++)
	{
		AudioCore.NetCt[i].SampleRate = CFG_DEFAULT_SAMPLERATE;
		AudioCore.NetCt[i].FrameSize = CFG_MAX_FRAME;
		AudioCore.NetCt[i].FrameUs = 1000000 * CFG_MAX_FRAME / CFG_DEFAULT_SAMPLERATE;
		AudioCore.NetCt[i].PcmWidth = CFG_DEFAULT_WIDTH;
		AudioCore.NetCt[i].State = MIX_STATE_IDLE;
#ifdef FRAME_SYNC_TRIGER_EN
		AudioCore.NetCt[i].TrigerMsgFunc = NULL;
#endif
	}
#ifdef CFG_FUNC_MCLK_ADJUST
	TimeOutSet(&AdaptHwCt.AdjTimer, ADJUST_APLL_PERIOD);
	AdaptHwCt.AdjustIO = ADJUST_APLL_IDLE;
#endif
	AudioCore.AudioFlowRenew = TRUE;
#if defined(AUDIOCORE_HWSRC_EN)
#ifdef HWSRC0_FIX_CASE
	AdaptHwCt.HwSrcCt[0].SrcCase = HWSRC0_FIX_CASE;
#else
	AdaptHwCt.HwSrcCt[0].SrcCase = HWSRC_IDLE;
#endif
#ifdef HWSRC1_FIX_CASE
	AdaptHwCt.HwSrcCt[1].SrcCase = HWSRC1_FIX_CASE;
#else
	AdaptHwCt.HwSrcCt[1].SrcCase = HWSRC_IDLE;
#endif
#ifdef HWSRC2_FIX_CASE
	AdaptHwCt.HwSrcCt[2].SrcCase = HWSRC2_FIX_CASE;
#else
	AdaptHwCt.HwSrcCt[2].SrcCase = HWSRC_IDLE;
#endif
#endif
	return TRUE;
}

void AudioCoreDeinit(void)
{
	AudioCore.NetCt[DefaultNet].State = MIX_STATE_IDLE;
#if defined(ROBOEFFECT_DOUBLE_ENGINE_EN)
	AudioCore.NetCt[SeparateNet].State = MIX_STATE_IDLE;
#endif
}

/**
 * @func        AudioCoreRun
 * @brief       音源拉流->音效处理+混音->推流
 * @param       None
 * @Output      None
 * @return      None
 * @Others      当前由audioCoreservice任务保障此功能有效持续。
 * Record
 */
//extern uint8_t 	IsAudioCorePause;
//extern void BtPlayACBtMonitor(uint16_t empty_flag);
void AudioProcessMain(void);
//__attribute__((optimize("Og")))
__attribute__((section(".ilm_section"),optimize("Og")))
void AudioCoreRun(void)
{
	NetContext *PNetCt;
	uint8_t LoopCnt = 0;
#ifdef CFG_FUNC_MCLK_ADJUST
	if(IsTimeOut(&AdaptHwCt.AdjTimer))
	{
		TimeOutSet(&AdaptHwCt.AdjTimer, ADJUST_APLL_PERIOD);
	}
#endif
	while(++LoopCnt)
	{
		AudioCore.ScanNet =  (AudioCore.ScanNet + 1) % MaxNet;
		PNetCt = &AudioCore.NetCt[AudioCore.ScanNet];
		if((AudioCore.Pause || AudioCore.AudioFlowRenew)
			&& (AudioCore.NetCt[DefaultNet].State == MIX_STATE_IDLE)
#if defined(ROBOEFFECT_DOUBLE_ENGINE_EN)
			&& (AudioCore.NetCt[SeparateNet].State == MIX_STATE_IDLE)
#endif
			)
		{
			if(AudioCore.Pause == 1)
			{
				MessageContext		msgSend;
				msgSend.msgId		= MSG_AUDIO_CORE_HOLD;
				MessageSend(GetAudioCoreServiceMsgHandle(), &msgSend);
				AudioCore.Pause = 2;
			}
			else if(AudioCore.AudioFlowRenew && !AudioCore.Pause)
			{
				memset(AudioCore.AudioSourceRecord, 0, sizeof(AudioCore.AudioSourceRecord));
				AudioCoreSourceEraseInfo(AudioCore.AudioSourceRecord);
				AudioCoreSinkEraseInfo(AudioCore.AudioSinkRecord);
				AudioCore.AudioFlowRenew = FALSE;
#if 1// defined(FRAME_SYNC_TRIGER_EN) && defined(CFG_PARA_CORE2_NET) && (CFG_PARA_CORE2_NET == SeparateNet)
				SendEffectServiceMsg(MSG_NONE);
#endif
			}
			return;
		}
		else
		{
			AudioCoreIOLenProcess(AudioCore.ScanNet);
#if defined(CFG_FUNC_EFFECT_CORE_DUAL_EN)
			if(AudioCore.ScanNet != CFG_PARA_CORE2_NET)
#endif
			{
				switch(PNetCt->State)
				{
					case MIX_STATE_IDLE:
						if(AudioCoreSyncing(AudioCore.ScanNet) == FALSE)
						{
							break;
						}
						AudioCoreSourceMuteApply(AudioCore.ScanNet);
						PNetCt->State = MIX_STATE_GOT;
					case MIX_STATE_GOT:
						if(AudioCore.AudioEffectProcess != NULL)
						{
	#if defined(CFG_FUNC_EFFECT_SERVICE_EN) && !defined(CFG_FUNC_EFFECT_CORE_DUAL_EN)
		#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
							if(AudioCore.NetCt[(AudioCore.ScanNet + 1) % MaxNet].State == MIX_STATE_SYNC)
							{
								return ;//免重入
							}
		#endif
							if(!GetEffectProcess())
							{
								EffectProcessConfig(AudioCore.AudioEffectProcess, AudioCore.ScanNet);
								PNetCt->State = MIX_STATE_SYNC;
							}
	#else
							AudioCore.AudioEffectProcess((AudioCoreContext*)&AudioCore, AudioCore.ScanNet);
	#endif
						}
						else
							return;
	#if defined(CFG_FUNC_EFFECT_SERVICE_EN) && !defined(CFG_FUNC_EFFECT_CORE_DUAL_EN)
					case MIX_STATE_SYNC:
						if(GetEffectProcess())
						{
							break;
						}
	#endif
	#ifdef CFG_FUNC_BEEP_EN
						if(AudioCore.AudioSink[AUDIO_DAC_SINK_NUM].Active == TRUE)   ////dacx buff
						{
							Beep(AudioCore.AudioSink[AUDIO_DAC_SINK_NUM].PcmOutBuf, SINKFRAME(AUDIO_DAC_SINK_NUM));
						}
	#endif

						AudioCoreSinkMuteApply(AudioCore.ScanNet);
						AudioCoreSynced(AudioCore.ScanNet);
						PNetCt->State = MIX_STATE_IDLE;
						break;
					default:
						return;
				}
			}
		}

		if(LoopCnt > MaxNet
			&& (AudioCore.NetCt[DefaultNet].State == MIX_STATE_IDLE || AudioCore.NetCt[DefaultNet].State == MIX_STATE_SYNC)
			&& (AudioCore.NetCt[DefaultNet].AdaptProcessed == FALSE)
#if defined(ROBOEFFECT_DOUBLE_ENGINE_EN)
	#if!defined(CFG_FUNC_EFFECT_CORE_DUAL_EN)
			&& (AudioCore.NetCt[SeparateNet].State == MIX_STATE_IDLE || AudioCore.NetCt[SeparateNet].State == MIX_STATE_SYNC)
	#endif
			&& (AudioCore.NetCt[SeparateNet].AdaptProcessed == FALSE)
#endif
						)
		{
			return ;
		}
	}
}

extern const EnumStringMap audio_source_map[];
extern const EnumStringMap audio_sink_map[];
void AudioCoreSourceEraseInfo(AudioEraseSourceRecord* out_records)
{
	uint32_t num = 0;
	uint32_t i;
#ifdef CFG_FUNC_ROBOEFFECT_EN
	if(EffectParambin.context_memory == NULL)
		return;
	roboeffect_effect_steps_table * totle = NULL;
	totle = (roboeffect_effect_steps_table *)roboeffect_parambin_get_sub_type((uint8_t *)EffectParambin.flow_data, EffectParambin.flow_size, ROBO_PB_SUBTYPE_FLOW_INFO, TRUE, NULL);
	if(totle == NULL)
	{
		DBG("EffectParambin error\n");
		return;
	}
	for(i = 0; (i < totle->src_unit_num); i++)
	{
		out_records[num].address = (uint32_t)roboeffect_get_source_buffer(EffectParambin.context_memory, i);
		out_records[num].framelength = roboeffect_get_buffer_size(EffectParambin.context_memory);
		out_records[num].flownum = 0;
		num++;
	}
	#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
	if(EffectParambin2.context_memory == NULL)
		return;

	totle = (roboeffect_effect_steps_table *)roboeffect_parambin_get_sub_type((uint8_t *)EffectParambin2.flow_data, EffectParambin2.flow_size, ROBO_PB_SUBTYPE_FLOW_INFO, TRUE, NULL);
	for(i = 0; (totle != NULL) && (i < totle->src_unit_num); i++)
	{
		out_records[num].address = (uint32_t)roboeffect_get_source_buffer(EffectParambin2.context_memory, i);
		out_records[num].framelength = roboeffect_get_buffer_size(EffectParambin2.context_memory);
		out_records[num].flownum = 1;
		num++;
	}
	#endif
#endif
}

void AudioCoreSinkEraseInfo(AudioEraseSinkRecord* out_records)
{
	uint32_t num = 0;
	uint32_t i;
	uint32_t a;
#ifdef CFG_FUNC_ROBOEFFECT_EN
	if(EffectParambin.context_memory == NULL)
		return;
	bool MatchOK = false;
	roboeffect_effect_steps_table * totle = NULL;
	totle = (roboeffect_effect_steps_table *)roboeffect_parambin_get_sub_type((uint8_t *)EffectParambin.flow_data, EffectParambin.flow_size, ROBO_PB_SUBTYPE_FLOW_INFO, TRUE, NULL);
	if(totle == NULL)
	{
		DBG("EffectParambin error\n");
		return;
	}
	for(i = 0; (i < totle->des_unit_num); i++)
	{
		for(a = 0; a < AUDIO_CORE_SINK_MAX_NUM; a++)
		{
			if(strcmp(AudioEffect_Parambin_GetSinkNameByIndex(&EffectParambin, i), audio_sink_map[a].enum_str) == 0)
			{
				if(!AudioCore.AudioSink[a].Enable)
				{
					out_records[num].address = (uint32_t)roboeffect_get_sink_buffer(EffectParambin.context_memory, i);
					out_records[num].framelength = roboeffect_get_buffer_size(EffectParambin.context_memory);
					out_records[num].flownum = 0;
					num++;
				}
				MatchOK = true;
				break;
			}
			else
			{
				MatchOK = false;
			}
		}

		if(MatchOK == false)
		{
			out_records[num].address = (uint32_t)roboeffect_get_sink_buffer(EffectParambin.context_memory, i);
			out_records[num].framelength = roboeffect_get_buffer_size(EffectParambin.context_memory);
			out_records[num].flownum = 0;
			num++;
		}
		MatchOK = false;
	}
	#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
	if(EffectParambin2.context_memory == NULL)
		return;
	MatchOK = false;
	totle = (roboeffect_effect_steps_table *)roboeffect_parambin_get_sub_type((uint8_t *)EffectParambin2.flow_data, EffectParambin2.flow_size, ROBO_PB_SUBTYPE_FLOW_INFO, TRUE, NULL);

	for(i = 0; (totle != NULL) && (i < totle->des_unit_num); i++)
	{
		for(a = 0; a < AUDIO_CORE_SINK_MAX_NUM; a++)
		{
			if(strcmp(AudioEffect_Parambin_GetSinkNameByIndex(&EffectParambin2, i), audio_sink_map[a].enum_str) == 0)
			{
				if(!(AudioCore.AudioSink[a].Enable))
				{
					out_records[num].address = (uint32_t)roboeffect_get_sink_buffer(EffectParambin2.context_memory, i);
					out_records[num].framelength = roboeffect_get_buffer_size(EffectParambin2.context_memory);
					out_records[num].flownum = 1;
					num++;
				}

				MatchOK = true;
				break;
			}
			else
			{
				MatchOK = false;
			}
		}

		if(MatchOK == false)
		{
			out_records[num].address = (uint32_t)roboeffect_get_sink_buffer(EffectParambin2.context_memory, i);
			out_records[num].framelength = roboeffect_get_buffer_size(EffectParambin2.context_memory);
			out_records[num].flownum = 1;
			num++;
		}
		MatchOK = false;
	}
	#endif
#endif
}

void AudioFlowSourceClear(uint8_t FlowNet)
{
	int i = 0;
	while(i < sizeof(AudioCore.AudioSourceRecord)/sizeof(AudioEraseSourceRecord) && AudioCore.AudioSourceRecord[i].address != 0)
	{
		if(AudioCore.AudioSourceRecord[i].flownum == FlowNet)
		{
			memset((void *)AudioCore.AudioSourceRecord[i].address, 0, AudioCore.AudioSourceRecord[i].framelength);
		}
		i++;
	}
}

void AudioCoreSourceMuteApply(uint8_t FlowNet)
{
	uint32_t i;
	uint8_t source_index;
	uint16_t LeftVol, RightVol, TargetVol, LeftVolStep, RightVolStep;
	bool mute;
	AudioCoreSource *pSourceCt = NULL;
	for(source_index = 0; source_index < AUDIO_CORE_SOURCE_MAX_NUM; source_index++)
	{
		pSourceCt = &AudioCore.AudioSource[source_index];
		mute = pSourceCt->LeftMuteFlag || pSourceCt->RightMuteFlag || mainAppCt.gSysVol.MuteFlag;
		if((!pSourceCt->InitFlag) || (pSourceCt->Net != FlowNet) || (mute == pSourceCt->MuteFlagbk))
		{
			if((pSourceCt->InitFlag) && (pSourceCt->Net == FlowNet) && (mainAppCt.gSysVol.MuteFlag || pSourceCt->LeftMuteFlag))
				memset(pSourceCt->PcmInBuf, 0, SOURCEFRAME(source_index) * pSourceCt->FlowCh * PCM_BYTE(pSourceCt->FlowWidth));
			pSourceCt->MuteFlagbk = mute;
			continue;
		}

		TargetVol = mute ? 0:4096;
		LeftVol = 4096 - TargetVol;
		RightVol = LeftVol;
		LeftVolStep = 4096 / SOURCEFRAME(source_index) + (4096 % SOURCEFRAME(source_index) ? 1 : 0);
		RightVolStep = LeftVolStep;
#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
		if(pSourceCt->FlowWidth == WIDTH_L24BIT) //24bit 数据
		{
			if(pSourceCt->FlowCh== 2)
			{
				for(i=0; i < SOURCEFRAME(source_index); i++)
				{
					pSourceCt->PcmInBuf[2 * i + 0] = __nds32__clips((((int64_t)pSourceCt->PcmInBuf[2 * i + 0]) * LeftVol + 2048) >> 12, (24)-1);
					pSourceCt->PcmInBuf[2 * i + 1] = __nds32__clips((((int64_t)pSourceCt->PcmInBuf[2 * i + 1]) * RightVol + 2048) >> 12, (24)-1);

					MixerFadeVolume(LeftVol, TargetVol, LeftVolStep);
					MixerFadeVolume(RightVol, TargetVol, RightVolStep);
				}
			}
			else if(pSourceCt->FlowCh == 1)
			{
				for(i=0; i<SOURCEFRAME(source_index); i++)
				{
					pSourceCt->PcmInBuf[i] = __nds32__clips((((int64_t)pSourceCt->PcmInBuf[i]) * LeftVol + 2048) >> 12, (24)-1);

					MixerFadeVolume(LeftVol, TargetVol, LeftVolStep);
				}
			}
		}
		else
#endif
		{
			int16_t * PcmInBuf = (int16_t *)pSourceCt->PcmInBuf;
			if(pSourceCt->FlowCh == 2)
			{
				for(i=0; i < SOURCEFRAME(source_index); i++)
				{
					PcmInBuf[2 * i + 0] = __nds32__clips((((int32_t)PcmInBuf[2 * i + 0]) * LeftVol + 2048) >> 12, (16)-1);
					PcmInBuf[2 * i + 1] = __nds32__clips((((int32_t)PcmInBuf[2 * i + 1]) * RightVol + 2048) >> 12, (16)-1);

					MixerFadeVolume(LeftVol, TargetVol, LeftVolStep);
					MixerFadeVolume(RightVol, TargetVol, RightVolStep);
				}
			}
			else if(pSourceCt->FlowCh == 1)
			{
				for(i=0; i<SOURCEFRAME(source_index); i++)
				{
					PcmInBuf[i] = __nds32__clips((((int32_t)PcmInBuf[i]) * LeftVol + 2048) >> 12, (16)-1);

					MixerFadeVolume(LeftVol, TargetVol, LeftVolStep);
				}
			}
		}
		pSourceCt->MuteFlagbk = mute;
	}
}
void AudioCoreSinkMuteApply(uint8_t FlowNet)
{
	uint32_t i;
	uint8_t sink_index;
	uint16_t LeftVol, RightVol, TargetVol, LeftVolStep, RightVolStep;
	bool mute;
	AudioCoreSink * pSinkCt = NULL;
	for(sink_index = 0; sink_index < AUDIO_CORE_SINK_MAX_NUM; sink_index++)
	{
		pSinkCt = &AudioCore.AudioSink[sink_index];
		mute = pSinkCt->LeftMuteFlag || pSinkCt->RightMuteFlag || mainAppCt.gSysVol.MuteFlag;
		if((!pSinkCt->InitFlag) || (pSinkCt->Net != FlowNet) || (mute == pSinkCt->MuteFlagbk))
		{
			if((pSinkCt->InitFlag) && (pSinkCt->Net == FlowNet) && (mainAppCt.gSysVol.MuteFlag || pSinkCt->LeftMuteFlag))
				memset(pSinkCt->PcmOutBuf, 0, SINKFRAME(sink_index) * pSinkCt->FlowCh * PCM_BYTE(pSinkCt->FlowWidth));
			pSinkCt->MuteFlagbk = mute;
			continue;
		}

		TargetVol = mute ? 0:4096;
		LeftVol = 4096 - TargetVol;
		RightVol = LeftVol;
		LeftVolStep = 4096 / SINKFRAME(sink_index) + (4096 % SINKFRAME(sink_index) ? 1 : 0);
		RightVolStep = LeftVolStep;

#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
		if(pSinkCt->FlowWidth == WIDTH_L24BIT) //24bit 数据
		{
			if(pSinkCt->FlowCh == 2)
			{
				for(i=0; i < SINKFRAME(sink_index); i++)
				{
					pSinkCt->PcmOutBuf[2 * i + 0] = __nds32__clips((((int64_t)pSinkCt->PcmOutBuf[2 * i + 0]) * LeftVol + 2048) >> 12, (24)-1);
					pSinkCt->PcmOutBuf[2 * i + 1] = __nds32__clips((((int64_t)pSinkCt->PcmOutBuf[2 * i + 1]) * RightVol + 2048) >> 12, (24)-1);

					MixerFadeVolume(LeftVol, TargetVol, LeftVolStep);
					MixerFadeVolume(RightVol, TargetVol, RightVolStep);
				}
			}
			else if(pSinkCt->FlowCh == 1)
			{
				for(i=0; i<SINKFRAME(sink_index); i++)
				{
					pSinkCt->PcmOutBuf[i] = __nds32__clips((((int64_t)pSinkCt->PcmOutBuf[i]) * LeftVol + 2048) >> 12, (24)-1);

					MixerFadeVolume(LeftVol, TargetVol, LeftVolStep);
				}
			}
		}
		else
#endif
		{
			int16_t * PcmOutBuf = (int16_t *)pSinkCt->PcmOutBuf;
			if(pSinkCt->FlowCh == 2)
			{
				for(i=0; i < SINKFRAME(sink_index); i++)
				{
					PcmOutBuf[2 * i + 0] = __nds32__clips((((int32_t)PcmOutBuf[2 * i + 0]) * LeftVol + 2048) >> 12, (16)-1);
					PcmOutBuf[2 * i + 1] = __nds32__clips((((int32_t)PcmOutBuf[2 * i + 1]) * RightVol + 2048) >> 12, (16)-1);

					MixerFadeVolume(LeftVol, TargetVol, LeftVolStep);
					MixerFadeVolume(RightVol, TargetVol, RightVolStep);
				}
			}
			else if(pSinkCt->FlowCh == 1)
			{
				for(i=0; i<SINKFRAME(sink_index); i++)
				{
					PcmOutBuf[i] = __nds32__clips((((int32_t)PcmOutBuf[i]) * LeftVol + 2048) >> 12, (16)-1);

					MixerFadeVolume(LeftVol, TargetVol, LeftVolStep);
				}
			}
		}
		pSinkCt->MuteFlagbk = mute;
	}
}
