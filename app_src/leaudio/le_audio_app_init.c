/**
 * @file le_audio_app_init.c
 * @author Cole (cole@mvsilicon.com)
 * @brief
 * @version 0.1
 * @date 2025-03-24
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "le_audio_app_init.h"
//#include "le_audio_config.h"
#include "gatt.h"
#include "bt_config.h"
// #include "ble_app_gatt.h"
#include "le_bis_src.h"
#include "le_bis_sink.h"
#include "le_bis_delegator.h"
#include "le_bis_assis.h"
#include "app_le_udsc.h"
#include "app_le_udss.h"
#include "utils_cb_gap.h"
#include "ble_api.h"
#include "bt_manager.h"
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
#include "le_cis_src.h"
#include "le_cis_sink.h"
#endif

// º¯ÊýÉùÃ÷
#include "debug.h"
#if (BLE_SUPPORT)
// #include "ble_tws.h"
extern BT_CONFIGURATION_PARAMS		*btStackConfigParams;

static const gapm_config_t CONFIGURATION = {
    .role = GAP_ROLE_LE_ALL,
    .pairing_mode = 0,
    .privacy_cfg = 0,
    .renew_dur = 1500,
    .private_identity.addr = {0, 0, 0, 0, 0, 0},
    .irk.key = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    .gap_start_hdl = 0,
    .gatt_start_hdl = 0,
    .att_cfg = 0,
    .sugg_max_tx_octets = GAP_LE_MAX_OCTETS,
    .sugg_max_tx_time = GAP_LE_MIN_TIME,
    .tx_pref_phy = GAP_PHY_ANY,
    .rx_pref_phy = GAP_PHY_ANY,
    .tx_path_comp = 0,
    .rx_path_comp = 0,
    .class_of_device = 0x200408,
    .dflt_link_policy = 0
};

static const gapm_config_t CONFIGURATION2 = {
    .role = GAP_ROLE_LE_ALL,
    .pairing_mode = GAPM_PAIRING_MODE_ALL,
    .privacy_cfg = 0,
    .renew_dur = 1500,
    .private_identity.addr = {0, 0, 0, 0, 0, 0},
    .irk.key = {0xFF, 0xE2, 0xB6, 0xB1, 0x04, 0x02, 0x8A, 0x18, 0xA8, 0xC2, 0xBF,   \
                    0xC3, 0x20, 0x37, 0x5C, 0xC9},
    .gap_start_hdl = 0,
    .gatt_start_hdl = 0,
    .att_cfg = 0,
    .sugg_max_tx_octets = GAP_LE_MAX_OCTETS,
    .sugg_max_tx_time = GAP_LE_MIN_TIME,
    .tx_pref_phy = GAP_PHY_ANY,
    .rx_pref_phy = GAP_PHY_ANY,
    .tx_path_comp = 0,
    .rx_path_comp = 0,
    .class_of_device = 0x200408,
    .dflt_link_policy = 0
};
enum le_error_codes {
    LE_SUCCESS = 0,
    LE_ERR_UDSS_INIT,
    LE_ERR_BIS_SOURCE_INIT,
    LE_ERR_BIS_SINK_INIT,
    LE_ERR_UDSC_INIT,
    LE_ERR_BIS_ASSIS_INIT,
    LE_ERR_BIS_DELEG_INIT,
    LE_ERR_GAPM_INIT,
};

enum gapm_cfg_id {
    LE_BIS_SOURCE_INIT = 0,
    LE_BIS_SINK_INIT,
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
    LE_CIS_SOURCE_INIT,
    LE_CIS_SINK_INIT,
#endif
    LE_UDSC_INIT,
    LE_UDSS_INIT,
    LE_BIS_ASSIS_INIT,
    LE_BIS_DELEG_INIT,
    LE_BLE_GS_INIT,
};

static gapm_all_cfg_t gapm_config[LE_BLE_GS_INIT + 1] = {
#if (BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT)
    [LE_BIS_SOURCE_INIT] = {0, &CONFIGURATION, &app_gap_cbs, bis_src_configuration_cb_gapm_cmp},
    [LE_BIS_SINK_INIT] = {0, &CONFIGURATION, &app_gap_cbs, le_bis_sink_configuration_cb_gapm_cmp},
#endif
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
#ifdef LE_CIS_SRC
    [LE_CIS_SOURCE_INIT] = {EVENT_CONFIGURATION_DONE, &CONFIGURATION2, &app_gap_cbs, (gapm_proc_cmp_cb)cis_src_configuration_cb_gapm_cmp},
#endif
#ifdef LE_CIS_SINK
    [LE_CIS_SINK_INIT] = {EVENT_CONFIGURATION_DONE, &CONFIGURATION2, &app_gap_cbs, (gapm_proc_cmp_cb)le_cis_sink_configuration_cb_gapm_cmp},
#endif
#endif
#ifdef LE_UDSC
    [LE_UDSC_INIT] = {EVENT_CONFIGURATION_DONE, &CONFIGURATION2, &app_gap_cbs, (gapm_proc_cmp_cb)udsc_configuration_cb_gapm_cmp},
#endif
#ifdef LE_UDSS
    [LE_UDSS_INIT] = {EVENT_CONFIGURATION_DONE, &CONFIGURATION2, &app_gap_cbs, (gapm_proc_cmp_cb)app_udss_configuration_cb_gapm_cmp},
#endif
#ifdef LE_BIS_ASSIS
    [LE_BIS_ASSIS_INIT] = {EVENT_CONFIGURATION_DONE, &CONFIGURATION2, &app_gap_cbs, (gapm_proc_cmp_cb)bis_assis_configuration_cb_gapm_cmp},
#endif

#ifdef LE_BIS_DELEG
    [LE_BIS_DELEG_INIT] = {EVENT_CONFIGURATION_DONE, &CONFIGURATION2, &app_gap_cbs, (gapm_proc_cmp_cb)le_bis_deleg_configuration_cb_gapm_cmp},
#endif
#if (BLE_LINK_SUPPORT)
    [LE_BLE_GS_INIT] = {0x100, &CONFIGURATION2, &ble_gs_gap_cbs, (gapm_proc_cmp_cb)ble_gs_configuration_cb_gapm_cmp},
#endif
};

uint16_t gapm_configure_init(uint8_t le_app_id)
{
    // const uint8_t le_app_id = LE_TWS_RUN_INIT;
    return gapm_configure(gapm_config[le_app_id].metainfo,
						  gapm_config[le_app_id].p_cfg,
						  gapm_config[le_app_id].p_cbs,
						  gapm_config[le_app_id].cmp_cb);
}

uint8_t get_le_app_id(void)
{
    uint8_t le_app_id = LE_BIS_SOURCE_INIT; 
#ifdef LE_UDSS
    le_app_id = LE_UDSS_INIT;
#endif
#ifdef LE_UDSC
    le_app_id = LE_UDSC_INIT;
#endif
#ifdef LE_BIS_ASSIS
    le_app_id = LE_BIS_ASSIS_INIT;
#endif
#ifdef LE_BIS_DELEG
    le_app_id = LE_BIS_DELEG_INIT;
#endif
#ifndef VENDOR_BLE_APP
#if (BLE_TWS_SUPPORT)
    le_app_id = LE_TWS_RUN_INIT;
#elif (BLE_LINK_SUPPORT)
    le_app_id = LE_BLE_GS_INIT;
#endif
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
#ifdef LE_CIS_SRC
    le_app_id = LE_CIS_SOURCE_INIT;
#endif
#ifdef LE_CIS_SINK
    le_app_id = LE_CIS_SINK_INIT;
#endif
#endif
#endif

    APP_DBG("get_le_app_id: %d\n", le_app_id);
    return le_app_id;
}

bool is_use_tws_link(void)
{
    bool use_tws_link = false;
#if (BLE_TWS_SUPPORT || BLE_LINK_SUPPORT)
    use_tws_link = true;
#endif
    return use_tws_link;
}

uint16_t le_stack_init(void)
{
    uint16_t ret = 0;

#ifdef DUAL_BIS_MODE
    set_src_wed_vaild(1);
#endif

    uint8_t le_app_id = get_le_app_id();
    ret = gapm_configure_init(le_app_id);
    if (ret)
        return (ret << 8) | LE_ERR_GAPM_INIT;
#if   (BLE_LINK_SUPPORT)
    ble_gs_app_init();
#endif
#ifdef VENDOR_BLE_APP
    vendor_ble_app_init();
#endif

#ifdef LE_UDSS
    app_udss_init();
#endif

#if BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT
    le_bis_source_init();
#ifndef LE_BIS_ASSIS
    le_bis_sink_init();
#endif
#endif

#ifdef LE_UDSC
    app_le_udsc_init();
#endif

#ifdef LE_BIS_ASSIS
    le_bis_assis_init();
    Set_bis_role(BIS_SRC);
#endif

#ifdef LE_BIS_DELEG
    le_bis_delegator_init();
    Set_bis_role(BIS_SINK);
#endif
#ifdef LE_CIS_SINK
#ifndef CIS_APP_HEADSET_LR
#if 1
    cis_app_init(CIS_APP_EARBUD, 1);
#else
    cis_app_init(CIS_APP_EARBUD, 2);
#endif
#else
    cis_app_init(CIS_APP_HEADSET, 3);
#endif
#endif
#ifdef LE_CIS_SRC
    cis_app_init(CIS_APP_MEDIA_PLAYER, 0);
#endif
    return 0;
}
#endif
