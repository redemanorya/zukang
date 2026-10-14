/**
 ****************************************************************************************
 *
 * @file app_le_gaf_advertising.h
 *
 * @brief Applet - GAF Advertising - Header file
 *
 *
 ****************************************************************************************
 */
#include "le_audio_config.h"
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
#ifndef APP_LE_GAF_ADVERTISING_H_
#define APP_LE_GAF_ADVERTISING_H_

/**
 ****************************************************************************************
 * @defgroup APP_LE_GAF_ADVERTISING GAF Advertising
 * @ingroup APP_LE
 * @brief GAF Advertising Applet description
 *
 * Applet adding support of Bluetooth Low Energy advertising for GAF.\n
 *
 * Limitations:\n
 *     - Can be bound with only one Output Provider\n
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>

/// @addtogroup APP_LE_GAF_ADVERTISING
/// @{

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Parameters structure for #COMPONENT_CONTROL_START control type
typedef struct app_le_gaf_advertising_start
{
    /// Indicate if advertising is a Solicitation Request
    uint8_t is_solicitation;
} app_le_gaf_advertising_start_t;

/// @} APP_LE_GAF_ADVERTISING

#endif // APP_LE_GAF_ADVERTISING_H_
#endif