/*****************************************************************************
 * @file    motor_wrap.h
 * @brief   auto generated                          
 * @author  castle (Automatically generated)        
 * @version V1.3.0                                  
 * @Created 2023-04-13T13:24:02 
 ***************************************************************************** * @attention
 *
 * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
 * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
 * TIME. AS A RESULT, MVSILICON SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
 * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
 * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
 * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 * <h2><center>&copy; COPYRIGHT 2023 MVSilicon </center></h2>
 *****************************************************************************/

#ifndef __MOTOR_WRAP_H__
#define __MOTOR_WRAP_H__

#include "stdio.h"
#include "type.h"
#include <nds32_intrinsic.h>
#include "audio_effect_library.h"

#define  MOTOR_ITEM_RAW_MAX     ( 6)
#define  MOTOR_ITEM_DEC_MAX     ( 6)

#define  MOTOR_SAMPLE_RATE      (24000)

#define  MOTOR_RPM_MIN_0        (  700)
#define  MOTOR_RPM_MAX_0        ( 5000)
#define  MOTOR_HAS_STARUP_0     (    0)

#define  MOTOR_RPM_MIN_1        (  700)
#define  MOTOR_RPM_MAX_1        ( 5000)
#define  MOTOR_HAS_STARUP_1     (    0)

typedef int32_t (*motor_data_cb)(int32_t index, int32_t n, int16_t *pcm_buf);
typedef int32_t (*motor_mm_init_cb)(void *memory, bool init);

typedef struct _WavFileHeader_moto
{
    uint8_t    chunk_id[4];		//  0 - ChunkID: "RIFF"
    uint32_t   chunk_size;		    //  4 - ChunkSize: (filesize-8)
    uint8_t    format[4];			//  8 - Format: "WAVE"
    uint8_t    subchunk1_id[4];	// 12 - SubChunk1ID: "fmt "
    uint32_t   subchunk1_size;	    // 16 - SubChunk1Size: 16 for PCM (18 o 40 for others)
    uint16_t   audio_format;		// 20 - AudioFormat: (1=Uncompressed)
    uint16_t   num_channels;		// 22 - NumChannels: 1 or 2
    uint32_t   sample_rate;		// 24 - SampleRate in Hz
    uint32_t   byte_rate;			// 28 - Byte Rate (SampleRate*NumChannels*(BitsPerSample/8)
    uint16_t   block_align;		// 32 - BlockAlign (== NumChannels * BitsPerSample/8)
    uint16_t   bits_per_sample;	// 34 - BitsPerSample: 16 bit
    uint8_t    subchunk2_id[4];	// 36 - Subchunk2ID: "data"
    uint32_t   subchunk2_size;	    // 40 - Subchunk2Size: BitsPerSample/8*NumChannels*NumSamples;
} WavFileHeader_moto;
#define WAV_HDR sizeof(WavFileHeader_moto)

typedef struct _AdPcmFileHeader
{
    uint8_t    chunk_id[4];		//  0 - ChunkID: "RIFF"
    uint32_t   SampleRate;
    uint32_t   Channels;
    uint32_t   SamplesPerBlock;
    uint32_t   BytesPerBlock;
    uint32_t   EncodedBlocks;
    uint32_t   TrailingPCMSamples;
} AdPcmFileHeader;
#define ADPCM_HDR sizeof(AdPcmFileHeader)

typedef struct _DecStackInfo
{
    const uint8_t    *DataBase;
    uint32_t   Blocks;		    //current block number
    uint32_t   BlocksOffset;   //offset(samples) in current block
    uint32_t   TrailingOffset; //offset(samples) in trailing data
} DecStackInfo;

typedef struct _table_motor
{
    const uint8_t *ptr;
    int32_t len;
    const uint8_t *data;
} table_motor;

typedef struct _table_motor_dec
{
    AdPcmFileHeader *hdr;
    DecStackInfo info;
} table_motor_dec;

// typedef struct _motor_dec_main
// {
//     int16_t dec_buffer[256];
//     int16_t table_size;
//     table_motor_dec *table;
// } motor_dec_main;

typedef struct _EngineSoundContextWrap
{
    EngineSoundContext esc;

    int16_t dec_buffer[64];

    union {
        table_motor raw_table[MOTOR_ITEM_RAW_MAX];
        table_motor_dec dec_table[MOTOR_ITEM_DEC_MAX];
     } api;
} EngineSoundContextWrap;

extern const uint8_t IdleDataBuffer_0[22435];
extern const uint8_t Wav0DataBuffer_0[26518];
extern const uint8_t Wav1DataBuffer_0[26294];
extern const uint8_t Wav2DataBuffer_0[26226];
extern const uint8_t Wav3DataBuffer_0[26571];
extern const uint8_t Wav4DataBuffer_0[26279];


extern const uint8_t IdleDataBuffer_1[81692];
extern const uint8_t Wav0DataBuffer_1[96656];
extern const uint8_t Wav1DataBuffer_1[95874];
extern const uint8_t Wav2DataBuffer_1[95806];
extern const uint8_t Wav3DataBuffer_1[96802];
extern const uint8_t Wav4DataBuffer_1[95766];


int32_t motor_data_dec_callback_0(int32_t index, int32_t n, int16_t *pcm_buf);
int32_t motor_mm_init_dec_0(void *memory, bool init);

int32_t motor_data_callback_1(int32_t index, int32_t n, int16_t *pcm_buf);
int32_t motor_mm_init_1(void *memory, bool init);

bool motor_dispatch_func(int index, motor_data_cb *data_cb, int32_t *rpm_min, int32_t *rpm_max, int16_t *has_startup, motor_mm_init_cb *mm_init_cb);

int32_t engine_sound_wrap_init(EngineSoundContextWrap *ct, int32_t sample_rate, int32_t n, int32_t index);

int32_t engine_sound_wrap_configure(EngineSoundContextWrap *ct, int32_t rpm);

int32_t engine_sound_wrap_apply(EngineSoundContextWrap *ct, int16_t *pcm_out);

#endif //__MOTOR_WRAP_H__

