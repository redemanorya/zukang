/**
 **************************************************************************************
 * @file    dac_interface.h
 * @brief   audio dac interface
 *
 * @author  Sam
 * @version V1.1.0
 *
 * $Created: 2015-09-23 14:01:05$
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#ifndef		__DAC_INTERFACE_H__
#define		__DAC_INTERFACE_H__

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus 
#include "dac.h"
#include "DMA.h"

/**
 * @brief  Audio DAC Init
 * @param DACModule: DAC0_MODULE / DAC1_MODULE / DAC2_MODULE / DAC0_DAC1_MODULE / 
 * 					DAC0_DAC2_MODULE /DAC1_DAC2_MODULE / DAC0_DAC1_DAC2_MODULE
 * @return NONE
 * @Note 确定好需要使用的DACMODULE模块,一次性init完成,能减少pop声的发生
 */
void AudioDAC_Init(AUDIODAC_MODULE DACMODULE, AUDIODAC_ParamCt *DAC0, AUDIODAC_ParamCt *DAC1, AUDIODAC_ParamCt *DAC2);

/**
 * @brief  Audio DAC Digital DeInit
 * @param  DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @return NONE
 */
void AudioDAC_DeInit(AUDIODAC_MODULE DACMODULE);

void AudioDAC_SampleRateChange(AUDIODAC_MODULE DACMODULE, uint32_t SampleRate);

uint16_t AudioDAC0_DataSet(void* Buf, uint16_t Len);
uint16_t AudioDAC0_DataSpaceLenGet(void);
uint16_t AudioDAC1_DataSet(void* Buf, uint16_t Len);
uint16_t AudioDAC1_DataSpaceLenGet(void);
uint16_t AudioDAC2_DataSet(void* Buf, uint16_t Len);
uint16_t AudioDAC2_DataSpaceLenGet(void);
uint16_t AudioDAC0_DataLenGet();
uint16_t AudioDAC1_DataLenGet();
uint16_t AudioDAC2_DataLenGet();
/**
 * @brief  设置DAC模块MCLK整数分频比：DAC_Mclk = PLL_Mclk/Div
 * @param  Module : AUDIODAC_MODULE，选择DAC模块
 * @param  Div : 分频系数
 * @return 无
 */
void AudioDAC_MclkDivSet(AUDIODAC_MODULE Module,uint32_t Div);
uint16_t dBToRegisterValue(int16_t dB);
//uint16_t AudioDAC0_DataSpaceLenGet(void);
//uint16_t AudioDAC0_DataSet(void* Buf, uint16_t Len);
//uint16_t AudioDAC0_DataLenGet(void);

/**
 * @brief  ADC 模块音量设置
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  LeftVol      左声道音量值，0x00:无增益, 范围：-722(-72.2dB) — 120(12dB)
 * @param  RightVol     右声道音量值，0x00:无增益, 范围：-722(-72.2dB) — 120(12dB)
 * @return 无
 */
void AudioDAC_VolGainSet_For_dB(AUDIODAC_MODULE DACModule, int16_t LeftVol, int16_t RightVol);

#ifdef __cplusplus
}
#endif // __cplusplus 

#endif //__DAC_INTERFACE_H__

