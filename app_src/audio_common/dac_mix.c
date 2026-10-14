/**
 **************************************************************************************
 * @file    dac1_mix.c
 * @brief   DAC1 MIX output initialization
 *
 * @author
 * @version V1.0.0
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include "app_config.h"
#include "main_task.h"
#include "audio_core_api.h"
#include "dac_interface.h"
#include "clk.h"
#include "module_config.h"
#include "rtos_api.h"
#include "ctrlvars.h"
#include "audio_effect_control.h"
#include "bt_tws_mode.h"
#ifdef CFG_RES_AUDIO_DAC1_MIX_OUT_EN
bool DAC1_MixInit(void)
{
	AudioCoreIO	AudioIOSet;

	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
	AudioIOSet.Depth = AudioCore.NetCt[AudioCoreGetNetBySinkChannelNUM(AUDIO_DAC_MIX1_OUT_SINK_NUM)].FrameSize * 3;
	AudioIOSet.PcmWidth = CFG_DEFAULT_WIDTH;

	if(!AudioCoreSinkIsInit(AUDIO_DAC_MIX1_OUT_SINK_NUM))
	{
		APP_DBG("AudioCoreSinkIsInit AUDIO_DAC_MIX1_OUT_SINK_NUM\n");
#if (BT_LE_AUDIO_SUPPORT && !defined(ROBOEFFECT_DOUBLE_ENGINE_EN))	//双图下面不关闭硬件操作，buffer缓存到BRIDGE_MUSIC_SINK
		mainAppCt.DAC1FIFO_LEN = get_bis_sync_need_samples(AudioIOSet.Depth) * sizeof(PCM_DATA_TYPE) * 2;
#else
		mainAppCt.DAC1FIFO_LEN = AudioIOSet.Depth * sizeof(PCM_DATA_TYPE) * 2;
#endif

#if (BT_TWS_SUPPORT == ENABLE)
		  if(BtTwsStatusGet() && TWS_DAC_SEL == DAC1_MODULE)
		 {
			 mainAppCt.DACFIFO_LEN = AudioIOSet.Depth * sizeof(PCM_DATA_TYPE) * 2;
		 }
#endif

		mainAppCt.DAC1FIFO = (uint32_t*)osPortMalloc(mainAppCt.DAC1FIFO_LEN);
		if(mainAppCt.DAC1FIFO != NULL)
		{
			memset(mainAppCt.DAC1FIFO, 0, mainAppCt.DAC1FIFO_LEN);
		}
		else
		{
			APP_DBG("malloc DAC1FIFO error\n");
			return FALSE;
		}

		//sink DAC1
		AudioIOSet.Adapt = STD;
		AudioIOSet.Sync = TRUE;
		AudioIOSet.Channels = 2;
		AudioIOSet.Net = AudioCoreGetNetBySinkChannelNUM(AUDIO_DAC_MIX1_OUT_SINK_NUM);

		#ifdef CFG_APP_BIS_BT_DUAL_CHIP_MODE_EN
			AudioIOSet.DataIOFunc = AudioDAC1_DataSet_LeAudio;
		#else
			AudioIOSet.DataIOFunc = DAC1_DATASET_SEL;
		#endif
		AudioIOSet.LenGetFunc = DAC1_LENGET_SEL;

		#if (BT_TWS_SUPPORT == ENABLE)
			 if(BtTwsStatusGet() && TWS_DAC_SEL == DAC1_MODULE)
			{
				AudioIOSet.DataIOFunc = AudioDAC1DataSet_tws;
			}
		#endif

		if(!AudioCoreSinkInit(&AudioIOSet, AUDIO_DAC_MIX1_OUT_SINK_NUM))
		{
			DBG("DAC1 init error\n");
			return FALSE;
		}
		AudioCoreSinkAudioPreset(AUDIO_DAC_MIX1_OUT_SINK_NUM);
		gCtrlVars.HwCt.DAC1Ct.Enable = 1;

		AUDIODAC_ParamCt Dac1ParamCt =
		{
			.SampleRate 	= AudioCoreMixSampleRateGet(AudioCoreGetNetBySinkChannelNUM(AUDIO_DAC_MIX1_OUT_SINK_NUM)),
			.DACMode 		= DAC1_MODE_SEL,
			.DACLoadStatus 	= DAC_NOLoad,
			.DACEnergyModel = ADC_ENERGY_SEL,
			.DACBitWidth	= DAC_WIDTH_SEL,
			.DoutMode		= DAC1_DOUT_MODE_SEL,
			.DacFifo		= mainAppCt.DAC1FIFO,
			.DacFifoLen		= mainAppCt.DAC1FIFO_LEN,
			.OCPEnable		= FALSE,
	#ifdef DELAY_SINK_DAC1_MIX_EN
			.FIRMode		= TRUE,
	#endif
		};
		AudioDAC_Init(DAC1_MODULE, NULL, &Dac1ParamCt, NULL);

	#ifdef CFG_FUNC_MCLK_USE_CUSTOMIZED_EN
        Clock_AudioMclkSel(AUDIO_DAC1, gCtrlVars.HwCt.DAC1Ct.dac_mclk_source > 2 ? (gCtrlVars.HwCt.DAC1Ct.dac_mclk_source - 1):gCtrlVars.HwCt.DAC1Ct.dac_mclk_source);
	#else
        gCtrlVars.HwCt.DAC1Ct.CLK_Source = Clock_AudioMclkGet(AUDIO_DAC1);
	#endif
	}
	else
	{
		AudioDAC_SampleRateChange(DAC1_MODULE, AudioCoreMixSampleRateGet(AudioCoreGetNetBySinkChannelNUM(AUDIO_DAC_MIX1_OUT_SINK_NUM)));
		gCtrlVars.HwCt.DAC1Ct.CLK_Source = Clock_AudioMclkGet(AUDIO_DAC1);
		AudioCore.AudioSink[AUDIO_DAC_MIX1_OUT_SINK_NUM].PcmWidth = CFG_DEFAULT_WIDTH;
	}
	gCtrlVars.HwCt.DAC1Ct.Sample_Rate = SampleRateIndexGet(AudioDAC_SampleRateGet(DAC1_MODULE));
	AudioCoreSinkEnable(AUDIO_DAC_MIX1_OUT_SINK_NUM);

	return TRUE;
}

void DAC1_MixDeinit(void)
{
	AudioDAC_SoftMute(DAC1_MODULE, 1, 1);
	AudioDAC_DeInit(DAC1_MODULE);

	if(mainAppCt.DAC1FIFO != NULL)
	{
		APP_DBG("DAC1FIFO\n");
		osPortFree(mainAppCt.DAC1FIFO);
		mainAppCt.DAC1FIFO = NULL;
	}
	AudioCoreSinkDeinit(AUDIO_DAC_MIX1_OUT_SINK_NUM);
}

#endif /* CFG_RES_AUDIO_DAC1_MIX_OUT_EN */

#ifdef CFG_RES_AUDIO_DAC2_MIX_OUT_EN
bool DAC2_MixInit(void)
{
	AudioCoreIO	AudioIOSet;

	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
	AudioIOSet.Depth = AudioCore.NetCt[AudioCoreGetNetBySinkChannelNUM(AUDIO_DAC_MIX2_OUT_SINK_NUM)].FrameSize * 3;
	AudioIOSet.PcmWidth = CFG_DEFAULT_WIDTH;

	if(!AudioCoreSinkIsInit(AUDIO_DAC_MIX2_OUT_SINK_NUM))
	{
		APP_DBG("AudioCoreSinkIsInit AUDIO_DAC_MIX2_OUT_SINK_NUM\n");
#ifdef CFG_APP_BIS_BT_DUAL_CHIP_MODE_EN
		mainAppCt.DAC2FIFO_LEN = AudioIOSet.Depth * sizeof(PCM_DATA_TYPE) * 20;
#else
		mainAppCt.DAC2FIFO_LEN = AudioIOSet.Depth * sizeof(PCM_DATA_TYPE) * 2;
#endif

#if (BT_TWS_SUPPORT == ENABLE)
		 if(BtTwsStatusGet() && TWS_DAC_SEL == DAC2_MODULE)
		 {
			 mainAppCt.DAC2FIFO_LEN = AudioIOSet.Depth * sizeof(PCM_DATA_TYPE) * 2 * 2;
		 }
#endif
		mainAppCt.DAC2FIFO = (uint32_t*)osPortMalloc(mainAppCt.DAC2FIFO_LEN);
		if(mainAppCt.DAC2FIFO != NULL)
		{
			memset(mainAppCt.DAC2FIFO, 0, mainAppCt.DAC2FIFO_LEN);
		}
		else
		{
			APP_DBG("malloc DAC2FIFO error\n");
			return FALSE;
		}

		//sink DAC2
		AudioIOSet.Adapt = STD;
		AudioIOSet.Sync = TRUE;
		AudioIOSet.Channels = 1;
		AudioIOSet.Net = AudioCoreGetNetBySinkChannelNUM(AUDIO_DAC_MIX2_OUT_SINK_NUM);
		AudioIOSet.DataIOFunc = DAC2_DATASET_SEL;
		AudioIOSet.LenGetFunc = DAC2_LENGET_SEL;

		#if (BT_TWS_SUPPORT == ENABLE)
			if(BtTwsStatusGet() && TWS_DAC_SEL == DAC2_MODULE)
			{
				AudioIOSet.DataIOFunc = AudioDAC1DataSet_tws;
			}
		#endif

		if(!AudioCoreSinkInit(&AudioIOSet, AUDIO_DAC_MIX2_OUT_SINK_NUM))
		{
			DBG("DAC2 init error\n");
			return FALSE;
		}
		AudioCoreSinkAudioPreset(AUDIO_DAC_MIX2_OUT_SINK_NUM);
		gCtrlVars.HwCt.DAC2Ct.Enable = 1;

		AUDIODAC_ParamCt Dac2ParamCt =
		{
			.SampleRate 	= AudioCoreMixSampleRateGet(AudioCoreGetNetBySinkChannelNUM(AUDIO_DAC_MIX2_OUT_SINK_NUM)),
			.DACMode 		= DAC2_MODE_SEL,
			.DACLoadStatus 	= DAC_NOLoad,
			.DACEnergyModel = ADC_ENERGY_SEL,
			.DACBitWidth	= DAC_WIDTH_SEL,
			.DoutMode		= DAC2_DOUT_MODE_SEL,
			.DacFifo		= mainAppCt.DAC2FIFO,
			.DacFifoLen		= mainAppCt.DAC2FIFO_LEN,
			.OCPEnable		= FALSE,
	#ifdef DELAY_SINK_DAC2_MIX_EN
			.FIRMode		= TRUE,
	#endif
		};
		AudioDAC_Init(DAC2_MODULE, NULL, NULL, &Dac2ParamCt);

	#ifdef CFG_FUNC_MCLK_USE_CUSTOMIZED_EN
        Clock_AudioMclkSel(AUDIO_DAC2, gCtrlVars.HwCt.DAC2Ct.dac_mclk_source > 2 ? (gCtrlVars.HwCt.DAC2Ct.dac_mclk_source - 1):gCtrlVars.HwCt.DAC2Ct.dac_mclk_source);
	#else
        gCtrlVars.HwCt.DAC2Ct.CLK_Source = Clock_AudioMclkGet(AUDIO_DAC2);
	#endif
	}
	else
	{
		AudioDAC_SampleRateChange(DAC2_MODULE, AudioCoreMixSampleRateGet(AudioCoreGetNetBySinkChannelNUM(AUDIO_DAC_MIX2_OUT_SINK_NUM)));
		gCtrlVars.HwCt.DAC2Ct.CLK_Source = Clock_AudioMclkGet(AUDIO_DAC2);
		AudioCore.AudioSink[AUDIO_DAC_MIX2_OUT_SINK_NUM].PcmWidth = CFG_DEFAULT_WIDTH;
	}
	gCtrlVars.HwCt.DAC2Ct.Sample_Rate = SampleRateIndexGet(AudioDAC_SampleRateGet(DAC2_MODULE));
	AudioCoreSinkEnable(AUDIO_DAC_MIX2_OUT_SINK_NUM);

	return TRUE;
}

void DAC2_MixDeinit(void)
{
	AudioDAC_SoftMute(DAC2_MODULE, 1, 1);
	AudioDAC_DeInit(DAC2_MODULE);

	if(mainAppCt.DAC2FIFO != NULL)
	{
		APP_DBG("DAC2FIFO\n");
		osPortFree(mainAppCt.DAC2FIFO);
		mainAppCt.DAC2FIFO = NULL;
	}
	AudioCoreSinkDeinit(AUDIO_DAC_MIX2_OUT_SINK_NUM);
}

#endif /* CFG_RES_AUDIO_DAC2_MIX_OUT_EN */
