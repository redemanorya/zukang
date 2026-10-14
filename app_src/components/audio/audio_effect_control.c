/**
 **************************************************************************************
 * @file    audio_effect_control.c
 * @brief   Implementation of Audio Effect Control API
 *
 * @author  Yangyu
 * @version V1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#include <nds32_utils_math.h>
#include <string.h>
#include "audio_effect_control.h"
#include "audio_effect_parambin.h"
#include "audio_vol.h"
#include "main_task.h"
#include "breakpoint.h"
#include "audio_core_service.h"
#include "communication.h"
#ifdef DUAL_CORE_EN
#include "dualcore_dsp.h"
#endif
#include "ctrlvars.h"
extern ControlVariablesContext gCtrlVars;

#ifdef CFG_FUNC_ROBOEFFECT_EN

#include "flow_config.h"//只在此包含,仅便于应用层配置

#ifndef ROBOEFFECT_DOUBLE_ENGINE_EN
	const char *effect_flow_name[] =
	{
		MAP_FLOW_LIST
	};
#else
	//图名表第0列不许为空
	const char *effect_flow_name[][2] =
	{
		MAP_DUALFLOW_LIST
	};
#endif

uint8_t AvailableNUM = 0;
#define EFFECT_FLOW_COUNT (sizeof(effect_flow_name) / sizeof(effect_flow_name[0]))
void init_effect_flows_Num(void)
{
	AvailableNUM = EFFECT_FLOW_COUNT;
}
#ifndef ROBOEFFECT_DOUBLE_ENGINE_EN
	SubEffect effect_flows[EFFECT_FLOW_COUNT] = {{0}};

	void init_effect_flows_Status(void)
	{
		for(uint8_t i = 0; i < EFFECT_FLOW_COUNT; i++)
		{
				effect_flows[i].name  = effect_flow_name[i];
				effect_flows[i].state = 0x01;   //全部都是就绪态
		}
		effect_flows[mainAppCt.effect_flow_index].state = 0x00;   //运行态设定
	}
#else
	SubEffect effect_flows[EFFECT_FLOW_COUNT][2] = {{{0}}};

	void init_effect_flows_Status(void)
	{
		for(uint8_t i = 0; i < EFFECT_FLOW_COUNT; i++)
		{
			for(uint8_t j = 0; j < 2; j++)
			{
				effect_flows[i][j].name  = effect_flow_name[i][j];
				effect_flows[i][j].state = 0x01;    //全部都是就绪态
			}
		}
		effect_flows[mainAppCt.effect_flow_index][0].state = 0x00;   //运行态设定
		effect_flows[mainAppCt.effect_flow_index][1].state = 0x00;   //运行态设定
	}
#endif

uint16_t AudioEffect_GetTotalFlowNum(void)
{
	return AvailableNUM;
}


/*
 * 音效参数循环切换
 * 双图时参数循环切换：当两个参数索引均达到最大值时,触发切换,二者均归零
 */
void AudioEffect_ParamSwitch(void)
{
    // 循环查找符合条件的模式
	// 基础索引递增逻辑
#ifndef ROBOEFFECT_DOUBLE_ENGINE_EN
	if(mainAppCt.effect_param_mode_index >= (AudioEffect_Parambin_GetTotalParamNumByName(effect_flow_name[mainAppCt.effect_flow_index]) - 1))
	{
		mainAppCt.effect_param_mode_index = 0;
	}
	else
	{
		mainAppCt.effect_param_mode_index = mainAppCt.effect_param_mode_index + 1;
	}
	AudioEffect_Parambin_SwitchEffectByIndex(mainAppCt.effect_flow_index, mainAppCt.effect_param_mode_index, -1);
#else
	if(mainAppCt.effect_param_mode_groupindex[0] >= (AudioEffect_Parambin_GetTotalParamNumByName(effect_flow_name[mainAppCt.effect_flow_index][0]) - 1) &&
			mainAppCt.effect_param_mode_groupindex[1] >= (AudioEffect_Parambin_GetTotalParamNumByName(effect_flow_name[mainAppCt.effect_flow_index][1]) - 1))
	{
		mainAppCt.effect_param_mode_groupindex[0] = 0;
		mainAppCt.effect_param_mode_groupindex[1] = 0;
	}
	else if(mainAppCt.effect_param_mode_groupindex[0] < (AudioEffect_Parambin_GetTotalParamNumByName(effect_flow_name[mainAppCt.effect_flow_index][0]) - 1) &&
			mainAppCt.effect_param_mode_groupindex[1] >= (AudioEffect_Parambin_GetTotalParamNumByName(effect_flow_name[mainAppCt.effect_flow_index][1]) - 1))
	{
		mainAppCt.effect_param_mode_groupindex[0] = mainAppCt.effect_param_mode_groupindex[0] + 1;
	}
	else if(mainAppCt.effect_param_mode_groupindex[0] >= (AudioEffect_Parambin_GetTotalParamNumByName(effect_flow_name[mainAppCt.effect_flow_index][0]) - 1) &&
			mainAppCt.effect_param_mode_groupindex[1] < (AudioEffect_Parambin_GetTotalParamNumByName(effect_flow_name[mainAppCt.effect_flow_index][1]) - 1))
	{
		mainAppCt.effect_param_mode_groupindex[1] = mainAppCt.effect_param_mode_groupindex[1] + 1;
	}
	else
	{
		mainAppCt.effect_param_mode_groupindex[0] = mainAppCt.effect_param_mode_groupindex[0] + 1;
		mainAppCt.effect_param_mode_groupindex[1] = mainAppCt.effect_param_mode_groupindex[1] + 1;
	}
	AudioEffect_Parambin_SwitchEffectByIndex(mainAppCt.effect_flow_index, mainAppCt.effect_param_mode_groupindex[0], mainAppCt.effect_param_mode_groupindex[1]);
#endif
}

/*
 * 根据effect_flow_name表格循环切换音效框图
 */
void AudioEffect_FlowSwitch(void)
{
    uint16_t flow_target = 0;
    bool found_valid_flow = FALSE;
    uint16_t original_flow = mainAppCt.effect_flow_index;   //保存一下当前索引

    // 循环查找符合条件的模式
    do {
        // 基础索引递增逻辑
    	if(mainAppCt.effect_flow_index >= (AudioEffect_GetTotalFlowNum() - 1))
    	{
    		flow_target = 0;
    	}
        else
        {
            flow_target = mainAppCt.effect_flow_index + 1;
        }

        // 检查是否为hfp模式,且进行有效性检查

#ifndef ROBOEFFECT_DOUBLE_ENGINE_EN
        if((strcmp("Hfp", effect_flow_name[flow_target]) == 0) || (AudioEffect_Parambin_GetTotalParamNumByName(effect_flow_name[flow_target]) == 0))
#else
        if((strcmp("Hfp", effect_flow_name[flow_target][0]) == 0) || (strcmp("Hfp", effect_flow_name[flow_target][1]) == 0))
#endif
        {
            //保存当前索引并尝试下一个
            mainAppCt.effect_flow_index = flow_target;

            // 边界检查：如果遍历完所有模式仍未找到有效值
            if(mainAppCt.effect_flow_index == original_flow)
            {
                APP_DBG("All flows are hfp, no valid mode found!\n");
                break;
            }
        }
        else
        {
            found_valid_flow = TRUE;
#ifndef ROBOEFFECT_DOUBLE_ENGINE_EN
            mainAppCt.effect_param_mode_index = 0;
#else
            mainAppCt.effect_param_mode_groupindex[0] = 0;
            mainAppCt.effect_param_mode_groupindex[1] = 0;
#endif
            break;
        }
    } while (1);

    // 只有找到有效值才执行切换
    if(found_valid_flow)
    {
#ifndef ROBOEFFECT_DOUBLE_ENGINE_EN
    	AudioEffect_Parambin_SwitchEffectByIndex(flow_target, mainAppCt.effect_param_mode_index, -1);
#else
    	AudioEffect_Parambin_SwitchEffectByIndex(flow_target, mainAppCt.effect_param_mode_groupindex[0], mainAppCt.effect_param_mode_groupindex[1]);
#endif
    }
    else
    {
        // 恢复原始状态
        mainAppCt.effect_flow_index = original_flow;
    }
}

void AudioEffect_Restart(void)
{
	uint32_t sample_rate, frame_size = 0;
#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
	uint32_t sample_rate1, frame_size1 = 0;
#endif

#ifndef ROBOEFFECT_DOUBLE_ENGINE_EN
	if(AudioEffect_Parambin_GetEffectListInfoByName(effect_flow_name[mainAppCt.effect_flow_index], mainAppCt.effect_param_mode_index, &sample_rate, &frame_size))
	{
		if((AudioCoreMixSampleRateGet(DefaultNet) == sample_rate) && (AudioCoreFrameSizeGet(DefaultNet) == frame_size))
		{
			AudioEffect_EffectMode_Refresh();
		}
		else if(MixSupport(sample_rate, frame_size, WIDTH_L24BIT))
		{
			APP_DBG("sampleRate or framesize change ,sysMode reboot!\n");
			SysCurModeReboot();
		}
		else
		{
			mainAppCt.effect_flow_index = 0;
			mainAppCt.effect_param_mode_index = 0;
			APP_DBG("sampleRate or framesize Not supported ,sysMode reboot!\n");
			SysCurModeReboot();
		}
	}
	else
	{
		APP_DBG("!!!prambin error, please check!!!\n");
	}
#else
	if(strlen(effect_flow_name[mainAppCt.effect_flow_index][1]) == 0)
	{
		if(AudioEffect_Parambin_GetEffectListInfoByName(effect_flow_name[mainAppCt.effect_flow_index][0], mainAppCt.effect_param_mode_groupindex[0], &sample_rate, &frame_size))
		{
			if((AudioCoreMixSampleRateGet(DefaultNet) == sample_rate) && (AudioCoreFrameSizeGet(DefaultNet) == frame_size))
			{
				AudioEffect_EffectMode_Refresh();
			}
			else if(MixSupport(sample_rate, frame_size, WIDTH_L24BIT))
			{
				APP_DBG("sampleRate or framesize change ,sysMode reboot!\n");
				SysCurModeReboot();
			}
			else
			{
				mainAppCt.effect_flow_index = 0;
				mainAppCt.effect_param_mode_groupindex[0] = 0;
				mainAppCt.effect_param_mode_groupindex[1] = 0;
				APP_DBG("sampleRate or framesize Not supported ,sysMode reboot!\n");
				SysCurModeReboot();
			}
		}
		else
		{
			APP_DBG("!!!prambin error, please check!!!\n");
		}
	}
	else
	{
		if(AudioEffect_Parambin_GetEffectListInfoByName(effect_flow_name[mainAppCt.effect_flow_index][0], mainAppCt.effect_param_mode_groupindex[0], &sample_rate, &frame_size) &&
				AudioEffect_Parambin_GetEffectListInfoByName(effect_flow_name[mainAppCt.effect_flow_index][1], mainAppCt.effect_param_mode_groupindex[1], &sample_rate1, &frame_size1))
		{
			if((AudioCoreMixSampleRateGet(DefaultNet) == sample_rate) && (AudioCoreFrameSizeGet(DefaultNet) == frame_size) &&
				(AudioCoreMixSampleRateGet(SeparateNet) == sample_rate1) && (AudioCoreFrameSizeGet(SeparateNet) == frame_size1))
			{
				AudioEffect_EffectMode_Refresh();
			}
			else if(MixSupport(sample_rate, frame_size, WIDTH_L24BIT) && MixSupport(sample_rate1, frame_size1, WIDTH_L24BIT)) //sample_rate & frame_size change, need reinit all
			{
				APP_DBG("sampleRate or framesize change ,sysMode reboot!\n");
				SysCurModeReboot();
			}
			else
			{
				mainAppCt.effect_flow_index = 0;
				mainAppCt.effect_param_mode_groupindex[0] = 0;
				mainAppCt.effect_param_mode_groupindex[1] = 0;
				APP_DBG("sampleRate or framesize Not supported ,sysMode reboot!\n");
				SysCurModeReboot();
			}
		}
		else
		{
			APP_DBG("!!!prambin error, please check!!!\n");
		}
	}
#endif
}

void AudioEffect_EffectMode_Refresh(void)
{
	if(IsAudioPlayerMute() == FALSE)
	{
		HardWareMuteOrUnMute();
	}

	PauseAudioCore();

#ifndef ROBOEFFECT_DOUBLE_ENGINE_EN
	AudioEffect_Parambin_Deinit(&EffectParambin);
	AudioEffect_Parambin_Init(&EffectParambin, effect_flow_name[mainAppCt.effect_flow_index], mainAppCt.effect_param_mode_index, 1);
	#ifndef	EFFECT_BYPASSMODE
	AudioEffect_Parambin_RefreshSourceSinkBuf(&EffectParambin);
	#endif
#else
	AudioEffect_Parambin_Deinit(&EffectParambin2);
	AudioEffect_Parambin_Deinit(&EffectParambin);
	AudioEffect_Parambin_Init(&EffectParambin2, effect_flow_name[mainAppCt.effect_flow_index][1], mainAppCt.effect_param_mode_groupindex[1], 0);
	AudioEffect_Parambin_Init(&EffectParambin, effect_flow_name[mainAppCt.effect_flow_index][0], mainAppCt.effect_param_mode_groupindex[0], 1);
	AudioEffect_Parambin_RefreshSourceSinkBuf(&EffectParambin2);
	AudioEffect_Parambin_RefreshSourceSinkBuf(&EffectParambin);   //双图重复的source/sink会被图0占用
#endif
	init_effect_flows_Status();

	SoftFlagDeregister(SoftFlagAudioCoreSourceIsDeInit);
	DefaultParamgsInit();
	AudioCodecHardwareSync();//update hardware config

#ifdef DUAL_CORE_EN
	DualCoreDspAddrInit();
#endif

	////Audio Core & Audioeffect音量配置
	AudioEffect_Params_Sync(&EffectParambin);
#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
	AudioEffect_Params_Sync(&EffectParambin2);
#endif

	AudioCoreServiceResume();

//	gCtrlVars.AutoRefresh = AutoRefresh_ALL_EFFECTS_PARA;

	if(IsAudioPlayerMute() == TRUE)
	{
		HardWareMuteOrUnMute();
	}
}

void AudioEffect_Params_Sync(AudioEffectParambinContext *AudioEffectParambin)
{
	if(AudioEffectParambin==NULL) return ;
	if(AudioEffectParambin->context_memory == NULL)
		return;
#if defined(ROBOEFFECT_DOUBLE_ENGINE_TWS_SWITCH_MUSIC_EN)
	if(AudioEffectParambin == &EffectParambin)
	{
		int16_t parameter = 0;//0：Enable;1:Disable for INDEX_GAIN_CONTROL_MUTE 控件
#ifdef FUNC_BT_TWS_SUPPORT
		if(GetSystemMode() == ModeTwsSlavePlay || BtTwsStatusGet())
		{
			parameter = 1;
		}
#endif
		AudioEffect_Enable(EffectParambin.context_memory, roboeffect_parambin_get_addr_by_name(EffectParambin.context_memory, "tws_gain_mute"), 1);
		//调整当前默认音效图0x81号音效（一般为gain_control）第1个参数为parameter
		AudioEffect_ParamSet(EffectParambin.context_memory, roboeffect_parambin_get_addr_by_name(EffectParambin.context_memory, "tws_gain_mute"), INDEX_GAIN_CONTROL_MUTE, &parameter);
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
		gCtrlVars.AutoRefresh = 0x81;
		AudioEffect_CommunicationQueue_Send(gCtrlVars.AutoRefresh);
#endif
	}
#endif

	AudioAPPDigitalGianProcess(AudioEffectParambin, mainAppCt.SysCurrentMode);
	SystemVolSet();
	AudioMusicVolSync(mainAppCt.MusicVolume);

#ifdef CFG_FUNC_MUSIC_EQ_EN
	AudioEffect_EqMode_Set(AudioEffectParambin, mainAppCt.EqMode);
#endif
#ifdef CFG_FUNC_MIC_ECHO_REVERB_EN
	AudioEchoReverbAjust(AudioEffectParambin, EFFECT_ECHO, PRESET_ECHO_delay);
	AudioEchoReverbAjust(AudioEffectParambin, EFFECT_ECHO, PRESET_ECHO_attenuation);
	AudioEchoReverbAjust(AudioEffectParambin, EFFECT_REVERB, PRESET_reverb_decay);
	AudioEchoReverbAjust(AudioEffectParambin, EFFECT_REVERB, PRESET_reverb_wetdry);
#endif
#ifdef CFG_FUNC_MIC_TREB_BASS_EN
	AudioBassTrebAjust(AudioEffectParambin, EFFECT_MICEQ, PRESET_EQ_BASS_VOL);
	AudioBassTrebAjust(AudioEffectParambin, EFFECT_MICEQ, PRESET_EQ_TREB_VOL);
#endif
#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
	AudioBassTrebAjust(AudioEffectParambin, EFFECT_MUSICEQ, PRESET_EQ_BASS_VOL);
	AudioBassTrebAjust(AudioEffectParambin, EFFECT_MUSICEQ, PRESET_EQ_TREB_VOL);
#endif

//	gCtrlVars.AutoRefresh = AutoRefresh_ALL_EFFECTS_PARA;
}

#endif

uint8_t AudioEffect_EqMode_Set(AudioEffectParambinContext *AudioEffectParambin, uint8_t EqMode)
{
	ROBOEFFECT_ERROR_CODE ret = ROBOEFFECT_ERROR_OK;

	switch(EqMode)
	{
		case EQ_MODE_FLAT:
			ret = AudioEffect_PresetSet(AudioEffectParambin, EFFECT_MUSICEQ, 0xff, PRESET_EQ_FLAT, 0);
		break;
		case EQ_MODE_CLASSIC:
			ret = AudioEffect_PresetSet(AudioEffectParambin, EFFECT_MUSICEQ, 0xff, PRESET_EQ_CLASSIC, 0);
		break;
		case EQ_MODE_POP:
			ret = AudioEffect_PresetSet(AudioEffectParambin, EFFECT_MUSICEQ, 0xff, PRESET_EQ_POP, 0);
		break;
		case EQ_MODE_ROCK:
			ret = AudioEffect_PresetSet(AudioEffectParambin, EFFECT_MUSICEQ, 0xff, PRESET_EQ_ROCK, 0);
		break;
		case EQ_MODE_JAZZ:
			ret = AudioEffect_PresetSet(AudioEffectParambin, EFFECT_MUSICEQ, 0xff, PRESET_EQ_JAZZ, 0);
		break;
		case EQ_MODE_VOCAL_BOOST:
			ret = AudioEffect_PresetSet(AudioEffectParambin, EFFECT_MUSICEQ, 0xff, PRESET_EQ_VOCALBOOST, 0);
		break;
		default:
			break;
	}

	if(ret != ROBOEFFECT_ERROR_OK)
		APP_DBG("AudioEffect_EqMode_Set failed or EFFECT_MUSICEQ doesn't exist!:%d\n", ret);

	return AudioEffect_Parambin_GetEffectAddr(AudioEffectParambin, EFFECT_MUSICEQ);
}
#if 0   //API废弃,待删除
uint8_t AudioEffect_effect_status_Get(AUDIO_EFFECT_NAME_NUM effect_index)
{
	if(AudioEffectParambin.context_memory == NULL)
		return 0;
	if(!AudioEffect_Parambin_GetEffectAddr(effect_index))
		return 0;

	return roboeffect_get_effect_status(AudioEffectParambin.context_memory, AudioEffect_Parambin_GetEffectAddr(effect_index));
};

void AudioEffect_effect_enable(AUDIO_EFFECT_NAME_NUM effect_index, uint8_t enable)
{
	if(AudioEffectParambin.context_memory == NULL)
		return;
	if(!AudioEffect_Parambin_GetEffectAddr(effect_index))
		return;

	AudioEffect_Parambin_UpdateTempEffectParamsByAddr(AudioEffect_Parambin_GetEffectAddr(effect_index), 0, &enable);
	AudioEffect_Parambin_UpdateFlashEffectParams();

	AudioEffect_EffectMode_Refresh();
	gCtrlVars.AutoRefresh = AudioEffect_Parambin_GetEffectAddr(effect_index);
};
#endif

void AudioEffect_Msg_Process(AudioEffectParambinContext *AudioEffectParambin, uint32_t Msg)
{
	switch(Msg)
	{
		case MSG_MUSIC_VOLUP:
		{
			AudioEffectParambinContext *musicParambin = AudioEffectParambin;
#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
			if (AudioCoreGetNetBySourceChannelNUM(APP_SOURCE_NUM) == SeparateNet)
			{
				musicParambin = &EffectParambin2;
			}
#endif
			APP_DBG("MUSIC_VOLUP\n");
			if((gCtrlVars.AutoRefresh = AudioEffect_Parambin_GetEffectAddr(musicParambin, EFFECT_MUSICGAIN)) != 0)
			{
				AudioMusicVolUp(musicParambin);
			}
			else
			{
				APP_DBG("EFFECT_MUSICGAIN doesn't exist\n");
			}
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
			AudioEffect_CommunicationQueue_Send(gCtrlVars.AutoRefresh);
#endif
			break;
		}
		case MSG_MUSIC_VOLDOWN:
		{
			AudioEffectParambinContext *musicParambin = AudioEffectParambin;
#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
			if (AudioCoreGetNetBySourceChannelNUM(APP_SOURCE_NUM) == SeparateNet)
			{
				musicParambin = &EffectParambin2;
			}
#endif
			APP_DBG("MUSIC_VOLDOWN\n");
			if((gCtrlVars.AutoRefresh = AudioEffect_Parambin_GetEffectAddr(musicParambin, EFFECT_MUSICGAIN)) != 0)
			{
				AudioMusicVolDown(musicParambin);
			}
			else
			{
				APP_DBG("EFFECT_MUSICGAIN doesn't exist\n");
			}
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
			AudioEffect_CommunicationQueue_Send(gCtrlVars.AutoRefresh);
#endif
			break;
		}
#if (CFG_RES_MIC_SELECT) && defined(CFG_FUNC_MIC_EN)
		case MSG_MIC_VOLUP:
		{
			AudioEffectParambinContext *micParambin = AudioEffectParambin;
//根据SOURCE来判断是哪张图上的音效
#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
			if (AudioCoreGetNetBySourceChannelNUM(MIC_SOURCE_NUM) == SeparateNet)
			{
				micParambin = &EffectParambin2;
			}
#endif
			APP_DBG("MIC_VOLUP\n");
			if((gCtrlVars.AutoRefresh = AudioEffect_Parambin_GetEffectAddr(micParambin, EFFECT_MICGAIN)) != 0)
			{
				AudioMicVolUp(micParambin);
			}
			else
			{
				APP_DBG("EFFECT_MICGAIN doesn't exist\n");
			}
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
			AudioEffect_CommunicationQueue_Send(gCtrlVars.AutoRefresh);
#endif
			break;
		}
		case MSG_MIC_VOLDOWN:
		{
			AudioEffectParambinContext *micParambin = AudioEffectParambin;
#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
			if (AudioCoreGetNetBySourceChannelNUM(MIC_SOURCE_NUM) == SeparateNet)
			{
				micParambin = &EffectParambin2;
			}
#endif
			APP_DBG("MIC_VOLDOWN\n");
			if((gCtrlVars.AutoRefresh = AudioEffect_Parambin_GetEffectAddr(micParambin, EFFECT_MICGAIN)) != 0)
			{
				AudioMicVolDown(micParambin);
			}
			else
			{
				APP_DBG("EFFECT_MICGAIN doesn't exist\n");
			}
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
			AudioEffect_CommunicationQueue_Send(gCtrlVars.AutoRefresh);
#endif
			break;
		}
#endif
#ifdef CFG_FUNC_MUSIC_EQ_EN
		case MSG_EFFECTPARAM_EQ:
			APP_DBG("MSG_EQ\n");
			mainAppCt.EqMode = (++mainAppCt.EqMode) > EQ_MODE_VOCAL_BOOST ? EQ_MODE_FLAT:mainAppCt.EqMode;
			APP_DBG("EqMode = %d\n", mainAppCt.EqMode);
			gCtrlVars.AutoRefresh = AudioEffect_EqMode_Set(AudioEffectParambin, mainAppCt.EqMode);
			#ifdef CFG_FUNC_BREAKPOINT_EN
			BackupInfoUpdata(BACKUP_SYS_INFO);
			#endif
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
			AudioEffect_CommunicationQueue_Send(gCtrlVars.AutoRefresh);
#endif
			break;
#endif

#ifdef CFG_FUNC_MIC_TREB_BASS_EN
		case MSG_MIC_BASS_DW:
			if(mainAppCt.MicBassStep > 0)
			{
				mainAppCt.MicBassStep--;
				#ifdef CFG_FUNC_BREAKPOINT_EN
				BackupInfoUpdata(BACKUP_SYS_INFO);
				#endif
			}
			AudioBassTrebAjust(AudioEffectParambin, EFFECT_MICEQ, PRESET_EQ_BASS_VOL);
			break;
		case MSG_MIC_BASS_UP:
			if(mainAppCt.MicBassStep < AudioVol_MaxLevel_Get(AudioEffectParambin, EFFECT_MICEQ, PRESET_EQ_BASS_VOL))
			{
				mainAppCt.MicBassStep++;
				#ifdef CFG_FUNC_BREAKPOINT_EN
				BackupInfoUpdata(BACKUP_SYS_INFO);
				#endif
			}
			AudioBassTrebAjust(AudioEffectParambin, EFFECT_MICEQ, PRESET_EQ_BASS_VOL);
			break;
		case MSG_MIC_TREB_DW:
			if(mainAppCt.MicTrebStep > 0)
			{
				mainAppCt.MicTrebStep--;
				#ifdef CFG_FUNC_BREAKPOINT_EN
				BackupInfoUpdata(BACKUP_SYS_INFO);
				#endif
			}
			AudioBassTrebAjust(AudioEffectParambin, EFFECT_MICEQ, PRESET_EQ_TREB_VOL);
			break;
		case MSG_MIC_TREB_UP:
			if(mainAppCt.MicTrebStep < AudioVol_MaxLevel_Get(AudioEffectParambin, EFFECT_MICEQ, PRESET_EQ_TREB_VOL))
			{
				mainAppCt.MicTrebStep++;
				#ifdef CFG_FUNC_BREAKPOINT_EN
				BackupInfoUpdata(BACKUP_SYS_INFO);
				#endif
			}
			AudioBassTrebAjust(AudioEffectParambin, EFFECT_MICEQ, PRESET_EQ_TREB_VOL);
			break;
#endif
#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
		case MSG_MUSIC_BASS_DW:
			if(mainAppCt.MusicBassStep > 0)
			{
				mainAppCt.MusicBassStep--;
				#ifdef CFG_FUNC_BREAKPOINT_EN
				BackupInfoUpdata(BACKUP_SYS_INFO);
				#endif
			}
			AudioBassTrebAjust(AudioEffectParambin, EFFECT_MUSICEQ, PRESET_EQ_BASS_VOL);
			break;
		case MSG_MUSIC_BASS_UP:
			if(mainAppCt.MusicBassStep < AudioVol_MaxLevel_Get(AudioEffectParambin, EFFECT_MUSICEQ, PRESET_EQ_BASS_VOL))
			{
				mainAppCt.MusicBassStep++;
				#ifdef CFG_FUNC_BREAKPOINT_EN
				BackupInfoUpdata(BACKUP_SYS_INFO);
				#endif
			}
			AudioBassTrebAjust(AudioEffectParambin, EFFECT_MUSICEQ, PRESET_EQ_BASS_VOL);
			break;
		case MSG_MUSIC_TREB_DW:
			if(mainAppCt.MusicTrebStep > 0)
			{
				mainAppCt.MusicTrebStep--;
				#ifdef CFG_FUNC_BREAKPOINT_EN
				BackupInfoUpdata(BACKUP_SYS_INFO);
				#endif
			}
			AudioBassTrebAjust(AudioEffectParambin, EFFECT_MUSICEQ, PRESET_EQ_TREB_VOL);
			break;
		case MSG_MUSIC_TREB_UP:
			if(mainAppCt.MusicTrebStep < AudioVol_MaxLevel_Get(AudioEffectParambin, EFFECT_MUSICEQ, PRESET_EQ_TREB_VOL))
			{
				mainAppCt.MusicTrebStep++;
				#ifdef CFG_FUNC_BREAKPOINT_EN
				BackupInfoUpdata(BACKUP_SYS_INFO);
				#endif
			}
			AudioBassTrebAjust(AudioEffectParambin, EFFECT_MUSICEQ, PRESET_EQ_TREB_VOL);
			break;
#endif
#ifdef CFG_FUNC_MIC_ECHO_REVERB_EN
		case MSG_MIC_EFFECT_DW:
			if(mainAppCt.ReverbStep > 0)
			{
				mainAppCt.ReverbStep--;
				#ifdef CFG_FUNC_BREAKPOINT_EN
				BackupInfoUpdata(BACKUP_SYS_INFO);
				#endif
			}
			APP_DBG("ReverbStep: %d\n", mainAppCt.ReverbStep);
			AudioEchoReverbAjust(AudioEffectParambin, EFFECT_ECHO, PRESET_ECHO_delay);
			AudioEchoReverbAjust(AudioEffectParambin, EFFECT_ECHO, PRESET_ECHO_attenuation);
			AudioEchoReverbAjust(AudioEffectParambin, EFFECT_REVERB, PRESET_reverb_decay);
			AudioEchoReverbAjust(AudioEffectParambin, EFFECT_REVERB, PRESET_reverb_wetdry);
			break;
		case MSG_MIC_EFFECT_UP:
			if(mainAppCt.ReverbStep < AudioVol_MaxLevel_Get(AudioEffectParambin, EFFECT_ECHO, PRESET_ECHO_delay))//目前混响最大音量等级默认使用一样的，所以这个混响的四个参数任意一个都行
			{
				mainAppCt.ReverbStep++;
				#ifdef CFG_FUNC_BREAKPOINT_EN
				BackupInfoUpdata(BACKUP_SYS_INFO);
				#endif
			}
			APP_DBG("ReverbStep: %d\n", mainAppCt.ReverbStep);
			AudioEchoReverbAjust(AudioEffectParambin, EFFECT_ECHO, PRESET_ECHO_delay);
			AudioEchoReverbAjust(AudioEffectParambin, EFFECT_ECHO, PRESET_ECHO_attenuation);
			AudioEchoReverbAjust(AudioEffectParambin, EFFECT_REVERB, PRESET_reverb_decay);
			AudioEchoReverbAjust(AudioEffectParambin, EFFECT_REVERB, PRESET_reverb_wetdry);
			break;
#endif
		default:
			break;
	}
}

bool AudioEffect_Msg_Check(uint16_t *Msg)
{
	if(*Msg > MSG_EFFECTPARAM_CONTROL_START && *Msg < MSG_EFFECTPARAM_CONTROL_END)
	{
		MessageContext msgSend;
		msgSend.msgId = *Msg;

		if((GetSystemMode() == ModeUsbDevicePlay || GetSystemMode() == ModeUsbFsDevicePlay) && (*Msg == MSG_MUSIC_VOLUP || *Msg == MSG_MUSIC_VOLDOWN))
		{
			if(*Msg == MSG_MUSIC_VOLUP)
			{
				*Msg = MSG_USB_MUSIC_VOLUP;
			}
			else
			{
				*Msg = MSG_USB_MUSIC_VOLDOWN;
			}
		}
		else
		{
#ifdef	CFG_FUNC_ROBOEFFECT_EN
			MessageSend(GetAudioCoreServiceMsgHandle(), &msgSend);
			return TRUE;
#endif
		}
	}
	return FALSE;
}

void AudioEffect_UpdateSamplerateFramesize(AudioEffectParambinContext *AudioEffectParambin, uint32_t sample_rate, uint32_t frame_size)
{
	AudioEffectParambin->user_effect_list->sample_rate = sample_rate;
	AudioEffectParambin->user_effect_list->frame_size = frame_size;

	AudioEffect_EffectMode_Refresh();

#ifdef CFG_FUNC_BREAKPOINT_EN
	BackupInfoUpdata(BACKUP_SYS_INFO);
#endif
}

uint32_t AudioEffect_GetUserEffectParamCRCLen(AudioEffectParambinContext *AudioEffectParambin)
{
	uint8_t *params = GET_USER_EFFECT_PARAMETERS_EFFECTDATA_P(AudioEffectParambin->user_effect_parameters);
	uint16_t data_len = GET_USER_EFFECT_PARAMETERS_DATA_LEN(AudioEffectParambin->user_effect_parameters);
	uint32_t crcLen = 0;

	for(uint32_t i = 0; i < data_len; i++)
	{
		crcLen += params[i];
	}

	return crcLen;
}

uint32_t AudioEffect_GetUserHWCfgCRCLen(uint8_t *effect_param)
{
	uint32_t crcLen = 0;
	uint32_t paramsLen = sizeof(gCtrlVars.HwCt);

	for(uint32_t i = 0; i < paramsLen; i++)
	{
		crcLen += effect_param[i];
	}

	return crcLen;
}

void AudioCoreFrameSampleSync(AudioEffectParambinContext *AudioEffectParambin, uint8_t net)
{
	if(AudioEffectParambin->context_memory == NULL)
	{
		return;
	}
	AudioCoreMixParamSet(net, AudioEffectParambin->user_effect_list->sample_rate, AudioEffectParambin->user_effect_list->frame_size, WIDTH_L24BIT);
	APP_DBG("set net %d samplerate:%ld, framesize:%d\n", net , AudioCoreMixSampleRateGet(net), AudioCoreFrameSizeGet(net));
}

uint8_t AudioCoreGetNetBySourceChannelNUM(AUDIO_CORE_SOURCE_NUM ChannelNUM)
{
	if(!AudioEffect_Parambin_GetSourceBuffer(&EffectParambin, ChannelNUM))
	{
#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
		if(AudioEffect_Parambin_GetSourceBuffer(&EffectParambin2, ChannelNUM))
		{
			return SeparateNet;
		}
		else
		{
			return DefaultNet;
		}
#endif
	}

	return DefaultNet;
}

uint8_t AudioCoreGetNetBySinkChannelNUM(AUDIO_CORE_SINK_NUM ChannelNUM)
{
	if(!AudioEffect_Parambin_GetSinkBuffer(&EffectParambin, ChannelNUM))
	{
#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
		if(AudioEffect_Parambin_GetSinkBuffer(&EffectParambin2, ChannelNUM))
		{
			return SeparateNet;
		}
		else
		{
			return DefaultNet;
		}
#endif
	}

	return DefaultNet;
}
