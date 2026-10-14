/**
 *************************************************************************************
 * @file	resampler_sinc.h
 * @brief	Resampler based on sinc table for good perceptual quality
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v1.1.2
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __RESAMPLER_SINC_H__
#define __RESAMPLER_SINC_H__

#include <stdint.h>


/** error code for resampler_sinc */
typedef enum _RESAMPLER_SINC_ERROR_CODE
{
	RESAMPLER_SINC_ERROR_NUMBER_OF_CHANNELS_NOT_SUPPORTED = -256,
	RESAMPLER_SINC_ERROR_UNSUPPORTED_QUALITY_VALUE,
	RESAMPLER_SINC_ERROR_ILLEGAL_SAMPLE_RATE,

	RESAMPLER_SINC_ERROR_ALLOC_FAILED,
	RESAMPLER_SINC_ERROR_OVERFLOW,

	// No Error	
	RESAMPLER_SINC_ERROR_SUCCESS = 0,					/**< no error              */
	
} RESAMPLER_SINC_ERROR_CODE;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


/**
* @brief Estimate the memory usage of resampler before actual initialization.
* @param num_channels Number of channels. Choose either 1 or 2.
* @param in_rate Input sample rate in Hz.
* @param out_rate Output sample rate in Hz.
* @param quality  Quality level. Range: 0 ~ 10. Higher value usually results in better SNR but higher computation cost, memory usage and longer delay.
*		 Usually 3~4 has good enough quality for most applications.
* @param[out] persistent_size  Size of persistent memory usage in bytes. The contents of this memory cannot be modified or cleared between frame calls.
* @return error code. RESAMPLER_SINC_ERROR_SUCCESS means successful, other codes indicate error.
* @note This function is usually called before resampler_sinc_init() to estimate the memory required for the resampler's context object.
*/
int32_t resampler_sinc_estimate_memory_usage(int32_t num_channels, int32_t in_rate, int32_t out_rate, int32_t quality, uint32_t* persistent_size);


/**
* @brief Initialize resampler module.
* @param ct Pointer to the context object.
* @param in_rate Input sample rate in Hz.
* @param out_rate Output sample rate in Hz.
* @param quality  Quality level. Range: 0 ~ 10. Higher value usually results in better SNR but higher computation cost, memory usage and longer delay.
*		 Usually 3~4 has good enough quality for most applications.
* @return error code. RESAMPLER_SINC_ERROR_SUCCESS means successful, other codes indicate error.
*/
int32_t resampler_sinc_init(uint8_t* ct, int32_t num_channels, int32_t in_rate, int32_t out_rate, int32_t quality);


/**
 * @brief Apply resampling (sample rate conversion) to a frame of 16-bit PCM data.
 * @param ct Pointer to the context object.
 * @param pcm_in Address of the PCM input buffer. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 * @param pcm_out Address of the PCM output buffer. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 *        pcm_out CANNOT be the same as pcm_in and the number of output PCM samples (num_out) may not be the same as the number of input PCM samples (num_in).
 * @param n Number of PCM samples to process. 
 * @return error code. RESAMPLER_SINC_ERROR_SUCCESS means successful, other codes indicate error.
 */
int32_t resampler_sinc_apply(uint8_t* ct, int16_t* pcm_in, int16_t* pcm_out, int32_t n);


/**
 * @brief Apply resampling (sample rate conversion) to a frame of 24-bit PCM data.
 * @param ct Pointer to the context object.
 * @param pcm_in Address of the PCM input buffer. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 * @param pcm_out Address of the PCM output buffer. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 *        pcm_out CANNOT be the same as pcm_in and the number of output PCM samples (num_out) may not be the same as the number of input PCM samples (num_in).
 * @param n Number of PCM samples to process. 
 * @return error code. RESAMPLER_SINC_ERROR_SUCCESS means successful, other codes indicate error. 
 */
int32_t resampler_sinc_apply24(uint8_t* ct, int32_t* pcm_in, int32_t* pcm_out, int32_t n);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif//__RESAMPLER_SINC_H__
