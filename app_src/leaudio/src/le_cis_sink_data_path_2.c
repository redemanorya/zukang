#include "le_audio_config.h"
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
#include "data_path.h"
#include "mcu_circular_buf.h"
#include "debug.h"

#ifdef LE_AUDIO_ENC_LC3
	#define CIS_SINK_BUFFER_SIZE     (LE_CIS_LC3_PAYLOAD *100)
#else
	#define CIS_SINK_BUFFER_SIZE     (1024*12)////(1024*20)//(1024*48)
#endif
extern uint8_t cis_src_type;
extern uint16_t lc3_packet_len;

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
__STATIC co_list_t MV_rx_env;
__STATIC MCU_CIRCULAR_CONTEXT CIS_SINK_2_FIFOCircular;
uint8_t cis_sink_2_buffer[ CIS_SINK_BUFFER_SIZE ];
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
__STATIC const data_path_builder_itf_t mv_data_path_out_builder =
{
    .cb_create_data_path          = MV_rx_dp_create,
    .cb_config                    = data_path_default_config,
};

/// Data path builder singleton
__STATIC data_path_builder_t MV_rx_dp_builder_right =
        DATA_PATH_BUILDER_INIT(CIS_SINK_DATA_PATH_ID_2, DATA_PATH_OUTPUT, &mv_data_path_out_builder);

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
        co_list_push_back(&(MV_rx_env), &(p_dp->item_ref));
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
    co_list_extract(&(MV_rx_env), &(p_dp->item_ref));

    // Clear environment
    ke_free(p_dp);
}

__STATIC uint8_t* MV_rx_sdu_get(MV_rx_dp_t* p_dp, uint32_t* p_ref_time, uint16_t* p_sdu_len)
{
    uint8_t* p_buf;
    MV_rx_data_t* p_data = p_dp->p_data;
    *p_sdu_len = p_data->max_sdu_size;
    p_buf      = p_data->buf[p_data->buf_idx];
//    MCUCircular_PutData(&CIS_SINK_2_FIFOCircular,p_buf,*p_sdu_len);
//BT_DBG("%s\n",p_buf);
    return (p_buf);
}

// uint8_t LastStatus = 0;
// uint16_t LastLen = 0xffff;
uint32_t cis_lost_cnt;
uint32_t cis_ok_cnt;
static const uint8_t plc_null_buff[LE_CIS_LC3_PAYLOAD] = {0};//填0数据，库里面会用这个做标记
__STATIC void MV_rx_sdu_done(MV_rx_dp_t* p_dp, uint16_t sdu_len, uint32_t ref_time, uint8_t* p_buf, uint8_t status)
{   
    // if (status != LastStatus || LastLen!= sdu_len)
    // {
    //     APP_DBG("MV_rx_sdu_done: status 0x%x len:%d\n", status, sdu_len);
    //     LastStatus = status;
    //     LastLen = sdu_len;
    // }

    if (status == SDU_OK)
    {
        cis_ok_cnt++;
        MCUCircular_PutData(&CIS_SINK_2_FIFOCircular, p_buf, sdu_len);
    }
    else
    {
        cis_lost_cnt++;
    }

//    app_print_lost_cnt();
    MV_rx_data_t* p_data = p_dp->p_data;
}

/**************************************************
 * foreward audio packet streaming Src->sink
 ***********************************************/
void le_cis_sink_2_datapath_init(void)
{
    if (Get_cis_app() != CIS_APP_HEADSET)
    {
        return;
    }

	MCUCircular_Config(&CIS_SINK_2_FIFOCircular, cis_sink_2_buffer, sizeof(cis_sink_2_buffer));
    printf("le_cis_sink_2_datapath_init: loc %d\n", Get_sink_audio_loc());
	data_path_add_builder(&MV_rx_dp_builder_right);
}

uint16_t get_cis_sink_2_buffer_len(void)
{
	return sizeof( cis_sink_2_buffer );
}

extern uint8_t cis_sink2_sync_ok;
uint16_t le_cis_sink_2_data_get(void* buf,uint32_t len)
{
    if(!cis_sink2_sync_ok)
        memset(buf,0,len);
	return MCUCircular_GetData(&CIS_SINK_2_FIFOCircular, buf, len);
}

uint32_t le_cis_sink_2_data_len_get()
{
	return MCUCircular_GetDataLen( &CIS_SINK_2_FIFOCircular );
}

void cis_sink_2_data_clr(void)
{
	memset(cis_sink_2_buffer,0,sizeof(cis_sink_2_buffer));
	MCUCircular_Config(&CIS_SINK_2_FIFOCircular, cis_sink_2_buffer, sizeof( cis_sink_2_buffer ) );
}

#endif//#if BT_LE_AUDIO_SUPPORT		LE_CIS_SINK
