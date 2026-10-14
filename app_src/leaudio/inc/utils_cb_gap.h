/**
 ****************************************************************************************
 *
 * @file utils_cb_gap.h
 *
 * @brief Application - GAP Callback Functions Manager - Header file
 *
 * Copyright (C) RivieraWaves 2009-2024
 * Release Identifier: c73db0ce
 *
 ****************************************************************************************
 */

#ifndef UTILS_CB_GAP_H_
#define UTILS_CB_GAP_H_

/**
 ****************************************************************************************
 * @defgroup CB_GAP GAP Callback Functions
 * @ingroup UTILS_ENV
 * @brief Description of GAP Callback Functions manager
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "gapm.h"
#include "gapc.h"
#include "gapc_sec.h"
#if (BLE_HOST_PRESENT)
#include "gapc_le.h"
#endif //(BLE_HOST_PRESENT)

/// @addtogroup CB_GAP
/// @{

/*
 * FUNCTIONS
 ****************************************************************************************
 */

#if (BLE_HOST_PRESENT)
/**
 ****************************************************************************************
 * @brief Register LE connection related callback functions
 *
 * @param[in] p_cbs_le_config    Pointer to set of callback functions
 ****************************************************************************************
 */
void utils_cb_gap_register_le_config(const gapc_le_config_cb_t* p_cbs_le_config);
#endif //(BLE_HOST_PRESENT)


/**
 ****************************************************************************************
 * @brief Register security related callback functions
 *
 * @param[in] p_cbs_security    Pointer to set of callback functions
 ****************************************************************************************
 */
void utils_cb_gap_register_security(const gapc_security_cb_t* p_cbs_security);

/**
 ****************************************************************************************
 * @brief Register connection information related callback functions
 *
 * @param[in] is_low_energy            Is provided set of callback functions for Low Energy connection ?
 * @param[in] p_cbs_connection_info    Pointer to set of callback functions
 ****************************************************************************************
 */
void utils_cb_gap_register_connection_info(bool is_low_energy,
                                           const gapc_connection_info_cb_t* p_cbs_connection_info);

/**
 ****************************************************************************************
 * @brief Register callback functions allowing to be informed when connection establishment is required
 *
 * @param[in] p_cbs_connection_req    Pointer to set of callback functions
 ****************************************************************************************
 */
void utils_cb_gap_register_connection_req(const gapc_connection_req_cb_t* p_cbs_connection_req);

/**
 ****************************************************************************************
 * @return Sets of callback functions to provide to GAP during device configuration
 ****************************************************************************************
 */
const gapm_callbacks_t* utils_cb_gap_get(void);

/// @} CB_GAP

#endif // UTILS_CB_GAP_H_
