/**
 *************************************************************************************
 * @file	resampler_farrow.h
 * @brief	Resampler based on farrow structure
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v1.2.0
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __RESAMPLER_FARROW_H__
#define __RESAMPLER_FARROW_H__

#include <stdint.h>

#define MAX_POLYNOMIAL_ORDER 4

/** error code for resampler */
typedef enum _RESAMPLER_FARROW_ERROR_CODE
{
	RESAMPLER_FARROW_ERROR_NUMBER_OF_CHANNELS_NOT_SUPPORTED = -256,
	RESAMPLER_FARROW_ERROR_UNSUPPORTED_POLYNOMIAL_ORDER,
	RESAMPLER_FARROW_ERROR_ILLEGAL_NUMBER_OF_IN_OUT_SAMPLES,
	RESAMPLER_FARROW_ERROR_ILLEGAL_SAMPLE_NUMBER_CHANGE,
	RESAMPLER_FARROW_ERROR_HALFWAY_CHANGE_NUM_FRAMES_NOT_ALLOWED,

	// No Error
	RESAMPLER_FARROW_ERROR_OK = 0,					/**< no error              */
} RESAMPLER_FARROW_ERROR_CODE;

/** Resampler Farrow Context */
typedef struct _ResamplerFarrowContext
{
	int32_t num_channels;
	int32_t polynomial_order;
	int32_t nframes;
	int32_t step;
	int32_t pha;
	int32_t kc;
	int32_t sample_prev[2];
	int32_t xbuf[2][MAX_POLYNOMIAL_ORDER + 1]; // fit for 16-bit/24-bit

} ResamplerFarrowContext;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

/**
 * @brief Initialize resampler module.
 * @param ct Pointer to a ResamplerFarrowContext object.
 * @param num_channels number of channels. Both 1 and 2 channels are supported.
 * @param polynomial_order Polynomial order used in interpolation. Range: 1 ~ 4. Higher order usually results in better SNR but higher computation cost.
 * @return error code. RESAMPLER_FARROW_ERROR_OK means successful, other codes indicate error.
 */
int32_t resampler_farrow_init(ResamplerFarrowContext *ct, int32_t num_channels, int32_t polynomial_order);


/**
 * @brief Apply resampling (sample rate conversion) to a frame of PCM data (fixed-point implementation).
 * @param ct Pointer to a ResamplerFarrowContext object.
 * @param pcm_in Address of the PCM input buffer. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 * @param pcm_out Address of the PCM output buffer. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 *        pcm_out CANNOT be the same as pcm_in and the number of output PCM samples (num_out) may not be the same as the number of input PCM samples (num_in).
 * @param num_in Number of input PCM samples per channel. Any positive value is legal.
 * @param num_out Number of output PCM samples per channel. Any positive value is legal.
 * @return error code. RESAMPLER_FARROW_ERROR_OK means successful, other codes indicate error.
 * @note Although there is no limit for num_in & num_out, the design of this resampling algorithm is for a very small modification of 
 * the sampling factor and it concerns a fine-tuning factor close to unity (e.g: R = 129/128). Large sampling rate conversion factor such 
 * as a large integer or simple rational (e.g: R = 1/4, R = 128/64) may result in severe alias effects.
 */
int32_t resampler_farrow_apply(ResamplerFarrowContext *ct, int16_t *pcm_in, int16_t *pcm_out, int32_t num_in, int32_t num_out);


/**
 * @brief Apply resampling (sample rate conversion) to a frame of PCM data (fixed-point implementation, 24-bit).
 * @param ct Pointer to a ResamplerFarrowContext object.
 * @param pcm_in Address of the PCM input buffer. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 * @param pcm_out Address of the PCM output buffer. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 *        pcm_out CANNOT be the same as pcm_in and the number of output PCM samples (num_out) may not be the same as the number of input PCM samples (num_in).
 * @param num_in Number of input PCM samples per channel. Any positive value is legal.
 * @param num_out Number of output PCM samples per channel. Any positive value is legal.
 * @return error code. RESAMPLER_FARROW_ERROR_OK means successful, other codes indicate error.
 * @note Although there is no limit for num_in & num_out, the design of this resampling algorithm is for a very small modification of
 * the sampling factor and it concerns a fine-tuning factor close to unity (e.g: R = 129/128). Large sampling rate conversion factor such
 * as a large integer or simple rational (e.g: R = 1/4, R = 128/64) may result in severe alias effects.
 */
int32_t resampler_farrow_apply24(ResamplerFarrowContext *ct, int32_t *pcm_in, int32_t *pcm_out, int32_t num_in, int32_t num_out);


/**
 * @brief Apply resampling (sample rate conversion) to a frame of PCM data (floating-point implementation).
 * @param ct Pointer to a ResamplerFarrowContext object.
 * @param pcm_in Address of the PCM input buffer. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 * @param pcm_out Address of the PCM output buffer. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 *        pcm_out CANNOT be the same as pcm_in and the number of output PCM samples (num_out) may not be the same as the number of input PCM samples (num_in).
 * @param num_in Number of input PCM samples per channel. Any positive value is legal.
 * @param num_out Number of output PCM samples per channel. Any positive value is legal.
 * @return error code. RESAMPLER_FARROW_ERROR_OK means successful, other codes indicate error.
 * @note Although there is no limit for num_in & num_out, the design of this resampling algorithm is for a very small modification of
 * the sampling factor and it concerns a fine-tuning factor close to unity (e.g: R = 129/128). Large sampling rate conversion factor such
 * as a large integer or simple rational (e.g: R = 1/4, R = 128/64) may result in severe alias effects.
 */
int32_t resampler_farrow_apply_float(ResamplerFarrowContext *ct, int16_t *pcm_in, int16_t *pcm_out, int32_t num_in, int32_t num_out);


/**
 * @brief Apply resampling (sample rate conversion) to a frame of PCM data in a group (fixed-point implementation, 16-bit).
 * @param ct Pointer to a ResamplerFarrowContext object.
 * @param pcm_in Address of the PCM input buffer. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 * @param pcm_out Address of the PCM output buffer. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 *        pcm_out CANNOT be the same as pcm_in and the number of output PCM samples (nout_actual) may not be the same as the number of input PCM samples (nin).
 * @param num_frames Number of frames as a group to realize 1 more or 1 less or the same output samples.
 * @param inc_dec Number of samples to increase or decrease in the group. 0: same; 1: 1 more sample; -1: 1 less sample. No other values are supported.
 * @param nin Number of input PCM samples per channel in frame. Any positive value is legal. This value should not change for each frame in the same group.
 * @param[out] nout_actual Actual number of output PCM samples per channel. This number is the same as nin until the last frame in the group.
 * @return error code. RESAMPLER_FARROW_ERROR_OK means successful, other codes indicate error.
 * @note This function realizes the change of PCM sample number over multiple frames instead of in just 1 frame to reduce the side effect.
 */
int32_t resampler_farrow_over_frames_apply(ResamplerFarrowContext* ct, int16_t* pcm_in, int16_t* pcm_out, int32_t num_frames, int32_t inc_dec, int32_t nin, int32_t* nout_actual);


/**
 * @brief Apply resampling (sample rate conversion) to a frame of PCM data in a group (fixed-point implementation, 24-bit).
 * @param ct Pointer to a ResamplerFarrowContext object.
 * @param pcm_in Address of the PCM input buffer. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 * @param pcm_out Address of the PCM output buffer. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 *        pcm_out CANNOT be the same as pcm_in and the number of output PCM samples (nout_actual) may not be the same as the number of input PCM samples (nin).
 * @param num_frames Number of frames as a group to realize 1 more or 1 less or the same output samples.
 * @param inc_dec Number of samples to increase or decrease in the group. 0: same; 1: 1 more sample; -1: 1 less sample. No other values are supported.
 * @param nin Number of input PCM samples per channel in frame. Any positive value is legal. This value should not change for each frame in the same group.
 * @param[out] nout_actual Actual number of output PCM samples per channel. This number is the same as nin until the last frame in the group.
 * @return error code. RESAMPLER_FARROW_ERROR_OK means successful, other codes indicate error.
 * @note This function realizes the change of PCM sample number over multiple frames instead of in just 1 frame to reduce the side effect.
 */
int32_t resampler_farrow_over_frames_apply24(ResamplerFarrowContext* ct, int32_t* pcm_in, int32_t* pcm_out, int32_t num_frames, int32_t inc_dec, int32_t nin, int32_t* nout_actual);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif//__RESAMPLER_FARROW_H__
