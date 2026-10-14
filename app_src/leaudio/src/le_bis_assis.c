/**
 ****************************************************************************************
 *
 * @file le_bis_assis.c
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
#include "le_audio_config.h"
#if (BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT)
#include "le_bis_assis.h"
#include "bap_bc_assist.h"
#include "gapc_sec.h"
#include "gapm_le_init.h"
#include "debug.h"
#include "gapm.h"
#include "bap.h"
#include "bap_bc_scan.h"
#include "gapm_le_per_sync.h"

char target_bcast_name[30] = "Galaxy S24";
// uint8_t target_deleg_name[30];
// uint8_t g_bcast_id[3];

#ifdef LE_BIS_ASSIS

#define COMPONENT_EVENT_SERVICE_CONTENT_DISCOVERED   (8)
#define COMPONENT_EVENT_SERVICE_DISCOVERED           (7)
#define EVENT_SERVICE_DISCOVERY_CONTENT_DISCOVERED   (3)
#define EVENT_SERVICE_DISCOVERY_COMPLETED            (4)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */
#define MAX_SRC_NUM      (10)
#define MAX_BCAST_NAME_LEN      (32)
static uint8_t g_src_idx = 0;
extern uint8_t switch_src_flag;

typedef struct synchronize_source
{
    /// Advertising ID
    bap_adv_id_t adv_id;
    /// Broadcast ID
    bap_bcast_id_t bcast_id;
    /// Synchronization bit field
    uint32_t sync_bf;
} synchronize_source_t;

// Event values for Service Discovery state machine
enum le_bis_assis_event_service_discovery
{
    // Request to start
    EVENT_SERVICE_DISCOVERY_START = 0,
    // Discovery of Broadcast Audio Scan Service content has been completed
    EVENT_SERVICE_CONTENT_DISCOVERED,
};

// Status bit field meaning
enum le_bis_assis_status_bf
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
// Bond data structure
typedef struct applet_bond_data_env
{
    // Keys exchanged during the pairing
    gapc_pairing_keys_t keys;
    // Local IRK
    gap_sec_key_t local_irk;
} le_assis_security_bond_data_t;

typedef struct app_control_start_discovery
{
    /// Connection index
    uint8_t conidx;
    /// Pointer to service information\n
    /// May be NULL
    const prf_svc_t* p_svc;
} app_control_start_discovery_t;

typedef struct sink_add_src
{
    uint8_t src_idx;
    bap_adv_id_t adv_id;
    bap_bcast_id_t bcast_id;
    uint8_t broadcast_name_len;
    uint8_t broadcast_name[MAX_BCAST_NAME_LEN];
} sink_add_src_t;

// Environment structure
typedef struct le_bis_assis_env
{
    // Connection local index
    uint8_t conidx;
    // Status bit field (see #le_bis_assis_status_bf enumeration)
    uint8_t status_bf;
    // Broadcast code
    uint8_t bcast_code[GAP_KEY_LEN];
    // Source added
    bool source_added;
    // Event callback function for backward communication with Mediator
    component_cb_event cb_mediator_event;
    uint8_t actv_idx;

    // Bond data for ongoing pairing
    le_assis_security_bond_data_t bond_data;
    // bap_adv_id_t adv_id;
    // bap_bcast_id_t bcast_id;

    //the src to be switched
    uint8_t target_src_idx;

    uint8_t src_lid;
    // bap_adv_id_t src_adv_id;
    sink_add_src_t src_info;
    // bcast_code added
    bool bcast_code_set;
} le_bis_assis_env_t;

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

__STATIC sink_add_src_t src_array[MAX_SRC_NUM];
/// Broadcast name
__STATIC const char lle_bis_assis_broadcast_name[] = "MV_BIS_ASSIS";

// Pointer to environment
__STATIC le_bis_assis_env_t* p_env;
extern uint16_t app_gaf_client_configure(void);
extern void app_le_audio_discovery(uint8_t event, uint8_t conidx, uint16_t status, const void* p_params);
/*
 * STATE MACHINES
 ****************************************************************************************
 */

__STATIC void le_bis_scan_cb_cmp_evt(uint8_t cmd_type, uint16_t status, uint8_t pa_lid)
{
    //cmd_type: bap_bc_scan_cmd_type
	APP_DBG("le_bis_scan_cb_cmp_evt: cmd_type 0x%x, status %d\n", cmd_type, status);
}

__STATIC void le_bis_scan_cb_timeout(void)
{
	APP_DBG("le_bis_scan_cb_timeout\n");
}

__STATIC void le_bis_scan_cb_report(const bap_adv_id_t* p_adv_id, const bap_bcast_id_t* p_bcast_id,
                                              uint8_t info_bf, const gaf_adv_report_air_info_t* p_air_info,
                                              uint16_t length, const uint8_t* p_data)
{
   APP_DBG("le_bis_scan_cb_report \n");
#if 1//(!APP_GAF_PBP)
#if 1//add filter of le name
    const uint8_t* p_reported_name = NULL;
    uint8_t name_length = 0;

    p_reported_name = gapm_get_ltv_value(GAP_AD_TYPE_SHORTENED_NAME, length, p_data,
                                            &(name_length));
    if (p_reported_name == NULL)
    {
        p_reported_name = gapm_get_ltv_value(GAP_AD_TYPE_COMPLETE_NAME, length, p_data,
                                                &(name_length));
    }

    if (p_reported_name != NULL)  
    {
        // if (!memcmp(p_reported_name, "Redmi K80", name_length))
        if (!memcmp(p_reported_name, target_bcast_name, name_length))
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
        APP_DBG("p_reported_name is null\n");
        return;      
    }
#endif//add filter of le name

#if 0
    g_bcast_id[0] = 0x55;
    g_bcast_id[0] = 0xa5;
    g_bcast_id[0] = 0x77;

    if (memcmp(p_bcast_id->id, g_bcast_id, 3))
    {
        return;
    }
#endif
    if (p_env->source_added == 0)
    {
        p_env->src_info.adv_id = *p_adv_id;
        p_env->src_info.bcast_id = *p_bcast_id;
        uint32_t bis_sync_bfs = 1; //sync bis_index[0]
        uint16_t status = bap_bc_assist_add_source(p_env->conidx, p_adv_id, p_bcast_id, BAP_BC_ASSIST_PA_SYNC_SYNC,
                                             BAP_BC_UNKNOWN_PA_INTV, 1, bis_sync_bfs, NULL); 
        if (status)
        {
            APP_DBG("le_bis_scan_cb_report: err 0x%x\n", status);
        }
        p_env->source_added = 1; 
    }
#endif
}

void app_broadcast_assistant_add_src(uint8_t src_idx)
{
    APP_DBG("app_broadcast_assistant_add_src: src_idx: %d, bcast_id 0x%x:0x%x:0x%x, addr 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n", 
                src_idx, src_array[src_idx].bcast_id.id[0], src_array[src_idx].bcast_id.id[1], 
                src_array[src_idx].bcast_id.id[2], src_array[src_idx].adv_id.addr[0], src_array[src_idx].adv_id.addr[1], 
                src_array[src_idx].adv_id.addr[2],src_array[src_idx].adv_id.addr[3], src_array[src_idx].adv_id.addr[4], 
                src_array[src_idx].adv_id.addr[5]);
                
    if (p_env->src_info.src_idx == GAF_INVALID_LID)
    {
        p_env->src_info.src_idx = src_idx;
        memcpy(&p_env->src_info.adv_id, &src_array[src_idx].adv_id, sizeof(bap_adv_id_t));
        memcpy(&p_env->src_info.bcast_id, &src_array[src_idx].bcast_id, sizeof(bap_bcast_id_t));
        memcpy(&p_env->src_info.broadcast_name, &src_array[src_idx].broadcast_name, src_array[src_idx].broadcast_name_len);

        //add src
        if (p_env->source_added == 0)
        {
            uint32_t bis_sync_bfs = 1; //sync bis_index[0]
            uint16_t status = bap_bc_assist_add_source(p_env->conidx, &p_env->src_info.adv_id, &p_env->src_info.bcast_id, 
                                BAP_BC_ASSIST_PA_SYNC_SYNC, BAP_BC_UNKNOWN_PA_INTV, 1, bis_sync_bfs, NULL);  
            if (status)
            {
                APP_DBG("app_broadcast_assistant_add_src: err 0x%x\n", status);
            }
            else
            {
                p_env->source_added = 1; 
            }
        }
    }
    else if (switch_src_flag)
    {

    }
}

bool check_src_in_array(const bap_adv_id_t* p_adv_id, const bap_bcast_id_t* p_bcast_id,
                                uint8_t broadcast_name_len, const uint8_t* p_broadcast_name)
{
    for (int i = 0; i < MAX_SRC_NUM; i++)
    {
        if ((!memcmp(p_bcast_id, &src_array[i].bcast_id, sizeof(bap_bcast_id_t)))
                && (!memcmp(p_adv_id, &(src_array[i].adv_id), sizeof(bap_adv_id_t)))
                && (!memcmp(p_broadcast_name, &src_array[i].broadcast_name, broadcast_name_len)))
        {
            return true;
        }
    }
    APP_DBG("check_src_in_array: bcast_id 0x%x, 0x%x, 0x%x\n", 
                p_bcast_id->id[0], p_bcast_id->id[1], p_bcast_id->id[2]);
    return false;
}

void app_broadcast_assistant_scan_src(const bap_adv_id_t* p_adv_id, const bap_bcast_id_t* p_bcast_id,
                                uint8_t broadcast_name_len, const uint8_t* p_broadcast_name)
{
    if (check_src_in_array(p_adv_id, p_bcast_id, broadcast_name_len, p_broadcast_name))
    {
        return;
    }

    for (int i = 0; i < MAX_SRC_NUM; i++)
    {
        if (src_array[i].src_idx == GAF_INVALID_LID)
        {
            src_array[i].src_idx = i;
            src_array[i].broadcast_name_len = broadcast_name_len;            
            memcpy(&src_array[i].bcast_id, p_bcast_id, sizeof(bap_bcast_id_t));  
            memcpy(&src_array[i].broadcast_name, p_broadcast_name, broadcast_name_len);  
            memcpy(&src_array[i].adv_id, p_adv_id, sizeof(bap_adv_id_t));

            APP_DBG("found src: src_idx: %d, bcast_id 0x%x:0x%x:0x%x, addr 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n", 
                        i, src_array[i].bcast_id.id[0], src_array[i].bcast_id.id[1],
                        src_array[i].bcast_id.id[2], src_array[i].adv_id.addr[0], src_array[i].adv_id.addr[1], 
                        src_array[i].adv_id.addr[2],src_array[i].adv_id.addr[3], src_array[i].adv_id.addr[4], 
                        src_array[i].adv_id.addr[5]); 
            
            // char target_bcast_name[] = "Redmi K70";
            if (!memcmp(&src_array[i].broadcast_name, target_bcast_name, broadcast_name_len))
            {
                APP_DBG("bcast name: Redmi K70\n");
            }
            else
            {
                return;
            }
            app_broadcast_assistant_add_src(i);
            return;
        }
    }
}

#if (APP_GAF_PBP)
__STATIC void le_bis_scan_cb_public_bcast_source(const bap_adv_id_t* p_adv_id,
                                                           const bap_bcast_id_t* p_bcast_id, uint8_t pbp_features_bf,
                                                           uint8_t broadcast_name_len,
                                                           const uint8_t* p_broadcast_name, uint8_t metadata_len,
                                                           const uint8_t* p_metadata)
{
	BT_DBG("le_bis_scan_cb_public_bcast_source\n");
#if 1
    const uint8_t* p_reported_name = NULL;
    uint8_t name_length = 0;
    if (!memcmp(p_reported_name, target_bcast_name, name_length))
    {
        APP_DBG("name_len %d\n", name_length);
    }
    else
    {
        return;    
    }
#endif
    //collect src info
    app_broadcast_assistant_scan_src(p_adv_id, p_bcast_id, broadcast_name_len, p_broadcast_name);
}
#endif

__STATIC void le_bis_scan_cb_pa_established(uint8_t pa_lid, const bap_adv_id_t* p_adv_id, uint8_t phy,
                                                      uint16_t interval_frames)
{
	APP_DBG("le_bis_scan_cb_pa_established\n");
}

__STATIC void le_bis_scan_cb_pa_terminated(uint8_t pa_lid, uint8_t reason)
{
	APP_DBG("le_bis_scan_cb_pa_terminated: pa_lid 0x%x, reason 0x%x\n", pa_lid, reason);
    bap_bc_scan_stop();
}

__STATIC void le_bis_scan_cb_pa_report(uint8_t pa_lid, const gaf_adv_report_air_info_t* p_air_info,
                                                 uint16_t length, const uint8_t* p_data)
{
	APP_DBG("le_bis_scan_cb_pa_report\n");
}

__STATIC void le_bis_scan_cb_big_info_report(uint8_t pa_lid, const gapm_le_big_info_t* p_report)
{
	APP_DBG("le_bis_scan_cb_big_info_report\n");
}

__STATIC void le_bis_scan_cb_group_report(uint8_t pa_lid, uint8_t nb_subgroups, uint8_t nb_streams,
                                                    uint32_t pres_delay_us)
{
	APP_DBG("le_bis_scan_cb_group_report\n");
}

__STATIC void le_bis_scan_cb_subgroup_report(uint8_t pa_lid, uint8_t sgrp_id, uint32_t stream_pos_bf,
                                                       const gaf_codec_id_t* p_codec_id, const bap_cfg_ptr_t* p_cfg,
                                                       const bap_cfg_metadata_ptr_t* p_metadata)
{
	APP_DBG("le_bis_scan_cb_subgroup_report\n");
}

__STATIC void le_bis_scan_cb_stream_report(uint8_t pa_lid, uint8_t sgrp_id, uint8_t stream_pos,
                                                     const gaf_codec_id_t* p_codec_id, const bap_cfg_ptr_t* p_cfg,
                                                     uint8_t* bcast_code, uint8_t* volume)
{
	APP_DBG("le_bis_scan_cb_stream_report\n");

}

void app_add_src_local(void)
{
    uint16_t status = 0;
    uint8_t nb_subgroups = GAP_INVALID_CONIDX;
    uint32_t bis_sync_bfs = 1; //sync bis_index[0]

    uint8_t grp_lid = app_get_broadcast_src_grp_lid();
    nb_subgroups = app_get_broadcast_src_nb_sgrp();

    if ((grp_lid == GAP_INVALID_CONIDX) || (nb_subgroups == 0))
    {
    	APP_DBG("app_add_src_local: grp_lid 0x%x, nb_subgroups 0x%x\n", grp_lid, nb_subgroups);
        return;
    }

    status = bap_bc_assist_add_source_local(p_env->conidx, grp_lid, BAP_BC_ASSIST_PA_SYNC_SYNC, nb_subgroups, &bis_sync_bfs);   
    if (status)
    {
        APP_DBG("bap_bc_assist_add_source_local: error 0x%x\n", status);  
    }

}

__STATIC const bap_bc_scan_cb_t cbs_bap_bc_scan =
{
    .cb_cmp_evt              = le_bis_scan_cb_cmp_evt,
    .cb_timeout              = le_bis_scan_cb_timeout,
    .cb_report               = le_bis_scan_cb_report,
    #if (APP_GAF_PBP)
    .cb_public_bcast_source  = le_bis_scan_cb_public_bcast_source,
    #endif //(APP_GAF_PBP)
    .cb_pa_established       = le_bis_scan_cb_pa_established,
    .cb_pa_terminated        = le_bis_scan_cb_pa_terminated,
    .cb_pa_report            = le_bis_scan_cb_pa_report,
    .cb_big_info_report      = le_bis_scan_cb_big_info_report,
    .cb_group_report         = le_bis_scan_cb_group_report,
    .cb_subgroup_report      = le_bis_scan_cb_subgroup_report,
    .cb_stream_report        = le_bis_scan_cb_stream_report,
};

void le_bis_assis_state_service_discovery(uint8_t event, uint16_t status, void* params)
{
    APP_DBG("le_bis_assis_state_service_discovery: event 0x%x\n", event);
    switch (event)
    {
        case EVENT_SERVICE_DISCOVERY_START:
        {
        	if (status)
        	{
        		return;
        	}

			app_control_start_discovery_t params_start_discovery = *(app_control_start_discovery_t *)params;
			p_env->conidx = params_start_discovery.conidx;

            status = bap_bc_assist_discover(params_start_discovery.conidx, params_start_discovery.p_svc->shdl, params_start_discovery.p_svc->ehdl);
            if (status)
            {
                APP_DBG("bap_bc_assist_discover: err 0x%x\n", status);
            }
        } break;

        case EVENT_SERVICE_CONTENT_DISCOVERED:
        {
            if (status != GAF_ERR_NO_ERROR)
            {
                break;
            }

            APP_DBG("BASS discovered (status = 0x%04X), conidx 0x%x\n", status, p_env->conidx);
            p_env->cb_mediator_event(COMPONENT_EVENT_SERVICE_CONTENT_DISCOVERED, GAF_ERR_NO_ERROR,
                                         &(p_env->conidx));

            SETB(p_env->status_bf, STATUS_DISCOVERING, 0);
        } break;

        default: { } break;
    }
}

void le_bis_assis_start_audio(void)
{
#ifdef ASSIS_ADD_SRC_LOCAL
    le_bis_source_play();
#else
            //remote scan
            bap_bc_scan_start(0);
            bap_bc_assist_update_scan(p_env->conidx, true);
#endif
}

/*
 * CALLBACK FUNCTIONS
 ****************************************************************************************
 */

void le_bis_assis_set_bcast_code(const uint8_t* p_bcast_code)
{
    if (p_bcast_code == NULL)
    {
        APP_DBG("Broadcast Key removed\n");
        SETB(p_env->status_bf, STATUS_BCAST_CODE_AVAILABLE, 0);
        p_env->bcast_code_set = 0;
    }
    else
    {
        APP_DBG("Broadcast Key set\n");
        memcpy(p_env->bcast_code, p_bcast_code, GAP_KEY_LEN);
        SETB(p_env->status_bf, STATUS_BCAST_CODE_AVAILABLE, 1);
        p_env->bcast_code_set = 1;
        uint8_t is_available = 1;
        
        bap_bc_assist_bcast_code_cfm(is_available, p_env->conidx, p_env->src_lid, p_env->bcast_code);
    }
}

void app_broadcast_assistant_add_source(uint32_t sync_bf)
{
    uint16_t status;
    status = bap_bc_assist_add_source(p_env->conidx, &(p_env->src_info.adv_id),
                                &(p_env->src_info.bcast_id), BAP_BC_ASSIST_PA_SYNC_SYNC,
                                BAP_BC_UNKNOWN_PA_INTV, 1, sync_bf, NULL);
    
    if (status)
    {
        APP_DBG("bap_bc_assist_add_source: err 0x%x\n", status);
    }
}

void app_broadcast_assistant_modify_source(uint8_t sync_bf, uint8_t pa_sync)
{
    uint16_t status;
    status = bap_bc_assist_modify_source(p_env->conidx, 0, pa_sync,
                                BAP_BC_UNKNOWN_PA_INTV, 1, (uint32_t)sync_bf, NULL);
    if (status)
    {
        APP_DBG("bap_bc_assist_modify_source: err 0x%x\n", status);
    }
}

uint16_t app_broadcast_assistant_remove_source(void)
{
    uint16_t status;
    status = bap_bc_assist_remove_source(p_env->conidx, p_env->src_lid);
    if (status)
    {
        APP_DBG("bap_bc_assist_remove_source: err 0x%x\n", status);
    }

    return status;
}

void app_bis_assistant_remove_src(void)
{
	uint8_t src_idx = p_env->src_info.src_idx;
    uint16_t status;

    APP_DBG("app_bis_assistant_remove_src: src_idx: %d, bcast_id 0x%x:0x%x:0x%x, addr 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n", 
                src_idx, src_array[src_idx].bcast_id.id[0], src_array[src_idx].bcast_id.id[1],
                src_array[src_idx].bcast_id.id[2], src_array[src_idx].adv_id.addr[0], src_array[src_idx].adv_id.addr[1], 
                src_array[src_idx].adv_id.addr[2],src_array[src_idx].adv_id.addr[3], src_array[src_idx].adv_id.addr[4], 
                src_array[src_idx].adv_id.addr[5]);

    //remove src
    status = app_broadcast_assistant_remove_source();
    if (status)
    {
        APP_DBG("app_broadcast_assistant_remove_source: err 0x%x\n", status);
    }
    else
    {
        memset(&p_env->src_info, 0, sizeof(sink_add_src_t));
        memset(&(p_env->src_info.adv_id), 0, sizeof(bap_adv_id_t));
        memset(&(p_env->src_info.bcast_id), 0, sizeof(bap_bcast_id_t));
        p_env->src_info.src_idx = GAF_INVALID_LID;
    }
}

void app_bis_assistant_switch_src(uint8_t target_src_idx)
{
    APP_DBG("idx 0x%x, 0x%x: 0x%x\n", target_src_idx, p_env->target_src_idx, p_env->src_info.src_idx);
    if (target_src_idx == p_env->src_info.src_idx)
    {
        return;
    }

    p_env->target_src_idx = target_src_idx;
    app_broadcast_assistant_modify_source(0, 0);
}

__STATIC void le_bis_assis_cb_bond_data_prov(uint16_t component_id, uint16_t type, uint8_t conidx,
                                                           bool found, uint16_t length, const void* p_data)
{
    if (found == true)
    {
        const bap_bc_assist_bass_t* p_bond_data = (const bap_bc_assist_bass_t*)p_data;
        APP_CHECK_STATUS(bap_bc_assist_restore_bond_data(conidx, p_bond_data));
    }
}

__STATIC void le_bis_assis_cb_cmp_evt(uint8_t cmd_type, uint16_t status, uint8_t conidx,
                                                    uint8_t src_lid)
{
    APP_DBG("le_bis_assis_cb_cmp_evt: cmd_type: 0x%X, status 0x%x, src_lid 0x%x\n", cmd_type, status, src_lid);

    switch (cmd_type)
    {
        case BAP_BC_ASSIST_CMD_TYPE_DISCOVER:
        {
            le_bis_assis_state_service_discovery(EVENT_SERVICE_CONTENT_DISCOVERED, status, NULL);
        } break;

        case BAP_BC_ASSIST_CMD_TYPE_ADD_SOURCE:
        {
            if (status)
            {
                APP_DBG("ADD_SOURCE err: 0x%x\n", status);
                //add local src again
                app_broadcast_assistant_add_source(1);
                // p_env->source_added = 0;
            }
            else
            {
                if (switch_src_flag)
                {
                    p_env->target_src_idx = GAP_INVALID_CONIDX;
                    switch_src_flag = 0;
                }
            }
        } break;

        case BAP_BC_ASSIST_CMD_TYPE_MODIFY_SOURCE:
        {
        } break;

        case BAP_BC_ASSIST_CMD_TYPE_REMOVE_SOURCE:
        {
            if (!status)
            {
                APP_DBG("REMOVE_SOURCE\n");
                p_env->source_added = 0;

                // if (p_env->target_src_idx == p_env->src_info.src_idx)
                // {
                //     switch_src_flag = 0;
                //     break;
                // }                
                //remote scan
                bap_bc_scan_start(0);
                bap_bc_assist_update_scan(p_env->conidx, true);
            }
        } break;

        case BAP_BC_ASSIST_CMD_TYPE_UPDATE_SCAN:
        {
            // if (switch_src_flag == 1)
            // {
            //     app_broadcast_assistant_add_src(p_env->target_src_idx);
            // }

        } break;

        case BAP_BC_ASSIST_CMD_TYPE_ADD_SOURCE_LOCAL:
        {
            if (status)
            {
                //add local src again
                app_add_src_local();
            }
        } break;
        

        

        default: { } break;
    }
}
uint8_t sink_synced = 0;
__STATIC void le_bis_assis_cb_state(uint8_t conidx, uint8_t src_lid, bool is_used, uint8_t src_id,
                                                  const bap_adv_id_t* p_adv_id, const bap_bcast_id_t* p_bcast_id,
                                                  uint8_t pa_sync, uint8_t big_enc, const uint8_t* p_bad_code,
                                                  uint8_t nb_subgroups, uint32_t bis_sync_bf,
                                                  const bap_cfg_metadata_ptr_t* p_metadata)
{
    uint16_t status;
    APP_DBG("le_bis_assis_cb_state, Source %d - PA = %d, BIS sync = 0x%08X, encryption = %d, conidx 0x%x\n",
                 src_lid, pa_sync, bis_sync_bf, big_enc, conidx);

    if ((pa_sync == BAP_BC_RX_STATE_PA_NOT_SYNCED) && (bis_sync_bf == 0))
    {
        if (p_env->source_added)
        {
            p_env->src_lid = src_lid;
        }

        // if (switch_src_flag && (p_env->target_src_idx != p_env->src_info.src_idx) 
        //         && (p_env->target_src_idx != GAP_INVALID_CONIDX) && p_env->source_added)
        if (switch_src_flag && p_env->source_added && (p_env->target_src_idx != GAP_INVALID_CONIDX) )
        {
            app_bis_assistant_remove_src();
        }
    }

    if ((pa_sync == BAP_BC_RX_STATE_PA_SYNCED) && (big_enc == BAP_BC_BIG_ENCRYPT_STATE_CODE_REQUIRED))
    {
        char tmp_bcast_code[] = LE_AUDIO_PHONE_BCAST_CODE;
        uint8_t bcast_code[16] = {0};

        memcpy(bcast_code, tmp_bcast_code, strlen(LE_AUDIO_PHONE_BCAST_CODE));
        // for (int i = 0; i < strlen(LE_AUDIO_PHONE_BCAST_CODE); i++) {
        //     APP_DBG("bcast_code %d, 0x%x\n", i, bcast_code[i]);
        // }

        le_bis_assis_set_bcast_code(bcast_code);
    }
}

__STATIC void le_bis_assis_cb_state_subgroup(uint8_t conidx, uint8_t src_lid,
                                                           uint8_t sgrp_idx, uint32_t bis_sync_bf,
                                                           const bap_cfg_metadata_ptr_t* p_metadata)
{
    APP_DBG("le_bis_assis_cb_state_subgroup\n");
}

__STATIC void le_bis_assis_cb_bcast_code_req(uint8_t conidx, uint8_t src_lid)
{
    bool is_available = GETB(p_env->status_bf, STATUS_BCAST_CODE_AVAILABLE);
    APP_DBG("Broadcast Code requested: 0x%x\n", is_available);

    bap_bc_assist_bcast_code_cfm(is_available, conidx, src_lid, p_env->bcast_code);
}

__STATIC void le_bis_assis_cb_bond_data(uint8_t conidx, const bap_bc_assist_bass_t* p_bass_info)
{
    APP_DBG("Number of Sources: %d\n", p_bass_info->nb_rx_state);
}

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

/*
 * STATE MACHINES
 ****************************************************************************************
 */

__STATIC void app_le_connection_state_connection(uint16_t event, uint16_t status, const void* p_params)
{
    APP_DBG("app_le_connection_state_connection: event 0x%x, event 0x%x\n", status, event);

    switch (event)
    {
        case EVENT_CREATE:
        {
        	const applet_solicatation_found_t* p_connect_params = (const applet_solicatation_found_t*)p_params;
            gapm_le_init_param_t init_params;
            uint16_t conn_intv, ce_len_min, ce_len_max;

            ASSERT_ERR(p_params != NULL);

            init_params.prop = GAPM_INIT_PROP_1M_BIT;
            init_params.conn_to = 0;
            memcpy(&(init_params.peer_addr), &(p_connect_params->addr), sizeof(gap_bdaddr_t));
            init_params.scan_param_1m.scan_intv = 160;
            init_params.scan_param_1m.scan_wd = 80;
            init_params.conn_param_1m.supervision_to = 100;
            init_params.conn_param_2m.supervision_to = 100;
            init_params.conn_param_coded.supervision_to = 100;

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


            status = gapm_le_create_init(0, GAPM_STATIC_ADDR, &cbs_gapm_le_init,
                                                       &(p_env->actv_idx));
            if (status)
            {
                APP_DBG("gapm_le_create_init: error 0x%x\n", status);
            }

            status = gapm_le_start_direct_connection(p_env->actv_idx, &init_params);
            if (status)
            {
                APP_DBG("gapm_le_start_direct_connection: error 0x%x\n", status);
            }

        } break;

        default: { } break;
    } while (false);
}

__STATIC void le_bis_assis_cb_solicitation(uint8_t addr_type, const uint8_t* p_addr, uint16_t length,
                                                         const uint8_t* p_adv_data)
{
    APP_DBG("le_bis_assis_cb_solicitation: addr 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n", 
                p_addr[0], p_addr[1], p_addr[2], p_addr[3],p_addr[4], p_addr[5]);
    uint16_t status = 0;
    gap_bdaddr_t solica_addr;

    memcpy(solica_addr.addr, p_addr, GAP_BD_ADDR_LEN);
    solica_addr.addr_type = addr_type;

    applet_solicatation_found_t params_solic_found = {
        .addr = solica_addr,
        #if (BT_53)
        .use_subrating = false,
        #endif //(BT_53)
    };

    app_le_connection_state_connection(EVENT_CREATE, status, &params_solic_found);

    if (status)
    {
    	APP_DBG("le_bis_assis_start_conn failed; status 0x%x\n", status);
    }
}

__STATIC void le_bis_assis_cb_timeout(void)
{
}

__STATIC void le_bis_assis_cb_svc_changed(uint8_t conidx)
{
}

// Set of callback functions for communication with BAP Broadcast Assistant module
__STATIC const bap_bc_assist_cb_t cbs_bap_bc_assist =
{
    .cb_cmp_evt = le_bis_assis_cb_cmp_evt,
    .cb_state = le_bis_assis_cb_state,
    .cb_state_subgroup = le_bis_assis_cb_state_subgroup,
    .cb_bcast_code_req = le_bis_assis_cb_bcast_code_req,
    .cb_bond_data = le_bis_assis_cb_bond_data,
    .cb_solicitation = le_bis_assis_cb_solicitation,
    .cb_timeout = le_bis_assis_cb_timeout,
    .cb_svc_changed = le_bis_assis_cb_svc_changed,
};

/*
 * FUNCTIONS
 ****************************************************************************************
 */

__STATIC void bis_assis_configuration_cb_gapm_bdaddr(uint32_t event, uint16_t status, const gap_addr_t* p_addr)
{
	bis_assis_configuration_state(event, status, p_addr);
}


void bis_assis_configuration_cb_gapm_cmp(uint32_t event, uint16_t status)
{
	bis_assis_configuration_state(event, status, NULL);
}

void bis_assis_configuration_state(uint16_t event, uint16_t status, const void* p_params)
{
    bool is_finished = false;

	switch (event)
	{

		case EVENT_CONFIGURATION_DONE:
		{
			APP_CHECK_STATUS(gapm_set_name(EVENT_CONFIGURATION_NAME_SET, strlen(lle_bis_assis_broadcast_name), (const uint8_t*)lle_bis_assis_broadcast_name,
							 bis_assis_configuration_cb_gapm_cmp));
		} break;

		case EVENT_CONFIGURATION_NAME_SET:
		{
			APP_CHECK_STATUS(gapm_get_controller_bdaddr(EVENT_CONFIGURE_READ_CONTROLLER_ADDRESS,
														bis_assis_configuration_cb_gapm_bdaddr));
		} break;

		case EVENT_CONFIGURE_READ_CONTROLLER_ADDRESS:
		{
			is_finished = true;
		} break;

		default: { } break;
	} while (false);


    if (is_finished )
    {
        //broadcast assistant scan solicate
        uint16_t status = bap_bc_assist_start_scan(0);

        if (status != GAF_ERR_NO_ERROR)
        {
            APP_DBG("bap_bc_assist_start_scan status: %d\n",status);
        }
    }
}

void le_bis_assis_start_scan(void)
{
    //broadcast assistant scan solicate
    uint16_t status = bap_bc_assist_start_scan(0);

    if (status != GAF_ERR_NO_ERROR)
    {
        APP_DBG("bap_bc_assist_start_scan status: %d\n",status);
    }
}

__STATIC void le_bis_assistant_configure(const void* p_params, component_cb_event cb_event)
{
    // Device appearance
    uint16_t APPEARANCE = 0x00;
    uint16_t status;

    bap_bc_assist_cfg_t cfg =  {
        .cfg_bf = 0,
        .pref_mtu = 0,
    };

    ASSERT_ERR(p_env == NULL);

    p_env = (le_bis_assis_env_t*)ke_malloc_user(sizeof(le_bis_assis_env_t),
                                                              KE_MEM_PROFILE);
    ASSERT_ERR(p_env != NULL);
    memset(p_env, 0, sizeof(le_bis_assis_env_t));
    memset(&p_env->src_info, 0, sizeof(sink_add_src_t));
    memset(&(p_env->src_info.adv_id), 0, sizeof(bap_adv_id_t));
    memset(&(p_env->src_info.bcast_id), 0, sizeof(bap_bcast_id_t));

    p_env->src_info.src_idx = GAF_INVALID_LID;
    p_env->conidx = GAP_INVALID_CONIDX;
    p_env->src_lid = GAP_INVALID_CONIDX;
    p_env->target_src_idx = GAP_INVALID_CONIDX;
    p_env->cb_mediator_event = cb_event;

    status = bap_bc_assist_configure(&cbs_bap_bc_assist, &cfg);
    if (status)
    {
        APP_DBG("bap_bc_assist_configure failed: status 0x%x\n", status);
    }

    status = bap_bc_scan_configure(BAP_ROLE_SUPP_BC_ASSIST_BIT | BAP_ROLE_SUPP_BC_SCAN_BIT | BAP_ROLE_SUPP_BC_SRC_BIT, 
                                    &cbs_bap_bc_scan);
    if (status)
    {
        APP_DBG("bap_bc_scan_configure failed: status 0x%x\n", status);
    }

    gapm_le_set_appearance(&APPEARANCE);
}

__STATIC void le_bis_audio_cb_event_broadcast_assistant(uint16_t type, uint16_t status,
                                                                     const void* p_params)
{
    switch (type)
    {
        case COMPONENT_EVENT_SERVICE_CONTENT_DISCOVERED:
        {
#ifdef LE_CIS_SRC
            app_le_audio_discovery(EVENT_SERVICE_DISCOVERY_CONTENT_DISCOVERED,
                                                                   *((uint8_t*)p_params), status, NULL);
#else
            app_le_audio_discovery(EVENT_SERVICE_DISCOVERY_COMPLETED, *((uint8_t*)p_params), status, NULL);
#endif
        } break;

        default: { } break;
    }
}


void reset_src_array(void)
{
    for (int i = 0;  i < MAX_SRC_NUM; i++)
    {
        memset(&(src_array[i]), 0, sizeof(sink_add_src_t));
        memset(&(src_array[i].bcast_id), 0, sizeof(bap_bcast_id_t));
        memset(&(src_array[i].adv_id), 0, sizeof(bap_adv_id_t));
        src_array[i].src_idx = GAF_INVALID_LID;
    }
}

uint16_t le_bis_assis_init(void)
{
    le_bis_assistant_configure(NULL, le_bis_audio_cb_event_broadcast_assistant);

    reset_src_array();
#ifndef LE_UDSC
    uint16_t status = app_gaf_client_configure();
    if (status)
    {
        APP_DBG("le_bis_assis_init: app_gaf_client_configure err 0x%x\n", status);
    }
#endif
    return status;
}

#endif
#endif