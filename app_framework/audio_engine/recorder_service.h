/**
 **************************************************************************************
 * @file    recorder_service.h
 * @brief    
 *
 * @author  pi
 * @version V1.0.0
 *
 * $Created: 2018-04-28 11:40:00$
 *
 * @Copyright (C) 2018, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __RECORDER_SERVICE_H__
#define __RECORDER_SERVICE_H__


#ifdef __cplusplus
extern "C"{
#endif // __cplusplus 

#include "type.h"

typedef struct _WavFileHeader
{
	uint8_t	chunk_id[4];		//  0 - ChunkID: "RIFF"
	uint32_t chunk_size;		//  4 - ChunkSize: (filesize-8)
	uint8_t format[4];			//  8 - Format: "WAVE"
	uint8_t subchunk1_id[4];	// 12 - SubChunk1ID: "fmt "
	uint32_t subchunk1_size;	// 16 - SubChunk1Size: 16 for PCM (18 o 40 for others)
	uint16_t audio_format;		// 20 - AudioFormat: (1=Uncompressed)
	uint16_t num_channels;		// 22 - NumChannels: 1 or 2
	uint32_t sample_rate;		// 24 - SampleRate in Hz
	uint32_t byte_rate;			// 28 - Byte Rate (SampleRate*NumChannels*(BitsPerSample/8)
	uint16_t block_align;		// 32 - BlockAlign (== NumChannels * BitsPerSample/8)
	uint16_t bits_per_sample;	// 34 - BitsPerSample: 16 bit
	uint8_t subchunk2_id[4];	// 36 - Subchunk2ID: "data"
	uint32_t subchunk2_size;	// 40 - Subchunk2Size: BitsPerSample/8*NumChannels*NumSamples;
} WavFileHeader;
// the critical information of a WAV file
typedef struct _WavInfo
{
	uint32_t sample_rate;		// sample rate in Hz
	uint32_t num_samples;		// total number of samples contained in WAV file
	uint16_t num_channels;		// number of channels
	uint16_t bits_per_sample;	// bits per sample (8/16/24/32)
} WavInfo_t;
#define	ENCODER_WAV_SAMPLE_SIZE		512

#ifdef	CFG_FUNC_RECORD_SD_UDISK
#define FILE_PATH_LEN							(strlen(MEDIA_VOLUME_STR_C)  + strlen(CFG_PARA_RECORDS_FOLDER) + 1 + 8 + 4 + 1 )//vol:3 + folder/:4 + name:8 + .ext:4
#define	FILE_INDEX_MAX							CFG_PARA_REC_MAX_FILE_NUM //录音文件最多这个数
#define FILE_NAME_MAX							65535//(1~65535) //录音文件名编号范围。根据录音先后顺序命令。
#define FILE_NAME_VALUE_SIZE					(sizeof(uint16_t))
#endif
#define MEDIA_ENCODER_SAMPLE_MAX			2000	//保护

#ifndef MEDIA_RECORDER_CHANNEL
#define MEDIA_RECORDER_CHANNEL				2
#endif
#ifndef MEDIA_RECORDER_BITRATE
#define MEDIA_RECORDER_BITRATE				96
#endif
#ifndef FILE_WRITE_FIFO_LEN
#define FILE_WRITE_FIFO_LEN					(512 * 15)//保障阻塞时 编码数据 缓冲。适应U盘、卡兼容性。
#endif

uint16_t RecFileIndex(char *string);
void IntToStrMP3Name(char *string, uint16_t number);
void RecServierMsg(uint16_t msg);
xTaskHandle GetMediaRecorderTaskHandle(void);
bool MediaRecorderServiceDeinit(void);

#ifdef CFG_FUNC_RECORD_EXTERN_FLASH_EN
typedef struct {
	uint32_t RecSize;
	uint16_t RecCrc;
}REC_EXTFLASH_HEAD;
bool IsRecorderRunning(void);
void ExFlashRecorderStartIndex(MessageHandle parentMsgHandle,uint8_t index);
void RemindServiceItemRequestExt(uint8_t index);
#endif
#ifdef __cplusplus
}
#endif // __cplusplus 

#endif // __RECORDER_SERVICE_H__
