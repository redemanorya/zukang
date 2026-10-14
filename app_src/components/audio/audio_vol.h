/**
 **************************************************************************************
 * @file    audio_vol.h
 * @brief   audio syetem vol set here
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2016-1-7 15:42:47$
 *
 * @copyright Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#ifndef __AUDIO_VOL_H__
#define __AUDIO_VOL_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#include "app_config.h"
#include "mode_task.h"
#include "audio_effect_parambin.h"

// “Ù¡ø Preset ±‡“Î∆⁄—°‘Ò
#if (BT_AVRCP_VOLUME_SYNC) && defined(CFG_APP_BT_MODE_EN)
#define VOL_PRESET PRESET_GAIN_BT_VOL
#else
#define VOL_PRESET PRESET_GAIN_VOL
#endif

void AudioAPPDigitalGianProcess(AudioEffectParambinContext *AudioEffectParambin, SysModeNumber AppMode);
bool IsAudioPlayerMute(void);
void AudioPlayerMenuCheck(void);
void AudioMusicVolSet(uint8_t musicVol);
void AudioHfVolSet(uint8_t HfVol);

uint8_t AudioMusicVolGet(void);
void AudioMusicVolDown(AudioEffectParambinContext *AudioEffectParambin);
void AudioMusicVolUp(AudioEffectParambinContext *AudioEffectParambin);
void SystemVolUp(void);
void SystemVolDown(void);
void SystemVolSet(void);
void SystemVolSync(void);

uint8_t BtAbsVolume2VolLevel(uint8_t absValue);
uint8_t BtLocalVolLevel2AbsVolme(uint8_t localValue);
void AudioMicVolDown(AudioEffectParambinContext *AudioEffectParambin);
void AudioMicVolUp(AudioEffectParambinContext *AudioEffectParambin);
void HardWareMuteOrUnMute(void);

void HardWareMuteOrUnMute(void);

void AudioBassTrebAjust(AudioEffectParambinContext *AudioEffectParambin, AUDIO_EFFECT_NAME_NUM effect_index, AUDIO_EFFECT_PRESET_NUM preset_index);
void AudioEchoReverbAjust(AudioEffectParambinContext *AudioEffectParambin, AUDIO_EFFECT_NAME_NUM effect_index, AUDIO_EFFECT_PRESET_NUM preset_index);
uint32_t AudioVol_MaxLevel_Get(AudioEffectParambinContext *AudioEffectParambin, AUDIO_EFFECT_NAME_NUM effect_index, AUDIO_EFFECT_PRESET_NUM preset_index);
void AudioMusicVolSync(uint8_t musicVol);

#ifdef  CFG_APP_HDMIIN_MODE_EN
void HDMISourceMute(void);
void HDMISourceUnmute(void);
bool IsHDMISourceMute(void);
#endif

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif

