/**
 *@file ble_gs_service.h
 * @author Cole (cole@mvsilicon.com)
 * @brief 
 * @version 0.1
 * @date 2025-12-25
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef _BLE_GS_SERVICE_H_
#define _BLE_GS_SERVICE_H_

#include "bt_config.h"
#if (BLE_SUPPORT)

#include <stdint.h>
#include <stdbool.h>
#include "ble_api.h"
//#include "ble_app_gatt.h"

#define BLE_GS_MAX_CONN         4
#define BLE_GS_ADV_DATA_MAX     31
#define BLE_GS_EXT_ADV_DATA_MAX 254     // 扩展广播最大数据长度

typedef enum {
    BLE_GS_UUID_TYPE_16 = 0,
    BLE_GS_UUID_TYPE_128,
} ble_gs_uuid_type_t;

typedef enum {
    BLE_GS_OK = 0,
    BLE_GS_ERR_PARAM,
    BLE_GS_ERR_NO_RESOURCE,
    BLE_GS_ERR_NOT_ALLOWED = 5,
    BLE_GS_ERR_NOT_CONN,
    BLE_GS_ERR_BUSY,
    BLE_GS_ERR_INTERNAL = 0xFF,
} ble_gs_err_t;

typedef enum {
    BLE_GS_IO_DISPLAY_ONLY = 0,
    BLE_GS_IO_DISPLAY_YESNO,
    BLE_GS_IO_KEYBOARD_ONLY,
    BLE_GS_IO_NO_IO,
    BLE_GS_IO_KEYBOARD_DISPLAY,
} ble_gs_io_cap_t;

/* 扩展广播类型 */
typedef enum {
    BLE_GS_EXT_ADV_CONN_SCAN = 0,       // 可连接可扫描 (Legacy兼容)
    BLE_GS_EXT_ADV_CONN_NOSCAN,         // 可连接不可扫描 (仅Extended)
    BLE_GS_EXT_ADV_NONCONN_SCAN,        // 不可连接可扫描
    BLE_GS_EXT_ADV_NONCONN_NOSCAN,      // 不可连接不可扫描
} ble_gs_ext_adv_type_t;

/* PHY类型 */
typedef enum {
    BLE_GS_PHY_1M = 0x01,
    BLE_GS_PHY_2M = 0x02,
    BLE_GS_PHY_CODED = 0x04,
} ble_gs_phy_t;

/* 扩展广播参数 */
typedef struct {
    ble_gs_ext_adv_type_t adv_type;
    uint16_t adv_intv_min;              // 单位: 0.625ms
    uint16_t adv_intv_max;
    uint8_t ch_map;                     // 0x07 = 所有通道
    int8_t max_tx_pwr;                  // 最大发送功率 (dBm)
    ble_gs_phy_t primary_phy;           // 主广播PHY (1M/Coded)
    ble_gs_phy_t secondary_phy;         // 次广播PHY (1M/2M/Coded)
    uint8_t adv_sid;                    // 广播SID (0-15)
} ble_gs_ext_adv_param_t;

/* 扩展广播数据 */
typedef struct {
    uint8_t* p_adv_data;                // 广播数据指针
    uint16_t adv_data_len;              // 广播数据长度 (最大254)
    uint8_t* p_scan_rsp;                // 扫描响应数据指针
    uint16_t scan_rsp_len;              // 扫描响应数据长度 (最大254)
} ble_gs_ext_adv_data_t;

typedef void (*ble_gs_rx_cb_t)(uint8_t conidx, uint16_t handle, uint16_t len, const uint8_t* data);
typedef void (*ble_gs_read_cb_t)(uint8_t conidx, uint16_t handle, uint16_t offset, uint16_t max_len, uint32_t token);
typedef void (*ble_gs_cccd_cb_t)(uint8_t conidx, uint16_t handle, uint16_t cfg);
typedef void (*ble_gs_tx_cmp_cb_t)(uint8_t conidx, uint16_t status);
typedef void (*ble_gs_conn_cb_t)(uint8_t conidx, bool connected, uint16_t reason);
typedef void (*ble_gs_mtu_cb_t)(uint8_t conidx, uint16_t mtu);
typedef void (*ble_gs_adv_cb_t)(bool started, uint16_t status);

typedef struct {
    ble_gs_rx_cb_t      cb_rx;
    ble_gs_read_cb_t    cb_read;
    ble_gs_cccd_cb_t    cb_cccd;
    ble_gs_tx_cmp_cb_t  cb_ntf_cmp;
    ble_gs_tx_cmp_cb_t  cb_ind_cmp;
    ble_gs_conn_cb_t    cb_conn;
    ble_gs_mtu_cb_t     cb_mtu;
} ble_gs_callbacks_t;

typedef ble_profile_add_t ble_gs_profile_t;

#define BLE_GS_PROFILE_16(uuid, db) \
    { UUID_TYPE_16, sizeof(db)/sizeof(ble_gatt_att16_desc_t), uuid, (ble_gatt_att16_desc_t*)(db), NULL, NULL }

#define BLE_GS_PROFILE_128(uuid_arr, db) \
    { UUID_TYPE_128, sizeof(db)/sizeof(ble_gatt_att128_desc_t), 0, NULL, (uint8_t*)(uuid_arr), (ble_gatt_att128_desc_t*)(db) }

/*
 * API
 ****************************************************************************************
 */

ble_gs_err_t ble_gs_init(const ble_gs_profile_t* p_profiles, uint16_t cnt, const ble_gs_callbacks_t* p_cbs);
void ble_gs_set_dev_name(const uint8_t* p_name, uint8_t len);
void ble_gs_set_adv_data(const uint8_t* p_data, uint8_t len);
void ble_gs_set_scan_rsp(const uint8_t* p_data, uint8_t len);
void ble_gs_set_adv_interval(uint16_t min, uint16_t max);
ble_gs_err_t ble_gs_adv_start(void);
ble_gs_err_t ble_gs_adv_stop(void);
ble_gs_err_t ble_gs_disconnect(uint8_t conidx);
ble_gs_err_t ble_gs_notify(uint8_t conidx, uint16_t handle, const uint8_t* data, uint16_t len);
ble_gs_err_t ble_gs_indicate(uint8_t conidx, uint16_t handle, const uint8_t* data, uint16_t len);
ble_gs_err_t ble_gs_read_rsp(uint8_t conidx, uint32_t token, uint16_t status, uint16_t len, const uint8_t* data);
uint16_t ble_gs_get_mtu(uint8_t conidx);
bool ble_gs_is_connected(uint8_t conidx);
uint8_t ble_gs_get_conn_count(void);
void ble_gs_set_pair_param(ble_gs_io_cap_t io_cap, bool auth_req, bool bond_req);
uint8_t ble_gs_build_adv_data(uint8_t* buf, const uint8_t* name, uint8_t name_len, 
                              const uint16_t* uuid16, uint8_t uuid16_cnt, const uint8_t* uuid128);

void ble_gs_set_adv_callback(ble_gs_adv_cb_t cb);
bool ble_gs_is_advertising(void);
ble_gs_err_t ble_gs_conn_param_update(uint8_t conidx, uint16_t interval, uint16_t latency, uint16_t timeout);
ble_gs_err_t ble_gs_get_peer_addr(uint8_t conidx, uint8_t* p_addr);

ble_gs_err_t ble_gs_mtu_exchange(uint8_t conidx);  // 请求MTU交换（最大247）
ble_gs_err_t ble_gs_set_phy(uint8_t conidx, ble_gs_phy_t tx_phy, ble_gs_phy_t rx_phy);  // 设置PHY（2M更快）
ble_gs_err_t ble_gs_set_data_len(uint8_t conidx, uint16_t tx_octets, uint16_t tx_time);  // 设置DLE

ble_gs_err_t ble_gs_ext_adv_set_data(const ble_gs_ext_adv_data_t* p_data);
ble_gs_err_t ble_gs_ext_adv_start(const ble_gs_ext_adv_param_t* p_param);
ble_gs_err_t ble_gs_ext_adv_stop(void);

typedef struct {
    uint16_t adv_intv_min;          // 扩展广播间隔 (单位: 0.625ms)
    uint16_t adv_intv_max;
    uint16_t period_intv_min;       // 周期广播间隔 (单位: 1.25ms, 最小7.5ms即6)
    uint16_t period_intv_max;
    uint8_t ch_map;                 // 通道掩码 (0x07=全部)
    int8_t max_tx_pwr;
    ble_gs_phy_t primary_phy;
    ble_gs_phy_t secondary_phy;
    uint8_t adv_sid;                // 广播SID (用于同步)
} ble_gs_periodic_adv_param_t;

/* 周期性广播数据 */
typedef struct {
    uint8_t* p_ext_adv_data;        // 扩展广播数据 (用于发现)
    uint16_t ext_adv_data_len;
    uint8_t* p_periodic_data;       // 周期性广播数据 (主要数据，接收端同步后可收到)
    uint16_t periodic_data_len;
} ble_gs_periodic_adv_data_t;

ble_gs_err_t ble_gs_periodic_adv_set_data(const ble_gs_periodic_adv_data_t* p_data);
ble_gs_err_t ble_gs_periodic_adv_start(const ble_gs_periodic_adv_param_t* p_param);
ble_gs_err_t ble_gs_periodic_adv_stop(void);

/*
 * 应用层 API (ble_gs_app.c)
 */
void ble_gs_app_init(void);
int ble_gs_app_send(const uint8_t* data, uint16_t len);
bool ble_gs_app_is_connected(void);
bool ble_gs_app_is_configured(void);
uint16_t ble_gs_app_start_adv(void);
uint16_t ble_gs_app_stop_adv(void);
void ble_gs_app_set_name(const uint8_t* name, uint8_t len);
bool ble_gs_app_is_paired(void);
bool ble_gs_app_is_encrypted(void);
uint8_t ble_gs_app_get_role_p_conidx(void);
bool ble_gs_app_is_ntf_enabled(void);
void ble_gs_app_clear_bond(void);

#endif /* BLE_SUPPORT */
#endif /* _BLE_GS_SERVICE_H_ */
