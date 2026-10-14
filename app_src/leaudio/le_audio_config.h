
#ifndef _LE_AUDIO_CONFIG_H
#define _LE_AUDIO_CONFIG_H

#include "type.h"
#include "le_public.h"
#include "bap.h"
#include "gaf.h"
#include "bt_config.h"
#include "le_audio_api.h"

#define BIS_NONE  						0xFF
#define BIS_SRC   						1
#define BIS_SINK  						2

#define CIS_NONE  						0xFF
#define CIS_SRC   						3
#define CIS_SINK  						4

#define LC3_MONO						1
#define LC3_STEREO						2
#define LC3_CHANNEL						LC3_STEREO		//声道选择

#if BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT
	// #define SINK_CHOOSE_SRC
	#define APP_GAF_PBP 1
	// #define COMMON_SINK		//默认BIS组网自适应功能,如需根据LE_AUDIO_MV_BROADCAST_ID和LE_AUDIO_MV_BROADCAST_NAME判断,则注释掉

#ifndef COMMON_SINK
	//for test
	// #define ONE_VS_ONE
#endif
	//	#define USE_PTO
	//	#define DUAL_BIS_MODE	//双BIS
	#define	BIS_SRC_SWITCH_MODES_WITHOUT_REBOOTING	//切换模式BIS广播不重启，保持在线
#endif

#if BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT
extern uint8_t cis_src2sink_streaming;//ase0
extern uint8_t cis_sink2src_streaming;//ase1

// #define CIS_APP_HEADSET_LR

#define LINK_WITH_PHONE
#define CIS_STEREO
	#if LE_AUDIO_ROLE == CIS_SRC
		#define LE_CIS_SRC
		#define LE_UDSC
	#elif LE_AUDIO_ROLE == CIS_SINK
		#define LE_CIS_SINK
	#endif
#endif

#define LE_AUDIO_ENC_LC3
#define LE_AUDIO_ENC_LC3_2	//使用LC3_2编码

#define LE_AUDIO_PHONE_BCAST_CODE				"6281"

#define	LE_AUDIO_MV_BROADCAST_ID				{0x22, 0x33, 0x44}
/**
 * @brief Auracast Broadcast Name
 * 
 * The default broadcast name does NOT contain the "JBL" field. To achieve
 * compatibility with JBL devices, the customer MUST manually add "JBL"
 * or other identifying marks into this broadcast name.
 * 
 * LEGAL NOTICE:
 * "JBL" is a registered trademark or trademark of HARMAN International
 * Industries, Incorporated or its affiliates. This chip and its firmware
 * do not use such trademark by default. If the customer modifies the
 * broadcast name to include the "JBL" field, the customer assumes full
 * responsibility for any and all legal consequences arising from such
 * trademark use, including but not limited to trademark infringement
 * and unfair competition. Our company does not authorize, endorse, nor
 * assume any liability for legal claims resulting from such customer
 * modifications.
 * 
 * RECOMMENDATION: Before enabling JBL compatibility, please seek advice
 * from qualified legal counsel or contact the holder of the JBL trademark.
 */

/**
 * @brief Auracast 广播名称（Broadcast Name）
 * 
 * 默认广播名称不含 "JBL" 字段。如需兼容 JBL 设备，客户必须手动将 "JBL" 
 * 或其他识别标识添加至该广播名称中。
 * 
 * 法律声明：
 * "JBL" 是 HARMAN International Industries, Incorporated 或其关联公司的
 * 注册商标或商标。本芯片及固件默认配置未使用该商标。客户若自行修改广播名称
 * 以包含 "JBL" 字段，则相关商标使用行为及法律后果（包括但不限于商标侵权、
 * 不正当竞争等）均由客户自行承担。我方未授权、不认可亦不承担
 * 因该等修改引发的任何法律责任。
 * 
 * 建议：在启用 JBL 兼容功能前，请咨询专业法律顾问。或与JBL 商标持有者联系。
 */
#define	LE_AUDIO_DUALBIS_BROADCAST_NAME		"JBL"
	
#define	LE_AUDIO_MV_BROADCAST_NAME		"MV_BIS_SRC_576"  

/*************************************************************************************
* 							LeAudio Debug配置项
************************************************************************************/
#ifdef LE_CIS_CONVERSATION
	#define		AUDIO_SINE_TEST_EN 	//不关,Mic和采样率尚未对接，for LE_CIS_CONVERSATION 16K Audio Pcm for encode, 正反向频率不同
//		#define		DEBUG_LC3_PACKET	//sink端 (反向传输)发固定包
#endif

#define VENDOR_DATA_LEN         0

#ifdef LE_AUDIO_ENC_LC3_2
	#define LE_AUDIO_LC3_TYPE		LE_CODEC_FORMAT_LC3_2
#else
	#define LE_AUDIO_LC3_TYPE		LE_CODEC_FORMAT_LC3
#endif

#define LC3_NUM_FRAMES				480							//LeAudio采样率48K, 每次处理帧长都是480sample(480*2*2)
#define LC3_FRAME_MS				(10*10)						//每帧时间为10ms,重要：LC3 MV编解码参数单位0.1mS，LC3编码器参数单位1uS
#define LC3_FRAME_US				(10000)						//每帧时间为10ms

#define LC3_BITRATE_DUALBIS			80000						//双bis比特率
#define LC3_BITRATE					96000						//比特率
#define CFG_LC3_SAMPLERATE			48000						//采样率

#define Get_LC3_FRAME_LEN(ms,bitrate)		(ms * bitrate / 8000)	//每帧长度


#define AUDIO_SBC_BITPOOL				33 //23 //20//23//
#define AUDIO_SBC_FRAME_LEN				(13+AUDIO_SBC_BITPOOL*2) //one frame length
#define AUDIO_SBC_INDEX_LEN				1 //index
#define AUDIO_SBC_PACKET_LEN			(AUDIO_SBC_FRAME_LEN + AUDIO_SBC_INDEX_LEN)

#define SBC_BLOCK   					16
#define SBC_SUBBAND 					8
#define SBC_BITPOOL 					AUDIO_SBC_BITPOOL


//send
#define AUDIO_SBC_SEND_FRAME_NUMBER 	3 //4  frames / packet
#define AUDIO_SBC_SEND_PACKET_LEN 		(AUDIO_SBC_SEND_FRAME_NUMBER * AUDIO_SBC_PACKET_LEN) //packet length

extern uint16_t lc3_packet_len;

/****************************************************************************************
 *                 组合宏编译警告
 ****************************************************************************************/
// #if (BLE_LINK_SUPPORT || BLE_TWS_SUPPORT)
// 	#error	"(TWS/BLE Link not support!!!)"
// #endif


#define min(a,b)            (((a) < (b)) ? (a) : (b))

#define BIS_SINK_SRC_RXTX_BUF_LEN	(1024+512)

/*
 * TYPES DEFINTIONS
 ****************************************************************************************
 */
enum _LE_AUDIO_SRC_
{
	LE_AUDIO_SRC_UNKNOWN = 0,
	LE_AUDIO_SRC_MV_BIS,
	LE_AUDIO_SRC_PHONE_1,
	LE_AUDIO_SRC_DUALBIS_CLIP,
	LE_AUDIO_SRC_DUALBIS_PARTY,
	LE_AUDIO_SRC_MV_DUALBIS,
	LE_AUDIO_SRC_EPPFUN,
};
//#if defined (LE_BIS_SOURCE ) && defined ( LE_BIS_SINK )
//#error BIS can be set either SRC or sink
//#endif

enum
{
	CIS_APP_NONE = 0,
	CIS_APP_MEDIA_PLAYER, /*UMS*/
	CIS_APP_PHONE,    /*CG*/
	CIS_APP_HEADSET, /*CT, UMR*/
	CIS_APP_EARBUD, /*CT, UMR*/
};

enum
{
	CIS_MODE_NONE = 0,
	CIS_MODE_MEDIA,
	CIS_MODE_CONVERSATION,   
};

/*
 * FUNCTIONS DEFINITION
 ****************************************************************************************
 */
void Set_bis_role(uint8_t role);
uint8_t Get_bis_role();
void Set_local_src(uint8_t is_local_src);
uint8_t Get_local_src(void);
void Set_cis_app(uint8_t cis_app);
uint8_t Get_cis_app(void);
void cis_app_init(uint8_t cis_app, uint8_t sink_loc);
uint8_t cis_app_get_rank(void);
uint8_t Get_sink_audio_loc(void);
void Set_sink_audio_loc(uint8_t audio_loc);
void Set_cis_role(uint8_t role);
uint8_t Get_cis_role(void);
void Set_cis_mode(uint8_t cis_mode);
uint8_t Get_cis_mode(void);
void set_src_wed_vaild(bool wed_vaild);
bool get_src_wed_vaild(void);
void set_sink_wed_vaild(bool wed_vaild);
bool get_sink_wed_vaild(void);
void set_sink_sync_lost(bool sync_lost);
bool get_sink_sync_lost(void);
void set_src_sync(bool src_sync);
bool get_src_sync(void);
void set_src1_sync(bool src_sync);
bool get_src1_sync(void);
void set_decoder_reinit(bool reinit);
bool get_decoder_reinit(void);
void set_encoder_reinit(bool reinit);
bool get_encoder_reinit(void);
void set_sink_dac_pcm_invalid(bool flag);
bool get_sink_dac_pcm_invalid_flag(void);
void set_decoder_cfg_flag(bool cfg);
bool get_decoder_cfg_flag(void);
void set_sink_scan_start_flag(uint8_t enable);
uint8_t get_sink_scan_start_flag(void);

// Event values for Configuration state machine
enum le_audio_configuration_event
{
	// Request to start Configuration state machine
	EVENT_CONFIGURATION_FIRST = 0U,
	// Device has been configured
	EVENT_CONFIGURATION_DONE,
	// Device name has been set
	EVENT_CONFIGURATION_NAME_SET,
	// Read Controller BD address
	EVENT_CONFIGURE_READ_CONTROLLER_ADDRESS,
};

#endif //#ifndef _LE_AUDIO_CONFIG_H
