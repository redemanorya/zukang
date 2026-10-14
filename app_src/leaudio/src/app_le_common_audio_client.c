/**
 ****************************************************************************************
 *
 * @file app_le_common_audio_client.c
 *
 * @brief Applet - Common Audio Client
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
#include "debug.h"
#include "app_le_common_audio_client.h"
#include "cap_cac.h"
#include "le_cis_src.h"
/*
 * ENUMERATIONS
 ****************************************************************************************
 */
#define COMPONENT_EVENT_SERVICE_CONTENT_DISCOVERED   (8)

// Event values for Service Discovery state machine
enum app_le_common_audio_client_event_service_discovery
{
    // State machine start has been required
    EVENT_SERVICE_DISCOVERY_START = 0U,
    // Included Coordinated Set Identification service has been discovered
    EVENT_SERVICE_DISCOVERY_INCLUDED_CSIS_FOUND,
    // Content of Common Audio Service has been discovered
    EVENT_SERVICE_CONTENT_DISCOVERED,
};

/*
 * TYPES DEFINITION
 ****************************************************************************************
 */
typedef struct app_event_service_discovered
{
    /// Connection index
    uint8_t conidx;
    /// UUID
    uint16_t uuid;
    /// Service information
    prf_svc_t svc;
} app_event_service_discovered_t;

// Environment structure
typedef struct app_le_common_audio_client_env
{
    // // Registration structure for events
    // utils_control_intf_t control_intf;
    // #if (APP_DEBUG)
    // // Pointer to Output Provider interface
    // const output_prov_intf_t* p_output_intf;
    // #endif //(APP_DEBUG)
    // // Pointer to Bond Data Provider interface
    // const bond_data_prov_intf_t* p_bond_data_intf;
    // Event callback function for backward communication with Mediator
    component_cb_event cb_mediator_event;
} app_le_common_audio_client_env_t;

/*
 * LOCAL VARIABLES
 ****************************************************************************************
 */

// Pointer to environment
__STATIC app_le_common_audio_client_env_t* p_env;
extern void app_le_audio_discovery(uint8_t event, uint8_t conidx, uint16_t status, const void* p_params);
/*
 * STATE MACHINES
 ****************************************************************************************
 */

void app_le_common_audio_client_transition_service_discovery(uint8_t event, uint8_t conidx, uint16_t status,
                                                                      const prf_svc_t* p_svc)
{
    switch (event)
    {
        case EVENT_SERVICE_DISCOVERY_START:
        {
            APP_DBG("Start CAS discovery (conidx = %d)\n", conidx);

            if (p_svc != NULL)
            {
                status = cap_cac_discover(conidx, p_svc->shdl, p_svc->ehdl);
                if (status)
                {
                    APP_DBG("cap_cac_discover: err 0x%x\n", status);
                }
            }
            else
            {
                status = cap_cac_discover(conidx, GATT_MIN_HDL, GATT_MAX_HDL);
                if (status)
                {
                    APP_DBG("cap_cac_discover2: err 0x%x\n", status);
                }
            }
        } break;

        case EVENT_SERVICE_DISCOVERY_INCLUDED_CSIS_FOUND:
        {
            app_event_service_discovered_t event_params = {
                .conidx = conidx,
                .uuid = GATT_SVC_COORD_SET_IDENTIFICATION,
                .svc = *p_svc,
            };

            APP_DBG("Included CSIS found (conidx = %d)\n", conidx);
            app_le_audio_discovery(1, conidx, GAF_ERR_NO_ERROR, &event_params);
        } break;

        case EVENT_SERVICE_CONTENT_DISCOVERED:
        {
            if (status != GAF_ERR_NO_ERROR) break;

            APP_DBG("CAS discovered (conidx = %d)\n", conidx);

            p_env->cb_mediator_event(COMPONENT_EVENT_SERVICE_CONTENT_DISCOVERED, GAF_ERR_NO_ERROR, &conidx);
        } break;

        default: { } break;
    }
}

/*
 * CALLBACK FUNCTIONS
 ****************************************************************************************
 */

// __STATIC void app_le_common_audio_client_cb_bond_data_prov(uint16_t component_id, uint16_t type, uint8_t conidx,
//                                                            bool found, uint16_t length, const void* p_data)
// {
//     uint16_t status;
//     if (found)
//     {
//         status = cap_cac_restore_bond_data(conidx, (const cap_cac_cas_t*)p_data);
//         if (status)
//         {
//             APP_DBG("cap_cac_restore_bond_data: err 0x%x\n", status);
//         }
//     }
// }

__STATIC void app_le_common_audio_client_cb_cmp_evt(uint8_t cmd_type, uint16_t status, uint8_t conidx)
{
    ASSERT_WARN(status == GAF_ERR_NO_ERROR, status, cmd_type);

    switch (cmd_type)
    {
        case CAP_CAC_CMD_TYPE_DISCOVER:
        {
            app_le_common_audio_client_transition_service_discovery(EVENT_SERVICE_CONTENT_DISCOVERED, conidx,
                                                                    status, NULL);
        } break;

        default: { } break;
    }
}

__STATIC void app_le_common_audio_client_cb_bond_data(uint8_t conidx, const cap_cac_cas_t* p_cas_info,
                                                      uint16_t csis_shdl, uint16_t csis_ehdl)
{
    APP_DBG("app_le_common_audio_client_cb_bond_data\n");
    if (csis_shdl != GATT_INVALID_HDL)
    {
        prf_svc_t svc = {
            .shdl = csis_shdl,
            .ehdl = csis_ehdl,
        };

        app_le_common_audio_client_transition_service_discovery(EVENT_SERVICE_DISCOVERY_INCLUDED_CSIS_FOUND, conidx,
                                                                GAF_ERR_NO_ERROR, &svc);
    }
}

__STATIC void app_le_common_audio_client_cb_svc_changed(uint8_t conidx)
{
}

// Set of callback functions for Common Audio Client module
__STATIC const cap_cac_cb_t cbs_cap_cac = {
    .cb_cmp_evt = app_le_common_audio_client_cb_cmp_evt,
    .cb_bond_data = app_le_common_audio_client_cb_bond_data,
    .cb_svc_changed = app_le_common_audio_client_cb_svc_changed,
};

/*
 * FUNCTIONS
 ****************************************************************************************
 */

void app_le_common_audio_client_configure(const void* p_params, component_cb_event cb_event)
{
    ASSERT_ERR(p_env == NULL);
    ASSERT_ERR(cb_event != NULL);
    uint16_t status;

    p_env = (app_le_common_audio_client_env_t*)ke_malloc_user(sizeof(app_le_common_audio_client_env_t),
                                                              KE_MEM_PROFILE);
    ASSERT_ERR(p_env != NULL);
    memset(p_env, 0, sizeof(app_le_common_audio_client_env_t));
    p_env->cb_mediator_event = cb_event;

    status = cap_cac_configure(&cbs_cap_cac);
    if (status)
    {
        APP_DBG("cap_cac_configure: err 0x%x\n", status);
    }
}

/// \example{lineno} app_le_common_audio_client.c
#endif