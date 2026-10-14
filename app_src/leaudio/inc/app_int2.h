/**
 ****************************************************************************************
 *
 * @file app_int2.h
 *
 * @brief Application Internals
 *
 * Copyright (C) RivieraWaves 2009-2022
 *
 ****************************************************************************************
 */


#ifndef APP_INT2_H_
#define APP_INT2_H_


/*
 * INCLUDE FILES
 ****************************************************************************************
 */
//#include "app.h"
#include "arch.h"
#include "gapm.h"
//#include "gapm_bt.h"

/*
 * DEFINES
 ****************************************************************************************
 */

// Application transition
enum app_transition
{
    // Application started  - idx meaningless
    APP_STARTED = 0,
    // Device is configured, air activities can be started - idx meaningless
    APP_DEVICE_CONFIGURED,

    // Connection is established
    APP_BT_CONNECTION_ESTABLISHED,
    // LE Connection is established
    APP_LE_CONNECTION_ESTABLISHED,
    // Pairing is successful
    APP_PAIRING_SUCCEED,
    // Connection has been established with a paired peer device
    APP_LE_CONNECTION_ESTABLISHED_PAIRED,
    // Encryption has been started
    APP_ENCRYPTION_STARTED,
    // Link disconnected
    APP_DISCONNECTED,

    // Inform that inquiry activity is stopped
    APP_INQUIRY_STOPPED,
    // Inform that inquiry scan activity is stopped
    APP_INQUIRY_SCAN_STOPPED,
    // Inform that inquiry scan activity is started
    APP_INQUIRY_SCAN_STARTED,
    // Inform that page activity is stopped
    APP_PAGE_STOPPED,
    // Inform that page scan activity is stopped
    APP_PAGE_SCAN_STOPPED,

    // LE Connectable Advertising Started
    APP_LE_CONNECTABLE_ADV_STARTED,
    // LE Connectable Advertising Stopped
    APP_LE_CONNECTABLE_ADV_STOPPED,

    // First transition value that can be used by a scenario
    APP_FIRST_TRANSITION,

    // Invalid transition
    APP_TRANSITION_INVALID = 0xFF,
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/*
 * Callback definition used by an application scenario to handle bluetooth event transition
 *
 * @param[in] event     Event transition (see #app_transition)
 * @param[in] status    Status of event transition
 * @param[in] p_param   Transition update context parameter
 */
typedef void (*app_state_transition_cb)(uint8_t event, uint16_t status, const void* p_param);

/*
 * MACROS
 ****************************************************************************************
 */

#if (RW_DEBUG)
#define APP_CHECK_STATUS(function_call)                      \
     {                                                       \
        uint16_t status = function_call;                     \
        ASSERT_INFO(status == GAP_ERR_NO_ERROR, status, 0);  \
     }
#else //(RW_DEBUG)
#define APP_CHECK_STATUS(function_call) function_call;
#endif //(RW_DEBUG)

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************************
 */

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/*
 * Function used to create a transition into the application
 *
 * @param[in] event     Event transition (see #app_transition)
 * @param[in] status    Status of event transition (see #hl_err)
 * @param[in] p_param   Transition update context parameter
 */
void app_state_transition(uint8_t event, uint16_t status, const void* p_param);

/*
 * Set device configuration - with default parameters
 *
 * @return Function execution status (see #hl_err)
 */
uint16_t app_set_dev_config(void);

/*
 * Set device configuration
 *
 * @param[in] p_param Pointer
 * @return Function execution status (see #hl_err)
 */
uint16_t app_set_dev_config_with_params(const gapm_config_t* p_params);

/*
 * Set appearance
 *
 * @param[in] appearance    Appearance
 */
void app_set_appearance(uint16_t appearance);

/*
 * Set device name
 *
 * @param[in] p_name Pointer to the new device name;
 */
void app_set_dev_name(const char* p_name);


/*
 * Update device name
 *
 * @param[in] p_name Pointer to the new device name;
 *
 * @return Function execution status (see #hl_err)
 */
uint16_t app_update_dev_name(const char* p_name);

/*
 * Set device role
 *
 * @param[in] role  Device role (see enum #gap_role)
 */
void app_set_dev_role(uint8_t role);

// Terminate connection
//uint16_t app_disconnect(uint8_t conidx); //george

/*
 * Get if device name present in a report (ADV or EIR)
 *
 * @param[in] p_data      Pointer to buffer that contains data in LTV format
 * @param[in] p_exp_name  Pointer to expected device name (UTF-8 string terminated with NULL character)
 *
 * @return True if expected name found, False otherwise
 */
bool app_is_name_match_in_report(co_buf_t* p_data, const char* p_exp_name);

#if (BLE_HOST_PRESENT)
/**
 ****************************************************************************************
 * @brief Initiate pairing for a LE connection
 *
 * @param[in] conidx        Connection index
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t app_le_start_pairing(uint8_t conidx);

/**
 ****************************************************************************************
 * @brief Required start of encryption
 *
 * @param[in] conidx        Connection index
 *
 * @return An error status
 ****************************************************************************************
 */
uint16_t app_le_encrypt(uint8_t conidx);

/**
 ****************************************************************************************
 * @param[in] conidx        Connection index
 *
 * @return True if pairing has been performed else false
 ****************************************************************************************
 */
bool app_is_paired(uint8_t conidx);
#endif //(BLE_HOST_PRESENT)

/**
 ****************************************************************************************
 * Set LE Connection Configuration callback interface
 *
 * @param[in] p_le_config_cb_itf  LE Connection Configuration callback interface
 ****************************************************************************************
 */
void app_set_con_le_config_cb_itf(const gapc_le_config_cb_t* p_le_config_cb_itf);

/**
 ****************************************************************************************
 * Set Connection Security callback interface
 *
 * @param[in] p_sec_cb_itf  Connection Security callback interface
 ****************************************************************************************
 */
void app_set_con_security_cb_itf(const gapc_security_cb_t* p_sec_cb_itf);

/**
 ****************************************************************************************
 * Set Connection Information callback interface
 *
 * @param[in] p_info_cb_itf  Connection Information callback interface
 ****************************************************************************************
 */
void app_set_con_info_itf(const gapc_connection_info_cb_t* p_info_cb_itf);

#endif /* APP_INT_H_ */

/// \example{lineno} app_int.h

