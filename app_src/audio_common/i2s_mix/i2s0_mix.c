/*
 * i2s0_mix.c
 *
 *  Created on: 2026年4月22日
 *      Author: mike
 */

#include <string.h>
#include "type.h"
#include "dma.h"
#include "reset.h"
#include "chip_interrupt.h"
#include "adc_interface.h"
#include "dac_interface.h"
#include "app_config.h"
#include "mode_task_api.h"
#include "main_task.h"
#include "debug.h"
#include "i2s_interface.h"
#include "audio_core_api.h"
#include "audio_core_service.h"
#include "effect_service.h"
#include "ctrlvars.h"
#include "audio_effect_control.h"
#include "i2s_dpll_policy.h"

#ifdef CFG_FUNC_I2S0_MIX
#ifdef CFG_FUNC_I2S_DPLL_POLICY_EN
	static bool sI2S0MixStreamsEnabled = FALSE;
#endif
	#ifdef CFG_I2S0_MIX_OUT_EN //CFG_RES_AUDIO_I2S_MIX_OUT_EN
	static void AudioI2s0OutParamsSet(void)
	{
		I2SParamCt i2s_set;
		i2s_set.LineMode = Single_Mode;  //TDM 多线模式 TdmChNum 必须 >= 8，且不支持 Quad_Mode
		i2s_set.TdmChNum = TDM_CH8;    //非 TDM 模式，参数无效
		i2s_set.IsMasterMode = CFG_I2S0_MIX_MODE;// 0:master 1:slave
		i2s_set.SampleRate = CFG_I2S0_MIX_SAMPLERATE; //外设采样率
		i2s_set.ALIGN_MODE   = I2S_LOW_BITS_ACTIVE;
		i2s_set.I2sFormat = I2S_FORMAT_I2S;
	#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
		i2s_set.I2sBits = I2S_LENGTH_24BITS;
	#else
		i2s_set.I2sBits = I2S_LENGTH_16BITS;
	#endif

	#if defined (CFG_I2S0_MIX_IN_EN) && defined (CFG_I2S0_MIX_OUT_EN)
		if(GetSystemMode() == ModeBtHfPlay)  // 通话模式不支持音源mix
			i2s_set.I2sTxRxEnable = 1;
		else
			i2s_set.I2sTxRxEnable = 3;
	#elif defined (CFG_I2S0_MIX_IN_EN)
		i2s_set.I2sTxRxEnable = 2;
	#elif defined (CFG_I2S0_MIX_OUT_EN)
		i2s_set.I2sTxRxEnable = 1;
	#else
		i2s_set.I2sTxRxEnable = 0;
	#endif

		i2s_set.TxPeripheralID = PERIPHERAL_1_ID_I2S0_TX;

		i2s_set.TxBuf = (void*)mainAppCt.I2S0_MIX_TX_FIFO;

		i2s_set.TxLen = mainAppCt.I2S0_MIX_TX_FIFO_LEN;
	#ifdef I2S0_MIX_MCLK_GPIO
		SET_I2S_PIN_MODE((I2S0_MIX_MCLK_GPIO), GET_I2S_GPIO_MODE(I2S0_MIX_MCLK_GPIO));//mclk
	#endif
		SET_I2S_PIN_MODE((I2S0_MIX_LRCLK_GPIO),GET_I2S_GPIO_MODE(I2S0_MIX_LRCLK_GPIO));//lrclk
		SET_I2S_PIN_MODE((I2S0_MIX_BCLK_GPIO), GET_I2S_GPIO_MODE(I2S0_MIX_BCLK_GPIO));//bclk
		SET_I2S_PIN_MODE((I2S0_MIX_DOUT_GPIO), GET_I2S_GPIO_MODE(I2S0_MIX_DOUT_GPIO));//do

		AudioI2S_Init(I2S0_MODULE, &i2s_set);//

	#ifdef CFG_FUNC_MCLK_USE_CUSTOMIZED_EN
		Clock_AudioMclkSel(AUDIO_I2S0, gCtrlVars.HwCt.I2S0Ct.CLK_Source > 2 ? (gCtrlVars.HwCt.I2S0Ct.CLK_Source - 1):gCtrlVars.HwCt.I2S0Ct.CLK_Source);
	#else
		gCtrlVars.HwCt.I2S0Ct.CLK_Source = Clock_AudioMclkGet(AUDIO_I2S0);
	#endif
	}
	#endif
#ifdef CFG_FUNC_I2S_DPLL_POLICY_EN
	static void I2S0MixStreamsEnable(void)
	{
		if(sI2S0MixStreamsEnabled)
		{
			return;
		}
#ifdef CFG_I2S0_MIX_IN_EN
		APP_DBG("AudioCoreSourceEnable(I2S0_MIX_SOURCE_NUM)\n");
		AudioCoreSourceEnable(I2S0_MIX_SOURCE_NUM);
		if(!I2SDpllPolicy_ShouldBypassSra(I2S0_MODULE, (CFG_I2S0_MIX_MODE == I2S_SLAVE_MODE)))
		{
			AudioCoreSourceAdjust(I2S0_MIX_SOURCE_NUM, TRUE);
			APP_DBG("AudioCoreSourceAdjust(I2S0_MIX_SOURCE_NUM)\n");
		}
#endif
#ifdef CFG_I2S0_MIX_OUT_EN
		AudioCoreSinkEnable(AUDIO_I2S0_MIX_OUT_SINK_NUM);
		if(!I2SDpllPolicy_ShouldBypassSra(I2S0_MODULE, (CFG_I2S0_MIX_MODE == I2S_SLAVE_MODE)))
		{
			AudioCoreSinkAdjust(AUDIO_I2S0_MIX_OUT_SINK_NUM, TRUE);
		}
#endif
		sI2S0MixStreamsEnabled = TRUE;
	}

	static void I2S0MixStreamsDisable(void)
	{
		if(!sI2S0MixStreamsEnabled)
		{
			return;
		}
#ifdef CFG_I2S0_MIX_OUT_EN
		AudioCoreSinkDisable(AUDIO_I2S0_MIX_OUT_SINK_NUM);
#endif
#ifdef CFG_I2S0_MIX_IN_EN
		AudioCoreSourceDisable(I2S0_MIX_SOURCE_NUM);
#endif
		APP_DBG("AudioCoreSourceDisable(I2S0_MIX_SOURCE_NUM)\n");
		sI2S0MixStreamsEnabled = FALSE;
	}
#endif

	bool I2S0_MixInit(bool hf_mode_flag)
	{
		AudioCoreIO	AudioIOSet;
	#ifdef CFG_FUNC_I2S_DPLL_POLICY_EN
		sI2S0MixStreamsEnabled = FALSE;
	#endif

	#ifdef CFG_I2S0_MIX_IN_EN
		uint8_t I2S0MixIn_Net = AudioCoreGetNetBySourceChannelNUM(I2S0_MIX_SOURCE_NUM);
		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
		if((!AudioCoreSourceIsInit(I2S0_MIX_SOURCE_NUM)) && (hf_mode_flag == FALSE))
		{
			I2SParamCt i2s_set;
			i2s_set.LineMode = Single_Mode;  //TDM 多线模式 TdmChNum 必须 >= 8，且不支持 Quad_Mode
			i2s_set.TdmChNum = TDM_CH8;    //非 TDM 模式，参数无效
			i2s_set.IsMasterMode=CFG_I2S0_MIX_MODE;// 0:master 1:slave
			i2s_set.SampleRate=CFG_I2S0_MIX_SAMPLERATE;
			i2s_set.ALIGN_MODE   = I2S_LOW_BITS_ACTIVE;
			i2s_set.I2sFormat=I2S_FORMAT_I2S;
		#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
			i2s_set.I2sBits = I2S_LENGTH_24BITS;
		#else
			i2s_set.I2sBits = I2S_LENGTH_16BITS;
		#endif
			i2s_set.I2sTxRxEnable = 2;

			i2s_set.RxPeripheralID = PERIPHERAL_1_ID_I2S0_RX;

			mainAppCt.I2S0_MIX_RX_FIFO_LEN = ((uint64_t)CFG_I2S0_MIX_SAMPLERATE * AudioCoreFrameSizeGet(I2S0MixIn_Net) * 2 / AudioCoreMixSampleRateGet(I2S0MixIn_Net)) * sizeof(PCM_DATA_TYPE) * 2;
			if (mainAppCt.I2S0_MIX_RX_FIFO == NULL)
			{
				mainAppCt.I2S0_MIX_RX_FIFO = (uint32_t*)osPortMalloc(mainAppCt.I2S0_MIX_RX_FIFO_LEN);//I2S mix rx fifo
			}
			i2s_set.RxBuf = (void*)mainAppCt.I2S0_MIX_RX_FIFO;
			i2s_set.RxLen = mainAppCt.I2S0_MIX_RX_FIFO_LEN ;//I2SIN_FIFO_LEN;
		#ifdef I2S0_MIX_MCLK_GPIO
			SET_I2S_PIN_MODE((I2S0_MIX_MCLK_GPIO), GET_I2S_GPIO_MODE(I2S0_MIX_MCLK_GPIO));//mclk
		#endif
			SET_I2S_PIN_MODE((I2S0_MIX_LRCLK_GPIO),GET_I2S_GPIO_MODE(I2S0_MIX_LRCLK_GPIO));//lrclk
			SET_I2S_PIN_MODE((I2S0_MIX_BCLK_GPIO), GET_I2S_GPIO_MODE(I2S0_MIX_BCLK_GPIO));//bclk
		#ifdef I2S0_MIX_DOUT_GPIO
			SET_I2S_PIN_MODE((I2S0_MIX_DOUT_GPIO), GET_I2S_GPIO_MODE(I2S0_MIX_DOUT_GPIO));//do
		#endif
		#ifdef I2S0_MIX_DIN_GPIO
			SET_I2S_PIN_MODE((I2S0_MIX_DIN_GPIO), GET_I2S_GPIO_MODE(I2S0_MIX_DIN_GPIO));//di
		#endif

			I2S_ModuleDisable(I2S0_MODULE);
			AudioI2S_Init(I2S0_MODULE, &i2s_set);
		#if defined(I2S0_MIX_IN_THRESHOLD_TRIGER_EN)
			void I2S0MixInThresholdInterrupt(void);
		    DMA_CircularThresholdLenSet(PERIPHERAL_1_ID_I2S0_RX, ((uint64_t)CFG_I2S0_MIX_SAMPLERATE * AudioCoreFrameSizeGet(I2S0MixIn_Net) / AudioCoreMixSampleRateGet(I2S0MixIn_Net)) * sizeof(PCM_DATA_TYPE) * 2);
			DMA_InterruptFunSet(PERIPHERAL_1_ID_I2S0_RX, DMA_THRESHOLD_INT, I2S0MixInThresholdInterrupt);
			#if defined(CFG_FUNC_EFFECT_CORE_DUAL_EN)
			if(I2S0MixIn_Net == CFG_PARA_CORE2_NET)
			{
				AudioCore.NetCt[I2S0MixIn_Net].TrigerMsgFunc = SendEffectServiceMsg;
			}
			else
			#endif
			{
				AudioCore.NetCt[I2S0MixIn_Net].TrigerMsgFunc = SendAudioCoreServiceMsg;
			}
			CoreInterruptEnable(INT_DMA1_T);
			DMA_InterruptEnable(PERIPHERAL_1_ID_I2S0_RX, DMA_THRESHOLD_INT, 1);
			NVIC_EnableIRQ(TMR4_DMAC1TINT_IRQn);
		#endif
		#ifdef CFG_FUNC_MCLK_USE_CUSTOMIZED_EN
			Clock_AudioMclkSel(AUDIO_I2S0, gCtrlVars.HwCt.I2S0Ct.CLK_Source > 2 ? (gCtrlVars.HwCt.I2S0Ct.CLK_Source - 1):gCtrlVars.HwCt.I2S0Ct.CLK_Source);
		#else
			gCtrlVars.HwCt.I2S0Ct.CLK_Source = Clock_AudioMclkGet(AUDIO_I2S0);
		#endif

		#if ((CFG_I2S0_MIX_MODE == I2S_MASTER_MODE) || !defined(CFG_FUNC_I2S0_MIX_SYNC_EN))
			{//master 或者关微调
				if(CFG_I2S0_MIX_SAMPLERATE == AudioCoreMixSampleRateGet(I2S0MixIn_Net))
					AudioIOSet.Adapt = STD;
				else
					AudioIOSet.Adapt = SRC_ONLY;
			}
		#else
			{//slave
			#ifdef CFG_FUNC_I2S_DPLL_POLICY_EN
				if(I2SDpllPolicy_ShouldBypassSra(I2S0_MODULE, TRUE))
				{
					AudioIOSet.Adapt = STD;
				}
				else
				{
					if(CFG_I2S0_MIX_SAMPLERATE == AudioCoreMixSampleRateGet(I2S0MixIn_Net))
						AudioIOSet.Adapt = SRA_ONLY;//CLK_ADJUST_ONLY;//
					else
						AudioIOSet.Adapt = SRC_SRA;//SRC_ADJUST;//
				}
			#else
				if(CFG_I2S0_MIX_SAMPLERATE == AudioCoreMixSampleRateGet(I2S0MixIn_Net))
					AudioIOSet.Adapt = SRA_ONLY;//CLK_ADJUST_ONLY;//
				else
					AudioIOSet.Adapt = SRC_SRA;//SRC_ADJUST;//
			#endif
			}
		#endif
			AudioIOSet.SrcQuality = SRC_QUALITY;
			AudioIOSet.Sync = FALSE;//
			AudioIOSet.Channels = 2;
			AudioIOSet.Net = AudioCoreGetNetBySourceChannelNUM(I2S0_MIX_SOURCE_NUM);
			AudioIOSet.Depth = AudioCoreFrameSizeGet(AudioIOSet.Net) * 2;//I2S1MixInPlayCt->I2SFIFO1 采样点深度
		//	DBG("Depth:%d", AudioIOSet.Depth);
			AudioIOSet.HighLevelCent = 60;
			AudioIOSet.LowLevelCent = 40;
			AudioIOSet.SampleRate = CFG_I2S0_MIX_SAMPLERATE;//根据实际外设选择
		//	AudioIOSet.CoreSampleRate = CFG_PARA_SAMPLE_RATE;
			AudioIOSet.DataIOFunc = AudioI2S0_DataGet ;
			AudioIOSet.LenGetFunc = AudioI2S0_DataLenGet;
			AudioIOSet.PcmWidth	= CFG_DEFAULT_WIDTH;
			if(!AudioCoreSourceInit(&AudioIOSet, I2S0_MIX_SOURCE_NUM))
			{
				DBG("I2S_MIX play source error!\n");
				return FALSE;
			}
		}
		#ifndef CFG_FUNC_I2S_DPLL_POLICY_EN
		if(hf_mode_flag == FALSE)
		{
			AudioCoreSourceEnable(I2S0_MIX_SOURCE_NUM);
			AudioCoreSourceAdjust(I2S0_MIX_SOURCE_NUM, TRUE);
		}
		#endif
	#endif

	#ifdef CFG_I2S0_MIX_OUT_EN
		uint8_t I2S0MixOut_Net = AudioCoreGetNetBySinkChannelNUM(AUDIO_I2S0_MIX_OUT_SINK_NUM);
		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
		AudioIOSet.Depth = (uint64_t)CFG_I2S0_MIX_SAMPLERATE * AudioCoreFrameSizeGet(I2S0MixOut_Net) * 3
				/ AudioCoreMixSampleRateGet(I2S0MixOut_Net);

		AudioIOSet.PcmWidth = CFG_DEFAULT_WIDTH;
		if(!AudioCoreSinkIsInit(AUDIO_I2S0_MIX_OUT_SINK_NUM))
		{
	#if (BT_LE_AUDIO_SUPPORT && !defined(ROBOEFFECT_DOUBLE_ENGINE_EN))	//双图下面不关闭硬件操作，buffer缓存到BRIDGE_MUSIC_SINK
			mainAppCt.I2S0_MIX_TX_FIFO_LEN = get_bis_sync_need_samples(AudioIOSet.Depth) * sizeof(PCM_DATA_TYPE) * 2;
	#else
			mainAppCt.I2S0_MIX_TX_FIFO_LEN = AudioIOSet.Depth * sizeof(PCM_DATA_TYPE) * 2;
	#endif			
			mainAppCt.I2S0_MIX_TX_FIFO = (uint32_t*)osPortMalloc(mainAppCt.I2S0_MIX_TX_FIFO_LEN);//I2S fifo
			if(mainAppCt.I2S0_MIX_TX_FIFO != NULL)
			{
				memset(mainAppCt.I2S0_MIX_TX_FIFO, 0, mainAppCt.I2S0_MIX_TX_FIFO_LEN);
			}
			else
			{
				APP_DBG("malloc I2S0FIFO error\n");
				return FALSE;
			}

	#if((CFG_I2S0_MIX_MODE == I2S_MASTER_MODE) || !defined(CFG_FUNC_I2S0_MIX_SYNC_EN))
			{// Master 或不开微调
				if(CFG_I2S0_MIX_SAMPLERATE == AudioCoreMixSampleRateGet(I2S0MixOut_Net))
					AudioIOSet.Adapt = STD;//SRC_ONLY
				else
					AudioIOSet.Adapt = SRC_ONLY;
			}
	#else//slave
			{
			#ifdef CFG_FUNC_I2S_DPLL_POLICY_EN
				if(I2SDpllPolicy_ShouldBypassSra(I2S0_MODULE, TRUE))
				{
					AudioIOSet.Adapt = STD;
				}
				else
				{
					if(CFG_I2S0_MIX_SAMPLERATE == AudioCoreMixSampleRateGet(I2S0MixOut_Net))
						AudioIOSet.Adapt = SRA_ONLY;//SRA_ONLY;//CLK_ADJUST_ONLY;
					else
						AudioIOSet.Adapt = SRC_SRA;//SRC_SRA;//SRC_ADJUST;
				}
			#else
				if(CFG_I2S0_MIX_SAMPLERATE == AudioCoreMixSampleRateGet(I2S0MixOut_Net))
					AudioIOSet.Adapt = SRA_ONLY;//SRA_ONLY;//CLK_ADJUST_ONLY;
				else
					AudioIOSet.Adapt = SRC_SRA;//SRC_SRA;//SRC_ADJUST;
			#endif
			}
	#endif
			AudioIOSet.SrcQuality = SRC_QUALITY;
			AudioIOSet.Sync = TRUE;//I2S slave 时候如果master没有接，有可能会导致DAC也不出声音。
			AudioIOSet.Channels = 2;
			AudioIOSet.Net = AudioCoreGetNetBySinkChannelNUM(AUDIO_I2S0_MIX_OUT_SINK_NUM);
			AudioIOSet.HighLevelCent = 60;
			AudioIOSet.LowLevelCent = 40;
			AudioIOSet.SampleRate = CFG_I2S0_MIX_SAMPLERATE;//实选
			AudioIOSet.DataIOFunc = AudioI2S0_DataSet;
			AudioIOSet.LenGetFunc = AudioI2S0_DataSpaceLenGet;
			AudioIOSet.PcmWidth	= CFG_DEFAULT_WIDTH;
			if(!AudioCoreSinkInit(&AudioIOSet, AUDIO_I2S0_MIX_OUT_SINK_NUM))
			{
				DBG("I2S0 Mix out init error\n");
				return FALSE;
			}

			AudioI2s0OutParamsSet();
			#ifndef CFG_FUNC_I2S_DPLL_POLICY_EN
			AudioCoreSinkEnable(AUDIO_I2S0_MIX_OUT_SINK_NUM);
			AudioCoreSinkAdjust(AUDIO_I2S0_MIX_OUT_SINK_NUM, TRUE);
			#endif
		}
		else//sam add,20230221
		{
			I2S_SampleRateSet(I2S0_MODULE, AudioCoreMixSampleRateGet(I2S0MixOut_Net));
			AudioCore.AudioSink[AUDIO_I2S0_MIX_OUT_SINK_NUM].PcmWidth = AudioIOSet.PcmWidth;
		#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
			AudioCore.AudioSink[AUDIO_I2S0_MIX_OUT_SINK_NUM].Sync = TRUE;
		#endif
		}
	#endif
#ifdef CFG_FUNC_I2S_DPLL_POLICY_EN
		I2SDpllPolicy_RegisterCandidate(I2S_DPLL_OWNER_I2S0_MIX, I2S0_MODULE,
				(CFG_I2S0_MIX_MODE == I2S_SLAVE_MODE), TRUE);
		I2SDpllPolicy_RegisterStreamCallback(I2S_DPLL_OWNER_I2S0_MIX,
				I2S0MixStreamsEnable, I2S0MixStreamsDisable);
		I2SDpllPolicy_RequestOwner(I2S_DPLL_OWNER_I2S0_MIX, I2S0_MODULE,
				CFG_I2S0_MIX_SAMPLERATE, (CFG_I2S0_MIX_MODE == I2S_SLAVE_MODE));
		if(!I2SDpllPolicy_ShouldDelayStreamEnable(I2S0_MODULE, (CFG_I2S0_MIX_MODE == I2S_SLAVE_MODE)))
		{
			I2S0MixStreamsEnable();
		}
#endif
		return TRUE;
	}

	void I2S0_MixDeinit(void)
	{
#ifdef CFG_FUNC_I2S_DPLL_POLICY_EN
		I2SDpllPolicy_ReleaseOwner(I2S_DPLL_OWNER_I2S0_MIX, I2S0_MODULE);
		I2SDpllPolicy_RegisterStreamCallback(I2S_DPLL_OWNER_I2S0_MIX, NULL, NULL);
#endif
	#ifdef CFG_I2S0_MIX_IN_EN
		AudioI2S_RXDeInit(I2S0_MODULE);
		if(AudioCoreSourceIsInit(I2S0_MIX_SOURCE_NUM))
		{
			AudioCoreSourceDisable(I2S0_MIX_SOURCE_NUM);
			AudioCoreSourceDeinit(I2S0_MIX_SOURCE_NUM);
		}

		if(mainAppCt.I2S0_MIX_RX_FIFO != NULL)
		{
			osPortFree(mainAppCt.I2S0_MIX_RX_FIFO);
			mainAppCt.I2S0_MIX_RX_FIFO = NULL;
		}
	#endif

	#ifdef CFG_I2S0_MIX_OUT_EN
		AudioI2S_TXDeInit(I2S0_MODULE);

		if(mainAppCt.I2S0_MIX_TX_FIFO != NULL)
		{
			APP_DBG("I2S0 MIX FIFO\n");
			osPortFree(mainAppCt.I2S0_MIX_TX_FIFO);
			mainAppCt.I2S0_MIX_TX_FIFO = NULL;
		}
		AudioCoreSinkDeinit(AUDIO_I2S0_MIX_OUT_SINK_NUM);
	#endif
	}
#endif
