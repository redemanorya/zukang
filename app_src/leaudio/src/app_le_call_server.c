/**
 ****************************************************************************************
 *
 * @file app_le_call_server.c
 *
 * @brief Applet - Call Server
 *
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "le_audio_config.h"
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
#include "bb_api.h"
#include "app_le_call_server.h"
#include "acc_tbs.h"
#include "hl_error.h"
#include "debug.h"
#include "app_le_call_server.h"

/*
 * DEFINES
 ****************************************************************************************
 */

// CCID of the Telephone Bearer Service instance
#define CALL_BEARER_CCID                    (0x43)
// Configuration bit field for Telephone Bearer Service instance
#define CALL_BEARER_CFG_BF                  (ACC_TBS_CFG_SIGNAL_STRENGTH_SUPP_BIT \
                                                    | ACC_TBS_CFG_FRIENDLY_NAME_SUPP_BIT)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

// Event values for Call state machine
enum app_le_call_server_event_call
{
    // Incoming call
    EVENT_CALL_INCOMING = 0U,
    // Outgoing call
    EVENT_CALL_OUTGOING,
    // Incoming call accepted
    EVENT_CALL_ACCEPTED,
    // Incoming call rejected
    EVENT_CALL_REJECTED,
    // Remote alert start signal has been received
    EVENT_CALL_REMOTE_ALERT_STARTED,
    // Remote user has answered
    EVENT_CALL_ANSWERED,
    // Call has been terminated
    EVENT_CALL_TERMINATED,
    // Ringtone stream has been stopped
    EVENT_CALL_RINGTONE_STOPPED,
};

// Status bit field meaning
enum app_le_call_server_status_bf
{
    // Indicate if Call stream is expected to be started
    STATUS_START_CALL_POS = 0U,
    STATUS_START_CALL_BIT = CO_BIT(STATUS_START_CALL_POS),

    // Indicate if Call stream has been started
    STATUS_CALL_STARTED_POS = 1U,
    STATUS_CALL_STARTED_BIT = CO_BIT(STATUS_CALL_STARTED_POS),
};

/*
 * TYPES DEFINITION
 ****************************************************************************************
 */

// Bond data structure
typedef struct app_le_call_server_bond_data
{
    // Client configuration bit field for Telephone Bearer Service
    uint32_t cli_cfg_bf;
    // Signal strength reporting interval (in seconds)
    uint8_t signal_strength_intv_s;
} app_le_call_server_bond_data_t;

// Bearer information
typedef struct app_le_call_server_bearer
{
    // Pointer to Bearer Provider Name value
    const char* p_provider_name;
    // Pointer to Incoming Call Friendly Name
    const char* p_friendly_name;
    // Pointer to Call URI
    const char* p_uri;
    // Bond data
    app_le_call_server_bond_data_t bond_data;
    // Signal strength
    uint8_t signal_strength;
    // Call ID
    uint8_t call_id;
    // Bearer local index
    uint8_t bearer_lid;
} app_le_call_server_bearer_t;

// Environment structure
typedef struct app_le_call_server_env
{
    // // Callback function for backward communication with Mediator
    component_cb_event cb_mediator_event;
    // Bearer information
    app_le_call_server_bearer_t bearer_info;
    // Status bit field (see #app_le_call_server_status_bf enumeration)
    uint8_t status_bf;
} app_le_call_server_env_t;

/*
 * LOCAL VARIABLES
 ****************************************************************************************
 */

// URI Schemes Supported List value
const char app_le_call_server_uri_schemes_supported[] = "tel,skype";
// UCI value
const char app_le_call_server_uci[] = "E.164";

// Pointer to environment
__STATIC app_le_call_server_env_t* p_env;
__STATIC void app_le_call_server_state_call(uint8_t event, uint16_t status);

/*
 * HOOKS FOR UPPER APPLICATION
 ****************************************************************************************
 */
void app_le_server_incoming_call(const char* p_uri, const char* p_friendly_name)
{
    p_env->bearer_info.p_uri = p_uri;
    p_env->bearer_info.p_friendly_name = p_friendly_name;
    app_le_call_server_state_call(EVENT_CALL_INCOMING, GAF_ERR_NO_ERROR);
}

void app_le_server_outgoing_call(const char* p_uri, const char* p_friendly_name)
{
    p_env->bearer_info.p_uri = p_uri;
    p_env->bearer_info.p_friendly_name = p_friendly_name;
    app_le_call_server_state_call(EVENT_CALL_OUTGOING, GAF_ERR_NO_ERROR);
}

void app_le_server_update_bearer_provider_name(const char* p_bearer_provider_name)
{
    p_env->bearer_info.p_provider_name = p_bearer_provider_name;
    uint16_t status = acc_tbs_set_long(p_env->bearer_info.bearer_lid, ACC_TB_CHAR_TYPE_PROV_NAME,
                                      strlen(p_bearer_provider_name), (const uint8_t*)p_bearer_provider_name);
    if (status)
    {
        APP_DBG("acc_tbs_set_long err 0x%x\n", status);
    }
}

void app_le_server_update_bearer_technology(uint8_t bearer_technology)
{
    uint16_t status = acc_tbs_set_bearer_techno(p_env->bearer_info.bearer_lid, bearer_technology);
    if (status)
    {
        APP_DBG("acc_tbs_set_long err 0x%x\n", status);
    }
}

void app_le_server_terminate(uint8_t reason)
{
    uint16_t status = acc_tbs_call_terminate(p_env->bearer_info.bearer_lid, p_env->bearer_info.call_id, reason);
    if (status)
    {
        APP_DBG("acc_tbs_set_long err 0x%x\n", status);
    }

    app_le_call_server_state_call(EVENT_CALL_TERMINATED, GAF_ERR_NO_ERROR);
}

void app_le_server_remote_alert_start(void)
{
    app_le_call_server_state_call(EVENT_CALL_REMOTE_ALERT_STARTED, GAF_ERR_NO_ERROR);
}

void app_le_server_answer(void)
{
    app_le_call_server_state_call(EVENT_CALL_ANSWERED, GAF_ERR_NO_ERROR);
}

/*
 * STATE MACHINES
 ****************************************************************************************
 */
__STATIC void app_le_call_server_state_call(uint8_t event, uint16_t status)
{
    APP_DBG("Call event - %d\n", event);

    switch (event)
    {
        case EVENT_CALL_INCOMING:
        {
            app_le_call_server_event_start_req_t event_params = {
                .ccid = CALL_BEARER_CCID,
                .type = APP_LE_CALL_SERVER_TYPE_RINGTONE,
            };

            status = acc_tbs_call_incoming(0, strlen(p_env->bearer_info.p_uri), 0,
                                                   strlen(p_env->bearer_info.p_friendly_name),
                                                   (const uint8_t*)(p_env->bearer_info.p_uri), NULL,
                                                   (const uint8_t*)(p_env->bearer_info.p_friendly_name),
                                                   &(p_env->bearer_info.call_id));
            if (status)
            {
                APP_DBG("acc_tbs_call_incoming: err 0x%x\n", status);
            }

            // Require start of Ringtone stream
            p_env->cb_mediator_event(APP_LE_CALL_SERVER_EVENT_START_REQ, GAF_ERR_NO_ERROR, &event_params);
        } break;

        case EVENT_CALL_OUTGOING:
        {
            status = acc_tbs_call_outgoing(0, strlen(p_env->bearer_info.p_uri),
                                                         strlen(p_env->bearer_info.p_friendly_name),
                                                         (const uint8_t*)(p_env->bearer_info.p_uri),
                                                         (const uint8_t*)(p_env->bearer_info.p_friendly_name),
                                                         &(p_env->bearer_info.call_id));
            if (status)
            {
                APP_DBG("acc_tbs_call_outgoing: err 0x%x\n", status);
                break;
            }

            status = acc_tbs_call_remote_alert_started(0, p_env->bearer_info.call_id);
            if (status)
            {
                APP_DBG("acc_tbs_call_remote_alert_started: err 0x%x\n", status);
            }
        } break;

        case EVENT_CALL_ACCEPTED:
        {
            SETB(p_env->status_bf, STATUS_START_CALL, 1);
            SETB(p_env->status_bf, STATUS_CALL_STARTED, 1);
        }
        // no break

        case EVENT_CALL_REJECTED:
        {
            app_le_call_server_event_stop_req_t event_params = {
                .type = APP_LE_CALL_SERVER_TYPE_RINGTONE,
            };

            // Require stop of Ringtone stream
            p_env->cb_mediator_event(APP_LE_CALL_SERVER_EVENT_STOP_REQ, GAF_ERR_NO_ERROR, &event_params);
        } break;

        case EVENT_CALL_REMOTE_ALERT_STARTED:
        {
            status = acc_tbs_call_remote_alert_started(p_env->bearer_info.bearer_lid, p_env->bearer_info.call_id);
            if (status)
            {
                APP_DBG("acc_tbs_call_remote_alert_started: err 0x%x\n", status);
            }
        } break;

        case EVENT_CALL_ANSWERED:
        {
            app_le_call_server_event_start_req_t event_params = {
                .ccid = CALL_BEARER_CCID,
                .type = APP_LE_CALL_SERVER_TYPE_CALL,
            };

            status = (acc_tbs_call_answer(0, p_env->bearer_info.call_id));
            if (status)
            {
                APP_DBG("acc_tbs_call_answer: err 0x%x\n", status);
            }
            SETB(p_env->status_bf, STATUS_CALL_STARTED, 1);
            p_env->cb_mediator_event(APP_LE_CALL_SERVER_EVENT_START_REQ, GAF_ERR_NO_ERROR, &event_params);
        } break;

        case EVENT_CALL_TERMINATED:
        {
            app_le_call_server_event_stop_req_t event_params = {
                .type = APP_LE_CALL_SERVER_TYPE_CALL,
            };

            SETB(p_env->status_bf, STATUS_CALL_STARTED, 0);
            p_env->cb_mediator_event(APP_LE_CALL_SERVER_EVENT_STOP_REQ, GAF_ERR_NO_ERROR, &event_params);
        } break;

        case EVENT_CALL_RINGTONE_STOPPED:
        {
            if (CHECKB(p_env->status_bf, STATUS_START_CALL))
            {
                app_le_call_server_event_start_req_t event_params = {
                    .ccid = CALL_BEARER_CCID,
                    .type = APP_LE_CALL_SERVER_TYPE_CALL,
                };

                SETB(p_env->status_bf, STATUS_START_CALL, 0);
                p_env->cb_mediator_event(APP_LE_CALL_SERVER_EVENT_START_REQ, GAF_ERR_NO_ERROR, &event_params);
            }
        } break;

        default: { ASSERT_INFO(0, event, 0); } break;
    }
}

/*
 * CALLBACK FUNCTIONS
 ****************************************************************************************
 */

__STATIC void app_le_call_server_cb_bond_data_prov(uint16_t component_id, uint16_t type, uint8_t conidx, bool found,
                                                   uint16_t length, const void* p_data)
{
    uint16_t status;
    APP_DBG("app_le_call_server_cb_bond_data_prov\n");
    if (found)
    {
        do
        {
            const app_le_call_server_bond_data_t* p_bond_data = (const app_le_call_server_bond_data_t*)p_data;

            status = acc_tbs_restore_bond_data(0, conidx,
                                                    p_bond_data->signal_strength_intv_s,
                                                    p_bond_data->cli_cfg_bf, 0, 0, 0, 0, 0,
                                                    NULL, NULL, NULL, NULL);
            if (status)
            {
                APP_DBG("acc_tbs_restore_bond_data: err 0x%x\n", status);
                break;
            }
        } while (false);
    }
}

__STATIC void app_le_call_server_cb_bond_data(uint8_t bearer_lid, uint8_t conidx, uint16_t cli_cfg_bf)
{
    APP_DBG("app_le_call_server_cb_bond_data\n");
    app_le_call_server_bearer_t* p_bearer_info
                                        = (app_le_call_server_bearer_t*)&(p_env->bearer_info);

    p_bearer_info->bond_data.cli_cfg_bf = cli_cfg_bf;
}

__STATIC void app_le_call_server_cb_report_intv(uint8_t bearer_lid, uint8_t conidx, uint8_t sign_strength_intv_s)
{
    app_le_call_server_bearer_t* p_bearer_info
                                        = (app_le_call_server_bearer_t*)&(p_env->bearer_info);

    APP_DBG("Set signal strength report interval: %ds\n", sign_strength_intv_s);

    p_bearer_info->bond_data.signal_strength_intv_s = sign_strength_intv_s;
}

__STATIC void app_le_call_server_cb_get_req(uint8_t bearer_lid, uint8_t call_id, uint8_t conidx,
                                            uint8_t char_type, uint16_t token, uint16_t offset, uint16_t length)
{
    const char* p_val = NULL;
    app_le_call_server_bearer_t* p_bearer_info
                                        = (app_le_call_server_bearer_t*)&(p_env->bearer_info);

    APP_DBG("Get request (char_type = %d)\n", char_type);

    switch (char_type)
    {
        case ACC_TB_CHAR_TYPE_PROV_NAME: { p_val = p_bearer_info->p_provider_name; } break;
        case ACC_TB_CHAR_TYPE_CALL_FRIENDLY_NAME: { p_val = p_bearer_info->p_friendly_name; } break;
        case ACC_TB_CHAR_TYPE_URI_SCHEMES_LIST: { p_val = app_le_call_server_uri_schemes_supported; } break;
        default: { } break;
    }

    if (p_val != NULL)
    {
        length = strlen(p_val);
    }
    else
    {
        length = 0;
    }

    acc_tbs_cfm_get(true, bearer_lid, call_id, conidx, char_type, token, offset, length, (const uint8_t*)p_val);
}

__STATIC void app_le_call_server_cb_call_req(uint8_t bearer_lid, uint8_t conidx, uint8_t opcode,
                                             uint8_t call_id, uint8_t len, const uint8_t* p_val)
{
    switch (opcode)
    {
        case ACC_TB_OPCODE_ACCEPT:
        {
            APP_DBG("Call accepted\n");

            app_le_call_server_state_call(EVENT_CALL_ACCEPTED, GAF_ERR_NO_ERROR);
        } break;

        case ACC_TB_OPCODE_TERMINATE:
        {
            APP_DBG("Terminate call received\n");

            if (!CHECKB(p_env->status_bf, STATUS_CALL_STARTED))
            {
                app_le_call_server_state_call(EVENT_CALL_REJECTED, GAF_ERR_NO_ERROR);
            }
            else
            {
                app_le_call_server_state_call(EVENT_CALL_TERMINATED, GAF_ERR_NO_ERROR);
            }
        } break;

        default: { ASSERT_INFO(0, bearer_lid, opcode); } break;
    }

    acc_tbs_cfm_call(ACC_TB_CP_NTF_RESULT_SUCCESS, bearer_lid, 0, NULL);
}

// Set of callback functions for communication with Audio Content Control - Call Control Server module
__STATIC const acc_tbs_cb_t cbs_acc_tbs = {
    .cb_bond_data = app_le_call_server_cb_bond_data,
    .cb_report_intv = app_le_call_server_cb_report_intv,
    .cb_get_req = app_le_call_server_cb_get_req,
    .cb_call_req = app_le_call_server_cb_call_req,
};

/*
 * FUNCTIONS
 ****************************************************************************************
 */

void app_le_call_server_configure(const void* p_params, component_cb_event cb_event)
{
    uint8_t bearer_lid;

    ASSERT_ERR(p_env == NULL);
    ASSERT_ERR(cb_event != NULL);
    uint16_t status;

    p_env = (app_le_call_server_env_t*)ke_malloc_user(sizeof(app_le_call_server_env_t), KE_MEM_PROFILE);
    ASSERT_ERR(p_env != NULL);
    memset(p_env, 0, sizeof(app_le_call_server_env_t));
    p_env->cb_mediator_event = cb_event;

    do
    {
        status = acc_tbs_configure(1, 2, 20, 20, &cbs_acc_tbs, 0);
        if (status)
        {
            APP_DBG("acc_tbs_configure: err 0x%x\n", status);
            break;
        }

        status = acc_tbs_add(CALL_BEARER_CFG_BF, 0, CALL_BEARER_CCID, 0, strlen(app_le_call_server_uci),
                                     (const uint8_t*)app_le_call_server_uci, &bearer_lid);
        if (status)
        {
            APP_DBG("acc_tbs_add: err 0x%x\n", status);
        }
        else
        {
            p_env->bearer_info.bearer_lid = bearer_lid;
        }
    } while (false);
}

void app_le_call_server_stopped(void)
{
    app_le_call_server_state_call(EVENT_CALL_RINGTONE_STOPPED, GAP_ERR_NO_ERROR);
}

/// \example{lineno} app_le_call_server.c
#endif