/*************************************************************************************
 * @file	l2hc_frame_decoder.h
 * @brief	L2HC Frame Decoder
 *
 * @author	Zhao Ying (Alfred)
 * @version	v1.2.1
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 *************************************************************************************/

#ifndef L2HC_FRAME_DECODER_H
#define L2HC_FRAME_DECODER_H

#include <stdint.h>
#include "audio_l2hc_dec_api.h"

/** L2HC Frame Decoder Context */
#define L2HC_DEC_PRI_CTX_SIZE 57320
typedef struct _L2HCFrameDecoderContext
{
	uint8_t context[L2HC_DEC_PRI_CTX_SIZE];	/**< L2HC Context (=sizeof(L2hcDecState) + 8) */
	int32_t sample_rate;					/**< sample rate in Hz */
	int32_t num_channels;					/**< number of channels */	
	int32_t pcm_length;						/**< PCM length in samples per channel in frame */	
	int32_t bytes_per_sample;				/**< bytes per PCM sample (2,3,4) */    
	void   *pcm;				    		/**< PCM buffer*/	
} L2HCFrameDecoderContext;


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Initialize the L2HC frame decoder.
 * @param[in] ct  Pointer to the L2HC frame decoder context. ct must be 8-byte aligned in memory.
 * @param[in] out_bit_depth  bit depth of the PCM samples to be decoded.Range:16,24,32,-32 for 16-bit integer, 24-bit integer, 32-bit integer and 32-bit floating-point respectively.
 * @return Initialization result. 0 means OK, other codes indicate error.
 */
int32_t l2hc_frame_decoder_initialize(L2HCFrameDecoderContext *ct, int32_t out_bit_depth);


/**
 * @brief  Decode an L2HC frame.
 * @param[in] ct  Pointer to the L2HC frame decoder context. ct must be 8-byte aligned in memory.
 * @param[in] data Data buffer that contains the encoded L2HC frame.
 * @param[in] length Length of the encoded L2HC frame in bytes, which should be no more than AUDIO_L2HCDEC_BITS_BYTES_MAX (=2640) bytes.
 * @return Decoding result. 0 means OK, other codes indicate error.
 * @note Once the L2HC frame is successfully decoded, the sampling rate, number of channels, PCM buffer address, PCM length are represented
 * by sample_rate, num_channels, pcm and pcm_length in ct structure respectively.
 * The data layout in PCM buffer for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 */
int32_t l2hc_frame_decoder_decode(L2HCFrameDecoderContext *ct, uint8_t* data, int32_t length);


#ifdef __cplusplus
}
#endif

#endif // L2HC_FRAME_DECODER_H
