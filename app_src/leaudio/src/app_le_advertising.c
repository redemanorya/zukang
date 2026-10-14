/**
 ****************************************************************************************
 *
 * @file app_le_gaf_advertising.c
 *
 * @brief Applet - GAF Advertising
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
#include <string.h>
#include "app_le_advertising.h"
#include "debug.h"
#include "gaf_adv.h"
#include <stdio.h>

/*
 * TYPES DEFINITION
 ****************************************************************************************
 */
/// Common control type values
enum component_control_type
{
    /// Start
    COMPONENT_CONTROL_START = 0U,
    /// Stop
    COMPONENT_CONTROL_STOP,

    /// Configure
    COMPONENT_CONTROL_CONFIGURE,

    APP_LE_AUDIO_UNICAST_CLIENT_CONTROL_PAUSE,

    APP_LE_AUDIO_UNICAST_CLIENT_CONTROL_ADD,

	COMPONENT_CONTROL_RESTART,
    /// First available control type value for child Components
    COMPONENT_CONTROL_FIRST,
};

extern const char* utils_get_device_name(void);
/*
 * CALLBACK FUNCTIONS
 ****************************************************************************************
 */
__STATIC void app_le_gaf_advertising_cb_cmp_evt(uint8_t cmd_type, uint16_t status, uint8_t set_lid)
{
    ASSERT_INFO(status == GAF_ERR_NO_ERROR, status, cmd_type);

    APP_DBG("app_le_gaf_advertising_cb_cmp_evt: cmd_type 0x%x, status 0x%x, set_lid 0x%x\n", cmd_type, status, set_lid);
    switch (cmd_type)
    {
        case GAF_ADV_CMD_TYPE_START:
        {
            // bool is_advertising = true;
        } break;

        default: { } break;
    }
}

__STATIC void app_le_gaf_advertising_cb_stopped(uint8_t set_lid, uint8_t reason)
{
    APP_DBG("app_le_gaf_advertising_cb_stopped\n");
    // bool is_advertising = false;
}

// GAF Advertising callback set
__STATIC const gaf_adv_cb_t cbs_gaf_adv = {
    .cb_cmp_evt = app_le_gaf_advertising_cb_cmp_evt,
    .cb_stopped = app_le_gaf_advertising_cb_stopped,
};

/*
 * FUNCTIONS
 ****************************************************************************************
 */

__STATIC void app_le_gaf_advertising_configure(const void* p_params, component_cb_event cb_event)
{
    uint16_t status;
    gaf_adv_cfg_t cfg = {
            .nb_sets = 1,
    };

    // Configure use of GAF ADV module
    status = gaf_adv_configure(&cfg, &cbs_gaf_adv);
    if (status)
    {
        APP_DBG("gaf_adv_configure err 0x%x\n", status);
    }
}

__STATIC void app_le_gaf_advertising_control(uint16_t type, const void* p_params)
{
    uint16_t status;
    switch (type)
    {
        case COMPONENT_CONTROL_START:
        case COMPONENT_CONTROL_RESTART:
        {
            const app_le_gaf_advertising_start_t* p_params_start = (const app_le_gaf_advertising_start_t*)p_params;
            // uint8_t cfg_bf = GAPM_ADV_MODE_GEN_DISC | GAF_ADV_CFG_GENERAL_ANNOUNCEMENT_BIT | GAF_ADV_CFG_SOLICITATION_REQ_BIT | GAF_ADV_CFG_PRIVACY_BIT;
            uint8_t cfg_bf = GAPM_ADV_MODE_GEN_DISC | GAF_ADV_CFG_GENERAL_ANNOUNCEMENT_BIT;
            const char* p_device = utils_get_device_name();
            char adv_data[17];

            // if ((p_params_start != NULL) && p_params_start->is_solicitation)
            // {
            //     SETB(cfg_bf, GAF_ADV_CFG_SOLICITATION_REQ, 1);
            // }

            adv_data[0] = strlen(p_device) + 1;
            adv_data[1] = GAP_AD_TYPE_COMPLETE_NAME;
            strcpy(&adv_data[2], p_device);

            status = gaf_adv_start(0, cfg_bf, 0, 1, strlen(adv_data), (uint8_t*)adv_data, NULL);
            if (status)
            {
                APP_DBG("gaf_adv_start err 0x%x\n", status);
            }
        } break;

        case COMPONENT_CONTROL_STOP:
        {
            status = gaf_adv_stop(0);
            if (status)
            {
                APP_DBG("gaf_adv_stop err 0x%x\n", status);
            }
        } break;

        default: { ASSERT_INFO(0, type, 0); }
    }
}

void app_le_adv_start(const void* p_params)
{
    app_le_gaf_advertising_configure(NULL, NULL);
    app_le_gaf_advertising_control(COMPONENT_CONTROL_START, p_params);
}

void app_le_adv_stop(void)
{
    app_le_gaf_advertising_control(COMPONENT_CONTROL_STOP, NULL);
}
/// \example{lineno} app_le_gaf_advertising.c
#endif
