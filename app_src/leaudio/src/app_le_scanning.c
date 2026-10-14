/**
 ****************************************************************************************
 *
 * @file app_le_gaf_scanning.c
 *
 * @brief Applet - GAF Scanning
 *
 * Copyright (C) RivieraWaves 2009-2024
 * Release Identifier: c73db0ce
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#if 0//BT_LE_AUDIO_SUPPORT
#include "app_le_scanning.h"
#include "gaf_scan.h"
#include "gaf.h"
#include "debug.h"
#include "gap_le.h"
#include "gapm_le_scan.h"
#include "le_audio_config.h"

#include "bt_config.h"


void app_le_scan_stop(void);
/*
 * ENUMERATIONS
 ****************************************************************************************
 */
extern void app_le_start_connect(app_le_gaf_scanning_device_found_t* p_params);
// extern uint8_t target_bcast_name[30];
char target_bcast_name[] = "MV_cis_sink";

// Event values for Scan state machine
enum app_le_gaf_scanning_event
{
    // Start has been requested
    EVENT_SCAN_START = 0U,
    // Peer device has been found
    EVENT_SCAN_FOUND,
    // Stop has been requested
    EVENT_SCAN_STOP,
    // Scanning has been stopped
    EVENT_SCAN_STOPPED,
};

// Status bit field meaning
enum app_le_gaf_scanning_status_bf
{
    // Indicate if Scan is started - Position
    STATUS_STARTED_POS = 1U,
    // Indicate if Scan is started - Bit
    STATUS_STARTED_BIT = CO_BIT(STATUS_STARTED_POS),

    // Indicate if filtering by device name is used - Position
    STATUS_FILTER_DEVICE_NAME_POS = 2U,
    // Indicate if filtering by device name is used - Bit
    STATUS_FILTER_DEVICE_NAME_BIT = CO_BIT(STATUS_FILTER_DEVICE_NAME_POS),

    // Indicate if filtering by device address is used - Position
    STATUS_FILTER_DEVICE_ADDRESS_POS = 3U,
    // Indicate if filtering by device name is used - Bit
    STATUS_FILTER_DEVICE_ADDRESS_BIT = CO_BIT(STATUS_FILTER_DEVICE_ADDRESS_POS),

    // Indicate if RSI is needed - Position
    STATUS_RSI_NEEDED_POS = 4U,
    // Indicate if RSI is needed- Bit
    STATUS_RSI_NEEDED_BIT = CO_BIT(STATUS_RSI_NEEDED_POS),
};

// Common event type values
enum component_event_type
{
    /// Start
	COMPONENT_CONTROL_START = 0U,
    /// Stop
	COMPONENT_CONTROL_STOP,

	COMPONENT_EVENT_DEVICE_FOUND,

	COMPONENT_EVENT_STOPPED,
    /// First available event type value for child Components
    COMPONENT_EVENT_FIRST,
};

/*
 * TYPES DEFINITION
 ****************************************************************************************
 */

// Environment structure
typedef struct app_le_gaf_scanning_env
{
    // Event callback function for backward communication with Mediator
    component_cb_event cb_mediator_event;
    // Parameters structure for #COMPONENT_EVENT_DEVICE_FOUND event
    app_le_gaf_scanning_device_found_t params_device_found;
    // Status bit field (see #app_le_gaf_scanning_event enumeration)
    uint8_t status_bf;
} app_le_gaf_scanning_env_t;

/*
 * LOCAL VARIABLES
 ****************************************************************************************
 */

// Pointer to environment
__STATIC app_le_gaf_scanning_env_t* p_env;

/*
 * STATE MACHINES
 ****************************************************************************************
 */

__STATIC void app_le_gaf_scanning_transition(uint8_t event)
{
    APP_DBG("app_le_gaf_scanning_transition: evt 0x%x\n", event);
    
    uint16_t status;
    switch (event)
    {
        case EVENT_SCAN_START:
        {
            if (!CHECKB(p_env->status_bf, STATUS_STARTED))
            {
                bool is_scanning = true;

                status = gaf_scan_start(GAF_SCAN_CFG_ASCS_REQ_BIT, 0, GAP_PHY_1MBPS);
                if (status)
                {
                    APP_DBG("gaf_scan_start: err 0x%x\n", status);
                }
                SETB(p_env->status_bf, STATUS_STARTED, 1);
            }
        } break;

        case EVENT_SCAN_FOUND:
        {
            p_env->cb_mediator_event(COMPONENT_EVENT_DEVICE_FOUND, 0, &(p_env->params_device_found));
            // p_env->params_device_found.p_addr = NULL;
        } break;

        case EVENT_SCAN_STOP:
        {
            if (CHECKB(p_env->status_bf, STATUS_STARTED))
            {
                status = gaf_scan_stop();
                if (status)
                {
                    APP_DBG("gaf_scan_stop: err 0x%x\n", status);
                }
                SETB(p_env->status_bf, STATUS_STARTED, 0);
            }
        } break;

        case EVENT_SCAN_STOPPED:
        {
            bool is_scanning = false;

            p_env->cb_mediator_event(COMPONENT_EVENT_STOPPED, 0, NULL);
        } break;

        default: { ASSERT_INFO(0, event, 0); } break;
    }
}

/*
 * CALLBACK FUNCTIONS
 ****************************************************************************************
 */

__STATIC void app_le_gaf_scanning_cb_cmp_evt(uint8_t cmd_type, uint16_t status)
{
    ASSERT_WARN(status == GAF_ERR_NO_ERROR, status, cmd_type);
}

__STATIC void app_le_gaf_scanning_cb_stopped(uint8_t reason)
{
    APP_DBG("app_le_gaf_scanning_cb_stopped: reason 0x%x\n", reason);
    app_le_gaf_scanning_transition(EVENT_SCAN_STOPPED);
}

__STATIC void app_le_gaf_scanning_cb_report(const gap_bdaddr_t* p_addr, uint8_t info_bf,
                                            const gaf_adv_report_air_info_t* p_air_info, uint8_t flags,
                                            uint16_t appearance, uint16_t tmap_roles, const atc_csis_rsi_t* p_rsi,
                                            uint16_t length, const uint8_t* p_data)
{

    do
    {
        const uint8_t* p_reported_name = NULL;

        if (CHECKB(p_env->status_bf, STATUS_RSI_NEEDED))
        {
            if (!CHECKB(info_bf, GAF_SCAN_REPORT_INFO_RSI)) { break; }
        }

        if (!CHECKB(info_bf, GAF_SCAN_REPORT_INFO_ASCS))
        {
            return;
        }

        p_env->params_device_found.name_length = 0;
        p_reported_name = gapm_get_ltv_value(GAP_AD_TYPE_SHORTENED_NAME, length, p_data,
                                             &(p_env->params_device_found.name_length));
        if (p_reported_name == NULL)
        {
            p_reported_name = gapm_get_ltv_value(GAP_AD_TYPE_COMPLETE_NAME, length, p_data,
                                                 &(p_env->params_device_found.name_length));
        }

        if (p_reported_name != NULL)  
        {
            // if (!memcmp(p_reported_name, "MV_BIS_SRC_bai", p_env->params_device_found.name_length))
            if (!memcmp(p_reported_name, target_bcast_name, p_env->params_device_found.name_length))
            {
                APP_DBG("name_len %d\n", p_env->params_device_found.name_length);
            }
            else
            {
                return;    
            }
        }
        else
        {
            APP_DBG("p_reported_name is null\n");
            return;      
        }
        
        // Note - p_addr, p_data and p_rsi are still valid in cb_announcement as both callback functions are called
        // from same function
        p_env->params_device_found.p_device_name = p_reported_name;
        p_env->params_device_found.p_addr = p_addr;
        p_env->params_device_found.p_rsi = CHECKB(info_bf, GAF_SCAN_REPORT_INFO_RSI) ? p_rsi : NULL;

        if (CHECKB(info_bf, GAF_SCAN_REPORT_INFO_ANNOUNCEMENT)) { break; }

        app_le_gaf_scanning_transition(EVENT_SCAN_FOUND);
    } while (false);
}

__STATIC void app_le_gaf_scanning_cb_announcement(const gap_bdaddr_t* p_addr, uint8_t type,
                                                  uint32_t context_bf, const gaf_ltv_t* p_metadata)
{
    if (p_env->params_device_found.p_addr != NULL)
    {
        app_le_gaf_scanning_transition(EVENT_SCAN_FOUND);
    }
}

// Set of callback function for communication with GAF Scanning module
__STATIC const gaf_scan_cb_t cbs_gaf_scan = {
    .cb_cmp_evt = app_le_gaf_scanning_cb_cmp_evt,
    .cb_stopped = app_le_gaf_scanning_cb_stopped,
    .cb_report = app_le_gaf_scanning_cb_report,
    .cb_announcement = app_le_gaf_scanning_cb_announcement,
};

/*
 * FUNCTIONS
 ****************************************************************************************
 */
uint8_t delay_connect = 1000;
__STATIC void mediator_audio_unicast_client_cb_event_scanning(uint16_t type, uint16_t status, const void* p_params)
{
    APP_DBG("mediator_audio_unicast_client_cb_event_scanning: type 0x%x, status 0x%x\n", type, status);
    switch (type)
    {
        case COMPONENT_EVENT_DEVICE_FOUND:
        {
            const app_le_gaf_scanning_device_found_t* p_params_device
                                                            = (const app_le_gaf_scanning_device_found_t*)p_params;

            APP_DBG("found addr 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n", 
                        p_params_device->p_addr->addr[0], p_params_device->p_addr->addr[1], p_params_device->p_addr->addr[2], 
                        p_params_device->p_addr->addr[3], p_params_device->p_addr->addr[4], p_params_device->p_addr->addr[5]);
            
            // if (memcmp(p_params_device->p_device_name, "MV_cis", 6)) { return; }
            
            // uint8_t target_addr[6] = {0x56, 0x83, 0x13, 0x22, 0xe8, 0xf6};
            // if (memcmp(p_params_device->p_addr->addr, target_addr, 6) != 0) { return; }
            // app_le_scan_stop();
        } break;

        case COMPONENT_EVENT_STOPPED:
        {
            app_le_start_connect(&p_env->params_device_found);
        } break;

        default: { } break;
    }
}

void app_start_le_conn(void)
{
#ifdef LE_CIS_SRC
    app_le_start_connect(&p_env->params_device_found);
#endif
}

__STATIC void app_le_gaf_scanning_configure(const void* p_params, component_cb_event cb_event)
{
    ASSERT_ERR(p_env == NULL);
    ASSERT_ERR(cb_event != NULL);

    p_env = (app_le_gaf_scanning_env_t*)ke_malloc_user(sizeof(app_le_gaf_scanning_env_t), KE_MEM_PROFILE);
    ASSERT_ERR(p_env != NULL);
    memset(p_env, 0, sizeof(app_le_gaf_scanning_env_t));
    p_env->cb_mediator_event = cb_event;

    // Configure use of GAF Scanning module
    uint16_t status = gaf_scan_configure(&cbs_gaf_scan);
    if (status)
    {
        APP_DBG("gaf_scan_configure: err 0x%x\n", status);
    }
}

__STATIC void app_le_gaf_scanning_control(uint16_t type, const void* p_params)
{
    APP_DBG("app_le_gaf_scanning_control: type 0x%x\n", type);
    ASSERT_ERR(p_env != NULL);

    switch (type)
    {
        case COMPONENT_CONTROL_START:
        {
            app_le_gaf_scanning_start_t* p_params_start = (app_le_gaf_scanning_start_t*)p_params;

            if ((p_params_start != NULL) && p_params_start->is_rsi_needed)
            {
                SETB(p_env->status_bf, STATUS_RSI_NEEDED, 1);
            }
            else
            {
                SETB(p_env->status_bf, STATUS_RSI_NEEDED, 0);
            }

            app_le_gaf_scanning_transition(EVENT_SCAN_START);
        } break;

        case COMPONENT_CONTROL_STOP:
        {
            app_le_gaf_scanning_transition(EVENT_SCAN_STOP);
        } break;

        default: { ASSERT_WARN(0, type, 0); } break;
    }
}

void app_le_scan_start(void)
{
    app_le_scan_configure();
    app_le_gaf_scanning_start_t scan_params;
    scan_params.is_rsi_needed = 0;
    app_le_gaf_scanning_control(COMPONENT_CONTROL_START, (const void*)&scan_params);
}

void app_le_scan_stop(void)
{
    APP_DBG("app_le_scan_stop\n");
    app_le_gaf_scanning_control(COMPONENT_CONTROL_STOP, NULL);
}

void app_le_scan_configure(void)
{
    app_le_gaf_scanning_configure(NULL, mediator_audio_unicast_client_cb_event_scanning);
}
/// \example{lineno} app_le_gaf_scanning.c

#endif  /* BT_LE_AUDIO_SUPPORT */
