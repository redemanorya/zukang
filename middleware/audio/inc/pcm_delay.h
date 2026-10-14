/**
 *************************************************************************************
 * @file	pcm_delay.h
 * @brief	Delay of PCM samples
 *
 * @author	ZHAO Ying (Alfred)
 * @version	V2.3.1
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 *
 *************************************************************************************
 */

#ifndef _PCM_DELAY_H
#define _PCM_DELAY_H

#include <stdint.h>


/** error code for PCM delay */
typedef enum _PCMDELAY_ERROR_CODE
{
    PCMDELAY_ERROR_NUMBER_OF_CHANNELS_NOT_SUPPORTED = -256,
	PCMDELAY_ERROR_PCM_BIT_DEPTH_NOT_SUPPORTED,
	PCMDELAY_ERROR_ILLEGAL_MAX_DELAY,
    PCMDELAY_ERROR_INVALID_DELAY_SAMPLES,

    // No Error
    PCMDELAY_ERROR_OK = 0,					/**< no error              */
} PCMDELAY_ERROR_CODE;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


/**
 * @brief Estimate the memory usage of PCM delay unit before actual initialization.
 * @param pcm_bit_depth Bit depth of PCM data. Choose either 16 or 24.
 * @param num_channels Number of channels. Both 1 channel and 2 channels are supported. 
 * @param max_delay_samples Maximum delay in samples. This number should be positive. For example if you'd like to have maximum 500ms delay at 44.1kHz sample rate, the max_delay_samples = delay time*sample rate = 500*44.1 = 22050.
 * @param high_quality High quality switch. If high_quality is set 1, the delay values are losslessly saved for high quality output, otherwise (high_quality = 0) the delay values are compressed for low memory requirement.
 * @param[out] persistent_size  Size of persistent memory usage in bytes. The contents of this memory cannot be modified or cleared between frame calls.
 * @return error code. PCMDELAY_ERROR_OK means successful, other codes indicate error.
 * @note This function is usually called before pcm_delay_init() to estimate the memory required for the context object.
 */
int32_t pcm_delay_estimate_memory_usage(int32_t pcm_bit_depth, int32_t num_channels, int32_t max_delay_samples, int32_t high_quality, uint32_t* persistent_size);


/**
 * @brief Initialization of PCM delay unit.
 * @param ct Pointer to the context object whose size is estimated by pcm_delay_estimate_memory_usage().
 * @param pcm_bit_depth Bit depth of PCM data. Choose either 16 or 24.
 * @param num_channels Number of channels. Choose either 1 or 2.
 * @param max_delay_samples Maximum delay in samples. For example if you want to have maximum 500ms delay at 44.1kHz sample rate, the max_delay_samples = delay time*sample rate = 500*44.1 = 22050.
 * @param high_quality High quality switch. If high_quality is set 1, the delay values are losslessly saved for high quality output, otherwise (high_quality = 0) the delay values are compressed for low memory requirement.
 * @return error code. PCMDELAY_ERROR_OK means successful, other codes indicate error.
 */
int32_t pcm_delay_init(uint8_t* ct, int32_t pcm_bit_depth, int32_t num_channels, int32_t max_delay_samples, int32_t high_quality);


/**
 * @brief Apply PCM delay to a frame of 16-bit PCM data.
 * @param ct Pointer to the context object whose size is estimated by pcm_delay_estimate_memory_usage().
 * @param pcm_in Address of the PCM input. The data layout for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 * @param pcm_out Address of the PCM output. The data layout for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 *        pcm_out can be the same as pcm_in. In this case, the PCM signals are changed in-place.
 * @param n Number of PCM samples to process.
 * @param delay_samples Delay in samples. e.g. 40ms delay @ 44.1kHz = 40*44.1 = 1764 samples. This value should not be greater than max_delay_samples or less than 0.
 * @return error code. PCMDELAY_ERROR_OK means successful, other codes indicate error.
 */
int32_t pcm_delay_apply16(uint8_t* ct, int16_t *pcm_in, int16_t *pcm_out, int32_t n, int32_t delay_samples);


/**
 * @brief Apply PCM delay to a frame of 24-bit PCM data.
 * @param ct Pointer to the context object whose size is estimated by pcm_delay_estimate_memory_usage().
 * @param pcm_in Address of the PCM input. The data layout for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 * @param pcm_out Address of the PCM output. The data layout for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 *        pcm_out can be the same as pcm_in. In this case, the PCM signals are changed in-place.
 * @param n Number of PCM samples to process.
 * @param delay_samples Delay in samples. e.g. 40ms delay @ 44.1kHz = 40*44.1 = 1764 samples. This value should not be greater than max_delay_samples or less than 0.
 * @return error code. PCMDELAY_ERROR_OK means successful, other codes indicate error.
 */
int32_t pcm_delay_apply24(uint8_t* ct, int32_t *pcm_in, int32_t *pcm_out, int32_t n, int32_t delay_samples);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif // _PCM_DELAY_H
