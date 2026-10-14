/**
 ****************************************************************************************
 *
 * @file app_le_telephony_media_client.h
 *
 * @brief Applet - Telephony and Media Audio Client - Header file
 *
 * Copyright (C) RivieraWaves 2009-2024
 * Release Identifier: c73db0ce
 *
 ****************************************************************************************
 */
#include "le_audio_config.h"
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
#ifndef APP_LE_TELEPHONY_MEDIA_CLIENT_H_
#define APP_LE_TELEPHONY_MEDIA_CLIENT_H_

/**
 ****************************************************************************************
 * @defgroup APP_LE_TELEPHONY_MEDIA_CLIENT Telephony and Media Audio Client
 * @ingroup APP_LE
 * @brief Applet adding support of Telephony and Media Audio Service as Client
 *
 * Once connection with an unknown device has been established, discovery of TMAP roles supported by the device can
 * be started by the Mediator using #COMPONENT_CONTROL_START_DISCOVERY Mediator control type.\n
 * Once discovery has been successfully performed, a #COMPONENT_EVENT_SERVICE_CONTENT_DISCOVERED Mediator event is
 * triggered.\n
 * Supported TMAP roles are stored in peer information structure.
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "arch.h"
/// @addtogroup APP_LE_TELEPHONY_MEDIA_CLIENT
/// @{

/*
 * INTERFACES
 ****************************************************************************************
 */
void app_le_telephony_media_client_configure(const void* p_params, component_cb_event cb_event);

/// @} APP_LE_TELEPHONY_MEDIA_CLIENT

#endif // APP_LE_TELEPHONY_MEDIA_CLIENT_H_
#endif