#include "le_audio_config.h"
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)

#include "data_path.h"
#include "arch.h"
#include "co_utils.h"
#include "co_endian.h"
#include "le_public.h"
#include "mcu_circular_buf.h"
#include "bt_config.h"


extern void cis_src_send_data_2(void* buf, uint32_t len);
extern uint16_t cis_src_get_len_2();
extern void cis_src_send_data_clr_2(void);

__STATIC MCU_CIRCULAR_CONTEXT CIS_SOUCE_FIFOCircular;
//__STATIC uint8_t cis_source_buffer[ CIS_SOURCE_BUFFER_SIZE ];
extern uint8_t cis_sink_buffer[];
/// Data used when data path driver is started
typedef struct MV_tx_data
{
    /// Expected packet length (in bytes)
    uint16_t max_sdu_size;
    /// Buffer index
    uint8_t  buf_idx;
    /// Number of SDU buffers
    uint8_t  nb_buf;
    /// SDU interval (in us)
    uint32_t sdu_interval;
    /// Reference time of next SDU (in us, based on Bluetooth timestamp)
    uint32_t next_sdu_ref_time;

    /// SDU buffers - nb_buffer + 1 element
    uint8_t* buf[];
} MV_tx_data_t;

/// Packet generation data path driver environment structure
typedef struct MV_tx_dp
{
    /// Inherited from data path
    data_path_t hdr;

    // Runtime parameters
    /// Pointer to data required by started data path (NULL if not started)
    MV_tx_data_t* p_data;
    /// SDU packet counter
    uint32_t sdu_cnt;
    /// Connection handle
    uint16_t conhdl;
    /// ISO Tx/Rx test payload type (@see enum le_iso_test_payl_type)
    uint8_t  payload_type;
} MV_tx_dp_t;


//static function declare
static  uint8_t mv_data_path_cb_start(MV_tx_dp_t* p_dp, uint32_t sdu_interval, uint32_t trans_latency, uint16_t max_sdu);
static  uint8_t* mv_data_path_cb_sdu_get(MV_tx_dp_t* p_dp, uint32_t* p_ref_time, uint16_t* p_sdu_len);

// Output data path interface
static uint8_t mv_data_path_cb_start(MV_tx_dp_t* p_dp, uint32_t sdu_interval, uint32_t trans_latency,
                                                  uint16_t max_sdu)
{
    uint8_t status = CO_ERROR_MEMORY_CAPA_EXCEED;
    // Compute number of TX buffer
    uint8_t nb_buf = CO_DIVIDE_CEIL(trans_latency, sdu_interval);
    // Compute data size required by data path driver when started
    uint16_t data_size = sizeof(MV_tx_data_t) + (sizeof(uint8_t*) * nb_buf) + (nb_buf * max_sdu);

    //APP_DBG("isogen_tx_start data_size:%d\n", data_size);
    // Allocate data memory for Controller to Host path
    MV_tx_data_t* p_data = (MV_tx_data_t*) ke_malloc_system(data_size, KE_MEM_ENV);
    if(p_data != NULL)
    {
        uint8_t* p_buf_cursor;
        uint8_t i;

        memset(p_data, 0, data_size);

        p_data->sdu_interval      = sdu_interval;
        p_data->nb_buf            = nb_buf;
        p_data->max_sdu_size      = max_sdu;
        p_data->next_sdu_ref_time = 0; // First SDU timestamp will be generated at the first request

        p_buf_cursor = ((uint8_t*) p_data) + sizeof(MV_tx_data_t) + (sizeof(uint8_t*) * nb_buf);

        // Allocate number of buffer required
        for (i = 0; i < nb_buf; i++)
        {
            p_data->buf[i] = p_buf_cursor;
            p_buf_cursor       += max_sdu;
        }

       {
        // Data path driver is considered started
        	p_dp->p_data = p_data;
        }
        status = CO_ERROR_NO_ERROR;


    }
//    ble_info("isogen_tx_start status :%d\n", status);
    return (status);
}

uint8_t cis_src_sync;
static  uint8_t* mv_data_path_cb_sdu_get(MV_tx_dp_t* p_dp, uint32_t* p_ref_time, uint16_t* p_sdu_len)
{
    MV_tx_data_t* p_data = p_dp->p_data;
    uint8_t*p_buf       = p_data->buf[p_data->buf_idx];

   if(!cis_src_sync)
   {
       *p_sdu_len = MCUCircular_GetDataLen( &CIS_SOUCE_FIFOCircular );

       if(*p_sdu_len>= 2* LE_CIS_LC3_PACKET_LEN)
       {
           cis_src_sync = 1;
       }
   }

   if ( cis_src_sync )
    {
        GIE_DISABLE();
        *p_sdu_len = MCUCircular_GetDataLen( &CIS_SOUCE_FIFOCircular );

#ifdef LE_AUDIO_ENC_LC3
#if (LE_CIS_LC3_PACKET_LEN == LE_CIS_LC3_PAYLOAD) //add header
{
        if(*p_sdu_len>= LE_CIS_LC3_PACKET_LEN)
        {
            *p_sdu_len = LE_CIS_LC3_PACKET_LEN;
        }
}
#endif
#endif

        if(*p_sdu_len)
        {
            MCUCircular_GetData( &CIS_SOUCE_FIFOCircular, p_buf, *p_sdu_len);
        }

        GIE_ENABLE();
    }
    else//no data
    {
        *p_sdu_len = 0;
    }

 //   APP_DBG("mv_data_path_cb_sdu_get src tx len:%d\n", *p_sdu_len);
return (p_buf);
}

static uint16_t Last_sdu_len = 0;
static void mv_data_path_cb_sdu_done(MV_tx_dp_t* p_dp, uint16_t sdu_len, uint32_t ref_time,  uint8_t* p_buf, uint8_t status)
{
	if(Last_sdu_len!= sdu_len)
	{
		Last_sdu_len = sdu_len;
	}
}

static void mv_data_path_cb_destroy(MV_tx_dp_t* p_dp, uint8_t reason)
{
  // Clear data if data path started
	if(p_dp->p_data != NULL)
	{
		ke_free(p_dp->p_data);
	}

	// Clear environment
	ke_free(p_dp);
}

static const data_path_itf_t mv_data_path_prov_control_out_itf =
{
    .cb_start      = (data_path_start_cb)mv_data_path_cb_start,
    .cb_sdu_get    = (data_path_sdu_get_cb)mv_data_path_cb_sdu_get,
    .cb_sdu_done   = (data_path_sdu_done_cb)mv_data_path_cb_sdu_done,
    .cb_destroy    = mv_data_path_cb_destroy,
    .cb_peer_sync  = NULL,
};

static data_path_t* mv_data_path_cb_create(uint16_t conhdl, const uint8_t* p_codec_id, uint32_t ctrl_delay,
                                                  uint8_t codec_cfg_len, const uint8_t* p_codec_cfg)
{
	MV_tx_dp_t* p_dp;

    // Finally allocate data path instance
    p_dp = (MV_tx_dp_t*)ke_malloc_user(sizeof(MV_tx_dp_t), KE_MEM_KE_MSG);
    ASSERT_ERR(p_dp != NULL);


    // Set data path interface

    memset(p_dp, 0, sizeof(MV_tx_dp_t));
    p_dp->hdr.p_itf = &mv_data_path_prov_control_out_itf;
    p_dp->conhdl = conhdl;

    return (&(p_dp->hdr));
}
// Function called to configure current data path type
__STATIC uint8_t mv_data_path_config(uint8_t cfg_len, const uint8_t* p_cfg)
{
    return (CO_ERROR_NO_ERROR);
}

__STATIC const data_path_builder_itf_t mv_data_path_in_builder =
{
    .cb_create_data_path = mv_data_path_cb_create,
    .cb_config           = mv_data_path_config,
};


// Create instance of data path input builder
__STATIC data_path_builder_t mv_data_path_builder =
        DATA_PATH_BUILDER_INIT(CIS_SINK_DATA_PATH_ID, DATA_PATH_INPUT, &mv_data_path_in_builder);

/**************************************************
 * foreward audio packet streaming fifo, Src->sink
 ***********************************************/
void le_cis_source_datapath_init()
{
	MCUCircular_Config(&CIS_SOUCE_FIFOCircular, cis_sink_buffer, get_cis_sink_buffer_len());//sizeof( cis_source_buffer ) );
	data_path_add_builder(&mv_data_path_builder);
}

void cis_src_send_data(void* buf, uint32_t len)
{
	{
		GIE_DISABLE();
#ifdef CIS_APP_HEADSET_LR
		MCUCircular_PutData( &CIS_SOUCE_FIFOCircular, buf, len/2);
        cis_src_send_data_2(buf+len/2, len/2);
#else
		MCUCircular_PutData( &CIS_SOUCE_FIFOCircular, buf, len);
        cis_src_send_data_2(buf, len);
#endif        
		GIE_ENABLE();
	}
}

uint16_t cis_src_get_len()
{
	uint16_t len1;
	GIE_DISABLE();
	len1 = MCUCircular_GetSpaceLen(&CIS_SOUCE_FIFOCircular);
    // len2 = cis_src_get_len_2();
	GIE_ENABLE();
	return len1;
}

void cis_src_send_data_clr(void)
{
	MCUCircular_Config(&CIS_SOUCE_FIFOCircular, cis_sink_buffer, get_cis_sink_buffer_len());
	cis_src_sync = 0;
    // cis_src_send_data_clr_2();
}

#endif //#if BT_LE_AUDIO_SUPPORT		LE_CIS_SOURCE
