/**
 *************************************************************************************
 * @file	pingpong.h
 * @brief	Ping-Pong Delay
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v1.6.1
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __PINGPONG_H__
#define __PINGPONG_H__

#include <stdint.h>


/** error code for ping-pong effect */
typedef enum _PINGPONG_ERROR_CODE
{
	PINGPONG_ERROR_ILLEGAL_MAX_DELAY = -128,
	PINGPONG_ERROR_PCM_BIT_DEPTH_NOT_SUPPORTED,
	PINGPONG_ERROR_ILLEGAL_WETDRYMIX,
	PINGPONG_ERROR_DELAY_TOO_LARGE,
	PINGPONG_ERROR_DELAY_NOT_POSITIVE,	

	// No Error
	PINGPONG_ERROR_OK = 0,					/**< no error */
} PINGPONG_ERROR_CODE;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

/**
 * @brief Estimate the memory usage of ping-pong delay module before actual initialization.
 * @param pcm_bit_depth Bit depth of PCM data. Choose either 16 or 24.
 * @param max_delay_samples Maximum delay in samples. This number should be positive. For example if you'd like to have maximum 500ms delay at 44.1kHz sample rate, the max_delay_samples = delay time*sample rate = 500*44.1 = 22050.
 * @param high_quality High quality switch. If high_quality is set 1, the delay values are losslessly saved for high quality output, otherwise (high_quality = 0) the delay values are compressed for low memory requirement.
 * @param[out] persistent_size  Size of persistent memory usage in bytes. The contents of this memory cannot be modified or cleared between frame calls.
 * @return error code. PINGPONG_ERROR_OK means successful, other codes indicate error.
 * @note This function is usually called before pingpong_init() to estimate the memory required for the context object.
 */
int32_t pingpong_estimate_memory_usage(int32_t pcm_bit_depth, int32_t max_delay_samples, int32_t high_quality, uint32_t* persistent_size);


/**
 * @brief Initialize ping-pong delay module.
 * @param ct Pointer to the context object whose size is estimated by pingpong_estimate_memory_usage().
 * @param pcm_bit_depth Bit depth of PCM data. Choose either 16 or 24.
 * @param max_delay_samples Maximum delay in samples. For example if you want to have maximum 500ms delay at 44.1kHz sample rate, the max_delay_samples = delay time*sample rate = 500*44.1 = 22050.
 * @param high_quality High quality switch. If high_quality is set 1, the delay values are losslessly saved for high quality output, otherwise (high_quality = 0) the delay values are compressed for low memory requirement.
 * @return error code. PINGPONG_ERROR_OK means successful, other codes indicate error.
 */
int32_t pingpong_init(uint8_t* ct, int32_t pcm_bit_depth, int32_t max_delay_samples, int32_t high_quality);


/**
 * @brief Apply ping-pong delay to a frame of 16-bit PCM data (must be stereo, i.e. 2 channels)
 * @param ct Pointer to the context object whose size is estimated by pingpong_estimate_memory_usage().
 * @param pcm_in Address of the PCM input. The data layout for stereo: L0,R0,L1,R1,L2,R2,...
 * @param pcm_out Address of the PCM output. The data layout for stereo: L0,R0,L1,R1,L2,R2,...
 *        pcm_out can be the same as pcm_in. In this case, the PCM signals are changed in-place.
 * @param n Number of PCM samples to process.
 * @param attenuation attenuation coefficient. Q1.15 format to represent value in range from 0 to 1. For example, 8192 represents 0.25 as the attenuation coefficient.
 * @param delay_samples Delay in samples. Range: 1 ~ max_delay_samples.
 * @param wetdrymix The ratio of wet (ping-pong delay) signal to the mixed output (wet+dry). Range: 0~100 for 0~100%.
 * @return error code. PINGPONG_ERROR_OK means successful, other codes indicate error.
 * @note Only stereo (2 channels) PCM signals are supported for processing.
 */
int32_t pingpong_apply16(uint8_t*ct, int16_t *pcm_in, int16_t *pcm_out, int32_t n, int16_t attenuation, int32_t delay_samples, int32_t wetdrymix);


/**
 * @brief Apply ping-pong delay to a frame of 24-bit PCM data (must be stereo, i.e. 2 channels)
 * @param ct Pointer to the context object whose size is estimated by pingpong_estimate_memory_usage().
 * @param pcm_in Address of the PCM input. The data layout for stereo: L0,R0,L1,R1,L2,R2,...
 * @param pcm_out Address of the PCM output. The data layout for stereo: L0,R0,L1,R1,L2,R2,...
 *        pcm_out can be the same as pcm_in. In this case, the PCM signals are changed in-place.
 * @param n Number of PCM samples to process.
 * @param attenuation attenuation coefficient. Q1.15 format to represent value in range from 0 to 1. For example, 8192 represents 0.25 as the attenuation coefficient.
 * @param delay_samples Delay in samples. Range: 1 ~ max_delay_samples.
 * @param wetdrymix The ratio of wet (ping-pong delay) signal to the mixed output (wet+dry). Range: 0~100 for 0~100%.
 * @return error code. PINGPONG_ERROR_OK means successful, other codes indicate error.
 * @note Only stereo (2 channels) PCM signals are supported for processing.
 */
int32_t pingpong_apply24(uint8_t*ct, int32_t *pcm_in, int32_t *pcm_out, int32_t n, int16_t attenuation, int32_t delay_samples, int32_t wetdrymix);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif//__PINGPONG_H__
