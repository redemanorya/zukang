/*
 * mode_task_api.c
 *
 *  Created on: Mar 30, 2021
 *      Author: piwang
 */
//driver、lib
#include "dma.h"
#include "watchdog.h"
#include "clk.h"
#include "reset.h"
#include "chip_interrupt.h"
#include "otg_device_hcd.h"
#include "adc_interface.h"
#include "dac_interface.h"
#include "i2s_interface.h"

//application
#include "app_config.h"
#include "otg_host_standard_enum.h"
#include "debug.h"
#include "main_task.h"
#include "audio_core_service.h"
#include "effect_service.h"
#include "roboeffect_prot.h"
#include "ctrlvars.h"
#include "breakpoint.h"
#include "audio_effect_control.h"
#include "remind_sound.h"
#include "bt_manager.h"
#include "audio_effect.h"
#include "hdmi_in_api.h"
#include "roboeffect_api.h"
#include "communication.h"
#include "bridge.h"
#include "audio_vol.h"
#include "bt_stack_service.h"
#include "bt_app_ddb_info.h"
#include "mvwireless2_config.h"
#if (BT_TWS_SUPPORT == ENABLE)
#include "bt_tws_mode.h"
#include "bt_tws_api.h"
#include "dac.h"
#include "module_config.h"
#endif

#if (BT_LE_AUDIO_SUPPORT)
#include "le_audio_config.h"
// LeAudio 相关函数声明
extern bool sink_wait_starting;

extern void LeAudioSrcResume(void);
extern void LeAudioSrcPause(void);
extern void le_bis_sink_start(void);
#ifdef SINK_CHOOSE_SRC
extern void app_add_src_for_sink(uint8_t src_idx);
#endif
#endif
#if (BT_AVRCP_VOLUME_SYNC)
#include "bt_app_avrcp_deal.h"
#endif

#ifdef	FUNC_WIRELESS2_SUPPORT
#include "wireless2_audiomain.h"
#endif
//Port#include "wireless_service.h"

#ifdef CFG_FUNC_I2S0_MIX
extern bool I2S0_MixInit(bool hf_mode_flag);
extern void I2S0_MixDeinit(void);
#endif
#ifdef CFG_FUNC_I2S1_MIX
extern bool I2S1_MixInit(bool hf_mode_flag);
extern void I2S1_MixDeinit(void);
#endif
#ifdef CFG_FUNC_I2S2_MIX
extern bool I2S2_MixInit(bool hf_mode_flag);
extern void I2S2_MixDeinit(void);
#endif
#ifdef CFG_RES_AUDIO_DAC1_MIX_OUT_EN
extern bool DAC1_MixInit(void);
extern bool DAC1_MixDeinit(void);
#endif
#ifdef CFG_RES_AUDIO_DAC2_MIX_OUT_EN
extern bool DAC2_MixInit(void);
extern bool DAC2_MixDeinit(void);
#endif
#ifdef CFG_FUNC_LINEIN_MIX_MODE
extern bool LineInMixPlayInit(void);
extern bool LineInMixPlayDeinit(void);
#endif
#ifdef CFG_FUNC_USB_AUDIO_MIX_MODE
extern bool UsbDevicePlayMixInit(void);
extern bool UsbDevicePlayMixDeinit(void);
#endif
#if defined(CFG_RES_AUDIO_SPDIF0OUT_EN) || defined(CFG_RES_AUDIO_SPDIF1OUT_EN)
extern bool SpdifOutInit(void);
extern bool SpdifOutDeinit(void);
#endif

#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
extern int32_t RemindMp3DecoderInit(void);
extern int32_t RemindMp3DecoderDeinit(void);
#endif

#ifdef CFG_RES_USB_FS_MIX_AUDIO_EN
extern bool UsbFSDevicePlayMixInit(void); // full speed
extern bool UsbFSDevicePlayMixDeinit(void);//full speed
#endif

//#ifdef CFG_RES_USB_HS_MIX_AUDIO_EN
//extern bool UsbHSDevicePlayMixInit(void); // HS speed
//extern bool UsbHSDevicePlayMixDeinit(void);//HS speed
//#endif

extern bool EffectModeCmp(uint8_t mode1,uint8_t mode2);
extern uint8_t EffectModeMsgProcess(uint16_t Msg);
extern uint8_t AudioCommonMsgProcess(uint16_t Msg);
extern uint32_t IsBtHfMode(void);

#ifdef CFG_FUNC_USB_AUDIO_MIX_MODE  //USB MIX通道，使用 USB_SOURCE_NUM
uint32_t	UsbAudioSourceNum = USB_SOURCE_NUM;

bool IsBtHfpSrc(void)
{
	//hfp模式，不能操作APP_SOURCE_NUM
	if(GetSystemMode() == ModeBtHfPlay &&
	   USB_AUDIO_SOURCE_NUM	 == APP_SOURCE_NUM)
		return 1;

	return 0;
}

bool IsBtHfpSink(void)
{
	//hfp模式，不能操作AUDIO_APP_SINK_NUM，HFP SCO_SINK也是用这个通道
	if(GetSystemMode() == ModeBtHfPlay &&
		USB_AUDIO_SINK_NUM	== AUDIO_APP_SINK_NUM)
		return 1;

	return 0;
}
#endif

#ifdef CFG_FUNC_ROBOEFFECT_EN
uint32_t get_audioeffect_default_frame_info(roboeffect_effect_list_info *user_effect_list,uint32_t type)
{
	uint32_t offset;
	roboeffect_effect_list_info *p;

	extern char __data_lmastart;
	extern char __data_start;
	extern char _edata;

	if((uint32_t)user_effect_list <  (uint32_t)&__data_start ||
	   (uint32_t)user_effect_list >  (uint32_t)&_edata)
	{
		//不在需要初始化值的全局变量区
		return 0;
	}
	offset = (uint32_t)user_effect_list - (uint32_t)(&__data_start);
	p = (roboeffect_effect_list_info *)((uint32_t)&__data_lmastart + offset);

	if(type)
		return p->frame_size;
	else
		return p->sample_rate;
}
#endif//#ifdef CFG_FUNC_ROBOEFFECT_EN

#ifdef CFG_APP_USB_PLAY_MODE_EN
void WaitUdiskUnlock(void)
{
//	if(GetSysModeState(ModeUDiskAudioPlay) == ModeStateDeinit)
//	{
//		while(osMutexLock_1000ms(UDisk_GetMutexId(HOST_DEV_FS)) != TRUE)WDG_Feed();
//	}
//	if(GetSysModeState(ModeUDiskHsAudioPlay) == ModeStateDeinit)
//	{
//		while(osMutexLock_1000ms(UDisk_GetMutexId(HOST_DEV_HS)) != TRUE)WDG_Feed();
//	}
}
#endif
#ifdef CFG_APP_USB_HS_PLAY_MODE_EN
void WaitUdiskHsUnlock(void)
{
//	if(GetSysModeState(ModeUDiskAudioPlay) == ModeStateDeinit)
//	{
//		while(osMutexLock_1000ms(UDisk_GetMutexId(HOST_DEV_FS)) != TRUE)WDG_Feed();
//	}
//	if(GetSysModeState(ModeUDiskHsAudioPlay) == ModeStateDeinit)
//	{
//		while(osMutexLock_1000ms(UDisk_GetMutexId(HOST_DEV_HS)) != TRUE)WDG_Feed();
//	}
}
#endif
#ifdef CFG_APP_CARD_PLAY_MODE_EN
void SDCardForceExitFuc(void)
{
	if(GetSysModeState(ModeCardAudioPlay) == ModeStateDeinit)
	{
//		SDCard_Force_Exit();
	}
}
#endif

#ifdef CFG_AUDIO_OUT_AUTO_SAMPLE_RATE_44100_48000
void AudioOutSampleRateSet(uint32_t SampleRate)
{
	bool	HighSampleRateFlag;
	AUDIOEFFECT_EFFECT_PARA *mpara;
	uint32_t SampleRateTemp;
	uint32_t adc_sampleRate = SampleRate;
    MessageContext msgSend;

	if(IsBtHfMode() || mainAppCt.EffectMode == EFFECT_MODE_HFP_AEC)
		return;

	HighSampleRateFlag = 1;
	SampleRateTemp = SampleRate;

	if((SampleRate == 11025) || (SampleRate == 22050) || (SampleRate == 44100)
			|| (SampleRate == 88200) || (SampleRate == 176400))
	{
		SampleRate = 44100;
	}
	else
	{
		SampleRate = 48000;
	}

#if (CFG_RES_MIC_SELECT) && defined(CFG_FUNC_MIC_EN)
	HighSampleRateFlag = 0;
#endif

#ifdef CFG_RES_DAC_MONITOR_EN
	if(HighSampleRateFlag)
	{
		if(AudioCoreMixSampleRateGet(DefaultNet) != SampleRateTemp)
		{
			mpara = get_user_effect_parameters(mainAppCt.EffectMode);
			mpara->user_effect_list->sample_rate = SampleRateTemp;
			APP_DBG("HighSampleRate: %d --> %d\n", (int)AudioCoreMixSampleRateGet(DefaultNet), (int)SampleRateTemp);

			AudioDAC_SampleRateChange(DAC_SEL, SampleRateTemp);
			DAC_CTRLVAR_SEL.dac_mclk_source = Clock_AudioMclkGet(DAC_AUDIO_MODULE);

			msgSend.msgId = MSG_EFFECTREINIT;
			MessageSend(GetMainMessageHandle(), &msgSend);
		}
		return;
	}
#endif

	if(AudioCoreMixSampleRateGet(DefaultNet) == SampleRate)
		return;
	mpara = get_user_effect_parameters(mainAppCt.EffectMode);
	mpara->user_effect_list->sample_rate = SampleRate;
	APP_DBG("SampleRate: %d --> %d\n", (int)AudioCoreMixSampleRateGet(DefaultNet), (int)SampleRate);

#ifdef CFG_RES_DAC_MONITOR_EN
	AudioDAC_SampleRateChange(DAC_SEL, SampleRate);
	DAC_CTRLVAR_SEL.dac_mclk_source = Clock_AudioMclkGet(DAC_AUDIO_MODULE);
#endif

#if (CFG_RES_MIC_SELECT) && defined(CFG_FUNC_MIC_EN)
	if((adc_sampleRate == 88200) || (adc_sampleRate == 176400))
	{
		adc_sampleRate = 44100;
	}
	else if((adc_sampleRate == 96000) || (adc_sampleRate == 192000))
	{
		adc_sampleRate = 48000;
	}
	AudioADC_SampleRateChange(MIC_ADC_SEL, adc_sampleRate);
	MIC_CTRLDIGITAL_SEL.adc_mclk_source = Clock_AudioMclkGet(MIC_ADC_SEL);
	AudioCoreSourceChange(MIC_SOURCE_NUM, 2, adc_sampleRate);
#endif

    msgSend.msgId = MSG_EFFECTREINIT;
    MessageSend(GetMainMessageHandle(), &msgSend);
}

//切换模式恢复框图 原始采样率
void AudioOutSampleRecover(void)
{
	uint32_t SampleRate;
	uint32_t i;
	AUDIOEFFECT_EFFECT_PARA *para = get_user_effect_parameters(mainAppCt.EffectMode);

	SampleRate = get_audioeffect_default_frame_info(para->user_effect_list,0);
	if(SampleRate && SampleRate != AudioCoreMixSampleRateGet(DefaultNet))
	{
		para->user_effect_list->sample_rate = SampleRate;
		for(i = 0; i < MaxNet; i++)
		{
			AudioCoreMixSampleRateSet(i, SampleRate);//默认系统采样率
		}
	}
}
#endif

void PauseAudioCore(void)
{
	while(GetAudioCoreServiceState() != TaskStatePaused)
	{
		AudioCoreServicePause();
		osTaskDelay(2);
	}
}

uint16_t AudioDACDataSet_LeAudioSink(void* Buf, uint16_t Len)
{
	extern bool get_sink_dac_pcm_invalid_flag(void);

	if(get_sink_dac_pcm_invalid_flag())
	{
		// printf("=========================dac_pcm_invalid_flag \n");
		return 0;
	}

	return DAC_DATASET_SEL(Buf,Len);
}

AUDIODAC_ParamCt DacParamCtCommon;

void AudioDAC_InitSniff(void)
{
#if defined(CFG_RES_DAC_MONITOR_EN)
	if(DAC_SEL == DAC0_MODULE)
	{
		AudioDAC_Init(DAC0_MODULE, &DacParamCtCommon, NULL, NULL);
	}
	else if(DAC_SEL == DAC1_MODULE)
	{
		AudioDAC_Init(DAC1_MODULE, NULL, &DacParamCtCommon, NULL);
	}
	else
#endif
	{
		AudioDAC_Init(DAC2_MODULE, NULL, NULL, &DacParamCtCommon);
	}
}

//配置系统标准通路
bool ModeCommonInit(void)
{
	AudioCoreIO AudioIOSet;

#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
	CommunicationChart = &EffectParambin;
	init_effect_flows_Num();
#ifndef ROBOEFFECT_DOUBLE_ENGINE_EN

#if (BT_TWS_SUPPORT == ENABLE)
	 if(BtTwsStatusGet())
	 {
		AudioEffect_Parambin_SwitchEffectByName("tws", 0, -1);
	 }
	 else
#endif
	 {
#if (BT_LE_AUDIO_SUPPORT)
		 AudioEffect_Parambin_SwitchEffectByName("Le_Audio", 0, -1);
#endif
	 }

#ifdef	FUNC_WIRELESS2_SUPPORT
	AudioEffect_Parambin_SwitchEffectByName("wireless2", 0, -1);
#endif

	if(!AudioEffect_Parambin_Init(&EffectParambin, effect_flow_name[mainAppCt.effect_flow_index], mainAppCt.effect_param_mode_index, 1))

#else
	if(!(AudioEffect_Parambin_Init(&EffectParambin2, effect_flow_name[mainAppCt.effect_flow_index][1], mainAppCt.effect_param_mode_groupindex[1], 0)
				&& AudioEffect_Parambin_Init(&EffectParambin, effect_flow_name[mainAppCt.effect_flow_index][0], mainAppCt.effect_param_mode_groupindex[0], 1)))
#endif
	{
		SetSysModeState(ModeSafe, ModeStateReady);
		SysModeEnter(ModeSafe);
		return FALSE;
	}

	//After effect init done, AudioCore know what frame size should be set.
	AudioCoreFrameSampleSync(&EffectParambin, DefaultNet);
#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
	AudioCoreFrameSampleSync(&EffectParambin2, SeparateNet);
#endif

	init_effect_flows_Status();
#endif

#ifdef CFG_FUNC_BREAKPOINT_EN
//	BackupInfoUpdata(BACKUP_SYS_INFO);
#endif

#ifdef CFG_AUDIO_OUT_AUTO_SAMPLE_RATE_44100_48000
	AudioOutSampleRecover();
#endif

	DefaultParamgsInit();	//refresh local hardware config params(just storage not set)

	//////////申请DMA fifo
#ifdef CFG_RES_DAC_MONITOR_EN
	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
	AudioIOSet.Depth = AudioCore.NetCt[AudioCoreGetNetBySinkChannelNUM(AUDIO_DAC_SINK_NUM)].FrameSize * 3;
	AudioIOSet.PcmWidth	= CFG_DEFAULT_WIDTH;
	if(!AudioCoreSinkIsInit(AUDIO_DAC_SINK_NUM))
	{
		APP_DBG("AudioCoreSinkIsInit AUDIO_DAC_SINK_NUM\n");
#if (BT_LE_AUDIO_SUPPORT && !defined(ROBOEFFECT_DOUBLE_ENGINE_EN))	//双图下面不关闭DAC操作，buffer缓存到BRIDGE_MUSIC_SINK
		mainAppCt.DACFIFO_LEN = get_bis_sync_need_samples(AudioIOSet.Depth) * sizeof(PCM_DATA_TYPE) * 2;
#else
		mainAppCt.DACFIFO_LEN = AudioIOSet.Depth * sizeof(PCM_DATA_TYPE) * 2;
#endif

#if (BT_TWS_SUPPORT == ENABLE)
		if(BtTwsStatusGet() && (TWS_DAC_SEL == DAC0_MODULE
#ifndef CFG_RES_AUDIO_DAC1_MIX_OUT_EN
		|| 	TWS_DAC_SEL == DAC1_MODULE
#endif
		))
		{
			 mainAppCt.DACFIFO_LEN = AudioIOSet.Depth * sizeof(PCM_DATA_TYPE) * 2;
		 }
#endif

		mainAppCt.DACFIFO = (uint32_t*)osPortMalloc(mainAppCt.DACFIFO_LEN);//DAC fifo
		if(mainAppCt.DACFIFO != NULL)
		{
			memset(mainAppCt.DACFIFO, 0, mainAppCt.DACFIFO_LEN);
		}
		else
		{
			APP_DBG("malloc DACFIFO error\n");
			return FALSE;
		}

		//sink0
		AudioIOSet.Adapt = STD;
		AudioIOSet.Sync = TRUE;
		AudioIOSet.Channels = 2;
		AudioIOSet.Net = AudioCoreGetNetBySinkChannelNUM(AUDIO_DAC_SINK_NUM);

	#if (BT_LE_AUDIO_SUPPORT)
		if(Get_bis_role() == BIS_SINK)
			AudioIOSet.DataIOFunc = AudioDACDataSet_LeAudioSink;
		else
	#endif
#ifdef	FUNC_WIRELESS2_SUPPORT
		AudioIOSet.DataIOFunc = wireless_AudioDACSet;
#else
		AudioIOSet.DataIOFunc = DAC_DATASET_SEL;
#endif
		AudioIOSet.LenGetFunc = DAC_LENGET_SEL;

	#if (BT_TWS_SUPPORT == ENABLE)
		if(BtTwsStatusGet() && (TWS_DAC_SEL == DAC0_MODULE
#ifndef CFG_RES_AUDIO_DAC1_MIX_OUT_EN
		|| 	TWS_DAC_SEL == DAC1_MODULE
#endif
		))
		{
		AudioIOSet.DataIOFunc = AudioDAC0DataSet_tws;
		}
	#endif

		if(!AudioCoreSinkInit(&AudioIOSet, AUDIO_DAC_SINK_NUM))
		{
			DBG("Dac init error\n");
			return FALSE;
		}
		AudioCoreSinkAudioPreset(AUDIO_DAC_SINK_NUM);
#if DAC_OUTPUT_CHANNEL == MONITOR_DAC0
		gCtrlVars.HwCt.DAC0Ct.Enable = 1;
#elif DAC_OUTPUT_CHANNEL == MONITOR_DAC1
		gCtrlVars.HwCt.DAC1Ct.Enable = 1;
#else
		gCtrlVars.HwCt.DAC2Ct.Enable = 1;
#endif
		DacParamCtCommon.SampleRate     = AudioCoreMixSampleRateGet(AudioCoreGetNetBySinkChannelNUM(AUDIO_DAC_SINK_NUM));
		DacParamCtCommon.DACMode        = DAC_MODE_SEL;
		DacParamCtCommon.DACLoadStatus  = DAC_NOLoad;
		DacParamCtCommon.DACEnergyModel = DAC_ENERGY_SEL;
		DacParamCtCommon.DACBitWidth    = DAC_WIDTH_SEL;
		DacParamCtCommon.DoutMode       = DAC_DOUT_MODE_SEL;
		DacParamCtCommon.DacFifo        = mainAppCt.DACFIFO;
		DacParamCtCommon.DacFifoLen     = mainAppCt.DACFIFO_LEN;
		DacParamCtCommon.OCPEnable      = FALSE;
#ifdef DELAY_SINK_DAC_MONITOR_EN
		DacParamCtCommon.FIRMode		= TRUE;
#endif
		if(DAC_SEL == DAC0_MODULE)
		{
			AudioDAC_Init(DAC0_MODULE, &DacParamCtCommon, NULL, NULL);
		}
		else if(DAC_SEL == DAC1_MODULE)
		{
			AudioDAC_Init(DAC1_MODULE, NULL, &DacParamCtCommon, NULL);
		}
		else
		{
			AudioDAC_Init(DAC2_MODULE, NULL, NULL, &DacParamCtCommon);
		}

	#ifdef CFG_FUNC_MCLK_USE_CUSTOMIZED_EN
        Clock_AudioMclkSel(AUDIO_DAC0, gCtrlVars.HwCt.DAC0Ct.dac_mclk_source > 2 ? (gCtrlVars.HwCt.DAC0Ct.dac_mclk_source - 1):gCtrlVars.HwCt.DAC0Ct.dac_mclk_source);
	#else
        DAC_CTRLVAR_SEL.CLK_Source = Clock_AudioMclkGet(AUDIO_DAC0);
	#endif
	}
	else//sam add,20230221
	{
		AudioDAC_SampleRateChange(DAC_SEL, AudioCoreMixSampleRateGet(AudioCoreGetNetBySinkChannelNUM(AUDIO_DAC_SINK_NUM)));
		DAC_CTRLVAR_SEL.CLK_Source = Clock_AudioMclkGet(DAC_AUDIO_MODULE);
		AudioCore.AudioSink[AUDIO_DAC_SINK_NUM].PcmWidth = CFG_DEFAULT_WIDTH;
	}
	DAC_CTRLVAR_SEL.Sample_Rate = SampleRateIndexGet(AudioDAC_SampleRateGet(DAC_SEL));
	AudioCoreSinkEnable(AUDIO_DAC_SINK_NUM);
#endif

#ifdef CFG_RES_AUDIO_DAC1_MIX_OUT_EN
	if(!DAC1_MixInit())
	{
		return FALSE;
	}
#endif

#ifdef CFG_RES_AUDIO_DAC2_MIX_OUT_EN
	if(!DAC2_MixInit())
	{
		return FALSE;
	}
#endif

#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
	//双图衔接通路
	if(!BridgeSinkSourceInit())
	{
		return FALSE;
	}
#endif

	//Mic1 analog  = Soure0.
	//AudioADC_AnaInit();
	//AudioADC_VcomConfig(1);//MicBias en
	// AudioADC_MicBias1Enable(1);
#if (CFG_RES_MIC_SELECT) && defined(CFG_FUNC_MIC_EN)
	uint8_t Mic_Net = AudioCoreGetNetBySourceChannelNUM(MIC_SOURCE_NUM);
	uint16_t Mic_FrameSample = AudioCoreFrameSizeGet(Mic_Net);
	#if defined(MIC_THRESHOLD_TRIGER_EN)
	uint16_t Mic_FrameByte = Mic_FrameSample * sizeof(PCM_DATA_TYPE) * 2;
	#endif
	uint16_t Mic_FifoSample = Mic_FrameSample * 3;
	uint32_t Mic_FifoByte = Mic_FifoSample * sizeof(PCM_DATA_TYPE) * 2;
	#if(MIC_INPUT_CHANNEL == ANA_INPUT_CH_MIC0)
		AUDIOADC_ParamCt MicHwCt =
		{
			.AUDIOADC_AnaLeft.INPUT 	= ADC0_MICL,
			#ifdef CHIP_MIC0_USE_DIFF
				.AUDIOADC_AnaLeft.Mode 		= ADC_MIC_Diff,
			#else
				.AUDIOADC_AnaLeft.Mode 		= ADC_MIC_Single,
			#endif
			.AUDIOADC_AnaLeft.PGAGain 	= 31 - MIC_CTRLPGA_SEL.MICL_Single_Gain,
			.AUDIOADC_AnaRight.INPUT 	= ADC0_MICR,
			#ifdef CHIP_MIC0_USE_DIFF
				.AUDIOADC_AnaRight.Mode 		= ADC_MIC_Diff,
			#else
				.AUDIOADC_AnaRight.Mode 		= ADC_MIC_Single,
			#endif
			.AUDIOADC_AnaRight.PGAGain 	= 31 - MIC_CTRLPGA_SEL.MICR_Single_Gain,
			.MICBias_EN					= CFG_MICBIAS_EN_VAL,
			.SampleRate					= AudioCoreMixSampleRateGet(Mic_Net),
			.ADCEnergyModel				= ADC_ENERGY_SEL,
			.ADCBitWidth				= AUDIOADC_WIDTH_SEL,
			.ADCFifo					= mainAppCt.ADCFIFO,
		};

		//for communication
		gCtrlVars.HwCt.ADC0DigitalCt.Enable = 3;
		gCtrlVars.HwCt.ADC0PGACt.MICL_EN = 1;
		gCtrlVars.HwCt.ADC0PGACt.MICR_EN = 1;
		gCtrlVars.HwCt.ADC0PGACt.LineinL_EN = 0;
		gCtrlVars.HwCt.ADC0PGACt.LineinR_EN = 0;

	#else
			AUDIOADC_ParamCt MicHwCt =
			{
				.AUDIOADC_AnaLeft.INPUT 	= ADC1_MICL,
				#ifdef CHIP_MIC1_USE_DIFF
					.AUDIOADC_AnaLeft.Mode 		= ADC_MIC_Diff,
				#else
					.AUDIOADC_AnaLeft.Mode 		= ADC_MIC_Single,
				#endif
				.AUDIOADC_AnaLeft.PGAGain 	= 31 - MIC_CTRLPGA_SEL.MICL_Single_Gain,
				.AUDIOADC_AnaRight.INPUT 	= ADC1_MICR,
				#ifdef CHIP_MIC1_USE_DIFF
					.AUDIOADC_AnaRight.Mode 		= ADC_MIC_Diff,
				#else
					.AUDIOADC_AnaRight.Mode 		= ADC_MIC_Single,
				#endif
				.AUDIOADC_AnaRight.PGAGain 	= 31 - MIC_CTRLPGA_SEL.MICR_Single_Gain,
				.MICBias_EN					= CFG_MICBIAS_EN_VAL,
				.SampleRate					= AudioCoreMixSampleRateGet(Mic_Net),
				.ADCEnergyModel				= ADC_ENERGY_SEL,
				.ADCBitWidth				= AUDIOADC_WIDTH_SEL,
				.ADCFifo					= mainAppCt.ADCFIFO,
			};

			gCtrlVars.HwCt.ADC1DigitalCt.Enable = 3;
			gCtrlVars.HwCt.ADC1PGACt.MICL_EN = 1;
			gCtrlVars.HwCt.ADC1PGACt.MICR_EN = 1;
			gCtrlVars.HwCt.ADC1PGACt.AUX0L_B20_EN = 0;
			gCtrlVars.HwCt.ADC1PGACt.AUX0R_B21_EN = 0;
			gCtrlVars.HwCt.ADC1PGACt.AUX1L_B18_EN = 0;
			gCtrlVars.HwCt.ADC1PGACt.AUX1R_B19_EN = 0;
	#endif

	if(!AudioCoreSourceIsInit(MIC_SOURCE_NUM))
	{
		MicHwCt.ADCFifoLen = Mic_FifoByte;//立体声，单位byte
		mainAppCt.ADCFIFO = (uint32_t*)osPortMalloc(MicHwCt.ADCFifoLen);//ADC fifo
		if(mainAppCt.ADCFIFO != NULL)
		{
			memset(mainAppCt.ADCFIFO, 0, MicHwCt.ADCFifoLen);
		}
		else
		{
			APP_DBG("malloc ADCFIFO error\n");
			return FALSE;
		}
		MicHwCt.ADCFifo = mainAppCt.ADCFIFO;
		AudioADC_DynamicElementMatch(MIC_ADC_SEL, TRUE, TRUE);

		//Mic1
		AudioADC_Init(MIC_ADC_SEL, &MicHwCt); //
	#if defined(MIC_THRESHOLD_TRIGER_EN)
		void MicThresholdInterrupt(void);
	    DMA_CircularThresholdLenSet(MIC_DMA_PERI_ID_SEL, Mic_FrameByte);
		DMA_InterruptFunSet(MIC_DMA_PERI_ID_SEL, DMA_THRESHOLD_INT, MicThresholdInterrupt);
		#if defined(CFG_FUNC_EFFECT_CORE_DUAL_EN)
		if(Mic_Net == CFG_PARA_CORE2_NET)
		{
			AudioCore.NetCt[Mic_Net].TrigerMsgFunc = SendEffectServiceMsg;
		}
		else
		#endif
		{
			AudioCore.NetCt[Mic_Net].TrigerMsgFunc = SendAudioCoreServiceMsg;
		}
		CoreInterruptEnable(INT_DMA1_T);
		DMA_InterruptEnable(MIC_DMA_PERI_ID_SEL, DMA_THRESHOLD_INT, 1);
		NVIC_EnableIRQ(TMR4_DMAC1TINT_IRQn);
	#endif
	#ifdef CFG_FUNC_MCLK_USE_CUSTOMIZED_EN
        Clock_AudioMclkSel(MIC_MODULE, MIC_CTRLDIGITAL_SEL.adc_mclk_source > 2 ? (MIC_CTRLDIGITAL_SEL.adc_mclk_source - 1) : MIC_CTRLDIGITAL_SEL.adc_mclk_source);
	#else
        MIC_CTRLDIGITAL_SEL.CLK_Source = Clock_AudioMclkGet(MIC_ADC_SEL);
	#endif
		//Soure0.
		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
		AudioIOSet.Adapt = STD;
		AudioIOSet.Sync = TRUE;
		AudioIOSet.Channels = 2;
		AudioIOSet.Net = Mic_Net;
#ifdef	FUNC_WIRELESS2_SUPPORT
		AudioIOSet.DataIOFunc = wireless_AudioADC0Get;
#else
		AudioIOSet.DataIOFunc = MIC_DATAGET_SEL;
#endif
		AudioIOSet.LenGetFunc = MIC_LENGET_SEL;
		AudioIOSet.PcmWidth	= CFG_DEFAULT_WIDTH;
		AudioIOSet.SampleRate = AudioCoreMixSampleRateGet(Mic_Net);
		if(!AudioCoreSourceInit(&AudioIOSet, MIC_SOURCE_NUM))
		{
			DBG("mic Source error");
			return FALSE;
		}
	}

//	AudioADC_AnaInit(MIC_ADC_SEL, &MicHwCt);

	//MADC_MIC_PowerUP(SingleEnded);
	MIC_CTRLDIGITAL_SEL.Sample_Rate = SampleRateIndexGet(AudioADC_SampleRateGet(MIC_ADC_SEL));

	#ifdef CFG_FUNC_AUDIO_SILENCE_AVOIDANCE_EN
	while(DMA_CircularDataLenGet(MIC_DMA_PERI_ID_SEL) < MicHwCt.ADCFifoLen/2)
	{
//	    printf("ADC %d \n",DMA_CircularDataLenGet(MIC_DMA_PERI_ID_SEL));
	}
	#endif
	AudioCoreSourceEnable(MIC_SOURCE_NUM);
#endif

#ifdef CFG_FUNC_REMIND_SOUND_EN
	if(!AudioCoreSourceIsInit(REMIND_SOURCE_NUM))
	{
		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
		AudioIOSet.Adapt = SRC_ONLY;
		AudioIOSet.SrcQuality = 0;//最低音质,节约资源
		AudioIOSet.SampleRate = AudioCoreMixSampleRateGet(AudioCoreGetNetBySourceChannelNUM(REMIND_SOURCE_NUM));//初始值
		AudioIOSet.Sync = FALSE;
		AudioIOSet.Channels = get_remind_channels();
		AudioIOSet.Net = AudioCoreGetNetBySourceChannelNUM(REMIND_SOURCE_NUM);
		AudioIOSet.DataIOFunc = RemindDataGet;
		AudioIOSet.LenGetFunc = RemindDataLenGet;
		AudioIOSet.PcmWidth	= WIDTH_16BIT;
		if(!AudioCoreSourceInit(&AudioIOSet, REMIND_SOURCE_NUM))
		{
			DBG("remind source error!\n");
			SoftFlagRegister(SoftFlagNoRemind);
			return FALSE;
		}
	#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
		RemindMp3DecoderInit();
	#endif
	}
#endif

#if defined(CFG_RES_AUDIO_SPDIF0OUT_EN) || defined(CFG_RES_AUDIO_SPDIF1OUT_EN)
	if(!SpdifOutInit())
	{
		return FALSE;
	}
#endif


#if (BT_TWS_SUPPORT == ENABLE)
 	if(BtTwsStatusGet())
 	{
 		if(!AudioCoreSourceIsInit(TWS_SOURCE_NUM))
 		{
 			memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
 			AudioIOSet.Adapt = STD;
 			AudioIOSet.Sync = FALSE;//TRUE
 			AudioIOSet.Channels = 2;
 			AudioIOSet.Net = AudioCoreGetNetBySourceChannelNUM(TWS_SOURCE_NUM);
 			AudioIOSet.DataIOFunc = TWSDataGet;
 			AudioIOSet.LenGetFunc = TWSDataLenGet;
			AudioIOSet.SampleRate = AudioCoreMixSampleRateGet(AudioCoreGetNetBySourceChannelNUM(TWS_SOURCE_NUM));
			AudioIOSet.PcmWidth = WIDTH_16BIT;
 		// #ifdef	CFG_AUDIO_WIDTH_24BIT
 		// 	AudioIOSet.IOBitWidth = PCM_DATA_16BIT_WIDTH;//0,16bit,1:24bit
 		// 	AudioIOSet.IOBitWidthConvFlag = 1;//需要数据进行位宽扩展
 		// #endif
 			if(!AudioCoreSourceInit(&AudioIOSet, TWS_SOURCE_NUM))
 			{
 				DBG("Tws Source error \n");
 				return FALSE;
 			}
 		}
 		AudioCoreSourceEnable(TWS_SOURCE_NUM);
 		//AudioCoreSourceDisable(TWS_SOURCE_NUM);

 		//if(GetSystemMode() != ModeTwsSlavePlay && !AudioCoreSinkIsInit(TWS_SINK_NUM))
 		if(!AudioCoreSinkIsInit(TWS_SINK_NUM))
 		{
 			memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
 			AudioIOSet.Adapt = STD;
 			AudioIOSet.Sync = FALSE;//TRUE
 			AudioIOSet.Channels = 2;
 			AudioIOSet.Net = AudioCoreGetNetBySinkChannelNUM(TWS_SINK_NUM);;
 			AudioIOSet.Depth = AudioCore.NetCt[AudioCoreGetNetBySinkChannelNUM(TWS_SINK_NUM)].FrameSize * 3;
 			AudioIOSet.DataIOFunc = TwsSinkDataSet;
 			AudioIOSet.LenGetFunc = TwsSinkSpaceLenGet;
			AudioIOSet.SampleRate = AudioCoreMixSampleRateGet(AudioCoreGetNetBySinkChannelNUM(TWS_SINK_NUM));
 			AudioIOSet.Resident = TRUE;
			AudioIOSet.PcmWidth = WIDTH_16BIT;
 		// #ifdef	CFG_AUDIO_WIDTH_24BIT
 		// 	AudioIOSet.IOBitWidth = PCM_DATA_16BIT_WIDTH;//0,16bit,1:24bit
 		// 	AudioIOSet.IOBitWidthConvFlag = 0;//不需要做位宽转换处理
 		// #endif
 			if(!AudioCoreSinkInit(&AudioIOSet, TWS_SINK_NUM))
 			{
 				DBG("Tws Sink error");
 				return FALSE;
 			}
 			else
 				DBG("TWS_SINK_Init!\n");
 			AudioCoreSinkEnable(TWS_SINK_NUM);
 			//AudioCoreSinkDisable(TWS_SINK_NUM);
 		}
 	}
 #endif

#ifdef CFG_FUNC_I2S0_MIX
	if(!I2S0_MixInit(FALSE))
	{
		return FALSE;
	}
#endif
#ifdef CFG_FUNC_I2S1_MIX
	if(!I2S1_MixInit(FALSE))
	{
		return FALSE;
	}
#endif
#ifdef CFG_FUNC_I2S2_MIX
	if(!I2S2_MixInit(FALSE))
	{
		return FALSE;
	}
#endif
#ifdef CFG_FUNC_LINEIN_MIX_MODE
	if(!LineInMixPlayInit())
	{
		return FALSE;
	}
#endif
//#ifdef CFG_FUNC_USB_AUDIO_MIX_MODE
//	if(!UsbDevicePlayMixInit())
//	{
//		return FALSE;
//	}
//#endif
#ifdef CFG_RES_USB_FS_MIX_AUDIO_EN
	if(!UsbFSDevicePlayMixInit())
	{
		return FALSE;
	}
#endif

#ifdef CFG_RES_USB_HS_MIX_AUDIO_EN
	if(!UsbHSDevicePlayMixInit())
	{
		return FALSE;
	}
#endif
#if BT_SOURCE_SUPPORT
	if(AudioCoreFrameSizeGet(DefaultNet) != 0 &&
	   (AudioCoreFrameSizeGet(DefaultNet) % 128) != 0)
	{
		//帧长不是128的整数倍，SBC编码一次是128，需要加fifo
		if(AudioCoreFrameSizeGet(DefaultNet) > 128)
			mainAppCt.sbc_encode_pcm_buf_len = AudioCoreFrameSizeGet(DefaultNet) * 2 * 2 * 2 + 2;
		else
			mainAppCt.sbc_encode_pcm_buf_len = 128 * 2 * 2 * 2 + 2;
		mainAppCt.sbc_encode_pcm_buf = (uint32_t*)osPortMalloc(mainAppCt.sbc_encode_pcm_buf_len);
		if(mainAppCt.sbc_encode_pcm_buf)
			MCUCircular_Config(&mainAppCt.sbc_encode_pcm_context, mainAppCt.sbc_encode_pcm_buf, mainAppCt.sbc_encode_pcm_buf_len);
		DBG("BT_SOURCE: Sbc encode buf! %d %d\n",mainAppCt.sbc_encode_pcm_buf_len,AudioCoreFrameSizeGet(DefaultNet));
	}

	{
		AudioCoreSink *Sink = &AudioCore.AudioSink[AUDIO_BT_SOURCE_SINK_NUM];

		//BT SOURCE复用DAC的PcmOutBuf，并且AUDIO_BT_SOURCE_SINK_NUM为16BIT数据
		//先保证DAC 24bit数据使用完以后，BT SOURCE才可以转16bit数据
	#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
		Sink->BitWidth = PCM_DATA_24BIT_WIDTH;
		Sink->BitWidthConvFlag = 1;	//BT SOURCE转16bit数据,时序上保证DAC先使用完才可以轮到BT SOURCE使用
	#endif
		Sink->PcmWidth	= CFG_DEFAULT_WIDTH;
		Sink->Adapt	= STD;
		Sink->Sync = FALSE;
		Sink->Channels = 2;
		Sink->Net = DefaultNet;
		Sink->PcmOutBuf =  AudioCore.AudioSink[AUDIO_DAC_SINK_NUM].PcmOutBuf;//复用DAC的PcmOutBuf
		Sink->SpaceLenFunc = GetBtSourceFifoSpaceLength;
		Sink->DataSetFunc = SbcSourceEncode;
		AudioCoreSinkEnable(AUDIO_BT_SOURCE_SINK_NUM);
	}
#elif defined(CFG_WIRELESS_OUT_EN)
	{
		AudioCoreSink *Sink = &AudioCore.AudioSink[WIRELESS_AUDIO_SINK_NUM];

		//WIRELESS SINK复用DAC的PcmOutBuf，并且WIRELESS_AUDIO_SINK_NUM为16BIT数据
		//先保证DAC 24bit数据使用完以后，WIRELESS SINK才可以转16bit数据
	#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
		Sink->BitWidth = PCM_DATA_24BIT_WIDTH;
		Sink->BitWidthConvFlag = 1;	//WIRELESS SINK转16bit数据,时序上保证DAC先使用完才可以轮到WIRELESS SINK使用
	#endif
		Sink->PcmWidth	= CFG_DEFAULT_WIDTH;
		Sink->Adapt	= STD;
		Sink->Sync = TRUE;
		Sink->Channels = 2;
		Sink->Net = DefaultNet;
		Sink->PcmOutBuf =  AudioCore.AudioSink[AUDIO_DAC_SINK_NUM].PcmOutBuf;//复用DAC的PcmOutBuf
		Sink->SpaceLenFunc = WirelessOutSpaceLenGet;
		Sink->DataSetFunc = WirelessOutDataSet;
		AudioCoreSinkEnable(WIRELESS_AUDIO_SINK_NUM);
	}
#endif


#if (BT_LE_AUDIO_SUPPORT)
	// if (AudioCoreSinkToRoboeffect(LE_AUDIO_SINK_NUM) == AUDIOCORE_SOURCE_SINK_ERROR)
	if(!AudioCoreSinkIsInit(LE_AUDIO_SINK_NUM))
	{
		extern uint16_t GetLeAudioEncoderPcmSpaceLength(void);
		extern uint16_t LeAudioEncoderPcmDataSet(void* Buf, uint16_t Len);
		extern uint16_t LeAudioGetSrcSampleRate(void);

		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
		AudioIOSet.Depth = AudioCore.NetCt[0].FrameSize * 2 ;

		//复用DAC的PcmOutBuf，并且LE_AUDIO为16BIT数据
		//先保证DAC 24bit数据使用完以后，LE_AUDIO才可以转16bit数据
		//框图里面有定义AUDIO_LE_AUDIO_SINK_NUM，使用单独的sink资源
	// #if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
	// 	AudioIOSet.BitWidth = PCM_DATA_24BIT_WIDTH;
	// 	AudioIOSet.BitWidthConvFlag = 1;	//转16bit数据,时序上保证DAC先使用完才可以轮到LE_AUDIO使用
	// #endif
//		AudioIOSet.IOBitWidth = PCM_DATA_24BIT_WIDTH;
//		AudioIOSet.IOBitWidthConvFlag = 0;	//转16bit数据,时序上保证DAC先使用完才可以轮到LE_AUDIO使用
		AudioIOSet.Adapt	= SRC_ONLY;
		AudioIOSet.Sync = FALSE;
		AudioIOSet.Channels = 2;
		AudioIOSet.Net = AudioCoreGetNetBySinkChannelNUM(LE_AUDIO_SINK_NUM);
		AudioIOSet.LenGetFunc = GetLeAudioEncoderPcmSpaceLength;
		AudioIOSet.DataIOFunc = LeAudioEncoderPcmDataSet;
		AudioIOSet.PcmWidth = WIDTH_16BIT;
		AudioIOSet.SampleRate = LeAudioGetSrcSampleRate();

		if(!AudioCoreSinkInit(&AudioIOSet, LE_AUDIO_SINK_NUM))
		{
			DBG("LE_AUDIO_SINK init error");
			return FALSE;
		}
#ifndef	ROBOEFFECT_DOUBLE_ENGINE_EN			
		AudioCoreSinkAudioPreset(LE_AUDIO_SINK_NUM);
#endif
		AudioCoreSinkEnable(LE_AUDIO_SINK_NUM);
	}
#endif//BT_LE_AUDIO_SUPPORT && (LE_AUDIO_ROLE == CIS_SRC)

#ifdef CFG_RES_DAC_MONITOR_EN
#ifndef CFG_HP_DAC0_BOOT_MUTE_TEST
	AudioDAC_SoftMute(DAC_SEL, 0, 0);
#endif
	AudioDAC_Enable(DAC_SEL);
#endif
#ifdef CFG_COMMUNICATION_BY_UART
	uart_data_init();
#endif
#ifdef CFG_UPGRADE_BY_UART
	uartupgrade_data_init();
#endif	
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
	AudioEffect_CommunicationQueue_Init();
#endif
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
	gCtrlVars.AutoRefresh = AutoRefresh_ALL_PARA;
	AudioEffect_CommunicationQueue_Send(gCtrlVars.AutoRefresh);   //防止出现上位机焦点和SDK不一致
#endif

#if (BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT)
	LeAudioSrcResume();
#endif

#if (BT_TWS_SUPPORT == ENABLE)
	if(mainAppCt.SysPrevMode == ModeBtHfPlay)
		BtStackServiceMsgSend(MSG_BT_STACK_TWS_SYNC_REINIT);

#ifdef CFG_FUNC_REMIND_SOUND_EN
	extern void RemindSoundSetTwsReSyncFlag(uint8_t flag);
	if(BtTwsStatusGet() && (mainAppCt.SysPrevMode == ModeBtHfPlay) )
		RemindSoundSetTwsReSyncFlag(1);
#endif

#endif
#if defined(CFG_LOW_DELAY_CTRL)
	#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
		if(!AudioEffect_Parambin_GetSourceBuffer(&EffectParambin2, FRAME_SYNC_SOURCE_NUM)
			|| !AudioEffect_Parambin_GetSinkBuffer(&EffectParambin2, DELAY_SINK_NUM))
	#endif
		{
			if(!AudioEffect_Parambin_GetSourceBuffer(&EffectParambin, FRAME_SYNC_SOURCE_NUM)
				|| !AudioEffect_Parambin_GetSinkBuffer(&EffectParambin, DELAY_SINK_NUM))
			{
				DBG("Low Delay Sync SourceSink flow error!!!\n");
			}
			else
			{
				DBG("Warning:Low Delay Sync SourceSink flow1!!!\n");
			}
		}
#endif

#ifdef CFG_FUNC_BREAKPOINT_EN
	//注意 是否需要模式过滤，部分模式无需记忆和恢复
	if(GetSystemMode() != ModeIdle)
	{
		BackupInfoUpdata(BACKUP_SYS_INFO);
	}
#endif

	return TRUE;
}

//释放公共通路，
void ModeCommonDeinit(void)
{
	SoftFlagRegister(SoftFlagAudioCoreSourceIsDeInit);
#ifdef CFG_RES_DAC_MONITOR_EN
//	AudioCoreSinkDisable(AUDIO_DAC_SINK_NUM);
	AudioDAC_SoftMute(DAC_SEL, 1, 1);

//	AudioDAC_Disable(DAC_SEL);
//	AudioDAC_FuncReset(DAC_SEL);
//	DMA_InterruptFlagClear(DAC_DMA_PERI_ID_SEL, DMA_DONE_INT);
//	DMA_InterruptFlagClear(DAC_DMA_PERI_ID_SEL, DMA_THRESHOLD_INT);
//	DMA_InterruptFlagClear(DAC_DMA_PERI_ID_SEL, DMA_ERROR_INT);
//	DMA_ChannelDisable(DAC_DMA_PERI_ID_SEL);
//	AudioDAC_Reset(DAC0);
	AudioDAC_DeInit(DAC_SEL);

	if(mainAppCt.DACFIFO != NULL)
	{
		osPortFree(mainAppCt.DACFIFO);
		mainAppCt.DACFIFO = NULL;
	}
	AudioCoreSinkDeinit(AUDIO_DAC_SINK_NUM);
#endif
#ifdef CFG_RES_AUDIO_DAC1_MIX_OUT_EN
	DAC1_MixDeinit();
#endif
#ifdef CFG_RES_AUDIO_DAC2_MIX_OUT_EN
	DAC2_MixDeinit();
#endif

#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
	//双图衔接通路
	BridgeSinkSourceDeinit();
#endif

#if (BT_TWS_SUPPORT == ENABLE)
	AudioCoreSinkDisable(TWS_SINK_NUM);
	AudioCoreSinkDeinit(TWS_SINK_NUM);
	AudioCoreSourceDisable(TWS_SOURCE_NUM);
	AudioCoreSourceDeinit(TWS_SOURCE_NUM);
#endif

#if (CFG_RES_MIC_SELECT) && defined(CFG_FUNC_MIC_EN)
	AudioCoreSourceDisable(MIC_SOURCE_NUM);
	osTaskDelay(5);
//	AudioADC_FuncReset(MIC_ADC_SEL);
//	AudioADC_Disable(MIC_ADC_SEL);
//	DMA_InterruptFlagClear(MIC_DMA_PERI_ID_SEL, DMA_DONE_INT);
//	DMA_InterruptFlagClear(MIC_DMA_PERI_ID_SEL, DMA_THRESHOLD_INT);
//	DMA_InterruptFlagClear(MIC_DMA_PERI_ID_SEL, DMA_ERROR_INT);
//	DMA_ChannelDisable(MIC_DMA_PERI_ID_SEL);
	AudioADC_DeInit(MIC_ADC_SEL);

	if(mainAppCt.ADCFIFO != NULL)
	{
		APP_DBG("ADC1FIFO\n");
		osPortFree(mainAppCt.ADCFIFO);
		mainAppCt.ADCFIFO = NULL;
	}
	AudioCoreSourceDeinit(MIC_SOURCE_NUM);
#endif

#if defined(CFG_RES_AUDIO_SPDIF0OUT_EN) || defined(CFG_RES_AUDIO_SPDIF1OUT_EN)
	SpdifOutDeinit();
#endif

//#endif

#ifdef CFG_FUNC_REMIND_SOUND_EN
	AudioCoreSourceDisable(REMIND_SOURCE_NUM);
	AudioCoreSourceDeinit(REMIND_SOURCE_NUM);
	RemindSoundAudioPlayEnd();
	RemindSoundBufferFree();
#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
	RemindMp3DecoderDeinit();
#endif
#endif
#ifdef CFG_FUNC_I2S0_MIX
	I2S0_MixDeinit();
#endif
#ifdef CFG_FUNC_I2S1_MIX
	I2S1_MixDeinit();
#endif
#ifdef CFG_FUNC_I2S2_MIX
	I2S2_MixDeinit();
#endif
#ifdef CFG_FUNC_LINEIN_MIX_MODE
	LineInMixPlayDeinit();
#endif
#ifdef CFG_FUNC_USB_AUDIO_MIX_MODE
	UsbDevicePlayMixDeinit();
#endif
#ifdef CFG_RES_USB_FS_MIX_AUDIO_EN
	UsbFSDevicePlayMixDeinit();
#endif
#ifdef CFG_RES_USB_HS_MIX_AUDIO_EN
	UsbHSDevicePlayMixDeinit();
#endif
#ifdef CFG_FUNC_ROBOEFFECT_EN
	AudioEffect_Parambin_Deinit(&EffectParambin);
#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
	AudioEffect_Parambin_Deinit(&EffectParambin2);
#endif
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
	AudioEffect_CommunicationQueue_Deinit();
#endif
#endif
#if BT_SOURCE_SUPPORT
	if(mainAppCt.sbc_encode_pcm_buf)
		osPortFree(mainAppCt.sbc_encode_pcm_buf);
	mainAppCt.sbc_encode_pcm_buf = NULL;
	AudioCoreSinkDisable(AUDIO_BT_SOURCE_SINK_NUM);
#elif defined(CFG_WIRELESS_OUT_EN)
	AudioCoreSinkDisable(WIRELESS_AUDIO_SINK_NUM);
#endif

#if (BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT)
	LeAudioSrcPause();
	if(AudioCoreSinkIsInit(LE_AUDIO_SINK_NUM))
	{
		AudioCoreSinkDisable(LE_AUDIO_SINK_NUM);
		AudioCoreSinkDeinit(LE_AUDIO_SINK_NUM);
	}
#endif
//	Reset_Function2Reset(DMAC1_FUNC_SEPA);
//	Reset_Function2Reset(DMAC2_FUNC_SEPA);
}

bool AudioIoCommonForHfp(uint16_t gain)
{
	AudioCoreIO AudioIOSet;
	uint16_t FifoLenStereo;
//	uint16_t FifoLenMono = SampleLen * 2 * 2;//单声到4倍大小于帧长，单位byte
	uint32_t sampleRate;
#ifdef CFG_FUNC_ROBOEFFECT_EN
#ifndef ROBOEFFECT_DOUBLE_ENGINE_EN
	if(!AudioEffect_Parambin_Init(&EffectParambin, effect_flow_name[mainAppCt.effect_flow_index], mainAppCt.effect_param_mode_index, 1))
#else
	if(!AudioEffect_Parambin_Init(&EffectParambin, effect_flow_name[mainAppCt.effect_flow_index][0], mainAppCt.effect_param_mode_groupindex[0], 1))
#endif
	{
		if(GetSysModeState(ModeSafe) == ModeStateSusend)
		{
			SetSysModeState(ModeSafe,ModeStateReady);
		}
		MessageContext		msgSend;
		msgSend.msgId = MSG_DEVICE_SERVICE_MODE_ERROR;
		MessageSend(GetMainMessageHandle(), &msgSend);
		return FALSE;
	}
	//After effect init done, AudioCore know what frame size should be set.
	AudioCoreFrameSampleSync(&EffectParambin, DefaultNet);

	init_effect_flows_Status();
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
	CommunicationChart = &EffectParambin;
#endif
#endif

#ifdef CFG_FUNC_BREAKPOINT_EN
//	BackupInfoUpdata(BACKUP_SYS_INFO);
#endif

	sampleRate = AudioCoreMixSampleRateGet(DefaultNet);
	APP_DBG("Systerm HFP SampleRate: %ld\n", sampleRate);

	FifoLenStereo = AudioCoreFrameSizeGet(DefaultNet) * sizeof(PCM_DATA_TYPE) * 2 * 3;//立体声8倍大小于帧长，单位byte
	//FifoLenStereo = AudioCoreFrameSizeGet(DefaultNet) * 2 * 2 * 2;//立体声8倍大小于帧长，单位byte

	DefaultParamgsInit();	//refresh local hardware config params(just storage not set)


#ifdef CFG_RES_DAC_MONITOR_EN
	// AudioCoreSinkDisable(AUDIO_DAC_SINK_NUM);

	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
	AudioIOSet.Depth = AudioCore.NetCt[DefaultNet].FrameSize * 3 ;
	AudioIOSet.PcmWidth	= CFG_DEFAULT_WIDTH;

	if(!AudioCoreSinkIsInit(AUDIO_DAC_SINK_NUM))
	{
		mainAppCt.DACFIFO_LEN = AudioIOSet.Depth * sizeof(PCM_DATA_TYPE) * 2;

		mainAppCt.DACFIFO = (uint32_t*)osPortMalloc(mainAppCt.DACFIFO_LEN);//DAC fifo
		if(mainAppCt.DACFIFO != NULL)
		{
			memset(mainAppCt.DACFIFO, 0, mainAppCt.DACFIFO_LEN);
		}
		else
		{
			APP_DBG("malloc DACFIFO error\n");
			return FALSE;
		}

		//sink0
		AudioIOSet.Adapt = STD;
		AudioIOSet.Sync = TRUE;
		AudioIOSet.Channels = 2;
		AudioIOSet.Net = AudioCoreGetNetBySinkChannelNUM(AUDIO_DAC_SINK_NUM);
		AudioIOSet.DataIOFunc = DAC_DATASET_SEL;
		AudioIOSet.LenGetFunc = DAC_LENGET_SEL;

		if(!AudioCoreSinkInit(&AudioIOSet, AUDIO_DAC_SINK_NUM))
		{
			DBG("Dac init error");
			return FALSE;
		}
		AudioCoreSinkAudioPreset(AUDIO_DAC_SINK_NUM);

#if DAC_OUTPUT_CHANNEL == MONITOR_DAC0
		gCtrlVars.HwCt.DAC0Ct.Enable = 1;
#elif DAC_OUTPUT_CHANNEL == MONITOR_DAC1
		gCtrlVars.HwCt.DAC1Ct.Enable = 1;
#else
		gCtrlVars.HwCt.DAC2Ct.Enable = 1;
#endif
		AUDIODAC_ParamCt DacParamCt =
		{
			.SampleRate 	= sampleRate,
			.DACMode 		= DAC_MODE_SEL,
			.DACLoadStatus 	= DAC_NOLoad,
			.DACEnergyModel = DAC_ENERGY_SEL,
			.DACBitWidth	= DAC_WIDTH_SEL,
			.DoutMode		= DAC_DOUT_MODE_SEL,
			.DacFifo		= mainAppCt.DACFIFO,
			.DacFifoLen		= mainAppCt.DACFIFO_LEN,
			.OCPEnable		= FALSE,
		};

		if(DAC_SEL == DAC0_MODULE)
		{
			AudioDAC_Init(DAC0_MODULE, &DacParamCt, NULL, NULL);
		}
		else if(DAC_SEL == DAC1_MODULE)
		{
			AudioDAC_Init(DAC1_MODULE, NULL, &DacParamCt, NULL);
		}
		else
		{
			AudioDAC_Init(DAC2_MODULE, NULL, NULL, &DacParamCt);
		}

	#ifdef CFG_FUNC_MCLK_USE_CUSTOMIZED_EN
        Clock_AudioMclkSel(DAC_AUDIO_MODULE, DAC_CTRLVAR_SEL.dac_mclk_source > 2 ? (DAC_CTRLVAR_SEL.dac_mclk_source - 1) : DAC_CTRLVAR_SEL.dac_mclk_source);
	#else
        DAC_CTRLVAR_SEL.CLK_Source = Clock_AudioMclkGet(DAC_AUDIO_MODULE);
	#endif
	}
	else
	{
		AudioDAC_SampleRateChange(DAC_SEL, sampleRate);
		DAC_CTRLVAR_SEL.CLK_Source = Clock_AudioMclkGet(DAC_AUDIO_MODULE);
		APP_DBG("mode task io set\n");
		AudioIOSet.PcmWidth	= CFG_DEFAULT_WIDTH;
	}
	DAC_CTRLVAR_SEL.Sample_Rate = SampleRateIndexGet(AudioDAC_SampleRateGet(DAC_SEL));
	AudioCoreSinkEnable(AUDIO_DAC_SINK_NUM);
#endif

#ifdef CFG_RES_AUDIO_DAC1_MIX_OUT_EN
//	if(!DAC1_MixInit())
//	{
//		return FALSE;
//	}
#endif

#ifdef CFG_RES_AUDIO_DAC2_MIX_OUT_EN
//	if(!DAC2_MixInit())
//	{
//		return FALSE;
//	}
#endif

#if (CFG_RES_MIC_SELECT) && defined(CFG_FUNC_MIC_EN)
	#if(MIC_INPUT_CHANNEL == ANA_INPUT_CH_MIC0)
		AUDIOADC_ParamCt MicHwCt =
		{
			.AUDIOADC_AnaLeft.INPUT 	= ADC0_MICL,
			.AUDIOADC_AnaLeft.Mode 		= MIC_CTRLPGA_SEL.MICL_Mode,
			.AUDIOADC_AnaLeft.PGAGain 	= 31 - MIC_CTRLPGA_SEL.MICL_Single_Gain,
			.AUDIOADC_AnaRight.INPUT 	= ADC0_MICR,
			.AUDIOADC_AnaRight.Mode 	= MIC_CTRLPGA_SEL.MICR_Mode,
			.AUDIOADC_AnaRight.PGAGain 	= 31 - MIC_CTRLPGA_SEL.MICR_Single_Gain,
			.MICBias_EN					= CFG_MICBIAS_EN_VAL,
			.SampleRate					= sampleRate,
			.ADCEnergyModel				= ADC_ENERGY_SEL,
			.ADCBitWidth				= AUDIOADC_WIDTH_SEL,
			.ADCFifo					= mainAppCt.ADCFIFO,
		};

		//for communication
		gCtrlVars.HwCt.ADC0DigitalCt.Enable = 3;
		gCtrlVars.HwCt.ADC0PGACt.MICL_EN = 1;
		gCtrlVars.HwCt.ADC0PGACt.MICR_EN = 1;
		gCtrlVars.HwCt.ADC0PGACt.LineinL_EN = 0;
		gCtrlVars.HwCt.ADC0PGACt.LineinR_EN = 0;
	#else
		AUDIOADC_ParamCt MicHwCt =
		{
			.AUDIOADC_AnaLeft.INPUT 	= ADC1_MICL,
			.AUDIOADC_AnaLeft.Mode 		= MIC_CTRLPGA_SEL.MICL_Mode,
			.AUDIOADC_AnaLeft.PGAGain 	= 31 - MIC_CTRLPGA_SEL.MICL_Single_Gain,
			.AUDIOADC_AnaRight.INPUT 	= ADC1_MICR,
			.AUDIOADC_AnaRight.Mode 	= MIC_CTRLPGA_SEL.MICR_Mode,
			.AUDIOADC_AnaRight.PGAGain 	= 31 - MIC_CTRLPGA_SEL.MICR_Single_Gain,
			.MICBias_EN					= CFG_MICBIAS_EN_VAL,
			.SampleRate					= sampleRate,
			.ADCEnergyModel				= ADC_ENERGY_SEL,
			.ADCBitWidth				= ADC_24BIT,//AUDIOADC_WIDTH_SEL,
			.ADCFifo					= mainAppCt.ADCFIFO,
		};

		gCtrlVars.HwCt.ADC1DigitalCt.Enable = 3;
		gCtrlVars.HwCt.ADC1PGACt.MICL_EN = 1;
		gCtrlVars.HwCt.ADC1PGACt.MICR_EN = 1;
		gCtrlVars.HwCt.ADC1PGACt.AUX0L_B20_EN = 0;
		gCtrlVars.HwCt.ADC1PGACt.AUX0R_B21_EN = 0;
		gCtrlVars.HwCt.ADC1PGACt.AUX1L_B18_EN = 0;
		gCtrlVars.HwCt.ADC1PGACt.AUX1R_B19_EN = 0;
	#endif

	//AudioCoreSourceDisable(MIC_SOURCE_NUM);

	if(!AudioCoreSourceIsInit(MIC_SOURCE_NUM))
	{
		//Mic1 analog  = Soure0.
		MicHwCt.ADCFifoLen = FifoLenStereo;
		mainAppCt.ADCFIFO = (uint32_t*)osPortMalloc(MicHwCt.ADCFifoLen);//ADC fifo
		if(mainAppCt.ADCFIFO != NULL)
		{
			memset(mainAppCt.ADCFIFO, 0, MicHwCt.ADCFifoLen);
		}
		else
		{
			APP_DBG("malloc ADCFIFO error\n");
			return FALSE;
		}
		MicHwCt.ADCFifo	= mainAppCt.ADCFIFO,
		AudioADC_DynamicElementMatch(MIC_ADC_SEL, TRUE, TRUE);

		//Mic1
		AudioADC_Init(MIC_ADC_SEL, &MicHwCt); //

	#ifdef CFG_FUNC_MCLK_USE_CUSTOMIZED_EN
        Clock_AudioMclkSel(MIC_MODULE, MIC_CTRLDIGITAL_SEL.adc_mclk_source > 2 ? (MIC_CTRLDIGITAL_SEL.adc_mclk_source - 1) : MIC_CTRLDIGITAL_SEL.adc_mclk_source);
	#else
        MIC_CTRLDIGITAL_SEL.CLK_Source = Clock_AudioMclkGet(MIC_ADC_SEL);
	#endif

		//Soure0 - mic source
		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
		AudioIOSet.Adapt = STD;
		AudioIOSet.Sync = TRUE;
		AudioIOSet.Channels = 2;//mono
		AudioIOSet.Net = AudioCoreGetNetBySourceChannelNUM(MIC_SOURCE_NUM);
		AudioIOSet.DataIOFunc = MIC_DATAGET_SEL;
		AudioIOSet.LenGetFunc = MIC_LENGET_SEL;
		AudioIOSet.PcmWidth	= CFG_DEFAULT_WIDTH;
		AudioIOSet.SampleRate = sampleRate;
		if(!AudioCoreSourceInit(&AudioIOSet, MIC_SOURCE_NUM))
		{
			DBG("mic Source error");
			return FALSE;
		}
	}
	
//	AudioADC_AnaInit(MIC_ADC_SEL, &MicHwCt);

	//MADC_MIC_PowerUP(SingleEnded);
	MIC_CTRLDIGITAL_SEL.Sample_Rate = SampleRateIndexGet(AudioADC_SampleRateGet(MIC_ADC_SEL));

	#ifdef CFG_FUNC_AUDIO_SILENCE_AVOIDANCE_EN
	while(DMA_CircularDataLenGet(MIC_DMA_PERI_ID_SEL) < MicHwCt.ADCFifoLen/2)
	{
//	    printf("ADC %d \n",DMA_CircularDataLenGet(MIC_DMA_PERI_ID_SEL));
	}
	#endif
	AudioCoreSourceEnable(MIC_SOURCE_NUM);
#endif

#ifdef CFG_FUNC_REMIND_SOUND_EN
	if(!AudioCoreSourceIsInit(REMIND_SOURCE_NUM))
	{
		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
		AudioIOSet.Adapt = SRC_ONLY;
		AudioIOSet.SrcQuality = 0;//最低音质,节约资源
		AudioIOSet.SampleRate = AudioCoreMixSampleRateGet(AudioCoreGetNetBySourceChannelNUM(REMIND_SOURCE_NUM));//初始值
		AudioIOSet.Sync = FALSE;
		AudioIOSet.Channels = get_remind_channels();
		AudioIOSet.Net = AudioCoreGetNetBySourceChannelNUM(REMIND_SOURCE_NUM);
		AudioIOSet.DataIOFunc = RemindDataGet;
		AudioIOSet.LenGetFunc = RemindDataLenGet;
		AudioIOSet.PcmWidth	= WIDTH_16BIT;
		if(!AudioCoreSourceInit(&AudioIOSet, REMIND_SOURCE_NUM))
		{
			DBG("remind source error!\n");
			SoftFlagRegister(SoftFlagNoRemind);
			return FALSE;
		}
	#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
		RemindMp3DecoderInit();
	#endif
	}
#endif

#if defined(CFG_RES_AUDIO_SPDIF0OUT_EN) || defined(CFG_RES_AUDIO_SPDIF1OUT_EN)
//	if(!SpdifOutInit())
//	{
//		return FALSE;
//	}
#endif

#ifdef CFG_FUNC_I2S0_MIX
	if(!I2S0_MixInit(TRUE))
	{
		return FALSE;
	}
#endif
#ifdef CFG_FUNC_I2S1_MIX
	if(!I2S1_MixInit(TRUE))
	{
		return FALSE;
	}
#endif
#ifdef CFG_FUNC_I2S2_MIX
	if(!I2S2_MixInit(TRUE))
	{
		return FALSE;
	}
#endif

#ifdef CFG_RES_DAC_MONITOR_EN
#ifndef CFG_HP_DAC0_BOOT_MUTE_TEST
	AudioDAC_SoftMute(DAC_SEL, 0, 0);
#endif
	AudioDAC_Enable(DAC_SEL);
#endif
#ifdef CFG_COMMUNICATION_BY_UART
	uart_data_init();
#endif
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
	AudioEffect_CommunicationQueue_Init();
#endif
	return TRUE;
}

//各模式下的通用消息处理, 共有的提示音在此处理，因此要求调用次API前，确保APP running状态。避免解码器没准备好。
void CommonMsgProccess(uint16_t Msg)
{
	if(SoftFlagGet(SoftFlagDiscDelayMask) && Msg == MSG_NONE)
	{
		Msg = MSG_BT_STATE_DISCONNECT;
	}

	switch(Msg)
	{
#if (BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT)
		case MSG_LE_BIS_SRC_CMD_TEST:
			if(!IsBtHfMode())
			{
				extern void app_bis_send_vendor_cmd(uint32_t vendor_data);

				app_bis_send_vendor_cmd(0x55aa1122);
				printf("MSG_LE_BIS_SRC_CMD_TEST!\n");
			}
			break;

#ifdef SINK_CHOOSE_SRC
		case MSG_BIS_SINK_ADD_SRC_0:
			if(!IsBtHfMode())
			{
				uint8_t src_idx = 0;
				app_add_src_for_sink(src_idx);
				printf("MSG_BIS_SINK_ADD_SRC_0!\n");
			}
			break;

		case MSG_BIS_SINK_RMV_SRC:
			if(!IsBtHfMode())
			{
				le_bis_sink_remove_src();
			}
			break;
			
		case MSG_BIS_SINK_ADD_SRC_1:
			if(!IsBtHfMode())
			{
				uint8_t src_idx = 1;
				app_add_src_for_sink(src_idx);
				printf("MSG_BIS_SINK_ADD_SRC_1!\n");
			}
			break;
#endif
		case MSG_LE_BIS:
			if(!IsBtHfMode())
			{
#if (BT_TWS_SUPPORT == ENABLE)
				if(!tws_exit_bis_enter()) break;
#endif

				if(bis_enter_btlink_check(MSG_LE_BIS)) break;

				BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_BIS_EVENT);
			}
			break;

#if 1
		case MSG_LE_BIS_SRC_START:
			if(!IsBtHfMode())
			{
#if (BT_TWS_SUPPORT == ENABLE)
				if(!tws_exit_bis_src_enter()) break;
#endif
				if(bis_enter_btlink_check(MSG_LE_BIS_SRC_START)) break;

				BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_BIS_SRC_START);
			}
			break;

		case MSG_LE_BIS_SRC_STOP:
			if(!IsBtHfMode())
			{
				printf("MSG_LE_BIS_SRC_STOP\n");
				BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_BIS_SRC_STOP);
			}
			break;

		case MSG_LE_BIS_SNK_START:
			if(!IsBtHfMode())
			{
#if (BT_TWS_SUPPORT == ENABLE)
				if(!tws_exit_bis_sink_enter()) break;
#endif
				BtStackServiceMsgSend(MSG_BTSTACK_MSG_BLE_ADV_STOP);
				BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_BIS_SNK_START);
			}
			break;

		case MSG_LE_BIS_SNK_STOP:
			if(!IsBtHfMode())
			{
				sink_wait_starting = 0;//clear flag
				BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_BIS_SNK_STOP);
				BtStackServiceMsgSend(MSG_BTSTACK_MSG_BLE_ADV_START);
			}
			break;
#endif

		case MSG_LE_BIS_DISCONNECT:
			printf("bis disconnect\n");
			if(GetSystemMode() != ModeLeAudioSinkPlay)
				break;
	#ifndef LE_BIS_DELEG
	#ifdef LE_UDSS
			// app_udss_adv_stop();
	#endif
			le_bis_sink_start();
	#endif
			break;
			
#ifdef LE_UDSS
		case MSG_LE_UDS_ADV_START:
			printf("MSG_LE_UDS_ADV_START\n");
			uint16_t status = app_udss_start_adv();
			if (status)
			{
				printf("MSG_LE_UDS_ADV_START error 0x%x\n", status);
			}
			break;
#endif
#endif//(BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT)

#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
#ifdef LE_UDSC
		case MSG_LE_SCAN_START:
			printf("MSG_LE_SCAN_START\n");
			app_le_udsc_start_scan();
			break;
#endif
#endif

#ifdef CFG_DMA_RGB_LED_EN
		case MSG_RGB_MODE:
			RgbModeSwitch();
			#ifdef CFG_FUNC_BREAKPOINT_EN
			BackupInfoUpdata(BACKUP_SYS_INFO);
			#endif
			break;
#endif
		case MSG_MUTE:
			APP_DBG("MSG_MUTE\n");
			#ifdef  CFG_APP_HDMIIN_MODE_EN
			extern HDMIInfo         *gHdmiCt;
			if(GetSystemMode() == ModeHdmiAudioPlay)
			{
				if(IsHDMISourceMute() == TRUE)
					HDMISourceUnmute();
				else
					HDMISourceMute();
				gHdmiCt->hdmiActiveReportMuteStatus = IsHDMISourceMute();
				gHdmiCt->hdmiActiveReportMuteflag = 2;
			}
			else
			#endif
			{
				HardWareMuteOrUnMute();
			}
			#ifdef CFG_FUNC_DISPLAY_EN
			SendSysModeMsg(MSG_DISPLAY_SERVICE_MUTE);
			#endif
			break;
#ifdef CFG_APP_BT_MODE_EN
		case MSG_BT_PLAY_SYNC_VOLUME_CHANGED:
			APP_DBG("MSG_BT_PLAY_SYNC_VOLUME_CHANGED\n");
#if (BT_AVRCP_VOLUME_SYNC)
			AudioMusicVolSet(GetBtSyncVolume());

#if (BT_TWS_SUPPORT == ENABLE)
			tws_vol_send(GetBtSyncVolume(), IsAudioPlayerMute());
#endif
#endif//BT_AVRCP_VOLUME_SYNC
			break;
#endif
#ifdef CFG_FUNC_ROBOEFFECT_EN
		case MSG_EFFECTPARAM_SWITCH:    //音效参数切换
			APP_DBG("MSG_EFFECTPARAM_SWITCH\n");
			AudioEffect_ParamSwitch();
			AudioEffect_Restart();
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
			gCtrlVars.AutoRefresh = AutoRefresh_ALL_EFFECTS_PARA;
			AudioEffect_CommunicationQueue_Send(gCtrlVars.AutoRefresh);
#endif
			break;

		case MSG_EFFECTFLOW_SWITCH:   //音效图切换
			APP_DBG("MSG_EFFECTFLOW_SWITCH\n");
			AudioEffect_FlowSwitch();
			AudioEffect_Restart();
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
			gCtrlVars.AutoRefresh = AutoRefresh_ALL_PARA;
			AudioEffect_CommunicationQueue_Send(gCtrlVars.AutoRefresh);
#endif
			break;

		case MSG_EFFECT_REFRESH:
			APP_DBG("MSG_EFFECT_REFRESH\n");
			AudioEffect_Restart();
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
			gCtrlVars.AutoRefresh = AutoRefresh_ALL_EFFECTS_PARA;
			AudioEffect_CommunicationQueue_Send(gCtrlVars.AutoRefresh);
#endif
			break;

		case MSG_ADJUST_ONEPARAM:
			DBG("MSG_ADJUST_ONEPARAM!\n");
			 int16_t parameter = 200;
			AudioEffect_Enable(EffectParambin.context_memory, 0x81, 1);
			//调整当前默认音效图0x81号音效（一般为gain_control）第1个参数为parameter
			AudioEffect_ParamSet(EffectParambin.context_memory, 0x81, 1, &parameter);
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
			gCtrlVars.AutoRefresh = 0x81; 
			AudioEffect_CommunicationQueue_Send(gCtrlVars.AutoRefresh);
#endif
			break;
#endif//#ifdef CFG_FUNC_ROBOEFFECT_EN

		#ifdef CFG_FUNC_RTC_EN
		case MSG_RTC_SET_TIME:
			RTC_ServiceModeSelect(0,0);
			break;

		case MSG_RTC_SET_ALARM:
			RTC_ServiceModeSelect(0,1);
			break;

		case MSG_RTC_DISP_TIME:
			RTC_ServiceModeSelect(0,2);
			break;

		case MSG_RTC_UP:
			RTC_RtcUp();
			break;

		case MSG_RTC_DOWN:
			RTC_RtcDown();
			break;

		#ifdef CFG_FUNC_SNOOZE_EN
		case MSG_RTC_SNOOZE:
			if(mainAppCt.AlarmRemindStart)
			{
				mainAppCt.AlarmRemindCnt = 0;
				mainAppCt.AlarmRemindStart = 0;
				mainAppCt.SnoozeOn = 1;
				mainAppCt.SnoozeCnt = 0;
			}
			break;
		#endif

		#endif //end of  CFG_FUNC_RTC_EN

		case MSG_REMIND1:
			#ifdef CFG_FUNC_REMIND_SOUND_EN
			RemindSoundServiceItemRequest(SOUND_REMIND_SHANGYIS,FALSE);
			#endif
			break;

		case MSG_DEVICE_SERVICE_BATTERY_LOW:
			//RemindSound request
			APP_DBG("MSG_DEVICE_SERVICE_BATTERY_LOW\n");
			#ifdef CFG_FUNC_REMIND_SOUND_EN
			RemindSoundServiceItemRequest(SOUND_REMIND_DLGUODI, FALSE);
			#endif
			break;

#ifdef CFG_APP_BT_MODE_EN
		case MSG_BT_OPEN_ACCESS:
			if (GetBtManager()->btAccessModeEnable != BT_ACCESSBLE_GENERAL)
			{
				GetBtManager()->btAccessModeEnable = BT_ACCESSBLE_GENERAL;
				BtStackServiceMsgSend(MSG_BTSTACK_ACCESS_MODE_SET);
				DBG("open bt access\n");
			}
			break;
			
		//蓝牙连接断开消息,用于提示音
		case MSG_BT_STATE_CONNECTED:
			APP_DBG("[BT_STATE]:BT Connected...\n");
			//异常回连过程中，不提示连接断开提示音
			SetBtUserState(BT_USER_STATE_CONNECTED);

#ifdef BT_ACCESS_MODE_SET_BY_POWER_ON_TIMEOUT
			btManager.btvisibilityDelayOn = FALSE;
#endif
			BtStackServiceMsgSend(MSG_BTSTACK_ACCESS_MODE_SET);

			if(btManager.btDutModeEnable)
				break;

			//if(!(btCheckEventList&BT_EVENT_L2CAP_LINK_DISCONNECT))
			{
				#ifdef CFG_FUNC_REMIND_SOUND_EN
				if(RemindSoundServiceItemRequest(SOUND_REMIND_CONNECT, REMIND_PRIO_SYS|REMIND_ATTR_NEED_HOLD_PLAY))
				{
					if(!SoftFlagGet(SoftFlagWaitBtRemindEnd)&&SoftFlagGet(SoftFlagDelayEnterBtHf))
					{
						SoftFlagRegister(SoftFlagWaitBtRemindEnd);
					}
				}
				else
				#endif
				{
					if(SoftFlagGet(SoftFlagDelayEnterBtHf))
					{
						MessageContext		msgSend;
						SoftFlagDeregister(SoftFlagDelayEnterBtHf);

						msgSend.msgId = MSG_DEVICE_SERVICE_ENTER_BTHF_MODE;
						MessageSend(GetMainMessageHandle(), &msgSend);
					}
				}
			}
			break;

		case MSG_BT_STATE_DISCONNECT:
			APP_DBG("[BT_STATE]:BT Disconnected...\n");
			SoftFlagDeregister(SoftFlagDiscDelayMask);
			SetBtUserState(BT_USER_STATE_DISCONNECTED);
			
			if(btManager.btDutModeEnable)
				break;

			BtStackServiceMsgSend(MSG_BTSTACK_ACCESS_MODE_SET);

			//异常回连过程中，不提示连接断开提示音
			//if(!(btCheckEventList&BT_EVENT_L2CAP_LINK_DISCONNECT))
			{
				#ifdef CFG_FUNC_REMIND_SOUND_EN
				if(GetSystemMode() != ModeIdle)
				{
					RemindSoundServiceItemRequest(SOUND_REMIND_DISCONNE, REMIND_PRIO_SYS|REMIND_ATTR_NEED_HOLD_PLAY);
				}
				#endif
			}
			break;

		case MSG_BT_CLEAR_PAIRED_LIST:
			memset(btManager.btLinkDeviceInfo,0,sizeof(btManager.btLinkDeviceInfo));
			BtDdb_EraseBtLinkInforMsg();
			break;
#endif

#ifdef BT_PROFILE_BQB_ENABLE
		case MSG_A2DP_CONNECT:
			APP_DBG("MSG_A2DP_CONNECT\n");
			A2dpConnect(0,btManager.btDdbLastAddr);
		break;


		case MSG_AVRCP_CONNECT:
			APP_DBG("MSG_AVRCP_CONNECT\n");
			AvrcpConnect(0,btManager.btDdbLastAddr);
		break;

		case MSG_AVRCP_STOP:
			APP_DBG("MSG_AVRCP_STOP\n");
			AvrcpCtrlStop(0);
			break;

		case MSG_HFP_CONNECT:
			APP_DBG("MSG_HFP_CONNECT\n");
			HfpConnect(0,btManager.btDdbLastAddr);
		break;

		case MSG_SCO_CONNECT:
			APP_DBG("MSG_SCO_CONNECT\n");
			HfpAudioConnect(0);
		break;

		case MSG_BT_BQB_AVDTP_SMG:
		{
			BTBqbAvdtpSmgSet(1);
			APP_DBG("enable AVDTP/SNK/ACP/SIG/SMG/BI-05-C BI-33-C test\n");
		}
		break;

		case MSG_BT_BQB_AVDTP_SMG_BI38C:
		{
			BTBqbAvdtpSmgBI38CSet(1);
			APP_DBG("enable AVDTP/SNK/ACP/SIG/SMG/BI-38-C test\n");
		}
		break;

		case MSG_BT_BQB_VRR_BV_01_C:
			if(GetHfpState(0) == BT_HFP_STATE_CONNECTED)
			{
				APP_DBG("MSG_BT_BQB_VRR_BV_01_C\n");
				HfpVoiceRecognition(0, '2');
			}
			break;
#endif//BT_PROFILE_BQB_ENABLE

		default:
			#ifdef CFG_FUNC_DISPLAY_EN
			//display
			Display(Msg);
			#endif
			#ifdef CFG_ADC_LEVEL_KEY_EN
			AdcLevelMsgProcess(Msg);
			#endif
			break;
	}
}

/*
 * FunctionName : tws_device_close
 * Description  : TWS设备音频配置
 */
void tws_device_close(void)
{
#if (BT_TWS_SUPPORT == ENABLE)
	btManager.TwsDacStatus = FALSE;
#endif
#ifdef  TWS_IIS0_OUT
	*(volatile unsigned long *)0x40029000 &= ~0x80;//disable iis0
	RST_I2SModule(I2S0_MODULE);
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S0_TX, DMA_DONE_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S0_TX, DMA_THRESHOLD_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S0_TX, DMA_ERROR_INT);
	DMA_ChannelDisable(PERIPHERAL_ID_I2S0_TX);

	#ifdef CFG_RES_AUDIO_I2S0OUT_EN
	DMA_CircularConfig(PERIPHERAL_ID_I2S0_TX, mainAppCt.I2S0_TX_FIFO_LEN/2, mainAppCt.I2S0_TX_FIFO, mainAppCt.I2S0_TX_FIFO_LEN);
	DMA_CircularWritePtrSet(PERIPHERAL_ID_I2S0_TX, mainAppCt.I2S0_TX_FIFO_LEN - sizeof(PCM_DATA_TYPE) * 2);
	#else
	DMA_CircularConfig(PERIPHERAL_ID_I2S0_TX, mainAppCt.I2SFIFO_LEN/2, mainAppCt.I2SFIFO, mainAppCt.I2SFIFO_LEN);
	DMA_CircularWritePtrSet(PERIPHERAL_ID_I2S0_TX, mainAppCt.I2SFIFO_LEN - sizeof(PCM_DATA_TYPE) * 2);
	#endif
	DMA_ChannelEnable(PERIPHERAL_ID_I2S0_TX);
#endif

#ifdef TWS_IIS1_OUT
	*(volatile unsigned long *)0x4002A000 &= ~0x80;//disable iis1
	RST_I2SModule(I2S1_MODULE);
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S1_TX, DMA_DONE_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S1_TX, DMA_THRESHOLD_INT);
	DMA_InterruptFlagClear(PERIPHERAL_ID_I2S1_TX, DMA_ERROR_INT);
	DMA_ChannelDisable(PERIPHERAL_ID_I2S1_TX);

	#ifdef CFG_RES_AUDIO_I2S1OUT_EN
	DMA_CircularConfig(PERIPHERAL_ID_I2S1_TX, mainAppCt.I2S1_TX_FIFO_LEN/2, mainAppCt.I2S1_TX_FIFO, mainAppCt.I2S1_TX_FIFO_LEN);
	DMA_CircularWritePtrSet(PERIPHERAL_ID_I2S1_TX, mainAppCt.I2S1_TX_FIFO_LEN - sizeof(PCM_DATA_TYPE) * 2);
	#else
	DMA_CircularConfig(PERIPHERAL_ID_I2S1_TX, mainAppCt.I2SFIFO_LEN/2, mainAppCt.I2SFIFO, mainAppCt.I2SFIFO_LEN);
	DMA_CircularWritePtrSet(PERIPHERAL_ID_I2S1_TX, mainAppCt.I2SFIFO_LEN - sizeof(PCM_DATA_TYPE) * 2);
	#endif
	DMA_ChannelEnable(PERIPHERAL_ID_I2S1_TX);
#endif
#ifdef CFG_RES_DAC_MONITOR_EN
#if (TWS_DAC_SEL_VAL == DAC0_MODULE_VAL)
	if(mainAppCt.DACFIFO == NULL) return;
#elif (TWS_DAC_SEL_VAL == DAC1_MODULE_VAL)
	#ifdef CFG_RES_AUDIO_DAC1_MIX_OUT_EN
		if(mainAppCt.DAC1FIFO == NULL) return;
	#else
		if(mainAppCt.DACFIFO == NULL) return;
	#endif
#else
	if(mainAppCt.DAC2FIFO == NULL) return;
#endif
#endif

#if (BT_TWS_SUPPORT == ENABLE)
#ifdef TWS_DAC0_OUT
	AudioDAC_DigitalMute(TWS_DAC_SEL, 1, 1);
	AudioDAC_DeInit(TWS_DAC_SEL);

	tws_dac_mute_flag_set(1);

//	extern void tws_pcm_buf_clear(void);
//	tws_pcm_buf_clear();
#if (TWS_DAC_SEL_VAL == DAC0_MODULE_VAL)
	{
		DMA_CircularConfig(TWS_DAC_DMA_PERI_ID_SEL, mainAppCt.DACFIFO_LEN/2, mainAppCt.DACFIFO, mainAppCt.DACFIFO_LEN);
		DMA_CircularWritePtrSet(TWS_DAC_DMA_PERI_ID_SEL, mainAppCt.DACFIFO_LEN - sizeof(PCM_DATA_TYPE) * 2);
		if(mainAppCt.DACFIFO != NULL)
			memset(mainAppCt.DACFIFO,0,mainAppCt.DACFIFO_LEN);
	}
#elif (TWS_DAC_SEL_VAL == DAC1_MODULE_VAL)
	{
		#ifdef CFG_RES_AUDIO_DAC1_MIX_OUT_EN
		DMA_CircularConfig(TWS_DAC_DMA_PERI_ID_SEL, mainAppCt.DAC1FIFO_LEN/2, mainAppCt.DAC1FIFO, mainAppCt.DAC1FIFO_LEN);
		DMA_CircularWritePtrSet(TWS_DAC_DMA_PERI_ID_SEL, mainAppCt.DAC1FIFO_LEN - sizeof(PCM_DATA_TYPE) * 2);
		if(mainAppCt.DACFIFO != NULL)
			memset(mainAppCt.DACFIFO,0,mainAppCt.DACFIFO_LEN);
		#else
		DMA_CircularConfig(TWS_DAC_DMA_PERI_ID_SEL, mainAppCt.DACFIFO_LEN/2, mainAppCt.DACFIFO, mainAppCt.DACFIFO_LEN);
		DMA_CircularWritePtrSet(TWS_DAC_DMA_PERI_ID_SEL, mainAppCt.DACFIFO_LEN - sizeof(PCM_DATA_TYPE) * 2);
		if(mainAppCt.DACFIFO != NULL)
			memset(mainAppCt.DACFIFO,0,mainAppCt.DACFIFO_LEN);
		#endif
	}
#else
	{
		DMA_CircularConfig(TWS_DAC_DMA_PERI_ID_SEL, mainAppCt.DAC2FIFO_LEN/2, mainAppCt.DAC2FIFO, mainAppCt.DAC2FIFO_LEN);
		DMA_CircularWritePtrSet(TWS_DAC_DMA_PERI_ID_SEL, mainAppCt.DAC2FIFO_LEN - sizeof(PCM_DATA_TYPE) * 2);
		if(mainAppCt.DAC2FIFO != NULL)
			memset(mainAppCt.DAC2FIFO,0,mainAppCt.DAC2FIFO_LEN);
	}
#endif
	DMA_ChannelEnable(TWS_DAC_DMA_PERI_ID_SEL);

	AudioDAC_DigitalMute(TWS_DAC_SEL, 0, 0);
	AudioDAC_Reset(TWS_DAC_SEL);
#endif
#endif
}

__attribute__((section(".tws_sync_code")))
void tws_device_open_isr(void)
{
#if (BT_TWS_SUPPORT == ENABLE)
	btManager.TwsDacStatus = TRUE;
#endif
#ifdef TWS_DAC0_OUT
	#if DAC_RESET_SET
	AudioDAC_Run(TWS_DAC_SEL);
	#else
	AudioDAC_Enable(TWS_DAC_SEL);
	#endif
#endif
#ifdef  TWS_IIS0_OUT
	*(volatile unsigned long *)0x40029000 |= 0x80;//enable iis0
#endif
#ifdef TWS_IIS1_OUT
	*(volatile unsigned long *)0x4002A000 |= 0x80;//enable iis1
#endif
}

#if (BT_TWS_SUPPORT == ENABLE)
void tws_effect_switch(uint8_t sw)
{
#if !defined(ROBOEFFECT_DOUBLE_ENGINE_EN) || (BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT)	//双框图不需要切框图
	APP_DBG("tws_effect_switch %d\n",sw);

	MessageContext		msgSend;
	msgSend.msgId = MSG_BT_TWS_EFFECT_SWTICH;
	MessageSend(GetMainMessageHandle(), &msgSend);
#endif
}

void tws_a2dp_refresh(void)
{
	if(GetA2dpState(0) == BT_A2DP_STATE_STREAMING)
	{
		APP_DBG("tws a2dp decoder refresh\n");
		a2dp_sbc_decoder_init();
	}
}
#endif
