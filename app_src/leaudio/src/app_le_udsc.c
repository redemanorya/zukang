/**
 ****************************************************************************************
 *
 * @file applet_broadcast_assistant.c
 *
 * @brief Applet - Broadcast Assistant
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

#include "gapm_le_init.h"
#include "le_audio_config.h"
#include "debug.h"
#include "le_udsc.h"
#include "uds_common.h"
#include "ad_parser.h"
#include "bluetooth_data_types.h"
#include "debug.h"

// extern uint8_t target_bcast_name[30];

void app_le_udsc_rcv_data(uint8_t conidx, uint16_t hdr, uint16_t len,uint8_t *data)
{
    APP_DBG("value: %d\n",len);
       for(int i=0;i<len;i++)
       {
           APP_DBG("0x%02x ",data[i]);
       }
}
#ifdef LE_UDSC
/*
 * ENUMERATIONS
 ****************************************************************************************
 */

#define CONFIGURATION                                                                   \
    {                                                                                   \
        .role = GAP_ROLE_LE_OBSERVER | GAP_ROLE_LE_CENTRAL | GAP_ROLE_LE_BROADCASTER | GAP_ROLE_LE_PERIPHERAL,        \
        .pairing_mode = GAPM_PAIRING_MODE_ALL,                                                              \
        .privacy_cfg = 0,                                                               \
        .renew_dur = 1500,                                                              \
        .private_identity.addr = {0, 0, 0, 0, 0, 0},                                    \
        .irk.key = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},                    \
        .gap_start_hdl = 0,                                                             \
        .gatt_start_hdl = 0,                                                            \
        .att_cfg = 0,                                                                   \
        .sugg_max_tx_octets = GAP_LE_MAX_OCTETS,                                        \
        .sugg_max_tx_time = GAP_LE_MIN_TIME,                                            \
        .tx_pref_phy = GAP_PHY_ANY,                                                     \
        .rx_pref_phy = GAP_PHY_ANY,                                                     \
        .tx_path_comp = 0,                                                              \
        .rx_path_comp = 0,                                                              \
        .class_of_device = 0x200408,                                                    \
        .dflt_link_policy = 0,                                                          \
    }


typedef struct synchronize_source
{
    /// Advertising ID
    bap_adv_id_t adv_id;
    /// Broadcast ID
    bap_bcast_id_t bcast_id;
    /// Synchronization bit field
    uint32_t sync_bf;
} synchronize_source_t;

// Status bit field meaning
enum app_le_udsc_status_bf
{
    // Indicate if discovery is in progress - Position
    STATUS_DISCOVERING_POS = 0,
    // Indicate if discovery is in progress - Bit
    STATUS_DISCOVERING_BIT = CO_BIT(STATUS_DISCOVERING_POS),

    // Indicate if Broadcast Code is available - Position
    STATUS_BCAST_CODE_AVAILABLE_POS = 1,
    // Indicate if discovery is in progress - Bit
    STATUS_BCAST_CODE_AVAILABLE_BIT = CO_BIT(STATUS_BCAST_CODE_AVAILABLE_POS),
};

/*
 * TYPES DEFINITION
 ****************************************************************************************
 */

// Environment structure
typedef struct app_le_udsc_env
{
    // Connection local index
    uint8_t conidx;
    // Status bit field (see #applet_broadcast_assistant_status_bf enumeration)
    uint8_t status_bf;
    // Source added
    bool source_added;
    uint8_t actv_idx;
    bap_adv_id_t adv_id;
    // Event callback function for backward communication with Mediator
    component_cb_event cb_mediator_event;
} app_le_udsc_env_t;


// Event values for main state machine
enum app_le_connection_event
{
    // Request to create and start initiating activity
    EVENT_CREATE = 0U,
    // Initiating activity has been started
    EVENT_STARTED,
    // Request activity to be stopped
    EVENT_STOP,
    // Initiating activity has been stopped
    EVENT_STOPPED,
    // Initiating activity has been deleted
    EVENT_DELETED,
    // Connection has been established
    EVENT_ESTABLISHED,
    // Request to disconnect
    EVENT_DISCONNECT,
    // Disconnection has occurred
    EVENT_DISCONNECTED,
};

// Pointer to environment
__STATIC app_le_udsc_env_t* p_env;

/// Broadcast name
__STATIC const char le_bis_source_broadcast_name[] = "MV_BIS_SRC";

extern uint16_t app_gaf_client_configure(void);
/*
 * CALLBACK FUNCTIONS
 ****************************************************************************************
 */
__STATIC void app_le_connection_cb_gapm_le_init_proc_cmp(uint32_t token, uint8_t proc_id, uint8_t actv_idx,
                                                            uint16_t status)
{
    APP_DBG("app_le_connection_cb_gapm_le_init_proc_cmp: proc_id 0x%X, status 0x%x\n", proc_id, status);

    switch (proc_id)
    {
        case GAPM_ACTV_START:
        {
            // app_le_connection_state_connection(EVENT_STARTED, status, NULL);
        } break;

        case GAPM_ACTV_DELETE:
        {
            // app_le_connection_state_connection(EVENT_DELETED, status, NULL);
        } break;

        default: { } break;
    }
}

__STATIC void app_le_connection_cb_gapm_le_init_stopped(uint32_t token, uint8_t actv_idx, uint16_t reason)
{
    APP_DBG("app_le_connection_cb_gapm_le_init_stopped: actv_idx 0x%x, reason 0x%x\n", actv_idx, reason);
    // app_le_connection_state_connection(EVENT_STOPPED, GAP_ERR_NO_ERROR, NULL);

}

__STATIC const gapm_le_init_cb_actv_t cbs_gapm_le_init =
{
    .hdr.actv.stopped = app_le_connection_cb_gapm_le_init_stopped,
    .hdr.actv.proc_cmp = app_le_connection_cb_gapm_le_init_proc_cmp,
    .hdr.addr_updated = NULL,
    .peer_name = NULL,
};

__STATIC void udsc_configuration_cb_gapm_bdaddr(uint32_t event, uint16_t status, const gap_addr_t* p_addr)
{
	udsc_configuration_state(event, status, p_addr);
}

void udsc_configuration_cb_gapm_cmp(uint32_t event, uint16_t status)
{
	udsc_configuration_state(event, status, NULL);
}

void udsc_configuration_state(uint16_t event, uint16_t status, const void* p_params)
{
    bool is_finished = false;
    APP_DBG("udsc_configuration_state event 0x%x, status: %d\n", event, status);

	switch (event)
	{

		case EVENT_CONFIGURATION_DONE:
		{
			APP_CHECK_STATUS(gapm_set_name(EVENT_CONFIGURATION_NAME_SET, strlen(le_bis_source_broadcast_name), (const uint8_t*)le_bis_source_broadcast_name,
							 udsc_configuration_cb_gapm_cmp));
		} break;

		case EVENT_CONFIGURATION_NAME_SET:
		{
			APP_CHECK_STATUS(gapm_get_controller_bdaddr(EVENT_CONFIGURE_READ_CONTROLLER_ADDRESS,
														udsc_configuration_cb_gapm_bdaddr));
		} break;

		case EVENT_CONFIGURE_READ_CONTROLLER_ADDRESS:
		{
			is_finished = true;
		} break;

		default: { } break;
	} while (false);


    if (is_finished)
    {   
#ifndef LE_BIS_ASSIS
        app_le_udsc_start_scan();
#endif
    }
}

__STATIC void app_le_connection_state_connection(uint16_t event, uint16_t status, const void* p_params)
{
    APP_DBG("app_le_connection_state_connection: event 0x%x, event 0x%x\n", status, event);
    uint8_t ret = 0;

    switch (event)
    {
        case EVENT_CREATE:
        {
        	const app_udsc_found_t* p_connect_params = (const app_udsc_found_t*)p_params;
            gapm_le_init_param_t init_params;
            uint16_t conn_intv, ce_len_min, ce_len_max;

            ASSERT_ERR(p_params != NULL);

            init_params.prop = GAPM_INIT_PROP_1M_BIT;
            init_params.conn_to = 0;
            memcpy(&(init_params.peer_addr), &(p_connect_params->addr), sizeof(gap_bdaddr_t));
            init_params.scan_param_1m.scan_intv = 160;
            init_params.scan_param_1m.scan_wd = 120;
            init_params.conn_param_1m.supervision_to = 1000;
            init_params.conn_param_2m.supervision_to = 1000;
            init_params.conn_param_coded.supervision_to = 1000;

            #if (BT_53)
            if (p_connect_params->use_subrating)
            {
                // Use 15ms connection interval
                conn_intv = 12;
                ce_len_min = 1;
                ce_len_max = 1;
            }
            else
            #endif //(BT_53)
            {
                conn_intv = 48;
                ce_len_min = 5;
                ce_len_max = 10;
            }

            init_params.conn_param_1m.conn_intv_min = conn_intv;
            init_params.conn_param_1m.conn_intv_max = conn_intv;
            init_params.conn_param_1m.ce_len_min = ce_len_min;
            init_params.conn_param_1m.ce_len_max = ce_len_max;
            init_params.conn_param_1m.conn_latency = 0;
            init_params.conn_param_2m.conn_intv_min = conn_intv;
            init_params.conn_param_2m.conn_intv_max = conn_intv;
            init_params.conn_param_2m.ce_len_min = ce_len_min;
            init_params.conn_param_2m.ce_len_max = ce_len_max;
            init_params.conn_param_2m.conn_latency = 0;
            init_params.conn_param_coded.conn_intv_min = conn_intv;
            init_params.conn_param_coded.conn_intv_max = conn_intv;
            init_params.conn_param_coded.ce_len_min = ce_len_min;
            init_params.conn_param_coded.ce_len_max = ce_len_max;
            init_params.conn_param_coded.conn_latency = 0;

            #if (BT_53)
            p_env->use_subrating = false;//p_connect_params->use_subrating;
            #endif //(BT_53)

            uint8_t actv_idx;
            ret = gapm_le_create_init(0, GAPM_STATIC_ADDR, &cbs_gapm_le_init,
                                                       &actv_idx);
            if (ret)    
            {
                APP_DBG("udsc: gapm_le_create_init: error 0x%x\n", ret);
                return;
            }                                       

            printf("baikal: gapm_le_create_init: actv_idx 0x%x\n", actv_idx);
            ret = gapm_le_start_direct_connection(actv_idx, &init_params);

            if (ret) 
            {
                APP_DBG("gapm_le_start_direct_connection: error 0x%x\n", ret);
            }

        } break;

        default: { } break;
    } while (false);
}

void app_le_start_conn(uint16_t status, const void* p_params)
{
    app_le_connection_state_connection(EVENT_CREATE, status, p_params);
    if (status)
    {
        APP_DBG("le_udsc_start_conn failed; status 0x%x\n", status);
    }
}


void app_le_udsc_start_scan(void)
{
    APP_DBG("app_le_udsc_start_scan\n");
    uint16_t status = le_udsc_start_scan(0);
    if (status)
    {
        APP_DBG("app_le_udsc_start_scan error 0x%x\n", status);
    }
}

uint16_t app_le_udsc_stop_scan(void)
{
    uint16_t status = le_udsc_stop_scan();
    if (status)
    {
        APP_DBG("app_le_udsc_stop_scan error 0x%x\n", status);
    }
    return status;
}
static void name_dumpvalue(const uint8_t* value, uint16_t value_len)
{
	uint8_t i = 0;
    APP_DBG("BLE name : ");
    for(;i<value_len;i++)
    {
    APP_DBG("%c",value[i]);
    }
    APP_DBG("\r\n");

}

int prase_adv_data(const char *name,const uint8_t *adv_data, uint16_t adv_len)
{
    int name_len = strlen(name);

    // iterate over advertisement data
    ad_context_t context;
    for (ad_iterator_init(&context, adv_len, adv_data); ad_iterator_has_more(&context); ad_iterator_next(&context))
    {
        uint8_t data_type = ad_iterator_get_data_type(&context);
        uint8_t data_size = ad_iterator_get_data_len(&context);
        const uint8_t *data = ad_iterator_get_data(&context);
        switch (data_type)
        {
        case BLUETOOTH_DATA_TYPE_SHORTENED_LOCAL_NAME:
        case BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME:
            // compare prefix
            if (data_size < name_len)
                break;
            name_dumpvalue(data,name_len);
            if (memcmp(data, name, name_len) == 0)
                return 1;
            //                return 1;
        default:
            break;
        }
    }
    return 0;
}
__STATIC void app_le_udsc_cb_scan_report(uint8_t addr_type, const uint8_t* p_addr, uint16_t length,
                                                         const uint8_t* p_adv_data)
{
    // APP_DBG("app_le_udsc_cb_scan_report: addr 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n", p_addr[0], p_addr[1], p_addr[2], p_addr[3], p_addr[4], p_addr[5]);
    uint16_t status = 0;
    gap_bdaddr_t udsc_addr;

#if 1//add filter of le name
    const uint8_t* p_reported_name = NULL;
    uint8_t name_length = 0;

    p_reported_name = gapm_get_ltv_value(GAP_AD_TYPE_SHORTENED_NAME, length, p_adv_data,
                                            &(name_length));
    if (p_reported_name == NULL)
    {
        p_reported_name = gapm_get_ltv_value(GAP_AD_TYPE_COMPLETE_NAME, length, p_adv_data,
                                                &(name_length));
    }

    if (p_reported_name != NULL)  
    {
        if (!memcmp(p_reported_name, "MV_cis_sink_baikal", name_length))
        // if (!memcmp(p_reported_name, target_bcast_name, name_length))
        {
            APP_DBG("name_len %d\n", name_length);
        }
        else
        {
            return;    
        }
    }
    else
    {
        // APP_DBG("p_reported_name is null\n");
        return;      
    }
#endif//add filter of le name
    memcpy(udsc_addr.addr, p_addr, GAP_BD_ADDR_LEN);
    udsc_addr.addr_type = addr_type;

    app_udsc_found_t app_udsc_found = {
        .addr = udsc_addr,
    };

#ifndef LE_BIS_ASSIS
    // status = le_udsc_start_conn(solica_addr);
    app_le_connection_state_connection(EVENT_CREATE, status, &app_udsc_found);
 
    if (status)
    {
    	APP_DBG("le_udsc_start_conn failed; status 0x%x\n", status);
    }
#endif
}
void app_le_udsc_cb_read_cmp(uint8_t conidx, uint16_t status, uint8_t val_id, const union uds_value* p_value)
{
    APP_DBG("app_le_udsc_cb_read_cmp\n");
}

void app_le_udsc_cb_write_cmp(uint8_t conidx, uint16_t status, uint8_t val_id)
{
    APP_DBG("app_le_udsc_cb_write_cmp\n");
}

void app_le_udsc_cb_db_chg_inc(uint8_t conidx, uint32_t value)
{
    APP_DBG("app_le_udsc_cb_db_chg_inc: condix %d, value 0x%x\n");
}

// Set of callback functions for communication with BAP Broadcast Assistant module
__STATIC const le_udsc_cb_t cbs_udsc =
{
    .cb_read_cmp = app_le_udsc_cb_read_cmp,
    .cb_write_cmp = app_le_udsc_cb_write_cmp,
    .cb_scan_report = app_le_udsc_cb_scan_report,
    .cb_db_chg_inc = app_le_udsc_cb_db_chg_inc,
};

/*
 * FUNCTIONS
 ****************************************************************************************
 */

void app_le_udsc_write(uint8_t val_id, uint32_t set_value)
{
    // uint8_t val_id = UDSC_CHAR_UDS_DB_CHG_INC;

    union uds_value value;
    union uds_value* p_value = &value;
    uint8_t conidx = get_conidx_from_role(ROLE_MASTER);
    if (conidx == 0xff)
    {
        APP_DBG("app_le_udsc_write: get invaid condix\n");
        return;
    }

    p_value = le_udsc_prepare_val_data(conidx, val_id);
    if (!p_value)
    {
    	return;
    }

    p_value->set[0] = (uint8_t)set_value;
    p_value->set[1] = (uint8_t)(set_value >> 8);
    p_value->set[2] = (uint8_t)(set_value >> 16);
    p_value->set[3] = (uint8_t)(set_value >> 24);
    
    le_udsc_write(conidx, val_id, p_value);
}

__STATIC void app_le_udsc_configure(const void* p_params, component_cb_event cb_event)
{
    // Device appearance
    uint16_t APPEARANCE = 0x00;
    uint8_t ret;

    ASSERT_ERR(p_env == NULL);
    ASSERT_ERR(cb_event != NULL);

    p_env = (app_le_udsc_env_t*)ke_malloc_user(sizeof(app_le_udsc_env_t),
                                                              KE_MEM_PROFILE);
    ASSERT_ERR(p_env != NULL);
    memset(p_env, 0, sizeof(app_le_udsc_env_t));
    p_env->conidx = GAP_INVALID_CONIDX;
    p_env->cb_mediator_event = cb_event;

    ret = le_udsc_configure(&cbs_udsc, 0);
    if (ret)
    {
        APP_DBG("app_le_udsc_configure failed: ret 0x%x\n", ret);
    }
}

void app_le_udsc_init(void)
{
#if !defined(LE_BIS_ASSIS) && !defined(LE_CIS_SRC)
    uint16_t status = app_gaf_client_configure();
    if (status)
    {
        APP_DBG("app_le_udsc_init: app_gaf_client_configure err 0x%x\n", status);
    }
#endif
    app_le_udsc_configure(NULL, NULL);
}
#endif
