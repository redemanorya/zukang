#include "le_audio_config.h"
#include "data_path.h"
#include "mcu_circular_buf.h"
#include "debug.h"
#include "chip_info.h"
#include <string.h>

// 简化的函数声明
void *ke_malloc_system(uint32_t size, uint8_t type);
void ke_free(void *mem_ptr);
extern uint8_t LeAudioCodecFormatGet(void);
extern void app_get_vendor_data(uint8_t *buf,uint8_t len);
extern uint8_t IsLeAudioPlusEPOFF(void);
extern int8_t get_bis_rssi(void);
extern int8_t get_bt_rssi(void);
extern uint16_t le_bis_sink1_data_get(uint8_t* buf, uint16_t len);
extern uint16_t le_bis_sink1_data_len_get(void);
extern void bis_sink1_data_clr(void);

#if BT_LE_AUDIO_SUPPORT//#ifdef LE_BIS_SINK
uint8_t bis_src_sink_buffer[BIS_SINK_SRC_RXTX_BUF_LEN];

extern uint8_t bis_src_type;
extern uint16_t lc3_packet_len;
extern void LC3_SetPlcStartFlag(uint8_t flag);

//added by zsq for test
/// Data used when data path driver is started
typedef struct MV_rx_data
{
    /// Expected packet length (in bytes)
    uint16_t max_sdu_size;
    /// Buffer index
    uint8_t  buf_idx;
    /// Number of SDU buffers
    uint8_t  nb_buf;

    /// SDU buffers - nb_buffer + 1 element
    uint8_t* buf[];
} MV_rx_data_t;

/// Packet generation data path driver environment structure
typedef struct MV_rx_dp
{
    /// Inherited from data path
    data_path_t hdr;

    /// Used to reference data path in list of instantiated data path.
    co_list_hdr_t item_ref;

    // Statistics
    /// ISO test mode - Valid received test ISO Data SDU
    uint32_t rx_pkt_cnt;
    /// ISO test mode - SDU made up of one or more invalid or missing PDUs
    uint32_t missed_pkt_cnt;
    /// ISO test mode - SDU where the size or packet counter does not match the expected size or value
    uint32_t failed_pkt_cnt;

    // Runtime parameters
    /// Pointer to data required by started data path (NULL if not started)
    MV_rx_data_t* p_data;
    /// SDU packet counter
    uint32_t sdu_cnt;
    /// Connection handle
    uint16_t conhdl;
    /// Used to know if first valid SDU received (only used in framed mode)
    bool     first_sdu_received;
    /// ISO Tx/Rx test payload type (@see enum le_iso_test_payl_type)
    uint8_t  payload_type;
} MV_rx_dp_t;

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */
__STATIC uint8_t MV_rx_start(MV_rx_dp_t* p_dp, uint32_t sdu_interval, uint32_t trans_latency, uint16_t max_sdu);
__STATIC uint8_t* MV_rx_sdu_get(MV_rx_dp_t* p_dp, uint32_t* p_ref_time, uint16_t* p_sdu_len);
__STATIC void MV_rx_sdu_done(MV_rx_dp_t* p_dp, uint16_t sdu_len, uint32_t ref_time, uint8_t* p_buf, uint8_t status);
__STATIC void MV_rx_destroy(MV_rx_dp_t* p_dp, uint8_t reason);

__STATIC data_path_t* MV_rx_dp_create(uint16_t conhdl, const uint8_t* p_codec_id, uint32_t ctrl_delay,
                                          uint8_t codec_cfg_len, const uint8_t* p_codec_cfg);
/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

/// Environment used to retrieve list of data path driver instantiated
//__STATIC co_list_t MV_rx_env;
__STATIC MCU_CIRCULAR_CONTEXT BIS_SINK_FIFOCircular;
//__STATIC  uint8_t sink_ready=0;

/* ***********************
 * DATA PATH
 * *********************** */

/// Data path interface
__STATIC const data_path_itf_t MV_rx_dp_itf =
{
    .cb_start     = (data_path_start_cb)    MV_rx_start,
    .cb_sdu_get   = (data_path_sdu_get_cb)  MV_rx_sdu_get,
    .cb_sdu_done  = (data_path_sdu_done_cb) MV_rx_sdu_done,
    .cb_destroy   = (data_path_destroy_cb)  MV_rx_destroy,
    .cb_peer_sync = NULL,
};

/* ***********************
 * DATA PATH BUILDER
 * *********************** */

/// Data path builder interface
__STATIC const data_path_builder_itf_t MV_rx_dp_builder_itf =
{
    .cb_create_data_path          = MV_rx_dp_create,
    .cb_config                    = data_path_default_config,
};

/// Data path builder singleton
__STATIC data_path_builder_t MV_rx_dp_builder =
        DATA_PATH_BUILDER_INIT(BIS_SINK_DATA_PATH_ID, DATA_PATH_OUTPUT, &MV_rx_dp_builder_itf);

/*
 * LOCAL FUNCTION DECLARATIONS
 ****************************************************************************************
 */

__STATIC data_path_t* MV_rx_dp_create(uint16_t conhdl, const uint8_t* p_codec_id, uint32_t ctrl_delay,
                                          uint8_t codec_cfg_len, const uint8_t* p_codec_cfg)
{
	MV_rx_dp_t* p_dp = (MV_rx_dp_t*) ke_malloc_user(sizeof(MV_rx_dp_t), KE_MEM_ENV);

    // If data path driver not assigned
    if(p_dp != NULL)
    {
        // Clean environment
        memset(p_dp, 0, sizeof(MV_rx_dp_t));
        // Register data path interface
        p_dp->hdr.p_itf = &MV_rx_dp_itf;
        // Save connection handle
        p_dp->conhdl = conhdl;
        // By default expect max packet size
        p_dp->payload_type = ISO_TEST_MAX_LEN;



        // Push reference to instantiated data path driver in module environment
//        co_list_push_back(&(MV_rx_env), &(p_dp->item_ref));
    }

    return ((data_path_t*) p_dp);
}

__STATIC uint8_t MV_rx_start(MV_rx_dp_t* p_dp, uint32_t sdu_interval, uint32_t trans_latency, uint16_t max_sdu)
{
    uint8_t status = CO_ERROR_MEMORY_CAPA_EXCEED;

    // Compute number of TX buffer
    uint8_t nb_buf = CO_DIVIDE_CEIL(trans_latency, sdu_interval);
    // Compute data size required by data path driver when started
    uint16_t data_size = sizeof(MV_rx_data_t) + (sizeof(uint8_t*) * nb_buf) + (nb_buf * max_sdu);

    // Allocate data memory for Controller to Host path
    MV_rx_data_t* p_data = (MV_rx_data_t*) ke_malloc_system(data_size, KE_MEM_ENV);
    if(p_data != NULL)
    {
        uint8_t* p_buf_cursor;
        uint8_t i;

        memset(p_data, 0, data_size);

        p_data->nb_buf             = nb_buf;
        p_data->max_sdu_size       = max_sdu;

        // Prepare to fill array of buffer pointers
        p_buf_cursor = ((uint8_t*) p_data) + sizeof(MV_rx_data_t) + (sizeof(uint8_t*) * nb_buf);

        // Allocate number of buffer required
        for (i = 0; i < nb_buf; i++)
        {
            p_data->buf[i] = p_buf_cursor;
            p_buf_cursor  += max_sdu;
        }

//memory leakage
//        p_dp->p_data = (MV_rx_data_t*) ke_malloc_system(data_size, KE_MEM_ENV);
//        if(p_dp->p_data != NULL)

        {
        // Data path driver is considered started
        	p_dp->p_data = p_data;
        	status = CO_ERROR_NO_ERROR;
        }
    }

    return (status);
}

__STATIC void MV_rx_destroy(MV_rx_dp_t* p_dp, uint8_t reason)
{
    // Clear data if data path started
    if(p_dp->p_data != NULL)
    {
        ke_free(p_dp->p_data);
    }

    // Remove reference of instantiated data path driver in module environment
//    co_list_extract(&(MV_rx_env), &(p_dp->item_ref));

    // Clear environment
    ke_free(p_dp);
}

__STATIC uint8_t* MV_rx_sdu_get(MV_rx_dp_t* p_dp, uint32_t* p_ref_time, uint16_t* p_sdu_len)
{
    uint8_t* p_buf;
    MV_rx_data_t* p_data = p_dp->p_data;
    *p_sdu_len = p_data->max_sdu_size;
    p_buf      = p_data->buf[p_data->buf_idx];
//    MCUCircular_PutData(&BIS_SINK_FIFOCircular,p_buf,*p_sdu_len);
//BT_DBG("%s\n",p_buf);
    return (p_buf);
}

#if 1
uint32_t bis_lost_cnt = 0;
uint32_t bis_ok_cnt = 0;
#endif

uint8_t const plc_null_buff[250] = {0};//填0数据，库里面会用这个做标记
__STATIC void MV_rx_sdu_done(MV_rx_dp_t* p_dp, uint16_t sdu_len, uint32_t ref_time, uint8_t* p_buf, uint8_t status)
{
    if (status)
    {
        bis_lost_cnt++;
    }
    else
    {
        bis_ok_cnt++;
    }
    uint32_t lSpaceLen =  MCUCircular_GetSpaceLen(&BIS_SINK_FIFOCircular);
    if( (lSpaceLen < sdu_len) || (lSpaceLen < lc3_packet_len))// 统一处理，剩余小于则不处理，防止覆盖
    {
    	return;
    }

    if(LeAudioCodecFormatGet() == LE_CODEC_FORMAT_LC3_2 || LeAudioCodecFormatGet() == LE_CODEC_FORMAT_LC3)
    {
        if (bis_src_type == LE_AUDIO_SRC_MV_BIS)
        {
            if (sdu_len == lc3_packet_len + VENDOR_DATA_LEN)
            {
                if (status == SDU_OK)
                {
                    app_get_vendor_data(p_buf,VENDOR_DATA_LEN);
                    MCUCircular_PutData(&BIS_SINK_FIFOCircular, &p_buf[VENDOR_DATA_LEN], sdu_len - VENDOR_DATA_LEN);
                }
                else
                {
                    LC3_SetPlcStartFlag(TRUE);//plc
                    MCUCircular_PutData(&BIS_SINK_FIFOCircular, (void *)plc_null_buff, lc3_packet_len);
                }
            }
            else if ((sdu_len != 0) && (sdu_len < lc3_packet_len))
            {
                if (IsLeAudioPlusEPOFF())
                {
                    MCUCircular_PutData(&BIS_SINK_FIFOCircular, p_buf, sdu_len);
                    MCUCircular_PutData(&BIS_SINK_FIFOCircular, (void *)plc_null_buff, lc3_packet_len-sdu_len);
                }
                else
                {
                	LC3_SetPlcStartFlag(TRUE); //plc
                    MCUCircular_PutData(&BIS_SINK_FIFOCircular, (void *)plc_null_buff, lc3_packet_len);
                }
            }
            else if (sdu_len == 0)
            {
                if (status)
                {
                    LC3_SetPlcStartFlag(TRUE);
                    MCUCircular_PutData(&BIS_SINK_FIFOCircular, (void *)plc_null_buff, lc3_packet_len);
                }
            }
            else
            {
                BT_DBG("error sdu_len %d\n", sdu_len);
            }
        }
        else 
        {
            if (status == SDU_OK && sdu_len == lc3_packet_len)
            {
                MCUCircular_PutData(&BIS_SINK_FIFOCircular, p_buf, sdu_len);
            }
            else
            {
                LC3_SetPlcStartFlag(TRUE);
                MCUCircular_PutData(&BIS_SINK_FIFOCircular, (void *)plc_null_buff, lc3_packet_len);
            }
        }
    }
	
    MV_rx_data_t* p_data __attribute__((unused)) = p_dp->p_data;
    if(LeAudioCodecFormatGet() == LE_CODEC_FORMAT_SBC)
    {
        if (sdu_len == AUDIO_SBC_PACKET_LEN*AUDIO_SBC_SEND_FRAME_NUMBER + VENDOR_DATA_LEN)
        {
            app_get_vendor_data(p_buf,VENDOR_DATA_LEN);

			MCUCircular_PutData(&BIS_SINK_FIFOCircular, &p_buf[VENDOR_DATA_LEN], sdu_len - VENDOR_DATA_LEN);
        }
	}
}

extern uint32_t g_56_cnt;
extern uint32_t g_other_cnt;

extern void printf_iso_state();
void app_print_lost_cnt(void)
{
    // if (Get_bis_role() == BIS_SINK)
    {
//	   	BT_DBG("bis_ok_cnt %lu,  bis_lost_cnt %lu, bis_rssi: %d, bt_rssi:%d\n",
//                    bis_ok_cnt, bis_lost_cnt, get_bis_rssi(), get_bt_rssi());
#if BT_LE_AUDIO_SUPPORT==LE_BIS_SUPPORT
		BT_DBG("bis_ok_cnt %lu,  bis_lost_cnt %lu\n",
					bis_ok_cnt, bis_lost_cnt);
		printf_iso_state();
#endif
    }

    // extern uint32_t a2dp_sn_lost_cnt;
    // extern uint32_t a2dp_sn_all_cnt;
    // BT_DBG("a2dp all:%lu, lost: %lu\n", (unsigned long)a2dp_sn_all_cnt, (unsigned long)a2dp_sn_lost_cnt);
}

void le_bis_sink_datapath_init()
{
	MCUCircular_Config(&BIS_SINK_FIFOCircular, bis_src_sink_buffer, sizeof( bis_src_sink_buffer ) );
	data_path_add_builder(&MV_rx_dp_builder);
//	sink_ready =1;
}

uint16_t get_bis_sink_buffer_len(void)
{
	return sizeof( bis_src_sink_buffer );
}

uint16_t le_bis_sink_data_get(void* buf,uint32_t len)
{
	if(get_sink_wed_vaild())
	{
		return MCUCircular_GetData(&BIS_SINK_FIFOCircular, buf, len/2) + le_bis_sink1_data_get(buf+len/2, len/2);
	}
	return MCUCircular_GetData(&BIS_SINK_FIFOCircular, buf, len);
}

uint32_t le_bis_sink_data_len_get()
{
	if(get_sink_wed_vaild())
	{
		uint16_t len1;
		uint16_t len2;

		GIE_DISABLE();
		len1 = le_bis_sink1_data_len_get();
		len2 = MCUCircular_GetDataLen( &BIS_SINK_FIFOCircular );
		GIE_ENABLE();

		if(len1 >= LeAudioGetLc3FrameLen() && len2 >= LeAudioGetLc3FrameLen())
			return len1+len2;
		return 0;
	}
	return MCUCircular_GetDataLen( &BIS_SINK_FIFOCircular );
}

uint8_t le_bis_sink_data_get_ID(void)
{
	uint8_t id = 0;

	if(LeAudioCodecFormatGet() == LE_CODEC_FORMAT_SBC)
		id = BIS_SINK_FIFOCircular.CircularBuf[(BIS_SINK_FIFOCircular.R + AUDIO_SBC_PACKET_LEN - 1) % BIS_SINK_FIFOCircular.BufDepth];

	return id;
}

void bis_sink_data_clr(void)
{
	memset(bis_src_sink_buffer,0,sizeof(bis_src_sink_buffer));
	MCUCircular_Config(&BIS_SINK_FIFOCircular, bis_src_sink_buffer, sizeof( bis_src_sink_buffer ) );
	bis_sink1_data_clr();
}

#endif//#if BT_LE_AUDIO_SUPPORT//#endif//LE_BIS_SINK
