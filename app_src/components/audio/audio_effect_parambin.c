/**
 **************************************************************************************
 * @file    audio_effect_parambin.c
 * @brief   Audio effect ctrl for Roboeffect param bin
 *
 * @author  Yangyu
 * @version V1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
//drvier
#include <nds32_utils_math.h>
#include <string.h>

//application
#include "flash_table.h"
#include "debug.h"
#include "audio_effect_parambin.h"
#include "audio_core_adapt.h"
#include "audio_core_api.h"
#include "rtos_api.h"
#include "breakpoint.h"
#include "main_task.h"
#include "roboeffect_prot.h"
#include "spi_flash.h"
#include "audio_vol.h"
#include "roboeffect_api.h"
#include "audio_effect_control.h"
#include "flash_interface.h"
#include "communication.h"
#ifdef DUAL_CORE_EN
#include "dualcore_dsp.h"
#endif

#ifdef DUAL_CORE_EN
static bool AudioEffect_ContextIsCore2(void *main_context)
{
#ifndef DUAL_CORE_EN
	return FALSE;
#elif !defined(ROBOEFFECT_DOUBLE_ENGINE_EN)
	return TRUE;
#elif  defined(CFG_PARA_CORE2_NET) && (CFG_PARA_CORE2_NET == DefaultNet)
	return main_context == EffectParambin.context_memory;
#elif defined(CFG_PARA_CORE2_NET) && (CFG_PARA_CORE2_NET == SeparateNet)
	extern 	AudioEffectParambinContext EffectParambin2;
	return main_context == EffectParambin2.context_memory;
#else
	return FALSE;
#endif
}
#endif

static int16_t s_param_buf[128];
/*
 * 具体param_input，param_index意义请查阅以下文档
 * B6_Audio_SDK\middleware\roboeffect\documents\audio_effects_overview.release.html
 */
ROBOEFFECT_ERROR_CODE AudioEffect_ParamSet(void *main_context, uint8_t addr, uint8_t param_index, int16_t *param_input)
{
	memset(s_param_buf, 0, sizeof(s_param_buf));

	if(main_context == NULL)
	{
		DBG("AudioEffect_ParamSet main_context NULL\n");
		return ROBOEFFECT_CONTEXT_MEMORY_ERROR;
	}
	if(param_input == NULL)
	{
		DBG("AudioEffect_ParamSet param_input NULL\n");
		return ROBOEFFECT_CONFIG_PARAM_ERROR;
	}

#ifdef DUAL_CORE_EN
	if(AudioEffect_ContextIsCore2(main_context))
	{
		if(!DualCoreDspAddrIsInitOk())
		{
			DBG("DualCore未初始化\n");
			return ROBOEFFECT_EFFECT_INIT_FAILED;
		}
		if (param_index == 0xff)
		{
			int16_t cnt = roboeffect_get_effect_parameter_count(main_context, addr);
			if (cnt > 128)
			{
				DBG("param count overflow\n");
				return ROBOEFFECT_PARAMS_ERROR;
			}
			memcpy(s_param_buf, param_input, cnt * sizeof(int16_t));
		}
		else
		{
			s_param_buf[0] = *param_input;
		}
		DualCoreDspEffectSet(main_context, addr, param_index, s_param_buf);

		return ROBOEFFECT_ERROR_OK;
	}
#endif
	return roboeffect_set_effect_parameter(main_context, addr, param_index, param_input);
}

const int16_t* AudioEffect_ParamGet(void *main_context, uint8_t addr, uint8_t param_index)
{
	if(main_context == NULL)
	{
		DBG("AudioEffect_ParamGet main_context NULL\n");
		return NULL;
	}

#ifdef DUAL_CORE_EN
	if(AudioEffect_ContextIsCore2(main_context))
	{
		if(!DualCoreDspAddrIsInitOk())
		{
			DBG("DualCore未初始化\n");
			return NULL;
		}
	}
#endif
	return roboeffect_get_effect_parameter(main_context, addr, param_index);
}

ROBOEFFECT_ERROR_CODE AudioEffect_Enable(void *main_context, uint8_t addr, uint8_t enable)
{
	if(main_context == NULL)
	{
		DBG("AudioEffect_Enable main_context NULL\n");
		return ROBOEFFECT_CONTEXT_MEMORY_ERROR;
	}
#ifdef DUAL_CORE_EN
	if(AudioEffect_ContextIsCore2(main_context))
	{
		if(!DualCoreDspAddrIsInitOk())
		{
			DBG("DualCore未初始化\n");
			return ROBOEFFECT_EFFECT_INIT_FAILED;
		}
		DualCoreDspEffectEnable(main_context, addr, enable);
		return ROBOEFFECT_ERROR_OK;
	}
#endif
	return roboeffect_enable_effect(main_context, addr, enable);
}

const EnumStringMap audio_source_map[] = {
    #define XX_MAP(name, str) {name, str},
    AUDIO_SOURCE_ENUM_MAP(XX_MAP)
    #undef XX_MAP
};
const EnumStringMap audio_sink_map[] = {
    #define XX_MAP(name, str) {name, str},
    AUDIO_SINK_ENUM_MAP(XX_MAP)
    #undef XX_MAP
};

static const EnumStringMap audio_effect_name_map[] = {
    #define XX_MAP(name, str) {name, str},
	AUDIO_EFFECT_NAME_ENUM_MAP(XX_MAP)
    #undef XX_MAP
};

static const EnumStringMap audio_effect_preset_map[] = {
    #define XX_MAP(name, str) {name, str},
	AUDIO_EFFECT_PRESET_ENUM_MAP(XX_MAP)
    #undef XX_MAP
};

#define AUDIO_EFFECT_ERRORCODE_ENUM_MAP(XX) \
    XX(ROBOEFFECT_EFFECT_NOT_EXISTED,					"ROBOEFFECT_EFFECT_NOT_EXISTED") \
    XX(ROBOEFFECT_EFFECT_PARAMS_NOT_FOUND,				"ROBOEFFECT_EFFECT_PARAMS_NOT_FOUND") \
    XX(ROBOEFFECT_INSUFFICIENT_MEMORY,					"ROBOEFFECT_INSUFFICIENT_MEMORY") \
    XX(ROBOEFFECT_EFFECT_INIT_FAILED,					"ROBOEFFECT_EFFECT_INIT_FAILED") \
    XX(ROBOEFFECT_ILLEGAL_OPERATION,					"ROBOEFFECT_ILLEGAL_OPERATION") \
    XX(ROBOEFFECT_EFFECT_LIB_NOT_MATCH_1,				"ROBOEFFECT_EFFECT_LIB_NOT_MATCH_1") \
    XX(ROBOEFFECT_EFFECT_LIB_NOT_MATCH_2,				"ROBOEFFECT_EFFECT_LIB_NOT_MATCH_2") \
    XX(ROBOEFFECT_ADDRESS_NOT_EXISTED,					"ROBOEFFECT_ADDRESS_NOT_EXISTED") \
    XX(ROBOEFFECT_PARAMS_ERROR,							"ROBOEFFECT_PARAMS_ERROR") \
    XX(ROBOEFFECT_FRAME_SIZE_ERROR,						"ROBOEFFECT_FRAME_SIZE_ERROR") \
    XX(ROBOEFFECT_MEMORY_SIZE_QUERY_ERROR,				"ROBOEFFECT_MEMORY_SIZE_QUERY_ERROR") \
    XX(ROBOEFFECT_EFFECT_VER_NOT_MATCH_ERROR,			"ROBOEFFECT_EFFECT_VER_NOT_MATCH_ERROR") \
    XX(ROBOEFFECT_LIB_VER_NOT_MATCH_ERROR,				"ROBOEFFECT_LIB_VER_NOT_MATCH_ERROR") \
    XX(ROBOEFFECT_3RD_PARTY_LIB_NOT_MATCH_ERROR,		"ROBOEFFECT_3RD_PARTY_LIB_NOT_MATCH_ERROR") \
    XX(ROBOEFFECT_PARAMBIN_ERROR,						"ROBOEFFECT_PARAMBIN_ERROR") \
    XX(ROBOEFFECT_CONTEXT_MEMORY_ERROR,					"ROBOEFFECT_CONTEXT_MEMORY_ERROR") \
    XX(ROBOEFFECT_PARAMBIN_DATA_NOT_FOUND,				"ROBOEFFECT_PARAMBIN_DATA_NOT_FOUND") \
    XX(ROBOEFFECT_PARAMBIN_DATA_VER_ERROR,				"ROBOEFFECT_PARAMBIN_DATA_VER_ERROR") \
    XX(ROBOEFFECT_MSG_VALUE_NAME_NOT_FOUND,				"ROBOEFFECT_MSG_VALUE_NAME_NOT_FOUND") \
    XX(ROBOEFFECT_MSG_DATA_ERROR,						"ROBOEFFECT_MSG_DATA_ERROR") \
    XX(ROBOEFFECT_FLUSH_FIFO_FULL,						"ROBOEFFECT_FLUSH_FIFO_FULL") \
    XX(ROBOEFFECT_ERROR_OK,								"ROBOEFFECT_ERROR_OK")

const char* audio_effect_errorcode_to_string(ROBOEFFECT_ERROR_CODE error_code) {
    switch(error_code) {
        #define XX_CASE(name, str) case name: return str;
        AUDIO_EFFECT_ERRORCODE_ENUM_MAP(XX_CASE)
        #undef XX_CASE
        default: return "UNKNOWN_ERROR_CODE";
    }
}

	AudioEffectParambinContext EffectParambin;
#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
	AudioEffectParambinContext EffectParambin2;
#endif

static int get_enum_value(const char *str, const EnumStringMap *map, int size) {
    for (int i = 0; i < size; i++) {
        if (strcmp(map[i].enum_str, str) == 0) {
            return map[i].enum_val;
        }
    }
    return -1;
}

#ifdef CFG_FUNC_ROBOEFFECT_EN
static bool AudioEffect_Parambin_Check(AudioEffectParambinContext *AudioEffectParambin, const char *name, uint16_t param_index)
{
	uint32_t AUDIOEFFECT_FLASHBIN_ADDRESS = AudioEffect_Parambin_GetFlashEffectAddr();

	if(AudioEffectParambin->user_effect_parameters)
	{
		osPortFree(AudioEffectParambin->user_effect_parameters);
		AudioEffectParambin->user_effect_parameters = NULL;
	}
	if(AudioEffectParambin->user_effect_list)
	{
		osPortFree(AudioEffectParambin->user_effect_list);
		AudioEffectParambin->user_effect_list = NULL;
	}

	if(roboeffect_parambin_check_whole_bin((uint8_t *)AUDIOEFFECT_FLASHBIN_ADDRESS, NULL) == ROBOEFFECT_ERROR_OK)
	{
		uint32_t param_size = 0;
//		if(need_switch_mode & SWITCH_MODE_PARAMBIN_MODE)//! force get flow and mode index from flashbin
//		{
//			roboeffect_parambin_flow_pair *pair = roboeffect_parambin_get_current_flow(AUDIOEFFECT_FLASHBIN_ADDRESS, NULL);
//			//* DU has only one pair of index
//			mainAppCt.effect_flow_index = pair[0].flow_index;
//			mainAppCt.effect_param_mode_index = pair[0].param_mode_index;
//		}
//		 APP_DBG("FLOW: %d, %d\n", mainAppCt.effect_flow_index, mainAppCt.effect_param_mode_index);

		if((AudioEffectParambin->flow_data = roboeffect_parambin_get_flow_by_name((uint8_t *)AUDIOEFFECT_FLASHBIN_ADDRESS, (char *)name, &AudioEffectParambin->flow_size)) != NULL)
		{
			uint8_t *temp_data, *mode_data_out = NULL, *codec_data_out = NULL;
			temp_data = roboeffect_parambin_get_sub_type((uint8_t *)AudioEffectParambin->flow_data, AudioEffectParambin->flow_size, ROBO_PB_SUBTYPE_SCRIPT, FALSE, NULL);
			AudioEffectParambin->user_flow_script = temp_data + 4 + 4;
			AudioEffectParambin->user_flow_script_len = *((uint32_t*)(temp_data + 4)) - 2;//-2 is crc bytes

			AudioEffectParambin->user_effect_list = osPortMalloc(sizeof(roboeffect_effect_list_info));
			memcpy(AudioEffectParambin->user_effect_list,roboeffect_parambin_get_sub_type((uint8_t *)AudioEffectParambin->flow_data,
					AudioEffectParambin->flow_size, ROBO_PB_SUBTYPE_EFFECTS_INFO, TRUE, NULL),sizeof(roboeffect_effect_list_info));

			AudioEffectParambin->user_effect_list_arry = (roboeffect_exec_effect_info *)roboeffect_parambin_get_sub_type((uint8_t *)AudioEffectParambin->flow_data,
					AudioEffectParambin->flow_size, ROBO_PB_SUBTYPE_EFFECTS_LIST, TRUE, NULL);

			AudioEffectParambin->user_effect_steps = (roboeffect_effect_steps_table *)roboeffect_parambin_get_sub_type((uint8_t *)AudioEffectParambin->flow_data,
					AudioEffectParambin->flow_size, ROBO_PB_SUBTYPE_FLOW_INFO, TRUE, NULL);

			AudioEffectParambin->sub_type_data = roboeffect_parambin_get_sub_type((uint8_t *)AudioEffectParambin->flow_data, AudioEffectParambin->flow_size, ROBO_PB_SUBTYPE_MODE_PARAMS, TRUE, NULL);
			roboeffect_parambin_get_mode_data_by_index(AudioEffectParambin->sub_type_data, param_index, &mode_data_out, &param_size, &codec_data_out, &AudioEffectParambin->user_codec_parameters_len);
//			AudioEffectParambin->user_effect_parameters = mode_data_out;
			AudioEffectParambin->user_effect_parameters = osPortMalloc(GET_USER_EFFECT_PARAMETERS_TOTAL_LEN(mode_data_out) * sizeof(uint8_t));
			memcpy(AudioEffectParambin->user_effect_parameters, mode_data_out, GET_USER_EFFECT_PARAMETERS_TOTAL_LEN(mode_data_out) * sizeof(uint8_t));
			if(codec_data_out)
				AudioEffectParambin->user_codec_parameters = codec_data_out;
			// temp_size = param_hdr->codec_param_len;

			AudioEffectParambin->source_io = (roboeffect_io_unit *)roboeffect_parambin_get_sub_type((uint8_t *)AudioEffectParambin->flow_data, AudioEffectParambin->flow_size, ROBO_PB_SUBTYPE_SOURCE, TRUE, NULL);
			AudioEffectParambin->sink_io = (roboeffect_io_unit *)roboeffect_parambin_get_sub_type((uint8_t *)AudioEffectParambin->flow_data, AudioEffectParambin->flow_size, ROBO_PB_SUBTYPE_SINK, TRUE, NULL);
			AudioEffectParambin->steps_flow = (uint32_t *)roboeffect_parambin_get_sub_type((uint8_t *)AudioEffectParambin->flow_data, AudioEffectParambin->flow_size, ROBO_PB_SUBTYPE_STEPS, TRUE, NULL);

			if(AudioEffectParambin->user_flow_script && AudioEffectParambin->user_effect_list && AudioEffectParambin->user_effect_steps
					&& AudioEffectParambin->sub_type_data && AudioEffectParambin->source_io && AudioEffectParambin->sink_io)
			{
//				APP_DBG("[AudioEffect_Parambin] AudioEffect_Parambin_Check ok!\n");
				return TRUE;
			}
			else
			{
				APP_DBG("[AudioEffect_Parambin] WARNING! AudioEffect_Parambin_Check have NULL pointer!\n");
			}
		}
		else
		{
			APP_DBG("Not found flow: %s!!!\n", name);
			return FALSE;
		}
	}
	else
	{
		APP_DBG("[AudioEffect_Parambin] ERROR! AudioEffect_Parambin_Check failed!\n");
		APP_DBG("Please refresh the audio_effect.bin by the ACPWorkbench and burn Firmware\n");
		#ifdef CFG_CHIP_26
		APP_DBG("Or Please refer to the 《MVsB62_Audio_SDK系统应用开发FAQ说明文档》/17.6 SDK报[AudioEffect_Parambin] ERROR错误 \n");
		#else
		APP_DBG("Or Please refer to the 《MVsB62_Audio_SDK系统应用开发FAQ说明文档》/16.6 SDK报[AudioEffect_Parambin] ERROR错误 \n");
		#endif
	}
	return FALSE;
}

uint32_t AudioEffect_Parambin_GetFlashEffectAddr(void)
{
	return get_effect_data_addr();
}
#endif


void *AudioEffect_Parambin_GetSourceBuffer(AudioEffectParambinContext *AudioEffectParambin, AUDIO_CORE_SOURCE_NUM source_index)
{
	if(AudioEffectParambin==NULL) return NULL;
	if((AudioEffectParambin->context_memory == NULL) || (source_index < 0 || source_index >= AUDIO_CORE_SOURCE_MAX_NUM))
		return NULL;

	const roboeffect_io_unit *device_node = NULL;

	if(IO_UNIT_VALID(AudioEffectParambin->source_io))
	{
		if(((device_node = roboeffect_parambin_get_io_by_name(AudioEffectParambin->context_memory, audio_source_map[source_index].enum_str)) != NULL))
		{
			return roboeffect_get_source_buffer(AudioEffectParambin->context_memory, device_node->io_id);
		}
	}

	//APP_DBG("Parambin source %d not found buffer\n", source_index);   //debug时开启,AudioCoreSourceMuteApply中会调用
	return NULL;
}

void *AudioEffect_Parambin_GetSinkBuffer(AudioEffectParambinContext *AudioEffectParambin, AUDIO_CORE_SINK_NUM sink_index)
{
	if(AudioEffectParambin==NULL) return NULL;
	if((AudioEffectParambin->context_memory == NULL) || (sink_index < 0 || sink_index >= AUDIO_CORE_SINK_MAX_NUM))
		return NULL;

	const roboeffect_io_unit *device_node = NULL;

	if(((device_node = roboeffect_parambin_get_io_by_name(AudioEffectParambin->context_memory, audio_sink_map[sink_index].enum_str)) != NULL) && IO_UNIT_VALID(device_node))
	{
		return roboeffect_get_sink_buffer(AudioEffectParambin->context_memory, device_node->io_id);
	}

	//APP_DBG("Parambin sink %d not found buffer\n", sink_index);
	return NULL;
}

void *AudioEffect_Parambin_GetSourceBuffer_Info(AudioEffectParambinContext *AudioEffectParambin, AUDIO_CORE_SOURCE_NUM source_index, uint8_t *Width, uint8_t *Ch)
{
	if(AudioEffectParambin==NULL) return NULL;
	if((AudioEffectParambin->context_memory == NULL) || (source_index < 0 || source_index >= AUDIO_CORE_SOURCE_MAX_NUM))
		return NULL;

	const roboeffect_io_unit *device_node = NULL;

	if(IO_UNIT_VALID(AudioEffectParambin->source_io))
	{
		if(((device_node = roboeffect_parambin_get_io_by_name(AudioEffectParambin->context_memory, audio_source_map[source_index].enum_str)) != NULL)&& IO_UNIT_VALID(device_node))
		{
			if(device_node->width == BITS_16)
				*Width = WIDTH_16BIT;
			else if(device_node->width == BITS_24)
				*Width = WIDTH_L24BIT;
			else
				*Width = WIDTH_32BIT;
			if(device_node->ch == CH_STEREO)
				*Ch = 2;
			else
				*Ch = 1;
			return roboeffect_get_source_buffer(AudioEffectParambin->context_memory, device_node->io_id);
		}
	}

	//APP_DBG("Parambin source %d not found buffer\n", source_index);   //debug时开启,AudioCoreSourceMuteApply中会调用
	return NULL;
}

void *AudioEffect_Parambin_GetSinkBuffer_Info(AudioEffectParambinContext *AudioEffectParambin, AUDIO_CORE_SINK_NUM sink_index, uint8_t *Width, uint8_t *Ch)
{
	if(AudioEffectParambin==NULL) return NULL;
	if((AudioEffectParambin->context_memory == NULL) || (sink_index < 0 || sink_index >= AUDIO_CORE_SINK_MAX_NUM))
		return NULL;

	const roboeffect_io_unit *device_node = NULL;

	if(((device_node = roboeffect_parambin_get_io_by_name(AudioEffectParambin->context_memory, audio_sink_map[sink_index].enum_str)) != NULL) && IO_UNIT_VALID(device_node))
	{
		if(device_node->width == BITS_16)
			*Width = WIDTH_16BIT;
		else if(device_node->width == BITS_24)
			*Width = WIDTH_L24BIT;
		else
			*Width = WIDTH_32BIT;
		if(device_node->ch == CH_STEREO)
			*Ch = 2;
		else
			*Ch = 1;
		return roboeffect_get_sink_buffer(AudioEffectParambin->context_memory, device_node->io_id);
	}

	//APP_DBG("Parambin sink %d not found buffer\n", sink_index);
	return NULL;
}

#ifdef CFG_FUNC_ROBOEFFECT_EN
int8_t AudioEffect_Parambin_GetSourceEnum(const char *str)
{
	int8_t size = sizeof(audio_source_map) / sizeof(EnumStringMap);
    return (AUDIO_CORE_SOURCE_NUM)get_enum_value(str, audio_source_map, size);
}

int8_t AudioEffect_Parambin_GetSinkEnum(const char *str)
{
	int8_t size = sizeof(audio_sink_map) / sizeof(EnumStringMap);
    return (AUDIO_CORE_SINK_NUM)get_enum_value(str, audio_sink_map, size);
}

uint16_t AudioEffect_Parambin_GetTotalFlowNum(void)
{
	roboeffect_parambin_header *hdr_ptr = (roboeffect_parambin_header *)AudioEffect_Parambin_GetFlashEffectAddr();

	if(hdr_ptr != NULL)
	{
		return hdr_ptr->flow_cnt;
	}
	return 0;
}

uint16_t AudioEffect_Parambin_GetTotalParamNumByName(const char *name)
{
	uint8_t *flow_data;
	uint32_t flow_size = 0;

	if((flow_data = roboeffect_parambin_get_flow_by_name((uint8_t *)AudioEffect_Parambin_GetFlashEffectAddr(), (char *)name, &flow_size)) != NULL)
	{
		AudioEffect_parambin_mode1_brief *sub_type_data = (AudioEffect_parambin_mode1_brief *)roboeffect_parambin_get_sub_type((uint8_t *)flow_data, flow_size, ROBO_PB_SUBTYPE_MODE_PARAMS, TRUE, NULL);

		if(sub_type_data != NULL)
		{
			return sub_type_data->mode_type_cnt;
		}
	}
	return 0;
}

/*
 * parameters  codec_data_out的指针
 * addr 需要解析的协议块(比如01)
 * 返回值是实际的数据的头地址
 */
int16_t *get_param_by_raw_data(uint8_t addr, uint8_t *parameters)
{
	uint8_t *ptr = parameters;//start from the first item

	while(((*ptr) >= 0x01) && ((*ptr) <= 0x0d))
	{
		if(*ptr == addr)
		{
			return (int16_t*)(ptr + 2);
		}
		else
		{
			ptr += *(ptr + 1) + 2;
		}
	}

	return NULL;
}

bool RoboeffectOK = false;
//MemorySet: 有限分配，1全部分配
bool AudioEffect_Parambin_Init(AudioEffectParambinContext *AudioEffectParambin, const char *name, uint16_t param_index, bool MemorySet)
{
	RoboeffectOK = false;

	ROBOEFFECT_ERROR_CODE audioeffect_ret;

	if(AudioEffectParambin->context_memory)
	{
		osPortFree(AudioEffectParambin->context_memory);
		AudioEffectParambin->context_memory = NULL;
	}

	if(name != NULL && name[0] == '\0')
	{
		return TRUE;
		APP_DBG("flow name is NULL, The engine won't run.");
	}

	roboeffect_prot_init();
	DataCacheInvalidAll();

	//* check flashbin and set global pointers.
	if(!AudioEffect_Parambin_Check(AudioEffectParambin, name, param_index))
	{
		APP_DBG("[AudioEffect_Parambin] ERROR! AudioEffect_Parambin_Check failed!\n");
		return FALSE;
	}

	//用roboeffect_get_parameter_section来设置帧长和采样率
	uint8_t *sr_data = roboeffect_get_parameter_section(AudioEffectParambin->user_effect_parameters, 0x00, PARAM_TYPE_SAMPLE_RATE);
	if(sr_data == NULL)
	{
		APP_DBG("[AudioEffect_Parambin] ERROR! 采样率获取失败!\n");
		return FALSE;
	}
	uint8_t *fz_data = roboeffect_get_parameter_section(AudioEffectParambin->user_effect_parameters, 0x00, PARAM_TYPE_FRAME_SIZE);
	if(fz_data == NULL)
	{
		APP_DBG("[AudioEffect_Parambin] ERROR! 帧长获取失败!\n");
		return FALSE;
	}
	AudioEffectParambin->user_effect_list->sample_rate = *((uint32_t*)sr_data);
	AudioEffectParambin->user_effect_list->frame_size  = *((uint32_t*)fz_data);
	if(roboeffect_parambin_estimate_frame_size(AudioEffectParambin->flow_data, param_index, AudioEffectParambin->user_effect_list))
	{
		//根据设置的已有帧长和信息估算一下是否合法,不合法会估算一个可行的其他帧长值
		AudioEffectParambin->user_effect_list->frame_size = roboeffect_parambin_estimate_frame_size(AudioEffectParambin->flow_data, param_index, AudioEffectParambin->user_effect_list);
	}
	else
	{
		APP_DBG("frame_size Error\n");
		return FALSE;
	}

	AudioEffectParambin->audioeffect_memory_size = roboeffect_parambin_estimate_memory_size(AudioEffectParambin->flow_data, param_index, AudioEffectParambin->user_effect_list);
	APP_DBG("flow:%s,mode:%d,memory_size:%ld\n",AudioEffectParambin->flow_data, param_index, AudioEffectParambin->audioeffect_memory_size);

	int32_t OsFreeSize_max = xPortGetFreeHeapSize();
	if(AudioEffectParambin->audioeffect_memory_size < 0)
	{
		APP_DBG("Get context size failed, Error: %s\n", audio_effect_errorcode_to_string(AudioEffectParambin->audioeffect_memory_size));
		return FALSE;
	}

	if(MemorySet == 0)
	{
#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
		if(AudioEffectParambin->audioeffect_memory_size + ROBOEFFECT_EXTRA_MEMORY2*1024 >= OsFreeSize_max)
		{
			APP_DBG("memory not enough!\n");
			return FALSE;
		}
		else
		{
			AudioEffectParambin->audioeffect_memory_size = AudioEffectParambin->audioeffect_memory_size + ROBOEFFECT_EXTRA_MEMORY2*1024;
		}
#endif
	}
	else
	{
		if(AudioEffectParambin->audioeffect_memory_size + ROBOEFFECT_EXTRA_MEMORY*1024 >= OsFreeSize_max)
		{
			APP_DBG("memory not enough!\n");
			return FALSE;
		}
		else
		{
			AudioEffectParambin->audioeffect_memory_size = AudioEffectParambin->audioeffect_memory_size + ROBOEFFECT_EXTRA_MEMORY*1024;
		}
	}

	if((AudioEffectParambin->context_memory = osPortMallocFromEnd(AudioEffectParambin->audioeffect_memory_size)) == NULL)
	{
		APP_DBG("context_memory is NULL\n");
		return FALSE;
	}

	memset(AudioEffectParambin->context_memory, 0, AudioEffectParambin->audioeffect_memory_size);

	if(ROBOEFFECT_ERROR_OK != (audioeffect_ret = roboeffect_parambin_init(AudioEffectParambin->context_memory, AudioEffectParambin->audioeffect_memory_size,
			AudioEffectParambin->flow_data, param_index, AudioEffectParambin->user_effect_list)) )
	{
		APP_DBG("roboeffect_init failed. Error: %s\n", audio_effect_errorcode_to_string(audioeffect_ret));
		osPortFree(AudioEffectParambin->context_memory);
		AudioEffectParambin->context_memory = NULL;
		return FALSE;
	}

	if(!MixSupport(AudioEffectParambin->user_effect_list->sample_rate, AudioEffectParambin->user_effect_list->frame_size, CFG_DEFAULT_WIDTH))
	{
		APP_DBG("sampleRate or framesize Not supported ,sysMode reboot!\n");
		return FALSE;
	}

	APP_DBG("Roboeffect init done!\n");
	RoboeffectOK = true;
	/*********************************************************debug info**********************************************************************/
//	APP_DBG("list: %d, %d\n", AudioEffectParambin.user_effect_list->sample_rate, AudioEffectParambin.user_effect_list->frame_size);
//	APP_DBG("steps: %d, %d, %d, %d\n", AudioEffectParambin.user_effect_steps->step_num, AudioEffectParambin.user_effect_steps->mem_used,
//			AudioEffectParambin.user_effect_steps->src_unit_num, AudioEffectParambin.user_effect_steps->des_unit_num);
//	const roboeffect_io_unit *device_node = AudioEffectParambin.source_io;
//	for(uint8_t i = 0; i < AudioEffectParambin.user_effect_steps->src_unit_num; i++, device_node++)
//	{
//		if(IO_UNIT_VALID(device_node))
//			APP_DBG("source %d, %s\n", i, roboeffect_parambin_get_io_name(AudioEffectParambin.context_memory, IO_UNIT_ID(device_node)));
//	}
//	device_node = AudioEffectParambin.sink_io;
//	for(uint8_t i = 0; i < AudioEffectParambin.user_effect_steps->des_unit_num; i++, device_node++)
//	{
//		if(IO_UNIT_VALID(device_node))
//			APP_DBG("sink %d, %s\n", i, roboeffect_parambin_get_io_name(AudioEffectParambin.context_memory, IO_UNIT_ID(device_node)));
//	}

//	for(uint8_t i = 0; i < AUDIO_CORE_SOURCE_MAX_NUM; i++)
//	{
//		if(AudioEffect_Parambin_GetSourceBuffer(i))
//			APP_DBG("source %d buf ok\n", i);
//	}
//	for(uint8_t i = 0; i < AUDIO_CORE_SINK_MAX_NUM; i++)
//	{
//		if(AudioEffect_Parambin_GetSinkBuffer(i))
//			APP_DBG("sink %d buf ok\n", i);
//	}
//	uint8_t *Hwct = AudioEffect_Parambin_GetCurHardwareConfig();
//	for(uint8_t i = 0; i < 200;)
//	{
//		APP_DBG("0x%x, 0x%x\n", Hwct[i++], Hwct[i++]);
//	}
//	for(uint16_t i = 0; i < AudioEffect_Parambin_GetTotalFlowNum(); i++)
//	{
//		APP_DBG("flow %d: %s, param:%d\n", i, AudioEffect_Parambin_GetFlowNameByIndex(i), AudioEffect_Parambin_GetTotalParamNumInFlow(i));
//	}
//	uint32_t *passwd = roboeffect_parambin_get_password(AudioEffect_Parambin_GetFlashEffectAddr());
//	APP_DBG("password:0x%x\n", passwd[0]);
//	APP_DBG("addr preGain:0x%x|0x%x, music_EQ:0x%x|0x%x\n", roboeffect_parambin_get_addr_by_name(AudioEffectParambin.context_memory, "preGain"),
//														AudioEffect_Parambin_GetEffectAddr(EFFECT_PREGAIN),
//														roboeffect_parambin_get_addr_by_name(AudioEffectParambin.context_memory, "music_EQ"),
//														AudioEffect_Parambin_GetEffectAddr(EFFECT_MUSICEQ));
//	int16_t *param;
//	if((param = AudioEffect_Parambin_PreSetEffectAllParams_Get(EFFECT_MUSICEQ, PRESET_EQ_FLAT)) != NULL)
//	{
//		for(uint8_t i = 0; i < 50;)
//		{
//			APP_DBG("EQ i:%d---0x%x\n", i, param[i++]);
//		}
//	}
//	uint32_t step;
//	if((param = AudioEffect_Parambin_PreSetEffectParam_Get(EFFECT_MUSICGAIN, INDEX_GAIN_CONTROL_GAIN, PRESET_GAIN_VOL, &step)) != NULL)
//	{
//		APP_DBG("step:%d\n", step);
//		for(uint8_t i = 0; i < 32; i++)
//		{
//			APP_DBG("VOL i:%d---%d\n", i, param[i]);
//		}
//	}
//	uint8_t *mode_data_out;
//	uint32_t param_size;
//	int16_t parambin_check = roboeffect_parambin_get_preset_param_by_name(AudioEffectParambin.sub_type_data, 0x88, 0xff, "FLAT", &mode_data_out, &param_size);
//	if(parambin_check == ROBOEFFECT_ERROR_OK)
//	{
//		for(int i = 0; i < param_size/2; i++)
//		{
//			printf("EQ %d-->0x%x\n", i, ((int16_t*)mode_data_out)[i]);
//		}
//	}
//	parambin_check = roboeffect_parambin_get_preset_param_by_name(AudioEffectParambin.sub_type_data, 0x8B, 0x01, "VOL", &mode_data_out, &param_size);
//	if(parambin_check == ROBOEFFECT_ERROR_OK)
//	{
//		for(int i = 0; i < param_size/2; i++)
//		{
//			printf("VOL %d-->%d\n", i, ((int16_t*)mode_data_out)[i]);
//		}
//	}
	/*********************************************************debug info**********************************************************************/
	return TRUE;
}

void AudioEffect_Parambin_Deinit(AudioEffectParambinContext *AudioEffectParambin)
{
	RoboeffectOK = false;
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
	g_host_sync_ok = false;  // 音效参数释放，清除上位机同步标志
#endif
	if(AudioEffectParambin->context_memory)
	{
		osPortFree(AudioEffectParambin->context_memory);
		AudioEffectParambin->context_memory = NULL;
	}
	if(AudioEffectParambin->user_effect_parameters)
	{
		osPortFree(AudioEffectParambin->user_effect_parameters);
		AudioEffectParambin->user_effect_parameters = NULL;
	}
	if(AudioEffectParambin->user_effect_list)
	{
		osPortFree(AudioEffectParambin->user_effect_list);
		AudioEffectParambin->user_effect_list = NULL;
	}
	memset(AudioEffectParambin, 0, sizeof(AudioEffectParambinContext));
}

const void *AudioEffect_Parambin_GetCurHardwareConfig(AudioEffectParambinContext *AudioEffectParambin)
{
	if(AudioEffectParambin->user_codec_parameters == NULL)
		return NULL;
	return AudioEffectParambin->user_codec_parameters;
}

const void *AudioEffect_Parambin_GetCurEffectParams(AudioEffectParambinContext *AudioEffectParambin)
{
	if(AudioEffectParambin->user_effect_parameters == NULL)
		return NULL;
	return AudioEffectParambin->user_effect_parameters;
}

void AudioEffect_Parambin_RefreshSourceSinkBuf(AudioEffectParambinContext *AudioEffectParambin)
{
	for(uint8_t source_index = 0; source_index < AUDIO_CORE_SOURCE_MAX_NUM; source_index++)
	{
		if(AudioEffect_Parambin_GetSourceBuffer(AudioEffectParambin, source_index))
		{
			AudioCore.AudioSource[source_index].PcmInBuf = AudioEffect_Parambin_GetSourceBuffer(AudioEffectParambin, source_index);
		}
	}
	for(uint8_t sink_index = 0; sink_index < AUDIO_CORE_SINK_MAX_NUM; sink_index++)
	{
		if(AudioEffect_Parambin_GetSinkBuffer(AudioEffectParambin, sink_index))
		{
			AudioCore.AudioSink[sink_index].PcmOutBuf = AudioEffect_Parambin_GetSinkBuffer(AudioEffectParambin, sink_index);
		}
	}
}

uint8_t *AudioEffect_Parambin_GetFlashEffectParam(AudioEffectParambinContext *AudioEffectParambin, uint8_t effect_addr)
{
    if (AudioEffectParambin->user_effect_parameters == NULL)
    {
        return NULL;
    }

//    uint16_t total_data_len = GET_USER_EFFECT_PARAMETERS_TOTAL_LEN(AudioEffectParambin.user_effect_parameters);
//    uint16_t third_data_len = GET_USER_EFFECT_PARAMETERS_THIRDDATA_LEN(AudioEffectParambin.user_effect_parameters);
    uint16_t data_len = GET_USER_EFFECT_PARAMETERS_DATA_LEN(AudioEffectParambin->user_effect_parameters);
    uint8_t *effectParams = GET_USER_EFFECT_PARAMETERS_EFFECTDATA_P(AudioEffectParambin->user_effect_parameters);
    uint16_t remaining = data_len;

    while (remaining > 0)
    {
        if (effectParams[0] == effect_addr)
        {
            return effectParams;
        } else {
            uint8_t len = effectParams[1];
            if (len + 2 > remaining)
            {
                break;
            }
            effectParams += (len + 2);
            remaining -= (len + 2);
        }
    }
    return NULL;
}

uint8_t *AudioEffect_Parambin_GetTempEffectParamByAddr(AudioEffectParambinContext *AudioEffectParambin, uint8_t effect_addr)
{
    if (AudioEffectParambin->user_effect_parameters == NULL)
    {
        return NULL;
    }
    if(effect_addr == 0)
    {
    	return AudioEffectParambin->user_effect_parameters;
    }

//    uint16_t total_data_len = GET_USER_EFFECT_PARAMETERS_TOTAL_LEN(AudioEffectParambin.user_effect_parameters);
//    uint16_t third_data_len = GET_USER_EFFECT_PARAMETERS_THIRDDATA_LEN(AudioEffectParambin.user_effect_parameters);
    uint16_t data_len = GET_USER_EFFECT_PARAMETERS_DATA_LEN(AudioEffectParambin->user_effect_parameters);
    uint8_t *effectParams = GET_USER_EFFECT_PARAMETERS_EFFECTDATA_P(AudioEffectParambin->user_effect_parameters);
    uint16_t remaining = data_len;

    while (remaining > 0)
    {
        if (effectParams[0] == effect_addr)
        {
            return effectParams;
        }
        else
        {
            uint8_t len = effectParams[1];
            if (len + 2 > remaining)
            {
                break;
            }
            effectParams += (len + 2);
            remaining -= (len + 2);
        }
    }
    return NULL;
}

#if 0
uint8_t *AudioEffect_Parambin_GetFlashEffectParamByAddr(AudioEffectParambinContext *AudioEffectParambin, uint8_t effect_addr)
{
	uint8_t *effectParams = NULL;
	roboeffect_parambin_get_mode_data_by_index(AudioEffectParambin->sub_type_data, mainAppCt.effect_param_mode_index, &effectParams, NULL, NULL, NULL);

    if(effect_addr == 0)
    {
    	return effectParams;
    }

    uint16_t data_len = GET_USER_EFFECT_PARAMETERS_DATA_LEN(effectParams);
    uint16_t remaining = data_len;
    effectParams = GET_USER_EFFECT_PARAMETERS_EFFECTDATA_P(effectParams);

    while (remaining > 0)
    {
        if (effectParams[0] == effect_addr)
        {
            return effectParams;
        } else {
            uint8_t len = effectParams[1];
            if (len + 2 > remaining)
            {
                break;
            }
            effectParams += (len + 2);
            remaining -= (len + 2);
        }
    }
    return NULL;
}
#endif

#define CFG_FLASH_SECTOR_SIZE		(4096)//4KB
static bool AudioEffect_Parambin_FlashUpdate(uint32_t flash_addr, uint8_t *write_data, uint32_t length)
{
    if (write_data == NULL || length == 0)
    {
        return FALSE;
    }

    uint32_t AUDIOEFFECT_FLASHBIN_ADDRESS = AudioEffect_Parambin_GetFlashEffectAddr();
    if (flash_addr < AUDIOEFFECT_FLASHBIN_ADDRESS)
    {
        return FALSE; // 地址非法
    }

    static uint8_t sector_buf[CFG_FLASH_SECTOR_SIZE] = {0};
    uint32_t total_written = 0;

    while (total_written < length)
    {
        // 计算当前扇区信息
        uint32_t sector_offset = flash_addr - (AUDIOEFFECT_FLASHBIN_ADDRESS - 32);
        uint32_t sector_index = sector_offset / CFG_FLASH_SECTOR_SIZE;
        uint32_t sector_start = (AUDIOEFFECT_FLASHBIN_ADDRESS -32) + sector_index * CFG_FLASH_SECTOR_SIZE;
        uint32_t sector_internal_offset = sector_offset % CFG_FLASH_SECTOR_SIZE;
        // 计算本次可写入长度
        uint32_t write_len = CFG_FLASH_SECTOR_SIZE - sector_internal_offset;
        if (write_len >= (length - total_written))
        {
            write_len = length - total_written;
        }
//        DBG("%ld, %ld, 0x%x, %ld, %ld, %ld, %ld\n", sector_offset, sector_index, sector_start, sector_internal_offset, length, write_len, total_written);

        // 读取整个扇区
        if(SPiFlashReadData(sector_start, sector_buf, CFG_FLASH_SECTOR_SIZE) == FLASH_NONE_ERR)
        {
            // 擦除扇区
        	SpiFlashEraseWpr(SECTOR_ERASE, sector_start / CFG_FLASH_SECTOR_SIZE, 0);
        }

        // 合并数据到缓冲区
        memcpy(sector_buf + sector_internal_offset, write_data + total_written, write_len);

        // 写入整个扇区
        if (SpiFlashWriteWpr(sector_start, sector_buf, CFG_FLASH_SECTOR_SIZE, 0) != FLASH_NONE_ERR)
        {
            APP_DBG("AudioEffect_Parambin_FlashUpdate ERROR!\n");
            return FALSE;
        }

        flash_addr += write_len;
        total_written += write_len;
    }
    return TRUE;
}

//effect params
//addr:	 1 byte
//length:1 byte
//enable:1 byte
//params:length-1 byte
void AudioEffect_Parambin_UpdateFlashEffectParamsByAddr(AudioEffectParambinContext *AudioEffectParambin, uint8_t effect_addr, uint8_t param_index, uint8_t *param_input)
{
	if(IsAudioPlayerMute() == FALSE)
	{
		HardWareMuteOrUnMute();
	}
    uint8_t *params = AudioEffect_Parambin_GetFlashEffectParamByAddr(AudioEffectParambin, effect_addr);
    if (params == NULL)
    {
        APP_DBG("Error: Invalid effect_addr 0x%02X\n", effect_addr);
        return;
    }

    uint8_t addr = params[0];
    uint8_t len = params[1];
    if (addr != effect_addr || len < 1)
    {
        APP_DBG("Error: Corrupted parameter block\n");
        return;
    }

//	 DBG("%02X: %d, %d\n", addr, param_index, param_input[0]);
    // 更新参数
    if (param_index == 0)//only enable/disable
    {
        if (param_input == NULL) return;
        AudioEffect_Parambin_FlashUpdate((uint32_t)(params + 2), param_input, 1);
    }
    else if (param_index == 0xFF)//all parameters
    {
        if (param_input == NULL || len < 1) return;
        AudioEffect_Parambin_FlashUpdate((uint32_t)(params + 2), param_input, len);
    }
    else//other parameter, only one parameter configurated
    {
        if (param_index < 1 || param_index > (len - 1)/2)
        {
            APP_DBG("Error: Invalid param_index %d\n", param_index);
            return;
        }
        AudioEffect_Parambin_FlashUpdate((uint32_t)(params + 3 + (param_index - 1)*2), param_input, 2);
    }
	if(IsAudioPlayerMute() == TRUE)
	{
		HardWareMuteOrUnMute();
	}
}

void AudioEffect_Parambin_UpdateFlashEffectParams(AudioEffectParambinContext *AudioEffectParambin)
{
	if(IsAudioPlayerMute() == FALSE)
	{
		HardWareMuteOrUnMute();
	}
    uint8_t *flash_params = AudioEffect_Parambin_GetFlashEffectParamByAddr(AudioEffectParambin, 0);
    uint8_t *temp_params = AudioEffect_Parambin_GetTempEffectParamByAddr(AudioEffectParambin, 0);
    if (flash_params == NULL || temp_params == NULL)
    {
        APP_DBG("Error: Invalid params point\n");
        return;
    }

    AudioEffect_Parambin_FlashUpdate((uint32_t)(flash_params), temp_params, GET_USER_EFFECT_PARAMETERS_TOTAL_LEN(temp_params));

	if(IsAudioPlayerMute() == TRUE)
	{
		HardWareMuteOrUnMute();
	}
}

void AudioEffect_Parambin_UpdateTempEffectParamsByAddr(AudioEffectParambinContext *AudioEffectParambin, uint8_t effect_addr, uint8_t param_index, uint8_t *param_input)
{
    uint8_t *params = AudioEffect_Parambin_GetTempEffectParamByAddr(AudioEffectParambin, effect_addr);
    if (params == NULL)
    {
        APP_DBG("Error: Invalid effect_addr 0x%02X\n", effect_addr);
        return;
    }

    uint8_t addr = params[0];
    uint8_t len = params[1];
    if (addr != effect_addr || len < 1)
    {
        APP_DBG("Error: Corrupted parameter block\n");
        return;
    }

//	 DBG("%02X: %d, %d\n", addr, param_index, param_input[0]);
    // 更新参数
    if (param_index == 0)//only enable/disable
    {
        if (param_input == NULL) return;
//        AudioEffect_Parambin_FlashUpdate((uint32_t)(params + 2), param_input, 1);
        memcpy((params + 2), param_input, 1);
    }
    else if (param_index == 0xFF)//all parameters
    {
        if (param_input == NULL || len < 1) return;
//        AudioEffect_Parambin_FlashUpdate((uint32_t)(params + 2), param_input, len);
        memcpy((params + 2), param_input, len);
    }
    else//other parameter, only one parameter configurated
    {
        if (param_index < 1 || param_index > (len - 1)/2)
        {
            APP_DBG("Error: Invalid param_index %d\n", param_index);
            return;
        }
//        AudioEffect_Parambin_FlashUpdate((uint32_t)(params + 3 + (param_index - 1)*2), param_input, 2);
        memcpy((params + 3 + (param_index - 1)*2), param_input, 2);
    }
//	if(IsAudioPlayerMute() == TRUE)
//	{
//		HardWareMuteOrUnMute();
//	}
}

#if 0
/** !!!new effect mode will be switched after AudioEffect_EffectMode_Refresh() or AudioEffect_Parambin_Init() **/
void AudioEffect_Parambin_SwitchEffectModeByIndex(uint16_t flow_index, uint16_t param_mode_index)
{
    APP_DBG("Switch Effect Mode %d, %d\n", flow_index, param_mode_index);
	if(flow_index < AudioEffect_Parambin_GetTotalFlowNum())
	{
		if(param_mode_index < AudioEffect_Parambin_GetTotalParamNumInFlow(flow_index))
		{
			mainAppCt.effect_flow_index = flow_index;
			mainAppCt.effect_param_mode_index = param_mode_index;
		}
		else
		{
	        APP_DBG("Error: Invalid param_mode_index %d\n", param_mode_index);
			return;
		}
	}
	else
	{
        APP_DBG("Error: Invalid flow_index %d\n", flow_index);
		return;
	}
}
#endif

/** !!!new effect mode will be switched after AudioEffect_EffectMode_Refresh() or AudioEffect_Parambin_Init()
 * flow_index: 图组的序号
 * param_mode_index：音效参数index
 * 单图：param_mode_index1参数无效
 **/
void AudioEffect_Parambin_SwitchEffectByIndex(int16_t flow_index, int16_t param_mode_index0, int16_t param_mode_index1)
{
#ifndef ROBOEFFECT_DOUBLE_ENGINE_EN
	if(flow_index < AudioEffect_GetTotalFlowNum() && param_mode_index0 < AudioEffect_Parambin_GetTotalParamNumByName(effect_flow_name[flow_index]))
	{
		mainAppCt.effect_flow_index = flow_index;
		mainAppCt.effect_param_mode_index = param_mode_index0;
	}
#else
	if(flow_index < AudioEffect_GetTotalFlowNum() && param_mode_index0 < AudioEffect_Parambin_GetTotalParamNumByName(effect_flow_name[flow_index][0]) &&
			((strlen(effect_flow_name[flow_index][1]) == 0) || param_mode_index1 < AudioEffect_Parambin_GetTotalParamNumByName(effect_flow_name[flow_index][1])))   //双图允许图1为空
	{
		mainAppCt.effect_flow_index = flow_index;
		mainAppCt.effect_param_mode_groupindex[0] = param_mode_index0;
		mainAppCt.effect_param_mode_groupindex[1] = param_mode_index1;
	}
#endif
	else
	{
        APP_DBG("Error: Invalid flow_index %d\n", flow_index);
		return;
	}
}

/** !!!new effect mode will be switched after AudioEffect_EffectMode_Refresh() or AudioEffect_Parambin_Init()
 * flow_index: 图的名称
 * param_mode_index：音效参数index
 * 单图：param_mode_index1参数无效
 **/
void AudioEffect_Parambin_SwitchEffectByName(const char *Flow_name, int16_t param_mode_index0, int16_t param_mode_index1)
{
	uint16_t flow_index = 0;
	uint16_t max_flows = AudioEffect_GetTotalFlowNum();
	uint8_t *flow_data;
	uint32_t flow_size = 0;
	bool found_Flow = FALSE;

	// 如果传入的模式名称为空，则直接返回
	if(Flow_name == NULL || strlen(Flow_name) == 0)
	{
		APP_DBG("Invalid mode name,Please check.\n");
		return;
	}
#ifndef ROBOEFFECT_DOUBLE_ENGINE_EN
	for(int i = 0; i < max_flows; i++)
	{
		if(strcmp(Flow_name, effect_flow_name[i]) == 0)
		{
			flow_index = i;
			found_Flow = TRUE;
			break;
		}
	}
#else
	for(int i = 0; i < max_flows; i++)
	{
		if((strcmp(Flow_name, effect_flow_name[i][0]) == 0) || (strcmp(Flow_name, effect_flow_name[i][1]) == 0))
		{
			flow_index = i;
			found_Flow = TRUE;
			break;
		}
	}
#endif

	if(!found_Flow)
	{
		APP_DBG("Invalid mode name.\n");
		return;
	}

	//uint8_t *mode_data_by_name = NULL, *mode_data_by_index = NULL;
	flow_data = roboeffect_parambin_get_flow_by_name((uint8_t *)AudioEffect_Parambin_GetFlashEffectAddr(), (char *)Flow_name, &flow_size);
//	uint8_t *sub_type_data = roboeffect_parambin_get_sub_type((uint8_t *)flow_data, flow_size, ROBO_PB_SUBTYPE_MODE_PARAMS, TRUE, NULL);
//	roboeffect_parambin_get_mode_param_by_name(sub_type_data, mode_name, &mode_data_by_name, NULL);
//	roboeffect_parambin_get_mode_data_by_index(sub_type_data, param_target, &mode_data_by_index, NULL, NULL, NULL);

// 切换到目标模式
#ifndef ROBOEFFECT_DOUBLE_ENGINE_EN
	if(flow_index < AudioEffect_GetTotalFlowNum() && param_mode_index0 < AudioEffect_Parambin_GetTotalParamNumByName(effect_flow_name[flow_index]) && flow_data != NULL)
	{
		mainAppCt.effect_flow_index = flow_index;
		mainAppCt.effect_param_mode_index = param_mode_index0;
	}
#else
	if(flow_index < AudioEffect_GetTotalFlowNum() && param_mode_index0 < AudioEffect_Parambin_GetTotalParamNumByName(effect_flow_name[flow_index][0]) &&
			((strlen(effect_flow_name[flow_index][1]) == 0) || param_mode_index1 < AudioEffect_Parambin_GetTotalParamNumByName(effect_flow_name[flow_index][1])))   //双图允许图1为空
	{
		mainAppCt.effect_flow_index = flow_index;
		mainAppCt.effect_param_mode_groupindex[0] = param_mode_index0;
		mainAppCt.effect_param_mode_groupindex[1] = param_mode_index1;
	}
#endif
	else
	{
		APP_DBG("Flow '%s' not found, restored to original mode.\n", Flow_name);
	}
}

uint8_t *AudioEffect_Parambin_GetFlowNameByIndex(uint16_t flow_index)
{
	if(flow_index >= AudioEffect_Parambin_GetTotalFlowNum())
	{
		return NULL;
	}
	return roboeffect_parambin_get_flow_by_index((uint8_t *)AudioEffect_Parambin_GetFlashEffectAddr(), flow_index, NULL);
}

uint8_t AudioEffect_Parambin_GetEffectAddr(AudioEffectParambinContext *AudioEffectParambin, AUDIO_EFFECT_NAME_NUM effect_index)
{
	if(AudioEffectParambin==NULL) return 0;

	if(effect_index >= EFFECT_MAX)
	{
		return 0;
	}
	return roboeffect_parambin_get_addr_by_name(AudioEffectParambin->context_memory, audio_effect_name_map[effect_index].enum_str);
}

/*
 * Preset 参数查询：从 Flash Preset 表读取参数值（只读，参数含 enable）
 * param_index: 参数索引，0xff = 全部参数
 * preset_param_size: [out] 返回数组元素个数
 */
int16_t* AudioEffect_PresetGet(AudioEffectParambinContext *AudioEffectParambin, AUDIO_EFFECT_NAME_NUM effect_index, uint8_t param_index,
		AUDIO_EFFECT_PRESET_NUM preset_index, uint32_t *preset_param_size)
{
	if((AudioEffectParambin->context_memory == NULL) || (effect_index >= EFFECT_MAX))
		return NULL;

	uint8_t *param;
	if(ROBOEFFECT_ERROR_OK != roboeffect_parambin_get_preset_param_by_name(AudioEffectParambin->sub_type_data, AudioEffect_Parambin_GetEffectAddr(AudioEffectParambin, effect_index),
												 param_index, audio_effect_preset_map[preset_index].enum_str, &param, preset_param_size))
	{
		return NULL;
	}

	if(preset_param_size)
		(*preset_param_size) /= 2;

	return (int16_t *)param;
}

/*
 * Preset 参数应用：从 Flash Preset 表查参（参数含 enable），按 step 索引取值，写入 RAM 生效
 * param_index: 参数索引，0xff = 全部参数
 * step: Preset 数组中的步进索引（param_index == 0xff 时忽略）
 */
ROBOEFFECT_ERROR_CODE AudioEffect_PresetSet(AudioEffectParambinContext *AudioEffectParambin, AUDIO_EFFECT_NAME_NUM effect_index, uint8_t param_index,
		AUDIO_EFFECT_PRESET_NUM preset_index, uint8_t step)
{
	if((AudioEffectParambin->context_memory == NULL) || (effect_index >= EFFECT_MAX))
		return -1;

	uint32_t preset_size = 0;
	int16_t *preset_data = AudioEffect_PresetGet(AudioEffectParambin, effect_index, param_index, preset_index, &preset_size);

	if(preset_data == NULL)
	{
		DBG("AUDIO_EFFECT_NAME: %s not exists or preset not found\n", audio_effect_name_map[effect_index].enum_str);
		return ROBOEFFECT_EFFECT_NOT_EXISTED;
	}

	if(param_index != 0xff && step >= preset_size)
	{
		DBG("PresetSet step[%d] out of range[%d]\n", step, preset_size);
		return ROBOEFFECT_PARAMS_ERROR;
	}

	uint8_t addr = AudioEffect_Parambin_GetEffectAddr(AudioEffectParambin, effect_index);
	int16_t *val = (param_index == 0xff) ? preset_data : &preset_data[step];
	return AudioEffect_ParamSet(AudioEffectParambin->context_memory, addr, param_index, val);
}

bool AudioEffect_Parambin_GetEffectListInfoByName(const char *FlowName, uint16_t param_mode_index, uint32_t *sample_rate, uint32_t *frame_size)
{
	uint32_t AUDIOEFFECT_FLASHBIN_ADDRESS = AudioEffect_Parambin_GetFlashEffectAddr();
	uint8_t *flow_data;
	roboeffect_effect_list_info *user_effect_list = NULL;
	uint8_t *sub_type_data;
	uint8_t *user_effect_parameters;

	if(roboeffect_parambin_check_whole_bin((uint8_t *)AUDIOEFFECT_FLASHBIN_ADDRESS, NULL) == ROBOEFFECT_ERROR_OK)
	{
		uint32_t param_size = 0;
		uint32_t flow_size = 0;

		if((flow_data = roboeffect_parambin_get_flow_by_name((uint8_t *)AUDIOEFFECT_FLASHBIN_ADDRESS, (char *)FlowName, &flow_size)) != NULL)
		{
			roboeffect_parambin_get_sub_type((uint8_t *)flow_data, flow_size, ROBO_PB_SUBTYPE_SCRIPT, FALSE, NULL);
			user_effect_list = (roboeffect_effect_list_info *)roboeffect_parambin_get_sub_type((uint8_t *)flow_data, flow_size, ROBO_PB_SUBTYPE_EFFECTS_INFO, TRUE, NULL);
			sub_type_data = roboeffect_parambin_get_sub_type((uint8_t *)flow_data, flow_size, ROBO_PB_SUBTYPE_MODE_PARAMS, TRUE, NULL);
			roboeffect_parambin_get_mode_data_by_index(sub_type_data, param_mode_index, &user_effect_parameters, &param_size, NULL, NULL);

			if(user_effect_list && sub_type_data && user_effect_parameters)
			{
//				APP_DBG("[AudioEffect_Parambin] AudioEffect_Parambin_Check ok!\n");
			}
			else
			{
				APP_DBG("[AudioEffect_Parambin] WARNING! Parambin_Check have null pointer!\n");
				return FALSE;
			}
		}
		else
		{
			APP_DBG("[AudioEffect_Parambin] ERROR! Parambin_Check failed!\n");
			return FALSE;
		}
	}
	else
	{
		APP_DBG("[AudioEffect_Parambin] ERROR! Parambin_Check failed!\n");
		return FALSE;
	}

	*sample_rate = user_effect_list->sample_rate;
	*frame_size = roboeffect_parambin_estimate_frame_size(flow_data, param_mode_index, user_effect_list);
	return true;
}

const char *AudioEffect_Parambin_GetSourceNameByIndex(AudioEffectParambinContext *AudioEffectParambin, uint16_t source_index)
{
	roboeffect_effect_steps_table * totle = NULL;
	const char *name = NULL;
	name = (const char *)roboeffect_parambin_get_sub_type((uint8_t *)AudioEffectParambin->flow_data, AudioEffectParambin->flow_size, ROBO_PB_SUBTYPE_IO_NAME, TRUE, NULL);
	totle = (roboeffect_effect_steps_table *)roboeffect_parambin_get_sub_type((uint8_t *)AudioEffectParambin->flow_data,AudioEffectParambin->flow_size, ROBO_PB_SUBTYPE_FLOW_INFO, TRUE, NULL);

	if((name != NULL) && (totle != NULL) && (source_index < totle->src_unit_num))
	{
		name = name + (source_index * 32);
		return name;
	}
	else
	{
		return NULL;
	}
}

const char *AudioEffect_Parambin_GetSinkNameByIndex(AudioEffectParambinContext *AudioEffectParambin, uint16_t sink_index)
{
	roboeffect_effect_steps_table * totle = NULL;
	const char *name = NULL;
	name = (const char *)roboeffect_parambin_get_sub_type((uint8_t *)AudioEffectParambin->flow_data, AudioEffectParambin->flow_size, ROBO_PB_SUBTYPE_IO_NAME, TRUE, NULL);
	totle = (roboeffect_effect_steps_table *)roboeffect_parambin_get_sub_type((uint8_t *)AudioEffectParambin->flow_data,AudioEffectParambin->flow_size, ROBO_PB_SUBTYPE_FLOW_INFO, TRUE, NULL);

	if((name != NULL) && (totle != NULL) && (sink_index < totle->des_unit_num))
	{
		name = name + (totle->src_unit_num * 32) + (sink_index * 32);
		return name;
	}
	else
	{
		return NULL;
	}
}
#endif
