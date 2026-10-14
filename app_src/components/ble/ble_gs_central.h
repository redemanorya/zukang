#ifndef _BLE_GS_CENTRAL_H_
#define _BLE_GS_CENTRAL_H_

#include "bt_config.h"
#if (BLE_SUPPORT)

#include <stdint.h>
#include <stdbool.h>
//#include "ble_app_gatt.h"

#define BLE_GSC_BD_ADDR_LEN     6
#define BLE_GSC_UUID_128_LEN    16
#define BLE_GSC_SCAN_DATA_MAX   254

#define BLE_GSC_INIT_PROP_1M    0x01
#define BLE_GSC_INIT_PROP_2M    0x02
#define BLE_GSC_INIT_PROP_CODED 0x04

typedef enum {
    BLE_GSC_OK = 0,
    BLE_GSC_ERR_PARAM,
    BLE_GSC_ERR_NO_RESOURCE,
    BLE_GSC_ERR_NOT_ALLOWED = 5,
    BLE_GSC_ERR_NOT_CONN,
    BLE_GSC_ERR_BUSY,
    BLE_GSC_ERR_INTERNAL = 0xFF,
} ble_gsc_err_t;

typedef enum {
    BLE_GSC_PHY_1M = 0x01,
    BLE_GSC_PHY_2M = 0x02,
    BLE_GSC_PHY_CODED = 0x04,
} ble_gsc_phy_t;

typedef enum {
    BLE_GSC_ADDR_PUBLIC = 0,
    BLE_GSC_ADDR_RANDOM,
} ble_gsc_addr_type_t;

typedef enum {
    BLE_GSC_SCAN_GENERAL = 0,
    BLE_GSC_SCAN_LIMITED = 1,
    BLE_GSC_SCAN_OBSERVER = 2,
    BLE_GSC_SCAN_CONNECTABLE = 4,
} ble_gsc_scan_type_t;

typedef enum {
    BLE_GSC_FILT_DUP_DIS = 0,
    BLE_GSC_FILT_DUP_EN,
    BLE_GSC_FILT_DUP_PER_PERIOD,
} ble_gsc_filt_dup_t;

typedef struct {
    ble_gsc_scan_type_t type;
    uint8_t phy;
    ble_gsc_filt_dup_t filter;
    bool active_scan;
    uint16_t scan_intv;
    uint16_t scan_wd;
    uint16_t duration;
    uint16_t period;
} ble_gsc_scan_param_t;

typedef struct {
    uint8_t addr[BLE_GSC_BD_ADDR_LEN];
} ble_gsc_addr_t;

typedef struct {
    uint8_t report_type;
    bool complete;
    bool connectable;
    bool scannable;
    bool directed;
    ble_gsc_addr_t addr;
    ble_gsc_addr_type_t addr_type;
    ble_gsc_addr_t target_addr;
    ble_gsc_addr_type_t target_addr_type;
    int8_t tx_pwr;
    int8_t rssi;
    uint8_t phy_prim;
    uint8_t phy_second;
    uint8_t adv_sid;
    uint16_t period_adv_intv;
    const uint8_t* p_data;
    uint16_t data_len;
} ble_gsc_adv_report_t;

typedef struct {
    uint16_t conn_intv_min;
    uint16_t conn_intv_max;
    uint16_t conn_latency;
    uint16_t sup_timeout;
    uint16_t ce_len_min;
    uint16_t ce_len_max;
} ble_gsc_conn_param_t;

typedef struct {
    uint8_t prop;
    uint16_t conn_to;
    uint16_t scan_intv;
    uint16_t scan_wd;
    ble_gsc_conn_param_t conn_param;
    ble_gsc_addr_t peer_addr;
    ble_gsc_addr_type_t peer_addr_type;
} ble_gsc_connect_param_t;

typedef enum {
    BLE_GSC_DISC_ALL_SVC = 0,
    BLE_GSC_DISC_SVC_BY_UUID,
} ble_gsc_disc_type_t;

typedef enum {
    BLE_GSC_WRITE = 0,
    BLE_GSC_WRITE_NO_RESP,
} ble_gsc_write_type_t;

typedef enum {
    BLE_GSC_EVT_SCAN_STARTED = 0,
    BLE_GSC_EVT_SCAN_STOPPED,
    BLE_GSC_EVT_ADV_REPORT,
    BLE_GSC_EVT_CONNECTING,
    BLE_GSC_EVT_CONNECT_FAILED,
    BLE_GSC_EVT_DISC_CMP,
    BLE_GSC_EVT_SVC_FOUND,
    BLE_GSC_EVT_CHAR_FOUND,
    BLE_GSC_EVT_DESC_FOUND,
    BLE_GSC_EVT_READ_CMP,
    BLE_GSC_EVT_READ_VAL,
    BLE_GSC_EVT_WRITE_CMP,
    BLE_GSC_EVT_NOTIFICATION,
    BLE_GSC_EVT_INDICATION,
} ble_gsc_evt_type_t;

typedef struct {
    uint16_t start_hdl;
    uint16_t end_hdl;
    uint8_t uuid_type;
    uint8_t uuid[BLE_GSC_UUID_128_LEN];
} ble_gsc_svc_info_t;

typedef struct {
    uint16_t char_hdl;
    uint16_t val_hdl;
    uint8_t prop;
    uint8_t uuid_type;
    uint8_t uuid[BLE_GSC_UUID_128_LEN];
} ble_gsc_char_info_t;

typedef struct {
    uint16_t desc_hdl;
    uint8_t uuid_type;
    uint8_t uuid[BLE_GSC_UUID_128_LEN];
} ble_gsc_desc_info_t;

typedef struct {
    uint16_t hdl;
    uint16_t offset;
    const uint8_t* p_data;
    uint16_t data_len;
} ble_gsc_read_data_t;

typedef struct {
    uint16_t hdl;
    const uint8_t* p_data;
    uint16_t data_len;
    bool complete;
} ble_gsc_ntf_ind_data_t;

typedef union {
    uint16_t status;
    ble_gsc_adv_report_t adv_report;
    ble_gsc_svc_info_t svc;
    ble_gsc_char_info_t charac;
    ble_gsc_desc_info_t desc;
    ble_gsc_read_data_t read;
    ble_gsc_ntf_ind_data_t ntf_ind;
} ble_gsc_evt_data_t;

typedef struct {
    ble_gsc_evt_type_t type;
    uint8_t conidx;
    ble_gsc_evt_data_t data;
} ble_gsc_evt_t;

typedef void (*ble_gsc_evt_cb_t)(const ble_gsc_evt_t* p_evt);

typedef struct {
    ble_gsc_evt_cb_t cb_evt;
} ble_gsc_callbacks_t;

ble_gsc_err_t ble_gsc_init(const ble_gsc_callbacks_t* p_cbs);

ble_gsc_err_t ble_gsc_scan_start(const ble_gsc_scan_param_t* p_param);
ble_gsc_err_t ble_gsc_scan_stop(void);
bool ble_gsc_is_scanning(void);

ble_gsc_err_t ble_gsc_connect(const ble_gsc_connect_param_t* p_param);
ble_gsc_err_t ble_gsc_connect_cancel(void);
bool ble_gsc_is_connecting(void);

ble_gsc_err_t ble_gsc_disconnect(uint8_t conidx);

ble_gsc_err_t ble_gsc_mtu_exchange(uint8_t conidx);

ble_gsc_err_t ble_gsc_discover_svc(uint8_t conidx, ble_gsc_disc_type_t disc_type,
                                    uint16_t start_hdl, uint16_t end_hdl,
                                    uint8_t uuid_type, const uint8_t* p_uuid);

ble_gsc_err_t ble_gsc_discover_char(uint8_t conidx, uint16_t start_hdl, uint16_t end_hdl,
                                     uint8_t uuid_type, const uint8_t* p_uuid);

ble_gsc_err_t ble_gsc_discover_desc(uint8_t conidx, uint16_t start_hdl, uint16_t end_hdl);

ble_gsc_err_t ble_gsc_read(uint8_t conidx, uint16_t hdl, uint16_t offset, uint16_t length);

ble_gsc_err_t ble_gsc_write(uint8_t conidx, ble_gsc_write_type_t type,
                              uint16_t hdl, const uint8_t* p_data, uint16_t len);

ble_gsc_err_t ble_gsc_event_register(uint8_t conidx, uint16_t start_hdl, uint16_t end_hdl);

ble_gsc_err_t ble_gsc_event_unregister(uint8_t conidx, uint16_t start_hdl, uint16_t end_hdl);

#endif /* BLE_SUPPORT */
#endif /* _BLE_GS_CENTRAL_H_ */
