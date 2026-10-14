/**
 ****************************************************************************************
 *
 * @file app_le_coordinated_set_member.c
 *
 * @brief Applet - Coordinated Set Member
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
#include "app_le_coordinated_set_member.h"
#include "atc_csism.h"
#include "debug.h"
/*
 * TYPES DEFINITION
 ****************************************************************************************
 */

// Bond data structure
typedef struct app_le_coordinated_set_member_bond_data
{
    // Client configuration bit field for Coordinated Set Identification Service
    uint8_t cli_cfg_bf;
} app_le_coordinated_set_member_bond_data_t;

// Environment structure
typedef struct app_le_coordinated_set_member_env
{
    // Event callback function for backward communication with Mediator
    component_cb_event cb_mediator_event;
} app_le_coordinated_set_member_env_t;

/*
 * LOCAL VARIABLES
 ****************************************************************************************
 */

// Pointer to environment
__STATIC app_le_coordinated_set_member_env_t* p_env;

/*
 * CALLBACK FUNCTIONS
 ****************************************************************************************
 */

__STATIC void app_le_coordinated_set_member_cb_bond_data_prov(uint16_t component_id, uint16_t type, uint8_t conidx,
                                                              bool found, uint16_t length, const void* p_data)
{
    uint16_t status;
    if (found)
    {
        app_le_coordinated_set_member_bond_data_t* p_bond_data
                                                        = (app_le_coordinated_set_member_bond_data_t*)p_data;

        status = atc_csism_restore_bond_data(conidx, 0, false, p_bond_data->cli_cfg_bf, 0);
        if (status)
        {
            APP_DBG("atc_csism_restore_bond_data: err 0x%x\n", status);
        }
    }
}

__STATIC void app_le_coordinated_set_member_cb_cmp_evt(uint16_t cmd_code, uint16_t status, uint8_t set_lids)
{
    APP_DBG("app_le_coordinated_set_member_cb_cmp_evt: cmd_code: %d status 0x%x, (set_lids = %d)\n", cmd_code, status, set_lids);
    ASSERT_WARN(status == GAF_ERR_NO_ERROR, status, cmd_code);

    switch (cmd_code)
    {
        case (CSISM_ADD):
        {
            p_env->cb_mediator_event(APP_LE_COORDINATED_SET_MEMBER_EVENT_ADDED, status, NULL);
        } break;

        default: { } break;
    }
}

__STATIC void app_le_coordinated_set_member_cb_lock(uint8_t set_lid, uint8_t lock, uint8_t conidx, uint8_t reason)
{
    APP_DBG("Lock: %d (conidx = %d)\n", lock, conidx);
}

__STATIC void app_le_coordinated_set_member_cb_bond_data(uint8_t set_lid, uint8_t conidx, uint8_t cli_cfg_bf)
{
    APP_DBG("CSIS Bond Data updated (conidx = %d, cli_cfg_bf = 0x%02X)\n", conidx, cli_cfg_bf);
}

__STATIC void app_le_coordinated_set_member_cb_ltk_req(uint8_t set_lid, uint8_t conidx)
{
    APP_DBG("app_le_coordinated_set_member_cb_ltk_req: conidx 0x%x, set_lid 0x%x\n", conidx, set_lid);

}
__STATIC void app_le_coordinated_set_member_cb_rsi(uint8_t set_lid, const csis_rsi_t* p_rsi)
{
    APP_DBG("set_lid 0x%x: New RSI: %02X:%02X:%02X:%02X:%02X:%02X\n",
                 set_lid, p_rsi->rsi[5], p_rsi->rsi[4], p_rsi->rsi[3], p_rsi->rsi[2], p_rsi->rsi[1], p_rsi->rsi[0]);

    p_env->cb_mediator_event(COMPONENT_EVENT_RSI_UPDATED, GAF_ERR_NO_ERROR, p_rsi);
}

// Set of callback functions for communication with Audio Topology Control - Coordinated Set Identification Service
// Member module
__STATIC const csism_cb_t cbs_atc_csism = {
    .cb_lock = app_le_coordinated_set_member_cb_lock,
    .cb_bond_data = app_le_coordinated_set_member_cb_bond_data,
    .cb_ltk_req = app_le_coordinated_set_member_cb_ltk_req,
    .cb_rsi = app_le_coordinated_set_member_cb_rsi,
    .cb_cmp_evt = app_le_coordinated_set_member_cb_cmp_evt,
};

/*
 * FUNCTIONS
 ****************************************************************************************
 */

void app_le_coordinated_set_member_configure(const app_le_coordinated_set_member_cfg_t* p_params,
                                                      component_cb_event cb_event)
{
    uint16_t status;
    ASSERT_ERR(p_env == NULL);
    ASSERT_ERR((p_params != NULL) && (p_params->p_sirk != NULL));
    ASSERT_ERR(cb_event != NULL);

    p_env = (app_le_coordinated_set_member_env_t*)ke_malloc_user(sizeof(app_le_coordinated_set_member_env_t),
                                                                      KE_MEM_PROFILE);
    ASSERT_ERR(p_env != NULL);
    memset(p_env, 0, sizeof(app_le_coordinated_set_member_env_t));
    p_env->cb_mediator_event = cb_event;

    do
    {
        status = atc_csism_configure(1, &cbs_atc_csism);
        if (status)
        {
            APP_DBG("atc_csism_configure err 0x%x\n", status);
            break;
        }

        status = atc_csism_add(CSISM_ADD_CFG_SIZE_BIT | CSISM_ADD_CFG_RANK_BIT, p_params->nb_members,
                                       p_params->rank, p_params->lock_timeout_s, 0, (csis_sirk_t*)p_params->p_sirk);
        if (status)
        {
            APP_DBG("atc_csism_add: err 0x%x\n", status);
        }
    } while (false);
}

/// \example{lineno} app_le_coordinated_set_member.c
#endif