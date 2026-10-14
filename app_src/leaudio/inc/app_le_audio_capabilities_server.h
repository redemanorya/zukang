/**
 ****************************************************************************************
 *
 * @file app_le_audio_capabilities_server.h
 *
 * @brief Applet - Audio Capabilities Server - Header file
 *
 * Copyright (C) RivieraWaves 2009-2024
 * Release Identifier: c73db0ce
 *
 ****************************************************************************************
 */
#include "le_audio_config.h"
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
#ifndef APP_LE_AUDIO_CAPABILITIES_SERVER_H_
#define APP_LE_AUDIO_CAPABILITIES_SERVER_H_

/**
 ****************************************************************************************
 * @defgroup APP_LE_AUDIO_CAPABILITIES_SERVER Audio Capabilities Server
 * @ingroup APPLET
 * @brief Applet adding support of Published Audio Capabilities Service as Server
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "bap.h"
#include "utils_audio.h"
/// @addtogroupAPP_LE_AUDIOCAPABILITIES_SERVER
/// @{

/*
 * TYPES DEFINITION
 ****************************************************************************************
 */

/// Parameters structure for Mediator configuration
typedef struct app_le_audio_capabilities_server_cfg
{
    /// Supported Audio Locations bit field for Sink direction
    uint32_t location_bf_sink;
    /// Supported Audio Locations bit field for Source direction
    uint32_t location_bf_src;
    /// Supported Audio Contexts bit field for Sink direction\n
    /// Meaningful only if location_bf_sink is different than 0
    uint16_t supp_context_bf_sink;
    /// Supported Audio Contexts bit field for Source direction\n
    /// Meaningful only if location_bf_src is different than 0
    uint16_t supp_context_bf_src;
    /// Supported number of frames per SDU (for Sink direction)
    uint8_t nb_frames_per_sdu_max;
    /// Supported channel counts bit field (for Sink direction)
    uint8_t channel_cnt_bf;
} app_le_audio_capabilities_server_cfg_t;

/*
 * INTERFACES
 ****************************************************************************************
 */

/// Mediator interface
///     - configure function
///         - p_params shall not be NULL and shall have #app_le_audio_capabilities_server_cfg_t type
///         - cb_event is not used
///     - control function shall not be used
///     - bind function
///         - A Bond Data Provider may be provided
///         - An Output Provider may be provided for debug purpose only


/// @} APP_LE_AUDIO_CAPABILITIES_SERVER

#endif // APP_LE_AUDIO_CAPABILITIES_SERVER_H_
#endif