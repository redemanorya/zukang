
#ifndef __BLE_APP_GATT_H__
#define __BLE_APP_GATT_H__

#include "ble_app_func.h"
#include "ble_api.h"

#define DEFAULT_MTU_SIZE (250)  // 250
#define LE_VAL_MAX_LEN (DEFAULT_MTU_SIZE-3)

#define RD_P    (PROP(RD)  | SEC_LVL(RP, NOT_ENC))
/// default write without response perm
#define WC_P    (PROP(WC)  | SEC_LVL(WP, NOT_ENC))
/// default write perm
#define WR_P    (PROP(WR)  | SEC_LVL(WP, NOT_ENC))
/// default notify perm
#define NTF_P   (PROP(N)   | SEC_LVL(NIP, NOT_ENC))
/// ind perm
#define IND_P   (PROP(I)   | SEC_LVL(NIP, NOT_ENC))


#define ATT_DECL_PRIMARY_SERVICE    0x2800
#define ATT_DECL_CHARACTERISTIC     0x2803
#define ATT_DESC_CLIENT_CHAR_CFG    0x2902

#define UUID16_1800_SERVICE (0x1800)
#define ATT16_DB_1800_SIZE (sizeof(ATT16_DB_1800)/sizeof(ble_gatt_att16_desc_t))
static ble_gatt_att16_desc_t ATT16_DB_1800[] = {
    // ATT UUID
    //  | Permission			| EXT PERM | MAX ATT SIZE
    {ATT_DECL_PRIMARY_SERVICE, RD_P, 0},

    {ATT_DECL_CHARACTERISTIC, RD_P,0},
    {0x2a00, RD_P | WR_P, LE_VAL_MAX_LEN},

};

#define UUID16_AB00_SERVICE (0xAB00)
#define ATT16_DB_AB00_SIZE (sizeof(ATT16_DB_AB00)/sizeof(ble_gatt_att16_desc_t))
static ble_gatt_att16_desc_t ATT16_DB_AB00[] = {
    // ATT UUID
    //  | Permission			| EXT PERM | MAX ATT SIZE
    {ATT_DECL_PRIMARY_SERVICE, RD_P, 0},

    {ATT_DECL_CHARACTERISTIC, RD_P,0},
    {0xAB01, RD_P | WR_P | WC_P, LE_VAL_MAX_LEN},

    {ATT_DECL_CHARACTERISTIC, RD_P,0},
    {0xAB02, NTF_P, LE_VAL_MAX_LEN},
    {ATT_DESC_CLIENT_CHAR_CFG, RD_P | WR_P, OPT(NO_OFFSET)},//client characteristic configuration descriptor

    {ATT_DECL_CHARACTERISTIC, RD_P,0},
    {0xAB03, NTF_P, LE_VAL_MAX_LEN},
    {ATT_DESC_CLIENT_CHAR_CFG, RD_P | WR_P, OPT(NO_OFFSET)},//client characteristic configuration descriptor

};

#define GATT_DECL_PRIMARY_SERVICE2 \
    { 0x00, 0x28 }
#define GATT_DECL_CHARACTERISTIC_128UUID2 \
    { 0x03, 0x28 }
#define GATT_DESC_CLIENT_CHAR_CFG_128UUID2 \
    { 0x02, 0x29 }
#define GATT_DESC_CHAR_USER_DESCRIPTION_128UUID2 \
    { 0x01, 0x29 }

#define PRIMARY_SERVICE_0000FD50_0000_1000_8000_00805F9B34FB_UUID128                                            \
    {                                                                    \
        0xFB,  0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10,0x00,\
        0x00, 0x50, 0xFD, 0x00, 0x00,                                    \
    }
#define CHARACTERISTIC_00000001_0000_1001_8001_00805F9B07D0_UUID128                                            \
    {                                                                    \
        0xD0,  0x07, 0x9B, 0x5F, 0x80, 0x00, 0x01, 0x80, 0x01, 0x10,0x00,\
        0x00, 0x01, 0x00, 0x00, 0x00,                                    \
    }
#define CHARACTERISTIC_00000002_0000_1001_8001_00805F9B07D0_UUID128                                            \
    {                                                                    \
        0xD0,  0x07, 0x9B, 0x5F, 0x80, 0x00, 0x01, 0x80, 0x01, 0x10,0x00,\
        0x00, 0x02, 0x00, 0x00, 0x00,                                    \
    }
#define CHARACTERISTIC_00000003_0000_1001_8001_00805F9B07D0_UUID128                                            \
    {                                                                    \
        0xD0,  0x07, 0x9B, 0x5F, 0x80, 0x00, 0x01, 0x80, 0x01, 0x10,0x00,\
        0x00, 0x03, 0x00, 0x00, 0x00,                                    \
    }


static uint8_t ATT128_SERVICE_0000FD50_0000_1000_8000_00805F9B34FB[GATT_UUID_128_LEN] = PRIMARY_SERVICE_0000FD50_0000_1000_8000_00805F9B34FB_UUID128;
#define ATT128_DB_0000FD50_0000_1000_8000_00805F9B34FB_SIZE (sizeof(ATT128_DB_0000FD50_0000_1000_8000_00805F9B34FB)/sizeof(ble_gatt_att128_desc_t))
// gatt_att_desc
static  ble_gatt_att128_desc_t ATT128_DB_0000FD50_0000_1000_8000_00805F9B34FB[] = {

    {GATT_DECL_PRIMARY_SERVICE2, RD_P, 0},

    {GATT_DECL_CHARACTERISTIC_128UUID2, PROP(RD), 0},
    {CHARACTERISTIC_00000001_0000_1001_8001_00805F9B07D0_UUID128, WC_P | ATT_UUID(128),LE_VAL_MAX_LEN},

    {GATT_DECL_CHARACTERISTIC_128UUID2, PROP(RD), 0},
    {CHARACTERISTIC_00000002_0000_1001_8001_00805F9B07D0_UUID128, NTF_P | ATT_UUID(128),LE_VAL_MAX_LEN},
    {GATT_DESC_CLIENT_CHAR_CFG_128UUID2, RD_P | WR_P, OPT(NO_OFFSET)},

    {GATT_DECL_CHARACTERISTIC_128UUID2, PROP(RD), 0},
    {CHARACTERISTIC_00000003_0000_1001_8001_00805F9B07D0_UUID128, RD_P | ATT_UUID(128),LE_VAL_MAX_LEN},
};
typedef enum
{
    UUID_2A00_DEVICE_NAME_HANDLE = 0x3,
    UUID_AB01_HANDLE = 0x6,
    UUID_AB02_HANDLE = 0x8,
    UUID_AB02_CFG_HANDLE = 0x9,
    UUID_AB03_HANDLE = 0xb,
    UUID_AB03_CFG_HANDLE = 0xc,
    UUID_00000001_0000_1001_8001_00805F9B07D0_HANDLE = 0xf,
    UUID_00000002_0000_1001_8001_00805F9B07D0_HANDLE = 0x11,
    UUID_00000002_0000_1001_8001_00805F9B07D0_CFG_HANDLE = 0x12,
    UUID_00000003_0000_1001_8001_00805F9B07D0_HANDLE = 0x14,
} profile_att_handle;


// 声明 (定义在 ble_gs_app.c)
extern ble_profile_add_t mv_profile1[];
extern const uint16_t MV_profile_db_size;


#endif/*__BLE_APP_GATT_H__*/
