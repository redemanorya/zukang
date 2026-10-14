/**
 ****************************************************************************************
 *
 * @file app_le_telephony_media_server.h
 *
 * @brief Applet - Telephony and Media Audio Server - Header file
 *
 *
 ****************************************************************************************
 */
#include "le_audio_config.h"
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
#ifndef APP_LE_TELEPHONY_MEDIA_SERVER_H_
#define APP_LE_TELEPHONY_MEDIA_SERVER_H_

/**
 ****************************************************************************************
 * @defgroup APP_LE_TELEPHONY_MEDIA_SERVER Telephony and Media Audio Server
 * @ingroup APP_LE
 * @brief Applet adding support of Telephony and Media Audio Service as Server
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "tmap.h"

/// @addtogroup APP_LE_TELEPHONY_MEDIA_SERVER
/// @{

/*
 * TYPES DEFINITION
 ****************************************************************************************
 */

/// Parameters structure for Mediator configuration
typedef struct app_le_telephony_media_server_cfg
{
    /// TMAP roles (see #tmap_role_bf enumeration)
    uint16_t role_bf;
} app_le_telephony_media_server_cfg_t;


/// @} APP_LE_TELEPHONY_MEDIA_SERVER

#endif // APP_LE_TELEPHONY_MEDIA_SERVER_H_
#endif