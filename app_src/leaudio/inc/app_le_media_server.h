/**
 ****************************************************************************************
 *
 * @file app_le_media_server.h
 *
 * @brief Applet - Media Server - Header file
 *
 *
 ****************************************************************************************
 */
#include "le_audio_config.h"
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
#ifndef APP_LE_MEDIA_SERVER_H_
#define APP_LE_MEDIA_SERVER_H_

/**
 ****************************************************************************************
 * @defgroup APP_LE_MEDIA_SERVER Media Server
 * @ingroup APP_LE
 * @brief Applet adding support of Media Control Service as Server
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "arch.h"
#include "acc_mc.h"

/// @addtogroup APP_LE_MEDIA_SERVER
/// @{

/*
 * DEFINES
 ****************************************************************************************
 */

/// CCID of the Media Content Service instance
#define MEDIA_CCID          (0x34)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Event type values
enum app_le_media_server_event_type
{
    /// Media state has been updated
    APP_LE_MEDIA_SERVER_EVENT_STATE_UPDATED,
};

/*
 * TYPES DEFINITION
 ****************************************************************************************
 */

/// Parameters structure for #APP_LE_MEDIA_SERVER_EVENT_STATE_UPDATED event type
typedef struct app_le_media_server_event_state_updated
{
    /// New state (see #acc_mc_media_state enumeration)
    uint8_t state;
} app_le_media_server_event_state_updated_t;

/*
 * INTERFACES
 ****************************************************************************************
 */
void app_le_mcs_play(void);
void app_le_mcs_pause(void);
void app_le_mcs_stop(void);
void app_le_mcs_previous(void);
void app_le_mcs_next(void);

void app_le_media_server_configure(const void* p_params, component_cb_event cb_event);

/// @} APP_LE_MEDIA_SERVER

#endif // APP_LE_MEDIA_SERVER_H_
#endif