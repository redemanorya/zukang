/**
 ****************************************************************************************
 *
 * @file data_path.h
 *
 * @brief Main API file for the Link Layer Data path manager
 *
 * Copyright (C) RivieraWaves 2009-2024
 * Release Identifier: 2d038f9b
 *
 ****************************************************************************************
 */

#ifndef DATA_PATH_H_
#define DATA_PATH_H_



/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "co_bt_defines.h"  // BT Definitions
#include "co_list.h"        // List manipulation
#include <stdbool.h>        // boolean definition
#include <stdint.h>         // integer definition

/*
 * MACROS
 ****************************************************************************************
 */

/// @addtogroup DATA_PATH_BUILDER_API
/// @{

/// Macro used to initialize data path builder
/// @param[in] dp_id Identifier of the data path that builder can create (see #data_path_id)
/// @param[in] dp_direction Direction of the data path that builder can create (see enum #data_path_direction)
/// @param[in] p_dp_itf Pointer to the data path builder interface (see #data_path_builder_itf_t
#define DATA_PATH_BUILDER_INIT(dp_id, dp_direction, p_dp_itf)\
    {                                         \
        .hdr.next            = NULL,          \
        .p_itf               = (p_dp_itf),      \
        .data_path_id        = (dp_id),         \
        .data_path_direction = (dp_direction),  \
    }


/// SDU status
enum sdu_status
{
    /// Correct SDU reception
    SDU_OK                 = 0x0,
    /// SDU partially received
    SDU_PARTIALLY_RECEIVED = 0x1,
    /// SDU size exceeded
    /// Some ISO packet in SDU reception has been lost
    SDU_PKT_LOST           = 0x2,
    /// A CRC error detected during reception
    SDU_CRC_ERR            = 0x3,
    /// SDU size exceed (due to a length error into an ISO packet)
    SDU_SIZE_EXCEED        = 0x4,
    /// No sync detected during reception of the packet
    SDU_SYNC_ERR           = 0x5,
    /// Wrong fragment received or unexpected LLID
    SDU_UNEXPECTED_FORMAT  = 0x6,
    /// SDU Not granted (never used for transmission or reception)
    SDU_NOT_GRANTED        = 0x7,
//    SDU_SIZE_EXCEED        = 0x2,
    /// Wrong fragment received or unexpected LLID
//    SDU_UNEXPECTED_FORMAT  = 0x3,
    /// No data received for this SDU
    SDU_NO_DATA            = 0x8,
    /// SDU reception ongoing (internal status during the reception of the SDU)
    SDU_RX_ONGOING         = 0x9,
};
/// @}




/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/*
 *********************
 * DATA PATH DRIVER
 *********************
 */

typedef struct data_path data_path_t;

/**
 ****************************************************************************************
 * Start the data path driver
 *
 * @param[in] p_dp           Pointer to data path driver instance
 * @param[in] sdu_interval   SDU interval (in us)
 * @param[in] trans_latency  Transport latency (in us)
 * @param[in] max_sdu        Maximum SDU size (in bytes) (12 bits meaningful)
 *
 * @return Status of the operation (see enum #co_error)
 ****************************************************************************************
 */
typedef uint8_t (*data_path_start_cb) (data_path_t* p_dp, uint32_t sdu_interval, uint32_t trans_latency, uint16_t max_sdu);

/**
 ****************************************************************************************
 * Get SDU buffer for transmission or reception on an Isochronous link
 *
 * @param[in]     p_dp         Pointer to data path driver instance
 * @param[in,out] p_ref_time   ISOAL provides the reference anchor point for the new SDU, data path returns
 *                             the SDU reference time (only for input direction) (in us, based on Bluetooth timestamp)
 * @param[out]    p_sdu_len    Size of SDU (in bytes) (input: size of the data to transmit, output: size of the buffer)
 *
 * @return Pointer of the SDU buffer
 ****************************************************************************************
 */
typedef uint8_t* (*data_path_sdu_get_cb) (data_path_t* p_dp, uint32_t* p_ref_time, uint16_t* p_sdu_len);

/**
 ****************************************************************************************
 * Indicate SDU transmission or reception is completed
 *
 * @param[in] p_dp     Pointer to data path driver instance
 * @param[in] sdu_len  Size of SDU received (only for output direction)
 * @param[in] ref_time SDU synchronization reference (only for output direction) (in us, based on Bluetooth timestamp)
 * @param[in] p_buf    Pointer of the SDU buffer
 * @param[in] status   Reception or transmission buffer status (see enum #sdu_status)
 ****************************************************************************************
 */
typedef void (*data_path_sdu_done_cb) (data_path_t* p_dp, uint16_t sdu_len, uint32_t ref_time, uint8_t* p_buf, uint8_t status);

/**
 ****************************************************************************************
 * Call when data path driver is no more needed.
 * Once destroyed, the data path driver instance in not used anymore.
 *
 * @param[in] p_dp    Pointer to data path driver instance
 * @param[in] reason  Reason for data path destruction
 ****************************************************************************************
 */
typedef void (*data_path_destroy_cb) (data_path_t* p_dp, uint8_t reason);

/**
 ****************************************************************************************
 * Indicate the local time of an ISO event, when receiving a packet from the CIS-master or BIS-transmitter
 *
 * @note Only for CIS-slave or BIS-receiver
 *
 * @param[in]     group_id          Driver identifier of the BIS or CIS group - can be used to retrieve context information
 * @param[in]     transport_id      Transport identifier (see #transport_id) - can be used to retrieve context information
 * @param[in]     evt_cnt           CIG/BIG event counter
 * @param[in]     ref_anchor_pt     Timestamp of the CIG/BIG reference anchor point (in us, based on Bluetooth timestamp)
 ****************************************************************************************
 */
//typedef void (*data_path_peer_sync_cb) (uint8_t group_id, uint8_t transport_id, uint16_t evt_cnt, uint32_t ref_anchor_pt);
typedef void (*data_path_peer_sync_cb) (uint16_t evt_cnt, uint32_t ref_anchor_pt);


/// Data path driver callback interface.
///
/// @note Except data_path_itf_t.cb_peer_sync, function in the interface are mandatory and shall be overwritten by
///       data path driver instance.
typedef struct data_path_itf
{
    /// Start the data path driver
    data_path_start_cb cb_start;

    /// Get SDU buffer for transmission or reception on an Isochronous link
    data_path_sdu_get_cb cb_sdu_get;

    /// Indicate SDU transmission or reception is completed
    data_path_sdu_done_cb cb_sdu_done;

    /// Destroy the data path driver
    data_path_destroy_cb cb_destroy;

    /// Indicate the local time of an ISO event, when receiving a packet from the CIS-master or BIS-transmitter
    /// This callback is optional
    data_path_peer_sync_cb cb_peer_sync;
} data_path_itf_t;

/// Data path driver container used has context information object by client of the data path.
struct data_path
{
    /// Pointer to the interface that contains callbacks
    const data_path_itf_t* p_itf;
};

/// @} DATA_PATH_DRIVER_API

/*
 *********************
 * DATA PATH BUILDER
 *********************
 */

/// @addtogroup DATA_PATH_BUILDER_API
/// @{

/**
 ****************************************************************************************
 * Callback executed by data path module in order to create instance of a new data path driver
 *
 * @param[in] conhdl        Connection handle (Range 0x0000-0x0EFF)
 * @param[in] p_codec_id    Pointer to the 5 bytes codec identifier
 * @param[in] ctrl_delay    Controller delay in ms (Range: 0x000000 to 0x3D0900)
 * @param[in] codec_cfg_len Codec configuration length
 * @param[in] p_codec_cfg   Pointer to array that contains codec configuration
 *
 * @return Pointer to instance of the new data path driver if creation succeed, otherwise returns NULL.
 ****************************************************************************************
 */
typedef data_path_t* (*data_path_create_cb) (uint16_t conhdl, const uint8_t* p_codec_id, uint32_t ctrl_delay,
                                             uint8_t codec_cfg_len, const uint8_t* p_codec_cfg);

/**
 ****************************************************************************************
 * Callback executed by data path module in order to provide configuration for data path type
 *
 * @param[in] cfg_len   Length of the vendor-specific configuration data
 * @param[in] p_cfg     Pointer to Vendor-specific configuration data for the data path being configured
 *
 * @return Status code of execution of the function using #co_error identifier
 ****************************************************************************************
 */
typedef uint8_t (*data_path_config_cb) (uint8_t cfg_len, const uint8_t* p_cfg);

/// Data path builder that shall be followed to create a specific type of data path.
/// All callback in the interface are mandatory
typedef struct data_path_builder_itf
{
    /// Callback executed by data path module in order to create instance of a new data path driver
    data_path_create_cb cb_create_data_path;
    /// Callback executed by data path module to provide vendor specific configuration information
    data_path_config_cb cb_config;
} data_path_builder_itf_t;

/// Structure that must be provided by data path builder to enable creation of specific data path.
typedef struct data_path_builder
{
    /// Use to put data path builder in a list.
    co_list_hdr_t hdr;
    /// Pointer to the interface that contains callbacks
    const data_path_builder_itf_t* p_itf;
    /// Identifier of the data path that builder can create (see #data_path_id)
    uint8_t data_path_id;
    /// Direction of the data path that builder can create (see enum #data_path_direction)
    uint8_t data_path_direction;
} data_path_builder_t;

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Add a new data path builder which allow data path module to create new data path driver instances.
 *
 * Each builder shall be uniquely identified by couple of data path identifier (see #data_path_id) and data path
 * direction (see #data_path_direction) present in #data_path_builder_t structure.
 *
 * Builder interface must implement all callbacks present in the #data_path_builder_itf_t structure
 *
 * @note It is recommended to populate the data path builder list at boot.
 *
 * @param[in] p_builder  Pointer to the data path builder
 ****************************************************************************************
 */
void data_path_add_builder(data_path_builder_t* p_builder);

/// @} DATA_PATH_BUILDER_API

/**
 ****************************************************************************************
 * @brief Initialization of the BLE Data Path driver
 *
 * @note This function should not be used by application
 *
 * @param[in] is_reset  True if reset is requested, false for an initialization
 ****************************************************************************************
 */
void data_path_init(void);

/**
 ****************************************************************************************
 * @brief Provide configuration data to a specific data path
 *
 * @note This function should not be used by application
 *
 * @param[in] id            Identifier of the data path that support codec (see #iso_data_path_id)
 * @param[in] direction     Direction of the data path that support codec (see enum #data_path_direction)
 * @param[in] conhdl        Connection handle (Range 0x0000-0x0EFF)
 * @param[in] p_codec_id    Pointer to the 5 bytes codec identifier
 * @param[in] ctrl_delay    Controller delay in ms (Range: 0x000000 to 0x3D0900)
 * @param[in] codec_cfg_len Codec configuration length
 * @param[in] p_codec_cfg   Pointer to array that contains codec configuration
 *
 * @return Pointer to instance of the new data path driver if creation succeed, otherwise returns NULL.
 ****************************************************************************************
 */
data_path_t* data_path_create(uint8_t id, uint8_t direction,  uint16_t conhdl, const uint8_t* p_codec_id,
                              uint32_t ctrl_delay, uint8_t codec_cfg_len, const uint8_t* p_codec_cfg);

/**
 ****************************************************************************************
 * @brief Inline function used to destroy an instance of data path driver
 *
 * @note This function should not be used by application
 *
 * @param[in] p_dp          Pointer to data path driver instance
 * @param[in] reason        destruction reason (see #co_error)
 ****************************************************************************************
 */
__INLINE void data_path_destroy(data_path_t* p_dp, uint8_t reason)
{
    p_dp->p_itf->cb_destroy(p_dp, reason);
}

/**
 ****************************************************************************************
 * @brief Provide configuration data to a specific data path
 *
 * @note This function should not be used by application
 *
 * @param[in] id        Identifier of the data path that support codec (see #iso_data_path_id)
 * @param[in] direction Direction of the data path that support codec (see enum #data_path_direction)
 * @param[in] cfg_len   Length of the vendor-specific configuration data
 * @param[in] p_cfg     Pointer to Vendor-specific configuration data for the data path being configured
 *
 * @return status 0 for success, or reason code otherwise
 ****************************************************************************************
 */
uint8_t data_path_config(uint8_t id, uint8_t direction, uint8_t cfg_len, const uint8_t* p_cfg);

/**
 ****************************************************************************************
 * Get the default data path that can be used as disabled data path driver by a stream
 *
 * @note This function should not be used by application
 *
 * @return Default disabled data path driver
 ****************************************************************************************
 */
data_path_t* data_path_get_default(void);

/**
 ****************************************************************************************
 * Get if given data path driver is default / disabled data path.
 *
 * @note This function should not be used by application
 *
 * @return True if default data path driver, false otherwise
 ****************************************************************************************
 */
bool data_path_is_default(const data_path_t* p_dp);

// Default Data Path Builder functions when not supported by builder

/// @addtogroup DATA_PATH_BUILDER_API
/// @{

/**
 ****************************************************************************************
 * Default #data_path_config_cb if not supported
 *
 * @param[in] cfg_len   Length of the vendor-specific configuration data
 * @param[in] p_cfg     Pointer to Vendor-specific configuration data for the data path being configured
 *
 * @return Status code of execution of the function using #co_error identifier
 ****************************************************************************************
 */

uint8_t data_path_default_config(uint8_t cfg_len, const uint8_t* p_cfg);
/// @} DATA_PATH_BUILDER_API

#if (BLE_EMB_PRESENT)

/// @addtogroup DATA_PATH_CLK_SAMPLE_API
/// @{

/**
 ****************************************************************************************
 * @brief Perform a clock sampling which raise an HW signal for different clock domain synchronization
 *
 * @return Bluetooth sampled clock value (32-bit value in microseconds).
 ****************************************************************************************
 */
uint32_t data_path_do_clk_sample(void);
/// @} DATA_PATH_CLK_SAMPLE_API
#endif // (BLE_EMB_PRESENT)


#if 1 //new add
typedef void (*data_path_local_sync_cb) (uint32_t t_gpio);

#endif

#endif // DATA_PATH_H_
