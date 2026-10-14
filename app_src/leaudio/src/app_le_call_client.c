/**
 ****************************************************************************************
 *
 * @file app_le_call_client.c
 *
 * @brief Applet - Call Client
 *
 * Copyright (C) RivieraWaves 2009-2024
 * Release Identifier: c73db0ce
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
#include "app_le_call_client.h"
#include "acc_tbc.h"
#include "co_buf.h"
#include "debug.h"
#include "arch.h"

/*
 * ENUMERATIONS
 ****************************************************************************************
 */
#define COMPONENT_EVENT_SERVICE_CONTENT_DISCOVERED   (8)

// Event values for Service Discovery state machine
enum app_le_call_client_event_service_discovery
{
    // Start Discovery state machine
    EVENT_SERVICE_DISCOVERY_START = 0U,
    // Discovery of Telephone Bearer Service instances has been completed
    EVENT_SERVICE_CONTENT_DISCOVERED,
};

/*
 * TYPES DEFINITION
 ****************************************************************************************
 */

// Bond data structure
typedef struct app_le_call_client_bond_data
{
    // (Generic) Telephone Bearer Control Service content
    acc_tbc_tbs_info_t tbs;
} app_le_call_client_bond_data_t;

// Environment structure
typedef struct app_le_call_client_env
{
    // Event callback function for backward communication with Mediator
    component_cb_event cb_mediator_event;
    // Pointer to buffer allocated during discovery for storage of bond data
    co_buf_t* p_buf;
    // Bearer state
    uint8_t bearer_state;
    // Incoming call identifier
    uint8_t call_id;
    // Connection Index
    uint8_t conidx;
    // Telephone bearer lid
	uint8_t bearer_lid;
} app_le_call_client_env_t;

/*
 * LOCAL VARIABLES
 ****************************************************************************************
 */

// Pointer to environment
__STATIC app_le_call_client_env_t* p_env;
extern void app_le_audio_discovery(uint8_t event, uint8_t conidx, uint16_t status, const void* p_params);

/*
 * STATE MACHINES
 ****************************************************************************************
 */

void app_le_call_client_state_service_discovery(uint8_t event, uint16_t status, uint8_t conidx)
{
    // app_le_call_client_bond_data_t* p_bond_data;

    switch (event)
    {
        case EVENT_SERVICE_DISCOVERY_START:
        {
            APP_DBG("Start TBS discovery (conidx = %d)\n", conidx);

            // if ((p_env->p_bond_data_intf != NULL) && utils_is_paired(conidx) && (p_env->p_buf == NULL))
            {
                status = co_buf_alloc(&(p_env->p_buf), 0, sizeof(app_le_call_client_bond_data_t), 0);
                if (status)
                {
                    APP_DBG("co_buf_alloc err 0x%x\n", status);
                    return;
                }
                // p_bond_data = (app_le_call_client_bond_data_t*)co_buf_data(p_env->p_buf);

            }

            status = acc_tbc_discover(conidx, 1, 0, 0, 0);
            if (status)
            {
                APP_DBG("acc_tbc_discover: err 0x%x\n", status);
            }
        } break;

        case EVENT_SERVICE_CONTENT_DISCOVERED:
        {
            if (status == GAF_ERR_NO_ERROR)
            {
                if (p_env->p_buf != NULL)
                {
                    // p_bond_data = (app_le_call_client_bond_data_t*)co_buf_data(p_env->p_buf);

                    APP_DBG("TBS discovered (conidx = %d)\n", conidx);
                }
            }
            else
            {
                APP_DBG("Error during TBS discovery (conidx = %d, status = 0x%02X)\n", conidx, status);
                break;
            }

            if (p_env->p_buf != NULL)
            {
                co_buf_release(p_env->p_buf);
                p_env->p_buf = NULL;
            }

            if (p_env->cb_mediator_event != NULL)
            {
                p_env->cb_mediator_event(COMPONENT_EVENT_SERVICE_CONTENT_DISCOVERED, status, &conidx);
            }
            app_le_audio_discovery(3, conidx, GAF_ERR_NO_ERROR, NULL);
        } break;

        default:
        {
            ASSERT_INFO(0, event, 0);
        } break;
    }
}

/*
 * CALLBACK FUNCTIONS
 ****************************************************************************************
 */

void app_le_client_call_accept(void)
{
    uint16_t status = acc_tbc_call_accept(p_env->conidx, p_env->bearer_lid, false, p_env->call_id);
    if (status)
    {
        APP_DBG("acc_tbc_call_accept: err 0x%x\n", status);
    }
}

void app_le_client_call_terminate(void)
{
    uint16_t status = acc_tbc_call_terminate(p_env->conidx, p_env->bearer_lid, false, p_env->call_id);
    if (status)
    {
        APP_DBG("acc_tbc_call_terminate: err 0x%x\n", status);
    }
}

// __STATIC void app_le_call_client_cb_bond_data_prov(uint16_t component_id, uint16_t type, uint8_t conidx, bool found,
//                                                    uint16_t length, const void* p_data)
// {
//     uint16_t status;
//     if (found)
//     {
//         const app_le_call_client_bond_data_t* p_bond_data = (const app_le_call_client_bond_data_t*)p_data;
//         status = acc_tbc_restore_bond_data(conidx, 1, &(p_bond_data->tbs));
//         if (status)
//         {
//             APP_DBG("acc_tbc_restore_bond_data err 0x%x\n", status);
//         }
//     }
// }

__STATIC void app_le_call_client_cb_cmp_evt(uint8_t cmd_type, uint16_t status, uint8_t conidx,
                                            uint8_t bearer_lid, uint8_t param, uint8_t call_id, uint8_t result)
{
    APP_DBG("app_le_call_client_cb_cmp_evt: cmd_type 0x%x, call_id 0x%x, bearer_lid 0x%x\n", 
                cmd_type, call_id, bearer_lid);
    switch (cmd_type)
    {
        case ACC_TBC_CMD_TYPE_DISCOVER:
        {
            p_env->conidx = conidx;
            app_le_call_client_state_service_discovery(EVENT_SERVICE_CONTENT_DISCOVERED, status, conidx);
        } break;

        default:
        {
            ASSERT_WARN(status == GAF_ERR_NO_ERROR, status, cmd_type);
        } break;
    }
}

__STATIC void app_le_call_client_cb_call_state(uint8_t conidx, uint8_t bearer_lid, uint8_t call_id,
                                               uint8_t flags, uint8_t state)
{
    APP_DBG("app_le_call_client_cb_call_state: bearer_lid %d, call_id: %d, state 0x%x\n", 
                bearer_lid, call_id, state);

    if (call_id != 0)
    {
        p_env->bearer_state = state;
    }
    else
    {
        APP_DBG("No call (conidx = %d)\n", conidx);
    }
}

__STATIC void app_le_call_client_cb_call_state_long(uint8_t conidx, uint8_t bearer_lid, uint8_t call_id,
                                                    uint8_t flags, uint8_t state, uint8_t uri_len,
                                                    const uint8_t* p_uri)
{
    APP_DBG("app_le_call_client_cb_call_state_long: bearer_lid %d, call_id: %d, state 0x%x\n", 
                bearer_lid, call_id, state);
    app_le_call_client_cb_call_state(conidx, bearer_lid, call_id, flags, state);

    switch (state)
    {
        case ACC_TB_CALL_STATE_INCOMING:
        {

        } break;

        default: { } break;
    }
}

__STATIC void app_le_call_client_cb_value_long(uint8_t conidx, uint8_t bearer_lid, uint8_t call_id,
                                               uint8_t char_type, uint16_t val_len, const char* p_val)
{
    APP_DBG("app_le_call_client_cb_value_long: bearer_lid %d, call_id: %d, char_type %d\n", 
                bearer_lid, call_id, char_type);

    p_env->call_id = call_id;
    switch (char_type)
    {
        case ACC_TB_CHAR_TYPE_PROV_NAME:
        {

        } break;

        default: { } break;
    }
}

__STATIC void app_le_call_client_cb_value(uint8_t conidx, uint8_t bearer_lid, uint8_t call_id,
                                          uint8_t char_type, uint16_t val)
{
    APP_DBG("app_le_call_client_cb_value: bearer_lid %d, call_id: %d, char_type %d\n", 
                bearer_lid, call_id, char_type);

    switch (char_type)
    {
        case ACC_TB_CHAR_TYPE_TECHNO:
        {

        } break;

        case ACC_TB_CHAR_TYPE_SIGN_STRENGTH:
        {

        } break;

        case ACC_TB_CHAR_TYPE_TERM_REASON:
        {
            p_env->bearer_state = ACC_TB_CALL_STATE_MAX;
        } break;

        default: { } break;
    }
}

__STATIC void app_le_call_client_cb_bond_data(uint8_t conidx, uint8_t bearer_lid, uint16_t uuid,
                                              const acc_tbc_tbs_info_t* p_tbs_info)
{
    APP_DBG("app_le_call_client_cb_bond_data, bearer_lid 0x%x\n", bearer_lid);
    if (p_env->p_buf != NULL)
    {
        app_le_call_client_bond_data_t* p_bond_data = (app_le_call_client_bond_data_t*)co_buf_data(p_env->p_buf);

        memcpy(&(p_bond_data->tbs), p_tbs_info, sizeof(acc_tbc_tbs_info_t));
    }
    p_env->bearer_lid = bearer_lid;
}

__STATIC void app_le_call_client_cb_svc_changed(uint8_t conidx)
{
}

// Set of callback functions for communication with Audio Content Control - Telephone Bearer Client module
__STATIC const acc_tbc_cb_t cbs_acc_tbc = {
    .cb_cmp_evt = app_le_call_client_cb_cmp_evt,
    .cb_call_state = app_le_call_client_cb_call_state,
    .cb_call_state_long = app_le_call_client_cb_call_state_long,
    .cb_value_long = app_le_call_client_cb_value_long,
    .cb_value = app_le_call_client_cb_value,
    .cb_bond_data = app_le_call_client_cb_bond_data,
    .cb_svc_changed = app_le_call_client_cb_svc_changed,
};

/*
 * FUNCTIONS
 ****************************************************************************************
 */

void app_le_call_client_configure(const void* p_params, component_cb_event cb_event)
{
    ASSERT_ERR(p_env == NULL);
    uint16_t status;

    p_env = (app_le_call_client_env_t*)ke_malloc_user(sizeof(app_le_call_client_env_t), KE_MEM_PROFILE);
    ASSERT_ERR(p_env != NULL);
    memset(p_env, 0, sizeof(app_le_call_client_env_t));
    p_env->cb_mediator_event = cb_event;

    // p_env->control_intf.control = app_le_call_client_mediator_intf.control;
    // p_env->control_intf.event_bf = CO_BIT(COMPONENT_EVENT_CONNECTED);
    // utils_register_event(&(p_env->control_intf));

    status = acc_tbc_configure(&cbs_acc_tbc, 0);
    if (status)
    {
        APP_DBG("acc_tbc_configure err 0x%x\n", status);
    }
}


/// \example{lineno} app_le_call_client.c
#endif
