/**
 * @file ble_app_func.h
 * @brief BLE GATT属性宏定义
 */
#ifndef __BLE_APP_FUNC_H__
#define __BLE_APP_FUNC_H__

#include "type.h"
#include "ble_api.h"
#include "gap.h"
#include "gatt.h"

/*
 * DEFINES
 ****************************************************************************************
 */

#define GATT_HANDLE_LEN      (2)
#define GATT_UUID_16_LEN     (2)
#define GATT_UUID_32_LEN     (4)
#define GATT_UUID_128_LEN    (16)
#define GATT_DB_HASH_LEN     (16)

/*
 * GATT Service Info Bit Field
 ****************************************************************************************
 */

enum ble_gatt_svc_info_bf
{
    GATT_SVC_SEC_LVL_MASK = 0x03,
    GATT_SVC_SEC_LVL_LSB = 0,
    GATT_SVC_EKS_BIT = 0x04,
    GATT_SVC_EKS_POS = 2,
    GATT_SVC_DIS_BIT = 0x08,
    GATT_SVC_DIS_POS = 3,
    GATT_SVC_HIDE_BIT = 0x10,
    GATT_SVC_HIDE_POS = 4,
    GATT_SVC_UUID_TYPE_MASK = 0x60,
    GATT_SVC_UUID_TYPE_LSB = 5,
};

/*
 * GATT Attribute Info Bit Field
 ****************************************************************************************
 */

enum ble_gatt_att_info_bf
{
    GATT_ATT_B_BIT = 0x0001,
    GATT_ATT_B_POS = 0,
    GATT_ATT_RD_BIT = 0x0002,
    GATT_ATT_RD_POS = 1,
    GATT_ATT_WC_BIT = 0x0004,
    GATT_ATT_WC_POS = 2,
    GATT_ATT_WR_BIT = 0x0008,
    GATT_ATT_WR_POS = 3,
    GATT_ATT_N_BIT = 0x0010,
    GATT_ATT_N_POS = 4,
    GATT_ATT_I_BIT = 0x0020,
    GATT_ATT_I_POS = 5,
    GATT_ATT_WS_BIT = 0x0040,
    GATT_ATT_WS_POS = 6,
    GATT_ATT_EXT_BIT = 0x0080,
    GATT_ATT_EXT_POS = 7,
    GATT_ATT_RP_MASK = 0x0300,
    GATT_ATT_RP_LSB = 8,
    GATT_ATT_WP_MASK = 0x0C00,
    GATT_ATT_WP_LSB = 10,
    GATT_ATT_NIP_MASK = 0x3000,
    GATT_ATT_NIP_LSB = 12,
    GATT_ATT_UUID_TYPE_MASK = 0xC000,
    GATT_ATT_UUID_TYPE_LSB = 14,
    GATT_ATT_PROP_MASK = 0x00FF,
    GATT_ATT_PROP_LSB = 0,
};

enum ble_gatt_att_ext_info_bf
{
    GATT_ATT_WRITE_MAX_SIZE_MASK = 0x7FFF,
    GATT_ATT_WRITE_MAX_SIZE_LSB = 0,
    GATT_ATT_NO_OFFSET_BIT = 0x8000,
    GATT_ATT_NO_OFFSET_POS = 15,
    GATT_INC_SVC_HDL_BIT = 0xFFFF,
    GATT_INC_SVC_HDL_POS = 0,
    GATT_ATT_EXT_PROP_VALUE_MASK = 0xFFFF,
    GATT_ATT_EXT_PROP_VALUE_LSB = 0,
};

/*
 * MACRO DEFINITIONS
 ****************************************************************************************
 */

/// 属性属性宏
#define PROP(prop)          (GATT_PROP_##prop##_BIT)

/// 属性选项宏
#define OPT(opt)            (GATT_ATT_##opt##_BIT)

/// 属性安全级别宏
#define SEC_LVL(perm, lvl_name)  (((GAP_SEC_##lvl_name) << (GATT_ATT_##perm##_LSB)) & (GATT_ATT_##perm##_MASK))
#define SEC_LVL_VAL(perm, lvl_val)  (((lvl_val) << (GATT_ATT_##perm##_LSB)) & (GATT_ATT_##perm##_MASK))

/// UUID类型宏
#define ATT_UUID(uuid_type)      (((GATT_UUID_##uuid_type) << (GATT_ATT_UUID_TYPE_LSB)) & (GATT_ATT_UUID_TYPE_MASK))

/// 服务安全级别宏
#define SVC_SEC_LVL(lvl_name)    (((GAP_SEC_##lvl_name) << (GATT_SVC_SEC_LVL_LSB)) & (GATT_SVC_SEC_LVL_MASK))
#define SVC_SEC_LVL_VAL(lvl_val) (((lvl_val) << (GATT_SVC_SEC_LVL_LSB)) & (GATT_SVC_SEC_LVL_MASK))
#define SVC_UUID(uuid_type)      (((GATT_UUID_##uuid_type) << (GATT_SVC_UUID_TYPE_LSB)) & (GATT_SVC_UUID_TYPE_MASK))

/*
 * API DECLARATIONS
 ****************************************************************************************
 */

/// BLE应用初始化入口
void BleAppInit(void);

#endif /* __BLE_APP_FUNC_H__ */
