#include "type.h"
#include "dac.h"
#include "dma.h"
#include "clk.h"
#include "debug.h"
#include "dac_interface.h"
#include "irqn.h"
#include "math.h"
#include "timeout.h"
#ifdef CFG_APP_CONFIG
#include "app_config.h"
#endif

void AudioDAC_Init(AUDIODAC_MODULE DACMODULE, AUDIODAC_ParamCt *DAC0, AUDIODAC_ParamCt *DAC1, AUDIODAC_ParamCt *DAC2)
{
	Clock_AudioPllClockSet(APLL_CLK_MODE, PLL_CLK_1, AUDIO_PLL_CLK1_FREQ);
	Clock_AudioPllClockSet(APLL_CLK_MODE, PLL_CLK_2, AUDIO_PLL_CLK2_FREQ);
	Clock_AudioPllClockSet(APLL_CLK_MODE, PLL_CLK_3, AUDIO_PLL_CLK3_FREQ);
	Clock_AudioPllClockSet(APLL_CLK_MODE, PLL_CLK_4, AUDIO_PLL_CLK4_FREQ);

	if((DACMODULE & DAC0_MODULE) && (DAC0 != NULL))
	{
		AudioDAC_REGClkEnable(DAC0_MODULE, TRUE);
		AudioDAC_ClkEnable(DAC0_MODULE, TRUE);

		if(IsSelectMclkClk1(DAC0->SampleRate))
		{
			Clock_AudioMclkSel(AUDIO_DAC0, PLL_CLOCK1);
		}
		else
		{
			Clock_AudioMclkSel(AUDIO_DAC0, PLL_CLOCK2);
		}

		if(DAC0->SampleRate > 48000)
		{
			AudioDAC_MclkDivSet(DAC0_MODULE, 1);
		}
		else
		{
			AudioDAC_MclkDivSet(DAC0_MODULE, 2);
		}

		if(DAC0->SampleRate >= 176400)
		{
			AudioDAC_FirSel(DAC0_MODULE, 1);   //采样率>176.4K 不许改动
		}
		else
		{
			AudioDAC_FirSel(DAC0_MODULE, DAC0->FIRMode);   //其余采样率下可配置
		}

		if(DAC0->DACBitWidth == DAC_16BIT)
		{
			AudioDAC_DoutModeSet(DAC0_MODULE, DAC0->DoutMode, DAC_16BIT); // WIDTH_24_BIT_1, WIDTH_24_BIT_2, WIDTH_16_BIT
		}
		else if((DAC0->DACBitWidth == DAC_24BIT_HIGHALIGN))
		{
			AudioDAC_DoutModeSet(DAC0_MODULE, DAC0->DoutMode, DAC_24BIT_HIGHALIGN);
		}
		else if(DAC0->DACBitWidth == DAC_24BIT_LOWALIGN)
		{
			AudioDAC_DoutModeSet(DAC0_MODULE, DAC0->DoutMode, DAC_24BIT_LOWALIGN);
		}

		AudioDAC_FuncReset(DAC0_MODULE);

		AudioDAC_SampleRateSet(DAC0_MODULE, DAC0->SampleRate);
		AudioDAC_EdgeSet(DAC0_MODULE, POSEDGE);
		AudioDAC_VolSet(DAC0_MODULE, 0x1000, 0x1000);
		//AudioDAC_DitherEnable(DAC0);
		AudioDAC_ScrambleEnable(DAC0_MODULE);
		AudioDAC_ScrambleModeSet(DAC0_MODULE, POS_NEG);
		AudioDAC_DsmOptimizeSet(DAC0_MODULE);
		AudioDAC_DsmOutdisModeSet(DAC0_MODULE, 1);
		AudioDAC_ZeroNumSet(DAC0_MODULE, 5);

		DMA_ChannelDisable(PERIPHERAL_1_ID_AUDIO_DAC0_TX);
    	DMA_CircularConfig(PERIPHERAL_1_ID_AUDIO_DAC0_TX, DAC0->DacFifoLen/2, DAC0->DacFifo, DAC0->DacFifoLen);
    	DMA_ChannelEnable(PERIPHERAL_1_ID_AUDIO_DAC0_TX);

		AudioDAC_ClkEnable(DAC0_MODULE, TRUE);
		AudioDAC_Reset(DAC0_MODULE);
		AudioDAC_Enable(DAC0_MODULE);
#ifdef CFG_HP_DAC0_BOOT_MUTE_TEST
		AudioDAC_SoftMute(DAC0_MODULE, TRUE, TRUE);
#else
		AudioDAC_SoftMute(DAC0_MODULE, FALSE, FALSE);
#endif
	}

	if((DACMODULE & DAC1_MODULE) && (DAC1 != NULL))
	{
		AudioDAC_REGClkEnable(DAC1_MODULE, TRUE);
		AudioDAC_ClkEnable(DAC1_MODULE, TRUE);

		if(IsSelectMclkClk1(DAC1->SampleRate))
		{
			Clock_AudioMclkSel(AUDIO_DAC1, PLL_CLOCK1);
		}
		else
		{
			Clock_AudioMclkSel(AUDIO_DAC1, PLL_CLOCK2);
		}

		if(DAC1->SampleRate > 48000)
		{
			AudioDAC_MclkDivSet(DAC1_MODULE, 1);
		}
		else
		{
			AudioDAC_MclkDivSet(DAC1_MODULE, 2);
		}

		if(DAC1->SampleRate >= 176400)
		{
			AudioDAC_FirSel(DAC1_MODULE, 1);   //采样率>176.4K 不许改动
		}
		else
		{
			AudioDAC_FirSel(DAC1_MODULE, DAC1->FIRMode);   //其余采样率下可配置
		}

		if(DAC1->DACBitWidth == DAC_16BIT)
		{
			AudioDAC_DoutModeSet(DAC1_MODULE, DAC1->DoutMode, DAC_16BIT); // WIDTH_24_BIT_1, WIDTH_24_BIT_2, WIDTH_16_BIT
		}
		else if((DAC1->DACBitWidth == DAC_24BIT_HIGHALIGN))
		{
			AudioDAC_DoutModeSet(DAC1_MODULE, DAC1->DoutMode, DAC_24BIT_HIGHALIGN);
		}
		else if(DAC1->DACBitWidth == DAC_24BIT_LOWALIGN)
		{
			AudioDAC_DoutModeSet(DAC1_MODULE, DAC1->DoutMode, DAC_24BIT_LOWALIGN);
		}

		AudioDAC_FuncReset(DAC1_MODULE);

		AudioDAC_SampleRateSet(DAC1_MODULE, DAC1->SampleRate);
		AudioDAC_EdgeSet(DAC1_MODULE, POSEDGE);
		AudioDAC_VolSet(DAC1_MODULE, 0x1000, 0x1000);
		//AudioDAC_DitherEnable(DAC0);
		AudioDAC_ScrambleEnable(DAC1_MODULE);
		AudioDAC_ScrambleModeSet(DAC1_MODULE, POS_NEG);
		AudioDAC_DsmOptimizeSet(DAC1_MODULE);
		AudioDAC_DsmOutdisModeSet(DAC1_MODULE, 1);
		AudioDAC_ZeroNumSet(DAC1_MODULE, 5);

		DMA_ChannelDisable(PERIPHERAL_1_ID_AUDIO_DAC1_TX);
    	DMA_CircularConfig(PERIPHERAL_1_ID_AUDIO_DAC1_TX, DAC1->DacFifoLen/2, DAC1->DacFifo, DAC1->DacFifoLen);
    	DMA_ChannelEnable(PERIPHERAL_1_ID_AUDIO_DAC1_TX);

		AudioDAC_ClkEnable(DAC1_MODULE, TRUE);
		AudioDAC_Reset(DAC1_MODULE);
		AudioDAC_Enable(DAC1_MODULE);
		AudioDAC_SoftMute(DAC1_MODULE, FALSE, FALSE);
	}

	if((DACMODULE & DAC2_MODULE) && (DAC2 != NULL))
	{
		AudioDAC_REGClkEnable(DAC2_MODULE, TRUE);
		AudioDAC_ClkEnable(DAC2_MODULE, TRUE);

    	if(IsSelectMclkClk1(DAC2->SampleRate))
		{
			Clock_AudioMclkSel(AUDIO_DAC2, PLL_CLOCK1);
		}
		else
		{
			Clock_AudioMclkSel(AUDIO_DAC2, PLL_CLOCK2);
		}

		if(DAC2->SampleRate > 48000)
		{
			AudioDAC_MclkDivSet(DAC2_MODULE, 1);
		}
		else
		{
			AudioDAC_MclkDivSet(DAC2_MODULE, 2);
		}

		if(DAC2->SampleRate >= 176400)
		{
			AudioDAC_FirSel(DAC2_MODULE, 1);   //采样率>176.4K 不许改动
		}
		else
		{
			AudioDAC_FirSel(DAC2_MODULE, DAC2->FIRMode);   //其余采样率下可配置
		}

		if(DAC2->DACBitWidth == DAC_16BIT)
		{
			AudioDAC_DoutModeSet(DAC2_MODULE, DAC2->DoutMode, DAC_16BIT); // WIDTH_24_BIT_1, WIDTH_24_BIT_2, WIDTH_16_BIT
		}
		else if((DAC2->DACBitWidth == DAC_24BIT_HIGHALIGN))
		{
			AudioDAC_DoutModeSet(DAC2_MODULE, DAC2->DoutMode, DAC_24BIT_HIGHALIGN);
		}
		else if(DAC2->DACBitWidth == DAC_24BIT_LOWALIGN)
		{
			AudioDAC_DoutModeSet(DAC2_MODULE, DAC2->DoutMode, DAC_24BIT_LOWALIGN);
		}

		AudioDAC_FuncReset(DAC2_MODULE);

		AudioDAC_SampleRateSet(DAC2_MODULE, DAC2->SampleRate);
		AudioDAC_EdgeSet(DAC2_MODULE, POSEDGE);
		AudioDAC_VolSet(DAC2_MODULE, 0x1000, 0x1000);
		//AudioDAC_DitherEnable(DAC0);
		AudioDAC_ScrambleEnable(DAC2_MODULE);
		AudioDAC_ScrambleModeSet(DAC2_MODULE, POS_NEG);
		AudioDAC_DsmOptimizeSet(DAC2_MODULE);
		AudioDAC_DsmOutdisModeSet(DAC2_MODULE, 1);
		AudioDAC_ZeroNumSet(DAC2_MODULE, 5);

    	DMA_ChannelDisable(PERIPHERAL_1_ID_AUDIO_DAC2_TX);
    	DMA_CircularConfig(PERIPHERAL_1_ID_AUDIO_DAC2_TX, DAC2->DacFifoLen/2, DAC2->DacFifo, DAC2->DacFifoLen);
		if(DAC2->DACBitWidth == DAC_16BIT)
		{
			DMA_ConfigDwidth(PERIPHERAL_1_ID_AUDIO_DAC2_TX, DMA_DWIDTH_HALF_WORD);
		}
    	DMA_ChannelEnable(PERIPHERAL_1_ID_AUDIO_DAC2_TX);

		AudioDAC_ClkEnable(DAC2_MODULE, TRUE);
		AudioDAC_Reset(DAC2_MODULE);
		AudioDAC_Enable(DAC2_MODULE);
		AudioDAC_SoftMute(DAC2_MODULE, FALSE, FALSE);
	}

	AudioDAC_PowerOn(DACMODULE, DAC0, DAC1, DAC2);
}

void AudioDAC_DeInit(AUDIODAC_MODULE DACMODULE)
{
	TIMER Request_cnt;
	uint8_t PeripheralID;

	AudioDAC_Pause(DACMODULE);
	TimeOutSet(&Request_cnt, 3);
	while(AudioDAC_RequestCountGet(DACMODULE) != 0)
	{
		if(IsTimeOut(&Request_cnt))
		{
			break;
		}
	}
	AudioDAC_Disable(DACMODULE);
	//AudioDAC_FuncReset(DACMODULE);//IC指导不要reset
	if(DACMODULE == DAC0_MODULE)
	{
		PeripheralID = PERIPHERAL_1_ID_AUDIO_DAC0_TX;
	}
	else if(DACMODULE == DAC1_MODULE)
	{
		PeripheralID = PERIPHERAL_1_ID_AUDIO_DAC1_TX;
	}
	else
	{
		PeripheralID = PERIPHERAL_1_ID_AUDIO_DAC2_TX;
	}
	DMA_InterruptFlagClear(PeripheralID, DMA_DONE_INT);
	DMA_InterruptFlagClear(PeripheralID, DMA_THRESHOLD_INT);
	DMA_InterruptFlagClear(PeripheralID, DMA_ERROR_INT);
	DMA_CircularFIFOClear(PeripheralID);
	DMA_ChannelDisable(PeripheralID);
	AudioDAC_Run(DACMODULE);
}

void AudioDAC_DataClear(AUDIODAC_MODULE DACMODULE)
{
	TIMER Request_cnt;
	uint8_t PeripheralID;

	AudioDAC_Pause(DACMODULE);
	TimeOutSet(&Request_cnt, 3);
	while(AudioDAC_RequestCountGet(DACMODULE) != 0)
	{
		if(IsTimeOut(&Request_cnt))
		{
			break;
		}
	}
//	AudioDAC_Disable(DACMODULE);
	//AudioDAC_FuncReset(DACMODULE);//IC指导不要reset
	if(DACMODULE == DAC0_MODULE)
	{
		PeripheralID = PERIPHERAL_1_ID_AUDIO_DAC0_TX;
	}
	else if(DACMODULE == DAC1_MODULE)
	{
		PeripheralID = PERIPHERAL_1_ID_AUDIO_DAC1_TX;
	}
	else
	{
		PeripheralID = PERIPHERAL_1_ID_AUDIO_DAC2_TX;
	}
	DMA_InterruptFlagClear(PeripheralID, DMA_DONE_INT);
	DMA_InterruptFlagClear(PeripheralID, DMA_THRESHOLD_INT);
	DMA_InterruptFlagClear(PeripheralID, DMA_ERROR_INT);
	DMA_CircularFIFOClear(PeripheralID);
	DMA_ChannelDisable(PeripheralID);
	DMA_CircularWritePtrSet(PeripheralID,0);
//	AudioDAC_Run(DACMODULE);
}

void AudioDAC_SampleRateChange(AUDIODAC_MODULE DACMODULE, uint32_t SampleRate)
{
	if(SampleRate >= 176400)
	{
		AudioDAC_FirSel(DACMODULE, 1);   //>= 176.4K 强制开启, 硬件限制
	}
	// 其余采样率时FIR保持原值不变, 沿用 AudioDAC_Init 时 FIRMode 的设置

	if(DACMODULE == DAC0_MODULE)
	{
		if(IsSelectMclkClk1(SampleRate))
		{
			Clock_AudioMclkSel(AUDIO_DAC0, PLL_CLOCK1);
		}
		else
		{
			Clock_AudioMclkSel(AUDIO_DAC0, PLL_CLOCK2);
		}
	}
	else if(DACMODULE == DAC1_MODULE)
	{
		if(IsSelectMclkClk1(SampleRate))
		{
			Clock_AudioMclkSel(AUDIO_DAC1, PLL_CLOCK1);
		}
		else
		{
			Clock_AudioMclkSel(AUDIO_DAC1, PLL_CLOCK2);
		}
	}
	else if((DACMODULE == DAC2_MODULE))
	{
		if(IsSelectMclkClk1(SampleRate))
		{
			Clock_AudioMclkSel(AUDIO_DAC2, PLL_CLOCK1);
		}
		else
		{
			Clock_AudioMclkSel(AUDIO_DAC2, PLL_CLOCK2);
		}
	}

	AudioDAC_SampleRateSet(DACMODULE, SampleRate);
}

uint16_t AudioDAC0_DataSpaceLenGet()
{
	if(AudioDAC_BitWidthGet(DAC0_MODULE) == DAC_16BIT)
		return DMA_CircularSpaceLenGet(PERIPHERAL_1_ID_AUDIO_DAC0_TX) / 4;
	else
		return DMA_CircularSpaceLenGet(PERIPHERAL_1_ID_AUDIO_DAC0_TX) / 8;
}

uint16_t AudioDAC1_DataSpaceLenGet()
{
	if(AudioDAC_BitWidthGet(DAC1_MODULE) == DAC_16BIT)
		return DMA_CircularSpaceLenGet(PERIPHERAL_1_ID_AUDIO_DAC1_TX) / 4;
	else
		return DMA_CircularSpaceLenGet(PERIPHERAL_1_ID_AUDIO_DAC1_TX) / 8;
}

uint16_t AudioDAC2_DataSpaceLenGet()
{
	if(AudioDAC_BitWidthGet(DAC2_MODULE) == DAC_16BIT)
		return DMA_CircularSpaceLenGet(PERIPHERAL_1_ID_AUDIO_DAC2_TX) / 2;
	else
		return DMA_CircularSpaceLenGet(PERIPHERAL_1_ID_AUDIO_DAC2_TX) / 4;
}

uint16_t AudioDAC0_DataLenGet()
{
	if(AudioDAC_BitWidthGet(DAC0_MODULE) == DAC_16BIT)
		return DMA_CircularDataLenGet(PERIPHERAL_1_ID_AUDIO_DAC0_TX) / 4;
	else
		return DMA_CircularDataLenGet(PERIPHERAL_1_ID_AUDIO_DAC0_TX) / 8;
}

uint16_t AudioDAC1_DataLenGet()
{
	if(AudioDAC_BitWidthGet(DAC1_MODULE) == DAC_16BIT)
		return DMA_CircularDataLenGet(PERIPHERAL_1_ID_AUDIO_DAC1_TX) / 4;
	else
		return DMA_CircularDataLenGet(PERIPHERAL_1_ID_AUDIO_DAC1_TX) / 8;
}

uint16_t AudioDAC2_DataLenGet()
{
	if(AudioDAC_BitWidthGet(DAC2_MODULE) == DAC_16BIT)
		return DMA_CircularDataLenGet(PERIPHERAL_1_ID_AUDIO_DAC2_TX) / 2;
	else
		return DMA_CircularDataLenGet(PERIPHERAL_1_ID_AUDIO_DAC2_TX) / 4;
}

uint16_t AudioDAC0_DataSet(void* Buf, uint16_t Len)
{
	uint16_t Length;

	if(Buf == NULL) return 0;
	if(AudioDAC_BitWidthGet(DAC0_MODULE) == DAC_16BIT)
	{
		Length = Len * 4;
		DMA_CircularDataPut(PERIPHERAL_1_ID_AUDIO_DAC0_TX, Buf, Length & 0xFFFFFFFC);
	}
	else
	{
		Length = Len * 8;
		DMA_CircularDataPut(PERIPHERAL_1_ID_AUDIO_DAC0_TX, Buf, Length & 0xFFFFFFF8);
	}
	return 0;
}

uint16_t AudioDAC1_DataSet(void* Buf, uint16_t Len)
{
	uint16_t Length;

	if(Buf == NULL) return 0;
	if(AudioDAC_BitWidthGet(DAC1_MODULE) == DAC_16BIT)
	{
		Length = Len * 4;
		DMA_CircularDataPut(PERIPHERAL_1_ID_AUDIO_DAC1_TX, Buf, Length & 0xFFFFFFFC);
	}
	else
	{
		Length = Len * 8;
		DMA_CircularDataPut(PERIPHERAL_1_ID_AUDIO_DAC1_TX, Buf, Length & 0xFFFFFFF8);
	}
	return 0;
}

uint16_t AudioDAC2_DataSet(void* Buf, uint16_t Len)
{
	uint16_t Length;

	if(Buf == NULL)
		return 0;

	if(AudioDAC_BitWidthGet(DAC2_MODULE) == DAC_16BIT)
	{
		Length = Len * 2;
		DMA_CircularDataPut(PERIPHERAL_1_ID_AUDIO_DAC2_TX, Buf, Length & 0xFFFFFFFC);
	}
	else
	{
		Length = Len * 4;
		DMA_CircularDataPut(PERIPHERAL_1_ID_AUDIO_DAC2_TX, Buf, Length & 0xFFFFFFFC);
	}

	return 0;
}

void AudioDAC_MclkDivSet(AUDIODAC_MODULE Module,uint32_t Div)
{
	if(Module == DAC0_MODULE)
    {
		Clock_DAC0ClkDivSet(Div - 1);
    }
	else if(Module == DAC1_MODULE)
	{
		Clock_DAC1ClkDivSet(Div - 1);
	}
	else
	{
		Clock_DAC2ClkDivSet(Div - 1);
	}
}

uint16_t dBToRegisterValue(int16_t dB)
{
    if (dB < -7220 || dB > 1200)
	{
        if (dB < -7220) return 0x0001;
        if (dB > 1200) return 0x3FFF;
    }

    float X = 4096 * powf(10, (dB/100.0f) / 20.0f);
    uint16_t regValue = (uint16_t)round(X);

    if (regValue < 0x0001) regValue = 0x0001;
    if (regValue > 0x3FFF) regValue = 0x3FFF;

    return regValue;
}

/**
 * @brief  DAC 模块音量设置
 * @param  DACModule    0,DAC0模块; 1,DAC1模块；2,DAC2模块
 * @param  LeftVol      左声道音量值，0x00:无增益, 范围：-7220(-72.2dB) ~ +1200(12dB)
 * @param  RightVol     右声道音量值，0x00:无增益, 范围：-7220(-72.2dB) ~ +1200(12dB)
 * @return 无
 */
void AudioDAC_VolGainSet_For_dB(AUDIODAC_MODULE DACModule, int16_t LeftVol, int16_t RightVol)
{
    uint16_t LeftRegVal, RightRegVal;

    LeftRegVal = dBToRegisterValue(LeftVol);
    RightRegVal = dBToRegisterValue(RightVol);

    AudioDAC_VolSet(DACModule, LeftRegVal, RightRegVal);
}
