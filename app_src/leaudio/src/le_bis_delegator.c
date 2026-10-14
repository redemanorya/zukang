/*
 * le_bis_delegator.c
 *
 *  Created on: Apr 15, 2024
 *     
 */

#include "le_audio_config.h"
#if (BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT)
#include "debug.h"
#include "gapm.h"
#include "bap_bc_sink.h"
#include "bap_bc_scan.h"
#include "utils_cb_gap.h"
#include "bap_bc_deleg.h"
#include "le_bis_delegator.h"
#include "bap.h"
#include "le_audio_config.h"
#include "debug.h"

extern void set_sink_synced(void);

#ifdef LE_BIS_DELEG
/*
 * TYPES DEFINITION
 ****************************************************************************************
 */
#define DATA_PATH_ISOOHCI  (0)

// Bond Data structure
typedef struct le_deleg_bond_data
{
    // Client characteristic configuration bit field
    uint16_t cli_cfg_bf;
} le_deleg_bond_data_t;

// Environment structure
typedef struct le_deleg_env
{
    // Pointer to Output Provider interface
//    const output_prov_intf_t* p_output_intf;
//    // Pointer to Bond Data Provider interface
//    const bond_data_prov_intf_t* p_bond_data_intf;
//    // Registration structure for events
//    utils_control_intf_t control_intf;
    #if (APP_CTRL)
    // Pointer to Data Path Provider interface
    const data_path_prov_intf_t* p_data_path_intf;
    #endif //(APP_CTRL)
    // Codec configuration for stream with which synchronization must be established
    bap_cfg_t codec_cfg;
    // Position of stream with which synchronization must be established
    uint8_t stream_pos;
    // Connection local index
    uint8_t conidx;
    // Group local index
    uint8_t grp_lid;
    // Waiting for codec configuration ?
    bool wait_codec_cfg;

    uint8_t con_lid;
} le_deleg_env_t;
/*
 * LOCAL VARIABLES
 ****************************************************************************************
 */

extern uint16_t lc3_packet_len;
// Pointer to environment
__STATIC le_deleg_env_t* p_env;
__STATIC void le_bis_deleg_configuration_state(uint16_t event, uint16_t status, const void* p_params);                                               
/*
 * CALLBACK FUNCTIONS
 ****************************************************************************************
 */

__STATIC void le_deleg_cb_bond_data_prov(uint16_t component_id, uint16_t type, uint8_t conidx,
                                                           bool found, uint16_t length, const void* p_data)
{
    if (found == true)
    {
        const le_deleg_bond_data_t* p_bond_data
                                        = (const le_deleg_bond_data_t*)p_data;
        bap_bc_deleg_restore_bond_data(conidx, p_bond_data->cli_cfg_bf);
    }
}

#if (!(0))
__STATIC void le_deleg_cb_cmp_evt(uint8_t cmd_type, uint16_t status, uint8_t src_lid)
{
    APP_DBG("le_deleg_cb_cmp_evt, cmd_type 0x%x, status 0x%x\n", cmd_type, status);
}

__STATIC void le_deleg_cb_solicite_stopped(uint8_t reason)
{
    APP_DBG("le_deleg_cb_solicite_stopped, reason 0x%x\n", reason);
}
#endif //(!(0))

__STATIC void le_deleg_cb_bond_data(uint8_t conidx, uint16_t cli_cfg_bf)
{
    APP_DBG("BASS Bond Data updated (conidx = %d, cli_cfg_bf = %d)\n", conidx, cli_cfg_bf);
}

__STATIC void le_deleg_cb_remote_scan(uint8_t conidx, uint8_t state)
{
    APP_DBG("le_deleg_cb_remote_scan: conidx: 0x%x, state: 0x%x\n", conidx, state);
}

__STATIC void le_deleg_cb_bcast_code(uint8_t src_lid, uint8_t conidx,
                                                       const gaf_bcast_code_t* p_bcast_code)
{
    APP_DBG("le_deleg_cb_bcast_code: conidx: 0x%x\n", conidx);
}
extern void DisplayTaskInf(void);
__STATIC void le_deleg_cb_add_source_req(uint8_t src_lid, uint8_t conidx,
                                                           const bap_adv_id_t* p_adv_id,
                                                           const bap_bcast_id_t* p_bcast_id, uint8_t pa_sync_req,
                                                           uint16_t pa_intv_frames, uint8_t nb_subgroups,
                                                           uint16_t metadata_len)
{
    uint16_t ret = 0;
    APP_DBG("le_deleg_cb_add_source_req \n");

    bap_bc_deleg_add_source_cfm(src_lid, true);

    set_decoder_reinit(1);
    set_sink_synced();
    
#if 1// 
    LeAudioSinkModeEnter();
#endif
}

 __STATIC void le_deleg_cb_modify_source_req(uint8_t src_lid, uint8_t conidx, uint8_t pa_sync_req,
                                                               uint16_t pa_intv_frames, uint8_t nb_subgroups,
                                                               uint16_t metadata_len)
 {
    APP_DBG("le_deleg_cb_modify_source_req \n");
    bap_bc_deleg_modify_source_cfm(src_lid, true);
 }

__STATIC void le_deleg_cb_remove_source_req(uint8_t src_lid, uint8_t conidx)
{
    bap_bc_deleg_remove_source_cfm(src_lid, true);
}

// Set of callback functions for communication with BAP Broadcast Delegator module
__STATIC const bap_bc_deleg_cb_t cbs_bap_bc_deleg =
{
//    #if (!(0))
    .cb_cmp_evt = le_deleg_cb_cmp_evt,
    .cb_solicite_stopped = le_deleg_cb_solicite_stopped,
//    #endif //(!(0))
    .cb_bond_data = le_deleg_cb_bond_data,
    .cb_remote_scan = le_deleg_cb_remote_scan,
    .cb_bcast_code = le_deleg_cb_bcast_code,
    .cb_add_source_req = le_deleg_cb_add_source_req,
    .cb_modify_source_req = le_deleg_cb_modify_source_req,
    .cb_remove_source_req = le_deleg_cb_remove_source_req,
};

// extern uint8_t bis_synced;
__STATIC void le_deleg_cb_sink_cmp_evt(uint8_t cmd_type, uint16_t status, uint8_t grp_lid,
                                                         uint8_t stream_pos)
{
    APP_DBG("le_deleg_cb_sink_cmp_evt: status 0x%x, cmd_type 0x%x\n", status, cmd_type);

    switch (cmd_type)
    {
        case BAP_BC_SINK_CMD_TYPE_START_STREAMING:
        {
            if (!status)
            {
        	    // dac_ctrl_on();
            }
        } break;

        case BAP_BC_SINK_CMD_TYPE_STOP_STREAMING:
        {
            if (!status)
            {
                // bis_synced = 0;
            }
        }
    }

}

__STATIC void le_deleg_cb_sink_quality_cmp_evt(uint16_t status, uint8_t grp_lid,
                                                                 uint8_t stream_pos, uint32_t crc_error_packets,
                                                                 uint32_t rx_unrx_packets, uint32_t duplicate_packets)
{
    APP_DBG("le_deleg_cb_sink_quality_cmp_evt: status 0x%x\n", status);
}

// Disconnect procedure function handler
__STATIC void app_disconnect_cmp(uint8_t conidx, uint32_t metainfo, uint16_t status)
{
    APP_DBG("DISCONNECT CMD COMPLETED %d, 0x%04X\n", conidx, status);
}

__STATIC uint16_t app_disconnect(uint8_t conidx)
{
    return gapc_disconnect(conidx, 0, 0xa3, app_disconnect_cmp);
}

__STATIC void le_deleg_cb_sink_status(uint8_t grp_lid, uint8_t state, uint32_t stream_pos_bf,
                                                        const gapi_bg_sync_config_t* p_bg_cfg, uint8_t nb_bis,
                                                        const uint16_t* p_conhdl)
{
    APP_DBG("BIG synchronization status = %d, BIG = 0x%08X\n", state, stream_pos_bf);

    if ((stream_pos_bf != 0) && (state == BAP_BC_SINK_ESTABLISHED))
    {
        uint8_t buffer[50];
        gaf_codec_id_t delg_codec_id = 
        {      
            .codec_id = { CODEC_ID_LC3, 0, 0, 0, 0 },
        };

        uint8_t dp_id = BIS_SINK_DATA_PATH_ID; // default value
        if (stream_pos_bf == 1)
        {
            dp_id = BIS_SINK_DATA_PATH_ID;
        }
        else if (stream_pos_bf == 2)
        {
            dp_id = BIS_SINK1_DATA_PATH_ID;    
        }

        // Enable reception of audio data
        uint16_t ret = bap_bc_sink_start_streaming(grp_lid, stream_pos_bf, &delg_codec_id, dp_id/*DATA_PATH_ISOOHCI*/, 0, (gaf_ltv_t*)buffer);
        if (ret)
        {
            APP_DBG("bap_bc_sink_start_streaming: failed 0x%x\n", ret);
        }
    }
    else if (state == BAP_BC_SINK_LOST)
    {
        // bap_bc_sink_enable_cfm(p_env->grp_lid, true, 1 << stream_pos_bf, 1000, 0);      
        bap_bc_scan_start(0);  
    }
}

__STATIC void le_deleg_cb_sink_enable_req(uint8_t grp_lid, uint8_t src_lid, uint8_t con_lid,
                                                            uint32_t stream_pos_bf, uint32_t stream_pos_bf_opt)
{
    // Synchronization with one stream only
    uint8_t stream_pos = co_ctz(stream_pos_bf | stream_pos_bf_opt);

    p_env->stream_pos = stream_pos + 1;
    p_env->wait_codec_cfg = true;
    p_env->grp_lid = grp_lid;
    p_env->con_lid = con_lid;

    APP_DBG("Synchronization with BIG requested - Position = %d\n", p_env->stream_pos);
}

__STATIC void le_deleg_cb_sink_disable_req(uint8_t grp_lid, uint8_t con_lid)
{
    APP_DBG("Desynchronization from BIG requested\n");
    bap_bc_sink_disable_cfm(grp_lid, true);
}

// Set of callback functions for communication with BAP Broadcast Sink module
__STATIC const bap_bc_sink_cb_t cbs_bap_bc_sink =
{
    .cb_cmp_evt = le_deleg_cb_sink_cmp_evt,
    .cb_quality_cmp_evt = le_deleg_cb_sink_quality_cmp_evt,
    .cb_status = le_deleg_cb_sink_status,
    .cb_enable_req = le_deleg_cb_sink_enable_req,
    .cb_disable_req = le_deleg_cb_sink_disable_req,
};

__STATIC void le_deleg_cb_scan_cmp_evt(uint8_t cmd_type, uint16_t status, uint8_t pa_lid)
{
}

__STATIC void le_deleg_cb_scan_timeout(void)
{
//    OUTPUT_DEBUG(p_env->p_output_intf, APPLET_ID(BROADCAST_DELEGATOR), "Scan timeout");
}

__STATIC void le_deleg_cb_scan_report(const bap_adv_id_t* p_adv_id, const bap_bcast_id_t* p_bcast_id,
                                                        uint8_t info_bf, const gaf_adv_report_air_info_t* p_air_info,
                                                        uint16_t length, const uint8_t* p_data)
{
}

__STATIC void le_deleg_cb_scan_pa_established(uint8_t pa_lid, const bap_adv_id_t* p_adv_id,
                                                                uint8_t phy, uint16_t interval_frames)
{
//    OUTPUT_DEBUG(p_env->p_output_intf, APPLET_ID(BROADCAST_DELEGATOR),
//                 "Synchronized with PA (phy = %d, interval = %dx1.25ms)", phy, interval_frames);
}

#if 1
__STATIC void le_deleg_cb_public_bcast_source(const bap_adv_id_t* p_adv_id,
                                                                const bap_bcast_id_t* p_bcast_id,
                                                                uint8_t pbp_features_bf, uint8_t broadcast_name_len,
                                                                const uint8_t* p_broadcast_name, uint8_t metadata_len,
                                                                const uint8_t* p_metadata)
{
}
#endif

__STATIC void le_deleg_cb_scan_pa_terminated(uint8_t pa_lid, uint8_t reason)
{
    APP_DBG("No more synchronized with PA (reason = %d)\n", reason);
}

__STATIC void le_deleg_cb_scan_pa_report(uint8_t pa_lid, const gaf_adv_report_air_info_t* p_air_info,
                                                           uint16_t length, const uint8_t* p_data)
{
}

__STATIC void le_deleg_cb_scan_big_info_report(uint8_t pa_lid, const gapm_le_big_info_t* p_report)
{
    // APP_DBG("le_deleg_cb_scan_big_info_report\n");
}

__STATIC void le_deleg_cb_scan_group_report(uint8_t pa_lid, uint8_t nb_subgroups, uint8_t nb_streams,
                                                              uint32_t pres_delay_us)
{
    // APP_DBG("le_deleg_cb_scan_group_report\n");
}

__STATIC void le_deleg_cb_scan_subgroup_report(uint8_t pa_lid, uint8_t sgrp_id,
                                                                 uint32_t stream_pos_bf,
                                                                 const gaf_codec_id_t* p_codec_id,
                                                                 const bap_cfg_ptr_t* p_cfg,
                                                                 const bap_cfg_metadata_ptr_t* p_metadata)
{
    // APP_DBG("le_deleg_cb_scan_subgroup_report\n");
}

__STATIC bool check_codec_cfg_is_vaild(const gaf_codec_id_t* p_codec_id, const bap_cfg_ptr_t* p_cfg)
{
    bool is_vaild = false;
    uint8_t	lc3_channel = 0;     // Initialize to safe default
    uint32_t lc3_bitrate = 0;     // Initialize to safe default  
    uint16_t lc3_frame_len = 0;   // Initialize to safe default
    uint16_t SampleRate = 0;
    APP_DBG("codec_id: 0x%x, sampling_freq 0x%x, frame_dur %d, frame_octet %d\n", 
                    p_codec_id->codec_id[0], p_cfg->param.sampling_freq, p_cfg->param.frame_dur, p_cfg->param.frame_octet);
    if (p_codec_id->codec_id[0] == GAPI_CODEC_FORMAT_LC3)
    {
        switch (p_cfg->param.sampling_freq)
        {
            case BAP_SAMPLING_FREQ_48000HZ:
            {
                if (p_cfg->param.frame_dur == BAP_FRAME_DUR_10MS)
                {
                    switch (p_cfg->param.frame_octet)
                    {
                        case BAP_BC_CFG_48_2_SDU_LEN:
                        case BAP_BC_CFG_48_4_SDU_LEN:
                        {
                            lc3_packet_len = p_cfg->param.frame_octet;
                            lc3_bitrate = 800 * p_cfg->param.frame_octet;
                            lc3_frame_len = p_cfg->param.frame_octet;
                            lc3_channel = LC3_MONO;
                            SampleRate = 48000;
                            is_vaild = true;
                        } break;

                        default:
                            break;                        
                    }
                }
            } break;

            default:
                break;
        }
        if(is_vaild == true)
        	LeAudioLc3DecoderConfig(lc3_channel, lc3_bitrate, lc3_frame_len,SampleRate);
    }
    APP_DBG("check_codec_cfg_is_vaild 0x%x, 0x%lx, 0x%lx, 0x%lx, 0x%x\n", 
                is_vaild, (unsigned long)lc3_packet_len, (unsigned long)lc3_bitrate, 
                (unsigned long)lc3_frame_len, lc3_channel);
    return is_vaild;
} 

__STATIC void le_deleg_cb_scan_stream_report(uint8_t pa_lid, uint8_t sgrp_id, uint8_t stream_pos,const gaf_codec_id_t* p_codec_id, 
                                                    const bap_cfg_ptr_t* p_cfg, uint8_t* bcast_id, uint8_t* volume)
{
    // APP_DBG("Stream report received (sgrp_id = %d, pos = %d), 0x%x, 0x%x\n", sgrp_id, stream_pos, p_env->wait_codec_cfg, p_env->stream_pos);

    if (p_env->wait_codec_cfg && (stream_pos == p_env->stream_pos))
    {
        if (p_codec_id->codec_id[0] == GAPI_CODEC_FORMAT_LC3)
        {
            bool is_codec_cfg_vaild = check_codec_cfg_is_vaild(p_codec_id, p_cfg);
            if (!is_codec_cfg_vaild)
            {
                // APP_DBG("LC3 codec cfg error\n");
                return;
            }

            memcpy(&(p_env->codec_cfg.param), &(p_cfg->param), sizeof(bap_cfg_param_t));
            p_env->wait_codec_cfg = false;

            bap_bc_scan_pa_report_ctrl(pa_lid, 0);
            bap_bc_sink_enable_cfm(p_env->grp_lid, true, 1 << stream_pos, 1000, 0);
        }
    }
}

__STATIC void le_deleg_cb_scan_pa_sync_req(uint8_t pa_lid, uint8_t src_lid, uint8_t con_lid)
{
    APP_DBG("Synchronization with PA requested\n");
    // bap_bc_scan_pa_synchronize_cfm(pa_lid, true, 0, BAP_BC_SCAN_REPORT_ANNOUNCE_LVL_3_BIT, 500, 10);
    bap_bc_scan_pa_synchronize_cfm(pa_lid, true, 0, BAP_BC_SCAN_REPORT_MASK, 500, 10);
}

__STATIC void le_deleg_cb_scan_pa_terminate_req(uint8_t pa_lid, uint8_t con_lid)
{
    APP_DBG("End of synchronization with PA requested\n");
    bap_bc_scan_pa_terminate_cfm(pa_lid, true);
}

// Set of callback functions for communication with BAP Broadcast Scan module
__STATIC const bap_bc_scan_cb_t cbs_bap_bc_scan =
{
    .cb_cmp_evt = le_deleg_cb_scan_cmp_evt,
    .cb_timeout = le_deleg_cb_scan_timeout,
    .cb_report = le_deleg_cb_scan_report,
    #if 1//(GAF_PBP)
    .cb_public_bcast_source = le_deleg_cb_public_bcast_source,
    #endif //(GAF_PBP)
    .cb_pa_established = le_deleg_cb_scan_pa_established,
    .cb_pa_terminated = le_deleg_cb_scan_pa_terminated,
    .cb_pa_report = le_deleg_cb_scan_pa_report,
    .cb_big_info_report = le_deleg_cb_scan_big_info_report,
    .cb_group_report = le_deleg_cb_scan_group_report,
    .cb_subgroup_report = le_deleg_cb_scan_subgroup_report,
    .cb_stream_report = le_deleg_cb_scan_stream_report,
    .cb_pa_sync_req = le_deleg_cb_scan_pa_sync_req,
    .cb_pa_terminate_req = le_deleg_cb_scan_pa_terminate_req,
};

/*
 * FUNCTIONS
 ****************************************************************************************
 */
void le_bis_deleg_start_solicite(void)
{    
    bap_bc_adv_param_t adv_param = {
        .adv_intv_min_slot = 320,
        .adv_intv_max_slot = 320,
        .ch_map = ADV_ALL_CHNLS_EN,
        .phy_prim = GAPM_PHY_TYPE_LE_1M,
        .phy_second = GAPM_PHY_TYPE_LE_2M,
        .adv_sid = 1,
    };
    uint16_t status = bap_bc_deleg_start_solicite(0, &adv_param, 0);
    if (status)
    {
        APP_DBG("le_bis_deleg_start_solicite: failed status 0x%x\n", status);     
    }
}

void le_bis_deleg_configuration_cb_gapm_cmp(uint32_t event, uint16_t status)
{
    le_bis_deleg_configuration_state(event, status, NULL);
}

__STATIC void le_bis_deleg_configuration_state(uint16_t event, uint16_t status, const void* p_params)
{
    bool is_finished = false;
    APP_DBG("le_bis_deleg_configuration_state event: 0x%x\n", event);

	switch (event)
	{
		case EVENT_CONFIGURATION_DONE:
			gapm_set_name(EVENT_CONFIGURATION_NAME_SET, strlen("MV_BC_DELEG"), (const uint8_t*)"MV_BC_DELEG",
											le_bis_deleg_configuration_cb_gapm_cmp);
		 break;

		case EVENT_CONFIGURATION_NAME_SET:
			gapm_get_controller_bdaddr(EVENT_CONFIGURE_READ_CONTROLLER_ADDRESS,
										le_bis_deleg_configuration_cb_gapm_cmp);
			break;

		case EVENT_CONFIGURE_READ_CONTROLLER_ADDRESS:
			is_finished = true;
			break;

		default:  break;
	}

    if ( is_finished )
    {
    	le_bis_deleg_start_solicite();
    }
}

__STATIC void le_bis_deleg_configure(const void* p_params, component_cb_event cb_event)
{
    uint16_t status;
    bap_bc_deleg_cfg_t cfg = {
        .nb_srcs = 1,
        .cfg_bf = 0,
        .shdl = 0,
        .pref_mtu = 0,
    };

    p_env = (le_deleg_env_t*)ke_malloc_user(sizeof(le_deleg_env_t), KE_MEM_PROFILE);

    memset(p_env, 0, sizeof(le_deleg_env_t));
    p_env->grp_lid = GAF_INVALID_LID;

    status = bap_bc_deleg_configure(&cbs_bap_bc_deleg, &cfg);
    if (status)
    {
        APP_DBG("le_bis_deleg_configure: bap_bc_deleg_configure error 0x%x\n", status);
    }

	status = bap_bc_scan_configure(BAP_ROLE_SUPP_BC_SINK_BIT | BAP_ROLE_SUPP_BC_SCAN_BIT | BAP_ROLE_SUPP_BC_DELEG_BIT,     &cbs_bap_bc_scan);
    if (status)
    {
        APP_DBG("le_bis_deleg_configure: bap_bc_scan_configure error 0x%x\n", status);
    }

	status = bap_bc_sink_configure(BAP_ROLE_SUPP_BC_SINK_BIT | BAP_ROLE_SUPP_BC_SCAN_BIT | BAP_ROLE_SUPP_BC_DELEG_BIT,     &cbs_bap_bc_sink);    
    if (status)
    {
        APP_DBG("le_bis_deleg_configure: bap_bc_sink_configure error 0x%x\n", status);
    }

    app_le_link_configure();
}

void le_bis_delegator_init(void)
{
    utils_set_device_name("MV_BC_DELEG");
	le_bis_deleg_configure(NULL, NULL);
}
#endif

#endif
