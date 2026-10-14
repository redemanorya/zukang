/**
 ****************************************************************************************
 *
 * @file app_le_common_audio_server.h
 *
 * @brief App_le - Common Audio Server - Header file
 *
 *
 ****************************************************************************************
 */
#include "le_audio_config.h"
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
#ifndef APP_LE_COMMON_AUDIO_SERVER_H_
#define APP_LE_COMMON_AUDIO_SERVER_H_

/**
 ****************************************************************************************
 * @defgroup APP_LE_COMMON_AUDIO_SERVER Common Audio Server
 * @ingroup APP_LE
 * @brief Applet adding support of Common Audio Service as Server
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

/// @addtogroup APP_LE_COMMON_AUDIO_SERVER
/// @{

/*
 * TYPES DEFINITIONS
 ****************************************************************************************
 */

/// Parameters structure for Mediator configuration
typedef struct app_le_common_audio_server_cfg
{
    /// Set local index
    uint8_t set_lid;
} app_le_common_audio_server_cfg_t;


/// @} APP_LE_COMMON_AUDIO_SERVER

#endif // APP_LE_COMMON_AUDIO_SERVER_H_
#endif