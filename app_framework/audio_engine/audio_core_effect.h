#ifndef __AUDIO_CORE_EFFECT_H__
#define __AUDIO_CORE_EFFECT_H__

#include "roboeffect_api.h"

#if 0 //2026.1.7 //´ýÉ¾³ý
/**
 * @brief Effect parameters for roboeffect engine
 */
typedef struct _AUDIOEFFECT_EFFECT_PARA
{
	roboeffect_effect_list_info 	*user_effect_list;
	roboeffect_effect_steps_table 	*user_effect_steps;
	uint8_t 						*user_effects_script;
	uint8_t 						*user_effect_name;

	uint8_t 						*user_effect_parameters;
	uint8_t 						*user_module_parameters;
	uint32_t 						(*get_user_effects_script_len)(void);
}AUDIOEFFECT_EFFECT_PARA;
#endif

#ifdef CFG_EFFECT_PARAM_IN_FLASH_EN
	#define	EFFECT_FLASH_MODE_NAME	"Flash_"
#endif

#endif
