/**
 *@file ble_gs_app.c
 * @author Cole (cole@mvsilicon.com)
 * @brief 
 * @version 0.1
 * @date 2026-01-20
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#include "bt_config.h"
#ifdef MV_APP_CONTROL
#include "mv_app_protocol_handler.h"
#endif
#if (BLE_SUPPORT)

#define BLE_HOST_PRESENT 1

#define GS_TEST_LEGACY_ADV

#if (BLE_DUAL_ROLE == ENABLE)
#define GS_CENTRAL_ENABLE
#endif

/************************************************************
 * BLE Central 工作模式选择
 *
 * 可选值：
 *   1 → 只扫描上报包（不连接）
 *   2 → 连接（不查询服务、不配对）
 *   3 → 连接——查询服务（不配对）
 *   4 → 连接配对绑定服务（完整功能，默认）
 ************************************************************/
#define BLE_CENTRAL_MODE_SCAN_ONLY      1
#define BLE_CENTRAL_MODE_CONNECT_ONLY   2
#define BLE_CENTRAL_MODE_CONNECT_DISC   3
#define BLE_CENTRAL_MODE_CONNECT_PAIR   4

#ifdef GS_CENTRAL_ENABLE
#define GS_CFG_ROLE  GAP_ROLE_LE_ALL
#define BLE_CENTRAL_MODE    BLE_CENTRAL_MODE_CONNECT_ONLY
#else
#define GS_CFG_ROLE  (GAP_ROLE_LE_PERIPHERAL | GAP_ROLE_LE_BROADCASTER)
#define BLE_CENTRAL_MODE    0
#endif

#if defined(GS_TEST_EXTENDED_ADV) || defined(GS_TEST_LONG_RANGE_ADV) || \
    defined(GS_TEST_PERIODIC_ADV) || defined(GS_TEST_ALL_ADV)
#include "ble_gs_ext_adv.h"
#endif

#include "ble_gs_service.h"
#ifdef GS_CENTRAL_ENABLE
#include "ble_gs_central.h"
#endif
#include "gapm.h"
#include "gapc.h"
#include "gapc_le.h"
#include "gapc_sec.h"
#include <string.h>
#include <stdio.h>
#include "hl_error.h"
#include "co_math.h"
#include "ble_app_gatt.h"
#include "bt_config.h"
#include "sys_param.h"
#include "bt_manager.h"
#ifdef BLE_TUYA_ENABLE
#include "app_message.h"
typedef uint16_t gs_err_t;
#endif
// mv_profile1[] 和 MV_profile_db_size 已由工具自动生成在 ble_app_gatt.c 中

#define GS_DEBUG_ENABLE
#ifdef GS_DEBUG_ENABLE
#define GS_DBG(fmt, ...)    printf("[GS] " fmt "\n", ##__VA_ARGS__)
#else
#define GS_DBG(fmt, ...)
#endif

#define GS_DEV_NAME         "MVS_BLE"
#define GS_DEV_NAME_LEN     7
#define GS_DEV_APPEARANCE   0x0000

#define GS_CONFIGURATION                                                                \
    {                                                                                   \
        .role = GS_CFG_ROLE,                                                            \
        .pairing_mode = GAPM_PAIRING_MODE_ALL,                                          \
        .privacy_cfg = 0,                                                               \
        .renew_dur = 1500,                                                              \
        .private_identity.addr = {0, 0, 0, 0, 0, 0},                                    \
        .irk.key = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},                    \
        .gap_start_hdl = 0,                                                             \
        .gatt_start_hdl = 0,                                                            \
        .att_cfg = 0,                                                                   \
        .sugg_max_tx_octets = GAP_LE_MAX_OCTETS,                                        \
        .sugg_max_tx_time = GAP_LE_MAX_TIME,                                            \
        .tx_pref_phy = GAP_PHY_ANY,                                                     \
        .rx_pref_phy = GAP_PHY_ANY,                                                     \
        .tx_path_comp = 0,                                                              \
        .rx_path_comp = 0,                                                              \
        .class_of_device = 0x200408,                                                    \
        .dflt_link_policy = 0,                                                          \
    }

enum {
    EVT_CFG_DONE = 0x100,
    EVT_CFG_NAME_SET,
    EVT_CFG_READ_ADDR,
};

#define GS_DISC_MAX_SVC   20
#define GS_DISC_MAX_CHAR  30
#define GS_DISC_MAX_DESC  10
 
typedef struct {
    uint8_t  state;      // 0=idle, 1=disc_svc, 2=disc_char, 3=disc_desc, 4=done
    uint8_t  svc_idx;    // current target service index in discovery chain
 
    // Service array
    struct {
        uint16_t start_hdl;
        uint16_t end_hdl;
    } svcs[GS_DISC_MAX_SVC];
    uint8_t svc_cnt;
 
    // Characteristic array
    struct {
        uint16_t char_hdl;
        uint16_t val_hdl;
        uint8_t  prop;
    } chars[GS_DISC_MAX_CHAR];
    uint8_t char_cnt;
 
    // Descriptor array
    struct {
        uint16_t desc_hdl;
    } descs[GS_DISC_MAX_DESC];
    uint8_t desc_cnt;
} gs_disc_t;
 
typedef struct {
    gap_bdaddr_t peer_addr;
    uint8_t conidx;
    bool paired;
    bool encrypted;
} gs_con_t;

// Discovery state machine
#define GS_DISC_IDLE  0
#define GS_DISC_SVC   1
#define GS_DISC_CHAR  2
#define GS_DISC_DESC  3
#define GS_DISC_DONE  4

typedef struct {
    gap_bdaddr_t addr;
    gapc_pairing_keys_t keys;
    bool valid;
} gs_bond_t;

static uint8_t gs_conidx_role_p = 0xFF;
static uint8_t gs_conidx_role_c = 0xFF;
static bool gs_ntf_en = false;
static uint16_t gs_ntf_hdl = 0;  // 动态分配的通知特征handle
static bool gs_configured = false;
static volatile bool gs_tx_done = true;  // 发送完成标志，初始为true表示可以发送
static bool gs_gapm_configured = false;
static uint8_t gs_dev_name[32] = GS_DEV_NAME;
static uint8_t gs_read_resp_test_data[3] = {0x11,0x22,0x33};
static uint8_t gs_dev_name_len = GS_DEV_NAME_LEN;
static gs_con_t gs_con;
static gs_disc_t gs_disc;
static gs_bond_t gs_bond;

static void gs_cfg_state(uint16_t event, uint16_t status, const void* p);
void ble_gs_configuration_cb_gapm_cmp(uint32_t event, uint16_t status);

static const char* gs_get_name(uint8_t* len) {
    if (len) *len = gs_dev_name_len;
    return (const char*)gs_dev_name;
}

static void gs_restart_adv(void) {
    if (!gs_configured) return;
#if defined(GS_TEST_EXTENDED_ADV)
    ble_gs_test_ext_adv_start();
#elif defined(GS_TEST_LONG_RANGE_ADV)
    ble_gs_test_ext_adv_2m();
#elif defined(GS_TEST_PERIODIC_ADV)
    ble_gs_test_periodic_adv_start();
#elif defined(GS_TEST_ALL_ADV)
    ble_gs_test_ext_adv_start();
#else
    ble_gs_adv_start();
#endif
}


// Central helper: start service discovery
#ifdef GS_CENTRAL_ENABLE
static void gs_start_svc_discovery(uint8_t conidx)
{
    if (gs_disc.state != GS_DISC_IDLE) return;
    memset(&gs_disc, 0, sizeof(gs_disc_t));
    gs_disc.state = GS_DISC_SVC;
    ble_gsc_err_t res = ble_gsc_discover_svc(conidx, BLE_GSC_DISC_ALL_SVC, 0x0001, 0xFFFF, 0, NULL);

    GS_DBG(" ble_gsc_discover_svc %d\n", res);
}

// Encryption completion callback for bonded device reconnection
static void gs_encrypt_cmp_cb(uint8_t conidx, uint32_t metainfo, uint16_t status)
{
    GS_DBG("Encrypt cmp: conidx=%d status=0x%04X", conidx, status);
    if (status == 0)
    {
        gs_con.encrypted = true;
#if (BLE_CENTRAL_MODE == BLE_CENTRAL_MODE_CONNECT_PAIR)
    	gs_start_svc_discovery(conidx);
#endif
    }
}



// Enable CCCD notification for a characteristic value handle
static void gs_cccd_set(uint8_t conidx, uint16_t desc_hdl, bool enable)
{
    uint8_t data[2] = { enable ? 0x01 : 0x00, 0x00 };
    ble_gsc_write(conidx, BLE_GSC_WRITE, desc_hdl, data, 2);
    GS_DBG("  -> CCCD %s on hdl 0x%04X", enable ? "ENABLED" : "DISABLED", desc_hdl);
}



static void gs_conn_pair_and_bond(uint8_t conidx,bool bonded)
{
	uint8_t sec_req_level =  GAP_SEC1_NOAUTH_PAIR_ENC;
	gapm_le_configure_security_level(sec_req_level);
    if (bonded)
    {
        uint16_t ret = gapc_le_encrypt(conidx, 0, &gs_bond.keys.ltk, gs_encrypt_cmp_cb);
        GS_DBG("Encrypting with stored LTK...%d\n", ret);
    }
    else
    {
        GS_DBG("gs app_le_start_pairing\n");
        gapc_pairing_t gs_pairing_info =
        {
                .iocap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT,
                .oob = GAP_OOB_AUTH_DATA_NOT_PRESENT,
                .auth = GAP_AUTH_BOND | GAP_AUTH_SEC_CON,
                .key_size = GAP_KEY_LEN,
                .ikey_dist = GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY,
                .rkey_dist = GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY,
        };
        gapc_le_bond(conidx, &gs_pairing_info, 0);
    }
}
#endif


static void gs_gap_conn_req(uint8_t conidx, uint32_t metainfo, uint8_t actv_idx,
                            uint8_t role, const gap_bdaddr_t* addr,
                            const gapc_le_con_param_t* param, uint8_t clk_acc)
{
    gapc_bond_data_t bond_data;
    bool bonded = false;
    
    GS_DBG("LE conn: conidx=%d addr=%02X:%02X:%02X:%02X:%02X:%02X",
            conidx, addr->addr[5], addr->addr[4], addr->addr[3],
            addr->addr[2], addr->addr[1], addr->addr[0]);
    gs_con.conidx = conidx;
    memcpy(&gs_con.peer_addr, addr, sizeof(gap_bdaddr_t));
    gs_con.paired = false;
    gs_con.encrypted = false;
    if (gs_bond.valid && memcmp(gs_bond.addr.addr, addr->addr, GAP_BD_ADDR_LEN) == 0)
    {
        bonded = true;
        bond_data.enc_key_present = true;
        bond_data.pairing_lvl = gs_bond.keys.pairing_lvl;
        gs_con.paired = true;
        GS_DBG("Reconnecting bonded device  %d",gs_bond.keys.pairing_lvl);
    }
    gapc_le_connection_cfm(conidx, 0, bonded ? &bond_data : NULL);

    if (role == ROLE_MASTER)
    {
    	gs_conidx_role_c = conidx;
    	ble_gsc_event_register(conidx, 0x0001, 0xFFFF);

#if (BLE_CENTRAL_MODE == BLE_CENTRAL_MODE_CONNECT_PAIR)
    	gs_conn_pair_and_bond(conidx, bonded);
#endif
    }
    else if(role == ROLE_SLAVE)
    {
    	gs_conidx_role_p = conidx;
    }

#if (BLE_CENTRAL_MODE == BLE_CENTRAL_MODE_CONNECT_DISC)
    gs_start_svc_discovery(conidx);
#endif
}

#ifdef GS_CENTRAL_ENABLE
static void gs_start_scan(void)
{
    if (ble_gsc_is_scanning())
        return;

    ble_gsc_scan_param_t sp = {
        .type        = BLE_GSC_SCAN_GENERAL,// 需要scan resp消息的时候可以使用BLE_GSC_SCAN_OBSERVER类型
        .phy         = BLE_GSC_PHY_1M,
        .filter      = BLE_GSC_FILT_DUP_DIS,
#if (BLE_CENTRAL_MODE == BLE_CENTRAL_MODE_SCAN_ONLY)
        .active_scan = false,
#else
        .active_scan = true,
#endif
        .scan_intv   = 160,
        .scan_wd     = 80,
        .duration    = 0,
        .period      = 0,
    };
    ble_gsc_err_t ret = ble_gsc_scan_start(&sp);
    GS_DBG("Central: restart scan ret=%d", ret);
}

#endif

static void gs_disconn_cleanup(uint8_t conidx)
{
    if (conidx == gs_conidx_role_p)
    {
        gs_conidx_role_p = 0xFF;
        gs_ntf_en = false;
        gs_ntf_hdl = 0;
        gs_tx_done = true;
        gs_restart_adv();
    }
    if (conidx == gs_conidx_role_c)
    {
        gs_conidx_role_c = 0xFF;
        gs_con.conidx = GAP_INVALID_CONIDX;
        gs_con.paired = false;
        gs_con.encrypted = false;
        memset(&gs_disc, 0, sizeof(gs_disc_t));
    #ifdef GS_CENTRAL_ENABLE
        gs_start_scan();
    #endif
    }
#ifdef BLE_TUYA_ENABLE
    tuya_ble_disconnected_handler();
#endif
}

static void gs_gap_disconn(uint8_t conidx, uint32_t metainfo, uint16_t reason) {
    GS_DBG("Disconnected: conidx=%d reason=0x%04X", conidx, reason);
    gs_disconn_cleanup(conidx);
}

static void gs_gap_name_get(uint8_t conidx, uint32_t metainfo, uint16_t token, 
                            uint16_t offset, uint16_t max_len)
{
    uint8_t len;
    const char* name = gs_get_name(&len);
    uint16_t status = (offset < len) ? GAP_ERR_NO_ERROR : ATT_ERR_INVALID_OFFSET;
    uint16_t remain = (offset < len) ? (len - offset) : 0;
    gapc_le_get_name_cfm(conidx, token, status, len, co_min(remain, max_len), (const uint8_t*)&name[offset]);
}

static void gs_gap_appear_get(uint8_t conidx, uint32_t metainfo, uint16_t token) {
    gapc_le_get_appearance_cfm(conidx, token, GAP_ERR_NO_ERROR, GS_DEV_APPEARANCE);
}

static const gapc_connection_req_cb_t gs_conn_req_cb = { .le_connection_req = gs_gap_conn_req };
static const gapc_connection_info_cb_t gs_conn_info_cb = {
    .disconnected = gs_gap_disconn,
    .name_get = gs_gap_name_get,
    .appearance_get = gs_gap_appear_get,
};

static void gs_gap_encrypt_req(uint8_t conidx, uint32_t metainfo, uint16_t ediv,
                               const gap_le_random_nb_t* rand)
{
    GS_DBG("Encrypt req: conidx=%d ediv=0x%04X", conidx, ediv);
    if (gs_bond.valid)
        gapc_le_encrypt_req_reply(conidx, true, &gs_bond.keys.ltk.key, gs_bond.keys.ltk.key_size);
    else
        gapc_le_encrypt_req_reply(conidx, false, NULL, 0);
}

static void gs_gap_auth_info(uint8_t conidx, uint32_t metainfo, uint8_t sec_lvl,
                             bool encrypted, uint8_t key_size)
{
    GS_DBG("Auth: conidx=%d encrypted=%d", conidx, encrypted);
    gs_con.encrypted = encrypted;
    if (encrypted)
    {
    	GS_DBG("gs_gap_auth_info (conidx=%d) =====", conidx);
    }
}

static void gs_gap_pair_ok(uint8_t conidx, uint32_t metainfo, uint8_t lvl, 
                           bool enc_key, uint8_t key_type)
{
    GS_DBG("Pairing OK: conidx=%d", conidx);
    gs_con.paired = true;
    GS_DBG("Pairing OK: conidx=%d lvl=%d enc_key=%d key_type=%d\n", conidx, lvl, enc_key, key_type);
#if (BLE_CENTRAL_MODE == BLE_CENTRAL_MODE_CONNECT_PAIR)
    gs_start_svc_discovery(conidx);
#endif
}

static void gs_gap_pair_fail(uint8_t conidx, uint32_t metainfo, uint16_t reason) {
    GS_DBG("Pairing failed: reason=0x%04X", reason);
    gs_con.paired = false;
}

static void gs_gap_info_req(uint8_t conidx, uint32_t metainfo, uint8_t info) {
    switch (info) {
        case GAPC_INFO_TK_DISPLAYED:
        case GAPC_INFO_TK_ENTERED:
        case GAPC_INFO_TK_OOB: {
            gap_sec_key_t tk = {.key = {0}};
            gapc_le_pairing_provide_tk(conidx, true, &tk);
        } break;
        case GAPC_INFO_PASSKEY_DISPLAYED:
        case GAPC_INFO_PASSKEY_ENTERED:
            gapc_pairing_provide_passkey(conidx, true, 123456);
            GS_DBG("Passkey: 123456");
            break;
        default: break;
    }
}

static void gs_gap_auth_req(uint8_t conidx, uint32_t metainfo, uint8_t auth) {}

static void gs_gap_pair_req(uint8_t conidx, uint32_t metainfo, uint8_t auth)
{
	GS_DBG("Pairing req: conidx=%d", conidx);
	gapm_le_configure_security_level(GAP_SEC1_NOAUTH_PAIR_ENC);
	gapc_pairing_t info =
	{
			.iocap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT,
			.oob = GAP_OOB_AUTH_DATA_NOT_PRESENT,
			.auth = GAP_AUTH_BOND ,
			.key_size = GAP_KEY_LEN,
			.ikey_dist = GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY,
			.rkey_dist = GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY,
	};
	gapc_le_pairing_accept(conidx, true, &info, 0);
}

static void gs_gap_numeric_cmp(uint8_t conidx, uint32_t metainfo, uint32_t val) {
    gapc_pairing_numeric_compare_rsp(conidx, true);
}

static void gs_gap_key_pressed(uint8_t conidx, uint32_t metainfo, uint8_t type) {}

static void gs_gap_ltk_req(uint8_t conidx, uint32_t metainfo, uint8_t key_size) {
    if (!gs_bond.valid)
    {
        gs_bond.keys.ltk.key_size = GAP_KEY_LEN;
        gs_bond.keys.ltk.ediv = (uint16_t)co_rand_word();
        for (uint8_t i = 0; i < GAP_KEY_LEN; i++)
            gs_bond.keys.ltk.key.key[i] = (uint8_t)co_rand_word();
        for (uint8_t i = 0; i < GAP_RAND_NB_LEN; i++)
            gs_bond.keys.ltk.randnb.nb[i] = (uint8_t)co_rand_word();
        GS_DBG("LTK req (new random)");
    } else {
        GS_DBG("LTK req (stored bond)");
    }
//    GS_DBG("  LTK:  %02X%02X%02X%02X-%02X%02X%02X%02X-%02X%02X%02X%02X-%02X%02X%02X%02X",
//            gs_bond.keys.ltk.key.key[15], gs_bond.keys.ltk.key.key[14],
//            gs_bond.keys.ltk.key.key[13], gs_bond.keys.ltk.key.key[12],
//            gs_bond.keys.ltk.key.key[11], gs_bond.keys.ltk.key.key[10],
//            gs_bond.keys.ltk.key.key[9],  gs_bond.keys.ltk.key.key[8],
//            gs_bond.keys.ltk.key.key[7],  gs_bond.keys.ltk.key.key[6],
//            gs_bond.keys.ltk.key.key[5],  gs_bond.keys.ltk.key.key[4],
//            gs_bond.keys.ltk.key.key[3],  gs_bond.keys.ltk.key.key[2],
//            gs_bond.keys.ltk.key.key[1],  gs_bond.keys.ltk.key.key[0]);
//    GS_DBG("  EDIV:  0x%04X", gs_bond.keys.ltk.ediv);
//    GS_DBG("  RAND:  %02X%02X%02X%02X%02X%02X%02X%02X",
//            gs_bond.keys.ltk.randnb.nb[7], gs_bond.keys.ltk.randnb.nb[6],
//            gs_bond.keys.ltk.randnb.nb[5], gs_bond.keys.ltk.randnb.nb[4],
//            gs_bond.keys.ltk.randnb.nb[3], gs_bond.keys.ltk.randnb.nb[2],
//            gs_bond.keys.ltk.randnb.nb[1], gs_bond.keys.ltk.randnb.nb[0]);
//    GS_DBG("  KEY_SIZE: %d", gs_bond.keys.ltk.key_size);
    gapc_le_pairing_provide_ltk(conidx, &gs_bond.keys.ltk);
}

static void gs_gap_key_recv(uint8_t conidx, uint32_t metainfo, const gapc_pairing_keys_t* keys) {
    gs_bond.keys = *keys;
    memcpy(&gs_bond.addr, &gs_con.peer_addr, sizeof(gap_bdaddr_t));
    gs_bond.valid = true;

    GS_DBG("=== Keys received [conidx=%d] ===", conidx);
    GS_DBG("  valid_key_bf=0x%02X, pairing_lvl=%d",
            keys->valid_key_bf, keys->pairing_lvl);

//    if (keys->valid_key_bf & GAP_KDIST_ENCKEY) {
//        GS_DBG("  LTK:  %02X%02X%02X%02X-%02X%02X%02X%02X-%02X%02X%02X%02X-%02X%02X%02X%02X",
//                keys->ltk.key.key[15], keys->ltk.key.key[14],
//                keys->ltk.key.key[13], keys->ltk.key.key[12],
//                keys->ltk.key.key[11], keys->ltk.key.key[10],
//                keys->ltk.key.key[9],  keys->ltk.key.key[8],
//                keys->ltk.key.key[7],  keys->ltk.key.key[6],
//                keys->ltk.key.key[5],  keys->ltk.key.key[4],
//                keys->ltk.key.key[3],  keys->ltk.key.key[2],
//                keys->ltk.key.key[1],  keys->ltk.key.key[0]);
//        GS_DBG("  EDIV:  0x%04X", keys->ltk.ediv);
//        GS_DBG("  RAND:  %02X%02X%02X%02X%02X%02X%02X%02X",
//                keys->ltk.randnb.nb[7], keys->ltk.randnb.nb[6],
//                keys->ltk.randnb.nb[5], keys->ltk.randnb.nb[4],
//                keys->ltk.randnb.nb[3], keys->ltk.randnb.nb[2],
//                keys->ltk.randnb.nb[1], keys->ltk.randnb.nb[0]);
//        GS_DBG("  KEY_SIZE: %d", keys->ltk.key_size);
//    }
//
//    if (keys->valid_key_bf & GAP_KDIST_IDKEY) {
//        GS_DBG("  IRK:  %02X%02X%02X%02X-%02X%02X%02X%02X-%02X%02X%02X%02X-%02X%02X%02X%02X",
//                keys->irk.key.key[15], keys->irk.key.key[14],
//                keys->irk.key.key[13], keys->irk.key.key[12],
//                keys->irk.key.key[11], keys->irk.key.key[10],
//                keys->irk.key.key[9],  keys->irk.key.key[8],
//                keys->irk.key.key[7],  keys->irk.key.key[6],
//                keys->irk.key.key[5],  keys->irk.key.key[4],
//                keys->irk.key.key[3],  keys->irk.key.key[2],
//                keys->irk.key.key[1],  keys->irk.key.key[0]);
//        GS_DBG("  IDENTITY: 0x%02X:%02X:%02X:%02X:%02X:%02X (%s)",
//                keys->irk.identity.addr[5], keys->irk.identity.addr[4],
//                keys->irk.identity.addr[3], keys->irk.identity.addr[2],
//                keys->irk.identity.addr[1], keys->irk.identity.addr[0],
//                keys->irk.identity.addr_type == GAP_ADDR_PUBLIC ? "Public" : "Private");
//    }
//
//    if (keys->valid_key_bf & GAP_KDIST_SIGNKEY) {
//        GS_DBG("  CSRK: %02X%02X%02X%02X-%02X%02X%02X%02X-%02X%02X%02X%02X-%02X%02X%02X%02X",
//                keys->csrk.key[15], keys->csrk.key[14],
//                keys->csrk.key[13], keys->csrk.key[12],
//                keys->csrk.key[11], keys->csrk.key[10],
//                keys->csrk.key[9],  keys->csrk.key[8],
//                keys->csrk.key[7],  keys->csrk.key[6],
//                keys->csrk.key[5],  keys->csrk.key[4],
//                keys->csrk.key[3],  keys->csrk.key[2],
//                keys->csrk.key[1],  keys->csrk.key[0]);
//    }
//    GS_DBG("================================");
}

static void gs_gap_repeat(uint8_t conidx, uint32_t metainfo) {}

static const gapc_security_cb_t gs_sec_cb = {
    .le_encrypt_req = gs_gap_encrypt_req,
    .auth_info = gs_gap_auth_info,
    .pairing_succeed = gs_gap_pair_ok,
    .pairing_failed = gs_gap_pair_fail,
    .info_req = gs_gap_info_req,
    .auth_req = gs_gap_auth_req,
    .pairing_req = gs_gap_pair_req,
    .numeric_compare_req = gs_gap_numeric_cmp,
    .key_pressed = gs_gap_key_pressed,
    .ltk_req = gs_gap_ltk_req,
    .key_received = gs_gap_key_recv,
    .repeated_attempt = gs_gap_repeat,
};

static void gs_gap_param_req(uint8_t conidx, uint32_t metainfo, const gapc_le_con_param_nego_t* p) {
    GS_DBG("Param req: interval=[%d,%d]", p->interval_min, p->interval_max);
    gapc_le_update_params_cfm(conidx, true, 0, 0);
}

static void gs_gap_param_upd(uint8_t conidx, uint32_t metainfo, const gapc_le_con_param_t* p) {
    GS_DBG("Param updated: interval=%d", p->interval);
}

static void gs_gap_pkt_size(uint8_t conidx, uint32_t metainfo, uint16_t tx_oct, uint16_t tx_time,
                            uint16_t rx_oct, uint16_t rx_time) {}

static void gs_gap_phy_upd(uint8_t conidx, uint32_t metainfo, uint8_t tx, uint8_t rx) {
    GS_DBG("PHY: tx=%d rx=%d", tx, rx);
}

static const gapc_le_config_cb_t gs_le_cfg_cb = {
    .param_update_req = gs_gap_param_req,
    .param_updated = gs_gap_param_upd,
    .packet_size_updated = gs_gap_pkt_size,
    .phy_updated = gs_gap_phy_upd,
};

static void gs_gap_hw_err(uint32_t metainfo, uint8_t code) {
    GS_DBG("HW Error: 0x%02X", code);
}

static const gapm_cb_t gs_gapm_cb = { .cb_hw_error = gs_gap_hw_err };

gapm_callbacks_t ble_gs_gap_cbs = {
    .p_con_req_cbs = &gs_conn_req_cb,
    .p_sec_cbs = &gs_sec_cb,
    .p_info_cbs = &gs_conn_info_cb,
    .p_le_config_cbs = &gs_le_cfg_cb,
    .p_gapm_cbs = &gs_gapm_cb,
};

static void gs_rx_cb(uint8_t conidx, uint16_t handle, uint16_t len, const uint8_t* data) {
    printf("[GS] RX: conidx=%d hdl=0x%04X len=%d data=", conidx, handle, len);
    for (uint16_t i = 0; i < len && i < 16; i++) printf("%02X ", data[i]);
    if (len > 16) printf("...");
    printf("\n");

#ifdef BLE_TUYA_ENABLE
        if( (handle >= (0xf+0x11)) && (handle <= (0x14+0x11)))
    	{
        	tuya_ble_gatt_receive_data(data,len);
    	}
#endif
#ifdef MV_APP_CONTROL
    mvble_data_received(data,len);
#endif
}
static void gs_cb_read(uint8_t conidx, uint16_t handle, uint16_t offset, uint16_t max_len, uint32_t token)
{
    printf("[GS] cb read: conidx=%d hdl=0x%04X ", conidx, handle);
    switch (handle)
    {
    case UUID_2A00_DEVICE_NAME_HANDLE:
    	gs_read_cfm(conidx, token, GAP_ERR_NO_ERROR,  gs_dev_name_len, gs_dev_name);
    	break;

    default:
    	BLE_INFO("\n*****test_buff*****\n");
    	gs_read_cfm(conidx, token, GAP_ERR_NO_ERROR, sizeof(gs_read_resp_test_data), gs_read_resp_test_data);// 响应数据客户可根据实际应用的句柄修改
    	break;
    }

}

static void gs_cccd_cb(uint8_t conidx, uint16_t handle, uint16_t cfg) {
    GS_DBG("CCCD: hdl=0x%04X cfg=0x%04X", handle, cfg);
    gs_ntf_en = (cfg & 0x0003) ? true : false;
    if (gs_ntf_en) {
        gs_ntf_hdl = handle - 1;  // Value handle = CCCD handle - 1
        GS_DBG("Notify ON, value_hdl=0x%04X", gs_ntf_hdl);
    } else {
        GS_DBG("Notify OFF");
    }
}

static void gs_ntf_cmp_cb(uint8_t conidx, uint16_t status) {
    (void)conidx; (void)status;
    gs_tx_done = true;  // 标记发送完成
    
    // 通知测试模块
    extern void ble_gs_test_on_tx_done(void);
    ble_gs_test_on_tx_done();
}

static void gs_conn_cb(uint8_t conidx, bool connected, uint16_t reason) {
    if (connected) {
        GS_DBG("GATT Connected: conidx=%d", conidx);
       // gs_conidx = conidx;
        gs_ntf_en = false;
        gs_ntf_hdl = 0;
        gs_tx_done = true;
#ifdef BLE_TUYA_ENABLE
		tuya_ble_connected_handler();
#endif
    } else {
        GS_DBG("GATT Disconnected: conidx=%d reason=0x%04X", conidx, reason);
        gs_disconn_cleanup(conidx);
    }
}

#ifdef BLE_TUYA_ENABLE
uint8_t ble_set_adv_flag = 0;
#endif

#ifdef GS_CENTRAL_ENABLE
static void gsc_init_and_scan(void);
#endif

static void gs_adv_cb(bool started, uint16_t status) {
    GS_DBG("ADV %s: 0x%04X", started ? "started" : "stopped", status);

#ifdef GS_CENTRAL_ENABLE
    if (started && status == 0) {
        gsc_init_and_scan();
    }
#endif

#ifdef BLE_TUYA_ENABLE
    if(!started && ble_set_adv_flag)
    {
    	tuya_ble_app_init();
    	ble_set_adv_flag = 0;
    }
#endif
}

#ifdef GS_CENTRAL_ENABLE
static void gsc_evt_handler(const ble_gsc_evt_t* p_evt)
{
    switch (p_evt->type)
    {
        case BLE_GSC_EVT_SCAN_STARTED:
            GS_DBG("Central: scan started st=0x%04X", p_evt->data.status);
            break;

        case BLE_GSC_EVT_SCAN_STOPPED:
            GS_DBG("Central: scan stopped st=0x%04X", p_evt->data.status);
            break;

        case BLE_GSC_EVT_ADV_REPORT:
        {
            const ble_gsc_adv_report_t* rpt = &p_evt->data.adv_report;
            char name[32];
            uint8_t name_len = 0;
            const uint8_t* d = rpt->p_data;
            uint16_t remain = rpt->data_len;
            while (remain >= 2) {
                uint8_t field_len = d[0];
                if (field_len == 0 || field_len > remain - 1) break;
                uint8_t ad_type = d[1];
                if (ad_type == 0x09 || ad_type == 0x08) {
                    name_len = field_len - 1;
                    if (name_len > sizeof(name) - 1) name_len = sizeof(name) - 1;
                    memcpy(name, &d[2], name_len);
                    name[name_len] = '\0';
                    if (ad_type == 0x09) break;
                }
                remain -= (field_len + 1);
                d += (field_len + 1);
            }
            if (name_len == 0) { name[0] = '?'; name[1] = '\0'; }

            GS_DBG("Central ADV_REPORT\n");
            if((name_len == 0) || (memcmp("B6_BLE",name,name_len) != 0))// 根据项目实际扫描的从机修改名称
            {
#if (BLE_CENTRAL_MODE == BLE_CENTRAL_MODE_CONNECT_PAIR)
            	if((!gs_bond.valid) || memcmp(gs_bond.addr.addr,rpt->addr.addr, BLE_GSC_BD_ADDR_LEN))
#endif
            	{
            		break;
            	}
            }
            GS_DBG("Central: [%s] %02X:%02X:%02X:%02X:%02X:%02X  rssi:%-4d %s%s%s",
                    name,
                    rpt->addr.addr[5], rpt->addr.addr[4], rpt->addr.addr[3],
                    rpt->addr.addr[2], rpt->addr.addr[1], rpt->addr.addr[0],
                    rpt->rssi,
                    rpt->connectable ? "CONN " : "",
                    rpt->scannable   ? "SCAN " : "",
                    rpt->directed    ? "DIR"   : "");
#if (BLE_CENTRAL_MODE != BLE_CENTRAL_MODE_SCAN_ONLY)
            ble_gsc_connect_param_t con_param =
            {
            		.prop = BLE_GSC_INIT_PROP_1M,
					.conn_to = 0,
					.scan_intv = 40,// 50ms
					.scan_wd = 40 ,
					.conn_param.conn_intv_min = 40,
					.conn_param.conn_intv_max = 40, // 50ms
					.conn_param.conn_latency = 5,
					.conn_param.sup_timeout = 500, // 5000 ms
					.conn_param.ce_len_min = 5,
					.conn_param.ce_len_max = 10,
            };
            memcpy(con_param.peer_addr.addr,  rpt->addr.addr, BLE_GSC_BD_ADDR_LEN);
            con_param.peer_addr_type = rpt->addr_type;
            ble_gsc_scan_stop();
            ble_gsc_connect(&con_param);
#endif
            break;
        }

        case BLE_GSC_EVT_CONNECTING:
            GS_DBG("Central: connecting...");
            break;

        case BLE_GSC_EVT_CONNECT_FAILED:
            GS_DBG("Central: connect failed st=0x%04X", p_evt->data.status);
            break;

        case BLE_GSC_EVT_DISC_CMP:
            GS_DBG("Central: disc cmp disc_state=%d st=0x%04X", gs_disc.state, p_evt->data.status);

            if (gs_disc.state == GS_DISC_SVC)
            {
                if (gs_disc.svc_cnt > 0) {
                    // All services found → start CHAR discovery service by service
                    gs_disc.state = GS_DISC_CHAR;
                    gs_disc.svc_idx = 0;
                    ble_gsc_discover_char(p_evt->conidx,
                        gs_disc.svcs[0].start_hdl,
                        gs_disc.svcs[0].end_hdl,
                        0, NULL);
                } else {
                    gs_disc.state = GS_DISC_DONE;
                    GS_DBG("Central: no services found, discovery skip to done");
                }
            }
            else if (gs_disc.state == GS_DISC_CHAR)
            {
                gs_disc.svc_idx++;

                if (gs_disc.svc_idx < gs_disc.svc_cnt) {
                    ble_gsc_discover_char(p_evt->conidx,
                        gs_disc.svcs[gs_disc.svc_idx].start_hdl,
                        gs_disc.svcs[gs_disc.svc_idx].end_hdl,
                        0, NULL);
                } else if (gs_disc.char_cnt > 0) {
                    // All services CHAR discovery done → discover ALL descriptors
                    gs_disc.state = GS_DISC_DESC;
                    uint16_t min_hdl = 0xFFFF, max_hdl = 0;
                    for (uint8_t i = 0; i < gs_disc.svc_cnt; i++) {
                        if (gs_disc.svcs[i].start_hdl < min_hdl) min_hdl = gs_disc.svcs[i].start_hdl;
                        if (gs_disc.svcs[i].end_hdl   > max_hdl) max_hdl = gs_disc.svcs[i].end_hdl;
                    }
                    GS_DBG("  -> desc discovery across all services 0x%04X-0x%04X", min_hdl, max_hdl);
                    ble_gsc_discover_desc(p_evt->conidx, min_hdl, max_hdl);
                } else {
                    gs_disc.state = GS_DISC_DONE;
                    GS_DBG("Central: no characteristics found on any service, skip desc");
                }
            }
            else if (gs_disc.state == GS_DISC_DESC) {
                gs_disc.state = GS_DISC_DONE;
                GS_DBG("Central: ===== full discovery complete =====");
            }
            break;

        case BLE_GSC_EVT_SVC_FOUND:
            if (gs_disc.state == GS_DISC_SVC && gs_disc.svc_cnt < GS_DISC_MAX_SVC) {
                gs_disc.svcs[gs_disc.svc_cnt].start_hdl = p_evt->data.svc.start_hdl;
                gs_disc.svcs[gs_disc.svc_cnt].end_hdl   = p_evt->data.svc.end_hdl;
                gs_disc.svc_cnt++;
            }
            if (p_evt->data.svc.uuid_type == 0) {
                uint16_t uuid16 = (p_evt->data.svc.uuid[1] << 8) | p_evt->data.svc.uuid[0];
                GS_DBG("Central: svc 0x%04X [0x%04X-0x%04X]", uuid16,
                        p_evt->data.svc.start_hdl, p_evt->data.svc.end_hdl);

            } else if (p_evt->data.svc.uuid_type == 2) {
                GS_DBG("Central: svc %02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X [0x%04X-0x%04X]",
                        p_evt->data.svc.uuid[15], p_evt->data.svc.uuid[14],
                        p_evt->data.svc.uuid[13], p_evt->data.svc.uuid[12],
                        p_evt->data.svc.uuid[11], p_evt->data.svc.uuid[10],
                        p_evt->data.svc.uuid[9],  p_evt->data.svc.uuid[8],
                        p_evt->data.svc.uuid[7],  p_evt->data.svc.uuid[6],
                        p_evt->data.svc.uuid[5],  p_evt->data.svc.uuid[4],
                        p_evt->data.svc.uuid[3],  p_evt->data.svc.uuid[2],
                        p_evt->data.svc.uuid[1],  p_evt->data.svc.uuid[0],
                        p_evt->data.svc.start_hdl, p_evt->data.svc.end_hdl);
            }
            break;

        case BLE_GSC_EVT_CHAR_FOUND:
            if (gs_disc.state == GS_DISC_CHAR && gs_disc.char_cnt < GS_DISC_MAX_CHAR) {
                gs_disc.chars[gs_disc.char_cnt].char_hdl = p_evt->data.charac.char_hdl;
                gs_disc.chars[gs_disc.char_cnt].val_hdl  = p_evt->data.charac.val_hdl;
                gs_disc.chars[gs_disc.char_cnt].prop     = p_evt->data.charac.prop;
                gs_disc.char_cnt++;
            }
            GS_DBG("Central: char hdl=0x%04X val=0x%04X prop=0x%02X",
                    p_evt->data.charac.char_hdl,
                    p_evt->data.charac.val_hdl,
                    p_evt->data.charac.prop);
            break;

        case BLE_GSC_EVT_DESC_FOUND:
        	if (p_evt->data.desc.uuid_type == 0) {
        		uint16_t desc_uuid = (p_evt->data.desc.uuid[1] << 8) | p_evt->data.desc.uuid[0];
        		if (desc_uuid == 0x2902) {
                    if (gs_disc.state == GS_DISC_DESC && gs_disc.desc_cnt < GS_DISC_MAX_DESC) {
                        gs_disc.descs[gs_disc.desc_cnt].desc_hdl = p_evt->data.desc.desc_hdl;
                        gs_disc.desc_cnt++;
                    }
        			gs_cccd_set(p_evt->conidx, p_evt->data.desc.desc_hdl, true);
        			GS_DBG("Central: CCCD set at 0x%04X", p_evt->data.desc.desc_hdl);
        		}
        	}
            break;

        case BLE_GSC_EVT_READ_CMP:
            GS_DBG("Central: read cmp st=0x%04X", p_evt->data.status);
            break;

        case BLE_GSC_EVT_READ_VAL:
            GS_DBG("Central: read hdl=0x%04X len=%d",
                    p_evt->data.read.hdl, p_evt->data.read.data_len);
            break;

        case BLE_GSC_EVT_WRITE_CMP:
            GS_DBG("Central: write cmp st=0x%04X", p_evt->data.status);
            break;

        case BLE_GSC_EVT_NOTIFICATION:
            GS_DBG("Central: ntf hdl=0x%04X len=%d",
                    p_evt->data.ntf_ind.hdl, p_evt->data.ntf_ind.data_len);
            break;

        case BLE_GSC_EVT_INDICATION:
            GS_DBG("Central: ind hdl=0x%04X len=%d",
                    p_evt->data.ntf_ind.hdl, p_evt->data.ntf_ind.data_len);
            break;

        default:
            break;
    }
}

static bool gsc_inited = false;

static void gsc_init_and_scan(void)
{
	if (gsc_inited)
	{
	   return;
	}
	ble_gsc_callbacks_t cbs = { .cb_evt = gsc_evt_handler };
	ble_gsc_err_t ret = ble_gsc_init(&cbs);
	if (ret != BLE_GSC_OK)
	{
		GS_DBG("Central: init fail %d", ret);
		return;
	}
	gsc_inited = true;
	gs_start_scan();
}
#endif

void ble_gs_configuration_cb_gapm_cmp(uint32_t event, uint16_t status) {
    gs_cfg_state((uint16_t)event, status, NULL);
}

static void gs_cfg_state(uint16_t event, uint16_t status, const void* p) {
    GS_DBG("Cfg: evt=0x%04X st=0x%04X", event, status);
    if (status != GAP_ERR_NO_ERROR) return;

    switch (event) {
        case EVT_CFG_DONE:
            gapm_set_name(EVT_CFG_NAME_SET, gs_dev_name_len, gs_dev_name, ble_gs_configuration_cb_gapm_cmp);
            break;
        case EVT_CFG_NAME_SET:
            gapm_get_controller_bdaddr(EVT_CFG_READ_ADDR, ble_gs_configuration_cb_gapm_cmp);
            break;
        case EVT_CFG_READ_ADDR:
            gs_gapm_configured = true;
            gs_configured = true;
            GS_DBG("Config done");
            
            ble_gs_callbacks_t cbs = {
                .cb_rx = gs_rx_cb,
                .cb_cccd = gs_cccd_cb,
                .cb_conn = gs_conn_cb,
                .cb_ntf_cmp = gs_ntf_cmp_cb,  // 通知完成回调
				.cb_read = gs_cb_read,
            };
            if (ble_gs_init((const ble_gs_profile_t*)mv_profile1, MV_profile_db_size, &cbs) != BLE_GS_OK) {
                GS_DBG("ble_gs_init failed");
                return;
            }
            
            ble_gs_set_dev_name(gs_dev_name, gs_dev_name_len);
            
            uint8_t adv[BLE_GS_ADV_DATA_MAX];
            uint16_t uuid = UUID16_AB00_SERVICE;
            uint8_t len = ble_gs_build_adv_data(adv, gs_dev_name, gs_dev_name_len, &uuid, 1, NULL);
            ble_gs_set_adv_data(adv, len);
            
            uint8_t rsp[BLE_GS_ADV_DATA_MAX];
            uint8_t rlen = 0;
            rsp[rlen++] = 2; rsp[rlen++] = 0x0A; rsp[rlen++] = 0x08;
            rsp[rlen++] = 5; rsp[rlen++] = 0xFF; rsp[rlen++] = 0x4D; rsp[rlen++] = 0x56; rsp[rlen++] = 0x01; rsp[rlen++] = 0x00;
            ble_gs_set_scan_rsp(rsp, rlen);
            
            ble_gs_set_adv_callback(gs_adv_cb);
            
#if defined(GS_TEST_ALL_ADV)
            GS_DBG("=== All ADV Test ===");
            ble_gs_test_all_adv_modes();
#elif defined(GS_TEST_EXTENDED_ADV)
            GS_DBG("=== Ext ADV (2M) ===");
            ble_gs_test_ext_adv_start();
#elif defined(GS_TEST_LONG_RANGE_ADV)
            GS_DBG("=== Coded PHY ADV ===");
            ble_gs_test_ext_adv_2m();
#elif defined(GS_TEST_PERIODIC_ADV)
            GS_DBG("=== Periodic ADV ===");
            ble_gs_test_periodic_adv_start();
#else
            GS_DBG("ADV start: %d", ble_gs_adv_start());
#endif
            break;
        default:
            break;
    }
}

const gapm_config_t ble_gs_configuration = GS_CONFIGURATION;

void ble_gs_app_init(void) {
    GS_DBG("ble_gs_app_init");

    //printf("ble_gs_app_init: BLE Device Name = %s\n", sys_parameter.ble_LocalDeviceName);

    extern BT_CONFIGURATION_PARAMS		*btStackConfigParams;

    if(btStackConfigParams)
    {
        gs_dev_name_len = strlen((char*)btStackConfigParams->ble_LocalDeviceName);
        memcpy(gs_dev_name,btStackConfigParams->ble_LocalDeviceName,gs_dev_name_len);
    }

    memset(&gs_con, 0, sizeof(gs_con));
    gs_con.conidx = GAP_INVALID_CONIDX;
    memset(&gs_bond, 0, sizeof(gs_bond));
    gs_conidx_role_p = 0xFF;
    gs_conidx_role_c = 0xFF;
    gs_ntf_en = false;
    gs_configured = false;
    gs_gapm_configured = false;
#ifdef MV_APP_CONTROL
    Mv_AppProtocol_Init();
#endif
}

int ble_gs_app_send(const uint8_t* data, uint16_t len) {
    if (gs_conidx_role_p == 0xFF) {
        return -1;  
    }
    if (!gs_ntf_en) {
        return -1;  
    }
    if (gs_ntf_hdl == 0) {
        return -1; 
    }
    
    ble_gs_err_t ret = ble_gs_notify(gs_conidx_role_p, gs_ntf_hdl, data, len);
    if (ret != BLE_GS_OK) {
        if (ret == 7) {
            return -2;  
        }
        return -1;  
    }
    return 0;  
}

bool ble_gs_app_is_connected(void) {
    return gs_conidx_role_p != 0xFF && ble_gs_is_connected(gs_conidx_role_p);
}

bool ble_gs_app_is_configured(void) { return gs_configured; }

uint16_t ble_gs_app_start_adv(void) {
    if (!gs_configured) return 0xFF;
    return (uint16_t)ble_gs_adv_start();
}

uint16_t ble_gs_app_stop_adv(void) { return (uint16_t)ble_gs_adv_stop(); }

void ble_gs_app_set_name(const uint8_t* name, uint8_t len) {
    if (name && len > 0 && len < sizeof(gs_dev_name)) {
        memcpy(gs_dev_name, name, len);
        gs_dev_name[len] = '\0';
        gs_dev_name_len = len;
        if (gs_gapm_configured) gapm_set_name(0, len, gs_dev_name, NULL);
    }
}

bool ble_gs_app_is_paired(void) { return gs_con.paired; }
bool ble_gs_app_is_encrypted(void) { return gs_con.encrypted; }
uint8_t ble_gs_app_get_role_p_conidx(void) { return gs_conidx_role_p; }
bool ble_gs_app_is_ntf_enabled(void) { return gs_ntf_en; }

void ble_gs_app_clear_bond(void) {
    memset(&gs_bond, 0, sizeof(gs_bond));
    GS_DBG("Bond cleared");
}

static void gs_disconnect_cmp(uint8_t conidx, uint32_t metainfo, uint16_t status)
{
    GS_DBG("Disconnect cmd completed: conidx=%d status=%d", conidx, status);
}

void ble_gs_app_disconnect(uint8_t conidx)
{
    GS_DBG("Disconnect conidx=%d", conidx);
    gapc_disconnect(conidx, 0, LL_ERR_REMOTE_USER_TERM_CON, gs_disconnect_cmp);
}

#ifdef BLE_TUYA_ENABLE
extern BT_CONFIGURATION_PARAMS		*btStackConfigParams;
/***********************************************************************************
 * get device ble addr (max 40bytes)
 **********************************************************************************/
uint8_t* BtDeviceBleAddrGet(void)
{
	return &btStackConfigParams->ble_LocalDeviceAddr[0];
}

int32_t BtDeviceBleAddrSet(uint8_t* deviceAddr)
{
	memcpy(btStackConfigParams->ble_LocalDeviceAddr, deviceAddr, BT_ADDR_SIZE);
	user_set_ble_bd_addr(deviceAddr);
}

uint8_t tuya_ble_send_flag = 0;
uint8_t tuya_ble_data[244];
uint8_t tuya_ble_data_len = 0;
int8_t	tuya_ble_send_state = 0;
void TuyaBleStackSendData(void)
{
	printf("##TuyaBleStackSendData\n");
	//tuya_ble_send_state = ble_send_data(0,0,0,UUID_00000002_0000_1001_8001_00805F9B07D0_HANDLE,(uint8_t*)tuya_ble_data,(uint16_t)tuya_ble_data_len,0x00);
	extern gs_err_t gs_notify_send(uint8_t conidx, uint16_t handle, uint16_t len, const uint8_t* data);
	tuya_ble_send_state = gs_notify_send(0,UUID_00000002_0000_1001_8001_00805F9B07D0_HANDLE+0x11,(uint16_t)tuya_ble_data_len,(uint8_t*)tuya_ble_data);
	tuya_ble_send_flag = 0;
}

int TuyaBleSendData(uint8_t*p_data,uint16_t data_len)
{
	printf("##TuyaBleSendData %d \n",tuya_ble_send_flag);
	while(tuya_ble_send_flag)
	{
		osTaskDelay(5);
	};

	memcpy(&tuya_ble_data,p_data,data_len);
	tuya_ble_data_len = data_len;
	printf("##MSG_BTSTACK_MSG_BLE_TUYA_DATA_SEND %d \n",tuya_ble_send_flag);
	tuya_ble_send_flag = 1;
	BtStackServiceMsgSend(MSG_BTSTACK_MSG_BLE_TUYA_DATA_SEND);

	while(tuya_ble_send_flag)
	{
		osTaskDelay(5);
	};
	return tuya_ble_send_state;
//	return ble_send_data(0,0,0,UUID_00000002_0000_1001_8001_00805F9B07D0_HANDLE,(uint8_t*)p_data,(uint16_t)data_len,0x00);
}
#endif

#endif /* BLE_SUPPORT */
