/**
 ****************************************************************************************
 *
 * @file app_le_common_audio_client.h
 *
 * @brief Applet - Common Audio Client - Header file
 *
 * Copyright (C) RivieraWaves 2009-2024
 * Release Identifier: c73db0ce
 *
 ****************************************************************************************
 */
#include "le_audio_config.h"
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
#ifndef APP_LE_COMMON_AUDIO_CLIENT_H_
#define APP_LE_COMMON_AUDIO_CLIENT_H_

/**
 ****************************************************************************************
 * @defgroup APP_LE_COMMON_AUDIO_CLIENT Common Audio Client
 * @ingroup APP_LE
 * @brief Applet adding support of Common Audio Service as Client
 *
 * Once connection with an unknown device has been established, discovery of Common Audio Service can be started
 * by the Mediator using #COMPONENT_CONTROL_START_DISCOVERY control type.\n
 * If an included Coordinated Set Identification service is found during the discovery, a
 * #COMPONENT_EVENT_SERVICE_DISCOVERED Mediator event is triggered.\n
 * Once discovery has been successfully performed, a #COMPONENT_EVENT_SERVICE_CONTENT_DISCOVERED Mediator event is
 * triggered.
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>
#include "arch.h"
#include "prf_types.h"

/// @addtogroup APP_LE_COMMON_AUDIO_CLIENT
/// @{

/*
 * INTERFACES
 ****************************************************************************************
 */
void app_le_common_audio_client_transition_service_discovery(uint8_t event, uint8_t conidx, uint16_t status,
                                                                      const prf_svc_t* p_svc);
void app_le_common_audio_client_configure(const void* p_params, component_cb_event cb_event);

/// @} APP_LE_COMMON_AUDIO_CLIENT

#endif // APP_LE_COMMON_AUDIO_CLIENT_H_
#endif