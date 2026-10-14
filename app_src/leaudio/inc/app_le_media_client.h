/**
 ****************************************************************************************
 *
 * @file app_le_media_client.h
 *
 * @brief Applet - Media Client - Header file
 *
 *
 ****************************************************************************************
 */
#include "le_audio_config.h"
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
#ifndef APP_LE_MEDIA_CLIENT_H_
#define APP_LE_MEDIA_CLIENT_H_

/**
 ****************************************************************************************
 * @defgroup APP_LE_MEDIA_CLIENT Media Client
 * @ingroup APP_LE
 * @brief Applet adding support of Media Control Service as Client\n
 * Only one instance of Media Control Service is supported
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */


#include <stdlib.h>
#include "arch.h"
#include "prf_types.h"

/// @addtogroup APP_LE_MEDIA_CLIENT
/// @{

/*
 * INTERFACES
 ****************************************************************************************
 */

void app_le_media_client_state_service_discovery(uint8_t event, uint16_t status, uint8_t conidx,
                                                          const prf_svc_t* p_svc);
void app_le_client_media_play(void);
void app_le_client_media_pause(void);
void app_le_client_media_stop(void);
void app_le_client_media_previous(void);
void app_le_client_media_next(void);

void app_le_media_client_configure(const void* p_params, component_cb_event cb_event);
/// @} APP_LE_MEDIA_CLIENT

#endif // APP_LE_MEDIA_CLIENT_H_
#endif