#include <string.h>
#include <nds32_intrinsic.h>
#include "math.h"
#include "debug.h"
#include "app_config.h"
#include "bt_config.h"
#include "rtos_api.h"
#include "audio_effect_library.h"
#include "audio_effect_control.h"
#include "audio_core_api.h"
#include "main_task.h"
#include "audio_effect.h"
#include "remind_sound.h"
#include "ctrlvars.h"
#include "mode_task.h"
#include "roboeffect_api.h"
#include "audio_effect_parambin.h"
#include "otg_host_audio.h"
#ifdef DUAL_CORE_EN
#include "dualcore_dsp.h"
#include "dualcore_api.h"
#endif
#ifdef CFG_FUNC_EQMODE_FADIN_FADOUT_EN
	extern uint32_t music_eq_mode_unit;
#endif

#ifdef CFG_FUNC_ROBOEFFECT_EN
	#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
		extern osMutexId Parambin1RWMutex;
		extern osMutexId Parambin2RWMutex;
	#endif
extern bool RoboeffectOK;
__attribute__((optimize("Og")))
void AudioMusicProcess(AudioCoreContext *pAudioCore, uint8_t Net)
{
	#ifdef CFG_APP_USB_AUDIO_MODE_EN
	PCM_DATA_TYPE *usb_out	= NULL;
	#endif

	#ifdef CFG_APP_USB_AUDIO_MODE_EN
	if(pAudioCore->AudioSink[USB_AUDIO_SINK_NUM].Active == TRUE && pAudioCore->AudioSink[USB_AUDIO_SINK_NUM].Net == Net)
	{
		usb_out = pAudioCore->AudioSink[USB_AUDIO_SINK_NUM].PcmOutBuf;
	}
	#endif

//	roboeffect_apply(AudioEffect.context_memory);


	if(RoboeffectOK == true)
	{
		if(Net == DefaultNet)
		{
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
			osMutexLock(Parambin1RWMutex);
#endif
			if(EffectParambin.context_memory != NULL)
			{
				{
		#if !defined(DUAL_CORE_EN) || (defined(CFG_PARA_CORE2_NET) && (CFG_PARA_CORE2_NET == SeparateNet))
					roboeffect_apply(EffectParambin.context_memory);
		#else
					DualCoreDspRoboeffectApply(E_PING);
		#endif
				}
			}
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
			osMutexUnlock(Parambin1RWMutex);
#endif
		}
	#if defined(ROBOEFFECT_DOUBLE_ENGINE_EN)// && !defined(CFG_FUNC_EFFECT_CORE_DUAL_EN)
		else
		{
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
		osMutexLock(Parambin2RWMutex);
#endif
			if(EffectParambin2.context_memory != NULL)
			{
			#if !defined(DUAL_CORE_EN) || (defined(CFG_PARA_CORE2_NET) && (CFG_PARA_CORE2_NET == DefaultNet))
					roboeffect_apply(EffectParambin2.context_memory);
					//DBG("+");
			#else
					DualCoreDspRoboeffectApply(E_PING2);
			#endif
			}
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
			osMutexUnlock(Parambin2RWMutex);
#endif
		}//Net == SeparateNet
	#endif
	}


	#ifdef CFG_APP_USB_AUDIO_MODE_EN
	if(usb_out)
	{
		if(pAudioCore->AudioSink[USB_AUDIO_SINK_NUM].Channels == 1)
		{
			for(int16_t s = 0; s < AudioCoreFrameSizeGet(Net); s++)
			{
		#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
				usb_out[s] = __nds32__clips(((int32_t)usb_out[2*s] + (int32_t)usb_out[2*s + 1] +1 ) >>1, 24-1);
		#else
				usb_out[s] = __nds32__clips(((int32_t)usb_out[2*s] + (int32_t)usb_out[2*s + 1] +1 ) >>1, 16-1);
		#endif
			}
		}
	}
	#endif


	ROBOEFFECT_ERROR_CODE  err_code_read;
	if(Net == DefaultNet)
	{
		err_code_read = roboeffect_get_error_code(EffectParambin.context_memory);
		if(err_code_read != ROBOEFFECT_ERROR_OK)
		{
			DBG("error code：%d@effect1\n", err_code_read);
			DBG("error code表示含义参考 middleware\\roboeffect\\documents\\roboeffect_user_manual.release.html 尾部附录部分");
		}
	}
	#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
	else
	{
		err_code_read = roboeffect_get_error_code(EffectParambin2.context_memory);
		if(err_code_read != ROBOEFFECT_ERROR_OK)
		{
			DBG("error code：%d@effect2\n", err_code_read);
			DBG("error code表示含义参考 middleware\\roboeffect\\documents\\roboeffect_user_manual.release.html 尾部附录部分");
		}
	}
	#endif
}


#if (defined(CFG_APP_BT_MODE_EN) && (BT_HFP_SUPPORT))
//#define BTHF_AUDIOEFFECT_BYPASS  //目的:便于调试,直接进行通路数据的搬运,不应用V3音效流程
void AudioEffectProcessBTHF(AudioCoreContext *pAudioCore, uint8_t Net)
{
	#ifdef BTHF_AUDIOEFFECT_BYPASS //audio effect bypass
	uint16_t n = AudioCoreFrameSizeGet(Net);
	#endif


	#ifdef BTHF_AUDIOEFFECT_BYPASS
	int16_t  *hf_mic_in     = pAudioCore->AudioSource[MIC_SOURCE_NUM].PcmInBuf;//蓝牙通话mic采样buffer
	int16_t  *hf_pcm_in     = pAudioCore->AudioSource[APP_SOURCE_NUM].PcmInBuf;//蓝牙通话下传buffer
//	int16_t  *hf_aec_in		= NULL;//pBuf->hf_aec_in;;//蓝牙通话下传delay buffer,专供aec算法处理
	int16_t  *hf_pcm_out    = pAudioCore->AudioSink[AUDIO_APP_SINK_NUM].PcmOutBuf;//蓝牙通话上传buffer
	int16_t  *hf_dac_out    = pAudioCore->AudioSink[AUDIO_DAC_SINK_NUM].PcmOutBuf;//蓝牙通话DAC的buffer
//	int16_t  *hf_rec_out    = NULL;//pBuf->hf_rec_out;//蓝牙通话送录音的buffer
	#endif

	#ifdef BTHF_AUDIOEFFECT_BYPASS //audio effect bypass
	int16_t  s;
	for(s = n-1; s >= 0; s--)
	{
		hf_dac_out[s*2 + 0] = hf_pcm_in[s];
		hf_dac_out[s*2 + 1] = hf_pcm_in[s];
	}

	for(s = 0; s < n; s++)
	{//mic单声道
		hf_pcm_out[s] = hf_mic_in[s];
	}
	#else//#ifdef BTHF_AUDIOEFFECT_BYPASS
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
	if(Net == DefaultNet)
		osMutexLock(Parambin1RWMutex);
#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
	else
		osMutexLock(Parambin2RWMutex);
#endif
#endif
	if(RoboeffectOK == true)
	{
		#ifndef DUAL_CORE_EN
		roboeffect_apply(EffectParambin.context_memory);
		//DBG("-");
		#else//#ifndef DUAL_CORE_EN
		DualCoreDspRoboeffectApply(E_PING);
		#endif//#ifndef DUAL_CORE_EN
	}
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
	if(Net == DefaultNet)
		osMutexUnlock(Parambin1RWMutex);
#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
	else
		osMutexUnlock(Parambin2RWMutex);
#endif
#endif
	#endif//#ifdef BTHF_AUDIOEFFECT_BYPASS

}
#endif //#if (defined(CFG_APP_BT_MODE_EN) && (BT_HFP_SUPPORT))
#endif //#ifdef CFG_FUNC_ROBOEFFECT_EN
/*
****************************************************************
* 无音效音频处理主函数
*
****************************************************************
*/
void AudioBypassProcess(AudioCoreContext *pAudioCore, uint8_t Net)
{
	int16_t  s;
	uint16_t n = AudioCoreFrameSizeGet(Net);
	PCM_DATA_TYPE *mic_pcm    	= NULL;//pBuf->mic_in;///mic input
	PCM_DATA_TYPE *monitor_out  = NULL;//pBuf->dac0_out;
	PCM_DATA_TYPE *music_pcm    = NULL;//pBuf->app_in;
#ifdef CFG_FUNC_REMIND_SOUND_EN
	PCM_DATA_TYPE *remind_in 	= NULL;
#endif
#if defined(CFG_RES_USB_HS_MIX_AUDIO_EN)
	PCM_DATA_TYPE *usb_speaker	= NULL;
	PCM_DATA_TYPE *usb_mic	= NULL;
#endif
#ifdef CFG_FUNC_LINEIN_MIX_MODE
	PCM_DATA_TYPE *linein_mixer_pcm    = NULL;//pBuf->linein_mixer;
#endif
#ifdef CFG_I2S0_MIX_IN_EN
	PCM_DATA_TYPE *i2s0_mixer_in		  = NULL;//pBuf->i2s1_in;
#endif
#ifdef CFG_I2S1_MIX_IN_EN
	PCM_DATA_TYPE *i2s1_mixer_in	  = NULL;//pBuf->i2s1_in;
#endif
#ifdef CFG_I2S2_MIX_IN_EN
	PCM_DATA_TYPE *i2s2_mixer_in	  = NULL;//pBuf->i2s1_in;
#endif

#ifdef CFG_FUNC_USB_HOST_AUDIO_MIX_MODE
	PCM_DATA_TYPE *usb_host_pcm_in   = NULL;//pBuf->app_in;
	PCM_DATA_TYPE *usb_host_pcm_out  = NULL;//Buf->app_out;
#endif

#if	(defined(CFG_APP_BT_MODE_EN) && (BT_HFP_SUPPORT)) || defined(CFG_APP_USB_AUDIO_MODE_EN) || (defined(FUNC_WIRELESS2_SUPPORT))
	PCM_DATA_TYPE *mode_out  	= NULL;//pBuf->app_out;
#endif
	PCM_DATA_TYPE *i2s_out      = NULL;//pBuf->i2s0_out;
#if (BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT)
	PCM_DATA_TYPE *le_out = NULL;
#endif
#ifdef CFG_I2S0_MIX_OUT_EN
	PCM_DATA_TYPE *i2s0_mixer_out      = NULL;//pBuf->i2s1_out;
#endif
#ifdef CFG_I2S1_MIX_OUT_EN
	PCM_DATA_TYPE *i2s1_mixer_out      = NULL;//pBuf->i2s0_out;
#endif

#ifdef CFG_I2S2_MIX_OUT_EN
	PCM_DATA_TYPE *i2s2_mixer_out      = NULL;//pBuf->i2s0_out;
#endif
#ifdef CFG_RES_AUDIO_DAC1_MIX_OUT_EN
	PCM_DATA_TYPE *dac1_mixer_out      = NULL;
#endif

#if (BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT)
    if(pAudioCore->AudioSink[LE_AUDIO_SINK_NUM].Active == TRUE)
	{
    	le_out = pAudioCore->AudioSink[LE_AUDIO_SINK_NUM].PcmOutBuf;
	}
#endif

#if defined(CFG_RES_USB_HS_MIX_AUDIO_EN)
	if(pAudioCore->AudioSink[USB_HS_MIX_SINK_NUM].Active == TRUE)   ////dac monitor buff
	{
		usb_mic = pAudioCore->AudioSink[USB_HS_MIX_SINK_NUM].PcmOutBuf;
	}
#endif

#ifdef CFG_RES_DAC_MONITOR_EN
	if(pAudioCore->AudioSink[AUDIO_DAC_SINK_NUM].Active == TRUE)   ////dac monitor buff
	{
		monitor_out = pAudioCore->AudioSink[AUDIO_DAC_SINK_NUM].PcmOutBuf;
	}
#endif
#ifdef CFG_FUNC_USB_HOST_AUDIO_MIX_MODE
	if(pAudioCore->AudioSink[USB_HOST_SINK_NUM].Active == TRUE)   ////dac monitor buff
	{
		usb_host_pcm_out  =pAudioCore->AudioSink[USB_HOST_SINK_NUM].PcmOutBuf;
	}
#endif
#if	(defined(CFG_APP_BT_MODE_EN) && (BT_HFP_SUPPORT)) || defined(CFG_APP_USB_AUDIO_MODE_EN) || (defined(FUNC_WIRELESS2_SUPPORT))
	if(pAudioCore->AudioSink[AUDIO_APP_SINK_NUM].Active == TRUE)
	{
		mode_out = pAudioCore->AudioSink[AUDIO_APP_SINK_NUM].PcmOutBuf;
	}
#endif
#ifdef CFG_I2S0_MIX_OUT_EN
	if(pAudioCore->AudioSink[AUDIO_I2S0_MIX_OUT_SINK_NUM].Active == TRUE)
	{
		i2s0_mixer_out = pAudioCore->AudioSink[AUDIO_I2S0_MIX_OUT_SINK_NUM].PcmOutBuf;
	}
#endif
#ifdef CFG_I2S1_MIX_OUT_EN
	if(pAudioCore->AudioSink[AUDIO_I2S1_MIX_OUT_SINK_NUM].Active == TRUE)
	{
		i2s1_mixer_out = pAudioCore->AudioSink[AUDIO_I2S1_MIX_OUT_SINK_NUM].PcmOutBuf;
	}
#endif

#ifdef CFG_I2S2_MIX_OUT_EN
	if(pAudioCore->AudioSink[AUDIO_I2S2_MIX_OUT_SINK_NUM].Active == TRUE)
	{
		i2s2_mixer_out = pAudioCore->AudioSink[AUDIO_I2S2_MIX_OUT_SINK_NUM].PcmOutBuf;
	}
#endif

#ifdef CFG_RES_AUDIO_DAC1_MIX_OUT_EN
	if(pAudioCore->AudioSink[AUDIO_DAC_MIX1_OUT_SINK_NUM].Active == TRUE)
	{
		dac1_mixer_out = pAudioCore->AudioSink[AUDIO_DAC_MIX1_OUT_SINK_NUM].PcmOutBuf;
	}
#endif

#ifdef CFG_FUNC_REMIND_SOUND_EN
	remind_in = pAudioCore->AudioSource[REMIND_SOURCE_NUM].PcmInBuf;
#endif

#if defined(CFG_RES_USB_HS_MIX_AUDIO_EN)
	if(pAudioCore->AudioSource[USB_HS_MIX_SOURCE_NUM].Active)
	{
		usb_speaker = pAudioCore->AudioSource[USB_HS_MIX_SOURCE_NUM].PcmInBuf;
	}
#endif
	if(pAudioCore->AudioSource[APP_SOURCE_NUM].Active)
	{
		music_pcm = pAudioCore->AudioSource[APP_SOURCE_NUM].PcmInBuf;
	}
	if(pAudioCore->AudioSource[MIC_SOURCE_NUM].Active)
	{
		mic_pcm = pAudioCore->AudioSource[MIC_SOURCE_NUM].PcmInBuf;
	}
#ifdef CFG_FUNC_LINEIN_MIX_MODE
	if(pAudioCore->AudioSource[LINEIN_MIX_SOURCE_NUM].Active)
	{
		linein_mixer_pcm = pAudioCore->AudioSource[LINEIN_MIX_SOURCE_NUM].PcmInBuf;
	}
#endif
#ifdef CFG_I2S0_MIX_IN_EN
	if(pAudioCore->AudioSource[I2S0_MIX_SOURCE_NUM].Active)
	{
		i2s0_mixer_in = pAudioCore->AudioSource[I2S0_MIX_SOURCE_NUM].PcmInBuf;
	}
#endif
#ifdef CFG_I2S1_MIX_IN_EN
	if(pAudioCore->AudioSource[I2S1_MIX_SOURCE_NUM].Active)
	{
		i2s1_mixer_in = pAudioCore->AudioSource[I2S1_MIX_SOURCE_NUM].PcmInBuf;
	}
#endif
#ifdef CFG_I2S2_MIX_IN_EN
	if(pAudioCore->AudioSource[I2S2_MIX_SOURCE_NUM].Active)
	{
		i2s2_mixer_in = pAudioCore->AudioSource[I2S2_MIX_SOURCE_NUM].PcmInBuf;
	}
#endif
#ifdef CFG_FUNC_USB_HOST_AUDIO_MIX_MODE
	if(pAudioCore->AudioSource[USB_HOST_SOURCE_NUM].Active)
	{
		usb_host_pcm_in = pAudioCore->AudioSource[USB_HOST_SOURCE_NUM].PcmInBuf;
	}
#endif
	if(monitor_out || i2s_out)
	{
		if(monitor_out == NULL)
			monitor_out = i2s_out;

	#ifdef CFG_FUNC_REMIND_SOUND_EN
		if(pAudioCore->AudioSource[REMIND_SOURCE_NUM].Active)
		{
			for(s = 0; s < n; s++)
			{
				monitor_out[2 * s + 0] = remind_in[s];
				monitor_out[2 * s + 1] = remind_in[s];
#if defined(CFG_RES_USB_HS_MIX_AUDIO_EN)
				if(usb_speaker)
				{
					monitor_out[2 * s + 0] = __nds32__clips(((int32_t)monitor_out[2 * s + 0] + usb_speaker[2 * s + 0] + 1), 24-1);
					monitor_out[2 * s + 1] = __nds32__clips(((int32_t)monitor_out[2 * s + 1] + usb_speaker[2 * s + 1] + 1), 24-1);
				}
#endif
			}

		}
		else
	#endif
		{
			if(music_pcm)
			{
				if(mic_pcm)
				{
					for(s = 0; s < n; s++)
					{
	#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
						monitor_out[2 * s + 0] = __nds32__clips(((int32_t)music_pcm[2 * s + 0] + (int32_t)mic_pcm[2 * s + 0] + 1), 24-1);
						monitor_out[2 * s + 1] = __nds32__clips(((int32_t)music_pcm[2 * s + 1] + (int32_t)mic_pcm[2 * s + 1] + 1), 24-1);
#if defined(CFG_RES_USB_HS_MIX_AUDIO_EN)
						if(usb_speaker)
						{
							monitor_out[2 * s + 0] = __nds32__clips(((int32_t)monitor_out[2 * s + 0] + (int32_t)usb_speaker[2 * s + 0] +1), 24-1);
							monitor_out[2 * s + 1] = __nds32__clips(((int32_t)monitor_out[2 * s + 1] + (int32_t)usb_speaker[2 * s + 1] +1), 24-1);
						}
#endif
		#if defined(CFG_RES_USB_HS_MIX_AUDIO_EN)
						if(usb_mic)
						{
							usb_mic[2 * s + 0] = __nds32__clips(((int32_t)music_pcm[2 * s + 0] + (int32_t)mic_pcm[2 * s + 0] + 1), 24-1);
							usb_mic[2 * s + 1] = __nds32__clips(((int32_t)music_pcm[2 * s + 1] + (int32_t)mic_pcm[2 * s + 1] + 1), 24-1);
						}
		#endif
	#else
						monitor_out[2 * s + 0] = __nds32__clips(((int32_t)music_pcm[2 * s + 0] + (int32_t)mic_pcm[2 * s + 0] + 1), 16-1);
						monitor_out[2 * s + 1] = __nds32__clips(((int32_t)music_pcm[2 * s + 1] + (int32_t)mic_pcm[2 * s + 1] + 1), 16-1);
	#endif
					}
				}
				else
				{
					for(s = 0; s < n; s++)
					{
						monitor_out[2 * s + 0] = music_pcm[2 * s + 0];
						monitor_out[2 * s + 1] = music_pcm[2 * s + 1];
#if defined(CFG_RES_USB_HS_MIX_AUDIO_EN)
						if(usb_speaker)
						{
							monitor_out[2 * s + 0] = __nds32__clips(((int32_t)monitor_out[2 * s + 0] + (int32_t)usb_speaker[2 * s + 0] +1), 24-1);
							monitor_out[2 * s + 1] = __nds32__clips(((int32_t)monitor_out[2 * s + 1] + (int32_t)usb_speaker[2 * s + 1] +1), 24-1);
						}
#endif
					}
				}
#ifdef CFG_FUNC_USB_HOST_AUDIO_MIX_MODE
				if(usb_host_pcm_out)
				{
					for(s = 0; s < n; s++)
					{
						usb_host_pcm_out[2*s + 0] = music_pcm[2*s + 0];
						usb_host_pcm_out[2*s + 1] = music_pcm[2*s + 1];
		//				usb_host_pcm_out[2*s + 0] = monitor_out[2*s + 0];
		//				usb_host_pcm_out[2*s + 1] = monitor_out[2*s + 1];
					}
				}
#endif
			}
			else if(mic_pcm)
			{
				for(s = 0; s < n; s++)
				{
					monitor_out[2 * s + 0] = mic_pcm[2 * s + 0];
					monitor_out[2 * s + 1] = mic_pcm[2 * s + 1];
#if defined(CFG_RES_USB_HS_MIX_AUDIO_EN)
					if(usb_speaker)
					{
						monitor_out[2 * s + 0] = __nds32__clips(((int32_t)monitor_out[2 * s + 0] + (int32_t)usb_speaker[2 * s + 0] +1), 24-1);
						monitor_out[2 * s + 1] = __nds32__clips(((int32_t)monitor_out[2 * s + 1] + (int32_t)usb_speaker[2 * s + 1] +1), 24-1);
					}
#endif
				}
			}
#ifdef CFG_I2S0_MIX_IN_EN
			if(i2s0_mixer_in)
			{
				for(s = 0; s < n; s++)
				{
#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
					monitor_out[2 * s + 0] = __nds32__clips(((int32_t)monitor_out[2 * s + 0] + (int32_t)i2s0_mixer_in[2 * s + 0] + 1)>>1, 24-1);
					monitor_out[2 * s + 1] = __nds32__clips(((int32_t)monitor_out[2 * s + 1] + (int32_t)i2s0_mixer_in[2 * s + 1] + 1)>>1, 24-1);
#else
					monitor_out[2 * s + 0] = __nds32__clips(((int32_t)monitor_out[2 * s + 0] + (int32_t)i2s0_mixer_in[2 * s + 0] + 1)>>1, 16-1);
					monitor_out[2 * s + 1] = __nds32__clips(((int32_t)monitor_out[2 * s + 1] + (int32_t)i2s0_mixer_in[2 * s + 1] + 1)>>1, 16-1);
#endif
				}
			}
#endif
#ifdef CFG_I2S1_MIX_IN_EN
			if(i2s1_mixer_in)
			{
				for(s = 0; s < n; s++)
				{
#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
					monitor_out[2 * s + 0] = __nds32__clips(((int32_t)monitor_out[2 * s + 0] + (int32_t)i2s1_mixer_in[2 * s + 0] + 1)>>1, 24-1);
					monitor_out[2 * s + 1] = __nds32__clips(((int32_t)monitor_out[2 * s + 1] + (int32_t)i2s1_mixer_in[2 * s + 1] + 1)>>1, 24-1);
#else
					monitor_out[2 * s + 0] = __nds32__clips(((int32_t)monitor_out[2 * s + 0] + (int32_t)i2s1_mixer_in[2 * s + 0] + 1)>>1, 16-1);
					monitor_out[2 * s + 1] = __nds32__clips(((int32_t)monitor_out[2 * s + 1] + (int32_t)i2s1_mixer_in[2 * s + 1] + 1)>>1, 16-1);
#endif
				}
			}
#endif
#ifdef CFG_I2S2_MIX_IN_EN
			if(i2s2_mixer_in)
			{
				for(s = 0; s < n; s++)
				{
#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
					monitor_out[2 * s + 0] = __nds32__clips(((int32_t)monitor_out[2 * s + 0] + (int32_t)i2s2_mixer_in[2 * s + 0] + 1)>>1, 24-1);
					monitor_out[2 * s + 1] = __nds32__clips(((int32_t)monitor_out[2 * s + 1] + (int32_t)i2s2_mixer_in[2 * s + 1] + 1)>>1, 24-1);
#else
					monitor_out[2 * s + 0] = __nds32__clips(((int32_t)monitor_out[2 * s + 0] + (int32_t)i2s2_mixer_in[2 * s + 0] + 1)>>1, 16-1);
					monitor_out[2 * s + 1] = __nds32__clips(((int32_t)monitor_out[2 * s + 1] + (int32_t)i2s2_mixer_in[2 * s + 1] + 1)>>1, 16-1);
#endif
				}
			}
#endif
#ifdef CFG_FUNC_LINEIN_MIX_MODE
			if(linein_mixer_pcm)
			{
				for(s = 0; s < n; s++)
				{
#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
					monitor_out[2 * s + 0] = __nds32__clips(((int32_t)monitor_out[2 * s + 0] + (int32_t)linein_mixer_pcm[2 * s + 0] + 1)>>1, 24-1);
					monitor_out[2 * s + 1] = __nds32__clips(((int32_t)monitor_out[2 * s + 1] + (int32_t)linein_mixer_pcm[2 * s + 1] + 1)>>1, 24-1);
#else
					monitor_out[2 * s + 0] = __nds32__clips(((int32_t)monitor_out[2 * s + 0] + (int32_t)linein_mixer_pcm[2 * s + 0] + 1)>>1, 16-1);
					monitor_out[2 * s + 1] = __nds32__clips(((int32_t)monitor_out[2 * s + 1] + (int32_t)linein_mixer_pcm[2 * s + 1] + 1)>>1, 16-1);
#endif
				}
			}
#endif

#ifdef CFG_FUNC_USB_HOST_AUDIO_MIX_MODE
			if(usb_host_pcm_in)
			{
				for(s = 0; s < n; s++)
				{
#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
					monitor_out[2 * s + 0] = __nds32__clips(((int32_t)monitor_out[2 * s + 0] + (int32_t)usb_host_pcm_in[2 * s + 0] + 1)>>1, 24-1);
					monitor_out[2 * s + 1] = __nds32__clips(((int32_t)monitor_out[2 * s + 1] + (int32_t)usb_host_pcm_in[2 * s + 1] + 1)>>1, 24-1);
#else
					monitor_out[2 * s + 0] = __nds32__clips(((int32_t)monitor_out[2 * s + 0] + (int32_t)usb_host_pcm_in[2 * s + 0] + 1)>>1, 16-1);
					monitor_out[2 * s + 1] = __nds32__clips(((int32_t)monitor_out[2 * s + 1] + (int32_t)usb_host_pcm_in[2 * s + 1] + 1)>>1, 16-1);
#endif
				}
			}
#endif
#if defined(CFG_FUNC_USB_AUDIO_MIX_MODE)
			if(usb_speaker)
			{
				for(s = 0; s < n; s++)
				{
	#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
					monitor_out[2 * s + 0] = __nds32__clips(((int32_t)monitor_out[2 * s + 0] + (int32_t)usb_speaker[2 * s + 0] + 1)>>1, 24-1);
					monitor_out[2 * s + 1] = __nds32__clips(((int32_t)monitor_out[2 * s + 1] + (int32_t)usb_speaker[2 * s + 1] + 1)>>1, 24-1);
	#else
					monitor_out[2 * s + 0] = __nds32__clips(((int32_t)monitor_out[2 * s + 0] + (int32_t)usb_speaker[2 * s + 0] + 1)>>1, 16-1);
					monitor_out[2 * s + 1] = __nds32__clips(((int32_t)monitor_out[2 * s + 1] + (int32_t)usb_speaker[2 * s + 1] + 1)>>1, 16-1);
	#endif
				}
			}
#endif
		}

		if(i2s_out && i2s_out != monitor_out)
		{
			for(s = 0; s < n; s++)
			{
				i2s_out[2*s + 0] = monitor_out[2*s + 0];
				i2s_out[2*s + 1] = monitor_out[2*s + 1];
			}
		}
#if (BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT)
		if(le_out)
		{
			for(s = 0; s < n; s++)
			{
				le_out[2*s + 0] = monitor_out[2*s + 0];
				le_out[2*s + 1] = monitor_out[2*s + 1];
			}
		}
#endif
#ifdef CFG_I2S0_MIX_OUT_EN
		if(i2s0_mixer_out && i2s0_mixer_out != monitor_out)
		{
			for(s = 0; s < n; s++)
			{
				i2s0_mixer_out[2*s + 0] = monitor_out[2*s + 0];
				i2s0_mixer_out[2*s + 1] = monitor_out[2*s + 1];
			}
		}
#endif
#ifdef CFG_I2S1_MIX_OUT_EN
		if(i2s1_mixer_out && i2s1_mixer_out != monitor_out)
		{
			for(s = 0; s < n; s++)
			{
				i2s1_mixer_out[2*s + 0] = monitor_out[2*s + 0];
				i2s1_mixer_out[2*s + 1] = monitor_out[2*s + 1];
			}
		}
#endif
#ifdef CFG_I2S2_MIX_OUT_EN
		if(i2s2_mixer_out && i2s2_mixer_out != monitor_out)
		{
			for(s = 0; s < n; s++)
			{
				i2s2_mixer_out[2*s + 0] = monitor_out[2*s + 0];
				i2s2_mixer_out[2*s + 1] = monitor_out[2*s + 1];
			}
		}
#endif

#ifdef CFG_RES_AUDIO_DAC1_MIX_OUT_EN
		if(dac1_mixer_out && dac1_mixer_out != monitor_out)
		{
			for(s = 0; s < n; s++)
			{
				dac1_mixer_out[2*s + 0] = monitor_out[2*s + 0];
				dac1_mixer_out[2*s + 1] = monitor_out[2*s + 1];
			}
		}
#endif
	}

#if	(defined(CFG_APP_BT_MODE_EN) && (BT_HFP_SUPPORT)) || defined(CFG_APP_USB_AUDIO_MODE_EN) || (defined(FUNC_WIRELESS2_SUPPORT))
	if(mode_out)
	{
		if(pAudioCore->AudioSink[AUDIO_APP_SINK_NUM].Channels == 1)
		{
			for(s = 0; s < n; s++)
			{
	#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
				mode_out[s] = __nds32__clips(((int32_t)mic_pcm[2 * s] + (int32_t)mic_pcm[2 * s + 1] + 1)>>1, 24-1);
	#else
				mode_out[s] = __nds32__clips(((int32_t)mic_pcm[2 * s] + (int32_t)mic_pcm[2 * s + 1] + 1)>>1, 16-1);
	#endif
			}
		}
		else
		{
			for(s = 0; s < n; s++)
			{
				mode_out[2 * s + 0] = mic_pcm[2 * s + 0];
				mode_out[2 * s + 1] = mic_pcm[2 * s + 1];
#if defined(CFG_RES_USB_HS_MIX_AUDIO_EN)
				if(usb_speaker)
				{
					mode_out[2 * s + 0] = usb_speaker[2 * s + 0];
					mode_out[2 * s + 1] = usb_speaker[2 * s + 1];
				}
#endif
			}
		}
	}
#endif

}

//无App通路音频处理主函数
void AudioNoAppProcess(AudioCoreContext *pAudioCore, uint8_t Net)
{
	int16_t  s;
	uint16_t n = AudioCoreFrameSizeGet(Net);
	PCM_DATA_TYPE *mic_pcm    	= NULL;//pBuf->mic_in;///mic input
	PCM_DATA_TYPE *monitor_out    = NULL;//pBuf->dac0_out;
#ifdef	CFG_FUNC_REMIND_SOUND_EN
	PCM_DATA_TYPE *remind_in = NULL;
#endif
	PCM_DATA_TYPE *i2s_out       = NULL;//pBuf->i2s0_out;

	if(pAudioCore->AudioSource[MIC_SOURCE_NUM].Active == TRUE)////mic buff
	{
		mic_pcm = pAudioCore->AudioSource[MIC_SOURCE_NUM].PcmInBuf;//双mic输入
	}

    if(pAudioCore->AudioSink[AUDIO_DAC_SINK_NUM].Active == TRUE)   ////dac0 buff
	{
    	monitor_out = pAudioCore->AudioSink[AUDIO_DAC_SINK_NUM].PcmOutBuf;
	}

#if defined(CFG_FUNC_REMIND_SOUND_EN)
	if(pAudioCore->AudioSource[REMIND_SOURCE_NUM].Active == TRUE)////remind buff
	{
		remind_in = pAudioCore->AudioSource[REMIND_SOURCE_NUM].PcmInBuf;
	}
#endif

    if(monitor_out)
	{
#ifdef CFG_FUNC_ROBOEFFECT_EN
		memset(monitor_out, 0, roboeffect_get_buffer_size(EffectParambin.context_memory));
#endif
    }

    if(i2s_out)
	{
#ifdef CFG_FUNC_ROBOEFFECT_EN
		memset(i2s_out, 0, roboeffect_get_buffer_size(EffectParambin.context_memory));
#endif
    }

	//DAC0立体声监听音效处理
	if(monitor_out || i2s_out)
	{
		if(monitor_out == NULL)
			monitor_out = i2s_out;
		for(s = 0; s < n; s++)
		{
		#if defined(CFG_FUNC_REMIND_SOUND_EN)
			if(remind_in)
			{
				monitor_out[2*s + 0] = remind_in[s];
				monitor_out[2*s + 1] = remind_in[s];
			}
			else
		#endif
			if(mic_pcm)
			{
				monitor_out[2*s + 0] = mic_pcm[2*s + 0];
				monitor_out[2*s + 1] = mic_pcm[2*s + 1];
			}
		}

		if(i2s_out && i2s_out != monitor_out)
		{
			for(s = 0; s < n; s++)
			{
				i2s_out[2*s + 0] = monitor_out[2*s + 0];
				i2s_out[2*s + 1] = monitor_out[2*s + 1];
			}
		}
	}

}
