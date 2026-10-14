/**
 *************************************************************************************
 * @file	flash_surround.h
 * @brief	Streaming stereo-to-surround upmixer
 * @version	0.4.1
 *
 * @author	ZHAO Ying (Alfred)
 *************************************************************************************
 */

#ifndef FLASH_SURROUND_H
#define FLASH_SURROUND_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FLASH_SURROUND_LAYOUT_5_1 = 0,
    FLASH_SURROUND_LAYOUT_7_1 = 1
} flash_surround_layout_t;

/** error code for flash_surround */
typedef enum _FLASH_SURROUND_ERROR_CODE
{
    FLASH_SURROUND_ERROR_ILLEGAL_BLOCK_LENGTH = -256,
    FLASH_SURROUND_ERROR_ILLEGAL_LAYOUT,
    FLASH_SURROUND_ERROR_ILLEGAL_LFE_PASSBAND,

    // No Error
    FLASH_SURROUND_ERROR_OK = 0, /**< no error */
} FLASH_SURROUND_ERROR_CODE;

/**
 * @brief Estimate the memory usage of the flash_surround module before initialization.
 * @param[in]  layout        FLASH_SURROUND_LAYOUT_5_1 or FLASH_SURROUND_LAYOUT_7_1.
 * @param[in]  blk_len       Input/output block length in samples, must satisfy 64 <= blk_len <= 1024.
 *                           A larger blk_len increases memory overhead and computational load,
 *                           but yields higher spectral precision.
 * @param[out] persistent_size  Size of persistent memory in bytes. Contents must not be modified between calls.
 * @param[out] scratch_size     Size of scratch memory in bytes. Can be reused between calls.
 * @return error code. FLASH_SURROUND_ERROR_OK means successful, other codes indicate error.
 */
int32_t flash_surround_estimate_memory_usage(flash_surround_layout_t layout, int blk_len,
                                             uint32_t *persistent_size, uint32_t *scratch_size);

/**
 * @brief Initialize the flash_surround module in caller-supplied memory.
 * @param[in]  ct        Pointer to persistent memory (persistent_size bytes).
 * @param[in]  scratch   Pointer to scratch memory (scratch_size bytes).
 * @param[in]  fs        Sample rate in Hz.
 * @param[in]  layout    FLASH_SURROUND_LAYOUT_5_1 or FLASH_SURROUND_LAYOUT_7_1.
 * @param[in]  blk_len   Input/output block length in samples, must satisfy 64 <= blk_len <= 1024.
 *                       A larger blk_len increases memory overhead and computational load,
 *                       but yields higher spectral precision.
 *                       Each call to run16/run24 consumes exactly 2 * blk_len interleaved stereo samples and
 *                       produces num_out_channels * blk_len output samples.
 * @param[in]  lfe_passband  LFE channel passband edge in Hz (range 20~120).
 * @return error code. FLASH_SURROUND_ERROR_OK means successful, other codes indicate error.
 */
int32_t flash_surround_init(uint8_t *ct, uint8_t *scratch,
                            int fs, flash_surround_layout_t layout, int blk_len, int lfe_passband);

/**
 * @brief Process one stereo input frame (16-bit PCM) and produce multi-channel output.
 *
 * The processor works in contiguous blocks of blk_len samples. xin contains exactly
 * 2 * blk_len interleaved stereo samples as L0,R0,L1,R1,...
 *
 * @param[in]  ct   Pointer to persistent memory.
 * @param[in]  xin  Input PCM data. Interleaved stereo, size = 2 * blk_len.
 * @param[out] xout Output PCM data. Interleaved multi-channel, size = num_out_channels * blk_len.
 * @return error code. FLASH_SURROUND_ERROR_OK means successful, other codes indicate error.
 */
int32_t flash_surround_apply16(uint8_t *ct, int16_t *xin, int16_t *xout);

/**
 * @brief Process one stereo input frame (24-bit PCM) and produce multi-channel output.
 *
 * 24-bit samples are stored in the lower 24 bits of int32_t. xin is interleaved stereo.
 *
 * @param[in]  ct   Pointer to persistent memory.
 * @param[in]  xin  Input PCM data. Interleaved stereo, size = 2 * blk_len.
 * @param[out] xout Output PCM data. Interleaved multi-channel, size = num_out_channels * blk_len.
 * @return error code. FLASH_SURROUND_ERROR_OK means successful, other codes indicate error.
 */
int32_t flash_surround_apply24(uint8_t *ct, int32_t *xin, int32_t *xout);

#ifdef __cplusplus
}
#endif

#endif /* FLASH_SURROUND_H */
