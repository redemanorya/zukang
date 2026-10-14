/**
 *************************************************************************************
 * @file sine_gen.h
 * @brief
 *
 * @author castle 
 * @version v0.1.0
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __SINE_GEN_H__
#define __SINE_GEN_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#include <stdint.h>
#include "type.h"

#define PCM_16_MAX 0x7FFF
#define PCM_16_HALF 0x3FFF
#define PCM_24_MAX 0x7FFFFF
#define PCM_24_HALF 0x3FFFFF
#define PCM_32_MAX 0x7FFFFFFF
#define PCM_32_HALF 0x3FFFFFFF

/** error code for sine generator */
typedef enum _SINE_GEN_ERROR_CODE
{
	//has error
	SINE_GEN_CHANNEL_ERROR = -256,
	SINE_GEN_FRQ_ERROR,
	SINE_GEN_AMPL_ERROR,
	SINE_GEN_INNER_ERROR,
	SINE_GEN_CHANNEL_NUM_ERROR,

    //No Error
	SINE_GEN_ERROR_OK = 0,

} SINE_GEN_ERROR_CODE;

/** status for mute/un-mute */
typedef enum _SineGenMuteStatus
{
  SINE_GEN_STATUS_NONE = 0,
	SINE_GEN_STATUS_FADE_IN,
	SINE_GEN_STATUS_FADE_OUT
} SineGenMuteStatus;

/** sine generator context struct */
typedef struct _SineGenContext
{
	int32_t sample_rate;
	int32_t channel_enable;
	int32_t num_channels;
	int32_t left_freq;
	int32_t right_freq;

	int16_t left_phase;
	int16_t right_phase;

	int16_t left_step;
	int16_t right_step;

	int32_t left_scale;
	int32_t right_scale;

	SineGenMuteStatus left_fio;
	SineGenMuteStatus right_fio;

} SineGenContext;

/** sine generator context struct */
typedef struct _Sine32GenContext
{
	int32_t sample_rate;
	int32_t channel_enable;
	int32_t num_channels;
	int32_t left_freq;
	int32_t right_freq;

	int32_t left_phase;
	int32_t right_phase;

	int32_t left_step;
	int32_t right_step;

	int64_t left_scale;
	int64_t right_scale;

	SineGenMuteStatus left_fio;
	SineGenMuteStatus right_fio;

} Sine32GenContext;

/**
 * @brief sine generator init func
 * 
 * @param sgc sine generator pointer to context
 * @param sample_rate sample rate.
 * @param channel_enable 1 for only left enable, 2 for only right enable, 3 for both enable 
 * @param left_freq left channel frequency, 1 ~ sample_rate/2 (Hz)
 * @param right_freq right channel frequency, 1 ~ sample_rate/2 (Hz)
 * @param left_ampl left channel amplitude, -960~0 as -96.0dB ~ 0dB
 * @param right_ampl right channedl amplitud, -960~0 as -96.0dB ~ 0dB
 * @return SINE_GEN_ERROR_CODE 
 */
SINE_GEN_ERROR_CODE sine_generator_init(SineGenContext *sgc, int32_t sample_rate, int16_t channel_enable, int16_t num_channels, int32_t left_freq, int32_t right_freq, int32_t left_ampl, int32_t right_ampl);

/**
 * @brief sine geenerator apply func
 * 
 * @param sgc sine generator pointer to context
 * @param pcm_in Address of the PCM input buffer. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 * @param pcm_out Address of the PCM output buffer. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...". pcm_out can be the same as pcm_in. In this case, the PCM signals are changed in-place.
 * @param pcm_len Number of PCM samples to process.
 * @return SINE_GEN_ERROR_CODE 
 */
SINE_GEN_ERROR_CODE sine_generator_apply(SineGenContext *sgc, int16_t *pcm_in, int16_t *pcm_out, uint16_t pcm_len);

/**
 * @brief 
 * 
 * @param sgc sine generator pointer to context
 * @param channel_enable 
 * @return SINE_GEN_ERROR_CODE 
 */
SINE_GEN_ERROR_CODE sine_generator_config_channel(SineGenContext *sgc, int32_t channel_enable);
SINE_GEN_ERROR_CODE sine24_generator_config_channel(Sine32GenContext *sgc, int32_t channel_enable);

/**
 * @brief 
 * 
 * @param sgc 
 * @param left_freq left channel frequency, 1 ~ sample_rate/2
 * @param right_freq right channel frequency, 1 ~ sample_rate/2
 * @return SINE_GEN_ERROR_CODE 
 */
SINE_GEN_ERROR_CODE sine_generator_config_freq(SineGenContext *sgc, int32_t left_freq, int32_t right_freq);

/**
 * @brief 
 * 
 * @param sgc sine generator pointer to context
 * @param left_ampl left channel amplitude, -960~0 as -96.0dB ~ 0dB
 * @param right_ampl right channedl amplitud, -960~0 as -96.0dB ~ 0dB
 * @return SINE_GEN_ERROR_CODE 
 */
SINE_GEN_ERROR_CODE sine_generator_config_ampl(SineGenContext *sgc, int32_t left_ampl, int32_t right_ampl);

/**
 * @brief sine 32bit pcm generator init func
 * 
 * @param sgc sine generator pointer to context
 * @param sample_rate sample rate.
 * @param channel_enable 1 for only left enable, 2 for only right enable, 3 for both enable 
 * @param left_freq left channel frequency, 1 ~ sample_rate/2 (Hz)
 * @param right_freq right channel frequency, 1 ~ sample_rate/2 (Hz)
 * @param left_ampl left channel amplitude, -960~0 as -96.0dB ~ 0dB
 * @param right_ampl right channedl amplitud, -960~0 as -96.0dB ~ 0dB
 * @return SINE_GEN_ERROR_CODE 
 */
SINE_GEN_ERROR_CODE sine32_generator_init(Sine32GenContext *sgc, int32_t sample_rate, int16_t channel_enable, int16_t num_channels, int32_t left_freq, int32_t right_freq, int32_t left_ampl, int32_t right_ampl);

/**
 * @brief sine geenerator apply func
 * 
 * @param sgc sine generator pointer to context
 * @param pcm_in Address of the PCM input buffer. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 * @param pcm_out Address of the PCM output buffer. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...". pcm_out can be the same as pcm_in. In this case, the PCM signals are changed in-place.
 * @param pcm_len Number of PCM samples to process.
 * @return SINE_GEN_ERROR_CODE 
 */
SINE_GEN_ERROR_CODE sine32_generator_apply(Sine32GenContext *sgc, int32_t *pcm_in, int32_t *pcm_out, uint16_t pcm_len);

/**
 * @brief 
 * 
 * @param sgc sine generator pointer to context
 * @param channel_enable 
 * @return SINE_GEN_ERROR_CODE 
 */
SINE_GEN_ERROR_CODE sine32_generator_config_channel(Sine32GenContext *sgc, int32_t channel_enable);

/**
 * @brief 
 * 
 * @param sgc 
 * @param left_freq left channel frequency, 1 ~ sample_rate/2
 * @param right_freq right channel frequency, 1 ~ sample_rate/2
 * @return SINE_GEN_ERROR_CODE 
 */
SINE_GEN_ERROR_CODE sine32_generator_config_freq(Sine32GenContext *sgc, int32_t left_freq, int32_t right_freq);

/**
 * @brief 
 * 
 * @param sgc sine generator pointer to context
 * @param left_ampl left channel amplitude, -960~0 as -96.0dB ~ 0dB
 * @param right_ampl right channedl amplitud, -960~0 as -96.0dB ~ 0dB
 * @return SINE_GEN_ERROR_CODE 
 */
SINE_GEN_ERROR_CODE sine32_generator_config_ampl(Sine32GenContext *sgc, int32_t left_ampl, int32_t right_ampl);

/**
 * @brief sine 24bit pcm generator init func
 *
 * @param sgc sine generator pointer to context
 * @param sample_rate sample rate.
 * @param channel_enable 1 for only left enable, 2 for only right enable, 3 for both enable
 * @param left_freq left channel frequency, 1 ~ sample_rate/2 (Hz)
 * @param right_freq right channel frequency, 1 ~ sample_rate/2 (Hz)
 * @param left_ampl left channel amplitude, -960~0 as -96.0dB ~ 0dB
 * @param right_ampl right channedl amplitud, -960~0 as -96.0dB ~ 0dB
 * @return SINE_GEN_ERROR_CODE
 */
SINE_GEN_ERROR_CODE sine24_generator_init(Sine32GenContext *sgc, int32_t sample_rate, int16_t channel_enable, int16_t num_channels, int32_t left_freq, int32_t right_freq, int32_t left_ampl, int32_t right_ampl);

/**
 * @brief sine geenerator apply func
 *
 * @param sgc sine generator pointer to context
 * @param pcm_in Address of the PCM input buffer. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 * @param pcm_out Address of the PCM output buffer. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...". pcm_out can be the same as pcm_in. In this case, the PCM signals are changed in-place.
 * @param pcm_len Number of PCM samples to process.
 * @return SINE_GEN_ERROR_CODE
 */
SINE_GEN_ERROR_CODE sine24_generator_apply(Sine32GenContext *sgc, int32_t *pcm_in, int32_t *pcm_out, uint16_t pcm_len);

/**
 * @brief
 *
 * @param sgc sine generator pointer to context
 * @param channel_enable
 * @return SINE_GEN_ERROR_CODE
 */
SINE_GEN_ERROR_CODE sine24_generator_config_channel(Sine32GenContext *sgc, int32_t channel_enable);

/**
 * @brief
 *
 * @param sgc
 * @param left_freq left channel frequency, 1 ~ sample_rate/2
 * @param right_freq right channel frequency, 1 ~ sample_rate/2
 * @return SINE_GEN_ERROR_CODE
 */
SINE_GEN_ERROR_CODE sine24_generator_config_freq(Sine32GenContext *sgc, int32_t left_freq, int32_t right_freq);

/**
 * @brief
 *
 * @param sgc sine generator pointer to context
 * @param left_ampl left channel amplitude, -960~0 as -96.0dB ~ 0dB
 * @param right_ampl right channedl amplitud, -960~0 as -96.0dB ~ 0dB
 * @return SINE_GEN_ERROR_CODE
 */
SINE_GEN_ERROR_CODE sine24_generator_config_ampl(Sine32GenContext *sgc, int32_t left_ampl, int32_t right_ampl);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__SINE_GEN_H__
