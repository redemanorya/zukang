/**
 **************************************************************************************
 * @file    audio_effect_control.h
 * @brief   Audio Effect control API
 *
 * @author  Yangyu
 * @version V1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __AUDIO_EFFECT_CONTROL_H__
#define __AUDIO_EFFECT_CONTROL_H__
#include "app_config.h"
#include "audio_effect_parambin.h"
#include "audio_core_adapt.h"

typedef enum _EQ_MODE
{
    EQ_MODE_FLAT,
	EQ_MODE_CLASSIC,
	EQ_MODE_POP,
	EQ_MODE_ROCK,
	EQ_MODE_JAZZ,
	EQ_MODE_VOCAL_BOOST,
} EQ_MODE;

#ifndef ROBOEFFECT_DOUBLE_ENGINE_EN
	extern const char *effect_flow_name[];
	extern 	uint8_t AvailableNUM;
#else
	extern const char *effect_flow_name[][2];
	extern 	uint8_t AvailableNUM;
#endif

void AudioEffect_Parambin_EffectModeAutoSwitch(void);

void AudioEffect_Parambin_EffectModeSwitch(void);

void AudioEffect_Parambin_EffectParamSwitch(void);

void AudioEffect_EffectMode_Refresh(void);

void AudioEffect_Params_Sync(AudioEffectParambinContext *AudioEffectParambin);

uint8_t AudioEffect_EqMode_Set(AudioEffectParambinContext *AudioEffectParambin, uint8_t EqMode);

uint8_t AudioEffect_effect_status_Get(AUDIO_EFFECT_NAME_NUM effect_index);

void AudioEffect_effect_enable(AUDIO_EFFECT_NAME_NUM effect_index, uint8_t enable);

void AudioEffect_Msg_Process(AudioEffectParambinContext *AudioEffectParambin, uint32_t Msg);

void AudioEffect_ParamSwitch(void);

bool AudioEffect_Msg_Check(uint16_t *Msg);

void AudioEffect_UpdateSamplerateFramesize(AudioEffectParambinContext *AudioEffectParambin, uint32_t sample_rate, uint32_t frame_size);

uint32_t AudioEffect_GetUserEffectParamCRCLen(AudioEffectParambinContext *AudioEffectParambin);

uint32_t AudioEffect_GetUserHWCfgCRCLen(uint8_t *effect_param);

void AudioCoreFrameSampleSync(AudioEffectParambinContext *AudioEffectParambin, uint8_t net);

uint8_t AudioCoreGetNetBySourceChannelNUM(AUDIO_CORE_SOURCE_NUM ChannelNUM);

uint8_t AudioCoreGetNetBySinkChannelNUM(AUDIO_CORE_SINK_NUM ChannelNUM);

void AudioEffect_Restart(void);

void AudioEffect_FlowSwitch(void);

uint16_t AudioEffect_GetTotalFlowNum(void);
#endif //__AUDIO_EFFECT_CONTROL_H__
