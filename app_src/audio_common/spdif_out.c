#include <spdif_api.h>
#include <string.h>
#include "type.h"
#include "gpio.h"
#include "dma.h"
#include "app_message.h"
#include "app_config.h"
#include "debug.h"
#include "delay.h"
#include "spdif.h"
#include "clk.h"
#include "mcu_circular_buf.h"
#include "ctrlvars.h"
#include "main_task.h"
#ifdef CFG_FUNC_I2S_DPLL_POLICY_EN
#include "i2s_dpll_policy.h"
#endif

#if defined(CFG_RES_AUDIO_SPDIF0OUT_EN) || defined(CFG_RES_AUDIO_SPDIF1OUT_EN)

#define SPDIFOUT_FIFO_LEN				(10 * 1024)

#ifdef CFG_RES_AUDIO_SPDIF0OUT_EN
uint32_t Spdif0Addr[SPDIFOUT_FIFO_LEN/4];
int32_t Spdif0Buf[1024];

#ifdef CFG_FUNC_I2S_DPLL_POLICY_EN
void SpdifOutDpllPolicySync(bool Enable);
#endif

void AudioSpdif0Out_SampleRateChange(uint32_t SampleRate)
{
#ifndef CFG_FUNC_I2S_DPLL_POLICY_EN
	if((SampleRate == 11025) || (SampleRate == 22050) || (SampleRate == 44100)
			|| (SampleRate == 88200) || (SampleRate == 176400))
	{
		Clock_DPllLock(DPLL0_MODULE, 44100 * 5120);
	}
	else
	{
		Clock_DPllLock(DPLL0_MODULE, 48000 * 5120);
	}
#endif
	SPDIF_SampleRateSet(SPDIF0_OUT_MODULE, SampleRate);
}

void AudioSpdif0OutParamsSet(uint32_t sampleRate)
{
	GPIO_PortBModeSet(SPDIF0_OUT_INDEX, SPDIF0_OUT_PORT_MODE);
	Clock_Spdif0ClkSelect(DPLL0_CLK_MODE);
	SPDIF_TXInit(SPDIF0_OUT_MODULE, 1, 1, 0, 0);
	DMA_CircularConfig(SPDIF0_OUT_DMA_ID, sizeof(Spdif0Addr)/4, Spdif0Addr, sizeof(Spdif0Addr));
	AudioSpdif0Out_SampleRateChange(sampleRate);
	SPDIF_ModuleEnable(SPDIF0_OUT_MODULE);
	DMA_ChannelEnable(SPDIF0_OUT_DMA_ID);
}

uint16_t AudioSpdif0TXDataSet(void* Buf, uint16_t Len)
{
	uint16_t Length;
	int m;
	if(Buf == NULL) return 0;

	memset(Spdif0Buf, 0, sizeof(Spdif0Buf));

#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
	Length = Len * 8;
	m = SPDIF_PCMDataToSPDIFData(SPDIF0_OUT_MODULE, (int32_t *)Buf, Length, (int32_t *)Spdif0Buf, SPDIF_WORDLTH_24BIT);
#else
	Length = Len * 4;
	m = SPDIF_PCMDataToSPDIFData(SPDIF0_OUT_MODULE, (int32_t *)Buf, Length, (int32_t *)Spdif0Buf, SPDIF_WORDLTH_16BIT);
#endif
	DMA_CircularDataPut(SPDIF0_OUT_DMA_ID, (void *)Spdif0Buf, m & 0xFFFFFFFC);
	return 0;
}

uint16_t AudioSpdif0TXDataSpaceLenGet(void)
{
#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
	return DMA_CircularSpaceLenGet(SPDIF0_OUT_DMA_ID) / 16;
#else
	return DMA_CircularSpaceLenGet(SPDIF0_OUT_DMA_ID) / 8;
#endif
}
#endif

// SPDIF 1 output buffers and functions
#ifdef CFG_RES_AUDIO_SPDIF1OUT_EN
uint32_t Spdif1Addr[SPDIFOUT_FIFO_LEN/4];
int32_t Spdif1Buf[1024];

void AudioSpdif1Out_SampleRateChange(uint32_t SampleRate)
{
#ifndef CFG_FUNC_I2S_DPLL_POLICY_EN
	if((SampleRate == 11025) || (SampleRate == 22050) || (SampleRate == 44100)
			|| (SampleRate == 88200) || (SampleRate == 176400))
	{
		Clock_DPllLock(DPLL0_MODULE, 44100 * 5120);
	}
	else
	{
		Clock_DPllLock(DPLL0_MODULE, 48000 * 5120);
	}
#endif
	SPDIF_SampleRateSet(SPDIF1_OUT_MODULE, SampleRate);
}

void AudioSpdif1OutParamsSet(uint32_t sampleRate)
{
	GPIO_PortBModeSet(SPDIF1_OUT_INDEX, SPDIF1_OUT_PORT_MODE);
	Clock_Spdif1ClkSelect(DPLL0_CLK_MODE);
	SPDIF_TXInit(SPDIF1_OUT_MODULE, 1, 1, 0, 0);
	DMA_CircularConfig(SPDIF1_OUT_DMA_ID, sizeof(Spdif1Addr)/4, Spdif1Addr, sizeof(Spdif1Addr));
	AudioSpdif1Out_SampleRateChange(sampleRate);
	SPDIF_ModuleEnable(SPDIF1_OUT_MODULE);
	DMA_ChannelEnable(SPDIF1_OUT_DMA_ID);
}

uint16_t AudioSpdif1TXDataSet(void* Buf, uint16_t Len)
{
	uint16_t Length;
	int m;
	if(Buf == NULL) return 0;

	memset(Spdif1Buf, 0, sizeof(Spdif1Buf));

#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
	Length = Len * 8;
	m = SPDIF_PCMDataToSPDIFData(SPDIF1_OUT_MODULE, (int32_t *)Buf, Length, (int32_t *)Spdif1Buf, SPDIF_WORDLTH_24BIT);
#else
	Length = Len * 4;
	m = SPDIF_PCMDataToSPDIFData(SPDIF1_OUT_MODULE, (int32_t *)Buf, Length, (int32_t *)Spdif1Buf, SPDIF_WORDLTH_16BIT);
#endif
	DMA_CircularDataPut(SPDIF1_OUT_DMA_ID, (void *)Spdif1Buf, m & 0xFFFFFFFC);
	return 0;
}

uint16_t AudioSpdif1TXDataSpaceLenGet(void)
{
#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
	return DMA_CircularSpaceLenGet(SPDIF1_OUT_DMA_ID) / 16;
#else
	return DMA_CircularSpaceLenGet(SPDIF1_OUT_DMA_ID) / 8;
#endif
}
#endif


bool SpdifOutInit(void)
{
	uint32_t sampleRate = AudioCoreMixSampleRateGet(AudioCoreGetNetBySinkChannelNUM(AUDIO_SPDIF0_SINK_NUM));
	
	#ifdef CFG_RES_AUDIO_SPDIF0OUT_EN
	{
		AudioCoreIO AudioIOSet;
		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
		AudioIOSet.Net = AudioCoreGetNetBySinkChannelNUM(AUDIO_SPDIF0_SINK_NUM);
		AudioIOSet.Depth = AudioCoreMixStepUsGet(AudioIOSet.Net) * 2 * sampleRate / 1000000;
		AudioIOSet.PcmWidth = CFG_DEFAULT_WIDTH;
		if(!AudioCoreSinkIsInit(AUDIO_SPDIF0_SINK_NUM))
		{
			mainAppCt.SPDIF0_OUT_FIFO_LEN = AudioIOSet.Depth * sizeof(PCM_DATA_TYPE) * 2;
			mainAppCt.SPDIF0_OUT_FIFO = (uint32_t*)osPortMalloc(mainAppCt.SPDIF0_OUT_FIFO_LEN);
			if(mainAppCt.SPDIF0_OUT_FIFO != NULL)
			{
				memset(mainAppCt.SPDIF0_OUT_FIFO, 0, mainAppCt.SPDIF0_OUT_FIFO_LEN);
			}
			else
			{
				APP_DBG("malloc SPDIF0_OUT_FIFO error\n");
				return FALSE;
			}

			AudioIOSet.Adapt = SRC_ONLY;
			AudioIOSet.SrcQuality = SRC_QUALITY;
			AudioIOSet.Sync = TRUE;
			AudioIOSet.Channels = 2;
			AudioIOSet.HighLevelCent = 60;
			AudioIOSet.LowLevelCent = 40;
			AudioIOSet.SampleRate = sampleRate;

			AudioIOSet.DataIOFunc = AudioSpdif0TXDataSet;
			AudioIOSet.LenGetFunc = AudioSpdif0TXDataSpaceLenGet;

			if(!AudioCoreSinkInit(&AudioIOSet, AUDIO_SPDIF0_SINK_NUM))
			{
				DBG("spdif0 out init error");
				return FALSE;
			}
			AudioSpdif0OutParamsSet(sampleRate);
#ifdef CFG_FUNC_I2S_DPLL_POLICY_EN
			SpdifOutDpllPolicySync(I2SDpllPolicy_IsStable());
#else
			AudioCoreSinkEnable(AUDIO_SPDIF0_SINK_NUM);
			AudioCoreSinkAdjust(AUDIO_SPDIF0_SINK_NUM, TRUE);
#endif
		}
		else
		{
			SPDIF_SampleRateSet(SPDIF0_OUT_MODULE, sampleRate);
		#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
			AudioCore.AudioSink[AUDIO_SPDIF0_SINK_NUM].Sync = TRUE;
		#endif
			AudioCore.AudioSink[AUDIO_SPDIF0_SINK_NUM].PcmWidth = AudioIOSet.PcmWidth;
#ifdef CFG_FUNC_I2S_DPLL_POLICY_EN
			SpdifOutDpllPolicySync(I2SDpllPolicy_IsStable());
#endif
		}
	}
	#endif

	sampleRate = AudioCoreMixSampleRateGet(AudioCoreGetNetBySinkChannelNUM(AUDIO_SPDIF1_SINK_NUM));
	#ifdef CFG_RES_AUDIO_SPDIF1OUT_EN
	{
		AudioCoreIO AudioIOSet;
		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
		AudioIOSet.Net = AudioCoreGetNetBySinkChannelNUM(AUDIO_SPDIF1_SINK_NUM);
		AudioIOSet.Depth = AudioCoreMixStepUsGet(AudioIOSet.Net) * 2 * sampleRate / 1000000;
		AudioIOSet.PcmWidth = CFG_DEFAULT_WIDTH;
		if(!AudioCoreSinkIsInit(AUDIO_SPDIF1_SINK_NUM))
		{
			mainAppCt.SPDIF1_OUT_FIFO_LEN = AudioIOSet.Depth * sizeof(PCM_DATA_TYPE) * 2;
			mainAppCt.SPDIF1_OUT_FIFO = (uint32_t*)osPortMalloc(mainAppCt.SPDIF1_OUT_FIFO_LEN);
			if(mainAppCt.SPDIF1_OUT_FIFO != NULL)
			{
				memset(mainAppCt.SPDIF1_OUT_FIFO, 0, mainAppCt.SPDIF1_OUT_FIFO_LEN);
			}
			else
			{
				APP_DBG("malloc SPDIF1_OUT_FIFO error\n");
				return FALSE;
			}

			AudioIOSet.Adapt = SRC_ONLY;
			AudioIOSet.SrcQuality = SRC_QUALITY;
			AudioIOSet.Sync = TRUE;
			AudioIOSet.Channels = 2;
			AudioIOSet.Net = AudioCoreGetNetBySinkChannelNUM(AUDIO_SPDIF1_SINK_NUM);
			AudioIOSet.HighLevelCent = 60;
			AudioIOSet.LowLevelCent = 40;
			AudioIOSet.SampleRate = sampleRate;

			AudioIOSet.DataIOFunc = AudioSpdif1TXDataSet;
			AudioIOSet.LenGetFunc = AudioSpdif1TXDataSpaceLenGet;

			if(!AudioCoreSinkInit(&AudioIOSet, AUDIO_SPDIF1_SINK_NUM))
			{
				DBG("spdif1 out init error");
				return FALSE;
			}
			AudioSpdif1OutParamsSet(sampleRate);
#ifdef CFG_FUNC_I2S_DPLL_POLICY_EN
			SpdifOutDpllPolicySync(I2SDpllPolicy_IsStable());
#else
			AudioCoreSinkEnable(AUDIO_SPDIF1_SINK_NUM);
			AudioCoreSinkAdjust(AUDIO_SPDIF1_SINK_NUM, TRUE);
#endif
		}
		else
		{
			SPDIF_SampleRateSet(SPDIF1_OUT_MODULE, sampleRate);
		#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
			AudioCore.AudioSink[AUDIO_SPDIF1_SINK_NUM].Sync = TRUE;
		#endif
			AudioCore.AudioSink[AUDIO_SPDIF1_SINK_NUM].PcmWidth = AudioIOSet.PcmWidth;
#ifdef CFG_FUNC_I2S_DPLL_POLICY_EN
			SpdifOutDpllPolicySync(I2SDpllPolicy_IsStable());
#endif
		}
	}
	#endif

	return TRUE;
}

bool SpdifOutDeinit(void)
{
	#ifdef CFG_RES_AUDIO_SPDIF0OUT_EN
	{
		SPDIF_ModuleDisable(SPDIF0_OUT_MODULE);
		DMA_InterruptFlagClear(SPDIF0_OUT_DMA_ID, DMA_DONE_INT);
		DMA_InterruptFlagClear(SPDIF0_OUT_DMA_ID, DMA_THRESHOLD_INT);
		DMA_InterruptFlagClear(SPDIF0_OUT_DMA_ID, DMA_ERROR_INT);
		DMA_ChannelDisable(SPDIF0_OUT_DMA_ID);

		if(mainAppCt.SPDIF0_OUT_FIFO != NULL)
		{
			APP_DBG("SPDIF0 OUT FIFO\n");
			osPortFree(mainAppCt.SPDIF0_OUT_FIFO);
			mainAppCt.SPDIF0_OUT_FIFO = NULL;
		}
		AudioCoreSinkDeinit(AUDIO_SPDIF0_SINK_NUM);
	}
	#endif

	#ifdef CFG_RES_AUDIO_SPDIF1OUT_EN
	{
		SPDIF_ModuleDisable(SPDIF1_OUT_MODULE);
		DMA_InterruptFlagClear(SPDIF1_OUT_DMA_ID, DMA_DONE_INT);
		DMA_InterruptFlagClear(SPDIF1_OUT_DMA_ID, DMA_THRESHOLD_INT);
		DMA_InterruptFlagClear(SPDIF1_OUT_DMA_ID, DMA_ERROR_INT);
		DMA_ChannelDisable(SPDIF1_OUT_DMA_ID);

		if(mainAppCt.SPDIF1_OUT_FIFO != NULL)
		{
			APP_DBG("SPDIF1 OUT FIFO\n");
			osPortFree(mainAppCt.SPDIF1_OUT_FIFO);
			mainAppCt.SPDIF1_OUT_FIFO = NULL;
		}
		AudioCoreSinkDeinit(AUDIO_SPDIF1_SINK_NUM);
	}
	#endif

	return TRUE;
}

#ifdef CFG_FUNC_I2S_DPLL_POLICY_EN
void SpdifOutDpllPolicySync(bool Enable)
{
	uint32_t sampleRate = 44100;
#ifdef CFG_RES_AUDIO_SPDIF0OUT_EN
	if(AudioCoreSinkIsInit(AUDIO_SPDIF0_SINK_NUM))
	{
		if(Enable)
		{
			if(!AudioCoreSinkIsEnable(AUDIO_SPDIF0_SINK_NUM))
			{
				sampleRate = AudioCoreMixSampleRateGet(AudioCoreGetNetBySinkChannelNUM(AUDIO_SPDIF0_SINK_NUM));
				AudioSpdif0Out_SampleRateChange(sampleRate);
				AudioCoreSinkEnable(AUDIO_SPDIF0_SINK_NUM);
				AudioCoreSinkAdjust(AUDIO_SPDIF0_SINK_NUM, TRUE);
				APP_DBG("SPDIF0 OUT sink enable by I2S DPLL policy\n");
			}
		}
		else if(AudioCoreSinkIsEnable(AUDIO_SPDIF0_SINK_NUM))
		{
			AudioCoreSinkDisable(AUDIO_SPDIF0_SINK_NUM);
			APP_DBG("SPDIF0 OUT sink disable by I2S DPLL policy\n");
		}
	}
#endif
#ifdef CFG_RES_AUDIO_SPDIF1OUT_EN
	if(AudioCoreSinkIsInit(AUDIO_SPDIF1_SINK_NUM))
	{
		if(Enable)
		{
			if(!AudioCoreSinkIsEnable(AUDIO_SPDIF1_SINK_NUM))
			{
				sampleRate = AudioCoreMixSampleRateGet(AudioCoreGetNetBySinkChannelNUM(AUDIO_SPDIF1_SINK_NUM));
				AudioSpdif1Out_SampleRateChange(sampleRate);
				AudioCoreSinkEnable(AUDIO_SPDIF1_SINK_NUM);
				AudioCoreSinkAdjust(AUDIO_SPDIF1_SINK_NUM, TRUE);
				APP_DBG("SPDIF1 OUT sink enable by I2S DPLL policy\n");
			}
		}
		else if(AudioCoreSinkIsEnable(AUDIO_SPDIF1_SINK_NUM))
		{
			AudioCoreSinkDisable(AUDIO_SPDIF1_SINK_NUM);
			APP_DBG("SPDIF1 OUT sink disable by I2S DPLL policy\n");
		}
	}
#endif
}
#endif

#endif
