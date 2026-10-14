#include <string.h>
#include "type.h"
#include "audio_adc.h"
#include "dma.h"
#include "clk.h"
#include "debug.h"
#include "adc_interface.h"
#include "dac_interface.h"
#include "gpio.h"
#include "chip_info.h"

void AudioADC_DigitalInit(AUDIOADC_MODULE Module, AUDIOADC_ParamCt *ct)
{
	//音频时钟使能，其他模块可能也会开启
	Clock_AudioPllClockSet(APLL_CLK_MODE, PLL_CLK_1, AUDIO_PLL_CLK1_FREQ);
	Clock_AudioPllClockSet(APLL_CLK_MODE, PLL_CLK_2, AUDIO_PLL_CLK2_FREQ);
	Clock_AudioPllClockSet(APLL_CLK_MODE, PLL_CLK_3, AUDIO_PLL_CLK3_FREQ);
	Clock_AudioPllClockSet(APLL_CLK_MODE, PLL_CLK_4, AUDIO_PLL_CLK4_FREQ);

	if(Module == ADC0_MODULE)
	{
		Clock_Module2Enable(ASDM_REG_CLK_EN);
		Clock_Module2Enable(ASDM0_CLK_EN);
	}
	else
	{
		Clock_Module2Enable(ASDM_REG_CLK_EN);
		Clock_Module2Enable(ASDM1_CLK_EN);
	}

	AUDIO_MODULE AUDIOMODULE;

	if(Module == ADC0_MODULE)
	{
		AUDIOMODULE = AUDIO_ADC0;
	}
	else
	{
		AUDIOMODULE = AUDIO_ADC1;
	}

	if(IsSelectMclkClk1(ct->SampleRate))
	{
		Clock_AudioMclkSel(AUDIOMODULE, PLL_CLOCK1);
	}
	else
	{
		Clock_AudioMclkSel(AUDIOMODULE, PLL_CLOCK2);
	}

	if(ct->SampleRate > 48000)
	{
		AudioADC_MclkDivSet(Module, 1);
	}
	else
	{
		AudioADC_MclkDivSet(Module, 2);
	}

	if(Module == ADC0_MODULE)
    {
    	AudioADC_Disable(ADC0_MODULE);
    	DMA_InterruptFlagClear(PERIPHERAL_1_ID_AUDIO_ADC0_RX, DMA_DONE_INT);
    	DMA_InterruptFlagClear(PERIPHERAL_1_ID_AUDIO_ADC0_RX, DMA_THRESHOLD_INT);
    	DMA_InterruptFlagClear(PERIPHERAL_1_ID_AUDIO_ADC0_RX, DMA_ERROR_INT);
		DMA_ChannelDisable(PERIPHERAL_1_ID_AUDIO_ADC0_RX);
        DMA_CircularConfig(PERIPHERAL_1_ID_AUDIO_ADC0_RX, ct->ADCFifoLen/2, ct->ADCFifo, ct->ADCFifoLen);
        if(AudioADC_IsOverflow(ADC0_MODULE))
		{
        	AudioADC_OverflowClear(ADC0_MODULE);
		}

    	AudioADC_SampleRateSet(ADC0_MODULE, ct->SampleRate);

    	if(ct->SampleRate <= 48000)
    	{
    		AudioADC_HighPassFilterConfig(ADC0_MODULE, 0x3FF8);
    	}
    	else if(ct->SampleRate > 48000 && ct->SampleRate <= 96000)
    	{
    		AudioADC_HighPassFilterConfig(ADC0_MODULE, 0x3FFC);
    	}
    	else if(ct->SampleRate > 96000 && ct->SampleRate <= 192000)
    	{
    		AudioADC_HighPassFilterConfig(ADC0_MODULE, 0x3FFE);
    	}
    	else if(ct->SampleRate > 192000 && ct->SampleRate <= 384000)
    	{
    		AudioADC_HighPassFilterConfig(ADC0_MODULE, 0x3FFF);
    	}

    	AudioADC_HighPassFilterSet(ADC0_MODULE, TRUE);
        AudioADC_LREnable(ADC0_MODULE, 1, 1);
        AudioADC_FadeTimeSet(ADC0_MODULE, 10);
        AudioADC_FadeEnable(ADC0_MODULE);
        AudioADC_VolSet(ADC0_MODULE, 0x1000, 0x1000);
        AudioADC_SoftMute(ADC0_MODULE, FALSE, FALSE);
        AudioADC_Clear(ADC0_MODULE);
        AudioADC_BitWidthSet(ADC0_MODULE, ct->ADCBitWidth);
        AudioADC_Enable(ADC0_MODULE);

        DMA_ChannelEnable(PERIPHERAL_1_ID_AUDIO_ADC0_RX);
    }
    else if(Module == ADC1_MODULE)
    {
    	AudioADC_Disable(ADC1_MODULE);
    	DMA_InterruptFlagClear(PERIPHERAL_1_ID_AUDIO_ADC1_RX, DMA_DONE_INT);
    	DMA_InterruptFlagClear(PERIPHERAL_1_ID_AUDIO_ADC1_RX, DMA_THRESHOLD_INT);
    	DMA_InterruptFlagClear(PERIPHERAL_1_ID_AUDIO_ADC1_RX, DMA_ERROR_INT);
		DMA_ChannelDisable(PERIPHERAL_1_ID_AUDIO_ADC1_RX);
        DMA_CircularConfig(PERIPHERAL_1_ID_AUDIO_ADC1_RX, ct->ADCFifoLen/2, ct->ADCFifo, ct->ADCFifoLen);
    	if(AudioADC_IsOverflow(ADC1_MODULE))
    	{
    		AudioADC_OverflowClear(ADC1_MODULE);
    	}

    	AudioADC_SampleRateSet(ADC1_MODULE, ct->SampleRate);

    	if(ct->SampleRate <= 48000)
    	{
    		AudioADC_HighPassFilterConfig(ADC1_MODULE, 0x3FF8);
    	}
    	else if(ct->SampleRate > 48000 && ct->SampleRate <= 96000)
    	{
    		AudioADC_HighPassFilterConfig(ADC1_MODULE, 0x3FFC);
    	}
    	else if(ct->SampleRate > 96000 && ct->SampleRate <= 192000)
    	{
    		AudioADC_HighPassFilterConfig(ADC1_MODULE, 0x3FFE);
    	}
    	else if(ct->SampleRate > 192000 && ct->SampleRate <= 384000)
    	{
    		AudioADC_HighPassFilterConfig(ADC1_MODULE, 0x3FFF);
    	}

        AudioADC_HighPassFilterSet(ADC1_MODULE, TRUE);
        AudioADC_LREnable(ADC1_MODULE, 1, 1);
        AudioADC_FadeTimeSet(ADC1_MODULE, 10);
        AudioADC_FadeEnable(ADC1_MODULE);
        //AudioADC_FadeDisable(ADC1_MODULE);
        AudioADC_VolSet(ADC1_MODULE, 0x1000, 0x1000);
        AudioADC_SoftMute(ADC1_MODULE, FALSE, FALSE);
        AudioADC_Clear(ADC1_MODULE);
        AudioADC_BitWidthSet(ADC1_MODULE, ct->ADCBitWidth);
        AudioADC_Enable(ADC1_MODULE);

        DMA_ChannelEnable(PERIPHERAL_1_ID_AUDIO_ADC1_RX);
    }
}

void AudioADC_AnaInit(AUDIOADC_MODULE ADCMODULE, AUDIOADC_ParamCt *ct)
{
	if(ct->AUDIOADC_AnaLeft.INPUT == ADC1_AUX0L_B20 && Chip_Version() == CHIP_VERSION_ECO0)
	{
		GPIO_RegBitsSet(GPIO_B_ANA_EN, GPIOB20);
	}
	else if(ct->AUDIOADC_AnaLeft.INPUT == ADC1_AUX1L_B18 && Chip_Version() == CHIP_VERSION_ECO0)
	{
		GPIO_RegBitsSet(GPIO_B_ANA_EN, GPIOB18);
	}

	if(ct->AUDIOADC_AnaRight.INPUT == ADC1_AUX0R_B21 && Chip_Version() == CHIP_VERSION_ECO0)
	{
		GPIO_RegBitsSet(GPIO_B_ANA_EN, GPIOB21);
	}
	else if(ct->AUDIOADC_AnaRight.INPUT == ADC1_AUX1R_B19 && Chip_Version() == CHIP_VERSION_ECO0)
	{
		GPIO_RegBitsSet(GPIO_B_ANA_EN, GPIOB19);
	}

	if(ct->MICBias_EN == 1)
	{
		AudioADC_BIASPowerOn(MICBIAS1);
		AudioADC_BIASPowerOff(MICBIAS2);
	}
	else if(ct->MICBias_EN == 2)
	{
		AudioADC_BIASPowerOff(MICBIAS1);
		AudioADC_BIASPowerOn(MICBIAS2);
	}
	else if(ct->MICBias_EN == 3)
	{
		AudioADC_BIASPowerOn(MICBIAS1);
		AudioADC_BIASPowerOn(MICBIAS2);
	}
	else
	{
		AudioADC_BIASPowerOff(MICBIAS1);
		AudioADC_BIASPowerOff(MICBIAS2);
	}
	
	AudioADC_AnaPowerup(ADCMODULE, ct->ADCEnergyModel);
	AudioADC_PGASel(ADCMODULE, ADC_LEFT, ADC_NONE);
	AudioADC_PGASel(ADCMODULE, ADC_RIGHT, ADC_NONE);

	if(ct->AUDIOADC_AnaLeft.INPUT != ADC_NONE)
	{
		AudioADC_PGASel(ADCMODULE, ADC_LEFT, ct->AUDIOADC_AnaLeft.INPUT);
		AudioADC_PGAGainSet(ADCMODULE, ADC_LEFT, ct->AUDIOADC_AnaLeft.INPUT, ct->AUDIOADC_AnaLeft.PGAGain);
		AudioADC_PGAMode(ADCMODULE, ADC_LEFT, ct->AUDIOADC_AnaLeft.Mode);  //差分or单端
	}

	if(ct->AUDIOADC_AnaRight.INPUT != ADC_NONE)
	{
		AudioADC_PGASel(ADCMODULE, ADC_RIGHT, ct->AUDIOADC_AnaRight.INPUT);
		AudioADC_PGAGainSet(ADCMODULE, ADC_RIGHT, ct->AUDIOADC_AnaRight.INPUT, ct->AUDIOADC_AnaRight.PGAGain);
		AudioADC_PGAMode(ADCMODULE, ADC_RIGHT, ct->AUDIOADC_AnaRight.Mode);  //差分or单端
	}
}

void AudioADC_Init(AUDIOADC_MODULE ADCMODULE, AUDIOADC_ParamCt *ct)
{
	AudioADC_AnaInit(ADCMODULE, ct);
	AudioADC_DigitalInit(ADCMODULE, ct);
}

void AudioADC_AnaDeInit(AUDIOADC_MODULE ADCMODULE)
{
    if(ADCMODULE == ADC0_MODULE)
    {
        AudioADC_PGAPowerUp(ADC0_MODULE, 0, 0);
        AudioADC_PGASel(ADC0_MODULE, ADC_LEFT, ADC_NONE);
        AudioADC_PGASel(ADC0_MODULE, ADC_RIGHT, ADC_NONE);
    }
    else
    {
        AudioADC_PGAPowerUp(ADC1_MODULE, 0, 0);
        AudioADC_PGASel(ADC1_MODULE, ADC_LEFT, ADC_NONE);
        AudioADC_PGASel(ADC1_MODULE, ADC_RIGHT, ADC_NONE);
    }
}

void AudioADC_DigitalDeInit(AUDIOADC_MODULE ADCMODULE)
{
	uint8_t PeripheralID = 0;

	AudioADC_Disable(ADCMODULE);
	AudioADC_FuncReset(ADCMODULE);

	if(ADCMODULE == ADC0_MODULE)
	{
		PeripheralID = PERIPHERAL_1_ID_AUDIO_ADC0_RX;
	}
	else if(ADCMODULE == ADC1_MODULE)
	{
		PeripheralID = PERIPHERAL_1_ID_AUDIO_ADC1_RX;
	}

	DMA_InterruptFlagClear(PeripheralID, DMA_DONE_INT);
	DMA_InterruptFlagClear(PeripheralID, DMA_THRESHOLD_INT);
	DMA_InterruptFlagClear(PeripheralID, DMA_ERROR_INT);
	DMA_CircularFIFOClear(PeripheralID);
	DMA_ChannelDisable(PeripheralID);
}

void AudioADC_DeInit(AUDIOADC_MODULE Module)
{
	AudioADC_DigitalDeInit(Module);
	AudioADC_AnaDeInit(Module);
}

//返回 uint：sample
uint16_t AudioADC0_DataLenGet()
{
	uint16_t NumSamples = 0;

	NumSamples = DMA_CircularDataLenGet(PERIPHERAL_1_ID_AUDIO_ADC0_RX);
	if(AudioADC_BitWidthGet(ADC0_MODULE) == ADC_16BIT)
		return NumSamples / 4;
	else
		return NumSamples / 8;
}

uint16_t AudioADC1_DataLenGet()
{
	uint16_t NumSamples = 0;

	NumSamples = DMA_CircularDataLenGet(PERIPHERAL_1_ID_AUDIO_ADC1_RX);
	if(AudioADC_BitWidthGet(ADC1_MODULE) == ADC_16BIT)
		return NumSamples / 4;
	else
		return NumSamples / 8;
}


//这一部分驱动需要更新一下,现在已经有了符号位，这个因该是就不需要再移位去赋值符号位
//返回Length uint：sample
uint16_t AudioADC0_DataGet(void* Buf, uint16_t Len)
{
	uint16_t Length = 0;//Samples

    Length = DMA_CircularDataLenGet(PERIPHERAL_1_ID_AUDIO_ADC0_RX);

    if(AudioADC_BitWidthGet(ADC0_MODULE) == ADC_16BIT)
    {
		if(Length > Len * 4)
		{
			Length = Len * 4;
		}
		DMA_CircularDataGet(PERIPHERAL_1_ID_AUDIO_ADC0_RX, Buf, Length & 0xFFFFFFFC);
		return Length / 4;
    }
    else
    {
		if(Length > Len * 8)
		{
			Length = Len * 8;
		}
		DMA_CircularDataGet(PERIPHERAL_1_ID_AUDIO_ADC0_RX, Buf, Length & 0xFFFFFFF8);

		return Length / 8;
    }
}

//此处后续可能要做处理，区分单声道和立体声,数据格式转换在外边处理
//和DMA的配置也会有关系
//返回Length uint：sample
uint16_t AudioADC1_DataGet(void* Buf, uint16_t Len)
{
	uint16_t Length = 0;//Samples

    Length = DMA_CircularDataLenGet(PERIPHERAL_1_ID_AUDIO_ADC1_RX);

    if(AudioADC_BitWidthGet(ADC1_MODULE) == ADC_16BIT)
    {
    	if(Length > Len * 4)
    	{
    		Length = Len * 4;
    	}
    	DMA_CircularDataGet(PERIPHERAL_1_ID_AUDIO_ADC1_RX, Buf, Length & 0xFFFFFFFC);
    	return Length / 4;
    }
    else
    {
		if(Length > Len * 8)
		{
			Length = Len * 8;
		}
		DMA_CircularDataGet(PERIPHERAL_1_ID_AUDIO_ADC1_RX, Buf, Length & 0xFFFFFFF8);

		return Length / 8;
    }
}

void AudioADC_SampleRateChange(AUDIOADC_MODULE ADCMODULE, uint32_t SampleRate)
{
	if(ADCMODULE == ADC0_MODULE)
    {
    	if(IsSelectMclkClk1(SampleRate))
    	{
    		Clock_AudioMclkSel(AUDIO_ADC0, PLL_CLOCK1);
    	}
    	else
    	{
    		Clock_AudioMclkSel(AUDIO_ADC0, PLL_CLOCK2);
    	}
    }
    else if(ADCMODULE == ADC1_MODULE)
    {
    	if(IsSelectMclkClk1(SampleRate))
    	{
    		Clock_AudioMclkSel(AUDIO_ADC1, PLL_CLOCK1);
    	}
    	else
    	{
    		Clock_AudioMclkSel(AUDIO_ADC1, PLL_CLOCK2);
    	}
    }
	AudioADC_SampleRateSet(ADCMODULE, SampleRate);
}

void AudioADC_MclkDivSet(AUDIOADC_MODULE ADCMODULE, uint32_t Div)
{
	if(ADCMODULE == ADC0_MODULE)
    {
		Clock_ADC0ClkDivSet(Div - 1);
    }
	else if(ADCMODULE == ADC1_MODULE)
	{
		Clock_ADC1ClkDivSet(Div - 1);
	}
}

/**
 * @brief  ADC 模块音量设置
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  LeftVol      左声道音量值，0x00:无增益, 范围：-7220(-72.2dB) ~ +1200(12dB)
 * @param  RightVol     右声道音量值，0x00:无增益, 范围：-7220(-72.2dB) ~ +1200(12dB)
 * @return 无
 */
void AudioADC_VolGainSet_For_dB(AUDIOADC_MODULE ADCModule, int16_t LeftVol, int16_t RightVol) 
{
    uint16_t LeftRegVal, RightRegVal;

    LeftRegVal = dBToRegisterValue(LeftVol);
    RightRegVal = dBToRegisterValue(RightVol);

    AudioADC_VolSet(ADCModule, LeftRegVal, RightRegVal);
}

/**
 * @brief  ADC 模块音量设置（左右声道独立设置）
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  ChannelSel   左右声道选择，0x00: 无；0x1：左声道；0x2：右声道
 * @param  Vol     		音量值，0x00:无增益, 范围：-722(-72.2dB) — 120(12dB)
 * @note   当ChannelSel为3时，同时选择了左右声道，此时Vol配置对左右声道均有效，左右声道音量值一致
 * @return 无
 */
void AudioADC_VolSetChannel_For_dB(AUDIOADC_MODULE ADCModule, AUDIOADC_CHANNEL ChannelSel, int16_t Vol)
{
    uint16_t RegVal;

    RegVal = dBToRegisterValue(Vol);

    AudioADC_VolSetChannel(ADCModule, ChannelSel, RegVal);
}
