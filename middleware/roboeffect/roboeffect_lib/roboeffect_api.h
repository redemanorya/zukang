/**
 **************************************************************************************
 * @file    roboeffect_api.h
 * @brief   roboeffect memory management
 *
 * @author  Castle Cai
 * @version V2.37.1
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#ifndef __ROBOEFFECT_API_H__
#define __ROBOEFFECT_API_H__

/*Roboeffect Library version*/
#define ROBOEFFECT_LIB_VER "2.48.1"

#include <stdio.h>
#include <nds32_intrinsic.h>
#include "type.h"
// #include "audio_effect_library.h"
#include "fader.h"

#define EXEC_PROC_MEM_BLOCKS 10
#define EXEC_PROC_STEP_MAX 50
#define EXEC_EFFECT_NODE_MAX 40
#define EXEC_MEM_4K_SIZE (4096)

#define ROBOEFFECT_START_ADDR (0x80)
#define ROBOEFFECT_FIRST_ADDR (ROBOEFFECT_START_ADDR + 1)

#define ROBOEFFECT_IO_TYPE_SRC 1
#define ROBOEFFECT_IO_TYPE_DES 2


#define ROBOEFFECT_CH_NA				0
#define ROBOEFFECT_CH_MONO          1
#define ROBOEFFECT_CH_STEREO        2
#define ROBOEFFECT_CH_MONO_STEREO   3


#define PARAMBIN_LEN_WIDTH (4)
#define PARAMBIN_SUBTYPE_WIDTH (4)
#define PARAMBIN_NAME_WIDTH (32)

#define PARAMBIN_MAGIC_NUM (0xA55AB44B)

#define PARAMBIN_VER_H 0
#define PARAMBIN_VER_M 10
#define PARAMBIN_VER_L 0

#define ALIGN4(x) (((x) + 3) & ~3)

#define IO_UNIT_ID(ptr) ((ptr)->io_id)
#define IO_UNIT_MEM_ID(ptr) ((ptr)->mem_id)
#define IO_UNIT_WIDTH(ptr) ((ptr)->width)
#define IO_UNIT_CH(ptr) ((ptr)->ch)
#define IO_UNIT_NAME(ptr) ((ptr)->name)
#define IO_UNIT_VALID(ptr) \
		((ptr)->width == BITS_16 || (ptr)->width == BITS_24 || (ptr)->width == BITS_32 ? TRUE : FALSE)
#define IO_UNIT_WIDTH_BYTES(ptr) \
    (((ptr)->width == BITS_16) ? 1 : (((ptr)->width == BITS_24) ? 2 : -1))



typedef enum _roboeffect_effect_type_enum
{
	ROBOEFFECT_AUTO_TUNE = 0x00,
	ROBOEFFECT_DC_BLOCKER,
	ROBOEFFECT_DRC,
	ROBOEFFECT_ECHO,
	ROBOEFFECT_EQ,
	ROBOEFFECT_NOISE_SUPPRESSOR_EXPANDER,
	ROBOEFFECT_FREQ_SHIFTER,
	ROBOEFFECT_HOWLING_SUPPRESSOR,
	ROBOEFFECT_NOISE_GATE,
	ROBOEFFECT_PITCH_SHIFTER,
	ROBOEFFECT_REVERB,
	ROBOEFFECT_SILENCE_DETECTOR,
	ROBOEFFECT_THREE_D,
	ROBOEFFECT_VIRTUAL_BASS,
	ROBOEFFECT_VOICE_CHANGER,
	ROBOEFFECT_GAIN_CONTROL,
	ROBOEFFECT_VOCAL_CUT,
	ROBOEFFECT_REVERB_PRO,
	ROBOEFFECT_VOICE_CHANGER_PRO,
	ROBOEFFECT_PHASE_CONTROL,
	ROBOEFFECT_VOCAL_REMOVER,
	ROBOEFFECT_PITCH_SHIFTER_PRO,
	ROBOEFFECT_VIRTUAL_BASS_CLASSIC,
	ROBOEFFECT_PCM_DELAY,
	ROBOEFFECT_HARMONIC_EXCITER,
	ROBOEFFECT_CHORUS,
	ROBOEFFECT_AUTO_WAH,
	ROBOEFFECT_STEREO_WIDENER,
	ROBOEFFECT_PINGPONG,
	ROBOEFFECT_THREE_D_PLUS,
	ROBOEFFECT_SINE_GENERATOR,
	ROBOEFFECT_NOISE_SUPPRESSOR_BLUE,
	ROBOEFFECT_FLANGER,
	ROBOEFFECT_FREQ_SHIFTER_FINE,
	ROBOEFFECT_OVER_DRIVE,
	ROBOEFFECT_DISTORTION_DS1,
	ROBOEFFECT_EQ_DRC,
	
	ROBOEFFECT_AEC,
	ROBOEFFECT_COMPANDER,
	ROBOEFFECT_LOW_LEVEL_COMPRESSOR,
	ROBOEFFECT_BEAT_TRACKER,
	ROBOEFFECT_ENGINE_SOUND,
	ROBOEFFECT_BIQUAD,
	ROBOEFFECT_CHORUS2,
	ROBOEFFECT_REVERB_PLATE,
	ROBOEFFECT_HOWLING_SUPPRESSOR_FINE,
	ROBOEFFECT_HOWLING_GUARD,
	ROBOEFFECT_VIRTUAL_BASS_TD,
	ROBOEFFECT_OVER_DRIVE_PLOY,
	ROBOEFFECT_TREMOLO,
	ROBOEFFECT_BEAMFORMING,
	ROBOEFFECT_VIRTUAL_SURROUND_2CH,
	ROBOEFFECT_ROBOT_TONE,
	ROBOEFFECT_NOISE_GENERATOR,
	ROBOEFFECT_NOISE_SUPPRESSOR_BLUE_DUAL,
	ROBOEFFECT_PHASE_INVERTER,
	ROBOEFFECT_FILTER_BUTTERWORTH,
	ROBOEFFECT_DYNAMIC_EQ,
	ROBOEFFECT_VAD,
	ROBOEFFECT_LR_BALANCER,
	ROBOEFFECT_HOWLING_SUPPRESSOR_SPECIFIED,
	ROBOEFFECT_DRC_LEGACY,
	ROBOEFFECT_PCM_DELAY_MS,
	ROBOEFFECT_DISTORTION_EXP,
	ROBOEFFECT_VIBRATO,
	ROBOEFFECT_PITCH_DETECTOR,
	ROBOEFFECT_FILTER_FIR,
	ROBOEFFECT_NOISE_SUPPRESSOR_BLUE_ADV,
	ROBOEFFECT_LR_SWAP,
	ROBOEFFECT_REVERB_PLATE_ME,
	ROBOEFFECT_DRC_SC,
	ROBOEFFECT_SILENCE_DETECTOR_L,
	ROBOEFFECT_PHASE_CONTROL_PLUS,
	ROBOEFFECT_PEAK_ENVELOPE,

	/*node type below*/
	ROBOEFFECT_FADER,//
	ROBOEFFECT_DOWNMIX_2TO1,
	ROBOEFFECT_UPMIX_1TO2,

	//channel
	ROBOEFFECT_CHANNEL_COMBINER,
	ROBOEFFECT_CHANNEL_SELECTOR,
	ROBOEFFECT_ROUTE_SELECTOR,


	ROBOEFFECT_USER_DEFINED_EFFECT_BEGIN,//
} roboeffect_effect_type_enum;

typedef enum _ROBOEFFECT_ERROR_CODE
{
  ROBOEFFECT_EFFECT_NOT_EXISTED = -256,
	ROBOEFFECT_EFFECT_PARAMS_NOT_FOUND,
	ROBOEFFECT_INSUFFICIENT_MEMORY,
	ROBOEFFECT_EFFECT_INIT_FAILED,
	ROBOEFFECT_ILLEGAL_OPERATION,
	ROBOEFFECT_EFFECT_LIB_NOT_MATCH_1,//for parameters
	ROBOEFFECT_EFFECT_LIB_NOT_MATCH_2,//for lib Macro
	ROBOEFFECT_ADDRESS_NOT_EXISTED,
	ROBOEFFECT_PARAMS_ERROR,
	ROBOEFFECT_FRAME_SIZE_ERROR,
	ROBOEFFECT_MEMORY_SIZE_QUERY_ERROR,
	ROBOEFFECT_EFFECT_VER_NOT_MATCH_ERROR,//one effect version not match
	ROBOEFFECT_LIB_VER_NOT_MATCH_ERROR,//roboeffect lib version in parameters not match
	ROBOEFFECT_3RD_PARTY_LIB_NOT_MATCH_ERROR,//third party library not match

	ROBOEFFECT_PARAMBIN_ERROR,//Invalid format for parameter bin
	ROBOEFFECT_CONTEXT_MEMORY_ERROR,//roboeffect context memory error, maybe a NULL
	ROBOEFFECT_PARAMBIN_DATA_NOT_FOUND,//
	ROBOEFFECT_PARAMBIN_DATA_VER_ERROR,//

	ROBOEFFECT_MSG_VALUE_NAME_NOT_FOUND,//msg value name not found
	ROBOEFFECT_MSG_DATA_ERROR,//msg data error

	ROBOEFFECT_FLUSH_FIFO_FULL,//flush fifo is full
	ROBOEFFECT_APPLY_ERROR,//engine execution error
	ROBOEFFECT_CONFIG_PARAM_ERROR,//set parameter error, maybe parameter not correct
    ROBOEFFECT_EFFECT_CROPPED,//effect is cropped to optimize storage capacity.

	// No Error
	ROBOEFFECT_ERROR_OK = 0, /**< no error */
} ROBOEFFECT_ERROR_CODE;

typedef enum _param_act_method
{
	ACTION_NONE     = 0x00,//
	ACTION_FADEOI   = 0x01,//
	ACTION_FADEOI_1 = 0x02,//
	ACTION_FADEOI_2 = 0x04,//
	ACTION_STEP     = 0x08,//
	ACTION_MAX      = 0xff,
}param_act_method;


typedef enum _roboeffect_width
{
	BITS_0  = 0,
	BITS_16 = 16,
	BITS_24 = 24,
	BITS_32 = 32,
} roboeffect_width;

typedef enum _roboeffect_channel
{
	CH_NONE = 0,
	CH_MONO = 1,
	CH_STEREO = 2,
} roboeffect_channel;

typedef enum _roboeffect_fader_step
{
	FADE_INOUT_NONE = 0,
	FADE_INOUT_OUT,
	FADE_INOUT_OUT_PROC,
	FADE_INOUT_ZERO,
	FADE_INOUT_IN,
	FADE_INOUT_IN_PROC,
} roboeffect_fader_step;

typedef enum _roboeffect_operation
{
	MEM_COPY = 0x01,
	MEM_MIX,
	MEM_CLEAN,
	MEM_INIT_CLEAN,
} roboeffect_operation;

typedef enum _roboeffect_param_cfg_method
{
	METHOD_NONE = 0x00,//only update
	METHOD_INIT = 0x01,//need call init func
	METHOD_CFG_1 = 0x02,//need call config func 1
	METHOD_CFG_2 = 0x04,//need call config func 2
	METHOD_CFG_3 = 0x08,//need call config func 3
	METHOD_CFG_FADEOI = 0x10,//need do fade out/in
	METHOD_CFG_STEP = 0x20,//need apply new param step by step
	METHOD_MAX = 0xff,
}roboeffect_param_cfg_method;

typedef enum _roboeffect_frame_size
{
	FZ_ANY = 0,
	FZ_GEAR,
	FZ_RANGE,
	FZ_ERROR,
}roboeffect_frame_size;

typedef enum _roboeffect_frame_operation
{
	FRAME_OP_INCREASE = 1,
	FRAME_OP_DECREASE = -1,

}roboeffect_frame_operation;

typedef struct _roboeffect_fader_context_inner
{
	uint8_t step;//roboeffect_fader_step
	FaderContext cnx;
} roboeffect_fader_context_inner;

typedef enum _roboeffect_fader_op
{
	FADER_TRIGGER_BYPASS = 0,
	FADER_TRIGGER_MUTE,
	FADER_TRIGGER_FADE_OUT,
	FADER_TRIGGER_FADE_IN,

	FADER_STATUS_READY,
	FADER_STATUS_OUT_PROC,
	FADER_STATUS_OUT_DONE,
	FADER_STATUS_IN_PROC,
	FADER_STATUS_IN_DONE,
	FADER_STATUS_DONE,
}roboeffect_fader_op;


typedef struct _roboeffect_proc_mm_node
{
	uint8_t id;
	void *mem;
	bool is_used;
	uint8_t width;//roboeffect_width
	uint8_t ch;//roboeffect_channel
	// fade_inout_step fade;
} roboeffect_proc_mm_node;

typedef struct _roboeffect_io_unit
{
	uint8_t io_id;
	uint8_t mem_id;
	uint8_t width;//roboeffect_width
	uint8_t ch;//roboeffect_channel
} roboeffect_io_unit;

typedef struct _roboeffect_memory_layout
{
	uint8_t mem_id;
	uint8_t width;//roboeffect_width
	uint8_t ch;//roboeffect_channel
    uint8_t reserved;
} roboeffect_memory_layout;


typedef struct _roboeffect_step
{
	uint8_t id;
	uint8_t operate;
	uint8_t input_a;
	uint8_t input_b;
	uint8_t output_a;
} roboeffect_step;

typedef struct _roboeffect_effect_steps_table
{
	uint8_t step_num;
	uint8_t mem_used;
	uint8_t src_unit_num;
	uint8_t des_unit_num;
	// uint16_t mem_size_tb[EXEC_PROC_MEM_BLOCKS];
	const roboeffect_io_unit *src_unit;
	const roboeffect_io_unit *des_unit;
	const uint32_t *step;
	
} roboeffect_effect_steps_table;

typedef struct _roboeffect_exec_effect_info
{
	uint8_t addr;
	roboeffect_effect_type_enum type;
	roboeffect_width width;
	uint8_t ch_num;
} roboeffect_exec_effect_info;//parameter header for all

typedef struct _roboeffect_effect_list_info
{
	uint32_t count;//
	uint32_t sample_rate;
	uint32_t frame_size;
	const roboeffect_exec_effect_info *effect_info;
	const uint8_t *robo_addi_info;
	const uint8_t *mounted_param;
} roboeffect_effect_list_info;//parameter header for all

typedef struct _roboeffect_memory_size_query
{
	uint32_t sample_rate;
	uint32_t frame_size;
	uint32_t sys_frame_size;
	uint32_t ch_num;
	uint32_t data_width;
	int16_t *params;
	uint8_t *preset_entry;//for get vectors
	uint8_t address;
} roboeffect_memory_size_query;

typedef struct _roboeffect_memory_size_response
{
	uint32_t context_memory_size;
	uint32_t additional_memory_size;
	uint32_t scratch_memory_size;
	int16_t effect_err_code;//is output
} roboeffect_memory_size_response;

typedef struct _roboeffect_frame_size_response
{
	uint32_t calced_frame_size;
	uint32_t effect_frame_size;
} roboeffect_frame_size_response;

typedef struct _roboeffect_effect_err
{
	uint8_t err_effect_addr;
	int32_t err_effect_code;
} roboeffect_effect_err;

typedef bool (*roboeffect_effect_apply_func)(void *node, int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t n);
typedef bool (*roboeffect_effect_init_func)(void *node);
typedef bool (*roboeffect_effect_config_func)(void *node, int16_t *new_param, uint8_t param_num, uint8_t len);//can be all param(param_num=0xff) or only ONE param(param_num=1)
typedef bool (*roboeffect_effect_memory_size_func)(roboeffect_memory_size_query *query, roboeffect_memory_size_response *response);

/*********************************************parambin start*********************************************************/


typedef enum _roboeffect_parambin_sub_type
{
	ROBO_PB_SUBTYPE_SCRIPT = 0x00,
	ROBO_PB_SUBTYPE_EFFECTS_LIST,
	ROBO_PB_SUBTYPE_EFFECTS_INFO,
	ROBO_PB_SUBTYPE_SOURCE,
	ROBO_PB_SUBTYPE_SINK,
	ROBO_PB_SUBTYPE_STEPS,
	ROBO_PB_SUBTYPE_FLOW_INFO,
	ROBO_PB_SUBTYPE_PARAMS_MODE_INFO,
	ROBO_PB_SUBTYPE_PRESET_INFO,
	ROBO_PB_SUBTYPE_IO_NAME,
	ROBO_PB_SUBTYPE_MODE_PARAMS,
	ROBO_PB_SUBTYPE_EFFECT_NAME,
	ROBO_PB_SUBTYPE_MSG_DATA,
	ROBO_PB_SUBTYPE_MAX,


} roboeffect_parambin_sub_type;

typedef enum _roboeffect_mode_params_type
{
	ROBO_PB_MP_TYPE_BRIEF = 0x01,
	ROBO_PB_MP_TYPE_MODE_DATA,
	ROBO_PB_MP_TYPE_PRESET_DATA,
	ROBO_PB_MP_TYPE_VECTOR_DATA,

} roboeffect_mode_params_type;

typedef enum _roboeffect_msg_entry_type
{
	ROBO_MG_ET_TYPE_BRIEF = 0x01,
	ROBO_MG_ET_TYPE_SESSTION,
	ROBO_PB_MP_TYPE_MESSAGE,

} roboeffect_msg_entry_type;

typedef enum _roboeffect_vector_type
{
	VECTOR_TYPE_INT8 = 0x00,
	VECTOR_TYPE_UINT8,
	VECTOR_TYPE_INT16,
	VECTOR_TYPE_UINT16,
	VECTOR_TYPE_INT32,
	VECTOR_TYPE_UINT32,
	VECTOR_TYPE_INT64,
	VECTOR_TYPE_UINT64,
	VECTOR_TYPE_FLOAT,
	VECTOR_TYPE_DOUBLE,
} roboeffect_vector_type;

#pragma pack(1)
typedef struct _roboeffect_parambin_header
{
	char id_char[4];
	uint32_t total_length;
	uint8_t version[4];
	uint8_t robo_version[4];
	uint16_t flow_cnt;
	uint16_t flow_name_len;
	uint16_t current_flow_cnt;
	uint16_t chip_id;//

} roboeffect_parambin_header;

typedef struct _roboeffect_parambin_flow_pair
{
	uint16_t flow_index;
	uint16_t param_mode_index;
} roboeffect_parambin_flow_pair;

typedef struct _roboeffect_vector_data
{
	uint16_t size;
	uint8_t type;
	uint8_t data[];
} roboeffect_vector_data;

#pragma pack()

typedef struct _roboeffect_parambin_brief
{
	uint32_t bin_size;
	const uint8_t *protocol_ver;
	const uint8_t *roboeffect_ver;
	uint16_t flow_num;
	uint16_t total_str_size;
	uint16_t current_flow_num;
	uint16_t chip_id;
	const roboeffect_parambin_flow_pair *flow_table;
	const char *name_str;
	const uint8_t *password;
	const uint8_t *flow_ptr;
	uint32_t parambin_crc32;
	uint16_t addi_data_len;
	uint8_t *addi_data;


} roboeffect_parambin_brief;

typedef enum _roboeffect_param_data_type
{
    PARAM_TYPE_LENGTH = 0,
    PARAM_TYPE_SAMPLE_RATE,
    PARAM_TYPE_FRAME_SIZE,
    PARAM_TYPE_EFFECT,

    PARAM_TYPE_MAX,

} roboeffect_param_data_type;


/*********************************************parambin end*********************************************************/

typedef struct _roboeffect_effect_property_struct
{
	uint8_t type_num;
	uint8_t channel_support;//1=only mono, 2=only stereo, 3=mono or stereo
	uint16_t param_size;//param numbers one as half-word
	uint8_t fz_type;
	uint8_t fz_cnt;
	uint16_t fz_list[8];
	// uint32_t cn_size;

	roboeffect_effect_init_func init_func;
	roboeffect_effect_config_func config_func;//if parameter change in init, should call init_func
	roboeffect_effect_apply_func apply_func;

	roboeffect_effect_memory_size_func memory_size_func;

	// int16_t param_list[ROBOEFFECT_PARAM_LEN_MAX];
	// roboeffect_param_cfg_method cfg_method[ROBOEFFECT_PARAM_LEN_MAX];//define how to config parameters
} roboeffect_effect_property_struct;//parameter header for all


typedef struct _roboeffect_user_defined_effect_info
{
	uint32_t sample_rate;
	uint8_t ch_num;
	uint8_t parameters_len;
	roboeffect_width width;
	uint16_t framesize_running;
	void* context_memory;
	void* additional_memory;

	int16_t* parameters;
	int16_t* parameters_backup;
	param_act_method param_act;

	bool is_active;

	uint8_t *scratch_memory;
} roboeffect_user_defined_effect_info;//


/**
 * @brief Get context current size for roboeffect
 * 
 * @param effect_steps : execution table context
 * @param effect_list : execution effect list
 * @param parameters : effects parameters data
 * @return int32_t : memory size in Bytes; if < 0, check ROBOEFFECT_ERROR_CODE
 */
int32_t roboeffect_estimate_memory_size(const roboeffect_effect_steps_table *effect_steps, const roboeffect_effect_list_info *effect_list, const uint8_t *parameters);

/**
 * @brief Get context memory size of ONE effect according to address, can be called BEFORE roboeffect_init().
 * 
 * @param address : effect address 
 * @param effect_list : execution effect list
 * @param parameters : effects parameters data
 * @return int32_t : memory size in Bytes; if < 0, check ROBOEFFECT_ERROR_CODE
 */
int32_t roboeffect_estimate_effect_size(uint8_t address, const roboeffect_effect_list_info *effect_list, const uint8_t *parameters);

/**
 * @brief Get context memory size of ONE effect according to address, should be called AFTER roboeffect_init();
 *
 * @param main_context : main context memory allocated by user.
 * @param address : effect address .
 * @param context_size : output context size of effect, only valid when the return value is ROBOEFFECT_ERROR_CODE.
 * @return ROBOEFFECT_ERROR_CODE 
 */
ROBOEFFECT_ERROR_CODE roboeffect_get_effect_size(void *context, uint8_t address, uint32_t *context_size);

/**
 * @brief Estimate frame size of the flow chart based on parameters.
 * 
 * @param effect_list : execution effect list
 * @param parameters : effects parameters data
 * @return uint32_t : 0 for error
 */
uint32_t roboeffect_estimate_frame_size(roboeffect_effect_list_info *exec_efft, uint8_t *param);

/**
 * @brief initial context for roboeffect
 * 
 * @param main_context : main context memory allocated by user
 * @param context_size : context memory size
 * @param effect_steps : execution table context
 * @param effect_list : execution effect list
 * @param parameters : effects parameters data
 * @return ROBOEFFECT_ERROR_CODE 
 */
ROBOEFFECT_ERROR_CODE roboeffect_init(void *main_context, uint32_t context_size, const roboeffect_effect_steps_table *effect_steps, const roboeffect_effect_list_info *effect_list, const uint8_t *parameters);

/**
 * @brief main loop for roboeffect application
 * 
 * @param main_context : context memory allocated by user
 * @return ROBOEFFECT_ERROR_CODE 
 */
ROBOEFFECT_ERROR_CODE roboeffect_apply(void *main_context);

/**
 * @brief Get free memory space in main_context
 * 
 * @param main_context : main context memory allocated by user
 * @return uint32_t 
 */
uint32_t roboeffect_get_free_memory_space(void *main_context);

/**
 * @brief Get source buffer block by source id
 * 
 * @param main_context: context memory allocated by user
 * @param id: source id
 * @return void* NULL if failed
 */
void *roboeffect_get_source_buffer(void *main_context, uint8_t id);

/**
 * @brief Get sink buffer block by source id
 * 
 * @param main_context: context memory allocated by user
 * @param id: source id
 * @return void* NULL if failed
 */
void *roboeffect_get_sink_buffer(void *main_context,  uint8_t id);

/**
 * @brief Get sink/source buffer size in bytes
 * 
 * @param main_context: context memory allocated by user
 * @return buffer size in bytes
 */
uint32_t roboeffect_get_buffer_size(void *main_context);

/**
 * @brief Enable/Disable one effect
 * 
 * @param main_context: context memory allocated by user
 * @param addr: effect's address, start from ROBOEFFECT_FIRST_ADDR
 * @param enable: 0 for disable, 1 for enable
 * @return ROBOEFFECT_ERROR_CODE 
 */
ROBOEFFECT_ERROR_CODE roboeffect_enable_effect(void *main_context, uint8_t addr, uint8_t enable);

/**
 * @brief Enable/Disable ALL effect
 * 
 * @param main_context 
 * @param enable: 0 for disable, 1 for enable
 * @return ROBOEFFECT_ERROR_CODE 
 */
ROBOEFFECT_ERROR_CODE roboeffect_enable_all_effects(void *main_context, uint8_t enable);

/**
 * @brief Get effect enable/disable status
 * 
 * @param main_context: context memory allocated by user
 * @param addr: effect node's address, start from ROBOEFFECT_FIRST_ADDR
 * @return true: enabled
 * @return false: disabled 
 */
bool roboeffect_get_effect_status(void *main_context, uint8_t addr);

/**
 * @brief Set parameter for one effect by address
 * 
 * @param main_context: context memory allocated by user 
 * @param addr: effect node's address, start from ROBOEFFECT_FIRST_ADDR 
 * @param param_index: parameter index, start with 0, 0xff indicates all parameters 
 * @param param_input: parameter data
 * @return ROBOEFFECT_ERROR_CODE 
 */
ROBOEFFECT_ERROR_CODE roboeffect_set_effect_parameter(void *main_context, uint8_t addr, uint8_t param_index, int16_t *param_input);


/**
 * @brief Get parameters for one effect by address
 * 
 * @param main_context: context memory allocated by user  
 * @param addr: effect node's address, start from ROBOEFFECT_FIRST_ADDR  
 * @param param_index: parameter index, start with 0, 0xff indicates all parameters  
 * @return const int16_t*: parameters data
 */
const int16_t* roboeffect_get_effect_parameter(void *main_context, uint8_t addr, uint8_t param_index);

/**
 * @brief Get the number of effect parameters
 * 
 * @param main_context : context memory allocated by user  
 * @param address : effect node's address, start from ROBOEFFECT_FIRST_ADDR  
 * @return int16_t 
 */
int16_t roboeffect_get_effect_parameter_count(void *main_context, uint8_t address);

/**
 * @brief Get suit frame size according to current_frame_size given
 * 
 * @param main_context : context memory allocated by user  
 * @param target_frame_size : Generally, it is the current frame_size, but can also fill in the desired frame_size.
 * @param address : effect node's address, start from ROBOEFFECT_FIRST_ADDR  
 * @param operation : 1 for increasing node, -1 for decreasing node.
 * @return error if 0 
 */

uint32_t roboeffect_recommend_frame_size_upon_effect_change(void *main_contex, uint32_t target_frame_size, uint8_t address, int8_t operation);
/**
 * @brief Get effect type name.
 * 
 * @param main_context : context memory allocated by user  
 * @param address : effect node's address, start from ROBOEFFECT_FIRST_ADDR  
 * @param type_output : [output]name string, user need send a block of memory to hold it.
 * @param type_code : [output]effect type code, refer to roboeffect_effect_type_enum.
 * @return ROBOEFFECT_ERROR_CODE 
 */
ROBOEFFECT_ERROR_CODE roboeffect_get_effect_type(void *main_context, uint8_t address, char *type_output, uint8_t *type_code);

/**
 * @brief Get effect class version number.
 * 
 * @param main_context : context memory allocated by user  
 * @param address : effect node's address, start from ROBOEFFECT_FIRST_ADDR  
 * @param ver_output : output version number at ver_output[0], ver_output[1], ver_output[2]
 * @return ROBOEFFECT_ERROR_CODE 
 */
ROBOEFFECT_ERROR_CODE roboeffect_get_effect_version(void *main_context, uint8_t address, uint8_t ver_output[]);

/**
 * @brief Get roboeffect error code
 * 
 * @param main_context : context memory allocated by user  
 * @return ROBOEFFECT_ERROR_CODE
 */
ROBOEFFECT_ERROR_CODE roboeffect_get_error_code(void *main_context);


/**
 * @brief Get the effect address and error code where the error occurred
 * 
 * @param main_context : context memory allocated by user  
 * @param effect_err : [out] pointer to a user-allocated `roboeffect_effect_err` structure
 *                     that will be filled with the error code and the effect address
 *                     where the error occurred (if applicable).
 * @return ROBOEFFECT_ERROR_CODE
 */
ROBOEFFECT_ERROR_CODE roboeffect_get_error_code_extended(void *main_context, roboeffect_effect_err *effect_err);

/**
 * @brief Set roboeffect error code
 * 
 * @param main_context : context memory allocated by user  
 * @param error_code : error code to be set.
 * @return ROBOEFFECT_ERROR_CODE 
 */
ROBOEFFECT_ERROR_CODE roboeffect_set_error_code(void *main_context, int32_t error_code);

/**
 * @brief Get effect context memory pointer
 * 
 * @param main_context : context memory allocated by user  
 * @param address : effect node's address, start from ROBOEFFECT_FIRST_ADDR  
 * @return the pointer of context memory, NULL for error 
 */
void *roboeffect_get_effect_context(void *main_context, uint8_t addr);

/**
 * @brief Get effect parameter pool address and size
 * 
 * @param main_context : context memory allocated by user  
 * @param node_size : pool size in bytes  
 * @return the address of parameter pool, NULL for error.
 */
void *roboeffect_get_effect_params_pool(void *main_context, uint32_t *node_size);

/**
 * @brief Get frame buffer pointer, and get frame buffer total size
 * 
 * @param main_context : context memory allocated by user  
 * @param **buffer_pointer : output, pointer to frame buffer address
 * @param *buffer_size : output, get frame buffer size in bytes
 * @return ROBOEFFECT_ERROR_CODE 
 */
ROBOEFFECT_ERROR_CODE roboeffect_get_frame_buffer_info(void *main_context, void **buffer_pointer, int32_t *buffer_size);


/**
 * @brief Get corresponding data based on the given type in parameter block
 * 
 * @param *parameter : effect parameter block
 * @param addr : effect node's address if type is PARAM_TYPE_EFFECT, start from ROBOEFFECT_FIRST_ADDR
 * @param data_type : data type
 * @return uint8_t*, a pointer to data. 
 */
uint8_t *roboeffect_get_parameter_section(uint8_t *parameter, uint8_t addr, roboeffect_param_data_type data_type);


/**
 * @brief For user defined effect api, check parameters, should be called in interface named roboeffect_XXXX_config_if
 * 
 * @param node : 1st parameter by roboeffect_XXXX_config_if, the context of effect instance.
 * @param new_param : 2nd parameter by roboeffect_XXXX_config_if
 * @param param_index : 3rd parameter by roboeffect_XXXX_config_if
 * @param len : 4th parameter by roboeffect_XXXX_config_if
 * @param *flag : output flag
 * @return ROBOEFFECT_ERROR_CODE 
 */
ROBOEFFECT_ERROR_CODE roboeffect_user_defined_params_check(void *node, int16_t *new_param, uint8_t param_index, uint8_t len, uint8_t *flag);

/**
 * @brief For user defined effect api, allocate memory, should be called in interface named roboeffect_XXXX_init_if, roboeffect_XXXX_config_if, roboeffect_XXXX_apply_if
 * 		Each effect entity can only allocate one memory block. 
 * 
 * @param node : 1st parameter by roboeffect_XXXX_config_if, the context of effect instance.
 * @param size : the size of the memory that needs to be allocated
 * @return the pointer of the memory allocated .
 */
void* roboeffect_user_defined_malloc(void *node, uint32_t size);


/**
 * @brief Get current effect information block
 * 
 * @param node : 1st parameter by roboeffect_XXXX_init_if, or roboeffect_XXXX_config_if, or roboeffect_XXXX_apply_if, the context of effect instance.
 * @param info : output, current effect information block, refers to roboeffect_user_defined_effect_info.
 * @return ROBOEFFECT_ERROR_CODE.
 */
ROBOEFFECT_ERROR_CODE roboeffect_user_defined_get_info(void *node, roboeffect_user_defined_effect_info *info);




/*********************************************parambin start*********************************************************/

/**
 * @brief Check if the parambin is valid and obtain the length of the parambin
 * 
 * @param parambin_addr : parambin address
 * @param *parambin_size : output, parambin size in bytes; can be NULL if not needed
 * @return ROBOEFFECT_ERROR_CODE : ROBOEFFECT_ERROR_OK for parambin is valid, others for parambin if invalid.
 */
ROBOEFFECT_ERROR_CODE roboeffect_parambin_check_whole_bin(const uint8_t *parambin_addr, uint32_t *parambin_size);

/**
 * @brief Get brief infomation of parambin
 * 
 * @param parambin_addr : parambin address
 * @param *parambin_size : output, parambin size in bytes; can be NULL if not needed
 * @return ROBOEFFECT_ERROR_CODE : ROBOEFFECT_ERROR_OK for parambin is valid, others for parambin if invalid.
 */
ROBOEFFECT_ERROR_CODE roboeffect_parambin_get_brief_info(const uint8_t *parambin_addr, roboeffect_parambin_brief *brief_info);

/**
 * @brief Get information of current flows in pairs
 * 
 * @param parambin_addr : parambin address
 * @param *item_cnt : output, number of flow pairs; can be NULL if not needed
 * @return roboeffect_parambin_flow_pair * : a table of roboeffect_parambin_flow_pair
 */
roboeffect_parambin_flow_pair *roboeffect_parambin_get_current_flow(const uint8_t *parambin_addr, uint32_t *item_cnt);

/**
 * @brief Get password for parameter bin
 * 
 * @param parambin_addr : parambin address
 * @return uint8_t * : pointer to password with 4 bytes
 */
uint8_t *roboeffect_parambin_get_password(const uint8_t *parambin_data);

/**
 * @brief Get flow data by index
 * 
 * @param parambin_addr : parambin address
 * @param flow_index : flow index
 * @param *data_size : output, flow data size in bytes; can be NULL if not needed
 * @return uint8_t * : flow data address, NULL for error
 */
uint8_t *roboeffect_parambin_get_flow_by_index(const uint8_t *parambin_addr, uint32_t flow_index, uint32_t *data_size);

/**
 * @brief Get flow data by name string
 * 
 * @param parambin_addr : parambin address
 * @param name : flow name string
 * @param *data_size : output, flow data size in bytes; can be NULL if not needed
 * @return uint8_t * : flow data address, NULL for error
 */
uint8_t *roboeffect_parambin_get_flow_by_name(const uint8_t *parambin_addr, char *name, uint32_t *data_size);


/**
 * @brief Get sub-package data by type id
 * 
 * @param flow_data : flow data address, maybe returned by roboeffect_parambin_get_flow_by_index() or roboeffect_parambin_get_flow_by_name()
 * @param flow_size : flow data size, maybe returned by roboeffect_parambin_get_flow_by_index() or roboeffect_parambin_get_flow_by_name()
 * @param sub_type : sub type, refer to roboeffect_parambin_sub_type
 * @param is_raw: if is_raw, just return raw data, if not is_raw, return whole sub-package.
 * @param *data_size : output, sub-package data size in bytes; can be NULL if not needed
 * @return uint8_t* : sub-package address, NULL for error
 */
uint8_t *roboeffect_parambin_get_sub_type(const uint8_t *flow_data, uint32_t flow_size, uint32_t sub_type, bool is_raw, uint32_t *data_size);



/**
 * @brief Get context current size for roboeffect, it can be considered the parambin version of the roboeffect_estimate_memory_size() function
 * 
 * @param flow_data : flow data address, maybe returned by roboeffect_parambin_get_flow_by_index() or roboeffect_parambin_get_flow_by_name()
 * @param params_index : the index of parameters group, it can be obtained from the roboeffect_parambin_get_current_flow() function return, or specified by the user.
 * @param effect_list : execution effect list
 * @return int32_t : memory size in Bytes; if < 0, check ROBOEFFECT_ERROR_CODE
 */
int32_t roboeffect_parambin_estimate_memory_size(const uint8_t *flow_data, uint32_t params_index, const roboeffect_effect_list_info *effect_list);


/**
 * @brief Estimate frame size of the flow chart based on parameters, it can be considered the parambin version of the roboeffect_estimate_frame_size() function
 * 
 * @param flow_data : flow data address, maybe returned by roboeffect_parambin_get_flow_by_index() or roboeffect_parambin_get_flow_by_name()
 * @param params_index : the index of parameters group, it can be obtained from the roboeffect_parambin_get_current_flow() function return, or specified by the user.
 * @param effect_list : execution effect list
 *@return uint32_t : 0 for error
 */
uint32_t roboeffect_parambin_estimate_frame_size(const uint8_t *flow_data, uint32_t params_index, const roboeffect_effect_list_info *effect_list);



/**
 * @brief Get context memory size of ONE effect according to address, can be called BEFORE roboeffect_init().
 * 				it can be considered the parambin version of the roboeffect_estimate_effect_size() function
 * 
 * @param flow_data : flow data address, maybe returned by roboeffect_parambin_get_flow_by_index() or roboeffect_parambin_get_flow_by_name()
 * @param params_index : the index of parameters group, it can be obtained from the roboeffect_parambin_get_current_flow() function return, or specified by the user.
 * @param addr: effect's address, start from ROBOEFFECT_FIRST_ADDR
 * @param effect_list : execution effect list
 *@return uint32_t : 0 for error
 */
int32_t roboeffect_parambin_estimate_effect_size(const uint8_t *flow_data, uint32_t params_index, uint8_t address, const roboeffect_effect_list_info *effect_list);


/**
 * @brief initial context for roboeffect, it can be considered the parambin version of the roboeffect_init() function
 * 
 * @param main_context : main context memory allocated by user
 * @param context_size : context memory size
 * @param flow_data : flow data address, maybe returned by roboeffect_parambin_get_flow_by_index() or roboeffect_parambin_get_flow_by_name()
 * @param params_index : the index of parameters group, it can be obtained from the roboeffect_parambin_get_current_flow() function return, or specified by the user.
 * @param effect_list : execution effect list
 * @return int32_t : memory size in Bytes; if < 0, check ROBOEFFECT_ERROR_CODE
 */
ROBOEFFECT_ERROR_CODE roboeffect_parambin_init(void *main_context, uint32_t context_size, const uint8_t *flow_data, uint32_t params_index, const roboeffect_effect_list_info *effect_list);


/**
 * @brief Get source/sink name string by id.
 * 
 * @param main_context : main context memory allocated by user
 * @param id: source/sink id
 * @return const char * : name string of source/sink, NULL for error.
 */
const char *roboeffect_parambin_get_io_name(void *main_context, uint8_t id);


/**
 * @brief Get source/sink unit by name string.
 * 
 * @param main_context : main context memory allocated by user
 * @param name: name string of source/sink, NULL for error.
 * @return const roboeffect_io_unit * : roboeffect_io_unit struct, NULL for error.
 */
const roboeffect_io_unit *roboeffect_parambin_get_io_by_name(void *main_context, const char *name);

/**
 * @brief Get effect address by name string.
 * 
 * @param main_context : main context memory allocated by user
 * @param name: name string of effect, NULL for error.
 * @return uint8_t : effect address with ROBOEFFECT_FIRST_ADDR ~ 0xF0, 0 for error.
 */
uint8_t roboeffect_parambin_get_addr_by_name(void *main_context, const char *name);


/**
 * @brief Checks if the specified IO name matches the device node.
 *
 * This macro compares the given IO name with the name obtained from a device node
 * and checks if the device node is valid.
 *
 * @param io_name The name of the IO to be matched.
 * @param device_node A pointer to the roboeffect_io_unit structure representing the device node.
 * @return Returns true if the IO name matches and the device node is valid, false otherwise.
 */
#define IS_IO_MATCH(io_name, device_node) \
    (strcmp((io_name), roboeffect_parambin_get_io_name(context_memory, IO_UNIT_ID(device_node))) == 0 && IO_UNIT_VALID(device_node))


/**
 * @brief Checks if the specified IO name exists and is valid.
 *
 * This macro retrieves the IO node by name and checks if it is valid.
 * It assigns the found IO node to the io_node variable.
 *
 * @param io_name The name of the IO to be checked.
 * @param io_node A pointer to be assigned to the found roboeffect_io_unit structure if it exists.
 * @return Returns true if the IO node exists and is valid, false otherwise.
 */
#define IS_IO_EXISTED(io_name, io_node) \
		((io_node = roboeffect_parambin_get_io_by_name(context_memory, io_name)) != NULL && IO_UNIT_VALID(io_node))


/**
 * @brief Get mode data by index, mode data including mode parameter data and mode codec parameter data
 * 
 * @param sub_type_data : sub type data address, maybe returned by roboeffect_parambin_get_sub_type()
 * @param mode_index : mode index number, a valid mode index
 * @param **mode_data_out : output pointer, point to mode parameter data
 * @param *mode_data_size: output, the mode parameter data size in bytes
 * @param **codec_data_out : output pointer, point to codec parameter data
 * @param *codec_data_size: output, the codec parameter data size in bytes
 * @return ROBOEFFECT_ERROR_CODE
 */
ROBOEFFECT_ERROR_CODE roboeffect_parambin_get_mode_data_by_index(const uint8_t *sub_type_data, uint32_t mode_index, uint8_t **mode_data_out, uint32_t *mode_data_size, uint8_t **codec_data_out, uint32_t *codec_data_size);


/**
 * @brief Get mode parameter data by name string
 * 
 * @param sub_type_data : sub type data address, maybe returned by roboeffect_parambin_get_sub_type()
 * @param mode_name : name string to be searched
 * @param **mode_param_out : output pointer, point to mode parameter data
 * @param *mode_param_size: output, the mode parameter data size in bytes
 * @return ROBOEFFECT_ERROR_CODE
 */
ROBOEFFECT_ERROR_CODE roboeffect_parambin_get_mode_param_by_name(const uint8_t *sub_type_data, const char *mode_name, uint8_t **mode_param_out, uint32_t *mode_param_size);



/**
 * @brief Get mode parameter data by name string
 * 
 * @param sub_type_data : sub type data address, maybe returned by roboeffect_parambin_get_sub_type()
 * @param mode_name : name string to be searched
 * @param **codec_param_out : output pointer, point to codec parameter data
 * @param *codec_param_size: output, the codec parameter data size in bytes
 * @return ROBOEFFECT_ERROR_CODE
 */
ROBOEFFECT_ERROR_CODE roboeffect_parambin_get_codec_param_by_name(const uint8_t *sub_type_data, const char *mode_name, uint8_t **codec_param_out, uint32_t *codec_param_size);

/**
 * @brief Get preset parameter data by name string
 * 
 * @param sub_type_data : sub type data address, maybe returned by roboeffect_parambin_get_sub_type()
 * @param effect_addr : address of the effect to be searched
 * @param param_index : parameter index to be searched, start from 0x00 for the first parameter, and can be 0xff for all parameter
 * @param *preset_name : preset name string to be searched
 * @param **preset_param_out : output pointer, point to preset parameter data
 * @param *preset_param_size: output, the preset parameter data size in bytes
 * @return ROBOEFFECT_ERROR_CODE
 */
ROBOEFFECT_ERROR_CODE roboeffect_parambin_get_preset_param_by_name(const uint8_t *sub_type_data, uint8_t effect_addr, uint8_t param_index, const char *preset_name, uint8_t **preset_param_out, uint32_t *preset_param_size);

/**
 * @brief Get vector parameter data by index
 * 
 * @param sub_type_data : sub type data address, maybe returned by roboeffect_parambin_get_sub_type()
 * @param effect_addr : address of the effect to be searched
 * @param param_index : parameter index to be searched, start from 0x00 for the first parameter
 * @param vector_index : vector index to be searched
 * @param **vector_data_out : output pointer, point to vector parameter data
 * @return ROBOEFFECT_ERROR_CODE
 */
ROBOEFFECT_ERROR_CODE roboeffect_parambin_get_vector_param_by_index(const uint8_t *sub_type_data, uint8_t effect_addr, uint8_t param_index, uint16_t vector_index, const roboeffect_vector_data **vector_data_out);


/*********************************************parambin end*********************************************************/


#endif/*__ROBOEFFECT_API_H__*/