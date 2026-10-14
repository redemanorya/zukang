/**
 **************************************************************************************
 * @file    bt_play_api.c
 * @brief   
 *
 * @author  kk
 * @version V1.0.0
 *
 * $Created: 2017-3-17 13:06:47$
 * 
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
//driver、lib
#include "string.h"
#include "type.h"
#include "irqn.h"

//有关蓝牙A2DP播放的相关处理函数放置此文件
//application
#include "app_config.h"
#include "rtos_api.h"
#include "bt_play_api.h"
#include "audio_core_api.h"
#include "audio_vol.h"
#include "main_task.h"
#include "bt_manager.h"

#ifdef CFG_APP_BT_MODE_EN

#define OS_SBC_LOCK	 	if(SbcDecoderMutex != NULL){osMutexLock(SbcDecoderMutex);}
#define OS_SBC_UNLOCK	if(SbcDecoderMutex != NULL){osMutexUnlock(SbcDecoderMutex);}

uint32_t gBtPlaySbcDecoderInitFlag = 0;
void BtPlayMonitor(bool AudioOk);
extern BT_A2DP_PLAYER *a2dp_player;
osMutexId SbcDecoderMutex = NULL;


#ifdef BT_AUDIO_AAC_ENABLE
#include "app_message.h"
#include "bt_interface.h"
#include "bt_manager.h"
#include "audio_decoder_api.h"

extern void BtDecoderDeinit(void);
extern int32_t BtDecoderInit(void *io_handle,int32_t decoder_type);
extern uint16_t BtDecodedPcmDataGet(void * pcmData, uint16_t sampleLen);
extern uint16_t BtDecodedPcmDataLenGet(void);


uint32_t a2dp_unmute_delay_cnt = 0;
uint8_t  a2dp_suspend_flag = 0;
uint16_t A2DPDataLenGet(void);
void set_a2dp_stream_suspend(void)
{
	if(A2DPDataLenGet() > 0)
	{
		AudioCoreSourceMute(APP_SOURCE_NUM, TRUE, TRUE);
		a2dp_suspend_flag = 1;
		BtDecoderDeinit();
	}
}

void a2dp_stream_suspend_play_end(void)
{
	if(!a2dp_suspend_flag)
		return;

#if (BT_TWS_SUPPORT == ENABLE)
	if(IsBtAudioMode())
	{
		AudioCoreSourceDisable(APP_SOURCE_NUM);
	}
#endif
	if(GetA2dpState(BtCurIndex_Get()) == BT_A2DP_STATE_STREAMING)
		SetA2dpState(BtCurIndex_Get(),BT_A2DP_STATE_CONNECTED);
	//SetA2dpState(BT_A2DP_STATE_CONNECTED);
	BtMidMessageSend(MSG_BT_MID_PLAY_STATE_CHANGE, 2);

	AudioCoreSourceUnmute(APP_SOURCE_NUM, TRUE, TRUE);
	BtDecoderDeinit();

	a2dp_suspend_flag = 0;
}

void a2dp_sbc_decoder_init(void)
{
	if(a2dp_player == NULL)
	{
		return;
	}
	OS_SBC_LOCK;
	int gie_ret = GIE_STATE_GET();
	GIE_DISABLE();
	AudioCoreSourceDisable(APP_SOURCE_NUM);
	AudioCoreSourceMute(APP_SOURCE_NUM, TRUE, TRUE);
	a2dp_unmute_delay_cnt = 0;
	memset(a2dp_player,0,sizeof(BT_A2DP_PLAYER));
	a2dp_player->sbc_init_flag = 1;

	a2dp_player->MemHandle.addr = a2dp_player->sbc_fifo;
	a2dp_player->MemHandle.mem_capacity = sizeof(a2dp_player->sbc_fifo);
	a2dp_player->MemHandle.mem_len = 0;
	a2dp_player->MemHandle.p = 0;

	if(gie_ret)
	{
		GIE_ENABLE();
	}
	OS_SBC_UNLOCK;

}

void a2dp_sbc_save(uint8_t *p,uint32_t len)
{
	if(!IsBtAudioMode())
		return;

	if(a2dp_player == NULL || p == NULL)
	{
		return;
	}

	if(a2dp_player->sbc_init_flag == 0)
		return ;

	if(btManager.cur_index >= BT_LINK_DEV_NUM)
	{
		return;
	}
	
	uint8_t index = btManager.btLinked_env[btManager.cur_index].a2dp_index;

	if(a2dp_unmute_delay_cnt < 20)
	{
		a2dp_unmute_delay_cnt++;
		if((a2dp_unmute_delay_cnt == 20)
			|| ( (GetBtManager()->a2dpStreamType[index] == BT_A2DP_STREAM_TYPE_SBC)&& (GetValidSbcDataSize() >= SBC_FIFO_LEVEL_HIGH) )
			|| ( (GetBtManager()->a2dpStreamType[index] == BT_A2DP_STREAM_TYPE_AAC)&& (btManager.aacFrameNumber >= BT_AAC_START_FRAME) ))
		{
			if(IsAudioPlayerMute() == TRUE)
			{
				HardWareMuteOrUnMute();
			}
			AudioCoreSourceUnmute(APP_SOURCE_NUM,TRUE,TRUE);
			a2dp_unmute_delay_cnt = 20;
		}
		//return;
	}

	OS_SBC_LOCK;
	if(mv_mremain(&a2dp_player->MemHandle) > len)
	{
		mv_mwrite(p, len, 1,&a2dp_player->MemHandle);
		if(GetBtManager()->a2dpStreamType[index] == BT_A2DP_STREAM_TYPE_AAC)
		{
			if(btManager.aacFrameNumber<0xffffffff)
				btManager.aacFrameNumber++;
		}
		
#ifdef CFG_DUMP_DEBUG_EN
		dumpUartSend(p, len);
#endif

	}

	if(( (GetBtManager()->a2dpStreamType[index] == BT_A2DP_STREAM_TYPE_SBC)&& (GetValidSbcDataSize() >= SBC_FIFO_LEVEL_HIGH) )
		|| ( (GetBtManager()->a2dpStreamType[index] == BT_A2DP_STREAM_TYPE_AAC)&& (btManager.aacFrameNumber >= BT_AAC_START_FRAME) ))
	{
		if(!AudioCoreSourceIsEnable(APP_SOURCE_NUM))
		{
			uint8_t sIndex = btManager.btLinked_env[btManager.cur_index].a2dp_index;
			if(sIndex < BT_LINK_DEV_NUM && btManager.a2dpStreamType[sIndex] == BT_A2DP_STREAM_TYPE_AAC)
			{
				BtDecoderInit(&a2dp_player->MemHandle,AAC_DECODER);
				BtPlay_FreqAdjustLevelSet(BT_AAC_LEVEL_LOW, BT_AAC_LEVEL_HIGH);
			}
			else
			{
				BtDecoderInit(&a2dp_player->MemHandle,SBC_DECODER);
				BtPlay_FreqAdjustLevelSet(BT_SBC_LEVEL_LOW, BT_SBC_LEVEL_HIGH);
			}
			AudioCoreSourceEnable(APP_SOURCE_NUM);
#ifdef CFG_PARA_BT_SYNC
			AudioCoreSourceAdjust(APP_SOURCE_NUM, TRUE);
#endif
			gBtPlaySbcDecoderInitFlag = 1;
		}
	}
	OS_SBC_UNLOCK;
}

uint32_t GetValidSbcDataSize(void)
{
	uint32_t	dataSize = 0;

	if(a2dp_player == NULL)
	{
		return 0;
	}

	dataSize =  mv_msize(&a2dp_player->MemHandle);

	return dataSize;
}

uint32_t GetValidFrameDataSize(void)
{
	uint32_t	dataSize = 0;
	extern uint8_t GetBtDecoderFlag(void);
	if(GetBtDecoderFlag())
	{
		dataSize = btManager.aacFrameNumber;
	}
	return dataSize;
}

uint16_t A2DPDataLenGet(void)
{
	uint16_t Len = 0;

	if(a2dp_player == NULL)
	{
		return 0;
	}
	if(a2dp_player->sbc_init_flag == 0)
		return 0;
	OS_SBC_LOCK;
	Len = BtDecodedPcmDataLenGet();
	OS_SBC_UNLOCK;
	if(Len == 0)
	{
		a2dp_stream_suspend_play_end();
	}
	return Len;
}

uint16_t A2DPDataGet(void* Buf, uint16_t Samples)
{
	uint16_t Len = 0;

	if(a2dp_player == NULL || Buf == NULL || !AudioCoreSourceIsEnable(APP_SOURCE_NUM))
	{
		return 0;
	}
	/* 与 SBC 路径一致，限制单次请求样本数，避免 Samples*4 等异常大缓冲 */
	if((a2dp_player->sbc_init_flag == 0) || (Samples > (CFG_MAX_FRAME*2)))
	{
		BtPlayMonitor(FALSE);
		return 0;
	}

	OS_SBC_LOCK;
	Len = BtDecodedPcmDataGet(Buf,Samples);
	OS_SBC_UNLOCK;
	BtPlayMonitor(Len >= Samples);
	return Len;
}

#else
extern int32_t sbc_decoder_init(SBCFrameDecoderContext *ct);
extern int sbc_get_frame_infor(uint8_t *data,uint32_t *fram_size,uint32_t *frequency,uint32_t *data_out_fram_size);
extern int32_t sbc_decoder_apply_phone(SBCFrameDecoderContext *ct,uint8_t *sbc_buf,uint8_t sbc_size,int16_t *pcm_buf);

static uint32_t gA2dpFifoFullCnt = 0;	//统计fifo满的次数,用于调试

/***********************************************************************************
 * a2dp sbc decoder 初始化函数
 **********************************************************************************/
uint32_t a2dp_unmute_delay_cnt = 0;
void a2dp_sbc_decoder_init(void)
{

	if(a2dp_player == NULL)
	{
		return;
	}
	OS_SBC_LOCK;
	int gie_ret = GIE_STATE_GET();
	GIE_DISABLE();
	memset(a2dp_player,0,sizeof(BT_A2DP_PLAYER));
	MCUCircular_Config(&a2dp_player->sbc_fifo_cnt,a2dp_player->sbc_fifo,sizeof(a2dp_player->sbc_fifo));
	sbc_decoder_init(&a2dp_player->sbc_dec_handle);

	a2dp_player->sbc_init_flag = 1;
	gBtPlaySbcDecoderInitFlag = 0;
	AudioCoreSourceMute(APP_SOURCE_NUM,TRUE,TRUE);
	if(gie_ret)
	{
		GIE_ENABLE();
	}
	OS_SBC_UNLOCK;
	a2dp_unmute_delay_cnt = 0;
}


/***********************************************************************************
 * a2dp sbc 数据包检测流程
 **********************************************************************************/
static int16_t sbc_frame_check(uint8_t *data,uint32_t len)
{
	uint32_t v_offset = 0;
	uint32_t v_frame_bytes = 0, v_freq = 0, v_out = 0;

	// 在校验循环前加
	if(len < 4)  // SBC 最小帧头 4 字节
	{
		APP_DBG("==== a2dp sbc pkt too short: %lu\n", len);
		return -1;
	}
	
	/* SBC 整包多帧校验：
	 * 1. 逐帧检查同步字是否为 0x9C
	 * 2. 调用 sbc_get_frame_infor 获取当前帧长，偏移至下一帧
	 * 3. 所有帧字节累加后须精确等于 len，否则丢弃本包
	 */
	while(v_offset < len)
	{
		if(data[v_offset] != 0x9C)			//同步字校验
		{
			APP_DBG("==== a2dp sbc frame[%lu] sync err: 0x%02x, pkt_len=%lu\n",
				v_offset, data[v_offset], len);
			return v_offset;
		}

		if(v_offset + 4 > len)			//边界保护：剩余不足最小帧头，丢弃当前坏帧及后续
        	return v_offset;

		if(sbc_get_frame_infor(data + v_offset, &v_frame_bytes, &v_freq, &v_out) != 0
			|| v_frame_bytes == 0)		//帧信息解析
		{
			APP_DBG("==== a2dp sbc frame[%lu] infor err, pkt_len=%lu\n",
				v_offset, len);
			return v_offset;
		}

		if(v_offset + v_frame_bytes > len)
		{
			APP_DBG("==== a2dp sbc frame[%lu] overflow: frame_len=%lu pkt_len=%lu\n",
				v_offset, v_frame_bytes, len);
			return v_offset;
		}
		v_offset += v_frame_bytes;
	}

	if(v_offset != len)
	{
		APP_DBG("==== a2dp sbc frame len mismatch: frames_total=%lu pkt_len=%lu\n",
			v_offset, len);
		return -1;
	}

	//校验合格
	return len;
}

/***********************************************************************************
 * a2dp sbc 接收数据保存处理函数
 **********************************************************************************/
void a2dp_sbc_save(uint8_t *p,uint32_t len)
{
	int info_ok = 0;
	int play_start = 0;
	int ret = 0;
	uint8_t errorFlag = 0;
	int16_t validLen = 0;
	
	if(!IsBtAudioMode())
		return;
		
	if(a2dp_player == NULL || p == NULL)
	{
		return;
	}

	//sbc frame check
	validLen = sbc_frame_check(p, len);
	if(validLen < len)
	{
		APP_DBG("==== a2dp recv sbc tail dropped: valid=%d, pkt_len=%lu\n", validLen, len);
		return;
	}

	// if(p[0] != 0x9C)
	// {
	// 	printf("==== a2dp recv sbc,len=%lu, error: 0x%02x\n", len, p[0]);
	// 	return;
	// }

	if(a2dp_unmute_delay_cnt < 20)
	{
		a2dp_unmute_delay_cnt++;
		if(a2dp_unmute_delay_cnt == 20 || MCUCircular_GetDataLen(&a2dp_player->sbc_fifo_cnt) >= SBC_FIFO_LEVEL_HIGH)
		{
			if(IsAudioPlayerMute() == TRUE)
			{
				HardWareMuteOrUnMute();
			}
			AudioCoreSourceUnmute(APP_SOURCE_NUM,TRUE,TRUE);
			a2dp_unmute_delay_cnt = 20;
		}
		//return;
	}
	OS_SBC_LOCK;
	if(gBtPlaySbcDecoderInitFlag == 0)
	{
		if(MCUCircular_GetDataLen(&a2dp_player->sbc_fifo_cnt) >= SBC_FIFO_LEVEL_HIGH)
		{
			//printf("sbc start decoder\n");
			gBtPlaySbcDecoderInitFlag = 1;
			if(!AudioCoreSourceIsEnable(APP_SOURCE_NUM))
			{
				AudioCoreSourceEnable(APP_SOURCE_NUM);
			}
#ifdef CFG_PARA_BT_SYNC
			AudioCoreSourceAdjust(APP_SOURCE_NUM, TRUE);
#endif
		}
	}
	
#ifdef CFG_DUMP_DEBUG_EN
	dumpUartSend(p, len);
#endif

	int gie_ret = GIE_STATE_GET();
	GIE_DISABLE();
	if(MCUCircular_GetSpaceLen(&a2dp_player->sbc_fifo_cnt) > len) //validLen) //
	{
		if(a2dp_player->sbc_bytes == 0)//未曾解码 获取采样率等参数
		{
			
			//uint32_t decoder_out_sample = 0;
			ret = sbc_get_frame_infor(p,&a2dp_player->sbc_bytes,&a2dp_player->sample_rate,&a2dp_player->decoder_out_sample);
#ifdef CFG_AUDIO_OUT_AUTO_SAMPLE_RATE_44100_48000
			AudioOutSampleRateSet(a2dp_player->sample_rate);
#endif
			AudioCoreSourceChange(APP_SOURCE_NUM, 2, a2dp_player->sample_rate);
			if(a2dp_player->sbc_bytes)
				AudioCoreSourceDepthChange(APP_SOURCE_NUM, (sizeof(a2dp_player->sbc_fifo) / a2dp_player->sbc_bytes) * a2dp_player->decoder_out_sample + sizeof(a2dp_player->last_pcm_buf) / 4);
			if(ret != 0)
			{
				info_ok = 0;
				if(gie_ret)
				{
					GIE_ENABLE();
				}
				goto err;
			}
			info_ok = 1;
		}
		MCUCircular_PutData(&a2dp_player->sbc_fifo_cnt, p, len); //validLen); //
		if(MCUCircular_GetSpaceLen(&a2dp_player->sbc_fifo_cnt) <= len) //validLen); //
		{
			errorFlag = 1;
			//MCUCircular_GetData(&a2dp_player->sbc_fifo_cnt,p,len);//抛弃旧数据
		}
		if(MCUCircular_GetDataLen(&a2dp_player->sbc_fifo_cnt) >= SBC_FIFO_LEVEL_HIGH)
		{
			if(!AudioCoreSourceIsEnable(APP_SOURCE_NUM))
			{
				/*if(SoftFlagGet(SoftFlagDecoderRemind))
				{
					AudioCoreSourceDisable(1);
					play_start = 0;
				}
				else
					*/
				{
					AudioCoreSourceEnable(APP_SOURCE_NUM);
					play_start = 1;
#ifdef CFG_PARA_BT_SYNC
					AudioCoreSourceAdjust(APP_SOURCE_NUM, TRUE);
#endif
				}
			}
		}
	}
	else
	{
		errorFlag = 2;
	}
	
	if(gie_ret)
	{
		GIE_ENABLE();
	}
	if(errorFlag)
	{
		if(errorFlag == 2)
		{
			//第1包,每隔100包打印一次
			gA2dpFifoFullCnt++;
			if(gA2dpFifoFullCnt == 1)
			{
				APP_DBG("error:sbc fifo is full, recv:%ld, current space len = %ld, recv len=%lu\n", gA2dpFifoFullCnt, MCUCircular_GetSpaceLen(&a2dp_player->sbc_fifo_cnt), len);
			}
			else if(gA2dpFifoFullCnt % 100 == 0)
			{
				APP_DBG("error:sbc fifo is full, recv:%ld, current space len = %ld, recv len=%lu\n", gA2dpFifoFullCnt, MCUCircular_GetSpaceLen(&a2dp_player->sbc_fifo_cnt), len);
			}
		}
		// else if(errorFlag == 1)
		// {
		// 	// APP_DBG("fifo full, cur_space_len = %ld, recv_len=%lu\n", MCUCircular_GetSpaceLen(&a2dp_player->sbc_fifo_cnt), len);
		// }

		errorFlag = 0;
	}
	else
	{
		//再次获取到数据时,打印fifo满的次数
		if(gA2dpFifoFullCnt > 0)
		{
			APP_DBG("sbc fifo full cnt = %ld\n", gA2dpFifoFullCnt);
			gA2dpFifoFullCnt = 0;
		}
	}
	if(info_ok)
	{
		APP_DBG("%ld %ld %ld\n", a2dp_player->sbc_bytes,a2dp_player->sample_rate, len); //validLen);
	}
	if(play_start)
	{
		APP_DBG("sbc start decoder\n");
	}
	OS_SBC_UNLOCK;
	return;

	err:
	AudioCoreSourceDisable(APP_SOURCE_NUM);
	MCUCircular_Config(&a2dp_player->sbc_fifo_cnt,a2dp_player->sbc_fifo,sizeof(a2dp_player->sbc_fifo));
	APP_DBG("fifo reset a2dp_sbc_save\n");
	a2dp_player->last_pcm_len = 0;
	a2dp_player->dec_out_pcm_offset = 0;
	a2dp_player->sbc_bytes = 0;
	OS_SBC_UNLOCK;

}

/***********************************************************************************
 * 获取 a2dp sbc fifo 有效数据长度
 **********************************************************************************/
uint32_t GetValidSbcDataSize(void)
{
	uint32_t	dataSize = 0;

	if(a2dp_player == NULL)
	{
		return 0;
	}

	if(gBtPlaySbcDecoderInitFlag)
	{
		dataSize = MCUCircular_GetDataLen(&a2dp_player->sbc_fifo_cnt);
	}

	return dataSize;
}

/***********************************************************************************
 * 获取 a2dp sbc 数据长度
 **********************************************************************************/
uint16_t A2DPDataLenGet(void)
{
	uint32_t Len = 0;
	if(a2dp_player == NULL)
	{
		return 0;
	}
	if(gBtPlaySbcDecoderInitFlag == 0)
		return 0;
	if(a2dp_player->sbc_init_flag == 0 || a2dp_player->sbc_bytes == 0)
		return 0;

	if((Len = MCUCircular_GetDataLen(&a2dp_player->sbc_fifo_cnt)) < 606)//bitpool ranging 2-250
	{
		//a2dp_stream_suspend_play_end();
		BtPlayMonitor(FALSE);
		return 0;
	}
	return ((Len / a2dp_player->sbc_bytes) * a2dp_player->decoder_out_sample) + (a2dp_player->last_pcm_len / 4);
}


/***********************************************************************************
 * 判断sbc数据是否异常
 **********************************************************************************/
/*static uint8_t sbc_error(void)
{
	int fifo_len = MCUCircular_GetDataLen(&a2dp_player->sbc_fifo_cnt);
	int read_len = 0;
	while(1)
	{
		uint8_t a[2];
		MCUCircular_ReadData(&a2dp_player->sbc_fifo_cnt,a,1);
		if(a[0] == 0x9C)
		{
			MCUCircular_ReadData(&a2dp_player->sbc_fifo_cnt,a,2);
			if( (a[1]&0xB1) == 0xB1)//44100
			{
				//printf("44100 header ok\n");
				return 1;
			}
			else if( (a[1]&0xF1) == 0xF1)//48000
			{
				//printf("48000 header ok\n");
				return 1;
			}
			else
			{
				MCUCircular_GetData(&a2dp_player->sbc_fifo_cnt,a,1);
				read_len++;
				//printf("sbc info error\n");
			}
		}
		else
		{
			MCUCircular_GetData(&a2dp_player->sbc_fifo_cnt,a,1);
			read_len++;
			//printf("sbc sync error\n");
		}
		if(read_len >= fifo_len)
		{
			return 0;
		}
		//printf("%u %u\n",read_len,fifo_len);
	}
}
*/

/***********************************************************************************
 * A2DP SBC数据异常，打印sbc fifo数据，供调试使用
 **********************************************************************************/
//uint8_t gPrintfSbcFifo[15*1024] = {0};
//void printfA2dpSbcInfo(uint8_t *data, uint32_t len)
//{
//	uint32_t ii = 0;
//	uint32_t remainLen = 0;
//	APP_DBG("Error Start: ");
//	for(ii=0; ii<len; ii++)
//	{
//		APP_DBG("%02x ", data[ii]);
//	}
//	APP_DBG("\n");
//
//	remainLen = MCUCircular_GetDataLen(&a2dp_player->sbc_fifo_cnt);
//	memset(gPrintfSbcFifo,0,sizeof(gPrintfSbcFifo));
//	MCUCircular_ReadData(&a2dp_player->sbc_fifo_cnt,&gPrintfSbcFifo[0],remainLen);
//
//	for(ii=0; ii<remainLen; ii++)
//	{
//		if(ii%10 == 0) APP_DBG("\n");
//		APP_DBG("%02x ", gPrintfSbcFifo[ii]);
//	}
//	APP_DBG("\n");
//
//}

/***********************************************************************************
 * 获取A2DP SBC数据
 **********************************************************************************/
uint16_t A2DPDataGet(void* Buf, uint16_t Samples)
{
	int error_code = 0;
	//uint32_t sbc_pcm_len;
	int ret = 0;

	if((a2dp_player == NULL) || Buf == NULL || !AudioCoreSourceIsEnable(APP_SOURCE_NUM))
	{
		return 0;
	}

	if(!a2dp_player->sbc_init_flag || gBtPlaySbcDecoderInitFlag == 0 || Samples > (CFG_MAX_FRAME*2))//if Samples is too large, return 0
	{
		BtPlayMonitor(FALSE);
		return 0;
	}
	
	OS_SBC_LOCK;
	//sbc_pcm_len = A2DPDataLenGet();
	//if(sbc_pcm_len >= Samples)
	
	if(MCUCircular_GetDataLen(&a2dp_player->sbc_fifo_cnt) > 256)
	{
		uint16_t all_len = Samples*4;
		uint8_t *p = Buf;
		uint8_t *q = (uint8_t *)a2dp_player->dec_out_pcm;
		uint16_t data_len = (a2dp_player->last_pcm_len >= all_len) ? all_len : a2dp_player->last_pcm_len;
		memcpy(p,a2dp_player->last_pcm_buf, data_len);
		a2dp_player->last_pcm_len -= data_len;
		if(a2dp_player->last_pcm_len)
		{
			memcpy(a2dp_player->last_pcm_buf, (uint8_t *)(a2dp_player->last_pcm_buf) + data_len, a2dp_player->last_pcm_len);
		}
		if(data_len == all_len)
		{
			OS_SBC_UNLOCK;
			BtPlayMonitor(TRUE);
			return Samples;
		}

		p += data_len;
		while(1)
		{
			//uint32_t fram_len = 0;
			//uint32_t decoder_out_sample = 0;
			uint32_t i_count = 0;
			int src_len = 0;
			
			MCUCircular_ReadData(&a2dp_player->sbc_fifo_cnt,a2dp_player->sbc_buffer,8);
			ret = sbc_get_frame_infor(a2dp_player->sbc_buffer,&a2dp_player->sbc_bytes,&a2dp_player->sample_rate,&a2dp_player->decoder_out_sample);
			if(ret != 0)
			{
				APP_DBG("sbc fram infor error1: %d %lu\n", (int)ret,MCUCircular_GetDataLen(&a2dp_player->sbc_fifo_cnt));
				//printfA2dpSbcInfo(a2dp_player->sbc_buffer, 8);
				goto err;
			}
			
			i_count = 0;
			while(i_count <128)
			{
				//printf("%d %d\n",a2dp_player->sbc_bytes,a2dp_player->dec_out_pcm_offset);
				MCUCircular_GetData(&a2dp_player->sbc_fifo_cnt,a2dp_player->sbc_buffer,a2dp_player->sbc_bytes);	
				ret = sbc_decoder_apply_phone(&a2dp_player->sbc_dec_handle,a2dp_player->sbc_buffer,a2dp_player->sbc_bytes,&a2dp_player->dec_out_pcm[a2dp_player->dec_out_pcm_offset]);
				if(ret !=0)
				{
					APP_DBG("decoder exit,a2dp_player->sbc_bytes = %ld  decoder_out_sample = %ld \n",a2dp_player->sbc_bytes,a2dp_player->decoder_out_sample);
					goto err;
				}
				i_count += a2dp_player->decoder_out_sample;
				a2dp_player->dec_out_pcm_offset += a2dp_player->decoder_out_sample*2;
				if(i_count < 128)
				{
					MCUCircular_ReadData(&a2dp_player->sbc_fifo_cnt,a2dp_player->sbc_buffer,8);
					ret = sbc_get_frame_infor(a2dp_player->sbc_buffer,&a2dp_player->sbc_bytes,&a2dp_player->sample_rate,&a2dp_player->decoder_out_sample);
					if(ret != 0)
					{
						APP_DBG("sbc fram infor2 error: %d %lu\n", (int)ret,MCUCircular_GetDataLen(&a2dp_player->sbc_fifo_cnt));
						//printfA2dpSbcInfo(a2dp_player->sbc_buffer, 8);
						goto err;
					}
				
				}
			}
			
			src_len = 128;//解码出来固定128samples

			if(a2dp_player->dec_out_pcm_offset > 256)
			{
				memmove(a2dp_player->dec_out_pcm, &a2dp_player->dec_out_pcm[256], a2dp_player->dec_out_pcm_offset - 256);
				a2dp_player->dec_out_pcm_offset = a2dp_player->dec_out_pcm_offset -256;
			}
			else
				a2dp_player->dec_out_pcm_offset = 0;

			
			// if(src_len < 0)
			// {
			// 	APP_DBG("resampler_polyphase_apply error: %d\n", src_len);
			// 	error_code = ret;
			// 	goto err;
			// }

			src_len = src_len*4;
			if((data_len + src_len) >= all_len)
			{
				int LL = all_len - data_len;
				memcpy(p,q,LL);
				memcpy(a2dp_player->last_pcm_buf,q+LL,src_len- LL);
				a2dp_player->last_pcm_len = src_len- LL;
				break;
			}
			else
			{
				memcpy(p,q,src_len);
				data_len += src_len;
				p += src_len;
			}
		}
		OS_SBC_UNLOCK;
		if(!AudioCoreSourceIsEnable(APP_SOURCE_NUM))
		{
			memset(Buf,0,Samples*4);
		}
		BtPlayMonitor(TRUE);
		return (Samples);
	}
	else
	{
		memset(Buf,0,Samples*4);
		OS_SBC_UNLOCK;
		BtPlayMonitor(FALSE);
		return 0;
	}
	err:
	{
		APP_DBG("err:%d\n",error_code);
		int gie_ret = GIE_STATE_GET();
		GIE_DISABLE();
		AudioCoreSourceDisable(APP_SOURCE_NUM);
		MCUCircular_Config(&a2dp_player->sbc_fifo_cnt,a2dp_player->sbc_fifo,sizeof(a2dp_player->sbc_fifo));
		a2dp_player->last_pcm_len = 0;
		a2dp_player->dec_out_pcm_offset = 0;
		a2dp_player->sbc_bytes = 0;
		if(gie_ret)
		{
			GIE_ENABLE();
		}
		//printf("fifo reset A2DPDataGet\n");
	}
	
	OS_SBC_UNLOCK;
	BtPlayMonitor(FALSE);
	return 0;
}
#endif

void BtPlayMonitor(bool AudioOk)
{
	static uint32_t BtAudioOkLast = 0; //蓝牙模式音频稳定计时器
	uint32_t Current = GetSysTick1MsCnt();
	if((AudioOk != TRUE)
		&& ((GetA2dpState(0) == BT_A2DP_STATE_STREAMING)||(GetA2dpState(1) == BT_A2DP_STATE_STREAMING))//正在播放音乐
		&& (gBtPlaySbcDecoderInitFlag)//decoder初始化已经完成
										)
	{
#if BT_TWS_SUPPORT
		if((uint32_t)(Current - BtAudioOkLast) >= 80)
#else
		if((uint32_t)(Current - BtAudioOkLast) >= 30)
#endif
		{
			APP_DBG("bt play empty %ld ms, reset decoder.\n", (uint32_t)(Current - BtAudioOkLast));
			BtAudioOkLast = Current;
			a2dp_sbc_decoder_init();
		}
	}
	else
	{
		BtAudioOkLast = Current;
	}
}
#endif//#ifdef CFG_APP_BT_MODE_EN
