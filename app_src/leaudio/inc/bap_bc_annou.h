/**
 ****************************************************************************************
 *
 * @file bap_bc_annou.h
 *
 * @brief Discover Broadcast Profile - Broadcast Announcer - Definitions
 *
 * Copyright (C) RivieraWaves 2009-2024
 * Release Identifier: c73db0ce
 *
 ****************************************************************************************
 */

#ifndef BAP_BC_ANNOU_H_
#define BAP_BC_ANNOU_H_

/**
 ****************************************************************************************
 * @defgroup BAP_BC_ANNOU Broadcast Announcer module
 * @ingroup BAP_BC
 * @brief Description of Broadcast Announcer module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup BAP_BC_ANNOU_ENUM Enumerations
 * @ingroup BAP_BC_ANNOU
 * @brief Enumerations for Broadcast Announcer module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup BAP_BC_ANNOU_NATIVE_API Native API
 * @ingroup BAP_BC_ANNOU
 * @brief Description of Native API for Broadcast Announcer module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup BAP_BC_ANNOU_CALLBACK Callback Functions
 * @ingroup BAP_BC_ANNOU_NATIVE_API
 * @brief Description of callback functions for Broadcast Announcer module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup BAP_BC_ANNOU_FUNCTION Functions
 * @ingroup BAP_BC_ANNOU_NATIVE_API
 * @brief Description of functions for Broadcast Announcer module
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "gaf.h"                // GAF Defines
#include "bap.h"                // broadcast announcer Profile Definitions
#include "bap_bc.h"             // broadcast announcer Profile Broadcast Definitions
#include "gap.h"                // GAP Definitions
#include "gapi.h"                // GAPI Definitions
/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// @addtogroup BAP_BC_ANNOU_ENUM
/// @{

/// List of command type values for Broadcast Announcer module
enum bap_bc_annou_cmd_type
{
    /// Enable Periodic Advertising for a Broadcast Group.
    BAP_BC_ANNOU_CMD_TYPE_ENABLE_PA = 0,
    /// Disable Periodic Advertising for a Broadcast Group.
    BAP_BC_ANNOU_CMD_TYPE_DISABLE_PA,
};

/// @} BAP_BC_ANNOU_ENUM

/*
 * CALLBACK FUNCTIONS DEFINITION
 ****************************************************************************************
 */

/// @addtogroup BAP_BC_ANNOU_CALLBACK
/// @{

/**
 ****************************************************************************************
 * @brief Callback function called each time a Broadcast Announcer command has been completed
 *
 * @param[in] cmd_type          Command type (see #bap_bc_annou_cmd_type enumeration)
 * @param[in] status            Status
 * @param[in] grp_lid           Group local index
 * @param[in] sgrp_lid          Subgroup local index
 ****************************************************************************************
 */
typedef void (*bap_bc_annou_cb_cmp_evt)(uint8_t cmd_type, uint16_t status);

/// Set of callback functions for Broadcast Source module
struct bap_bc_annou_cb
{
    /// Callback function called when a command has been completed
    bap_bc_annou_cb_cmp_evt cb_cmp_evt;
};

/// @} BAP_BC_ANNOU_CALLBACK

/*
 * API FUNCTIONS DECLARATION
 ****************************************************************************************
 */

#if 1//(GAF_BAP_BC_SRC)
/// @addtogroup BAP_BC_ANNOU_FUNCTION
/// @{

/**
 ****************************************************************************************
 * @brief Configure use of BAP Broadcast Announcer module
 *
 * @param[in] p_cb          Pointer to set to callback functions for communication with upper layer
 *
 * @return An error status (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t bap_bc_annou_configure(const bap_bc_annou_cb_t* p_cb, uint8_t path_loss_threshold,
                                int8_t radiated_tx_power);

/**
 ****************************************************************************************
 * @brief Enable Periodic Advertising
 *
 * @param[in] grp_lid            Group local index.
 * @param[in] adv_data_len       Length of additional Advertising Data
 * @param[in] per_adv_data_len   Length of additional Periodic Advertising Data
 * @param[in] p_adv_data         Pointer to additional Advertising Data
 * @param[in] p_per_adv_data     Pointer to additional Periodic Advertising Data
 *                               Memory containing the additional Periodic Advertising Data must have been allocated
 *                               by the upper layer
 * @param[in] broadcast_name_len Length of Public Broadcast Name
 * @param[in] p_broadcast_name   Pointer to Public Broadcast Name
 * @param[in] metadata_len       Length of Public Broadcast Metadata
 * @param[in] p_metadata         Pointer to Public Broadcast Metadata
 *
 * @return An error status (see #gaf_err enumeration) (see #gaf_err enumeration).
 *         If no error, no new function should be executed until @see bap_bc_annou_cb_cmp_evt callback function with
 *         BAP_BC_ANNOU_ENABLE_PA command code is called.
 ****************************************************************************************
 */
uint16_t bap_bc_annou_enable_pa(const bap_bc_adv_param_t* p_adv_param, const bap_bc_per_adv_param_t* p_per_adv_param,
                              uint8_t num_subgroup, uint8_t metadata_len, const uint8_t* p_metadata,
                              const gap_bdaddr_t* p_src_addr, uint16_t add_adv_data_len, uint16_t add_per_adv_data_len,
                              const uint8_t* p_add_adv_data, const uint8_t* p_add_per_adv_data,
                              uint8_t subg_metadata_len, const uint8_t* p_subg_metadata);

/**
 ****************************************************************************************
 * @brief Disable Periodic Advertising for a Broadcast Group.
 *
 * @param[in] grp_lid            Group local index
 *
 * @return An error status (see #gaf_err enumeration) (see #gaf_err enumeration).
 *         If no error, no new function should be executed until @see bap_bc_annou_cb_cmp_evt callback function with
 *         BAP_BC_ANNOU_DISABLE_PA command code is called.
 ****************************************************************************************
 */
uint16_t bap_bc_annou_disable_pa(void);

/// @} BAP_BC_ANNOU_FUNCTION
#endif //(GAF_BAP_BC_SRC)

#endif // BAP_BC_ANNOU_H_
