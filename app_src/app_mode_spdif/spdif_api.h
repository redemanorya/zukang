#ifndef __SPDIF_API_H__
#define __SPDIF_API_H__

#include "type.h"
#include "mcu_circular_buf.h"
#include "spdif.h"
#include "app_config.h"

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

#define SPDIF_SOURCE_NUM				APP_SOURCE_NUM

//#define SPDIF_DPLL_LOCK_MODE
#define PCM_REMAIN_SMAPLES				3//spdif数据丢声道时,返回数据会超过一帧128，用于缓存数据，试听感更加好
//spdif单个采样点8字节
//recv, dma buf len,MAX_FRAME_SAMPLES * 2 * 2 * 2 * 2是基础，OS切换间隔实测需要加倍。
#define	SPDIF_FIFO_LEN					(MAX_FRAME_SAMPLES * sizeof(PCM_DATA_TYPE) * 2 * 2 * 2 * 2)
//由于采样值从32bit转换为16bit，可以使用同一个buf，否则要独立申请
#define SPDIF_CARRY_LEN					(MAX_FRAME_SAMPLES * sizeof(PCM_DATA_TYPE) * 2 * 2 + PCM_REMAIN_SMAPLES * 2 * 4)//支持192000输入 buf len for get data form dma fifo, deal + 6samples 用于缓存偶尔多余的数据，#9817
#define SPDIF_CARRY_LEN_24				(SPDIF_CARRY_LEN + 300)	//192000输入的时候，buf会增大100-300个字节，经验值需要实测
//spdif data carry step len from DMA to circular buf
#define SPDIF_CARRY_FRAME_SAMPLES 32

typedef struct _SpdifPlayContext
{
	uint32_t			*SpdifPwcFIFO;		//
	uint32_t            *SpdifCarry;
	uint32_t			*SpdifPcmFifo;
	MCU_CIRCULAR_CONTEXT SpdifPcmCircularBuf;
#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
	uint32_t            *SpdifCarry24;
#endif
	//play
	uint32_t 			SampleRate;

	bool 				SpdifLockFlag;
#ifdef SPDIF_DPLL_LOCK_MODE
	bool 				SpdifDpllLockFlag;
	uint32_t 			dpll_lock_cnt;
#endif
	SPDIF_TYPE_STR		AudioInfo;
	uint16_t			SpdifMuteCnt;
}SpdifPlayContext;

extern SpdifPlayContext* SpdifPlayCt;

uint16_t Spdif_Rx_DataLenGet(void);
uint16_t Spdif_Rx_DataGet(void *pcm_out, uint16_t MaxPoint);
void SpdifDataCarry(uint8_t spdif_dma_ch);

#ifdef SPDIF_DPLL_LOCK_MODE
void DpllLockReInit(void);
#endif

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __SPDIF_API_H__

