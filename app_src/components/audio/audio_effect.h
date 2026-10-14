/*******************************************************
 *         MVSilicon Audio Effects Process
 *
 *                All Right Reserved
 *******************************************************/

#ifndef __AUDIO_EFFECT_H__
#define __AUDIO_EFFECT_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#include "audio_core_api.h"
#include "audio_effect_control.h"

//音效Apply抽象函数指针
//typedef void (*AudioEffectApplyFunc)(void *effectUint, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);

void AudioMusicProcess(AudioCoreContext *pAudioCore, uint8_t Net);
void AudioBypassProcess(AudioCoreContext *pAudioCore, uint8_t Net);
void AudioEffectProcessBTHF(AudioCoreContext *pAudioCore, uint8_t Net);
void AudioNoAppProcess(AudioCoreContext *pAudioCore, uint8_t Net);
uint8_t AudioMsgProccess(uint16_t Msg);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__AUDIO_EFFECT_H__
