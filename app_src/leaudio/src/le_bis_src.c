#include <string.h>
#include "le_audio_config.h"
#include "le_public.h"
#include "le_bis_src.h"
#include "bap.h"
#include "bap_bc_src.h"
#include "debug.h"
#include "bt_config.h"
#include "le_audio_config.h"
#include "app_message.h"
#include "bt_manager.h"
#ifdef DISCOV_BIS
#include "bap_bc_annou.h"
#endif

extern uint8_t sink_need_restart_udss_adv;

// 函数声明
extern const char* utils_get_device_name(void);
extern void LeAudioSerEncodeInit(void);
extern void bis_sync_audio_close(void);
extern void clear_and_send_pa_disable_status(void);
extern void bis_src_pd_delay_clear(void);
// extern void LeAudioSrcStartCmd(void);
extern void le_bis_src1_in_datapath_init(void);
extern void LeAudioSrcStopCmd(void);
extern void set_bis_pres_delay_us(uint32_t pres_delay_us);

#if (BT_LE_AUDIO_SUPPORT)//#ifdef LE_BIS_SOURCE
#define NUM_BIS_1      (1)
#define NUM_BIS_2      (2)
#define BCAST_ID_LEN   (3)
uint8_t g_bcast_id[BCAST_ID_LEN] = {0};

// uint8_t bcast_code[16] = {1, 2, 3, 0xfe, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 13};
uint8_t bcast_code[16] = {0};//no encry
uint8_t g_vendor_data[VENDOR_DATA_LEN];

extern void lli_bis_vendor_data(uint32_t vendor_data);
extern void set_pa_disable_status(uint8_t status);
uint8_t* get_bcast_id(void);
uint8_t g_volume = 0x20;//0x15;
/*
 * ENUMERATIONS
 ****************************************************************************************
 */

// Status bit field meaning
enum le_bis_source_status_bf
{
    //Indicate if bis src is cfged or not - Position
    STATUS_GROUP_CFG_POS = 0,
    // Indicate if Group is cfged or not - Bit
    STATUS_GROUP_CFG_BIT = CO_BIT(STATUS_GROUP_CFG_POS),

    // Indicate if PA is enabled or not - Position
    STATUS_GROUP_PA_ENABLED_POS = 1U,
    // Indicate if PA is enabled or not - Bit
    STATUS_GROUP_PA_ENABLED_BIT = CO_BIT(STATUS_GROUP_PA_ENABLED_POS),

    // Indicate if Group is enabled or not - Position
    STATUS_GROUP_ENABLED_POS = 2U,
    // Indicate if Group is enabled or not - Bit
    STATUS_GROUP_ENABLED_BIT = CO_BIT(STATUS_GROUP_ENABLED_POS),

    // Indicate if Group is streaming or not - Position
    STATUS_GROUP_STREAM_POS = 3U,
    // Indicate if Group is streaming or not - Bit
    STATUS_GROUP_STREAM_BIT = CO_BIT(STATUS_GROUP_STREAM_POS),
};

/*
 * TYPES DEFINITION
 ****************************************************************************************
 */


// Stream configuration structure
typedef struct le_bis_source_stream_cfg
{
    // Stream configuration name
    const char* p_cfg_name;
    // Configuration
    bap_bc_cfg_t cfg;
} le_bis_source_stream_cfg_t;

// Environment structure
typedef struct le_bis_source_env
{
    // Pointer to Input Provider interface
  //  const input_prov_intf_t* p_input_intf;
    // General configuration
    le_bis_source_cfg_t cfg;
    // Subgroup configuration
    bap_cfg_t subgroup_cfg;
    bap_cfg_t bis_cfg[2];  
    // Metadata configuration
    bap_cfg_metadata_t metadata;
    // Group local index
    uint8_t grp_lid;
    // Status bit field (see #applet_broadcast_source_status_bf enumeration)
    uint8_t status_bf;
#ifdef DISCOV_BIS
    bap_bc_adv_param_t adv_param;
    bap_bc_per_adv_param_t per_adv_param;
    uint8_t path_loss_threshold;
    int8_t  radiated_tx_power;
#endif
} le_bis_source_env_t;

le_bis_source_cfg_t broadcast_source_cfg;


#ifdef USE_PTO
#define BAP_BC_CFG_TEST             { .pres_delay_us = 40000,                   \
                                      .sdu_intv_us = 20000,                      \
                                      .samp_freq = BAP_SAMPLING_FREQ_48000HZ,    \
                                      .framing = ISO_UNFRAMED_MODE,             \
                                      .max_sdu = LE_BIS_LC3_PACKET_LEN + VENDOR_DATA_LEN,         \
                                      .rtn = 4,                                 \
                                      .max_tlatency_ms = 14 }
#else
#define BAP_BC_CFG_TEST             { .pres_delay_us = 40000,                   \
                                      .sdu_intv_us = 20000,                      \
                                      .samp_freq = BAP_SAMPLING_FREQ_48000HZ,    \
                                      .framing = ISO_UNFRAMED_MODE,             \
                                      .max_sdu = 250,         \
                                      .rtn = 4,                                 \
                                      .max_tlatency_ms = 14 }
#endif//USE_PTO

#define BAP_BC_CFG_BIS1             { .pres_delay_us = 60000/*40000*/,                   \
                                      .sdu_intv_us = 10000,                      \
                                      .samp_freq = BAP_SAMPLING_FREQ_48000HZ,    \
                                      .framing = ISO_UNFRAMED_MODE,             \
                                      .max_sdu = 250,         \
                                      .rtn = 2,                                 \
                                      .max_tlatency_ms = 21 }

__STATIC void bis_src_configuration_state(uint16_t event, uint16_t status, const void* p_params);

// Pointer to environment
__STATIC le_bis_source_env_t* p_env;

__STATIC void bis_src_configuration_cb_gapm_name_get(uint8_t conidx, uint32_t metainfo, uint16_t token,
                                                    uint16_t offset, uint16_t max_length) __attribute__((unused));
__STATIC void bis_src_configuration_cb_gapm_name_get(uint8_t conidx, uint32_t metainfo, uint16_t token,
                                                    uint16_t offset, uint16_t max_length)
{
    const char* p_device_name = utils_get_device_name();
    uint16_t length = strlen(p_device_name);

    gapc_le_get_name_cfm(conidx, token, 0, length, length, (const uint8_t*)p_device_name);
}

__STATIC void bis_src_configuration_cb_gapm_appearance_get(uint8_t conidx, uint32_t metainfo, uint16_t token) __attribute__((unused));
__STATIC void bis_src_configuration_cb_gapm_appearance_get(uint8_t conidx, uint32_t metainfo, uint16_t token)
{

    gapc_le_get_appearance_cfm(conidx, token, 0, 0);
}

__STATIC void bis_src_configuration_cb_gapm_appearance_set(uint8_t conidx, uint32_t metainfo, uint16_t token,
                                                          uint16_t appearance) __attribute__((unused));
__STATIC void bis_src_configuration_cb_gapm_appearance_set(uint8_t conidx, uint32_t metainfo, uint16_t token,
                                                          uint16_t appearance)
{
    gapm_le_set_appearance(appearance);
    gapc_le_set_appearance_cfm(conidx, token, 0);
}

#ifdef DISCOV_BIS
void le_bis_annou_enable_pa(void)
{
    gap_bdaddr_t src_addr;
    uint16_t status;

    src_addr.addr_type = 0;
    src_addr.addr[0] = btManager.btDevAddr[5];
    src_addr.addr[1] = btManager.btDevAddr[4];
    src_addr.addr[2] = btManager.btDevAddr[3];
    src_addr.addr[3] = btManager.btDevAddr[2];
    src_addr.addr[4] = btManager.btDevAddr[1];
    src_addr.addr[5] = btManager.btDevAddr[0];

    bap_bc_adv_param_t adv_param = {
        .adv_intv_min_slot = 40,
        .adv_intv_max_slot = 40,
        .ch_map = ADV_ALL_CHNLS_EN,
        .phy_prim = GAPM_PHY_TYPE_LE_1M,
        .phy_second = GAPM_PHY_TYPE_LE_2M, //GAPM_PHY_TYPE_LE_2M, //GAPM_PHY_TYPE_LE_1M, //GAPM_PHY_TYPE_LE_2M,
        .adv_sid = 1,
        .max_tx_pwr = 127,//-70,
        .own_addr_type = GAPM_GEN_RSLV_ADDR,//GAPM_STATIC_ADDR,
        .max_skip = 0,
        .send_tx_pwr = false,
    };
    bap_bc_per_adv_param_t per_adv_param = {
        .adv_intv_min_frame = 80,
        .adv_intv_max_frame = 80,
    };
    printf("bap_bc_annou_enable_pa: meta_len %d\n", sizeof(p_env->metadata));
    status = bap_bc_annou_enable_pa(&adv_param, &per_adv_param, 1, 0, NULL,
                                        &src_addr, 0, 0, NULL, NULL, sizeof(p_env->metadata), &(p_env->metadata));
    if (status)
    {
        APP_DBG("bap_bc_src_enable_pa error 0x%x\n", status);
    }   
}
#endif

//bap_bc_src的事件返回
__STATIC void le_bis_source_cb_cmp_evt(uint8_t cmd_type, uint16_t status, uint8_t grp_lid, uint8_t sgrp_lid)
{
    ASSERT_WARN(status == GAF_ERR_NO_ERROR, status, cmd_type);

    APP_DBG("le_bis_source_cb_cmp_evt: cmd_type 0x%x, 0x%x\n", cmd_type, status);
    switch (cmd_type)
    {
        case BAP_BC_SRC_CMD_TYPE_ENABLE_PA:
        {
            if (status)
            {
                APP_DBG("BAP_BC_SRC_CMD_TYPE_ENABLE_PA error 0x%x\n", status);
            }
            else
            {
                SETB(p_env->status_bf, STATUS_GROUP_PA_ENABLED, 1);
                status = bap_bc_src_enable(p_env->grp_lid);
                if (status)
                {
                    APP_DBG("bap_bc_src_enable error 0x%x\n", status);                
                }
            }
        }
            break;

        case BAP_BC_SRC_CMD_TYPE_ENABLE:
        {
            if (status) 
            {
                APP_DBG("BAP_BC_SRC_CMD_TYPE_ENABLE error 0x%x\n", status);
            }
            else
            {
                SETB(p_env->status_bf, STATUS_GROUP_ENABLED, 1);
                status = bap_bc_src_start_streaming(p_env->grp_lid, 0xFFFFFFFF);//在这个函数调用里会配置ISO_DATAPATH，在使能广播后
                if (status)
                {
                    APP_DBG("bap_bc_src_start_streaming error 0x%x\n", status);
                }
            }
        } break;
    
        case BAP_BC_SRC_CMD_TYPE_START_STREAMING:
        {
            if (status)
            {
                APP_DBG("BAP_BC_SRC_CMD_TYPE_START_STREAMING error 0x%x\n", status); 
            }
            else
            {
                SETB(p_env->status_bf, STATUS_GROUP_STREAM, 1);                

                LeAudioSerEncodeInit();
                bis_sync_audio_close();
#if defined(LE_BIS_ASSIS) && defined(ASSIS_ADD_SRC_LOCAL)
                app_add_src_local();
#endif
            }
#ifdef LE_UDSS
			status = app_start_le_adv();
            if (status)
            {
                APP_DBG("app_start_le_adv error 0x%x\n", status);                 
            }
#endif    
#ifdef DISCOV_BIS    
            Set_discov_bis(1);    
            le_bis_annou_enable_pa();
#endif
        } break;

        case BAP_BC_SRC_CMD_TYPE_STOP_STREAMING:
        {
            if (status)
            {
                APP_DBG("BAP_BC_SRC_CMD_TYPE_STOP_STREAMING error 0x%x\n", status); 
            }
            else
            {
                SETB(p_env->status_bf, STATUS_GROUP_STREAM, 0); 
                status = bap_bc_src_disable(p_env->grp_lid, true);
                if (status)
                {
                    APP_DBG("bap_bc_src_disable: error 0x%x\n", status);
                }
            }
        } break;

        case BAP_BC_SRC_CMD_TYPE_DISABLE:
        {
            if (status)
            {
                APP_DBG("BAP_BC_SRC_CMD_TYPE_DISABLE error 0x%x\n", status);                 
            }
            else
            {
                SETB(p_env->status_bf, STATUS_GROUP_ENABLED, 0);
                clear_and_send_pa_disable_status();

                bis_last_offset_reset();
                // bis_src_pd_delay_clear();
                status = bap_bc_src_remove_group(p_env->grp_lid);
                if (status)
                {
                    APP_DBG("bap_bc_src_remove_group: error 0x%x\n", status);
                }
            }
        }
            break;

        case BAP_BC_SRC_CMD_TYPE_REMOVE_GROUP:
        {
            if (status)
            {
                APP_DBG("BAP_BC_SRC_CMD_TYPE_REMOVE_GROUP error 0x%x\n", status);    
            }
            else
            {
                SETB(p_env->status_bf, STATUS_GROUP_CFG, 0); 
                p_env->grp_lid = GAF_INVALID_LID;
                // bis_src_pd_delay_clear();
                bis_last_offset_reset();
#ifdef DISCOV_BIS
                bap_bc_annou_disable_pa();
#endif
            }

        } break;

        case BAP_BC_SRC_CMD_TYPE_DISABLE_PA:
        {
             if (status)
            {
                APP_DBG("BAP_BC_SRC_CMD_TYPE_DISABLE_PA error 0x%x\n", status);
            }
            else
            {
                SETB(p_env->status_bf, STATUS_GROUP_PA_ENABLED, 0);
                set_pa_disable_status(0xFF);
            }
        } break;

        default: { } break;
    }
}

__STATIC void le_bis_source_cb_info(uint8_t grp_lid, const gapi_bg_config_t* p_bg_cfg, uint8_t nb_bis,
                                              const uint16_t* p_conhdl)
{

}

// Set of callback functions for communication for BAP Broadcast Source module
__STATIC const bap_bc_src_cb_t cbs_bap_bc_src =
{
    .cb_cmp_evt = le_bis_source_cb_cmp_evt,
    .cb_info    = le_bis_source_cb_info,
};

__STATIC void bis_src_configuration_cb_gapm_bdaddr(uint32_t event, uint16_t status, const gap_addr_t* p_addr)
{
	bis_src_configuration_state(event, status, p_addr);
}

void bis_src_configuration_cb_gapm_cmp(uint32_t event, uint16_t status)
{
	bis_src_configuration_state(event, status, NULL);
}

void app_bis_send_vendor_cmd(uint32_t vendor_data)
{
    lli_bis_vendor_data(vendor_data);
}

void le_bis_src_stop_pa(void)
{
    bap_bc_src_disable_pa(p_env->grp_lid);
}

__STATIC void le_bis_source_configure_group(void)
{
    uint16_t status;
    uint8_t cnt = 0;  // Initialize to safe default
    const bap_bc_cfg_t* p_cfg;
    extern bool get_src_wed_vaild(void);
    extern void LeAudioSetSrcFramesSdu(uint8_t frames_sdu);
    extern uint16_t LeAudioGetSrcSampleRate(void);

    if (get_src_wed_vaild())
    {
        bap_bc_cfg_t config1 = BAP_BC_CFG_BIS1;
        p_cfg = &config1;

        printf("cfg: %d, %d\n", p_cfg->samp_freq, config1.samp_freq);
    }
    else
    {
        bap_bc_cfg_t config2 = BAP_BC_CFG_TEST;
        p_cfg = &config2;
        printf("cfg: %d, %d\n", p_cfg->samp_freq, config2.samp_freq);
    }
    set_bis_pres_delay_us(p_cfg->pres_delay_us);

    bap_bc_grp_param_t grp_param = {
        .sdu_intv_us = p_cfg->sdu_intv_us,
        .max_sdu = p_cfg->max_sdu,
        .max_tlatency_ms = p_cfg->max_tlatency_ms,
        .packing = 1,//0,//ISO_PACKING_SEQUENTIAL(0)
        .framing = p_cfg->framing,
        .phy_bf = GAPM_PHY_TYPE_LE_2M,// GAPM_PHY_TYPE_LE_2M, //GAPM_PHY_TYPE_LE_1M, //GAPM_PHY_TYPE_LE_2M,
        .rtn = p_cfg->rtn,
    };
#ifdef LE_AUDIO_ENC_SBC
    gaf_codec_id_t codec_id = GAF_CODEC_ID_SBC;
#endif
#ifdef LE_AUDIO_ENC_LC3
    gaf_codec_id_t codec_id = GAF_CODEC_ID_LC3;
#endif

    bap_bc_adv_param_t adv_param = {
#ifdef BIS_TEST_WITH_PHONE
        .adv_intv_min_slot = 160,//320,//160,
        .adv_intv_max_slot = 160,//320,//160,
#else
        .adv_intv_min_slot = 192,//96,
        .adv_intv_max_slot = 192,//96,//
#endif

        .ch_map = ADV_ALL_CHNLS_EN,
        .phy_prim = GAPM_PHY_TYPE_LE_1M,
        .phy_second = GAPM_PHY_TYPE_LE_2M, //GAPM_PHY_TYPE_LE_2M, //GAPM_PHY_TYPE_LE_1M, //GAPM_PHY_TYPE_LE_2M,
        .adv_sid = 1,
        .max_tx_pwr = 127,//-70,
        .own_addr_type = GAPM_STATIC_ADDR,
        .max_skip = 0,
        .send_tx_pwr = false,
    };
    bap_bc_per_adv_param_t per_adv_param = {
#ifdef BIS_TEST_WITH_PHONE
        .adv_intv_min_frame = 160,//320, //160,
        .adv_intv_max_frame = 160,//320, //160,
#else
        .adv_intv_min_frame = 96,//320, //160,
        .adv_intv_max_frame = 96,//320, //160,
#endif
    };

    if (p_env->cfg.nb_streams == NUM_BIS_1)
    {
        if (LC3_CHANNEL == LC3_MONO)
        {
            p_env->subgroup_cfg.param.location_bf = GAF_LOC_FRONT_LEFT_BIT;    
        }
        else if (LC3_CHANNEL == LC3_STEREO)
        {
            p_env->subgroup_cfg.param.location_bf = GAF_LOC_FRONT_LEFT_BIT | GAF_LOC_FRONT_RIGHT_BIT;
        }
    }
    else if (p_env->cfg.nb_streams == NUM_BIS_2)
    {
        p_env->subgroup_cfg.param.location_bf = GAF_LOC_FRONT_LEFT_BIT | GAF_LOC_FRONT_RIGHT_BIT;
    }

    // printf("nb_streams %d\n", p_env->cfg.nb_streams);
    if(p_cfg->sdu_intv_us == 20000)
        p_env->subgroup_cfg.param.frames_sdu = 2;
    else
        p_env->subgroup_cfg.param.frames_sdu = 1;  
    LeAudioSetSrcFramesSdu(p_env->subgroup_cfg.param.frames_sdu);  
    if (get_src_wed_vaild())
    {
        grp_param.max_sdu = lc3_packet_len;
        p_env->subgroup_cfg.param.sampling_freq = p_cfg->samp_freq;
    }
    else
    {
        grp_param.max_sdu = lc3_packet_len + VENDOR_DATA_LEN;
        if(LeAudioGetSrcSampleRate() == 24000)
        {
            p_env->subgroup_cfg.param.sampling_freq = BAP_SAMPLING_FREQ_24000HZ;
        }
        else if(LeAudioGetSrcSampleRate() == 16000)
        {
            p_env->subgroup_cfg.param.sampling_freq = BAP_SAMPLING_FREQ_16000HZ;
        }  
        else
        {
            p_env->subgroup_cfg.param.sampling_freq = BAP_SAMPLING_FREQ_48000HZ;
        }      
    }    
    p_env->subgroup_cfg.param.frame_octet = grp_param.max_sdu;//p_cfg->max_sdu;
    p_env->subgroup_cfg.param.frame_dur = (p_cfg->sdu_intv_us == 7500)
                                                                    ? BAP_FRAME_DUR_7_5MS : BAP_FRAME_DUR_10MS;
    p_env->subgroup_cfg.add_cfg.len = 0;

    p_env->metadata.param.context_bf = BAP_CONTEXT_TYPE_UNSPECIFIED_BIT | BAP_CONTEXT_TYPE_MEDIA_BIT;
    p_env->metadata.add_metadata.len = 0;
    printf("nb_streams %d, %d, %d, %d\n", p_env->cfg.nb_streams, 
            p_env->subgroup_cfg.param.frame_octet, p_env->subgroup_cfg.param.sampling_freq,
            p_env->subgroup_cfg.param.frame_dur);
    do
    {
        const gaf_bcast_code_t* p_bcast_code = (p_env->cfg.is_encrypted == true) ? &(p_env->cfg.broadcast_code): NULL;

        status = bap_bc_src_add_group(&(p_env->cfg.broadcast_id), p_bcast_code, p_env->cfg.nb_streams, 1,
                                                    &grp_param, &adv_param, &per_adv_param, p_cfg->pres_delay_us,
                                                    &(p_env->grp_lid));

        APP_DBG("bap_bc_src_add_group grp_lid 0x%x\n", p_env->grp_lid);
        if (status)
        {
            APP_DBG("bap_bc_src_add_group error 0x%x\n", status);
            break;
        }
        
        status = bap_bc_src_set_subgroup(p_env->grp_lid, 0, &codec_id, &(p_env->subgroup_cfg),
                                                       &(p_env->metadata));
        if (status)
        {
            APP_DBG("bap_bc_src_set_subgroup error 0x%x\n", status);
            break;
        }

         uint8_t dp_id = BIS_SRC_DATA_PATH_ID;

        if (p_env->cfg.nb_streams == NUM_BIS_1)
        {
            status = bap_bc_src_set_stream(p_env->grp_lid, 0, 0, dp_id, 0, NULL);
            if (status)
            {
                APP_DBG("bap_bc_src_set_subgroup cnt %d, error 0x%x\n", cnt, status);
                break;
            } 
        }
        else if (p_env->cfg.nb_streams == NUM_BIS_2)
        {
            for (cnt = 0; cnt < p_env->cfg.nb_streams; cnt++)
            { 
                p_env->bis_cfg[cnt].add_cfg.len = 0;

                p_env->bis_cfg[cnt].param.frame_octet = p_env->subgroup_cfg.param.frame_octet;
                p_env->bis_cfg[cnt].param.sampling_freq = p_env->subgroup_cfg.param.sampling_freq;
                p_env->bis_cfg[cnt].param.frame_dur = p_env->subgroup_cfg.param.frame_dur;
                p_env->bis_cfg[cnt].param.frames_sdu = p_env->subgroup_cfg.param.frames_sdu;

                if (cnt == 0)
                {
                    p_env->bis_cfg[cnt].param.location_bf = GAF_LOC_FRONT_LEFT_BIT;
                }
                else if (cnt == 1)
                {
                    p_env->bis_cfg[cnt].param.location_bf = GAF_LOC_FRONT_RIGHT_BIT;
                    dp_id = BIS_SRC1_DATA_PATH_ID;
                }

                status = bap_bc_src_set_stream(p_env->grp_lid, cnt, 0, dp_id, 0, &p_env->bis_cfg[cnt]);
                if (status)
                {
                    APP_DBG("bap_bc_src_set_stream cnt %d, error 0x%x\n", cnt, status);
                }
            }
        }
    } while (false);
}

#if BLE_TWS_SUPPORT
void le_bis_broadcast_id_set(uint8_t *id)
{
	if(p_env)
	{
		p_env->cfg.broadcast_id.id[0] = id[0];
		p_env->cfg.broadcast_id.id[1] = id[1];
		p_env->cfg.broadcast_id.id[2] = id[2];
	}
}

void le_bis_broadcast_id_reset(void)
{
	if(p_env)
	{
		  memcpy(p_env->cfg.broadcast_id.id, get_bcast_id(), BCAST_ID_LEN);
	}
}
#endif

//使能周期性广播
void le_bis_source_play(void)
{
    if (get_src_wed_vaild())
    {
        char tmp_bcast_name[] = LE_AUDIO_DUALBIS_BROADCAST_NAME;
        memcpy((void*)p_env->cfg.p_broadcast_name, tmp_bcast_name, sizeof(tmp_bcast_name));
        p_env->cfg.bcast_name_len = sizeof(tmp_bcast_name);
        p_env->cfg.nb_streams = NUM_BIS_2;
    }
    else
    {
        char tmp_bcast_name[] = LE_AUDIO_MV_BROADCAST_NAME;
        memcpy((void*)p_env->cfg.p_broadcast_name, tmp_bcast_name, sizeof(tmp_bcast_name));
        p_env->cfg.bcast_name_len = sizeof(tmp_bcast_name);
        p_env->cfg.nb_streams = NUM_BIS_1;
    }

    uint16_t status;
    uint8_t metadata_test[] = {0x04,0x03,0x03,0x04,0x00};

    if (p_env->grp_lid == GAF_INVALID_LID)
    {
        le_bis_source_configure_group();
        SETB(p_env->status_bf, STATUS_GROUP_CFG, 1); 
    }
    APP_DBG("le_bis_source_play: grp_lid 0x%x\n", p_env->grp_lid);
    // uint8_t name_test[30] = {0};
    // name_test[0] = 1 + p_env->cfg.bcast_name_len;
    // name_test[1] = 0x9;
    // memcpy(&name_test[2], p_env->cfg.p_broadcast_name, p_env->cfg.bcast_name_len);

    // status = bap_bc_src_enable_pa(p_env->grp_lid,  p_env->cfg.bcast_name_len + 2, 0, name_test, NULL,
    //                                     p_env->cfg.bcast_name_len,
    //                                         (const uint8_t*)(p_env->cfg.p_broadcast_name), sizeof(metadata_test), metadata_test, bcast_code);    
    status = bap_bc_src_enable_pa(p_env->grp_lid, 0, 0, NULL, NULL, p_env->cfg.bcast_name_len, (const uint8_t*)(p_env->cfg.p_broadcast_name), 
                                        sizeof(metadata_test), metadata_test, bcast_code, g_volume); 
    if (status)
    {
        APP_DBG("bap_bc_src_enable_pa error 0x%x\n", status);
    }

    // LeAudioSrcStartCmd();
}

__STATIC void bis_src_configuration_state(uint16_t event, uint16_t status, const void* p_params)
{
    bool is_finished = false;

	switch (event)
	{

		case EVENT_CONFIGURATION_DONE:
		{
			 APP_CHECK_STATUS(gapm_set_name(EVENT_CONFIGURATION_NAME_SET, p_env->cfg.bcast_name_len, (const uint8_t*)p_env->cfg.p_broadcast_name,
			 				 bis_src_configuration_cb_gapm_cmp));

		} break;

		case EVENT_CONFIGURATION_NAME_SET:
		{
			APP_CHECK_STATUS(gapm_get_controller_bdaddr(EVENT_CONFIGURE_READ_CONTROLLER_ADDRESS,
														bis_src_configuration_cb_gapm_bdaddr));
		} break;

		case EVENT_CONFIGURE_READ_CONTROLLER_ADDRESS:
		{
			is_finished = true;
		} break;

		default: { } break;
	} while (false);


    if (is_finished )
    {
    	// le_bis_source_play();
    }
}

void set_bcast_id(uint8_t* p_bcast_id)
{
    memcpy(g_bcast_id, p_bcast_id, BCAST_ID_LEN);
}

uint8_t* get_bcast_id(void)
{
    return g_bcast_id;
}

__STATIC void le_bis_src_set_bcast_name(char* p_bcast_name, uint8_t bcast_name_len)
{
    broadcast_source_cfg.p_broadcast_name = (char*)ke_malloc_user(bcast_name_len, KE_MEM_PROFILE);
    if (!broadcast_source_cfg.p_broadcast_name)
    {
        APP_DBG("malloc err\n");
        return;
    }
    
    memcpy((char*)broadcast_source_cfg.p_broadcast_name, p_bcast_name, bcast_name_len);
    broadcast_source_cfg.bcast_name_len = bcast_name_len;

    memcpy(broadcast_source_cfg.broadcast_id.id, get_bcast_id(), BCAST_ID_LEN);
    broadcast_source_cfg.is_encrypted = false;
}

char* get_bis_src_bcast_name(void)
{
    return (char*)p_env->cfg.p_broadcast_name;
}

uint8_t get_bis_src_bcast_name_len(void)
{
    return p_env->cfg.bcast_name_len;
}

void le_bis_source_set_bcast_code(const uint8_t* p_bcast_code)
{
	 uint8_t null_code[GAP_KEY_LEN] = {0};
    if (memcmp(p_bcast_code, null_code, GAP_KEY_LEN) == 0)    
    {
        p_env->cfg.is_encrypted = false;
    }
    else
    {
        memcpy((p_env->cfg.broadcast_code.bcast_code), p_bcast_code, GAP_KEY_LEN);
        p_env->cfg.is_encrypted = true;
    }
    APP_DBG("le_bis_source_set_bcast_code: is_encrypted 0x%x\n", p_env->cfg.is_encrypted);
}

#ifdef DISCOV_BIS
__STATIC void le_bis_annou_cb_cmp_evt(uint8_t cmd_type, uint16_t status)
{
    ASSERT_WARN(status == GAF_ERR_NO_ERROR, status, cmd_type);

    APP_DBG("le_bis_annou_cb_cmp_evt: cmd_type 0x%x, 0x%x\n", cmd_type, status);
    switch (cmd_type)
    {
        case BAP_BC_ANNOU_CMD_TYPE_ENABLE_PA:
        {
            if (status)
            {
                APP_DBG("BAP_BC_ANNOU_CMD_TYPE_ENABLE_PA error 0x%x\n", status);
            }
        }
            break;

        case BAP_BC_ANNOU_CMD_TYPE_DISABLE_PA:
        {
             if (status)
            {
                APP_DBG("BAP_BC_ANNOU_CMD_TYPE_DISABLE_PA error 0x%x\n", status);
            }
        } break;

        default: { } break;
    }
}

__STATIC const struct bap_bc_annou_cb cbs_bap_bc_annou =
{
    .cb_cmp_evt = le_bis_annou_cb_cmp_evt,
};
#endif

static uint16_t le_bis_source_configure(const le_bis_source_cfg_t* p_params,
                                                component_cb_event cb_event)
{
    /// Device appearance
    uint16_t APPEARANCE = 0x00;
    uint16_t ret;

    // APP_DBG("register data path\n");
    le_bis_source_datapath_init();
    le_bis_src1_in_datapath_init();

    p_env = (le_bis_source_env_t*)ke_malloc_user(sizeof(le_bis_source_env_t), KE_MEM_PROFILE);

    memset(p_env, 0, sizeof(le_bis_source_env_t));
    p_env->grp_lid = GAF_INVALID_LID;

    p_env->cfg.p_broadcast_name = (char*)ke_malloc_user(p_params->bcast_name_len, KE_MEM_PROFILE);
    memcpy((char*)p_env->cfg.p_broadcast_name, p_params->p_broadcast_name, p_params->bcast_name_len);
    memcpy(&(p_env->cfg), p_params, sizeof(le_bis_source_cfg_t));

#ifdef DISCOV_BIS
    p_env->path_loss_threshold = 55;
    p_env->radiated_tx_power = 0;

    ret = bap_bc_annou_configure(&cbs_bap_bc_annou, p_env->path_loss_threshold, p_env->radiated_tx_power);
    if (ret)
    {
        APP_DBG("le_bis_source_configure: bap_bc_annon_configure error, ret 0x%x\n", ret);
    }
#endif

    ret = bap_bc_src_configure(&cbs_bap_bc_src);
    if (ret)
    {
        APP_DBG("le_bis_source_configure: bap_bc_src_configure error, ret 0x%x\n", ret);
    }

    gapm_le_set_appearance(APPEARANCE);

    le_bis_source_set_bcast_code(bcast_code);
    return ret;
}

uint8_t app_get_broadcast_src_grp_lid(void)
{
    return p_env->grp_lid;
}

uint8_t app_get_broadcast_src_nb_sgrp(void)
{
    return 1;
}

void app_set_vendor_data(uint8_t *vendor_data)
{
    memcpy(g_vendor_data,vendor_data,VENDOR_DATA_LEN);
//    APP_DBG("app_set_vendor_data: 0x%x\n", g_vendor_data);
}

void app_pa_evt_cnt_cb(uint8_t* p_act_id, uint16_t* p_pa_evt_cnt)
{
    printf("app_pa_evt_cnt_cb: id %d, evt_cnt %d\n", *p_act_id, *p_pa_evt_cnt);
}

void le_bis_src_print_state(void)
{
    BT_DBG("le_bis_src_print_state: p_env->status_bf 0x%x: 0x%x, 0x%x, 0x%X, 0x%x\n", 
                p_env->status_bf, CHECKB(p_env->status_bf, STATUS_GROUP_CFG), CHECKB(p_env->status_bf, STATUS_GROUP_PA_ENABLED),
                CHECKB(p_env->status_bf, STATUS_GROUP_ENABLED), CHECKB(p_env->status_bf, STATUS_GROUP_STREAM));  
}

void le_bis_src_handle_stop(void)
{
	uint16_t status;
    BT_DBG("le_bis_src_handle_stop: p_env->status_bf 0x%x: 0x%x, 0x%x, 0x%X, 0x%x\n", 
                p_env->status_bf, CHECKB(p_env->status_bf, STATUS_GROUP_CFG), CHECKB(p_env->status_bf, STATUS_GROUP_PA_ENABLED),
                CHECKB(p_env->status_bf, STATUS_GROUP_ENABLED), CHECKB(p_env->status_bf, STATUS_GROUP_STREAM));

    if (CHECKB(p_env->status_bf, STATUS_GROUP_STREAM))
    {
        uint32_t stream_lid_bf = 1;
        status = bap_bc_src_stop_streaming(p_env->grp_lid, stream_lid_bf);
        if (status)
        {
            APP_DBG("le_bis_src_handle_stop error 0x%x\n", status);
        }
    }

    if (CHECKB(p_env->status_bf, STATUS_GROUP_ENABLED))
    {
        status = bap_bc_src_disable(p_env->grp_lid, true);
        if (status)
        {
            BT_DBG("bap_bc_src_disable error 0x%x\n", status);
        }
    }
    else
    {
        if (CHECKB(p_env->status_bf, STATUS_GROUP_PA_ENABLED))
        {
            SETB(p_env->status_bf, STATUS_GROUP_PA_ENABLED, 0);           
        }
    }

    if (CHECKB(p_env->status_bf, STATUS_GROUP_CFG))
    {
        SETB(p_env->status_bf, STATUS_GROUP_CFG, 0); 
    }
}

bool check_bis_src_is_streamed(void)
{
    if (CHECKB(p_env->status_bf, STATUS_GROUP_STREAM))
    {
        return true;
    }
    return false;
}

bool check_bis_src_is_in_idle(void)
{
    if (p_env->status_bf == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

uint16_t le_bis_source_init(void)
{
#ifdef DISCOV_BIS
    bap_bc_annou_init();

#endif
#ifdef SINK_CHOOSE_SRC
    uint8_t tmp_bcast_id[BCAST_ID_LEN];
    tmp_bcast_id[0] = btManager.btDevAddr[0];
    tmp_bcast_id[1] = btManager.btDevAddr[1];
    tmp_bcast_id[2] = btManager.btDevAddr[5];
#else
    uint8_t tmp_bcast_id[] = LE_AUDIO_MV_BROADCAST_ID;
#endif
    set_bcast_id(tmp_bcast_id);

    char tmp_bcast_name[] = LE_AUDIO_MV_BROADCAST_NAME;

    le_bis_src_set_bcast_name(tmp_bcast_name, sizeof(LE_AUDIO_MV_BROADCAST_NAME));

	return le_bis_source_configure(&broadcast_source_cfg, NULL);
}

uint8_t get_if_use_pto(void)
{
#ifdef USE_PTO
    return true;
#else
    return false;
#endif
}

//关闭周期性广播
void le_bis_source_stop(void)
{
    uint32_t stream_lid_bf ;
    if (get_src_wed_vaild())
    {
        stream_lid_bf = 3;
    }
    else
    {
        stream_lid_bf = 1;       
    }

    if (CHECKB(p_env->status_bf, STATUS_GROUP_STREAM))
    {
        uint16_t status = bap_bc_src_stop_streaming(p_env->grp_lid, stream_lid_bf);
        if (status)
        {
            APP_DBG("le_bis_source_stop error 0x%x\n", status);
        }
//        LeAudioSrcStopCmd();
    }
}

void le_bis_source_set_codec_configuration(const char* p_params_codec_cfg)
{
    memcpy(&(p_env->cfg.cfg_name), p_params_codec_cfg, LE_BIS_SOURCE_CFG_NAME_MAX_LENGTH);
}

#endif //#if (BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT)   LE_BIS_SOURCE
