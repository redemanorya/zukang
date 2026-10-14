/**
 ****************************************************************************************
 *
 * @file app_le_coordinated_set_coordinator.c
 *
 * @brief Applet - Coordinated Set Coordinator
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
#include "app_le_coordinated_set_coordinator.h"
#include "atc_csisc.h"
#include "co_list.h"
#include "utils_audio.h"
#include "hl_error.h"
#include "debug.h"
/*
 * ENUMERATIONS
 ****************************************************************************************
 */

// Event values for Service Discovery state machine
enum app_le_coordinated_set_coordinator_event_service_discovery
{
    // Start Discovery state machine
    EVENT_SERVICE_DISCOVERY_START = 0U,
    // Coordinated Set Identification Service has been discovered
    EVENT_SERVICE_DISCOVERY_CSIS_DISCOVERED,
};

// Event values for RSI Resolution state machine
enum app_le_coordinated_set_coordinator_event_rsi_resolution
{
    // Start Resolution state machine
    EVENT_RESOLUTION_START = 0U,
    // RSI has been resolved
    EVENT_RESOLUTION_DONE,
    // Stop Resolution
    EVENT_RESOLUTION_STOP,
};

// Status bit field meaning
enum app_le_coordinated_set_coordinator_status_bf
{
    // RSI is being resolved - Position
    STATUS_RSI_RESOLVING_POS = 0U,
    // RSI is being resolved - Bit
    STATUS_RSI_RESOLVING_BIT = CO_BIT(STATUS_RSI_RESOLVING_POS),

    // Wrong SIRK has been read - Position
    STATUS_WRONG_SIRK_POS = 1U,
    // Wrong SIRK has been read - Bit
    STATUS_WRONG_SIRK_BIT = CO_BIT(STATUS_WRONG_SIRK_POS),

    // Stop has been required - Position
    STATUS_STOP_POS = 2U,
    // Stop has been required - Bit
    STATUS_STOP_BIT = CO_BIT(STATUS_STOP_POS),
};

/*
 * TYPES DEFINITION
 ****************************************************************************************
 */

// List structure containing RSI to resolved
typedef struct app_le_coordinated_set_coordinator_rsi
{
    // List header
    co_list_hdr_t hdr;
    // RSI to resolve
    csis_rsi_t rsi;
    // Device address
    gap_bdaddr_t addr;
} app_le_coordinated_set_coordinator_rsi_t;

// Environment structure
typedef struct app_le_coordinated_set_coordinator_env
{
    // Callback function for backward communication with Mediator
    component_cb_event cb_mediator_event;
    // List of RSI to resolve\n
    // See #app_le_coordinated_set_coordinator_rsi_t
    co_list_t list_rsi;
    // List of available RSI structures
    // See ##app_le_coordinated_set_coordinator_rsi_t
    co_list_t list_rsi_available;
    // Pointer to expected SIRK
    const csis_sirk_t* p_expected_sirk;
    // Key local index
    uint8_t key_lid;
    // Status bit field (see #app_le_coordinated_set_coordinator_status_bf enumeration)
    uint8_t status_bf;
    // Array of RSI structures
    app_le_coordinated_set_coordinator_rsi_t rsi[__ARRAY_EMPTY];
} app_le_coordinated_set_coordinator_env_t;

/*
 * LOCAL VARIABLES
 ****************************************************************************************
 */
#define COMPONENT_EVENT_SERVICE_CONTENT_DISCOVERED    (8)

// Pointer to environment
__STATIC app_le_coordinated_set_coordinator_env_t* p_env;

/*
 * STATE MACHINES
 ****************************************************************************************
 */

void app_le_coordinated_set_coordinator_state_service_discovery(uint8_t event, uint16_t status,
                                                                         const void* p_params)
{
    switch (event)
    {
        case EVENT_SERVICE_DISCOVERY_START:
        {
            const app_le_coordinated_set_coordinator_start_discovery_t* p_params_start
                                            = (const app_le_coordinated_set_coordinator_start_discovery_t*)p_params;
            const prf_svc_t* p_svc = p_params_start->basic.p_svc;

            APP_DBG("Start CSIS discovery (conidx = %d)\n", p_params_start->basic.conidx);

            if (p_svc != NULL)
            {
                status = atc_csisc_discover(p_params_start->basic.conidx, 1, p_svc->shdl, p_svc->ehdl);
                if (status)
                {
                    APP_DBG("atc_csisc_discover err 0x%x\n", status);  
                    break;     
                }
            }
            else
            {
                status = atc_csisc_discover(p_params_start->basic.conidx,
                                                          GATT_MIN_HDL, GATT_MAX_HDL, GATT_INVALID_HDL);
                if (status)
                {
                    APP_DBG("atc_csisc_discover2 err 0x%x\n", status);  
                    break;     
                }
            }

            p_env->p_expected_sirk = p_params_start->p_sirk;
            SETB(p_env->status_bf, STATUS_WRONG_SIRK, 0);
        } break;

        case EVENT_SERVICE_DISCOVERY_CSIS_DISCOVERED:
        {
            uint8_t conidx = *((const uint8_t*)p_params);

            if (CHECKB(p_env->status_bf, STATUS_WRONG_SIRK))
            {
                APP_DBG("Wrong SIRK received\n");
                status = GAF_ERR_INVALID_PARAM;
            }

            if (status == GAF_ERR_NO_ERROR)
            {
                APP_DBG("CSIS discovered (conidx = %d)\n", conidx);
            }
            else
            {
                utils_audio_le_cfg_t* p_le_audio_cfg = utils_audio_le_get_cfg(conidx);

                APP_DBG("Error during CSIS discovery (status = 0x%02X)\n", status);

                p_le_audio_cfg->nb_set_members = 0;
            }

            p_env->cb_mediator_event(COMPONENT_EVENT_SERVICE_CONTENT_DISCOVERED, status, &conidx);
        } break;

        default: { } break;
    }
}

__STATIC void app_le_coordinated_set_coordinator_state_resolve(uint8_t event, uint16_t status)
{
    app_le_coordinated_set_coordinator_rsi_t* p_rsi;

    APP_DBG("app_le_coordinated_set_coordinator_state_resolve: evt 0x%x\n", event);
    switch (event)
    {
        case EVENT_RESOLUTION_START:
        {
            if (CHECKB(p_env->status_bf, STATUS_RSI_RESOLVING)) { break; }

            p_rsi = (app_le_coordinated_set_coordinator_rsi_t*)co_list_pick(&(p_env->list_rsi));
            while (p_rsi != NULL)
            {
                uint16_t status_resolve = atc_csisc_resolve(&(p_rsi->rsi));

                if (status_resolve == GAP_ERR_NO_ERROR)
                {
                    SETB(p_env->status_bf, STATUS_RSI_RESOLVING, 1U);
                    break;
                }
                else
                {
                    app_le_coordinated_set_coordinator_resolved_t params_resolved = {
                        .p_addr = &(p_rsi->addr),
                    };

                    p_env->cb_mediator_event(APP_LE_COORDINATED_SET_COORDINATOR_EVENT_RESOLVED, status_resolve,
                                             &params_resolved);

                    co_list_pop_front(&(p_env->list_rsi));
                    co_list_push_back(&(p_env->list_rsi_available), &(p_rsi->hdr));
                    p_rsi = (app_le_coordinated_set_coordinator_rsi_t*)co_list_pick(&(p_env->list_rsi));
                }
            }
        } break;

        case EVENT_RESOLUTION_DONE:
        {
            if (!CHECKB(p_env->status_bf, STATUS_STOP))
            {
                app_le_coordinated_set_coordinator_resolved_t params_resolved;

                p_rsi = (app_le_coordinated_set_coordinator_rsi_t*)co_list_pop_front(&(p_env->list_rsi));
                ASSERT_ERR(p_rsi != NULL);

                params_resolved.p_addr = &(p_rsi->addr);
                SETB(p_env->status_bf, STATUS_RSI_RESOLVING, 0U);

                co_list_push_back(&(p_env->list_rsi_available), &(p_rsi->hdr));
                p_env->cb_mediator_event(APP_LE_COORDINATED_SET_COORDINATOR_EVENT_RESOLVED, status, &params_resolved);

                // Recall function - Should be done only once
                app_le_coordinated_set_coordinator_state_resolve(EVENT_RESOLUTION_START, GAP_ERR_NO_ERROR);

                break;
            }
        }
        // no break;

        case EVENT_RESOLUTION_STOP:
        {
            while (co_list_pick(&(p_env->list_rsi)))
            {
                app_le_coordinated_set_coordinator_rsi_t* p_rsi
                            = (app_le_coordinated_set_coordinator_rsi_t*)co_list_pop_front(&(p_env->list_rsi));

                co_list_push_back(&(p_env->list_rsi_available), &(p_rsi->hdr));
            }
        } break;

        default: { } break;
    }
}

/*
 * CALLBACK FUNCTIONS
 ****************************************************************************************
 */

__STATIC void app_le_coordinated_set_coordinator_cb_bond_data_prov(uint16_t component_id, uint16_t type,
                                                                   uint8_t conidx, bool found, uint16_t length,
                                                                   const void* p_data)
{
    uint16_t status;
    if (found)
    {
        status = atc_csisc_restore_bond_data(conidx, 1, (const csisc_csis_info_t*)p_data);
        if (status)
        {
            APP_DBG("atc_csisc_restore_bond_data err 0x%x\n", status);
        }
    }
}

__STATIC void app_le_coordinated_set_coordinator_cb_cmp_evt(uint16_t cmd_code, uint16_t status, uint8_t lid,
                                                            uint8_t set_lid, uint8_t char_type)
{
    switch (cmd_code)
    {
        case CSISC_RESOLVE:
        {
            app_le_coordinated_set_coordinator_state_resolve(EVENT_RESOLUTION_DONE, status);
            app_le_coordinated_set_coordinator_state_resolve(EVENT_RESOLUTION_START, GAF_ERR_NO_ERROR);

        } break;

        case CSISC_DISCOVER:
        {
            app_le_coordinated_set_coordinator_state_service_discovery(EVENT_SERVICE_DISCOVERY_CSIS_DISCOVERED,
                                                                       status, &lid);
        } break;

        default: { } break;
    }
}

__STATIC void app_le_coordinated_set_coordinator_cb_bond_data(uint8_t conidx, uint8_t set_lid,
                                                              const csisc_csis_info_t* p_csis_info)
{
    APP_DBG("Bond data updated\n");
}

__STATIC void app_le_coordinated_set_coordinator_cb_sirk(uint8_t conidx, uint8_t set_lid, uint8_t key_lid,
                                                         const csis_sirk_t* p_sirk)
{
    APP_DBG("SIRK 0x%lx, 0x%lx, key_lid 0x%x, 0x%x\n", (uint32_t)p_sirk, (uint32_t)p_env->p_expected_sirk, key_lid, p_env->key_lid);
    uint16_t status;
    do
    {
        APP_DBG("SIRK received\n");

        if (p_env->p_expected_sirk != NULL)
        {
            if (memcmp(p_sirk, p_env->p_expected_sirk, sizeof(csis_sirk_t)))
            {
                SETB(p_env->status_bf, STATUS_WRONG_SIRK, 1);
                break;
            }
        }

        if (key_lid == GAF_INVALID_LID)
        {
            if (p_env->key_lid != GAF_INVALID_LID)
            {
                status = atc_csisc_remove_sirk(p_env->key_lid);
                if (status)
                {
                    APP_DBG("atc_csisc_remove_sirk: err 0x%x\n", status);
                }
            }

            status = atc_csisc_add_sirk(p_sirk, &(p_env->key_lid));
            if (status)
            {
                APP_DBG("atc_csisc_add_sirk: err 0x%x\n", status);
            }
        }
    } while (false);
}

__STATIC void app_le_coordinated_set_coordinator_cb_info(uint8_t conidx, uint8_t set_lid, uint8_t char_type,
                                                         uint8_t val)
{
    switch (char_type)
    {
        case CSIS_CHAR_TYPE_SIZE:
        {
            utils_audio_le_cfg_t* p_le_audio_cfg = utils_audio_le_get_cfg(conidx);

            APP_DBG("Set size: %d\n", val);

            p_le_audio_cfg->nb_set_members = val;
        } break;

        case CSIS_CHAR_TYPE_RANK:
        {
            utils_audio_le_cfg_t* p_le_audio_cfg = utils_audio_le_get_cfg(conidx);

            APP_DBG("Rank: %d\n", val);

            p_le_audio_cfg->set_rank = val;
        } break;

        default: { } break;
    }
}

__STATIC void app_le_coordinated_set_coordinator_cb_svc_changed(uint8_t conidx)
{
}

__STATIC void app_le_coordinated_set_coordinator_cb_ltk_req(uint8_t conidx, uint8_t set_lid)
{
    APP_DBG("LTK requested\n");
    atc_csisc_ltk_cfm(utils_get_ltk(conidx));
}

// Set of callback functions for communication with Audio Topology Control - Coordinated Set Identification Service
// Coordinator module
__STATIC const csisc_cb_t cbs_atc_csisc = {
    .cb_bond_data = app_le_coordinated_set_coordinator_cb_bond_data,
    .cb_sirk = app_le_coordinated_set_coordinator_cb_sirk,
    .cb_info = app_le_coordinated_set_coordinator_cb_info,
    .cb_cmp_evt = app_le_coordinated_set_coordinator_cb_cmp_evt,
    .cb_svc_changed = app_le_coordinated_set_coordinator_cb_svc_changed,
    .cb_ltk_req = app_le_coordinated_set_coordinator_cb_ltk_req,
};

/*
 * FUNCTIONS
 ****************************************************************************************
 */

void app_le_coordinated_set_coordinator_configure(const app_le_coordinated_set_coordinator_cfg_t* p_params,
                                                           component_cb_event cb_event)
{
    uint16_t size, status;
    uint8_t cnt;

    ASSERT_ERR(p_env == NULL);
    ASSERT_ERR((p_params != NULL) && (p_params->nb_rsi_max != 0));
    ASSERT_ERR(cb_event != NULL);

    size = sizeof(app_le_coordinated_set_coordinator_env_t)
                                    + (p_params->nb_rsi_max * sizeof(app_le_coordinated_set_coordinator_rsi_t));
    p_env = (app_le_coordinated_set_coordinator_env_t*)ke_malloc_user(size, KE_MEM_PROFILE);
    ASSERT_ERR(p_env != NULL);
    memset(p_env, 0, size);
    p_env->cb_mediator_event = cb_event;
    p_env->key_lid = GAF_INVALID_LID;

    for (cnt = 0; cnt < p_params->nb_rsi_max; cnt++)
    {
        co_list_push_back(&(p_env->list_rsi_available), &(p_env->rsi[cnt].hdr));
    }

    status = atc_csisc_configure(1, &cbs_atc_csisc);
    if (status)
    {
        APP_DBG("atc_csisc_configure: err 0x%x\n", status);
    }
}

__STATIC void app_le_coordinated_set_coordinator_control(uint16_t type, const void* p_params)
{
    uint16_t status;
    ASSERT_ERR(p_env != NULL);

    APP_DBG("app_le_coordinated_set_coordinator_control: type 0x%x\n", type);
    switch (type)
    {
        case APP_LE_COORDINATED_SET_COORDINATOR_CONTROL_STOP:
        {
            if (CHECKB(p_env->status_bf, STATUS_RSI_RESOLVING))
            {
                SETB(p_env->status_bf, STATUS_STOP, 1);
                break;
            }

            app_le_coordinated_set_coordinator_state_resolve(EVENT_RESOLUTION_STOP, GAF_ERR_NO_ERROR);
        } break;

        case APP_LE_COORDINATED_SET_COORDINATOR_CONTROL_RESOLVE:
        {
            app_le_coordinated_set_coordinator_rsi_t* p_rsi
                        = (app_le_coordinated_set_coordinator_rsi_t*)co_list_pop_front(&(p_env->list_rsi_available));

            SETB(p_env->status_bf, STATUS_STOP, 0);

            if (p_rsi != NULL)
            {
                app_le_coordinated_set_coordinator_resolve_t* p_params_resolve
                                                    = (app_le_coordinated_set_coordinator_resolve_t*)p_params;

                ASSERT_ERR(p_params != NULL);
                ASSERT_ERR((p_params_resolve->p_rsi != NULL) && (p_params_resolve->p_addr != NULL));

                memcpy(&(p_rsi->rsi), p_params_resolve->p_rsi, sizeof(csis_rsi_t));
                memcpy(&(p_rsi->addr), p_params_resolve->p_addr, sizeof(gap_bdaddr_t));
                co_list_push_back(&(p_env->list_rsi), &(p_rsi->hdr));

                app_le_coordinated_set_coordinator_state_resolve(EVENT_RESOLUTION_START, GAF_ERR_NO_ERROR);
            }
        } break;

        case APP_LE_COORDINATED_SET_COORDINATOR_CONTROL_ADD_SIRK:
        {
            app_le_coordinated_set_coordinator_add_sirk_t* p_params_add_sirk
                                                = (app_le_coordinated_set_coordinator_add_sirk_t*)p_params;

            ASSERT_ERR(p_params != NULL);
            ASSERT_ERR(p_params_add_sirk->p_sirk != NULL);

            status = atc_csisc_add_sirk(p_params_add_sirk->p_sirk, &(p_env->key_lid));
            if (status)
            {
                APP_DBG("atc_csisc_add_sirk: err 0x%x\n", status);
            }
        } break;

        default: { ASSERT_INFO(0, type, 0); } break;
    }
}

void app_le_coordinated_set_coor_rsi_reslove(const void* p_params)
{
    app_le_coordinated_set_coordinator_control(APP_LE_COORDINATED_SET_COORDINATOR_CONTROL_RESOLVE, p_params);
}

void app_le_coordinated_set_coor_stop(void)
{
    app_le_coordinated_set_coordinator_control(APP_LE_COORDINATED_SET_COORDINATOR_CONTROL_STOP, NULL);
}

void app_le_coordinated_set_coor_add_sirk(const void* p_params)
{
    app_le_coordinated_set_coordinator_control(APP_LE_COORDINATED_SET_COORDINATOR_CONTROL_ADD_SIRK, p_params);
}

/// \example{lineno} app_le_coordinated_set_coordinator.c
#endif