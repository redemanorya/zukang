/**
 ****************************************************************************************
 *
 * @file le_cis_src.c
 *
 * @brief Applet - Broadcast Assistant
 *
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "le_audio_config.h"
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
#include <string.h>
#include "le_cis_src.h"
#include "hl_error.h"
#include "gapc_sec.h"
#include "gapm_le_init.h"
#include "le_audio_config.h"
#include "co_bt_defines.h"
#include "debug.h"
#include "gapm.h"
#include "gapc.h"
#include "bap.h"
#include "bap_uc.h"
#include "bap_uc_cli.h"
#include "bap_uc_cfg.h"
#include "co_djob.h"
#include "arch.h"
#include "le_cis_src.h"
#include "utils_audio.h"
#include "app_le_scanning.h"
#include "app_le_volume_controller.h"
#include "app_le_audio_capabilities_client.h"
#include "app_le_media_server.h"
#include "app_le_call_server.h"
#include "app_le_coordinated_set_coordinator.h"
#include "app_le_telephony_media_client.h"
#include "app_le_common_audio_client.h"
#include "bt_app_ddb_info.h"
#include "le_cis_src.h"
#include "co_utils.h"
#include "gapm_le_init.h"

extern uint8_t cis_src2sink_streaming;//ase0
extern uint8_t cis_sink2src_streaming;//ase1
extern bool app_is_paired(uint8_t conidx);

#ifdef LE_CIS_SRC

#ifdef CIS_APP_HEADSET_LR
#define CODEC_CFG               "48_2_1"
#else
/// Default codec configuration
#ifndef LE_CIS_CONVERSATION
#define CODEC_CFG               "48_v_1"//"48_2_1"
#else
#define CODEC_CFG               "16_2_1"
#endif
#endif


// First CIS ID
#define STREAM_CIS_ID                      (1U)
// CIS PHY
#define STREAM_PHY                         (GAPM_PHY_TYPE_LE_2M)
// Maximum number of streams that may be handled
//    - Media
//        -> Up to 2 streams for transmission of stereo
//    - Call
//        -> Up to 2 streams for transmission of stereo
//        -> One of these streams also used for reception
//    - Ringtone
//        -> 1 stream for transmission of mono
#define STREAM_MAX_NB                      (5U)
// Maximum number of streams that may be part of a group
#define GROUP_STREAM_MAX_NB                (2U)
// Maximum number of Sink ASEs for a stream
#define STREAM_SINK_ASE_MAX_NB             (2U)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */
// Event values for main state machine
enum app_le_le_connection_event
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

// Group state values
enum le_cis_src_group_state
{
    // Free
    GROUP_STATE_FREE = 0U,
    // Added
    GROUP_STATE_ADDED,
    // Created
    GROUP_STATE_CREATED,
    // QoS Configured
    GROUP_STATE_QOS_CONFIGURED,
};

// Group status bit field meaning
enum le_cis_src_group_status_bf
{
    // Does group support stereo ? - Position
    GROUP_STATUS_STEREO_POS = 0U,
    // Does group support stereo ? - Bit
    GROUP_STATUS_STEREO_BIT = CO_BIT(GROUP_STATUS_STEREO_POS),

    // Indicate if stop has been required - Position
    GROUP_STATUS_STOP_POS = 1U,
    // Indicate if stop has been required - Bit
    GROUP_STATUS_STOP_BIT = CO_BIT(GROUP_STATUS_STOP_POS),

    // Indicate if start has been required - Position
    GROUP_STATUS_START_POS = 2U,
    // Indicate if stop has been required - Bit
    GROUP_STATUS_START_BIT = CO_BIT(GROUP_STATUS_START_POS),

    // Stream state - Position
    GROUP_STATUS_STATE_LSB = 4U,
    // Stream state - Mask
    GROUP_STATUS_STATE_MASK = 0x30,
};

// Event values for Service Discovery state machine
enum le_cis_src_event_service_discovery
{
    // Request to start
    EVENT_SERVICE_DISCOVERY_START = 0U,
    // Content of Audio Stream Control Service has been discovered
    EVENT_SERVICE_DISCOVERY_CONTENT,
    // General service discovery has been completed
    EVENT_SERVICE_DISCOVERY_GENERAL_END,
    // Content of a service has been discovered
    EVENT_SERVICE_DISCOVERY_CONTENT_DISCOVERED,
    // LE Audio Discovery has been completed
    EVENT_SERVICE_DISCOVERY_COMPLETED,
};

// Event values for Group state machine
enum le_cis_src_event_group
{
    // Codec has been configured
    EVENT_GROUP_CODEC_CONFIGURED = 0U,
    // QoS has been configured
    EVENT_GROUP_QOS_CONFIGURED,
    // Stream has been started
    EVENT_GROUP_STARTED,
    // Stream has been stopped
    EVENT_GROUP_STOPPED,
};

/// Common event type values
enum component_event_type
{
    /// Component has been started
    COMPONENT_EVENT_STARTED = 0U,
    /// Component has been stopped
    COMPONENT_EVENT_STOPPED,
    /// Connection has been established/n
    /// See #component_event_connected_t
    COMPONENT_EVENT_CONNECTED,
    /// Disconnection has occurred
    /// See #component_event_disconnected_t
    COMPONENT_EVENT_DISCONNECTED,
    /// Pairing has been completed
    /// See #component_event_paired_t
    COMPONENT_EVENT_PAIRED,
    /// Encryption has been started
    /// See #component_event_encryption_started_t
    COMPONENT_EVENT_ENCRYPTION_STARTED,
    /// A device has been discovered
    COMPONENT_EVENT_DEVICE_FOUND,
    /// A service has been discovered
    /// See #component_event_service_discovered_t
    COMPONENT_EVENT_SERVICE_DISCOVERED,
    /// Content of service has been discovered
    ///     Type: uint8_t conidx
    COMPONENT_EVENT_SERVICE_CONTENT_DISCOVERED,
    /// Resolvable Set Identifier (RSI) value has been update
    ///     - Type: uint8_t*
    COMPONENT_EVENT_RSI_UPDATED,

    /// First available event type value for child Components
    COMPONENT_EVENT_FIRST,
};

// Device local index
enum app_device_lid
{
    // Primary
    APP_DEVICE_LID_PRIMARY = 0,
    // Secondary
    APP_DEVICE_LID_SECONDARY,

    APP_DEVICE_LID_MAX,
};
/*
 * TYPES DEFINITION
 ****************************************************************************************
 */

// Stream configuration structure
typedef struct le_cis_src_stream_cfg
{
    // Stream configuration name
    const char* p_cfg_name;
    // Configuration
    bap_uc_cfg_t cfg;
} le_cis_src_stream_cfg_t;

/// Audio configuration structure
typedef struct le_cis_src_audio_cfg
{
    // Configuration
    bap_uc_cfg_t cfg;
    /// Data Path ID
    uint8_t dp_id;
    /// Codec ID
    gaf_codec_id_t codec_id;
} le_cis_src_audio_cfg_t;

// ASE structure
typedef struct le_cis_src_ase
{
    // Connection index
    uint8_t conidx;
    // ASE local index
    uint8_t ase_lid;
    // ASE instance index
    uint8_t ase_instance_idx;
    // CIS ID
    uint8_t cis_id;
    // Codec Configuration
    bap_cfg_t codec_cfg;
} le_cis_src_ase_t;

// Group structure
typedef struct le_cis_src_group
{
    // Delayed Job structure
    co_djob_t djob;
    // Configuration for Sink direction
    le_cis_src_audio_cfg_t cfg_sink[STREAM_SINK_ASE_MAX_NB];
    // Configuration for Source direction
    le_cis_src_audio_cfg_t cfg_src;
    // Type (see #app_stream_type enumeration)
    uint8_t type;
    // Group local index
    uint8_t group_lid;
    // Status bit field
    uint8_t status_bf;
    // ASE information for Sink direction - 2 in order to support stereo
    le_cis_src_ase_t ase_sink[STREAM_SINK_ASE_MAX_NB];
    // ASE information for Source direction
    le_cis_src_ase_t ase_src;
    // Bit field allowing to know if a procedure has been completed for a group
    uint8_t ase_done_bf;
    // Bit field allowing to mask ase_done_bf;
    uint8_t ase_done_mask;
    // ASE counter
    uint8_t ase_cnt;
    // Event for which delayed job has been programmed
    uint8_t djob_event;
} le_cis_src_group_t;

// Environment structure
typedef struct le_cis_src_env
{
    // Group structures
    le_cis_src_group_t group[APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_MAX];

    // Connection handle
    uint16_t conhdl;

    // Group type for each ASE
    uint8_t type_ase_table[STREAM_MAX_NB];
    // Next ASE local index
    uint8_t next_ase_lid;

    // ASE for which data path provider has been started
    uint8_t ase_lid_dp;
    
    uint8_t actv_idx;
    // Event callback function for backward communication with Mediator
    component_cb_event cb_mediator_event;
    // Bond data
    app_bond_data_t bond_data[APP_DEVICE_LID_MAX];

} le_cis_src_env_t;


/*
 * LOCAL VARIABLES
 ****************************************************************************************
 */

// Pointer to environment
__STATIC le_cis_src_env_t* p_env;
__STATIC uint8_t g_scene = 0xff;

// Stream configuration table
__STATIC const le_cis_src_stream_cfg_t le_cis_src_stream_cfg[] =
{
    // Latency
    { "8_1_1", BAP_UC_CFG_8_1_1 },
    { "8_2_1", BAP_UC_CFG_8_2_1 },
    { "16_1_1", BAP_UC_CFG_16_1_1 },
    { "16_2_1", BAP_UC_CFG_16_2_1 },
    { "24_1_1", BAP_UC_CFG_24_1_1 },
    { "24_2_1", BAP_UC_CFG_24_2_1 },
    { "32_1_1", BAP_UC_CFG_32_1_1 },
    { "32_2_1", BAP_UC_CFG_32_2_1 },
    { "441_1_1", BAP_UC_CFG_441_1_1 },
    { "441_2_1", BAP_UC_CFG_441_2_1 },
    { "48_1_1", BAP_UC_CFG_48_1_1 },
    { "48_2_1", BAP_UC_CFG_48_2_1 },
    { "48_v_1", BAP_UC_CFG_48_v_1 },
    { "48_3_1", BAP_UC_CFG_48_3_1 },
    { "48_4_1", BAP_UC_CFG_48_4_1 },
    { "48_5_1", BAP_UC_CFG_48_5_1 },
    { "48_6_1", BAP_UC_CFG_48_6_1 },

    // Reliability
    { "8_1_2", BAP_UC_CFG_8_1_2 },
    { "8_2_2", BAP_UC_CFG_8_2_2 },
    { "16_1_2", BAP_UC_CFG_16_1_2 },
    { "16_2_2", BAP_UC_CFG_16_2_2 },
    { "24_1_2", BAP_UC_CFG_24_1_2 },
    { "24_2_2", BAP_UC_CFG_24_2_2 },
    { "32_1_2", BAP_UC_CFG_32_1_2 },
    { "32_2_2", BAP_UC_CFG_32_2_2 },
    { "441_1_2", BAP_UC_CFG_441_1_2 },
    { "441_2_2", BAP_UC_CFG_441_2_2 },
    { "48_1_2", BAP_UC_CFG_48_1_2 },
    { "48_2_2", BAP_UC_CFG_48_2_2 },
    { "48_3_2", BAP_UC_CFG_48_3_2 },
    { "48_4_2", BAP_UC_CFG_48_4_2 },
    { "48_5_2", BAP_UC_CFG_48_5_2 },
    { "48_6_2", BAP_UC_CFG_48_6_2 },
};

void le_cis_src_cfg_lc3(uint8_t conidx);
/*
 * TYPES DEFINITION
 ****************************************************************************************
 */

typedef struct app_control_start_discovery
{
    /// Connection index
    uint8_t conidx;
    /// Pointer to service information\n
    /// May be NULL
    const prf_svc_t* p_svc;
} app_control_start_discovery_t;


// Pointer to environment
__STATIC le_cis_src_env_t* p_env;

/// Broadcast name
__STATIC const char le_cis_src_name[] = "MV_CIS_SRC";

extern uint16_t app_gaf_client_configure(void);
extern void app_le_audio_discovery(uint8_t event, uint8_t conidx, uint16_t status, const void* p_params);
#ifdef LE_UDSC
extern void app_le_start_conn(uint16_t status, const void* p_params);
#endif
/*
 * FUNCTIONS
 ****************************************************************************************
 */

__STATIC void cis_src_configuration_cb_gapm_bdaddr(uint32_t event, uint16_t status, const gap_addr_t* p_addr)
{
	cis_src_configuration_state(event, status, p_addr);
}

void cis_src_configuration_cb_gapm_cmp(uint32_t event, uint16_t status)
{
	cis_src_configuration_state(event, status, NULL);
}

void cis_src_configuration_state(uint16_t event, uint16_t status, const void* p_params)
{
    bool is_finished = false;

	switch (event)
	{

		case EVENT_CONFIGURATION_DONE:
		{
			APP_CHECK_STATUS(gapm_set_name(EVENT_CONFIGURATION_NAME_SET, strlen(le_cis_src_name), (const uint8_t*)le_cis_src_name,
							 cis_src_configuration_cb_gapm_cmp));
		} break;

		case EVENT_CONFIGURATION_NAME_SET:
		{
			APP_CHECK_STATUS(gapm_get_controller_bdaddr(EVENT_CONFIGURE_READ_CONTROLLER_ADDRESS,
														cis_src_configuration_cb_gapm_bdaddr));
		} break;

		case EVENT_CONFIGURE_READ_CONTROLLER_ADDRESS:
		{
			is_finished = true;
		} break;

		default: { } break;
	} while (false);


    if (is_finished )
    {
// #ifdef LE_UDSC
//         app_le_udsc_start_scan();
// #endif
    }
}

__STATIC void app_le_le_connection_cb_gapm_le_init_proc_cmp(uint32_t token, uint8_t proc_id, uint8_t actv_idx,
                                                            uint16_t status)
{
    APP_DBG("app_le_le_connection_cb_gapm_le_init_proc_cmp: proc_id 0x%X, status 0x%x\n", proc_id, status);

    switch (proc_id)
    {
        case GAPM_ACTV_START:
        {
            // app_le_le_connection_state_connection(EVENT_STARTED, status, NULL);
        } break;

        case GAPM_ACTV_DELETE:
        {
            // app_le_le_connection_state_connection(EVENT_DELETED, status, NULL);
        } break;

        default: { } break;
    }
}

__STATIC void app_le_le_connection_cb_gapm_le_init_stopped(uint32_t token, uint8_t actv_idx, uint16_t reason)
{
    APP_DBG("app_le_le_connection_cb_gapm_le_init_stopped: actv_idx 0x%x, reason 0x%x\n", actv_idx, reason);
    // app_le_le_connection_state_connection(EVENT_STOPPED, GAP_ERR_NO_ERROR, NULL);

}

__STATIC const gapm_le_init_cb_actv_t cbs_gapm_le_init =
{
    .hdr.actv.stopped = app_le_le_connection_cb_gapm_le_init_stopped,
    .hdr.actv.proc_cmp = app_le_le_connection_cb_gapm_le_init_proc_cmp,
    .hdr.addr_updated = NULL,
    .peer_name = NULL,
};

void app_le_start_connect(app_le_gaf_scanning_device_found_t* p_params)
{

    uint8_t ret = 0;

    app_le_gaf_scanning_device_found_t* p_connect_params = (app_le_gaf_scanning_device_found_t*)p_params;
    gapm_le_init_param_t init_params;
    uint16_t conn_intv, ce_len_min, ce_len_max;

    ASSERT_ERR(p_params != NULL);

    init_params.prop = GAPM_INIT_PROP_1M_BIT;
    init_params.conn_to = 0;
    APP_DBG("app_le_start_connect: addr 0x%2x:0x%2x:0x%2x:0x%2x:0x%2x:0x%2x\n", 
                p_connect_params->p_addr->addr[0], p_connect_params->p_addr->addr[1], p_connect_params->p_addr->addr[2], 
                p_connect_params->p_addr->addr[3], p_connect_params->p_addr->addr[4], p_connect_params->p_addr->addr[5]);

    memcpy(&(init_params.peer_addr), &(p_connect_params->p_addr), sizeof(gap_bdaddr_t));

    uint8_t target[6] = {0x56, 0x83, 0x13, 0x22, 0xe8, 0xf6};
    memcpy(&init_params.peer_addr.addr, target, 6);
    init_params.peer_addr.addr_type = GAP_ADDR_PUBLIC;
    
    init_params.scan_param_1m.scan_intv = 160;
    init_params.scan_param_1m.scan_wd = 80;
    init_params.conn_param_1m.supervision_to = 100;
    init_params.conn_param_2m.supervision_to = 100;
    init_params.conn_param_coded.supervision_to = 100;

	conn_intv = 48;
	ce_len_min = 5;
	ce_len_max = 10;

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


    ret = gapm_le_create_init(0, GAPM_STATIC_ADDR, &cbs_gapm_le_init,
                                                &(p_env->actv_idx));
    if (ret)
    {
        APP_DBG("gapm_le_create_init: error 0x%x\n", ret);
    }

    ret = gapm_le_start_direct_connection(p_env->actv_idx, &init_params);
    if (ret)
    {
        APP_DBG("gapm_le_start_direct_connection: error 0x%x\n", ret);
    }

}

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

__STATIC const bap_uc_cfg_t* le_cis_src_find_stream_configuration(const char* p_cfg_name)
{
    const bap_uc_cfg_t* p_cfg = NULL;
    uint8_t cnt;

    for (cnt = 0;
         cnt < (sizeof(le_cis_src_stream_cfg) / sizeof(le_cis_src_stream_cfg_t));
         cnt++)
    {
        if (strcmp(p_cfg_name, le_cis_src_stream_cfg[cnt].p_cfg_name) == 0)
        {
            p_cfg = &(le_cis_src_stream_cfg[cnt].cfg);
            break;
        }
    }

    ASSERT_ERR(p_cfg != NULL);
    return (p_cfg);
}

__STATIC void le_cis_src_set_configuration_lc3(const le_cis_src_configure_t* p_params)
{
    le_cis_src_group_t* p_group;
    const bap_uc_cfg_t* p_cfg;

    p_group = &(p_env->group[p_params->type]);

    p_cfg = le_cis_src_find_stream_configuration(p_params->cfg_name);

    if (p_cfg != NULL)
    {
        gaf_codec_id_t codec_id = GAF_CODEC_ID_LC3;

        if (p_params->sink)
        {
            memcpy(&(p_group->cfg_sink[0].cfg), p_cfg, sizeof(bap_uc_cfg_t));
            p_group->cfg_sink[0].dp_id = p_params->dp_id;
            memcpy(&(p_group->cfg_sink[0].codec_id), &codec_id, sizeof(gaf_codec_id_t));

            if (p_params->dp_id_1 != 0xff)
            {
                memcpy(&(p_group->cfg_sink[1].cfg), p_cfg, sizeof(bap_uc_cfg_t));
                p_group->cfg_sink[1].dp_id = p_params->dp_id_1;
                memcpy(&(p_group->cfg_sink[1].codec_id), &codec_id, sizeof(gaf_codec_id_t));
            }
        }
        else if (p_params->type == APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_CALL)
        {
            memcpy(&(p_group->cfg_src.cfg), p_cfg, sizeof(bap_uc_cfg_t));
            p_group->cfg_src.dp_id = p_params->dp_id;
            memcpy(&(p_group->cfg_src.codec_id), &codec_id, sizeof(gaf_codec_id_t));
        }
    }
}

__STATIC void le_cis_src_create_group(le_cis_src_group_t* p_group)
{
    bap_uc_cli_grp_param_t grp_param =
    {
        .sdu_intv_m2s_us = p_group->cfg_sink[0].cfg.sdu_intv_us,
        .sdu_intv_s2m_us = p_group->cfg_sink[0].cfg.sdu_intv_us,
        .packing = 1,//0,
        .framing = p_group->cfg_sink[0].cfg.framing,
        .sca = 0,
        .tlatency_m2s_ms = p_group->cfg_sink[0].cfg.max_tlatency_ms,
        .tlatency_s2m_ms = p_group->cfg_sink[0].cfg.max_tlatency_ms,
    };

    APP_DBG("Create group %d\n", p_group->type);

    uint16_t status = bap_uc_cli_create_group(1 + p_group->type, &grp_param, &(p_group->group_lid));
    if (status)
    {
        APP_DBG("bap_uc_cli_create_group err 0x%x\n", status);
    }
}

__STATIC void le_cis_src_remove_group(le_cis_src_group_t* p_group)
{
    uint16_t status;
    APP_DBG("Remove group %d\n", p_group->type);

    status = bap_uc_cli_remove_group(p_group->group_lid);
    if (status)
    {
        APP_DBG("le_cis_src_remove_group: bap_uc_cli_remove_group: err 0x%x\n", status);
        return;
    }

    p_group->group_lid = GAF_INVALID_LID;
    SETF(p_group->status_bf, GROUP_STATUS_STATE, GROUP_STATE_ADDED);
}

__STATIC void le_cis_src_configure_codec(le_cis_src_group_t* p_group)
{
    uint8_t tgt_latency = (p_group->type == APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_MEDIA)
                                            ? BAP_UC_TGT_LATENCY_RELIABLE : BAP_UC_TGT_LATENCY_LOWER;
    uint8_t tgt_phy = STREAM_PHY;
    gaf_codec_id_t* p_codec_id;
    uint8_t ase_lid, cnt;
    uint16_t status;

    p_group->ase_done_bf = 0;
    p_group->ase_done_mask = 0;

    // Sink direction
    for (cnt = 0; cnt < STREAM_SINK_ASE_MAX_NB; cnt++)
    {
        le_cis_src_ase_t* p_ase_sink = &(p_group->ase_sink[cnt]);
        uint8_t dp_id;

        ase_lid = p_ase_sink->ase_lid;
        if (ase_lid == GAF_INVALID_LID)
        {
            continue;
        }

        dp_id = p_group->cfg_sink[cnt].dp_id;

        p_codec_id = &(p_group->cfg_sink[cnt].codec_id);
        p_ase_sink->codec_cfg.param.frame_octet = p_group->cfg_sink[cnt].cfg.max_sdu_size;
        p_ase_sink->codec_cfg.param.sampling_freq = p_group->cfg_sink[cnt].cfg.samp_freq;
        p_ase_sink->codec_cfg.param.frame_dur = (p_group->cfg_sink[cnt].cfg.sdu_intv_us == 7500)
                                                                        ? BAP_FRAME_DUR_7_5MS : BAP_FRAME_DUR_10MS;
        p_ase_sink->codec_cfg.param.frames_sdu = 1;
        p_ase_sink->codec_cfg.param.location_bf = (cnt == 0) ? GAF_LOC_FRONT_LEFT_BIT : GAF_LOC_FRONT_RIGHT_BIT;
        p_ase_sink->codec_cfg.add_cfg.len = 0;

        APP_DBG("Configure codec (Sink) - ASE instance = %d, dp_id 0x%x\n", p_group->ase_sink[cnt].ase_instance_idx, dp_id);
        APP_DBG("Sampling freq = %d / Octets = %d / Duration = %d\n", p_ase_sink->codec_cfg.param.sampling_freq,
                     p_ase_sink->codec_cfg.param.frame_octet, p_ase_sink->codec_cfg.param.frame_dur);
        status = bap_uc_cli_configure_codec(p_group->ase_sink[cnt].conidx,
                                                          p_group->ase_sink[cnt].ase_instance_idx, ase_lid,
                                                          dp_id, p_codec_id, tgt_latency, tgt_phy, 0,
                                                          &(p_ase_sink->codec_cfg));
        if (status)
        {
            APP_DBG("bap_uc_cli_configure_codec: sink ase err 0x%x\n", status);
            return;
        }
        p_group->ase_done_mask |= (1 << ase_lid);

        #if (APP_CTRL)
        if (p_group->ase_sink[cnt].ase_instance_idx == 0)
        {
            p_env->ase_lid_dp = ase_lid;
        }
        #endif //(APP_CTRL)
    }

    // Source direction
    ase_lid = p_group->ase_src.ase_lid;
    if (ase_lid != GAF_INVALID_LID)
    {
        do
        {
            le_cis_src_ase_t* p_ase_src = &(p_group->ase_src);

            p_codec_id = &(p_group->cfg_src.codec_id);
            p_ase_src->codec_cfg.param.frame_octet = p_group->cfg_src.cfg.max_sdu_size;
            p_ase_src->codec_cfg.param.sampling_freq = p_group->cfg_src.cfg.samp_freq;
            p_ase_src->codec_cfg.param.frame_dur = (p_group->cfg_src.cfg.sdu_intv_us == 7500)
                                                                        ? BAP_FRAME_DUR_7_5MS : BAP_FRAME_DUR_10MS;
            p_ase_src->codec_cfg.param.frames_sdu = 1;
            p_ase_src->codec_cfg.param.location_bf = GAF_LOC_FRONT_LEFT_BIT;
            p_ase_src->codec_cfg.add_cfg.len = 0;

            APP_DBG("Configure codec (Source) - ASE instance = %d\n", p_group->ase_src.ase_instance_idx);
            APP_DBG("Sampling freq = %d / Octets = %d / Duration = %d, sdu_intv_us %d\n",
                         p_ase_src->codec_cfg.param.sampling_freq,
                         p_ase_src->codec_cfg.param.frame_octet, p_ase_src->codec_cfg.param.frame_dur, p_group->cfg_src.cfg.sdu_intv_us);

            status = bap_uc_cli_configure_codec(p_group->ase_src.conidx,
                                                              p_group->ase_src.ase_instance_idx,
                                                              ase_lid, p_group->cfg_src.dp_id, p_codec_id,
                                                              tgt_latency, tgt_phy, 0, &(p_ase_src->codec_cfg));
            if (status)
            {
                APP_DBG("bap_uc_cli_configure_codec: src ase err 0x%x\n", status);
                return;
            }
                                                
            p_group->ase_done_mask |= (1 << ase_lid);
        } while (false);
    }
}

__STATIC void le_cis_src_configure_qos(le_cis_src_group_t* p_group)
{
    uint16_t status;
    le_cis_src_ase_t* p_ase;
    bap_uc_cli_qos_cfg_t qos_cfg = {
        .phy = STREAM_PHY,
    };

    APP_DBG("le_cis_src_configure_qos: ase_cnt %d\n", p_group->ase_cnt);
    if (p_group->ase_cnt < STREAM_SINK_ASE_MAX_NB)
    {
        p_ase = &(p_group->ase_sink[p_group->ase_cnt]);
        qos_cfg.retx_nb = p_group->cfg_sink[0].cfg.rtn;
        qos_cfg.max_sdu_size = p_group->cfg_sink[0].cfg.max_sdu_size;
        qos_cfg.pres_delay_us = p_group->cfg_sink[0].cfg.pres_delay_us;
    }
    else
    {
        p_ase = &(p_group->ase_src);
        qos_cfg.retx_nb = p_group->cfg_src.cfg.rtn;
        qos_cfg.max_sdu_size = p_group->cfg_src.cfg.max_sdu_size;
        qos_cfg.pres_delay_us = p_group->cfg_src.cfg.pres_delay_us;
    }

    APP_DBG("Configure QoS - %d\n", p_ase->ase_lid);
    status = bap_uc_cli_configure_qos(p_ase->ase_lid, p_group->group_lid, p_ase->cis_id, &qos_cfg);
    if (status)
    {
        APP_DBG("bap_uc_cli_configure_qos err 0x%x\n", status);
        return;
    }
    
    p_group->ase_cnt++;

    if (!CHECKB(p_group->status_bf, GROUP_STATUS_STEREO))
    {
        p_group->ase_cnt++;
    }
}

__STATIC void le_cis_src_enable(le_cis_src_group_t* p_group)
{
    bap_cfg_metadata_t cfg_metadata;
    uint8_t ase_lid, cnt;
	uint16_t status;
	 
    p_group->ase_done_bf = 0;
    p_group->ase_done_mask = 0;

    cfg_metadata.param.context_bf = BAP_CONTEXT_TYPE_UNSPECIFIED_BIT;
    cfg_metadata.add_metadata.len = 0;

    // Sink direction
    for (cnt = 0; cnt < STREAM_SINK_ASE_MAX_NB; cnt++)
    {
        ase_lid = p_group->ase_sink[cnt].ase_lid;
        if (ase_lid == GAF_INVALID_LID)
        {
            continue;
        }

        APP_DBG("Enable (Sink) - %d\n", ase_lid);
        status = bap_uc_cli_enable(ase_lid, &cfg_metadata);
        if (status)
        {
            APP_DBG("bap_uc_cli_enable: sink ase err 0x%x\n", status);
            return;
        }
        p_group->ase_done_mask |= (1 << ase_lid);
    }

    // Source direction
    ase_lid = p_group->ase_src.ase_lid;
    if (ase_lid != GAF_INVALID_LID)
    {
        APP_DBG("Enable (Source) - %d\n", ase_lid);
        status = bap_uc_cli_enable(ase_lid, &cfg_metadata);
        if (status)
        {
            APP_DBG("bap_uc_cli_enable: src ase err 0x%x\n", status);
            return;
        }
        p_group->ase_done_mask |= (1 << ase_lid);
    }
}

__STATIC void le_cis_src_disable(le_cis_src_group_t* p_group)
{
    uint8_t ase_lid, cnt;
	uint16_t status;
    p_group->ase_done_bf = 0;
    p_group->ase_done_mask = 0;

    // Sink direction
    for (cnt = 0; cnt < STREAM_SINK_ASE_MAX_NB; cnt++)
    {
        ase_lid = p_group->ase_sink[cnt].ase_lid;
        if (ase_lid == GAF_INVALID_LID)
        {
            continue;
        }

        APP_DBG("Disable (Sink) - %d\n", ase_lid);
        status = bap_uc_cli_disable(ase_lid);
        if (status)
        {
            APP_DBG("bap_uc_cli_disable: sink ase err 0x%x\n", status);
            return;
        }
        p_group->ase_done_mask |= (1 << ase_lid);
    }

    // Source direction
    ase_lid = p_group->ase_src.ase_lid;
    if (ase_lid != GAF_INVALID_LID)
    {
        APP_DBG("Disable (Source) - %d\n", ase_lid);
        status = bap_uc_cli_disable(ase_lid);
        if (status)
        {
            APP_DBG("bap_uc_cli_disable: src ase err 0x%x\n", status);
            return;
        }
        p_group->ase_done_mask |= (1 << ase_lid);
    }
}

__STATIC void le_cis_src_release(le_cis_src_group_t* p_group)
{
    uint8_t ase_lid, cnt;
	uint16_t status;
	
    p_group->ase_done_bf = 0;
    p_group->ase_done_mask = 0;

    // Sink direction
    for (cnt = 0; cnt < STREAM_SINK_ASE_MAX_NB; cnt++)
    {
        ase_lid = p_group->ase_sink[cnt].ase_lid;
        if (ase_lid == GAF_INVALID_LID)
        {
            continue;
        }

        APP_DBG("Release (Sink) - %d\n", ase_lid);
        status = bap_uc_cli_release(ase_lid);
        if (status)
        {
            APP_DBG("bap_uc_cli_release: Sink ase err 0x%x\n", status);
            return;
        } 
        p_group->ase_done_mask |= (1 << ase_lid);
    }

    // Source direction
    ase_lid = p_group->ase_src.ase_lid;
    if (ase_lid != GAF_INVALID_LID)
    {
        APP_DBG("Release (Source) - %d\n", ase_lid);
        status = bap_uc_cli_release(ase_lid);
        if (status)
        {
            APP_DBG("bap_uc_cli_release1: src ase err 0x%x\n", status);
            return;
        } 
        p_group->ase_done_mask |= (1 << ase_lid);
    }
}

/*
 * STATE MACHINES
 ****************************************************************************************
 */

void le_cis_src_state_service_discovery(uint8_t event, uint8_t conidx, uint16_t status,
                                                                  const prf_svc_t* p_svc)
{
    APP_DBG("le_cis_src_state_service_discovery: event 0x%x\n", event);
    switch (event)
    {
        case EVENT_SERVICE_DISCOVERY_START:
        {
            APP_DBG("Start ASCS discovery (conidx = %d)\n", conidx);

            if (p_svc != NULL)
            {
                status = bap_uc_cli_discover(conidx, p_svc->shdl, p_svc->ehdl);
                if (status)
                {
                    APP_DBG("bap_uc_cli_discover: err 0x%x\n", status);
                    return;
                } 
                else
                {
                    memcpy(&p_env->bond_data[conidx].svc_info, p_svc, sizeof(prf_svc_t));
                }
            }
            else
            {
                status = bap_uc_cli_discover(conidx, GATT_MIN_HDL, GATT_MAX_HDL);
                if (status)
                {
                    APP_DBG("bap_uc_cli_discover1: err 0x%x\n", status);
                    return;
                } 
            }
        } break;

        case EVENT_SERVICE_DISCOVERY_CONTENT:
        {
            if (status == GAF_ERR_NO_ERROR)
            {
                APP_DBG("ASCS discovered (conidx = %d)\n", conidx);
                p_env->cb_mediator_event(COMPONENT_EVENT_SERVICE_CONTENT_DISCOVERED, status, &conidx);
                // le_cis_src_start_audio();
            }
            else
            {
                APP_DBG("Issue during ASCS discovery: 0x%04X (conidx = %d)\n", status, conidx);
            }
        } break;

        default:
        {

        } break;
    }
}

__STATIC void le_cis_src_state_group(uint8_t event, le_cis_src_group_t* p_group,
                                                      uint16_t status)
{
    printf("le_cis_src_state_group: event 0x%x, status 0x%x\n", event, status);

    switch (event)
    {
        case EVENT_GROUP_CODEC_CONFIGURED:
        {
            if (p_group->group_lid != GAF_INVALID_LID)
            {
                if (p_group->ase_done_bf == p_group->ase_done_mask)
                {
                    p_group->ase_done_bf = 0;
                    p_group->ase_cnt = 0;
                    le_cis_src_configure_qos(p_group); 
                }
            }
        } break;

        case EVENT_GROUP_QOS_CONFIGURED:
        {
            if (GETF(p_group->status_bf, GROUP_STATUS_STATE) == GROUP_STATE_QOS_CONFIGURED)
            {
                if (CHECKB(p_group->status_bf, GROUP_STATUS_STOP)
                        && (p_group->ase_done_bf == p_group->ase_done_mask))
                {
                    SETB(p_group->status_bf, GROUP_STATUS_STOP, 0);
                    le_cis_src_release(p_group);
                }

                break;
            }

            if (p_group->ase_done_bf != p_group->ase_done_mask)
            {
                le_cis_src_configure_qos(p_group);
            }
            else
            {
                SETF(p_group->status_bf, GROUP_STATUS_STATE, GROUP_STATE_QOS_CONFIGURED);
                le_cis_src_enable(p_group);
            }
        } break;

        case EVENT_GROUP_STARTED:
        {
        } break;

        case EVENT_GROUP_STOPPED:
        {
            if (p_group->ase_done_bf == p_group->ase_done_mask)
            {
                le_cis_src_group_stopped_t params_stopped = {
                    .type = p_group->type,
                };

                le_cis_src_remove_group(p_group);

            //    p_env->cb_mediator_event(APP_LE_AUDIO_UNICAST_CLIENT_EVENT_GROUP_STOPPED, GAF_ERR_NO_ERROR,
            //                             &params_stopped);
            }
        } break;

        default:
        {

        } break;
    }
}

void load_ascs_src_info(uint8_t conidx, ascs_bond_data_t* p_ascs_bond_data)
{
    printf("load_ascs_src_info: 0x%x: 0x%x:0x%x, 0x%x:0x%x 0x%X\n", 
                sizeof(p_ascs_bond_data->svc_info), p_ascs_bond_data->svc_info.shdl, p_ascs_bond_data->svc_info.ehdl,
                p_ascs_bond_data->nb_ases_sink, p_ascs_bond_data->nb_ases_src, p_env->bond_data[conidx].p_ascs);

    memcpy(&p_env->bond_data[conidx].svc_info, &p_ascs_bond_data->svc_info, sizeof(p_ascs_bond_data->svc_info));

    uint8_t nb_chars = p_ascs_bond_data->nb_ases_sink + p_ascs_bond_data->nb_ases_src + BAP_UC_CHAR_TYPE_ASE;
    uint16_t size = sizeof(bap_uc_cli_ascs_t) + (nb_chars * sizeof(bap_uc_cli_ascs_char_t));
    p_env->bond_data[conidx].p_ascs = (bap_uc_cli_ascs_t*)ke_malloc_user(size, KE_MEM_PROFILE);

    memcpy(&(p_env->bond_data[conidx].p_ascs->char_info[0]), &(p_ascs_bond_data->char_info[0]), nb_chars * sizeof(bap_uc_cli_ascs_char_t));
    memcpy(&p_env->bond_data[conidx].p_ascs->svc_info, &p_ascs_bond_data->svc_info, sizeof(p_ascs_bond_data->svc_info));
    p_env->bond_data[conidx].p_ascs->nb_ases_sink = p_ascs_bond_data->nb_ases_sink;
    p_env->bond_data[conidx].p_ascs->nb_ases_src = p_ascs_bond_data->nb_ases_src;   
    // for (int i = 0; i < nb_chars; i++)
    // {
    //     printf("0x%x: 0x%x:0x%x:0x%x\n", i, p_env->bond_data[conidx].p_ascs->char_info[i].ase_id, 
    //             p_env->bond_data[conidx].p_ascs->char_info[i].desc_hdl, p_env->bond_data[conidx].p_ascs->char_info[i].val_hdl);
    // }

    // printf("load_ascs_src_info: conidx 0x%x, sink 0x%x, src 0x%x, shdl 0x%x, ehdl 0x%x, ascs 0x%x, 0x%x, nb_chars 0x%x\n", 
    //             conidx, p_env->bond_data[conidx].p_ascs->nb_ases_sink, 
    //             p_env->bond_data[conidx].p_ascs->nb_ases_src,
    //             p_env->bond_data[conidx].svc_info.shdl, p_env->bond_data[conidx].svc_info.ehdl,
    //             p_env->bond_data[conidx].p_ascs->svc_info.shdl, p_env->bond_data[conidx].p_ascs->svc_info.ehdl, nb_chars);   
}

app_bond_data_t* get_ascs_src_bond_data(uint8_t conidx)
{
    uint8_t nb_chars = p_env->bond_data[conidx].p_ascs->nb_ases_sink + p_env->bond_data[conidx].p_ascs->nb_ases_src + BAP_UC_CHAR_TYPE_ASE;
    // for (int i = 0; i < nb_chars; i++)
    // {
    //     printf("0x%x: 0x%x:0x%x:0x%x\n", i, p_env->bond_data[conidx].p_ascs->char_info[i].ase_id, 
    //             p_env->bond_data[conidx].p_ascs->char_info[i].desc_hdl, p_env->bond_data[conidx].p_ascs->char_info[i].val_hdl);
    // }

    // printf("get_ascs_src_bond_data: conidx 0x%x, sink 0x%x, src 0x%x, shdl 0x%x, ehdl 0x%x, ascs 0x%x, 0x%x, nb_chars 0x%x\n", 
    //             conidx, p_env->bond_data[conidx].p_ascs->nb_ases_sink, 
    //             p_env->bond_data[conidx].p_ascs->nb_ases_src,
    //             p_env->bond_data[conidx].svc_info.shdl, p_env->bond_data[conidx].svc_info.ehdl,
    //             p_env->bond_data[conidx].p_ascs->svc_info.shdl, p_env->bond_data[conidx].p_ascs->svc_info.ehdl, nb_chars);

    return &(p_env->bond_data[conidx]);
}

/*
 * CALLBACK FUNCTIONS
 ****************************************************************************************
 */

__STATIC void le_cis_src_cb_djob(co_djob_t* p_djob)
{
    le_cis_src_group_t* p_group = CO_CONTAINER_OF(p_djob, le_cis_src_group_t, djob);

    le_cis_src_state_group(p_group->djob_event, p_group, GAF_ERR_NO_ERROR);
}

__STATIC void le_cis_src_cb_cmp_evt(uint8_t cmd_type, uint16_t status, uint8_t conidx,
                                                     uint8_t param_1, uint8_t pac_lid)
{
    APP_DBG("le_cis_src_cb_cmp_evt: cmd_type 0x%x\n", cmd_type);
    ASSERT_WARN(status == GAF_ERR_NO_ERROR, status, cmd_type);

    switch (cmd_type)
    {
        case BAP_UC_CLI_CMD_TYPE_DISCOVER:
        {
            le_cis_src_state_service_discovery(EVENT_SERVICE_DISCOVERY_CONTENT, conidx, status, NULL);
        } break;

        case BAP_UC_CLI_CMD_TYPE_REMOVE_GROUP:
        {
            uint8_t cnt;
            uint8_t grp_lid = (uint8_t)conidx;

            for (cnt = 0; cnt < APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_MAX; cnt++)
            {
                le_cis_src_group_t* p_group = &(p_env->group[cnt]);

                if ((GETF(p_group->status_bf, GROUP_STATUS_STATE) >= GROUP_STATE_CREATED)
                        && (p_group->group_lid == grp_lid))
                {
                    SETF(p_group->status_bf, GROUP_STATUS_STATE, GROUP_STATE_ADDED);
                    break;
                }
            }
        } break;

        default: {} break;
    }
}

__STATIC void le_cis_src_cb_bond_data(uint8_t conidx, bap_uc_cli_ascs_t* p_ascs_info)
{
    APP_DBG("Number of Sink ASEs = %d / Number of Source ASEs = %d (conidx = %d)\n", p_ascs_info->nb_ases_sink,
                 p_ascs_info->nb_ases_src, conidx);

    if (app_is_paired(conidx))
    {
        uint8_t nb_chars = p_ascs_info->nb_ases_sink + p_ascs_info->nb_ases_src + BAP_UC_CHAR_TYPE_ASE;
        uint16_t size = sizeof(bap_uc_cli_ascs_t) + (nb_chars * sizeof(bap_uc_cli_ascs_char_t));
        utils_audio_le_cfg_t* p_le_audio_cfg = utils_audio_le_get_cfg(conidx);

        // BOND_DATA_STORE(p_env->p_bond_data_intf, APPLET_ID(AUDIO_UNICAST_CLIENT), 0, conidx,
        //                 size, p_ascs_info);
        p_env->bond_data[conidx].p_ascs = p_ascs_info;

        p_le_audio_cfg->nb_ases_sink = p_ascs_info->nb_ases_sink;
        p_le_audio_cfg->nb_ases_src = p_ascs_info->nb_ases_src;
    }
}

void le_cis_src_restore_bond_data(uint8_t conidx)
{
    uint16_t status;
    app_bond_data_t* p_bond_data = &(p_env->bond_data[conidx]);

    uint8_t nb_chars = p_env->bond_data[conidx].p_ascs->nb_ases_sink + p_env->bond_data[conidx].p_ascs->nb_ases_src + BAP_UC_CHAR_TYPE_ASE;
    uint16_t size = sizeof(bap_uc_cli_ascs_t) + (nb_chars * sizeof(bap_uc_cli_ascs_char_t));
    bap_uc_cli_ascs_t* p_ascs_info = (bap_uc_cli_ascs_t*)ke_malloc_user(size, KE_MEM_PROFILE);

    if (p_bond_data->p_ascs != NULL)
    {
        memcpy(p_ascs_info, p_bond_data->p_ascs, size);
    }

    status = bap_uc_cli_restore_bond_data(conidx, p_ascs_info);
    if (status)
    {
        APP_DBG("bap_uc_cli_restore_bond_data err 0x%x, conidx %d\n", status, conidx);
    }

    ke_free((void*)p_ascs_info);
}

__STATIC void le_cis_src_control(uint16_t type, const void* p_params)
{
    ASSERT_ERR(p_env != NULL);

    APP_DBG("le_cis_src_control: type 0x%x\n", type);
    switch (type)
    {
        case COMPONENT_CONTROL_START:
        {
            le_cis_src_start_t* p_params_start = (le_cis_src_start_t*)p_params;
            le_cis_src_group_t* p_group;

            ASSERT_ERR(p_params != NULL);
            ASSERT_ERR(p_params_start->type < APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_MAX);

            p_group = &(p_env->group[p_params_start->type]);

            if (GETF(p_group->status_bf, GROUP_STATUS_STATE) == GROUP_STATE_QOS_CONFIGURED)
            {
                le_cis_src_enable(p_group);
            }
            else
            {
                if (GETF(p_group->status_bf, GROUP_STATUS_STATE) == GROUP_STATE_ADDED)
                {
                    le_cis_src_create_group(p_group);
                    SETF(p_group->status_bf, GROUP_STATUS_STATE, GROUP_STATE_CREATED);
                }

                le_cis_src_configure_codec(p_group);
            }
        } break;

        case COMPONENT_CONTROL_STOP:
        {
            le_cis_src_stop_t* p_params_stop = (le_cis_src_stop_t*)p_params;
            le_cis_src_group_t* p_group;

            ASSERT_ERR(p_params != NULL);
            ASSERT_ERR(p_params_stop->type < APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_MAX);

            p_group = &(p_env->group[p_params_stop->type]);
            if (p_params_stop->type == APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_CALL)
            {
                // Disable first as there are two ASE with same CIG ID/CIS ID
                SETB(p_group->status_bf, GROUP_STATUS_STOP, 1);
                le_cis_src_disable(p_group);
            }
            else
            {
                le_cis_src_release(p_group);
            }
        } break;


        case APP_LE_AUDIO_UNICAST_CLIENT_CONTROL_PAUSE:
        {
            le_cis_src_pause_t* p_params_pause = (le_cis_src_pause_t*)p_params;
            le_cis_src_group_t* p_group;

            ASSERT_ERR(p_params != NULL);
            ASSERT_ERR(p_params_pause->type < APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_MAX);

            p_group = &(p_env->group[p_params_pause->type]);

            ASSERT_INFO(GETF(p_group->status_bf, GROUP_STATUS_STATE) >= GROUP_STATE_QOS_CONFIGURED, type, 0);

            le_cis_src_disable(p_group);
        } break;

        case APP_LE_AUDIO_UNICAST_CLIENT_CONTROL_ADD:
        {
            le_cis_src_add_t* p_params_add = (le_cis_src_add_t*)p_params;
            le_cis_src_group_t* p_group;
            le_cis_src_ase_t* p_ase;

            ASSERT_ERR(p_params != NULL);
            ASSERT_ERR(p_params_add->type < APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_MAX);

            APP_DBG("APP_LE_AUDIO_UNICAST_CLIENT_CONTROL_ADD: type 0x%x, left %d, right %d\n", 
                        p_params_add->type, p_params_add->conidx_left, p_params_add->conidx_right);

            p_group = &(p_env->group[p_params_add->type]);
            if (GETF(p_group->status_bf, GROUP_STATUS_STATE) == GROUP_STATE_FREE)
            {
                uint8_t cis_id = (p_params_add->type << 2) + STREAM_CIS_ID;

                p_ase = &(p_group->ase_sink[0]);
                p_ase->ase_lid = p_env->next_ase_lid;
                p_ase->cis_id = cis_id;
                p_ase->conidx = p_params_add->conidx_left;
                p_ase->ase_instance_idx = 0;

                p_env->type_ase_table[p_env->next_ase_lid] = p_params_add->type;
                p_env->next_ase_lid++;

                // if (p_params_add->is_stereo
                //         && (p_params_add->conidx_right != GAP_INVALID_CONIDX))
                // {
                //     p_ase = &(p_group->ase_sink[1]);
                //     p_ase->ase_lid = p_env->next_ase_lid;
                //     p_ase->conidx = p_params_add->conidx_right;
                //     p_ase->cis_id = cis_id + 1;
                //     if (p_params_add->conidx_left != p_params_add->conidx_right)
                //     {
                //         p_ase->ase_instance_idx = 0;
                //     }
                //     else
                //     {
                //         p_ase->ase_instance_idx = 1;
                //     }

                //     p_env->type_ase_table[p_env->next_ase_lid] = p_params_add->type;
                //     p_env->next_ase_lid++;
                // }
                // else
                // {
                //     p_group->ase_sink[1].ase_lid = GAF_INVALID_LID;
                // }

                if (p_params_add->is_stereo
                        && (p_params_add->conidx_right == 0))
                {
                    p_ase = &(p_group->ase_sink[1]);
                    p_ase->ase_lid = p_env->next_ase_lid;
                    p_ase->conidx = p_params_add->conidx_left;
                    p_ase->cis_id = cis_id + 1;

                    {
                        p_ase->ase_instance_idx = 1;
                    }

                    p_env->type_ase_table[p_env->next_ase_lid] = p_params_add->type;
                    p_env->next_ase_lid++;
                }
                else
                {
                    p_group->ase_sink[1].ase_lid = GAF_INVALID_LID;
                }
            APP_DBG(": ase_lid 0x%x, cis_id %d, ase_instance_idx %d, next_ase_lid %d\n", 
                        p_ase->ase_lid, p_ase->cis_id, p_ase->ase_instance_idx, p_env->next_ase_lid);
                        
                if (p_params_add->type == APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_CALL)
                {
                    utils_audio_le_cfg_t* p_le_audio_cfg = utils_audio_le_get_cfg(p_params_add->conidx_left);
                    if (!p_le_audio_cfg->nb_ases_src)
                    {
                        printf("no src ase available!!!\n");
                        return;
                    }

                    p_ase = &(p_group->ase_src);
                    p_ase->ase_lid = p_env->next_ase_lid;
                    p_ase->cis_id = cis_id;
                    p_ase->conidx = p_params_add->conidx_left;
                    p_ase->ase_instance_idx = p_le_audio_cfg->nb_ases_sink;
                    p_env->type_ase_table[p_env->next_ase_lid] = p_params_add->type;
                    p_env->next_ase_lid++;
                }
                else
                {
                    p_group->ase_src.ase_lid = GAF_INVALID_LID;
                }

                APP_DBG("src_ase_id 0x%x, instance_idx 0x%X, cis_id 0x%x\n", 
                            p_group->ase_src.ase_lid, p_group->ase_src.ase_instance_idx, p_group->ase_src.cis_id);
                p_group->type = p_params_add->type;
                p_group->status_bf = 0;
                SETB(p_group->status_bf, GROUP_STATUS_STEREO, p_params_add->is_stereo);
                SETF(p_group->status_bf, GROUP_STATUS_STATE, GROUP_STATE_ADDED);
                co_djob_init(&(p_group->djob), le_cis_src_cb_djob);
            }
        } break;

        case COMPONENT_CONTROL_CONFIGURE:
        {
            const le_cis_src_configure_t* p_params_configure
                                                        = (const le_cis_src_configure_t*)p_params;

            ASSERT_ERR(p_params != NULL);
            ASSERT_ERR(p_params_configure->type < APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_MAX);

            le_cis_src_set_configuration_lc3(p_params_configure);
        } break;

        default: {} break;
    }
}

void le_cis_src_cfg_lc3(uint8_t conidx)
{
    utils_audio_le_cfg_t* p_audio_cfg = utils_audio_le_get_cfg(conidx);
    APP_DBG("le_cis_src_cfg_lc3: conidx %d, nb_ases_sink %d\n", conidx, p_audio_cfg->nb_ases_sink);

#ifndef LE_CIS_CONVERSATION
    le_cis_src_configure_t params_configure = {
        .type = APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_MEDIA,
        .sink = true,
        .cfg_name = CODEC_CFG,
        .dp_id = CIS_SINK_DATA_PATH_ID,
        .dp_id_1 = 0xff,
    };

    if (p_audio_cfg->nb_ases_sink == 1)
    {
        if (conidx == 1)
        {
            params_configure.type = APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_MEDIA_2;
            params_configure.dp_id = CIS_SINK_DATA_PATH_ID_2;
        }
    }

    if (p_audio_cfg->nb_ases_sink == 2)
    {
        params_configure.dp_id_1 = CIS_SINK_DATA_PATH_ID_2;        
    }  

    le_cis_src_control(COMPONENT_CONTROL_CONFIGURE, &params_configure);
#else
    le_cis_src_configure_t params_configure_call_source = {
        .type = APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_CALL,
        .sink = false,
        .cfg_name = CODEC_CFG,
        .dp_id = CIS_SINK_DATA_PATH_ID,
    };

    le_cis_src_configure_t params_configure_call_sink = {
        .type = APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_CALL,
        .sink = true,
        .cfg_name = CODEC_CFG,
        .dp_id = CIS_SINK_DATA_PATH_ID,
    };

    le_cis_src_control(COMPONENT_CONTROL_CONFIGURE, &params_configure_call_source);
    le_cis_src_control(COMPONENT_CONTROL_CONFIGURE, &params_configure_call_sink);
#endif
}

void le_cis_src_start_audio(uint8_t conidx)
{
    le_cis_src_add_t params_add;
    utils_audio_le_cfg_t* p_le_audio_cfg = utils_audio_le_get_cfg(conidx);
#ifndef LE_CIS_CONVERSATION
    le_cis_src_start_t params_start = {
        .type = APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_MEDIA,
    };

    params_add.type = APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_MEDIA;
#else
    le_cis_src_start_t params_start = {
        .type = APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_CALL,
    };
    params_add.type = APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_CALL;
#endif
    params_add.conidx_left = conidx;

    params_add.conidx_right = p_le_audio_cfg->conidx_set_member;


    params_add.is_stereo = (p_le_audio_cfg->nb_ases_sink >= 2);

    APP_DBG("nb_ases_sink %d, is_stereo %d\n", p_le_audio_cfg->nb_ases_sink, params_add.is_stereo);
    le_cis_src_control(APP_LE_AUDIO_UNICAST_CLIENT_CONTROL_ADD, &params_add);

    le_cis_src_control(COMPONENT_CONTROL_START, &params_start);
}

__STATIC void le_cis_src_cb_quality_cmp_evt(uint16_t status, uint8_t ase_lid,
                                                             uint32_t tx_unacked_packets,
                                                             uint32_t tx_flushed_packets,
                                                             uint32_t tx_last_subevent_packets,
                                                             uint32_t retransmitted_packets,
                                                             uint32_t crc_error_packets,
                                                             uint32_t rx_unreceived_packets,
                                                             uint32_t duplicate_packets)
{
}

__STATIC void le_cis_src_cb_error(uint8_t ase_lid, uint8_t opcode, uint8_t rsp_code, uint8_t reason)
{
    ASSERT_WARN(0, opcode, rsp_code);
}

__STATIC void le_cis_src_cb_cis_state(uint8_t stream_lid, uint8_t event, uint8_t conidx,
                                                       uint8_t ase_lid_sink, uint8_t ase_lid_src, uint8_t grp_lid,
                                                       uint8_t cis_id, uint16_t conhdl, gapi_ug_config_t* p_cig_cfg,
                                                       gapi_us_config_t* p_cis_cfg)
{
    #if (APP_CTRL)
    p_env->conhdl = conhdl;
    #endif //(APP_CTRL)
}

__STATIC void le_cis_src_cb_state_empty(uint8_t conidx, uint8_t ase_instance_idx, uint8_t ase_lid,
                                                         uint8_t state)
{
    printf("ase_lid %d, ase state 0x%x\n", ase_lid, state);
    switch (state)
    {
        case BAP_UC_ASE_STATE_IDLE:
        {
             APP_DBG("State - ASE Instance %d / ASE %d = Idle\n", ase_instance_idx, ase_lid);

            // if ((ase_lid != GAF_INVALID_LID)
            //         && (ase_instance_idx != GAF_INVALID_LID))
            if (ase_lid != GAF_INVALID_LID)
            {
                uint8_t type = p_env->type_ase_table[ase_lid];
                le_cis_src_group_t* p_group = &(p_env->group[type]);

                // Detect release
                if (GETF(p_group->status_bf, GROUP_STATUS_STATE) == GROUP_STATE_QOS_CONFIGURED)
                {
                    p_group->ase_done_bf |= CO_BIT(ase_lid);
                    p_group->djob_event = EVENT_GROUP_STOPPED;
                    co_djob_reg(CO_DJOB_LOW, &(p_group->djob));
                }

                #if (APP_CTRL)
                if (p_env->ase_lid_dp == ase_lid)
                {
                    p_env->ase_lid_dp = GAF_INVALID_LID;
                }
                #endif //(APP_CTRL)
            }
        } break;

        default:
        {
            APP_DBG("State - ASE %d = Releasing\n", ase_lid);
        } break;
    }
}

__STATIC void le_cis_src_cb_state_codec(uint8_t conidx, uint8_t ase_instance_idx, uint8_t ase_lid,
                                                         gaf_codec_id_t* p_codec_id, bap_qos_req_t* p_qos_req,
                                                         const bap_cfg_ptr_t* p_cfg)
{
    uint8_t type = p_env->type_ase_table[ase_lid];
    le_cis_src_group_t* p_group = &(p_env->group[type]);

    APP_DBG("State - ASE %d = Codec Configured\n", ase_lid);

    p_group->ase_done_bf |= CO_BIT(ase_lid);

    if (GETF(p_group->status_bf, GROUP_STATUS_STATE) == GROUP_STATE_QOS_CONFIGURED)
    {
        p_group->djob_event = EVENT_GROUP_STOPPED;
    }
    else
    {
        p_group->djob_event = EVENT_GROUP_CODEC_CONFIGURED;
    }

    co_djob_reg(CO_DJOB_LOW, &(p_group->djob));
}

__STATIC void le_cis_src_cb_state_qos(uint8_t ase_lid, const bap_qos_cfg_t* p_qos_cfg)
{
    uint8_t type = p_env->type_ase_table[ase_lid];
    le_cis_src_group_t* p_group = &(p_env->group[type]);

    APP_DBG("State - ASE %d = QoS Configured\n", ase_lid);

    p_group->ase_done_bf |= CO_BIT(ase_lid);
    p_group->djob_event = EVENT_GROUP_QOS_CONFIGURED;
    co_djob_reg(CO_DJOB_LOW, &(p_group->djob));
}


__STATIC void le_cis_src_cb_state_metadata(uint8_t ase_lid, uint8_t state,
                                                            const bap_cfg_metadata_ptr_t* p_metadata)
{
    printf("le_cis_src_cb_state_metadata: ase_lid %d, state 0x%x\n", ase_lid, state);
    switch (state)
    {
        case BAP_UC_ASE_STATE_STREAMING:
        {
            uint8_t type = p_env->type_ase_table[ase_lid];
            le_cis_src_group_t* p_group = &(p_env->group[type]);

            APP_DBG("State - ASE %d = Streaming\n", ase_lid);
            if(ase_lid == 1)
            {
                extern uint8_t cis_src_sync_ok ;
                cis_src_send_data_clr_2();
                cis_src_sync_ok = 1;
            }
            p_group->ase_done_bf |= CO_BIT(ase_lid);
            p_group->djob_event = EVENT_GROUP_STARTED;
            co_djob_reg(CO_DJOB_LOW, &(p_group->djob));
        } break;

        case BAP_UC_ASE_STATE_DISABLING:
        {
            if (!ase_lid)
            {
            	cis_src2sink_streaming = 0;//sink ase
            }
            else if (ase_lid == 1)
            {
            	cis_src2sink_streaming = 0;//src ase
            }

        } break;

        case BAP_UC_ASE_STATE_RELEASING:
        {
            if (!ase_lid)
            {
            	cis_src2sink_streaming = 0;//sink ase
            }
            else if (ase_lid == 1)
            {
            	cis_src2sink_streaming = 0;//src ase
            }
        } break;

        default:
        {
            APP_DBG("State - ASE %d = state 0x%x\n", ase_lid, state);
        } break;
    }
}

__STATIC void le_cis_src_cb_svc_changed(uint8_t conidx)
{
    APP_DBG("le_cis_src_cb_svc_changed (conidx %d)\n", conidx);
}

__STATIC void le_cis_src_cb_dp_update_req(uint8_t ase_lid, bool start)
{
    APP_DBG("start: 0x%x data path requested (ASE %d)\n", start, ase_lid);
    if (!ase_lid)
    {
    	cis_src2sink_streaming = 1; //sink ase
        LeAudioSerEncodeInit();
    }
    else if (ase_lid == 1)
    {
#ifdef LE_CIS_CONVERSATION
    	cis_sink2src_streaming = 1; //src ase
    	void LeAudioSinkModeEnter(void);
    	LeAudioSinkModeEnter();
#endif
    }


    // Automatically accept to start/stop data path
    bap_uc_cli_dp_update_cfm(ase_lid, start);
}

// Set of callback functions for communication fir BAP Unicast Client module
__STATIC const bap_uc_cli_cb_t cbs_bap_uc_cli = {
    .cb_cmp_evt = le_cis_src_cb_cmp_evt,
    .cb_quality_cmp_evt = le_cis_src_cb_quality_cmp_evt,
    .cb_bond_data = le_cis_src_cb_bond_data,
    .cb_error = le_cis_src_cb_error,
    .cb_cis_state = le_cis_src_cb_cis_state,
    .cb_state_empty = le_cis_src_cb_state_empty,
    .cb_state_codec = le_cis_src_cb_state_codec,
    .cb_state_qos = le_cis_src_cb_state_qos,
    .cb_state_metadata = le_cis_src_cb_state_metadata,
    .cb_svc_changed = le_cis_src_cb_svc_changed,
    .cb_dp_update_req = le_cis_src_cb_dp_update_req,
};

__STATIC void le_cis_src_configure(const void* p_params, component_cb_event cb_event)
{
    // Device appearance
    uint16_t APPEARANCE = 0x00;
    uint16_t status;

    bap_uc_cli_cfg_t uc_cli_cfg = {
        .cfg_bf = 0,
        .nb_ases_cfg = STREAM_MAX_NB,
        .pref_mtu = 64,
        .timeout_s = 2,
    };
    uint8_t cnt;

    ASSERT_ERR(p_env == NULL);
    ASSERT_ERR(cb_event != NULL);

    p_env = (le_cis_src_env_t*)ke_malloc_user(sizeof(le_cis_src_env_t), KE_MEM_PROFILE);
    ASSERT_ERR(p_env != NULL);
    memset(p_env, 0, sizeof(le_cis_src_env_t));

    p_env->cb_mediator_event = cb_event;

    p_env->ase_lid_dp = GAF_INVALID_LID;

    for (cnt = 0; cnt < APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_MAX; cnt++)
    {
        p_env->group[cnt].group_lid = GAF_INVALID_LID;
    }

    // Add Audio Stream Control Service
    status = bap_uc_cli_configure(&cbs_bap_uc_cli, &uc_cli_cfg);
    if (status)
    {
        APP_DBG("le_cis_src_configure: bap_uc_cli_configure error 0x%x\n", status);
    }

    gapm_le_set_appearance(&APPEARANCE);
}

void app_choose_cis_scene(uint8_t scene)
{
    g_scene = scene;
}

uint8_t get_cis_scene(void)
{
    APP_DBG("g_scene 0x%x\n", g_scene);
    return g_scene;
}

__STATIC void le_cis_src_cb_event_audio_unicast_client(uint16_t type, uint16_t status,
                                                                  const void* p_params)
{
    APP_DBG("le_cis_src_cb_event_audio_unicast_client: type 0x%x\n", type);
    switch (type)
    {
        case COMPONENT_EVENT_SERVICE_CONTENT_DISCOVERED:
        {
            app_le_audio_discovery(EVENT_SERVICE_DISCOVERY_COMPLETED,
                                                               *((uint8_t*)p_params), status, NULL);
        } break;

        default: { } break;
    }
}
__STATIC void app_unicast_client_cb_event_audio_capabilities_client(uint16_t type, uint16_t status, const void* p_params)
{
    switch (type)
    {
        case COMPONENT_EVENT_SERVICE_CONTENT_DISCOVERED:
        {
            app_le_audio_discovery(EVENT_SERVICE_DISCOVERY_CONTENT_DISCOVERED,
                                                        *((uint8_t*)p_params), status, NULL);
        } break;

        default: { } break;
    }
}

__STATIC void app_unicast_client_cb_event_volume_controller(uint16_t type, uint16_t status, const void* p_params)
{
    switch (type)
    {
        case COMPONENT_EVENT_SERVICE_CONTENT_DISCOVERED:
        {
            app_le_audio_discovery(EVENT_SERVICE_DISCOVERY_CONTENT_DISCOVERED,
                                                               *((uint8_t*)p_params), status, NULL);
        } break;

        default: { } break;
    }
}

__STATIC void app_unicast_client_cb_event_telephony_media_client(uint16_t type, uint16_t status, const void* p_params)
{
    switch (type)
    {
        case COMPONENT_EVENT_SERVICE_CONTENT_DISCOVERED:
        {
            app_le_audio_discovery(EVENT_SERVICE_DISCOVERY_CONTENT_DISCOVERED,
                                                        *((uint8_t*)p_params), status, NULL);
        } break;

        default: { } break;
    }
}

__STATIC void app_unicast_client_cb_event_common_audio_client(uint16_t type, uint16_t status, const void* p_params)
{
    switch (type)
    {
        case COMPONENT_EVENT_SERVICE_CONTENT_DISCOVERED:
        {
            app_le_audio_discovery(EVENT_SERVICE_DISCOVERY_CONTENT_DISCOVERED,
                                                        *((uint8_t*)p_params), status, NULL);
        } break;

        default: { } break;
    }
}

void media_player_transition_group(uint8_t event, uint16_t status, uint8_t conidx)
{
    APP_DBG("media_player_transition_group: event 0x%x, status 0x%x, conidx %d\n", event, status, conidx);
    switch (event)
    {
        case EVENT_GROUP_START:
        {
            {
                utils_audio_le_cfg_t* p_le_audio_cfg = utils_audio_le_get_cfg(conidx);
                le_cis_src_add_t params_add;

                params_add.type = APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_MEDIA;
                params_add.conidx_left = conidx;
                if (p_le_audio_cfg->conidx_set_member != GAP_INVALID_CONIDX)
                {
                    params_add.conidx_right = p_le_audio_cfg->conidx_set_member,
                    params_add.is_stereo = true;
                }
                else
                {
                    params_add.conidx_right = 0,
                    params_add.is_stereo = (p_le_audio_cfg->nb_ases_sink >= 2);
                }

                printf("baikal: is_stereo %d, nb_ases_sink %d, conidx_set_member 0x%x\n", params_add.is_stereo, p_le_audio_cfg->nb_ases_sink, p_le_audio_cfg->conidx_set_member);
                // params_add.is_stereo = 1;
                

                if (p_le_audio_cfg->nb_ases_sink == 1)
                {
                    if (params_add.conidx_left == 1)
                    {
                        params_add.type = APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_MEDIA_2;
                    }
                }

                le_cis_src_control(APP_LE_AUDIO_UNICAST_CLIENT_CONTROL_ADD, &params_add);

                //start cis
                le_cis_src_start_t params_start = {
                    .type = APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_MEDIA,
                };

                if (p_le_audio_cfg->nb_ases_sink == 1)
                {
                    if (conidx == 1)
                    {
                        params_start.type = APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_MEDIA_2;
                    }
                }

                le_cis_src_control(COMPONENT_CONTROL_START, &params_start);

            }
        } break;

        case EVENT_GROUP_PLAY:
        {
            {
                le_cis_src_start_t params_start = {
                    .type = APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_MEDIA,
                };
#if 1
                if (conidx == 1)
                {
                    params_start.type = APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_MEDIA_2;
                }
#endif
                le_cis_src_control(COMPONENT_CONTROL_START, &params_start);
            }
        } break;

        case EVENT_GROUP_PAUSE:
        {
            le_cis_src_pause_t params_pause = {
                .type = APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_MEDIA,
            };
#if 1
                if (conidx == 1)
                {
                	params_pause.type = APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_MEDIA_2;
                }
#endif
            le_cis_src_control(APP_LE_AUDIO_UNICAST_CLIENT_CONTROL_PAUSE, &params_pause);

        } break;

        case EVENT_GROUP_STOP:
        {
            {
                le_cis_src_stop_t params_stop = {
                .type = APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_MEDIA,
                };
#if 1
                if (conidx == 1)
                {
                	params_stop.type = APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_MEDIA_2;
                }
#endif
                le_cis_src_control(COMPONENT_CONTROL_STOP, &params_stop);
            }
        } break;

        default: { } break;
    }
}

// void le_cis_start_media(void)
// {
//     media_player_transition_group(EVENT_GROUP_START, GAF_ERR_NO_ERROR);
// }

__STATIC void app_unicast_client_cb_event_media_server(uint16_t type, uint16_t status, const void* p_params)
{
    APP_DBG("app_unicast_client_cb_event_media_server: type 0x%x\n", type);

    switch (type)
    {
        case APP_LE_MEDIA_SERVER_EVENT_STATE_UPDATED:
        {
            app_le_media_server_event_state_updated_t* p_params_state
                                                = (app_le_media_server_event_state_updated_t*)p_params;

            switch (p_params_state->state)
            {
                case ACC_MC_MEDIA_STATE_INACTIVE:
                {
                    media_player_transition_group(EVENT_GROUP_STOP, GAF_ERR_NO_ERROR, 0xff);
                } break;

                case ACC_MC_MEDIA_STATE_PLAYING:
                {
                    media_player_transition_group(EVENT_GROUP_PLAY, GAF_ERR_NO_ERROR, 0xff);
                } break;

                case ACC_MC_MEDIA_STATE_PAUSED:
                {
                    media_player_transition_group(EVENT_GROUP_PAUSE, GAF_ERR_NO_ERROR, 0xff);
                } break;

                default: { } break;
            }
        } break;

        default: { } break;
    }
}

void mediator_phone_start_group(uint8_t conidx)
{
    utils_audio_le_cfg_t* p_le_audio_cfg = utils_audio_le_get_cfg(conidx);
    le_cis_src_add_t params_add;

    params_add.type = APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_CALL;
    params_add.conidx_left = conidx;
    if (p_le_audio_cfg->conidx_set_member != GAP_INVALID_CONIDX)
    {
        params_add.conidx_right = p_le_audio_cfg->conidx_set_member,
        params_add.is_stereo = true;
    }
    else
    {
        params_add.conidx_right = 0,
        params_add.is_stereo = (p_le_audio_cfg ->nb_ases_sink >= 2);
    }

    le_cis_src_control(APP_LE_AUDIO_UNICAST_CLIENT_CONTROL_ADD, &params_add);


    params_add.type = APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_RINGTONE;
    params_add.conidx_left = conidx;
    params_add.is_stereo = false;

    le_cis_src_control(APP_LE_AUDIO_UNICAST_CLIENT_CONTROL_ADD, &params_add);

#if 0
    {
        le_cis_src_start_t params_start = {
            .type = APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_CALL,
        };

        le_cis_src_control(COMPONENT_CONTROL_START, &params_start);
    }
#endif
}

__STATIC void app_unicast_client_cb_event_call_server(uint16_t type, uint16_t status, const void* p_params)
{
    APP_DBG("app_unicast_client_cb_event_call_server: type 0x%x\n", type);
    switch (type)
    {
        case APP_LE_CALL_SERVER_EVENT_START_REQ:
        {
            app_le_call_server_event_start_req_t* p_params_start
                                                = (app_le_call_server_event_start_req_t*)p_params;
            le_cis_src_start_t params_start = {
                    .type = (p_params_start->type == APP_LE_CALL_SERVER_TYPE_CALL)
                                                    ? APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_CALL
                                                    : APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_RINGTONE,
            };

            le_cis_src_control(COMPONENT_CONTROL_START, &params_start);
        } break;

        case APP_LE_CALL_SERVER_EVENT_STOP_REQ:
        {
            app_le_call_server_event_stop_req_t* p_params_stop = (app_le_call_server_event_stop_req_t*)p_params;
            le_cis_src_stop_t params_stop = {
                .type = (p_params_stop->type == APP_LE_CALL_SERVER_TYPE_CALL)
                    ? APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_CALL : APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_RINGTONE,
            };

            le_cis_src_control(COMPONENT_CONTROL_STOP, &params_stop);
        } break;

        default: { } break;
    }
}

__STATIC void app_unicast_client_cb_event_coordinated_set_coordinator(uint16_t type, uint16_t status,
                                                                         const void* p_params)
{
    APP_DBG("app_unicast_client_cb_event_coordinated_set_coordinator: type 0x%x\n", type);
    switch (type)
    {
        case COMPONENT_EVENT_SERVICE_CONTENT_DISCOVERED:
        {
            app_le_audio_discovery(EVENT_SERVICE_DISCOVERY_CONTENT_DISCOVERED, *((uint8_t*)p_params), status, NULL);
        } break;

        case APP_LE_COORDINATED_SET_COORDINATOR_EVENT_RESOLVED:
        {
            if (status == GAP_ERR_NO_ERROR)
            {
                app_le_coordinated_set_coordinator_resolved_t* p_params_resolved
                                                        = (app_le_coordinated_set_coordinator_resolved_t*)p_params;

#ifdef LE_UDSC
                app_le_start_conn(status, p_params_resolved);
#endif   
                    // app_le_conn_start(p_params_resolved);
            }
        } break;

        default: { } break;
    }
}

void le_cis_src_init(uint8_t cis_app)
{
    le_cis_src_configure(NULL, le_cis_src_cb_event_audio_unicast_client);
    app_le_audio_capabilities_client_configure(NULL, app_unicast_client_cb_event_audio_capabilities_client);
    app_le_volume_controller_configure(NULL, app_unicast_client_cb_event_volume_controller);
    app_le_telephony_media_client_configure(NULL, app_unicast_client_cb_event_telephony_media_client);
    app_le_common_audio_client_configure(NULL, app_unicast_client_cb_event_common_audio_client);
    app_le_media_server_configure(NULL, app_unicast_client_cb_event_media_server);
    app_le_coordinated_set_coordinator_configure(NULL, app_unicast_client_cb_event_coordinated_set_coordinator);
    app_le_call_server_configure(NULL, app_unicast_client_cb_event_call_server);
    uint16_t status = app_gaf_client_configure();
    if (status)
    {
        APP_DBG("le_cis_src_init: app_gaf_client_configure err 0x%x\n", status);
    }

    app_choose_cis_scene(CIS_APP_MEDIA_PLAYER);
    utils_init();
}

#endif
#endif