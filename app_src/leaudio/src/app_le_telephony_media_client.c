/**
 ****************************************************************************************
 *
 * @file app_le_telephony_media_client.c
 *
 * @brief Applet - Telephony and Media Audio Client
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
#include "app_le_telephony_media_client.h"
#include "tmap_tmac.h"
#include "le_cis_src.h"
#include "utils_audio.h"
#include "debug.h"

/*
 * ENUMERATIONS
 ****************************************************************************************
 */
#define COMPONENT_EVENT_SERVICE_CONTENT_DISCOVERED   (8)

// Event values for Service Discovery state machine
enum app_le_telephony_media_client_event_service_discovery
{
    // State machine start has been required
    EVENT_SERVICE_DISCOVERY_START = 0U,
    // Content of Telephony and Media Control Service has been discovered
    EVENT_SERVICE_CONTENT_DISCOVERED,
};

// Bond data types
enum app_le_telephony_media_client_bond_data_type
{
    // TMAS content
    BOND_DATA_TMAS_CONTENT = 0U,
    // TMAP roles bit field
    BOND_DATA_TMAP_ROLES,
};

/*
 * TYPES DEFINITION
 ****************************************************************************************
 */

// Environment structure
typedef struct app_le_telephony_media_client_env
{
    // Event callback function for backward communication with Mediator
    component_cb_event cb_mediator_event;
} app_le_telephony_media_client_env_t;

/*
 * LOCAL VARIABLES
 ****************************************************************************************
 */

// Pointer to environment
__STATIC app_le_telephony_media_client_env_t* p_env;

/*
 * STATE MACHINES
 ****************************************************************************************
 */

void app_le_telephony_media_client_state_service_discovery(uint8_t event, uint8_t conidx, uint16_t status,
                                                                    const prf_svc_t* p_svc)
{
    switch (event)
    {
        case EVENT_SERVICE_DISCOVERY_START:
        {
            APP_DBG("Start TMAS discovery (conidx = %d)\n", conidx);

            if (p_svc != NULL)
            {
                status = tmap_tmac_discover(conidx, p_svc->shdl, p_svc->ehdl);
                if (status)
                {
                    APP_DBG("tmap_tmac_discover: err 0x%x\n", status);
                }
            }
            else
            {
                status = tmap_tmac_discover(conidx, GATT_MIN_HDL, GATT_MAX_HDL);
                if (status)
                {
                    APP_DBG("tmap_tmac_discover2: err 0x%x\n", status);
                }
            }
        } break;

        case EVENT_SERVICE_CONTENT_DISCOVERED:
        {
            if (status != GAF_ERR_NO_ERROR) break;

            APP_DBG("TMAP discovered (conidx = %d)\n", conidx);
            p_env->cb_mediator_event(COMPONENT_EVENT_SERVICE_CONTENT_DISCOVERED, GAF_ERR_NO_ERROR, &conidx);
        } break;

        default: { } break;
    }
}

/*
 * CALLBACK FUNCTIONS
 ****************************************************************************************
 */

__STATIC void app_le_telephony_media_client_cb_bond_data_prov(uint16_t component_id, uint16_t type, uint8_t conidx,
                                                         bool found, uint16_t length, const void* p_data)
{
    uint16_t status;
    if (found)
    {
        switch (type)
        {
            case BOND_DATA_TMAS_CONTENT:
            {
                status = tmap_tmac_restore_bond_data(conidx, (const tmap_tmac_tmas_t*)p_data);
                if (status)
                {
                    APP_DBG("tmap_tmac_restore_bond_data err 0x%x\n", status);
                }
            } break;

            case BOND_DATA_TMAP_ROLES:
            {
                utils_audio_le_cfg_t* p_le_audio_cfg = utils_audio_le_get_cfg(conidx);
                p_le_audio_cfg->tmap_role_bf = *((uint8_t*)p_data);
            } break;

            default: { } break;
        }
    }
}

__STATIC void app_le_telephony_media_client_cb_cmp_evt(uint8_t cmd_type, uint16_t status, uint8_t conidx)
{
    ASSERT_WARN(status == GAF_ERR_NO_ERROR, status, cmd_type);

    switch (cmd_type)
    {
        case TMAP_TMAC_CMD_TYPE_DISCOVER:
        {
            app_le_telephony_media_client_state_service_discovery(EVENT_SERVICE_CONTENT_DISCOVERED, conidx,
                                                                  status, NULL);
        } break;

        default: { } break;
    }
}

__STATIC void app_le_telephony_media_client_cb_bond_data(uint8_t conidx, const tmap_tmac_tmas_t* p_tmas_info)
{
    APP_DBG("app_le_telephony_media_client_cb_bond_data\n");
}

__STATIC void app_le_telephony_media_client_cb_svc_changed(uint8_t conidx)
{
}

__STATIC void app_le_telephony_media_client_cb_role(uint8_t conidx, uint16_t role_bf)
{
    APP_DBG("TMAP Roles = 0x%02X (conidx = %d)\n", role_bf, conidx);
}

// Telephony and Media Audio Client callback set
__STATIC const tmap_tmac_cb_t cbs_tmap_tmac = {
    .cb_cmp_evt = app_le_telephony_media_client_cb_cmp_evt,
    .cb_bond_data = app_le_telephony_media_client_cb_bond_data,
    .cb_svc_changed = app_le_telephony_media_client_cb_svc_changed,
    .cb_role = app_le_telephony_media_client_cb_role,
};

/*
 * FUNCTIONS
 ****************************************************************************************
 */

void app_le_telephony_media_client_configure(const void* p_params, component_cb_event cb_event)
{
    ASSERT_ERR(p_env == NULL);
    ASSERT_ERR(cb_event != NULL);
    uint16_t status;

    p_env = (app_le_telephony_media_client_env_t*)ke_malloc_user(sizeof(app_le_telephony_media_client_env_t),
                                                                 KE_MEM_PROFILE);
    ASSERT_ERR(p_env != NULL);
    memset(p_env, 0, sizeof(app_le_telephony_media_client_env_t));
    p_env->cb_mediator_event = cb_event;

    status = tmap_tmac_configure(&cbs_tmap_tmac);
    if (status)
    {
        APP_DBG("tmap_tmac_configure err 0x%x\n", status);
    }
}

/// \example{lineno} app_le_telephony_media_client.c
#endif