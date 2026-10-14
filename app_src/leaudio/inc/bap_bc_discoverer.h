/**
 ****************************************************************************************
 *
 * @file bap_bc_discov.h
 *
 * @brief Discover Broadcast Profile - Broadcast Scan - Definitions
 *
 * Copyright (C) RivieraWaves 2009-2024
 * Release Identifier: c73db0ce
 *
 ****************************************************************************************
 */

#ifndef BAP_BC_DISCOV_H_
#define BAP_BC_DISCOV_H_

/**
 ****************************************************************************************
 * @defgroup BAP_BC_DISCOV Broadcast Scan module
 * @ingroup BAP_BC
 * @brief Description of Broadcast Scan module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup BAP_BC_DISCOV_ENUM Enumerations
 * @ingroup BAP_BC_DISCOV
 * @brief Enumerations for Broadcast Scan module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup BAP_BC_DISCOV_STRUCT Structures
 * @ingroup BAP_BC_DISCOV
 * @brief Structures for Broadcast Scan module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup BAP_BC_DISCOV_NATIVE_API Native API
 * @ingroup BAP_BC_DISCOV
 * @brief Description of Native API for Broadcast Scan module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup BAP_BC_DISCOV_CALLBACK Callback Functions
 * @ingroup BAP_BC_DISCOV_NATIVE_API
 * @brief Description of callback functions for Broadcast Scan module
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @defgroup BAP_BC_DISCOV_FUNCTION Functions
 * @ingroup BAP_BC_DISCOV_NATIVE_API
 * @brief Description of functions for Broadcast Scan module
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "gaf.h"
#include "bap.h"
#include "bap_bc.h"
#include "gapm_le.h"
#include "gapm_le_per_sync.h"
#include "gapi.h"

#if 1//(GAF_BAP_BC_SCAN)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// @addtogroup BAP_BC_DISCOV_ENUM
/// @{

/// List of command type values for Broadcast Scan module
enum bap_bc_discov_cmd_type
{
    /// Start seeking for Broadcast Audio Announcements sent by Broadcast Source devices
    BAP_BC_DISCOV_CMD_TYPE_START = 0,
    /// Stop all scan procedures currently in progress
    BAP_BC_DISCOV_CMD_TYPE_STOP,
    /// Synchronize with a Periodic Advertising
    BAP_BC_DISCOV_CMD_TYPE_PA_SYNCHRONIZE,
    /// Terminate synchronization or cancel synchronization establishment with Periodic Advertising
    BAP_BC_DISCOV_CMD_TYPE_PA_TERMINATE,

    /// INTERNAL - Synchronize with a Periodic Advertising using connection
    BAP_BC_DISCOV_CMD_TYPE_PA_SYNCHRONIZE_CON,
    /// INTERNAL - Transfer information about synchronization with a Periodic Advertising
    BAP_BC_DISCOV_CMD_TYPE_PA_TRANSFER,
};

/// Bit Field of Periodic Advertising information that is supposed to be report by scan module
/// Filtering level to report information about basic audio announcement present in Periodic Advertising report
enum bap_bc_discov_report_filter_bf
{
    /// Report Periodic Advertising data (in raw format)
    BAP_BC_DISCOV_REPORT_PER_ADV_DATA_POS = 0,
    BAP_BC_DISCOV_REPORT_PER_ADV_DATA_BIT = CO_BIT(BAP_BC_DISCOV_REPORT_PER_ADV_DATA_POS),
    /// Report BIG Info present in ACAD data of a Periodic Advertising report
    BAP_BC_DISCOV_REPORT_BIG_INFO_POS = 1,
    BAP_BC_DISCOV_REPORT_BIG_INFO_BIT = CO_BIT(BAP_BC_DISCOV_REPORT_BIG_INFO_POS),
    /// Report Broadcast Group related information of Basic Audio Announcements
    BAP_BC_DISCOV_REPORT_ANNOUNCE_LVL_1_POS = 2,
    BAP_BC_DISCOV_REPORT_ANNOUNCE_LVL_1_BIT = CO_BIT(BAP_BC_DISCOV_REPORT_ANNOUNCE_LVL_1_POS),
    /// Report Broadcast Subgroup related information of Basic Audio Announcements
    BAP_BC_DISCOV_REPORT_ANNOUNCE_LVL_2_POS = 3,
    BAP_BC_DISCOV_REPORT_ANNOUNCE_LVL_2_BIT = CO_BIT(BAP_BC_DISCOV_REPORT_ANNOUNCE_LVL_2_POS),
    /// Report Broadcast Stream related information of Basic Audio Announcements
    BAP_BC_DISCOV_REPORT_ANNOUNCE_LVL_3_POS = 4,
    BAP_BC_DISCOV_REPORT_ANNOUNCE_LVL_3_BIT = CO_BIT(BAP_BC_DISCOV_REPORT_ANNOUNCE_LVL_3_POS),

    /// Mask for Report Filtering bit field
    BAP_BC_DISCOV_REPORT_MASK = 0x1F,
};

/// Termination reason values for synchronization with Periodic Advertising
enum bap_bc_discov_pa_term_reason
{
    /// Stopped due to upper layer request
    BAP_BC_DISCOV_PA_TERM_REASON_UPPER_TERMINATE = 0,
    /// Stopped due to a synchronization timeout
    BAP_BC_DISCOV_PA_TERM_REASON_TIMEOUT,
    /// Stopped due to synchronization establishment timeout
    BAP_BC_DISCOV_PA_TERM_REASON_ESTAB_TIMEOUT,
};

/// Information bit field meaning for received Advertising Report
enum bap_bc_discov_info_bf
{
    /// Indicates that Broadcast Source device claims support of Public Broadcast Source role - Position
    BAP_BC_DISCOV_PUBLIC_BROADCAST_SUPPORT_POS = 0,
    /// Indicates that Broadcast Source device claims support of Public Broadcast Source role - Bit
    BAP_BC_DISCOV_PUBLIC_BROADCAST_SUPPORT_BIT = CO_BIT(BAP_BC_DISCOV_PUBLIC_BROADCAST_SUPPORT_POS),
};

/// @} BAP_BC_DISCOV_ENUM

/*
 * TYPES DEFINITION
 ****************************************************************************************
 */
/// Minimal length of Broadcast DISCOV 
#define BAP_BC_BCAST_DISCOV_MIN_LEN    (17)

/// @addtogroup BAP_BC_DISCOV_STRUCT
/// @{

/// Scan Parameters structure
typedef struct bap_bc_discov_param
{
    /// Scan interval for LE 1M PHY in multiple of 0.625ms\n
    /// 0 means LE 1M PHY is not supported, else must be higher than 2.5ms\n
    /// If intv_1m_slot = 0, intv_coded_slot shall not be 0
    uint16_t intv_1m_slot;
    /// Scan interval for LE Coded PHY in multiple of 0.625ms\n
    /// 0 means LE Coded PHY is not supported, else must be higher than 2.5ms\n
    /// If intv_coded_slot = 0, intv_1m_slot shall not be 0
    uint16_t intv_coded_slot;
    /// Scan window for LE 1M PHY in multiple of 0.625ms\n
    /// Meaningless if intv_1m_slot = 0, else must be higher than 2.5ms
    uint16_t wd_1m_slot;
    /// Scan window for LE Coded PHY in multiple of 0.625ms\n
    /// Meaningless if intv_coded_slot = 0, else must be higher than 2.5ms
    uint16_t wd_coded_slot;
} bap_bc_discov_param_t;

/// @} BAP_BC_DISCOV_STRUCT

/*
 * CALLBACK FUNCTIONS DEFINITION
 ****************************************************************************************
 */

/// @addtogroup BAP_BC_DISCOV_CALLBACK
/// @{

/**
 ****************************************************************************************
 * @brief Callback function called each time a Broadcast Scan command has been completed
 *
 * @param[in] cmd_type      Command type (see #bap_bc_discov_cmd_type enumeration)
 * @param[in] status        Status
 * @param[in] pa_lid        Periodic Advertising local index
 ****************************************************************************************
 */
typedef void (*bap_bc_discov_cb_cmp_evt)(uint8_t cmd_type, uint16_t status, uint8_t pa_lid);

/**
 ****************************************************************************************
 * @brief Callback function called when scan has timeout
 ****************************************************************************************
 */
typedef void (*bap_bc_discov_cb_timeout)(void);

/**
 ****************************************************************************************
 * @brief Callback function called when a Broadcast Source device has been discovered
 *
 * @param[in] p_adv_id          Pointer to Advertising identification structure
 * @param[in] p_bcast_id        Pointer to Broadcast ID value
 * @param[in] info_bf           Information bit field (see #bap_bc_discov_info_bf enumeration)\n
 *                              If support of PBP is claimed, call of cb_public_bcast_source will follow
 * @param[in] p_air_info        Pointer to air information structure
 * @param[in] length            Length of complete advertising data
 * @param[in] p_data            Pointer to complete advertising data containing the complete Broadcast Audio
 *                              Announcement
 ****************************************************************************************
 */
typedef void (*bap_bc_discov_cb_report)(const bap_adv_id_t* p_adv_id, const gaf_adv_report_air_info_t* p_air_info, uint16_t length,
                                      const uint8_t* p_data);


/**
 ****************************************************************************************
 * @brief Callback function called when synchronization with Periodic Advertising has been established
 *
 * @param[in] pa_lid            Periodic Advertising local index
 * @param[in] p_adv_id          Pointer to Advertising identification structure
 * @param[in] phy               PHY on which synchronization has been established (see #gap_le_phy_val enumeration)
 * @param[in] interval_frames   Periodic advertising interval (in unit of 1.25ms, min is 7.5ms)
 ****************************************************************************************
 */
typedef void (*bap_bc_discov_cb_pa_established)(uint8_t pa_lid, const bap_adv_id_t* p_adv_id, uint8_t phy,
                                              uint16_t interval_frames);

/**
 ****************************************************************************************
 * @brief Callback function called as result of @see bap_bc_discov_pa_terminate function or if synchronization with
 *        the Periodic Advertising is canceled or lost
 *
 * @param[in] pa_lid        Periodic Advertising local index
 * @param[in] reason        Stop reason (see #gaf_err enumeration)
 ****************************************************************************************
 */
typedef void (*bap_bc_discov_cb_pa_terminated)(uint8_t pa_lid, uint8_t reason);

/**
 ****************************************************************************************
 * @brief Callback function called when a Periodic Advertising report is received
 *
 * @param[in] pa_lid        Periodic Advertising local index
 * @param[in] p_air_info    Pointer to air information structure
 * @param[in] length        Length of Periodic Advertising data
 * @param[in] p_data        Pointer to Periodic Advertising data
 ****************************************************************************************
 */
typedef void (*bap_bc_discov_cb_pa_report)(uint8_t pa_lid, const gaf_adv_report_air_info_t* p_air_info,
                                         uint16_t length, const uint8_t* p_data, bool sink_enable);

/**
 ****************************************************************************************
 * @brief Callback function called when a BIG Info report is received
 *
 * @param[in] pa_lid        Periodic Advertising local index
 * @param[in] p_report      Pointer to structure that contains Big Info data
 ****************************************************************************************
 */
typedef void (*bap_bc_discov_cb_big_info_report)(uint8_t pa_lid, const gapm_le_big_info_t* p_report);

/**
 ****************************************************************************************
 * @brief Callback executed when Broadcast Assistant device requests to establish synchronization with a Periodic
 *        Advertising
 *        @see bap_bc_discov_pa_synchronize_cfm must be called to accept or reject request
 *
 * @param[in] pa_lid        Periodic Advertising local index
 * @param[in] src_lid       Source local index
 * @param[in] con_lid       Connection local index
 ****************************************************************************************
 */
typedef void (*bap_bc_discov_cb_pa_sync_req)(uint8_t pa_lid, uint8_t src_lid, uint8_t con_lid);

/**
 ****************************************************************************************
 * @brief Callback executed when Broadcast Assistant device requests to terminate synchronization with a Periodic
 *        Advertising
 *        @see bap_bc_discov_pa_terminate_cfm must be called to accept or reject request
 *
 * @param[in] pa_lid        Periodic Advertising local index
 * @param[in] con_lid       Connection local index
 ****************************************************************************************
 */
typedef void (*bap_bc_discov_cb_pa_terminate_req)(uint8_t pa_lid, uint8_t con_lid);

/*
 * CALLBACK SET DEFINITION
 ****************************************************************************************
 */

/// Set of callback functions for Discover Broadcast Profile - Broadcast Scan role
struct bap_bc_discov_cb
{
    /// Callback function called when a command has been completed
    bap_bc_discov_cb_cmp_evt cb_cmp_evt;
    /// Callback function called when a scan has timeout
    bap_bc_discov_cb_timeout cb_timeout;
    /// Callback function called when a Broadcast Source device has been discovered
    bap_bc_discov_cb_report cb_report;
    /// Callback function called when synchronization with a Periodic Advertising has been established
    bap_bc_discov_cb_pa_established cb_pa_established;
    /// Callback function called when synchronization with a Periodic Advertising has been terminated
    bap_bc_discov_cb_pa_terminated cb_pa_terminated;
    /// Callback function called when a Periodic Advertising report has been received
    bap_bc_discov_cb_pa_report cb_pa_report;
    /// Callback function called when a BIG Info report has been received
    bap_bc_discov_cb_big_info_report cb_big_info_report;
};

/// @} BAP_BC_DISCOV_CALLBACK

/*
 * API FUNCTIONS DECLARATION
 ****************************************************************************************
 */

/// @addtogroup BAP_BC_DISCOV_FUNCTION
/// @{

/**
 ****************************************************************************************
 * @brief Configure use of BAP Broadcast Scan module
 *
 * @param[in] p_cb          Pointer to set to callback functions for communication with upper layer
 *
 * @return An error status (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t bap_bc_discov_configure( const bap_bc_discov_cb_t* p_cb);

/**
 ****************************************************************************************
 * @brief Set scan parameters to be used for all scan procedures.
 *        By default, following scan parameters are used:
 *          - LE 1M PHY only
 *          - Scan Windows = 5ms
 *          - Scan Interval = 10ms
 *
 * @param[in] p_scan_param      Pointer to new scan parameters
 *
 * @return An error status (see #gaf_err enumeration) (see #gaf_err enumeration)
 ****************************************************************************************
 */
uint16_t bap_bc_discov_set_params(const bap_bc_discov_param_t* p_scan_param);

/**
 ****************************************************************************************
 * @brief Start seeking for Broadcast Audio Announcements sent by Broadcast Source devices.
 *
 *        When Broadcast Assistant role is supported and configured, all connected Broadcast
 *        Delegators are informed that scan on their behalf has been started.
 *
 * @param[in] timeout_s    Timeout duration in seconds
 *
 * @return An error status (see #gaf_err enumeration) (see #gaf_err enumeration).
 *         If no error, no new function should be executed until @see bap_bc_discov_cb_cmp_evt
 *         with BAP_BC_DISCOV_START command code is called.
 ****************************************************************************************
 */
uint16_t bap_bc_discov_start(uint16_t timeout_s);

/**
 ****************************************************************************************
 * @brief Stop all scan procedures currently in progress.
 *
 *        When Broadcast Assistant role is supported and configured, all connected
 *        Broadcast Delegators are informed that scan on their behalf has been stopped.
 *
 * @return An error status (see #gaf_err enumeration) (see #gaf_err enumeration).
 *         If no error, no new function should be executed until @see bap_bc_discov_cb_cmp_evt
 *         with BAP_BC_DISCOV_STOP command code is called.
 ****************************************************************************************
 */
uint16_t bap_bc_discov_stop(void);

/**
 ****************************************************************************************
 * @brief Establish synchronization with a Periodic Advertising
 *        Once the synchronization has been established, @see cb_pa_established callback is executed.
 *
 * @param[in] p_adv_id           Pointer to Advertising identification structure
 * @param[in] skip               Number of Periodic Advertising that can be skipped after a successful reception
 *                               Maximum authorized value is 499
 * @param[in] report_filter_bf   Report filtering bit field (see #bap_bc_discov_report_filter_bf enumeration)
 * @param[in] sync_to_10ms       Synchronization timeout for the Periodic Advertising
 *                               (in unit of 10ms between 100ms and 163.84s)
 * @param[in] timeout_s          Scan Timeout in seconds
 * @param[out] p_pa_lid          Pointer at which allocated Periodic Advertising local index will be returned
 *
 *
 * @return An error status (see #gaf_err enumeration) (see #gaf_err enumeration)
 *         If no error, no new function should be executed until @see bap_bc_discov_cb_cmp_evt
 *         with BAP_BC_DISCOV_PA_SYNCHRONIZE command code is called.
 ****************************************************************************************
 */
uint16_t bap_bc_discov_pa_synchronize(const bap_adv_id_t* p_adv_id, uint16_t skip, uint8_t report_filter_bf,
                                    uint16_t sync_to_10ms, uint16_t timeout_s, uint8_t* p_pa_lid);

/**
 ****************************************************************************************
 * @brief Terminate synchronization with a Periodic Advertising
 *        Once the synchronization with the Periodic Advertising is terminated, @see cb_pa_terminated callback is
 *        executed.
 *
 * @param[in] pa_lid          Periodic Advertising local index
 *
 * @return An error status (see #gaf_err enumeration) (see #gaf_err enumeration)
 *         If no error, no new function should be executed until @see bap_bc_discov_cb_cmp_evt
 *         with BAP_BC_DISCOV_PA_SYNCHRONIZE command code is called.
 ****************************************************************************************
 */
uint16_t bap_bc_discov_pa_terminate(uint8_t pa_lid);

#endif //(GAF_BAP_BC_DISCOV)

#endif // BAP_BC_DISCOV_H_
