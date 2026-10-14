/**
 *****************************************************************************
 * @file     otg_device_audio_api.h
 * @author   Owen
 * @version  V1.0.0
 * @date     24-June-2015
 * @brief    audio device interface
 *****************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
 */

#ifndef __OTG_DEVICE_AUDIO_API_H__
#define	__OTG_DEVICE_AUDIO_API_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus
	
#include "type.h"
#include "otg_device_audio.h"

extern const uint32_t Uac10SpeakerRates[FREQ_NUM_MAX];
extern const uint32_t Uac10Speaker1Rates[FREQ_NUM_MAX];
extern const uint32_t Uac10MicRates[FREQ_NUM_MAX];
extern const uint32_t Uac20SpeakerRates[FREQ_NUM_MAX];
extern const uint32_t Uac20Speaker1Rates[FREQ_NUM_MAX];
extern const uint32_t Uac20MicRates[FREQ_NUM_MAX];

// 公共 API 函数
void UsbAudioPcmIsoToInternal(uint8_t *buf, uint32_t pcmNum, uint8_t byteSet);
void UsbAudioPcmInternalToIso(uint8_t *buf, uint32_t pcmNum, uint8_t byteSet);

uint16_t UsbAudioBufDataGet(UsbAudio *audio, void *Buffer, uint16_t Len);
uint16_t UsbAudioBufDataLen(UsbAudio *audio);
uint16_t UsbAudioBufDataSet(UsbAudio *audio, void *Buffer, uint16_t Len);
uint16_t UsbAudioBufSpaceLen(UsbAudio *audio);
uint16_t UsbAudioBufDepth(UsbAudio *audio, uint8_t channels);

void UsbAudioGainApply(UsbAudio *audio, void *Buffer, uint32_t samples);
void UsbAudioSampleRateInit(UsbAudio *audio,const uint32_t *rates, uint8_t count);
int16_t UsbValToMcuGain(int16_t UsbVal);
void UsbAudioVolumeInit(UsbAudio *audio, int16_t volume);

/**
 * @brief 获取指定通道的 USB 音量 DB 值
 * @param audio   UsbAudio 实例指针
 * @param channel 通道索引 (0 ~ audio->Channels-1)
 * @return USB 音量值（单位  dB；若通道无效返回 0）
 */
int16_t UsbAudioGetChannelDb(UsbAudio *audio, uint8_t channel);
#ifdef  __cplusplus
}
#endif//__cplusplus

#endif
