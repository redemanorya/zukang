/**
 *************************************************************************************
 * @file	phase_shifter.h
 * @brief	Phase Shifter
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v2.0.0
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __PHASE_SHIFTER_H__
#define __PHASE_SHIFTER_H__

#include <stdint.h>


/** error code for phase shifter*/
typedef enum _PHASESHIFTER_ERROR_CODE
{
	PHASESHIFTER_ERROR_UNSUPPORTED_NUMBER_OF_CHANNELS = -256,
	PHASESHIFTER_ERROR_SAMPLE_RATE_NOT_SUPPORTED,
	PHASESHIFTER_ERROR_PHASE_SHIFT_OUT_OF_RANGE,
	PHASESHIFTER_ERROR_ILLEGAL_STEP_SIZE,

	// No Error
	PHASESHIFTER_ERROR_OK = 0,					/**< no error              */
} PHASESHIFTER_ERROR_CODE;


/** Phase Shifter Context (1024-point FFT) */
typedef struct _PhaseShifterContext
{
    int32_t sample_rate;
    int32_t num_channels;
    int32_t n;
    int32_t scale_factor;
	int32_t phase_shift;
	int32_t pscos;
	int32_t pssin;
	int32_t x[1024];
	int32_t pcm_x[2][1024];
	int32_t pcm_y[2][1024];
} PhaseShifterContext;


/** Phase Shifter Plus Context (2048-point FFT, better low-frequency accuracy) */
typedef struct _PhaseShifterPlusContext
{
    int32_t sample_rate;
    int32_t num_channels;
    int32_t n;
    int32_t scale_factor;
	int32_t phase_shift;
	int32_t pscos;
	int32_t pssin;
	int32_t x[2048];
	int32_t pcm_x[2][2048];
	int32_t pcm_y[2][2048];
} PhaseShifterPlusContext;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

// ============================================================================
// API: 1024-point FFT
// ============================================================================

/**
 * @brief Initialize the phase shifter (1024-point FFT).
 * @param ct Pointer to a PhaseShifterContext object.
 * @param num_channels Number of channels. Choose either 1 or 2.
 * @param sample_rate Sample rate.
 * @param step_size Step size in samples. Range: 1 <= step_size <= 256 (Must satisfy 1024/step_size >= 4).
 * @return error code. PHASESHIFTER_ERROR_OK means successful, other codes indicate error.
 */
int32_t phase_shifter_init(PhaseShifterContext *ct, int32_t num_channels, int32_t sample_rate, int32_t step_size);


/**
 * @brief Apply phase shifting (1024-point FFT).
 * @param ct Pointer to a PhaseShifterContext object.
 * @param pcm_in PCM input buffer.
 * @param pcm_out PCM output buffer. pcm_out can be the same as pcm_in. In this case, the PCM data is changed in-place.
 * @param phase_shift Phase to shift in degrees. Range: -360 ~ 360 degrees.
 * @return error code. PHASESHIFTER_ERROR_OK means successful, other codes indicate error.
 * @note Note that the number of PCM samples to process is equal to the step_size set in phase_shifter_init(...).
 */
int32_t phase_shifter_apply(PhaseShifterContext *ct, int16_t *pcm_in, int16_t *pcm_out, int32_t phase_shift);


/**
 * @brief Apply phase shifting (24-bit, 1024-point FFT).
 * @param ct Pointer to a PhaseShifterContext object.
 * @param pcm_in PCM input buffer.
 * @param pcm_out PCM output buffer. pcm_out can be the same as pcm_in. In this case, the PCM data is changed in-place.
 * @param phase_shift Phase to shift in degrees. Range: -360 ~ 360 degrees.
 * @return error code. PHASESHIFTER_ERROR_OK means successful, other codes indicate error.
 * @note Note that the number of PCM samples to process is equal to the step_size set in phase_shifter_init(...).
 */
int32_t phase_shifter_apply24(PhaseShifterContext *ct, int32_t *pcm_in, int32_t *pcm_out, int32_t phase_shift);


// ============================================================================
// API: 2048-point FFT
// ============================================================================

/**
 * @brief Initialize the phase shifter plus (2048-point FFT).
 * @param ct Pointer to a PhaseShifterPlusContext object.
 * @param num_channels Number of channels. Choose either 1 or 2.
 * @param sample_rate Sample rate.
 * @param step_size Step size in samples. Range: 1 <= step_size <= 512 (Must satisfy 2048/step_size >= 4).
 * @return error code. PHASESHIFTER_ERROR_OK means successful, other codes indicate error.
 */
int32_t phase_shifter_plus_init(PhaseShifterPlusContext *ct, int32_t num_channels, int32_t sample_rate, int32_t step_size);


/**
 * @brief Apply phase shifting plus (2048-point FFT).
 * @param ct Pointer to a PhaseShifterPlusContext object.
 * @param pcm_in PCM input buffer.
 * @param pcm_out PCM output buffer. pcm_out can be the same as pcm_in. In this case, the PCM data is changed in-place.
 * @param phase_shift Phase to shift in degrees. Range: -360 ~ 360 degrees.
 * @return error code. PHASESHIFTER_ERROR_OK means successful, other codes indicate error.
 * @note Note that the number of PCM samples to process is equal to the step_size set in phase_shifter_plus_init(...).
 */
int32_t phase_shifter_plus_apply(PhaseShifterPlusContext *ct, int16_t *pcm_in, int16_t *pcm_out, int32_t phase_shift);


/**
 * @brief Apply phase shifting plus (24-bit, 2048-point FFT).
 * @param ct Pointer to a PhaseShifterPlusContext object.
 * @param pcm_in PCM input buffer.
 * @param pcm_out PCM output buffer. pcm_out can be the same as pcm_in. In this case, the PCM data is changed in-place.
 * @param phase_shift Phase to shift in degrees. Range: -360 ~ 360 degrees.
 * @return error code. PHASESHIFTER_ERROR_OK means successful, other codes indicate error.
 * @note Note that the number of PCM samples to process is equal to the step_size set in phase_shifter_plus_init(...).
 */
int32_t phase_shifter_plus_apply24(PhaseShifterPlusContext *ct, int32_t *pcm_in, int32_t *pcm_out, int32_t phase_shift);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif//__PHASE_SHIFTER_H__
