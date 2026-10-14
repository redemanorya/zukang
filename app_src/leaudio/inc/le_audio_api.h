/*
 * le_audio_api.h
 *
 *  Created on: May 7, 2025
 *      Author: piwang
 */

#ifndef BTDM_HOST_LE_AUDIO_API_H_
#define BTDM_HOST_LE_AUDIO_API_H_
#include "type.h"

/*****************************************
 * 	api for application
 ***************************************/

typedef enum
{
	LE_CODEC_FORMAT_NONE = 0,
	LE_CODEC_FORMAT_SBC,
	LE_CODEC_FORMAT_LC3,
	LE_CODEC_FORMAT_LC3_2,
} LE_CODEC_FORMAT;

 /*********************************
 *FramePacketLen: Strans Payload Len
 *FrameEncodeLen: Encode out Len
 **********************************/
void LeAudioTypeSet(LE_CODEC_FORMAT type,uint16_t FramePacketLen,uint16_t FrameEncodeLen);

/*************************************
 * encoder PCM input fifo config
 *******************************/
void LeAudioEncPcmBufInit(void *buf,uint16_t pcm_fifo_len);

/************************************
 * Decoder PCM ouput Fifo config
 *************************************/
void LeAudioDecPcmBufInit(void *buf,uint16_t pcm_fifo_len);


void LeAudioDecPcmBufClear(void);

/************************************
 * encoder_str:Encoder handle
 *************************************/
void LeAudio_Encoder_init(void *encoder_str,uint16_t SampleRate);

/************************************
 * decoder_str: decoder handle
 * fifo:decoder fifo
 *************************************/
void LeAudio_Decoder_init(void *decoder_str,void *fifo,uint16_t SampleRate);

bool LeAudio_Encode(void);

bool LeAudio_Decode(void);

/****************************
 * channel £º1/2
 * FrameTime: Ö¡Ê±¼ä@us for LE_CODEC_FORMAT_LC3_2 / @0.1mS for LE_CODEC_FORMAT_LC3
 * BitRate:
 ***************************/
void Lc3EncodeFormatSet(uint8_t		Channel,
						uint16_t	FrameTime,
						uint32_t	BitRate);

void SbcEncodeFormatSet(uint8_t		SbcBlock,
						uint8_t 	SbcSubband,
						uint8_t 	Sbc_BITPOOL);

/*****************************
 * le audio send packect fifo, api config
 * set_data: set packet func
 * get_len: get space len func
 *****************************/
void LeAudioSrcDataApiRegister(void *set_data,void *get_len);

/*****************************
 * le audio packect receive fifo, api config
 * get_data: get packet func
 * get_len: get data len func
 *****************************/
void LeAudioSinkDataApiRegister(void *get_data,void *get_len,void *get_id);


void LeAudioSetDecode1ch(bool flag);

#endif /* BTDM_HOST_LE_AUDIO_API_H_ */
