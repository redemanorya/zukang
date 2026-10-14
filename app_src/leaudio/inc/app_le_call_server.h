/**
 ****************************************************************************************
 *
 * @file app_le_call_server.h
 *
 * @brief Applet - Call Server - Header file
 *
 *
 ****************************************************************************************
 */
#include "le_audio_config.h"
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
#ifndef APP_LE_CALL_SERVER_H_
#define APP_LE_CALL_SERVER_H_

/**
 ****************************************************************************************
 * @defgroup APP_LE_CALL_SERVER Call Server
 * @ingroup APP_LE
 * @brief Call Server Applet description
 *
 * Applet adding support of Telephone Bearer Service as Server.\n
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "arch.h"

/// @addtogroup APP_LE_CALL_SERVER
/// @{

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Specific control type values
enum app_le_call_server_control_type
{
    /// Inform that ringtone stream has been stopped
    APP_LE_CALL_SERVER_CONTROL_RINGTONE_STOPPED,
};

/// Stream type type values
enum app_le_call_server_type
{
    /// Call
    APP_LE_CALL_SERVER_TYPE_CALL = 0U,
    /// Ringtone
    APP_LE_CALL_SERVER_TYPE_RINGTONE,
};

/// Event type values
enum app_le_call_server_event_type
{
    /// Request to start a stream
    APP_LE_CALL_SERVER_EVENT_START_REQ,
    /// Request to stop a stream
    APP_LE_CALL_SERVER_EVENT_STOP_REQ,
};

/*
 * TYPES DEFINITION
 ****************************************************************************************
 */

/// Parameters structure for #APP_LE_CALL_SERVER_EVENT_START_REQ event type
typedef struct app_le_call_server_event_start_req
{
    /// CCID
    uint8_t ccid;
    /// Type
    uint8_t type;
} app_le_call_server_event_start_req_t;

/// Parameters structure for #APP_LE_CALL_SERVER_EVENT_STOP_REQ event type
typedef struct app_le_call_server_event_stop_req
{
    /// Type
    uint8_t type;
} app_le_call_server_event_stop_req_t;

/*
 * INTERFACES
 ****************************************************************************************
 */
void app_le_server_outgoing_call(const char* p_uri, const char* p_friendly_name);
void app_le_server_incoming_call(const char* p_uri, const char* p_friendly_name);
void app_le_server_update_bearer_provider_name(const char* p_bearer_provider_name);
void app_le_server_update_bearer_technology(uint8_t bearer_technology);
void app_le_server_terminate(uint8_t reason);
void app_le_server_remote_alert_start(void);
void app_le_server_answer(void);

void app_le_call_server_stopped(void);
void app_le_call_server_configure(const void* p_params, component_cb_event cb_event);

/// @} APP_LE_CALL_SERVER

#endif // APP_LE_CALL_SERVER_H_
#endif