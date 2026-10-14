/**
 ****************************************************************************************
 *
 * @file app_le_media_client.c
 *
 * @brief App le- Media Client
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "le_audio_config.h"
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
#include "app_le_media_client.h"
#include "co_buf.h"
#include "acc_mcc.h"
#include "prf_types.h"
#include "debug.h"


/*
 * ENUMERATIONS
 ****************************************************************************************
 */
#define COMPONENT_EVENT_SERVICE_CONTENT_DISCOVERED     (8)

// Event values for Service Discovery state machine
enum app_le_media_client_event_service_discovery
{
    // Request to start
    EVENT_SERVICE_DISCOVERY_START = 0U,
    // Discovery of Media Control Service instances has been completed
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

// Bond data structure
typedef struct app_le_media_client_bond_data
{
    // (Generic) Media Control Service content
    acc_mcc_mcs_info_t mcs;
} app_le_media_client_bond_data_t;

// Media state structure
typedef struct app_le_media_info
{
    // Duration in multiple of 0.01s
    int32_t duration_0_01s;
    // State
    uint8_t media_state;
    //loacl id
    uint8_t media_lid;
} app_le_media_info_t;

// Environment structure
typedef struct app_le_media_client_env
{
    // Event callback function for backward communication with Mediator
    component_cb_event cb_mediator_event;
    // Pointer to buffer allocated during discovery for storage of bond data
    co_buf_t* p_buf;
    // Media state
    app_le_media_info_t info;

} app_le_media_client_env_t;

/*
 * LOCAL VARIABLES
 ****************************************************************************************
 */

// Pointer to environment
__STATIC app_le_media_client_env_t* p_env;
extern void app_le_audio_discovery(uint8_t event, uint8_t conidx, uint16_t status, const void* p_params);
/*
 * STATE MACHINES
 ****************************************************************************************
 */

void app_le_media_client_state_service_discovery(uint8_t event, uint16_t status, uint8_t conidx,
                                                          const prf_svc_t* p_svc)
{
    app_le_media_client_bond_data_t* p_bond_data;

    switch (event)
    {
        case EVENT_SERVICE_DISCOVERY_START:
        {
            APP_DBG("Start MCS discovery\n");

            if (p_svc != NULL)
            {
                status = acc_mcc_discover(conidx, 1, p_svc->shdl, p_svc->ehdl);
                if (status)
                {
                    APP_DBG("acc_mcc_discover err 0x%x\n", status);
                    break;
                }
            }
            else
            {
                status = acc_mcc_discover(conidx, 1, GATT_MIN_HDL, GATT_MAX_HDL);
                if (status)
                {
                    APP_DBG("acc_mcc_discover2 err 0x%x\n", status);
                    break;
                }
            }
        } break;

        case EVENT_SERVICE_CONTENT_DISCOVERED:
        {
            if (status == GAF_ERR_NO_ERROR)
            {
                if (p_env->p_buf != NULL)
                {
                    APP_DBG("MCS discovery completed\n");

                    // p_bond_data = (app_le_media_client_bond_data_t*)co_buf_data(p_env->p_buf);
                    // BOND_DATA_STORE(p_env->p_bond_data_intf, APP_LE_ID(MEDIA_CLIENT), 0, conidx,
                    //                 sizeof(app_le_media_client_bond_data_t), p_bond_data);
                }
            }
            else
            {
                APP_DBG("Error during MCS discovery (status = 0x%02X)\n", status);
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

        default: { ASSERT_INFO(0, event, 0); } break;
    }
}

/*
 * CALLBACK FUNCTIONS
 ****************************************************************************************
 */
void app_le_client_media_play(void)
{
    APP_DBG("app_le_client_media_play: 0x%x\n", p_env->info.media_state);
    uint8_t conidx = 0;
    uint16_t status;
    if (p_env->info.media_state == ACC_MC_MEDIA_STATE_INACTIVE)
    {

        // A valid track must be selected first
        status = acc_mcc_control(conidx, p_env->info.media_lid,
                        ACC_MC_OPCODE_FIRST_TRACK, false, 0);
        if (status)
        {
            APP_DBG("acc_mcc_control: err 0x%x\n", status);
        }
    }    
    else
    {
        status = acc_mcc_play(conidx, p_env->info.media_lid, true); 
        if (status)
        {
            APP_DBG("acc_mcc_play12: err 0x%x\n", status);
        }              
    }
}

void app_le_client_media_pause(void)
{
    uint16_t status;
    uint8_t conidx = 0;
    status = acc_mcc_pause(conidx, p_env->info.media_lid, true);
    if (status)
    {
        APP_DBG("acc_mcc_pause: err 0x%x\n", status);
    }        
    // SETF(p_app_env->status_bf, APP_STATUS_USER_STATE, APP_STATE_USER_PAUSE_PUSHED);
}

void app_le_client_media_stop(void)
{
    uint16_t status;
    uint8_t conidx = 0;
    status = acc_mcc_stop(conidx, p_env->info.media_lid, true);
    if (status)
    {
        APP_DBG("acc_mcc_stop: err 0x%x\n", status);
    }    
    // SETF(p_app_env->status_bf, APP_STATUS_USER_STATE, APP_STATE_USER_STOP_PUSHED);
}

void app_le_client_media_previous(void)
{
    uint16_t status;
    uint8_t conidx = 0;
    status = acc_mcc_previous(conidx, p_env->info.media_lid, true);
    if (status)
    {
        APP_DBG("acc_mcc_previous: err 0x%x\n", status);
    }   
    // SETF(p_app_env->status_bf, APP_STATUS_USER_STATE, APP_STATE_USER_PREVIOUS_PUSHED);
}

void app_le_client_media_next(void)
{
    uint16_t status;
    uint8_t conidx = 0;
    status = acc_mcc_next(conidx, p_env->info.media_lid, true);
    if (status)
    {
        APP_DBG("acc_mcc_next: err 0x%x\n", status);
    } 
    // SETF(p_app_env->status_bf, APP_STATUS_USER_STATE, APP_STATE_USER_NEXT_PUSHED);
}

__STATIC void app_le_media_client_cb_bond_data_prov(uint16_t component_id, uint16_t type, uint8_t conidx, bool found,
                                                    uint16_t length, const void* p_data)
{
    uint16_t status;
    if (found)
    {
        const app_le_media_client_bond_data_t* p_bond_data = (const app_le_media_client_bond_data_t*)p_data;
        status = acc_mcc_restore_bond_data(conidx, 1, &(p_bond_data->mcs));
        if (status)
        {
            APP_DBG("acc_mcc_restore_bond_data: err 0x%x\n", status);
        }
    }
}

__STATIC void app_le_media_client_cb_cmp_evt(uint8_t cmd_type, uint16_t status, uint8_t conidx,
                                             uint8_t media_lid, uint8_t param, uint8_t result)
{
    APP_DBG("app_le_media_client_cb_cmp_evt: cmd_type 0x%x, media_lid 0x%x\n", cmd_type, media_lid);
    switch (cmd_type)
    {
        case ACC_MCC_CMD_TYPE_DISCOVER:
        {
            app_le_media_client_state_service_discovery(EVENT_SERVICE_CONTENT_DISCOVERED, status, conidx, NULL);
        } break;

        case ACC_MCC_CMD_TYPE_CONTROL:
        {
            ASSERT_WARN(status == GAF_ERR_NO_ERROR, status, cmd_type);

            if ((status == GAF_ERR_NO_ERROR) && (param == ACC_MC_OPCODE_FIRST_TRACK))
            {

                if (p_env->info.media_state == ACC_MC_MEDIA_STATE_INACTIVE)
                {
                    // A valid track must be selected first
                    acc_mcc_control(conidx, media_lid, ACC_MC_OPCODE_FIRST_TRACK, false, 0);
                }
                else
                {
                    acc_mcc_play(conidx, media_lid, true);
                }
            }
        } break;

        default:
        {
            ASSERT_WARN(status == GAF_ERR_NO_ERROR, status, cmd_type);
        } break;
    }
}

__STATIC void app_le_media_client_cb_track_changed(uint8_t conidx, uint8_t media_lid)
{
    APP_DBG("Media %d - Track Changed\n", media_lid);
    p_env->info.duration_0_01s = ACC_MC_TRACK_DUR_UNKNOWN;
}

__STATIC void app_le_media_client_cb_object_id(uint8_t conidx, uint8_t media_lid, uint8_t char_type,
                                               const acc_mc_object_id_t* p_obj_id)
{
    APP_DBG("app_le_media_client_cb_object_id: media_lid 0x%x, char_type 0x%x\n", 
                media_lid, char_type);
}

__STATIC void app_le_media_client_cb_value_long(uint8_t conidx, uint8_t media_lid, uint8_t char_type,
                                                uint16_t val_len, const char* p_val)
{ 
    APP_DBG("app_le_media_client_cb_value_long: char_type 0x%x\n", char_type);
    switch (char_type)
    {
        case ACC_MC_CHAR_TYPE_TRACK_TITLE:
        {

        } break;

        case ACC_MC_CHAR_TYPE_PLAYER_NAME:
        {

        } break;

        default: { } break;
    }
}

__STATIC void app_le_media_client_cb_value(uint8_t conidx, uint8_t media_lid, uint8_t char_type, uint32_t val)
{
    APP_DBG("app_le_media_client_cb_value: media_lid 0x%x, char_type 0x%x, val %lu\n", media_lid, char_type, val);
    switch (char_type)
    {
        case ACC_MC_CHAR_TYPE_MEDIA_STATE:
        {
            p_env->info.media_state = val;
        } break;

        case ACC_MC_CHAR_TYPE_TRACK_DURATION:
        {
            p_env->info.duration_0_01s = val;
        } break;

        case ACC_MC_CHAR_TYPE_TRACK_POSITION:
        {

        } break;

        default: { } break;
    }
}

__STATIC void app_le_media_client_cb_bond_data(uint8_t conidx, uint8_t media_lid, const acc_mcc_mcs_info_t* p_mcs_info)
{
    if (p_env->p_buf != NULL)
    {
        app_le_media_client_bond_data_t* p_bond_data = (app_le_media_client_bond_data_t*)co_buf_data(p_env->p_buf);

        APP_DBG("MCS Bond Data updated (media_lid = %d)\n", media_lid);

        p_env->info.media_lid = media_lid;
        memcpy(&(p_bond_data->mcs), p_mcs_info, sizeof(acc_mcc_mcs_info_t));
    }
}

__STATIC void app_le_media_client_cb_included_svc(uint8_t conidx, uint8_t media_lid, uint16_t shdl, uint16_t ehdl)
{
    app_event_service_discovered_t event_params = {
        .conidx = conidx,
        .uuid = GATT_SVC_OBJECT_TRANSFER,
        .svc = {
            .shdl = shdl,
            .ehdl = ehdl,
        },
    };

    APP_DBG("Included OTS found\n");
    // p_env->cb_mediator_event(COMPONENT_EVENT_SERVICE_DISCOVERED, GAF_ERR_NO_ERROR, &event_params);
    app_le_audio_discovery(1, conidx, GAF_ERR_NO_ERROR, &event_params);
}

__STATIC void app_le_media_client_cb_svc_changed(uint8_t conidx)
{
}

// Set of callback functions for communication with Audio Content Control - Media Control Client module
__STATIC const acc_mcc_cb_t cbs_acc_mcc = {
    .cb_cmp_evt = app_le_media_client_cb_cmp_evt,
    .cb_track_changed = app_le_media_client_cb_track_changed,
    .cb_object_id = app_le_media_client_cb_object_id,
    .cb_value_long = app_le_media_client_cb_value_long,
    .cb_value = app_le_media_client_cb_value,
    .cb_bond_data = app_le_media_client_cb_bond_data,
    .cb_included_svc = app_le_media_client_cb_included_svc,
    .cb_svc_changed = app_le_media_client_cb_svc_changed,
};

/*
 * FUNCTIONS
 ****************************************************************************************
 */

void app_le_media_client_configure(const void* p_params, component_cb_event cb_event)
{
    uint16_t status;
    ASSERT_ERR(p_env == NULL);
    ASSERT_ERR(cb_event != NULL);

    p_env = (app_le_media_client_env_t*)ke_malloc_user(sizeof(app_le_media_client_env_t), KE_MEM_PROFILE);
    ASSERT_ERR(p_env != NULL);
    memset(p_env, 0, sizeof(app_le_media_client_env_t));
    p_env->cb_mediator_event = cb_event;
    p_env->info.duration_0_01s = ACC_MC_TRACK_DUR_UNKNOWN;

    status = acc_mcc_configure(&cbs_acc_mcc, 0);
    if (status)
    {
        APP_DBG("acc_mcc_configure: err 0x%x\n", status);
    }
}
#endif
/// \example{lineno} app_le_media_client.c
