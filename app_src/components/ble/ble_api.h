/**
 * @file ble_api.h
 * @brief BLE API类型定义
 */
#ifndef _LE_APP_API_H_
#define _LE_APP_API_H_

#include <stdint.h>
#include <stdio.h>
#include "gapm_le.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define BLE_INFO(fmt, args...) printf("[BLE_INFO]:" fmt, ##args)
#define BLE_GATT_UUID_128_LEN (16)
#define BLE_DFLT_DEVICE_MAX_NAME_LEN (18)

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// 16位UUID属性描述
typedef struct ble_gatt_att16_desc
{
    uint16_t uuid16;    ///< 16位UUID (LSB First)
    uint16_t info;      ///< 属性信息
    uint16_t ext_info;  ///< 扩展信息
} ble_gatt_att16_desc_t;

/// 128位UUID属性描述
typedef struct ble_gatt_att128_desc
{
    uint8_t uuid[BLE_GATT_UUID_128_LEN]; ///< 128位UUID (LSB First)
    uint16_t info;      ///< 属性信息
    uint16_t ext_info;  ///< 扩展信息
} ble_gatt_att128_desc_t;

/// UUID类型
typedef enum
{
    UUID_TYPE_16 = 0,
    UUID_TYPE_128,
} uuid_type_t;

/// Profile配置
typedef struct ble_profile_add
{
    uuid_type_t type;
    uint16_t size;
    uint16_t att16_service;
    ble_gatt_att16_desc_t* att_desc16;
    uint8_t* att128_service;
    ble_gatt_att128_desc_t* att_desc128;
} ble_profile_add_t;

/// 广播间隔参数
typedef struct set_adv_interval
{
    uint32_t adv_intv_min;
    uint32_t adv_intv_max;
    uint8_t ch_map;
} set_adv_interval_t;

/// 连接参数
typedef struct le_connection_param
{
    uint16_t intv_min;
    uint16_t intv_max;
    uint16_t latency;
    uint16_t time_out;
} le_connection_param_t;

/*
 * API DECLARATIONS
 ****************************************************************************************
 */

/// 获取设备名称
uint8_t app_get_dev_name(uint8_t *p_name);

/// 设置广播参数
void app_set_adv_param(set_adv_interval_t adv_param);

/// 更新连接参数
void app_update_param(struct le_connection_param *p_conn_param);

#endif /* _LE_APP_API_H_ */
