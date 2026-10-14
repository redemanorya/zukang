///////////////////////////////////////////////////////////////////////////////
//               Mountain View Silicon Tech. Inc.
//  Copyright 2026, Mountain View Silicon Tech. Inc., Shanghai, China
//                       All rights reserved.
//  Filename: mvwireless2_config.h
//  maintainer: Tony
///////////////////////////////////////////////////////////////////////////////
#ifndef __MVWIRELESS2_CONFIG_H__
#define __MVWIRELESS2_CONFIG_H__

#include "app_config.h"
#include "bt_config.h"
#include "chip_config.h"
#include "flash_table.h"

/*****************************************************************
 * 山景私有无线功能开关
 * !!!需要打私有无线项目专用补丁!!!
 *****************************************************************/
//#define FUNC_WIRELESS2_SUPPORT
//LC3plus的产品制造商，需分别单独向Fraunhofer和Ericsson写英文邮件申请专利授权。
//(详见"LC3plus_License.txt")
//#define LC3P_LICENSE

#ifdef FUNC_WIRELESS2_SUPPORT
#include "wireless2.h"

#define	WIRELESS_TURNKEY4_1

#if defined(WIRELESS_TURNKEY4_1)
	#define	TURNKEY_NAME					"4_1"
	#define AUDIO_24BIT
	#define ONE_FRAME						240
	#define SAMPLE_RATE						48000
	#define ENCODER_CH						2
	#define DECODER_CH						2
	//#define WIRELESSAUDIOSYNCDAC

	#define PACKET_CRC_LEN				    (2)
	#define PACKET_CNT_LEN				    (2)
	#define RFPACK_NAUDIO					1
	#define WIRELESS_RECV_FIFO_THRHLD		(RFPACK_NAUDIO)

	#define AUDIO_FRAME_BITRATE				(108800)
	#define RFAUDIO_DECODE_CH				(1)
	#define RFAUDIO_ENCODE_LEN				(AUDIO_FRAME_BITRATE*(ONE_FRAME*1000000/SAMPLE_RATE)/8000000)
	#define RFAUDIO_DECODE_LEN				(AUDIO_FRAME_BITRATE*(ONE_FRAME*1000000/SAMPLE_RATE)/8000000)

	#define RFAUDIO_TRANS_LEN				(RFAUDIO_ENCODE_LEN+PACKET_CNT_LEN+PACKET_CRC_LEN)
	#define RFAUDIO_BACKTRANS_LEN			(RFAUDIO_ENCODE_LEN*2+PACKET_CNT_LEN+PACKET_CRC_LEN)
	#define	MV_WIRELESS2_PARAM1				1
	#define WIRELESS_FUNCTION				wireless2_4_X_initfuncset

//	#define WIRELESS_TX_DEVICE
	#ifndef	WIRELESS_TX_DEVICE
	#define EFFECT_BYPASSMODE
	#endif

	#ifdef WIRELESS_TX_DEVICE
		#define	WIRELESS_SDK_ROLE		MVWIRE2_SLAVER_ROLE
		#define ROLE_TAG				slaver
		#define RFAUDIO_ENCODE_CH		(1)
	#else
		#define	WIRELESS_SDK_ROLE		MVWIRE2_MASTER_ROLE
		#define ROLE_TAG				master
		#define RFAUDIO_ENCODE_CH		(2)
	#endif

#endif//WIRELESS_TURNKEY

/*****无线配对码 初值，TX和RX初始值要一致才能正确配对，用户可修改****/
#define			WIRELESS_LINK_KEY0					(0xA5)//配对的Key
#define			WIRELESS_LINK_KEY1					(0x56)//配对的Key

/****************************************************************************************
 *                 配对记忆保存flash地址
 ****************************************************************************************/
//#define CFG_LOCK_PAIRED_TXRX

#define CONINF_FLASH_ADDR					(WIRELEDD_CONFIG_ADDR)

typedef struct Flash_param
{
	unsigned int  PairedDev1Info;
	unsigned int  PairedDev2Info;
	unsigned char PairedDev1Type;
}Flash_param_t;

//#define WIRELESS2_EFFECTTESTMODE

#ifdef FUNC_WIRELESS2_SUPPORT
	#if (defined(CFG_FUNC_REMIND_SOUND_EN)||\
		 defined(CFG_FUNC_OPEN_SLOW_DEVICE_TASK)||\
		 defined(CFG_FUNC_BREAKPOINT_EN)||\
		 defined(FUNCT_SUPPORT)||\
		 defined(CHIP_PIN_RF))
		#error	"please close define (CFG_FUNC_OPEN_SLOW_DEVICE_TASK  \
				                      CFG_FUNC_REMIND_SOUND_EN  \
									  FUNC_BT_SUPPORT\
									  CHIP_PIN_RF)"
	#endif
	#ifndef	LC3P_LICENSE
		#error	"Check LC3P_LICENSE!!!"
	#endif
#endif

#ifdef WIRELESS2_EFFECTTESTMODE
	#if (!defined(WIRELESS_TX_DEVICE))
		#error	"WIRELESS2_EFFECTTESTMODE Must in TxMode(WIRELESS_TX_DEVICE) "
	#endif
	#if ((defined(CFG_FUNC_ROBOEFFECT_EN))&&(ONE_FRAME!=240))
		#error	"if CFG_FUNC_ROBOEFFECT_EN framesize must 240"
	#endif
#endif
#endif//FUNC_WIRELESS2_SUPPORT

#endif /*__MVWIRELESS2_CONFIG_H__*/

