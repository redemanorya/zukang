/**
 **************************************************************************************
 * @file    bt_play_mode.c
 * @brief   
 *
 * @author  KK
 * @version V1.0.0
 *
 * $Created: 2017-12-28 18:00:00$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
//driver、lib
#include "dma.h"

//application
#include "app_config.h"
#include "app_message.h"
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
#include "communication.h"
#ifdef DUAL_CORE_EN
#include "dualcore_api.h"
#include "dualcore_dsp.h"
#endif
#if (BT_TWS_SUPPORT == ENABLE)
#include "bt_app_tws_connect.h"
#include "bt_app_tws.h"
#endif

#ifdef CFG_APP_BT_MODE_EN

typedef struct _btPlayContext
{
	BT_PLAYER_STATE		curPlayState;

	uint32_t			fastControl;//0x01: ff ; 0x02: fb

	uint32_t			btCurPlayStateMaskCnt;//从通话模式恢复到播放音乐模式,延时大概1s多时间来确认是否恢复到播放状态
}BtPlayContext;


#define BT_PLAY_DECODER_SOURCE_NUM		APP_SOURCE_NUM

#ifdef CFG_APP_BT_MODE_EN
#define BT_SOURCE_NUM				APP_SOURCE_NUM
#else
#define BT_SOURCE_NUM				BT_MIX_SOURCE_NUM
#endif
/****************************************
 * DMA CTRL 仅8通道，超出无效：
 * 常驻设备排前部,所有mode序号一致
 * *************************************/
static const uint8_t Dma0ChannelMap[16] = {
	MAP0_COMMON_FIRST
	/*在此新增独有设备,确保不超*/
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,//8 CH-default
};

static const uint8_t Dma1ChannelMap[16] = {
	MAP1_COMMON_FIRST
	/*在此新增独有设备,确保不超*/
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,//8 CH-default
};


static BtPlayContext	*BtPlayCt;

BT_A2DP_PLAYER			*a2dp_player = NULL;

#ifdef CFG_FUNC_DISPLAY_EN
	uint8_t Bt_link_bak = 0xff;
#endif


bool GetBtCurPlayState(void);


#if (BT_TWS_SUPPORT == ENABLE)
extern uint32_t gBtTwsSniffLinkLoss;
#endif

extern uint32_t gBtPlaySbcDecoderInitFlag;
extern uint16_t A2DPDataLenGet(void);
extern uint16_t A2DPDataGet(void* Buf, uint16_t Samples);
static void BtPlayRunning(uint16_t msgId);


/************************************************************************************************************
 * 蓝牙连接状态
 * 提供给客户进行灯效显示
 ************************************************************************************************************/
BT_USER_STATE GetBtUserState(void)
{
	return btManager.btuserstate;
}
void SetBtUserState(BT_USER_STATE bt_state)
{
	btManager.btuserstate = bt_state;
}

/************************************************************************************************************
 * @func        BtPlayRun
 * @brief       BtPlay模式运行函数
 * @param       msgId
 * @return      none
 ************************************************************************************************************/
void BtPlayRun(uint16_t msgId)
{
	if (BtPlayCt == NULL)
		return;
#if (BT_HFP_SUPPORT) && (BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT)
	if((SoftFlagGet(SoftFlagLeAudioSrcResume)) && (GetScoLinkConnectFlag()))
	{
		LeAudioSrcResume();
	}
#endif
	if(SoftFlagGet(SoftFlagBtCurPlayStateMask)&&(BtPlayCt->btCurPlayStateMaskCnt)&&(IsAvrcpConnected(BtCurIndex_Get())))
	{
		BtPlayCt->btCurPlayStateMaskCnt++;
		#if (BT_LINK_DEV_NUM >= 2)
		if(btManager.IndexSwFlag)
		{
			btManager.IndexSwFlag = 0;
			BtPlayCt->btCurPlayStateMaskCnt = 0;
			SoftFlagDeregister(SoftFlagBtCurPlayStateMask);
			BtCurIndex_Set(!btManager.cur_index);
			BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_PLAY);
			APP_DBG("BT memory play 3\n");
		}
		else if(BtPlayCt->btCurPlayStateMaskCnt>=15)
		{
			APP_DBG("BT memory play 1\n");
			BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_PLAY);
			BtPlayCt->btCurPlayStateMaskCnt = 0;
			SoftFlagDeregister(SoftFlagBtCurPlayStateMask);
		}
		#else
		if(GetBtCurPlayState())
		{
			APP_DBG("BT memory play 2\n");
			BtPlayCt->btCurPlayStateMaskCnt = 0;
			SoftFlagDeregister(SoftFlagBtCurPlayStateMask);
		}
		else if(BtPlayCt->btCurPlayStateMaskCnt>=15)
		{
			APP_DBG("BT memory play 1\n");
			BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_PLAY);
			BtPlayCt->btCurPlayStateMaskCnt = 0;
			SoftFlagDeregister(SoftFlagBtCurPlayStateMask);
		}
		#endif
	}

	BtPlayRunning(msgId);

	ChangePlayer();

	#ifdef CFG_FUNC_DISPLAY_EN
	if(Bt_link_bak != GetA2dpState(BtCurIndex_Get()))
	{
		if(GetA2dpState(BtCurIndex_Get()) >= BT_A2DP_STATE_CONNECTED)
			SendSysModeMsg(MSG_DISPLAY_SERVICE_BT_LINKED);
		else
			SendSysModeMsg(MSG_DISPLAY_SERVICE_BT_UNLINK);
	}
	Bt_link_bak = GetA2dpState(BtCurIndex_Get());
	#endif
	
	#if (BT_TWS_SUPPORT == ENABLE)
	if(btManager.TwsDacNeedTimeout)
	{
		if(IsTimeOut(&TwsDacOnTimer))
		{
			if(!btManager.TwsDacStatus)
			{
				tws_device_open_isr();
				APP_DBG("\n--- timeout dac on\n");
			}
			btManager.TwsDacNeedTimeout = FALSE;
		}
	}
	#endif
}

#if (BT_AUTO_PLAY_MUSIC)
static struct 
{
	uint32_t delay_cnt;
	uint8_t  state;
	uint8_t  play_state;
}auto_play = {0,0,0};
extern uint32_t gSysTick;

void BtAutoPlayMusic(void)
{
	//考虑有蓝牙后台的情况，转到BT模式也能启动自动播放
	auto_play.state 		= 1;
	auto_play.play_state 	= 0;
}

void BtAutoPlaySetAvrcpPlayStatus(uint8_t play_state)
{
	auto_play.play_state = play_state;
}

static void BtAutoPlayMusicProcess(void)
{
	if(GetA2dpState(BtCurIndex_Get()) < BT_A2DP_STATE_CONNECTED && GetAvrcpState(BtCurIndex_Get()) < BT_AVRCP_STATE_CONNECTED)
		auto_play.state = 0;
	else
	{
		switch(auto_play.state)
		{
			default:
				auto_play.state = 0;
			case 0:
				break;
			case 1:
				auto_play.delay_cnt = gSysTick;
				auto_play.state = 2;
				break;
			case 2:
				// 1秒延时以后判断当前播放状态
				if(gSysTick > (auto_play.delay_cnt + 1000))	
				{
					auto_play.state 	= 0;
					APP_DBG("BtCurPlayState %d %d\n", GetBtPlayState(),auto_play.play_state);
					if(GetBtPlayState() != BT_PLAYER_STATE_PLAYING || auto_play.play_state == AVRCP_ADV_MEDIA_PAUSED)
					{
						BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_PLAY);
						
						if (btManager.fristBtLinkState == 1)
						{
							btManager.fristBtLinkState = 0;
							osTaskDelay(50);
							BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_PLAY);
						}
					}
				}				
				break;
		}
	}
}
#endif

/***********************************************************************************
 * Bt Play 运行时消息处理
 **********************************************************************************/
extern uint32_t ChangePlayerStateGet(void);
static void BtPlayRunning(uint16_t msgId)
{
	if (BtPlayCt == NULL)
		return;
#if (BT_AUTO_PLAY_MUSIC)
	BtAutoPlayMusicProcess();
#endif
	switch(msgId)
	{
#ifdef BT_AUDIO_AAC_ENABLE
		case MSG_DECODER_SERVICE_DISK_ERROR:
			a2dp_sbc_decoder_init();
			APP_DBG("BT:MSG_DECODER_SERVICE_DISK_ERROR!!!\n");
			break;
#endif
/////////////////////////////////////////////////////////////////////////////////
		//AVRCP CONTROL
		case MSG_PLAY_PAUSE:
			if(!IsAvrcpConnected(BtCurIndex_Get()) || ChangePlayerStateGet())//ChangePlayerStateGet() :抢占时过滤掉播放暂停消息
				return;
			APP_DBG("MSG_PLAY_PAUSE = %d \n",GetBtPlayState());
			if(((GetBtPlayState() == BT_PLAYER_STATE_PLAYING)
				||(GetBtPlayState() == BT_PLAYER_STATE_FWD_SEEK)
				||(GetBtPlayState() == BT_PLAYER_STATE_REV_SEEK))
				&&(GetA2dpState(BtCurIndex_Get()) != BT_A2DP_STATE_CONNECTED))
			{
				//pause
				BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_PAUSE);
				if (btManager.fristBtLinkState == 1)
				{
					btManager.fristBtLinkState = 0;
					osTaskDelay(50);
					BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_PAUSE);
				}
				SetBtPlayState(BT_PLAYER_STATE_PAUSED);
#ifdef CFG_PARA_BT_SYNC

#if (BT_TWS_SUPPORT == ENABLE)
				if(!BtTwsStatusGet())
#endif
				AudioCoreSourceAdjust(APP_SOURCE_NUM, FALSE);


#endif
			}
			else if((GetBtPlayState() == BT_PLAYER_STATE_PAUSED) 
				|| (GetBtPlayState() == BT_PLAYER_STATE_STOP)
				|| (GetA2dpState(BtCurIndex_Get()) == BT_A2DP_STATE_CONNECTED))
			{
				//play
				BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_PLAY);
				if (btManager.fristBtLinkState == 1)
				{
					btManager.fristBtLinkState = 0;
					osTaskDelay(50);
					BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_PLAY);
				}
				SetBtPlayState(BT_PLAYER_STATE_PLAYING);
				if(gBtPlaySbcDecoderInitFlag)
				{
#ifdef CFG_PARA_BT_SYNC
#if (BT_TWS_SUPPORT == ENABLE)
				if(!BtTwsStatusGet())
#endif
					AudioCoreSourceAdjust(APP_SOURCE_NUM, TRUE);
#endif
				}
			}
			break;
		
		case MSG_NEXT:
			if(!IsAvrcpConnected(BtCurIndex_Get()))
				break;
			APP_DBG("MSG_NEXT\n");

			BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_NEXT);
			break;
		
		case MSG_PRE:
			if(!IsAvrcpConnected(BtCurIndex_Get()))
				break;
			APP_DBG("MSG_PRE\n");
			
			BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_PREV);
			break;
		
		case MSG_FF_START:
			if(BtPlayCt->fastControl != 0x01) 
			{
				BtPlayCt->fastControl = 0x01;
				BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_FF_START);
				APP_DBG("BT:MSG_FF_START\n");
			}
			break;
		
		case MSG_FB_START:
			if(BtPlayCt->fastControl != 0x02)
			{
				BtPlayCt->fastControl = 0x02;
				BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_FB_START);
				APP_DBG("BT:MSG_FB_START\n");
			}
			break;
		
		case MSG_FF_FB_END:
			if(BtPlayCt->fastControl == 0x01)
				BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_FF_END);
			else if(BtPlayCt->fastControl == 0x02)
				BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_FB_END);
			BtPlayCt->fastControl = 0;
			APP_DBG("BT:MSG_FF_FB_END\n");
			break;

		case MSG_BT_PLAY_VOLUME_SET:
#if (BT_AVRCP_VOLUME_SYNC)
			if(GetAvrcpState(BtCurIndex_Get()) != BT_AVRCP_STATE_CONNECTED)
				break;
			
			APP_DBG("MSG_BT_PLAY_VOLUME_SET\n");
			BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_VOLUME_SET);
#endif
			break;

#if (BT_HFP_SUPPORT)
		case MSG_BT_HF_REDAIL_LAST_NUM:
			if(GetHfpState(BtCurIndex_Get()) >= BT_HFP_STATE_CONNECTED)
			{
				BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_REDIAL);
			}
			break;

		case MSG_BT_HF_VOICE_RECOGNITION:
			BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_OPEN_VOICE_RECONGNITION);
			break;
#endif
		case MSG_BT_CONNECT_MODE:
			if(GetBtDeviceConnState() != BtAccessModeGeneralAccessible)
			{
				Vis_Set(BtAccessModeGeneralAccessible,__FUNCTION__);
			}
			else if(GetBtDeviceConnState() == BtAccessModeGeneralAccessible)
			{
				Vis_Set(BtAccessModeConnectableOnly,__FUNCTION__);
			}
			break;
		case MSG_BT_CONNECT_CTRL:
			if((GetA2dpState(BtCurIndex_Get()) >= BT_A2DP_STATE_CONNECTED)
				|| (GetAvrcpState(BtCurIndex_Get()) >= BT_AVRCP_STATE_CONNECTED)
#if (BT_HFP_SUPPORT)
				|| (GetHfpState(BtCurIndex_Get()) >= BT_HFP_STATE_CONNECTED)
#endif
				)
			{
				//手动断开
				BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_DISCONNECT_DEV_CTRL);
			}
			else
			{
#if (BT_TWS_SUPPORT == ENABLE)
				//手动连接
				//TWS组网过程中,先取消TWS组网,然后再发起连接
				if(BtTwsPairingState())
				{
					//取消组网流程
					BtStackServiceMsgSend(MSG_BT_STACK_TWS_PAIRING_CANCEL);
				}
				else
#endif
				BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_CONNECT_DEV_CTRL);
			}
			break;

		case MSG_BT_RST:
			if(GetBtManager()->btRstState == BT_RST_STATE_NONE)
				GetBtManager()->btRstState = BT_RST_STATE_START;
			break;
#ifdef CFG_FUNC_REMIND_SOUND_EN
		case MSG_REMIND_PLAY_END:
			/*if(GetBtPlayState()  == BT_PLAYER_STATE_STOP && (!RemindSoundIsPlay()))
			{
				if(IsAudioPlayerMute() == FALSE)
				{
					HardWareMuteOrUnMute();
				}			
				AudioCoreSourceDisable(APP_SOURCE_NUM);
			}*/
			break;
#endif
#ifdef BT_UNIFIED_CONTROL
		case MSG_BT_ENABLED:
			BtSetPowerState(true);
		break;

		case MSG_BT_DISABLED:
			BtSetPowerState(false);
		break;
#endif
		default:
			CommonMsgProccess(msgId);
			break;
	}
}

/************************************************************************************************************
 * @func        BtPlayInitRes
 * @brief       BtPlay模式参数配置，资源初始化
 * @param       MessageHandle
 * @Output      None
 * @return      bool
 * @Others      任务块、Dac、AudioCore配置，数据源自DecoderService
 * @Others      数据流从Decoder到audiocore配有函数指针，audioCore到Dac同理，由audiocoreService任务按需驱动
 * Record
 ************************************************************************************************************/
static bool BtPlayInitRes(void)
{
	//DMA channel
#ifdef AUDIO_STREAM_UNINTERRUPT
	a2dp_player = (BT_A2DP_PLAYER*)osPortMallocFromEnd(sizeof(BT_A2DP_PLAYER));
#else
	a2dp_player = (BT_A2DP_PLAYER*)osPortMalloc(sizeof(BT_A2DP_PLAYER));
#endif
	if(a2dp_player == NULL)
	{
		return FALSE;
	}
	gBtPlaySbcDecoderInitFlag = 0;
	a2dp_sbc_decoder_init();
#ifdef AUDIO_STREAM_UNINTERRUPT
	BtPlayCt = (BtPlayContext*)osPortMallocFromEnd(sizeof(BtPlayContext));
#else
	BtPlayCt = (BtPlayContext*)osPortMalloc(sizeof(BtPlayContext));
#endif
	if(BtPlayCt == NULL)
	{
		osPortFree(a2dp_player);
		a2dp_player = NULL;
		return FALSE;
	}
	memset(BtPlayCt, 0, sizeof(BtPlayContext));

//	DecoderSourceNumSet(BT_PLAY_DECODER_SOURCE_NUM,DECODER_MODE_CHANNEL);
	AudioCoreIO	AudioIOSet;
	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
#ifdef CFG_PARA_BT_SYNC
#ifdef CFG_PARA_BT_FREQ_ADJUST
	AudioIOSet.Adapt = SRC_ADJUST;
#else
	#if defined(AUDIOCORE_HWSRC_EN)
	if((AudioIOSet.HwSrcIndex = HwSrcIdleGet(BT_PLAY_DECODER_SOURCE_NUM, TRUE)) != HWSRC_IDLE)
	{
		AudioIOSet.Adapt = HWSRC_SRA;
	}
	else
	#endif
	{
		AudioIOSet.Adapt = SRC_SRA;
	}
#endif
#else //关闭微调
	AudioIOSet.Adapt = SRC_ONLY;
#endif
	AudioIOSet.SrcQuality = SRC_QUALITY;
	AudioIOSet.Sync = FALSE;
	AudioIOSet.Channels = 2;
	AudioIOSet.Net = AudioCoreGetNetBySourceChannelNUM(BT_PLAY_DECODER_SOURCE_NUM);
	AudioIOSet.DataIOFunc = A2DPDataGet;
	AudioIOSet.LenGetFunc = A2DPDataLenGet;
	AudioIOSet.SampleRate =  AudioCoreMixSampleRateGet(AudioCoreGetNetBySourceChannelNUM(BT_PLAY_DECODER_SOURCE_NUM));//初始值,待解码数据后更新
#ifdef BT_AUDIO_AAC_ENABLE
	AudioIOSet.Depth = DECODER_FIFO_SIZE_FOR_MP3/2;//AudioCoreFrameSizeGet(DefaultNet) * 2;
#else
	AudioIOSet.Depth = sizeof(a2dp_player->sbc_fifo) / 119 * 128 + sizeof(a2dp_player->last_pcm_buf) / 4;
#endif
	AudioIOSet.LowLevelCent = 50;
	AudioIOSet.HighLevelCent = 80;//90;

	AudioIOSet.PcmWidth	= WIDTH_16BIT;
	
	if(!AudioCoreSourceInit(&AudioIOSet, BT_PLAY_DECODER_SOURCE_NUM))
	{
		DBG("btplay source error!\n");
		osPortFree(BtPlayCt);
		BtPlayCt = NULL;
		osPortFree(a2dp_player);
		a2dp_player = NULL;
		return FALSE;
	}

#if (BT_AVRCP_SONG_TRACK_INFOR)
	extern void GetBtMediaInfo(void *params);
	BtAppiFunc_GetMediaInfo(GetBtMediaInfo);
#else
	BtAppiFunc_GetMediaInfo(NULL);
#endif

#if (BT_AVRCP_VOLUME_SYNC)
	//模式音量同步手机音量值
	//在无音量同步功能的手机连接成功后,不需要同步手机的音量
	if(GetBtManager()->avrcpSyncEnable)
	{
		AudioMusicVolSet(GetBtManager()->avrcpSyncVol);
	}
#endif
#ifdef AUDIO_STREAM_UNINTERRUPT
	if(IsResRamReallocate == TRUE)
#endif
	{
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
	}
	return TRUE;
}

bool BtPlayInit(void)
{
	bool		ret = TRUE;
#if (BT_TWS_SUPPORT == ENABLE)
	tws_delay = BT_TWS_DELAY_DEFAULT;
#endif

	APP_DBG("Bt Play Init\n");
	DMA_ChannelAllocTableSet(DMA0, (uint8_t* )Dma0ChannelMap);
	DMA_ChannelAllocTableSet(DMA1, (uint8_t* )Dma1ChannelMap);
#ifdef CFG_FUNC_DISPLAY_EN
	Bt_link_bak = 0xff;
#endif
#ifdef AUDIO_STREAM_UNINTERRUPT
	if(IsResRamReallocate == TRUE)
#endif
	{
		if(!ModeCommonInit())
		{
			ModeCommonDeinit();
			return FALSE;
		}
	}
#if (BT_SOURCE_SUPPORT)// source
	BtSourceBtModeInit();
#else
	if(!SoftFlagGet(SoftFlagSysCurModeReboot))
	{
		if(sys_parameter.bt_BackgroundType == BT_BACKGROUND_FAST_POWER_ON_OFF)
			BtFastPowerOn();
		else if(sys_parameter.bt_BackgroundType == BT_BACKGROUND_DISABLE)
			BtPowerOn();
	}
#endif// source

	ret = BtPlayInitRes();
#ifdef AUDIO_STREAM_UNINTERRUPT
	if(IsResRamReallocate == TRUE)
#endif
	{
		if (!ret)
		{
			ModeCommonDeinit();
			return FALSE;
		}
	}
	APP_DBG("Bt Play mode\n");

//	AudioEffect_CommunicationQueue_Send(AutoRefresh_ALL_PARA);

	AudioCodecHardwareSync();//update hardware config

	if(GetA2dpState(BtCurIndex_Get()) == BT_A2DP_STATE_STREAMING)
		SetBtPlayState(BT_PLAYER_STATE_PLAYING);
	else
		SetBtPlayState(BT_PLAYER_STATE_STOP);

	btManager.hfp_CallFalg = 0;

#if (BT_TWS_SUPPORT == ENABLE)
	if(gBtTwsSniffLinkLoss)
	{
		tws_sync_reinit();
		gBtTwsSniffLinkLoss= 0;
	}
#endif

	if(SoftFlagGet(SoftFlagBtCurPlayStateMask))
	{
		APP_DBG("BT memory play 0\n");
		BtPlayCt->btCurPlayStateMaskCnt = 1;
	}
	
#ifdef CFG_FUNC_REMIND_SOUND_EN
	if(mainAppCt.SysPrevMode != ModeBtHfPlay)
	{
		if(!SoftFlagGet(SoftFlagSysCurModeReboot) && RemindSoundServiceItemRequest(SOUND_REMIND_BTMODE, REMIND_PRIO_NORMAL) == FALSE)
		{
			if(IsAudioPlayerMute() == TRUE)
			{
				HardWareMuteOrUnMute();
			}
		}
	}
	else
#endif
	{
		if(IsAudioPlayerMute() == TRUE)
		{
			HardWareMuteOrUnMute();
		}
	}

	btManager.HfpCurIndex = 0xff;//赋初值

#ifdef BT_AUDIO_AAC_ENABLE
	DecoderServiceInit(GetSysModeMsgHandle(),DECODER_MODE_CHANNEL, DECODER_BUF_SIZE, DECODER_FIFO_SIZE_FOR_MP3);// decode step1
#endif
	return ret;
}

void BtDisconnectWaitComplete(void)
{
	uint8_t i = 0;
	BtDisconnectCtrl(TRUE);
	while(btManager.linkedNumber != 0)
	{
		//APP_DBG("...\n");
		osTaskDelay(10);
		if(i++ >= 200)
			break;
	}
}
/***********************************************************************************
 * Bt Play 反初始化
 **********************************************************************************/
bool BtPlayDeinit(void)
{
	if(BtPlayCt == NULL)
	{
		return TRUE;
	}
	
	APP_DBG("Bt Play mode Deinit\n");

//注意此处，如果在TaskStateCreating发起stop，它尚未init.
//	BtPlayerDeinitialize();
#ifdef AUDIO_STREAM_UNINTERRUPT
	if(IsResRamRelease == TRUE)
#endif
	{
		if(IsAudioPlayerMute() == FALSE)
		{
			HardWareMuteOrUnMute();
		}	
		PauseAudioCore();
		
		AudioCoreProcessConfig((void*)AudioNoAppProcess);
	}
//	AudioCoreSourceMute(BT_PLAY_DECODER_SOURCE_NUM, TRUE, TRUE);
	AudioCoreSourceUnmute(APP_SOURCE_NUM,TRUE,TRUE);
	AudioCoreSourceDisable(BT_PLAY_DECODER_SOURCE_NUM);

	AudioCoreSourceDeinit(BT_PLAY_DECODER_SOURCE_NUM);
#ifdef AUDIO_STREAM_UNINTERRUPT
	if(IsResRamRelease == TRUE)
#endif
	{
		ModeCommonDeinit();//通路全部释放
	}
	
	//AudioCoreSourceUnmute(BT_PLAY_DECODER_SOURCE_NUM, 1, 1);
	//ModeCommonDeinit(); 通路清理 待下一个模式重配
	
	if (SbcDecoderMutex != NULL)
	{
		osMutexLock(SbcDecoderMutex);
		osPortFree(a2dp_player);
		a2dp_player = NULL;
		osMutexUnlock(SbcDecoderMutex);
	}
	else
	{
		osPortFree(a2dp_player);
		a2dp_player = NULL;
	}
	
	APP_DBG("!!BtPlayCt\n");
#if (BT_SOURCE_SUPPORT)// source
	BtSourceBtModeDeinit();
#else
	if(!SoftFlagGet(SoftFlagSysCurModeReboot))
	{
		if(  sys_parameter.bt_BackgroundType == BT_BACKGROUND_FAST_POWER_ON_OFF
		  || sys_parameter.bt_BackgroundType == BT_BACKGROUND_DISABLE	)
		{
		if((GetSysModeState(ModeBtHfPlay) != ModeStateInit)
		#if (BT_TWS_SUPPORT)
		 && (GetSysModeState(ModeTwsSlavePlay) != ModeStateInit)
		#endif
		 )
			{
				BtDisconnectWaitComplete();

				if(sys_parameter.bt_BackgroundType == BT_BACKGROUND_FAST_POWER_ON_OFF)
					BtFastPowerOff();
				else
					BtPowerOff();
			}
			else
			{
				BtStackServiceWaitResume();
			}
		}
	}
#endif
	osPortFree(BtPlayCt);
	BtPlayCt = NULL;
#ifdef BT_AUDIO_AAC_ENABLE
	DecoderServiceDeinit(DECODER_MODE_CHANNEL);
	void BtDecoderDeinit(void);
	BtDecoderDeinit();
#endif
	osTaskDelay(10);// for printf 

#if (BT_TWS_SUPPORT == ENABLE)
	tws_delay = BT_TWS_DELAY_DEINIT;
#endif

	return TRUE;
}

/***********************************************************************************
 * Bt Play 播放状态
 **********************************************************************************/
void SetBtPlayState(uint8_t state)
{
	if(!BtPlayCt)
		return;

	if(BtPlayCt->curPlayState != state)
	{
		BtPlayCt->curPlayState = state;
		//APP_DBG("BtPlayState[%d]", BtPlayCt->curPlayState);
	}
}

BT_PLAYER_STATE GetBtPlayState(void)
{
	if(!BtPlayCt)
		return 0;
	else
		return BtPlayCt->curPlayState;
}

bool GetBtCurPlayState(void)
{
	if(!BtPlayCt)
		return 0;
	else
		return (BtPlayCt->curPlayState == BT_PLAYER_STATE_PLAYING);
}

//uint32_t gACBtMonitor = 0; //audio core检测:蓝牙模式下数据播空计数器
//void BtPlayACBtMonitor(uint16_t empty_flag)
//{
//	//每隔1ms查询一次,app_source 数据不足,进行登记
////	if(SOURCE_BIT_GET(AudioCore.FrameReady, APP_SOURCE_NUM) == 0)
//	if(empty_flag == 0)
//	{
//		if((GetSystemMode() == ModeBtAudioPlay)//蓝牙模式
//			&&((GetA2dpState(0) == BT_A2DP_STATE_STREAMING)||(GetA2dpState(1) == BT_A2DP_STATE_STREAMING))//正在播放音乐
//			&&(gBtPlaySbcDecoderInitFlag)//decoder初始化已经完成
//			&&(AudioCoreSourceIsEnable(APP_SOURCE_NUM))//source通路已经开启
//			)
//		{
//#if BT_TWS_SUPPORT
//			if(++gACBtMonitor>=80)//80ms播空,重置decoder
//#else
//			if(++gACBtMonitor>=30)//30ms播空,重置decoder
//#endif
//			{
//				gACBtMonitor = 0;
//				a2dp_sbc_decoder_init();
//			}
//		}
//		else
//		{
//			gACBtMonitor = 0;
//		}
//	}
//	else//一旦收到数据就清寄存器
//	{
//		gACBtMonitor = 0;
//	}
//}

#endif//#ifdef CFG_APP_BT_MODE_EN

