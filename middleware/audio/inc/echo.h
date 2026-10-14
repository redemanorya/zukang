/**
 *************************************************************************************
 * @file	echo.h
 * @brief	Echo effect
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v2.3.1
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __ECHO_H__
#define __ECHO_H__

#include <stdint.h>

/** error code for echo effect */
typedef enum _ECHO_ERROR_CODE
{
    ECHO_ERROR_CHANNEL_NUMBER_NOT_SUPPORTED = -256,
	ECHO_ERROR_SAMPLE_RATE_NOT_SUPPORTED,
	ECHO_ERROR_PCM_BIT_DEPTH_NOT_SUPPORTED,
	ECHO_ERROR_ILLEGAL_MAX_DELAY,
	ECHO_ERROR_DELAY_TOO_LARGE,	
	ECHO_ERROR_DELAY_NOT_POSITIVE,
	ECHO_ERROR_ILLEGAL_DRY,
	ECHO_ERROR_ILLEGAL_WET,

	// No Error
	ECHO_ERROR_OK = 0,					/**< no error */
} ECHO_ERROR_CODE;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


/**
 * @brief Estimate the memory usage of echo effect module before actual initialization.
 * @param pcm_bit_depth  Bit depth of PCM data. Choose either 16 or 24.
 * @param max_delay_samples Maximum delay in samples. This number should be positive. For example if you'd like to have maximum 500ms delay at 44.1kHz sample rate, the max_delay_samples = delay time*sample rate = 500*44.1 = 22050.
 * @param high_quality High quality switch. If high_quality is set 1, the delay values are losslessly saved for high quality output, otherwise (high_quality = 0) the delay values are compressed for low memory requirement.
 * @param[out] persistent_size  Size of persistent memory usage in bytes. The contents of this memory cannot be modified or cleared between frame calls.
 * @return error code. ECHO_ERROR_OK means successful, other codes indicate error.
 * @note This function is usually called before echo_init() to estimate the memory required for the context object.
 */
int32_t echo_estimate_memory_usage(int32_t pcm_bit_depth, int32_t max_delay_samples, int32_t high_quality, uint32_t* persistent_size);


/**
 * @brief Initialize echo effect module.
 * @param ct Pointer to the context object whose size is estimated by echo_estimate_memory_usage().
 * @param num_channels Number of channels. Choose either 1 or 2.
 * @param sample_rate Sample rate.
 * @param pcm_bit_depth  Bit depth of PCM data. Choose either 16 or 24.
 * @param fc Cutoff frequency of the low-pass filter in Hz. Set 0 to disable the use of the low-pass filter in echo effect. Note that this value should not exceed half of the sample rate, i.e. Nyquist frequency.
 * @param max_delay_samples Maximum delay in samples. This number should be positive. For example if you'd like to have maximum 500ms delay at 44.1kHz sample rate, the max_delay_samples = delay time*sample rate = 500*44.1 = 22050.
 * @param high_quality High quality switch. If high_quality is set 1, the delay values are losslessly saved for high quality output, otherwise (high_quality = 0) the delay values are compressed for low memory requirement.
 * @return error code. ECHO_ERROR_OK means successful, other codes indicate error.
 */
int32_t echo_init(uint8_t* ct, int32_t num_channels, int32_t sample_rate, int32_t pcm_bit_depth, int32_t fc, int32_t max_delay_samples, int32_t high_quality);


/**
 * @brief Apply echo effect to a frame of 16-bit PCM data.
 * @param ct Pointer to the context object whose size is estimated by echo_estimate_memory_usage().
 * @param pcm_in Address of the PCM input. The data layout for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 * @param pcm_out Address of the PCM output. The data layout for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 *        pcm_out can be the same as pcm_in. In this case, the PCM signals are changed in-place.
 * @param n Number of PCM samples to process.
 * @param attenuation attenuation coefficient. Q1.15 format to represent value in range from 0 to 1. For example, 8192 represents 0.25 as the attenuation coefficient.
 * @param delay_samples Delay in samples. Range: 1 ~ max_delay_samples.
 * @param dry The level of dry(direct) signals in the output. Range: 0% ~ 100%.
 * @param wet The level of wet(effect) signals in the output. Range: 0% ~ 100%.
 * @return error code. ECHO_ERROR_OK means successful, other codes indicate error.
 */
int32_t echo_apply16(uint8_t* ct, int16_t *pcm_in, int16_t *pcm_out, int32_t n, int16_t attenuation, int32_t delay_samples, int32_t dry, int32_t wet);


/**
 * @brief Apply echo effect to a frame of 24-bit PCM data.
 * @param ct Pointer to the context object whose size is estimated by echo_estimate_memory_usage().
 * @param pcm_in Address of the PCM input. The data layout for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 * @param pcm_out Address of the PCM output. The data layout for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 *        pcm_out can be the same as pcm_in. In this case, the PCM signals are changed in-place.
 * @param n Number of PCM samples to process.
 * @param attenuation attenuation coefficient. Q1.15 format to represent value in range from 0 to 1. For example, 8192 represents 0.25 as the attenuation coefficient.
 * @param delay_samples Delay in samples. Range: 1 ~ max_delay_samples.
 * @param dry The level of dry(direct) signals in the output. Range: 0% ~ 100%.
 * @param wet The level of wet(effect) signals in the output. Range: 0% ~ 100%.
 * @return error code. ECHO_ERROR_OK means successful, other codes indicate error.
 */
int32_t echo_apply24(uint8_t* ct, int32_t *pcm_in, int32_t *pcm_out, int32_t n, int16_t attenuation, int32_t delay_samples, int32_t dry, int32_t wet);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif//__ECHO_H__
