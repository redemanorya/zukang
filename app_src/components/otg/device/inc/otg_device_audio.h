/**
 *****************************************************************************
 * @file     otg_device_audio.h
 * @author   Owen
 * @version  V1.0.0
 * @date     24-June-2015
 * @brief    audio device interface
 *****************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
 */

#ifndef __OTG_DEVICE_AUDIO_H__
#define	__OTG_DEVICE_AUDIO_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus
	
#include "type.h"
#include "mvintrinsics.h"
#include "mcu_circular_buf.h"
#ifdef CFG_APP_CONFIG
	#include "app_config.h"
#endif

#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
	#define USB_AUDIO_WIDTH	24
	typedef int32_t pcm_int;
	typedef int64_t gain_int;
#else
	#define USB_AUDIO_WIDTH	16
	typedef int16_t pcm_int;
	typedef int32_t gain_int;
#endif

#define DB_TO_USBVAL(db)	((int16_t)(db*256))

#define AUDIO_MAX_VOLUME	DB_TO_USBVAL(0)		//0db
#define AUDIO_MIN_VOLUME	DB_TO_USBVAL(-60)	//-60db
#define AUDIO_RES_VOLUME	0x0080				//0.5db
#define AUDIO_DEF_VOLUME    AUDIO_MAX_VOLUME	//默认音量为0db

#define PCM16BIT	2
#define PCM24BIT	3
#define PCM32BIT	4

#define AUDIO_UAC_10		10
#define AUDIO_UAC_20		20

//修改以下配置需要在PC设备管理器-声音、视频和游戏控制器—卸载相关设备
//#if defined CFG_OTG_MODE_MIC_EN || defined CFG_OTG_FS_MODE_MIC_EN
	#define MIC_ALT2_EN			//麦克风第二种PCM格式使能
	#define MIC_ALT3_EN			//麦克风第三种PCM格式使能

	#define MIC_FREQ_NUM								6			//采样率个数  MAX: FREQ_NUM_MAX
	#define	USBD_AUDIO_MIC_FREQ							192000		//192000 : bits per seconds 麦克风最大采样率
	#define	USBD_AUDIO_MIC_FREQ1						176400		//176400 : bits per seconds
	#define	USBD_AUDIO_MIC_FREQ2						96000		//96000 : bits per seconds
	#define	USBD_AUDIO_MIC_FREQ3						88200		//88200 : bits per seconds
	#define	USBD_AUDIO_MIC_FREQ4						48000		//48000 : bits per seconds
	#define	USBD_AUDIO_MIC_FREQ5						44100		//44100 : bits per seconds
	#define	USBD_AUDIO_MIC_FREQ6						44100		//44100 : bits per seconds
	#define	USBD_AUDIO_MIC_FREQ7						44100		//44100 : bits per seconds
	#define	USBD_AUDIO_MIC_FREQ8						44100		//44100 : bits per seconds
	#define	USBD_AUDIO_MIC_FREQ9						44100		//44100 : bits per seconds
	#define MIC_CHANNELS_NUM							2			//麦克风声道数  1 or 2

	#define USBD_AUDIO10_MIC_FREQ_NUM					2			//采样率个数  MAX: FREQ_NUM_MAX
	#define	USBD_AUDIO10_MIC_FREQ						48000		//192000 : bits per seconds 麦克风最大采样率
	#define	USBD_AUDIO10_MIC_FREQ1						44100		//176400 : bits per seconds
	#define	USBD_AUDIO10_MIC_FREQ2						44100		//96000 : bits per seconds
	#define	USBD_AUDIO10_MIC_FREQ3						44100		//88200 : bits per seconds
	#define	USBD_AUDIO10_MIC_FREQ4						44100		//48000 : bits per seconds
	#define	USBD_AUDIO10_MIC_FREQ5						44100		//44100 : bits per seconds
	#define	USBD_AUDIO10_MIC_FREQ6						44100		//96000 : bits per seconds
	#define	USBD_AUDIO10_MIC_FREQ7						44100		//88200 : bits per seconds
	#define	USBD_AUDIO10_MIC_FREQ8						44100		//48000 : bits per seconds
	#define	USBD_AUDIO10_MIC_FREQ9						44100		//44100 : bits per seconds

	#define	MIC_ALT1_BITS								PCM16BIT	//PCM16BIT or PCM24BIT or PCM32BIT
	#define	MIC_ALT2_BITS								PCM24BIT	//PCM16BIT or PCM24BIT or PCM32BIT
	#define	MIC_ALT3_BITS								PCM32BIT	//PCM16BIT or PCM24BIT or PCM32BIT

	#define MIC_TYPE									0x0402		//Headset
//#endif
//#if defined CFG_OTG_MODE_AUDIO_EN || defined CFG_OTG_FS_MODE_AUDIO_EN
	#define SPEAKER_ALT2_EN		//扬声器第二种PCM格式使能
	#define SPEAKER_ALT3_EN		//扬声器第三种PCM格式使能

	#define SPEAKER_FREQ_NUM							7			//采样率个数  MAX: FREQ_NUM_MAX
	#define	USBD_AUDIO_FREQ								384000		//384000 : bits per seconds 扬声器最大采样率
	#define	USBD_AUDIO_FREQ1							192000		//192000 : bits per seconds
	#define	USBD_AUDIO_FREQ2							176400		//176400 : bits per seconds
	#define	USBD_AUDIO_FREQ3							96000		//96000 : bits per seconds
	#define	USBD_AUDIO_FREQ4							88200		//88200 : bits per seconds
	#define	USBD_AUDIO_FREQ5							48000		//48000 : bits per seconds
	#define	USBD_AUDIO_FREQ6							44100		//44100 : bits per seconds
	#define	USBD_AUDIO_FREQ7							44100		//44100 : bits per seconds
	#define	USBD_AUDIO_FREQ8							44100		//44100 : bits per seconds
	#define	USBD_AUDIO_FREQ9							44100		//44100 : bits per seconds

	#define USBD_AUDIO10_SPEAKER_FREQ_NUM				2			//采样率个数  MAX: FREQ_NUM_MAX
	#define	USBD_AUDIO10_FREQ							48000		//192000 : bits per seconds 扬声器最大采样率
	#define	USBD_AUDIO10_FREQ1							44100		//176400 : bits per seconds
	#define	USBD_AUDIO10_FREQ2							44100		//96000 : bits per seconds
	#define	USBD_AUDIO10_FREQ3							44100		//88200 : bits per seconds
	#define	USBD_AUDIO10_FREQ4							44100		//48000 : bits per seconds
	#define	USBD_AUDIO10_FREQ5							44100		//44100 : bits per seconds
	#define	USBD_AUDIO10_FREQ6							44100		//44100 : bits per seconds
	#define	USBD_AUDIO10_FREQ7							44100		//44100 : bits per seconds
	#define	USBD_AUDIO10_FREQ8							44100		//44100 : bits per seconds
	#define	USBD_AUDIO10_FREQ9							44100		//44100 : bits per seconds

	#define PACKET_CHANNELS_NUM							2			//扬声器声道数  1 or 2
	#define	SPEAKER_ALT1_BITS							PCM16BIT	//PCM16BIT or PCM24BIT or PCM32BIT
	#define	SPEAKER_ALT2_BITS							PCM24BIT	//PCM16BIT or PCM24BIT or PCM32BIT
	#define	SPEAKER_ALT3_BITS							PCM32BIT	//PCM16BIT or PCM24BIT or PCM32BIT

	#define SPEAKER_TYPE								0x0402		//Headset
//#endif
//#if defined CFG_OTG_MODE_AUDIO1_EN || defined CFG_OTG_FS_MODE_AUDIO1_EN
	#define SPEAKER1_ALT2_EN		//扬声器第二种PCM格式使能
	#define SPEAKER1_ALT3_EN		//扬声器第三种PCM格式使能

	#define SPEAKER1_FREQ_NUM							6			//采样率个数  MAX: FREQ_NUM_MAX
	#define	USBD_AUDIO1_FREQ							192000		//48000 : bits per seconds 扬声器最大采样率
	#define	USBD_AUDIO1_FREQ1							176400		//44100 : bits per seconds
	#define	USBD_AUDIO1_FREQ2							96000		//44100 : bits per seconds
	#define	USBD_AUDIO1_FREQ3							88200		//44100 : bits per seconds
	#define	USBD_AUDIO1_FREQ4							48000		//44100 : bits per seconds
	#define	USBD_AUDIO1_FREQ5							44100		//44100 : bits per seconds
	#define	USBD_AUDIO1_FREQ6							44100		//44100 : bits per seconds
	#define	USBD_AUDIO1_FREQ7							44100		//44100 : bits per seconds
	#define	USBD_AUDIO1_FREQ8							44100		//44100 : bits per seconds
	#define	USBD_AUDIO1_FREQ9							44100		//44100 : bits per seconds

	#define USBD_AUDIO10_SPEAKER1_FREQ_NUM				2			//采样率个数  MAX: FREQ_NUM_MAX
	#define	USBD_AUDIO10_SPEAKER1_FREQ					48000		//192000 : bits per seconds 扬声器最大采样率
	#define	USBD_AUDIO10_SPEAKER1_FREQ1					44100		//176400 : bits per seconds
	#define	USBD_AUDIO10_SPEAKER1_FREQ2					44100		//96000 : bits per seconds
	#define	USBD_AUDIO10_SPEAKER1_FREQ3					44100		//88200 : bits per seconds
	#define	USBD_AUDIO10_SPEAKER1_FREQ4					44100		//48000 : bits per seconds
	#define	USBD_AUDIO10_SPEAKER1_FREQ5					44100		//44100 : bits per seconds
	#define	USBD_AUDIO10_SPEAKER1_FREQ6					44100		//44100 : bits per seconds
	#define	USBD_AUDIO10_SPEAKER1_FREQ7					44100		//44100 : bits per seconds
	#define	USBD_AUDIO10_SPEAKER1_FREQ8					44100		//44100 : bits per seconds
	#define	USBD_AUDIO10_SPEAKER1_FREQ9					44100		//44100 : bits per seconds

	#define PACKET1_CHANNELS_NUM						2			//扬声器声道数  1 or 2
	#define	SPEAKER1_ALT1_BITS							PCM16BIT	//PCM16BIT or PCM24BIT or PCM32BIT
	#define	SPEAKER1_ALT2_BITS							PCM24BIT	//PCM16BIT or PCM24BIT or PCM32BIT
	#define	SPEAKER1_ALT3_BITS							PCM32BIT	//PCM16BIT or PCM24BIT or PCM32BIT

	#define SPEAKER1_TYPE								0x0402		//Headset
//#endif

//以下参数不需要修改

#define MIC_FREQ_DESCRIPTOR_UAC_10_SIZE				(0x08+USBD_AUDIO10_MIC_FREQ_NUM*3)
#define MIC_FREQ_DESCRIPTOR_UAC_20_SIZE				(0x08+MIC_FREQ_NUM*3)

#ifdef MIC_ALT2_EN
#define	MIC_ALT2_DESCRIPTOR_UAC_10_SIZE				(0x28+USBD_AUDIO10_MIC_FREQ_NUM*3)		//描述符长度
#define	MIC_ALT2_DESCRIPTOR_UAC_20_SIZE				0x2E		//描述符长度
#else
#define	MIC_ALT2_DESCRIPTOR_UAC_10_SIZE				0x00
#define	MIC_ALT2_DESCRIPTOR_UAC_20_SIZE				0x00		//描述符长度
#endif
#ifdef MIC_ALT3_EN
#define	MIC_ALT3_DESCRIPTOR_UAC_10_SIZE				(0x28+USBD_AUDIO10_MIC_FREQ_NUM*3)		//描述符长度
#define	MIC_ALT3_DESCRIPTOR_UAC_20_SIZE				0x2E		//描述符长度
#else
#define	MIC_ALT3_DESCRIPTOR_UAC_10_SIZE				0x00
#define	MIC_ALT3_DESCRIPTOR_UAC_20_SIZE				0x00		//描述符长度
#endif

#define SPEAKER_FREQ_DESCRIPTOR_UAC_10_SIZE			(0x08+USBD_AUDIO10_SPEAKER_FREQ_NUM*3)
#define SPEAKER_FREQ_DESCRIPTOR_UAC_20_SIZE			(0x08+SPEAKER_FREQ_NUM*3)

#ifdef SPEAKER_ALT2_EN
#define	SPEAKER_ALT2_DESCRIPTOR_UAC_10_SIZE			(0x28+USBD_AUDIO10_SPEAKER_FREQ_NUM*3)		//描述符长度
#define	SPEAKER_ALT2_DESCRIPTOR_UAC_20_SIZE			0x2E		//描述符长度
#else
#define	SPEAKER_ALT2_DESCRIPTOR_UAC_10_SIZE			0x00
#define	SPEAKER_ALT2_DESCRIPTOR_UAC_20_SIZE			0x00		//描述符长度
#endif
#ifdef SPEAKER_ALT3_EN
#define	SPEAKER_ALT3_DESCRIPTOR_UAC_10_SIZE			(0x28+USBD_AUDIO10_SPEAKER_FREQ_NUM*3)		//描述符长度
#define	SPEAKER_ALT3_DESCRIPTOR_UAC_20_SIZE			0x2E		//描述符长度
#else
#define	SPEAKER_ALT3_DESCRIPTOR_UAC_10_SIZE			0x00
#define	SPEAKER_ALT3_DESCRIPTOR_UAC_20_SIZE			0x00		//描述符长度
#endif

#define SPEAKER1_FREQ_DESCRIPTOR_UAC_10_SIZE		(0x08+USBD_AUDIO10_SPEAKER1_FREQ_NUM*3)
#define SPEAKER1_FREQ_DESCRIPTOR_UAC_20_SIZE		(0x08+SPEAKER1_FREQ_NUM*3)

#ifdef SPEAKER1_ALT2_EN
#define	SPEAKER1_ALT2_DESCRIPTOR_UAC_10_SIZE		(0x28+USBD_AUDIO10_SPEAKER1_FREQ_NUM*3)		//描述符长度
#define	SPEAKER1_ALT2_DESCRIPTOR_UAC_20_SIZE		0x2E
#else
#define	SPEAKER1_ALT2_DESCRIPTOR_UAC_10_SIZE		0x00
#define	SPEAKER1_ALT2_DESCRIPTOR_UAC_20_SIZE		0x00
#endif
#ifdef SPEAKER1_ALT3_EN
#define	SPEAKER1_ALT3_DESCRIPTOR_UAC_10_SIZE		(0x28+USBD_AUDIO10_SPEAKER1_FREQ_NUM*3)		//描述符长度
#define	SPEAKER1_ALT3_DESCRIPTOR_UAC_20_SIZE		0x2E
#else
#define	SPEAKER1_ALT3_DESCRIPTOR_UAC_10_SIZE		0x00
#define	SPEAKER1_ALT3_DESCRIPTOR_UAC_20_SIZE		0x00
#endif


#if defined CFG_OTG_MODE_MIC_EN
	#define	MIC_HS_DESCRIPTOR_UAC_10_SIZE	( MIC_CHANNELS_NUM+MIC_FREQ_DESCRIPTOR_UAC_10_SIZE+MIC_ALT2_DESCRIPTOR_UAC_10_SIZE)
	#define	MIC_HS_DESCRIPTOR_UAC_20_SIZE	((MIC_CHANNELS_NUM*4)+MIC_ALT2_DESCRIPTOR_UAC_20_SIZE+MIC_ALT3_DESCRIPTOR_UAC_20_SIZE)
#else
	#define	MIC_HS_DESCRIPTOR_UAC_10_SIZE		0
	#define	MIC_HS_DESCRIPTOR_UAC_20_SIZE		0
#endif
#if defined CFG_OTG_MODE_AUDIO_EN
	#define	SPEAKER_HS_DESCRIPTOR_UAC_10_SIZE	( PACKET_CHANNELS_NUM+SPEAKER_FREQ_DESCRIPTOR_UAC_10_SIZE+SPEAKER_ALT2_DESCRIPTOR_UAC_10_SIZE )
	#define	SPEAKER_HS_DESCRIPTOR_UAC_20_SIZE	((PACKET_CHANNELS_NUM*4)+SPEAKER_ALT2_DESCRIPTOR_UAC_20_SIZE+SPEAKER_ALT3_DESCRIPTOR_UAC_20_SIZE )
#else
	#define	SPEAKER_HS_DESCRIPTOR_UAC_10_SIZE	0
	#define	SPEAKER_HS_DESCRIPTOR_UAC_20_SIZE	0
#endif
#if defined CFG_OTG_MODE_AUDIO1_EN
	#define	SPEAKER1_HS_DESCRIPTOR_UAC_10_SIZE	( PACKET1_CHANNELS_NUM+SPEAKER1_FREQ_DESCRIPTOR_UAC_10_SIZE+SPEAKER1_ALT2_DESCRIPTOR_UAC_10_SIZE )
	#define	SPEAKER1_HS_DESCRIPTOR_UAC_20_SIZE	((PACKET1_CHANNELS_NUM*4)+SPEAKER1_ALT2_DESCRIPTOR_UAC_20_SIZE+SPEAKER1_ALT3_DESCRIPTOR_UAC_20_SIZE )
#else
	#define	SPEAKER1_HS_DESCRIPTOR_UAC_10_SIZE	0
	#define	SPEAKER1_HS_DESCRIPTOR_UAC_20_SIZE	0
#endif

#if defined CFG_OTG_FS_MODE_MIC_EN
	#define	MIC_FS_DESCRIPTOR_UAC_10_SIZE	( MIC_CHANNELS_NUM+MIC_FREQ_DESCRIPTOR_UAC_10_SIZE+MIC_ALT2_DESCRIPTOR_UAC_10_SIZE )
	#define	MIC_FS_DESCRIPTOR_UAC_20_SIZE	((MIC_CHANNELS_NUM*4)+ MIC_ALT2_DESCRIPTOR_UAC_20_SIZE)
#else
	#define	MIC_FS_DESCRIPTOR_UAC_10_SIZE	0
	#define	MIC_FS_DESCRIPTOR_UAC_20_SIZE	0
#endif
#if defined CFG_OTG_FS_MODE_AUDIO_EN
	#define	SPEAKER_FS_DESCRIPTOR_UAC_10_SIZE	( PACKET_CHANNELS_NUM+SPEAKER_FREQ_DESCRIPTOR_UAC_10_SIZE+SPEAKER_ALT2_DESCRIPTOR_UAC_10_SIZE )
	#define	SPEAKER_FS_DESCRIPTOR_UAC_20_SIZE	((PACKET_CHANNELS_NUM*4)+SPEAKER_ALT2_DESCRIPTOR_UAC_20_SIZE)
#else
	#define	SPEAKER_FS_DESCRIPTOR_UAC_10_SIZE	0
	#define	SPEAKER_FS_DESCRIPTOR_UAC_20_SIZE	0
#endif
#if defined CFG_OTG_FS_MODE_AUDIO1_EN
	#define	SPEAKER1_FS_DESCRIPTOR_UAC_10_SIZE	( PACKET1_CHANNELS_NUM+SPEAKER1_FREQ_DESCRIPTOR_UAC_10_SIZE+SPEAKER1_ALT2_DESCRIPTOR_UAC_10_SIZE )
	#define	SPEAKER1_FS_DESCRIPTOR_UAC_20_SIZE	((PACKET1_CHANNELS_NUM*4)+SPEAKER1_ALT2_DESCRIPTOR_UAC_20_SIZE)
#else
	#define	SPEAKER1_FS_DESCRIPTOR_UAC_10_SIZE	0
	#define	SPEAKER1_FS_DESCRIPTOR_UAC_20_SIZE	0
#endif

#define USER_HS_CONFIG_DESCRIPTOR_UAC_10_SIZE	(MIC_HS_DESCRIPTOR_UAC_10_SIZE + SPEAKER_HS_DESCRIPTOR_UAC_10_SIZE + SPEAKER1_HS_DESCRIPTOR_UAC_10_SIZE)

#define USER_HS_CONFIG_DESCRIPTOR_UAC_20_SIZE	(MIC_HS_DESCRIPTOR_UAC_20_SIZE + SPEAKER_HS_DESCRIPTOR_UAC_20_SIZE + SPEAKER1_HS_DESCRIPTOR_UAC_20_SIZE)

#define USER_FS_CONFIG_DESCRIPTOR_UAC_10_SIZE	(MIC_FS_DESCRIPTOR_UAC_10_SIZE + SPEAKER_FS_DESCRIPTOR_UAC_10_SIZE + SPEAKER1_FS_DESCRIPTOR_UAC_10_SIZE)

#define USER_FS_CONFIG_DESCRIPTOR_UAC_20_SIZE	(MIC_FS_DESCRIPTOR_UAC_20_SIZE + SPEAKER_FS_DESCRIPTOR_UAC_20_SIZE + SPEAKER1_FS_DESCRIPTOR_UAC_20_SIZE)


#define	DEVICE_HS_ISO_IN_MPS		2048//(USBD_AUDIO_MIC_FREQ*MIC_CHANNELS_NUM*MAX(MIC_ALT1_BITS,MIC_ALT2_BITS)/1000)
#define	DEVICE_HS_ISO_OUT_MPS		2048//(USBD_AUDIO_FREQ*PACKET_CHANNELS_NUM*MAX(SPEAKER_ALT1_BITS,SPEAKER_ALT2_BITS)/1000)
#define	DEVICE_HS_ISO_OUT1_MPS		2048//(USBD_AUDIO1_FREQ*PACKET1_CHANNELS_NUM*MAX(SPEAKER1_ALT1_BITS,SPEAKER1_ALT2_BITS)/1000)

#if defined(CFG_OTG_FS_MODE_MIC_EN) || defined(CFG_OTG_MODE_MIC_EN)
	#ifndef MIC_ALT2_EN
		#undef	MIC_ALT2_BITS
		#define	MIC_ALT2_BITS	0
	#endif
	#ifndef MIC_ALT3_EN
		#undef	MIC_ALT3_BITS
		#define	MIC_ALT3_BITS	0
	#endif
//	#define	DEVICE_FS_ISO_IN_MPS	(MAX(USBD_AUDIO10_MIC_FREQ,48000)*MIC_CHANNELS_NUM*MAX(MAX(MIC_ALT1_BITS,MIC_ALT2_BITS),MIC_ALT3_BITS)/1000)
	#define	DEVICE_FS_ISO_IN_MPS	(MAX(USBD_AUDIO10_MIC_FREQ,48000)*MIC_CHANNELS_NUM*MAX(MIC_ALT1_BITS,MIC_ALT2_BITS)/1000)
#else
	#define	DEVICE_FS_ISO_IN_MPS		0
#endif

#if defined(CFG_OTG_FS_MODE_AUDIO_EN) || defined(CFG_OTG_MODE_AUDIO_EN)
	#ifndef SPEAKER_ALT2_EN
		#undef	SPEAKER_ALT2_BITS
		#define	SPEAKER_ALT2_BITS	0
	#endif
	#ifndef SPEAKER_ALT3_EN
		#undef	SPEAKER_ALT3_BITS
		#define	SPEAKER_ALT3_BITS	0
	#endif
//	#define	DEVICE_FS_ISO_OUT_MPS	(MAX(USBD_AUDIO10_FREQ,48000)*PACKET_CHANNELS_NUM*MAX(MAX(SPEAKER_ALT1_BITS,SPEAKER_ALT2_BITS),SPEAKER_ALT3_BITS)/1000)
	#define	DEVICE_FS_ISO_OUT_MPS	(MAX(USBD_AUDIO10_FREQ,48000)*PACKET_CHANNELS_NUM*MAX(SPEAKER_ALT1_BITS,SPEAKER_ALT2_BITS)/1000)
#else
	#define	DEVICE_FS_ISO_OUT_MPS		0
#endif

#if defined(CFG_OTG_FS_MODE_AUDIO1_EN) || defined(CFG_OTG_MODE_AUDIO1_EN)
	#ifndef SPEAKER1_ALT2_EN
		#undef	SPEAKER1_ALT2_BITS
		#define	SPEAKER1_ALT2_BITS	0
	#endif
	#ifndef SPEAKER1_ALT3_EN
		#undef	SPEAKER1_ALT3_BITS
		#define	SPEAKER1_ALT3_BITS	0
	#endif
//	#define	DEVICE_FS_ISO_OUT1_MPS	(MAX(USBD_AUDIO10_SPEAKER1_FREQ,48000)*PACKET1_CHANNELS_NUM*MAX(MAX(SPEAKER1_ALT1_BITS,SPEAKER1_ALT2_BITS),SPEAKER1_ALT3_BITS)/1000)
	#define	DEVICE_FS_ISO_OUT1_MPS	(MAX(USBD_AUDIO10_SPEAKER1_FREQ,48000)*PACKET1_CHANNELS_NUM*MAX(SPEAKER1_ALT1_BITS,SPEAKER1_ALT2_BITS)/1000)
#else
	#define	DEVICE_FS_ISO_OUT1_MPS		0
#endif


#if (DEVICE_FS_ISO_IN_MPS + DEVICE_FS_ISO_OUT_MPS + DEVICE_FS_ISO_OUT1_MPS > 1000)
#error usb带宽不够
#endif


#define AUDIO_CONTROL_MUTE                            0x01
#define AUDIO_CONTROL_VOLUME                          0x02

#define AUDIO_SPEAKER_IT_ID							  0x01
#define AUDIO_SPEAKER_FU_ID							  0x02		//控制MUTE、VOLUME
#define AUDIO_SPEAKER_OT_ID							  0x03
#define AUDIO_SPEAKER_CLK_ID                          0x08

#define AUDIO_MIC_IT_ID								  0x04
#define AUDIO_MIC_FU_ID								  0x05
#define AUDIO_MIC_SL_ID								  0x06
#define AUDIO_MIC_OT_ID								  0x07
#define AUDIO_MIC_CLK_ID                              0x09

#define AUDIO_SPEAKER1_IT_ID						  0x10
#define AUDIO_SPEAKER1_FU_ID						  0x11	//控制MUTE、VOLUME
#define AUDIO_SPEAKER1_OT_ID						  0x12
#define AUDIO_SPEAKER1_CLK_ID                         0x13

#define AUDIO_MIC_MONITOR_FU_ID						  0x14
#define AUDIO_MIC_MONITOR_MIX_ID					  0x15

#define CHANNEL_CONFIG(chn)  		(uint8_t)(((1U << (chn)) - 1))	//1~8chn

#define FRQ_MAX_SZE(frq) 			((frq%1000?frq/1000+1:frq/1000))
#define SWAP_BUF_TO_U16(buf) 		((buf[0]) | (buf[1]<<8))
#define SWAP_BUF_TO_U32(buf) 		((buf[0]) | (buf[1]<<8) | (buf[2]<<16) | (buf[3]<<24))
#define SAMPLE_FREQ_NUM(num)        (uint8_t)(num), (uint8_t)((num >> 8))
#define SAMPLE_FREQ(frq)            (uint8_t)(frq), (uint8_t)((frq >> 8)), (uint8_t)((frq >> 16))
#define SAMPLE_FREQ_4B(frq)         (uint8_t)(frq), (uint8_t)((frq >> 8)), \
									(uint8_t)((frq >> 16)), (uint8_t)((frq >> 24))

#define GET_PACKET_CNT(total) ( (total) <= 1023 ? 0 : (total) <= (1023 * 2) ? 1 : 2 )
#define GET_SINGLE_LEN(total) ( total / (GET_PACKET_CNT(total)+1 ) )
#define GET_SINGLE_LEN(total) ( total / (GET_PACKET_CNT(total)+1 ) )

#define GET_MAX_PACK(frq,chn,bytes,fps)  \
    ( (uint16_t)( \
        (GET_SINGLE_LEN((frq*13/10/fps)*chn*bytes) & 0x03FF) |  /* 自动获取单包长度，保留低10位 */ \
        ((GET_PACKET_CNT((frq*13/10/fps)*chn*bytes) & 0x03) << 11) |  /* 自动获取包次数，左移11位占bit11~12 */ \
        0x0000  /* 保留位bit13~15填0，符合USB描述符规范 */ \
    ) )

#define GET_MAX_PACK_FS(frq,chn,bytes,fps)  \
    ( (uint16_t)( (FRQ_MAX_SZE(frq) * chn * bytes)) )


#define AUDIO_EP_MAX_SZE(frq,chn,bytes) (uint8_t)(((FRQ_MAX_SZE(frq) * chn * bytes)) & 0xFF), \
                                       (uint8_t)((((FRQ_MAX_SZE(frq) * chn * bytes)) >> 8) & 0xFF)

#define ISO_EP_MAX_SZE(frq,chn,bytes,fps) 	(uint8_t)(GET_MAX_PACK(frq,chn,bytes,fps) & 0xFF), \
                                			(uint8_t)((GET_MAX_PACK(frq,chn,bytes,fps)>>8) & 0xFF)

#define USBD_AUDIO10_FREQ_1()    SAMPLE_FREQ(USBD_AUDIO10_FREQ)
#define USBD_AUDIO10_FREQ_2()    USBD_AUDIO10_FREQ_1(), SAMPLE_FREQ(USBD_AUDIO10_FREQ1)
#define USBD_AUDIO10_FREQ_3()    USBD_AUDIO10_FREQ_2(), SAMPLE_FREQ(USBD_AUDIO10_FREQ2)
#define USBD_AUDIO10_FREQ_4()    USBD_AUDIO10_FREQ_3(), SAMPLE_FREQ(USBD_AUDIO10_FREQ3)
#define USBD_AUDIO10_FREQ_5()    USBD_AUDIO10_FREQ_4(), SAMPLE_FREQ(USBD_AUDIO10_FREQ4)
#define USBD_AUDIO10_FREQ_6()    USBD_AUDIO10_FREQ_5(), SAMPLE_FREQ(USBD_AUDIO10_FREQ5)
#define USBD_AUDIO10_FREQ_7()    USBD_AUDIO10_FREQ_6(), SAMPLE_FREQ(USBD_AUDIO10_FREQ6)
#define USBD_AUDIO10_FREQ_8()    USBD_AUDIO10_FREQ_7(), SAMPLE_FREQ(USBD_AUDIO10_FREQ7)
#define USBD_AUDIO10_FREQ_9()    USBD_AUDIO10_FREQ_8(), SAMPLE_FREQ(USBD_AUDIO10_FREQ8)
#define USBD_AUDIO10_FREQ_10()   USBD_AUDIO10_FREQ_9(), SAMPLE_FREQ(USBD_AUDIO10_FREQ9)
#define SAMPLE_USBD_AUDIO10_FREQ_ARRAY(num) USBD_AUDIO10_FREQ_##num()

#define USBD_AUDIO10_MIC_FREQ_1()    SAMPLE_FREQ(USBD_AUDIO10_MIC_FREQ)
#define USBD_AUDIO10_MIC_FREQ_2()    USBD_AUDIO10_MIC_FREQ_1(), SAMPLE_FREQ(USBD_AUDIO10_MIC_FREQ1)
#define USBD_AUDIO10_MIC_FREQ_3()    USBD_AUDIO10_MIC_FREQ_2(), SAMPLE_FREQ(USBD_AUDIO10_MIC_FREQ2)
#define USBD_AUDIO10_MIC_FREQ_4()    USBD_AUDIO10_MIC_FREQ_3(), SAMPLE_FREQ(USBD_AUDIO10_MIC_FREQ3)
#define USBD_AUDIO10_MIC_FREQ_5()    USBD_AUDIO10_MIC_FREQ_4(), SAMPLE_FREQ(USBD_AUDIO10_MIC_FREQ4)
#define USBD_AUDIO10_MIC_FREQ_6()    USBD_AUDIO10_MIC_FREQ_5(), SAMPLE_FREQ(USBD_AUDIO10_MIC_FREQ5)
#define USBD_AUDIO10_MIC_FREQ_7()    USBD_AUDIO10_MIC_FREQ_6(), SAMPLE_FREQ(USBD_AUDIO10_MIC_FREQ6)
#define USBD_AUDIO10_MIC_FREQ_8()    USBD_AUDIO10_MIC_FREQ_7(), SAMPLE_FREQ(USBD_AUDIO10_MIC_FREQ7)
#define USBD_AUDIO10_MIC_FREQ_9()    USBD_AUDIO10_MIC_FREQ_8(), SAMPLE_FREQ(USBD_AUDIO10_MIC_FREQ8)
#define USBD_AUDIO10_MIC_FREQ_10()   USBD_AUDIO10_MIC_FREQ_9(), SAMPLE_FREQ(USBD_AUDIO10_MIC_FREQ9)
#define SAMPLE_USBD_AUDIO10_MIC_FREQ_ARRAY(num)  USBD_AUDIO10_MIC_FREQ_##num()

#define USBD_AUDIO_FREQ_1()    SAMPLE_FREQ(USBD_AUDIO_FREQ)
#define USBD_AUDIO_FREQ_2()    USBD_AUDIO_FREQ_1(), SAMPLE_FREQ(USBD_AUDIO_FREQ1)
#define USBD_AUDIO_FREQ_3()    USBD_AUDIO_FREQ_2(), SAMPLE_FREQ(USBD_AUDIO_FREQ2)
#define USBD_AUDIO_FREQ_4()    USBD_AUDIO_FREQ_3(), SAMPLE_FREQ(USBD_AUDIO_FREQ3)
#define USBD_AUDIO_FREQ_5()    USBD_AUDIO_FREQ_4(), SAMPLE_FREQ(USBD_AUDIO_FREQ4)
#define USBD_AUDIO_FREQ_6()    USBD_AUDIO_FREQ_5(), SAMPLE_FREQ(USBD_AUDIO_FREQ5)
#define USBD_AUDIO_FREQ_7()    USBD_AUDIO_FREQ_6(), SAMPLE_FREQ(USBD_AUDIO_FREQ6)
#define USBD_AUDIO_FREQ_8()    USBD_AUDIO_FREQ_7(), SAMPLE_FREQ(USBD_AUDIO_FREQ7)
#define USBD_AUDIO_FREQ_9()    USBD_AUDIO_FREQ_8(), SAMPLE_FREQ(USBD_AUDIO_FREQ8)
#define USBD_AUDIO_FREQ_10()   USBD_AUDIO_FREQ_9(), SAMPLE_FREQ(USBD_AUDIO_FREQ9)
#define SAMPLE_USBD_AUDIO_FREQ_ARRAY(num)  USBD_AUDIO_FREQ_##num()

#define USBD_AUDIO_MIC_FREQ_1()    SAMPLE_FREQ(USBD_AUDIO_MIC_FREQ)
#define USBD_AUDIO_MIC_FREQ_2()    USBD_AUDIO_MIC_FREQ_1(), SAMPLE_FREQ(USBD_AUDIO_MIC_FREQ1)
#define USBD_AUDIO_MIC_FREQ_3()    USBD_AUDIO_MIC_FREQ_2(), SAMPLE_FREQ(USBD_AUDIO_MIC_FREQ2)
#define USBD_AUDIO_MIC_FREQ_4()    USBD_AUDIO_MIC_FREQ_3(), SAMPLE_FREQ(USBD_AUDIO_MIC_FREQ3)
#define USBD_AUDIO_MIC_FREQ_5()    USBD_AUDIO_MIC_FREQ_4(), SAMPLE_FREQ(USBD_AUDIO_MIC_FREQ4)
#define USBD_AUDIO_MIC_FREQ_6()    USBD_AUDIO_MIC_FREQ_5(), SAMPLE_FREQ(USBD_AUDIO_MIC_FREQ5)
#define USBD_AUDIO_MIC_FREQ_7()    USBD_AUDIO_MIC_FREQ_6(), SAMPLE_FREQ(USBD_AUDIO_MIC_FREQ6)
#define USBD_AUDIO_MIC_FREQ_8()    USBD_AUDIO_MIC_FREQ_7(), SAMPLE_FREQ(USBD_AUDIO_MIC_FREQ7)
#define USBD_AUDIO_MIC_FREQ_9()    USBD_AUDIO_MIC_FREQ_8(), SAMPLE_FREQ(USBD_AUDIO_MIC_FREQ8)
#define USBD_AUDIO_MIC_FREQ_10()   USBD_AUDIO_MIC_FREQ_9(), SAMPLE_FREQ(USBD_AUDIO_MIC_FREQ9)
#define SAMPLE_USBD_AUDIO_MIC_FREQ_ARRAY(num)  USBD_AUDIO_MIC_FREQ_##num()


#define USBD_AUDIO1_FREQ_1()    SAMPLE_FREQ(USBD_AUDIO1_FREQ)
#define USBD_AUDIO1_FREQ_2()    USBD_AUDIO1_FREQ_1(), SAMPLE_FREQ(USBD_AUDIO1_FREQ1)
#define USBD_AUDIO1_FREQ_3()    USBD_AUDIO1_FREQ_2(), SAMPLE_FREQ(USBD_AUDIO1_FREQ2)
#define USBD_AUDIO1_FREQ_4()    USBD_AUDIO1_FREQ_3(), SAMPLE_FREQ(USBD_AUDIO1_FREQ3)
#define USBD_AUDIO1_FREQ_5()    USBD_AUDIO1_FREQ_4(), SAMPLE_FREQ(USBD_AUDIO1_FREQ4)
#define USBD_AUDIO1_FREQ_6()    USBD_AUDIO1_FREQ_5(), SAMPLE_FREQ(USBD_AUDIO1_FREQ5)
#define USBD_AUDIO1_FREQ_7()    USBD_AUDIO1_FREQ_6(), SAMPLE_FREQ(USBD_AUDIO1_FREQ6)
#define USBD_AUDIO1_FREQ_8()    USBD_AUDIO1_FREQ_7(), SAMPLE_FREQ(USBD_AUDIO1_FREQ7)
#define USBD_AUDIO1_FREQ_9()    USBD_AUDIO1_FREQ_8(), SAMPLE_FREQ(USBD_AUDIO1_FREQ8)
#define USBD_AUDIO1_FREQ_10()   USBD_AUDIO1_FREQ_9(), SAMPLE_FREQ(USBD_AUDIO1_FREQ9)
#define SAMPLE_USBD_AUDIO1_FREQ_ARRAY(num)  USBD_AUDIO1_FREQ_##num()

#define USBD_AUDIO10_SPEAKER1_FREQ_1()    SAMPLE_FREQ(USBD_AUDIO10_SPEAKER1_FREQ)
#define USBD_AUDIO10_SPEAKER1_FREQ_2()    USBD_AUDIO10_SPEAKER1_FREQ_1(), SAMPLE_FREQ(USBD_AUDIO10_SPEAKER1_FREQ1)
#define USBD_AUDIO10_SPEAKER1_FREQ_3()    USBD_AUDIO10_SPEAKER1_FREQ_2(), SAMPLE_FREQ(USBD_AUDIO10_SPEAKER1_FREQ2)
#define USBD_AUDIO10_SPEAKER1_FREQ_4()    USBD_AUDIO10_SPEAKER1_FREQ_3(), SAMPLE_FREQ(USBD_AUDIO10_SPEAKER1_FREQ3)
#define USBD_AUDIO10_SPEAKER1_FREQ_5()    USBD_AUDIO10_SPEAKER1_FREQ_4(), SAMPLE_FREQ(USBD_AUDIO10_SPEAKER1_FREQ4)
#define USBD_AUDIO10_SPEAKER1_FREQ_6()    USBD_AUDIO10_SPEAKER1_FREQ_5(), SAMPLE_FREQ(USBD_AUDIO10_SPEAKER1_FREQ5)
#define USBD_AUDIO10_SPEAKER1_FREQ_7()    USBD_AUDIO10_SPEAKER1_FREQ_6(), SAMPLE_FREQ(USBD_AUDIO10_SPEAKER1_FREQ6)
#define USBD_AUDIO10_SPEAKER1_FREQ_8()    USBD_AUDIO10_SPEAKER1_FREQ_7(), SAMPLE_FREQ(USBD_AUDIO10_SPEAKER1_FREQ7)
#define USBD_AUDIO10_SPEAKER1_FREQ_9()    USBD_AUDIO10_SPEAKER1_FREQ_8(), SAMPLE_FREQ(USBD_AUDIO10_SPEAKER1_FREQ8)
#define USBD_AUDIO10_SPEAKER1_FREQ_10()   USBD_AUDIO10_SPEAKER1_FREQ_9(), SAMPLE_FREQ(USBD_AUDIO10_SPEAKER1_FREQ9)
#define SAMPLE_USBD_AUDIO10_SPEAKER1_FREQ_ARRAY(num)  USBD_AUDIO10_SPEAKER1_FREQ_##num()

#define SAMPLE_FREQ_ARRAY(type,num)  SAMPLE_##type##_ARRAY(num)

#define FREQ_NUM_MAX 				10
#define CHANNEL_NUM_MAX 			8
#define PCM_LEN_MAX 				MAX(DEVICE_HS_ISO_IN_MPS,MAX(DEVICE_HS_ISO_OUT1_MPS,DEVICE_HS_ISO_OUT_MPS))


#define FS_FRAM_PRT_SECOND  		1000U
#define FS_ISO_INTERVAL  			1U

#define HS_UAC_10_FRAM_PRT_SECOND	1000U
#define HS_UAC_10_ISO_INTERVAL  	4U

#define HS_UAC_20_FRAM_PRT_SECOND	2000U
#define HS_UAC_20_ISO_INTERVAL  	3U

typedef void (*UsbAudioSetFunc)(void);
typedef void (*UsbAudioVolumeFunc)(uint8_t channel);

#define USB_SINK_FLAG				0x80
#define USB_SOURCE_SET(STREAM, NUM) ((STREAM)->AudioCoreId = NUM)
#define USB_SINK_SET(STREAM, NUM) 	((STREAM)->AudioCoreId = NUM | USB_SINK_FLAG)
#define USB_IS_SINK(STREAM)			((STREAM)->AudioCoreId & USB_SINK_FLAG)
#define USB_AUDIO_NUM_GET(STREAM) 	((STREAM)->AudioCoreId & (~USB_SINK_FLAG))


// 功能：把FS值还原为HS基础值（HS值直接返回，FS值扣掉偏移）
// 输入：任意参数值（比如12→2，14→4，5→5）
#define TO_HS_VALUE(param)  ((param) > FS_DESC_BASE_OFFSET ? ((param) - FS_DESC_BASE_OFFSET) : (param))

// 功能：判断还原后的HS值是否是有效声卡值（1~6）
// 输入：任意参数值（返回1=有效，0=无效）
#define IS_VALID_AUDIO(param)  (TO_HS_VALUE(param) >= 1 && TO_HS_VALUE(param) <= (UAC10_OFFSET*2)) ? 1 : 0

// 功能：判断有效声卡值的UAC版本（仅对有效声卡值生效）
// 输入：任意参数值（返回1=UAC1.0，2=UAC2.0，0=无效）
#define GET_UAC_VERSION(param)  (IS_VALID_AUDIO(param) == 1) ? (TO_HS_VALUE(param) <= UAC10_OFFSET ? AUDIO_UAC_20 : AUDIO_UAC_10) : 0

// 功能：UAC2.0模式 → UAC1.0模式（支持HS/FS）
// 输入：UAC2.0的模式值
#define UAC20_TO_UAC10(param)  ((param) + UAC10_OFFSET)

typedef struct _UsbAudio
{
	uint8_t					AltSet;
	bool 					FirstThrhld;//Wait FIFO_THRHLD
	uint8_t					ByteSet;
	uint8_t 				Channels;
	uint8_t 				Mute;
	int16_t					Vol[CHANNEL_NUM_MAX];
	int16_t					Gain[CHANNEL_NUM_MAX];
	int16_t					VolMax;
	int16_t					VolMin;
	int16_t					VolRes;
	uint8_t					FuId;
	uint8_t					ClkId;
	uint8_t					EpNum;
	uint8_t					SampleRateNum;
	uint32_t				SampleRateS[FREQ_NUM_MAX];
	uint8_t					SampleRateValid;
	uint32_t				AudioSampleRate;
	uint16_t				FramesPerSecond;
	uint32_t				FramCount;
	uint32_t				TempFramCount;
	uint32_t 				Accumulator;
#if defined(CFG_RES_AUDIO_USB_VOL_SET_EN) || defined(CFG_RES_AUDIO_USB_FS_VOL_SET_EN)
	int16_t					CurGain[CHANNEL_NUM_MAX];
#endif
	UsbAudioSetFunc			SampleRateSet;
	UsbAudioVolumeFunc		VolumeSet;

	//Mount audioIO info、FIFO, no Clear
	uint8_t					AudioCoreId;//AudioCoreNum;
	MCU_CIRCULAR_CONTEXT 	CircularBuf;
	int16_t*				PCMBuffer;
	/*care about Samplerate、Channels reset*/
}UsbAudio;

extern UsbAudio UsbAudioMic;
extern UsbAudio UsbAudioSpeaker;
extern UsbAudio UsbAudioSpeaker1;

extern UsbAudio UsbAudioMixMic;
extern UsbAudio UsbAudioMixSpeaker;
extern UsbAudio UsbAudioMixSpeaker1;
// 1. 定义最大支持的实例数量（根据实际需求调整）
#define MAX_REGISTERED_AUDIO 3

typedef struct _UsbAudioClass
{
	uint8_t *iso_buf;
    UsbAudio *registered_audio[MAX_REGISTERED_AUDIO]; // 注册的UsbAudio指针数组
    uint8_t registered_count; // 已注册的数量
}UsbAudioClass;

#define SET_CUR		0x2101
#define SET_IDLE	0x210A
#define GET_CUR		0xA181
#define GET_MIN		0xA182
#define GET_MAX		0xA183
#define GET_RES		0xA184

#define SET_CUR_EP	0x2201
#define GET_CUR_EP	0xA281
//uac 2.0
#define GET_RANGE	0xA102
#define GET_CUR_2	0xA101

#define CS_SAM_FREQ_CONTROL 	0x01
#define CS_CLOCK_VALID_CONTROL 	0x02

#define AS_VAL_ALT_SETTINGS_CONTROL 0x02


#define AUDIO_VOL_UP      BIT(0)
#define AUDIO_VOL_DN      BIT(1)
#define AUDIO_PP          BIT(2)
#define AUDIO_NEXT        BIT(3)
#define AUDIO_PREV        BIT(4)
#define AUDIO_STOP        BIT(5)
#define AUDIO_FF          BIT(6)
#define AUDIO_FD          BIT(7)

#define AUDIO_REDIAL 	  BIT(8)
#define AUDIO_HOOK_SWITCH BIT(9)
#define AUDIO_PHONE_MUTE  BIT(10)

int16_t UsbValToMcuGain(int16_t UsbVal);


//HS
bool OTG_HS_DeviceAudioSendPcCmd(uint16_t Cmd);
bool OTG_HS_DeviceAudioSendHidKeyDown(uint16_t Cmd);
bool OTG_HS_DeviceAudioSendHidKeyUp(void);

uint16_t UsbAudioSpeaker1DataGet(void *Buffer,uint16_t Len);
uint16_t UsbAudioSpeaker1DataLenGet(void);
uint16_t UsbAudioSpeaker1DepthGet(void);

uint16_t UsbAudioSpeakerDataGet(void *Buffer,uint16_t Len);
uint16_t UsbAudioSpeakerDataLenGet(void);
uint16_t UsbAudioSpeakerDepthGet(void);

uint16_t UsbAudioMicDataSet(void *Buffer,uint16_t Len);
uint16_t UsbAudioMicSpaceLenGet(void);
uint16_t UsbAudioMicDepthGet(void);

void OnDeviceHsAudioSendIsoPacket(void);
void OnDeviceHsAudioRcvIsoPacket(void);
void OnDeviceAudioRcvIsoPacket1(void);
void OnDeviceAudioSendIntHeartBeatPacket(void);

void UsbHsMicSampleRateSet(void);
void UsbHsSpeakerSampleRateSet(void);
void UsbHsSpeaker1SampleRateSet(void);

void UsbHsMicVolumeSet(uint8_t channel);
void UsbHsSpeakerVolumeSet(uint8_t channel);
void UsbHsSpeaker1VolumeSet(uint8_t channel);

void UsbHsSpeaker1Alt(uint8_t Alt);
void UsbHsSpeakerAlt(uint8_t Alt);
void UsbHsMicAlt(uint8_t Alt);

void UsbAudioHsTimer1msProcess(void);

void OTG_HS_DeviceAudioModeAutoSwitchProcess(void);

//FS
bool OTG_FS_DeviceAudioSendPcCmd(uint16_t Cmd);
bool OTG_FS_DeviceAudioSendHidKeyDown(uint16_t Cmd);
bool OTG_FS_DeviceAudioSendHidKeyUp(void);

uint16_t UsbFSAudioMixSpeaker1DataGet(void *Buffer, uint16_t Len);
uint16_t UsbFSAudioMixSpeaker1DataLenGet(void);
uint16_t UsbFSAudioMixSpeaker1DepthGet(void);

uint16_t UsbFSAudioMixSpeakerDataGet(void *Buffer, uint16_t Len);
uint16_t UsbFSAudioMixSpeakerDataLenGet(void);
uint16_t UsbFSAudioMixSpeakerDepthGet(void);

uint16_t UsbFSAudioMixMicDataSet(void *Buffer, uint16_t Len);
uint16_t UsbFSAudioMixMicSpaceLenGet(void);
uint16_t UsbFSAudioMixMicDepthGet(void);

void OnDeviceFsAudioSendIsoPacket(void);
void OnDeviceFsAudioRcvIsoPacket(void);
void OnDeviceFsAudioRcvIsoPacket1(void);

void UsbFsMicSampleRateSet(void);
void UsbFsSpeakerSampleRateSet(void);
void UsbFsSpeaker1SampleRateSet(void);

void UsbFsMicVolumeSet(uint8_t channel);
void UsbFsSpeakerVolumeSet(uint8_t channel);
void UsbFsSpeaker1VolumeSet(uint8_t channel);

void UsbFsSpeakerAlt(uint8_t Alt);
void UsbFsSpeaker1Alt(uint8_t Alt);
void UsbFsMicAlt(uint8_t Alt);

void UsbAudioFsTimer1msProcess(void);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif
