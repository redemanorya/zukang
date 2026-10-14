/**
 **************************************************************************************
 * @file    Spdif_mode.c
 * @brief
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2017-12-26 13:06:47$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
 
#include <spdif_api.h>
#include <string.h>
#include "type.h"
#include "irqn.h"
#include "gpio.h"
#include "dma.h"
#include "rtos_api.h"
#include "app_message.h"
#include "app_config.h"
#include "debug.h"
#include "delay.h"
#include "audio_adc.h"
#include "dac.h"
#include "adc_interface.h"
#include "spdif.h"
#include "dac_interface.h"
#include "clk.h"
#include "audio_core_api.h"
#include "audio_core_service.h"
#include "audio_effect.h"
#include "powercontroller.h"
//#include "deepsleep.h"
#include "resampler_polyphase.h"
#include "mcu_circular_buf.h"
#include "breakpoint.h"
#include "main_task.h"
#include "remind_sound_item.h"
#include "mode_task_api.h"
#include "remind_sound.h"
#include "audio_vol.h"
#include "ctrlvars.h"
#include "reset.h"

#if defined(CFG_APP_OPTICAL_MODE_EN) || defined(CFG_APP_COAXIAL_MODE_EN)

SpdifPlayContext* SpdifPlayCt = NULL;

//sample为单位
uint16_t Spdif_Rx_DataLenGet(void)
{
	return MCUCircular_GetDataLen(&SpdifPlayCt->SpdifPcmCircularBuf) / (sizeof(PCM_DATA_TYPE) * 2);
}

//sample为单位，buf大小：8 * MaxSize
uint16_t Spdif_Rx_DataGet(void *pcm_out, uint16_t MaxPoint)
{
	return MCUCircular_GetData(&SpdifPlayCt->SpdifPcmCircularBuf, pcm_out, MaxPoint * (sizeof(PCM_DATA_TYPE) * 2)) / (sizeof(PCM_DATA_TYPE) * 2);
}

void SpdifDataCarry(uint8_t spdif_dma_ch)
{
	int16_t pcm_space;
	uint16_t spdif_len;
	int16_t pcm_len;
	int16_t *pcmBuf  = (int16_t *)SpdifPlayCt->SpdifCarry;
	uint16_t cnt;

	spdif_len = DMA_CircularDataLenGet(spdif_dma_ch);
	pcm_space = MCUCircular_GetSpaceLen(&SpdifPlayCt->SpdifPcmCircularBuf) - (sizeof(PCM_DATA_TYPE) * 8);

	if(pcm_space < (sizeof(PCM_DATA_TYPE) * 8))
	{
		DBG("err\n");
		return;
	}

	if((spdif_len >> 1) > pcm_space)
	{
		spdif_len = pcm_space * 2;
	}

	spdif_len = spdif_len & 0xFFF8;
	if(!spdif_len)
	{
		return ;
	}

	cnt = (spdif_len / (sizeof(PCM_DATA_TYPE) * 4)) / (SPDIF_CARRY_FRAME_SAMPLES);

	while(cnt--)
	{
		pcm_len = DMA_CircularDataGet(spdif_dma_ch, pcmBuf, SPDIF_CARRY_FRAME_SAMPLES * (sizeof(PCM_DATA_TYPE) * 4));
#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
//		pcm_len = SPDIF_SPDIFDataToPCMData(CFG_SPDIF_MODULE,(int32_t *)pcmBuf, pcm_len, (int32_t *)SpdifPlayCt->SpdifCarry24, SPDIF_WORDLTH_24BIT);
		SPDIF_SPDIFDatatoAudioData((int32_t *)pcmBuf, pcm_len, (int32_t *)SpdifPlayCt->SpdifCarry24, SPDIF_WORDLTH_24BIT, &SpdifPlayCt->AudioInfo);
		pcm_len = SpdifPlayCt->AudioInfo.output_length;
		//printf("pcm_len = %d\n",pcm_len);
		if(SpdifPlayCt->AudioInfo.audio_type != SPDIF_AUDIO_PCM_DATA_TYPE)
		{
			return;
		}
		if(pcm_len < 0)
		{
			return;
		}
		MCUCircular_PutData(&SpdifPlayCt->SpdifPcmCircularBuf, SpdifPlayCt->SpdifCarry24, pcm_len);
#else
		//由于从32bit转换为16bit，buf可以使用同一个，否则要独立申请。
		SPDIF_SPDIFDatatoAudioData((int32_t *)pcmBuf, pcm_len, (int32_t *)pcmBuf, SPDIF_WORDLTH_16BIT, &SpdifPlayCt->AudioInfo);
		pcm_len = SpdifPlayCt->AudioInfo.output_length;
		//printf("pcm_len = %d\n",pcm_len);
		if(SpdifPlayCt->AudioInfo.audio_type != SPDIF_AUDIO_PCM_DATA_TYPE)
		{
			return;
		}
		if(pcm_len < 0)
		{
			return;
		}
		MCUCircular_PutData(&SpdifPlayCt->SpdifPcmCircularBuf, pcmBuf, pcm_len);
#endif
		SpdifPlayCt->SpdifMuteCnt = (SpdifPlayCt->SpdifMuteCnt > 1 ? (SpdifPlayCt->SpdifMuteCnt-1):0);
	}
}

#ifdef SPDIF_DPLL_LOCK_MODE
void DpllLockReInit(void)
{
	SPDIF_DpllLockDeInit();
	SPDIF_DpllLockInit();//dpll lock mode
	SpdifPlayCt->SpdifDpllLockFlag = FALSE;
	SpdifPlayCt->SpdifLockFlag = FALSE;
}
#endif


#endif 

