/**
 *************************************************************************************
 * @file	lr_swap.h
 * @brief	Swap Left & Right Channel
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __LR_SWAP_H__
#define __LR_SWAP_H__

#include <stdint.h>


/** error code for LR Swap */
typedef enum _LR_SWAP_ERROR_CODE
{
	LR_SWAP_ERROR_NUMBER_OF_CHANNELS_NOT_SUPPORTED = -256,
	LR_SWAP_ERROR_NON_POSITIVE_NUMBER_OF_SAMPLES,	

	// No Error
	LR_SWAP_ERROR_OK = 0,					/**< no error              */
} LR_SWAP_ERROR_CODE;


/** LR Swap Context */
typedef struct _LRSwapContext
{
	int32_t num_channels;
	
} LRSwapContext;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


/**
 * @brief Initialize the context for left & right swapping.
 * @param ct Pointer to a LRSwapContext object.
 * @param num_channels Number of channels. Only 1 (mono) and 2 (stereo) are supported.
 * @return error code. LR_SWAP_ERROR_OK means successful, other codes indicate error.
 */
int32_t lr_swap_init(LRSwapContext* ct, int32_t num_channels);


/**
 * @brief Apply left & right swapping to a frame of PCM data (16-bit).
 * @param ct Pointer to a LRSwapContext object.
 * @param pcm_in Address of the PCM input. The PCM layout must be the same as in Microsoft WAVE format for both mono and stereo cases.
 * @param pcm_out Address of the PCM output. The PCM layout is the same as in Microsoft WAVE format for both mono and stereo cases.
 *        pcm_out can be the same as pcm_in. In this case, the PCM is changed in-place.
 * @param n Number of PCM samples to process.
 * @return error code. LR_SWAP_ERROR_OK means successful, other codes indicate error. 
 */
int32_t lr_swap_apply16(LRSwapContext* ct, int16_t *pcm_in, int16_t *pcm_out, int32_t n);


/**
 * @brief Apply left & right swapping to a frame of PCM data (24-bit).
 * @param ct Pointer to a LRSwapContext object.
 * @param pcm_in Address of the PCM input. The PCM layout must be the same as in Microsoft WAVE format for both mono and stereo cases.
 * @param pcm_out Address of the PCM output. The PCM layout is the same as in Microsoft WAVE format for both mono and stereo cases.
 *        pcm_out can be the same as pcm_in. In this case, the PCM is changed in-place.
 * @param n Number of PCM samples to process.
 * @return error code. LR_SWAP_ERROR_OK means successful, other codes indicate error.
 */
int32_t lr_swap_apply24(LRSwapContext* ct, int32_t *pcm_in, int32_t *pcm_out, int32_t n);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif//__LR_SWAP_H__
