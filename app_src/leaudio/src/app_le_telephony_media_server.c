/**
 ****************************************************************************************
 *
 * @file app_le_telephony_media_server.c
 *
 * @brief Applet - Telephony and Media Server
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "le_audio_config.h"
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
#include "app_le_telephony_media_server.h"
#include "tmap_tmas.h"
#include "utils_audio.h"
#include "debug.h"

#if 1//(GAF_TMAP_TMAS)

/*
 * FUNCTIONS
 ****************************************************************************************
 */

__STATIC void app_le_telephony_media_server_configure(const app_le_telephony_media_server_cfg_t* p_params,
                                                      component_cb_event cb_event)
{
    tmap_tmas_cfg_param_t cfg_tmas;
    utils_audio_le_cfg_t* p_le_audio_cfg = utils_audio_le_get_cfg_local();

    ASSERT_ERR(p_params != NULL);

    cfg_tmas.role_bf = p_params->role_bf;
    cfg_tmas.shdl = 0;

    uint16_t status = tmap_tmas_configure(&cfg_tmas);
    if (status)
    {
        APP_DBG("tmap_tmas_configure: err 0x%x\n", status);
    }
    p_le_audio_cfg->tmap_role_bf = p_params->role_bf;
}

void app_le_tmap_tmas_cfg(uint16_t role_bf)
{
    // app_le_telephony_media_server_cfg_t telephony_media_server_cfg = {
    //     .role_bf = TMAP_ROLE_UMR_BIT | TMAP_ROLE_BMR_BIT | TMAP_ROLE_CT_BIT,
    // };
    app_le_telephony_media_server_cfg_t telephony_media_server_cfg = {
        .role_bf = role_bf,
    };
    app_le_telephony_media_server_configure(&telephony_media_server_cfg, NULL);
}
///

#endif //(GAF_TMAP_TMAS)
#endif