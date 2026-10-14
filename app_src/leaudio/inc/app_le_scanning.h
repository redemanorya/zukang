/**
 ****************************************************************************************
 *
 * @file app_le_gaf_scanning.h
 *
 * @brief Applet - GAF Scanning - Header file
 *
 * Copyright (C) RivieraWaves 2009-2024
 * Release Identifier: c73db0ce
 *
 ****************************************************************************************
 */
#include "le_audio_config.h"
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
#ifndef APP_LE_GAF_SCANNING_H_
#define APP_LE_GAF_SCANNING_H_

/**
 ****************************************************************************************
 * @defgroup APP_LE_GAF_SCANNING GAF Scanning
 * @ingroup APP_LE
 * @brief GAF Scanning Applet description
 *
 * Applet adding support of Bluetooth Low Energy scanning for GAF.\n
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "atc_csi.h"
#include "gap.h"

/// @addtogroup APP_LE_GAF_SCANNING
/// @{

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Parameters structure for #COMPONENT_CONTROL_START control type
typedef struct app_le_gaf_scanning_start
{
    /// Indicate if RSI is needed
    uint8_t is_rsi_needed;
} app_le_gaf_scanning_start_t;

/// Parameters structure for #COMPONENT_EVENT_DEVICE_FOUND event type
typedef struct app_le_gaf_scanning_device_found
{
    /// Pointer to device address
    const gap_bdaddr_t* p_addr;
    /// Device name
    const uint8_t* p_device_name;
    /// Name length
    uint8_t name_length;
    /// Pointer to RSI value
    const atc_csis_rsi_t* p_rsi;
} app_le_gaf_scanning_device_found_t;

/// Parameters structure for #COMPONENT_EVENT_DEVICE_FOUND event type
typedef struct app_scanning_device_found
{
    /// Pointer to device address
    const gap_bdaddr_t* p_addr;
} app_scanning_device_found_t;

/// @} APP_LE_GAF_SCANNING

#endif // APP_LE_GAF_SCANNING_H_
#endif