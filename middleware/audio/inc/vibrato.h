/**
*************************************************************************************
* @file	    vibrato.h
* @brief	Vibrato effect for mono signals
*
* @author	ZHAO Ying (Alfred)
* @version	v1.0.0
*
* &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
*************************************************************************************
*/

#ifndef __VIBRATO_H__
#define __VIBRATO_H__

#include <stdint.h>

#define MAX_VIBRATO_DELAY_SAMPLES (10*2*48+2) // maximum delay in samples 10ms*2@48kHz

/** error code for vibrato */
typedef enum _VIBRATO_ERROR_CODE
{
	VIBRATO_ERROR_SAMPLE_RATE_NOT_SUPPORTED = -256,	
	VIBRATO_ERROR_ILLEGAL_DELAY_LENGTH,
	VIBRATO_ERROR_ILLEGAL_MODULATION_DEPTH,
	VIBRATO_ERROR_ILLEGAL_MODULATION_RATE,

	// No Error
	VIBRATO_ERROR_OK = 0,					/**< no error              */
} VIBRATO_ERROR_CODE;

/** vibrato context */
typedef struct _VibratoContext
{
	int32_t sample_rate;
	//int32_t num_channels;
	int32_t dp;
	int32_t mod_count;
	int32_t mod_depth_samples;	
	int32_t mod_rate;
	int32_t mod_rate_step;	
	int16_t delay[MAX_VIBRATO_DELAY_SAMPLES];
} VibratoContext;

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

/**
* @brief Initialize the vibrato module.
* @param ct Pointer to a VibratoContext object.
* @param sample_rate Sample rate.
* @param mod_depth Modulation depth in microsecond. Range: 20 ~ 10000us (=10ms)
* @param mod_rate Modulation rate in 0.01Hz. Range: 0.00 ~ 14.00Hz in step of 0.01Hz. For example, 2 for 0.02Hz, 30 for 0.30Hz, 1000 for 10.00Hz.
* @return error code. VIBRATO_ERROR_OK means successful, other codes indicate error.
*/
int32_t vibrato_init(VibratoContext*ct, int32_t sample_rate, int32_t mod_depth, int32_t mod_rate);


/**
* @brief Apply vibrato effect to a frame of PCM data (mono only)
* @param ct Pointer to a VibratoContext object.
* @param pcm_in PCM input buffer.
* @param pcm_out PCM output buffer.
*        pcm_out can be the same as pcm_in. In this case, the PCM data is changed in-place.
* @param n Number of PCM samples to process.
* @param mod_rate Modulation rate in 0.01Hz. Range: 0.00 ~ 14.00Hz in step of 0.01Hz. For example, 2 for 0.02Hz, 30 for 0.30Hz, 1000 for 10.00Hz.
* @return error code. VIBRATO_ERROR_OK means successful, other codes indicate error.
* @note Note that only mono signals are accepted.
*/
int32_t vibrato_apply(VibratoContext* ct, int16_t* pcm_in, int16_t* pcm_out, int32_t n, int32_t mod_rate);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif // __VIBRATO_H__

