/**
 ****************************************************************************************
 *
 * @file app_le_coordinated_set_coordinator.h
 *
 * @brief Applet - Coordinated Set Coordinator - Header file
 *
 ****************************************************************************************
 */
#include "le_audio_config.h"
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
#ifndef APP_LE_COORDINATED_SET_COORDINATOR_H_
#define APP_LE_COORDINATED_SET_COORDINATOR_H_

/**
 ****************************************************************************************
 * @defgroup APP_LE_COORDINATED_SET_COORDINATOR Coordinated Set Coordinator
 * @ingroup APP_LE
 * @brief Applet adding support of Coordinated Set Identification Service as Set Coordinator
 *
 * In order to support reception of several RSIs while scanning, a FIFO queue is implemented. Maximum number of RSIs
 * that can be put into this queue shall be provided during Mediator configuration
 * (#app_le_coordinated_set_coordinator_cfg_t).
 * Resolution of an RSI can be required using #APP_LE_COORDINATED_SET_COORDINATOR_CONTROL_RESOLVE control type.
 * RSIs are resolved one by one, #APP_LE_COORDINATED_SET_COORDINATOR_EVENT_RESOLVED event is generated as soon as
 * an RSI has been successfully resolved.
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>
#include "atc_csi.h"
#include "arch.h"
#include "csis.h"

/// @addtogroup APP_LE_COORDINATED_SET_COORDINATOR
/// @{

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

/// Control type values
enum app_le_coordinated_set_coordinator_control_type
{
	APP_LE_COORDINATED_SET_COORDINATOR_CONTROL_STOP,
    /// Request RSI resolution\n
    /// See #app_le_coordinated_set_coordinator_resolve_t
    APP_LE_COORDINATED_SET_COORDINATOR_CONTROL_RESOLVE,
    /// Request to add an SIRK
    APP_LE_COORDINATED_SET_COORDINATOR_CONTROL_ADD_SIRK,
};

/// Event type values
enum app_le_coordinated_set_coordinator_event_type
{
    /// Indicate if RSI has been resolved\n
    /// See #app_le_coordinated_set_coordinator_resolved_t
    APP_LE_COORDINATED_SET_COORDINATOR_EVENT_RESOLVED,
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */

/// Parameters structure for Mediator configuration
typedef struct app_le_coordinated_set_coordinator_cfg
{
    /// Number of RSI that may be stored in resolution queue\n
    /// Shall not be 0
    uint8_t nb_rsi_max;
} app_le_coordinated_set_coordinator_cfg_t;

/// Parameters structure for #COMPONENT_CONTROL_START_DISCOVERY control type
typedef struct component_control_start_discovery
{
    /// Connection index
    uint8_t conidx;
    /// Pointer to service information\n
    /// May be NULL
    const prf_svc_t* p_svc;
} component_control_start_discovery_t;

/// Parameters structure for #COMPONENT_CONTROL_START_DISCOVERY Mediator control type
typedef struct app_le_coordinated_set_coordinator_start_discovery
{
    /// Start discovery basic parameters
    component_control_start_discovery_t basic;
    /// Pointer to expected SIRK\n
    /// May be NULL
    const csis_sirk_t* p_sirk;
} app_le_coordinated_set_coordinator_start_discovery_t;

/// Parameters structure for #APP_LE_COORDINATED_SET_COORDINATOR_CONTROL_RESOLVE Mediator control type
typedef struct app_le_coordinated_set_coordinator_resolve
{
    /// Pointer to RSI to resolve
    const atc_csis_rsi_t* p_rsi;
    /// Pointer to device address
    const gap_bdaddr_t* p_addr;
} app_le_coordinated_set_coordinator_resolve_t;

/// Parameters structure for #APP_LE_COORDINATED_SET_COORDINATOR_CONTROL_ADD_SIRK Mediator control type
typedef struct app_le_coordinated_set_coordinator_add_sirk
{
    /// Pointer to expected SIRK\n
    /// Cannot be NULL
    const csis_sirk_t* p_sirk;
} app_le_coordinated_set_coordinator_add_sirk_t;

/// Parameters structure for #APP_LE_COORDINATED_SET_COORDINATOR_EVENT_RESOLVED Mediator event type
typedef struct app_le_coordinated_set_coordinator_resolved
{
    /// Pointer to device address
    const gap_bdaddr_t* p_addr;
} app_le_coordinated_set_coordinator_resolved_t;

/*
 * INTERFACES
 ****************************************************************************************
 */
void app_le_coordinated_set_coordinator_state_service_discovery(uint8_t event, uint16_t status,
                                                                         const void* p_params);
                                                                         
void app_le_coordinated_set_coor_rsi_reslove(const void* p_params);
void app_le_coordinated_set_coor_stop(void);
void app_le_coordinated_set_coor_add_sirk(const void* p_params);

void app_le_coordinated_set_coordinator_configure(const app_le_coordinated_set_coordinator_cfg_t* p_params,
                                                           component_cb_event cb_event);                                                                         
/// @} APP_LE_COORDINATED_SET_COORDINATOR

#endif // APP_LE_COORDINATED_SET_COORDINATOR_H_
#endif