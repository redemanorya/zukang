//driver、lib
#include "dma.h"

//application
#include "app_config.h"
#include "bt_config.h"
#include "app_message.h"
#include "main_task.h"
#include "audio_vol.h"
#include "bt_manager.h"
#include "bt_play_api.h"
#include "audio_core_api.h"
#include "audio_effect_control.h"
#include "remind_sound.h"
#include "bt_stack_service.h"
#include "ctrlvars.h"
#include "audio_effect.h"
#include "bt_interface.h"
#include "mode_task.h"
#include "le_audio_config.h"
#include "le_public.h"
#include "bt_play_mode.h"
#ifdef DUAL_CORE_EN
#include "dualcore_api.h"
#include "dualcore_dsp.h"
#endif

#if (BT_LE_AUDIO_SUPPORT)

/****************************************
 * DMA CTRL 仅8通道，超出无效：
 * 常驻设备排前部,所有mode序号一致
 * *************************************/
static const uint8_t Dma1ChannelMap[16] = {
	MAP1_COMMON_FIRST
	/*在此新增独有设备,确保不超*/
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,//8 CH-default
};

extern uint16_t LeAudioDecoderPcmDataGet(void* Buf, uint16_t Samples);
extern uint16_t LeAudioDecoderPcmDataLenGet(void);

static uint8_t 	SystemLeEffectMode;//用于标记system当前的音效模式，LE SINK音效单独

bool LeAudioSinkPlayInit(void)
{
	bool		ret = TRUE;
	AudioCoreIO	AudioIOSet;

	DMA_ChannelAllocTableSet(DMA1,(uint8_t *)Dma1ChannelMap);
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

	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));

	AudioIOSet.Adapt = SRC_ONLY;
	AudioIOSet.Sync = TRUE;
	AudioIOSet.Channels = 2;
	AudioIOSet.Net = AudioCoreGetNetBySourceChannelNUM(APP_SOURCE_NUM);
	AudioIOSet.DataIOFunc = LeAudioDecoderPcmDataGet;
	AudioIOSet.LenGetFunc = LeAudioDecoderPcmDataLenGet;
	AudioIOSet.SampleRate =  AudioCoreMixSampleRateGet(AudioCoreGetNetBySourceChannelNUM(APP_SOURCE_NUM));//初始值,待解码数据后更新
	AudioIOSet.PcmWidth	= WIDTH_16BIT;//CFG_DEFAULT_WIDTH;


	if(!AudioCoreSourceInit(&AudioIOSet, APP_SOURCE_NUM))
	{
		DBG("LeAudio source error!\n");
		return FALSE;
	}

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

	APP_DBG("LeAudio Play mode\n");

	AudioCodecHardwareSync();//update hardware config

#if 1//ndef CFG_FUNC_REMIND_SOUND_EN
	if(IsAudioPlayerMute() == TRUE)
	{
		HardWareMuteOrUnMute();
	}
#endif
	AudioCoreSourceUnmute(APP_SOURCE_NUM,TRUE,TRUE);

	return ret;
}


bool LeAudioSinkPlayDeinit(void)
{
	extern void  LeAudioSerDecoderDeinit(void);
	APP_DBG("LeAudio mode Deinit\n");

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
	AudioCoreSourceDisable(APP_SOURCE_NUM);

	AudioCoreSourceDeinit(APP_SOURCE_NUM);
#ifdef AUDIO_STREAM_UNINTERRUPT
	if(IsResRamRelease == TRUE)
#endif
	{
		ModeCommonDeinit();//通路全部释放
	}

	LeAudioSerDecoderDeinit();
	LeAudioServiceMsgSend(MSG_TASK_PAUSE);
	return TRUE;
}
#ifdef LE_CIS_SINK
bool cis_sink_streaming;
#endif
void LeAudioSinkPlayRunning(uint16_t msgId)
{
	extern uint32_t le_bis_sink_data_len_get();
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT) && defined(LE_CIS_CONVERSATION) && defined(LE_CIS_SRC)
	if(get_decoder_reinit() && cis_sink2src_streaming)
#elif (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT) && defined(LE_CIS_SINK)
	if(get_decoder_reinit() && cis_src2sink_streaming)
#else
	if(get_decoder_reinit() && le_bis_sink_is_synced() && le_bis_sink_data_len_get() > 0)
	//if(LeAudioDecoderReinit && le_bis_sink_is_synced() && (le_bis_sink_data_len_get() > get_lc3_frame_len() * 10)) //for PTO
#endif
	{
		GIE_DISABLE();
		LeAudioSerDecoderInit();
		LeAudioServiceMsgSend(MSG_TASK_START);
//		if(AudioCore.AudioSource[1].Enable == FALSE)
		{
			AudioCoreSourceEnable(APP_SOURCE_NUM);
		}
	#ifdef CFG_PARA_BT_SYNC
		AudioCoreSourceAdjust(APP_SOURCE_NUM, TRUE);
	#endif
		AudioCoreSourceUnmute(APP_SOURCE_NUM,TRUE,TRUE);
		GIE_ENABLE();
	}

	switch(msgId)
	{
		default:
			CommonMsgProccess(msgId);
			break;
	}
}


void LeAudioSinkModeEnter(void)
{
	MessageContext		msgSend;
	extern volatile uint32_t gInsertEventDelayActTimer;

	APP_DBG("Enter LeAudioSink Mode\n");

	if(GetSysModeState(ModeLeAudioSinkPlay) == ModeStateSusend)
	{
		SetSysModeState(ModeLeAudioSinkPlay,ModeStateReady);
	}
	msgSend.msgId = MSG_DEVICE_SERVICE_LEAUDIO_SINK_CONNECTED;
	gInsertEventDelayActTimer = 0;
	MessageSend(GetMainMessageHandle(), &msgSend);
}

void LeAudioSinkModeExit(void)
{
	MessageContext		msgSend;

	APP_DBG("Exit LeAudioSink Mode\n");
	msgSend.msgId = MSG_DEVICE_SERVICE_LEAUDIO_SINK_DISCONNECT;
	MessageSend(GetMainMessageHandle(), &msgSend);

}

void LeAudioSinkModeDisable(void)
{
	if(GetSysModeState(ModeLeAudioSinkPlay) == ModeStateRunning)
	{
		AudioCoreSourceDisable(APP_SOURCE_NUM);
		// AudioDAC_DigitalMute(DAC_SEL, 1, 1);
	}
}

#endif
