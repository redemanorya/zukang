/**
 *************************************************************************************
 * @file	reverb_plate_me.h
 * @brief	Plate class reverberation effect with memory efficient implementation.
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v3.1.2
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __REVERB_PLATE_ME_H__
#define __REVERB_PLATE_ME_H__

#include <stdint.h>


/** error code for reverb plate ME */
typedef enum _REVERB_PLATE_ME_ERROR_CODE
{
    REVERB_PLATE_ME_ERROR_SAMPLE_RATE_NOT_SUPPORTED = -256,
	REVERB_PLATE_ME_ERROR_NUMBER_OF_CHANNELS_NOT_SUPPORTED,
	REVERB_PLATE_ME_ERROR_ILLEGAL_PREDELAY,
	REVERB_PLATE_ME_ERROR_ILLEGAL_DIFFUSION,
	REVERB_PLATE_ME_ERROR_ILLEGAL_DECAY,
	REVERB_PLATE_ME_ERROR_ILLEGAL_DAMPING,
	REVERB_PLATE_ME_ERROR_ILLEGAL_WETDRYMIX,
	REVERB_PLATE_ME_ERROR_ILLEGAL_MAX_PREDELAY,
	REVERB_PLATE_ME_ERROR_NULL_POINTER,

	// No Error
	REVERB_PLATE_ME_ERROR_OK = 0,					/**< no error              */
} REVERB_PLATE_ME_ERROR_CODE;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

/**
 * @brief Estimate memory usage for reverberation effect.
 * @param sample_rate Sample rate.
 * @param max_predelay Maximum pre-delay in samples. Used for predelayline allocation. The actual pre-delay used in processing can be configured later via reverb_plate_me_configure() and can be less than or equal to this value.
 * @param persistent_size Output: required buffer size in bytes (includes context and all delay lines).
 * @return error code. REVERB_PLATE_ME_ERROR_OK means successful, other codes indicate error.
 */
int32_t reverb_plate_me_estimate_memory_usage(int32_t sample_rate, int32_t max_predelay, uint32_t *persistent_size);

/**
 * @brief Initialize reverberation audio effect module.
 * @param ct Pointer to a buffer allocated for the reverb context (allocated via reverb_plate_me_estimate_memory_usage).
 * @param num_channels Number of channels. Both 1 and 2 channels are supported.
 * @param sample_rate Sample rate.
 * @param highcut_freq Lowpass filter cutoff in the range 0 to sample_rate/2.
 * @param modulation_en Enable delay modulation. 0: disabled, 1: enabled
 * @param max_predelay Maximum pre-delay in samples. Used for predelayline allocation. The actual pre-delay used in processing can be configured later via reverb_plate_me_configure() and can be less than or equal to this value.
 * @return error code. REVERB_PLATE_ME_ERROR_OK means successful, other codes indicate error.
 */
int32_t reverb_plate_me_init(uint8_t *ct, int32_t num_channels, int32_t sample_rate, int32_t highcut_freq, int32_t modulation_en, int32_t max_predelay);


/**
 * @brief Configure reverberation effect's parameters.
 * @param ct Pointer to a buffer allocated for the reverb context.
 * @param predelay Pre-delay in samples. Range: 0 ~ max_predelay (the max_predelay is set during initialization and used for memory allocation, predelay can be configured to any value between 0 and max_predelay).
 * @param diffusion Density of reverb tail. Diffusion is proportional to the rate at which the reverb tail builds in density. Increasing Diffusion pushes the reflections closer together, thickening the sound. Reducing Diffusion creates more discrete echoes. Range: 0 ~ 100 for 0 ~ 100%.
 * @param decay Decay of reverb tail. Decay factor is proportional to the time it takes for reflections to run out of energy. To model a large room, use a long reverb tail. To model a small room, use a short reverb tail. Range: 0 ~ 100 for 0 ~ 100%.
 * @param damping High frequency damping. High frequency damping is proportional to the attenuation of high frequencies in the reverberation output. Setting High frequency damping to a large value makes high-frequency reflections decay faster than low-frequency reflections. Range: 0 ~ 10000 for 0.00 ~ 100.00%.
 * @param wetdrymix The ratio of wet (reverberated) signal to the mixed output (wet+dry). Range: 0 ~ 100 for 0 ~ 100%.
 * @return error code. REVERB_PLATE_ME_ERROR_OK means successful, other codes indicate error.
 */
int32_t reverb_plate_me_configure(uint8_t *ct, int32_t predelay, int32_t diffusion, int32_t decay, int32_t damping, int32_t wetdrymix);


/**
 * @brief Apply reverberation effect to a frame of PCM data (16-bit PCM in & out).
 * @param ct Pointer to a buffer allocated for the reverb context.
 * @param pcm_in Address of the PCM input. The PCM layout must be the same as in Microsoft WAVE format for both mono and stereo cases.
 * @param pcm_out Address of the PCM output. The PCM layout must be the same as in Microsoft WAVE format for both mono and stereo cases.
 *        pcm_out can be the same as pcm_in. In this case, the PCM is changed in-place.
 * @param n Number of PCM samples to process.
 * @return error code. REVERB_PLATE_ME_ERROR_OK means successful, other codes indicate error.
 */
int32_t reverb_plate_me_apply16(uint8_t *ct, int16_t *pcm_in, int16_t *pcm_out, int32_t n);


/**
 * @brief Apply reverberation effect to a frame of PCM data (24-bit PCM in & out).
 * @param ct Pointer to a buffer allocated for the reverb context.
 * @param pcm_in Address of the PCM input. The PCM layout must be the same as in Microsoft WAVE format for both mono and stereo cases.
 * @param pcm_out Address of the PCM output. The PCM layout must be the same as in Microsoft WAVE format for both mono and stereo cases.
 *        pcm_out can be the same as pcm_in. In this case, the PCM is changed in-place.
 * @param n Number of PCM samples to process.
 * @return error code. REVERB_PLATE_ME_ERROR_OK means successful, other codes indicate error.
 */
int32_t reverb_plate_me_apply24(uint8_t *ct, int32_t *pcm_in, int32_t *pcm_out, int32_t n);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif//__REVERB_PLATE_ME_H__
