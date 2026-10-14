/**
 *************************************************************************************
 * @file	filter_fir.h
 * @brief	Finite Impulse Response (FIR) filter
 *
 * @author	ZHAO Ying (Alfred)
 * @version	V1.3.1
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *
 *************************************************************************************
 */

#ifndef __FILTER_FIR_H__
#define __FILTER_FIR_H__

#include <stdint.h>


/** error code for FIR filter */
typedef enum _FILTERFIR_ERROR_CODE
{
	FILTERFIR_ERROR_NUMBER_OF_CHANNELS_NOT_SUPPORTED = -256,
	FILTERFIR_ERROR_ILLEGAL_FILTER_ORDER,
	FILTERFIR_ERROR_PCM_BIT_DEPTH_NOT_SUPPORTED,
	FILTERFIR_ERROR_NUMBER_OF_SAMPLES_OUT_OF_RANGE,

	FILTERFIR_ERROR_VECTOR_NOT_FOUND,//roboeffect added

	// No Error
	FILTERFIR_ERROR_OK = 0,					/**< no error              */
} FILTERFIR_ERROR_CODE;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


/**
 * @brief Estimate the memory usage of FIR filter before actual initialization.
 * @param num_channels Number of channels. Choose either 1 or 2.
 * @param pcm_bit_depth  Bit depth of PCM data. Choose either 16 or 24.
 * @param filter_order     Order of FIR filter. Order must be a positive integer.
 * @param[out] persistent_size  Size of persistent memory usage in bytes. The contents of this memory cannot be modified or cleared between frame calls.
 * @param[out] scratch_size  Size of scratch memory in bytes. The contents of this memory can be modified or cleared for other use between frame calls.
 * @return error code. FILTERFIR_ERROR_OK means successful, other codes indicate error.
 * @note This function is usually called before filter_fir_init() to estimate the memory required for FIR filter context object.
 * The scratch memory is only used for stereo case. If the number of channels is 1, scratch_size will be 0.
 */
int32_t filter_fir_estimate_memory_usage(int32_t num_channels, int32_t pcm_bit_depth, int32_t filter_order, uint32_t* persistent_size, uint32_t* scratch_size);


/**
 * @brief Initialize FIR filter module
 * @param ct Pointer to an FIR filter context object.
 * @param scratch Pointer to the scratch area (scratch_size in bytes).
 * @param num_channels Number of channels. Choose either 1 or 2.
 * @param pcm_bit_depth Bit depth of PCM data. Choose either 16 or 24.
 * @param filter_order Order of FIR filter. Order must be a positive integer.
 * @param coefficients Pointer to the filter coefficients. Each coefficient is in single precision floating point format (4 bytes).
 * 					   The size of the entire coefficients array is (filter_order + 1)*4 bytes.
 * @return error code. FILTERFIR_ERROR_OK means successful, other codes indicate error.
 */
int32_t filter_fir_init(uint8_t* ct, uint8_t* scratch, int32_t num_channels, int32_t pcm_bit_depth, int32_t filter_order, const float* coefficients);


/**
 * @brief Apply FIR filter to a frame of PCM data (16-bit).
 * @param ct Pointer to an FIR filter context object.
 * @param pcm_in Address of the PCM input. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 * @param pcm_out Address of the PCM output. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 *        pcm_out can be the same as pcm_in. In this case, the PCM is changed in-place.
 * @param n Number of PCM samples to process. Range: 1 ~ 128.
 * @return error code. FILTERFIR_ERROR_OK means successful, other codes indicate error.
 */
int32_t filter_fir_apply16(uint8_t* ct, int16_t* pcm_in, int16_t* pcm_out, int32_t n);


/**
 * @brief Apply FIR filter to a frame of PCM data (24-bit).
 * @param ct Pointer to an FIR filter context object.
 * @param pcm_in Address of the PCM input. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 * @param pcm_out Address of the PCM output. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 *        pcm_out can be the same as pcm_in. In this case, the PCM is changed in-place.
 * @param n Number of PCM samples to process. Range: 1 ~ 128.
 * @return error code. FILTERFIR_ERROR_OK means successful, other codes indicate error.
 */
int32_t filter_fir_apply24(uint8_t* ct, int32_t* pcm_in, int32_t* pcm_out, int32_t n);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif // __FILTER_FIR_H__
