/**
 ****************************************************************************************
 *
 * @file applet_volume_renderer.c
 *
 * @brief Applet - Volume Renderer
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
#include "app_le_volume_renderer.h"
#include "arc_vcs.h"
#include "debug.h"

/*
 * TYPES DEFINITION
 ****************************************************************************************
 */

// Bond Data structure
typedef struct applet_volume_renderer_bond_data
{
    // Client configuration bit field for Volume Control Service
    uint8_t cli_cfg_bf;
    // Volume when disconnection has occurred
    uint8_t volume;
} applet_volume_renderer_bond_data_t;

/*
 * LOCAL VARIABLES
 ****************************************************************************************
 */

// Pointer to environment
// __STATIC applet_volume_renderer_env_t* p_env;

/*
 * CALLBACK FUNCTIONS
 ****************************************************************************************
 */


void app_le_vcs_volume_increase(void)
{
    uint16_t status = arc_vcs_volume_increase();
    if (status)
    {
        APP_DBG("arc_vcs_volume_increase: err 0x%x\n", status);
    }
}

void app_le_vcs_volume_decrease(void)
{
    uint16_t status = arc_vcs_volume_decrease();
    if (status)
    {
        APP_DBG("arc_vcs_volume_decrease: err 0x%x\n", status);
    }
}

void app_le_offset_moved(uint8_t index, int16_t volume_offset)
{
    uint16_t status = arc_vocs_set_offset(index, volume_offset);
    if (status)
    {
        APP_DBG("arc_vocs_set_offset: err 0x%x\n", status);
    }
}

__STATIC void applet_volume_renderer_cb_bond_data(uint8_t conidx, uint8_t cli_cfg_bf)
{
    APP_DBG("VCS Bond Data updated (conidx = %d, cli_cfg_bf = 0x%02X)\n", conidx, cli_cfg_bf);
}

__STATIC void applet_volume_renderer_cb_volume(uint8_t volume, uint8_t mute, bool local)
{
    APP_DBG("applet_volume_renderer_cb_volume: volume 0x%x, mute 0x%x, local 0x%x\n", volume, mute, local);
}

__STATIC void applet_volume_renderer_cb_flags(uint8_t flags)
{
    APP_DBG("applet_volume_renderer_cb_flags: flags 0x%x\n", flags);
}

// Audio Rendering Control - Volume Control Server callback set
__STATIC const arc_vcs_cb_t cbs_arc_vcs = {
    .cb_bond_data = applet_volume_renderer_cb_bond_data,
    .cb_volume = applet_volume_renderer_cb_volume,
    .cb_flags = applet_volume_renderer_cb_flags,
};

/*
 * FUNCTIONS
 ****************************************************************************************
 */

void app_le_volume_renderer_configure(const applet_volume_renderer_cfg_t* p_params,
                                               component_cb_event cb_event)
{
    // ASSERT_ERR(p_env == NULL);
    ASSERT_ERR(p_params != NULL);
    uint16_t status;

    // p_env = (applet_volume_renderer_env_t*)ke_malloc_user(sizeof(applet_volume_renderer_env_t), KE_MEM_PROFILE);
    // ASSERT_ERR(p_env != NULL);
    // memset(p_env, 0, sizeof(applet_volume_renderer_env_t));

    status = arc_vcs_configure(&cbs_arc_vcs, 10, 0, p_params->volume, 0, 0, ARC_VCS_CFG_FLAGS_NTF_BIT,
                                       0, NULL);
    
    if (status)
    {
        APP_DBG("arc_vcs_configure err 0x%x\n", status);
    }
}


/// \example{lineno} applet_volume_renderer.c

#endif // (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
