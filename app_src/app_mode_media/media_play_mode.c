/**
 **************************************************************************************
 * @file    media_play_mode.c
 * @brief   
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2017-12-28 18:00:00$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include <string.h>
#include "main_task.h"
#include "media_play_api.h"
#include "media_play_mode.h"
#include "remind_sound.h"
#include "device_detect.h"
#include "audio_effect.h"
#include "audio_vol.h"
#include "dma.h"
#include "ctrlvars.h"
#include "otg_host_hcd.h"
#include "otg_device_hcd.h"
#include "otg_detect.h"
//#include "usbh_core.h"
#include "breakpoint.h"
#ifdef DUAL_CORE_EN
#include "dualcore_api.h"
#include "dualcore_dsp.h"
#endif

#if defined(CFG_APP_USB_PLAY_MODE_EN) || defined(CFG_APP_USB_HS_PLAY_MODE_EN) || defined(CFG_APP_CARD_PLAY_MODE_EN)
typedef struct _mediaPlayContext
{

	DEV_ID				Device;	//当前播放的设备
	bool				MountRequest;
	uint8_t				SourceNum;//播放器和回放使用不同通道，前者有音效
#ifdef CFG_FUNC_APP_USB_CARD_IDLE
	uint8_t				IdleStep;
#endif
}MediaPlayContext;
static MediaPlayContext*	sMediaPlayCt = NULL;
static uint32_t sPlugOutTimeOutCount = 0; 
//extern uint32_t CmdErrCnt;
extern volatile uint32_t gInsertEventDelayActTimer;

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


uint8_t MediaPlayDevice(void)
{
	if(sMediaPlayCt == NULL)
	{
		return 255;
	}
	return sMediaPlayCt->Device;
}

static  void MediaPlayerSetAudioSource()
{
	AudioCoreSourceEnable(sMediaPlayCt->SourceNum);
	//AudioCoreSourceUnmute(sMediaPlayCt->SourceNum, TRUE, TRUE);

#ifdef CFG_FUNC_DISPLAY_EN
	SendSysModeMsg(MSG_DISPLAY_SERVICE_FILE_NUM);
#endif
	
}

static void MediaPlayMsgProcess(uint16_t msgId)
{
	switch(msgId)
	{
		case MSG_REPEAT:
			APP_DBG("Media:MSG_REPEAT\n");
			MediaPlayerSwitchPlayMode(PLAY_MODE_SUM);
#ifdef CFG_FUNC_DISPLAY_EN
			SendSysModeMsg(MSG_DISPLAY_SERVICE_REPEAT);
#endif
			break;

		case MSG_FOLDER_PRE:
//			if(CmdErrCnt >= 3)//临时处理U盘异常但是U盘插着的情况
//			{
//				if(!IsMediaPlugOut())
//				{
//					SendModeKeyMsg();
//				}
//				SoftFlagRegister(SoftFlagMediaDevicePlutOut);
//				CmdErrCnt = 0;
//				return;
//			}
			MediaPlayerPreFolder();
			break;

		case MSG_FOLDER_NEXT:
//			if(CmdErrCnt >= 3)//临时处理U盘异常但是U盘插着的情况
//			{
//				if(!IsMediaPlugOut())
//				{
//					SendModeKeyMsg();
//				}
//				SoftFlagRegister(SoftFlagMediaDevicePlutOut);
//				CmdErrCnt = 0;
//				return;
//			}
			MediaPlayerNextFolder();
			break;
		case MSG_PLAY_PAUSE:
			APP_DBG("Media:MSG_PLAY_PAUSE\n");
			MediaPlayerPlayPause();
#ifdef CFG_FUNC_DISPLAY_EN
			SendSysModeMsg(MSG_DISPLAY_SERVICE_MEDIA);
#endif
			break;

		case MSG_STOP:
			APP_DBG("Media:MSG_STOP\n");
			MediaPlayerStop();
			break;

		case MSG_DECODER_SERVICE_MUTLI_EMPTY:
			APP_DBG("Media:Disk ERROR\n");
			MediaPlayerStop();
			SendModeKeyMsg();
			break;
		case MSG_DECODER_SERVICE_SONG_PLAY_END:
		//case MSG_DECODER_SERVICE_FF_END_SONG:
			if(GetDecoderState(DECODER_MODE_CHANNEL) == DecoderStatePause)//确保播放结束 ，依旧暂停态，减少stop消息交叉
			{
				APP_DBG("Media:Last end, play next\n");
#if defined(CFG_APP_USB_PLAY_MODE_EN) || defined(CFG_APP_USB_HS_PLAY_MODE_EN)
//				if(CmdErrCnt >= 3)//临时处理U盘异常但是U盘插着的情况
//				{
//					APP_DBG("cmd error >= 3\n");
//					if(!IsMediaPlugOut())
//					{
//						SendModeKeyMsg();
//					}
//					SoftFlagRegister(SoftFlagMediaDevicePlutOut);
//					CmdErrCnt = 0;
//					return;
//				}
#endif

				if(!IsMediaPlugOut())
				{
					if(gMediaPlayer->CurPlayMode==PLAY_MODE_REPEAT_OFF)
					{
						//gMediaPlayer->CurFileIndex;
						if(gMediaPlayer->CurFileIndex+1 > gMediaPlayer->TotalFileSumInDisk)
						{
							gMediaPlayer->CurFileIndex = 1;
							MediaPlayerStop();
						}
						else
						{
							DecoderMuteAndStop(DECODER_MODE_CHANNEL);
							MediaPlayerNextSong(TRUE);
						}
					}
					else
					{
						DecoderMuteAndStop(DECODER_MODE_CHANNEL);
						MediaPlayerNextSong(TRUE);
					}
				}
			}
			break;
		case MSG_NEXT:
#ifdef CFG_FUNC_REMIND_SOUND_EN
//			if(CmdErrCnt < 3)
			{
				DecoderMuteAndStop(DECODER_MODE_CHANNEL);
				if(RemindSoundServiceItemRequest(SOUND_REMIND_XIAYISOU, REMIND_PRIO_NORMAL|REMIND_ATTR_NEED_MUTE_APP_SOURCE))//RemindSound request
				{
					gMediaPlayer->RemindSoundFlag = 1;
					break;
				}
			}
#endif
		case MSG_SOFT_NEXT: 	
			APP_DBG("Media:play next\n");
//			if(CmdErrCnt >= 3)//临时处理U盘异常但是U盘插着的情况
//			{
//				if(!IsMediaPlugOut())
//				{
//					SendModeKeyMsg();
//				}
//				SoftFlagRegister(SoftFlagMediaDevicePlutOut);
//				CmdErrCnt = 0;
//				return;
//			}
			DecoderMuteAndStop(DECODER_MODE_CHANNEL);
			MediaPlayerNextSong(FALSE);
			break;
		
		case MSG_PRE:
#ifdef CFG_FUNC_REMIND_SOUND_EN
//			if(CmdErrCnt < 3)
			{
				DecoderMuteAndStop(DECODER_MODE_CHANNEL);
				if(RemindSoundServiceItemRequest(SOUND_REMIND_SHANGYIS, REMIND_PRIO_NORMAL|REMIND_ATTR_NEED_MUTE_APP_SOURCE))//RemindSound request
				{
					gMediaPlayer->RemindSoundFlag = 2;
					break;
				}
			}
#endif
		case MSG_SOFT_PRE:
			APP_DBG("Media:MSG_PRE\n");
//			if(CmdErrCnt >= 3)//临时处理U盘异常但是U盘插着的情况
//			{
//				if(!IsMediaPlugOut())
//				{
//					SendModeKeyMsg();
//				}
//				SoftFlagRegister(SoftFlagMediaDevicePlutOut);
//				CmdErrCnt = 0;
//				return;
//			}
			
			DecoderMuteAndStop(DECODER_MODE_CHANNEL);
			MediaPlayerPreSong();
			break;
#ifdef CFG_FUNC_REMIND_SOUND_EN
		case MSG_REMIND_PLAY_END:
			if(gMediaPlayer->RemindSoundFlag == 1)
			{
				SendSysModeMsg(MSG_SOFT_NEXT);
			}
			if(gMediaPlayer->RemindSoundFlag == 2)
			{
				SendSysModeMsg(MSG_SOFT_PRE);
			}
			gMediaPlayer->RemindSoundFlag = 0;
			break;
#endif
		case MSG_FF_START:
			MediaPlayerFastForward();
			APP_DBG("Media:MSG_FF_START\n");
			break;
		
		case MSG_FB_START:
			MediaPlayerFastBackward();
			APP_DBG("Media:MSG_FB_START\n");
			break;

		case MSG_FF_FB_END:
			MediaPlayerFFFBEnd();
			APP_DBG("Media:MSG_FF_FB_END\n");
			break;

#ifdef CFG_RES_IR_NUMBERKEY
		case MSG_NUM_0:
		case MSG_NUM_1:
		case MSG_NUM_2:
		case MSG_NUM_3:
		case MSG_NUM_4:
		case MSG_NUM_5:
		case MSG_NUM_6:
		case MSG_NUM_7:
		case MSG_NUM_8:
		case MSG_NUM_9:
			if(!Number_select_flag)
				Number_value = msgId - MSG_NUM_0;
			else
				Number_value = Number_value * 10 + (msgId - MSG_NUM_0);
			if(Number_value > 9999)
				Number_value = 0;
			Number_select_flag = 1;
			TimeOutSet(&Number_selectTimer, 2000);
            #ifdef CFG_FUNC_DISPLAY_EN
			SendSysModeMsg(MSG_DISPLAY_SERVICE_NUMBER);
            #endif
			break;
#endif

		case MSG_DECODER_SERVICE_UPDATA_PLAY_TIME:
			MediaPlayerTimeUpdate();
			SendPreviewPlayNextMsg();
            #ifdef CFG_FUNC_DISPLAY_EN
			SendSysModeMsg(MSG_DISPLAY_SERVICE_MEDIA);
            #endif
			break;

		case MSG_DECODER_MODE_PLAYING:
			AudioCoreSourceUnmute(sMediaPlayCt->SourceNum, TRUE, TRUE);
			break;

		case MSG_REPEAT_AB:
			APP_DBG("MSG_REPEAT_AB\n");
			MediaPlayerRepeatAB();
			break;
			
		default:
			if(gMediaPlayer->RepeatAB.RepeatFlag == REPEAT_OPENED && (IsNormalPlay()))
			{
				MediaPlayerTimerCB();
			}
			CommonMsgProccess(msgId);
			break;
	}
#ifdef CFG_RES_IR_NUMBERKEY
	if(Number_select_flag)
	{
		if(IsTimeOut(&Number_selectTimer))
		{
			if((gMediaPlayer->CurPlayMode ==PLAY_MODE_REPEAT_FOLDER) || (gMediaPlayer->CurPlayMode == PLAY_MODE_RANDOM_FOLDER))
			{
				if((Number_value <= (gMediaPlayer->PlayerFolder.FirstFileIndex + gMediaPlayer->PlayerFolder.FileNumLen - 1)) && (Number_value))
				{
					DecoderMuteAndStop(DECODER_MODE_CHANNEL);
					gMediaPlayer->CurFileIndex = Number_value;
					gMediaPlayer->CurPlayTime = 0;
					gMediaPlayer->SongSwitchFlag = 0;
					SetMediaPlayerState(PLAYER_STATE_PLAYING);
					MediaPlayerSong();
				}
			}
#ifdef FUNC_BROWSER_PARALLEL_EN
			else if(GetBrowserPlay_state()==Browser_Play_None)
			{
				APP_DBG("browser normal playing,not support ir number key select song \n");
			}
#endif
			else
			{
				if((Number_value <= gMediaPlayer->TotalFileSumInDisk) && (Number_value))
				{
					DecoderMuteAndStop(DECODER_MODE_CHANNEL);
					gMediaPlayer->CurFileIndex = Number_value;
					gMediaPlayer->CurPlayTime = 0;
					gMediaPlayer->SongSwitchFlag = 0;
					SetMediaPlayerState(PLAYER_STATE_PLAYING);
					MediaPlayerSong();
				}
			}
			Number_select_flag = 0;
		}
	}
#endif
}

/**
 * @func        MediaPlayInit
 * @brief       Media模式参数配置，资源初始化
 * @param       MessageHandle
 * @Output      None
 * @return      bool
 * @Others      任务块、Dac、AudioCore配置，数据源自DecoderService
 * @Others      数据流从Decoder到audiocore配有函数指针，audioCore到Dac同理，由audiocoreService任务按需驱动
 * Record
 */
bool MediaPlayInit(void)
{
	bool ret = FALSE;

	if(sMediaPlayCt != NULL)
	{
		return FALSE;
	}
#if defined(CFG_RES_CARD_USE)
	if ((GetSystemMode() == ModeCardAudioPlay) && ((CFG_RES_CARD_GPIO == SDIO_B0_TO_B5) || (CFG_RES_CARD_GPIO == SDIO_B0_B1_B2)))
	{
		CardPortInit(CFG_RES_CARD_GPIO);
	}
#endif
#ifndef CFG_FUNC_APP_USB_CARD_IDLE
	if(gInsertEventDelayActTimer)
	{
	#ifdef CFG_FUNC_HOST_FS_DETECT
		if( GetCurrentPlaybackDevice() == DEV_ID_USB && GetUdiscState()==DETECT_STATE_OUT)
		{
			APP_DBG(" not have usb,return\n");
			return FALSE;
		}
	#endif
	#ifdef CFG_FUNC_HOST_HS_DETECT
		if(GetCurrentPlaybackDevice() == DEV_ID_USB_HS && GetUdiscHsState()==DETECT_STATE_OUT)
		{
			APP_DBG(" not have usb,return\n");
			return FALSE;
		}
	#endif
	#ifdef CFG_FUNC_CARD_DETECT
		if(GetCurrentPlaybackDevice() == DEV_ID_SD && GetCardState()==DETECT_STATE_OUT)
		{
			APP_DBG(" not have sd,return\n");
			return FALSE;
		}
	#endif
	}
#endif
	//DMA channel
	DMA_ChannelAllocTableSet(DMA0, (uint8_t* )Dma0ChannelMap);
	DMA_ChannelAllocTableSet(DMA1, (uint8_t* )Dma1ChannelMap);
#ifdef AUDIO_STREAM_UNINTERRUPT
	if(IsResRamReallocate == TRUE)
#endif
	{
	if(!ModeCommonInit())
	{
		ModeCommonDeinit();
		DBG("Common Audio error!\n");
		return FALSE;
	}
	}

	APP_DBG("Media Play Init\n");
#ifdef AUDIO_STREAM_UNINTERRUPT
	sMediaPlayCt = (MediaPlayContext*)osPortMallocFromEnd(sizeof(MediaPlayContext));
#else
	sMediaPlayCt = (MediaPlayContext*)osPortMalloc(sizeof(MediaPlayContext));
#endif
	if(sMediaPlayCt == NULL)
	{
		return FALSE;
	}
	memset(sMediaPlayCt, 0, sizeof(MediaPlayContext));

#ifdef CFG_FUNC_RECORDER_EN
	if(IsNormalPlayback())// || GetSystemMode() == AppModeFlashFsPlayBack)
	{
		sMediaPlayCt->SourceNum = PLAYBACK_SOURCE_NUM;
	}
	else
#endif
	{
		sMediaPlayCt->SourceNum = MEDIA_PLAY_DECODER_SOURCE_NUM;
	}
	DecoderSourceNumSet(sMediaPlayCt->SourceNum, DECODER_MODE_CHANNEL);
	AudioCoreIO	AudioIOSet;
	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
	#if defined(AUDIOCORE_HWSRC_EN)
	if((AudioIOSet.HwSrcIndex = HwSrcIdleGet(sMediaPlayCt->SourceNum, TRUE)) != HWSRC_IDLE)
	{
		AudioIOSet.Adapt = HWSRC_ONLY;
	}
	else
	#endif
	{
		AudioIOSet.Adapt = SRC_ONLY;
	}
	AudioIOSet.SrcQuality = SRC_QUALITY;
	AudioIOSet.Sync = FALSE;
	AudioIOSet.Channels = 2;
	AudioIOSet.Net = AudioCoreGetNetBySourceChannelNUM(sMediaPlayCt->SourceNum);
	AudioIOSet.DataIOFunc = ModeDecoderPcmDataGet;
	AudioIOSet.LenGetFunc = ModeDecoderPcmDataLenGet;
	AudioIOSet.SampleRate = AudioCoreMixSampleRateGet(AudioIOSet.Net);//初始值
	AudioIOSet.PcmWidth	= WIDTH_16BIT;
	if(!AudioCoreSourceInit(&AudioIOSet, sMediaPlayCt->SourceNum))
	{
		DBG("mediaplay source error!\n");
		return FALSE;
	}
#ifdef AUDIO_STREAM_UNINTERRUPT
	if(IsResRamReallocate == TRUE)
#endif
	{
	//Core Process
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
	sMediaPlayCt->Device = GetCurrentPlaybackDevice();
	sMediaPlayCt->MountRequest = TRUE;
	APP_DBG("Media:Dev %d\n", sMediaPlayCt->Device);
	if(gMediaPlayer != NULL)
	{
		APP_DBG("player is reopened\n");
	}
	else
	{
#ifdef AUDIO_STREAM_UNINTERRUPT
		gMediaPlayer = osPortMallocFromEnd(sizeof(MEDIA_PLAYER));
#else
		gMediaPlayer = osPortMalloc(sizeof(MEDIA_PLAYER));
#endif
		if(gMediaPlayer == NULL)
		{
			APP_DBG("gMediaPlayer malloc error\n");
			return FALSE;
		}
	}
	memset(gMediaPlayer, 0, sizeof(MEDIA_PLAYER));
#ifdef AUDIO_STREAM_UNINTERRUPT
	gMediaPlayer->AccRamBlk = (uint8_t*)osPortMallocFromEnd(MAX_ACC_RAM_SIZE);
#else
	gMediaPlayer->AccRamBlk = (uint8_t*)osPortMalloc(MAX_ACC_RAM_SIZE);
#endif
	if(gMediaPlayer->AccRamBlk == NULL)
	{
		APP_DBG("AccRamBlk malloc error\n");
		return FALSE;
	}

#if defined(FUNC_MATCH_PLAYER_BP) && (defined(CFG_APP_USB_PLAY_MODE_EN) || defined(CFG_APP_USB_HS_PLAY_MODE_EN) || defined(CFG_APP_CARD_PLAY_MODE_EN))
	if(IsNormalPlay())
	{
		gMediaPlayer->CurPlayMode = BPDiskSongPlayModeGet();
	}
#endif

#ifdef CFG_RES_IR_NUMBERKEY
	Number_select_flag = 0;
	Number_value = 0;
#endif
	SoftFlagDeregister(SoftFlagMediaDevicePlutOut);
	SoftFlagRegister(SoftFlagMediaModeRead);

	AudioCodecHardwareSync();//update hardware config
	
#ifdef FUNC_BROWSER_PARALLEL_EN
	BrowserVarDefaultSet();
#endif

#ifdef CFG_FUNC_REMIND_SOUND_EN
	if(!SoftFlagGet(SoftFlagSysCurModeReboot) && GetSystemMode() == ModeUDiskAudioPlay)
		ret = RemindSoundServiceItemRequest(SOUND_REMIND_UPANMODE, REMIND_ATTR_NEED_MUTE_APP_SOURCE);
	if(!SoftFlagGet(SoftFlagSysCurModeReboot) && GetSystemMode() == ModeUDiskHsAudioPlay)
		ret = RemindSoundServiceItemRequest(SOUND_REMIND_UPANMODE, REMIND_ATTR_NEED_MUTE_APP_SOURCE);
	if(!SoftFlagGet(SoftFlagSysCurModeReboot) && GetSystemMode() == ModeCardAudioPlay)
		ret = RemindSoundServiceItemRequest(SOUND_REMIND_CARDMODE, REMIND_ATTR_NEED_MUTE_APP_SOURCE);
	#ifdef CFG_FUNC_RECORDER_EN
	if(!SoftFlagGet(SoftFlagSysCurModeReboot) && IsNormalPlayback())
		ret = RemindSoundServiceItemRequest(SOUND_REMIND_RECHUIFA, REMIND_ATTR_NEED_MUTE_APP_SOURCE);
	#endif
	gMediaPlayer->WaitFlag = ret;
	if(ret == FALSE)
#endif
	{
		if(IsAudioPlayerMute() == TRUE)
		{
			HardWareMuteOrUnMute();
		}
	}

#ifdef CFG_FUNC_APP_USB_CARD_IDLE
	sMediaPlayCt->IdleStep = 1;
#endif

#ifdef CFG_FUNC_RECORDER_EN
	if(IsNormalPlayback())
	{
		return DecoderServiceInit(GetSysModeMsgHandle(),DECODER_MODE_CHANNEL, DECODER_BUF_SIZE_MP3, DECODER_FIFO_SIZE_FOR_MP3);//DECODER_BUF_SIZE_MP3
	}
	else
#endif
	{
		return DecoderServiceInit(GetSysModeMsgHandle(),DECODER_MODE_CHANNEL, DECODER_BUF_SIZE, DECODER_FIFO_SIZE_FOR_PLAYER);// decode step1
	}
	return TRUE;
}

#ifdef CFG_FUNC_APP_USB_CARD_IDLE
bool MediaPlayRunIdleWait(uint16_t msgId)
{

	if((GetUdiscState() != DETECT_STATE_IN && GetSystemMode() == ModeUDiskAudioPlay) ||
		(GetUdiscHsState() != DETECT_STATE_IN && GetSystemMode() == ModeUDiskHsAudioPlay) ||
		(GetCardState() != DETECT_STATE_IN && GetSystemMode() == ModeCardAudioPlay)

		)

	{
		sMediaPlayCt->IdleStep = 0;
		if((GetSystemMode() == ModeUDiskAudioPlay && msgId == MSG_DEVICE_SERVICE_U_DISK_OUT)||
			(GetSystemMode() == ModeUDiskHsAudioPlay && msgId == MSG_DEVICE_SERVICE_U_DISK_HS_OUT)||
	   		(GetSystemMode() == ModeCardAudioPlay && msgId == MSG_DEVICE_SERVICE_CARD_OUT))
		{

			AudioCoreSourceDisable(sMediaPlayCt->SourceNum);
			DecoderServiceStop(DECODER_MODE_CHANNEL);
			MediaPlayerCloseSongFile();
			if(GetSystemMode() == ModeUDiskAudioPlay)
				f_unmount(MEDIA_VOLUME_STR_U);
			else if(GetSystemMode() == ModeUDiskHsAudioPlay)
				f_unmount(MEDIA_VOLUME_STR_UH);
			else if(GetSystemMode() == ModeCardAudioPlay)
				f_unmount(MEDIA_VOLUME_STR_C);
			SoftFlagDeregister(SoftFlagUpgradeOK);
			APP_DBG("unmount device!!!\n");
			//PortFree
			ffpresearch_deinit();
//			CmdErrCnt = 0;
			SoftFlagDeregister(SoftFlagMediaDevicePlutOut);
		}

		return TRUE;
	}

	if(sMediaPlayCt->IdleStep == 0)
	{
		if((GetSystemMode() == ModeUDiskAudioPlay && msgId == MSG_DEVICE_SERVICE_U_DISK_IN)||
			(GetSystemMode() == ModeUDiskHsAudioPlay && msgId == MSG_DEVICE_SERVICE_U_DISK_HS_IN)||
		   (GetSystemMode() == ModeCardAudioPlay && msgId == MSG_DEVICE_SERVICE_CARD_IN))
		{
			sMediaPlayCt->IdleStep = 1;
			SoftFlagRegister(SoftFlagMediaModeRead);
			sMediaPlayCt->MountRequest = TRUE;
//			CmdErrCnt = 0;
			SoftFlagDeregister(SoftFlagMediaDevicePlutOut);
		}
		return TRUE;
	}

	return FALSE;
}
#endif

void MediaPlayRun(uint16_t msgId)
{
#ifdef CFG_FUNC_REMIND_SOUND_EN
	if(gMediaPlayer->WaitFlag)
	{//等提示音播完
		if(!RemindSoundIsPlay())
		{
			gMediaPlayer->WaitFlag = FALSE;
		}
		return;
	}
#endif
#ifdef CFG_FUNC_APP_USB_CARD_IDLE
	if(MediaPlayRunIdleWait(msgId))
		return;
#endif
	if(sMediaPlayCt->MountRequest)
	{
		if(!MediaPlayerInitialize(sMediaPlayCt->Device, 1, 1))// decode step 2: include : DecoderInit(&gMediaPlayer->PlayerFile,DECODER_MODE_CHANNEL,(int32_t)IO_TYPE_FILE, (int32_t)SongFileType)
		{
#ifdef CFG_FUNC_RECORDER_EN
			extern void RecServierToParent(uint16_t id);
			if(GetSystemMode() == ModeUDiskPlayBack)
			{
				RecServierToParent(MSG_DEVICE_SERVICE_U_DISK_BACK_OUT);
				return ;
			}
			else if(GetSystemMode() == ModeUDiskHsPlayBack)
			{
				RecServierToParent(MSG_DEVICE_SERVICE_U_DISK_HS_BACK_OUT);
				return ;
			}
			else if(GetSystemMode() == ModeCardPlayBack)
			{
				RecServierToParent(MSG_DEVICE_SERVICE_CARD_BACK_OUT);
				return ;
			}
#endif

			if(!IsMediaPlugOut() && IsNormalPlay() && !SoftFlagGet(SoftFlagMediaModeExitTimeOut))
			{
				APP_DBG("Media decoder init error ,exit media init\n");
				SendModeKeyMsg();
			}

			SoftFlagRegister(SoftFlagMediaDevicePlutOut);
//			SoftFlagDeregister(SoftFlagMediaModeRead);
			sMediaPlayCt->MountRequest = FALSE;
			return ;
		}
//		SoftFlagDeregister(SoftFlagMediaModeRead);
		sMediaPlayCt->MountRequest = FALSE;
		MediaPlayerSetAudioSource();
		DecoderPlay(DECODER_MODE_CHANNEL);//  decode step 3
		APP_DBG("Media Play run\n");
	}
	if(SoftFlagGet(SoftFlagMediaDevicePlutOut))
	{
		sPlugOutTimeOutCount++;
		if(sPlugOutTimeOutCount > 600) 
		{
			SendModeKeyMsg();
			sPlugOutTimeOutCount = 0;
		}	
		return;
	}

	MediaPlayMsgProcess(msgId);	
#if (defined(FUNC_BROWSER_PARALLEL_EN) || defined(FUNC_BROWSER_TREE_EN))
	BrowserMsgProcess(msgId);
#endif
	ModeDecodeProcess();//  decode step 4

}

bool MediaPlayDeinit(void)
{
	APP_DBG("Media Play Deinit\n");
	SoftFlagDeregister(SoftFlagMediaModeExitTimeOut);

	if(sMediaPlayCt == NULL)
	{
		return TRUE;
	}
	
#ifdef CFG_FUNC_BREAKPOINT_EN
	BackupInfoUpdata(BACKUP_PLAYER_INFO);
#endif

#ifdef AUDIO_STREAM_UNINTERRUPT
	if(IsResRamRelease == TRUE)
#endif
	{
	if(IsAudioPlayerMute() == FALSE)
	{
		HardWareMuteOrUnMute();
	}
	}
//	CmdErrCnt = 0;
	SoftFlagDeregister(SoftFlagMediaModeRead);
	SoftFlagDeregister(SoftFlagMediaNextOrPrev);
	
	DecoderServiceStop(DECODER_MODE_CHANNEL);
#ifdef AUDIO_STREAM_UNINTERRUPT
	if(IsResRamRelease == TRUE)
#endif
	{
		PauseAudioCore();
		AudioCoreProcessConfig((void*)AudioNoAppProcess);
	}
	MediaPlayerCloseSongFile();

//注意此处，如果在TaskStateCreated,进入stop，它尚未init。
	AudioCoreSourceDisable(sMediaPlayCt->SourceNum);
	//AudioCoreSourceUnmute(sMediaPlayCt->SourceNum, TRUE, TRUE);
//	AudioCoreSourceDeinit(REMIND_SOURCE_NUM);
	AudioCoreSourceDeinit(sMediaPlayCt->SourceNum);
	//Kill used services
	DecoderServiceDeinit(DECODER_MODE_CHANNEL);
	if(GetCurrentPlaybackDevice() == DEV_ID_USB)
	{
	#ifdef	CFG_APP_USB_PLAY_MODE_EN
		f_unmount(MEDIA_VOLUME_STR_U);
		APP_DBG("unmount u disk\n");
	#endif
	}
	else if(GetCurrentPlaybackDevice() == DEV_ID_USB_HS)
	{
	#ifdef	CFG_APP_USB_HS_PLAY_MODE_EN
		f_unmount(MEDIA_VOLUME_STR_UH);
		APP_DBG("unmount u disk hs\n");
	#endif
	}
	else if(GetCurrentPlaybackDevice() == DEV_ID_SD)
	{
	#if (defined(CFG_APP_CARD_PLAY_MODE_EN) )
		f_unmount(MEDIA_VOLUME_STR_C);
		osMutexUnlock(SDIOMutex);
		APP_DBG("unmount sd card\n");
	#endif
	}
//	SDCardDeinit(CFG_RES_CARD_GPIO);
	//PortFree
	ffpresearch_deinit();
	if(gMediaPlayer->AccRamBlk!= NULL)
	{
		osPortFree(gMediaPlayer->AccRamBlk);
		gMediaPlayer->AccRamBlk = NULL;
	}

	extern void DecoderTimeClear(DecoderChannels DecoderChannel);
	DecoderTimeClear(DECODER_MODE_CHANNEL);

	MediaPlayerDeinitialize();
#ifdef AUDIO_STREAM_UNINTERRUPT
	if(IsResRamRelease == TRUE)
#endif
	{
		ModeCommonDeinit();
	}

	osPortFree(sMediaPlayCt);
	sMediaPlayCt = NULL;
	return TRUE;

}

#endif
