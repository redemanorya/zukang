/**
 **************************************************************************************
 * @file    audio_effect_parambin.h
 * @brief   Audio effect ctrl for Roboeffect param bin
 *
 * @author  Yangyu
 * @version V1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __AUDIO_EFFECT_PARAM_BIN_H__
#define __AUDIO_EFFECT_PARAM_BIN_H__
#include "roboeffect_api.h"
#include "effects_param_v3.h"
#include "core_d1088.h"
#include "flow_config.h"


typedef enum _AUDIO_CORE_SOURCE_NUM {
    #define XX_ENUM(name, str) name,
    AUDIO_SOURCE_ENUM_MAP(XX_ENUM)
    #undef XX_ENUM
    AUDIO_CORE_SOURCE_MAX_NUM
} AUDIO_CORE_SOURCE_NUM;

typedef enum _AUDIO_CORE_SINK_NUM {
    #define XX_ENUM(name, str) name,
    AUDIO_SINK_ENUM_MAP(XX_ENUM)
    #undef XX_ENUM
    AUDIO_CORE_SINK_MAX_NUM
} AUDIO_CORE_SINK_NUM;



typedef enum _AUDIO_EFFECT_NAME_NUM {
    #define XX_ENUM(name, str) name,
	AUDIO_EFFECT_NAME_ENUM_MAP(XX_ENUM)
    #undef XX_ENUM
	EFFECT_MAX
} AUDIO_EFFECT_NAME_NUM;



typedef enum _AUDIO_EFFECT_PRESET_NUM {
    #define XX_ENUM(name, str) name,
	AUDIO_EFFECT_PRESET_ENUM_MAP(XX_ENUM)
    #undef XX_ENUM
	PRESET_MAX
} AUDIO_EFFECT_PRESET_NUM;

typedef struct {
    int enum_val;
    const char *enum_str;
} EnumStringMap;
/**************************** source + sink + effect_name end*********************************************/

//total data length  	---- 2 Bytes
//Effect Version		---- 3 Bytes
//Roboeffect Version  	---- 3 Bytes
//3rd part data length	---- 2 Bytes
//3rd part data
//MV effect data
#define GET_USER_EFFECT_PARAMETERS_TOTAL_LEN(p) 	((uint16_t)(((uint16_t)((p)[1] << 8) | (p)[0]) + 2))
#define GET_USER_EFFECT_PARAMETERS_THIRDDATA_LEN(p) ((uint16_t)((uint16_t)((p)[9] << 8) | (p)[8]))
#define GET_USER_EFFECT_PARAMETERS_DATA_LEN(p)		(GET_USER_EFFECT_PARAMETERS_TOTAL_LEN(p) - 8 - GET_USER_EFFECT_PARAMETERS_THIRDDATA_LEN(p) - 2)
#define GET_USER_EFFECT_PARAMETERS_EFFECTDATA_P(p) 	((uint8_t*)((p) + 8 + GET_USER_EFFECT_PARAMETERS_THIRDDATA_LEN(p) + 2))

typedef struct _AudioEffectParambinContext
{
	uint8_t *context_memory;
	roboeffect_effect_list_info *user_effect_list;
	roboeffect_exec_effect_info *user_effect_list_arry;
	const roboeffect_effect_steps_table *user_effect_steps;
	unsigned char *user_effect_parameters;
	const unsigned char *user_codec_parameters;   //只是指针，实际硬件参数需要接续
	uint32_t user_codec_parameters_len;
	const unsigned char *user_flow_script;
	const roboeffect_io_unit *source_io;
	const roboeffect_io_unit *sink_io;
	uint32_t user_flow_script_len;
	const uint32_t *steps_flow;
	uint8_t *sub_type_data;
	uint8_t *flow_data;
	uint32_t flow_size;
	int32_t audioeffect_memory_size;
//	uint16_t flow_index;
//	uint16_t param_mode_index;
}AudioEffectParambinContext;

	extern AudioEffectParambinContext EffectParambin;
#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
	extern AudioEffectParambinContext EffectParambin2;
#endif

typedef struct _AudioEffect_parambin_mode1_brief
{
	uint16_t item_type;
	uint16_t item_len;
	uint16_t mode_type_cnt;
	uint16_t preset_type_cnt;
	uint32_t base_address;
} AudioEffect_parambin_mode1_brief;

uint32_t AudioEffect_Parambin_GetFlashEffectAddr(void);

void *AudioEffect_Parambin_GetSourceBuffer(AudioEffectParambinContext *AudioEffectParambin, AUDIO_CORE_SOURCE_NUM source_index);

void *AudioEffect_Parambin_GetSinkBuffer(AudioEffectParambinContext *AudioEffectParambin, AUDIO_CORE_SINK_NUM sink_index);

void *AudioEffect_Parambin_GetSourceBuffer_Info(AudioEffectParambinContext *AudioEffectParambin, AUDIO_CORE_SOURCE_NUM source_index, uint8_t *Width, uint8_t *Ch);

void *AudioEffect_Parambin_GetSinkBuffer_Info(AudioEffectParambinContext *AudioEffectParambin, AUDIO_CORE_SINK_NUM sink_index, uint8_t *Width, uint8_t *Ch);

int8_t AudioEffect_Parambin_GetSourceEnum(const char *str);

int8_t AudioEffect_Parambin_GetSinkEnum(const char *str);

uint16_t AudioEffect_Parambin_GetTotalFlowNum(void);

uint16_t AudioEffect_Parambin_GetTotalParamNumByName(const char *name);
//uint16_t AudioEffect_Parambin_GetTotalParamNumInFlow(uint16_t flow_index);

bool AudioEffect_Parambin_Init(AudioEffectParambinContext *AudioEffectParambin, const char *name, uint16_t param_index, bool MemorySet);
//bool AudioEffect_Parambin_Init(AudioEffectParambinContext *AudioEffectParambin, const char *name, bool MemorySet);

void AudioEffect_Parambin_Deinit(AudioEffectParambinContext *AudioEffectParambin);

const void *AudioEffect_Parambin_GetCurHardwareConfig(AudioEffectParambinContext *AudioEffectParambin);

const void *AudioEffect_Parambin_GetCurEffectParams(AudioEffectParambinContext *AudioEffectParambin);

void AudioEffect_Parambin_RefreshSourceSinkBuf(AudioEffectParambinContext *AudioEffectParambin);

uint8_t *AudioEffect_Parambin_GetTempEffectParamByAddr(AudioEffectParambinContext *AudioEffectParambin, uint8_t effect_addr);

uint8_t *AudioEffect_Parambin_GetFlashEffectParamByAddr(AudioEffectParambinContext *AudioEffectParambin, uint8_t effect_addr);

void AudioEffect_Parambin_UpdateFlashEffectParamsByAddr(AudioEffectParambinContext *AudioEffectParambin, uint8_t effect_addr, uint8_t param_index, uint8_t *param_input);

void AudioEffect_Parambin_UpdateFlashEffectParams(AudioEffectParambinContext *AudioEffectParambin);

void AudioEffect_Parambin_UpdateTempEffectParamsByAddr(AudioEffectParambinContext *AudioEffectParambin, uint8_t effect_addr, uint8_t param_index, uint8_t *param_input);

void AudioEffect_Parambin_SwitchEffectByIndex(int16_t flow_index, int16_t param_mode_index0, int16_t param_mode_index1);
//void AudioEffect_Parambin_SwitchEffectModeByIndex(uint16_t flow_index, uint16_t param_mode_index);

void AudioEffect_Parambin_SwitchEffectByName(const char *Flow_name, int16_t param_mode_index0, int16_t param_mode_index1);
//void AudioEffect_Parambin_SwitchEffectModeByName(const char *mode_name);

uint8_t *AudioEffect_Parambin_GetFlowNameByIndex(uint16_t flow_index);

uint8_t AudioEffect_Parambin_GetEffectAddr(AudioEffectParambinContext *AudioEffectParambin, AUDIO_EFFECT_NAME_NUM effect_index);

/*
 * Memory直接读写（参数不含 enable）
 * param_index: 参数索引，0xff = 全部参数
 */
ROBOEFFECT_ERROR_CODE AudioEffect_ParamSet(void *main_context, uint8_t addr, uint8_t param_index, int16_t *param_input);

const int16_t* AudioEffect_ParamGet(void *main_context, uint8_t addr, uint8_t param_index);

/* 使能/禁能音效 */
ROBOEFFECT_ERROR_CODE AudioEffect_Enable(void *main_context, uint8_t addr, uint8_t enable);

/*
 * === Preset 类：Flash 预设参数（参数含 enable） ===
 * param_index: 参数索引，0xff = 全部参数
 * PresetGet:  从 Flash 读取预设参数值（只读）
 * PresetSet:  从 Flash 查预设表，按 step 索引取值，写入 Memory 生效
 */
int16_t* AudioEffect_PresetGet(AudioEffectParambinContext *AudioEffectParambin, AUDIO_EFFECT_NAME_NUM effect_index, uint8_t param_index,
		AUDIO_EFFECT_PRESET_NUM preset_index, uint32_t *preset_param_size);

ROBOEFFECT_ERROR_CODE AudioEffect_PresetSet(AudioEffectParambinContext *AudioEffectParambin, AUDIO_EFFECT_NAME_NUM effect_index, uint8_t param_index,
		AUDIO_EFFECT_PRESET_NUM preset_index, uint8_t step);

bool AudioEffect_Parambin_EffectModeIsRunning(AudioEffectParambinContext *AudioEffectParambin, const char *FlowName, uint16_t param_mode_index);
//bool AudioEffect_Parambin_EffectModeIsRunning(AudioEffectParambinContext *AudioEffectParambin, uint16_t flow_index, uint16_t param_mode_index);

bool AudioEffect_Parambin_GetEffectListInfoByName(const char *FlowName, uint16_t param_mode_index, uint32_t *sample_rate, uint32_t *frame_size);
//bool AudioEffect_Parambin_GetEffectListInfoByIndex(uint16_t flow_index, uint16_t param_mode_index, uint32_t *sample_rate, uint32_t *frame_size);

const char *AudioEffect_Parambin_GetSourceNameByIndex(AudioEffectParambinContext *AudioEffectParambin, uint16_t source_index);

const char *AudioEffect_Parambin_GetSinkNameByIndex(AudioEffectParambinContext *AudioEffectParambin, uint16_t sink_index);

void init_effect_flows_Num(void);
void init_effect_flows_Status(void);
#endif //__AUDIO_EFFECT_PARAM_BIN_H__
