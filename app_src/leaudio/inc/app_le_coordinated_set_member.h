/**
 ****************************************************************************************
 *
 * @file app_le_coordinated_set_member.h
 *
 * @brief Applet - Coordinated Set Member - Header file
 *
 * Copyright (C) RivieraWaves 2009-2024
 * Release Identifier: c73db0ce
 *
 ****************************************************************************************
 */
#include "le_audio_config.h"
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
#ifndef APP_LE_COORDINATED_SET_MEMBER_H_
#define APP_LE_COORDINATED_SET_MEMBER_H_

/**
 ****************************************************************************************
 * @defgroup APP_LE_COORDINATED_SET_MEMBER Coordinated Set Member
 * @ingroup APP_LE
 * @brief Applet adding support of Coordinated Set Identification Service as Set Member
 *
 * Only one Coordinated Set is supported.\n
 * A Mediator event #COMPONENT_EVENT_RSI_UPDATED is raised each time RSI value is updated.
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>
/// @addtogroup APP_LE_COORDINATED_SET_MEMBER
/// @{

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Event type values
enum app_le_coordinated_set_member_event_type
{
    /// Resolvable Set Identifier (RSI) value has been update
    ///     - Type: uint8_t*
    COMPONENT_EVENT_RSI_UPDATED,
    /// CSIS has been added
    APP_LE_COORDINATED_SET_MEMBER_EVENT_ADDED,
};

/*
 * TYPES DEFINITIONS
 ****************************************************************************************
 */

/// Parameters structure for Mediator configuration
typedef struct app_le_coordinated_set_member_cfg
{
    /// Pointer to SIRK\n
    /// Shall not be NULL
    const uint8_t* p_sirk;
    /// Number of devices in the Coordinated Set\n
    /// 0 means that Coordinated Set Size characteristic is not supported
    uint8_t nb_members;
    /// Rank of device in the Coordinated Set\n
    /// 0 means that Set Member Rank characteristic is not supported
    /// Shall not be 0 if lock_timeout_s is also set to 0\n
    uint8_t rank;
    /// Lock timeout (in seconds)
    /// 0 means that Set Member Lock characteristic is not supported
    uint8_t lock_timeout_s;
} app_le_coordinated_set_member_cfg_t;

/*
 * INTERFACES
 ****************************************************************************************
 */


/// @} APP_LE_COORDINATED_SET_MEMBER

#endif // APP_LE_COORDINATED_SET_MEMBER_H_
#endif