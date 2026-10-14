/**
 **************************************************************************************
 * @file    audio_vol.c
 * @brief   audio syetem vol set here
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2016-1-7 15:42:47$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include "type.h"
#include "dac.h"
#include "i2s.h"
#include "delay.h"
#include "audio_adc.h"
#include "adc_interface.h"
#include "dac_interface.h"
#include "debug.h"
#include "timeout.h"

#include "app_config.h"
#include "app_message.h"
#include "audio_vol.h"
#include "audio_core_api.h"
#include "main_task.h"
#include "remind_sound.h"
#include "breakpoint.h"
#include "communication.h"
#ifdef CFG_FUNC_HEADPHONE_EN
#include "components/headphone/headphone.h"
#endif
//Port#include "bt_manager.h"
//Port#include "bt_play_mode.h"
//Port#include "bt_app_avrcp_deal.h"
#if (BT_HFP_SUPPORT)
	//Port#include "bt_hf_mode.h"
	#include "bt_hf_api.h"
#endif
#include "ctrlvars.h"
#ifdef CFG_FUNC_RTC_EN
	#include "rtc_ctrl.h"
#endif

#ifdef  CFG_APP_HDMIIN_MODE_EN
	#include "hdmi_in_api.h"
	extern HDMIInfo *gHdmiCt;
#endif

uint8_t gBtAbsVolTable[17]={
	0x00, 0x07, 0x0f, 0x17, 0x1f, 0x27, 0x2f, 0x37, 0x3f, 0x47,
	0x4f, 0x57, 0x5f, 0x67, 0x6f, 0x77, 0x7f};

uint8_t gBtAbsVolSetTable[17]={
	0x00, 0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38, 0x40, 0x48,
	0x50, 0x58, 0x60, 0x68, 0x70, 0x78, 0x7f};

const int16_t AudioModeDigitalGianTable[][2]=
{
	{ModeBtAudioPlay,		0/*dB*/},
	{ModeUDiskAudioPlay,	0/*dB*/},
	{ModeUDiskHsAudioPlay,	0/*dB*/},
	{ModeUsbDevicePlay,		0/*dB*/},
	{ModeUsbFsDevicePlay,	0/*dB*/},
	{ModeCardAudioPlay,		0/*dB*/},
	{ModeLineAudioPlay,		0/*dB*/},
	{ModeI2SInAudioPlay,	0/*dB*/},
	{ModeOpticalAudioPlay,	0/*dB*/},
	{ModeCoaxialAudioPlay,	0/*dB*/},
};

#ifdef CFG_APP_BIS_BT_DUAL_CHIP_MODE_EN
void Audio_BisDACSwtichChannal(uint8_t channal)
{
	int16_t Swtich;
	if(channal == 1)
	{
		Swtich = 0;
	}
	else
	{
		Swtich = 1;
	}
	#ifdef CFG_FUNC_ROBOEFFECT_EN
	AudioEffect_ParamSet(AudioEffect.context_memory, GetBisEffectRouteSelector(), 0, (int16_t *)&Swtich);
	#endif
}
#endif

#ifdef CFG_FUNC_ROBOEFFECT_EN
/* VOL_PRESET 宏定义见 audio_vol.h */

//extern uint8_t GetEffectControlIndex(AUDIOEFFECT_EFFECT_CONTROL type);
//app通路数字预增益处理
//app通路数字预增益处理
void AudioAPPDigitalGianProcess(AudioEffectParambinContext *AudioEffectParambin, SysModeNumber AppMode)
{
	uint32_t i;
	int16_t preGain = 0;

	for(i = 0; i < sizeof(AudioModeDigitalGianTable)/(sizeof(int16_t)*2); i++)
	{
		if(AppMode == AudioModeDigitalGianTable[i][0])
		{
			preGain = AudioModeDigitalGianTable[i][1] * 100;
			DBG("CurMode preGain = %d dB\n", AudioModeDigitalGianTable[i][1]);
			AudioEffect_ParamSet(AudioEffectParambin->context_memory, AudioEffect_Parambin_GetEffectAddr(AudioEffectParambin, EFFECT_PREGAIN), INDEX_GAIN_CONTROL_GAIN, &preGain);
			break;
		}
	}

}

//获取上位机设置的最大音量等级
uint32_t AudioVol_MaxLevel_Get(AudioEffectParambinContext *AudioEffectParambin, AUDIO_EFFECT_NAME_NUM effect_index, AUDIO_EFFECT_PRESET_NUM preset_index)
{
	uint32_t step = 0;

	if (preset_index == PRESET_EQ_BASS_VOL){
		AudioEffect_PresetGet(AudioEffectParambin, effect_index, INDEX_EQ_FILTER2_GAIN, PRESET_EQ_BASS_VOL, &step);
	}else if (preset_index == PRESET_EQ_TREB_VOL){
		AudioEffect_PresetGet(AudioEffectParambin, effect_index, INDEX_EQ_FILTER6_GAIN, PRESET_EQ_TREB_VOL, &step);
	}else if (preset_index == PRESET_ECHO_delay){
		AudioEffect_PresetGet(AudioEffectParambin, effect_index, INDEX_ECHO_DELAY, PRESET_ECHO_delay, &step);
	}else if (preset_index == PRESET_ECHO_attenuation){
		AudioEffect_PresetGet(AudioEffectParambin, effect_index, INDEX_ECHO_ATTENUATION, PRESET_ECHO_attenuation, &step);
	}else if (preset_index == PRESET_reverb_decay){
		AudioEffect_PresetGet(AudioEffectParambin, effect_index, INDEX_REVERB_PLATE_DECAY, PRESET_reverb_decay, &step);
	}else if (preset_index == PRESET_reverb_wetdry){
		AudioEffect_PresetGet(AudioEffectParambin, effect_index, INDEX_REVERB_PLATE_WETDRYMIX, PRESET_reverb_wetdry, &step);
	}else if (preset_index == PRESET_GAIN_VOL){
		AudioEffect_PresetGet(AudioEffectParambin, effect_index, INDEX_GAIN_CONTROL_GAIN, PRESET_GAIN_VOL, &step);
	}else if (preset_index == PRESET_GAIN_BT_VOL){
		AudioEffect_PresetGet(AudioEffectParambin, effect_index, INDEX_GAIN_CONTROL_GAIN, PRESET_GAIN_BT_VOL, &step);
	}else{
		DBG("preset_index error\n");
	}

	if (step <= 1)
	{
		DBG("volume level error! 请检查上位机低音音量等级设置");
		return 0;
	}
	return (step - 1); // 上位机从1开始计算等级，sdk是从0开始的，所以这里减 1
}

// 获取当前音量最大级数，由Flash preset表决定
uint32_t AudioVol_GetMaxLevel(void)
{
	if (EffectParambin.context_memory == NULL)
		return 0;
	return AudioVol_MaxLevel_Get(&EffectParambin, EFFECT_MUSICGAIN, VOL_PRESET);
}

#endif

uint8_t BtAbsVolume2VolLevel(uint8_t absValue)
{
	uint8_t i;
	for(i=0;i<=16;i++)
	{
		if(absValue == gBtAbsVolTable[i])
			return i;

		if(absValue < gBtAbsVolTable[i])
			return (i-1);
	}
	if(i>16) i=16;
	return i;
}

uint8_t BtLocalVolLevel2AbsVolme(uint8_t localValue)
{
	return gBtAbsVolSetTable[localValue];
}

void HardWareMuteOrUnMute(void)
{
	mainAppCt.gSysVol.MuteFlag = !mainAppCt.gSysVol.MuteFlag;
#ifdef CFG_FUNC_HEADPHONE_EN
	HeadphoneSetOutput(!mainAppCt.gSysVol.MuteFlag);
#endif
#if (BT_TWS_SUPPORT == ENABLE)
	if(mainAppCt.gSysVol.MuteFlag)
	{
		tws_master_mute_send(TRUE);
	}
	else
	{
		tws_master_mute_send(FALSE);
	}
#endif
#ifdef CFG_RES_DAC_MONITOR_EN
	if(AudioCoreSinkIsEnable(AUDIO_DAC_SINK_NUM))
	{
#ifdef CFG_HP_DAC0_BOOT_MUTE_TEST
		AudioDAC_SoftMute(DAC_SEL, TRUE, TRUE);
#else
		AudioDAC_SoftMute(DAC_SEL, mainAppCt.gSysVol.MuteFlag, mainAppCt.gSysVol.MuteFlag);
#endif
	}
#endif
#if (CFG_RES_MIC_SELECT) && defined(CFG_FUNC_MIC_EN)
	if(AudioCoreSourceIsEnable(MIC_SOURCE_NUM))
	{
		AudioADC_SoftMute(MIC_ADC_SEL, mainAppCt.gSysVol.MuteFlag, mainAppCt.gSysVol.MuteFlag);
	}
#endif
	if (mainAppCt.gSysVol.MuteFlag)
	{
		DBG("HardWare Mute\n");
	}
	else
	{
		DBG("HardWare unMute\n");
	}
}

bool IsAudioPlayerMute(void)
{
	return mainAppCt.gSysVol.MuteFlag;
}

uint8_t AudioMusicVolGet(void)
{
	return mainAppCt.gSysVol.AudioSourceVol[APP_SOURCE_NUM];
}

void AudioMusicVolUp(AudioEffectParambinContext *AudioEffectParambin)
{
	bool volume = FALSE;
	if(IsAudioPlayerMute() == TRUE)
	{
		HardWareMuteOrUnMute();
	}
	
#if (defined(CFG_APP_BT_MODE_EN) && (BT_HFP_SUPPORT))
	if(GetSystemMode() == ModeBtHfPlay)
	{
		if(mainAppCt.HfVolume < AudioVol_GetMaxLevel())
		{
			mainAppCt.HfVolume++;
			#ifdef CFG_FUNC_BREAKPOINT_EN
			BackupInfoUpdata(BACKUP_SYS_INFO);
			#endif
		}
	    mainAppCt.gSysVol.AudioSourceVol[APP_SOURCE_NUM] = mainAppCt.HfVolume;
	}
	else
#endif
	{
		if(mainAppCt.MusicVolume < AudioVol_GetMaxLevel())
		{
			volume = TRUE;
			mainAppCt.MusicVolume++;

			#if (BT_TWS_SUPPORT == ENABLE)
			tws_vol_send(mainAppCt.MusicVolume, IsAudioPlayerMute());
			#endif

			#ifdef CFG_FUNC_BREAKPOINT_EN
			BackupInfoUpdata(BACKUP_SYS_INFO);
			#endif
		}

		if(mainAppCt.MusicVolume >= AudioVol_GetMaxLevel())
		{
			#ifdef CFG_FUNC_REMIND_SOUND_EN
			if(!RemindSoundIsPlay())
			{
				RemindSoundServiceItemRequest(SOUND_REMIND_VOLMAX, REMIND_PRIO_NORMAL);
			}
			#endif
		}
	    mainAppCt.gSysVol.AudioSourceVol[APP_SOURCE_NUM] = mainAppCt.MusicVolume;
	}
#ifdef CFG_FUNC_ROBOEFFECT_EN
	APP_DBG("APP_SOURCE VOL = %d\n", mainAppCt.gSysVol.AudioSourceVol[APP_SOURCE_NUM]);
	AudioEffect_PresetSet(AudioEffectParambin, EFFECT_MUSICGAIN, INDEX_GAIN_CONTROL_GAIN, VOL_PRESET, mainAppCt.gSysVol.AudioSourceVol[APP_SOURCE_NUM]);
#endif
#ifdef CFG_FUNC_REMIND_SOUND_EN
	#if CFG_PARAM_FIXED_REMIND_VOL
	mainAppCt.gSysVol.AudioSourceVol[REMIND_SOURCE_NUM] = CFG_PARAM_FIXED_REMIND_VOL;
	#else
	mainAppCt.gSysVol.AudioSourceVol[REMIND_SOURCE_NUM] = mainAppCt.MusicVolume;
	#endif
	APP_DBG("REMIND_SOURCE_NUM vol = %d\n", mainAppCt.gSysVol.AudioSourceVol[REMIND_SOURCE_NUM]);
	#ifdef CFG_FUNC_ROBOEFFECT_EN
	AudioEffect_PresetSet(&EffectParambin, EFFECT_REMIND_GAIN, INDEX_GAIN_CONTROL_GAIN, VOL_PRESET, mainAppCt.gSysVol.AudioSourceVol[REMIND_SOURCE_NUM]);
	#endif
#endif
#ifdef CFG_FUNC_RECORDER_EN
	mainAppCt.gSysVol.AudioSourceVol[PLAYBACK_SOURCE_NUM] = mainAppCt.MusicVolume;
	APP_DBG("PLAYBACK_SOURCE_NUM vol = %d\n", mainAppCt.gSysVol.AudioSourceVol[PLAYBACK_SOURCE_NUM]);
	#ifdef CFG_FUNC_ROBOEFFECT_EN
	AudioEffect_PresetSet(&EffectParambin, EFFECT_RECGAIN, INDEX_GAIN_CONTROL_GAIN, VOL_PRESET, mainAppCt.gSysVol.AudioSourceVol[PLAYBACK_SOURCE_NUM]);
	#endif
#endif

#ifdef CFG_APP_BT_MODE_EN
#if (BT_AVRCP_VOLUME_SYNC)
	//add volume sync(bluetooth play mode)
	if((GetSystemMode() == ModeBtAudioPlay) && (volume == TRUE))
	{
		SetBtSyncVolume(mainAppCt.gSysVol.AudioSourceVol[APP_SOURCE_NUM]);
		SendSysModeMsg(MSG_BT_PLAY_VOLUME_SET);
	}
#endif

#if (BT_HFP_SUPPORT)
	if(GetSystemMode() == ModeBtHfPlay)
	{
		SetBtHfSyncVolume(mainAppCt.gSysVol.AudioSourceVol[APP_SOURCE_NUM]);
	}
#endif
#endif
#ifdef CFG_APP_HDMIIN_MODE_EN
	if(GetSystemMode() == ModeHdmiAudioPlay)
	{
		gHdmiCt->hdmiActiveReportVolUpDownflag = 2;
	}
#endif
}

void AudioMusicVolDown(AudioEffectParambinContext *AudioEffectParambin)
{
	if(IsAudioPlayerMute() == TRUE)
	{
		HardWareMuteOrUnMute();
	}
	
#if (defined(CFG_APP_BT_MODE_EN) && (BT_HFP_SUPPORT))
	if(GetSystemMode() == ModeBtHfPlay)
	{
		if(mainAppCt.HfVolume > 0)
		{
			mainAppCt.HfVolume--;
			#ifdef CFG_FUNC_BREAKPOINT_EN
			BackupInfoUpdata(BACKUP_SYS_INFO);
			#endif
		}
		mainAppCt.gSysVol.AudioSourceVol[APP_SOURCE_NUM] = mainAppCt.HfVolume;
	}
	else
#endif
	{
		if(mainAppCt.MusicVolume > 0)
		{
			mainAppCt.MusicVolume--;
			
			#if (BT_TWS_SUPPORT == ENABLE)
			tws_vol_send(mainAppCt.MusicVolume, IsAudioPlayerMute());
			#endif

			#ifdef CFG_FUNC_BREAKPOINT_EN
			BackupInfoUpdata(BACKUP_SYS_INFO);
			#endif
		}
	    mainAppCt.gSysVol.AudioSourceVol[APP_SOURCE_NUM] = mainAppCt.MusicVolume;
	}
#ifdef CFG_FUNC_ROBOEFFECT_EN
	APP_DBG("APP_SOURCE_NUM vol = %d\n", mainAppCt.gSysVol.AudioSourceVol[APP_SOURCE_NUM]);
	AudioEffect_PresetSet(AudioEffectParambin, EFFECT_MUSICGAIN, INDEX_GAIN_CONTROL_GAIN, VOL_PRESET, mainAppCt.gSysVol.AudioSourceVol[APP_SOURCE_NUM]);
#endif
#ifdef CFG_FUNC_REMIND_SOUND_EN
	#if CFG_PARAM_FIXED_REMIND_VOL
	mainAppCt.gSysVol.AudioSourceVol[REMIND_SOURCE_NUM] = CFG_PARAM_FIXED_REMIND_VOL;
	#else
	mainAppCt.gSysVol.AudioSourceVol[REMIND_SOURCE_NUM] = mainAppCt.MusicVolume;
	#endif
	APP_DBG("REMIND_SOURCE_NUM vol = %d\n", mainAppCt.gSysVol.AudioSourceVol[REMIND_SOURCE_NUM]);
	#ifdef CFG_FUNC_ROBOEFFECT_EN
	AudioEffect_PresetSet(&EffectParambin, EFFECT_REMIND_GAIN, INDEX_GAIN_CONTROL_GAIN, VOL_PRESET, mainAppCt.gSysVol.AudioSourceVol[REMIND_SOURCE_NUM]);
	#endif
#endif
#ifdef CFG_FUNC_RECORDER_EN
	mainAppCt.gSysVol.AudioSourceVol[PLAYBACK_SOURCE_NUM] = mainAppCt.MusicVolume;
	APP_DBG("PLAYBACK_SOURCE_NUM vol = %d\n", mainAppCt.gSysVol.AudioSourceVol[PLAYBACK_SOURCE_NUM]);
	#ifdef CFG_FUNC_ROBOEFFECT_EN
	AudioEffect_PresetSet(&EffectParambin, EFFECT_RECGAIN, INDEX_GAIN_CONTROL_GAIN, VOL_PRESET, mainAppCt.gSysVol.AudioSourceVol[PLAYBACK_SOURCE_NUM]);
	#endif
#endif

#ifdef CFG_APP_BT_MODE_EN
#if (BT_AVRCP_VOLUME_SYNC)
	//add volume sync(bluetooth play mode)
	if(GetSystemMode() == ModeBtAudioPlay)
	{
		SetBtSyncVolume(mainAppCt.gSysVol.AudioSourceVol[APP_SOURCE_NUM]);
		SendSysModeMsg(MSG_BT_PLAY_VOLUME_SET);
	}
#endif

#if (BT_HFP_SUPPORT)
	if(GetSystemMode() == ModeBtHfPlay)
	{
		SetBtHfSyncVolume(mainAppCt.gSysVol.AudioSourceVol[APP_SOURCE_NUM]);
	}
#endif
#endif
#ifdef CFG_APP_HDMIIN_MODE_EN
	if(GetSystemMode() == ModeHdmiAudioPlay)
	{
		gHdmiCt->hdmiActiveReportVolUpDownflag = 2;
	}
#endif
}

void AudioMusicVolSet(uint8_t musicVol)
{
	if(IsAudioPlayerMute() == TRUE)
	{
		HardWareMuteOrUnMute();
	}
	AudioMusicVolSync(musicVol);
}

void AudioMusicVolSync(uint8_t musicVol)
{
	uint32_t maxLevel = AudioVol_GetMaxLevel();
	if(musicVol > maxLevel)
		mainAppCt.MusicVolume = maxLevel;
	else
		mainAppCt.MusicVolume = musicVol;
	mainAppCt.gSysVol.AudioSourceVol[APP_SOURCE_NUM] = mainAppCt.MusicVolume;
#ifdef CFG_FUNC_REMIND_SOUND_EN
	#if CFG_PARAM_FIXED_REMIND_VOL
	mainAppCt.gSysVol.AudioSourceVol[REMIND_SOURCE_NUM] = CFG_PARAM_FIXED_REMIND_VOL;
	#else
	mainAppCt.gSysVol.AudioSourceVol[REMIND_SOURCE_NUM] = mainAppCt.MusicVolume;
	#endif
	APP_DBG("REMIND_SOURCE_NUM vol = %d\n", mainAppCt.gSysVol.AudioSourceVol[REMIND_SOURCE_NUM]);
	#ifdef CFG_FUNC_ROBOEFFECT_EN
	AudioEffect_PresetSet(&EffectParambin, EFFECT_REMIND_GAIN, INDEX_GAIN_CONTROL_GAIN, \
		VOL_PRESET, mainAppCt.gSysVol.AudioSourceVol[REMIND_SOURCE_NUM]);
	#endif
#endif
#ifdef CFG_FUNC_ROBOEFFECT_EN
 	APP_DBG("APP_SOURCE_NUM vol = %d\n", mainAppCt.gSysVol.AudioSourceVol[APP_SOURCE_NUM]);
	#ifdef CFG_FUNC_ROBOEFFECT_EN
 	AudioEffect_PresetSet(&EffectParambin, EFFECT_MUSICGAIN, INDEX_GAIN_CONTROL_GAIN, \
		VOL_PRESET, mainAppCt.gSysVol.AudioSourceVol[APP_SOURCE_NUM]);
	#endif
#endif
}

#ifdef CFG_APP_BT_MODE_EN
void AudioHfVolSet(uint8_t HfVol)
{
	if(IsAudioPlayerMute() == TRUE)
	{
		HardWareMuteOrUnMute();
	}
	
	uint32_t maxLevel = AudioVol_GetMaxLevel();
	if(HfVol > maxLevel)
		mainAppCt.HfVolume = maxLevel;
	else
		mainAppCt.HfVolume = HfVol;
	mainAppCt.gSysVol.AudioSourceVol[APP_SOURCE_NUM] = mainAppCt.HfVolume;
	
	APP_DBG("source1 vol = %d\n", mainAppCt.gSysVol.AudioSourceVol[APP_SOURCE_NUM]);
#ifdef CFG_FUNC_ROBOEFFECT_EN
	AudioEffect_PresetSet(&EffectParambin, EFFECT_MUSICGAIN, INDEX_GAIN_CONTROL_GAIN, \
		VOL_PRESET, mainAppCt.gSysVol.AudioSourceVol[APP_SOURCE_NUM]);
#endif
}
#endif

#if (CFG_RES_MIC_SELECT) && defined(CFG_FUNC_MIC_EN)
void AudioMicVolUp(AudioEffectParambinContext *AudioEffectParambin)
{
	if(IsAudioPlayerMute() == TRUE)
	{
		HardWareMuteOrUnMute();
	}

	if(mainAppCt.MicVolume < AudioVol_GetMaxLevel())
	{
		mainAppCt.MicVolume++;
		#ifdef CFG_FUNC_BREAKPOINT_EN
		BackupInfoUpdata(BACKUP_SYS_INFO);
		#endif
	}
    mainAppCt.gSysVol.AudioSourceVol[MIC_SOURCE_NUM] = mainAppCt.MicVolume;
	APP_DBG("MIC_SOURCE_NUM vol = %d\n", mainAppCt.gSysVol.AudioSourceVol[MIC_SOURCE_NUM]);
	#ifdef CFG_FUNC_ROBOEFFECT_EN
	AudioEffect_PresetSet(AudioEffectParambin, EFFECT_MICGAIN, INDEX_GAIN_CONTROL_GAIN, \
		VOL_PRESET, mainAppCt.gSysVol.AudioSourceVol[MIC_SOURCE_NUM]);
	#endif
}

void AudioMicVolDown(AudioEffectParambinContext *AudioEffectParambin)
{
	if(IsAudioPlayerMute() == TRUE)
	{
		HardWareMuteOrUnMute();
	}

	if(mainAppCt.MicVolume > 0)
	{
		mainAppCt.MicVolume--;
		#ifdef CFG_FUNC_BREAKPOINT_EN
		BackupInfoUpdata(BACKUP_SYS_INFO);
		#endif
	}
    mainAppCt.gSysVol.AudioSourceVol[MIC_SOURCE_NUM] = mainAppCt.MicVolume;
	APP_DBG("MIC_SOURCE_NUM vol = %d\n", mainAppCt.gSysVol.AudioSourceVol[MIC_SOURCE_NUM]);
	#ifdef CFG_FUNC_ROBOEFFECT_EN
	AudioEffect_PresetSet(AudioEffectParambin, EFFECT_MICGAIN, INDEX_GAIN_CONTROL_GAIN, \
		VOL_PRESET, mainAppCt.gSysVol.AudioSourceVol[MIC_SOURCE_NUM]);
	#endif
}
#endif

void AudioBassTrebAjust(AudioEffectParambinContext *AudioEffectParambin, AUDIO_EFFECT_NAME_NUM effect_index, AUDIO_EFFECT_PRESET_NUM preset_index)
{
	if((gCtrlVars.AutoRefresh = AudioEffect_Parambin_GetEffectAddr(AudioEffectParambin, effect_index)) != 0)
	{
		#ifdef CFG_FUNC_ROBOEFFECT_EN
		if (effect_index == EFFECT_MICEQ)
		{
#ifdef CFG_FUNC_MIC_TREB_BASS_EN
			if (preset_index == PRESET_EQ_BASS_VOL)
			{
				APP_DBG("MicBassStep: %d\n", mainAppCt.MicBassStep); // bass设置mic EQ 的第 二 个频点gain , 对应上位机显示的 f1 频点(因为上位机从0开始计数)
				AudioEffect_PresetSet(&EffectParambin, effect_index, INDEX_EQ_FILTER2_GAIN, preset_index, mainAppCt.MicBassStep);
			}
			else if (preset_index == PRESET_EQ_TREB_VOL)
			{
				APP_DBG("MicTrebStep: %d\n", mainAppCt.MicTrebStep); // treb设置mic EQ 的第 六 个频点gain , 对应上位机显示的 f5 频点(因为上位机从0开始计数)
				AudioEffect_PresetSet(&EffectParambin, effect_index, INDEX_EQ_FILTER6_GAIN, preset_index, mainAppCt.MicTrebStep);
			}
#endif
		}
		else
		{
#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
			if (preset_index == PRESET_EQ_BASS_VOL)
			{
				APP_DBG("MusicBassStep: %d\n", mainAppCt.MusicBassStep);// bass设置music EQ 的第 二 个频点gain , 对应上位机显示的 f1 频点(因为上位机从0开始计数)
				AudioEffect_PresetSet(&EffectParambin, effect_index, INDEX_EQ_FILTER2_GAIN, preset_index, mainAppCt.MusicBassStep);
			}
			else if (preset_index == PRESET_EQ_TREB_VOL)
			{
				APP_DBG("MusicTrebStep: %d\n", mainAppCt.MusicTrebStep);// treb设置music EQ 的第 六 个频点gain , 对应上位机显示的 f5 频点(因为上位机从0开始计数)
				AudioEffect_PresetSet(&EffectParambin, effect_index, INDEX_EQ_FILTER6_GAIN, preset_index, mainAppCt.MusicTrebStep);
			}
#endif
		}
		#endif
	}
	else
	{
		APP_DBG("EFFECT_MICEQ doesn't exist\n");
	}
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
	AudioEffect_CommunicationQueue_Send(gCtrlVars.AutoRefresh);
#endif
}

void AudioEchoReverbAjust(AudioEffectParambinContext *AudioEffectParambin, AUDIO_EFFECT_NAME_NUM effect_index, AUDIO_EFFECT_PRESET_NUM preset_index)
{
#ifdef CFG_FUNC_MIC_ECHO_REVERB_EN
	if((gCtrlVars.AutoRefresh = AudioEffect_Parambin_GetEffectAddr(AudioEffectParambin, effect_index)) != 0)
	{
		#ifdef CFG_FUNC_ROBOEFFECT_EN
		if (preset_index == PRESET_ECHO_delay){
			AudioEffect_PresetSet(&EffectParambin, effect_index, INDEX_ECHO_DELAY, preset_index, mainAppCt.ReverbStep);
		}else if (preset_index == PRESET_ECHO_attenuation){
			AudioEffect_PresetSet(&EffectParambin, effect_index, INDEX_ECHO_ATTENUATION, preset_index, mainAppCt.ReverbStep);
		}else if (preset_index == PRESET_reverb_decay){
			AudioEffect_PresetSet(&EffectParambin, effect_index, INDEX_REVERB_PLATE_DECAY, preset_index, mainAppCt.ReverbStep);
		}else if (preset_index == PRESET_reverb_wetdry){
			AudioEffect_PresetSet(&EffectParambin, effect_index, INDEX_REVERB_PLATE_WETDRYMIX, preset_index, mainAppCt.ReverbStep);
		}else{
			DBG("preset_index is error!!\n");
		}
		#endif
	}
	else
	{
		APP_DBG("EFFECT doesn't exist\n");
	}
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
	AudioEffect_CommunicationQueue_Send(gCtrlVars.AutoRefresh);
#endif
#endif
}

void SystemVolSet(void)
{
#ifdef CFG_FUNC_ROBOEFFECT_EN
	if(EffectParambin.context_memory != NULL)
	{
		AudioEffect_PresetSet(&EffectParambin, EFFECT_MUSICGAIN, INDEX_GAIN_CONTROL_GAIN, \
			VOL_PRESET, mainAppCt.gSysVol.AudioSourceVol[APP_SOURCE_NUM]);
	}
#endif
}

void SystemVolSync(void)
{
	mainAppCt.gSysVol.AudioSourceVol[APP_SOURCE_NUM] = mainAppCt.MusicVolume;
	mainAppCt.gSysVol.AudioSourceVol[MIC_SOURCE_NUM] = mainAppCt.MicVolume;
}

#ifdef CFG_ADC_LEVEL_KEY_EN	
/*
****************************************************************
* 电位器消息接收处理函数
*说明:
*    AdcLevelCh和app_config.h中选择的GPIO有如下对应关系:

       #define  ADCLEVL_CHANNEL_MAP            (ADC_GPIOA20|ADC_GPIOA21|ADC_GPIOA22)
       
       ADC_GPIOA20对应AdcLevelCh为1；
       ADC_GPIOA21对应AdcLevelCh为2；
       ADC_GPIOA22对应AdcLevelCh为3；
****************************************************************
*/
void AdcLevelMsgProcess(uint16_t Msg)//Sliding resistance
{
	uint16_t AdcLevelCh, AdcValue;

    if( (Msg > MSG_ADC_LEVEL_MSG_START)&&(Msg < MSG_ADC_LEVEL_MSG_END) )
	{				
		AdcLevelCh      =   Msg&0xff00;
		AdcLevelCh      -=  MSG_ADC_LEVEL_MSG_START;
		AdcLevelCh      >>= 8;

		AdcValue      =   Msg &0x00ff;
        //APP_DBG("AdcLevelCh = %d\n", AdcLevelCh);
		//APP_DBG("AdcValue = %d\n",AdcValue);

		switch(AdcLevelCh)
		{

			case 1://ADC LEVEL Channel 1
	#if (CFG_RES_MIC_SELECT) && defined(CFG_FUNC_MIC_EN)
				mainAppCt.MicVolumeBak = AdcValue;
				APP_DBG("MicVolumeBak = %d\n", mainAppCt.MicVolumeBak);
			    //mainAppCt.gSysVol.AudioSourceVol[0] = mainAppCt.MicVolume = AdcValue;
				//APP_DBG("source0 vol = %d\n", mainAppCt.gSysVol.AudioSourceVol[0]);
				//AudioEffect_SourceGain_Update(0, mainAppCt.gSysVol.AudioSourceVol[0]);
	#endif
				break;
				
			case 2://ADC LEVEL Channel 2
				break;
				
			case 3://ADC LEVEL Channel 3
//				mainAppCt.MicBassStepBak = 15 - AdcValue/2;
//				mainAppCt.MicTrebStepBak = AdcValue/2;
				//MicBassTrebAjust(mainAppCt.MicBassStep, mainAppCt.MicTrebStep);
				break;

			case 4://ADC LEVEL Channel 4
				
				break;
			case 5://ADC LEVEL Channel 5
				
				break;
			case 6://ADC LEVEL Channel 6
				
				break;
			case 7://ADC LEVEL Channel 7
				
				break;
			case 8://ADC LEVEL Channel 8
				
				break;
			case 9://ADC LEVEL Channel 9
				
				break;
			case 10://ADC LEVEL Channel 10
				
				break;
			case 11://ADC LEVEL Channel 11
				
				break;
		}
	}
}
#endif

#ifdef  CFG_APP_HDMIIN_MODE_EN
void HDMISourceMute(void)
{
	mainAppCt.hdmiSourceMuteFlg = 1;
	AudioCoreSourceMute(APP_SOURCE_NUM, TRUE, TRUE);
	//APP_DBG("hdmi mute\n");
}

void HDMISourceUnmute(void)
{
	mainAppCt.hdmiSourceMuteFlg = 0;
	AudioCoreSourceUnmute(APP_SOURCE_NUM, TRUE, TRUE);
	//APP_DBG("hdmi unmute\n");
}

bool IsHDMISourceMute(void)
{
	return mainAppCt.hdmiSourceMuteFlg;
}
#endif
