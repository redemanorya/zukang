/**
 * @file ble_gs_service.c
 * @brief BLE Generic Service 适配层 - 精简版
 */
#include "bt_config.h"
#if (BLE_SUPPORT)

#include "ble_gs_service.h"
#include <string.h>
#include <stdio.h>

typedef uint16_t gs_err_t;
#define GS_ERR_NO_ERROR     0x0000
#define GS_ERR_NOT_ALLOWED  0x0005

typedef struct gs_callbacks {
    void (*cb_read)(uint8_t, uint16_t, uint16_t, uint16_t, uint32_t);
    void (*cb_write)(uint8_t, uint16_t, uint16_t, uint16_t, const uint8_t*, uint32_t);
    void (*cb_cccd_upd)(uint8_t, uint16_t, uint16_t);
    void (*cb_ntf_cmp)(uint8_t, uint16_t);
    void (*cb_ind_cmp)(uint8_t, uint16_t);
    void (*cb_connect)(uint8_t);
    void (*cb_disconnect)(uint8_t, uint16_t);
    void (*cb_pair_complete)(uint8_t, uint16_t);
    void (*cb_mtu_update)(uint8_t, uint16_t);
} gs_callbacks_t;

typedef enum {
    GS_IO_CAP_NO_INPUT_NO_OUTPUT = 3,
} gs_io_cap_t;

typedef struct gs_pair_param {
    gs_io_cap_t io_cap;
    bool oob_support;
    bool auth_req;
    bool bond_req;
    bool secure_con;
    uint8_t key_size_min;
    uint8_t key_size_max;
    bool dist_irk;
    bool dist_csrk;
} gs_pair_param_t;

typedef enum {
    GS_ADV_TYPE_LEGACY_CONN_SCAN = 0,
} gs_adv_type_t;

typedef enum {
    GS_ADDR_PUBLIC = 0,
} gs_addr_type_t;

#define GS_BD_ADDR_LEN 6
typedef struct gs_bd_addr {
    uint8_t addr[GS_BD_ADDR_LEN];
} gs_bd_addr_t;

typedef struct gs_adv_param {
    gs_adv_type_t adv_type;
    uint16_t adv_intv_min;
    uint16_t adv_intv_max;
    uint8_t ch_map;
    gs_addr_type_t own_addr_type;
    gs_bd_addr_t peer_addr;
    gs_addr_type_t peer_addr_type;
    uint16_t timeout_s;
    int8_t max_tx_pwr;
} gs_adv_param_t;

#define GS_ADV_DATA_MAX_LEN      31
#define GS_SCAN_RSP_DATA_MAX_LEN 31
#define GS_EXT_ADV_DATA_MAX_LEN  254

typedef struct gs_adv_data {
    uint8_t adv_data[GS_ADV_DATA_MAX_LEN];
    uint8_t adv_data_len;
    uint8_t scan_rsp_data[GS_SCAN_RSP_DATA_MAX_LEN];
    uint8_t scan_rsp_len;
} gs_adv_data_t;

typedef enum {
    GS_PHY_1M = 0x01,
    GS_PHY_2M = 0x02,
    GS_PHY_CODED = 0x04,
} gs_phy_type_t;

typedef struct gs_ext_adv_param {
    uint8_t adv_type;
    uint16_t adv_intv_min;
    uint16_t adv_intv_max;
    uint8_t ch_map;
    gs_addr_type_t own_addr_type;
    gs_bd_addr_t peer_addr;
    gs_addr_type_t peer_addr_type;
    uint16_t timeout_s;
    int8_t max_tx_pwr;
    gs_phy_type_t primary_phy;
    gs_phy_type_t secondary_phy;
    uint8_t adv_sid;
} gs_ext_adv_param_t;

typedef struct gs_ext_adv_data {
    uint8_t* p_adv_data;
    uint16_t adv_data_len;
    uint8_t* p_scan_rsp;
    uint16_t scan_rsp_len;
} gs_ext_adv_data_t;

typedef enum {
    GS_LINK_EVT_ADV_STARTED = 0,
    GS_LINK_EVT_ADV_STOPPED,
    GS_LINK_EVT_PAIR_REQ = 7,
} gs_link_evt_type_t;

typedef struct gs_link_evt {
    gs_link_evt_type_t type;
    union {
        struct { uint8_t conidx; uint16_t status; } pair;
        uint16_t adv_status;
    } data;
} gs_link_evt_t;

typedef void (*gs_link_evt_cb_t)(const gs_link_evt_t*);
typedef struct gs_link_callbacks {
    gs_link_evt_cb_t cb_link_evt;
    void* cb_bond_store;
    void* cb_bond_load;
} gs_link_callbacks_t;

typedef struct gs_conn_param {
    uint16_t conn_intv;
    uint16_t slave_latency;
    uint16_t sup_timeout;
} gs_conn_param_t;

typedef struct gs_periodic_adv_param {
    uint16_t adv_intv_min;
    uint16_t adv_intv_max;
    uint16_t period_intv_min;
    uint16_t period_intv_max;
    uint8_t ch_map;
    gs_addr_type_t own_addr_type;
    int8_t max_tx_pwr;
    gs_phy_type_t primary_phy;
    gs_phy_type_t secondary_phy;
    uint8_t adv_sid;
} gs_periodic_adv_param_t;

typedef struct gs_periodic_adv_data {
    uint8_t* p_ext_adv_data;
    uint16_t ext_adv_data_len;
    uint8_t* p_periodic_data;
    uint16_t periodic_data_len;
} gs_periodic_adv_data_t;

#define GS_ADV_CH_ALL 0x07

extern gs_err_t gs_services_init(const void* p_profiles, uint16_t cnt, const gs_callbacks_t* p_cbs);
extern gs_err_t gs_read_cfm(uint8_t conidx, uint32_t token, uint16_t status, uint16_t len, const uint8_t* data);
extern gs_err_t gs_write_cfm(uint8_t conidx, uint32_t token, uint16_t status);
extern gs_err_t gs_notify_send(uint8_t conidx, uint16_t handle, uint16_t len, const uint8_t* data);
extern gs_err_t gs_indicate_send(uint8_t conidx, uint16_t handle, uint16_t len, const uint8_t* data);
extern uint16_t gs_get_mtu(uint8_t conidx);
extern bool gs_is_connected(uint8_t conidx);
extern gs_err_t gs_link_init(const gs_link_callbacks_t* p_cbs);
extern gs_err_t gs_link_set_pair_param(const gs_pair_param_t* p_param);
extern gs_err_t gs_link_set_dev_name(const uint8_t* name, uint8_t len);
extern gs_err_t gs_link_set_adv_data(const gs_adv_data_t* p_data);
extern gs_err_t gs_link_adv_start(const gs_adv_param_t* p_param);
extern gs_err_t gs_link_adv_stop(void);
extern gs_err_t gs_link_disconnect(uint8_t conidx, uint8_t reason);
extern uint8_t gs_link_get_conn_count(void);
extern gs_err_t gs_link_pair_response(uint8_t conidx, bool accept);
extern gs_err_t gs_link_conn_param_update(uint8_t conidx, const gs_conn_param_t* p_param);
extern gs_err_t gs_link_get_conn_info(uint8_t conidx, gs_conn_param_t* p_param, gs_bd_addr_t* p_addr);
extern bool gs_link_is_advertising(void);
extern gs_err_t gs_link_ext_adv_set_data(const gs_ext_adv_data_t* p_data);
extern gs_err_t gs_link_ext_adv_start(const gs_ext_adv_param_t* p_param);
extern gs_err_t gs_link_ext_adv_stop(void);
extern gs_err_t gs_link_periodic_adv_set_data(const gs_periodic_adv_data_t* p_data);
extern gs_err_t gs_link_periodic_adv_start(const gs_periodic_adv_param_t* p_param);
extern gs_err_t gs_link_periodic_adv_stop(void);

static ble_gs_callbacks_t gs_cbs;
static uint8_t gs_adv_buf[BLE_GS_ADV_DATA_MAX];
static uint8_t gs_adv_len = 0;
static uint8_t gs_rsp_buf[BLE_GS_ADV_DATA_MAX];
static uint8_t gs_rsp_len = 0;
static uint16_t gs_adv_min = 160;
static uint16_t gs_adv_max = 240;
static gs_pair_param_t gs_pair = {
    .io_cap = GS_IO_CAP_NO_INPUT_NO_OUTPUT,
    .bond_req = true,
    .key_size_min = 7,
    .key_size_max = 16,
    .dist_irk = true,
};
static bool gs_init_done = false;
static ble_gs_adv_cb_t gs_adv_cb = NULL;

static void on_read(uint8_t conidx, uint16_t hdl, uint16_t off, uint16_t max, uint32_t tok) {
    printf("[GS] READ: conidx=%d hdl=0x%04X off=%d max=%d\n", conidx, hdl, off, max);
    if (gs_cbs.cb_read) gs_cbs.cb_read(conidx, hdl, off, max, tok);
    else gs_read_cfm(conidx, tok, 0x0A, 0, NULL);
}

static void on_write(uint8_t conidx, uint16_t hdl, uint16_t off, uint16_t len, const uint8_t* data, uint32_t tok) {
    printf("[GS] WRITE: conidx=%d hdl=0x%04X len=%d data=", conidx, hdl, len);
    for (uint16_t i = 0; i < len && i < 16; i++) printf("%02X ", data[i]);
    if (len > 16) printf("...");
    printf("\n");
    gs_write_cfm(conidx, tok, 0);
    if (gs_cbs.cb_rx) gs_cbs.cb_rx(conidx, hdl, len, data);
}

static void on_cccd(uint8_t conidx, uint16_t hdl, uint16_t cfg) {
    printf("[GS] CCCD_UPD: conidx=%d hdl=0x%04X cfg=0x%04X\n", conidx, hdl, cfg);
    if (gs_cbs.cb_cccd) gs_cbs.cb_cccd(conidx, hdl, cfg);
}

static void on_ntf(uint8_t conidx, uint16_t st) {
    printf("[GS] NTF_CMP: conidx=%d status=0x%04X\n", conidx, st);
    if (gs_cbs.cb_ntf_cmp) gs_cbs.cb_ntf_cmp(conidx, st);
}

static void on_ind(uint8_t conidx, uint16_t st) {
    printf("[GS] IND_CMP: conidx=%d status=0x%04X\n", conidx, st);
    if (gs_cbs.cb_ind_cmp) gs_cbs.cb_ind_cmp(conidx, st);
}

static void on_conn(uint8_t conidx) {
    printf("[GS] CONNECTED: conidx=%d\n", conidx);
    if (gs_cbs.cb_conn) gs_cbs.cb_conn(conidx, true, 0);
}

static void on_disc(uint8_t conidx, uint16_t reason) {
    printf("[GS] DISCONNECTED: conidx=%d reason=0x%04X\n", conidx, reason);
    if (gs_cbs.cb_conn) gs_cbs.cb_conn(conidx, false, reason);
}

static void on_mtu(uint8_t conidx, uint16_t mtu) {
    printf("[GS] MTU_UPD: conidx=%d mtu=%d\n", conidx, mtu);
    if (gs_cbs.cb_mtu) gs_cbs.cb_mtu(conidx, mtu);
}

static void on_link_evt(const gs_link_evt_t* evt) {
    switch (evt->type) {
        case GS_LINK_EVT_ADV_STARTED:
            if (gs_adv_cb) gs_adv_cb(true, evt->data.adv_status);
            break;
        case GS_LINK_EVT_ADV_STOPPED:
            if (gs_adv_cb) gs_adv_cb(false, evt->data.adv_status);
            break;
        case GS_LINK_EVT_PAIR_REQ:
            gs_link_pair_response(evt->data.pair.conidx, true);
            break;
        default:
            break;
    }
}

ble_gs_err_t ble_gs_init(const ble_gs_profile_t* p_profiles, uint16_t cnt, const ble_gs_callbacks_t* p_cbs) {
    if (!p_profiles || cnt == 0) return BLE_GS_ERR_PARAM;
    
    if (p_cbs) memcpy(&gs_cbs, p_cbs, sizeof(ble_gs_callbacks_t));
    else memset(&gs_cbs, 0, sizeof(ble_gs_callbacks_t));
    
    gs_link_callbacks_t link_cbs = { .cb_link_evt = on_link_evt };
    uint16_t ret = gs_link_init(&link_cbs);
    if (ret != GS_ERR_NO_ERROR && ret != GS_ERR_NOT_ALLOWED) return BLE_GS_ERR_INTERNAL;
    
    gs_link_set_pair_param(&gs_pair);
    
    gs_callbacks_t cbs = {
        .cb_read = on_read,
        .cb_write = on_write,
        .cb_cccd_upd = on_cccd,
        .cb_ntf_cmp = on_ntf,
        .cb_ind_cmp = on_ind,
        .cb_connect = on_conn,
        .cb_disconnect = on_disc,
        .cb_mtu_update = on_mtu,
    };
    
    ret = gs_services_init(p_profiles, cnt, &cbs);
    if (ret != GS_ERR_NO_ERROR) return BLE_GS_ERR_INTERNAL;
    
    gs_init_done = true;
    return BLE_GS_OK;
}

void ble_gs_set_dev_name(const uint8_t* name, uint8_t len) {
    if (name && len > 0) gs_link_set_dev_name(name, len > 32 ? 32 : len);
}

void ble_gs_set_adv_data(const uint8_t* data, uint8_t len) {
    if (!data || len == 0) return;
    len = len > BLE_GS_ADV_DATA_MAX ? BLE_GS_ADV_DATA_MAX : len;
    memcpy(gs_adv_buf, data, len);
    gs_adv_len = len;
}

void ble_gs_set_scan_rsp(const uint8_t* data, uint8_t len) {
    if (!data || len == 0) return;
    len = len > BLE_GS_ADV_DATA_MAX ? BLE_GS_ADV_DATA_MAX : len;
    memcpy(gs_rsp_buf, data, len);
    gs_rsp_len = len;
}

void ble_gs_set_adv_interval(uint16_t min, uint16_t max) {
    gs_adv_min = min;
    gs_adv_max = max;
}

ble_gs_err_t ble_gs_adv_start(void) {
    if (!gs_init_done) return BLE_GS_ERR_NOT_ALLOWED;
    
    if (gs_adv_len > 0) {
        gs_adv_data_t adv = {0};
        memcpy(adv.adv_data, gs_adv_buf, gs_adv_len);
        adv.adv_data_len = gs_adv_len;
        if (gs_rsp_len > 0) {
            memcpy(adv.scan_rsp_data, gs_rsp_buf, gs_rsp_len);
            adv.scan_rsp_len = gs_rsp_len;
        }
        gs_link_set_adv_data(&adv);
    }
    
    gs_adv_param_t param = {
        .adv_type = GS_ADV_TYPE_LEGACY_CONN_SCAN,
        .adv_intv_min = gs_adv_min,
        .adv_intv_max = gs_adv_max,
        .ch_map = GS_ADV_CH_ALL,
#ifdef BLE_USE_DIFF_ADDR_THAN_BT
        .own_addr_type = GS_ADDR_PUBLIC,//GS_ADDR_PUBLIC,
#else
		 .own_addr_type = GAPM_GEN_RSLV_ADDR,//GS_ADDR_PUBLIC,
#endif
        .max_tx_pwr = 10,
    };
    
    return gs_link_adv_start(&param) == GS_ERR_NO_ERROR ? BLE_GS_OK : BLE_GS_ERR_INTERNAL;
}

ble_gs_err_t ble_gs_adv_stop(void) {
    return gs_link_adv_stop() == GS_ERR_NO_ERROR ? BLE_GS_OK : BLE_GS_ERR_INTERNAL;
}

ble_gs_err_t ble_gs_disconnect(uint8_t conidx) {
    return gs_link_disconnect(conidx, 0x13) == GS_ERR_NO_ERROR ? BLE_GS_OK : BLE_GS_ERR_INTERNAL;
}

ble_gs_err_t ble_gs_notify(uint8_t conidx, uint16_t handle, const uint8_t* data, uint16_t len) {
    return (ble_gs_err_t)gs_notify_send(conidx, handle, len, data);
}

ble_gs_err_t ble_gs_indicate(uint8_t conidx, uint16_t handle, const uint8_t* data, uint16_t len) {
    return (ble_gs_err_t)gs_indicate_send(conidx, handle, len, data);
}

ble_gs_err_t ble_gs_read_rsp(uint8_t conidx, uint32_t token, uint16_t status, uint16_t len, const uint8_t* data) {
    return gs_read_cfm(conidx, token, status, len, data) == GS_ERR_NO_ERROR ? BLE_GS_OK : BLE_GS_ERR_INTERNAL;
}

uint16_t ble_gs_get_mtu(uint8_t conidx) { return gs_get_mtu(conidx); }
bool ble_gs_is_connected(uint8_t conidx) { return gs_is_connected(conidx); }
uint8_t ble_gs_get_conn_count(void) { return gs_link_get_conn_count(); }

void ble_gs_set_pair_param(ble_gs_io_cap_t io_cap, bool auth_req, bool bond_req) {
    gs_pair.io_cap = (uint8_t)io_cap;
    gs_pair.auth_req = auth_req;
    gs_pair.bond_req = bond_req;
    gs_link_set_pair_param(&gs_pair);
}

void ble_gs_set_adv_callback(ble_gs_adv_cb_t cb) { gs_adv_cb = cb; }
bool ble_gs_is_advertising(void) { return gs_link_is_advertising(); }

extern uint16_t gapc_le_update_params(uint8_t conidx, uint32_t metainfo, 
    const void* p_params, void (*cmp_cb)(uint8_t, uint32_t, uint16_t));
extern uint16_t gapc_le_set_phy(uint8_t conidx, uint32_t metainfo, 
    uint8_t tx_phy, uint8_t rx_phy, uint8_t phy_opt, void (*cmp_cb)(uint8_t, uint32_t, uint16_t));

static void conn_param_cmp_cb(uint8_t conidx, uint32_t metainfo, uint16_t status) {
    printf("[GS_SVC] Conn param update complete: status=0x%04X\n", status);
}

static void phy_cmp_cb(uint8_t conidx, uint32_t metainfo, uint16_t status) {
    printf("[GS_SVC] PHY update complete: status=0x%04X\n", status);
}

typedef struct {
    uint16_t interval_min;
    uint16_t interval_max;
    uint16_t latency;
    uint16_t sup_to;
    uint16_t ce_len_min;
    uint16_t ce_len_max;
} conn_param_nego_t;

ble_gs_err_t ble_gs_conn_param_update(uint8_t conidx, uint16_t interval, uint16_t latency, uint16_t timeout) {
    printf("[GS_SVC] conn_param_update: conidx=%d intv=%d lat=%d to=%d\n", conidx, interval, latency, timeout);
    
    conn_param_nego_t params = {
        .interval_min = interval,
        .interval_max = interval,
        .latency = latency,
        .sup_to = timeout,
        .ce_len_min = 0,
        .ce_len_max = 0,
    };
    
    uint16_t status = gapc_le_update_params(conidx, 0, &params, conn_param_cmp_cb);
    printf("[GS_SVC] gapc_le_update_params: status=0x%04X\n", status);
    
    return status == 0 ? BLE_GS_OK : BLE_GS_ERR_INTERNAL;
}

ble_gs_err_t ble_gs_get_peer_addr(uint8_t conidx, uint8_t* p_addr) {
    if (!p_addr) return BLE_GS_ERR_PARAM;
    gs_bd_addr_t addr;
    if (gs_link_get_conn_info(conidx, NULL, &addr) != GS_ERR_NO_ERROR) return BLE_GS_ERR_NOT_CONN;
    memcpy(p_addr, addr.addr, GS_BD_ADDR_LEN);
    return BLE_GS_OK;
}

ble_gs_err_t ble_gs_mtu_exchange(uint8_t conidx) {
    return gs_link_mtu_exchange(conidx) == GS_ERR_NO_ERROR ? BLE_GS_OK : BLE_GS_ERR_INTERNAL;
}

ble_gs_err_t ble_gs_set_phy(uint8_t conidx, ble_gs_phy_t tx_phy, ble_gs_phy_t rx_phy) {
    printf("[GS_SVC] set_phy: conidx=%d tx=%d rx=%d\n", conidx, tx_phy, rx_phy);
    
    // 直接调用 gapc_le_set_phy，绕过 gs_link 层
    uint16_t status = gapc_le_set_phy(conidx, 0, (uint8_t)tx_phy, (uint8_t)rx_phy, 0, phy_cmp_cb);
    printf("[GS_SVC] gapc_le_set_phy: status=0x%04X\n", status);
    
    return status == 0 ? BLE_GS_OK : BLE_GS_ERR_INTERNAL;
}

ble_gs_err_t ble_gs_set_data_len(uint8_t conidx, uint16_t tx_octets, uint16_t tx_time) {
    (void)conidx; (void)tx_octets; (void)tx_time;
    return BLE_GS_OK;
}

uint8_t ble_gs_build_adv_data(uint8_t* buf, const uint8_t* name, uint8_t name_len,
                              const uint16_t* uuid16, uint8_t uuid16_cnt, const uint8_t* uuid128) {
    if (!buf) return 0;
    uint8_t off = 0;
    
    if (name && name_len > 0) {
        uint8_t max = BLE_GS_ADV_DATA_MAX - off - 2;
        uint8_t len = name_len > max ? max : name_len;
        buf[off++] = len + 1;
        buf[off++] = len < name_len ? 0x08 : 0x09;
        memcpy(&buf[off], name, len);
        off += len;
    }
    
    if (uuid16 && uuid16_cnt > 0) {
        uint8_t max = (BLE_GS_ADV_DATA_MAX - off - 2) / 2;
        uint8_t cnt = uuid16_cnt > max ? max : uuid16_cnt;
        if (cnt > 0) {
            buf[off++] = cnt * 2 + 1;
            buf[off++] = cnt < uuid16_cnt ? 0x02 : 0x03;
            for (uint8_t i = 0; i < cnt; i++) {
                buf[off++] = uuid16[i] & 0xFF;
                buf[off++] = (uuid16[i] >> 8) & 0xFF;
            }
        }
    }
    
    if (uuid128 && (off + 18) <= BLE_GS_ADV_DATA_MAX) {
        buf[off++] = 17;
        buf[off++] = 0x07;
        memcpy(&buf[off], uuid128, 16);
        off += 16;
    }
    
    return off;
}

ble_gs_err_t ble_gs_ext_adv_set_data(const ble_gs_ext_adv_data_t* p_data) {
    if (!p_data || p_data->adv_data_len > BLE_GS_EXT_ADV_DATA_MAX || p_data->scan_rsp_len > BLE_GS_EXT_ADV_DATA_MAX)
        return BLE_GS_ERR_PARAM;
    
    gs_ext_adv_data_t d = {
        .p_adv_data = p_data->p_adv_data, .adv_data_len = p_data->adv_data_len,
        .p_scan_rsp = p_data->p_scan_rsp, .scan_rsp_len = p_data->scan_rsp_len
    };
    return gs_link_ext_adv_set_data(&d) == GS_ERR_NO_ERROR ? BLE_GS_OK : BLE_GS_ERR_INTERNAL;
}

ble_gs_err_t ble_gs_ext_adv_start(const ble_gs_ext_adv_param_t* p) {
    if (!gs_init_done) return BLE_GS_ERR_NOT_ALLOWED;
    
    gs_ext_adv_param_t param = {
        .adv_type = (uint8_t)p->adv_type, .adv_intv_min = p->adv_intv_min, .adv_intv_max = p->adv_intv_max,
        .ch_map = p->ch_map, .own_addr_type = GS_ADDR_PUBLIC, .max_tx_pwr = p->max_tx_pwr,
        .primary_phy = (gs_phy_type_t)p->primary_phy, .secondary_phy = (gs_phy_type_t)p->secondary_phy, .adv_sid = p->adv_sid
    };
    return gs_link_ext_adv_start(&param) == GS_ERR_NO_ERROR ? BLE_GS_OK : BLE_GS_ERR_INTERNAL;
}

ble_gs_err_t ble_gs_ext_adv_stop(void) {
    return gs_link_ext_adv_stop() == GS_ERR_NO_ERROR ? BLE_GS_OK : BLE_GS_ERR_INTERNAL;
}

ble_gs_err_t ble_gs_periodic_adv_set_data(const ble_gs_periodic_adv_data_t* p) {
    if (!p) return BLE_GS_ERR_PARAM;
    gs_periodic_adv_data_t d = {
        .p_ext_adv_data = p->p_ext_adv_data, .ext_adv_data_len = p->ext_adv_data_len,
        .p_periodic_data = p->p_periodic_data, .periodic_data_len = p->periodic_data_len
    };
    return gs_link_periodic_adv_set_data(&d) == GS_ERR_NO_ERROR ? BLE_GS_OK : BLE_GS_ERR_INTERNAL;
}

ble_gs_err_t ble_gs_periodic_adv_start(const ble_gs_periodic_adv_param_t* p) {
    if (!gs_init_done || !p) return BLE_GS_ERR_PARAM;
    
    gs_periodic_adv_param_t param = {
        .adv_intv_min = p->adv_intv_min, .adv_intv_max = p->adv_intv_max,
        .period_intv_min = p->period_intv_min, .period_intv_max = p->period_intv_max,
        .ch_map = p->ch_map, .own_addr_type = GS_ADDR_PUBLIC, .max_tx_pwr = p->max_tx_pwr,
        .primary_phy = (gs_phy_type_t)p->primary_phy, .secondary_phy = (gs_phy_type_t)p->secondary_phy, .adv_sid = p->adv_sid
    };
    return gs_link_periodic_adv_start(&param) == GS_ERR_NO_ERROR ? BLE_GS_OK : BLE_GS_ERR_INTERNAL;
}

ble_gs_err_t ble_gs_periodic_adv_stop(void) {
    return gs_link_periodic_adv_stop() == GS_ERR_NO_ERROR ? BLE_GS_OK : BLE_GS_ERR_INTERNAL;
}

#endif /* BLE_SUPPORT */
