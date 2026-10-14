/*****************************************************************************
 * @file    motor_api.c
 * @brief   auto generated                          
 * @author  castle (Automatically generated)        
 * @version V1.1.0                                  
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
#include "stdio.h"
#include <string.h>
#include <nds32_intrinsic.h>
#include "audio_effect_library.h"
#include "motor_wrap.h"


static const table_motor_dec g_table_motor_dec_0[] =
{
    {(AdPcmFileHeader *)IdleDataBuffer_0,           {IdleDataBuffer_0    + ADPCM_HDR, 0, 0, 0}},
    {(AdPcmFileHeader *)IdleDataBuffer_0,           {IdleDataBuffer_0    + ADPCM_HDR, 0, 0, 0}},
    {(AdPcmFileHeader *)Wav0DataBuffer_0,           {Wav0DataBuffer_0    + ADPCM_HDR, 0, 0, 0}},
    {(AdPcmFileHeader *)Wav1DataBuffer_0,           {Wav1DataBuffer_0    + ADPCM_HDR, 0, 0, 0}},
    {(AdPcmFileHeader *)Wav2DataBuffer_0,           {Wav2DataBuffer_0    + ADPCM_HDR, 0, 0, 0}},
    {(AdPcmFileHeader *)Wav3DataBuffer_0,           {Wav3DataBuffer_0    + ADPCM_HDR, 0, 0, 0}},
    {(AdPcmFileHeader *)Wav4DataBuffer_0,           {Wav4DataBuffer_0    + ADPCM_HDR, 0, 0, 0}},
};

static EngineSoundContextWrap *dec_main_0 = NULL;


int32_t motor_data_dec_callback_0(int32_t index, int32_t n, int16_t *pcm_buf)
{
    table_motor_dec *item;
    uint32_t rest_samples = n;
    uint32_t offset_rest, finished_samples;
    int16_t *output_offset = pcm_buf, *dec_buffer;
    uint8_t *block_data;
    
    if(dec_main_0 == NULL)
        return 0;
    
    dec_buffer = dec_main_0->dec_buffer;
    if(index < 0)//startup
    {
        item = &(dec_main_0->api.dec_table[0]);
        finished_samples = 0;
        while(rest_samples > 0)
        {
            if(item->info.BlocksOffset > 0)//any rest data?
            {
                offset_rest = item->hdr->SamplesPerBlock - item->info.BlocksOffset;
                block_data = (uint8_t*)(item->info.DataBase + item->info.Blocks * item->hdr->BytesPerBlock);
                adpcm_decode_block_mono(block_data, dec_buffer, item->hdr->SamplesPerBlock);
                
                if(rest_samples < offset_rest)//
                {
                    memcpy(output_offset, dec_buffer + item->info.BlocksOffset, rest_samples*2);
                    
                    item->info.BlocksOffset += rest_samples;//update offset
                    rest_samples = 0;//enough data, finished
                    
                    finished_samples += rest_samples;
                }
                else
                {
                    memcpy(output_offset, dec_buffer + item->info.BlocksOffset, offset_rest*2);
                    
                    rest_samples -= offset_rest;//not enough data, next loop
                    item->info.BlocksOffset = 0 ;//update offset
                    item->info.Blocks ++;//next blocks
                    output_offset += offset_rest;
                    
                    finished_samples += offset_rest;
                }
            }
            else//no rest data
            {
                if(item->info.Blocks < item->hdr->EncodedBlocks)//not at trailing
                {
                    block_data = (uint8_t*)(item->info.DataBase + item->info.Blocks * item->hdr->BytesPerBlock);
                    adpcm_decode_block_mono(block_data, dec_buffer, item->hdr->SamplesPerBlock);
                    
                    if(rest_samples < item->hdr->SamplesPerBlock)
                    {
                        memcpy(output_offset, dec_buffer, rest_samples*2);
                        item->info.BlocksOffset = rest_samples;
                        rest_samples = 0;//enough data, finished
                        
                        finished_samples += rest_samples;
                    }
                    else
                    {
                        memcpy(output_offset, dec_buffer, item->hdr->SamplesPerBlock*2);
                        rest_samples -= item->hdr->SamplesPerBlock;
                        item->info.Blocks ++;//next blocks
                        output_offset += item->hdr->SamplesPerBlock;
                        
                        finished_samples += item->hdr->SamplesPerBlock;
                    }
                }
                else if(item->hdr->TrailingPCMSamples > 0)//at trailing
                {
                    offset_rest = item->hdr->TrailingPCMSamples - item->info.TrailingOffset;
                    block_data = (uint8_t*)(item->info.DataBase + item->hdr->EncodedBlocks * item->hdr->BytesPerBlock + item->info.TrailingOffset * 2);
                    if(rest_samples < offset_rest)
                    {
                        memcpy(output_offset, block_data, rest_samples*2);
                        item->info.TrailingOffset += rest_samples;
                        
                        rest_samples = 0;//enough data, finished
                        
                        finished_samples += rest_samples;
                    }
                    else
                    {
                        memcpy(output_offset, block_data, offset_rest*2);
                        item->info.TrailingOffset = 0;//reset trailing offset
                        rest_samples -= offset_rest;
                        output_offset += offset_rest;
                        
                        item->info.Blocks = 0;//reset blocks index
                        
                        finished_samples += offset_rest;
                        
                        break;//finished, just break;
                    }
                }
                else//no trailing at all
                {
                    //reset all?
                    item->info.Blocks = 0;
                    item->info.BlocksOffset = 0;
                    item->info.TrailingOffset = 0;
                    
                    break;//finished, just break;
                }
            }
        }
        
        return finished_samples;
    }
    else
    {
        index ++;
        item = &(dec_main_0->api.dec_table[index]);
        while(rest_samples > 0)
        {
            if(item->info.BlocksOffset > 0)//any rest data?
            {
                offset_rest = item->hdr->SamplesPerBlock - item->info.BlocksOffset;
                block_data = (uint8_t*)(item->info.DataBase + item->info.Blocks * item->hdr->BytesPerBlock);
                adpcm_decode_block_mono(block_data, dec_buffer, item->hdr->SamplesPerBlock);
                
                if(rest_samples < offset_rest)// 
                {
                    memcpy(output_offset, dec_buffer + item->info.BlocksOffset, rest_samples*2);
                    
                    item->info.BlocksOffset += rest_samples;//update offset
                    rest_samples = 0;//enough data, finished
                }
                else
                {
                    memcpy(output_offset, dec_buffer + item->info.BlocksOffset, offset_rest*2);
                    
                    rest_samples -= offset_rest;//not enough data, next loop
                    item->info.BlocksOffset = 0 ;//update offset
                    item->info.Blocks ++;//next blocks
                    output_offset += offset_rest;
                }
            }
            else//no rest data
            {
                if(item->info.Blocks < item->hdr->EncodedBlocks)//not at trailing
                {
                    block_data = (uint8_t*)(item->info.DataBase + item->info.Blocks * item->hdr->BytesPerBlock);
                    adpcm_decode_block_mono(block_data, dec_buffer, item->hdr->SamplesPerBlock);
                    
                    if(rest_samples < item->hdr->SamplesPerBlock)
                    {
                        memcpy(output_offset, dec_buffer, rest_samples*2);
                        item->info.BlocksOffset = rest_samples;
                        rest_samples = 0;//enough data, finished
                    }
                    else
                    {
                        memcpy(output_offset, dec_buffer, item->hdr->SamplesPerBlock*2);
                        rest_samples -= item->hdr->SamplesPerBlock;
                        item->info.Blocks ++;//next blocks
                        output_offset += item->hdr->SamplesPerBlock;
                    }
                }
                else if(item->hdr->TrailingPCMSamples > 0)//at trailing
                {
                    // printf("B1");
                    offset_rest = item->hdr->TrailingPCMSamples - item->info.TrailingOffset;
                    block_data = (uint8_t*)(item->info.DataBase + item->hdr->EncodedBlocks * item->hdr->BytesPerBlock + item->info.TrailingOffset * 2);
                    if(rest_samples < offset_rest)
                    {
                        memcpy(output_offset, block_data, rest_samples*2);
                        item->info.TrailingOffset += rest_samples;
                        
                        rest_samples = 0;//enough data, finished
                    }
                    else
                    {
                        memcpy(output_offset, block_data, offset_rest*2);
                        item->info.TrailingOffset = 0;//reset trailing offset
                        rest_samples -= offset_rest;
                        output_offset += offset_rest;
                        
                        item->info.Blocks = 0;//reset blocks index
                    }
                }
                else//no trailing at all
                {
                    // printf("C1");
                    //reset all?
                    item->info.Blocks = 0;
                    item->info.BlocksOffset = 0;
                    item->info.TrailingOffset = 0;
                    
                    continue;
                }
            }
        }
        
        return n;
    }
}

int32_t motor_mm_init_dec_0(void *memory, bool init)
{
    //the decoder's context memory is allocated by the roboeffect environment
    dec_main_0 = memory;
    
    if(init)
    {
        //initialize decoder's context memory
        memcpy(dec_main_0->api.dec_table, g_table_motor_dec_0, sizeof(g_table_motor_dec_0));
    }
    
    return 1;
}

static const table_motor g_table_motor_1[] =
{
    {IdleDataBuffer_1     + WAV_HDR,       81692 - WAV_HDR,  IdleDataBuffer_1       + WAV_HDR},
    {IdleDataBuffer_1     + WAV_HDR,       81692 - WAV_HDR,  IdleDataBuffer_1       + WAV_HDR},
    {Wav0DataBuffer_1     + WAV_HDR,       96656 - WAV_HDR,  Wav0DataBuffer_1       + WAV_HDR},
    {Wav1DataBuffer_1     + WAV_HDR,       95874 - WAV_HDR,  Wav1DataBuffer_1       + WAV_HDR},
    {Wav2DataBuffer_1     + WAV_HDR,       95806 - WAV_HDR,  Wav2DataBuffer_1       + WAV_HDR},
    {Wav3DataBuffer_1     + WAV_HDR,       96802 - WAV_HDR,  Wav3DataBuffer_1       + WAV_HDR},
    {Wav4DataBuffer_1     + WAV_HDR,       95766 - WAV_HDR,  Wav4DataBuffer_1       + WAV_HDR},
};

static EngineSoundContextWrap *dec_main_1 = NULL;

int32_t motor_data_callback_1(int32_t index, int32_t n, int16_t *pcm_buf)
{
    table_motor *item;
    int16_t *cur_ptr;
    int i;
    
    if(dec_main_1 == NULL)
        return 0;
    
    if(index < 0)
    {
        item = &(dec_main_1->api.raw_table[0]);
        if(item->ptr + n*2 > item->data + item->len)
        {
            int32_t once = item->len - (uint32_t)(item->ptr - item->data);
            memcpy(pcm_buf, item->ptr, once);
            item->ptr = item->data;//restart to head;
            n = once/2; //samples
        }
        else
        {
            memcpy(pcm_buf, item->ptr, n*2);
            item->ptr += n*2;
        }
        return n;
    }
    else
    {
        index++;
        item = &(dec_main_1->api.raw_table[index]);
        if(item->ptr + n*2 > item->data + item->len)
        {
            int32_t once = item->len - (uint32_t)(item->ptr - item->data);
            
            memcpy(pcm_buf, item->ptr, once);
            item->ptr = item->data;
            memcpy(pcm_buf + once/2, item->ptr, n*2 - once);
            item->ptr += n*2 - once;
        }
        else
        {
            memcpy(pcm_buf, item->ptr, n*2);
            item->ptr += n*2;
        }
        return n;
    }
}

int32_t motor_mm_init_1(void *memory, bool init)
{
    //the decoder's context memory is allocated by the roboeffect environment
    dec_main_1 = memory;
    
    if(init)
    {
        //initialize decoder's context memory
        memcpy(dec_main_1->api.raw_table, g_table_motor_1, sizeof(g_table_motor_1));
    }
    
    return 1;
}
