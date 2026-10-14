/**
 **************************************************************************************
 * @file    remind_sound_service.c
 * @brief   
 *
 * @author  pi
 * @version V1.0.0
 *
 * $Created: 2018-2-27 13:06:47$
 *
 * @Copyright (C) 2018, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
//driver、lib
#include <string.h>
#include "type.h"
#include "spi_flash.h"
#include "audio_utility.h"
#include "dac_interface.h"
#include "timeout.h"
#include "delay.h"
#include "mcu_circular_buf.h"

//application
#include "app_config.h"
#include "rtos_api.h"
#include "app_message.h"
#include "debug.h"
#include "audio_vol.h"
#include "remind_sound_item.h"
#include "audio_core_api.h"
#include "audio_core_service.h"
#include "main_task.h"
#include "bt_manager.h"
#include "bt_hf_mode.h"
#include "remind_sound.h"
#include "flash_table.h"
#include "audio_vol.h"
#include "recorder_service.h"
#include "audio_effect_control.h"

#ifdef CFG_FUNC_REMIND_SOUND_EN

#define CFG_DBUS_ACCESS_REMIND_SOUND_DATA  	//开启宏则使用DBUS从flash中获取提示音数据

#define CFG_PARAM_REMIND_LIST_MAX		15	//提示音阻塞播放最大个数。

#define	REMIND_DBG(format, ...)		//printf(format, ##__VA_ARGS__)

#ifndef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
#include "mp2.h"
#endif
#include "rom.h"

#if BT_TWS_SUPPORT
	#define	TWS_REMIND_LOCAL_AUDIO_PLAY		//tws提示音本地播放

	uint8_t RemindSoundTwsReSyncFlag = 0;
#endif

#pragma pack(1)
typedef struct _SongClipsHdr
{
	char sync[4];
	uint32_t crc;
#if(REMIND_SOUND_NUM_MODE == 4)
	uint32_t cnt;
#else
	uint8_t cnt;
#endif
} SongClipsHdr;
#pragma pack()

#pragma pack(1)
typedef struct _SongClipsEntry
{
	uint8_t id[8];
	uint32_t offset;
	uint32_t size;
} SongClipsEntry;
#pragma pack()

#define		REMIND_SOUND_ID_LEN				sizeof(((SongClipsEntry *)0)->id)

#pragma pack(1)
typedef struct _REMIND_REQUEST
{
	uint16_t ItemRef;
	uint8_t Attr;
} REMIND_REQUEST;
#pragma pack()

#define		REMIND_REQUEST_LEN				sizeof(REMIND_REQUEST)
#define		REMIND_MUTE_LEN					1024

typedef struct _RemindSoundContext
{
	uint32_t 				ConstDataAddr;
	uint32_t				ConstDataSize;
	uint32_t 				ConstDataOffset;
	uint32_t 				FramSize;

	REMIND_REQUEST			Request[CFG_PARAM_REMIND_LIST_MAX];//RemindBlockBuf[REMIND_SOUND_ID_BUF_LEN];//提示音队列
	uint8_t					EmptyIndex;//非0时，播Request[0]
	REMIND_ITEM_STATE		ItemState;
	bool					RequestUpdate;
//	uint32_t				Samples;
//	TIMER					Timer;

	uint8_t 				player_init;
	bool					MuteAppFlag;
	bool					Disable;
	bool 					NeedUnmute;
#ifdef TWS_REMIND_LOCAL_AUDIO_PLAY
	bool                    NeedSync;
#endif	
	#ifdef CFG_FUNC_RECORD_EXTERN_FLASH_EN
	bool                    Isflash;
	uint8_t 				Idflash;
	#endif
}RemindSoundContext;

//#define REMIND_FLASH_MAX_NUM		255 //flash提示音区配置决定
#define REMIND_FLASH_HDR_SIZE		REMIND_SOUND_HEADER_SIZE//0x1000 //提示音条目信息区大小
#define REMIND_FLASH_READ_TIMEOUT 	100
#define REMIND_FLASH_ADDR(n) 		(REMIND_FLASH_STORE_BASE + sizeof(SongClipsEntry) * n + sizeof(SongClipsHdr))//flash提示音区配置决定
#define REMIND_ID(Addr)				((Addr - REMIND_FLASH_STORE_BASE - sizeof(SongClipsHdr)) / (sizeof(SongClipsEntry)))
#define REMIND_FLASH_FLAG_STR		("MVUB")

#ifdef CFG_DBUS_ACCESS_REMIND_SOUND_DATA
	#define	RemindDataRead(addr,buf,len)	(NULL == memcpy((void *)(buf),(void *)(addr),len))
#else
	#define	RemindDataRead(addr,buf,len)	(FLASH_NONE_ERR != SpiFlashRead(addr,(uint8_t *)(buf),len,REMIND_FLASH_READ_TIMEOUT))
#endif

static RemindSoundContext		RemindSoundCt;

osMutexId RemindMutex = NULL;

#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
	#define REMIND_DECODER_IN_FIFO_SIZE		2*1024

	static uint8_t read_flash_buf[512];
	static MemHandle RemindDecoderInMemHandle;	
	bool RemindMp3DataRead(void);
	extern uint16_t RemindDecoderPcmDataGet(void * pcmData,uint16_t sampleLen);// call by audio core one by one
	extern uint16_t RemindDecoderPcmDataLenGet(void);	
#else
	#define CFG_REMIND_MP2_DECODE_BUF_USE_MALLOC	//使用malloc申请内存  动态使用
	static	struct _Mp2DecodeContext
	{
		MPADecodeContext  	dec_cnt;
		int16_t 			dec_fifo[15*128 * MPA_MAX_CHANNELS];//3840字节
		uint32_t 			dec_last_len;
		uint8_t 			dec_buf[626 * MPA_MAX_CHANNELS];
	}
	#ifdef CFG_REMIND_MP2_DECODE_BUF_USE_MALLOC
		* Mp2Decode = NULL;
	#else
		Mp2DecodeTemp;

		#define Mp2Decode		((struct _Mp2DecodeContext *)(&Mp2DecodeTemp))
	#endif

	extern bool decode_header(uint32_t header);
	extern bool MP2_decode_frame(void* PcmData, uint8_t* Mp2Data);
	extern void MP2_decode_init(void* MemAddr);
#endif

uint32_t get_remind_state(void)
{
	return RemindSoundCt.player_init;
}

uint8_t get_remind_channels(void)
{
#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
	return 2;//mp3默认双声道
#else
	return MPA_MAX_CHANNELS;
#endif
}

static uint16_t RemindSountItemFind(uint8_t *RemindItem)
{
	uint16_t j;
	SongClipsEntry SongClips;

	//查找对应的ConstDataId
	for(j = 0; j < SOUND_REMIND_TOTAL; j++)
	{
		if(RemindDataRead(REMIND_FLASH_ADDR(j), &SongClips, sizeof(SongClipsEntry)))
		{
			return SOUND_REMIND_TOTAL;
		}
		if(memcmp(&SongClips.id,RemindItem, sizeof(SongClips.id)) == 0)//找到
		{
//			RemindSoundCt.ConstDataOffset = 0;
//			RemindSoundCt.ConstDataAddr = SongClips.offset + REMIND_FLASH_STORE_BASE; //工具制作提示音bin 使用相对地址
//			RemindSoundCt.ConstDataSize = SongClips.size;
			REMIND_DBG("Find Remind Ref:%d\n", j);
			return j;
		}
	}
	return SOUND_REMIND_TOTAL;
}
#ifdef CFG_FUNC_RECORD_EXTERN_FLASH_EN
extern uint32_t	rec_addr_start;
#endif
//根据flash驱动设计，最大支持255条提示音。
static bool	RemindSoundReadItemInfo(uint16_t ItemRef)
{
	uint16_t j;
	SongClipsEntry SongClips;

#ifdef CFG_FUNC_RECORD_EXTERN_FLASH_EN
	REC_EXTFLASH_HEAD rec_msg;
	if(!RemindSoundCt.Isflash)
#endif
	{
		if(ItemRef >= SOUND_REMIND_TOTAL)
			return FALSE;	
	}

#ifdef CFG_FUNC_RECORD_EXTERN_FLASH_EN
	if(RemindSoundCt.Isflash)
	{
		SpiRead(rec_addr_start + CFG_PARA_RECORDS_MAX_SIZE *(RemindSoundCt.Idflash -1),&rec_msg,sizeof(REC_EXTFLASH_HEAD));//读取flash
	}
	else	
#endif
	{
		if(RemindDataRead(REMIND_FLASH_ADDR(ItemRef), &SongClips, sizeof(SongClipsEntry)))
		{
			return FALSE;
		}
	}

#ifdef CFG_FUNC_RECORD_EXTERN_FLASH_EN
	if(RemindSoundCt.Isflash)
	{
		RemindSoundCt.ConstDataOffset   = 0;//CFG_PARA_RECORDS_INFO_SIZE;
		RemindSoundCt.ConstDataAddr 	= rec_addr_start + CFG_PARA_RECORDS_MAX_SIZE *(RemindSoundCt.Idflash - 1)+CFG_PARA_RECORDS_INFO_SIZE;
		RemindSoundCt.ConstDataSize 	= rec_msg.RecSize;
		APP_DBG("rec_msg.RecSize:%d\n",rec_msg.RecSize);
	}
	else
#endif
	{
		RemindSoundCt.ConstDataOffset = 0;
		RemindSoundCt.ConstDataAddr = SongClips.offset + REMIND_FLASH_STORE_BASE; //工具制作提示音bin 使用相对地址
		RemindSoundCt.ConstDataSize = SongClips.size;

		DBG("play: ");
		for(j=0;j<sizeof(SongClips.id);j++)
			DBG("%c",SongClips.id[j]);
		DBG("\n");
	}
	return TRUE;
}

//提示音条目和数据区完整性校验，影响开机速度。
bool sound_clips_all_crc(void)
{
	SongClipsHdr *hdr;
	SongClipsEntry *ptr;
	uint16_t crc=0, i, j, CrcRead;
	uint32_t FlashAddr, all_len = 0;
	uint8_t *data_ptr = NULL;
	bool ret = TRUE;
	FlashAddr = REMIND_FLASH_STORE_BASE;

	if(FlashAddr == 0)
	{
		return FALSE;
	}

	data_ptr = (uint8_t *)osPortMalloc(REMIND_FLASH_HDR_SIZE);
	if(data_ptr == NULL)
	{
		return FALSE;
	}

	if(RemindDataRead(FlashAddr,data_ptr,REMIND_FLASH_HDR_SIZE))
	{
		REMIND_DBG("read const data error!\r\n");
		ret = FALSE;
	}
	else
	{
		ptr = (SongClipsEntry*)(data_ptr + sizeof(SongClipsHdr));
		hdr = (SongClipsHdr *)(data_ptr);
		if(strncmp((const char *)hdr->sync, REMIND_FLASH_FLAG_STR, 4) || !hdr->cnt)
		{
			REMIND_DBG("sync not found or no Item\n");
			ret = FALSE;
		}
		else
		{
			for(i = 0; i < hdr->cnt; i++)
			{
				all_len += ptr[i].size;
				for(j = 0; j < REMIND_SOUND_ID_LEN; j++)
				{
					REMIND_DBG("%c", ((uint8_t *)&ptr[i].id)[j]);
				}
				REMIND_DBG("/");
			}
			REMIND_DBG("\nALL clips size = %d\n", (int)all_len);
//			if(REMIND_FLASH_STORE_BASE + REMIND_FLASH_HDR_SIZE + all_len >= REMIND_FLASH_STORE_OVERFLOW)
//			{
//				REMIND_DBG("Remind flash const data overflow.\n");
//				ret = FALSE;
//			}
			CrcRead = hdr->crc;
			crc = ROM_CRC16((char *)data_ptr, 4, crc);
			crc = ROM_CRC16((char *)data_ptr + 8, REMIND_FLASH_HDR_SIZE - 8, crc);
			FlashAddr += REMIND_FLASH_HDR_SIZE;
			while(all_len && ret)
			{
				if(all_len > REMIND_FLASH_HDR_SIZE)
				{
					i = REMIND_FLASH_HDR_SIZE;
				}
				else
				{
					i = all_len;
				}
				if(RemindDataRead(FlashAddr,data_ptr,i))
				{
					REMIND_DBG("read const data error!\r\n");
					ret = FALSE;
				}
				else
				{
					crc = ROM_CRC16((char *)data_ptr, i, crc);
					FlashAddr += i;
					all_len -= i;
				}
			}
			if(crc == CrcRead)
			{
				REMIND_DBG("Crc = %04X\n", crc);
			}
			else
			{
				REMIND_DBG("Crc error: %04X != %04X\n", crc, CrcRead);
				ret = FALSE;
			}
		}
	}
	osPortFree(data_ptr);
	return ret;
}

#ifndef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
static void mp2_play_init(void)
{
	MP2_decode_init(&Mp2Decode->dec_cnt);

#ifdef CFG_FUNC_RECORD_EXTERN_FLASH_EN
	if(RemindSoundCt.Isflash)
	{
		SpiRead(RemindSoundCt.ConstDataAddr, Mp2Decode->dec_buf, 4);
	}
	else
#endif
	{
		if(RemindDataRead(RemindSoundCt.ConstDataAddr, Mp2Decode->dec_buf, 4))
		{
			return;
		}
	}
	//读取4个字节确定帧长
	if(!decode_header((Mp2Decode->dec_buf[0] << 24) | (Mp2Decode->dec_buf[1] << 16) | (Mp2Decode->dec_buf[2] << 8) | Mp2Decode->dec_buf[3]))
	{
		REMIND_DBG("decode_header error!\n");
		return;
	}
	RemindSoundCt.FramSize = Mp2Decode->dec_cnt.frame_size;
	Mp2Decode->dec_last_len = 0;
	REMIND_DBG("sample ramte:%u\n",Mp2Decode->dec_cnt.sample_rate);
	REMIND_DBG("fram size   :%u\n",Mp2Decode->dec_cnt.frame_size);
	REMIND_DBG("bit rate    :%u\n",Mp2Decode->dec_cnt.bit_rate);
	REMIND_DBG("channels    :%u\n",Mp2Decode->dec_cnt.nb_channels);
	AudioCoreSourceChange(REMIND_SOURCE_NUM, Mp2Decode->dec_cnt.nb_channels, Mp2Decode->dec_cnt.sample_rate);
}
#endif

void RemindSoundAudioPlayEnd(void)
{
//	AudioCoreSourceDisable(REMIND_SOURCE_NUM);
//	RemindSoundCt.IsBlock = FALSE;
	RemindSoundCt.player_init = MP2_DECODE_IDLE;
	//REMIND_DBG("remind audio close!\n");
#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
	DecoderStop(DECODER_REMIND_CHANNEL);
#endif
#ifdef CFG_FUNC_RECORD_EXTERN_FLASH_EN
	RemindSoundCt.Isflash = FALSE;
#endif
}

void SendRemindSoundEndMsg(void)
{
	MessageContext		msgSend;
	msgSend.msgId = MSG_REMIND_PLAY_END;
	MessageSend(GetMainMessageHandle(), &msgSend);
}

//提示音播放结束通知其他task
void RemindSoundPlayEndNotify(void)
{
	//通话时,连上蓝牙,播放连接提示音结束后,再进入通话模式
	SoftFlagDeregister(SoftFlagWaitBtRemindEnd);
	if(SoftFlagGet(SoftFlagDelayEnterBtHf))
	{
		MessageContext		msgSend;
		SoftFlagDeregister(SoftFlagDelayEnterBtHf);

		msgSend.msgId = MSG_DEVICE_SERVICE_ENTER_BTHF_MODE;
		MessageSend(GetMainMessageHandle(), &msgSend);
	}	
	
	if(RemindSoundCt.MuteAppFlag && (!RemindSoundWaitingPlay()))
	{	
		RemindSoundCt.MuteAppFlag = FALSE;	
		//AudioCoreSourceUnmute(MIC_SOURCE_NUM,TRUE,TRUE);
		AudioCoreSourceUnmute(APP_SOURCE_NUM,TRUE,TRUE);
		//Roboeffect_AppSourceMute_Set(RemindSoundCt.MuteAppFlag);
	}
	
	SendRemindSoundEndMsg();

#ifdef CFG_LOW_DELAY_CTRL
	void lowDelaySyncFlagSet(bool Flag);
	lowDelaySyncFlagSet(1);
#endif
}

#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
void RemindSoundAudioDecoderStop(void)
{
	RemindSoundAudioPlayEnd();
	REMIND_DBG("remind play end!\n");
	RemindSoundPlayEndNotify();
	RemindSoundCt.Request[0].Attr |= REMIND_ATTR_CLEAR;
	RemindSoundCt.ItemState = REMIND_ITEM_IDLE;
	RemindSoundCt.RequestUpdate = TRUE;
	AudioCoreSourceDisable(REMIND_SOURCE_NUM);
}
#endif

#ifndef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
void RemindMp2Decode(void)
{
	uint8_t RemindSourceFlowNet = AudioCoreGetNetBySourceChannelNUM(REMIND_SOURCE_NUM);
	if(RemindSoundCt.player_init == MP2_DECODE_HEADER)
	{
		REMIND_DBG("remind play start!\n");
#ifdef CFG_REMIND_MP2_DECODE_BUF_USE_MALLOC
		if(!Mp2Decode)
			Mp2Decode = osPortMalloc(sizeof(struct _Mp2DecodeContext));
		if(Mp2Decode)
			memset(Mp2Decode,0,sizeof(struct _Mp2DecodeContext));
		else
		{
			APP_DBG("RemindSound Malloc ERROR!!!\n");
			RemindSoundAudioPlayEnd();
			RemindSoundCt.NeedUnmute = TRUE;
			RemindSoundPlayEndNotify();
			RemindSoundCt.EmptyIndex = 0;
			RemindSoundCt.ItemState = REMIND_ITEM_IDLE;
			return;
		}
#endif
		mp2_play_init();
		RemindSoundCt.player_init = MP2_DECODE_FRAME;
		RemindSoundCt.NeedUnmute = TRUE;
//		RemindSoundCt.Samples = 0;
	}
	if(RemindSoundCt.player_init == MP2_DECODE_FRAME)
	{
		uint8_t *p = (uint8_t*)Mp2Decode->dec_fifo;
		if(Mp2Decode->dec_last_len < AudioCoreFrameSizeGet(RemindSourceFlowNet))
		{
#ifdef CFG_FUNC_RECORD_EXTERN_FLASH_EN
			if(RemindSoundCt.Isflash)
			{
				if(FLASH_NONE_ERR != SpiRead(RemindSoundCt.ConstDataAddr+RemindSoundCt.ConstDataOffset, Mp2Decode->dec_buf, 4))
				{
					//APP_DBG("FLASH_NONE_ERR!!!\n");
				}
			}
			else
#endif
			{
				if(RemindDataRead(RemindSoundCt.ConstDataAddr+RemindSoundCt.ConstDataOffset, Mp2Decode->dec_buf, 4))
				{

				}	
			}

			if(!decode_header((Mp2Decode->dec_buf[0] << 24) | (Mp2Decode->dec_buf[1] << 16) | (Mp2Decode->dec_buf[2] << 8) | Mp2Decode->dec_buf[3]))
			{
				REMIND_DBG("decode_header error!\n");
				RemindSoundAudioPlayEnd();
				RemindSoundCt.NeedUnmute = TRUE;			
				//SendRemindSoundEndMsg();
				RemindSoundPlayEndNotify();
				return;
			}
			RemindSoundCt.FramSize = Mp2Decode->dec_cnt.frame_size;

#ifdef CFG_FUNC_RECORD_EXTERN_FLASH_EN
			if(RemindSoundCt.Isflash)
			{
				if(FLASH_NONE_ERR != SpiRead(RemindSoundCt.ConstDataAddr+RemindSoundCt.ConstDataOffset, Mp2Decode->dec_buf, RemindSoundCt.FramSize))
				{
					//APP_DBG("FLASH_NONE_ERR!!!\n");
				}
			}
			else
#endif
			{
				if(RemindDataRead(RemindSoundCt.ConstDataAddr+RemindSoundCt.ConstDataOffset, Mp2Decode->dec_buf, RemindSoundCt.FramSize))
				{

				}	
			}
			//REMIND_DBG("RemindSoundCt.FramSize:%u %08X\n",RemindSoundCt.FramSize,RemindSoundCt.ConstDataAddr+RemindSoundCt.ConstDataOffset);
			RemindSoundCt.ConstDataOffset += RemindSoundCt.FramSize;
//			DBG("Remind %d @ %d\n", RemindSoundCt.ConstDataOffset, RemindSoundCt.ConstDataSize);
			if(RemindSoundCt.ConstDataOffset >= RemindSoundCt.ConstDataSize)
			{
				REMIND_DBG("Remind end\n");
				RemindSoundCt.player_init = MP2_DECODE_END;
				//SendRemindSoundEndMsg();
				RemindSoundPlayEndNotify();
			}
//			REMIND_DBG("%02X,%02X,%02X,%02X\n",Mp2Decode.dec_buf[0],Mp2Decode.dec_buf[1],Mp2Decode.dec_buf[2],Mp2Decode.dec_buf[3]);
			if(MP2_decode_frame(p+Mp2Decode->dec_last_len*2*Mp2Decode->dec_cnt.nb_channels,Mp2Decode->dec_buf) == FALSE)
			{
				REMIND_DBG("MP2_decode_frame error!\n");
				RemindSoundAudioPlayEnd();
				RemindSoundCt.NeedUnmute = TRUE;				
				//SendRemindSoundEndMsg();
				RemindSoundPlayEndNotify();
				return;
			}
			Mp2Decode->dec_last_len += 1152;
		}
	}
}
#endif

uint16_t RemindDataLenGet(void)
{
	static uint16_t delay_cnt = 0;
	uint8_t RemindSourceFlowNet = AudioCoreGetNetBySourceChannelNUM(REMIND_SOURCE_NUM);
#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
	uint16_t len ;//= RemindDecoderPcmDataLenGet();

	if(RemindSoundCt.player_init == MP2_DECODE_HEADER)
	{
		delay_cnt = 0;
		if(RemindMp3DataRead())	
			RemindSoundCt.player_init = MP2_DECODE_FRAME;
	}
	if(RemindSoundCt.player_init != MP2_DECODE_IDLE)
		RemindDecodeProcess();//  decode step 4

	if(RemindSoundCt.NeedUnmute)
	{
		RemindSoundCt.NeedUnmute = FALSE;
		if(IsAudioPlayerMute() == TRUE)
		{
			HardWareMuteOrUnMute();
		}
	}

	len = RemindDecoderPcmDataLenGet();
	if(len == 0 && RemindSoundCt.player_init == MP2_DECODE_FRAME && mv_msize(&RemindDecoderInMemHandle) == 0)
	{
		if(++delay_cnt > 100) //延时结束
		{
			RemindSoundAudioDecoderStop();
			delay_cnt = 0;
		}
	}
	return len;
#else	
	if(RemindSoundCt.player_init == MP2_DECODE_HEADER)
	{
		RemindSoundCt.NeedUnmute = FALSE;
		delay_cnt = 0;
	}
	RemindMp2Decode();

	if(RemindSoundCt.NeedUnmute)
	{
		RemindSoundCt.NeedUnmute = FALSE;
		if(IsAudioPlayerMute() == TRUE)
		{
			HardWareMuteOrUnMute();
		}
	}

	if(RemindSoundCt.player_init == MP2_DECODE_END && (Mp2Decode == NULL || Mp2Decode->dec_last_len < AudioCoreFrameSizeGet(RemindSourceFlowNet)))
	{
		++delay_cnt;
		if(delay_cnt > 100) //延时结束		
		{
			RemindSoundAudioPlayEnd();
			REMIND_DBG("remind play end!\n");
			RemindSoundPlayEndNotify();	
			delay_cnt = 0;
		}
	}
	if(Mp2Decode != NULL)
		return Mp2Decode->dec_last_len;
	else
		return 0;
#endif
}


uint16_t RemindDataGet(void* Buf, uint16_t Samples)
{

#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
	return RemindDecoderPcmDataGet(Buf,Samples);
#else
	if(Mp2Decode != NULL && Mp2Decode->dec_last_len >= Samples)
	{
		uint8_t *p = (uint8_t*)Mp2Decode->dec_fifo;
		memcpy(Buf,p,Samples*2*Mp2Decode->dec_cnt.nb_channels);
//		RemindSoundCt.Samples += Samples;
		Mp2Decode->dec_last_len = Mp2Decode->dec_last_len-Samples;
		memcpy(p,p+Samples*2*Mp2Decode->dec_cnt.nb_channels,Mp2Decode->dec_last_len*2*Mp2Decode->dec_cnt.nb_channels);
		return Samples;
	}
	return 0;
#endif	
}

void RemindSoundItemRequestDisable(void)
{
	RemindSoundCt.Disable = TRUE;
}

bool GetRemindSoundItemDisable(void)
{
	return RemindSoundCt.Disable;
}

bool RemindSoundServiceItemRequest(char *SoundItem, uint32_t play_attribute)
{
	uint8_t i;
	uint16_t ItemRef = SOUND_REMIND_TOTAL;

	if(SoftFlagGet(SoftFlagNoRemind) || RemindSoundCt.Disable == TRUE)
		return FALSE;
	if(SoundItem == NULL)//strlen(SoundItem) != REMIND_SOUND_ID_LEN ||
		return FALSE;
	if(RemindSoundCt.EmptyIndex == CFG_PARAM_REMIND_LIST_MAX)
	{
		REMIND_DBG("REMIND_SOUND_ID_BUF is full!\n");
		return FALSE;
	}
	ItemRef = RemindSountItemFind((uint8_t *)SoundItem);
	if(ItemRef >= SOUND_REMIND_TOTAL)
		return FALSE;

	RemindSoundCt.RequestUpdate = TRUE;
	osMutexLock(RemindMutex);

	for(i = RemindSoundCt.EmptyIndex; i > 1; i--)//if index==0 no move
	{
		if((RemindSoundCt.Request[i - 1].Attr & REMIND_PRIO_MASK) < (play_attribute & REMIND_PRIO_MASK))
		{
			memcpy(&RemindSoundCt.Request[i], &RemindSoundCt.Request[i - 1], REMIND_REQUEST_LEN);
		}
		else
		{
			break;
		}
	}

	RemindSoundCt.Request[i].ItemRef = ItemRef;
	RemindSoundCt.Request[i].Attr = play_attribute;// & REMIND_PRIO_MASK;
	RemindSoundCt.EmptyIndex++;
	REMIND_DBG("i:%d Addr:%x Attr:%x\n", i, ItemRef, play_attribute);
	if(RemindSoundCt.EmptyIndex > 1
			&& (RemindSoundCt.Request[0].Attr & REMIND_ATTR_CLEAR) == 0
			&& (play_attribute & REMIND_ATTR_NEED_HOLD_PLAY) == 0
			&& (RemindSoundCt.Request[0].Attr & REMIND_PRIO_MASK) == REMIND_PRIO_NORMAL)
	{//当前播放 REMIND_PRIO_NORMAL时 先停播。
		//需检测同步。
		RemindSoundCt.Request[0].Attr |= REMIND_ATTR_CLEAR;
#ifdef TWS_REMIND_LOCAL_AUDIO_PLAY
		tws_local_audio_stop(RemindSoundCt.Request[0].ItemRef);
#endif		
	}

	osMutexUnlock(RemindMutex);

//	if(play_attribute & REMIND_SOUND_NEED_HOLD_PLAY)
//	{
//		if(RemindSoundCt.player_init != MP2_DECODE_IDLE
//			&& RemindSoundCt.player_init != MP2_WAIT_FOR_DECODE
//			&& !RemindSoundCt.IsBlock)
//			RemindSoundCt.IsBlock = TRUE;
//	}


//	if(RemindSoundCt.player_init == MP2_DECODE_IDLE)
//		RemindSoundCt.player_init = MP2_WAIT_FOR_DECODE;
//	if(play_attribute & REMIND_SOUND_NEED_MUTE_APP_SOURCE)
//	{
//		RemindSoundCt.MuteAppFlag = TRUE;
//		//AudioCoreSourceMute(MIC_SOURCE_NUM,TRUE,TRUE);
//		AudioCoreSourceMute(APP_SOURCE_NUM,TRUE,TRUE);
//	}
	return TRUE;
}
#ifdef TWS_REMIND_LOCAL_AUDIO_PLAY
extern void tws_local_audio_stopped(uint16_t ItemRef);
bool RemindSoundSyncRequest(uint16_t ItemRef, TWS_AUDIO_CMD CMD)
{
	uint8_t i;
	uint8_t play_attribute = REMIND_PRIO_NORMAL;//REMIND_PRIO_PARTNER;

	if(SoftFlagGet(SoftFlagNoRemind) || RemindSoundCt.Disable == TRUE || ItemRef >= SOUND_REMIND_TOTAL)
		return FALSE;

	if(RemindSoundCt.EmptyIndex == CFG_PARAM_REMIND_LIST_MAX && CMD != TWS_CMD_LOCAL_STOP)
	{
		REMIND_DBG("REMIND_SOUND_ID_BUF is full!\n");
		return FALSE;
	}

	if(CMD == TWS_CMD_LOCAL_STOP)
	{
//		for(i = 0; i < RemindSoundCt.EmptyIndex; i++)
//		{
//			if(ItemAddr == RemindSoundCt.Request[i].Remind
//					&& (RemindSoundCt.Request[i].Attr & REMIND_PRIO_MASK) == REMIND_PRIO_PARTNER
//					&& (RemindSoundCt.Request[i].Attr & REMIND_ATTR_CLEAR) == 0)
//			{
				RemindSoundCt.Request[0].Attr |= REMIND_ATTR_CLEAR;
//				break;
//			}
//		}
	}
	else
	{
		if((CMD == TWS_CMD_LOCAL_MASTER && tws_get_role() == BT_TWS_SLAVE)
				|| (CMD == TWS_CMD_LOCAL_SLAVE && tws_get_role() == BT_TWS_MASTER))
		{
			play_attribute |= REMIND_ATTR_MUTE;
		}
		RemindSoundCt.RequestUpdate = TRUE;
		osMutexLock(RemindMutex);
		for(i = RemindSoundCt.EmptyIndex; i > 1; i--)//if index==0 no move
		{
			if((RemindSoundCt.Request[i - 1].Attr & REMIND_PRIO_MASK) < (play_attribute & REMIND_PRIO_MASK))
			{
				memcpy(&RemindSoundCt.Request[i], &RemindSoundCt.Request[i - 1], REMIND_REQUEST_LEN);
			}
			else
			{
				break;
			}
		}
		RemindSoundCt.Request[i].ItemRef = ItemRef;
		RemindSoundCt.Request[i].Attr = play_attribute;// & REMIND_PRIO_MASK;
		RemindSoundCt.EmptyIndex++;
		if(RemindSoundCt.EmptyIndex > 1 && ((RemindSoundCt.Request[0].Attr & REMIND_PRIO_MASK) == REMIND_PRIO_NORMAL))
		{//当前播放 REMIND_PRIO_NORMAL时 先停播。
			//需检测同步。
			RemindSoundCt.Request[0].Attr |= REMIND_ATTR_CLEAR;
		}
		osMutexUnlock(RemindMutex);
	}
	return TRUE;
}
#endif

void RemindSoundSyncEnable(void)
{
	AudioCoreSourceEnable(REMIND_SOURCE_NUM);
}

//bool RemindSoundItemClearWaitting(uint8_t Prio)
//{
//	if(DisablePrio < Attr)
//}


//有提示音在队列或者buf中等待播放
bool RemindSoundWaitingPlay(void)
{
//	if(RemindSoundCt.RequestRemind[0] || MCUCircular_GetDataLen(&RemindSoundCt.RemindBlockCircular))
//		return 1;
	return 0;
}

void RemindRequestsRecast(void)
{
	uint8_t i, Index = 0;

	if(RemindSoundCt.ItemState != REMIND_ITEM_IDLE || RemindSoundCt.EmptyIndex == 0)
	{
		return ;
	}

	osMutexLock(RemindMutex);

	for(i = 0; i < RemindSoundCt.EmptyIndex; i++)
	{
		REMIND_DBG("Remind Item:%d %d\n", i, RemindSoundCt.Request[i].Attr & REMIND_ATTR_CLEAR);

		if((RemindSoundCt.Request[i].Attr & REMIND_ATTR_CLEAR) == 0)
		{
			if(Index == 0)
			{
				memcpy(&RemindSoundCt.Request[Index++], &RemindSoundCt.Request[i], REMIND_REQUEST_LEN);

			}
			else if((RemindSoundCt.Request[0].Attr & REMIND_PRIO_MASK) == REMIND_PRIO_NORMAL)
			{//play last REMIND_PRIO_NORMAL
				memcpy(&RemindSoundCt.Request[0], &RemindSoundCt.Request[i], REMIND_REQUEST_LEN);
			}
			else if((RemindSoundCt.Request[i].Attr & REMIND_PRIO_MASK) > (RemindSoundCt.Request[0].Attr & REMIND_PRIO_MASK))
			{//冗余检测,先由RemindSoundServiceItemRequest排序
				memcpy(&RemindSoundCt.Request[0], &RemindSoundCt.Request[i], REMIND_REQUEST_LEN);
			}
			else
			{
				memcpy(&RemindSoundCt.Request[Index++], &RemindSoundCt.Request[i], REMIND_REQUEST_LEN);
			}
		}
	}
	RemindSoundCt.EmptyIndex = Index;
	RemindSoundCt.RequestUpdate = FALSE;
	osMutexUnlock(RemindMutex);

}

bool RemindSoundRun(SysModeState ModeState)
{
	int i;

	if(RemindSoundCt.EmptyIndex == 0 && RemindSoundCt.ItemState == REMIND_ITEM_IDLE)
	{
#ifdef TWS_REMIND_LOCAL_AUDIO_PLAY
		RemindSoundCt.NeedSync = FALSE;
#endif		
		return FALSE;
	}
	else if(ModeState == ModeStateDeinit
			&& RemindSoundCt.ItemState == REMIND_ITEM_IDLE)
	{//切模式期间提示音 sys播放或其他打断处理完毕
		return FALSE;
	}

	if(ModeState == ModeStateDeinit)
	{
		if((RemindSoundCt.Request[0].Attr & REMIND_ATTR_CLEAR) == 0)
		{
#ifdef TWS_REMIND_LOCAL_AUDIO_PLAY
			tws_local_audio_stop(RemindSoundCt.Request[0].ItemRef);
#endif			
			RemindSoundCt.Request[0].Attr |= REMIND_ATTR_CLEAR;
		}
		osMutexLock(RemindMutex);
		for(i = 1; i < RemindSoundCt.EmptyIndex; i++)
		{
			if((RemindSoundCt.Request[i].Attr & REMIND_ATTR_CLEAR) == 0
				&& ((RemindSoundCt.Request[i].Attr & REMIND_PRIO_MASK) != REMIND_PRIO_SYS))
			{
				RemindSoundCt.Request[i].Attr |= REMIND_ATTR_CLEAR;
			}
		}
#ifdef TWS_REMIND_LOCAL_AUDIO_PLAY
		if(!RemindSoundCt.NeedSync)
		{
			RemindSoundCt.NeedSync = TRUE;
			tws_master_remindclear_send(TRUE);
		}
#endif
		osMutexUnlock(RemindMutex);
	}
	else if(ModeState == ModeStateInit)
	{

		if(RemindSoundCt.ItemState != REMIND_ITEM_IDLE)
		{
			RemindSoundCt.Request[0].Attr |= REMIND_ATTR_CLEAR;
			RemindSoundCt.RequestUpdate = TRUE;
			RemindSoundCt.ItemState = REMIND_ITEM_IDLE;
#ifdef TWS_REMIND_LOCAL_AUDIO_PLAY
			tws_local_audio_stopped(RemindSoundCt.Request[0].ItemRef);
#endif			
		}
		RemindRequestsRecast();
		return TRUE;
	}

	switch(RemindSoundCt.ItemState)
	{
		case REMIND_ITEM_IDLE:
			if(RemindSoundCt.EmptyIndex && ModeState == ModeStateRunning)
			{
				#ifdef CFG_FUNC_RECORD_EXTERN_FLASH_EN
				if((!RemindSoundCt.RequestUpdate && (RemindSoundCt.Request[0].Attr & REMIND_ATTR_CLEAR) == 0) || RemindSoundCt.Isflash)
				#else
				if((!RemindSoundCt.RequestUpdate && (RemindSoundCt.Request[0].Attr & REMIND_ATTR_CLEAR) == 0))
				#endif
				{
					if(RemindSoundReadItemInfo(RemindSoundCt.Request[0].ItemRef))
					{
						RemindSoundCt.ItemState = REMIND_ITEM_PREPARE;
						RemindSoundCt.player_init = MP2_DECODE_HEADER;
						if(RemindSoundCt.Request[0].Attr & REMIND_ATTR_NEED_MUTE_APP_SOURCE)
						{
							RemindSoundCt.MuteAppFlag = TRUE;
							AudioCoreSourceMute(APP_SOURCE_NUM,TRUE,TRUE);
							//Roboeffect_AppSourceMute_Set(RemindSoundCt.MuteAppFlag);
						}
#ifdef TWS_REMIND_LOCAL_AUDIO_PLAY
						if(GetSystemMode() != ModeIdle)
						{
							if((RemindSoundCt.Request[0].Attr & REMIND_ATTR_MUTE) == 0 && ((RemindSoundCt.Request[0].Attr & REMIND_PRIO_MASK) == REMIND_PRIO_PARTNER))
							{
								tws_local_audio_set(RemindSoundCt.Request[0].ItemRef, TWS_CMD_LOCAL_MASTER);
							}
							else
							{
								tws_local_audio_set(RemindSoundCt.Request[0].ItemRef, TWS_CMD_LOCAL_SYNC);
							}
						}	
#endif											
						break;
					}
					else
					{
						RemindSoundCt.Request[0].Attr |= REMIND_ATTR_CLEAR;
						RemindSoundCt.RequestUpdate = TRUE;
					}
				}
				RemindRequestsRecast();
			}
			break;

		case REMIND_ITEM_PREPARE:
#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
			if(GetDecoderState(DECODER_REMIND_CHANNEL) != DecoderStateNone)
				DecoderStop(DECODER_REMIND_CHANNEL);
			DecoderSourceNumSet(REMIND_SOURCE_NUM,DECODER_REMIND_CHANNEL);
			if(RemindMp3DataRead())
				RemindSoundCt.player_init = MP2_DECODE_FRAME;
			DecoderInit(&RemindDecoderInMemHandle,DECODER_REMIND_CHANNEL, (int32_t)IO_TYPE_MEMORY, MP3_DECODER);
			DecoderPlay(DECODER_REMIND_CHANNEL);
			REMIND_DBG("remind play start!\n");
			RemindSoundCt.NeedUnmute = TRUE;
#endif
#ifdef TWS_REMIND_LOCAL_AUDIO_PLAY
			if(!tws_local_audio_wait())
#endif	
			{
				AudioCoreSourceEnable(REMIND_SOURCE_NUM);
				RemindSoundCt.ItemState = REMIND_ITEM_PLAY;
			}
#ifdef TWS_REMIND_LOCAL_AUDIO_PLAY
			else if(RemindSoundCt.Request[0].Attr & REMIND_ATTR_CLEAR
					&&	!AudioCoreSourceIsEnable(REMIND_SOURCE_NUM))
			{//等待开播时直接结束
					RemindSoundCt.ItemState = REMIND_ITEM_IDLE;
					RemindSoundCt.RequestUpdate = TRUE;
					AudioCoreSourceDisable(REMIND_SOURCE_NUM);
					tws_local_audio_stopped(RemindSoundCt.Request[0].ItemRef);
			}
#endif			
		case REMIND_ITEM_PLAY:
			#ifdef CFG_FUNC_RECORD_EXTERN_FLASH_EN
			if(RemindSoundCt.Request[0].Attr & REMIND_ATTR_CLEAR
					&& RemindSoundCt.FramSize > 0
					&& RemindSoundCt.ConstDataOffset + RemindSoundCt.FramSize * 2 < RemindSoundCt.ConstDataSize
					&& !RemindSoundCt.Isflash)
			#else
			if(RemindSoundCt.Request[0].Attr & REMIND_ATTR_CLEAR
					&& RemindSoundCt.FramSize > 0
					&& RemindSoundCt.ConstDataOffset + RemindSoundCt.FramSize * 2 < RemindSoundCt.ConstDataSize)
			#endif
			{
//				RemindSoundCt.ItemState = REMIND_ITEM_IDLE;
//				AudioCoreSourceDisable(REMIND_SOURCE_NUM);
//				RemindSoundCt.RequestUpdate = TRUE;
				RemindSoundCt.ConstDataSize = RemindSoundCt.ConstDataOffset + RemindSoundCt.FramSize * 2;
			}
			else if((RemindSoundCt.Request[0].Attr & REMIND_ATTR_MUTE) == 0
					&& RemindSoundCt.FramSize > 0
					&& RemindSoundCt.ConstDataOffset >= RemindSoundCt.FramSize * 2)
			{
				AudioCoreSourceUnmute(REMIND_SOURCE_NUM, TRUE, TRUE);
				DBG("Unmute Remind\n");
				RemindSoundCt.ItemState = REMIND_ITEM_UNMUTE;
			}
			else if(RemindSoundCt.ConstDataOffset + RemindSoundCt.FramSize - 1 >= RemindSoundCt.ConstDataSize
					&& (RemindSoundCt.Request[0].Attr & REMIND_ATTR_MUTE))
//					&& Mp2Decode.dec_last_len < CFG_PARA_SAMPLES_PER_FRAME))//(RemindSoundCt.Request[0].Attr & REMIND_ATTR_CLEAR) ||
			{
				RemindSoundCt.ItemState = REMIND_ITEM_MUTE;
			}
			break;

		case REMIND_ITEM_UNMUTE:
#ifdef CFG_FUNC_RECORD_EXTERN_FLASH_EN
			if(RemindSoundCt.Isflash)
			{
				if(RemindSoundCt.ConstDataOffset >= RemindSoundCt.ConstDataSize)
				{
					AudioCoreSourceMute(REMIND_SOURCE_NUM, TRUE, TRUE);
					RemindSoundCt.Isflash = FALSE;
					DBG("Mute Remind\n");
					RemindSoundCt.ItemState = REMIND_ITEM_MUTE;

				}
			}
			else
#endif
			{
				if((RemindSoundCt.Request[0].Attr & REMIND_ATTR_CLEAR) || (RemindSoundCt.ConstDataOffset >= RemindSoundCt.ConstDataSize))
				{
					AudioCoreSourceMute(REMIND_SOURCE_NUM, TRUE, TRUE);
					#ifdef CFG_FUNC_RECORD_EXTERN_FLASH_EN
					RemindSoundCt.Isflash = FALSE;
					#endif
					DBG("Mute Remind\n");
					RemindSoundCt.ItemState = REMIND_ITEM_MUTE;

#ifdef TWS_REMIND_LOCAL_AUDIO_PLAY
					if(RemindSoundCt.EmptyIndex == 1 && RemindSoundTwsReSyncFlag)
					{
						RemindSoundTwsReSyncFlag = 0;
						BtStackServiceMsgSend(MSG_BT_STACK_TWS_SYNC_REINIT);
					}
#endif
				}
			}
		
			break;

		case REMIND_ITEM_MUTE:
#ifndef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
			if(Mp2Decode->dec_last_len < AudioCoreFrameSizeGet(AudioCoreGetNetBySourceChannelNUM(REMIND_SOURCE_NUM))
			 && RemindSoundCt.player_init == MP2_DECODE_IDLE)
#endif
			{
				RemindSoundCt.Request[0].Attr |= REMIND_ATTR_CLEAR;
				RemindSoundCt.ItemState = REMIND_ITEM_IDLE;
				RemindSoundCt.RequestUpdate = TRUE;
				AudioCoreSourceDisable(REMIND_SOURCE_NUM);
	#ifdef CFG_REMIND_MP2_DECODE_BUF_USE_MALLOC
				osPortFree(Mp2Decode);
				Mp2Decode = NULL;
	#endif
			}
			break;
	}

	return TRUE;
}

void RemindSoundInit(void)
{
#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
	memset(&RemindDecoderInMemHandle, 0, sizeof(RemindDecoderInMemHandle));
#else
	if(Mp2Decode != NULL)
		memset(Mp2Decode,0,sizeof(struct _Mp2DecodeContext));
#endif
	memset(&RemindSoundCt,0,sizeof(RemindSoundCt));

	if(!sound_clips_all_crc())
	{
		SoftFlagRegister(SoftFlagNoRemind);
	}
}

uint32_t SoundRemindItemGet(void)
{
	if(RemindSoundCt.EmptyIndex)
	{
		return RemindSoundCt.Request[0].ItemRef;
	}
	return 0;
}


void RemindSoundClearPlay(void)
{
	RemindSoundAudioPlayEnd();
	memset(&RemindSoundCt,0,sizeof(RemindSoundCt));

}

#if BT_TWS_SUPPORT
void RemindSoundClearSlavePlay(void)
{
	memset(&RemindSoundCt,0,sizeof(RemindSoundCt));
#ifndef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
	if(Mp2Decode != NULL)
		memset(Mp2Decode,0,sizeof(struct _Mp2DecodeContext));
#endif
}
#endif

void RemindSoundBufferFree(void)
{
#ifdef CFG_REMIND_MP2_DECODE_BUF_USE_MALLOC
	osPortFree(Mp2Decode);
	Mp2Decode = NULL;
#endif
}

uint8_t RemindSoundIsPlay(void)
{
//	if(RemindSoundCt.player_init == MP2_DECODE_IDLE)
//		return 0;
//	else if(RemindSoundCt.player_init == MP2_WAIT_FOR_DECODE)
//		return 1;
//	else
//		return 2;
	return RemindSoundCt.EmptyIndex;
}

uint8_t RemindSoundIsBlock(void)
{
//	if(MCUCircular_GetDataLen(&RemindSoundCt.RemindBlockCircular) >= REMIND_SOUND_ID_MIX_FLAG_LEN)
//		return 1;
	return 0;
}

bool RemindSoundIsMix(void)
{
	return RemindSoundCt.EmptyIndex && (RemindSoundCt.Request[0].Attr & REMIND_ATTR_NEED_MIX);
}

#if BT_TWS_SUPPORT
void RemindSoundSetTwsReSyncFlag(uint8_t flag)
{
	RemindSoundTwsReSyncFlag = 1;
}
#endif

#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
int32_t RemindMp3DecoderInit(void)
{
	RemindDecoderInMemHandle.addr = osPortMalloc(REMIND_DECODER_IN_FIFO_SIZE);
	if(RemindDecoderInMemHandle.addr == NULL)
		return -1;
	
	memset(RemindDecoderInMemHandle.addr, 0, REMIND_DECODER_IN_FIFO_SIZE);
	RemindDecoderInMemHandle.mem_capacity = REMIND_DECODER_IN_FIFO_SIZE;
	RemindDecoderInMemHandle.mem_len = 0;
	RemindDecoderInMemHandle.p = 0;

	return DecoderServiceInit(GetSysModeMsgHandle(),DECODER_REMIND_CHANNEL, DECODER_BUF_SIZE_MP3, DECODER_FIFO_SIZE_FOR_MP3);// decode step1
}

int32_t RemindMp3DecoderDeinit(void)
{
	DecoderServiceDeinit(DECODER_REMIND_CHANNEL);

	if(RemindDecoderInMemHandle.addr)
	{
		osPortFree(RemindDecoderInMemHandle.addr);
	}

	RemindDecoderInMemHandle.addr = NULL;
	RemindDecoderInMemHandle.mem_capacity = 0;
	RemindDecoderInMemHandle.mem_len = 0;
	RemindDecoderInMemHandle.p = 0;
	
	return 0;
}

bool RemindMp3DataRead(void)
{
	if(mv_mremain(&RemindDecoderInMemHandle) > sizeof(read_flash_buf))
	{
		uint32_t len = RemindSoundCt.ConstDataSize - RemindSoundCt.ConstDataOffset;
		
		if(len > sizeof(read_flash_buf))
			len = sizeof(read_flash_buf);
		if(RemindDataRead(RemindSoundCt.ConstDataAddr+RemindSoundCt.ConstDataOffset, read_flash_buf, len))
		{

		}
		RemindSoundCt.ConstDataOffset += len;
		mv_mwrite(read_flash_buf, len, 1, &RemindDecoderInMemHandle);
		if(RemindSoundCt.ConstDataOffset >= RemindSoundCt.ConstDataSize)
		{
			return 1;
		}
	}
	return 0;
}
#endif

#else
uint32_t SoundRemindItemGet(void)
{
	return 0;
}

void RemindSoundSyncEnable(void)
{

}

bool RemindSoundSyncRequest(uint16_t ItemRef, TWS_AUDIO_CMD CMD)
{
	return 0;
}
#endif

#ifdef CFG_FUNC_RECORD_EXTERN_FLASH_EN
#define MAX_READ_FLASH_LEN		256
extern uint32_t	rec_addr_start;
static uint32_t RemindCrcCheck(uint8_t index)
{
	uint8_t buf[MAX_READ_FLASH_LEN];
	uint16_t crc = 0;
	uint32_t offset,addr;
	REC_EXTFLASH_HEAD rec_msg;

	SpiRead(rec_addr_start + CFG_PARA_RECORDS_MAX_SIZE *(index -1),&rec_msg,sizeof(REC_EXTFLASH_HEAD));//读取flash

	if(rec_msg.RecSize > CFG_PARA_RECORDS_MAX_SIZE)
	{
		DBG("[HHH]RecSize Error %u!\n", rec_msg.RecSize);
		return 0;
	}
	offset = 0;
	addr = rec_addr_start + CFG_PARA_RECORDS_MAX_SIZE *(index -1)+ CFG_PARA_RECORDS_INFO_SIZE;
	while(offset < rec_msg.RecSize)
	{
		if((offset + MAX_READ_FLASH_LEN) < rec_msg.RecSize)
		{
			SpiRead(addr,&buf,MAX_READ_FLASH_LEN);//读取flash
			offset += MAX_READ_FLASH_LEN;
			addr += MAX_READ_FLASH_LEN;
			crc = ROM_CRC16(buf, MAX_READ_FLASH_LEN, crc);
		}
		else
		{
			SpiRead(addr,&buf,rec_msg.RecSize - offset);//读取flash
			crc = ROM_CRC16(buf, rec_msg.RecSize-offset, crc);
			offset = rec_msg.RecSize;
		}
	}

	DBG("[HHH]Crc= %x-%x,RecSize:%u\n", crc ,rec_msg.RecCrc,rec_msg.RecSize);
	if(crc != rec_msg.RecCrc)
	{
		return 0;
	}
	return rec_msg.RecSize;
}


bool RemindServiceItemReplaying()
{
	return RemindSoundCt.Isflash;
}

void RemindServiceItemClear()
{
	if(RemindSoundCt.Isflash)
	{
		RemindSoundAudioPlayEnd();
	}
	RemindSoundCt.EmptyIndex = 0;
	RemindSoundCt.Idflash = 0;
}


void RemindServiceItemRequestExt(uint8_t index)
{
	uint32_t RecFileSize;

	if(index == 0 || index > CFG_PARA_RECORDS_INDEX)
		return;
	RemindServiceItemClear();
	RecFileSize = RemindCrcCheck(index);
	APP_DBG("RecFileSize:%d\n",RecFileSize);
	if(RecFileSize)
	{
		RemindSoundCt.EmptyIndex = 1;
		RemindSoundCt.Isflash = TRUE;
		RemindSoundCt.Idflash = index;
	}
}
#endif


