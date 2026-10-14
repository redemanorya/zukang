#include "bt_config.h"
#if (BLE_SUPPORT)

#include "ble_gs_central.h"
#include <string.h>
#include <stdio.h>

#define BLE_GSC_DBG(fmt, ...)   printf("[BLE_GSC] " fmt "\n", ##__VA_ARGS__)

typedef uint16_t gs_err_t;
#define GS_ERR_NO_ERROR     0x0000

#define GS_BD_ADDR_LEN      6

typedef enum {
    GS_ADDR_PUBLIC = 0,
    GS_ADDR_RANDOM,
} gs_addr_type_t;

typedef struct gs_bd_addr {
    uint8_t addr[GS_BD_ADDR_LEN];
} gs_bd_addr_t;

typedef enum {
    GS_SCAN_TYPE_GENERAL = 0,
    GS_SCAN_TYPE_LIMITED = 1,
    GS_SCAN_TYPE_OBSERVER = 2,
    GS_SCAN_TYPE_CONNECTABLE = 4,
} gs_scan_type_t;

typedef enum {
    GS_SCAN_PHY_1M   = 0x01,
    GS_SCAN_PHY_CODED = 0x02,
} gs_scan_phy_t;

typedef enum {
    GS_SCAN_FILT_DUP_DIS = 0,
    GS_SCAN_FILT_DUP_EN,
    GS_SCAN_FILT_DUP_EN_PER_PERIOD,
} gs_scan_filter_t;

typedef struct gs_scan_param {
    gs_scan_type_t type;
    gs_scan_phy_t  phy;
    gs_scan_filter_t filter;
    bool active_scan;
    uint16_t scan_intv;
    uint16_t scan_wd;
    uint16_t duration;
    uint16_t period;
} gs_scan_param_t;

typedef enum {
    GS_INIT_DIRECT = 0,
    GS_INIT_AUTO,
    GS_INIT_NAME_DISC,
} gs_init_type_t;

typedef struct gs_scan_wd_param {
    uint16_t scan_intv;
    uint16_t scan_wd;
} gs_scan_wd_param_t;

typedef struct gs_init_conn_param {
    uint16_t conn_intv_min;
    uint16_t conn_intv_max;
    uint16_t conn_latency;
    uint16_t sup_timeout;
    uint16_t ce_len_min;
    uint16_t ce_len_max;
} gs_init_conn_param_t;

typedef struct gs_init_param {
    gs_init_type_t type;
    uint8_t prop;
    uint16_t conn_to;
    gs_scan_wd_param_t scan_1m;
    gs_scan_wd_param_t scan_coded;
    gs_init_conn_param_t conn_1m;
    gs_init_conn_param_t conn_2m;
    gs_init_conn_param_t conn_coded;
    gs_bd_addr_t peer_addr;
    gs_addr_type_t peer_addr_type;
    gs_addr_type_t own_addr_type;
} gs_init_param_t;

typedef enum {
    GS_ADV_REPORT_EXT = 0,
    GS_ADV_REPORT_LEG,
    GS_ADV_REPORT_SCAN_RSP_EXT,
    GS_ADV_REPORT_SCAN_RSP_LEG,
    GS_ADV_REPORT_PER,
} gs_adv_report_type_t;

typedef struct gs_adv_report {
    gs_adv_report_type_t report_type;
    bool complete;
    bool connectable;
    bool scannable;
    bool directed;
    gs_bd_addr_t addr;
    gs_addr_type_t addr_type;
    gs_bd_addr_t target_addr;
    gs_addr_type_t target_addr_type;
    int8_t tx_pwr;
    int8_t rssi;
    uint8_t phy_prim;
    uint8_t phy_second;
    uint8_t adv_sid;
    uint16_t period_adv_intv;
    const uint8_t* p_data;
    uint16_t data_len;
} gs_adv_report_t;

typedef enum {
    GS_CENTRAL_EVT_SCAN_STARTED = 0,
    GS_CENTRAL_EVT_SCAN_STOPPED,
    GS_CENTRAL_EVT_ADV_REPORT,
    GS_CENTRAL_EVT_CONN_STARTED,
    GS_CENTRAL_EVT_CONN_ESTABLISHED,
    GS_CENTRAL_EVT_CONN_FAILED,
    GS_CENTRAL_EVT_CONN_TIMEOUT,
    GS_CENTRAL_EVT_NAME_DISCOVERED,
} gs_central_evt_type_t;

typedef struct gs_central_conn_info {
    uint8_t conidx;
    gs_bd_addr_t peer_addr;
    gs_addr_type_t peer_addr_type;
} gs_central_conn_info_t;

typedef struct gs_name_disc_info {
    gs_bd_addr_t peer_addr;
    gs_addr_type_t peer_addr_type;
    const uint8_t* p_name;
    uint16_t name_len;
} gs_name_disc_info_t;

typedef union gs_central_evt_data {
    uint16_t status;
    gs_adv_report_t adv_report;
    gs_central_conn_info_t conn;
    gs_name_disc_info_t name;
} gs_central_evt_data_t;

typedef struct gs_central_evt {
    gs_central_evt_type_t type;
    gs_central_evt_data_t data;
} gs_central_evt_t;

typedef void (*gs_central_evt_cb_t)(const gs_central_evt_t* p_evt);

typedef struct gs_central_callbacks {
    gs_central_evt_cb_t cb_evt;
} gs_central_callbacks_t;

#define GS_GATTC_UUID_128_LEN   16

typedef enum {
    GS_GATTC_UUID_16 = 0,
    GS_GATTC_UUID_32,
    GS_GATTC_UUID_128,
} gs_gattc_uuid_type_t;

typedef enum {
    GS_GATTC_WRITE = 0,
    GS_GATTC_WRITE_NO_RESP,
    GS_GATTC_WRITE_SIGNED,
} gs_gattc_write_type_t;

typedef enum {
    GS_GATTC_DISC_ALL_SVC = 0,
    GS_GATTC_DISC_SVC_BY_UUID,
} gs_gattc_disc_type_t;

typedef enum {
    GS_GATTC_EVT_DISCOVER_CMP = 0,
    GS_GATTC_EVT_READ_CMP,
    GS_GATTC_EVT_WRITE_CMP,
    GS_GATTC_EVT_SVC_FOUND,
    GS_GATTC_EVT_SVC_INFO,
    GS_GATTC_EVT_CHAR_FOUND,
    GS_GATTC_EVT_DESC_FOUND,
    GS_GATTC_EVT_READ_VAL,
    GS_GATTC_EVT_NOTIFICATION,
    GS_GATTC_EVT_INDICATION,
    GS_GATTC_EVT_SVC_CHANGED,
} gs_gattc_evt_type_t;

typedef struct gs_gattc_svc_info {
    uint16_t start_hdl;
    uint16_t end_hdl;
    gs_gattc_uuid_type_t uuid_type;
    uint8_t uuid[GS_GATTC_UUID_128_LEN];
} gs_gattc_svc_info_t;

typedef struct gs_gattc_char_found {
    uint16_t char_hdl;
    uint16_t val_hdl;
    uint8_t prop;
    gs_gattc_uuid_type_t uuid_type;
    uint8_t uuid[GS_GATTC_UUID_128_LEN];
} gs_gattc_char_found_t;

typedef struct gs_gattc_desc_found {
    uint16_t desc_hdl;
    gs_gattc_uuid_type_t uuid_type;
    uint8_t uuid[GS_GATTC_UUID_128_LEN];
} gs_gattc_desc_found_t;

typedef struct gs_gattc_read_val {
    uint16_t hdl;
    uint16_t offset;
    const uint8_t* p_data;
    uint16_t data_len;
} gs_gattc_read_val_t;

typedef struct gs_gattc_ntf_ind {
    uint16_t hdl;
    const uint8_t* p_data;
    uint16_t data_len;
    bool complete;
} gs_gattc_ntf_ind_t;

typedef struct gs_gattc_svc_changed {
    uint16_t start_hdl;
    uint16_t end_hdl;
    bool out_of_sync;
} gs_gattc_svc_changed_t;

typedef union gs_gattc_evt_data {
    uint16_t status;
    gs_gattc_svc_info_t svc_info;
    gs_gattc_char_found_t charac;
    gs_gattc_desc_found_t desc;
    gs_gattc_read_val_t read;
    gs_gattc_ntf_ind_t ntf_ind;
    gs_gattc_svc_changed_t svc_changed;
} gs_gattc_evt_data_t;

typedef struct gs_gattc_evt {
    gs_gattc_evt_type_t type;
    uint8_t conidx;
    uint16_t metainfo;
    gs_gattc_evt_data_t data;
} gs_gattc_evt_t;

typedef void (*gs_gattc_evt_cb_t)(const gs_gattc_evt_t* p_evt);

typedef struct gs_gattc_callbacks {
    gs_gattc_evt_cb_t cb_evt;
} gs_gattc_callbacks_t;

extern gs_err_t gs_central_init(const gs_central_callbacks_t* p_cbs);
extern gs_err_t gs_central_scan_start(const gs_scan_param_t* p_param);
extern gs_err_t gs_central_scan_stop(void);
extern bool gs_central_is_scanning(void);
extern gs_err_t gs_central_connect(const gs_init_param_t* p_param);
extern gs_err_t gs_central_connect_cancel(void);
extern bool gs_central_is_connecting(void);
extern gs_err_t gs_central_disconnect(uint8_t conidx, uint8_t reason);

extern gs_err_t gs_gattc_init(uint16_t pref_mtu, const gs_gattc_callbacks_t* p_cbs);
extern gs_err_t gs_gattc_discover_svc(uint8_t conidx, uint16_t metainfo,
                                       gs_gattc_disc_type_t disc_type, bool full,
                                       uint16_t start_hdl, uint16_t end_hdl,
                                       gs_gattc_uuid_type_t uuid_type, const uint8_t* p_uuid);
extern gs_err_t gs_gattc_discover_char(uint8_t conidx, uint16_t metainfo,
                                        uint16_t start_hdl, uint16_t end_hdl,
                                        gs_gattc_uuid_type_t uuid_type, const uint8_t* p_uuid);
extern gs_err_t gs_gattc_discover_desc(uint8_t conidx, uint16_t metainfo,
                                        uint16_t start_hdl, uint16_t end_hdl);
extern gs_err_t gs_gattc_read(uint8_t conidx, uint16_t metainfo,
                               uint16_t hdl, uint16_t offset, uint16_t length);
extern gs_err_t gs_gattc_write(uint8_t conidx, uint16_t metainfo,
                                gs_gattc_write_type_t write_type,
                                uint16_t hdl, uint16_t offset,
                                const uint8_t* p_data, uint16_t data_len);
extern gs_err_t gs_gattc_event_register(uint8_t conidx, uint16_t start_hdl, uint16_t end_hdl);
extern gs_err_t gs_gattc_event_unregister(uint8_t conidx, uint16_t start_hdl, uint16_t end_hdl);
extern gs_err_t gs_gattc_mtu_exchange(uint8_t conidx);

typedef struct ble_gsc_env
{
    ble_gsc_callbacks_t cbs;
    bool initialized;
} ble_gsc_env_t;

static ble_gsc_env_t ble_gsc_env;

static void ble_gsc_send_evt(ble_gsc_evt_type_t type, uint8_t conidx, const ble_gsc_evt_data_t* p_data)
{
    if (ble_gsc_env.cbs.cb_evt == NULL)
        return;

    ble_gsc_evt_t evt;
    evt.type   = type;
    evt.conidx = conidx;
    if (p_data)
        memcpy(&evt.data, p_data, sizeof(ble_gsc_evt_data_t));
    else
        memset(&evt.data, 0, sizeof(ble_gsc_evt_data_t));

    ble_gsc_env.cbs.cb_evt(&evt);
}

static void ble_gsc_central_evt_handler(const gs_central_evt_t* p_evt)
{
    ble_gsc_evt_data_t d;
    memset(&d, 0, sizeof(d));

    switch (p_evt->type)
    {
        case GS_CENTRAL_EVT_SCAN_STARTED:
            d.status = p_evt->data.status;
            ble_gsc_send_evt(BLE_GSC_EVT_SCAN_STARTED, 0, &d);
            break;

        case GS_CENTRAL_EVT_SCAN_STOPPED:
            d.status = p_evt->data.status;
            ble_gsc_send_evt(BLE_GSC_EVT_SCAN_STOPPED, 0, &d);
            break;

        case GS_CENTRAL_EVT_ADV_REPORT:
        {
            const gs_adv_report_t* rpt = &p_evt->data.adv_report;
            d.adv_report.report_type = (uint8_t)rpt->report_type;
            d.adv_report.complete    = rpt->complete;
            d.adv_report.connectable = rpt->connectable;
            d.adv_report.scannable   = rpt->scannable;
            d.adv_report.directed    = rpt->directed;
            memcpy(d.adv_report.addr.addr, rpt->addr.addr, BLE_GSC_BD_ADDR_LEN);
            d.adv_report.addr_type = (ble_gsc_addr_type_t)rpt->addr_type;
            memcpy(d.adv_report.target_addr.addr, rpt->target_addr.addr, BLE_GSC_BD_ADDR_LEN);
            d.adv_report.target_addr_type = (ble_gsc_addr_type_t)rpt->target_addr_type;
            d.adv_report.tx_pwr          = rpt->tx_pwr;
            d.adv_report.rssi            = rpt->rssi;
            d.adv_report.phy_prim        = rpt->phy_prim;
            d.adv_report.phy_second      = rpt->phy_second;
            d.adv_report.adv_sid         = rpt->adv_sid;
            d.adv_report.period_adv_intv = rpt->period_adv_intv;
            d.adv_report.p_data          = rpt->p_data;
            d.adv_report.data_len        = rpt->data_len;
            ble_gsc_send_evt(BLE_GSC_EVT_ADV_REPORT, 0, &d);
            break;
        }

        case GS_CENTRAL_EVT_CONN_STARTED:
            d.status = p_evt->data.status;
            ble_gsc_send_evt(BLE_GSC_EVT_CONNECTING, 0, &d);
            break;

        case GS_CENTRAL_EVT_CONN_FAILED:
        case GS_CENTRAL_EVT_CONN_TIMEOUT:
            d.status = p_evt->data.status;
            ble_gsc_send_evt(BLE_GSC_EVT_CONNECT_FAILED, 0, &d);
            break;

        case GS_CENTRAL_EVT_NAME_DISCOVERED:
            break;

        default:
            break;
    }
}

static void ble_gsc_gattc_evt_handler(const gs_gattc_evt_t* p_evt)
{
    ble_gsc_evt_data_t d;
    memset(&d, 0, sizeof(d));
    uint8_t conidx = p_evt->conidx;

    switch (p_evt->type)
    {
        case GS_GATTC_EVT_DISCOVER_CMP:
            d.status = p_evt->data.status;
            ble_gsc_send_evt(BLE_GSC_EVT_DISC_CMP, conidx, &d);
            break;

        case GS_GATTC_EVT_READ_CMP:
            d.status = p_evt->data.status;
            ble_gsc_send_evt(BLE_GSC_EVT_READ_CMP, conidx, &d);
            break;

        case GS_GATTC_EVT_WRITE_CMP:
            d.status = p_evt->data.status;
            ble_gsc_send_evt(BLE_GSC_EVT_WRITE_CMP, conidx, &d);
            break;

        case GS_GATTC_EVT_SVC_INFO:
        {
            const gs_gattc_svc_info_t* si = &p_evt->data.svc_info;
            d.svc.start_hdl = si->start_hdl;
            d.svc.end_hdl   = si->end_hdl;
            d.svc.uuid_type = (uint8_t)si->uuid_type;
            memcpy(d.svc.uuid, si->uuid, BLE_GSC_UUID_128_LEN);
            ble_gsc_send_evt(BLE_GSC_EVT_SVC_FOUND, conidx, &d);
            break;
        }

        case GS_GATTC_EVT_CHAR_FOUND:
        {
            const gs_gattc_char_found_t* cf = &p_evt->data.charac;
            d.charac.char_hdl  = cf->char_hdl;
            d.charac.val_hdl   = cf->val_hdl;
            d.charac.prop      = cf->prop;
            d.charac.uuid_type = (uint8_t)cf->uuid_type;
            memcpy(d.charac.uuid, cf->uuid, BLE_GSC_UUID_128_LEN);
            ble_gsc_send_evt(BLE_GSC_EVT_CHAR_FOUND, conidx, &d);
            break;
        }

        case GS_GATTC_EVT_DESC_FOUND:
        {
            const gs_gattc_desc_found_t* df = &p_evt->data.desc;
            d.desc.desc_hdl  = df->desc_hdl;
            d.desc.uuid_type = (uint8_t)df->uuid_type;
            memcpy(d.desc.uuid, df->uuid, BLE_GSC_UUID_128_LEN);
            ble_gsc_send_evt(BLE_GSC_EVT_DESC_FOUND, conidx, &d);
            break;
        }

        case GS_GATTC_EVT_READ_VAL:
        {
            const gs_gattc_read_val_t* rv = &p_evt->data.read;
            d.read.hdl      = rv->hdl;
            d.read.offset   = rv->offset;
            d.read.p_data   = rv->p_data;
            d.read.data_len = rv->data_len;
            ble_gsc_send_evt(BLE_GSC_EVT_READ_VAL, conidx, &d);
            break;
        }

        case GS_GATTC_EVT_NOTIFICATION:
        {
            const gs_gattc_ntf_ind_t* ni = &p_evt->data.ntf_ind;
            d.ntf_ind.hdl      = ni->hdl;
            d.ntf_ind.p_data   = ni->p_data;
            d.ntf_ind.data_len = ni->data_len;
            d.ntf_ind.complete = ni->complete;
            ble_gsc_send_evt(BLE_GSC_EVT_NOTIFICATION, conidx, &d);
            break;
        }

        case GS_GATTC_EVT_INDICATION:
        {
            const gs_gattc_ntf_ind_t* ni = &p_evt->data.ntf_ind;
            d.ntf_ind.hdl      = ni->hdl;
            d.ntf_ind.p_data   = ni->p_data;
            d.ntf_ind.data_len = ni->data_len;
            d.ntf_ind.complete = ni->complete;
            ble_gsc_send_evt(BLE_GSC_EVT_INDICATION, conidx, &d);
            break;
        }

        default:
            break;
    }
}

ble_gsc_err_t ble_gsc_init(const ble_gsc_callbacks_t* p_cbs)
{
    if (ble_gsc_env.initialized)
        return BLE_GSC_ERR_NOT_ALLOWED;

    memset(&ble_gsc_env, 0, sizeof(ble_gsc_env));

    if (p_cbs)
        memcpy(&ble_gsc_env.cbs, p_cbs, sizeof(ble_gsc_callbacks_t));

    gs_central_callbacks_t central_cbs = { .cb_evt = ble_gsc_central_evt_handler };
    gs_err_t ret = gs_central_init(&central_cbs);
    if (ret != GS_ERR_NO_ERROR)
    {
        BLE_GSC_DBG("central init fail %d", ret);
        return BLE_GSC_ERR_INTERNAL;
    }

    gs_gattc_callbacks_t gattc_cbs = { .cb_evt = ble_gsc_gattc_evt_handler };
    ret = gs_gattc_init(0, &gattc_cbs);
    if (ret != GS_ERR_NO_ERROR)
    {
        BLE_GSC_DBG("gattc init fail %d", ret);
        return BLE_GSC_ERR_INTERNAL;
    }

    ble_gsc_env.initialized = true;
    BLE_GSC_DBG("initialized");
    return BLE_GSC_OK;
}

ble_gsc_err_t ble_gsc_scan_start(const ble_gsc_scan_param_t* p_param)
{
    if (!ble_gsc_env.initialized)
        return BLE_GSC_ERR_NOT_ALLOWED;

    gs_scan_param_t sp;
    memset(&sp, 0, sizeof(sp));

    if (p_param)
    {
        sp.type        = (gs_scan_type_t)p_param->type;
        sp.phy         = (gs_scan_phy_t)p_param->phy;
        sp.filter      = (gs_scan_filter_t)p_param->filter;
        sp.active_scan = p_param->active_scan;
        sp.scan_intv   = p_param->scan_intv;
        sp.scan_wd     = p_param->scan_wd;
        sp.duration    = p_param->duration;
        sp.period      = p_param->period;
        return (ble_gsc_err_t)gs_central_scan_start(&sp);
    }

    return (ble_gsc_err_t)gs_central_scan_start(NULL);
}

ble_gsc_err_t ble_gsc_scan_stop(void)
{
    if (!ble_gsc_env.initialized)
        return BLE_GSC_ERR_NOT_ALLOWED;

    return (ble_gsc_err_t)gs_central_scan_stop();
}

bool ble_gsc_is_scanning(void)
{
    return gs_central_is_scanning();
}

ble_gsc_err_t ble_gsc_connect(const ble_gsc_connect_param_t* p_param)
{
    if (!ble_gsc_env.initialized)
        return BLE_GSC_ERR_NOT_ALLOWED;

    if (!p_param)
        return BLE_GSC_ERR_PARAM;

    gs_init_param_t ip;
    memset(&ip, 0, sizeof(ip));

    ip.type = GS_INIT_DIRECT;
    ip.prop = p_param->prop ? p_param->prop : BLE_GSC_INIT_PROP_1M;
    ip.conn_to = p_param->conn_to;
    ip.own_addr_type = GS_ADDR_PUBLIC;

    ip.scan_1m.scan_intv = p_param->scan_intv;
    ip.scan_1m.scan_wd   = p_param->scan_wd;

    ip.conn_1m.conn_intv_min = p_param->conn_param.conn_intv_min;
    ip.conn_1m.conn_intv_max = p_param->conn_param.conn_intv_max;
    ip.conn_1m.conn_latency  = p_param->conn_param.conn_latency;
    ip.conn_1m.sup_timeout   = p_param->conn_param.sup_timeout;
    ip.conn_1m.ce_len_min    = p_param->conn_param.ce_len_min;
    ip.conn_1m.ce_len_max    = p_param->conn_param.ce_len_max;

    memcpy(ip.peer_addr.addr, p_param->peer_addr.addr, BLE_GSC_BD_ADDR_LEN);
    ip.peer_addr_type = (gs_addr_type_t)p_param->peer_addr_type;

    return (ble_gsc_err_t)gs_central_connect(&ip);
}

ble_gsc_err_t ble_gsc_connect_cancel(void)
{
    if (!ble_gsc_env.initialized)
        return BLE_GSC_ERR_NOT_ALLOWED;

    return (ble_gsc_err_t)gs_central_connect_cancel();
}

bool ble_gsc_is_connecting(void)
{
    return gs_central_is_connecting();
}

ble_gsc_err_t ble_gsc_disconnect(uint8_t conidx)
{
    if (!ble_gsc_env.initialized)
        return BLE_GSC_ERR_NOT_ALLOWED;

    return (ble_gsc_err_t)gs_central_disconnect(conidx, 0x13);
}

ble_gsc_err_t ble_gsc_mtu_exchange(uint8_t conidx)
{
    if (!ble_gsc_env.initialized)
        return BLE_GSC_ERR_NOT_ALLOWED;

    return (ble_gsc_err_t)gs_gattc_mtu_exchange(conidx);
}

ble_gsc_err_t ble_gsc_discover_svc(uint8_t conidx, ble_gsc_disc_type_t disc_type,
                                    uint16_t start_hdl, uint16_t end_hdl,
                                    uint8_t uuid_type, const uint8_t* p_uuid)
{
    if (!ble_gsc_env.initialized)
        return BLE_GSC_ERR_NOT_ALLOWED;

    return (ble_gsc_err_t)gs_gattc_discover_svc(conidx, 0,
                                                  (gs_gattc_disc_type_t)disc_type, false,
                                                  start_hdl, end_hdl,
                                                  (gs_gattc_uuid_type_t)uuid_type, p_uuid);
}

ble_gsc_err_t ble_gsc_discover_char(uint8_t conidx, uint16_t start_hdl, uint16_t end_hdl,
                                     uint8_t uuid_type, const uint8_t* p_uuid)
{
    if (!ble_gsc_env.initialized)
        return BLE_GSC_ERR_NOT_ALLOWED;

    return (ble_gsc_err_t)gs_gattc_discover_char(conidx, 0,
                                                   start_hdl, end_hdl,
                                                   (gs_gattc_uuid_type_t)uuid_type, p_uuid);
}

ble_gsc_err_t ble_gsc_discover_desc(uint8_t conidx, uint16_t start_hdl, uint16_t end_hdl)
{
    if (!ble_gsc_env.initialized)
        return BLE_GSC_ERR_NOT_ALLOWED;

    return (ble_gsc_err_t)gs_gattc_discover_desc(conidx, 0, start_hdl, end_hdl);
}

ble_gsc_err_t ble_gsc_read(uint8_t conidx, uint16_t hdl, uint16_t offset, uint16_t length)
{
    if (!ble_gsc_env.initialized)
        return BLE_GSC_ERR_NOT_ALLOWED;

    return (ble_gsc_err_t)gs_gattc_read(conidx, 0, hdl, offset, length);
}

ble_gsc_err_t ble_gsc_write(uint8_t conidx, ble_gsc_write_type_t type,
                              uint16_t hdl, const uint8_t* p_data, uint16_t len)
{
    if (!ble_gsc_env.initialized)
        return BLE_GSC_ERR_NOT_ALLOWED;

    return (ble_gsc_err_t)gs_gattc_write(conidx, 0,
                                          (gs_gattc_write_type_t)type,
                                          hdl, 0, p_data, len);
}

ble_gsc_err_t ble_gsc_event_register(uint8_t conidx, uint16_t start_hdl, uint16_t end_hdl)
{
    if (!ble_gsc_env.initialized)
        return BLE_GSC_ERR_NOT_ALLOWED;

    return (ble_gsc_err_t)gs_gattc_event_register(conidx, start_hdl, end_hdl);
}

ble_gsc_err_t ble_gsc_event_unregister(uint8_t conidx, uint16_t start_hdl, uint16_t end_hdl)
{
    if (!ble_gsc_env.initialized)
        return BLE_GSC_ERR_NOT_ALLOWED;

    return (ble_gsc_err_t)gs_gattc_event_unregister(conidx, start_hdl, end_hdl);
}

#endif /* BLE_SUPPORT */
