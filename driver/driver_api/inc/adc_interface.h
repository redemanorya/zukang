/**
 **************************************************************************************
 * @file    adc_interface.h
 * @brief   audio adc interface
 *
 * @author  Sam
 * @version V1.1.0
 *
 * $Created: 2015-09-23 14:01:05$
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#ifndef		__ADC_INTERFACE_H__
#define		__ADC_INTERFACE_H__

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus 
#include "type.h"
#include "audio_adc.h"
#include "dma.h"

typedef struct __AUDIOADC_Ana
{
	AUDIOADC_INPUT			INPUT;
	AUDIOADC_Mode			Mode;
	uint16_t				PGAGain;
}AUDIOADC_Ana;

typedef struct __AUDIOADC_ParamCt
{	
	uint8_t					MICBias_EN;  //0:MICBias1/2 disable; 1:MICBias1 enable; 2:MICBias2 enable; 3:All MICBias enable
	AUDIOADC_Ana			AUDIOADC_AnaLeft;
	AUDIOADC_Ana			AUDIOADC_AnaRight;
	uint32_t 				SampleRate;
	AUDIOADC_EnergyModel 	ADCEnergyModel;
	AUDIOADC_BitWidth 		ADCBitWidth;
	void *					ADCFifo;
	uint16_t         		ADCFifoLen;
} AUDIOADC_ParamCt;

void AudioADC_DMARestart(AUDIOADC_MODULE Module, void* Buf, uint16_t Len);
void AudioADC_DigitalInit(AUDIOADC_MODULE Module, AUDIOADC_ParamCt *ct);
void AudioADC_DeInit(AUDIOADC_MODULE Module);
void AudioADC_AnaInit(AUDIOADC_MODULE ADCMODULE, AUDIOADC_ParamCt *ct);
void AudioADC_AnaDeInit(AUDIOADC_MODULE ADCMODULE);
void AudioADC_Init(AUDIOADC_MODULE ADCMODULE, AUDIOADC_ParamCt *ct);

uint16_t AudioADC0_DataLenGet();
uint16_t AudioADC1_DataLenGet();

uint16_t AudioADC0_DataGet(void* Buf, uint16_t Len);
uint16_t AudioADC1_DataGet(void* Buf, uint16_t Len);

void AudioADC_SampleRateChange(AUDIOADC_MODULE ADCMODULE, uint32_t SampleRate);

/**
 * @brief  设置ADC模块MCLK整数分频比：ADC_Mclk = PLL_Mclk/Div
 * @param  ADCMODULE : AUDIOADC_MODULE，选择ADC模块
 * @param  Div : 分频系数
 * @return 无
 */
void AudioADC_MclkDivSet(AUDIOADC_MODULE ADCMODULE, uint32_t Div);

/**
 * @brief  ADC 模块音量设置
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  LeftVol      左声道音量值，0x00:无增益, 范围：-722(-72.2dB) — 120(12dB)
 * @param  RightVol     右声道音量值，0x00:无增益, 范围：-722(-72.2dB) — 120(12dB)
 * @return 无
 */
void AudioADC_VolGainSet_For_dB(AUDIOADC_MODULE ADCModule, int16_t LeftVol, int16_t RightVol);

/**
 * @brief  ADC 模块音量设置（左右声道独立设置）
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  ChannelSel   左右声道选择，0x00: 无；0x1：左声道；0x2：右声道
 * @param  Vol     		音量值，0x00:无增益, 范围：-722(-72.2dB) — 120(12dB)
 * @note   当ChannelSel为3时，同时选择了左右声道，此时Vol配置对左右声道均有效，左右声道音量值一致
 * @return 无
 */
void AudioADC_VolSetChannel_For_dB(AUDIOADC_MODULE ADCModule, AUDIOADC_CHANNEL ChannelSel, int16_t Vol);

#ifdef __cplusplus
}
#endif // __cplusplus 

#endif //__ADC_INTERFACE_H__

