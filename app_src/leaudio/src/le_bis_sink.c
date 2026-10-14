#include "le_audio_config.h"
#include "arch.h"
#include "bap_bc_sink.h"
#include "bap_bc_scan.h"
#include "bap_bc_discoverer.h"
#include "bap.h"
#include "gapc_le.h"
#include "debug.h"
#include "rtos_api.h"
#include "app_message.h"
#include "le_bis_sink.h"
#include <string.h>
// #include "ble_tws.h"

#if (BT_LE_AUDIO_SUPPORT)

void le_bis_sink_handle_stop(void);
void le_bis_sink_stop_scan(void);

extern uint8_t* get_bcast_id(void);
extern uint8_t* le_tws_broadcast_id_get(void);
extern char* get_bis_src_bcast_name(void);
extern void LeAudioCodecFormatSet(uint8_t format, uint8_t channel);
extern void bis_sync_audio_close(void);
extern void utils_audio_le_pack_codec_cfg(void* buffer, void* codec_cfg);
extern void le_bis_sink_datapath_init(void);
extern void le_bis_sink1_datapath_init(void);
extern const char* utils_get_device_name(void);
extern void utils_set_device_name(const char* name);
extern void LeAudioSinkModeDisable(void);
extern void set_bis_pres_delay_us(uint32_t pres_delay_us);

static uint8_t DUALBIS_sink_vol = 0xff;
bool pa_sync_lost = 0;
bool sink_wait_starting = 0;
uint8_t bis_src_type = LE_AUDIO_SRC_UNKNOWN;

static const struct
{
	const uint8_t *name;
	const uint8_t type;
	const uint8_t min_flag;
}BisSrcMap[] =
{
	{(const uint8_t *)LE_AUDIO_DUALBIS_BROADCAST_NAME,	LE_AUDIO_SRC_MV_DUALBIS,	0},
	{(const uint8_t *)LE_AUDIO_MV_BROADCAST_NAME,	LE_AUDIO_SRC_MV_BIS,	0},
	{(const uint8_t *)"DUALBIS PartyBox",				LE_AUDIO_SRC_DUALBIS_PARTY,	1},
	{(const uint8_t *)LE_AUDIO_DUALBIS_BROADCAST_NAME,	LE_AUDIO_SRC_DUALBIS_CLIP,	1},
    {(const uint8_t *)"Galaxy",						LE_AUDIO_SRC_PHONE_1,	1},
    {(const uint8_t *)"TYL.Auracast",				LE_AUDIO_SRC_EPPFUN,	1},
	{NULL,	0,	0},
};

extern void LeAudioLc3DecoderConfig(uint8_t	channel,
							 uint32_t 	lc3_bitrate,
							 uint16_t 	lc3_frame_len,
							 uint16_t 	SampleRate);
extern uint16_t lc3_packet_len;
// extern bool is_use_pto;

#define SINK_APPEARANCE  (0x0000)

#define GAP_ERR_NO_ERROR  (0)

#define APP_SCAN_START_WAIT_MS          (100)
#define APP_BIGINFO_START_WAIT_MS       (2000)//(1000)

#ifdef SINK_CHOOSE_SRC
#define MAX_SRC_NUM      (10)
#define MAX_BCAST_NAME_LEN      (32)
typedef struct sink_add_src
{
    uint8_t src_idx;
    bap_adv_id_t adv_id;
    bap_bcast_id_t bcast_id;
    uint8_t broadcast_name_len;
    uint8_t broadcast_name[MAX_BCAST_NAME_LEN];   
} sink_add_src_t;

__STATIC sink_add_src_t src_array[MAX_SRC_NUM];
#endif

__STATIC le_bis_sink_env_t* p_env= NULL;

uint8_t check_bis_sink_state(void);

// Event values for Group state machine
enum le_bis_sink_event_group
{
    // Play has been required
    EVENT_GROUP_PLAY = 0U,
    // Stop has been required
    EVENT_GROUP_STOP,
    // Synchronization with Periodic Advertising has been established
    EVENT_GROUP_PA_SYNCHRONIZED,
    // Information about Broadcast Group have been received
    EVENT_GROUP_INFO_RECEIVED,
    // Synchronization with Group has been established
    EVENT_GROUP_SYNCHRONIZED,
    // Reception started
    EVENT_GROUP_STREAMING_STARTED,
    // Synchronization with Periodic Advertising has been stopped
    EVENT_GROUP_PA_DESYNCHRONIZED,
    // Synchronization with Group has been stopped
    EVENT_GROUP_DESYNCHRONIZED,

    EVENT_GROUP_DISCOV,
};

// Event values for Scan state machine
enum le_bis_sink_event_scan
{
    // Start Scan state machine
    EVENT_SCAN_START = 0U,
    // Broadcast Source has been discovered
    EVENT_SCAN_SOURCE_FOUND,
    // Stop has been requested
    EVENT_SCAN_STOP,
    // Scan has been stopped
    EVENT_SCAN_STOPPED,
    
    //LL scan stopped
    EVENT_LL_SCAN_STOPPED,
};

// Event values for Scan state machine
enum le_bis_sink_event_discov
{
    // Start Scan state machine
    EVENT_DISCOV_START = 4U,
    // Broadcast Source has been discovered
    EVENT_DISCOV_SOURCE_FOUND,
    // Stop has been requested
    EVENT_DISCOV_STOP,
    // Scan has been stopped
    EVENT_DISCOV_STOPPED,
};

// Status bit field meaning
enum le_bis_sink_status_bf
{
    // Indicate if synchronization with a Group has been established - Position
    STATUS_GROUP_SYNCHRONIZED_POS = 0U,
    // Indicate if synchronization with a Group has been established - Bit
    STATUS_GROUP_SYNCHRONIZED_BIT = CO_BIT(STATUS_GROUP_SYNCHRONIZED_POS),

    // Indicate if Broadcast Source has been found - Position
    STATUS_SOURCE_FOUND_POS = 1U,
    // Indicate if Broadcast Source has been found - Bit
    STATUS_SOURCE_FOUND_BIT = CO_BIT(STATUS_SOURCE_FOUND_POS),

    // Indicate if being synchronized - Position
    STATUS_GROUP_SYNCHRONIZING_POS = 2U,
    // Indicate if being synchronized - Bit
    STATUS_GROUP_SYNCHRONIZING_BIT = CO_BIT(STATUS_GROUP_SYNCHRONIZING_POS),

    // SCAN enable- Position
    STATUS_SCAN_ENABLE_POS = 3U,
    // Indication of SCAN enable - Bit
    STATUS_SCAN_ENABLE_BIT = CO_BIT(STATUS_SCAN_ENABLE_POS),

    // PA_SYNCHRONIZING- Position
    STATUS_PA_SYNCHRONIZING_POS = 4U,
    // Indication of PA_SYNCHRONIZING - Bit
    STATUS_PA_SYNCHRONIZING_BIT = CO_BIT(STATUS_PA_SYNCHRONIZING_POS),

    // PA_SYNCHRONIZED- Position
    STATUS_PA_SYNCHRONIZED_POS = 5U,
    // Indication of PA_SYNCHRONIZE - Bit
    STATUS_PA_SYNCHRONIZED_BIT = CO_BIT(STATUS_PA_SYNCHRONIZED_POS),

    // GROUP_STREAMED- Position
    STATUS_GROUP_STREAMED_POS = 6u,//9U,
    // Indication of GROUP_STREAMED - Bit
    STATUS_GROUP_STREAMED_BIT = CO_BIT(STATUS_GROUP_STREAMED_POS),
};
// Status bit field meaning
enum le_bis_discov_status_bf
{
    // Indicate if Broadcast Source has been found - Position
    STATUS_DISCOV_SOURCE_FOUND_POS = 7U,
    // Indicate if Broadcast Source has been found - Bit
    STATUS_DISCOV_SOURCE_FOUND_BIT = CO_BIT(STATUS_DISCOV_SOURCE_FOUND_POS),

    // PA_SYNCHRONIZING- Position
    STATUS_DISCOV_PA_SYNCHRONIZING_POS = 8U,
    // Indication of PA_SYNCHRONIZING - Bit
    STATUS_DISCOV_PA_SYNCHRONIZING_BIT = CO_BIT(STATUS_DISCOV_PA_SYNCHRONIZING_POS),

    // PA_SYNCHRONIZED- Position
    STATUS_DISCOV_PA_SYNCHRONIZED_POS = 9U,
    // Indication of PA_SYNCHRONIZE - Bit
    STATUS_DISCOV_PA_SYNCHRONIZED_BIT = CO_BIT(STATUS_DISCOV_PA_SYNCHRONIZED_POS),
};
//static function declare
__STATIC void le_bis_sink_cb_bap_bc_sink_cmp_evt(uint8_t cmd_type, uint16_t status, uint8_t grp_lid,  uint8_t stream_pos);
__STATIC void le_bis_sink_cb_bap_bc_sink_quality_cmp_evt(uint16_t status, uint8_t grp_lid, uint8_t stream_pos, uint32_t crc_error_packets,
                                                         uint32_t rx_unrx_packets,
                                                         uint32_t duplicate_packets);
__STATIC void le_bis_sink_cb_bap_bc_sink_status(uint8_t grp_lid, uint8_t state, uint32_t stream_pos_bf,
                                                          const gapi_bg_sync_config_t* p_bg_cfg, uint8_t nb_bis,
                                                          const uint16_t* p_conhdl);
__STATIC void le_bis_sink_cb_cmp_evt(uint8_t cmd_type, uint16_t status, uint8_t pa_lid);
__STATIC void le_bis_sink_cb_timeout(void);
__STATIC void le_bis_sink_cb_report(const bap_adv_id_t* p_adv_id, const bap_bcast_id_t* p_bcast_id,
                                              uint8_t info_bf, const gaf_adv_report_air_info_t* p_air_info,
                                              uint16_t length, const uint8_t* p_data);
#if (APP_GAF_PBP)
__STATIC void le_bis_sink_cb_public_bcast_source(const bap_adv_id_t* p_adv_id,
                                                           const bap_bcast_id_t* p_bcast_id, uint8_t pbp_features_bf,
                                                           uint8_t broadcast_name_len,
                                                           const uint8_t* p_broadcast_name, uint8_t metadata_len,
                                                           const uint8_t* p_metadata, const gaf_adv_report_air_info_t* p_air_info);
#endif
__STATIC void le_bis_sink_cb_pa_established(uint8_t pa_lid, const bap_adv_id_t* p_adv_id, uint8_t phy,
                                                      uint16_t interval_frames);
__STATIC void le_bis_sink_cb_pa_terminated(uint8_t pa_lid, uint8_t reason);
__STATIC void le_bis_sink_cb_pa_report(uint8_t pa_lid, const gaf_adv_report_air_info_t* p_air_info,
                                                 uint16_t length, const uint8_t* p_data);
__STATIC void le_bis_sink_cb_big_info_report(uint8_t pa_lid, const gapm_le_big_info_t* p_report);
__STATIC void le_bis_sink_cb_group_report(uint8_t pa_lid, uint8_t nb_subgroups, uint8_t nb_streams,
                                                    uint32_t pres_delay_us);
__STATIC void le_bis_sink_cb_stream_report(uint8_t pa_lid, uint8_t sgrp_id, uint8_t stream_pos,
                                                     const gaf_codec_id_t* p_codec_id, const bap_cfg_ptr_t* p_cfg, 
                                                     uint8_t* volume);
__STATIC void le_bis_sink_cb_subgroup_report(uint8_t pa_lid, uint8_t sgrp_id, uint32_t stream_pos_bf,
                                                       const gaf_codec_id_t* p_codec_id, const bap_cfg_ptr_t* p_cfg,
                                                       const bap_cfg_metadata_ptr_t* p_metadata);
__STATIC void le_bis_sink_state_group(uint8_t event, uint16_t status);

void le_bis_sink_configuration_cb_gapm_cmp(uint32_t event, uint16_t status);

__STATIC void le_bis_sink_bdaddr_cb(uint32_t metainfo, uint16_t status, const gap_addr_t* p_addr);

__STATIC void app_cb_timer_user_handler(co_timer_t* p_timer);

// Set of callback functions for communication with BAP Broadcast Scan module
__STATIC const bap_bc_scan_cb_t cbs_bap_bc_scan =
{
    .cb_cmp_evt              = le_bis_sink_cb_cmp_evt,
    .cb_timeout              = le_bis_sink_cb_timeout,
    .cb_report               = le_bis_sink_cb_report,
    #if (APP_GAF_PBP)
    .cb_public_bcast_source  = le_bis_sink_cb_public_bcast_source,
    #endif //(APP_GAF_PBP)
    .cb_pa_established       = le_bis_sink_cb_pa_established,
    .cb_pa_terminated        = le_bis_sink_cb_pa_terminated,
    .cb_pa_report            = le_bis_sink_cb_pa_report,
    .cb_big_info_report      = le_bis_sink_cb_big_info_report,
    .cb_group_report         = le_bis_sink_cb_group_report,
    .cb_subgroup_report      = le_bis_sink_cb_subgroup_report,
    .cb_stream_report        = le_bis_sink_cb_stream_report,
};

// Set of callback functions for communication with BAP Broadcast Sink module
__STATIC const bap_bc_sink_cb_t cbs_bap_bc_sink =
{
    .cb_cmp_evt         = le_bis_sink_cb_bap_bc_sink_cmp_evt,
    .cb_quality_cmp_evt = le_bis_sink_cb_bap_bc_sink_quality_cmp_evt,
    .cb_status          = le_bis_sink_cb_bap_bc_sink_status,
};

extern uint8_t get_sync_pa_accept(void);
extern void set_pa_sync_accept(uint8_t sync_accept);

uint8_t get_bis_sink_target_bcast_type(uint8_t *name,uint8_t len)
{
	uint8_t type,i;

	type = LE_AUDIO_SRC_UNKNOWN;
	i = 0;
	while(BisSrcMap[i].name != NULL)
	{
		if(BisSrcMap[i].min_flag)
		{
			len = min(strlen(BisSrcMap[i].name), len);
		}

		if (!memcmp(BisSrcMap[i].name, name,  len))
		{
			type = BisSrcMap[i].type;
			break;
		}
		i++;
	}

	return type;
}

uint8_t * get_bis_sink_type_name(uint8_t type)
{
	uint8_t i = 0;

	while(BisSrcMap[i].name != NULL)
	{
		if (type == BisSrcMap[i].type)
		{
			return BisSrcMap[i].name ;
		}
		i++;
	}

	return NULL;
}
/*
 * FUNCTIONS
 ****************************************************************************************
 */
uint8_t Is_Srcid_match(const uint8_t* src_id)
{
	const uint8_t* bis_srcid_filter = get_bcast_id();

	if( ( bis_srcid_filter[0] != 0 ) &&  ( bis_srcid_filter[1] != 0 ) && (  bis_srcid_filter[2] != 0 ) )
	   return !memcmp(bis_srcid_filter, src_id, 3);
	return 1;
}

uint8_t LE_Tws_Is_Srcid_match(const uint8_t* src_id)
{
	uint8_t *bis_srcid_filter = le_tws_broadcast_id_get();

	if( ( bis_srcid_filter[0] == 0 ) &&  ( bis_srcid_filter[1] == 0 ) && (  bis_srcid_filter[2] == 0 ) )
	   return 1;
	return !memcmp(bis_srcid_filter,src_id,3);
}

__STATIC void le_bis_sink_state_scan(uint8_t event, uint16_t status)
{
   APP_DBG("le_bis_sink_state_scan: event 0x%x, status 0x%x, 0x%x, 0x%x\n", 
                event, status, CHECKB(p_env->status_bf, STATUS_SOURCE_FOUND), p_env->status_bf);
    switch (event)
    {
        case EVENT_SCAN_START:
        {
            SETB(p_env->status_bf, STATUS_SOURCE_FOUND, 0);
            status = bap_bc_scan_start(0);
            if (status)
            {
                APP_DBG("bap_bc_scan_start err 0x%x\n", status);
            }
        } break;

        case EVENT_SCAN_SOURCE_FOUND:
        {
            {
                if (CHECKB(p_env->status_bf, STATUS_SOURCE_FOUND))
                {
                    return;
                }

                printf("EVENT_SCAN_SOURCE_FOUND: addr 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n", 
                            p_env->adv_id.addr[0], p_env->adv_id.addr[1], p_env->adv_id.addr[2], 
                            p_env->adv_id.addr[3], p_env->adv_id.addr[4], p_env->adv_id.addr[5]);
                            
                SETB(p_env->status_bf, STATUS_SOURCE_FOUND, 1);
            }
            le_bis_sink_state_group(EVENT_GROUP_PLAY, GAF_ERR_NO_ERROR);
        } break;

        case EVENT_SCAN_STOP:
        {
            status = bap_bc_scan_stop();
            if (status)
            {
                APP_DBG("EVENT_SCAN_STOP: bap_bc_scan_stop error 0x%x\n", status);
            }
        } break;

        case EVENT_SCAN_STOPPED: 
        {

        } break;

        case EVENT_LL_SCAN_STOPPED: 
        {
            app_get_bis_state();
            printf("SCAN_STOP statu_bf %02x:0x%X:0x%x\n", p_env->status_bf, sink_wait_starting, Get_bis_role());
            if(sink_wait_starting && GETB(p_env->status_bf, STATUS_SOURCE_FOUND))
            {
                SETB(p_env->status_bf, STATUS_SOURCE_FOUND, 0);
                printf("** clear STATUS_SOURCE_FOUND **,%x\n",  p_env->status_bf);
            }
            if (!status)
            {
                SETB(p_env->status_bf, STATUS_SCAN_ENABLE, 0);
#ifdef DISCOV_BIS
                uint16_t status = bap_bc_discov_pa_terminate(p_env->discov_pa_lid);
                if (status)
                {
                    APP_DBG("bap_bc_discov_pa_terminate: err 0x%x\n", status);
                }
#endif
            }
        } break;

#ifdef DISCOV_BIS
        case EVENT_DISCOV_START:
        {
            SETB(p_env->status_bf, STATUS_SOURCE_FOUND, 0);
            bap_bc_discov_start(0);
        } break;

        case EVENT_DISCOV_STOP:
        {
            status = bap_bc_discov_stop();
            if (status)
            {
                APP_DBG("EVENT_SCAN_STOP: bap_bc_scan_stop error 0x%x\n", status);
            }
        } break;

        case EVENT_DISCOV_SOURCE_FOUND:
        {
            if (CHECKB(p_env->status_bf, STATUS_DISCOV_SOURCE_FOUND))
            {
                return;
            }
            SETB(p_env->status_bf, STATUS_DISCOV_SOURCE_FOUND, 1);
            le_bis_sink_state_group(EVENT_GROUP_DISCOV, GAF_ERR_NO_ERROR);
        } break;

        case EVENT_DISCOV_STOPPED: 
        {
            if (!status)
            {
                SETB(p_env->status_bf, STATUS_SCAN_ENABLE, 0);
            }
        } break;
#endif

        default: { } break;
    }
}

void scan_disable_cb(uint8_t status)
{
    le_bis_sink_state_scan(EVENT_LL_SCAN_STOPPED, status);
}

__STATIC void le_bis_sink_cb_cmp_evt(uint8_t cmd_type, uint16_t status, uint8_t pa_lid)
{
	APP_DBG("le_bis_sink_cb_cmp_evt: %d, status 0x%x\n", cmd_type, status);

    switch (cmd_type)
    {
        case BAP_BC_SCAN_CMD_TYPE_PA_SYNCHRONIZE:
        {
            if (status)
            {
                SETB(p_env->status_bf, STATUS_PA_SYNCHRONIZING, 0);  

            }
            else
            {
                SETB(p_env->status_bf, STATUS_PA_SYNCHRONIZING, 1);                
            }

        } break;

        case BAP_BC_SCAN_CMD_TYPE_START:
        {
            if (!status)
            {
                SETB(p_env->status_bf, STATUS_SCAN_ENABLE, 1);
            }

        } break;

        case BAP_BC_SCAN_CMD_TYPE_STOP:
        {
//            if (get_start_bf() != 0xffffff)
//            {
//                printf("baikal: 00000 0x%x\n", get_start_bf());
//                clear_start_bf();
//            }

//             if (!status)
//             {
//                 // SETB(p_env->status_bf, STATUS_SCAN_ENABLE, 0);

//                 set_sink_sync_lost(0);//clear sync_lost
// #ifdef DISCOV_BIS
//                 le_bis_sink_state_scan(EVENT_SCAN_STOPPED, GAF_ERR_NO_ERROR);
// #endif
//                 if (get_sink_wed_vaild() == 1)
//                 {
//                     le_bis_sink_state_scan(EVENT_SCAN_STOPPED, GAF_ERR_NO_ERROR);    
//                 }
//             }

        } break;

        default: { } break;
    }
}

__STATIC void le_bis_sink_cb_timeout(void)
{
	APP_DBG("le_bis_sink_cb_timeout\n");
}


__STATIC void le_bis_sink_cb_report(const bap_adv_id_t* p_adv_id, const bap_bcast_id_t* p_bcast_id,
                                              uint8_t info_bf, const gaf_adv_report_air_info_t* p_air_info,
                                              uint16_t length, const uint8_t* p_data)
{
    // if (p_air_info->rssi < -60)
    // {
    //     return;
    // }
    APP_DBG("le_bis_sink_cb_report: sync_lost %d, rssi %d\n", get_sink_sync_lost(), p_air_info->rssi);
    //add filter of le name
    
    const uint8_t* p_reported_name = NULL;
    uint8_t name_length = 0;

    p_reported_name = gapm_get_ltv_value(GAP_AD_TYPE_SHORTENED_NAME, length, p_data,
                                            &(name_length));
    if (p_reported_name == NULL)
    {
        p_reported_name = gapm_get_ltv_value(GAP_AD_TYPE_COMPLETE_NAME, length, p_data,
                                                &(name_length));
    }
    
    if (p_reported_name == NULL)
    {
        p_reported_name = gapm_get_ltv_value(GAP_AD_TYPE_BROADCAST_NAME, length, p_data,
                                                &(name_length));
    }

#ifndef SINK_CHOOSE_SRC
    if (get_sink_sync_lost())
    {
        if (p_adv_id->addr_type != p_env->src_adv_id.addr_type)
        {
            return;
        }
        else
        {
            if (p_adv_id->addr_type == GAP_ADDR_PUBLIC)
            {
#ifdef COMMON_SINK
                if (memcmp(p_adv_id->addr, p_env->src_adv_id.addr, 6))
                {
                    return;
                } 
#else
                if (p_reported_name != NULL)  
                {
#ifndef ONE_VS_ONE
                    uint8_t type = get_bis_sink_target_bcast_type(p_reported_name,name_length);
                    if(type != LE_AUDIO_SRC_UNKNOWN)
                    {
                        if (check_bis_sink_state() == 8)
                        {
                            APP_DBG("found src: %s\n",get_bis_sink_type_name(type));
                        }
                        bis_src_type = type;
                    }
                    else
                    {
                        return;
                    }
#else
#ifndef DUAL_BIS_MODE
                    if (!memcmp(p_reported_name, LE_AUDIO_MV_BROADCAST_NAME, strlen(LE_AUDIO_MV_BROADCAST_NAME)))
                    {
                        APP_DBG("reconn src(MAC): 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n",
                                    p_adv_id->addr[0], p_adv_id->addr[1], p_adv_id->addr[2],
                                    p_adv_id->addr[3], p_adv_id->addr[4], p_adv_id->addr[5]);

                        bis_src_type = LE_AUDIO_SRC_MV_BIS;
                    }
#else
                    if (!memcmp(p_reported_name, LE_AUDIO_DUALBIS_BROADCAST_NAME, strlen(LE_AUDIO_DUALBIS_BROADCAST_NAME)))
                    {
                        APP_DBG("reconn src(MAC): 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n",
                                    p_adv_id->addr[0], p_adv_id->addr[1], p_adv_id->addr[2],
                                    p_adv_id->addr[3], p_adv_id->addr[4], p_adv_id->addr[5]);

                        bis_src_type = LE_AUDIO_SRC_MV_DUALBIS;
                    }
#endif
                    else
                    {
                        APP_DBG("no target src\n");
                        return;
                    }


#endif

                }
                else
                {
                    APP_DBG("p_reported_name is null\n");
                    return;      
                }

                APP_DBG("le_bis_sink_cb_report: SOURCE_RECONN 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n", 
                            p_adv_id->addr[0], p_adv_id->addr[1], p_adv_id->addr[2],
                             p_adv_id->addr[3], p_adv_id->addr[4], p_adv_id->addr[5]);  

#endif
            }       

            // if (get_sync_pa_accept() == 0)
            // {
            //     SETB(p_env->status_bf, STATUS_SOURCE_FOUND, 0);

            //     uint16_t status = bap_bc_scan_pa_terminate(p_env->pa_lid);
            //     if (status)
            //     {
            //         APP_DBG("bap_bc_scan_pa_terminate: err 0x%x, pa_lid 0x%x\n", status, p_env->pa_lid); 
            //     }
            //     // else
            //     {
            //         set_pa_sync_accept(0xff);
            //     }
            // }   
        }
    }
#ifndef COMMON_SINK
    else
    {
        if (p_reported_name != NULL)  
        {
#ifndef ONE_VS_ONE
            uint8_t type = get_bis_sink_target_bcast_type(p_reported_name,name_length);
            if(type != LE_AUDIO_SRC_UNKNOWN)
            {
				if (check_bis_sink_state() == 8)
				{
					APP_DBG("found src: %s\n",get_bis_sink_type_name(type));
					APP_DBG("found src(MAC): 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n",
								p_adv_id->addr[0], p_adv_id->addr[1], p_adv_id->addr[2],
								p_adv_id->addr[3], p_adv_id->addr[4], p_adv_id->addr[5]);
				}
				bis_src_type = type;
            }
            else
            {
                return;
            }
#else
#ifndef DUAL_BIS_MODE
            if (!memcmp(p_reported_name, LE_AUDIO_MV_BROADCAST_NAME, strlen(LE_AUDIO_MV_BROADCAST_NAME)))
            {
                APP_DBG("found src(MAC): 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n",
                            p_adv_id->addr[0], p_adv_id->addr[1], p_adv_id->addr[2],
                            p_adv_id->addr[3], p_adv_id->addr[4], p_adv_id->addr[5]);

                bis_src_type = LE_AUDIO_SRC_MV_BIS;
            }
#else
            if (!memcmp(p_reported_name, LE_AUDIO_DUALBIS_BROADCAST_NAME, strlen(LE_AUDIO_DUALBIS_BROADCAST_NAME)))
            {
                APP_DBG("found src(MAC): 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n",
                            p_adv_id->addr[0], p_adv_id->addr[1], p_adv_id->addr[2],
                            p_adv_id->addr[3], p_adv_id->addr[4], p_adv_id->addr[5]);

                bis_src_type = LE_AUDIO_SRC_MV_DUALBIS;
            }
#endif
            else
            {
                APP_DBG("no target src\n");
                return;
            }
#endif
        }
        else
        {
            // APP_DBG("p_reported_name is null\n");
            return;      
        }
    }
#endif
    le_bis_sink_printf_state();

    if (!CHECKB(p_env->status_bf, STATUS_GROUP_SYNCHRONIZED)
            && !CHECKB(p_env->status_bf, STATUS_GROUP_SYNCHRONIZING)
            &&!CHECKB(p_env->status_bf, STATUS_PA_SYNCHRONIZED)
            && !CHECKB(p_env->status_bf, STATUS_PA_SYNCHRONIZING)
            && !CHECKB(p_env->status_bf, STATUS_GROUP_STREAMED))
            //&&p_air_info->rssi >= -90) //-60
    {
        memcpy(&(p_env->adv_id), p_adv_id, sizeof(bap_adv_id_t));
        memcpy(&(p_env->broadcast_id), p_bcast_id, sizeof(bap_bcast_id_t));
    
        if (Get_bis_role() == BIS_SINK)
        {
            le_bis_sink_state_scan(EVENT_SCAN_SOURCE_FOUND, GAF_ERR_NO_ERROR);
        }
    }

    if (pa_sync_lost)
    {
        if (Get_bis_role() == BIS_SINK)
        {
            le_bis_sink_state_scan(EVENT_SCAN_SOURCE_FOUND, GAF_ERR_NO_ERROR);
        }        
    }
#else
    if (p_reported_name)
    {
        // printf("le_bis_sink_cb_report: bcast_id 0x%x:0x%x:0x%x\n", p_bcast_id->id[0], p_bcast_id->id[1], p_bcast_id->id[2]);
        le_bis_sink_scan_src(p_adv_id, p_bcast_id, name_length, p_reported_name); 
    }
#endif
}

void set_phone_bcast_code(uint8_t* p_bcast_code)
{
	if(p_env)
	{
		memset((void*)p_env->broadcast_code.bcast_code, 0, sizeof(p_env->broadcast_code.bcast_code));
		memcpy((void*)p_env->broadcast_code.bcast_code, p_bcast_code, 4);
	}
}

#ifdef SINK_CHOOSE_SRC
bool check_src_in_array(const bap_adv_id_t* p_adv_id, const bap_bcast_id_t* p_bcast_id,
                                uint8_t broadcast_name_len, const uint8_t* p_broadcast_name)
{
    for (int i = 0; i < MAX_SRC_NUM; i++)
    {	
        if (memcmp(p_bcast_id, &src_array[i].bcast_id, sizeof(bap_bcast_id_t))==0)
        {//have a same id
        	if(!memcmp(p_broadcast_name, &src_array[i].broadcast_name, broadcast_name_len))
        	{
			memcpy(&src_array[i].broadcast_name, p_broadcast_name, broadcast_name_len);  
        	}
		return true;
        }
        if ((!memcmp(p_bcast_id, &src_array[i].bcast_id, sizeof(bap_bcast_id_t)))
                && (!memcmp(p_adv_id, &(src_array[i].adv_id), sizeof(bap_adv_id_t)))
                && (!memcmp(p_broadcast_name, &src_array[i].broadcast_name, broadcast_name_len)))
        {
            return true;
        }
    }
    printf("check_src_in_array: bcast_id 0x%x, 0x%x, 0x%x\n", 
                p_bcast_id->id[0], p_bcast_id->id[1], p_bcast_id->id[2]);
    return false;
}

void le_bis_sink_scan_src(const bap_adv_id_t* p_adv_id, const bap_bcast_id_t* p_bcast_id,
                                uint8_t broadcast_name_len, const uint8_t* p_broadcast_name)
{
	// printf("bcast_id: %x %x %x\n", p_bcast_id->id[0], p_bcast_id->id[1], p_bcast_id->id[2]);
	// printf("name: %s\n", p_broadcast_name);

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

            printf("found src: src_idx: %d, bcast_id 0x%x:0x%x:0x%x, addr 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n", 
                        i, src_array[i].bcast_id.id[0], src_array[i].bcast_id.id[1],
                        src_array[i].bcast_id.id[2], src_array[i].adv_id.addr[0], src_array[i].adv_id.addr[1], 
                        src_array[i].adv_id.addr[2],src_array[i].adv_id.addr[3], src_array[i].adv_id.addr[4], 
                        src_array[i].adv_id.addr[5]); 

            // Set_bis_role(BIS_SINK);
            // le_bis_sink_add_src(src_array[i].src_idx); //baikal
            return;
        }
    }
}

void printf_vaild_src(void)
{
    for (int i = 0; i < MAX_SRC_NUM; i++)
    {
        if (src_array[i].src_idx != GAF_INVALID_LID)
        {
            printf("vaild src: src_idx: %d, bcast_id 0x%x:0x%x:0x%x, addr 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n", 
                        i, src_array[i].bcast_id.id[0], src_array[i].bcast_id.id[1],
                        src_array[i].bcast_id.id[2], src_array[i].adv_id.addr[0], src_array[i].adv_id.addr[1], 
                        src_array[i].adv_id.addr[2],src_array[i].adv_id.addr[3], src_array[i].adv_id.addr[4], 
                        src_array[i].adv_id.addr[5]); 
        }
    }   
}

void le_bis_sink_add_src(uint8_t src_idx)
{
    for (int i = 0; i < MAX_SRC_NUM; i++)
    {
        if (src_array[i].src_idx != GAF_INVALID_LID)
        {
            if (src_array[i].src_idx == src_idx)
            {
                break;
            }
        }
        
		if (i == MAX_SRC_NUM - 1)
        {
            return;
        }
    }

    printf("le_bis_sink_add_src: src_idx: %d, bcast_id 0x%x:0x%x:0x%x, addr 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n", 
                src_idx, src_array[src_idx].bcast_id.id[0], src_array[src_idx].bcast_id.id[1], 
                src_array[src_idx].bcast_id.id[2], src_array[src_idx].adv_id.addr[0], src_array[src_idx].adv_id.addr[1], 
                src_array[src_idx].adv_id.addr[2],src_array[src_idx].adv_id.addr[3], src_array[src_idx].adv_id.addr[4], 
                src_array[src_idx].adv_id.addr[5]);

    if (!CHECKB(p_env->status_bf, STATUS_GROUP_SYNCHRONIZED)
        && !CHECKB(p_env->status_bf, STATUS_GROUP_SYNCHRONIZING)
        &&!CHECKB(p_env->status_bf, STATUS_PA_SYNCHRONIZED)
        && !CHECKB(p_env->status_bf, STATUS_PA_SYNCHRONIZING)
        && !CHECKB(p_env->status_bf, STATUS_GROUP_STREAMED))
    {
        set_sink_sync_lost(0);
	    printf("add src: %d\n", src_idx);
        memcpy(&(p_env->adv_id), &(src_array[src_idx].adv_id), sizeof(bap_adv_id_t));
        memcpy(&(p_env->broadcast_id), &(src_array[src_idx].bcast_id), sizeof(bap_bcast_id_t));

        if (Get_bis_role() == BIS_SINK)
        {
            p_env->src_idx = src_idx;
            le_bis_sink_state_scan(EVENT_SCAN_SOURCE_FOUND, GAF_ERR_NO_ERROR);
        }
    }   
	else
	{
		printf("add src error, p_env->status_bf: %d\n", p_env->status_bf);
		//SETB(p_env->status_bf, STATUS_GROUP_SYNCHRONIZED, 0);
	}
}

void app_add_src_for_sink(uint8_t src_idx)
{
    Set_bis_role(BIS_SINK);
    le_bis_sink_add_src(src_idx);
}

void le_bis_sink_remove_src(void)
{
    uint8_t src_idx = p_env->src_idx;
    printf("le_bis_sink_remove_src: src_idx: %d, bcast_id 0x%x:0x%x:0x%x, addr 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n", 
                src_idx, src_array[src_idx].bcast_id.id[0], src_array[src_idx].bcast_id.id[1], 
                src_array[src_idx].bcast_id.id[2], src_array[src_idx].adv_id.addr[0], src_array[src_idx].adv_id.addr[1], 
                src_array[src_idx].adv_id.addr[2],src_array[src_idx].adv_id.addr[3], src_array[src_idx].adv_id.addr[4], 
                src_array[src_idx].adv_id.addr[5]);

    memset(&(p_env->adv_id), 0, sizeof(bap_adv_id_t));
    memset(&(p_env->broadcast_id), 0, sizeof(bap_bcast_id_t));
    p_env->src_idx = GAF_INVALID_LID;

    reset_src_array();
}
#endif

#if (APP_GAF_PBP)
__STATIC void le_bis_sink_cb_public_bcast_source(const bap_adv_id_t* p_adv_id,
                                                           const bap_bcast_id_t* p_bcast_id, uint8_t pbp_features_bf,
                                                           uint8_t broadcast_name_len,
                                                           const uint8_t* p_broadcast_name, uint8_t metadata_len,
                                                           const uint8_t* p_metadata, const gaf_adv_report_air_info_t* p_air_info)
{
    // if (p_air_info->rssi < -60)
    // {
    //     return;
    // }
#ifndef SINK_CHOOSE_SRC
    uint16_t status;
    if (get_sink_sync_lost())
    {
        if (p_adv_id->addr_type != p_env->src_adv_id.addr_type)
        {
            return;
        }
        else
        {
            if (p_adv_id->addr_type == GAP_ADDR_PUBLIC)
            {
                if (memcmp(p_adv_id->addr, p_env->src_adv_id.addr, 6))
                {
                    return;
                } 
                // APP_DBG("le_bis_sink_cb_public_bcast_source: SOURCE_RECONN 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n",
                //             p_adv_id->addr[0], p_adv_id->addr[1], p_adv_id->addr[2], p_adv_id->addr[3], p_adv_id->addr[4], p_adv_id->addr[5]);  
            }       

            // if (get_sync_pa_accept() == 0)
            // {
            //     SETB(p_env->status_bf, STATUS_SOURCE_FOUND, 0);

            //     status = bap_bc_scan_pa_terminate(p_env->pa_lid);
            //     if (status)
            //     {
            //         APP_DBG("bap_bc_scan_pa_terminate: err 0x%x, pa_lid 0x%x\n", status, p_env->pa_lid); 
            //     }
            //     // else
            //     {
            //         set_pa_sync_accept(0xff);
            //     }
            // }   
        }
    }
#ifndef COMMON_SINK
#if BLE_TWS_SUPPORT
	if(le_tws_is_enable())
	{
		if( !LE_Tws_Is_Srcid_match(p_bcast_id->id) )
		{
			return;
		}
		bis_src_type = LE_AUDIO_SRC_MV_BIS;
	}
	else
#endif
	{
#ifndef ONE_VS_ONE
		uint8_t type = get_bis_sink_target_bcast_type(p_broadcast_name,broadcast_name_len);
        if(type != LE_AUDIO_SRC_UNKNOWN)
        {
            if (check_bis_sink_state() == 8)
            {
                APP_DBG("found src: %s\n",get_bis_sink_type_name(type));
				if(type == LE_AUDIO_SRC_MV_BIS)
				{
					APP_DBG("found MV src\n");
					if( !Is_Srcid_match(p_bcast_id->id) )
					{
						return;
					}
				}
            }
            if(type == LE_AUDIO_SRC_PHONE_1)
            {
            	APP_DBG("le_bis_sink_cb_public_bcast_source: phone src found: 0x%x\n", p_adv_id->addr_type);
            }
            bis_src_type = type;
        }
		else
		{
			return;
		}	
#else
#ifndef DUAL_BIS_MODE
        if (!memcmp(p_broadcast_name, LE_AUDIO_MV_BROADCAST_NAME, strlen(LE_AUDIO_MV_BROADCAST_NAME)))
        {
            APP_DBG("found src(MAC): 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n",
                        p_adv_id->addr[0], p_adv_id->addr[1], p_adv_id->addr[2],
                        p_adv_id->addr[3], p_adv_id->addr[4], p_adv_id->addr[5]);

            bis_src_type = LE_AUDIO_SRC_MV_BIS;
        }

        
#else
        if (!memcmp(p_broadcast_name, LE_AUDIO_DUALBIS_BROADCAST_NAME, strlen(LE_AUDIO_DUALBIS_BROADCAST_NAME)))
        {
            APP_DBG("found src(MAC): 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n",
                        p_adv_id->addr[0], p_adv_id->addr[1], p_adv_id->addr[2],
                        p_adv_id->addr[3], p_adv_id->addr[4], p_adv_id->addr[5]);

            bis_src_type = LE_AUDIO_SRC_MV_DUALBIS;
        }

#endif
        else
        {
            printf("baikal: other src\n");
            return;
        }
#endif
	}
#endif
    APP_DBG("bis_src_type 0x%x, sync_lost: 0x%x, status 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, role 0x%x\n",
    			bis_src_type, get_sink_sync_lost(), CHECKB(p_env->status_bf, STATUS_GROUP_SYNCHRONIZED),
                CHECKB(p_env->status_bf, STATUS_GROUP_SYNCHRONIZING), 
                CHECKB(p_env->status_bf, STATUS_PA_SYNCHRONIZED),
                CHECKB(p_env->status_bf, STATUS_PA_SYNCHRONIZING), 
                CHECKB(p_env->status_bf, STATUS_GROUP_STREAMED),
                Get_bis_role());

    if (!CHECKB(p_env->status_bf, STATUS_GROUP_SYNCHRONIZED)
            && !CHECKB(p_env->status_bf, STATUS_GROUP_SYNCHRONIZING)
            &&!CHECKB(p_env->status_bf, STATUS_PA_SYNCHRONIZED)
            && !CHECKB(p_env->status_bf, STATUS_PA_SYNCHRONIZING)
            && !CHECKB(p_env->status_bf, STATUS_GROUP_STREAMED))
    {
        memcpy(&(p_env->adv_id), p_adv_id, sizeof(bap_adv_id_t));
        memcpy(&(p_env->broadcast_id), p_bcast_id, sizeof(bap_bcast_id_t));
        if (Get_bis_role() == BIS_SINK)
        {
            le_bis_sink_state_scan(EVENT_SCAN_SOURCE_FOUND, GAF_ERR_NO_ERROR);
        }
    }
#else
    // printf("le_bis_sink_cb_public_bcast_source: bcast_id 0x%x:0x%x:0x%x\n", p_bcast_id->id[0], p_bcast_id->id[1], p_bcast_id->id[2]);
    // le_bis_sink_scan_src(p_adv_id, p_bcast_id, broadcast_name_len, p_broadcast_name); 
#endif
}
#endif

__STATIC void app_cb_timer_biginfo_handler(co_timer_t* p_timer)
{
    APP_DBG("app_cb_timer_biginfo_handler\n");
    //stop pa sync
    if (CHECKB(p_env->status_bf, STATUS_PA_SYNCHRONIZED))
    {
        uint16_t status = bap_bc_scan_pa_terminate(p_env->pa_lid);
        if (!status)
        {
            SETB(p_env->status_bf, STATUS_PA_SYNCHRONIZING, 0);
        }
        else
        {
            APP_DBG("bap_bc_scan_pa_terminate: err 0x%x\n", status);
        }
    }
}

__STATIC void le_bis_sink_cb_pa_established(uint8_t pa_lid, const bap_adv_id_t* p_adv_id, uint8_t phy,
                                                      uint16_t interval_frames)
{
	APP_DBG("le_bis_sink_cb_pa_established, pa_lid 0x%x\n", pa_lid);
    if (pa_sync_lost)
    {
	    le_bis_sink_state_scan(EVENT_SCAN_STOP, GAF_ERR_NO_ERROR);
        pa_sync_lost = 0;
    }
    p_env->pa_lid = pa_lid;
    memcpy(p_env->src_adv_id.addr, p_adv_id->addr, 6);
    p_env->src_adv_id.addr_type = p_adv_id->addr_type;

    if (get_sink_wed_vaild() == 0)
    {
        // wait for biginfo
        co_timer_config(&(p_env->biginfo_timer_user), app_cb_timer_biginfo_handler);
        co_timer_start(&(p_env->biginfo_timer_user), APP_BIGINFO_START_WAIT_MS);
    }

    le_bis_sink_state_group(EVENT_GROUP_PA_SYNCHRONIZED, GAF_ERR_NO_ERROR);
}

__STATIC void le_bis_sink_cb_pa_terminated(uint8_t pa_lid, uint8_t reason)
{
    uint16_t status __attribute__((unused));
	APP_DBG("le_bis_sink_cb_pa_terminated, reason 0x%x\n", reason);
    le_bis_sink_state_group(EVENT_GROUP_PA_DESYNCHRONIZED, reason);
}

__STATIC void le_bis_sink_cb_pa_report(uint8_t pa_lid, const gaf_adv_report_air_info_t* p_air_info,
                                                 uint16_t length, const uint8_t* p_data)
{
	// APP_DBG("le_bis_sink_cb_pa_report\n");
}

__STATIC void le_bis_sink_cb_big_info_report(uint8_t pa_lid, const gapm_le_big_info_t* p_report)
{
    APP_DBG("le_bis_sink_cb_big_info_report: num_bis %d, encrypted 0x%x, codec vaild %d\n", 
                    p_report->num_bis, p_report->encrypted, p_env->is_codec_cfg_vaild);
    if (CHECKB(p_env->status_bf, STATUS_GROUP_STREAMED))
    {
        return;
    }

    if (!p_env->is_codec_cfg_vaild)
    {
        return;
    }

    if (p_report->encrypted
#if BLE_TWS_SUPPORT
    		&& !le_tws_is_enable()
#endif
			)
    {
#ifdef LE_AUDIO_PHONE_BCAST_CODE
#ifndef COMMON_SINK
        if (bis_src_type == LE_AUDIO_SRC_PHONE_1)
#endif
        {
            p_env->is_encrypted = true;
            set_phone_bcast_code(LE_AUDIO_PHONE_BCAST_CODE);
        }
#endif
    }

    le_bis_sink_state_group(EVENT_GROUP_INFO_RECEIVED, GAF_ERR_NO_ERROR);
}

__STATIC void le_bis_sink_cb_group_report(uint8_t pa_lid, uint8_t nb_subgroups, uint8_t nb_streams,
                                                    uint32_t pres_delay_us)
{
	BT_DBG("le_bis_sink_cb_group_report: pres_delay_us %d, %d, %d\n", pres_delay_us, nb_streams, nb_subgroups);
	set_bis_pres_delay_us(pres_delay_us);
#ifdef COMMON_SINK
    if (nb_streams == 1)
    {
        p_env->sync_bf = 1;
        set_sink_wed_vaild(0);
    }
    else if (nb_streams == 2)
    {
        p_env->sync_bf = 3;   
        set_sink_wed_vaild(1);     
    }
#endif
}

__STATIC void le_bis_sink_cb_subgroup_report(uint8_t pa_lid, uint8_t sgrp_id, uint32_t stream_pos_bf,
                                                       const gaf_codec_id_t* p_codec_id, const bap_cfg_ptr_t* p_cfg,
                                                       const bap_cfg_metadata_ptr_t* p_metadata)
{
	BT_DBG("le_bis_sink_cb_subgroup_report: codec_id: 0x%x, frame_dur: %d, frame_octet %d, frames_sdu %d, location_bf 0x%x, sampling_freq 0x%x, context_bf 0x%x, is_DUALBIS_clip %d\n", 
                p_codec_id->codec_id[0], p_cfg->param.frame_dur, p_cfg->param.frame_octet, p_cfg->param.frames_sdu, 
                p_cfg->param.location_bf, p_cfg->param.sampling_freq, p_metadata->param.context_bf, bis_src_type);

#ifndef COMMON_SINK
    if (p_cfg->param.location_bf == (GAF_LOC_FRONT_LEFT_BIT | GAF_LOC_FRONT_RIGHT_BIT))
    {
        if (bis_src_type == LE_AUDIO_SRC_MV_DUALBIS)
        {
            set_sink_wed_vaild(1);//2bis
            p_env->sync_bf = p_cfg->param.location_bf;            
        }
        else if (bis_src_type == LE_AUDIO_SRC_MV_BIS)
        {
            set_sink_wed_vaild(0);    
            p_env->sync_bf = 1;        
        }
        else if (bis_src_type == LE_AUDIO_SRC_DUALBIS_CLIP || bis_src_type == LE_AUDIO_SRC_DUALBIS_PARTY)
        {
            set_sink_wed_vaild(1);//2bis
            p_env->sync_bf = p_cfg->param.location_bf;
        }
        else
        {
            set_sink_wed_vaild(1);//2bis
            p_env->sync_bf = p_cfg->param.location_bf;            
        }
    }
    else if (p_cfg->param.location_bf == GAF_LOC_FRONT_LEFT_BIT)
    {
        set_sink_wed_vaild(0);//1bis        
        p_env->sync_bf = p_cfg->param.location_bf;
    }
    else if (!p_cfg->param.location_bf)
    {
        // if ( is_DUALBIS_clip || (bis_src_type == LE_AUDIO_SRC_PHONE_1))
        set_sink_wed_vaild(1);//2bis, DUALBIS filp7 , galaxy  
        p_env->sync_bf = GAF_LOC_FRONT_LEFT_BIT | GAF_LOC_FRONT_RIGHT_BIT;    
    }

#endif
    if (p_codec_id->codec_id[0] == GAPI_CODEC_FORMAT_LC3)
    {
        p_env->bis_metadata.param.context_bf = p_metadata->param.context_bf;
    }
}

__STATIC bool check_codec_cfg_is_vaild(const gaf_codec_id_t* p_codec_id, const bap_cfg_ptr_t* p_cfg)
{
    bool is_vaild = false;
    uint8_t	lc3_channel = 0;     // Initialize to safe default
    uint32_t lc3_bitrate = 0;     // Initialize to safe default  
    uint16_t lc3_frame_len = 0;   // Initialize to safe default
    uint16_t SampleRate = 0;
    APP_DBG("codec_id: 0x%x, sampling_freq 0x%x, frame_dur %d, frame_octet %d, frames_sdu %d\n", 
                    p_codec_id->codec_id[0], p_cfg->param.sampling_freq, p_cfg->param.frame_dur, p_cfg->param.frame_octet, p_cfg->param.frames_sdu);
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
            #if 1   //兼容以前的代码         
                        case 100:
                        case 120:
                            if(p_cfg->param.frames_sdu == 1)
                            {
                                lc3_packet_len = p_cfg->param.frame_octet;
                                lc3_bitrate = 800 * p_cfg->param.frame_octet;
                                lc3_frame_len = p_cfg->param.frame_octet;
                                lc3_channel = LC3_MONO;
                                SampleRate = 48000;
                                is_vaild = true;
                                break;
                            } 
                        case 160:
                        case 240:
                            if(p_cfg->param.frames_sdu == 1)
                            {
                                lc3_packet_len = p_cfg->param.frame_octet;
                                lc3_bitrate = 800 * (p_cfg->param.frame_octet) >> 1;
                                lc3_frame_len = (p_cfg->param.frame_octet) >> 1;
                                lc3_channel = LC3_STEREO;
                                SampleRate = 48000;
                                is_vaild = true;
                                break;
                            } 
				#if (VENDOR_DATA_LEN > 0)
                        case (240 + VENDOR_DATA_LEN):
                        case (160 + VENDOR_DATA_LEN):
                            if(p_cfg->param.frames_sdu == 1)
                            {
                                lc3_packet_len = p_cfg->param.frame_octet - VENDOR_DATA_LEN;
                                lc3_bitrate = 800 * (p_cfg->param.frame_octet - VENDOR_DATA_LEN) >> 1;
                                lc3_frame_len = (p_cfg->param.frame_octet - VENDOR_DATA_LEN) >> 1;
                                lc3_channel = LC3_STEREO;
                                SampleRate = 48000;                                                        
                                is_vaild = true;
                                break; 
                            } 
				#endif
            #endif            
                        default:
                        {
                            uint16_t frame_octet = p_cfg->param.frame_octet;
                            #if (VENDOR_DATA_LEN > 0)
                                frame_octet -= VENDOR_DATA_LEN;
                            #endif  
                                if (p_cfg->param.location_bf == (GAF_LOC_FRONT_LEFT_BIT | GAF_LOC_FRONT_RIGHT_BIT))//stereo
                                {
                                    lc3_channel = LC3_STEREO;
                                }
                                else //if (p_cfg->param.location_bf == GAF_LOC_FRONT_LEFT_BIT) //MONO
                                {
                                    lc3_channel = LC3_MONO;
                                }
                                lc3_packet_len = frame_octet;
                                lc3_bitrate = 800 * frame_octet;
                                lc3_frame_len = frame_octet; 
                                if(p_cfg->param.frames_sdu == 2)
                                {
                                    lc3_frame_len   /= 2;
                                    lc3_bitrate     /= 2;
                                }            
                                SampleRate = 48000;                                                        
                                is_vaild = true;                      
                        }
                        break;
                    } 
                }
            }break;

            case BAP_SAMPLING_FREQ_24000HZ:
            {
                if (p_cfg->param.frame_dur == BAP_FRAME_DUR_10MS)
                {
                    switch (p_cfg->param.frame_octet)
                    {
                    default:
                        {
                            uint16_t frame_octet = p_cfg->param.frame_octet;
                            #if (VENDOR_DATA_LEN > 0)
                                frame_octet -= VENDOR_DATA_LEN;
                            #endif  
                                if (p_cfg->param.location_bf == (GAF_LOC_FRONT_LEFT_BIT | GAF_LOC_FRONT_RIGHT_BIT))//stereo
                                {
                                    lc3_channel = LC3_STEREO;
                                }
                                else //if (p_cfg->param.location_bf == GAF_LOC_FRONT_LEFT_BIT) //MONO
                                {
                                    lc3_channel = LC3_MONO;
                                }
                                lc3_packet_len = frame_octet;
                                lc3_bitrate = 800 * frame_octet;
                                lc3_frame_len = frame_octet; 
                                if(p_cfg->param.frames_sdu == 2)
                                {
                                    lc3_frame_len   /= 2;
                                    lc3_bitrate     /= 2;
                                }            
                                SampleRate = 24000;                                                        
                                is_vaild = true;                      
                        }
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
    APP_DBG("check_codec_cfg_is_vaild %d, %d, %d, %d, %d,%d\n", 
                is_vaild, (unsigned long)lc3_packet_len, (unsigned long)lc3_bitrate, 
                (unsigned long)lc3_frame_len, lc3_channel,SampleRate);
    return is_vaild;
} 

__STATIC void le_bis_sink_cb_stream_report(uint8_t pa_lid, uint8_t sgrp_id, uint8_t stream_pos,
                                                     const gaf_codec_id_t* p_codec_id, const bap_cfg_ptr_t* p_cfg, 
                                                    uint8_t* volume)
{
//    APP_DBG("le_bis_sink_cb_stream_report: codec_id: 0x%x, frame_dur: %lu, frame_octet %lu, frames_sdu %d, location_bf 0x%lx, sampling_freq 0x%x, stream_pos 0x%x, p_env->sync_bf 0x%X\n",
//                p_codec_id->codec_id[0], (unsigned long)p_cfg->param.frame_dur, (unsigned long)p_cfg->param.frame_octet, p_cfg->param.frames_sdu,
//                (unsigned long)p_cfg->param.location_bf, p_cfg->param.sampling_freq, stream_pos, p_env->sync_bf);

    if (stream_pos == (co_ctz(p_env->sync_bf) + 1))
    {
    	uint8_t	CodecFormat = LE_CODEC_FORMAT_NONE;
    	uint8_t	channel = 0;
	    // APP_DBG("le_bis_sink_cb_stream_report: GROUP_INFO_RECEIVED 0x%x, 0x%lx, p_codec_id 0x%x, 0x%lx\n", 
        //             stream_pos, (unsigned long)(co_ctz(p_env->sync_bf) + 1), p_codec_id->codec_id[0], 
//                    (unsigned long)p_cfg->param.location_bf);
        if (p_codec_id->codec_id[0] == GAPI_CODEC_FORMAT_LC3)
        {
            p_env->is_codec_cfg_vaild = check_codec_cfg_is_vaild(p_codec_id, p_cfg);
            if (!p_env->is_codec_cfg_vaild)
            {
                APP_DBG("LC3 codec cfg error\n");
                return;
            }

            memcpy(&(p_env->codec_cfg), &(p_cfg->param), sizeof(bap_cfg_param_t));
            CodecFormat = LE_AUDIO_LC3_TYPE;
        }
        else if (p_codec_id->codec_id[0] == GAPI_CODEC_FORMAT_VENDOR)//SBC
        {
        	CodecFormat = LE_CODEC_FORMAT_SBC;
            p_env->is_codec_cfg_vaild = true;
        }
        else
        {
            APP_DBG("other codec 0x%x\n", p_codec_id->codec_id[0]);
            return;
        }
        
        if (p_cfg->param.location_bf == (GAF_LOC_FRONT_LEFT_BIT | GAF_LOC_FRONT_RIGHT_BIT))//stereo
        {
        	channel = 2;
        }
        else if (p_cfg->param.location_bf == GAF_LOC_FRONT_LEFT_BIT) //MONO
        {
        	channel = 1;
        }

        LeAudioCodecFormatSet(CodecFormat,channel);
    }

#ifdef COMMON_SINK
    if (bis_src_type != LE_AUDIO_SRC_PHONE_1)
#endif
    {
        if ((sgrp_id == 0) && (stream_pos == 0) && (p_codec_id == 0) && (p_cfg == 0)) //bcast_code setting
        {
            if (get_sink_wed_vaild() == 0)
            {
                // if (bcast_code)
                // {
                //     uint8_t null_code[16] = {0};
                //     if (memcmp(bcast_code, null_code, 16) == 0)
                //     {
                //         p_env->is_encrypted = false;
                //     }
                //     else
                //     {
                //         p_env->is_encrypted = true;
                //         memcpy((void*)p_env->broadcast_code.bcast_code, bcast_code, 16);
                //     }
                // }
            }
            else
            {
                if (!volume)
                    return;

            	if(DUALBIS_sink_vol == 0xff)
            		DUALBIS_sink_vol = *volume;
                if(*volume != DUALBIS_sink_vol)
                {
                    APP_DBG("app get vol 0x%x:0x%X\n", *volume, bis_src_type);
                    if (bis_src_type == LE_AUDIO_SRC_DUALBIS_CLIP || bis_src_type == LE_AUDIO_SRC_DUALBIS_PARTY) //+-3 or +-6
                    // if (bis_src_type == LE_AUDIO_SRC_MV_DUALBIS) //+-3 or +-6
                    {
                        MessageContext		msgSend;
                        if(*volume <= 100)
                        {
                            if(DUALBIS_sink_vol < *volume)
                                msgSend.msgId = MSG_MUSIC_VOLUP;
                            else
                                msgSend.msgId = MSG_MUSIC_VOLDOWN;
                            MessageSend(GetMainMessageHandle(), &msgSend);
                        }  
                    }
                    DUALBIS_sink_vol = *volume;
                }
            }
        }
    }
}

bool le_bis_sink_is_synced(void)
{
    return CHECKB(p_env->status_bf, STATUS_GROUP_STREAMED);
}

void set_sink_synced(void)
{
    SETB(p_env->status_bf, STATUS_GROUP_STREAMED, 1);
}

__STATIC void clear_src_flag(void)
{
	bis_src_type = LE_AUDIO_SRC_UNKNOWN;
    set_sink_wed_vaild(0);
}

void app_stop_biginfo_timer(void)
{
    co_timer_stop(&p_env->biginfo_timer_user);
}

__STATIC void le_bis_sink_cb_bap_bc_sink_cmp_evt(uint8_t cmd_type, uint16_t status, uint8_t grp_lid,  uint8_t stream_pos)
{
	APP_DBG("le_bis_sink_cb_bap_bc_sink_cmp_evt = %d, status 0x%X, %d\n", cmd_type, status, stream_pos);
    switch (cmd_type)
    {
        case BAP_BC_SINK_CMD_TYPE_START_STREAMING:
        {
            if (!status)
            {
                co_timer_stop(&p_env->biginfo_timer_user);
            	set_sink_sync_lost(0);
            	DUALBIS_sink_vol = 0xff;
#if 1
        	    bis_sync_audio_close();
#endif
                SETB(p_env->status_bf, STATUS_GROUP_STREAMED, 1);
            }
        	le_bis_sink_state_group(EVENT_GROUP_STREAMING_STARTED, status);
        } break;

        case BAP_BC_SINK_CMD_TYPE_STOP_STREAMING:
        {
            if ((get_sink_wed_vaild() == 1) && (stream_pos == 1)) //waitting for another bis streaming
            {
                break;
            }
            
        	le_bis_sink_state_group(EVENT_GROUP_STOP, status);
            if (!status)
            {
                SETB(p_env->status_bf, STATUS_GROUP_STREAMED, 0);

                clear_src_flag();
            }
        } break;

        case BAP_BC_SINK_CMD_TYPE_ENABLE:
        {
            if (status)
            {
                p_env->grp_lid = 0xff;
            }
        } break;

        case BAP_BC_SINK_CMD_TYPE_DISABLE:
        {
            if (!status)
            {
                le_bis_sink_state_group(EVENT_GROUP_DESYNCHRONIZED, GAF_ERR_NO_ERROR);
            }
        } break;

        default: { } break;
    }
}

__STATIC void le_bis_sink_cb_bap_bc_sink_quality_cmp_evt(uint16_t status, uint8_t grp_lid,
                                                                   uint8_t stream_pos, uint32_t crc_error_packets,
                                                                   uint32_t rx_unrx_packets,
                                                                   uint32_t duplicate_packets)
{
	APP_DBG("le_bis_sink_cb_bap_bc_sink_quality_cmp_evt\n");
}


/*
 * STATE MACHINES
 ****************************************************************************************
 */
__STATIC void le_bis_sink_state_group(uint8_t event, uint16_t status)
{
	APP_DBG("le_bis_sink_state_group  %d, status 0x%x\n", event, status);
    uint8_t pa_lid;
    switch (event)
    {
        case EVENT_GROUP_PLAY:
            if (CHECKB(p_env->status_bf, STATUS_PA_SYNCHRONIZED))
            {
            	break;
            }

            uint8_t pa_lid;
            if (Get_bis_role() == BIS_SINK)
            {
                uint16_t pa_sync_to = 0x3ff;

                APP_DBG("EVENT_GROUP_PLAY: is jbl 0x%x\n", get_sink_wed_vaild());

                status = bap_bc_scan_pa_synchronize(&(p_env->adv_id), 0, BAP_BC_SCAN_REPORT_MASK, pa_sync_to, 0, &(pa_lid));                         

                if (!status)
                {
                    SETB(p_env->status_bf, STATUS_PA_SYNCHRONIZING, 1);
                    p_env->pa_lid = pa_lid;
                }
                else
                {
                    APP_DBG("EVENT_GROUP_PLAY: bap_bc_scan_pa_synchronize: error 0x%x\n", status);
#if 1 // Fix: clear SOURCE_FOUND on PA sync failure so next ad report can retry
                    SETB(p_env->status_bf, STATUS_SOURCE_FOUND, 0);
#endif
                }
            }
            break;

#ifdef DISCOV_BIS
        case EVENT_GROUP_DISCOV:
            if (Get_bis_role() == BIS_SINK)
            {
                status = bap_bc_discov_pa_synchronize(&(p_env->discov_adv_id), 0, BAP_BC_SCAN_REPORT_PER_ADV_DATA_BIT | BAP_BC_SCAN_REPORT_BIG_INFO_BIT, 0x3ff, 0, &(pa_lid));
                if (status)
                {
                    APP_DBG("EVENT_GROUP_DISCOV: bap_bc_discov_pa_synchronize: error 0x%x\n", status);
                }
            }
            break;
#endif
        case EVENT_GROUP_STOP:
            if (!CHECKB(p_env->status_bf, STATUS_GROUP_SYNCHRONIZED)) { break; }
            status = bap_bc_sink_disable(p_env->grp_lid);
            if (status)
            {
                APP_DBG("le_bis_sink_state_group: bap_bc_sink_disable error 0x%x\n", status);

            }
            break;

        case EVENT_GROUP_PA_SYNCHRONIZED:
            if (!status)
            {
                SETB(p_env->status_bf, STATUS_PA_SYNCHRONIZING, 0);
                SETB(p_env->status_bf, STATUS_PA_SYNCHRONIZED, 1);
            }
            break;

        case EVENT_GROUP_INFO_RECEIVED:
            if (get_sink_wed_vaild() == 1/* && (bis_src_type == LE_AUDIO_SRC_DUALBIS_CLIP || bis_src_type == LE_AUDIO_SRC_DUALBIS_PARTY)*/)
            {
                status = bap_bc_scan_pa_report_ctrl(p_env->pa_lid, BAP_BC_SCAN_REPORT_ANNOUNCE_LVL_3_BIT); 
                if (status)
                {
                    APP_DBG("1_bap_bc_scan_pa_report_ctrl: status 0x%x\n", status);
                }
            }
            else
            {
                status = bap_bc_scan_pa_report_ctrl(p_env->pa_lid, 0);
                if (status)
                {
                    APP_DBG("2_bap_bc_scan_pa_report_ctrl: status 0x%x\n", status);
                }
            }

            APP_DBG("EVENT_GROUP_INFO_RECEIVED: is_encrypted 0x%x, %d\n", p_env->is_encrypted, p_env->sync_bf);

            if (p_env->is_encrypted)
            {
                if (Get_bis_role() == BIS_SINK)
                {
                    status = bap_bc_sink_enable(p_env->pa_lid, &(p_env->broadcast_id), p_env->sync_bf,
                                                    &(p_env->broadcast_code), 0, 100, &(p_env->grp_lid));
                    if (!status)
                    {
                        SETB(p_env->status_bf, STATUS_GROUP_SYNCHRONIZING, 1);
                    }
                }
            }
            else
            {
                APP_DBG("bap_bc_sink_enable pa_lid=0x%x, sync_bf=0x%x, grp_lid=0x%x\n", 
                            p_env->pa_lid, p_env->sync_bf, p_env->grp_lid);
                uint8_t grp_lid;
                if (Get_bis_role() == BIS_SINK)
                {
                    status = bap_bc_sink_enable(p_env->pa_lid, &(p_env->broadcast_id), p_env->sync_bf,
                                                    NULL, 0, 100, &(grp_lid));
                    if (status)
                    {
                        APP_DBG("bap_bc_sink_enable error 0x%x\n", status);
                    }
                    else
                    {
                        p_env->grp_lid = grp_lid;
                        SETB(p_env->status_bf, STATUS_GROUP_SYNCHRONIZING, 1);                    
                    }
                }
            }

            break;

        case EVENT_GROUP_SYNCHRONIZED:
            SETB(p_env->status_bf, STATUS_GROUP_SYNCHRONIZED, 1);
            SETB(p_env->status_bf, STATUS_GROUP_SYNCHRONIZING, 0);
            // bis_sync_falied_retry = 1;

            // if (!get_sink_wed_vaild()/* || (bis_src_type != LE_AUDIO_SRC_DUALBIS_CLIP && bis_src_type != LE_AUDIO_SRC_DUALBIS_PARTY)*/)
            // {
            //     status = bap_bc_scan_pa_terminate(p_env->pa_lid);
            //     if (!status)
            //     {
            //         SETB(p_env->status_bf, STATUS_PA_SYNCHRONIZING, 0);        
            //     }
            //     else
            //     {
            //         APP_DBG("bap_bc_scan_pa_terminate: err 0x%x\n", status);
            //     }
            // }
            // else
            // {
            //     le_bis_sink_stop_scan();
            // }

        // no break;

        case EVENT_GROUP_STREAMING_STARTED:
        {
             if (p_env->sync_bf != 0)
             {
                gaf_codec_id_t codec_id = GAF_CODEC_ID_LC3;
                uint8_t stream_pos = co_ctz(p_env->sync_bf) + 1;
                uint8_t buffer[sizeof(gaf_ltv_t) + UTILS_AUDIO_LE_CODEC_CFG_PACKED_LEN];

                utils_audio_le_pack_codec_cfg((gaf_ltv_t*)buffer, &(p_env->codec_cfg));

                APP_DBG("stream_pos %d\n", stream_pos);
                uint8_t dp_id = BIS_SINK_DATA_PATH_ID; // default value
                if (stream_pos == 1)
                {
                    dp_id = BIS_SINK_DATA_PATH_ID;
                }
                else if (stream_pos == 2)
                {
                    dp_id = BIS_SINK1_DATA_PATH_ID;    
                }

                status = bap_bc_sink_start_streaming(p_env->grp_lid, stream_pos, &codec_id,
                                                                dp_id, 0, (gaf_ltv_t*)buffer);
            
                if (status)
                {
                    APP_DBG("EVENT_GROUP_STREAMING_STARTED error 0x%x, stream_pos %d\n", status, stream_pos);
                }

                p_env->sync_bf &= ~CO_BIT(stream_pos - 1);

            }
            else
            {
                if (!get_sink_wed_vaild())
                {
                    status = bap_bc_scan_pa_terminate(p_env->pa_lid);
                    if (!status)
                    {
                        SETB(p_env->status_bf, STATUS_PA_SYNCHRONIZING, 0);        
                    }
                    else
                    {
                        APP_DBG("bap_bc_scan_pa_terminate: err1 0x%x\n", status);
                    }
                }
                else
                {
#if 0 // Fix: also terminate PA in dual-BIS path, otherwise PA scan activity leaks
                    if (CHECKB(p_env->status_bf, STATUS_PA_SYNCHRONIZED) || CHECKB(p_env->status_bf, STATUS_PA_SYNCHRONIZING))
                    {
                        status = bap_bc_scan_pa_terminate(p_env->pa_lid);
                        APP_DBG("bap_bc_scan_pa_terminate dual-BIS: err 0x%x\n", status);
                        // PA status bits will be cleared by EVENT_GROUP_PA_DESYNCHRONIZED callback
                    }
#endif
                    if (CHECKB(p_env->status_bf, STATUS_SCAN_ENABLE))
                    {
                        le_bis_sink_stop_scan();
                    }
                }
            }
        } break;

        case EVENT_GROUP_PA_DESYNCHRONIZED:
            if (!status)
            {
                SETB(p_env->status_bf, STATUS_PA_SYNCHRONIZED, 0);
                SETB(p_env->status_bf, STATUS_PA_SYNCHRONIZING, 0);
                if (get_sink_sync_lost())
                {
                    SETB(p_env->status_bf, STATUS_SOURCE_FOUND, 0);   
                }
                p_env->pa_lid = GAF_INVALID_LID; 

                if (!get_sink_sync_lost()) //normal process
                {
                    if (CHECKB(p_env->status_bf, STATUS_SCAN_ENABLE))
                    {
	                    le_bis_sink_state_scan(EVENT_SCAN_STOP, GAF_ERR_NO_ERROR);
                    }
                }
// #else
//                 le_bis_sink_state_scan(EVENT_SCAN_START, GAF_ERR_NO_ERROR);    
// #endif
            }
            else if (status == BAP_BC_SCAN_PA_TERM_REASON_ESTAB_TIMEOUT)
            {
                SETB(p_env->status_bf, STATUS_PA_SYNCHRONIZED, 0);
                SETB(p_env->status_bf, STATUS_PA_SYNCHRONIZING, 0);
                SETB(p_env->status_bf, STATUS_SOURCE_FOUND, 0);  

                // {
                // 	uint8_t addr[GAP_BD_ADDR_LEN] = {0x00};
                // 	if(memcmp(addr, p_env->src_adv_id.addr, 6))
                // 		set_sink_sync_lost(1);
                // }
            }
            else if (status == BAP_BC_SCAN_PA_TERM_REASON_TIMEOUT)
            {
                SETB(p_env->status_bf, STATUS_PA_SYNCHRONIZED, 0);
                SETB(p_env->status_bf, STATUS_PA_SYNCHRONIZING, 0);
                SETB(p_env->status_bf, STATUS_SOURCE_FOUND, 0);   
                APP_DBG("pa timeout %d\n", get_sync_pa_accept());


                // {
                // 	uint8_t addr[GAP_BD_ADDR_LEN] = {0x00};
                // 	if(memcmp(addr, p_env->src_adv_id.addr, 6))
                // 		set_sink_sync_lost(1);
                // }
            }

            if (status)
            {
                le_bis_sink_printf_state();
                if (CHECKB(p_env->status_bf, STATUS_GROUP_STREAMED) && get_src_wed_vaild())
                {
                    pa_sync_lost = 1;
                    if (!CHECKB(p_env->status_bf, STATUS_SCAN_ENABLE))
                    {
                        le_bis_sink_state_scan(EVENT_SCAN_START, GAF_ERR_NO_ERROR);     
                    }
                }

                if (!p_env->status_bf)
                {
                    if (get_sink_sync_lost())
                    {
                        if (!CHECKB(p_env->status_bf, STATUS_SCAN_ENABLE))
                        {
	                        le_bis_sink_state_scan(EVENT_SCAN_START, GAF_ERR_NO_ERROR);    
                        }
                    }
                }
            }
            break;

        case EVENT_GROUP_DESYNCHRONIZED:
        {
            SETB(p_env->status_bf, STATUS_GROUP_SYNCHRONIZED, 0);
            SETB(p_env->status_bf, STATUS_GROUP_SYNCHRONIZING, 0);   

            if (CHECKB(p_env->status_bf, STATUS_GROUP_STREAMED))
            {
                SETB(p_env->status_bf, STATUS_GROUP_STREAMED, 0);     
            }
            
            if (status == BAP_BC_SINK_FAILED)
            {
                SETB(p_env->status_bf, STATUS_PA_SYNCHRONIZING, 0);
                SETB(p_env->status_bf, STATUS_SOURCE_FOUND, 0);
                le_bis_sink_handle_stop();  
                clear_src_flag();      
            }    
            else if (status == BAP_BC_SINK_PEER_TERMINATE)
            {
                if (CHECKB(p_env->status_bf, STATUS_SCAN_ENABLE))
                {
                    le_bis_sink_stop_scan();
                }

                // SETB(p_env->status_bf, STATUS_SOURCE_FOUND, 0);
                p_env->grp_lid = GAF_INVALID_LID;  
                le_bis_sink_handle_stop(); 
                clear_src_flag();                     
            }
            else if (status == BAP_BC_SINK_LOST)
            {
                SETB(p_env->status_bf, STATUS_SOURCE_FOUND, 0);
                p_env->grp_lid = GAF_INVALID_LID;  
                le_bis_sink_handle_stop(); 
                clear_src_flag();     
            }
            else if (status == BAP_BC_SINK_MIC_FAILURE)
            {
                SETB(p_env->status_bf, STATUS_SOURCE_FOUND, 0);
                p_env->grp_lid = GAF_INVALID_LID; 
                le_bis_sink_handle_stop(); 
                clear_src_flag();
            }
            else if (status == BAP_BC_SINK_UPPER_TERMINATE)
            {
                SETB(p_env->status_bf, STATUS_SOURCE_FOUND, 0);
                p_env->grp_lid = GAF_INVALID_LID; 
                clear_src_flag();
            }
            else
            {
                SETB(p_env->status_bf, STATUS_SOURCE_FOUND, 0);
                p_env->grp_lid = GAF_INVALID_LID; 
                clear_src_flag();
            }

            APP_DBG("EVENT_GROUP_DESYNCHRONIZED 0x%x\n", p_env->status_bf);
        } break;

        default: { } break;
    }
}

void le_bis_sink_printf_state(void)
{
    APP_DBG("sink_state: 0x%x: 0x%x, 0x%x, 0x%X, 0x%x, 0x%X, 0x%x, 0x%x, sync_lost %d, role %d\n", 
                p_env->status_bf, CHECKB(p_env->status_bf, STATUS_GROUP_STREAMED), CHECKB(p_env->status_bf, STATUS_GROUP_SYNCHRONIZED),
                CHECKB(p_env->status_bf, STATUS_PA_SYNCHRONIZED), CHECKB(p_env->status_bf, STATUS_SCAN_ENABLE),
                CHECKB(p_env->status_bf, STATUS_GROUP_SYNCHRONIZING), CHECKB(p_env->status_bf, STATUS_PA_SYNCHRONIZING),
                CHECKB(p_env->status_bf, STATUS_SOURCE_FOUND), get_sink_sync_lost(), Get_bis_role());
}


__STATIC void app_cb_timer_user_handler(co_timer_t* p_timer)
{
    APP_DBG("app_cb_timer_user_handler\n");
    if (!CHECKB(p_env->status_bf, STATUS_SCAN_ENABLE))
    {
        le_bis_sink_state_scan(EVENT_SCAN_START, GAF_ERR_NO_ERROR);
        set_sink_scan_start_flag(0);
    }
}
#ifdef SINK_CHOOSE_SRC
__STATIC void app_cb_scan_timer_user_handler(co_timer_t* p_timer)
{
    APP_DBG("app_cb_scan_timer_user_handler\n");
    le_bis_sink_state_scan(EVENT_SCAN_START, GAF_ERR_NO_ERROR);
}
#endif
void le_bis_sink_handle_stop(void)
{
	uint16_t status;
    APP_DBG("le_bis_sink_handle_stop: p_env->status_bf 0x%x: 0x%x, 0x%x, 0x%X, 0x%x, 0x%X, 0x%x, 0x%x\n", 
                p_env->status_bf, CHECKB(p_env->status_bf, STATUS_GROUP_STREAMED), CHECKB(p_env->status_bf, STATUS_GROUP_SYNCHRONIZED),
                CHECKB(p_env->status_bf, STATUS_PA_SYNCHRONIZED), CHECKB(p_env->status_bf, STATUS_SCAN_ENABLE),
                CHECKB(p_env->status_bf, STATUS_GROUP_SYNCHRONIZING), CHECKB(p_env->status_bf, STATUS_PA_SYNCHRONIZING),
                CHECKB(p_env->status_bf, STATUS_SOURCE_FOUND));

    // if (!CHECKB(p_env->status_bf, STATUS_PA_SYNCHRONIZED) && CHECKB(p_env->status_bf, STATUS_PA_SYNCHRONIZING))
    // {
    //     printf("please wait pa sync success or timeout\n");
    //     return;
    // }

    if (CHECKB(p_env->status_bf, STATUS_GROUP_STREAMED))
    {
        uint8_t stream_pos = 1;
        uint16_t status = bap_bc_sink_stop_streaming(p_env->grp_lid, stream_pos);
        if (status)
        {
            APP_DBG("bap_bc_sink_stop_streaming error 0x%x\n", status);
        }

        if (get_sink_wed_vaild() == 1)
        {
            stream_pos = 2;
            status = bap_bc_sink_stop_streaming(p_env->grp_lid, stream_pos);
            if (status)
            {
                APP_DBG("bap_bc_sink_stop_streaming error 0x%x\n", status);
            }
        }
    }

    if (CHECKB(p_env->status_bf, STATUS_GROUP_SYNCHRONIZED))
    {
        status = bap_bc_sink_disable(p_env->grp_lid);
        if (status)
        {
            APP_DBG("le_bis_sink_handle_stop: bap_bc_sink_disable error 0x%x\n", status);
        }
    }

    if (!CHECKB(p_env->status_bf, STATUS_GROUP_SYNCHRONIZED))
    {
        if (CHECKB(p_env->status_bf, STATUS_GROUP_SYNCHRONIZING))
        {
            SETB(p_env->status_bf, STATUS_GROUP_SYNCHRONIZING, 0); 
        } 
    }

#if 1
    if (CHECKB(p_env->status_bf, STATUS_PA_SYNCHRONIZED) || CHECKB(p_env->status_bf, STATUS_PA_SYNCHRONIZING))
    {
        status = bap_bc_scan_pa_terminate(p_env->pa_lid);
        APP_DBG("bap_bc_scan_pa_terminate2 error 0x%x\n", status); 
        
        if (!status)
        {
            SETB(p_env->status_bf, STATUS_PA_SYNCHRONIZING, 0);
        }
        else
        {
            APP_DBG("bap_bc_scan_pa_terminate error 0x%x\n", status); 

            if (status == GAF_ERR_BUSY)   
            {
                APP_DBG("bap_bc_scan_pa_terminate: GAF_ERR_BUSY: %d\n", get_sync_pa_accept());
                if (get_sync_pa_accept() == 0xff)
                {
                    SETB(p_env->status_bf, STATUS_PA_SYNCHRONIZING, 0);
                    SETB(p_env->status_bf, STATUS_PA_SYNCHRONIZED, 0);
                }
            }
#if 1 // Fix: force clear PA status bits when pa_terminate returns error other than BUSY
            else
            {
                SETB(p_env->status_bf, STATUS_PA_SYNCHRONIZING, 0);
                SETB(p_env->status_bf, STATUS_PA_SYNCHRONIZED, 0);
            }
#endif
        }
    }

    if (!CHECKB(p_env->status_bf, STATUS_PA_SYNCHRONIZED))
    {
        if (CHECKB(p_env->status_bf, STATUS_PA_SYNCHRONIZING))
        {
            SETB(p_env->status_bf, STATUS_PA_SYNCHRONIZING, 0);            
        }     
    }
#else
    if (CHECKB(p_env->status_bf, STATUS_PA_SYNCHRONIZED))
    {
        if (!CHECKB(p_env->status_bf, STATUS_PA_SYNCHRONIZING))
        {
            status = bap_bc_scan_pa_terminate(p_env->pa_lid);
            APP_DBG("bap_bc_scan_pa_terminate2 error 0x%x\n", status); 
            
            if (status)
            {
                APP_DBG("bap_bc_scan_pa_terminate2 error 0x%x\n", status); 

                if (status == GAF_ERR_BUSY)   
                {
                    APP_DBG("bap_bc_scan_pa_terminate: GAF_ERR_BUSY: %d\n", get_sync_pa_accept());
                    if (get_sync_pa_accept() == 0xff)
                    {
                        SETB(p_env->status_bf, STATUS_PA_SYNCHRONIZING, 0);
                        SETB(p_env->status_bf, STATUS_PA_SYNCHRONIZED, 0);
                    }
                }
    #if 1 // Fix: force clear PA status bits when pa_terminate returns error other than BUSY
                else
                {
                    SETB(p_env->status_bf, STATUS_PA_SYNCHRONIZING, 0);
                    SETB(p_env->status_bf, STATUS_PA_SYNCHRONIZED, 0);
                }
    #endif
            }
        }
        else
        {
            SETB(p_env->status_bf, STATUS_PA_SYNCHRONIZING, 0);            
        }
    }
#endif
    if (CHECKB(p_env->status_bf, STATUS_SOURCE_FOUND))
    {
        SETB(p_env->status_bf, STATUS_SOURCE_FOUND, 0);  
    }

#ifdef SINK_CHOOSE_SRC
        co_timer_config(&(p_env->scan_timer_user), app_cb_scan_timer_user_handler);
        co_timer_start(&(p_env->scan_timer_user), APP_SCAN_START_WAIT_MS);

#endif   
}

void le_bis_sink_stop_scan(void)
{
    le_bis_sink_state_scan(EVENT_SCAN_STOP, GAF_ERR_NO_ERROR);
}

__STATIC void le_bis_sink_cb_bap_bc_sink_status(uint8_t grp_lid, uint8_t state, uint32_t stream_pos_bf,
                                                          const gapi_bg_sync_config_t* p_bg_cfg, uint8_t nb_bis,
                                                          const uint16_t* p_conhdl)
{
    uint16_t status __attribute__((unused));
	MessageContext msgSend;
	APP_DBG("le_bis_sink_cb_bap_bc_sink_status = %d\n", state);

    //clear is_encrypted
    p_env->is_encrypted = false;

    // if (state)
    {
        APP_DBG("clear biginfo_timer_user\n");
        co_timer_stop(&p_env->biginfo_timer_user);
    }

    switch (state)
    {
        case BAP_BC_SINK_ESTABLISHED:
        {
            sink_wait_starting = 0;
        	le_bis_sink_state_group(EVENT_GROUP_SYNCHRONIZED, GAF_ERR_NO_ERROR);
        } break;

        case BAP_BC_SINK_FAILED:
        {
            sink_wait_starting = 1;
            if (CHECKB(p_env->status_bf, STATUS_GROUP_STREAMED))
            {
                SETB(p_env->status_bf, STATUS_GROUP_STREAMED, 0);    
            }
            le_bis_sink_state_group(EVENT_GROUP_DESYNCHRONIZED, BAP_BC_SINK_FAILED);
        } break;

        case BAP_BC_SINK_PEER_TERMINATE:
        {
        	// set_sink_sync_lost(1);
            sink_wait_starting = 1;
            LeAudioSinkModeDisable();
            if (CHECKB(p_env->status_bf, STATUS_GROUP_STREAMED))
            {
                SETB(p_env->status_bf, STATUS_GROUP_STREAMED, 0);    
            }        

        	le_bis_sink_state_group(EVENT_GROUP_DESYNCHRONIZED, GAF_ERR_NO_ERROR);
        } break;

        case BAP_BC_SINK_UPPER_TERMINATE:
        {
            if (CHECKB(p_env->status_bf, STATUS_GROUP_STREAMED))
            {
                SETB(p_env->status_bf, STATUS_GROUP_STREAMED, 0);    
            }

        	le_bis_sink_state_group(EVENT_GROUP_DESYNCHRONIZED, GAF_ERR_NO_ERROR);

#ifdef SINK_CHOOSE_SRC
			set_sink_sync_lost(1);
#endif
            le_bis_sink_handle_stop(); 

        } break;

        case BAP_BC_SINK_LOST:
		{
            sink_wait_starting = 1;
			// set_sink_sync_lost(1);
            LeAudioSinkModeDisable();
            if (CHECKB(p_env->status_bf, STATUS_GROUP_STREAMED))
            {
                SETB(p_env->status_bf, STATUS_GROUP_STREAMED, 0);    
            }

            le_bis_sink_state_group(EVENT_GROUP_DESYNCHRONIZED, BAP_BC_SINK_LOST);
            SendSysModeMsg(MSG_LE_BIS_DISCONNECT);
        } break;

        case BAP_BC_SINK_MIC_FAILURE:
		{
            sink_wait_starting = 1;
            le_bis_sink_state_group(EVENT_GROUP_DESYNCHRONIZED, BAP_BC_SINK_MIC_FAILURE);

            if (CHECKB(p_env->status_bf, STATUS_GROUP_STREAMED))
            {
                SETB(p_env->status_bf, STATUS_GROUP_STREAMED, 0);    
            }

        } break;
            
        default: { ASSERT_WARN(0, state, 0); } break;
    }
}

//API function

__STATIC void le_bis_sink_configuration_state(uint16_t event, uint16_t status, const void* p_params)
{
    bool is_finished = false;

    // APP_DBG("le_bis_sink_configuration_state = %d\n", status);
	switch (event)
	{
		case EVENT_CONFIGURATION_DONE:
			gapm_set_name(EVENT_CONFIGURATION_NAME_SET, strlen("MV_BC_SINK"), (const uint8_t*)"MV_BC_SINK",
											le_bis_sink_configuration_cb_gapm_cmp);
		 break;

		case EVENT_CONFIGURATION_NAME_SET:
			gapm_get_controller_bdaddr(EVENT_CONFIGURE_READ_CONTROLLER_ADDRESS,
										le_bis_sink_bdaddr_cb);
			break;

		case EVENT_CONFIGURE_READ_CONTROLLER_ADDRESS:
			is_finished = true;
			break;

		default:  break;
	}

    if ( is_finished )
    {
    	p_env->status_bf = 0;
    	// le_bis_sink_state_scan(EVENT_SCAN_START, GAF_ERR_NO_ERROR);
    }
}

uint8_t check_bis_sink_state(void)
{
    // APP_DBG("check_bis_sink_state: p_env->status_bf 0x%x: 0x%x, 0x%x, 0x%X, 0x%x, 0x%X, 0x%x, 0x%x\n", 
    //             p_env->status_bf, CHECKB(p_env->status_bf, STATUS_GROUP_STREAMED), CHECKB(p_env->status_bf, STATUS_GROUP_SYNCHRONIZED),
    //             CHECKB(p_env->status_bf, STATUS_PA_SYNCHRONIZED), CHECKB(p_env->status_bf, STATUS_SCAN_ENABLE),
    //             CHECKB(p_env->status_bf, STATUS_GROUP_SYNCHRONIZING), CHECKB(p_env->status_bf, STATUS_PA_SYNCHRONIZING),
    //             CHECKB(p_env->status_bf, STATUS_SOURCE_FOUND));
	
    return p_env->status_bf;
}

void app_get_vendor_data(uint8_t *buf,uint8_t len)
{
	uint8_t i;
	for(i=0; i<len; i++)
	{
	    APP_DBG("vendor_data[%u]: 0x%x\n", i,buf[i]);
	}
}

void le_bis_sink_configuration_cb_gapm_cmp(uint32_t event, uint16_t status)
{
    le_bis_sink_configuration_state(event, status, NULL);
}

__STATIC void le_bis_sink_bdaddr_cb(uint32_t metainfo, uint16_t status, const gap_addr_t* p_addr)
{
    le_bis_sink_configuration_state(metainfo, status, NULL);
}

__STATIC void bis_configuration_cb_gapm_name_get(uint8_t conidx, uint32_t metainfo, uint16_t token,
                                                    uint16_t offset, uint16_t max_length) __attribute__((unused));
__STATIC void bis_configuration_cb_gapm_name_get(uint8_t conidx, uint32_t metainfo, uint16_t token,
                                                    uint16_t offset, uint16_t max_length)
{
    const char* p_device_name = utils_get_device_name();
    uint16_t length = strlen(p_device_name);

    gapc_le_get_name_cfm(conidx, token, GAP_ERR_NO_ERROR, length, length, (const uint8_t*)p_device_name);
}

__STATIC void bis_configuration_cb_gapm_appearance_get(uint8_t conidx, uint32_t metainfo, uint16_t token) __attribute__((unused));
__STATIC void bis_configuration_cb_gapm_appearance_get(uint8_t conidx, uint32_t metainfo, uint16_t token)
{
    gapc_le_get_appearance_cfm(conidx, token, GAP_ERR_NO_ERROR, 0);
}

__STATIC void bis_configuration_cb_gapm_appearance_set(uint8_t conidx, uint32_t metainfo, uint16_t token,
                                                          uint16_t appearance) __attribute__((unused));
__STATIC void bis_configuration_cb_gapm_appearance_set(uint8_t conidx, uint32_t metainfo, uint16_t token,
                                                          uint16_t appearance)
{
    gapm_le_set_appearance(appearance);
    gapc_le_set_appearance_cfm(conidx, token, GAP_ERR_NO_ERROR);
}

void le_bis_sink_start(void)
{
    sink_wait_starting = 1;
#ifdef DISCOV_BIS
    le_bis_sink_state_scan(EVENT_DISCOV_START, GAF_ERR_NO_ERROR); 
#else
    if (!CHECKB(p_env->status_bf, STATUS_SCAN_ENABLE))
    {
        le_bis_sink_state_scan(EVENT_SCAN_START, GAF_ERR_NO_ERROR);   
    }
#endif
    clear_src_flag();
}

void le_bis_sink_stop(void)
{
    uint8_t stream_pos = 1;
    uint16_t status = bap_bc_sink_stop_streaming(p_env->grp_lid, stream_pos);
    if (status)
    {
        APP_DBG("bap_bc_sink_stop_streaming error 0x%x\n", status);
        return;
    }

    if (get_sink_wed_vaild() == 1)
    {
        stream_pos = 2;
        status = bap_bc_sink_stop_streaming(p_env->grp_lid, stream_pos);
        if (status)
        {
            APP_DBG("bap_bc_sink_stop_streaming error 0x%x\n", status);
            return;
        }
    }
}

void le_bis_vendor_cmd_rx_cb(uint32_t* p_vendor_data)
{
    APP_DBG("le_bis_vendor_cmd_rx_cb: data 0x%lx\n", (unsigned long)*p_vendor_data);
    //TODO:
}

#ifdef DISCOV_BIS
__STATIC void le_bis_discov_cb_cmp_evt(uint8_t cmd_type, uint16_t status, uint8_t pa_lid)
{
	// APP_DBG("le_bis_discov_cb_cmp_evt: cmd_type %d, status 0x%x\n", cmd_type, status);

    switch (cmd_type)
    {
        case BAP_BC_DISCOV_CMD_TYPE_STOP:
        {
            if (!status)
            {
                le_bis_sink_state_scan(EVENT_DISCOV_STOPPED, 0);
                // discov_pa_est = 0;
            }
        } break;

        default: { } break;
    }  
}

__STATIC void le_bis_discov_cb_timeout(void)
{
	// APP_DBG("le_bis_discov_cb_timeout\n");
}

__STATIC void le_bis_discov_cb_report(const bap_adv_id_t* p_adv_id, const gaf_adv_report_air_info_t* p_air_info,
                                              uint16_t length, const uint8_t* p_data)
{
    // APP_DBG("le_bis_discov_cb_report: adv_sid 0x%x, rssi 0x%x, addr 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n", 
    //             p_adv_id->adv_sid, p_air_info->rssi, p_adv_id->addr[0], p_adv_id->addr[1], p_adv_id->addr[2], 
    //             p_adv_id->addr[3], p_adv_id->addr[4], p_adv_id->addr[5]);   

    // if (!discov_pa_est)
    {
        memcpy(&(p_env->discov_adv_id), p_adv_id, sizeof(bap_adv_id_t));
        if (Get_bis_role() == BIS_SINK)
        {
            le_bis_sink_state_scan(EVENT_DISCOV_SOURCE_FOUND, GAF_ERR_NO_ERROR);
        }
    }
}

__STATIC void le_bis_discov_cb_pa_established(uint8_t pa_lid, const bap_adv_id_t* p_adv_id, uint8_t phy,
                                                      uint16_t interval_frames)
{
	APP_DBG("le_bis_discov_cb_pa_established, pa_lid 0x%x\n", pa_lid);
    p_env->discov_pa_lid = pa_lid;
}

__STATIC void le_bis_discov_cb_pa_terminated(uint8_t pa_lid, uint8_t reason)
{
    uint16_t status __attribute__((unused));
	APP_DBG("le_bis_discov_cb_pa_terminated, reason 0x%x\n", reason);
    SETB(p_env->status_bf, STATUS_DISCOV_SOURCE_FOUND, 0);
    le_bis_sink_state_scan(EVENT_DISCOV_STOP, 0);
}

__STATIC void le_bis_discov_cb_pa_report(uint8_t pa_lid, const gaf_adv_report_air_info_t* p_air_info,
                                                 uint16_t length, const uint8_t* p_data, bool sink_enable)
{
	APP_DBG("le_bis_discov_cb_pa_report: pa_lid 0x%X, sink_enable 0x%x\n", pa_lid, sink_enable);
    p_env->sink_enable = sink_enable;

    if (sink_enable)
    {
        le_bis_sink_state_scan(EVENT_SCAN_START, GAF_ERR_NO_ERROR);
    }
    else
    {
        SETB(p_env->status_bf, STATUS_DISCOV_SOURCE_FOUND, 0);
    }
}

__STATIC void le_bis_discov_cb_big_info_report(uint8_t pa_lid, const gapm_le_big_info_t* p_report)
{
	APP_DBG("le_bis_discov_cb_big_info_report, encrypted 0x%x, codec vaild %d\n", p_report->encrypted, p_env->is_codec_cfg_vaild);
}

__STATIC const bap_bc_discov_cb_t cbs_bap_bc_discov =
{
    .cb_cmp_evt              = le_bis_discov_cb_cmp_evt,
    .cb_timeout              = le_bis_discov_cb_timeout,
    .cb_report               = le_bis_discov_cb_report,
    .cb_pa_established       = le_bis_discov_cb_pa_established,
    .cb_pa_terminated        = le_bis_discov_cb_pa_terminated,
    .cb_pa_report            = le_bis_discov_cb_pa_report,
    .cb_big_info_report      = le_bis_discov_cb_big_info_report,
};
#endif

#ifdef SINK_CHOOSE_SRC
void reset_src_array(void)
{
    for (int i = 0;  i < MAX_SRC_NUM; i++)
    {
        memset(&(src_array[i]), 0, sizeof(sink_add_src_t));
        memset(&(src_array[i].bcast_id), 0, sizeof(bap_bcast_id_t));
        memset(&(src_array[i].adv_id), 0, sizeof(bap_adv_id_t));
        src_array[i].src_idx = GAF_INVALID_LID;
	BT_DBG("src array reset: %d %d\n", i, src_array[i].broadcast_name_len);
    }
}
#endif

uint16_t le_bis_sink_init(void)
{
    uint16_t ret = 0;

	//set datapath
	le_bis_sink_datapath_init();
    le_bis_sink1_datapath_init();

	gapm_le_set_appearance( SINK_APPEARANCE );

	p_env = (le_bis_sink_env_t*)ke_malloc_user(sizeof(le_bis_sink_env_t), KE_MEM_PROFILE);

	memset(p_env, 0, sizeof(le_bis_sink_env_t));
	p_env->grp_lid = GAF_INVALID_LID;
#ifndef LE_BIS_DELEG
	ret = bap_bc_scan_configure(BAP_ROLE_SUPP_BC_SINK_BIT | BAP_ROLE_SUPP_BC_SCAN_BIT, &cbs_bap_bc_scan);
    if (ret)
    {
        APP_DBG("le_bis_sink_init: bap_bc_scan_configure: ret 0x%x\n", ret);
    }

	ret = bap_bc_sink_configure(BAP_ROLE_SUPP_BC_SINK_BIT | BAP_ROLE_SUPP_BC_SCAN_BIT, &cbs_bap_bc_sink);
    if (ret)
    {
        APP_DBG("le_bis_sink_init: bap_bc_sink_configure: ret 0x%x\n", ret);
    }
#ifdef DISCOV_BIS
    ret = bap_bc_discov_configure(&cbs_bap_bc_discov);
    if (ret)
    {
        APP_DBG("le_bis_sink_init: bap_bc_discov_configure: ret 0x%x\n", ret);
    }
#endif
#endif
    utils_set_device_name("MV_BC_SINK");
    clear_src_flag();
#ifdef SINK_CHOOSE_SRC
    reset_src_array();
#endif
    return ret;
}

#else

bool le_bis_sink_is_synced(void)
{
    return 0;
}


void scan_disable_cb(uint8_t status)
{
    
}

#endif//#if (BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT)
