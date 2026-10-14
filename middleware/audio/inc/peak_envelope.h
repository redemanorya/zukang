/**
 *************************************************************************************
 * @file	peak_envelope.h
 * @brief	Instantaneous-attack, exponential-release peak envelope follower.
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __PEAK_ENVELOPE_H__
#define __PEAK_ENVELOPE_H__

#include <stdint.h>


/** error code for peak envelope */
typedef enum _PEAK_ENVELOPE_ERROR_CODE
{
	PEAK_ENVELOPE_ERROR_NUMBER_OF_CHANNELS_NOT_SUPPORTED = -256,
	PEAK_ENVELOPE_ERROR_ILLEGAL_RELEASE_TIME,

	// No Error
	PEAK_ENVELOPE_ERROR_OK = 0,					/**< no error              */
} PEAK_ENVELOPE_ERROR_CODE;


/** Peak Envelope Context */
typedef struct _PeakEnvelopeContext
{
	int32_t num_channels;
	int32_t sample_rate;
	int32_t alpha_release;			/**< release coefficient */
	int32_t envelope_state[2];		/**< per-channel envelope state */

} PeakEnvelopeContext;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


/**
 * @brief Initialize peak envelope follower.
 * @param ct Pointer to a PeakEnvelopeContext object.
 * @param num_channels number of channels. Choose either 1 or 2.
 * @param sample_rate Sample rate in Hz.
 * @param release_time Release time constant in milliseconds. Must be > 0.
 * @return error code. PEAK_ENVELOPE_ERROR_OK means successful, other codes indicate error.
 */
int32_t peak_envelope_init(PeakEnvelopeContext* ct, int32_t num_channels, int32_t sample_rate, int32_t release_time);


/**
 * @brief Apply peak envelope following to a frame of 16-bit PCM data.
 * @param ct Pointer to a PeakEnvelopeContext object.
 * @param pcm_in Address of the PCM input. The PCM layout for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 * @param pcm_out Address of the envelope output (int16_t). The PCM layout for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 *        pcm_out can be the same as pcm_in. In this case, the PCM is overwritten by the envelope in-place.
 * @param n Number of PCM samples to process per channel.
 * @return error code. PEAK_ENVELOPE_ERROR_OK means successful, other codes indicate error.
 */
int32_t peak_envelope_apply16(PeakEnvelopeContext* ct, int16_t *pcm_in, int16_t *pcm_out, int32_t n);


/**
 * @brief Apply peak envelope following to a frame of 24-bit PCM data.
 * @param ct Pointer to a PeakEnvelopeContext object.
 * @param pcm_in Address of the PCM input. The PCM layout for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 * @param pcm_out Address of the envelope output (int32_t, Q1.23). The PCM layout for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 *        pcm_out can be the same as pcm_in. In this case, the PCM is overwritten by the envelope in-place.
 * @param n Number of PCM samples to process per channel.
 * @return error code. PEAK_ENVELOPE_ERROR_OK means successful, other codes indicate error.
 */
int32_t peak_envelope_apply24(PeakEnvelopeContext* ct, int32_t *pcm_in, int32_t *pcm_out, int32_t n);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif//__PEAK_ENVELOPE_H__
