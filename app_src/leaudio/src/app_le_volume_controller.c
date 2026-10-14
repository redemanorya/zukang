/**
 ****************************************************************************************
 *
 * @file app_le_volume_controller.c
 *
 * @brief Applet - Volume Controller
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
#include "app_le_volume_controller.h"
#include "arc_vcc.h"
#include "le_cis_src.h"
#include "debug.h"

/*
 * ENUMERATIONS
 ****************************************************************************************
 */
#define COMPONENT_EVENT_SERVICE_CONTENT_DISCOVERED    (8)

// Event values for Service Discovery state machine
enum app_le_volume_controller_event_service_discovery
{
    // State machine start has been required
    EVENT_SERVICE_DISCOVERY_START = 0U,
    // Content of Volume Controller Service has been discovered
    EVENT_SERVICE_CONTENT_DISCOVERED,
};

/*
 * TYPES DEFINITION
 ****************************************************************************************
 */

/// Parameters structure for #COMPONENT_EVENT_SERVICE_DISCOVERED event type
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
typedef struct app_le_volume_controller_env
{
    // Event callback function for backward communication with Mediator
    component_cb_event cb_mediator_event;

    uint8_t conidx;
} app_le_volume_controller_env_t;

/*
 * LOCAL VARIABLES
 ****************************************************************************************
 */

// Pointer to environment
__STATIC app_le_volume_controller_env_t* p_env;
extern void app_le_audio_discovery(uint8_t event, uint8_t conidx, uint16_t status, const void* p_params);
/*
 * STATE MACHINES
 ****************************************************************************************
 */

void app_le_volume_controller_state_service_discovery(uint8_t event, uint8_t conidx, uint16_t status,
                                                               const prf_svc_t* p_svc)
{
    APP_DBG("app_le_volume_controller_state_service_discovery: evnt 0x%x, status 0x%x\n", event, status);
    if (status == GAF_ERR_NO_ERROR)
    {
        switch (event)
        {
            case EVENT_SERVICE_DISCOVERY_START:
            {
                APP_DBG("Start VCS discovery (conidx = %d)\n", conidx);

                if (p_svc != NULL)
                {
                    status = arc_vcc_discover(conidx, p_svc->shdl, p_svc->ehdl);
                    if (status)
                    {
                        APP_DBG("arc_vcc_discover err 0x%x\n", status);
                    }
                }
                else
                {
                    status = arc_vcc_discover(conidx, GATT_MIN_HDL, GATT_MAX_HDL);
                    if (status)
                    {
                        APP_DBG("arc_vcc_discover2 err 0x%x\n", status);
                    }
                }
            } break;

            case EVENT_SERVICE_CONTENT_DISCOVERED:
            {
                if (status != GAF_ERR_NO_ERROR) break;

                APP_DBG("VCS discovered (conidx = %d)\n", conidx);
                p_env->cb_mediator_event(COMPONENT_EVENT_SERVICE_CONTENT_DISCOVERED, GAF_ERR_NO_ERROR, &conidx);
            } break;

            default: { } break;
        }
    }
}

/*
 * CALLBACK FUNCTIONS
 ****************************************************************************************
 */
void app_vcc_volume_increase(void)
{
    uint16_t status = arc_vcc_volume_increase(p_env->conidx);
    if (status)
    {
        APP_DBG("app_vcc_volume_increase err 0x%x\n", status);
    }
}

void app_vcc_volume_decrease(void)
{
    uint16_t status = arc_vcc_volume_decrease(p_env->conidx);
    if (status)
    {
        APP_DBG("app_vcc_volume_decrease err 0x%x\n", status);
    }
}

void app_vcc_volume_offset(uint8_t index, int16_t volume_offset)
{
    uint16_t status = arc_vocc_set_offset(p_env->conidx, index, volume_offset);
    if (status)
    {
        APP_DBG("app_vcc_volume_offset err 0x%x\n", status);
    }
}

__STATIC void app_le_volume_controller_cb_bond_data_prov(uint16_t component_id, uint16_t type, uint8_t conidx,
                                                         bool found, uint16_t length, const void* p_data)
{
    uint16_t status;
    if (found)
    {
        status = arc_vcc_restore_bond_data(conidx, (const arc_vcc_vcs_t*)p_data);
        if (status)
        {
            APP_DBG("arc_vcc_restore_bond_data err 0x%x\n", status);           
        }
    }
}

__STATIC void app_le_volume_controller_cb_cmp_evt(uint8_t cmd_type, uint16_t status, uint8_t conidx, uint8_t param)
{
    APP_DBG("app_le_volume_controller_cb_cmp_evt: cmd_type 0x%x\n", cmd_type);
    ASSERT_WARN(status == GAF_ERR_NO_ERROR, status, cmd_type);

    switch (cmd_type)
    {
        case ARC_VCC_CMD_TYPE_DISCOVER:
        {
            app_le_volume_controller_state_service_discovery(EVENT_SERVICE_CONTENT_DISCOVERED, conidx, status, NULL);
        } break;

        default: { } break;
    }
}

__STATIC void app_le_volume_controller_cb_bond_data(uint8_t conidx, arc_vcc_vcs_t* p_svc_info)
{
    APP_DBG("app_le_volume_controller_cb_bond_data: conidx 0x%x\n", conidx);
}

__STATIC void app_le_volume_controller_cb_volume(uint8_t conidx, uint8_t volume, uint8_t mute)
{
    APP_DBG("app_le_volume_controller_cb_volume: volume %d, mute %d\n", volume, volume);
}

__STATIC void app_le_volume_controller_cb_flags(uint8_t conidx, uint8_t flags)
{
    APP_DBG("Flags: %d (conidx = %d)\n", flags, conidx);
}

__STATIC void app_le_volume_controller_cb_included_svc(uint8_t conidx, uint16_t uuid, uint16_t shdl, uint16_t ehdl)
{
    app_event_service_discovered_t event_params = {
        .conidx = conidx,
        .uuid = uuid,
        .svc = {
            .shdl = shdl,
            .ehdl = ehdl,
        },
    };

    APP_DBG("Included uuid: 0x%x found (conidx = %d)", uuid, conidx);
    app_le_audio_discovery(1, conidx, GAF_ERR_NO_ERROR, &event_params);
}

__STATIC void app_le_volume_controller_cb_svc_changed(uint8_t conidx)
{
}

// Audio Rendering Control - Volume Control Client callback set
__STATIC const arc_vcc_cb_t cbs_arc_vcc = {
    .cb_cmp_evt = app_le_volume_controller_cb_cmp_evt,
    .cb_volume = app_le_volume_controller_cb_volume,
    .cb_flags = app_le_volume_controller_cb_flags,
    .cb_bond_data = app_le_volume_controller_cb_bond_data,
    .cb_included_svc = app_le_volume_controller_cb_included_svc,
    .cb_svc_changed = app_le_volume_controller_cb_svc_changed,
};

/*
 * FUNCTIONS
 ****************************************************************************************
 */

void app_le_volume_controller_configure(const void* p_params, component_cb_event cb_event)
{
    uint16_t status;
    ASSERT_ERR(p_env == NULL);
    ASSERT_ERR(cb_event != NULL);

    p_env = (app_le_volume_controller_env_t*)ke_malloc_user(sizeof(app_le_volume_controller_env_t), KE_MEM_PROFILE);
    ASSERT_ERR(p_env != NULL);
    memset(p_env, 0, sizeof(app_le_volume_controller_env_t));
    p_env->cb_mediator_event = cb_event;

    status = arc_vcc_configure(&cbs_arc_vcc);
    if (status)
    {
        APP_DBG("arc_vcc_configure: err 0x%x\n", status);
    }
}

/// \example{lineno} app_le_volume_controller.c

#endif // (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
