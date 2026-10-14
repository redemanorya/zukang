/**
 **************************************************************************************
 * @file    bt_tws_mode.c
 * @brief   tws slave mode
 *
 * @author	KK
 * @version V1.0.0
 *
 * $Created: 2024-6-25 13:06:47$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#include "string.h"
#include "app_config.h"
#include "app_message.h"
#include "dma.h"
#include "main_task.h"
#include "audio_vol.h"
#include "bt_manager.h"
#include "bt_play_api.h"
#include "bt_play_mode.h"
#include "audio_core_api.h"
#include "remind_sound.h"
#include "bt_stack_service.h"
#include "ctrlvars.h"
#include "audio_effect.h"
#include "bt_interface.h"
#include "mode_task.h"
#include "bt_config.h"
#include "bt_tws_mode.h"

#include "bb_api.h"

#if (BT_TWS_SUPPORT == ENABLE)

#include "bt_app_tws.h"

static const uint8_t Dma0ChannelMap[] = {
	MAP0_COMMON_FIRST
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,//8 CH-default
};

static const uint8_t Dma1ChannelMap[] = {
	MAP1_COMMON_FIRST
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,//8 CH-default
};

#define TWS_SLAVE_SOURCE_NUM				APP_SOURCE_NUM
#define TWS_SLAVE_LINK_TIMEOUT				(100*60*3)

typedef struct _TwsSlavePlayContext
{
	uint8_t				runflag;
	uint8_t				umuteflag;
	TIMER				umuteTime;
	
	uint32_t			TwsBufMuteTimeout;

	uint32_t			twsSlaveLinkTimeout;
	uint32_t			twsSlaveDiscTimeout;
	uint32_t 			mode_start_tick;
	uint32_t 			mode_start_busy;

	uint8_t				SystemEffectMode;
}TwsSlavePlayContext;

static  TwsSlavePlayContext*		gTwsSlavePlayCt;
/* 供 TWS 缓冲及 bt_app_tws.c 中 extern 引用 */
uint8_t *TwsMem = NULL;
const int16_t jingyin[512]={0};
TWS_CONFIG	gTwsCfg;
extern uint32_t gSysTick;

// extern uint8_t tws_slave_cap;

uint8_t				TwsMuteDacFlag;

/**********************************************************************************************
 *
 * 
 *
 *********************************************************************************************/
static void TwsSlavePlayTimeCheck(void)
{
	if (gTwsSlavePlayCt == NULL)
		return;
	if(btManager.twsState == BT_TWS_STATE_NONE)
	{
		gTwsSlavePlayCt->twsSlaveLinkTimeout++;
		if(gTwsSlavePlayCt->twsSlaveLinkTimeout >= TWS_SLAVE_LINK_TIMEOUT)
		{
			//3分钟无连接超时退出
			gTwsSlavePlayCt->twsSlaveLinkTimeout=0;
			TwsSlaveModeExit();
		}

		if(gTwsSlavePlayCt->twsSlaveDiscTimeout)
		{
			gTwsSlavePlayCt->twsSlaveDiscTimeout--;
			if(gTwsSlavePlayCt->twsSlaveDiscTimeout == 0)
			{
				TwsSlaveModeExit();
			}
		}
	}
	else
	{
		gTwsSlavePlayCt->twsSlaveLinkTimeout = 0;
	}
}

/**********************************************************************************************
 *
 * 
 *
 *********************************************************************************************/
#if 1//(LINEIN_INPUT_CHANNEL != ANA_INPUT_CH_LINEIN3)
static uint16_t TwsSlavePlayDataGet(void* Buf, uint16_t Len)
{
	if (Buf == NULL || Len == 0)
		return 0;
	memset(Buf, 0, Len);
	return Len;
}

static uint16_t TwsSlavePlayDataLenGet(void)
{
	//待处理
	return AudioCoreFrameSizeGet(DefaultNet);//虚拟数据
}
#endif
/**********************************************************************************************
 *
 * 
 *
 *********************************************************************************************/
void TwsSlaveFifoMuteTimeSet(void)
{
	if(gTwsSlavePlayCt == NULL)
	{
		APP_DBG("gTwsSlavePlayCt is NULL! %d\n",__LINE__);
		return;
	}
	gTwsSlavePlayCt->TwsBufMuteTimeout = GetSysTick1MsCnt() + (TWS_STATRT_PLAY_FRAM * 128 * 1000 ) / CFG_TWS_SAMPLE_RATE;
	gTwsSlavePlayCt->runflag = 1;
	gTwsSlavePlayCt->umuteflag = 0;
	TimeOutSet(&gTwsSlavePlayCt->umuteTime, 0xfffffff);//设一个近似无限值。
}

/**********************************************************************************************
 *
 * 
 *
 *********************************************************************************************/
//收到unmute 命令后 如果已超过fifo深度就快速解mute，反之根据参数延时解mute。
void TwsSlaveFifoUnmuteSet(void)
{
	if(gTwsSlavePlayCt == NULL)
	{
		APP_DBG("gTwsSlavePlayCt is NULL! %d\n",__LINE__);
		return;
	}

	gTwsSlavePlayCt->umuteflag = 0;
	if(gTwsSlavePlayCt->runflag == 1 && gTwsSlavePlayCt->umuteflag == 0)
	{
		if(gTwsSlavePlayCt->TwsBufMuteTimeout)
		{
			if(GetSysTick1MsCnt() >= gTwsSlavePlayCt->TwsBufMuteTimeout + 1)
			{
				TimeOutSet(&gTwsSlavePlayCt->umuteTime, 1);//旨在触发解mute.
			}
			else
			{
				TimeOutSet(&gTwsSlavePlayCt->umuteTime, gTwsSlavePlayCt->TwsBufMuteTimeout - GetSysTick1MsCnt());
			}
			gTwsSlavePlayCt->TwsBufMuteTimeout = 0;//清理 fifo mute延时机制。
		}
	}
}

/**********************************************************************************************
 * @brief       
 * @param       
 * @return      
 *********************************************************************************************/
bool TwsSlavePlayResMalloc(uint16_t SampleLen)
{
	//InCore1 buf
	return TRUE;
}

/**********************************************************************************************
 * @func        TwsSlavePlay_Init
 * @brief       TwsSlave模式参数配置，资源初始化
 * @param       MessageHandle 
 * @Output      None
 * @return      bool
 * @Others      任务块、Adc、Dac、AudioCore配置
 * @Others      数据流从Adc到audiocore配有函数指针，audioCore到Dac同理，由audiocoreService任务按需驱动
 *********************************************************************************************/
bool TwsSlavePlayInit(void)
{
	APP_DBG("TwsSlave:init\n");

	//dma channel map
//	DMA_ChannelAllocTableSet((uint8_t *)DmaChannelMap);//TwsSlave
	DMA_ChannelAllocTableSet(DMA0, (uint8_t* )Dma0ChannelMap);
	DMA_ChannelAllocTableSet(DMA1, (uint8_t* )Dma1ChannelMap);

	//Task & App Config
	if(gTwsSlavePlayCt == NULL)
	{
		gTwsSlavePlayCt = (TwsSlavePlayContext*)osPortMalloc(sizeof(TwsSlavePlayContext));
		if(gTwsSlavePlayCt == NULL)
		{
			return FALSE;
		}
	}
	memset(gTwsSlavePlayCt, 0, sizeof(TwsSlavePlayContext));
	
	if(!TwsSlavePlayResMalloc(AudioCoreFrameSizeGet(DefaultNet)))
	{
		APP_DBG("TwsSlavePlay Res Error!\n");
		osPortFree(gTwsSlavePlayCt);
		gTwsSlavePlayCt = NULL;
		return FALSE;
	}

//	gTwsSlavePlayCt->SystemEffectMode = mainAppCt.EffectMode;	//保存EffectMode，退出tws slave模式以后需要恢复
//	mainAppCt.EffectMode = EFFECT_MODE_TWS_SLAVE;

	if(!ModeCommonInit())
	{
		ModeCommonDeinit();
		osPortFree(gTwsSlavePlayCt);
		gTwsSlavePlayCt = NULL;
		return FALSE;
	}

	//Audio init
	//note Soure0.和sink0已经在main app中配置，不要随意配置
	//Core Soure1.Para
	AudioCoreIO	AudioIOSet;
	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
	AudioIOSet.Adapt = STD;
	AudioIOSet.Sync = FALSE;
	AudioIOSet.Channels = 2;
	AudioIOSet.Net = AudioCoreGetNetBySourceChannelNUM(TWS_SLAVE_SOURCE_NUM);
	AudioIOSet.DataIOFunc = TwsSlavePlayDataGet;
	AudioIOSet.LenGetFunc = TwsSlavePlayDataLenGet;//待修缮;
	AudioIOSet.PcmWidth = WIDTH_24BIT;

	//AudioIOSet.
	if(!AudioCoreSourceInit(&AudioIOSet, TWS_SLAVE_SOURCE_NUM))
	{
		DBG("twsplay source error!\n");
		ModeCommonDeinit();
		osPortFree(gTwsSlavePlayCt);
		gTwsSlavePlayCt = NULL;
		return FALSE;
	}

#ifdef CFG_FUNC_ROBOEFFECT_EN
	AudioCoreProcessConfig((void*)AudioMusicProcess);
#else
	AudioCoreProcessConfig((void*)AudioBypassProcess);
#endif

#ifdef DUAL_CORE_EN
	DualCoreDspAddrInit();
#endif
#ifdef CFG_FUNC_ROBOEFFECT_EN
	AudioEffect_Params_Sync(&EffectParambin);
#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
	AudioEffect_Params_Sync(&EffectParambin2);
#endif
#endif
	gTwsSlavePlayCt->runflag = 0;
	gTwsSlavePlayCt->umuteflag = 0;
	gTwsSlavePlayCt->TwsBufMuteTimeout = 0;

#ifdef TWS_AUDIO_BUFFER_DYNAMIC_MALLOC
	if(tws_mem_check())
	{
		TWS_Params_Init();
		if (TwsMem == NULL)
		{
			APP_DBG("TwsSlave: TWS buffer malloc failed\n");
			AudioCoreSourceDeinit(TWS_SLAVE_SOURCE_NUM);
			ModeCommonDeinit();
			osPortFree(gTwsSlavePlayCt);
			gTwsSlavePlayCt = NULL;
			return FALSE;
		}
	}
	else
		return TRUE;
#endif
	extern void TwsAuidoSyncInit(void);
	TwsAuidoSyncInit();
		
#if (TWS_PAIRING_MODE != CFG_TWS_PEER_SLAVE)
	if(btManager.twsState != BT_TWS_STATE_CONNECTED)
	{
		TwsSlaveModeExit();
		APP_DBG("TwsSlavePlayInit: Tws Disconnect!\n");
	}
#endif

	gTwsSlavePlayCt->runflag = 1;
	gTwsSlavePlayCt->umuteflag = 0;
	TimeOutSet(&gTwsSlavePlayCt->umuteTime, 200);
	
	
	AudioCoreSourceEnable(TWS_SLAVE_SOURCE_NUM);

#ifdef CFG_AUTO_ENTER_TWS_SLAVE_MODE
	if(btManager.twsState == BT_TWS_STATE_CONNECTED && (!IsIdleModeReady()))
	{
		tws_audio_connect_cmd();
	}
#endif

	APP_DBG("TwsSlave:run\n");
	gTwsSlavePlayCt->mode_start_tick = gSysTick;
	gTwsSlavePlayCt->mode_start_busy = 1;
	
	if(IsAudioPlayerMute() == TRUE)
	{
	 	HardWareMuteOrUnMute();
	}
	//BtAccessModeUpdate(0x02);
	return TRUE;
}

/**********************************************************************************************
 * @func        TwsSlavePlayEntrance
 * @brief       模式执行主体
 * @param       void * param  
 * @Output      None
 * @return      None
 * @Others      模式建立和结束过程
 *********************************************************************************************/

void TwsSlavePlayRun(uint16_t msgId)
{
#ifdef CFG_FUNC_AUDIO_EFFECT_EN
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
	if(EffectParamFlashUpdataFlag == 1)
	{
		EffectParamFlashUpdataFlag = 0;
		EffectParamFlashUpdata();
	}
#endif
#endif
	if (gTwsSlavePlayCt == NULL)
		return;
	if(gTwsSlavePlayCt->mode_start_busy == 1)
	{
		if((gSysTick - gTwsSlavePlayCt->mode_start_tick) > 3000)
		{
			if(tws_audio_state_get() == TWS_DISCONNECT)
			{
				tws_sync_reinit();
				gTwsSlavePlayCt->mode_start_tick = gSysTick;
				gTwsSlavePlayCt->mode_start_busy = 2;
			}
			else
			{
				gTwsSlavePlayCt->mode_start_busy = 0;
			}
		}
	}
	if(gTwsSlavePlayCt->mode_start_busy == 2)
	{
		if((gSysTick - gTwsSlavePlayCt->mode_start_tick) > 3000)
		{
			if(tws_audio_state_get() != TWS_DISCONNECT)
			{
				gTwsSlavePlayCt->mode_start_busy = 0;
			}
		}
	}
	switch(msgId)//警告：在此段代码，禁止新增提示音插播位置。
	{
		case MSG_BT_TWS_DISCONNECT:
			gTwsSlavePlayCt->twsSlaveDiscTimeout = 5;//50ms自动退出
			break;

		case MSG_BT_TWS_LINKLOSS:
			gTwsSlavePlayCt->twsSlaveDiscTimeout = 1000;//10s回连不成功自动退出
//			BtStartReconnectTws(1, 1);
			break;
			
		default:
			CommonMsgProccess(msgId);
			break;
	}

	if(gTwsSlavePlayCt->umuteflag == 0)
	{
		if(gTwsSlavePlayCt->runflag == 1)
		{
			if(IsTimeOut(&gTwsSlavePlayCt->umuteTime))
			{
				gTwsSlavePlayCt->umuteflag = 1;
				if(IsAudioPlayerMute() == TRUE)//主机发起mute阶段不解mute
				{
					HardWareMuteOrUnMute();
				}
			}
		}
	}

	TwsSlavePlayTimeCheck();
}

/*********************************************************************************************
 * @func        TwsSlavePlayDeinit
 * @brief       
 * @param       void
 * @Output      None
 * @return      None
 * @Others      
 * Record
 *********************************************************************************************/
bool TwsSlavePlayDeinit(void)
{
	#ifdef TWS_SLAVE_MODE_SWITCH_EN
	if(btManager.twsState == BT_TWS_STATE_CONNECTED)
	{
		APP_DBG("tws disconnect\n");
		tws_link_disconnect();
	}
	#endif
	
	if(gTwsSlavePlayCt == NULL)
	{
		return TRUE;
	}
	APP_DBG("Bt Tws Slave Play Deinit\n");
	
	if(IsAudioPlayerMute() == FALSE)
	{
		HardWareMuteOrUnMute();
	}
	PauseAudioCore();

	//注意：AudioCore父任务调整到mainApp下，此处只关闭AudioCore通道，不关闭任务
	AudioCoreProcessConfig((void*)AudioNoAppProcess);
	AudioCoreSourceDisable(TWS_SLAVE_SOURCE_NUM);
	AudioCoreSourceDeinit(TWS_SLAVE_SOURCE_NUM);

	APP_DBG("Tws Slave:Kill Ct\n");

	ModeCommonDeinit();//通路全部释放
//	mainAppCt.EffectMode = gTwsSlavePlayCt->SystemEffectMode;	//恢复EffectMode
	
	osPortFree(gTwsSlavePlayCt);
	gTwsSlavePlayCt = NULL;

	return TRUE;
}


/*****************************************************************************************
 * Enter or Exit Tws Slave Mode
 ****************************************************************************************/
void TwsSlaveModeEnter(void)
{
	MessageContext		msgSend;
	
	APP_DBG("Enter Tws Slave Mode\n");
	
	if(GetSysModeState(ModeTwsSlavePlay) == ModeStateSusend)
	{
		SetSysModeState(ModeTwsSlavePlay,ModeStateReady);
	}
	msgSend.msgId = MSG_DEVICE_SERVICE_TWS_SLAVE_CONNECTED;
	MessageSend(GetMainMessageHandle(), &msgSend);
}
/*****************************************************************************************
 * 
 ****************************************************************************************/
void TwsSlaveModeExit(void)
{
	MessageContext		msgSend;
	if(btManager.twsState == BT_TWS_STATE_CONNECTED)
	{
		//tws_link_disconnect();
		BtTwsDisconnectApi();
	}

	APP_DBG("Exit Tws Slave Mode\n");
	msgSend.msgId = MSG_DEVICE_SERVICE_TWS_SLAVE_DISCONNECT;
	MessageSend(GetMainMessageHandle(), &msgSend);
//	tws_time_init();
	tws_play = BT_TWS_PLAY_REINIT;
	// BtSetFreqTrim(tws_slave_cap);
}




/*****************************************************************************************
 * TWS params init
 ****************************************************************************************/
void TWS_Params_Init(void)
{
	uint32_t tws_mem;
	//uint8_t*p;
	if(TwsMem == NULL)
	{
		gTwsCfg.AudioMode    = TWS_M_L__S_R;


		tws_mem = tws_mem_size(TWS_STATRT_PLAY_FRAM,gTwsCfg.AudioMode);
		TwsMem = (uint8_t*)osPortMalloc(tws_mem);
		if(TwsMem == NULL)
		{
			printf("tws mem malloc error\n");
			return;
		}
		memset(TwsMem, 0, tws_mem);
		printf("tws_mem_size:%lu\n",tws_mem);
		tws_mem_set(TwsMem);
		gTwsCfg.PairMode    = 0;
		gTwsCfg.IsRemindSyncEn = 0;//0:提示音不发送给slave；1:提示音发送给slave同步播放提示音

		#ifdef TWS_AUDIO_BUFFER_DYNAMIC_MALLOC
		extern void tws_delay_set(uint8_t cnt);
		tws_delay_set(TWS_DELAY_DEF);
		#endif

		gTwsCfg.TwsEnable = 1;
	}
}

#ifdef TWS_AUDIO_BUFFER_DYNAMIC_MALLOC
void TWS_Params_DeInit(void)
{
	extern void tws_mem_free(void);
	if(TwsMem != NULL)
	{
		gTwsCfg.TwsEnable = 0;

		osPortFree(TwsMem);
		TwsMem = NULL;
		tws_mem_free();
	}

}

void TWS_TwsEnable(uint8_t sw)
{
	gTwsCfg.TwsEnable = sw;
}

uint8_t TWS_TwsEnableGet(void)
{
	return gTwsCfg.TwsEnable;
}
#endif

uint16_t TwsSinkDataSet(void* Buf, uint16_t Len)
{
	 tws_music_pcm_process(Buf, Len, APP_SOURCE_NUM);
	return Len;
}

uint16_t TwsSinkSpaceLenGet(void)
{
	return AudioCore.NetCt[AudioCoreGetNetBySinkChannelNUM(TWS_SINK_NUM)].FrameSize;//虚拟数据
}

uint16_t AudioDAC0DataSet_tws(void* Buf, uint16_t Len)
{
#if 0
	bool gie_ret = GIE_STATE_GET();

	GIE_DISABLE();
	DAC_DATASET_SEL(Buf, TwsDeviceSinkSync(Len));//AudioDAC0_DataSet(Buf, TwsDeviceSinkSync(Len));
	if(gie_ret)
	{
		GIE_ENABLE();
	}
#else
	bool gie_ret = GIE_STATE_GET();

	GIE_DISABLE();
	//DAC_DATASET_SEL(Buf, TwsDeviceSinkSync(Len));//AudioDAC0_DataSet(Buf, TwsDeviceSinkSync(Len));
	uint16_t Length = TwsDeviceSinkSync(Len);

	if(Buf == NULL)
	{
		if(gie_ret)
		{
			GIE_ENABLE();
		}
		return 0;
	}
#if 0//ndef CFG_AUDIO_WIDTH_24BIT
	{
		Length = Length * 4;
		if(TwsMuteDacFlag)
		{
			memset(Buf,0,Length);
		}
 		DMA_CircularDataPut(DAC_DMA_PERI_ID_SEL, Buf, Length & 0xFFFFFFFC);
	}
#else
	{
		Length = Length * 8;
		if(TwsMuteDacFlag)
		{
			memset(Buf,0,Length);
		}
		DMA_CircularDataPut(TWS_DAC_DMA_PERI_ID_SEL, Buf, Length & 0xFFFFFFF8);
	}
#endif

	if(gie_ret)
	{
		GIE_ENABLE();
	}
#endif
	return 0;
}

uint16_t AudioDAC1DataSet_tws(void* Buf, uint16_t Len)
{
#if 0
	bool gie_ret = GIE_STATE_GET();

	GIE_DISABLE();
	DAC_DATASET_SEL(Buf, TwsDeviceSinkSync(Len));//AudioDAC0_DataSet(Buf, TwsDeviceSinkSync(Len));
	if(gie_ret)
	{
		GIE_ENABLE();
	}
#else
	bool gie_ret = GIE_STATE_GET();

	GIE_DISABLE();
	//DAC_DATASET_SEL(Buf, TwsDeviceSinkSync(Len));//AudioDAC0_DataSet(Buf, TwsDeviceSinkSync(Len));
	uint16_t Length = TwsDeviceSinkSync(Len);

	if(Buf == NULL)
	{
		if(gie_ret)
		{
			GIE_ENABLE();
		}
		return 0;
	}
#if 0//ndef CFG_AUDIO_WIDTH_24BIT
	{
		Length = Length * 4;
		if(TwsMuteDacFlag)
		{
			memset(Buf,0,Length);
		}
 		DMA_CircularDataPut(DAC_DMA_PERI_ID_SEL, Buf, Length & 0xFFFFFFFC);
	}
#else
	{
		Length = Length * 4;
		if(TwsMuteDacFlag)
		{
			memset(Buf,0,Length);
		}
		DMA_CircularDataPut(TWS_DAC_DMA_PERI_ID_SEL, Buf, Length & 0xFFFFFFFC);
	}
#endif

	if(gie_ret)
	{
		GIE_ENABLE();
	}
#endif
	return 0;
}

__attribute__((section(".tws_sync_code")))
uint16_t tws_device_space_len(void)
{
//#if  (TWS_AUDIO_OUT_PATH	== TWS_IIS0_OUT)
//	return AudioI2S0_TX_DataLenGet();
//#elif(TWS_AUDIO_OUT_PATH	== TWS_IIS1_OUT)
//	return AudioI2S1_TX_DataLenGet();
//#elif(TWS_AUDIO_OUT_PATH	== TWS_DAC0_OUT)
//	return AudioDAC0_DataSpaceLenGet();
//#endif
	return TWS_DAC_LENGET_SEL();//AudioDAC0_DataSpaceLenGet();
}

void tws_dac_mute_flag_set(uint8_t flag)
{
	printf("#tws_dac_mute_flag_set %d\n",flag);
	if(flag != TwsMuteDacFlag)
	{
		TwsMuteDacFlag = flag;
	}
}

#endif

