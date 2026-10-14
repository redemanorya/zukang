#include "data_path.h"
#include "arch.h"
#include "co_utils.h"
#include "co_endian.h"
#include "le_public.h"
#include "le_audio_config.h"
#include "mcu_circular_buf.h"
#include "bt_config.h"
#include "irqn.h"
#include <string.h>

// 简化的函数声明
void *ke_malloc_system(uint32_t size, uint8_t type);
void *ke_malloc_user(uint32_t size, uint8_t type);
void ke_free(void *mem_ptr);
extern uint32_t get_bis_sink_buffer_len(void);
extern void bis_src1_in_send_data(uint8_t* buf, uint16_t len);
extern uint16_t bis_src1_in_get_len(void);
extern void bis_src1_in_send_data_clr(void);

#if (BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT)//#ifdef LE_BIS_SOURCE
extern uint8_t g_vendor_data[];
__STATIC MCU_CIRCULAR_CONTEXT BIS_SOUCE_FIFOCircular;
//__STATIC uint8_t bis_source_buffer[ BIS_SOURCE_BUFFER_SIZE ];
extern uint8_t bis_src_sink_buffer[];
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

//memory leakage
//        p_dp->p_data = (MV_tx_data_t*) ke_malloc_system(data_size, KE_MEM_ENV);
//
//
//        if(p_dp->p_data != NULL)

       {
        // Data path driver is considered started
        	p_dp->p_data = p_data;
        }
        status = CO_ERROR_NO_ERROR;


    }
//    ble_info("isogen_tx_start status :%d\n", status);
    return (status);
}

static  uint8_t* mv_data_path_cb_sdu_get(MV_tx_dp_t* p_dp, uint32_t* p_ref_time, uint16_t* p_sdu_len)
{
    MV_tx_data_t* p_data = p_dp->p_data;
    uint8_t*p_buf       = p_data->buf[p_data->buf_idx];

    if(!get_src_sync())
    {
        *p_sdu_len = MCUCircular_GetDataLen( &BIS_SOUCE_FIFOCircular );
#ifdef LE_AUDIO_ENC_LC3
        //if(*p_sdu_len>= 4* lc3_packet_len)
        if(*p_sdu_len>= 2* lc3_packet_len)
#elif defined LE_AUDIO_ENC_SBC
        if(*p_sdu_len>= 4* AUDIO_SBC_SEND_PACKET_LEN)
#endif
        {
        	set_src_sync(1);
        }
    }

    if( get_src_sync() ){
        GIE_DISABLE();
        *p_sdu_len = MCUCircular_GetDataLen( &BIS_SOUCE_FIFOCircular );

    
#ifdef LE_AUDIO_ENC_SBC
        //240 = (79+1)*3 //middle quality 48KHz Stereo
        if(*p_sdu_len>= AUDIO_SBC_SEND_PACKET_LEN)//p_data->max_sdu_size )
            *p_sdu_len = AUDIO_SBC_SEND_PACKET_LEN;//p_data->max_sdu_size;
        else
            *p_sdu_len = 0;
#endif

#ifdef LE_AUDIO_ENC_LC3
        {
            if(*p_sdu_len>=  lc3_packet_len) 
                *p_sdu_len = lc3_packet_len;
            else
                *p_sdu_len = 0;                
        }
#endif

        if(*p_sdu_len)
        {
#ifdef BIS_TEST_WITH_PHONE
            MCUCircular_GetData( &BIS_SOUCE_FIFOCircular, p_buf, *p_sdu_len);
#else
            if(VENDOR_DATA_LEN > 0)
            	memcpy(p_buf,g_vendor_data,VENDOR_DATA_LEN);
            MCUCircular_GetData( &BIS_SOUCE_FIFOCircular, p_buf + VENDOR_DATA_LEN, *p_sdu_len);
            *p_sdu_len += VENDOR_DATA_LEN;//vendor_data size
#endif
        }

        GIE_ENABLE();
    }else
    *p_sdu_len=0;
#ifdef BIS_SRC_SWITCH_MODES_WITHOUT_REBOOTING
	extern bool BisSrcInsertZeroFlag;
    if(*p_sdu_len == 0 && BisSrcInsertZeroFlag)
    {
        if(get_src_wed_vaild())
        {
            *p_sdu_len = lc3_packet_len;
            memset(p_buf,0,*p_sdu_len);
        }
        else
        {
            *p_sdu_len = lc3_packet_len + VENDOR_DATA_LEN;
            if(VENDOR_DATA_LEN > 0)
            	memcpy(p_buf,g_vendor_data,VENDOR_DATA_LEN);
            memset(p_buf + VENDOR_DATA_LEN,0,lc3_packet_len);
        }
    }
#endif    
    return (p_buf);
}


static void mv_data_path_cb_sdu_done(MV_tx_dp_t* p_dp, uint16_t sdu_len, uint32_t ref_time,  uint8_t* p_buf, uint8_t status)
{

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
    .cb_destroy    = (data_path_destroy_cb)mv_data_path_cb_destroy,
    .cb_peer_sync  = NULL,
};


data_path_t* mv_data_path_cb_create(uint16_t conhdl, const uint8_t* p_codec_id, uint32_t ctrl_delay,
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

__STATIC const data_path_builder_itf_t mv_data_path_out_builder_itf =
{
    .cb_create_data_path = mv_data_path_cb_create,
    .cb_config           = mv_data_path_config,
};


// Create instance of data path output builder
__STATIC data_path_builder_t mv_data_path_out_builder =
        DATA_PATH_BUILDER_INIT(BIS_SRC_DATA_PATH_ID, DATA_PATH_INPUT, &mv_data_path_out_builder_itf);


void le_bis_source_datapath_init()
{
	MCUCircular_Config(&BIS_SOUCE_FIFOCircular, bis_src_sink_buffer, get_bis_sink_buffer_len());//sizeof( bis_source_buffer ) );
	data_path_add_builder(&mv_data_path_out_builder);
//	bis_src_ready = 1;
}


void bis_src_send_data(void* buf, uint32_t len)
{
	//if( bis_src_send_data )
	{
		GIE_DISABLE();
		if(get_src_wed_vaild())
		{
			MCUCircular_PutData( &BIS_SOUCE_FIFOCircular, buf, len/2); 	//塞入一个CHANNEL数据
			bis_src1_in_send_data(buf+len/2, len/2);					//另一个CHANNEL数据塞入到bis src1
		}
		else
			MCUCircular_PutData( &BIS_SOUCE_FIFOCircular, buf, len);
		GIE_ENABLE();
	}
}

uint16_t bis_src_get_len()
{
	uint16_t len;

	if(get_src_wed_vaild())
	{
		uint16_t len1;
		uint16_t len2;
		len1 = bis_src1_in_get_len();

		GIE_DISABLE();
		len2 = MCUCircular_GetSpaceLen(&BIS_SOUCE_FIFOCircular);
		GIE_ENABLE();

		if(len1 >= lc3_packet_len && len2 >= lc3_packet_len)
			return len1+len2;
		return 0;
	}

	GIE_DISABLE();
	len = MCUCircular_GetSpaceLen(&BIS_SOUCE_FIFOCircular);
	GIE_ENABLE();
	return len;
}

void bis_src_send_data_clr(void)
{
	MCUCircular_Config(&BIS_SOUCE_FIFOCircular, bis_src_sink_buffer, get_bis_sink_buffer_len());
	set_src_sync(0);
	bis_src1_in_send_data_clr();
}



#endif //#if (BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT)		LE_BIS_SOURCE
