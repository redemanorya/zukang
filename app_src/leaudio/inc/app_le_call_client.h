/**
 ****************************************************************************************
 *
 * @file app_le_call_client.h
 *
 * @brief Applet - Call Client - Header file
 *
 * Copyright (C) RivieraWaves 2009-2024
 * Release Identifier: c73db0ce
 *
 ****************************************************************************************
 */
#include "le_audio_config.h"
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
#ifndef APP_LE_CALL_CLIENT_H_
#define APP_LE_CALL_CLIENT_H_

/**
 ****************************************************************************************
 * @defgroup APP_LE_CALL_CLIENT Call Client
 * @ingroup APP_LE
 * @brief Applet adding support of Telephone Bearer Service as Client
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>

/// @addtogroup APP_LE_CALL_CLIENT
/// @{

void app_le_client_call_accept(void);
void app_le_client_call_terminate(void);
void app_le_call_client_state_service_discovery(uint8_t event, uint16_t status, uint8_t conidx);
/// @} APP_LE_CALL_CLIENT

#endif // APP_LE_CALL_CLIENT_H_
#endif