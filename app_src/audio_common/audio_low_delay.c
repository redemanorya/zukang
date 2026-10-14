/**
 **************************************************************************************
 * @file    audio low delay.c
 * @brief
 *
 * @author	Sam
 * @version V1.0.0
 *
 * @Copyright (C) 2026, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include "mvintrinsics.h"
#include "irqn.h"
#include "dma.h"
#include "dualcore.h"
#include "dac_interface.h"
#include "adc_interface.h"
#include "i2s_interface.h"
#include "app_config.h"
#include "audio_core_api.h"
#include "audio_core_adapt.h"
#include "audio_effect_parambin.h"
#include "remind_sound.h"

#if defined(OSTICK_RELOAD) && (OSTICK_RELOAD != 1000)
void SysTickInit(void)
{
	//PLMT init
	PLMT_Init(E_PLMT_CLK_HOSC, OSTICK_RELOAD, OSTICK_RELOAD, OSTICK_RELOAD, FALSE, TRUE, TRUE);
	NVIC_EnableIRQ(PLMT_IRQn);
}
#endif
#ifdef CFG_LOW_DELAY_CTRL

bool LowDelyaSycnFlag;
uint16_t Buf[1024];

uint8_t RemindSoundIsPlay(void);
__attribute__((section(".ilm_section")))
void AudioMicDataSysc(void)
{
	static uint32_t cnt =0;
#if defined(DELAY_SOURCE_LEN_FUNC)
	uint32_t SourceSampleDelay = DELAY_SOURCE_LEN_FUNC();
#endif
#if defined(DELAY_SINK_LEN_FUNC)
	uint32_t SinkSampleDelay = DELAY_SINK_LEN_FUNC();
#endif
	uint32_t DelayTarget = LOW_DELAY_SAMPLE_TARGET;
	if(((SinkSampleDelay + SourceSampleDelay) > DelayTarget) && !RemindSoundIsPlay())
	{
		cnt++;
		if(cnt > 10)
		{
			cnt = 0;
			LowDelyaSycnFlag = 1;
		}
	}
	if(((SinkSampleDelay + SourceSampleDelay) > DelayTarget) && LowDelyaSycnFlag)
	{
		DELAY_SOURCE_GET_FUNC(Buf,  MIN(SinkSampleDelay + SourceSampleDelay - DelayTarget, sizeof(Buf)/8));
		LowDelyaSycnFlag = 0;
	}
}

void lowDelaySyncFlagSet(bool Flag)
{
	LowDelyaSycnFlag = Flag;
}

bool lowDelaySyncFlagGet(void)
{
	return LowDelyaSycnFlag;
}
#endif

#if defined(MIC_THRESHOLD_TRIGER_EN)
	__attribute__((section(".tcm_section")))
	void MicThresholdInterrupt(void)
	{
		if(DMA_InterruptFlagGet(MIC_DMA_PERI_ID_SEL, DMA_THRESHOLD_INT))
		{
			DMA_InterruptFlagClear(MIC_DMA_PERI_ID_SEL, DMA_THRESHOLD_INT);
			DMA_InterruptEnable(MIC_DMA_PERI_ID_SEL, DMA_THRESHOLD_INT, 0);
			if(AudioCore.AudioSource[MIC_SOURCE_NUM].InitFlag && (AudioCore.NetCt[AudioCore.AudioSource[MIC_SOURCE_NUM].Net].TrigerMsgFunc != NULL))
			{
				AudioCore.NetCt[AudioCore.AudioSource[MIC_SOURCE_NUM].Net].TrigerMsgFunc(MSG_NONE);
			}
		}
	}
#elif defined(I2S0_MIX_IN_THRESHOLD_TRIGER_EN)
	__attribute__((section(".tcm_section")))
	void I2S0MixInThresholdInterrupt(void)
	{
		if(DMA_InterruptFlagGet(PERIPHERAL_1_ID_I2S0_RX, DMA_THRESHOLD_INT))
		{
			DMA_InterruptFlagClear(PERIPHERAL_1_ID_I2S0_RX, DMA_THRESHOLD_INT);
			DMA_InterruptEnable(PERIPHERAL_1_ID_I2S0_RX, DMA_THRESHOLD_INT, 0);
			if(AudioCore.AudioSource[I2S0_MIX_SOURCE_NUM].InitFlag && (AudioCore.NetCt[AudioCore.AudioSource[I2S0_MIX_SOURCE_NUM].Net].TrigerMsgFunc != NULL))
			{
				AudioCore.NetCt[AudioCore.AudioSource[I2S0_MIX_SOURCE_NUM].Net].TrigerMsgFunc(MSG_NONE);
			}
		}
	}
#elif defined(I2S1_MIX_IN_THRESHOLD_TRIGER_EN)
	__attribute__((section(".tcm_section")))
	void I2S1MixInThresholdInterrupt(void)
	{
		if(DMA_InterruptFlagGet(PERIPHERAL_1_ID_I2S1_RX, DMA_THRESHOLD_INT))
		{
			DMA_InterruptFlagClear(PERIPHERAL_1_ID_I2S1_RX, DMA_THRESHOLD_INT);
			DMA_InterruptEnable(PERIPHERAL_1_ID_I2S1_RX, DMA_THRESHOLD_INT, 0);
			if(AudioCore.AudioSource[I2S1_MIX_SOURCE_NUM].InitFlag && (AudioCore.NetCt[AudioCore.AudioSource[I2S1_MIX_SOURCE_NUM].Net].TrigerMsgFunc != NULL))
			{
				AudioCore.NetCt[AudioCore.AudioSource[I2S1_MIX_SOURCE_NUM].Net].TrigerMsgFunc(MSG_NONE);
			}
		}
	}
#elif defined(I2S2_MIX_IN_THRESHOLD_TRIGER_EN)
	__attribute__((section(".tcm_section")))
	void I2S2MixInThresholdInterrupt(void)
	{
		if(DMA_InterruptFlagGet(PERIPHERAL_1_ID_I2S2_RX, DMA_THRESHOLD_INT))
		{
			DMA_InterruptFlagClear(PERIPHERAL_1_ID_I2S2_RX, DMA_THRESHOLD_INT);
			DMA_InterruptEnable(PERIPHERAL_1_ID_I2S2_RX, DMA_THRESHOLD_INT, 0);
			if(AudioCore.AudioSource[I2S2_MIX_SOURCE_NUM].InitFlag && (AudioCore.NetCt[AudioCore.AudioSource[I2S2_MIX_SOURCE_NUM].Net].TrigerMsgFunc != NULL))
			{
				AudioCore.NetCt[AudioCore.AudioSource[I2S2_MIX_SOURCE_NUM].Net].TrigerMsgFunc(MSG_NONE);
			}
		}
	}
#endif
