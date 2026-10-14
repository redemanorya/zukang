/**
 **************************************************************************************
 * @file    audio_mixer.h
 * @brief   
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2025-09-04 10:32:38$
 *
 * @copyright Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

/**
 * @addtogroup AUDIOMIXER
 * @{
 * @defgroup audio_mixer audio_mixer.h
 * @{
 */

#ifndef __AUDIO_MIXER_H__
#define __AUDIO_MIXER_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#include "type.h"

//时钟和DA或者I2S同步
typedef enum __MIXER_CLOCK_MODULE{
	AUDIOMixer_CLK_DAC0 = 0,
	AUDIOMixer_CLK_DAC1,
	AUDIOMixer_CLK_DAC2,
	AUDIOMixer_CLK_I2S0,
	AUDIOMixer_CLK_I2S1,
	AUDIOMixer_CLK_I2S2,
}MIXER_CLOCK_MODULE;

typedef enum __MIXER_WIDTH{
	AUDIOMixer_16bit = 0,
	AUDIOMixer_24bit,
	AUDIOMixer_32bit,
}MIXER_WIDTH;

/**
 * @brief  音频硬件混音模块使能
 * @param  无
 * @return 无
 */
void AudioMixer_Enable(void);
/**
 * @brief  音频硬件混音模块禁能
 * @param  无
 * @return 无
 */
void AudioMixer_Disable(void);
/**
 * @brief  音频硬件混音模块时钟配置
 * @param  Clk  时钟源来源于指定外设
 * @return 无
 */
void AudioMixer_ClockConfig(MIXER_CLOCK_MODULE Clk);

/**
 * @brief  音频硬件混音模块采样率获取
 * @param  无
 * @return 采样率频率
 */
uint32_t AudioMixer_SampleRateGet(void);

/**
 * @brief  音频硬件混音模块采样率设置
 * @param  SampleRate 采样率频率
 * @return 无
 */
void AudioMixer_SampleRateSet(uint32_t SampleRate);

/**
 * @brief  音频硬件混音模块配置
 * @param  width 位宽设置
 * @param  InChannels 2：2外设混音；3：3外设混音
 * @param  OutChannels 1：1路输出音频；2：2路输出音频
 * @return 无
 */
void AudioMixer_Config(MIXER_WIDTH width, uint32_t InChannels, uint32_t OutChannels);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//AUDIO_MIXER

/**
 * @}
 * @}
 */
