/**
 ****************************************************************************************
 *
 * @file le_udsc.h
 *
 * @brief 
 *
 * Copyright (C) RivieraWaves 2009-2024
 * Release Identifier: c73db0ce
 *
 ****************************************************************************************
 */

#ifndef LE_UDSC_H_
#define LE_UDSC_H_
#include "gap.h"

/// Patter allowing to filter reception of Solicitation Requests
#define UDSC_PATTERN_SIZE             (3)
/// 1M PHY default Scan interval (10ms) in slots
#define  BAP_UDS_SCAN_INTERVAL_1M_DEFAULT   (16)
/// 1M PHY default Scan window (5ms) in slots
#define BAP_UDS_SCAN_WINDOW_1M_DEFAULT        (8)

/// Set of callback functions for user data service client module
typedef struct le_udsc_cb
{
    void (*cb_scan_report)(uint8_t addr_type, const uint8_t* p_addr, uint16_t length,
                                              const uint8_t* p_adv_data);
    void (*cb_read_cmp)(uint8_t conidx, uint16_t status, uint8_t val_id, const union uds_value* p_value);
    void (*cb_write_cmp)(uint8_t conidx, uint16_t status, uint8_t val_id);
    void (*cb_db_chg_inc)(uint8_t conidx, uint32_t value);
} le_udsc_cb_t;

/// Broadcast Assistant environment structure
typedef struct udsc_env
{
    /// Pointer to set of callback functions for communication with upper layers
    const le_udsc_cb_t* p_cb;

    /// Scan activity local index
    uint8_t scan_act_lid;
} udsc_env_t;

typedef struct app_udsc_found
{
    /// Pointer to device address
    const gap_bdaddr_t addr;
} app_udsc_found_t;

__STATIC udsc_env_t* p_udsc_env;

uint16_t le_udsc_start_scan(uint16_t timeout_s);
uint16_t le_udsc_stop_scan(void);
uint16_t le_udsc_configure(const le_udsc_cb_t* p_cb, uint8_t open_scan_udsc);
#endif // LE_UDSC_H_
