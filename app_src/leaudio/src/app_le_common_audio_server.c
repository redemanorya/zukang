/**
 ****************************************************************************************
 *
 * @file app_le_common_audio_server.c
 *
 * @brief App_le - Common Audio Server
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
#include <stdint.h>
#include "app_le_common_audio_server.h"
#include "cap_cas.h"
#include "debug.h"

/*
 * FUNCTIONS
 ****************************************************************************************
 */

void app_le_common_audio_server_configure(const app_le_common_audio_server_cfg_t* p_params,
                                                   component_cb_event cb_event)
{
    cap_cas_cfg_param_t cfg_cas = {
        .set_lid = (p_params != NULL) ? p_params->set_lid : GAF_INVALID_LID,
        .shdl = 0,
    };

    uint16_t status = cap_cas_configure(&cfg_cas);
    if (status)
    {
        APP_DBG("cap_cas_configure: err 0x%x\n", status);
    }
}

/// 
#endif