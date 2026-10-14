/*
 * le_cis_sink.c
 *
 *  Created on: Apr 15, 2024
 *      
 */

#include "gapm.h"
#include "utils_cb_gap.h"
#include "bap.h"
#include "bap_uc.h"
#include "bap_uc_srv.h"
#include "le_audio_config.h"
#include "bt_app_ddb_info.h"
#include "debug.h"
#include "le_cis_sink.h"
#include "utils_audio.h"
#include "co_bt_defines.h"
#include "hl_error.h"
#include "tmap.h"
#include "app_le_audio_capabilities_server.h"
#include "app_le_common_audio_server.h"
#include "app_le_coordinated_set_member.h"
/*
 * TYPES DEFINITION
 ****************************************************************************************
 */

#ifdef LE_CIS_SINK

#define SIRK  ("!!!MV SIRK!!!!")


// ASE information structure
typedef struct le_cis_sink_ase
{
    // Pointer to Codec Configuration
    bap_cfg_t* p_codec_cfg;
    // Pointer to Metadata
    bap_cfg_metadata_t* p_metadata;
} le_cis_sink_ase_t;

// Environment structure
typedef struct le_cis_sink_env
{
    // Event callback function for backward communication with Mediator
    component_cb_event cb_mediator_event;
    // Connection handle
    uint16_t conhdl;

    // Bit field allowing to know if a procedure has been completed for an ASE
    uint8_t ase_done_bf;
    // Bit field allowing to mask ase_done_bf;
    uint8_t ase_done_mask;

    // ASE for which data path provider has been started
    uint8_t ase_lid_dp;

    app_sink_bond_data_t bond_data;
    // Pointer to Codec Configuration for the ASE
    le_cis_sink_ase_t ase_info[__ARRAY_EMPTY];
} le_cis_sink_env_t;

/*
 * LOCAL VARIABLES
 ****************************************************************************************
 */

// Pointer to environment
__STATIC le_cis_sink_env_t* p_env;  
extern uint8_t cis_src2sink_streaming;//ase0
extern uint8_t cis_sink2src_streaming;//ase1
extern uint16_t lc3_packet_len;

extern void app_le_coordinated_set_member_configure(const app_le_coordinated_set_member_cfg_t* p_params,
                                                      component_cb_event cb_event);

void app_le_common_audio_server_configure(const app_le_common_audio_server_cfg_t* p_params,
                                                   component_cb_event cb_event);

__STATIC void le_cis_sink_configuration_state(uint16_t event, uint16_t status, const void* p_params);

extern uint16_t app_gaf_client_configure(void);
/*
 * CALLBACK FUNCTIONS
 ****************************************************************************************
 */
__STATIC void le_cis_sink_cb_cmp_evt(uint8_t cmd_type, uint16_t status, uint8_t ase_lid)
{
    ASSERT_WARN(status == GAF_ERR_NO_ERROR, status, cmd_type);
}

__STATIC void le_cis_sink_cb_quality_cmp_evt(uint16_t status, uint8_t ase_lid,
                                                             uint32_t tx_unacked_packets, uint32_t tx_flushed_packets,
                                                             uint32_t tx_last_subevent_packets,
                                                             uint32_t retransmitted_packets,
                                                             uint32_t crc_error_packets,
                                                             uint32_t rx_unreceived_packets,
                                                             uint32_t duplicate_packets)
{
}

void load_ascs_sink_info(uint8_t conidx, ascs_bond_data_t* p_ascs_bond_data)
{
    p_env->bond_data.cli_cfg_bf = p_ascs_bond_data->cli_cfg_bf;
    p_env->bond_data.ase_cli_cfg_bf = p_ascs_bond_data->ase_cli_cfg_bf;

    APP_DBG("load_ascs_sink_info (conidx = %d, cli_cfg_bf = 0x%02X, ase_cli_cfg_bf = 0x%02X)\n",
                 conidx, p_env->bond_data.cli_cfg_bf, p_env->bond_data.ase_cli_cfg_bf);
}

app_sink_bond_data_t* get_ascs_sink_bond_data(uint8_t conidx)
{
    APP_DBG("get_ascs_sink_bond_data (conidx = %d, cli_cfg_bf = 0x%02X, ase_cli_cfg_bf = 0x%02X)\n",
                 conidx, p_env->bond_data.cli_cfg_bf, p_env->bond_data.ase_cli_cfg_bf);
    return &(p_env->bond_data);
}
bool save_sink_data = 0;
__STATIC void le_cis_sink_cb_bond_data(uint8_t conidx, uint8_t cli_cfg_bf, uint16_t ase_cli_cfg_bf)
{
    APP_DBG("ASCS Bond Data updated (conidx = %d, cli_cfg_bf = 0x%02X, ase_cli_cfg_bf = 0x%02X)\n",
                 conidx, cli_cfg_bf, ase_cli_cfg_bf);
    p_env->bond_data.cli_cfg_bf = cli_cfg_bf;
    p_env->bond_data.ase_cli_cfg_bf = ase_cli_cfg_bf;
    if ((!save_sink_data) && (3 == ase_cli_cfg_bf))
    {
        save_sink_data = 1;
        //save bond_data
        ble_save_bond_data();
    }
}

void le_cis_sink_restore_bond_data(uint8_t conidx)
{
    APP_DBG("le_cis_sink_restore_bond_data: 0x%x: 0x%x: 0x%x\n", 
                conidx, p_env->bond_data.cli_cfg_bf, p_env->bond_data.ase_cli_cfg_bf);
                
    uint16_t status = bap_uc_srv_restore_bond_data(conidx, p_env->bond_data.cli_cfg_bf,
                                                p_env->bond_data.ase_cli_cfg_bf, 0);
    if (status)
    {
        APP_DBG("bap_uc_srv_restore_bond_data: err 0x%x\n", status);
    }
}


__STATIC void le_cis_sink_cb_ase_state(uint8_t ase_lid, uint8_t conidx, uint8_t state,
                                                       bap_qos_cfg_t* p_qos_cfg)
{
    le_cis_sink_ase_t* p_ase_info = &(p_env->ase_info[ase_lid]);

    APP_DBG("ASE %d - state 0x%x (conidx = %d)\n", ase_lid, state, conidx);

    if (state == BAP_UC_ASE_STATE_IDLE)
    {
        p_env->ase_done_bf |= CO_BIT(ase_lid);

        #if (APP_CTRL)
        if (ase_lid == p_env->ase_lid_dp)
        {
            p_env->ase_lid_dp = GAF_INVALID_LID;
        }
        #endif //(APP_CTRL)

        if (p_ase_info->p_codec_cfg != NULL)
        {
            ke_free(p_ase_info->p_codec_cfg);
            p_ase_info->p_codec_cfg = NULL;
        }
    }
    else if (state == BAP_UC_ASE_STATE_QOS_CONFIGURED)
    {
        if (p_ase_info->p_metadata != NULL)
        {
            ke_free(p_ase_info->p_metadata);
            p_ase_info->p_metadata = NULL;
        }
    }
    else if (state == BAP_UC_ASE_STATE_STREAMING)
    {
        if(ase_lid == 1)
        {
            extern uint8_t cis_sink2_sync_ok ;
            cis_sink_2_data_clr();
            cis_sink2_sync_ok = 1;
        }
    }
    else if ((state == BAP_UC_ASE_STATE_DISABLING) || (state == BAP_UC_ASE_STATE_RELEASING))
    {
        if (!ase_lid)
        {
        	cis_src2sink_streaming = 0;
        }
        else if (ase_lid == 1)
        {
        	cis_sink2src_streaming = 0;
        }
    }
}

__STATIC void le_cis_sink_cb_cis_state(uint8_t stream_lid, uint8_t conidx, uint8_t ase_lid_sink,
                                                       uint8_t ase_lid_src, uint8_t cig_id, uint8_t cis_id,
                                                       uint16_t conhdl, gapi_ug_config_t* p_cig_cfg,
                                                       gapi_us_config_t* p_cis_cfg)
{
    APP_DBG("CIS %d - conhdl = 0x%04X / cig_id = %d / cis_id = %d (conidx = %d)\n", stream_lid, conhdl, cig_id,
                 cis_id, conidx);

    #if (APP_CTRL)
    p_env->conhdl = conhdl;
    #endif //(APP_CTRL)
}

__STATIC bool set_codec_cfg(const gaf_codec_id_t* p_codec_id, const bap_cfg_ptr_t* p_cfg)
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
                            if (Get_cis_app() == CIS_APP_HEADSET)
                            {
                                lc3_channel = LC3_STEREO;
                            }
                            else if (Get_cis_app() == CIS_APP_EARBUD)
                            {
                                lc3_channel = LC3_MONO;
                            }

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
    APP_DBG("set_codec_cfg 0x%x, 0x%lx, 0x%lx, 0x%lx, 0x%x\n", 
                is_vaild, (unsigned long)lc3_packet_len, (unsigned long)lc3_bitrate, 
                (unsigned long)lc3_frame_len, lc3_channel);
    return is_vaild;
} 

__STATIC void le_cis_sink_cb_configure_codec_req(uint8_t conidx, uint8_t ase_instance_idx,
                                                                 uint8_t ase_lid, uint8_t tgt_latency,
                                                                 uint8_t tgt_phy, gaf_codec_id_t* p_codec_id,
                                                                 const bap_cfg_ptr_t* p_cfg)
{
    printf("le_cis_sink_cb_configure_codec_req: ase_lid %d\n", ase_lid);
    uint16_t size = sizeof(bap_cfg_t) + ((p_cfg->p_add_cfg != NULL) ? p_cfg->p_add_cfg->len : 0);
    bap_cfg_t* p_ase_codec_cfg;
    uint8_t ase_lid_cfm  = ase_instance_idx;
    bap_qos_req_t qos_req = {
        .pres_delay_min_us = 7500,
        .pres_delay_max_us = 40000,
        .pref_pres_delay_min_us = 7500,
        .pref_pres_delay_max_us = 40000,
        .trans_latency_max_ms = 30,
        .framing = 0,
        .phy_bf = 3,
        .retx_nb = (tgt_latency == BAP_UC_TGT_LATENCY_LOWER) ? 5 : 13,
    };

    uint8_t dp_id;
    if (Get_cis_app() == CIS_APP_EARBUD)
    {
        if (Get_sink_audio_loc() == GAF_LOC_FRONT_LEFT_BIT)
        {
            dp_id = CIS_SINK_DATA_PATH_ID;
        }
        else if (Get_sink_audio_loc() == GAF_LOC_FRONT_RIGHT_BIT)
        {
            dp_id = CIS_SINK_DATA_PATH_ID_2;        
        }
    }
    else if (Get_cis_app() == CIS_APP_HEADSET)
    {
        if (ase_instance_idx == 0)
        {
            dp_id = CIS_SINK_DATA_PATH_ID; 
        }
        else if (ase_instance_idx == 1)
        {
            dp_id = CIS_SINK_DATA_PATH_ID_2;        
        }
    }

    if (!set_codec_cfg(p_codec_id, p_cfg))
    {
        printf("baikal: cis err codec cfg\n");
        return;
    }
    APP_DBG("Configure Codec requested (ASE instance %d)\n", ase_instance_idx);

    p_ase_codec_cfg = (bap_cfg_t*)ke_malloc_user(size, KE_MEM_PROFILE);

    if (p_ase_codec_cfg != NULL)
    {
        le_cis_sink_ase_t* p_ase_info = &(p_env->ase_info[ase_lid_cfm]);

        APP_DBG("Codec ID = %02X:%02X:%02X:%02X:%02X\n", p_codec_id->codec_id[0], p_codec_id->codec_id[1],
                     p_codec_id->codec_id[2], p_codec_id->codec_id[3], p_codec_id->codec_id[4]);
        APP_DBG("Freq = 0x%x, Duration = 0x%x, Length = %d, Location = 0x%04X, Channels = %d\n",
                     p_cfg->param.sampling_freq, p_cfg->param.frame_dur,
                     p_cfg->param.frame_octet, p_cfg->param.location_bf, p_cfg->param.frames_sdu);

        // if (p_cfg->param.sampling_freq == BAP_SAMPLING_FREQ_48000HZ)
        // {
        //     Set_cis_mode(CIS_MODE_MEDIA);
        // }
        // else if (p_cfg->param.sampling_freq == BAP_SAMPLING_FREQ_16000HZ)
        // {
        //     Set_cis_mode(CIS_MODE_CONVERSATION);
        // }
        // else
        // {
        //     //TODO:
        // }

        if (p_ase_info->p_codec_cfg != NULL)
        {
            ke_free(p_ase_info->p_codec_cfg);
        }

        p_ase_info->p_codec_cfg = p_ase_codec_cfg;
        memcpy(&(p_ase_codec_cfg->param), &(p_cfg->param), sizeof(bap_cfg_param_t));
        if (p_cfg->p_add_cfg != NULL)
        {
            p_ase_codec_cfg->add_cfg.len = p_cfg->p_add_cfg->len;
            memcpy(&(p_ase_codec_cfg->add_cfg.data[0]), &(p_cfg->p_add_cfg->data[0]), p_cfg->p_add_cfg->len);
        }
        else
        {
            p_ase_codec_cfg->add_cfg.len = 0;
        }

        p_env->ase_done_mask |= CO_BIT(ase_lid_cfm);
        
        #if (APP_CTRL)
        if (ase_instance_idx == 0)
        {
        	p_env->ase_lid_dp = ase_lid_cfm;
        }
        #endif //(APP_CTRL)
    }

    bap_uc_srv_configure_codec_cfm(conidx,
                                   (p_ase_codec_cfg != NULL)
                                            ? BAP_UC_CP_RSP_CODE_SUCCESS : BAP_UC_CP_RSP_CODE_INSUFFICIENT_RESOURCES,
                                   0, ase_lid_cfm, &qos_req, p_ase_codec_cfg, 0, dp_id);
}

__STATIC void le_cis_sink_cb_configure_qos_req(uint8_t ase_lid, uint8_t stream_lid,
                                                               const bap_qos_cfg_t* p_qos_cfg)
{
	// void LeAudioSinkModeEnter(void);
	// if(ase_lid == 0)
	// {
		LeAudioSinkModeEnter();
	// }

    APP_DBG("Configure QoS requested (ASE %d)\n", ase_lid);
    bap_uc_srv_configure_qos_cfm(ase_lid, BAP_UC_CP_RSP_CODE_SUCCESS, 0);
}

__STATIC void le_cis_sink_cb_enable_req(uint8_t ase_lid, bap_cfg_metadata_ptr_t* p_metadata)
{
    uint16_t size = sizeof(bap_cfg_metadata_t)
                            + ((p_metadata->p_add_metadata != NULL) ? p_metadata->p_add_metadata->len : 0);
    bap_cfg_metadata_t* p_ase_metadata = (bap_cfg_metadata_t*)ke_malloc_user(size, KE_MEM_PROFILE);

    APP_DBG("Enable requested (ASE %d)\n", ase_lid);

    if (p_ase_metadata != NULL)
    {
        le_cis_sink_ase_t* p_ase_info = &(p_env->ase_info[ase_lid]);

        ASSERT_WARN(p_ase_info->p_metadata == NULL, ase_lid, 0);

        memset(p_ase_metadata, 0, sizeof(bap_cfg_metadata_t));
        p_ase_info->p_metadata = p_ase_metadata;
        p_ase_metadata->param.context_bf = p_metadata->param.context_bf;
        if (p_metadata->p_add_metadata != NULL)
        {
            memcpy(&(p_ase_metadata->add_metadata), p_metadata->p_add_metadata,
                   sizeof(gaf_ltv_t) + p_metadata->p_add_metadata->len);
        }
    }

    bap_uc_srv_enable_cfm(ase_lid, (p_ase_metadata != NULL)
                                        ? BAP_UC_CP_RSP_CODE_SUCCESS : BAP_UC_CP_RSP_CODE_INSUFFICIENT_RESOURCES,
                          0, p_ase_metadata);
}

__STATIC void le_cis_sink_cb_update_metadata_req(uint8_t ase_lid,
                                                                 bap_cfg_metadata_ptr_t* p_metadata)
{
    uint16_t size = sizeof(bap_cfg_metadata_t)
                            + ((p_metadata->p_add_metadata != NULL) ? p_metadata->p_add_metadata->len : 0);
    bap_cfg_metadata_t* p_ase_metadata = (bap_cfg_metadata_t*)ke_malloc_user(size, KE_MEM_PROFILE);

    APP_DBG("Update metadata requested (ASE %d)\n", ase_lid);

    if (p_ase_metadata != NULL)
    {
        le_cis_sink_ase_t* p_ase_info = &(p_env->ase_info[ase_lid]);

        ASSERT_WARN(p_ase_info->p_metadata != NULL, ase_lid, 0);
        ke_free(p_ase_info->p_metadata);

        memset(p_ase_metadata, 0, sizeof(bap_cfg_metadata_t));
        p_ase_info->p_metadata = p_ase_metadata;
        p_ase_metadata->param.context_bf = p_metadata->param.context_bf;
        if (p_metadata->p_add_metadata != NULL)
        {
            memcpy(&(p_ase_metadata->add_metadata), p_metadata->p_add_metadata,
                   sizeof(gaf_ltv_t) + p_metadata->p_add_metadata->len);
        }
    }

    bap_uc_srv_update_metadata_cfm(ase_lid, (p_ase_metadata != NULL)
                                        ? BAP_UC_CP_RSP_CODE_SUCCESS : BAP_UC_CP_RSP_CODE_INSUFFICIENT_RESOURCES,
                                   0, p_ase_metadata);
}

__STATIC void le_cis_sink_cb_release_req(uint8_t ase_lid)
{
    APP_DBG("Release requested (ASE %d)\n", ase_lid);
    bap_uc_srv_release_cfm(ase_lid, BAP_UC_CP_RSP_CODE_SUCCESS, 0, true);
}

__STATIC void le_cis_sink_cb_dp_update_req(uint8_t ase_lid, bool start)
{
    APP_DBG("start 0x%x data path requested (ASE %d)\n", start, ase_lid);

    if (start)
    {
        if (!ase_lid)//sink ase
        {
        	cis_src2sink_streaming = 1;
        }
        else if (ase_lid == 1)//src ase
        {
#ifdef LE_CIS_CONVERSATION
        	cis_sink2src_streaming = 1;
            LeAudioSerEncodeInit();
#endif
        }
    }

    bap_uc_srv_dp_update_cfm(ase_lid, start);
}

// Set of callback functions for communication with BAP Unicast Server module
__STATIC const bap_uc_srv_cb_t cbs_bap_uc_srv = {
    .cb_cmp_evt = le_cis_sink_cb_cmp_evt,
    .cb_quality_cmp_evt = le_cis_sink_cb_quality_cmp_evt,
    .cb_bond_data = le_cis_sink_cb_bond_data,
    .cb_ase_state = le_cis_sink_cb_ase_state,
    .cb_cis_state = le_cis_sink_cb_cis_state,
    .cb_configure_codec_req = le_cis_sink_cb_configure_codec_req,
    .cb_configure_qos_req = le_cis_sink_cb_configure_qos_req,
    .cb_enable_req = le_cis_sink_cb_enable_req,
    .cb_update_metadata_req = le_cis_sink_cb_update_metadata_req,
    .cb_release_req = le_cis_sink_cb_release_req,
    .cb_dp_update_req = le_cis_sink_cb_dp_update_req,
};

/*
 * FUNCTIONS
 ****************************************************************************************
 */
void le_cis_sink_configuration_cb_gapm_cmp(uint32_t event, uint16_t status)
{
    le_cis_sink_configuration_state(event, status, NULL);
}

__STATIC void le_cis_sink_configuration_state(uint16_t event, uint16_t status, const void* p_params)
{
    bool is_finished = false;
    BT_DBG("le_cis_sink_configuration_state event: 0x%x\n", event);

	switch (event)
	{
		case EVENT_CONFIGURATION_DONE:
			gapm_set_name(EVENT_CONFIGURATION_NAME_SET, strlen("MV_CIS_SINK"), (const uint8_t*)"MV_CIS_SINK",
											le_cis_sink_configuration_cb_gapm_cmp);
		 break;

		case EVENT_CONFIGURATION_NAME_SET:
			gapm_get_controller_bdaddr(EVENT_CONFIGURE_READ_CONTROLLER_ADDRESS,
										le_cis_sink_configuration_cb_gapm_cmp);
			break;

		case EVENT_CONFIGURE_READ_CONTROLLER_ADDRESS:
			is_finished = true;
			break;

		default:  break;
	}

    if ( is_finished )
    {

        // if (Get_cis_app() == CIS_APP_HEADSET)
        {
            app_le_adv_start(NULL);
        }
    }
}

void app_le_pacs_configure(app_le_audio_capabilities_server_cfg_t* p_audio_capabilities_server_cfg)
{
    app_le_audio_capabilities_server_configure(p_audio_capabilities_server_cfg, NULL);
}

__STATIC void le_cis_sink_configure(const le_cis_sink_cfg_t* p_params, component_cb_event cb_event)
{
    uint16_t status;

    bap_uc_srv_cfg_t uc_srv_cfg;
    utils_audio_le_cfg_t* p_le_audio_cfg = utils_audio_le_get_cfg_local();
    uint16_t size;

    ASSERT_ERR(p_env == NULL);
    ASSERT_ERR(p_params != NULL);

    uc_srv_cfg.nb_ase_chars_sink = p_params->nb_ases_sink;
    uc_srv_cfg.nb_ase_chars_src = p_params->nb_ases_src;
    uc_srv_cfg.nb_ases_cfg = uc_srv_cfg.nb_ase_chars_sink + uc_srv_cfg.nb_ase_chars_src;
    uc_srv_cfg.cfg_bf = 0;
    uc_srv_cfg.pref_mtu = 0;
    uc_srv_cfg.shdl = 0;

    size = sizeof(le_cis_sink_env_t)
                    + ((p_params->nb_ases_sink + p_params->nb_ases_src) * sizeof(le_cis_sink_ase_t));
    p_env = (le_cis_sink_env_t*)ke_malloc_user(size, KE_MEM_PROFILE);
    ASSERT_ERR(p_env != NULL);
    memset(p_env, 0, size);
    p_env->ase_lid_dp = GAF_INVALID_LID;

    p_env->cb_mediator_event = cb_event;

    p_le_audio_cfg->nb_ases_sink = p_params->nb_ases_sink;
    p_le_audio_cfg->nb_ases_src = p_params->nb_ases_src;

    // Add Audio Stream Control Service
    status = bap_uc_srv_configure(&cbs_bap_uc_srv, &uc_srv_cfg);
    if (status)
    {
        APP_DBG("le_cis_sink_configure: bap_uc_srv_configure error 0x%x\n", status);
    }
}

__STATIC void le_cis_sink_cb_event_coordinated_set_member(uint16_t type, uint16_t status, const void* p_params)
{
    APP_DBG("le_cis_sink_cb_event_coordinated_set_member: type 0x%x, status 0x%x\n", type, status);
    if (status == GAP_ERR_NO_ERROR)
    {
        switch (type)
        {
            case APP_LE_COORDINATED_SET_MEMBER_EVENT_ADDED:
            {
                app_le_common_audio_server_cfg_t cfg_common_audio_server = {
                    .set_lid = 0,
                };

                app_le_common_audio_server_configure(&cfg_common_audio_server, NULL);
                app_le_adv_start(NULL);

            } break;

            default: { } break;
        }
    }
}

__STATIC void applet_configuration_cb_hw_error(uint32_t metainfo, uint8_t code)
{
    printf("Hardware Error (0x%02X)\n", code);
}

void le_cis_sink_init(uint8_t cis_app, uint8_t sink_loc)
{
//    gapm_config_t config = CONFIGURATION;
    le_cis_sink_cfg_t audio_unicast_server_cfg;
    app_le_audio_capabilities_server_cfg_t audio_capabilities_server_cfg;
    uint16_t status;

    switch (cis_app)
    {
        case CIS_APP_HEADSET:
        {
            audio_unicast_server_cfg.nb_ases_sink = 2;
            audio_unicast_server_cfg.nb_ases_src = 1;

            audio_capabilities_server_cfg.location_bf_sink = GAF_LOC_FRONT_LEFT_BIT | GAF_LOC_FRONT_RIGHT_BIT;
            audio_capabilities_server_cfg.location_bf_src = GAF_LOC_FRONT_LEFT_BIT;  
            audio_capabilities_server_cfg.supp_context_bf_sink = BAP_CONTEXT_TYPE_MEDIA_BIT | BAP_CONTEXT_TYPE_RINGTONE_BIT
                                                                    | BAP_CONTEXT_TYPE_CONVERSATIONAL_BIT;
            audio_capabilities_server_cfg.supp_context_bf_src = BAP_CONTEXT_TYPE_CONVERSATIONAL_BIT;    
            audio_capabilities_server_cfg.nb_frames_per_sdu_max = 1;
            audio_capabilities_server_cfg.channel_cnt_bf = 1;       
                               
        } break;

        case CIS_APP_EARBUD:
        {
            audio_unicast_server_cfg.nb_ases_sink = 1;
            audio_unicast_server_cfg.nb_ases_src = 1;

            audio_capabilities_server_cfg.location_bf_sink = sink_loc;
            audio_capabilities_server_cfg.location_bf_src = GAF_LOC_FRONT_LEFT_BIT;  
            audio_capabilities_server_cfg.supp_context_bf_sink = BAP_CONTEXT_TYPE_MEDIA_BIT | BAP_CONTEXT_TYPE_RINGTONE_BIT
                                                                    | BAP_CONTEXT_TYPE_CONVERSATIONAL_BIT;
            audio_capabilities_server_cfg.supp_context_bf_src = BAP_CONTEXT_TYPE_CONVERSATIONAL_BIT;    
            audio_capabilities_server_cfg.nb_frames_per_sdu_max = 1;
            audio_capabilities_server_cfg.channel_cnt_bf = 1;   
        } break;

        default:
            break;
    }

    Set_sink_audio_loc(audio_capabilities_server_cfg.location_bf_sink);

    app_le_media_client_configure(NULL, NULL);

    app_le_call_client_configure(NULL, NULL);
	le_cis_sink_configure(&audio_unicast_server_cfg, NULL);
    uint16_t role_bf = TMAP_ROLE_UMR_BIT | TMAP_ROLE_BMR_BIT | TMAP_ROLE_CT_BIT;
    app_le_tmap_tmas_cfg(role_bf);
    app_le_pacs_configure(&audio_capabilities_server_cfg);

    if (cis_app == CIS_APP_EARBUD)
    {
        app_le_coordinated_set_member_cfg_t coordinated_set_member_cfg = {
            .p_sirk = (const uint8_t*)SIRK,
            .nb_members = CIS_SINK_NB_MEM,
            .rank = cis_app_get_rank(),
            .lock_timeout_s = 30,
        };

        if (!coordinated_set_member_cfg.rank)
        {
            APP_DBG("err rank is 0\n");
            return;
        }

        app_le_coordinated_set_member_configure(&coordinated_set_member_cfg, le_cis_sink_cb_event_coordinated_set_member);
    }
    app_le_volume_renderer_configure(NULL, NULL);
    status = app_gaf_client_configure();
    if (status)
    {
        APP_DBG("le_cis_sink_init: app_gaf_client_configure err 0x%x\n", status);
    }

    utils_set_device_name("MV_cis_sink_baikal");

#if 0//ndef LE_UDSS
    const gapm_cb_t cb_gapm_config = {
        .cb_hw_error = applet_configuration_cb_hw_error,
    };
    utils_cb_gap_register_gapm_config(&cb_gapm_config);

    utils_init();
	// status = gapm_configure(EVENT_CONFIGURATION_DONE, &config, app_gap_cbs_itf_get(), le_cis_sink_configuration_cb_gapm_cmp);
	// if (status)
	// {
	// 	printf("le_cis_sink_configure: gapm_configure error 0x%x\n", status);
	// }
#endif
}
#endif


