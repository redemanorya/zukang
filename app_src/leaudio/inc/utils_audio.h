/**
 ****************************************************************************************
 *
 * @file utils_audio.c
 *
 * @brief Application - Utilitaries for Audio - Header file
 *
 * Copyright (C) RivieraWaves 2009-2024
 * Release Identifier: c73db0ce
 *
 ****************************************************************************************
 */

#ifndef UTILS_AUDIO_H_
#define UTILS_AUDIO_H_

/**
 ****************************************************************************************
 * @defgroup UTILS_AUDIO Audio
 * @ingroup UTILS_ENV
 * @brief Audio
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

// #include "app_int.h"
#include "bap.h"


/// @addtogroup UTILS_AUDIO
/// @{

/*
 * DEFINES
 ****************************************************************************************
 */

// Length of packed codec configuration
#define UTILS_AUDIO_LE_CODEC_CFG_PACKED_LEN    (19U)

/*
 * TYPES DEFINITION
 ****************************************************************************************
 */

/// LE Audio configuration structure
typedef struct utils_audio_le_cfg
{
    /// Supported TMAP roles
    uint16_t tmap_role_bf;
    /// Number of Sink ASEs
    uint8_t nb_ases_sink;
    /// Number of Source ASEs
    uint8_t nb_ases_src;
    /// Connection index of other Coordinated Set member\n
    /// GAP_INVALID_CONIDX indicates that device is not part of a Coordinated Set or alone in a Coordinated Set
    uint8_t conidx_set_member;
    /// Supported BAP unicast audio configurations bit field for Sink direction\n
    /// Meaningful only if nb_ases_sink != 0
    uint32_t bap_uc_cfg_bf_sink;
    /// Supported BAP unicast audio configurations bit field for Source direction\n
    /// Meaningful only if nb_ases_sink != 0
    uint32_t bap_uc_cfg_bf_src;
    /// Number of devices in Coordinated Set\n
    /// 0 means device is not part of a Coordinated Set
    uint8_t nb_set_members;
    /// Rank in the Coordinated Set\n
    /// Meaningful only if device is part of a Coordinated Set\n
    /// 0 means that information is not available
    uint8_t set_rank;
} utils_audio_le_cfg_t;

/*
 * FUNCTIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Initialize Application Global Environment for Audio
 ****************************************************************************************
 */
void utils_audio_init(void);

/**
 ****************************************************************************************
 * @brief Handle connection established with a peer device
 *
 * @param[in] conidx        Connection index
 ****************************************************************************************
 */
void utils_audio_add_peer(uint8_t conidx);

/**
 ****************************************************************************************
 * @brief Pack codec configuration
 * See #UTILS_AUDIO_LE_CODEC_CFG_PACKED_LEN for size of data part of output buffer
 *
 * @param[in] p_output      Pointer to output buffer
 * @param[in] p_codec_cfg   Pointer to codec configuration
 ****************************************************************************************
 */
void utils_audio_le_pack_codec_cfg(gaf_ltv_t* p_output, const bap_cfg_param_t* p_codec_cfg);

/**
 ****************************************************************************************
 * @return Get LE Audio configuration of a given connected device
 *
 * @param[in] conidx        Connection index
 ****************************************************************************************
 */
utils_audio_le_cfg_t* utils_audio_le_get_cfg(uint8_t conidx);

/**
 ****************************************************************************************
 * @return Return local LE Audio configuration
 ****************************************************************************************
 */
utils_audio_le_cfg_t* utils_audio_le_get_cfg_local(void);

/// @} UTILS_AUDIO

#endif // UTILS_AUDIO_H_
