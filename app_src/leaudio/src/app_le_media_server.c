/**
 ****************************************************************************************
 *
 * @file app_le_media_server.c
 *
 * @brief Applet - Media Server
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
#include "string.h"
#include "app_le_media_server.h"
#include "acc_mcs.h"
#include "debug.h"
/*
 * TYPES DEFINITION
 ****************************************************************************************
 */

// Bond data structure
typedef struct app_le_media_server_bond_data
{
    // Client configuration bit field for Media Control Service
    uint32_t cli_cfg_bf;
} app_le_media_server_bond_data_t;

// Environment structure
typedef struct app_le_media_server_env
{
    // Callback function for backward communication with Mediator
    component_cb_event cb_mediator_event;
    // Pointer to track title
    const char* p_track_title;
    // Media local index
    uint8_t media_lid;
} app_le_media_server_env_t;

/*
 * LOCAL VARIABLES
 ****************************************************************************************
 */

// Output description values
__STATIC const char app_le_media_server_player_name[] = "MY PLAYER";
__STATIC const char app_le_media_server_track_title_first[] = "CEVA FIRST TRACK";
__STATIC const char app_le_media_server_track_title_second[] = "CEVA SECOND TRACK";

// Pointer to environment
__STATIC app_le_media_server_env_t* p_env;

/*
 * LOCAL FUNCTION
 ****************************************************************************************
 */

__STATIC void app_le_media_server_track_change(uint8_t media_lid)
{
    uint16_t status;
    APP_DBG("app_le_media_server_track_change\n");
    status = acc_mcs_track_change_basic(media_lid, ACC_MC_TRACK_DUR_UNKNOWN,
                                                strlen(p_env->p_track_title), (uint8_t*)(p_env->p_track_title));
    if (status)
    {
        APP_DBG("acc_mcs_track_change_basic: err 0x%x\n", status);
    }
}

void app_le_mcs_play(void)
{
    app_le_media_server_event_state_updated_t params_state = {
        .state = ACC_MC_MEDIA_STATE_PLAYING,
    };

    if (p_env->p_track_title == NULL)
    {
        p_env->p_track_title = app_le_media_server_track_title_first;
        app_le_media_server_track_change(p_env->media_lid);
    }

    uint16_t status = acc_mcs_play(p_env->media_lid);
    if (status)
    {
        APP_DBG("acc_mcs_play: err 0x%x\n", status);
    }
    p_env->cb_mediator_event(APP_LE_MEDIA_SERVER_EVENT_STATE_UPDATED, GAF_ERR_NO_ERROR, &params_state);
}

void app_le_mcs_pause(void)
{
    app_le_media_server_event_state_updated_t params_state = {
        .state = ACC_MC_MEDIA_STATE_PAUSED,
    };

    uint16_t status = acc_mcs_pause(p_env->media_lid);
    if (status)
    {
        APP_DBG("acc_mcs_play: err 0x%x\n", status);
    }
    p_env->cb_mediator_event(APP_LE_MEDIA_SERVER_EVENT_STATE_UPDATED, GAF_ERR_NO_ERROR, &params_state);
}

void app_le_mcs_stop(void)
{
    app_le_media_server_event_state_updated_t params_state = {
        .state = ACC_MC_MEDIA_STATE_INACTIVE,
    };

    uint16_t status = acc_mcs_stop(p_env->media_lid);
    if (status)
    {
        APP_DBG("acc_mcs_stop: err 0x%x\n", status);     
    }

    p_env->cb_mediator_event(APP_LE_MEDIA_SERVER_EVENT_STATE_UPDATED, GAF_ERR_NO_ERROR, &params_state);
}

void app_le_mcs_previous(void)
{
    p_env->p_track_title = app_le_media_server_track_title_first;
    app_le_media_server_track_change(p_env->media_lid);
}

void app_le_mcs_next(void)
{
    p_env->p_track_title = app_le_media_server_track_title_second;
    app_le_media_server_track_change(p_env->media_lid);
}

/*
 * CALLBACK FUNCTIONS
 ****************************************************************************************
 */
__STATIC void app_le_media_server_cb_bond_data_prov(uint16_t component_id, uint16_t type, uint8_t conidx, bool found,
                                                    uint16_t length, const void* p_data)
{
    uint16_t status;
    if (found)
    {
        uint16_t length = (p_env->p_track_title != NULL) ? strlen(p_env->p_track_title) : 0;
        const app_le_media_server_bond_data_t* p_bond_data = (const app_le_media_server_bond_data_t*)p_data;

        status = acc_mcs_restore_bond_data(0, conidx, p_bond_data->cli_cfg_bf, 0, 0, length, NULL,
                                                   (uint8_t*)(p_env->p_track_title));
        if (status)
        {
            APP_DBG("acc_mcs_restore_bond_data: err 0x%x\n", status);
        }
    }
}

__STATIC void app_le_media_server_cb_bond_data(uint8_t media_lid, uint8_t conidx, uint32_t cli_cfg_bf)
{
    APP_DBG("MCS Bond Data updated (conidx= %d, media_lid = %d, cli_cfg_bf = 0x%02lX)\n", conidx, media_lid, cli_cfg_bf);
}

__STATIC void app_le_media_server_cb_set_object_id_req(uint8_t media_lid, uint8_t conidx, uint8_t char_type,
                                                       const acc_mc_object_id_t* p_obj_id)
{
    APP_DBG("Set object ID request (conidx= %d, media_lid = %d, char_type = %d)\n", conidx, media_lid, char_type);
}

__STATIC void app_le_media_server_cb_search_req(uint8_t media_lid, uint8_t conidx, uint8_t param_len,
                                                const uint8_t* p_param)
{
    APP_DBG("Search request (conidx= %d, media_lid = %d)\n", conidx, media_lid);
}

__STATIC void app_le_media_server_cb_control_req(uint8_t media_lid, uint8_t conidx, uint8_t opcode, int32_t val)
{
    uint8_t action = ACC_MCS_ACTION_NO_ACTION;

    APP_DBG("Control request (conidx = %d, media_lid = %d, opcode = %d)\n", conidx, media_lid, opcode);

    switch (opcode)
    {
        case ACC_MC_OPCODE_PLAY:
        {
            action = ACC_MCS_ACTION_PLAY;
        } break;

        case ACC_MC_OPCODE_PAUSE:
        {
            action = ACC_MCS_ACTION_PAUSE;
        } break;

        case ACC_MC_OPCODE_STOP:
        {
            action = ACC_MCS_ACTION_INACTIVE;
        } break;

        case ACC_MC_OPCODE_PREV_TRACK:
        case ACC_MC_OPCODE_FIRST_TRACK:
        {
            p_env->p_track_title = &(app_le_media_server_track_title_first[0]);
            action = ACC_MCS_ACTION_CHANGE_TRACK;
        } break;

        case ACC_MC_OPCODE_NEXT_TRACK:
        {
            p_env->p_track_title = &(app_le_media_server_track_title_second[0]);
            action = ACC_MCS_ACTION_CHANGE_TRACK;
        } break;

        default: { ASSERT_INFO(0, opcode, 0); } break;
    }

    acc_mcs_cfm_control(ACC_MC_MEDIA_CP_RESULT_SUCCESS, media_lid, action, 0, 0);

    switch (action)
    {
        case ACC_MC_OPCODE_PLAY:
        {
            app_le_media_server_event_state_updated_t params_state_updated = {
                .state = ACC_MC_MEDIA_STATE_PLAYING,
            };

            p_env->cb_mediator_event(APP_LE_MEDIA_SERVER_EVENT_STATE_UPDATED, GAF_ERR_NO_ERROR,
                                     &params_state_updated);
        } break;

        case ACC_MC_OPCODE_PAUSE:
        {
            app_le_media_server_event_state_updated_t params_state_updated = {
                .state = ACC_MC_MEDIA_STATE_PAUSED,
            };

            p_env->cb_mediator_event(APP_LE_MEDIA_SERVER_EVENT_STATE_UPDATED, GAF_ERR_NO_ERROR,
                                     &params_state_updated);
        } break;

        case ACC_MC_OPCODE_STOP:
        {
            app_le_media_server_event_state_updated_t params_state_updated = {
                .state = ACC_MC_MEDIA_STATE_INACTIVE,
            };

            p_env->cb_mediator_event(APP_LE_MEDIA_SERVER_EVENT_STATE_UPDATED, GAF_ERR_NO_ERROR,
                                     &params_state_updated);
        } break;

        case ACC_MCS_ACTION_CHANGE_TRACK:
        {
            app_le_media_server_track_change(media_lid);
        } break;

        default: { } break;
    }
}

__STATIC void app_le_media_server_cb_get_req(uint8_t media_lid, uint8_t conidx, uint8_t char_type,
                                             uint16_t token, uint16_t offset, uint16_t length)
{
    switch (char_type)
    {
        case ACC_MC_CHAR_TYPE_PLAYER_NAME:
        {
            acc_mcs_cfm_get(true, media_lid, conidx, ACC_MC_CHAR_TYPE_PLAYER_NAME, token,
                            strlen(app_le_media_server_player_name), (uint8_t*)app_le_media_server_player_name);
        } break;

        case ACC_MC_CHAR_TYPE_TRACK_TITLE:
        {
            uint16_t length = (p_env->p_track_title != NULL) ? strlen(p_env->p_track_title) : 0;

            acc_mcs_cfm_get(true, media_lid, conidx, ACC_MC_CHAR_TYPE_TRACK_TITLE, token, length,
                            (uint8_t*)(p_env->p_track_title));
        } break;

        default: { } break;
    }
}

__STATIC void app_le_media_server_cb_get_position_req(uint8_t media_lid, uint8_t conidx, uint16_t token)
{
    APP_DBG("Get position request (conidx= %d, media_lid = %d)\n", conidx, media_lid);
    acc_mcs_cfm_get_position(true, media_lid, conidx, token, ACC_MC_TRACK_POS_UNAVAILABLE);
}

__STATIC void app_le_media_server_cb_set_req(uint8_t media_lid, uint8_t conidx, uint8_t char_type,
                                                     uint32_t val)
{
    APP_DBG("Set request (conidx= %d, media_lid = %d, char_type = %d)\n", conidx, media_lid, char_type);
}

// Set of callback functions for communication with Audio Content Control - Media Control Server module
__STATIC const acc_mcs_cb_t cbs_acc_mcs = {
    .cb_bond_data = app_le_media_server_cb_bond_data,
    .cb_set_object_id_req = app_le_media_server_cb_set_object_id_req,
    .cb_search_req = app_le_media_server_cb_search_req,
    .cb_control_req = app_le_media_server_cb_control_req,
    .cb_get_req = app_le_media_server_cb_get_req,
    .cb_get_position_req = app_le_media_server_cb_get_position_req,
    .cb_set_req = app_le_media_server_cb_set_req,
};

/*
 * FUNCTIONS
 ****************************************************************************************
 */

void app_le_media_server_configure(const void* p_params, component_cb_event cb_event)
{
    uint8_t media_lid;
    uint16_t status;

    ASSERT_ERR(p_env == NULL);
    ASSERT_ERR(cb_event != NULL);

    p_env = (app_le_media_server_env_t*)ke_malloc_user(sizeof(app_le_media_server_env_t), KE_MEM_PROFILE);
    ASSERT_ERR(p_env != NULL);
    memset(p_env, 0, sizeof(app_le_media_server_env_t));
    p_env->cb_mediator_event = cb_event;

    do
    {
        status = acc_mcs_configure(1, &cbs_acc_mcs, 0);
        if (status)
        {
            APP_DBG("acc_mcs_configure err 0x%x\n", status);
            break;
        }

        status = acc_mcs_add(ACC_MCS_CFG_TRACK_TITLE_NTF_SUPP_BIT | ACC_MCS_CFG_TRACK_DURATION_NTF_SUPP_BIT |
                                     ACC_MCS_CFG_MEDIA_CP_SUPP_BIT | ACC_MCS_CFG_PLAYBACK_SPEED_NTF_SUPP_BIT |
                                     ACC_MCS_CFG_SEEKING_SPEED_NTF_SUPP_BIT | ACC_MCS_CFG_PLAYING_ORDER_SUPP_BIT |
                                     ACC_MCS_CFG_PLAYING_ORDER_NTF_SUPP_BIT,
                                     ACC_MC_PLAY_ORDER_SUPP_MASK, 0, MEDIA_CCID, NULL, 0, &media_lid);
        if (status)
        {
            APP_DBG("acc_mcs_add err 0x%x\n", status);
        }
        else
        {
            p_env->media_lid = media_lid;
        }

    } while (false);
}

/// \example{lineno} app_le_media_server.c
#endif