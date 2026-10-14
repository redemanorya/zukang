/*
 * audio_core_adapt.c
 *
 *  Created on: Mar 11, 2021
 *      Author: piwang
 */
//driver、lib
#include <nds32_intrinsic.h>
#include "string.h"
#include "clk.h"
#include "dac.h"
#include "reset.h"
#include "resampler_polyphase.h"
#include "resampler_sinc.h"

//application
#include "app_config.h"
#include "rtos_api.h"
#include "src_interface.h"
#include "debug.h"
#include "mixer.h"
#include "audio_core_api.h"
#include "audio_core_adapt.h"
#include "mode_task.h"
#include "audio_vol.h"
#include "bt_manager.h"
//Port#include "main_task.h"
//Port#include "ctrlvars.h"

#ifdef CFG_FUNC_SRC_POLYPHASE_EN
	#define SRC_APPLY_24(SAMPLES)	resampler_polyphase_apply24(&SrcAdapter->SrcCt, (int32_t *)AudioCore.AdaptIn, (int32_t *)AudioCore.AdaptOut, SAMPLES)
	#define SRC_APPLY_16(SAMPLES)	resampler_polyphase_apply(&SrcAdapter->SrcCt, (int16_t *)AudioCore.AdaptIn, (int16_t *)AudioCore.AdaptOut, SAMPLES)
#elif defined(CFG_FUNC_SRC_SINC_EN)
	#define SRC_APPLY_24(SAMPLES)	resampler_sinc_apply24(SrcAdapter->SincCt, ((int32_t *)AudioCore.AdaptIn), ((int32_t *)AudioCore.AdaptOut), SAMPLES)
	#define SRC_APPLY_16(SAMPLES)	resampler_sinc_apply(SrcAdapter->SincCt, (int16_t *)AudioCore.AdaptIn, (int16_t *)AudioCore.AdaptOut, SAMPLES);
#else
	#error Conflict: 软件SRC配置缺失!!!
#endif

extern uint16_t AudioDAC0DataLenGet(void);
extern bool gain_control_apply(int16_t *pcm_in, int16_t *pcm_out, uint32_t n, int16_t mute, int16_t gain, int8_t ch_num);
#if defined(CFG_FUNC_DEBUG_EN) || defined(CFG_FUNC_USBDEBUG_EN)
	#define AUDIO_CORE_DEBUG 	//强化配置安全检测和定位
#endif
#if defined(CFG_DEBUG_DAC_EMPTY_EN) && defined(CFG_RES_DAC_MONITOR_EN)
	uint32_t DacEmptyCnt;
#endif

AdaptHwResource AdaptHwCt;
#if defined(HWSRC_MODE_LIST_SCAN_EN) && defined(AUDIOCORE_HWSRC_EN)
const uint8_t HwSrcModeList[RES_SRC_NUM][SRC_MODE_LIST_MAX][2] = {
	#ifdef HWSRC0_MODE_LIST
		HWSRC0_MODE_LIST,
	#endif
	#ifdef HWSRC1_MODE_LIST
		HWSRC1_MODE_LIST,
	#endif
	#ifdef HWSRC2_MODE_LIST
		HWSRC2_MODE_LIST
	#endif
};
#endif
const uint32_t AudioCoreSampleRateTBL[] = CORE_SAMPLERATE_LIST;

RESAMPLER_POLYPHASE_SRC_RATIO GetRatioEnum(uint32_t Scale1000)
{
	switch(Scale1000)
	{
		case (1000 * 6):
			return RESAMPLER_POLYPHASE_SRC_RATIO_6_1;
		case (1000 * 441 / 80):
			return RESAMPLER_POLYPHASE_SRC_RATIO_441_80;
		case (1000 * 640 / 147):
			return RESAMPLER_POLYPHASE_SRC_RATIO_640_147;
		case (1000 * 4):
			return RESAMPLER_POLYPHASE_SRC_RATIO_4_1;
		case (1000 * 147 / 40):
			return RESAMPLER_POLYPHASE_SRC_RATIO_147_40;
		case (1000 * 3):
			return RESAMPLER_POLYPHASE_SRC_RATIO_3_1;
		case (1000 * 441 / 160):
			return RESAMPLER_POLYPHASE_SRC_RATIO_441_160;
		case (1000 * 320 / 147):
			return RESAMPLER_POLYPHASE_SRC_RATIO_320_147;
		case (1000 * 2):
			return RESAMPLER_POLYPHASE_SRC_RATIO_2_1;
		case (1000 * 147 / 80):
			return RESAMPLER_POLYPHASE_SRC_RATIO_147_80;
		case (1000 * 3 / 2):
			return RESAMPLER_POLYPHASE_SRC_RATIO_3_2;
		case (1000 * 441 / 320):
			return RESAMPLER_POLYPHASE_SRC_RATIO_441_320;
		case (1000 * 4 / 3):
			return RESAMPLER_POLYPHASE_SRC_RATIO_4_3;
		case (1000 * 160 / 147):
			return RESAMPLER_POLYPHASE_SRC_RATIO_160_147;
/*****************************************************************/
		case (1000 * 147 / 160):
			return RESAMPLER_POLYPHASE_SRC_RATIO_147_160;
		case (1000 * 3 / 4):
			return RESAMPLER_POLYPHASE_SRC_RATIO_3_4;
		case (1000 * 80 / 147):
			return RESAMPLER_POLYPHASE_SRC_RATIO_80_147;
		case (1000 / 2):
			return RESAMPLER_POLYPHASE_SRC_RATIO_1_2;
		case (1000 * 147 / 320):
			return RESAMPLER_POLYPHASE_SRC_RATIO_147_320;
		case (1000 * 160 / 441):
			return RESAMPLER_POLYPHASE_SRC_RATIO_160_441;
		case (1000 / 3):
			return RESAMPLER_POLYPHASE_SRC_RATIO_1_3;
		case (1000 * 40 / 147):
			return RESAMPLER_POLYPHASE_SRC_RATIO_40_147;
		case (1000 / 4):
			return RESAMPLER_POLYPHASE_SRC_RATIO_1_4;
		case (1000 * 147 / 640):
			return RESAMPLER_POLYPHASE_SRC_RATIO_147_640;
		case (1000 * 320 / 441):
			return RESAMPLER_POLYPHASE_SRC_RATIO_320_441;
		case (1000 * 2 / 3):
			return RESAMPLER_POLYPHASE_SRC_RATIO_2_3;
		default:
#ifdef AUDIO_CORE_DEBUG
			if(Scale1000 != 1000)
			{
				DBG("SRC Samplerate Error %lu!\n", Scale1000);
			}
#endif
			return RESAMPLER_POLYPHASE_SRC_RATIO_UNSUPPORTED;
	}
}
//根据转采样比例，输入数据和输出空间，获取合适的输入长度，返回 SRC_INPUT_MIN~CFG_MAX_FRAME
uint16_t	SRCValidLenGet(uint32_t SampleRateIn, uint32_t AudioLen, uint32_t SampleRateOut, uint32_t SpaceLen)
{
	uint16_t ValidData = 0;
	uint16_t ValidSpace = sizeof(AudioCore.AdaptOut) / 4;
	if(SpaceLen >= SRC_OUPUT_MIN && AudioLen >= (SampleRateIn) *(SRC_OUPUT_MIN - SRC_OUPUT_JITTER)/(SampleRateOut))
	{
		if(SpaceLen < ValidSpace)
		{
			ValidSpace = SpaceLen;
		}
		ValidData = SampleRateIn *(ValidSpace - SRC_OUPUT_JITTER)/(SampleRateOut);
		if(ValidData > AudioLen)
		{
			ValidData = AudioLen;
		}

		if(ValidData < SRC_INPUT_MIN)
		{
			ValidData = 0;
		}
		else if(ValidData > CFG_MAX_FRAME)
		{
			ValidData = CFG_MAX_FRAME;
		}
	}
	return ValidData;
}
#if defined(AUDIOCORE_HWSRC_EN)
//根据输入和输出采样时长与Applyu Buf限制，获取合适的输入长度
uint16_t	HWSRCValidLenGet(uint32_t SampleRateIn, uint32_t AudioLen, uint32_t SampleRateOut, uint32_t SpaceLen)
{
	uint32_t SampleTime = (uint64_t)AudioLen * 1000000 / SampleRateIn;
	int32_t OutSampleTime = ((int64_t)SpaceLen - SRC_OUPUT_JITTER) * 1000000 / SampleRateOut;
	uint16_t VaildLen;

	if(SampleTime > OutSampleTime)
	{
		SampleTime = OutSampleTime;
	}
	if(SampleTime > HWSRC_BUF_TIME)
	{
		SampleTime = HWSRC_BUF_TIME;
	}
#ifdef HWSRC_MIN_TIME
	if(SampleTime < HWSRC_MIN_TIME)
	{
		SampleTime = 0;
	}
#endif
	VaildLen = (uint64_t)SampleRateIn * SampleTime /1000000;
	if(VaildLen < SRC_OUPUT_MIN)
	{
		VaildLen = 0;
	}

	return VaildLen;
}
#endif
//使用此宏的参数 注意不要与中断变参 耦合
#define SRAValidLenGet(AudioLen, SpaceLen)	(((AudioLen) >= ADAPTER_INPUT_BLOCK && (SpaceLen) >= (ADAPTER_INPUT_BLOCK + ADAPTER_MAX_CHG)) ? ADAPTER_INPUT_BLOCK : 0)

//1 Low 0 High -1
#define ADJLEVEL(LEVEL, LOW, HIGH)			((LEVEL) < (LOW) ? 1 : ((LEVEL) > ( HIGH)? (-1) : (0)))
#define LEVELDELTA(LEVEL, LOW, HIGH)		((LEVEL) < (LOW) ? (0) : ((LEVEL) > (HIGH)? (HIGH - LOW) : (LEVEL - LOW)))
uint32_t Min3(uint32_t x, uint32_t y, uint32_t z)
{
	uint32_t Val;
	if(x > y)
	{
		Val = y;
	}
	else
	{
		Val = x;
	}
	if(Val > z)
	{
		Val = z;
	}
	return Val;
}

//IOID：Source/sink通路统一编号
void ClkAdjust(CLK_ADJUST_ADAPTER * AdjAdapter, uint8_t IOId)
{
	uint32_t AverageLevel = AdjAdapter->TotalNum / AdjAdapter->Count;

	if(IOId == AdaptHwCt.AdjustIO)
	{
		AdjAdapter->LastLevel = AverageLevel;
		AverageLevel = LEVELDELTA(100 * AverageLevel, (AdjAdapter->ValidDepth * AdjAdapter->LowLevelCent), (AdjAdapter->ValidDepth * AdjAdapter->HighLevelCent));//care for Delta * 100
		int16_t	AdjustVal =  ADJUST_DIV_MAX -  (AverageLevel * 2 * ADJUST_DIV_MAX) / (AdjAdapter->ValidDepth * (AdjAdapter->HighLevelCent - AdjAdapter->LowLevelCent));
		AdjustVal = (AdjAdapter->AdjustVal + AdjustVal) / 2; //抑制波动
		if(AdjAdapter->AdjustVal != AdjustVal)
		{
			AdjAdapter->AdjustVal = AdjustVal;
			if(AdjustVal >= 0)
			{
				Clock_AudioPllClockAdjust(PLL_CLK_1, 0, AdjustVal);
				Clock_AudioPllClockAdjust(PLL_CLK_2, 0, AdjustVal);
				Clock_AudioPllClockAdjust(PLL_CLK_3, 0, AdjustVal);
				Clock_AudioPllClockAdjust(PLL_CLK_4, 0, AdjustVal);
			}
			else
			{
				Clock_AudioPllClockAdjust(PLL_CLK_1, 1, -AdjustVal);
				Clock_AudioPllClockAdjust(PLL_CLK_2, 1, -AdjustVal);
				Clock_AudioPllClockAdjust(PLL_CLK_3, 1, -AdjustVal);
				Clock_AudioPllClockAdjust(PLL_CLK_4, 1, -AdjustVal);
			}
		}
	}
	else
	{
		AdjAdapter->RiseTimes = 0;
		AdjAdapter->AdjustVal = 0;
	}

	AdjAdapter->TotalNum = 0;
	AdjAdapter->Count = 0;
	AdjAdapter->AdjTickVal = AdaptHwCt.AdjTimer.TickValCache;
}

bool AudioCoreSourceInit(AudioCoreIO * AudioIO, uint8_t Index)
{
	uint32_t	UsedSize;// byte
	uint16_t     SampleByte;
	void		* AdaptBuf;
	AudioCoreSource * Source = &AudioCore.AudioSource[Index];
#ifdef AUDIO_CORE_DEBUG
	if(AudioCoreSourceIsInit(Index)
			|| AudioIO == NULL
			|| AudioIO->Channels > 2
			|| AudioIO->Adapt >= AUDIO_ADAPT_MAX
			|| AudioIO->Net >= MaxNet
			|| AudioIO->PcmWidth ==0
			|| AudioIO->DataIOFunc == NULL
			|| AudioIO->LenGetFunc == NULL)
	{
		DBG("Soure init error!\n");
		return FALSE;
	}
#endif
	Source->Channels = AudioIO->Channels;
	Source->Adapt = AudioIO->Adapt;
	Source->Net = AudioIO->Net;
	Source->Sync = AudioIO->Sync;
	Source->DataGetFunc = (AudioCoreDataGetFunc)(AudioIO->DataIOFunc);
	Source->DataLenFunc = (AudioCoreSpaceLenFunc)AudioIO->LenGetFunc;
	Source->LenFuncDepth = AudioIO->Depth;
	Source->PcmWidth = AudioIO->PcmWidth;
	Source->FlowCh = Source->Channels;
	Source->FlowWidth = AudioCore.NetCt[Source->Net].PcmWidth;//先用net,后从Flow获取
	Source->PcmBufFlag = 0;
	SampleByte = PCM_BYTE(Source->PcmWidth) * Source->Channels;
#ifdef CFG_FUNC_ROBOEFFECT_EN
	#ifndef ROBOEFFECT_DOUBLE_ENGINE_EN
		#ifndef	EFFECT_BYPASSMODE
		Source->PcmInBuf = AudioEffect_Parambin_GetSourceBuffer_Info(&EffectParambin, Index, &Source->FlowWidth, &Source->FlowCh);
		#endif
	#else
		if((Source->PcmInBuf = AudioEffect_Parambin_GetSourceBuffer_Info(&EffectParambin, Index, &Source->FlowWidth, &Source->FlowCh)) == NULL)
		{
			Source->PcmInBuf = AudioEffect_Parambin_GetSourceBuffer_Info(&EffectParambin2, Index, &Source->FlowWidth, &Source->FlowCh);
		}
	#endif
#endif
	if(Source->PcmInBuf == NULL)
	{
		Source->PcmInBuf = (PCM_DATA_TYPE *)osPortMalloc(SOURCEFRAME(Index) * sizeof(PCM_DATA_TYPE) * 2);
		if(Source->PcmInBuf == NULL)
		{
			DBG("PcmInBuf NULL error!\n");
			return FALSE;
		}
		Source->PcmBufFlag = 1;
	}
	switch(AudioIO->Adapt)
	{
		case	STD:
			Source->AdaptBuf = NULL;
			Source->AdjAdapter = NULL;
			Source->SrcAdapter = NULL;
			break;

		case   SRC_ONLY:
		{
			SRC_ADAPTER * SrcAdapter;
			UsedSize = SRC_FIFO_SIZE(SOURCEFRAME(Index)) * SampleByte;
			AdaptBuf = osPortMalloc(UsedSize);
			if(AdaptBuf == NULL
#ifdef AUDIO_CORE_DEBUG
					|| AudioIO->SampleRate == 0
#endif
					)
			{
				DBG("Src set error!\n");
				if(Source->PcmBufFlag)
					osPortFree(Source->PcmInBuf);
				return FALSE;
			}
			memset(AdaptBuf, 0, UsedSize);

			SrcAdapter = (SRC_ADAPTER *)osPortMalloc(sizeof(SRC_ADAPTER));
			if(SrcAdapter == NULL)
			{
				osPortFree(AdaptBuf);
				if(Source->PcmBufFlag)
					osPortFree(Source->PcmInBuf);
				return FALSE;
			}
			memset(SrcAdapter, 0, sizeof(SRC_ADAPTER));
			SrcAdapter->SrcQuality = AudioIO->SrcQuality;
			SrcAdapter->SampleRate = AudioIO->SampleRate;
			MCUCircular_Config(&SrcAdapter->SrcBufHandler, AdaptBuf, SRC_FIFO_SIZE(SOURCEFRAME(Index)) * SampleByte);
			Source->AdaptBuf = AdaptBuf;
			Source->SrcAdapter = SrcAdapter;
			Source->AdjAdapter = NULL;
			break;
		}

		case SRA_ONLY:
		{
			SRA_ADAPTER *AdjAdapter;
			UsedSize = SRA_FIFO_SIZE(SOURCEFRAME(Index)) * SampleByte;
			AdaptBuf = osPortMalloc(UsedSize);
			if(AdaptBuf == NULL
#ifdef AUDIO_CORE_DEBUG
					|| AudioIO->HighLevelCent == 0
					|| AudioIO->Depth == 0
#endif
				)
			{
				DBG("Sra set error!\n");
				if(Source->PcmBufFlag)
					osPortFree(Source->PcmInBuf);
				return FALSE;
			}
			memset(AdaptBuf, 0, UsedSize);

			AdjAdapter = (SRA_ADAPTER *)osPortMalloc(sizeof(SRA_ADAPTER));
			if(AdjAdapter == NULL)
			{
				osPortFree(AdaptBuf);
				if(Source->PcmBufFlag)
					osPortFree(Source->PcmInBuf);
				return FALSE;
			}
			memset(AdjAdapter, 0, sizeof(SRA_ADAPTER));
			MCUCircular_Config(&AdjAdapter->SraBufHandler, AdaptBuf, SRA_FIFO_SIZE(SOURCEFRAME(Index)) * SampleByte);
			AdjAdapter->HighLevelCent = AudioIO->HighLevelCent;
			AdjAdapter->LowLevelCent = AudioIO->LowLevelCent;
			Source->AdaptBuf = AdaptBuf;
			Source->AdjAdapter = (void *)AdjAdapter;
			Source->SrcAdapter = NULL;
			break;
		}

		case CLK_ADJUST_ONLY:
		{
			CLK_ADJUST_ADAPTER *AdjAdapter;

			if(Source->PcmInBuf  == NULL
#ifdef AUDIO_CORE_DEBUG
					|| AudioIO->HighLevelCent == 0
					|| AudioIO->Depth == 0
#endif
									)
			{
				DBG("Clkadj set error!\n");
				if(Source->PcmBufFlag)
					osPortFree(Source->PcmInBuf);
				return FALSE;
			}

			AdjAdapter = (CLK_ADJUST_ADAPTER *)osPortMalloc(sizeof(CLK_ADJUST_ADAPTER));
			if(AdjAdapter == NULL)
			{
				if(Source->PcmBufFlag)
					osPortFree(Source->PcmInBuf);
				return FALSE;
			}
			memset(AdjAdapter, 0, sizeof(CLK_ADJUST_ADAPTER));

			AdjAdapter->HighLevelCent = AudioIO->HighLevelCent;
			AdjAdapter->LowLevelCent = AudioIO->LowLevelCent;
			Source->AdaptBuf = NULL;
			Source->AdjAdapter = (void *)AdjAdapter;
			Source->SrcAdapter = NULL;
			AdaptHwCt.AdjustIO = SOURCE_GET_IO(Index);
			break;
		}

		case SRC_SRA:
		{
			SRA_ADAPTER *AdjAdapter;
			SRC_ADAPTER *SrcAdapter;
			UsedSize = SRA_FIFO_SIZE(SOURCEFRAME(Index)) * SampleByte + SRC_FIFO_SIZE(SOURCEFRAME(Index)) * SampleByte;
			AdaptBuf = osPortMalloc(UsedSize);
			if(AdaptBuf == NULL
#ifdef AUDIO_CORE_DEBUG
					|| AudioIO->SampleRate == 0
					|| AudioIO->HighLevelCent == 0
					|| AudioIO->Depth == 0
#endif
							)
			{
				DBG("Srcsra set error!\n");
				if(Source->PcmBufFlag)
					osPortFree(Source->PcmInBuf);
				return FALSE;
			}
			memset(AdaptBuf, 0, UsedSize);

			Source->AdjAdapter = osPortMalloc(sizeof(SRA_ADAPTER) + sizeof(SRC_ADAPTER));
			if(Source->AdjAdapter == NULL)
			{
				osPortFree(AdaptBuf);
				if(Source->PcmBufFlag)
					osPortFree(Source->PcmInBuf);
				return FALSE;
			}
			memset(Source->AdjAdapter, 0, sizeof(SRA_ADAPTER) + sizeof(SRC_ADAPTER));
			AdjAdapter = (SRA_ADAPTER *)Source->AdjAdapter;
			SrcAdapter = (SRC_ADAPTER *)(Source->AdjAdapter + sizeof(SRA_ADAPTER));
			SrcAdapter->SrcQuality = AudioIO->SrcQuality;
			SrcAdapter->SampleRate = AudioIO->SampleRate;
			MCUCircular_Config(&SrcAdapter->SrcBufHandler,
					AdaptBuf + SRA_FIFO_SIZE(SOURCEFRAME(Index)) * SampleByte,
					SRC_FIFO_SIZE(SOURCEFRAME(Index)) * SampleByte);
			MCUCircular_Config(&AdjAdapter->SraBufHandler, AdaptBuf, SRA_FIFO_SIZE(SOURCEFRAME(Index)) * SampleByte);
			AdjAdapter->HighLevelCent = AudioIO->HighLevelCent;
			AdjAdapter->LowLevelCent = AudioIO->LowLevelCent;
			Source->AdaptBuf = AdaptBuf;
			Source->SrcAdapter = SrcAdapter;
			Source->AdjAdapter = (void *)AdjAdapter;

			break;
		}

		case SRC_ADJUST:
		{
			SRC_ADAPTER *SrcAdapter;
			CLK_ADJUST_ADAPTER *AdjAdapter;
			UsedSize = SRC_FIFO_SIZE(SOURCEFRAME(Index)) * SampleByte;
			AdaptBuf = osPortMalloc(UsedSize);
			if(AdaptBuf == NULL
#ifdef AUDIO_CORE_DEBUG
					|| AudioIO->SampleRate == 0
					|| AudioIO->HighLevelCent == 0
					|| AudioIO->Depth == 0
#endif
							)
			{
				DBG("Srcadj set error!\n");
				if(Source->PcmBufFlag)
					osPortFree(Source->PcmInBuf);
				return FALSE;
			}
			memset(AdaptBuf, 0, UsedSize);

			Source->AdjAdapter = osPortMalloc(sizeof(CLK_ADJUST_ADAPTER) + sizeof(SRC_ADAPTER));
			if(Source->AdjAdapter == NULL)
			{
				osPortFree(AdaptBuf);
				if(Source->PcmBufFlag)
					osPortFree(Source->PcmInBuf);
				return FALSE;
			}
			memset(Source->AdjAdapter, 0, sizeof(CLK_ADJUST_ADAPTER) + sizeof(SRC_ADAPTER));

			SrcAdapter = (SRC_ADAPTER *)(Source->AdjAdapter + sizeof(CLK_ADJUST_ADAPTER));
			SrcAdapter->SrcQuality = AudioIO->SrcQuality;
			SrcAdapter->SampleRate = AudioIO->SampleRate;
			MCUCircular_Config(&SrcAdapter->SrcBufHandler, AdaptBuf, SRC_FIFO_SIZE(SOURCEFRAME(Index)) * SampleByte);

			AdjAdapter = (CLK_ADJUST_ADAPTER *)Source->AdjAdapter;
			AdjAdapter->HighLevelCent = AudioIO->HighLevelCent;
			AdjAdapter->LowLevelCent = AudioIO->LowLevelCent;
			Source->AdaptBuf = AdaptBuf;
			Source->SrcAdapter = SrcAdapter;
			Source->AdjAdapter = (void *)AdjAdapter;
			AdaptHwCt.AdjustIO = SOURCE_GET_IO(Index);
			break;
		}
#if defined(AUDIOCORE_HWSRC_EN)
		case   HWSRC_ONLY:
		{
			SRC_ADAPTER * SrcAdapter;
			UsedSize = SRC_FIFO_SIZE(SOURCEFRAME(Index)) * SampleByte;
			AdaptBuf = osPortMalloc(UsedSize);
			if(AdaptBuf == NULL
#ifdef AUDIO_CORE_DEBUG
					|| AudioIO->SampleRate == 0
					|| AudioIO->HwSrcIndex >= RES_SRC_NUM
					|| AdaptHwCt.HwSrcCt[AudioIO->HwSrcIndex].SrcCase != SOURCE_GET_IO(Index)
#endif
					)
			{
				DBG("HwSrc set error!\n");
				if(Source->PcmBufFlag)
					osPortFree(Source->PcmInBuf);
				return FALSE;
			}
			memset(AdaptBuf, 0, UsedSize);

			SrcAdapter = (SRC_ADAPTER *)osPortMalloc(sizeof(SRC_ADAPTER));
			if(SrcAdapter == NULL)
			{
				osPortFree(AdaptBuf);
				if(Source->PcmBufFlag)
					osPortFree(Source->PcmInBuf);
				return FALSE;
			}
			memset(SrcAdapter, 0, sizeof(SRC_ADAPTER));
			SrcAdapter->SrcQuality = AudioIO->SrcQuality;
			SrcAdapter->SampleRate = AudioIO->SampleRate;
			MCUCircular_Config(&SrcAdapter->SrcBufHandler, AdaptBuf, SRC_FIFO_SIZE(SOURCEFRAME(Index)) * SampleByte);
			Source->AdaptBuf = AdaptBuf;
			Source->SrcAdapter = SrcAdapter;
			Source->AdjAdapter = NULL;
			SrcAdapter->HwSrcIndex = AudioIO->HwSrcIndex;
			AdaptHwCt.HwSrcCt[AudioIO->HwSrcIndex].SampleLenStartIn = 0;
			break;
		}

		case HWSRC_SRA:
		{
			SRA_ADAPTER *AdjAdapter;
			SRC_ADAPTER *SrcAdapter;
			UsedSize = SRA_FIFO_SIZE(SOURCEFRAME(Index)) * SampleByte + SRC_FIFO_SIZE(SOURCEFRAME(Index)) * SampleByte;
			AdaptBuf = osPortMalloc(UsedSize);
			if(AdaptBuf == NULL
#ifdef AUDIO_CORE_DEBUG
					|| AudioIO->SampleRate == 0
					|| AudioIO->HighLevelCent == 0
					|| AudioIO->Depth == 0
					|| AudioIO->HwSrcIndex >= RES_SRC_NUM
					|| AdaptHwCt.HwSrcCt[AudioIO->HwSrcIndex].SrcCase != SOURCE_GET_IO(Index)
#endif
							)
			{
				DBG("HwSrcsra set error!\n");
				if(Source->PcmBufFlag)
					osPortFree(Source->PcmInBuf);
				return FALSE;
			}
			memset(AdaptBuf, 0, UsedSize);

			Source->AdjAdapter = osPortMalloc(sizeof(SRA_ADAPTER) + sizeof(SRC_ADAPTER));
			if(Source->AdjAdapter == NULL)
			{
				osPortFree(AdaptBuf);
				if(Source->PcmBufFlag)
					osPortFree(Source->PcmInBuf);
				return FALSE;
			}
			memset(Source->AdjAdapter, 0, sizeof(SRA_ADAPTER) + sizeof(SRC_ADAPTER));
			AdjAdapter = (SRA_ADAPTER *)Source->AdjAdapter;
			SrcAdapter = (SRC_ADAPTER *)(Source->AdjAdapter + sizeof(SRA_ADAPTER));
			SrcAdapter->SrcQuality = AudioIO->SrcQuality;
			SrcAdapter->SampleRate = AudioIO->SampleRate;
			MCUCircular_Config(&SrcAdapter->SrcBufHandler,
					AdaptBuf + SRA_FIFO_SIZE(SOURCEFRAME(Index)) * SampleByte,
					SRC_FIFO_SIZE(SOURCEFRAME(Index)) * SampleByte);
			MCUCircular_Config(&AdjAdapter->SraBufHandler, AdaptBuf, SRA_FIFO_SIZE(SOURCEFRAME(Index)) * SampleByte);
			AdjAdapter->HighLevelCent = AudioIO->HighLevelCent;
			AdjAdapter->LowLevelCent = AudioIO->LowLevelCent;
			Source->AdaptBuf = AdaptBuf;
			Source->SrcAdapter = SrcAdapter;
			Source->AdjAdapter = (void *)AdjAdapter;
			SrcAdapter->HwSrcIndex = AudioIO->HwSrcIndex;
			AdaptHwCt.HwSrcCt[AudioIO->HwSrcIndex].SampleLenStartIn = 0;
			break;
		}

		case HWSRC_ADJUST:
		{
			SRC_ADAPTER *SrcAdapter;
			CLK_ADJUST_ADAPTER *AdjAdapter;
			UsedSize = SRC_FIFO_SIZE(SOURCEFRAME(Index)) * SampleByte;
			AdaptBuf = osPortMalloc(UsedSize);
			if(AdaptBuf == NULL
#ifdef AUDIO_CORE_DEBUG
					|| AudioIO->SampleRate == 0
					|| AudioIO->HighLevelCent == 0
					|| AudioIO->Depth == 0
					|| AudioIO->HwSrcIndex >= RES_SRC_NUM
					|| AdaptHwCt.HwSrcCt[AudioIO->HwSrcIndex].SrcCase != SOURCE_GET_IO(Index)
#endif
							)
			{
				DBG("Srcadj set error!\n");
				if(Source->PcmBufFlag)
					osPortFree(Source->PcmInBuf);
				return FALSE;
			}
			memset(AdaptBuf, 0, UsedSize);

			Source->AdjAdapter = osPortMalloc(sizeof(CLK_ADJUST_ADAPTER) + sizeof(SRC_ADAPTER));
			if(Source->AdjAdapter == NULL)
			{
				osPortFree(AdaptBuf);
				if(Source->PcmBufFlag)
					osPortFree(Source->PcmInBuf);
				return FALSE;
			}
			memset(Source->AdjAdapter, 0, sizeof(CLK_ADJUST_ADAPTER) + sizeof(SRC_ADAPTER));

			SrcAdapter = (SRC_ADAPTER *)(Source->AdjAdapter + sizeof(CLK_ADJUST_ADAPTER));
			SrcAdapter->SrcQuality = AudioIO->SrcQuality;
			SrcAdapter->SampleRate = AudioIO->SampleRate;
			MCUCircular_Config(&SrcAdapter->SrcBufHandler, AdaptBuf, SRC_FIFO_SIZE(SOURCEFRAME(Index)) * SampleByte);

			AdjAdapter = (CLK_ADJUST_ADAPTER *)Source->AdjAdapter;
			AdjAdapter->HighLevelCent = AudioIO->HighLevelCent;
			AdjAdapter->LowLevelCent = AudioIO->LowLevelCent;
			Source->AdaptBuf = AdaptBuf;
			Source->SrcAdapter = SrcAdapter;
			Source->AdjAdapter = (void *)AdjAdapter;
			AdaptHwCt.AdjustIO = SOURCE_GET_IO(Index);
			SrcAdapter->HwSrcIndex = AudioIO->HwSrcIndex;
			AdaptHwCt.HwSrcCt[AudioIO->HwSrcIndex].SampleLenStartIn = 0;
			break;
		}
#endif
	}

	Source->InitFlag = TRUE;
	AudioCoreSourceAdapterReset(Index);
	return TRUE;
}

void AudioCoreSourceDeinit(uint8_t Index)
{
	AudioCoreSource * Source = &AudioCore.AudioSource[Index];
#ifdef AUDIO_CORE_DEBUG
	if(!AudioCoreSourceIsInit(Index))
		return;
#endif
	Source->Enable = FALSE;
	Source->Sync = FALSE;
	Source->Active = FALSE;
//	SOURCE_BIT_DIS(AudioCore.FrameReady, Index);
	SOURCE_BIT_DIS(SOURCEREADY(Index), Index);
	Source->DataGetFunc = NULL;
	Source->DataLenFunc = NULL;
	if(Source->PcmBufFlag)
		osPortFree(Source->PcmInBuf);
	Source->PcmBufFlag = 0;
	Source->PcmInBuf = NULL;
	switch(Source->Adapt)
	{
		case   	SRC_ONLY:
#if defined(CFG_FUNC_SRC_SINC_EN)
			osPortFree(Source->SrcAdapter->SincCt);
#endif
			osPortFree(Source->AdaptBuf);
			Source->AdaptBuf = NULL;
			osPortFree(Source->SrcAdapter);
			Source->SrcAdapter = NULL;
			break;
#if defined(AUDIOCORE_HWSRC_EN)
		case   	HWSRC_ONLY:
			if(Source->SrcAdapter->HwSrcIndex  < RES_SRC_NUM \
					&& AdaptHwCt.HwSrcCt[Source->SrcAdapter->HwSrcIndex].SrcCase == SOURCE_GET_IO(Index))
			{
				Reset_Function2Reset((SRC0_FUNC_SEPA) << (Source->SrcAdapter->HwSrcIndex));
				Reset_Register2Reset(SRC0_REG_SEPA << (Source->SrcAdapter->HwSrcIndex));
				osPortFree(AdaptHwCt.HwSrcCt[Source->SrcAdapter->HwSrcIndex].SrcContext.InBuf);
				osPortFree(AdaptHwCt.HwSrcCt[Source->SrcAdapter->HwSrcIndex].SrcContext.OutBuf);
				AdaptHwCt.HwSrcCt[Source->SrcAdapter->HwSrcIndex].SrcContext.InBuf = NULL;
				AdaptHwCt.HwSrcCt[Source->SrcAdapter->HwSrcIndex].SrcContext.OutBuf = NULL;
//				AdaptHwCt.HwSrcCt[Source->SrcAdapter->HwSrcIndex].SrcCase = HWSRC_IDLE;
				AdaptHwCt.HwSrcCt[Source->SrcAdapter->HwSrcIndex].SampleLenStartIn = 0;
			}
			osPortFree(Source->AdaptBuf);
			Source->AdaptBuf = NULL;
			osPortFree(Source->SrcAdapter);
			Source->SrcAdapter = NULL;
			break;
		case 	HWSRC_SRA:
		case 	HWSRC_ADJUST:
			if(Source->SrcAdapter->HwSrcIndex  < RES_SRC_NUM \
					&& AdaptHwCt.HwSrcCt[Source->SrcAdapter->HwSrcIndex].SrcCase == SOURCE_GET_IO(Index))
			{
				Reset_Function2Reset((SRC0_FUNC_SEPA) << (Source->SrcAdapter->HwSrcIndex));
				Reset_Register2Reset(SRC0_REG_SEPA << (Source->SrcAdapter->HwSrcIndex));
				osPortFree(AdaptHwCt.HwSrcCt[Source->SrcAdapter->HwSrcIndex].SrcContext.InBuf);
				osPortFree(AdaptHwCt.HwSrcCt[Source->SrcAdapter->HwSrcIndex].SrcContext.OutBuf);
				AdaptHwCt.HwSrcCt[Source->SrcAdapter->HwSrcIndex].SrcContext.InBuf = NULL;
				AdaptHwCt.HwSrcCt[Source->SrcAdapter->HwSrcIndex].SrcContext.OutBuf = NULL;
//				AdaptHwCt.HwSrcCt[Source->SrcAdapter->HwSrcIndex].SrcCase = HWSRC_IDLE;
				AdaptHwCt.HwSrcCt[Source->SrcAdapter->HwSrcIndex].SampleLenStartIn = 0;
			}
			osPortFree(Source->AdaptBuf);
			Source->AdaptBuf = NULL;
			osPortFree(Source->AdjAdapter);
			Source->SrcAdapter = NULL;
			Source->AdjAdapter = NULL;
			break;
#endif
		case	SRA_ONLY:
			osPortFree(Source->AdaptBuf);
			Source->AdaptBuf = NULL;
			osPortFree(Source->AdjAdapter);
			Source->AdjAdapter = NULL;
			break;
		case 	CLK_ADJUST_ONLY:
			osPortFree(Source->AdjAdapter);
			Source->AdjAdapter = NULL;
			break;
		case 	SRC_SRA:
		case 	SRC_ADJUST:
#if defined(CFG_FUNC_SRC_SINC_EN)
			osPortFree(Source->SrcAdapter->SincCt);
#endif
			osPortFree(Source->AdaptBuf);
			Source->AdaptBuf = NULL;
			osPortFree(Source->AdjAdapter);
			Source->SrcAdapter = NULL;
			Source->AdjAdapter = NULL;
			break;
		default:
			break;
	}
	Source->InitFlag = FALSE;
}

bool AudioCoreSinkInit(AudioCoreIO * AudioIO, uint8_t Index)
{
	uint32_t	UsedSize;// byte
	uint16_t	SampleByte;
	void		* AdaptBuf;
	AudioCoreSink *Sink = &AudioCore.AudioSink[Index];
#ifdef AUDIO_CORE_DEBUG
	if(AudioCoreSinkIsInit(Index)
			|| AudioIO == NULL
			|| AudioIO->Channels > 2
			|| AudioIO->Net >= MaxNet
			|| AudioIO->Depth <= (uint64_t)(AudioIO->SampleRate) * AudioCore.NetCt[AudioIO->Net].FrameSize / AudioCore.NetCt[AudioIO->Net].SampleRate
			|| AudioIO->Adapt >= AUDIO_ADAPT_MAX
			|| AudioIO->PcmWidth == 0
			|| AudioIO->DataIOFunc == NULL
			|| AudioIO->LenGetFunc ==NULL)
	{
		DBG("Sink para error!\n");
		return FALSE;
	}
#endif
	Sink->Channels = AudioIO->Channels;
	Sink->Adapt = AudioIO->Adapt;
	Sink->Net = AudioIO->Net;
	Sink->Sync = AudioIO->Sync;
	Sink->LenFuncDepth = AudioIO->Depth;
	Sink->DataSetFunc = (AudioCoreDataSetFunc)AudioIO->DataIOFunc;
	Sink->SpaceLenFunc = (AudioCoreSpaceLenFunc)AudioIO->LenGetFunc;
	Sink->PcmWidth = AudioIO->PcmWidth;
	Sink->FlowCh = Sink->Channels;
	Sink->FlowWidth = AudioCore.NetCt[Sink->Net].PcmWidth;
	Sink->PcmBufFlag = 0;
	SampleByte = PCM_BYTE(Sink->PcmWidth) * Sink->Channels;
#if defined(CFG_FUNC_ROBOEFFECT_EN)
	#ifndef ROBOEFFECT_DOUBLE_ENGINE_EN
		#ifndef	EFFECT_BYPASSMODE
		Sink->PcmOutBuf = AudioEffect_Parambin_GetSinkBuffer_Info(&EffectParambin, Index, &Sink->FlowWidth, &Sink->FlowCh);
		#endif
	#else
		if((Sink->PcmOutBuf = AudioEffect_Parambin_GetSinkBuffer_Info(&EffectParambin, Index, &Sink->FlowWidth, &Sink->FlowCh)) == NULL)
		{
			Sink->PcmOutBuf = AudioEffect_Parambin_GetSinkBuffer_Info(&EffectParambin2, Index, &Sink->FlowWidth, &Sink->FlowCh);
		}
	#endif
#endif
	if(Sink->PcmOutBuf == NULL)
	{
		Sink->PcmOutBuf = (PCM_DATA_TYPE *)osPortMalloc(SINKFRAME(Index) * sizeof(PCM_DATA_TYPE) * 2);
		if(Sink->PcmOutBuf == NULL)
		{
			return FALSE;
		}
		Sink->PcmBufFlag = 1;
	}

	switch(AudioIO->Adapt)
	{
		case	STD:

			Sink->AdaptBuf = NULL;
			Sink->AdjAdapter = NULL;
			Sink->SrcAdapter = NULL;
			break;

		case   SRC_ONLY:
		{
			SRC_ADAPTER * SrcAdapter;
			UsedSize = SRC_FIFO_SIZE(SINKFRAME(Index)) * SampleByte;
			if(AudioIO->Resident)
			{
				AdaptBuf = osPortMalloc(UsedSize);
			}
			else
			{
				AdaptBuf = osPortMalloc(UsedSize);
			}
			if(AdaptBuf == NULL
#ifdef AUDIO_CORE_DEBUG
					|| AudioIO->SampleRate == 0
#endif
					)
			{
				DBG("SRC set error!\n");
				if(Sink->PcmBufFlag)
					osPortFree(Sink->PcmOutBuf);
				return FALSE;
			}
			memset(AdaptBuf, 0, UsedSize);

			SrcAdapter = (SRC_ADAPTER *)osPortMalloc(sizeof(SRC_ADAPTER));
			if(SrcAdapter == NULL)
			{
				osPortFree(AdaptBuf);
				if(Sink->PcmBufFlag)
					osPortFree(Sink->PcmOutBuf);
				return FALSE;
			}
			memset(SrcAdapter, 0, sizeof(SRC_ADAPTER));
			SrcAdapter->SrcQuality = AudioIO->SrcQuality;
			SrcAdapter->SampleRate = AudioIO->SampleRate;
			MCUCircular_Config(&SrcAdapter->SrcBufHandler, AdaptBuf, SRC_FIFO_SIZE(SINKFRAME(Index)) * SampleByte);
			Sink->AdaptBuf = AdaptBuf;
			Sink->SrcAdapter = SrcAdapter;
			Sink->AdjAdapter = NULL;
			break;
		}

		case SRA_ONLY:
		{
			SRA_ADAPTER *AdjAdapter;
			UsedSize = SRA_FIFO_SIZE(SINKFRAME(Index)) * SampleByte;
			if(AudioIO->Resident)
			{
				AdaptBuf = osPortMalloc(UsedSize);
			}
			else
			{
				AdaptBuf = osPortMalloc(UsedSize);
			}
			if(AdaptBuf == NULL
#ifdef AUDIO_CORE_DEBUG
					|| AudioIO->HighLevelCent == 0
					|| AudioIO->Depth == 0
#endif
							)
			{
				DBG("SRA set error!\n");
				if(Sink->PcmBufFlag)
					osPortFree(Sink->PcmOutBuf);
				return FALSE;
			}
			memset(AdaptBuf, 0, UsedSize);
			AdjAdapter = (SRA_ADAPTER *)osPortMalloc(sizeof(SRA_ADAPTER));
			if(AdjAdapter == NULL)
			{
				osPortFree(AdaptBuf);
				if(Sink->PcmBufFlag)
					osPortFree(Sink->PcmOutBuf);
				return FALSE;
			}
			memset(AdjAdapter, 0, sizeof(SRA_ADAPTER));

			MCUCircular_Config(&AdjAdapter->SraBufHandler, AdaptBuf, SRA_FIFO_SIZE(SINKFRAME(Index)) * SampleByte);
			AdjAdapter->HighLevelCent = AudioIO->HighLevelCent;
			AdjAdapter->LowLevelCent = AudioIO->LowLevelCent;
			Sink->AdaptBuf = AdaptBuf;
			Sink->AdjAdapter = (void *)AdjAdapter;
			Sink->SrcAdapter = NULL;
			break;
		}

		case CLK_ADJUST_ONLY:
		{
			CLK_ADJUST_ADAPTER *AdjAdapter;

			if(Sink->PcmOutBuf == NULL
#ifdef AUDIO_CORE_DEBUG
					|| AudioIO->HighLevelCent == 0
					|| AudioIO->Depth == 0
#endif
									)
			{
				DBG("Clkadj set error!\n");
				if(Sink->PcmBufFlag)
					osPortFree(Sink->PcmOutBuf);
				return FALSE;
			}

			AdjAdapter = (CLK_ADJUST_ADAPTER *)osPortMalloc(sizeof(CLK_ADJUST_ADAPTER));
			if(AdjAdapter == NULL)
			{
				if(Sink->PcmBufFlag)
					osPortFree(Sink->PcmOutBuf);
				return FALSE;
			}
			memset(AdjAdapter, 0, sizeof(CLK_ADJUST_ADAPTER));

			AdjAdapter->HighLevelCent = AudioIO->HighLevelCent;
			AdjAdapter->LowLevelCent = AudioIO->LowLevelCent;
			Sink->AdaptBuf = NULL;
			Sink->AdjAdapter = (void *)AdjAdapter;
			Sink->SrcAdapter = NULL;
			AdaptHwCt.AdjustIO = SINK_GET_IO(Index);
			break;
		}

		case SRC_SRA:
		{
			SRC_ADAPTER *SrcAdapter;
			SRA_ADAPTER *AdjAdapter;
			UsedSize = SRA_FIFO_SIZE(SINKFRAME(Index)) * SampleByte
					 + SRC_FIFO_SIZE(SINKFRAME(Index)) * SampleByte;
			if(AudioIO->Resident)
			{
				AdaptBuf = osPortMalloc(UsedSize);
			}
			else
			{
				AdaptBuf = osPortMalloc(UsedSize);
			}
			if(AdaptBuf == NULL
#ifdef AUDIO_CORE_DEBUG
					|| AudioIO->SampleRate == 0
					|| AudioIO->HighLevelCent == 0
					|| AudioIO->Depth == 0
#endif
							)
			{
				DBG("Srcsra set error!\n");
				if(Sink->PcmBufFlag)
					osPortFree(Sink->PcmOutBuf);
				return FALSE;
			}
			memset(AdaptBuf, 0, UsedSize);

			Sink->AdjAdapter = osPortMalloc(sizeof(SRA_ADAPTER) + sizeof(SRC_ADAPTER));
			if(Sink->AdjAdapter == NULL)
			{
				osPortFree(AdaptBuf);
				if(Sink->PcmBufFlag)
					osPortFree(Sink->PcmOutBuf);
				return FALSE;
			}
			memset(Sink->AdjAdapter, 0, sizeof(SRA_ADAPTER) + sizeof(SRC_ADAPTER));
			AdjAdapter = (SRA_ADAPTER *)Sink->AdjAdapter;
			SrcAdapter = (SRC_ADAPTER *)(Sink->AdjAdapter + sizeof(SRA_ADAPTER));

			MCUCircular_Config(&AdjAdapter->SraBufHandler, AdaptBuf, SRA_FIFO_SIZE(SINKFRAME(Index)) * SampleByte);
			AdjAdapter->HighLevelCent = AudioIO->HighLevelCent;
			AdjAdapter->LowLevelCent = AudioIO->LowLevelCent;
			Sink->AdjAdapter = (void *)AdjAdapter;
			SrcAdapter->SrcQuality = AudioIO->SrcQuality;
			SrcAdapter->SampleRate = AudioIO->SampleRate;
			MCUCircular_Config(&SrcAdapter->SrcBufHandler,
					AdaptBuf + SRA_FIFO_SIZE(SINKFRAME(Index)) * SampleByte,
								SRC_FIFO_SIZE(SINKFRAME(Index)) * SampleByte);
			Sink->AdaptBuf = AdaptBuf;
			Sink->SrcAdapter = SrcAdapter;
			break;
		}

		case SRC_ADJUST:
		{
			CLK_ADJUST_ADAPTER *AdjAdapter;
			SRC_ADAPTER *SrcAdapter;
			UsedSize = SRC_FIFO_SIZE(SINKFRAME(Index)) * SampleByte;
			if(AudioIO->Resident)
			{
				AdaptBuf = osPortMalloc(UsedSize);
			}
			else
			{
				AdaptBuf = osPortMalloc(UsedSize);
			}
			if(AdaptBuf == NULL
#ifdef AUDIO_CORE_DEBUG
					|| AudioIO->SampleRate == 0
					|| AudioIO->HighLevelCent == 0
					|| AudioIO->Depth == 0
#endif
								)
			{
				DBG("Srcadj set error!\n");
				if(Sink->PcmBufFlag)
					osPortFree(Sink->PcmOutBuf);
				return FALSE;
			}
			memset(AdaptBuf, 0, UsedSize);

			Sink->AdjAdapter = osPortMalloc(sizeof(CLK_ADJUST_ADAPTER) + sizeof(SRC_ADAPTER));
			if(Sink->AdjAdapter == NULL)
			{
				osPortFree(AdaptBuf);
				if(Sink->PcmBufFlag)
					osPortFree(Sink->PcmOutBuf);
				return FALSE;
			}
			memset(Sink->AdjAdapter, 0, sizeof(CLK_ADJUST_ADAPTER) + sizeof(SRC_ADAPTER));
			AdjAdapter = (CLK_ADJUST_ADAPTER *)Sink->AdjAdapter;
			SrcAdapter = (SRC_ADAPTER *)((uint8_t*)Sink->AdjAdapter + sizeof(CLK_ADJUST_ADAPTER));


			AdjAdapter->HighLevelCent = AudioIO->HighLevelCent;
			AdjAdapter->LowLevelCent = AudioIO->LowLevelCent;
			Sink->AdjAdapter = (void *)AdjAdapter;
			SrcAdapter->SrcQuality = AudioIO->SrcQuality;
			SrcAdapter->SampleRate = AudioIO->SampleRate;
			MCUCircular_Config(&SrcAdapter->SrcBufHandler, AdaptBuf, SRC_FIFO_SIZE(SINKFRAME(Index)) * SampleByte);
			Sink->AdaptBuf = AdaptBuf;
			Sink->SrcAdapter = SrcAdapter;
			AdaptHwCt.AdjustIO = SINK_GET_IO(Index);
			break;
		}
		default:
			break;
	}
	Sink->InitFlag = TRUE;
	AudioCoreSinkAdapterReset(Index);
	return TRUE;
}

void AudioCoreSinkDeinit(uint8_t Index)
{

	AudioCoreSink * Sink = &AudioCore.AudioSink[Index];
#ifdef AUDIO_CORE_DEBUG
	if(!AudioCoreSinkIsInit(Index))
		return;
#endif
	Sink->Enable = FALSE;
	Sink->Sync = FALSE;
	Sink->Active = FALSE;
//	SINK_BIT_DIS(AudioCore.FrameReady, Index);
	SINK_BIT_DIS(SINKREADY(Index), Index);
	Sink->DataSetFunc = NULL;
	Sink->SpaceLenFunc = NULL;
	if(Sink->PcmBufFlag)
		osPortFree(Sink->PcmOutBuf);
	Sink->PcmBufFlag = 0;
	Sink->PcmOutBuf = NULL;
	switch(Sink->Adapt)
	{
		case   	SRC_ONLY:
#if defined(CFG_FUNC_SRC_SINC_EN)
			osPortFree(Sink->SrcAdapter->SincCt);
#endif
			osPortFree(Sink->AdaptBuf);
			Sink->AdaptBuf = NULL;
			osPortFree(Sink->SrcAdapter);
			Sink->SrcAdapter = NULL;
			break;
		case	SRA_ONLY:
			osPortFree(Sink->AdaptBuf);
			Sink->AdaptBuf = NULL;
			osPortFree(Sink->AdjAdapter);
			Sink->AdjAdapter = NULL;
			break;
		case 	CLK_ADJUST_ONLY:
			osPortFree(Sink->AdjAdapter);
			Sink->AdjAdapter = NULL;
			break;
		case 	SRC_SRA:
		case 	SRC_ADJUST:
#if defined(CFG_FUNC_SRC_SINC_EN)
			osPortFree(Sink->SrcAdapter->SincCt);
#endif
			osPortFree(Sink->AdaptBuf);
			Sink->AdaptBuf = NULL;
			osPortFree(Sink->AdjAdapter);
			Sink->SrcAdapter = NULL;
			Sink->AdjAdapter = NULL;
			break;
		default:
			break;
	}

	Sink->InitFlag = FALSE;
}

bool AudioCoreDataSpaceCheck(void)
{
	uint16_t Index;
	bool	 flag;
	AudioCoreSource *Source;
	AudioCoreSink *Sink;

	flag = FALSE;
	for(Index = 0; Index < AUDIO_CORE_SOURCE_MAX_NUM; Index++)
	{
		Source = &AudioCore.AudioSource[Index];
		if(Source->Enable
#ifdef AUDIO_CORE_DEBUG
			&& Source->DataGetFunc
			&& Source->DataLenFunc
#endif
			)
		{
#ifdef CFG_FUNC_REMIND_SOUND_EN
			if(Index == REMIND_SOURCE_NUM) 	//提示音解码在DataLenFunc里进行，屏蔽这个通道防止解码器重入
				continue;
#endif
			if(Source->DataLenFunc() < SOURCEFRAME(Index))
			{
				return FALSE;
			}
			flag = TRUE; //Source有数据
		}
	}

	if(flag)	//所有的Source都有数据
	{
		flag = FALSE;
		for(Index = 0; Index < AUDIO_CORE_SINK_MAX_NUM; Index++)
		{
			Sink = &AudioCore.AudioSink[Index];
			if(Sink->Enable
	#ifdef AUDIO_CORE_DEBUG
				&& Sink->DataSetFunc
				&& Sink->SpaceLenFunc
	#endif
				)
			{
				if(Sink->SpaceLenFunc() < SINKFRAME(Index))
				{
					return FALSE;
				}
				flag = TRUE; //sink有剩余空间
			}
		}
	}

	return flag;
}

__attribute__((section(".ilm_section")))
void AudioCoreIOLenProcess(uint8_t FlowNet)
{
	NetContext *pNetCt = &AudioCore.NetCt[FlowNet];
	uint16_t Index;
	int32_t SrcAudioLen;
	uint32_t SraAudioLen;//Sample
	uint16_t SampleByte;
	AudioCoreSource *Source;
	AudioCoreSink *Sink;
	pNetCt->AdaptProcessed = FALSE;
	for(Index = 0; Index < AUDIO_CORE_SOURCE_MAX_NUM; Index++)
	{
		Source = &AudioCore.AudioSource[Index];
		SampleByte = PCM_BYTE(Source->PcmWidth) * Source->Channels;
		if(Source->Enable
#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
			&& Source->Net == FlowNet
#endif
#ifdef AUDIO_CORE_DEBUG
			&& Source->DataGetFunc
			&& Source->DataLenFunc
#endif
			)
		{

			switch(Source->Adapt)
			{
				case STD:
				case CLK_ADJUST_ONLY:
					if(Source->DataLenFunc() >= SOURCEFRAME(Index))
					{
						SOURCE_BIT_EN(pNetCt->FrameReady, Index);
					}
					else
					{
						SOURCE_BIT_DIS(pNetCt->FrameReady, Index);
					}
					break;

				case SRC_ONLY:
				case SRC_ADJUST:
				{
					SRC_ADAPTER * SrcAdapter = Source->SrcAdapter;
					do{
#ifdef CFG_FUNC_MIXER_SRC_EN
						if(SrcAdapter->SampleRate != AudioCore.NetCt[AudioCore.AudioSource[Index].Net].SampleRate)
						{
							SrcAudioLen = SRCValidLenGet(SrcAdapter->SampleRate,
												Source->DataLenFunc(),
												AudioCore.NetCt[AudioCore.AudioSource[Index].Net].SampleRate,
												MCUCircular_GetSpaceLen(&SrcAdapter->SrcBufHandler) / SampleByte - 1);
							if(SrcAudioLen)
							{
								SrcAudioLen = Source->DataGetFunc(AudioCore.AdaptIn, SrcAudioLen);
	#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
								if(Source->PcmWidth == WIDTH_L24BIT)
								{
									SrcAudioLen = SRC_APPLY_24(SrcAudioLen);
									if(SrcAudioLen < 0) SrcAudioLen = 0;
								}
								else
	#endif
								{
									SrcAudioLen = SRC_APPLY_16(SrcAudioLen);
									if(SrcAudioLen < 0) SrcAudioLen = 0;
								}
								if(SrcAudioLen > 0)
								{
									MCUCircular_PutData(&SrcAdapter->SrcBufHandler, AudioCore.AdaptOut, SrcAudioLen * SampleByte);
									pNetCt->AdaptProcessed = TRUE;
								}
							}
						}
						else
#endif
						{
							SrcAudioLen = Min3(Source->DataLenFunc(),
												MCUCircular_GetSpaceLen(&SrcAdapter->SrcBufHandler) / SampleByte - 1,
												sizeof(AudioCore.AdaptOut)/SampleByte);
							if(SrcAudioLen)
							{
								SrcAudioLen = Source->DataGetFunc(AudioCore.AdaptOut, SrcAudioLen);
								if(SrcAudioLen > 0)
								{
									MCUCircular_PutData(&SrcAdapter->SrcBufHandler, AudioCore.AdaptOut, SrcAudioLen * SampleByte);
									pNetCt->AdaptProcessed = TRUE;
								}
							}
						}
						if(MCUCircular_GetDataLen(&SrcAdapter->SrcBufHandler) / (SampleByte) >= SOURCEFRAME(Index))
						{
							SOURCE_BIT_EN(pNetCt->FrameReady, Index);
						}
						else
						{
							SOURCE_BIT_DIS(pNetCt->FrameReady, Index);
						}
					}while(!SOURCE_BIT_GET(pNetCt->FrameReady, Index) && SrcAudioLen);
					break;
				}
				case SRA_ONLY:
				{
					SRA_ADAPTER * AdjAdapter = (SRA_ADAPTER *)Source->AdjAdapter;
//					uint32_t     PcmDataLen = 	PCM_BYTE(Source->PcmWidth) * Source->Channels;

					if(AdjAdapter->Enable)
					{
						do{
							SraAudioLen = Source->DataLenFunc();
							SraAudioLen = SRAValidLenGet(SraAudioLen, MCUCircular_GetSpaceLen(&AdjAdapter->SraBufHandler) / SampleByte);
							if(SraAudioLen)
							{
								SraAudioLen = Source->DataGetFunc(AudioCore.AdaptIn, SraAudioLen);
								if(AdjAdapter->SraResFarCt.step == 0)
								{
									AdjAdapter->GroupAdj = AdjAdapter->AdjustVal;
								}
#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
								if(Source->PcmWidth == WIDTH_L24BIT)
								{
									resampler_farrow_over_frames_apply24(&AdjAdapter->SraResFarCt, (int32_t *)AudioCore.AdaptIn, (int32_t *)AudioCore.AdaptOut, ADJUST_GROUP_FRAME(AdjAdapter->GroupAdj), AdjAdapter->GroupAdj, ADAPTER_INPUT_BLOCK, (int32_t*)&SraAudioLen);
								}
								else
#endif
								{
									resampler_farrow_over_frames_apply(&AdjAdapter->SraResFarCt, (int16_t *)AudioCore.AdaptIn, (int16_t *)AudioCore.AdaptOut, ADJUST_GROUP_FRAME(AdjAdapter->GroupAdj), AdjAdapter->GroupAdj, ADAPTER_INPUT_BLOCK, (int32_t*)&SraAudioLen);
								}
								MCUCircular_PutData(&AdjAdapter->SraBufHandler, AudioCore.AdaptOut, SraAudioLen * SampleByte);
								pNetCt->AdaptProcessed = TRUE;
							}

						}while(!SOURCE_BIT_GET(pNetCt->FrameReady, Index) && SraAudioLen);
					}
					else
					{
						SraAudioLen = Min3(Source->DataLenFunc(),
											MCUCircular_GetSpaceLen(&AdjAdapter->SraBufHandler) / SampleByte - 1,
											sizeof(AudioCore.AdaptOut)/SampleByte);
						if(SraAudioLen)
						{
							SraAudioLen = Source->DataGetFunc(AudioCore.AdaptOut, SraAudioLen);
							MCUCircular_PutData(&AdjAdapter->SraBufHandler, AudioCore.AdaptOut, SraAudioLen * SampleByte);
							pNetCt->AdaptProcessed = TRUE;
						}
					}
					if(MCUCircular_GetDataLen(&AdjAdapter->SraBufHandler) / SampleByte >= SOURCEFRAME(Index))
					{
						SOURCE_BIT_EN(pNetCt->FrameReady, Index);
					}
					else
					{
						SOURCE_BIT_DIS(pNetCt->FrameReady, Index);
					}
					break;
				}
				case SRC_SRA:
				{
					SRC_ADAPTER * SrcAdapter = Source->SrcAdapter;
					SRA_ADAPTER * AdjAdapter = (SRA_ADAPTER *)Source->AdjAdapter;

					do{
#ifdef CFG_FUNC_MIXER_SRC_EN
						if(SrcAdapter->SampleRate != AudioCore.NetCt[AudioCore.AudioSource[Index].Net].SampleRate)
						{
							SrcAudioLen = SRCValidLenGet(SrcAdapter->SampleRate,
												Source->DataLenFunc(),
												AudioCore.NetCt[AudioCore.AudioSource[Index].Net].SampleRate,
												MCUCircular_GetSpaceLen(&SrcAdapter->SrcBufHandler) / SampleByte);
							if(SrcAudioLen)
							{
								SrcAudioLen = Source->DataGetFunc(AudioCore.AdaptIn, SrcAudioLen);
	#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
								if(Source->PcmWidth == WIDTH_L24BIT)
								{
									SrcAudioLen = SRC_APPLY_24(SrcAudioLen);
									if(SrcAudioLen < 0) SrcAudioLen = 0;
								}
								else
	#endif
								{
									SrcAudioLen = SRC_APPLY_16(SrcAudioLen);
									if(SrcAudioLen < 0) SrcAudioLen = 0;
								}
								if(SrcAudioLen > 0)
								{
									MCUCircular_PutData(&SrcAdapter->SrcBufHandler, AudioCore.AdaptOut, SrcAudioLen * SampleByte);
									pNetCt->AdaptProcessed = TRUE;
								}
							}
						}
						else
#endif
						{
							SrcAudioLen = Min3(Source->DataLenFunc(),
												MCUCircular_GetSpaceLen(&SrcAdapter->SrcBufHandler) / SampleByte - 1,
												sizeof(AudioCore.AdaptOut)/SampleByte);
							if(SrcAudioLen)
							{
								SrcAudioLen = Source->DataGetFunc(AudioCore.AdaptOut, SrcAudioLen);
								if(SrcAudioLen > 0)
								{
									MCUCircular_PutData(&SrcAdapter->SrcBufHandler, AudioCore.AdaptOut, SrcAudioLen * SampleByte);
									pNetCt->AdaptProcessed = TRUE;
								}
							}
						}
						if(AdjAdapter->Enable)
						{

							SraAudioLen = SRAValidLenGet(MCUCircular_GetDataLen(&SrcAdapter->SrcBufHandler) / SampleByte, MCUCircular_GetSpaceLen(&AdjAdapter->SraBufHandler) / SampleByte);
							if(SraAudioLen)
							{

								SraAudioLen = MCUCircular_GetData(&SrcAdapter->SrcBufHandler, AudioCore.AdaptIn, SraAudioLen * SampleByte) / SampleByte;
								if(AdjAdapter->SraResFarCt.step == 0)
								{
									AdjAdapter->GroupAdj = AdjAdapter->AdjustVal;
								}
#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
								if(Source->PcmWidth == WIDTH_L24BIT)
								{
									resampler_farrow_over_frames_apply24(&AdjAdapter->SraResFarCt, (int32_t *)AudioCore.AdaptIn, (int32_t *)AudioCore.AdaptOut, ADJUST_GROUP_FRAME(AdjAdapter->GroupAdj), AdjAdapter->GroupAdj, ADAPTER_INPUT_BLOCK, (int32_t*)&SraAudioLen);
								}
								else
#endif
								{
									resampler_farrow_over_frames_apply(&AdjAdapter->SraResFarCt, (int16_t *)AudioCore.AdaptIn, (int16_t *)AudioCore.AdaptOut, ADJUST_GROUP_FRAME(AdjAdapter->GroupAdj), AdjAdapter->GroupAdj, ADAPTER_INPUT_BLOCK, (int32_t*)&SraAudioLen);
								}
								MCUCircular_PutData(&AdjAdapter->SraBufHandler, AudioCore.AdaptOut, SraAudioLen * SampleByte);
								pNetCt->AdaptProcessed = TRUE;
							}
						}
						else
						{
							SraAudioLen = Min3(MCUCircular_GetDataLen(&SrcAdapter->SrcBufHandler) / SampleByte,
												MCUCircular_GetSpaceLen(&AdjAdapter->SraBufHandler) / SampleByte - 1,
												sizeof(AudioCore.AdaptOut)/SampleByte);
							if(SraAudioLen)
							{
								SraAudioLen = MCUCircular_GetData(&SrcAdapter->SrcBufHandler, AudioCore.AdaptOut, SraAudioLen * SampleByte) / SampleByte;
								MCUCircular_PutData(&AdjAdapter->SraBufHandler, AudioCore.AdaptOut, SraAudioLen * SampleByte);
								pNetCt->AdaptProcessed = TRUE;
							}
						}
						if(MCUCircular_GetDataLen(&AdjAdapter->SraBufHandler) / SampleByte >= SOURCEFRAME(Index))
						{
							SOURCE_BIT_EN(pNetCt->FrameReady, Index);
						}
						else
						{
							SOURCE_BIT_DIS(pNetCt->FrameReady, Index);
						}
					}while(!SOURCE_BIT_GET(pNetCt->FrameReady, Index) && (SrcAudioLen || SraAudioLen));
					break;
				}
#if defined(AUDIOCORE_HWSRC_EN)
				case HWSRC_ONLY:
				case HWSRC_ADJUST:
				{
					SRC_ADAPTER * SrcAdapter = Source->SrcAdapter;
					do{
#ifdef CFG_FUNC_MIXER_SRC_EN
						if(SrcAdapter->SampleRate != AudioCore.NetCt[AudioCore.AudioSource[Index].Net].SampleRate \
								&& SrcAdapter->HwSrcIndex  < RES_SRC_NUM \
								&& AdaptHwCt.HwSrcCt[SrcAdapter->HwSrcIndex].SrcCase == SOURCE_GET_IO(Index))
						{
							if(SRC_IsBlockDone(SrcAdapter->HwSrcIndex))
							{
								uint32_t SrcLen = SRC_BlockDataLenGet((SRC_MODULE)(SrcAdapter->HwSrcIndex));
								if(SrcLen < MCUCircular_GetSpaceLen(&SrcAdapter->SrcBufHandler) / SampleByte)
								{
									DataCacheRangeInvalid((uint32_t)AdaptHwCt.HwSrcCt[SrcAdapter->HwSrcIndex].SrcContext.OutBuf, SrcLen * SampleByte);
									MCUCircular_PutData(&SrcAdapter->SrcBufHandler, AdaptHwCt.HwSrcCt[SrcAdapter->HwSrcIndex].SrcContext.OutBuf, SrcLen * SampleByte);
									SRC_BlockDoneFlagClean(SrcAdapter->HwSrcIndex);
									AdaptHwCt.HwSrcCt[Source->SrcAdapter->HwSrcIndex].SampleLenStartIn = 0;
								}
								pNetCt->AdaptProcessed = TRUE;
							}
							if(AdaptHwCt.HwSrcCt[Source->SrcAdapter->HwSrcIndex].SampleLenStartIn == 0)
							{
								SrcAudioLen = HWSRCValidLenGet(SrcAdapter->SampleRate,
													Source->DataLenFunc(),
													AudioCore.NetCt[AudioCore.AudioSource[Index].Net].SampleRate,
													MCUCircular_GetSpaceLen(&SrcAdapter->SrcBufHandler) / SampleByte - 1);
								if(SrcAudioLen)
								{
									SrcAudioLen = Source->DataGetFunc(AdaptHwCt.HwSrcCt[SrcAdapter->HwSrcIndex].SrcContext.InBuf, SrcAudioLen);
									SRC_BlockLenSet(SrcAdapter->HwSrcIndex, SrcAudioLen);
									SRC_BlockStart(SrcAdapter->HwSrcIndex);
								}
								AdaptHwCt.HwSrcCt[SrcAdapter->HwSrcIndex].SampleLenStartIn = SrcAudioLen;//记录点数
							}
						}
						else
#endif
						{
							SrcAudioLen = Min3(Source->DataLenFunc(),
												MCUCircular_GetSpaceLen(&SrcAdapter->SrcBufHandler) / SampleByte - 1,
												sizeof(AudioCore.AdaptOut)/SampleByte);
							if(SrcAudioLen)
							{
								SrcAudioLen = Source->DataGetFunc(AudioCore.AdaptOut, SrcAudioLen);
								MCUCircular_PutData(&SrcAdapter->SrcBufHandler, AudioCore.AdaptOut, SrcAudioLen * SampleByte);
								pNetCt->AdaptProcessed = TRUE;
							}
						}
						if(MCUCircular_GetDataLen(&SrcAdapter->SrcBufHandler) / (SampleByte) >= SOURCEFRAME(Index))
						{
							SOURCE_BIT_EN(pNetCt->FrameReady, Index);
						}
						else
						{
							SOURCE_BIT_DIS(pNetCt->FrameReady, Index);
						}
					}while(0);
					break;
				}
				case HWSRC_SRA:
				{
					SRC_ADAPTER * SrcAdapter = Source->SrcAdapter;
					SRA_ADAPTER * AdjAdapter = (SRA_ADAPTER *)Source->AdjAdapter;

					do{
#ifdef CFG_FUNC_MIXER_SRC_EN
						if(SrcAdapter->SampleRate != AudioCore.NetCt[AudioCore.AudioSource[Index].Net].SampleRate \
									&& SrcAdapter->HwSrcIndex  < RES_SRC_NUM \
									&& AdaptHwCt.HwSrcCt[SrcAdapter->HwSrcIndex].SrcCase == SOURCE_GET_IO(Index))
						{
							if(SRC_IsBlockDone(SrcAdapter->HwSrcIndex))
							{
								uint32_t SrcLen = SRC_BlockDataLenGet((SRC_MODULE)(SrcAdapter->HwSrcIndex));
								if(SrcLen < MCUCircular_GetSpaceLen(&SrcAdapter->SrcBufHandler) / SampleByte)
								{
									DataCacheRangeInvalid((uint32_t)AdaptHwCt.HwSrcCt[SrcAdapter->HwSrcIndex].SrcContext.OutBuf, SrcLen * SampleByte);
									MCUCircular_PutData(&SrcAdapter->SrcBufHandler, AdaptHwCt.HwSrcCt[SrcAdapter->HwSrcIndex].SrcContext.OutBuf, SrcLen * SampleByte);
									SRC_BlockDoneFlagClean(SrcAdapter->HwSrcIndex);
									AdaptHwCt.HwSrcCt[Source->SrcAdapter->HwSrcIndex].SampleLenStartIn = 0;
								}
								pNetCt->AdaptProcessed = TRUE;
							}
							if(AdaptHwCt.HwSrcCt[Source->SrcAdapter->HwSrcIndex].SampleLenStartIn == 0)
							{
								SrcAudioLen = HWSRCValidLenGet(SrcAdapter->SampleRate,
													Source->DataLenFunc(),
													AudioCore.NetCt[AudioCore.AudioSource[Index].Net].SampleRate,
													MCUCircular_GetSpaceLen(&SrcAdapter->SrcBufHandler) / SampleByte);
								if(SrcAudioLen)
								{
									SrcAudioLen = Source->DataGetFunc(AdaptHwCt.HwSrcCt[SrcAdapter->HwSrcIndex].SrcContext.InBuf, SrcAudioLen);
									SRC_BlockLenSet(SrcAdapter->HwSrcIndex, SrcAudioLen);
									SRC_BlockStart(SrcAdapter->HwSrcIndex);
								}
								AdaptHwCt.HwSrcCt[SrcAdapter->HwSrcIndex].SampleLenStartIn = SrcAudioLen;//记录点数
							}
						}
						else
#endif
						{
							SrcAudioLen = Min3(Source->DataLenFunc(),
												MCUCircular_GetSpaceLen(&SrcAdapter->SrcBufHandler) / SampleByte - 1,
												sizeof(AudioCore.AdaptOut)/SampleByte);
							if(SrcAudioLen)
							{
								SrcAudioLen = Source->DataGetFunc(AudioCore.AdaptOut, SrcAudioLen);
								if(SrcAudioLen > 0)
								{
									MCUCircular_PutData(&SrcAdapter->SrcBufHandler, AudioCore.AdaptOut, SrcAudioLen * SampleByte);
									pNetCt->AdaptProcessed = TRUE;
								}

							}
						}
						if(AdjAdapter->Enable)
						{

							SraAudioLen = SRAValidLenGet(MCUCircular_GetDataLen(&SrcAdapter->SrcBufHandler) / SampleByte, MCUCircular_GetSpaceLen(&AdjAdapter->SraBufHandler) / SampleByte);
							if(SraAudioLen)
							{
								SraAudioLen = MCUCircular_GetData(&SrcAdapter->SrcBufHandler, AudioCore.AdaptIn, SraAudioLen * SampleByte) / SampleByte;
								if(AdjAdapter->SraResFarCt.step == 0)
								{
									AdjAdapter->GroupAdj = AdjAdapter->AdjustVal;
								}
#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
								if(Source->PcmWidth == WIDTH_L24BIT)
								{
									resampler_farrow_over_frames_apply24(&AdjAdapter->SraResFarCt, (int32_t *)AudioCore.AdaptIn, (int32_t *)AudioCore.AdaptOut, ADJUST_GROUP_FRAME(AdjAdapter->GroupAdj), AdjAdapter->GroupAdj, ADAPTER_INPUT_BLOCK, (int32_t*)&SraAudioLen);
								}
								else
#endif
								{
									resampler_farrow_over_frames_apply(&AdjAdapter->SraResFarCt, (int16_t *)AudioCore.AdaptIn, (int16_t *)AudioCore.AdaptOut, ADJUST_GROUP_FRAME(AdjAdapter->GroupAdj), AdjAdapter->GroupAdj, ADAPTER_INPUT_BLOCK, (int32_t*)&SraAudioLen);
								}
								MCUCircular_PutData(&AdjAdapter->SraBufHandler, AudioCore.AdaptOut, SraAudioLen * SampleByte);
								pNetCt->AdaptProcessed = TRUE;
							}
						}
						else
						{
							SraAudioLen = Min3(MCUCircular_GetDataLen(&SrcAdapter->SrcBufHandler) / SampleByte,
												MCUCircular_GetSpaceLen(&AdjAdapter->SraBufHandler) / SampleByte - 1,
												sizeof(AudioCore.AdaptOut)/SampleByte);
							if(SraAudioLen)
							{
								SraAudioLen = MCUCircular_GetData(&SrcAdapter->SrcBufHandler, AudioCore.AdaptOut, SraAudioLen * SampleByte) / SampleByte;
								MCUCircular_PutData(&AdjAdapter->SraBufHandler, AudioCore.AdaptOut, SraAudioLen * SampleByte);
								pNetCt->AdaptProcessed = TRUE;
							}
						}
						if(MCUCircular_GetDataLen(&AdjAdapter->SraBufHandler) / SampleByte >= SOURCEFRAME(Index))
						{
							SOURCE_BIT_EN(pNetCt->FrameReady, Index);
						}
						else
						{
							SOURCE_BIT_DIS(pNetCt->FrameReady, Index);
						}
					}while(!SOURCE_BIT_GET(pNetCt->FrameReady, Index) && SraAudioLen);
					break;
				}
#endif //#if defined(AUDIOCORE_HWSRC_EN)
			}
		}
	}

	for(Index = 0; Index < AUDIO_CORE_SINK_MAX_NUM; Index++)
	{
		Sink = &AudioCore.AudioSink[Index];
		SampleByte = PCM_BYTE(Sink->PcmWidth) * Sink->Channels;

		if(Sink->Enable
#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
			&& Sink->Net == FlowNet
#endif
#ifdef AUDIO_CORE_DEBUG
			&& Sink->DataSetFunc
			&& Sink->SpaceLenFunc
#endif
			)
		{

			switch(Sink->Adapt)
			{
				case STD:
				case CLK_ADJUST_ONLY:
					if(Sink->SpaceLenFunc() >= SINKFRAME(Index))
					{
						SINK_BIT_EN(pNetCt->FrameReady, Index);
					}
					else
					{
						SINK_BIT_DIS(pNetCt->FrameReady, Index);
					}
					break;
				case SRC_ONLY:
				case SRC_ADJUST:
				{
					SRC_ADAPTER * SrcAdapter = Sink->SrcAdapter;
					do{
#ifdef CFG_FUNC_MIXER_SRC_EN
						if(SrcAdapter->SampleRate != AudioCore.NetCt[Sink->Net].SampleRate)
						{
							SrcAudioLen = SRCValidLenGet(AudioCore.NetCt[Sink->Net].SampleRate,
												MCUCircular_GetDataLen(&SrcAdapter->SrcBufHandler) / SampleByte,
												SrcAdapter->SampleRate,
												Sink->SpaceLenFunc() - 1);
							if(SrcAudioLen)
							{
								SrcAudioLen = MCUCircular_GetData(&SrcAdapter->SrcBufHandler, AudioCore.AdaptIn, SrcAudioLen * SampleByte) / SampleByte;
	#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
								if(Sink->PcmWidth == WIDTH_L24BIT)
								{
									SrcAudioLen = SRC_APPLY_24(SrcAudioLen);
									if(SrcAudioLen < 0) SrcAudioLen = 0;
								}
								else if(Sink->PcmWidth == WIDTH_16BIT)
	#endif
								{
									SrcAudioLen = SRC_APPLY_16(SrcAudioLen);
									if(SrcAudioLen < 0) SrcAudioLen = 0;
								}
								Sink->DataSetFunc(AudioCore.AdaptOut, SrcAudioLen);
								pNetCt->AdaptProcessed = TRUE;
							}
						}
						else
#endif
						{
							SrcAudioLen = Min3(MCUCircular_GetDataLen(&SrcAdapter->SrcBufHandler) / SampleByte,
												Sink->SpaceLenFunc() - 1,
												sizeof(AudioCore.AdaptOut)/ SampleByte);
							if(SrcAudioLen)
							{
								SrcAudioLen = MCUCircular_GetData(&SrcAdapter->SrcBufHandler, AudioCore.AdaptOut, SrcAudioLen * SampleByte) / SampleByte;
								Sink->DataSetFunc(AudioCore.AdaptOut, SrcAudioLen);
								pNetCt->AdaptProcessed = TRUE;
							}
						}

						if(MCUCircular_GetSpaceLen(&SrcAdapter->SrcBufHandler) / SampleByte >= SINKFRAME(Index))
						{
							SINK_BIT_EN(pNetCt->FrameReady, Index);
						}
						else
						{
							SINK_BIT_DIS(pNetCt->FrameReady, Index);
						}
					}while(Sink->SpaceLenFunc() > SINKFRAME(Index) && SrcAudioLen);
					break;
				}
				case SRA_ONLY:
				{
					SRA_ADAPTER * AdjAdapter = (SRA_ADAPTER *)Sink->AdjAdapter;
					do{
						if(AdjAdapter->Enable)
						{
							SraAudioLen = Sink->SpaceLenFunc();
							SraAudioLen = SRAValidLenGet(MCUCircular_GetDataLen(&AdjAdapter->SraBufHandler) / SampleByte, SraAudioLen);
							if(SraAudioLen)
							{
								SraAudioLen = MCUCircular_GetData(&AdjAdapter->SraBufHandler, AudioCore.AdaptIn, SraAudioLen * SampleByte) / SampleByte;
								if(AdjAdapter->SraResFarCt.step == 0)
								{
									AdjAdapter->GroupAdj = AdjAdapter->AdjustVal;
								}
#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
								if(Sink->PcmWidth == WIDTH_L24BIT)
								{
									resampler_farrow_over_frames_apply24(&AdjAdapter->SraResFarCt, (int32_t *)AudioCore.AdaptIn, (int32_t *)AudioCore.AdaptOut, ADJUST_GROUP_FRAME(AdjAdapter->GroupAdj), AdjAdapter->GroupAdj, ADAPTER_INPUT_BLOCK, (int32_t*)&SraAudioLen);
								}
								else
#endif
								{
									resampler_farrow_over_frames_apply(&AdjAdapter->SraResFarCt, (int16_t *)AudioCore.AdaptIn, (int16_t *)AudioCore.AdaptOut, ADJUST_GROUP_FRAME(AdjAdapter->GroupAdj), AdjAdapter->GroupAdj, ADAPTER_INPUT_BLOCK, (int32_t*)&SraAudioLen);
								}
								PcmWidthConvert(AudioCore.AdaptOut, CFG_DEFAULT_WIDTH, AudioCore.AdaptOut, Sink->PcmWidth, SraAudioLen * Sink->Channels);
								Sink->DataSetFunc(AudioCore.AdaptOut, SraAudioLen);
								pNetCt->AdaptProcessed = TRUE;
							}
						}
						else
						{
							SraAudioLen = Min3(MCUCircular_GetDataLen(&AdjAdapter->SraBufHandler) / SampleByte,
												Sink->SpaceLenFunc() - 1,
											sizeof(AudioCore.AdaptOut)/SampleByte);
							if(SraAudioLen)
							{
								SraAudioLen = MCUCircular_GetData(&AdjAdapter->SraBufHandler, AudioCore.AdaptOut, SraAudioLen * SampleByte) / SampleByte;
								Sink->DataSetFunc(AudioCore.AdaptOut, SraAudioLen);
								pNetCt->AdaptProcessed = TRUE;
							}
						}

						if(MCUCircular_GetSpaceLen(&AdjAdapter->SraBufHandler) / SampleByte >= SINKFRAME(Index))
						{
							SINK_BIT_EN(pNetCt->FrameReady, Index);
						}
						else
						{
							SINK_BIT_DIS(pNetCt->FrameReady, Index);
						}
					}while(Sink->SpaceLenFunc() > SINKFRAME(Index) && SraAudioLen);
					break;
				}
				case SRC_SRA:
				{
					SRA_ADAPTER * AdjAdapter = (SRA_ADAPTER *)Sink->AdjAdapter;
					SRC_ADAPTER * SrcAdapter = Sink->SrcAdapter;
					uint32_t      PcmDataLen = PCM_BYTE(Sink->PcmWidth) * Sink->Channels;

					do{
						if(AdjAdapter->Enable)
						{
							SraAudioLen = SRAValidLenGet(MCUCircular_GetDataLen(&AdjAdapter->SraBufHandler) / SampleByte,
														MCUCircular_GetSpaceLen(&SrcAdapter->SrcBufHandler) / SampleByte);
							if(SraAudioLen)
							{
								SraAudioLen = MCUCircular_GetData(&AdjAdapter->SraBufHandler, AudioCore.AdaptIn, SraAudioLen * SampleByte) / SampleByte;
								if(AdjAdapter->SraResFarCt.step == 0)
								{
									AdjAdapter->GroupAdj = AdjAdapter->AdjustVal;
								}
#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
								if(Sink->PcmWidth == WIDTH_L24BIT)
								{
									resampler_farrow_over_frames_apply24(&AdjAdapter->SraResFarCt, (int32_t *)AudioCore.AdaptIn, (int32_t *)AudioCore.AdaptOut, ADJUST_GROUP_FRAME(AdjAdapter->GroupAdj), AdjAdapter->GroupAdj, ADAPTER_INPUT_BLOCK, (int32_t*)&SraAudioLen);
								}
								else
#endif
								{
									resampler_farrow_over_frames_apply(&AdjAdapter->SraResFarCt, (int16_t *)AudioCore.AdaptIn, (int16_t *)AudioCore.AdaptOut, ADJUST_GROUP_FRAME(AdjAdapter->GroupAdj), AdjAdapter->GroupAdj, ADAPTER_INPUT_BLOCK, (int32_t*)&SraAudioLen);
								}
								MCUCircular_PutData(&SrcAdapter->SrcBufHandler, AudioCore.AdaptOut, SraAudioLen * SampleByte);
								pNetCt->AdaptProcessed = TRUE;
							}
						}
						else
						{
							SraAudioLen = Min3(MCUCircular_GetDataLen(&AdjAdapter->SraBufHandler) / SampleByte,
												MCUCircular_GetSpaceLen(&SrcAdapter->SrcBufHandler) / SampleByte - 1,
												sizeof(AudioCore.AdaptOut)/PcmDataLen);
							if(SraAudioLen)
							{
								SraAudioLen = MCUCircular_GetData(&AdjAdapter->SraBufHandler, AudioCore.AdaptOut, SraAudioLen * SampleByte) / SampleByte;
								MCUCircular_PutData(&SrcAdapter->SrcBufHandler, AudioCore.AdaptOut, SraAudioLen * SampleByte);
								pNetCt->AdaptProcessed = TRUE;
							}
						}
						if(MCUCircular_GetSpaceLen(&AdjAdapter->SraBufHandler) / PcmDataLen >= SINKFRAME(Index))
						{
							SINK_BIT_EN(pNetCt->FrameReady, Index);
						}
						else
						{
							SINK_BIT_DIS(pNetCt->FrameReady, Index);
						}
#ifdef CFG_FUNC_MIXER_SRC_EN
						if(SrcAdapter->SampleRate != AudioCore.NetCt[AudioCore.AudioSink[Index].Net].SampleRate)
						{
							SrcAudioLen = SRCValidLenGet(AudioCore.NetCt[AudioCore.AudioSink[Index].Net].SampleRate,
												MCUCircular_GetDataLen(&SrcAdapter->SrcBufHandler) / WIDTH_L24BIT,
												SrcAdapter->SampleRate,
												Sink->SpaceLenFunc());
							if(SrcAudioLen)
							{
								SrcAudioLen = MCUCircular_GetData(&SrcAdapter->SrcBufHandler, AudioCore.AdaptIn, SrcAudioLen * SampleByte) / SampleByte;
	#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
								if(Sink->PcmWidth == WIDTH_L24BIT)
								{
									SrcAudioLen = SRC_APPLY_24(SrcAudioLen);
									if(SrcAudioLen < 0) SrcAudioLen = 0;
								}
								else
	#endif
								{
									SrcAudioLen = SRC_APPLY_16(SrcAudioLen);
									if(SrcAudioLen < 0) SrcAudioLen = 0;
								}
								Sink->DataSetFunc(AudioCore.AdaptOut, SrcAudioLen);
								pNetCt->AdaptProcessed = TRUE;
							}
						}
						else
#endif
						{
							SrcAudioLen = Min3(MCUCircular_GetDataLen(&SrcAdapter->SrcBufHandler) / SampleByte,
												Sink->SpaceLenFunc() - 1,
												sizeof(AudioCore.AdaptOut)/SampleByte);
							if(SrcAudioLen)
							{
								SrcAudioLen = MCUCircular_GetData(&SrcAdapter->SrcBufHandler, AudioCore.AdaptOut, SrcAudioLen * SampleByte) / SampleByte;
								Sink->DataSetFunc(AudioCore.AdaptOut, SrcAudioLen);
								pNetCt->AdaptProcessed = TRUE;
							}
						}

					}while(Sink->SpaceLenFunc() > SINKFRAME(Index) && (SrcAudioLen || SraAudioLen));
					break;
				}
				default:
					break;
			}
		}
	}
}

//获取adapter 数据等效存量
uint16_t AudioCoreSinkAdapterDataLenGet(uint8_t Index)
{
	AudioCoreSink *Sink = &AudioCore.AudioSink[Index];
	SRC_ADAPTER *SrcAdapter = Sink->SrcAdapter;
	uint16_t DataLen = 0;
	uint32_t SampleByte;

	if(!Sink->Enable)
		return 0;

	SampleByte = PCM_BYTE(Sink->PcmWidth) * Sink->Channels;

	switch(Sink->Adapt)
	{
		case STD:
		case CLK_ADJUST_ONLY:
			DataLen = 0;
			break;

		case SRC_ONLY:
		case SRC_ADJUST:
		{
			DataLen = MCUCircular_GetDataLen(&SrcAdapter->SrcBufHandler) / SampleByte;
			break;
		}

		case SRA_ONLY:
		{
			SRA_ADAPTER * AdjAdapter = (SRA_ADAPTER *)Sink->AdjAdapter;
			DataLen = MCUCircular_GetDataLen(&AdjAdapter->SraBufHandler) / SampleByte;
			break;
		}

		case SRC_SRA:
		{
			SRA_ADAPTER * AdjAdapter = (SRA_ADAPTER *)Sink->AdjAdapter;
			DataLen = (MCUCircular_GetDataLen(&AdjAdapter->SraBufHandler) + MCUCircular_GetDataLen(&SrcAdapter->SrcBufHandler)) / SampleByte;
			break;
		}
		default:
			break;
	}
	return DataLen;
}

//依据Spacelenfunc和depth，获取数据等效存量
uint32_t AudioCoreSinkDataLenGet(uint8_t Index)
{
	AudioCoreSink *Sink = &AudioCore.AudioSink[Index];
	SRC_ADAPTER *SrcAdapter = Sink->SrcAdapter;
	uint32_t DataLen = 0;

	if(!Sink->Enable)
		return 0;

	DataLen = Sink->SpaceLenFunc();
	if(Sink->LenFuncDepth <= DataLen) //防(错配)溢出
	{
		DataLen = 0;
	}
	else
	{
		DataLen = Sink->LenFuncDepth - DataLen;
	}

	switch(Sink->Adapt)
	{
		case SRC_ONLY:
		case SRC_SRA:
		case SRC_ADJUST:
			DataLen = ((uint64_t)AudioCore.NetCt[AudioCore.AudioSink[Index].Net].SampleRate * DataLen) / SrcAdapter->SampleRate;
			break;
		default:
			break;
	}
	return DataLen;
}

#ifdef BT_AUDIO_AAC_ENABLE
static uint16_t AdjustLevelHigh	= 0;
static uint16_t AdjustLevelLow	= 0;

void BtPlay_FreqAdjustLevelSet(uint16_t LevelLow, uint16_t LevelHigh)
{
	AdjustLevelHigh = LevelHigh;
	AdjustLevelLow  = LevelLow;
}
#endif

void AudioCoreSourceGet(uint8_t Index)
{
	NetContext *pNetCt = &AudioCore.NetCt[AudioCore.AudioSource[Index].Net];
	AudioCoreSource *Source = &AudioCore.AudioSource[Index];
	SRC_ADAPTER * SrcAdapter = Source->SrcAdapter;
	uint16_t SampleByte = PCM_BYTE(Source->PcmWidth) * Source->Channels;
	void *DataBuffer = (void*)Source->PcmInBuf;
#if defined(AUDIOCORE_HWSRC_EN)
	uint16_t PartLen = 0;
#endif
	if((Source->FlowCh != Source->Channels)
#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
			||(Source->FlowWidth != Source->PcmWidth)
#endif
			)//防止Flow 16bit溢出
	{
		memset(pNetCt->NetBuf, 0, SOURCEFRAME(Index) * SampleByte);
		DataBuffer = pNetCt->NetBuf;
	}

	switch(Source->Adapt)
	{
		case STD:
			Source->DataGetFunc(DataBuffer, SOURCEFRAME(Index));
			break;
#if defined(AUDIOCORE_HWSRC_EN)
		case HWSRC_ONLY:
#endif
		case SRC_ONLY:
		{
			MCUCircular_GetData(&SrcAdapter->SrcBufHandler, DataBuffer, SOURCEFRAME(Index) * SampleByte);
			break;
		}
		case SRA_ONLY:
		{
			SRA_ADAPTER * AdjAdapter = (SRA_ADAPTER *)Source->AdjAdapter;
			MCUCircular_GetData(&AdjAdapter->SraBufHandler, DataBuffer, SOURCEFRAME(Index) * SampleByte);
			if(AdjAdapter->Enable)
			{
				AdjAdapter->TotalNum += Source->DataLenFunc() + MCUCircular_GetDataLen(&AdjAdapter->SraBufHandler) / SampleByte;
				AdjAdapter->Count++;
				if(AdjAdapter->Count >= ADJUST_PERIOD / SOURCEFRAME(Index))
				{
					AdjAdapter->AdjustVal = ADJLEVEL(AdjAdapter->TotalNum / AdjAdapter->Count,
										(AdjAdapter->ValidDepth * AdjAdapter->LowLevelCent) / 100,
										(AdjAdapter->ValidDepth * AdjAdapter->HighLevelCent) / 100);
					AdjAdapter->TotalNum = 0;
					AdjAdapter->Count = 0;
				}
			}
			break;
		}
		case CLK_ADJUST_ONLY:
		{
			CLK_ADJUST_ADAPTER * AdjAdapter = (CLK_ADJUST_ADAPTER *)Source->AdjAdapter;
			Source->DataGetFunc(DataBuffer, SOURCEFRAME(Index));
			if(AdjAdapter->Enable)
			{
				AdjAdapter->TotalNum += Source->DataLenFunc();
				AdjAdapter->Count++;
				if(AdjAdapter->AdjTickVal !=  AdaptHwCt.AdjTimer.TickValCache)
				{
					ClkAdjust(AdjAdapter, SOURCE_GET_IO(Index));
				}
			}
			break;
		}
#if defined(AUDIOCORE_HWSRC_EN)
		case HWSRC_SRA:
			PartLen = AdaptHwCt.HwSrcCt[SrcAdapter->HwSrcIndex].SampleLenStartIn;
#endif
		case SRC_SRA:
		{
			SRA_ADAPTER * AdjAdapter = (SRA_ADAPTER *)Source->AdjAdapter;
			MCUCircular_GetData(&AdjAdapter->SraBufHandler, DataBuffer, SOURCEFRAME(Index) * SampleByte);
			if(AdjAdapter->Enable)
#ifdef BT_AUDIO_AAC_ENABLE
			{
				static int8_t  AdjustFlag = 0;//方向
				uint8_t index = btManager.btLinked_env[btManager.cur_index].a2dp_index;
				if((GetSystemMode() == ModeBtAudioPlay)
					&& (index < BT_LINK_DEV_NUM)
					&& (btManager.a2dpStreamType[index] == BT_A2DP_STREAM_TYPE_AAC || btManager.a2dpStreamType[index] == BT_A2DP_STREAM_TYPE_SBC))
				{
					if(btManager.a2dpStreamType[index] == BT_A2DP_STREAM_TYPE_AAC)
						AdjAdapter->TotalNum += GetValidFrameDataSize();
					else
						AdjAdapter->TotalNum += GetValidSbcDataSize();
					AdjAdapter->Count++;

					if(AdjAdapter->Count >= ADJUST_PERIOD / SOURCEFRAME(Index))
					{
						AdjAdapter->TotalNum = AdjAdapter->TotalNum/AdjAdapter->Count;

						if(AdjustFlag == 0)
						{
							if(AdjAdapter->TotalNum < AdjustLevelLow)
							{
								AdjustFlag = 1;//正
							}
							else if(AdjAdapter->TotalNum  > AdjustLevelHigh)
							{
								AdjustFlag = -1;//负
							}
							else
							{
								AdjustFlag = 0;
							}
						}
						else if(AdjustFlag == 1)
						{
							if(AdjAdapter->TotalNum  < AdjustLevelLow)
							{
								//插点
								AdjAdapter->AdjustVal = 1;
								//APP_DBG("!>!");
							}
							else
							{
								AdjustFlag = 0;
							}
						}
						else if(AdjustFlag == -1)
						{
							if(AdjAdapter->TotalNum > AdjustLevelHigh)
							{
								//丢点
								AdjAdapter->AdjustVal = -1;
								//APP_DBG("!<!");
							}
							else
							{
								AdjustFlag = 0;
							}
						}

						AdjAdapter->TotalNum = 0;
						AdjAdapter->Count = 0;
					}
				}
			}
			else
#endif
			{
#if defined(AUDIOCORE_HWSRC_EN)
				AdjAdapter->TotalNum += ((uint64_t)AudioCore.NetCt[Source->Net].SampleRate * (Source->DataLenFunc() + PartLen) / SrcAdapter->SampleRate \
										 + MCUCircular_GetDataLen(&SrcAdapter->SrcBufHandler) / SampleByte \
										 + MCUCircular_GetDataLen(&AdjAdapter->SraBufHandler) / SampleByte);
#else
				AdjAdapter->TotalNum += ((uint64_t)AudioCore.NetCt[Source->Net].SampleRate * (Source->DataLenFunc()) / SrcAdapter->SampleRate \
										 + MCUCircular_GetDataLen(&SrcAdapter->SrcBufHandler) / SampleByte \
										 + MCUCircular_GetDataLen(&AdjAdapter->SraBufHandler) / SampleByte);
#endif

				AdjAdapter->Count++;
				if(AdjAdapter->Count >= ADJUST_PERIOD / SOURCEFRAME(Index))
				{
					AdjAdapter->AdjustVal = ADJLEVEL(AdjAdapter->TotalNum / AdjAdapter->Count,
										(AdjAdapter->ValidDepth * AdjAdapter->LowLevelCent) / 100,
										(AdjAdapter->ValidDepth * AdjAdapter->HighLevelCent) / 100);
//						printf("\n %d %d val %d\n",ValidDepth,AdjAdapter->TotalNum / AdjAdapter->Count,AdjAdapter->AdjustVal);
					AdjAdapter->TotalNum = 0;
					AdjAdapter->Count = 0;
				}
			}
			break;
		}
#if defined(AUDIOCORE_HWSRC_EN)
		case HWSRC_ADJUST:
			PartLen = AdaptHwCt.HwSrcCt[SrcAdapter->HwSrcIndex].SampleLenStartIn;
#endif
		case SRC_ADJUST:
		{
			CLK_ADJUST_ADAPTER * AdjAdapter = (CLK_ADJUST_ADAPTER *)Source->AdjAdapter;
			MCUCircular_GetData(&SrcAdapter->SrcBufHandler, DataBuffer, SOURCEFRAME(Index) * SampleByte);
			if(AdjAdapter->Enable)
			{
#if defined(AUDIOCORE_HWSRC_EN)
				AdjAdapter->TotalNum += ((uint64_t)AudioCore.NetCt[Source->Net].SampleRate * (Source->DataLenFunc() + PartLen)) / SrcAdapter->SampleRate + MCUCircular_GetDataLen(&SrcAdapter->SrcBufHandler) / SampleByte;
#else
				AdjAdapter->TotalNum += ((uint64_t)AudioCore.NetCt[Source->Net].SampleRate * (Source->DataLenFunc())) / SrcAdapter->SampleRate + MCUCircular_GetDataLen(&SrcAdapter->SrcBufHandler) / SampleByte;
#endif
				AdjAdapter->Count++;
				if(AdjAdapter->AdjTickVal !=  AdaptHwCt.AdjTimer.TickValCache)
				{
					ClkAdjust(AdjAdapter, SOURCE_GET_IO(Index));
				}
			}
			break;
		}
	}

	if((Source->FlowCh != Source->Channels)
#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
			||(Source->FlowWidth != Source->PcmWidth)
#endif
			)
	{
#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
		PcmWidthConvert(DataBuffer, Source->PcmWidth, DataBuffer, Source->FlowWidth, SOURCEFRAME(Index) * Source->Channels);
		if((Source->FlowWidth == WIDTH_L24BIT) && (Source->Channels == 1) && (Source->FlowCh == 2))
		{
			upmix_1to2_apply24(DataBuffer, Source->PcmInBuf, SOURCEFRAME(Index));
		}
		else if((Source->FlowWidth == WIDTH_L24BIT) && (Source->Channels == 2) && (Source->FlowCh == 1))
		{
			downmix_2to1_apply24(DataBuffer, Source->PcmInBuf, SOURCEFRAME(Index));
		}
		else
#endif
		{
			if((Source->Channels == 1) && (Source->FlowCh == 2))
			{
				upmix_1to2_apply(DataBuffer, (int16_t*)Source->PcmInBuf, SOURCEFRAME(Index));
			}
			else if((Source->Channels == 2) && (Source->FlowCh == 1))
			{
				downmix_2to1_apply(DataBuffer, (int16_t*)Source->PcmInBuf, SOURCEFRAME(Index));
			}
			else
			{
				memcpy(Source->PcmInBuf, DataBuffer, SOURCEFRAME(Index) * PCM_BYTE(Source->FlowWidth) * Source->FlowCh);
			}
		}
	}
}

void AudioCoreSinkSet(uint8_t Index)
{
	NetContext *pNetCt = &AudioCore.NetCt[AudioCore.AudioSink[Index].Net];
	AudioCoreSink *Sink = &AudioCore.AudioSink[Index];
	SRC_ADAPTER *SrcAdapter = Sink->SrcAdapter;
	uint16_t SampleByte = PCM_BYTE(Sink->PcmWidth) * Sink->Channels;
	void *DataBuffer = (void*)Sink->PcmOutBuf;
	void *PresetBuf = pNetCt->NetBuf;

	if((Sink->FlowCh != Sink->Channels)
#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
			||(Sink->FlowWidth != Sink->PcmWidth)
#endif
	)//防止Flow 16bit Buf溢出
	{

#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
		PcmWidthConvert(DataBuffer, Sink->FlowWidth, pNetCt->NetBuf, Sink->PcmWidth, SINKFRAME(Index) * Sink->FlowCh);
		DataBuffer = pNetCt->NetBuf;
		if((Sink->PcmWidth == WIDTH_L24BIT) && (Sink->FlowCh == 1) && (Sink->Channels == 2))
		{
			upmix_1to2_apply24(DataBuffer, (int32_t*)pNetCt->NetBuf, SINKFRAME(Index));
			DataBuffer = pNetCt->NetBuf;
		}
		else if((Sink->PcmWidth == WIDTH_L24BIT) && (Sink->FlowCh == 2) && (Sink->Channels == 1))
		{
			downmix_2to1_apply24(DataBuffer, (int32_t*)pNetCt->NetBuf, SINKFRAME(Index));
			DataBuffer = pNetCt->NetBuf;
		}
		else
#endif
		{
			if((Sink->FlowCh == 1) && (Sink->Channels == 2))
			{
				upmix_1to2_apply(DataBuffer, (int16_t*)pNetCt->NetBuf, SINKFRAME(Index));
				DataBuffer = pNetCt->NetBuf;
			}
			else if((Sink->FlowCh == 2) && (Sink->Channels == 1))
			{
				downmix_2to1_apply(DataBuffer, (int16_t*)pNetCt->NetBuf, SINKFRAME(Index));
				DataBuffer = pNetCt->NetBuf;
			}
		}
	}
	if(DataBuffer == pNetCt->NetBuf)
	{
		PresetBuf = (void*)Sink->PcmOutBuf;
	}

	switch(Sink->Adapt)
	{
		case STD:
			if(Sink->PresetFlag)
			{
				memset(PresetBuf, 0, SampleByte * SINKFRAME(Index) * 3 / 4);
				Sink->DataSetFunc(PresetBuf, SINKFRAME(Index) * 3 / 4);
				Sink->PresetFlag = FALSE;
			}
			Sink->DataSetFunc(DataBuffer, SINKFRAME(Index));
			break;
		case SRC_ONLY:
		{
			MCUCircular_PutData(&SrcAdapter->SrcBufHandler, DataBuffer, SINKFRAME(Index) * SampleByte);
			break;
		}
		case SRA_ONLY:
		{
			SRA_ADAPTER * AdjAdapter = (SRA_ADAPTER *)Sink->AdjAdapter;
			MCUCircular_PutData(&AdjAdapter->SraBufHandler, DataBuffer, SINKFRAME(Index) * SampleByte);
			if(AdjAdapter->Enable)
			{
				AdjAdapter->TotalNum += Sink->SpaceLenFunc() + MCUCircular_GetSpaceLen(&AdjAdapter->SraBufHandler) / SampleByte;
				AdjAdapter->Count++;
				if(AdjAdapter->Count >= ADJUST_PERIOD / SINKFRAME(Index))
				{
					AdjAdapter->AdjustVal = -ADJLEVEL(AdjAdapter->TotalNum / AdjAdapter->Count,
													(AdjAdapter->ValidDepth * AdjAdapter->LowLevelCent) / 100,
													(AdjAdapter->ValidDepth * AdjAdapter->HighLevelCent) / 100);
					AdjAdapter->TotalNum = 0;
					AdjAdapter->Count = 0;
				}
			}
			break;
		}
		case CLK_ADJUST_ONLY:
		{
			CLK_ADJUST_ADAPTER * AdjAdapter = (CLK_ADJUST_ADAPTER *)Sink->AdjAdapter;


			if(Sink->PresetFlag)
			{
				memset(PresetBuf, 0, SampleByte * SINKFRAME(Index) * 3 / 4);
				Sink->DataSetFunc(PresetBuf, SINKFRAME(Index) * 3 / 4);
				Sink->PresetFlag = FALSE;
			}
			Sink->DataSetFunc(DataBuffer, SINKFRAME(Index));
			if(AdjAdapter->Enable)
			{
				AdjAdapter->TotalNum += Sink->SpaceLenFunc();
				AdjAdapter->Count++;
				if(AdjAdapter->AdjTickVal !=  AdaptHwCt.AdjTimer.TickValCache)
				{
					ClkAdjust(AdjAdapter, SINK_GET_IO(Index));
				}
			}
			break;
		}
		case SRC_SRA://数据存放于fifo
		{
			SRA_ADAPTER * AdjAdapter = (SRA_ADAPTER *)Sink->AdjAdapter;
			MCUCircular_PutData(&AdjAdapter->SraBufHandler, DataBuffer, SINKFRAME(Index) * SampleByte);
			if(AdjAdapter->Enable)
			{
				AdjAdapter->TotalNum += ((Sink->SpaceLenFunc() * AudioCore.NetCt[AudioCore.AudioSink[Index].Net].SampleRate) / (SrcAdapter->SampleRate) \
										+ MCUCircular_GetSpaceLen(&AdjAdapter->SraBufHandler) / SampleByte \
															 + MCUCircular_GetSpaceLen(&SrcAdapter->SrcBufHandler) / SampleByte);
				AdjAdapter->Count++;
				if(AdjAdapter->Count >= ADJUST_PERIOD / SINKFRAME(Index))
				{

					AdjAdapter->AdjustVal = -ADJLEVEL(AdjAdapter->TotalNum / AdjAdapter->Count,
													(AdjAdapter->ValidDepth * AdjAdapter->LowLevelCent) / 100,
													(AdjAdapter->ValidDepth * AdjAdapter->HighLevelCent) / 100);
//					printf("\n %d %d val %d\n",ValidDepth,AdjAdapter->TotalNum / AdjAdapter->Count,AdjAdapter->AdjustVal);
					AdjAdapter->TotalNum = 0;
					AdjAdapter->Count = 0;
				}
			}
			break;
		}
		case SRC_ADJUST:
		{
			CLK_ADJUST_ADAPTER * AdjAdapter = (CLK_ADJUST_ADAPTER *)Sink->AdjAdapter;
			MCUCircular_PutData(&SrcAdapter->SrcBufHandler, DataBuffer, SINKFRAME(Index) * SampleByte);
			if(AdjAdapter->Enable)
			{
				AdjAdapter->TotalNum += ((uint64_t)AudioCore.NetCt[AudioCore.AudioSink[Index].Net].SampleRate * Sink->SpaceLenFunc()) / SrcAdapter->SampleRate
										+ MCUCircular_GetSpaceLen(&SrcAdapter->SrcBufHandler) / SampleByte ;
				AdjAdapter->Count++;
				if(AdjAdapter->AdjTickVal !=  AdaptHwCt.AdjTimer.TickValCache)
				{
					ClkAdjust(AdjAdapter, SINK_GET_IO(Index));
				}
			}
			break;
		}
		default:
			break;
	}
}

__attribute__((section(".ilm_section")))
bool AudioCoreSyncing(uint8_t FlowNet)
{
	uint8_t ScanNet = FlowNet;
	NetContext *pNetCt = &AudioCore.NetCt[FlowNet];
	uint8_t i;
	uint64_t ValidIndex, BlockIndex;

	{
		ValidIndex = 0;
		BlockIndex = 0;
		for(i = 0; i < AUDIO_CORE_SOURCE_MAX_NUM; i++)
		{
			if(AudioCore.AudioSource[i].Enable
					&& (AudioCore.AudioSource[i].Net == ScanNet)
#ifdef AUDIO_CORE_DEBUG
					&& AudioCore.AudioSource[i].DataGetFunc
					&& AudioCore.AudioSource[i].DataLenFunc
#endif
					)
			{
				SOURCE_BIT_EN(ValidIndex, i);
				if(AudioCore.AudioSource[i].Sync)
				{
					SOURCE_BIT_EN(BlockIndex, i);
				}
#ifdef CFG_FUNC_EFFECT_CORE_DUAL_EN
				if(!(SOURCE_BIT_GET(pNetCt->FrameReady, i))
						&& ((AudioCore.AudioSource[i].Adapt == STD) || (AudioCore.AudioSource[i].Adapt == CLK_ADJUST_ONLY))
						&& (AudioCore.AudioSource[i].DataLenFunc() >= SOURCEFRAME(i))
							)

				{
						SOURCE_BIT_EN(pNetCt->FrameReady, i);
				}
#endif
			}
		}
		for(i = 0; i < AUDIO_CORE_SINK_MAX_NUM; i++)
		{
			if(AudioCore.AudioSink[i].Enable
					&& (AudioCore.AudioSink[i].Net == ScanNet)
#ifdef AUDIO_CORE_DEBUG
					&& AudioCore.AudioSink[i].DataSetFunc
					&& AudioCore.AudioSink[i].SpaceLenFunc
#endif
					)
			{
				SINK_BIT_EN(ValidIndex, i);
				if(AudioCore.AudioSink[i].Sync)
				{
					SINK_BIT_EN(BlockIndex, i);
				}
#ifdef CFG_FUNC_EFFECT_CORE_DUAL_EN
			if(!(SINK_BIT_GET(pNetCt->FrameReady, i))
					&& ((AudioCore.AudioSink[i].Adapt == STD) || (AudioCore.AudioSink[i].Adapt == CLK_ADJUST_ONLY))
					&& (AudioCore.AudioSink[i].SpaceLenFunc() >= SINKFRAME(i))
						)

			{
					SINK_BIT_EN(pNetCt->FrameReady, i);
			}
#endif
			}
		}

		if((SOURCE_BIT_MASK(ValidIndex)	//Souce >= 1
			|| SINK_BIT_MASK(ValidIndex)) //或 Sink >= 1
			&& (BlockIndex)				//Block >= 1
			&& (BlockIndex & (pNetCt->FrameReady)) == BlockIndex) //FrameSync
		{
			AudioFlowSourceClear(FlowNet);

			pNetCt->FrameStart = McuCycleCountGet();
			for(i = 0; i < AUDIO_CORE_SOURCE_MAX_NUM; i++)
			{
				if(SOURCE_BIT_GET(ValidIndex, i))
				{
					SOURCE_BIT_DIS(pNetCt->FrameReady, i);
					AudioCore.AudioSource[i].Active = TRUE;
					AudioCoreSourceGet(i);
				}
			}

			for(i = 0; i < AUDIO_CORE_SINK_MAX_NUM; i++)
			{
				if(SINK_BIT_GET(ValidIndex, i))
				{
					AudioCore.AudioSink[i].Active = TRUE;
				}
			}
			return TRUE;
		}
	}

	return FALSE;
}

__attribute__((section(".ilm_section")))
bool AudioCoreSynced(uint8_t FlowNet)
{
	uint8_t i;
	AudioCoreSource *pSourceCt = NULL;
	AudioCoreSink *pSinkCt = NULL;
	NetContext *pNetCt = &AudioCore.NetCt[FlowNet];
	for(i= 0; i < AUDIO_CORE_SOURCE_MAX_NUM; i++)
	{
		pSourceCt = &AudioCore.AudioSource[i];

		if((pSourceCt->Active) && (pSourceCt->Net == FlowNet))//Enable &&
		{
			pSourceCt->Active = FALSE;
		}
	}
	for(i = 0; i < AUDIO_CORE_SINK_MAX_NUM; i++)
	{
		pSinkCt = &AudioCore.AudioSink[i];

		if(pSinkCt->Active && (pSinkCt->Net == FlowNet))//Enable
		{
#if defined(CFG_DEBUG_DAC_EMPTY_EN) && defined(CFG_RES_DAC_MONITOR_EN)
			if(i == AUDIO_DAC_SINK_NUM && ((AudioCore.AudioSink[i].SpaceLenFunc() + 1) >= AudioCore.AudioSink[i].LenFuncDepth))
			{
				DacEmptyCnt++;
			}
#endif
			AudioCoreSinkSet(i);
			AudioCore.AudioSink[i].Active = FALSE;
			SINK_BIT_DIS(pNetCt->FrameReady, i);
		}
	}

	pNetCt->FrameCycle += McuCycleCountGet() - pNetCt->FrameStart;
	pNetCt->FrameCnt++;
	if(pNetCt->FrameCnt >= 100)
	{
		pNetCt->AudioMcps = (uint16_t)((uint64_t)(pNetCt->SampleRate) * pNetCt->FrameCycle / (pNetCt->FrameCnt * pNetCt->FrameSize) /1000000);
		pNetCt->FrameCycle = 0;
		pNetCt->FrameCnt = 0;
	}
#if defined(FRAME_SYNC_TRIGER_EN)
	if(AudioCore.AudioSource[FRAME_SYNC_SOURCE_NUM].InitFlag && (AudioCore.AudioSource[FRAME_SYNC_SOURCE_NUM].Net == FlowNet))
	{
		DMA_InterruptFlagClear(FRAME_SYNC_DMA_ID, DMA_THRESHOLD_INT);
		DMA_InterruptEnable(FRAME_SYNC_DMA_ID, DMA_THRESHOLD_INT, 1);
	#if defined(CFG_LOW_DELAY_CTRL)
		AudioMicDataSysc();
	#endif
	}
#endif
	return TRUE;
}

void AudioCoreAdjustIORenew(void)
{
	int i;
	bool LastIoVaild = FALSE;
	uint8_t NewAdjustIO = 0;

	for(i = 0; i < AUDIO_CORE_SOURCE_MAX_NUM; i++)
	{
		if(AudioCore.AudioSource[i].InitFlag
			&& AudioCore.AudioSource[i].Enable
			&& (AudioCore.AudioSource[i].Adapt == CLK_ADJUST_ONLY || AudioCore.AudioSource[i].Adapt == SRC_ADJUST) \
			&& ((CLK_ADJUST_ADAPTER *)(AudioCore.AudioSource[i].AdjAdapter))->Enable
			)
		{
			NewAdjustIO  = SOURCE_GET_IO(i);
			if(NewAdjustIO == AdaptHwCt.AdjustIO)
			{
				LastIoVaild = TRUE;
			}
		}
	}
	for(i = 0; i < AUDIO_CORE_SINK_MAX_NUM; i++)
	{
		if(AudioCore.AudioSink[i].InitFlag
			&&	AudioCore.AudioSink[i].Enable
			&& (AudioCore.AudioSink[i].Adapt == CLK_ADJUST_ONLY || AudioCore.AudioSink[i].Adapt == SRC_ADJUST)\
			&& ((CLK_ADJUST_ADAPTER *)(AudioCore.AudioSink[i].AdjAdapter))->Enable
				)
		{
			NewAdjustIO  = SINK_GET_IO(i);
			if(NewAdjustIO == AdaptHwCt.AdjustIO)
			{
				LastIoVaild = TRUE;
			}
		}
	}
	if(!LastIoVaild && NewAdjustIO)
	{
		AdaptHwCt.AdjustIO = NewAdjustIO;
	}
}

/**for adj or Sra**/
void AudioCoreSourceValidDepthRenew(uint8_t Index)
{
	if(Index < AUDIO_CORE_SOURCE_MAX_NUM && AudioCoreSourceIsInit(Index))
	{
		AudioCoreSource *Source = &AudioCore.AudioSource[Index];
		SRA_ADAPTER * AdjAdapter = (SRA_ADAPTER *)Source->AdjAdapter;
		CLK_ADJUST_ADAPTER * ClkAdjAdapter = (CLK_ADJUST_ADAPTER *)Source->AdjAdapter;
		SRC_ADAPTER * SrcAdapter = Source->SrcAdapter;
		switch(Source->Adapt)
		{
			case SRA_ONLY:
				AdjAdapter->ValidDepth = Source->LenFuncDepth + AdjAdapter->SraBufHandler.BufDepth / (PCM_BYTE(Source->PcmWidth) * Source->Channels) - SOURCEFRAME(Index) - ADAPTER_INPUT_BLOCK;
				break;
#if defined(AUDIOCORE_HWSRC_EN)
			case HWSRC_SRA:
#endif
			case SRC_SRA:
				AdjAdapter->ValidDepth = ((uint64_t)AudioCore.NetCt[Source->Net].SampleRate * Source->LenFuncDepth) / SrcAdapter->SampleRate \
										+ SrcAdapter->SrcBufHandler.BufDepth / (PCM_BYTE(AudioCore.NetCt[Source->Net].PcmWidth) * Source->Channels) \
										+ AdjAdapter->SraBufHandler.BufDepth /(PCM_BYTE(AudioCore.NetCt[Source->Net].PcmWidth) * Source->Channels) \
										- SOURCEFRAME(Index) - ADAPTER_INPUT_BLOCK;
				break;

			case CLK_ADJUST_ONLY:
				ClkAdjAdapter->ValidDepth = Source->LenFuncDepth - SOURCEFRAME(Index);
				ClkAdjAdapter->TotalNum = 0;
				ClkAdjAdapter->Count = 0;
				ClkAdjAdapter->LastLevel = 0;
				ClkAdjAdapter->RiseTimes = 0;
				ClkAdjAdapter->AdjustVal = 0;
				break;
#if defined(AUDIOCORE_HWSRC_EN)
			case HWSRC_ADJUST:
#endif
			case SRC_ADJUST:
				ClkAdjAdapter->ValidDepth = ((uint64_t)AudioCore.NetCt[Source->Net].SampleRate * Source->LenFuncDepth) / SrcAdapter->SampleRate \
										+ SrcAdapter->SrcBufHandler.BufDepth / (PCM_BYTE(AudioCore.NetCt[Source->Net].PcmWidth) * Source->Channels) \
										- SOURCEFRAME(Index) - ADAPTER_INPUT_BLOCK;
				ClkAdjAdapter->TotalNum = 0;
				ClkAdjAdapter->Count = 0;
				ClkAdjAdapter->LastLevel = 0;
				ClkAdjAdapter->RiseTimes = 0;
				ClkAdjAdapter->AdjustVal = 0;
				break;
			default:
				break;
		}
	}
}

void AudioCoreSinkValidDepthRenew(uint8_t Index)
{
	if(Index < AUDIO_CORE_SINK_MAX_NUM && AudioCoreSinkIsInit(Index))
	{
		AudioCoreSink *Sink = &(AudioCore.AudioSink[Index]);
		SRA_ADAPTER * AdjAdapter = (SRA_ADAPTER *)Sink->AdjAdapter;
		CLK_ADJUST_ADAPTER * ClkAdjAdapter = (CLK_ADJUST_ADAPTER *)Sink->AdjAdapter;
		SRC_ADAPTER * SrcAdapter = Sink->SrcAdapter;
		switch(Sink->Adapt)
		{
			case SRA_ONLY:
				AdjAdapter->ValidDepth = Sink->LenFuncDepth + AdjAdapter->SraBufHandler.BufDepth / (PCM_BYTE(Sink->PcmWidth) * Sink->Channels) - SINKFRAME(Index) - ADAPTER_INPUT_BLOCK;
				break;
#if defined(AUDIOCORE_HWSRC_EN)
			case HWSRC_SRA:
#endif
			case SRC_SRA:
				AdjAdapter->ValidDepth = ((uint64_t)AudioCore.NetCt[Sink->Net].SampleRate * Sink->LenFuncDepth) / SrcAdapter->SampleRate \
										+ SrcAdapter->SrcBufHandler.BufDepth / (PCM_BYTE(AudioCore.NetCt[Sink->Net].PcmWidth) * Sink->Channels) \
										+ AdjAdapter->SraBufHandler.BufDepth /(PCM_BYTE(AudioCore.NetCt[Sink->Net].PcmWidth) * Sink->Channels) \
										- SINKFRAME(Index) - ADAPTER_INPUT_BLOCK;
				break;

			case CLK_ADJUST_ONLY:
				ClkAdjAdapter->ValidDepth = Sink->LenFuncDepth - SINKFRAME(Index);
				ClkAdjAdapter->TotalNum = 0;
				ClkAdjAdapter->Count = 0;
				ClkAdjAdapter->LastLevel = 0;
				ClkAdjAdapter->RiseTimes = 0;
				ClkAdjAdapter->AdjustVal = 0;
				break;
#if defined(AUDIOCORE_HWSRC_EN)
			case HWSRC_ADJUST:
#endif
			case SRC_ADJUST:
				ClkAdjAdapter->ValidDepth = (uint32_t)((uint64_t)AudioCore.NetCt[Sink->Net].SampleRate * Sink->LenFuncDepth) / SrcAdapter->SampleRate \
										+ SrcAdapter->SrcBufHandler.BufDepth / (PCM_BYTE(AudioCore.NetCt[Sink->Net].PcmWidth) * Sink->Channels) \
										- SINKFRAME(Index) - ADAPTER_INPUT_BLOCK;
				ClkAdjAdapter->TotalNum = 0;
				ClkAdjAdapter->Count = 0;
				ClkAdjAdapter->LastLevel = 0;
				ClkAdjAdapter->RiseTimes = 0;
				ClkAdjAdapter->AdjustVal = 0;
				break;
			default:
				break;
		}
	}
}

void AudioCoreSourceChange(uint8_t Index, uint8_t Channels, uint32_t SampleRate)
{
	SRC_ADAPTER		*SrcAdapter = AudioCore.AudioSource[Index].SrcAdapter;
	bool Changed = FALSE;
	if(!AudioCoreSourceIsInit(Index) || !Channels || !SampleRate)
	{
		return;
	}
	if((SrcAdapter != NULL) && (SrcAdapter->SampleRate != SampleRate))
	{
		SrcAdapter->SampleRate = SampleRate;
		Changed = TRUE;
		SOURCE_BIT_DIS(SOURCEREADY(Index), Index);
	}
	if(AudioCore.AudioSource[Index].Channels != Channels)
	{
		AudioCore.AudioSource[Index].Channels = Channels;
		Changed = TRUE;
		SOURCE_BIT_DIS(SOURCEREADY(Index), Index);
	}

	if(Changed)
	{
		AudioCoreSourceAdapterReset(Index);
	}
}

void AudioCoreSourceAdapterReset(uint8_t Index)
{
	AudioCoreSource *Source = &AudioCore.AudioSource[Index];
	AUDIO_ADAPT	Adapt = Source->Adapt;
	SRA_ADAPTER * AdjAdapter = (SRA_ADAPTER *)Source->AdjAdapter;
	CLK_ADJUST_ADAPTER * ClkAdjAdapter = (CLK_ADJUST_ADAPTER *)Source->AdjAdapter;
	SRC_ADAPTER	*SrcAdapter = Source->SrcAdapter;
	if(!AudioCoreSourceIsInit(Index))
	{
		return;
	}
	switch(Adapt)
	{
		case SRA_ONLY:
			AdjAdapter->SraBufHandler.R = 0;
			AdjAdapter->SraBufHandler.W = 0;
			resampler_farrow_init(&AdjAdapter->SraResFarCt,AudioCore.AudioSource[Index].Channels, POLYNOMIAL_ORDER);
			AdjAdapter->ValidDepth = Source->LenFuncDepth + AdjAdapter->SraBufHandler.BufDepth / (PCM_BYTE(Source->PcmWidth) * Source->Channels) - SOURCEFRAME(Index) - ADAPTER_INPUT_BLOCK;
			break;

		case SRC_SRA://no break,for SRCcode
			AdjAdapter->SraBufHandler.R = 0;
			AdjAdapter->SraBufHandler.W = 0;
			resampler_farrow_init(&AdjAdapter->SraResFarCt,AudioCore.AudioSource[Index].Channels, POLYNOMIAL_ORDER);
			AdjAdapter->ValidDepth = ((uint64_t)AudioCore.NetCt[Source->Net].SampleRate * Source->LenFuncDepth) / SrcAdapter->SampleRate \
									+ SrcAdapter->SrcBufHandler.BufDepth / (PCM_BYTE(AudioCore.NetCt[Source->Net].PcmWidth) * Source->Channels) \
									+ AdjAdapter->SraBufHandler.BufDepth /(PCM_BYTE(AudioCore.NetCt[Source->Net].PcmWidth) * Source->Channels) \
									- SOURCEFRAME(Index) - ADAPTER_INPUT_BLOCK;
		case SRC_ADJUST://no break,for SRCcode
			if(Adapt == SRC_ADJUST)
			{
				ClkAdjAdapter->TotalNum = 0;
				ClkAdjAdapter->Count = 0;
				ClkAdjAdapter->LastLevel = 0;
				ClkAdjAdapter->RiseTimes = 0;
				ClkAdjAdapter->AdjustVal = 0;
				ClkAdjAdapter->ValidDepth = ((uint64_t)AudioCore.NetCt[Source->Net].SampleRate * Source->LenFuncDepth) / SrcAdapter->SampleRate \
										+ SrcAdapter->SrcBufHandler.BufDepth / (PCM_BYTE(AudioCore.NetCt[Source->Net].PcmWidth) * Source->Channels) \
										- SOURCEFRAME(Index) - ADAPTER_INPUT_BLOCK;
			}
		case SRC_ONLY:
			if(SrcAdapter->SampleRate != AudioCore.NetCt[AudioCore.AudioSource[Index].Net].SampleRate)
			{
	#if defined(CFG_FUNC_SRC_SINC_EN)
				uint32_t SincCtSize;
				uint8_t *NewSincCt, *OldSincCt;
				if(resampler_sinc_estimate_memory_usage(AudioCore.AudioSource[Index].Channels, SrcAdapter->SampleRate, AudioCore.NetCt[AudioCore.AudioSource[Index].Net].SampleRate, SrcAdapter->SrcQuality, &SincCtSize) != RESAMPLER_SINC_ERROR_SUCCESS)
				{
					DBG("SincChange Err\n");
					return ;
				}
				/* Allocate and init new context BEFORE freeing old one */
				NewSincCt = osPortMalloc(SincCtSize);
				if(NewSincCt == NULL)
				{
					DBG("SincCtChange Err\n");
					return ;
				}
				resampler_sinc_init(NewSincCt, AudioCore.AudioSource[Index].Channels, SrcAdapter->SampleRate, AudioCore.NetCt[AudioCore.AudioSource[Index].Net].SampleRate, SrcAdapter->SrcQuality);
				/* Atomically swap old and new SincCt to avoid race with audio processing */
				vPortEnterCritical();
				OldSincCt = SrcAdapter->SincCt;
				SrcAdapter->SincCt = NewSincCt;
				vPortExitCritical();
				/* Free old context after swap */
				osPortFree(OldSincCt);
	#elif defined(CFG_FUNC_SRC_POLYPHASE_EN)
				resampler_polyphase_init(&SrcAdapter->SrcCt, AudioCore.AudioSource[Index].Channels, GetRatioEnum(1000 * AudioCore.NetCt[AudioCore.AudioSource[Index].Net].SampleRate / SrcAdapter->SampleRate));
	#endif
			}
			SrcAdapter->SrcBufHandler.R = 0;
			SrcAdapter->SrcBufHandler.W = 0;
			break;

#if defined(AUDIOCORE_HWSRC_EN)
		case HWSRC_SRA://no break,for SRCcode
			AdjAdapter->SraBufHandler.R = 0;
			AdjAdapter->SraBufHandler.W = 0;
			resampler_farrow_init(&AdjAdapter->SraResFarCt,AudioCore.AudioSource[Index].Channels, POLYNOMIAL_ORDER);
			AdjAdapter->ValidDepth = ((uint64_t)AudioCore.NetCt[Source->Net].SampleRate * Source->LenFuncDepth) / SrcAdapter->SampleRate \
									+ SrcAdapter->SrcBufHandler.BufDepth / (PCM_BYTE(AudioCore.NetCt[Source->Net].PcmWidth) * Source->Channels) \
									+ AdjAdapter->SraBufHandler.BufDepth /(PCM_BYTE(AudioCore.NetCt[Source->Net].PcmWidth) * Source->Channels) \
									- SOURCEFRAME(Index) - ADAPTER_INPUT_BLOCK;
		case HWSRC_ADJUST://no break,for SRCcode
			if(Adapt == HWSRC_ADJUST)
			{
				ClkAdjAdapter->TotalNum = 0;
				ClkAdjAdapter->Count = 0;
				ClkAdjAdapter->LastLevel = 0;
				ClkAdjAdapter->RiseTimes = 0;
				ClkAdjAdapter->AdjustVal = 0;
				ClkAdjAdapter->ValidDepth = ((uint64_t)AudioCore.NetCt[Source->Net].SampleRate * Source->LenFuncDepth) / SrcAdapter->SampleRate \
										+ SrcAdapter->SrcBufHandler.BufDepth / (PCM_BYTE(AudioCore.NetCt[Source->Net].PcmWidth) * Source->Channels) \
										- SOURCEFRAME(Index) - ADAPTER_INPUT_BLOCK;
			}
		case HWSRC_ONLY:
			if(SrcAdapter->SampleRate != AudioCore.NetCt[AudioCore.AudioSource[Index].Net].SampleRate \
					&& SrcAdapter->HwSrcIndex  < RES_SRC_NUM \
					&& AdaptHwCt.HwSrcCt[SrcAdapter->HwSrcIndex].SrcCase == SOURCE_GET_IO(Index))
			{

				SRC_Context NewHwSrc;
				NewHwSrc.InBufLen = (uint64_t)SrcAdapter->SampleRate * HWSRC_BUF_TIME / 1000000;
				NewHwSrc.OutBufLen = (uint64_t)(AudioCore.NetCt[AudioCore.AudioSource[Index].Net].SampleRate) * HWSRC_BUF_TIME * 11 / 10000000;
				NewHwSrc.InBuf = osPortMalloc(NewHwSrc.InBufLen * PCM_BYTE(Source->PcmWidth) * Source->Channels);
				NewHwSrc.OutBuf = osPortMalloc(NewHwSrc.OutBufLen * PCM_BYTE(Source->PcmWidth) * Source->Channels);
				if(NewHwSrc.InBuf == NULL || NewHwSrc.OutBuf== NULL)
				{
					osPortFree(NewHwSrc.InBuf);
					osPortFree(NewHwSrc.OutBuf);
					DBG("HwSrcBuf Error!\n");
					return;
				}
				NewHwSrc.Mode = MODE_BLOCK;
				NewHwSrc.NumChannels = Source->Channels;
				NewHwSrc.BlockSamples = NewHwSrc.InBufLen / 2;
				if(Source->PcmWidth == WIDTH_L24BIT)
					NewHwSrc.Width = SRC_24BIT;
				else
					NewHwSrc.Width = SRC_16BIT;
				vPortEnterCritical();
				osPortFree(AdaptHwCt.HwSrcCt[SrcAdapter->HwSrcIndex].SrcContext.InBuf);
				osPortFree(AdaptHwCt.HwSrcCt[SrcAdapter->HwSrcIndex].SrcContext.OutBuf);
				memcpy(&AdaptHwCt.HwSrcCt[SrcAdapter->HwSrcIndex].SrcContext, &NewHwSrc, sizeof(SRC_Context));
				Reset_Function2Reset((SRC0_FUNC_SEPA) << (SrcAdapter->HwSrcIndex));
				Reset_Register2Reset(SRC0_REG_SEPA << (SrcAdapter->HwSrcIndex));
				SRC_BlockInit(SrcAdapter->HwSrcIndex, &AdaptHwCt.HwSrcCt[SrcAdapter->HwSrcIndex].SrcContext, SrcAdapter->SampleRate, AudioCore.NetCt[AudioCore.AudioSource[Index].Net].SampleRate);//设置一个初始的采样率转换值
				AdaptHwCt.HwSrcCt[SrcAdapter->HwSrcIndex].SampleLenStartIn = 0;
				vPortExitCritical();
			}
			SrcAdapter->SrcBufHandler.R = 0;
			SrcAdapter->SrcBufHandler.W = 0;
			break;
#endif//#if defined(AUDIOCORE_HWSRC_EN)
		case CLK_ADJUST_ONLY:
			ClkAdjAdapter->TotalNum = 0;
			ClkAdjAdapter->Count = 0;
			ClkAdjAdapter->LastLevel = 0;
			ClkAdjAdapter->RiseTimes = 0;
			ClkAdjAdapter->AdjustVal = 0;
			ClkAdjAdapter->ValidDepth = Source->LenFuncDepth - SOURCEFRAME(Index);
			break;

		default:
			break;
	}
}

void AudioCoreSinkChange(uint8_t Index, uint8_t Channels, uint32_t SampleRate)
{
	SRC_ADAPTER		*SrcAdapter = AudioCore.AudioSink[Index].SrcAdapter;
	bool Changed = FALSE;
	if(!AudioCoreSinkIsInit(Index) || !Channels || !SampleRate)
	{
		return;
	}

	if((SrcAdapter != NULL) && (SrcAdapter->SampleRate != SampleRate))
	{
		SrcAdapter->SampleRate = SampleRate;
		Changed = TRUE;
		SINK_BIT_DIS(SINKREADY(Index), Index);
	}
	if(AudioCore.AudioSink[Index].Channels != Channels)
	{
		AudioCore.AudioSink[Index].Channels = Channels;
		Changed = TRUE;
		SINK_BIT_DIS(SINKREADY(Index), Index);
	}

	if(Changed)
	{
		AudioCoreSinkAdapterReset(Index);
	}
}

void AudioCoreSinkAdapterReset(uint8_t Index)
{
	AudioCoreSink *Sink = &(AudioCore.AudioSink[Index]);
	AUDIO_ADAPT	Adapt = Sink->Adapt;
	SRA_ADAPTER * AdjAdapter = (SRA_ADAPTER *)Sink->AdjAdapter;
	CLK_ADJUST_ADAPTER * ClkAdjAdapter = (CLK_ADJUST_ADAPTER *)Sink->AdjAdapter;
	SRC_ADAPTER	*SrcAdapter = Sink->SrcAdapter;
	if(!AudioCoreSinkIsInit(Index))
	{
		return;
	}
	switch(Adapt)
	{
		case SRA_ONLY:
			AdjAdapter->SraBufHandler.R = 0;
			AdjAdapter->SraBufHandler.W = 0;
			resampler_farrow_init(&AdjAdapter->SraResFarCt,AudioCore.AudioSink[Index].Channels, POLYNOMIAL_ORDER);
			AdjAdapter->ValidDepth = Sink->LenFuncDepth + AdjAdapter->SraBufHandler.BufDepth / (PCM_BYTE(Sink->PcmWidth) * Sink->Channels) - SINKFRAME(Index) - ADAPTER_INPUT_BLOCK;
			break;

		case SRC_SRA:
			AdjAdapter->SraBufHandler.R = 0;
			AdjAdapter->SraBufHandler.W = 0;
			resampler_farrow_init(&AdjAdapter->SraResFarCt,AudioCore.AudioSink[Index].Channels, POLYNOMIAL_ORDER);
			AdjAdapter->ValidDepth = ((uint64_t)AudioCore.NetCt[Sink->Net].SampleRate * Sink->LenFuncDepth) / SrcAdapter->SampleRate \
									+ SrcAdapter->SrcBufHandler.BufDepth / (PCM_BYTE(AudioCore.NetCt[Sink->Net].PcmWidth) * Sink->Channels) \
									+ AdjAdapter->SraBufHandler.BufDepth /(PCM_BYTE(AudioCore.NetCt[Sink->Net].PcmWidth) * Sink->Channels) \
									- SINKFRAME(Index) - ADAPTER_INPUT_BLOCK;
		case SRC_ONLY:
			if(SrcAdapter->SampleRate != AudioCore.NetCt[AudioCore.AudioSink[Index].Net].SampleRate)
			{
	#if defined(CFG_FUNC_SRC_SINC_EN)
				uint32_t SincCtSize;
				uint8_t *NewSincCt, *OldSincCt;
				if(resampler_sinc_estimate_memory_usage(AudioCore.AudioSink[Index].Channels, AudioCore.NetCt[AudioCore.AudioSink[Index].Net].SampleRate, SrcAdapter->SampleRate, SrcAdapter->SrcQuality, &SincCtSize) != RESAMPLER_SINC_ERROR_SUCCESS)
				{
					DBG("SincChange Err\n");
					return ;
				}
				/* Allocate and init new context BEFORE freeing old one */
				NewSincCt = osPortMalloc(SincCtSize);
				if(NewSincCt == NULL)
				{
					DBG("SincCtChange Err\n");
					return ;
				}
				resampler_sinc_init(NewSincCt, AudioCore.AudioSink[Index].Channels, AudioCore.NetCt[AudioCore.AudioSink[Index].Net].SampleRate, SrcAdapter->SampleRate, SrcAdapter->SrcQuality);
				/* Atomically swap old and new SincCt to avoid race with audio processing */
				vPortEnterCritical();
				OldSincCt = SrcAdapter->SincCt;
				SrcAdapter->SincCt = NewSincCt;
				vPortExitCritical();
				/* Free old context after swap */
				osPortFree(OldSincCt);
	#elif defined(CFG_FUNC_SRC_POLYPHASE_EN)
				resampler_polyphase_init(&SrcAdapter->SrcCt, AudioCore.AudioSink[Index].Channels, GetRatioEnum(1000 * SrcAdapter->SampleRate / AudioCore.NetCt[AudioCore.AudioSink[Index].Net].SampleRate));
	#endif
			}
			SrcAdapter->SrcBufHandler.R = 0;
			SrcAdapter->SrcBufHandler.W = 0;
			break;

		case CLK_ADJUST_ONLY:
			ClkAdjAdapter->TotalNum = 0;
			ClkAdjAdapter->Count = 0;
			ClkAdjAdapter->LastLevel = 0;
			ClkAdjAdapter->RiseTimes = 0;
			ClkAdjAdapter->AdjustVal = 0;
			ClkAdjAdapter->ValidDepth = Sink->LenFuncDepth - SINKFRAME(Index);
			break;

		case SRC_ADJUST:
			if(SrcAdapter->SampleRate != AudioCore.NetCt[AudioCore.AudioSink[Index].Net].SampleRate)
			{
	#if defined(CFG_FUNC_SRC_SINC_EN)
				uint32_t SincCtSize;
				uint8_t *NewSincCt, *OldSincCt;
				if(resampler_sinc_estimate_memory_usage(AudioCore.AudioSink[Index].Channels, AudioCore.NetCt[AudioCore.AudioSink[Index].Net].SampleRate, SrcAdapter->SampleRate, SrcAdapter->SrcQuality, &SincCtSize) != RESAMPLER_SINC_ERROR_SUCCESS)
				{
					DBG("SincChange Err\n");
					return ;
				}
				/* Allocate and init new context BEFORE freeing old one */
				NewSincCt = osPortMalloc(SincCtSize);
				if(NewSincCt == NULL)
				{
					DBG("SincCtChange Err\n");
					return ;
				}
				resampler_sinc_init(NewSincCt, AudioCore.AudioSink[Index].Channels, AudioCore.NetCt[AudioCore.AudioSink[Index].Net].SampleRate, SrcAdapter->SampleRate, SrcAdapter->SrcQuality);
				/* Atomically swap old and new SincCt to avoid race with audio processing */
				vPortEnterCritical();
				OldSincCt = SrcAdapter->SincCt;
				SrcAdapter->SincCt = NewSincCt;
				vPortExitCritical();
				/* Free old context after swap */
				osPortFree(OldSincCt);
	#elif defined(CFG_FUNC_SRC_POLYPHASE_EN)
				resampler_polyphase_init(&SrcAdapter->SrcCt, AudioCore.AudioSink[Index].Channels, GetRatioEnum(1000 * SrcAdapter->SampleRate / AudioCore.NetCt[AudioCore.AudioSink[Index].Net].SampleRate));
	#endif
			}
			SrcAdapter->SrcBufHandler.R = 0;
			SrcAdapter->SrcBufHandler.W = 0;

			ClkAdjAdapter->TotalNum = 0;
			ClkAdjAdapter->Count = 0;
			ClkAdjAdapter->LastLevel = 0;
			ClkAdjAdapter->RiseTimes = 0;
			ClkAdjAdapter->AdjustVal = 0;
			ClkAdjAdapter->ValidDepth = (uint32_t)((uint64_t)AudioCore.NetCt[Sink->Net].SampleRate * Sink->LenFuncDepth) / SrcAdapter->SampleRate \
									+ SrcAdapter->SrcBufHandler.BufDepth / (PCM_BYTE(AudioCore.NetCt[Sink->Net].PcmWidth) * Sink->Channels) \
									- SINKFRAME(Index) - ADAPTER_INPUT_BLOCK;
			break;
		default:
			break;
	}
}

void AudioCoreSourceDepthChange(uint8_t Index, uint32_t NewDepth)
{
	AudioCoreSource * Source = &AudioCore.AudioSource[Index];
	if(!AudioCoreSourceIsInit(Index))
	{
		return;
	}
	Source->LenFuncDepth = NewDepth;
	AudioCoreSourceValidDepthRenew(Index);
}

void AudioCoreSinkDepthChange(uint8_t Index, uint32_t NewDepth)
{
	AudioCoreSink * Sink = &AudioCore.AudioSink[Index];
	if(!AudioCoreSinkIsInit(Index))
	{
		return;
	}
	Sink->LenFuncDepth = NewDepth;
	AudioCoreSinkValidDepthRenew(Index);
}

bool AudioCoreSourceIsInit(uint8_t Index)
{
	if(Index < AUDIO_CORE_SOURCE_MAX_NUM)
	{
		return AudioCore.AudioSource[Index].InitFlag;
	}
	return FALSE;
}

bool AudioCoreSinkIsInit(uint8_t Index)
{
	if(Index < AUDIO_CORE_SINK_MAX_NUM)
	{
		return AudioCore.AudioSink[Index].InitFlag;
	}
	return FALSE;
}

void AudioCoreSourceAdjust(uint8_t Index, bool OnOff)
{
	AudioCoreSource * Source = &AudioCore.AudioSource[Index];
	if(!AudioCoreSourceIsInit(Index) || Source->AdjAdapter == NULL)
	{
		return;
	}
	switch(Source->Adapt)
	{
		case SRA_ONLY:
		case SRC_SRA:
#if defined(AUDIOCORE_HWSRC_EN)
		case HWSRC_SRA:
#endif
			((SRA_ADAPTER *)(Source->AdjAdapter))->Enable = OnOff;
			break;

		case CLK_ADJUST_ONLY:
		case SRC_ADJUST:
#if defined(AUDIOCORE_HWSRC_EN)
		case HWSRC_ADJUST:
#endif
			((CLK_ADJUST_ADAPTER *)(Source->AdjAdapter))->Enable = OnOff;
			if(!OnOff)
			{
				Clock_AudioPllClockAdjust(PLL_CLK_1, 0, 0);
				Clock_AudioPllClockAdjust(PLL_CLK_2, 0, 0);
				Clock_AudioPllClockAdjust(PLL_CLK_3, 0, 0);
				Clock_AudioPllClockAdjust(PLL_CLK_4, 0, 0);
			}
			break;

		default:
			break;
	}
	AudioCoreAdjustIORenew();
}

void AudioCoreSinkAdjust(uint8_t Index, bool OnOff)
{
	AudioCoreSink * Sink = &AudioCore.AudioSink[Index];
	if(!AudioCoreSinkIsInit(Index) || Sink->AdjAdapter == NULL)
	{
		return;
	}
	switch(Sink->Adapt)
	{
		case SRA_ONLY:
		case SRC_SRA:
#if defined(AUDIOCORE_HWSRC_EN)
		case HWSRC_SRA:
#endif
			((SRA_ADAPTER *)(Sink->AdjAdapter))->Enable = OnOff;
			break;

		case CLK_ADJUST_ONLY:
		case SRC_ADJUST:
#if defined(AUDIOCORE_HWSRC_EN)
		case HWSRC_ADJUST:
#endif
			((CLK_ADJUST_ADAPTER *)(Sink->AdjAdapter))->Enable = OnOff;
			break;

		default:
			break;
	}
	AudioCoreAdjustIORenew();
}

bool AudioCoreSinkAdjustIsEnable(uint8_t Index)
{
	AudioCoreSink * Sink = &AudioCore.AudioSink[Index];
	bool Ret = FALSE;
	if(!AudioCoreSinkIsInit(Index) || Sink->AdjAdapter == NULL)
	{
		return FALSE;
	}
	switch(Sink->Adapt)
	{
		case SRA_ONLY:
		case SRC_SRA:
#if defined(AUDIOCORE_HWSRC_EN)
		case HWSRC_SRA:
#endif
			Ret = ((SRA_ADAPTER *)(Sink->AdjAdapter))->Enable;
			break;

		case CLK_ADJUST_ONLY:
		case SRC_ADJUST:
#if defined(AUDIOCORE_HWSRC_EN)
		case HWSRC_ADJUST:
#endif
			Ret = ((CLK_ADJUST_ADAPTER *)(Sink->AdjAdapter))->Enable;
			break;
		default:
			break;
	}
	return Ret;
}

//void AudioCoreMixSampleRateSet(uint8_t MixNet, uint32_t SampleRate)
void AudioCoreMixParamSet(uint8_t MixNet, uint32_t SampleRate, uint16_t FrameSize, uint8_t Width)
{
	uint8_t i;
	bool Changed = FALSE;
	if(AudioCore.NetCt[MixNet].SampleRate != SampleRate)
	{
		AudioCore.NetCt[MixNet].SampleRate = SampleRate;
		Changed = TRUE;
	}
	if(AudioCore.NetCt[MixNet].FrameSize != FrameSize)
	{
		AudioCore.NetCt[MixNet].FrameSize = FrameSize;
		Changed = TRUE;
		AudioCore.NetCt[MixNet].FrameReady = 0;
	}
	if(AudioCore.NetCt[MixNet].PcmWidth != Width)
	{
		AudioCore.NetCt[MixNet].PcmWidth = Width;
		Changed = TRUE;
	}

	if(Changed)
	{
		AudioCore.NetCt[MixNet].FrameUs = 1000000 * AudioCore.NetCt[MixNet].FrameSize / AudioCore.NetCt[MixNet].SampleRate;
		for(i= 0; i < AUDIO_CORE_SOURCE_MAX_NUM; i++)
		{
			if(AudioCore.AudioSource[i].SrcAdapter && AudioCore.AudioSource[i].Net == MixNet)
			{
#if !defined(AUDIOCORE_HWSRC_EN)
	#if defined(CFG_FUNC_SRC_SINC_EN)
				uint32_t SincCtSize;
				uint8_t *NewSincCt, *OldSincCt;
				if(resampler_sinc_estimate_memory_usage(AudioCore.AudioSource[i].Channels, AudioCore.AudioSource[i].SrcAdapter->SampleRate, AudioCore.NetCt[MixNet].SampleRate, AudioCore.AudioSource[i].SrcAdapter->SrcQuality, &SincCtSize) != RESAMPLER_SINC_ERROR_SUCCESS)
				{
					DBG("SincChange Err\n");
					 return ;
				}
				/* Allocate and init new context BEFORE freeing old one */
				NewSincCt = osPortMalloc(SincCtSize);
				if(NewSincCt == NULL)
				{
					DBG("SincCtChange Err\n");
					 return ;
				}
				resampler_sinc_init(NewSincCt, AudioCore.AudioSource[i].Channels, AudioCore.AudioSource[i].SrcAdapter->SampleRate, AudioCore.NetCt[MixNet].SampleRate, AudioCore.AudioSource[i].SrcAdapter->SrcQuality);
				vPortEnterCritical();
				OldSincCt = AudioCore.AudioSource[i].SrcAdapter->SincCt;
				AudioCore.AudioSource[i].SrcAdapter->SincCt = NewSincCt;
				vPortExitCritical();
				osPortFree(OldSincCt);
	#elif defined(CFG_FUNC_SRC_POLYPHASE_EN)
				resampler_polyphase_init(&(AudioCore.AudioSource[i].SrcAdapter->SrcCt), AudioCore.AudioSource[i].Channels, GetRatioEnum(1000 * SampleRate / AudioCore.AudioSource[i].SrcAdapter->SampleRate));
	#endif

#else	//简化process
				AudioCoreSourceAdapterReset(i);
#endif
			}
			AudioCoreSourceValidDepthRenew(i);
		}
		for(i = 0; i < AUDIO_CORE_SINK_MAX_NUM; i++)
		{
			if(AudioCore.AudioSink[i].SrcAdapter && AudioCore.AudioSink[i].Net == MixNet)
			{
				AudioCoreSinkAdapterReset(i);
#if !defined(AUDIOCORE_HWSRC_EN)	//简化process
			#if defined(CFG_FUNC_SRC_SINC_EN)
			uint32_t SincCtSize;
			uint8_t *NewSincCt, *OldSincCt;
			if(resampler_sinc_estimate_memory_usage(AudioCore.AudioSink[i].Channels, AudioCore.NetCt[MixNet].SampleRate, AudioCore.AudioSink[i].SrcAdapter->SampleRate, AudioCore.AudioSink[i].SrcAdapter->SrcQuality, &SincCtSize) != RESAMPLER_SINC_ERROR_SUCCESS)
		{
			DBG("SincChange Err\n");
			 return ;
			}
			/* Allocate and init new context BEFORE freeing old one */
			NewSincCt = osPortMalloc(SincCtSize);
			if(NewSincCt == NULL)
		{
			DBG("SincCtChange Err\n");
			 return ;
			}
				resampler_sinc_init(NewSincCt, AudioCore.AudioSink[i].Channels, AudioCore.NetCt[MixNet].SampleRate, AudioCore.AudioSink[i].SrcAdapter->SampleRate, AudioCore.AudioSink[i].SrcAdapter->SrcQuality);
				vPortEnterCritical();
				OldSincCt = AudioCore.AudioSink[i].SrcAdapter->SincCt;
				AudioCore.AudioSink[i].SrcAdapter->SincCt = NewSincCt;
				vPortExitCritical();
				osPortFree(OldSincCt);
#elif defined(CFG_FUNC_SRC_POLYPHASE_EN)
				resampler_polyphase_init(&(AudioCore.AudioSink[i].SrcAdapter->SrcCt), AudioCore.AudioSink[i].Channels, GetRatioEnum(1000 * AudioCore.AudioSink[i].SrcAdapter->SampleRate / SampleRate));
#endif
#endif //#if !defined(AUDIOCORE_HWSRC_EN)
			}
			AudioCoreSinkValidDepthRenew(i);
		}
	}
}

uint16_t AudioCoreFrameSizeGet(uint8_t MixNet)
{
	return AudioCore.NetCt[MixNet].FrameSize;
}

uint32_t AudioCoreMixStepUsGet(uint8_t MixNet)
{
	return AudioCore.NetCt[MixNet].FrameUs;
}

uint32_t AudioCoreMixSampleRateGet(uint8_t MixNet)
{
	return AudioCore.NetCt[MixNet].SampleRate;
}

uint8_t AudioCoreSourceMixNetGet(uint8_t Index)
{
	if(Index < AUDIO_CORE_SOURCE_MAX_NUM)
	{
		return AudioCore.AudioSource[Index].Net;
	}
	return DefaultNet;
}

uint8_t AudioCoreSinkMixNetGet(uint8_t Index)
{
	if(Index < AUDIO_CORE_SINK_MAX_NUM)
	{
		return AudioCore.AudioSink[Index].Net;
	}
	return DefaultNet;
}

uint8_t AudioCoreSourceBitWidthGet(uint8_t Index)
{
	return AudioCore.AudioSource[Index].PcmWidth;
}

uint8_t AudioCoreSinkBitWidthGet(uint8_t Index)
{
	return AudioCore.AudioSink[Index].PcmWidth;
}

uint16_t AudioCoreSourceFrameSizeGet(uint8_t Index)
{
	return AudioCore.NetCt[AudioCore.AudioSource[Index].Net].FrameSize;
}

uint16_t AudioCoreSinkFrameSizeGet(uint8_t Index)
{
	return AudioCore.NetCt[AudioCore.AudioSink[Index].Net].FrameSize;
}

uint32_t AudioCoreSourceMixSampleRateGet(uint8_t Index)
{
	return AudioCore.NetCt[AudioCore.AudioSource[Index].Net].SampleRate;
}

uint32_t AudioCoreSinkMixSampleRateGet(uint8_t Index)
{
	return AudioCore.NetCt[AudioCore.AudioSink[Index].Net].SampleRate;
}


void AudioCoreSinkAudioPreset(uint8_t Index)
{
	if(Index < AUDIO_CORE_SINK_MAX_NUM)
	{
		AudioCore.AudioSink[Index].PresetFlag = TRUE;
	}
}

bool MixSupport(uint32_t SampleRate, uint16_t FrameSize, uint8_t Width)
{
	int i;
	//16BIT图默认可以,L24BIT需宏允许
	if((FrameSize > CFG_MAX_FRAME) || (CFG_MAX_FRAME < CFG_MIN_FRAME) ||((Width != WIDTH_16BIT) && (Width != CFG_DEFAULT_WIDTH)))
		return FALSE;

	for(i = 0; i < SAMPLERATE_TBL_CNT; i++)
	{
		if(SampleRate == AudioCoreSampleRateTBL[i])
		{
			return TRUE;
		}
	}
	return FALSE;
}

//tws used
void AudioFadein(int16_t* pcm_in, uint16_t pcm_length, uint16_t ch)
{
	int16_t* temp = (int16_t *)pcm_in;
	int32_t n = 0, w = 0, step = 32768/pcm_length;

	if(ch == 2)
	{
		for(n = 0;	n < pcm_length; n++)
		{
			temp[n * 2] = (int16_t)(((int32_t)temp[n * 2] * w + 16384) >> 15);
			temp[n * 2 + 1] = (int16_t)(((int32_t)temp[n * 2 + 1] * w + 16384) >> 15);
			w += step;
		}
	}
	else if(ch == 1)
	{
		for(n = 0;	n < pcm_length; n++)
		{
			temp[n] = (int16_t)(((int32_t)temp[n] * w + 16384) >> 15);
			w += step;
		}
	}
}


void AudioFadeout(int16_t* pcm_in, uint16_t pcm_length, uint16_t ch)
{
	int16_t* temp = (int16_t *)pcm_in;
	int32_t n = 0, w = (32768/pcm_length)*(pcm_length-1), step = 32768/pcm_length;

	if(ch == 2)
	{
		for(n = 0; n < pcm_length; n++)
		{
			temp[n * 2] = (int16_t)(((int32_t)temp[n * 2] * w + 16384) >> 15);
			temp[n * 2 + 1] = (int16_t)(((int32_t)temp[n * 2 + 1] * w + 16384) >> 15);
			w -= step;
		}
	}
	else if(ch == 1)
	{
		for(n = 0; n < pcm_length; n++)
		{
			temp[n] = (int16_t)(((int32_t)temp[n] * w + 16384) >> 15);
			w -= step;
		}
	}
}

#if defined(HWSRC_MODE_LIST_SCAN_EN) && defined(AUDIOCORE_HWSRC_EN)
void HwSrcAppModePreSet(uint8_t AppMode)
{
	int SrcIndex, Item;
	for(SrcIndex = 0; SrcIndex < RES_SRC_NUM; SrcIndex++)
	{
		for(Item = 0; Item < SRC_MODE_LIST_MAX; Item++)
		{
			if(AppMode == HwSrcModeList[SrcIndex][Item][0] && HwSrcModeList[SrcIndex][Item][1] > HWSRC_IDLE && HwSrcModeList[SrcIndex][Item][1] < HWSRC_MODE_MAX)
			{
				AdaptHwCt.HwSrcCt[SrcIndex].SrcCase = HwSrcModeList[SrcIndex][Item][1];
			}
		}
	}
}
#endif

#if defined(HWSRC_MODE_CONFIG_EN) && defined(AUDIOCORE_HWSRC_EN)
void HwSrcAppModeClear(uint8_t AppMode)
{
	int SrcIndex;
	for(SrcIndex = 0; SrcIndex < RES_SRC_NUM; SrcIndex++)
	{
		if(AdaptHwCt.HwSrcCt[SrcIndex].SrcCase < HWSRC_MODE_MAX )
		{
			AdaptHwCt.HwSrcCt[SrcIndex].SrcCase = HWSRC_IDLE;
		}
	}
}

uint8_t HwSrcIdleGet(uint8_t AudioNum, bool IsSource)
{
	int SrcIndex;
	for(SrcIndex = 0; SrcIndex < RES_SRC_NUM; SrcIndex++)
	{
		if(AdaptHwCt.HwSrcCt[SrcIndex].SrcCase == HWSRC_IDLE)
		{
			if(IsSource)
				AdaptHwCt.HwSrcCt[SrcIndex].SrcCase = SOURCE_GET_IO(AudioNum);
			else
				AdaptHwCt.HwSrcCt[SrcIndex].SrcCase = IO_GET_SINK(AudioNum);
			AdaptHwCt.HwSrcCt[SrcIndex].SampleLenStartIn = 0;
			return SrcIndex;
		}
	}
	return HWSRC_IDLE;
}
#endif

uint16_t FrameMcps_Communication(void)
{
#if defined(ROBOEFFECT_DOUBLE_ENGINE_EN) && defined(CFG_FUNC_EFFECT_CORE_DUAL_EN)
	return AudioCore.NetCt[CFG_PARA_CORE2_NET].AudioMcps;
#else
	return AudioCore.NetCt[DefaultNet].AudioMcps;
#endif
}

#if defined(CFG_FUNC_DEBUG_EN) || defined(CFG_FUNC_USBDEBUG_EN)
void FrameMcpsLog(void)
{
	DBG("Frame:%d M ", AudioCore.NetCt[DefaultNet].AudioMcps);
	#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
		DBG("+ %d M ",  AudioCore.NetCt[SeparateNet].AudioMcps);
	#endif
}
#endif
