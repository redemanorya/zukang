/**
 **************************************************************************************
 * @file    i2sin_mode.c
 * @brief   
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2019-1-4 17:29:47$
 *
 * @Copyright (C) 2019, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
//driver、lib
#include <string.h>
#include "type.h"
#include "irqn.h"
#include "gpio.h"
#include "dma.h"
#include "clk.h"
#include "delay.h"
#include "powercontroller.h"
#include "audio_adc.h"
#include "dac.h"
#include "adc_interface.h"
#include "dac_interface.h"
#include "i2s.h"
#include "i2s_interface.h"
#include "mcu_circular_buf.h"

//application
#include "app_config.h"
#include "debug.h"
#include "rtos_api.h"
#include "app_message.h"
#include "audio_core_api.h"
#include "audio_core_service.h"
#include "decoder.h"
#include "remind_sound.h"
#include "main_task.h"
#include "audio_effect.h"
//#include "deepsleep.h"
#include "breakpoint.h"
#include "audio_vol.h"
#include "ctrlvars.h"
#include "mode_task_api.h"
#include "i2s_dpll_policy.h"
#ifdef DUAL_CORE_EN
#include "dualcore_api.h"
#include "dualcore_dsp.h"
#endif
#include "i2sin_mode.h"

#ifdef CFG_APP_I2SIN_MODE_EN
	#define I2SIN_SOURCE_NUM				APP_SOURCE_NUM

#if defined(CFG_FUNC_I2SIN_TX_EN)
	#define I2SIN_SINK_NUM				AUDIO_APP_SINK_NUM
#endif

#if (CFG_RES_I2S_MODE == 1)
volatile uint32_t CurrentSampleRate = 0;
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

	PERIPHERAL_1_ID_I2S0_RX + 2 * CFG_RES_I2S_MODULE,
#if defined(CFG_FUNC_I2SIN_TX_EN)
	PERIPHERAL_1_ID_I2S0_TX + 2 * CFG_RES_I2S_MODULE,
#endif

	/*在此新增独有设备,确保不超*/
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,//8 CH-default
};

I2SInPlayContext* sI2SInPlayCt;

uint8_t I2SInDecoderSourceNum(void);

#ifdef CFG_FUNC_I2S_DPLL_POLICY_EN
static bool sI2SInStreamsEnabled = FALSE;
static void I2SInPlayStreamsEnable(void)
{
	if(sI2SInStreamsEnabled)
	{
		return;
	}

	AudioCoreSourceEnable(I2SIN_SOURCE_NUM);
	if(!I2SDpllPolicy_ShouldBypassSra(CFG_RES_I2S_MODULE, (CFG_RES_I2S_MODE == I2S_SLAVE_MODE)))
	{
		AudioCoreSourceAdjust(I2SIN_SOURCE_NUM, TRUE);
	}

#if defined(CFG_FUNC_I2SIN_TX_EN)
	AudioCoreSinkEnable(I2SIN_SINK_NUM);
	if(!I2SDpllPolicy_ShouldBypassSra(CFG_RES_I2S_MODULE, (CFG_RES_I2S_MODE == I2S_SLAVE_MODE)))
	{
		AudioCoreSinkAdjust(I2SIN_SINK_NUM, TRUE);
	}
#endif
	AudioCoreSourceUnmute(I2SIN_SOURCE_NUM, TRUE, TRUE);
	sI2SInStreamsEnabled = TRUE;
	APP_DBG("I2SIn stream enable: stable=%u follow=0x%lx\n",
			(uint32_t)I2SDpllPolicy_IsStable(),
			(unsigned long)I2SDpllPolicy_GetFollowMask());
}

static void I2SInPlayStreamsDisable(void)
{
	if(!sI2SInStreamsEnabled)
	{
		return;
	}

#if defined(CFG_FUNC_I2SIN_TX_EN)
	AudioCoreSinkDisable(I2SIN_SINK_NUM);
#endif
	AudioCoreSourceDisable(I2SIN_SOURCE_NUM);
	sI2SInStreamsEnabled = FALSE;
	APP_DBG("I2SIn stream disable\n");
}
#endif

void I2SInPlayResFree(void)
{
	//注意：AudioCore父任务调整到mainApp下，此处只关闭AudioCore通道，不关闭任务
#if defined(CFG_FUNC_I2SIN_TX_EN)
	AudioCoreSinkDisable(I2SIN_SINK_NUM);
	AudioCoreSinkDeinit(I2SIN_SINK_NUM);
	I2S_ModuleTxDisable(CFG_RES_I2S_MODULE);
#endif

	AudioCoreSourceDisable(I2SIN_SOURCE_NUM);
	AudioCoreSourceDeinit(I2SIN_SOURCE_NUM);

	I2S_ModuleRxDisable(CFG_RES_I2S_MODULE);
	I2S_ModuleDisable(CFG_RES_I2S_MODULE);
	DMA_ChannelDisable(PERIPHERAL_1_ID_I2S0_RX + 2 * CFG_RES_I2S_MODULE);
#if defined(CFG_FUNC_I2SIN_TX_EN)
	DMA_ChannelDisable(PERIPHERAL_1_ID_I2S0_TX + 2 * CFG_RES_I2S_MODULE);
#endif

	//PortFree

#if	0//defined(CFG_FUNC_REMIND_SOUND_EN)
	AudioCoreSourceDeinit(REMIND_SOURCE_NUM);
#endif

	if(sI2SInPlayCt->I2SFIFO1 != NULL)
	{
		osPortFree(sI2SInPlayCt->I2SFIFO1);
		sI2SInPlayCt->I2SFIFO1 = NULL;
	}


#if defined(CFG_FUNC_I2SIN_TX_EN)
	if(sI2SInPlayCt->I2STxFIFO != NULL)
	{
		osPortFree(sI2SInPlayCt->I2STxFIFO);
		sI2SInPlayCt->I2STxFIFO = NULL;
	}
#endif

	APP_DBG("I2s:Kill Ct\n");
}

bool I2SInPlayResMalloc(uint16_t SampleLen)
{
	//I2SIn  digital (DMA)
#ifdef AUDIO_STREAM_UNINTERRUPT
	sI2SInPlayCt->I2SFIFO1 = (uint32_t*)osPortMallocFromEnd(SampleLen * sizeof(PCM_DATA_TYPE) * 2 * 3);
#else
	sI2SInPlayCt->I2SFIFO1 = (uint32_t*)osPortMalloc(SampleLen * sizeof(PCM_DATA_TYPE) * 2 * 3);
#endif
	if(sI2SInPlayCt->I2SFIFO1 == NULL)
	{
		return FALSE;
	}
	memset(sI2SInPlayCt->I2SFIFO1, 0, SampleLen * sizeof(PCM_DATA_TYPE) * 2 * 3);


#if defined(CFG_FUNC_I2SIN_TX_EN)
	//I2SOut  digital (DMA)
#ifdef AUDIO_STREAM_UNINTERRUPT
	sI2SInPlayCt->I2STxFIFO = (uint32_t*)osPortMallocFromEnd(SampleLen * sizeof(PCM_DATA_TYPE) * 2 * 3);
#else
	sI2SInPlayCt->I2STxFIFO = (uint32_t*)osPortMalloc(SampleLen * sizeof(PCM_DATA_TYPE) * 2 * 3);
#endif
	if(sI2SInPlayCt->I2STxFIFO == NULL)
	{
		return FALSE;
	}
	memset(sI2SInPlayCt->I2STxFIFO, 0, SampleLen * sizeof(PCM_DATA_TYPE) * 2 * 3);
#endif
	return TRUE;
}

bool I2SInPlayResInit(void)
{
	I2SParamCt i2s_set;
	uint32_t sampleRate  = AudioCoreMixSampleRateGet(DefaultNet);

	sI2SInPlayCt->SampleRate = CFG_PARA_I2S_SAMPLERATE;//根据实际外设选择
	//Core Source1 para
	i2s_set.LineMode = Single_Mode;  //TDM 多线模式 TdmChNum 必须 >= 8，且不支持 Quad_Mode
	i2s_set.TdmChNum = TDM_CH8;    //非 TDM 模式，参数无效
	i2s_set.IsMasterMode= CFG_RES_I2S_MODE;// 0:master 1:slave
	i2s_set.SampleRate=sI2SInPlayCt->SampleRate;
	i2s_set.ALIGN_MODE   = I2S_LOW_BITS_ACTIVE;
	i2s_set.I2sFormat = I2S_FORMAT_I2S;  //I2S_FORMAT_TDM

	#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
		i2s_set.I2sBits = I2S_LENGTH_24BITS;
	#else
		i2s_set.I2sBits = I2S_LENGTH_16BITS;
	#endif
#if defined(CFG_FUNC_I2SIN_TX_EN)
	i2s_set.I2sTxRxEnable = 3;
	i2s_set.TxPeripheralID = PERIPHERAL_1_ID_I2S0_TX + 2 * CFG_RES_I2S_MODULE;
	i2s_set.TxBuf=sI2SInPlayCt->I2STxFIFO;
	i2s_set.TxLen=AudioCoreFrameSizeGet(DefaultNet) * sizeof(PCM_DATA_TYPE) * 2 * 3 ;//I2SIN_FIFO_LEN;
#else
	i2s_set.I2sTxRxEnable = 2;
#endif
	i2s_set.RxPeripheralID = PERIPHERAL_1_ID_I2S0_RX + 2 * CFG_RES_I2S_MODULE;

	i2s_set.RxBuf=sI2SInPlayCt->I2SFIFO1;
	i2s_set.RxLen=AudioCoreFrameSizeGet(DefaultNet) * sizeof(PCM_DATA_TYPE) * 2 * 3 ;//I2SIN_FIFO_LEN;


#ifdef	I2S_MCLK_GPIO
	SET_I2S_PIN_MODE((I2S_MCLK_GPIO), GET_I2S_GPIO_MODE(I2S_MCLK_GPIO));//mclk
#endif
	SET_I2S_PIN_MODE((I2S_LRCLK_GPIO),GET_I2S_GPIO_MODE(I2S_LRCLK_GPIO));//lrclk
	SET_I2S_PIN_MODE((I2S_BCLK_GPIO), GET_I2S_GPIO_MODE(I2S_BCLK_GPIO));//bclk
#if defined(CFG_FUNC_I2SIN_TX_EN)
	SET_I2S_PIN_MODE((I2S_DOUT_GPIO), GET_I2S_GPIO_MODE(I2S_DOUT_GPIO));//do
#endif
#ifdef I2S_DIN_GPIO
		SET_I2S_PIN_MODE((I2S_DIN_GPIO), GET_I2S_GPIO_MODE(I2S_DIN_GPIO));//di
#endif

	I2S_ModuleDisable(CFG_RES_I2S_MODULE);
	AudioI2S_Init(CFG_RES_I2S_MODULE, &i2s_set);

#ifdef CFG_FUNC_MCLK_USE_CUSTOMIZED_EN
	#if	(CFG_RES_I2S_MODULE == 0)
		Clock_AudioMclkSel(AUDIO_I2S0, gCtrlVars.HwCt.I2S0Ct.CLK_Source > 2 ? (gCtrlVars.HwCt.I2S0Ct.CLK_Source - 1):gCtrlVars.HwCt.I2S0Ct.CLK_Source);
	#elif (CFG_RES_I2S_MODULE == 1)
		Clock_AudioMclkSel(AUDIO_I2S1, gCtrlVars.HwCt.I2S1Ct.CLK_Source > 2 ? (gCtrlVars.HwCt.I2S1Ct.CLK_Source - 1):gCtrlVars.HwCt.I2S1Ct.CLK_Source);
	#else
		Clock_AudioMclkSel(AUDIO_I2S2, gCtrlVars.HwCt.I2S2Ct.CLK_Source > 2 ? (gCtrlVars.HwCt.I2S2Ct.CLK_Source - 1):gCtrlVars.HwCt.I2S2Ct.CLK_Source);

	#endif
#else
	#if(CFG_RES_I2S_MODULE == 0)
		gCtrlVars.HwCt.I2S0Ct.CLK_Source = Clock_AudioMclkGet(AUDIO_I2S0);
	#elif (CFG_RES_I2S_MODULE == 1)
		gCtrlVars.HwCt.I2S1Ct.CLK_Source = Clock_AudioMclkGet(AUDIO_I2S1);
	#else
		gCtrlVars.HwCt.I2S2Ct.CLK_Source = Clock_AudioMclkGet(AUDIO_I2S2);
	#endif
#endif

#ifdef CFG_I2S_SLAVE_TO_SPDIFOUT_EN
#if CFG_RES_I2S_MODE == 1
	I2S_SampleRateCheckInterruptClr(CFG_RES_I2S_MODULE);
	I2S_SampleRateCheckInterruptEnable(CFG_RES_I2S_MODULE);
#endif
#endif
//	//note Soure0.和sink0已经在main app中配置，不要随意配置
	//Core Soure1.Para
	AudioCoreIO	AudioIOSet;
	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));

	{
#if ((CFG_RES_I2S_MODE == I2S_MASTER_MODE) || !defined(CFG_FUNC_I2S_IN_SYNC_EN))
	{//master 或者关微调
		if(CFG_PARA_I2S_SAMPLERATE == sampleRate)
			AudioIOSet.Adapt = STD;
		else
			AudioIOSet.Adapt = SRC_ONLY;
	}
#else
	{//slave
	#ifdef CFG_FUNC_I2S_DPLL_POLICY_EN
		bool BypassSra = I2SDpllPolicy_ShouldBypassSra(CFG_RES_I2S_MODULE, TRUE);
		if(BypassSra)
		{
			AudioIOSet.Adapt = STD;
		}
		else
		{
			AudioIOSet.Adapt = SRC_ADJUST;//SRC_ADJUST for slave in samplerate change
		}
		APP_DBG("I2SIn src adapt: module=%u slave=1 bypass=%u stable=%u adapt=%u fs_cfg=%lu fs_mix=%lu\n",
				(uint32_t)CFG_RES_I2S_MODULE,
				(uint32_t)BypassSra,
				(uint32_t)I2SDpllPolicy_IsStable(),
				(uint32_t)AudioIOSet.Adapt,
				(unsigned long)CFG_PARA_I2S_SAMPLERATE,
				(unsigned long)sampleRate);
	#else
//		if(CFG_PARA_I2S_SAMPLERATE == sampleRate)
//			AudioIOSet.Adapt = STD;//SRA_ONLY;//CLK_ADJUST_ONLY;//
//		else
//			AudioIOSet.Adapt = SRC_SRA;//SRC_ADJUST;//
	#ifdef CFG_I2S_SLAVE_TO_SPDIFOUT_EN
		AudioIOSet.Adapt = STD;
	#else
		AudioIOSet.Adapt = SRC_ADJUST;//SRC_ADJUST for slave in samplerate change
	#endif
	#endif
	}
#endif
		AudioIOSet.SrcQuality = SRC_QUALITY;
		AudioIOSet.Sync = TRUE;//FALSE;//
		AudioIOSet.Channels = 2;
		AudioIOSet.Net = AudioCoreGetNetBySourceChannelNUM(I2SIN_SOURCE_NUM);
		AudioIOSet.Depth = AudioCoreFrameSizeGet(DefaultNet) * 3;//sI2SInPlayCt->I2SFIFO1 采样点深度
	//	DBG("Depth:%d", AudioIOSet.Depth);
		AudioIOSet.HighLevelCent = 60;
		AudioIOSet.LowLevelCent = 40;
		AudioIOSet.SampleRate = CFG_PARA_I2S_SAMPLERATE;//根据实际外设选择
	//	AudioIOSet.CoreSampleRate = CFG_PARA_SAMPLE_RATE;
#if (CFG_RES_I2S_MODULE == 0)
		AudioIOSet.DataIOFunc = AudioI2S0_DataGet ;
		AudioIOSet.LenGetFunc = AudioI2S0_DataLenGet;
#elif (CFG_RES_I2S_MODULE == 1)
		AudioIOSet.DataIOFunc = AudioI2S1_DataGet ;
		AudioIOSet.LenGetFunc = AudioI2S1_DataLenGet;
#else
		AudioIOSet.DataIOFunc = AudioI2S2_DataGet ;
		AudioIOSet.LenGetFunc = AudioI2S2_DataLenGet;
#endif

		AudioIOSet.PcmWidth	= CFG_DEFAULT_WIDTH;

		if(!AudioCoreSourceInit(&AudioIOSet, I2SIN_SOURCE_NUM))
		{
			DBG("I2Splay source error!\n");
			return FALSE;
		}
	}

#if defined(CFG_FUNC_I2SIN_TX_EN)
	#if ((CFG_RES_I2S_MODE == I2S_MASTER_MODE) || !defined(CFG_FUNC_I2S_OUT_SYNC_EN))
	{//master 或者关微调
		if(CFG_PARA_I2S_SAMPLERATE == sampleRate)
			AudioIOSet.Adapt = STD;
		else
			AudioIOSet.Adapt = SRC_ONLY;
	}
	#else
	{//slave
	#ifdef CFG_FUNC_I2S_DPLL_POLICY_EN
		bool BypassSra = I2SDpllPolicy_ShouldBypassSra(CFG_RES_I2S_MODULE, TRUE);
		if(BypassSra)
		{
			AudioIOSet.Adapt = STD;
		}
		#ifdef CFG_I2S_SLAVE_TO_SPDIFOUT_EN
			else
			AudioIOSet.Adapt = STD;
		#else
			else
			AudioIOSet.Adapt = SRC_ADJUST;//SRC_ADJUST for slave in samplerate change
		#endif
		APP_DBG("I2SIn sink adapt: module=%u slave=1 bypass=%u stable=%u adapt=%u fs_cfg=%lu fs_mix=%lu\n",
				(uint32_t)CFG_RES_I2S_MODULE,
				(uint32_t)BypassSra,
				(uint32_t)I2SDpllPolicy_IsStable(),
				(uint32_t)AudioIOSet.Adapt,
				(unsigned long)CFG_PARA_I2S_SAMPLERATE,
				(unsigned long)sampleRate);
	#else
//		if(CFG_PARA_I2S_SAMPLERATE == sampleRate)
//			AudioIOSet.Adapt = STD;//SRA_ONLY;//CLK_ADJUST_ONLY;//
//		else
//			AudioIOSet.Adapt = SRC_SRA;//SRC_ADJUST;//
		#ifdef CFG_I2S_SLAVE_TO_SPDIFOUT_EN
			AudioIOSet.Adapt = STD;
		#else
			AudioIOSet.Adapt = SRC_ADJUST;//SRC_ADJUST for slave in samplerate change
		#endif
	#endif
	}
	#endif
		AudioIOSet.SrcQuality = SRC_QUALITY;
		AudioIOSet.Sync = TRUE;//FALSE;//
		AudioIOSet.Channels = 2;
		AudioIOSet.Net = AudioCoreGetNetBySinkChannelNUM(I2SIN_SINK_NUM);
		AudioIOSet.Depth = AudioCoreFrameSizeGet(DefaultNet) * 3;//sI2SInPlayCt->I2STxFIFO 采样点深度
	//	DBG("Depth:%d", AudioIOSet.Depth);
		AudioIOSet.HighLevelCent = 60;
		AudioIOSet.LowLevelCent = 40;
		AudioIOSet.SampleRate = CFG_PARA_I2S_SAMPLERATE;//根据实际外设选择

#if (CFG_RES_I2S_MODULE == 0)
		AudioIOSet.DataIOFunc = AudioI2S0_DataSet ;
		AudioIOSet.LenGetFunc = AudioI2S0_DataSpaceLenGet;
#elif (CFG_RES_I2S_MODULE == 1)
		AudioIOSet.DataIOFunc = AudioI2S1_DataSet ;
		AudioIOSet.LenGetFunc = AudioI2S1_DataSpaceLenGet;
#else
		AudioIOSet.DataIOFunc = AudioI2S2_DataSet ;
		AudioIOSet.LenGetFunc = AudioI2S2_DataSpaceLenGet;
#endif

		AudioIOSet.PcmWidth	= CFG_DEFAULT_WIDTH;

		if(!AudioCoreSinkInit(&AudioIOSet, I2SIN_SINK_NUM))
		{
			DBG("I2Splay sink error!\n");
			return FALSE;
		}
#endif//#if defined(CFG_FUNC_I2SIN_TX_EN)
#ifdef AUDIO_STREAM_UNINTERRUPT
	if(IsResRamReallocate == TRUE)
#endif
	{
	//Core Process
#ifdef CFG_FUNC_ROBOEFFECT_EN
	AudioCoreProcessConfig((AudioCoreProcessFunc)AudioMusicProcess);
#else
	AudioCoreProcessConfig((AudioCoreProcessFunc)AudioBypassProcess);
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

/**
 * @func        I2SInPlay_Init
 * @brief       I2SIn模式参数配置，资源初始化
 * @param       MessageHandle 
 * @Output      None
 * @return      bool
 * @Others      任务块、I2S、Dac、AudioCore配置
 * @Others      数据流从I2S到audiocore配有函数指针，audioCore到Dac同理，由audiocoreService任务按需驱动
 * Record
 */
bool  I2SInPlayInit(void)
{
	bool ret;

	APP_DBG("I2SIn init\n");
	DMA_ChannelAllocTableSet(DMA0, (uint8_t* )Dma0ChannelMap);
	DMA_ChannelAllocTableSet(DMA1, (uint8_t* )Dma1ChannelMap);
#ifdef AUDIO_STREAM_UNINTERRUPT
	sI2SInPlayCt = (I2SInPlayContext*)osPortMallocFromEnd(sizeof(I2SInPlayContext));
#else
	sI2SInPlayCt = (I2SInPlayContext*)osPortMalloc(sizeof(I2SInPlayContext));
#endif
	if(sI2SInPlayCt == NULL)
	{
		return FALSE;
	}
	memset(sI2SInPlayCt, 0, sizeof(I2SInPlayContext));

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
	if(!I2SInPlayResMalloc(AudioCoreFrameSizeGet(DefaultNet)))
	{
		APP_DBG("I2SInPlay Res Error!\n");
		return FALSE;
	}

	ret = I2SInPlayResInit();
#ifdef CFG_FUNC_I2S_DPLL_POLICY_EN
	sI2SInStreamsEnabled = FALSE;
	I2SDpllPolicy_RegisterCandidate(I2S_DPLL_OWNER_I2SIN_MODE, CFG_RES_I2S_MODULE,
			(CFG_RES_I2S_MODE == I2S_SLAVE_MODE), TRUE);
	I2SDpllPolicy_RegisterStreamCallback(I2S_DPLL_OWNER_I2SIN_MODE,
			I2SInPlayStreamsEnable, I2SInPlayStreamsDisable);
	I2SDpllPolicy_RequestOwner(I2S_DPLL_OWNER_I2SIN_MODE, CFG_RES_I2S_MODULE,
			sI2SInPlayCt->SampleRate, (CFG_RES_I2S_MODE == I2S_SLAVE_MODE));
	APP_DBG("I2SIn init policy: module=%u slave=%u stable=%u follow=0x%lx fs=%lu\n",
			(uint32_t)CFG_RES_I2S_MODULE,
			(uint32_t)(CFG_RES_I2S_MODE == I2S_SLAVE_MODE),
			(uint32_t)I2SDpllPolicy_IsStable(),
			(unsigned long)I2SDpllPolicy_GetFollowMask(),
			(unsigned long)sI2SInPlayCt->SampleRate);
	if(I2SDpllPolicy_ShouldDelayStreamEnable(CFG_RES_I2S_MODULE, (CFG_RES_I2S_MODE == I2S_SLAVE_MODE)))
	{
		APP_DBG("I2SIn stream defer: module=%u stable=%u\n",
				(uint32_t)CFG_RES_I2S_MODULE,
				(uint32_t)I2SDpllPolicy_IsStable());
	}
	else
	{
		I2SInPlayStreamsEnable();
	}
#else
	AudioCoreSourceEnable(I2SIN_SOURCE_NUM);
	AudioCoreSourceAdjust(I2SIN_SOURCE_NUM, TRUE);

#if defined(CFG_FUNC_I2SIN_TX_EN)
	AudioCoreSinkEnable(I2SIN_SINK_NUM);
	AudioCoreSinkAdjust(I2SIN_SINK_NUM, TRUE);
#endif
#endif
	AudioCodecHardwareSync();//update hardware config

#ifdef CFG_FUNC_REMIND_SOUND_EN
	if(!SoftFlagGet(SoftFlagSysCurModeReboot) && RemindSoundServiceItemRequest(SOUND_REMIND_I2SMODE, REMIND_PRIO_NORMAL) == FALSE)
#endif
	{
		if(IsAudioPlayerMute() == TRUE)
		{
			HardWareMuteOrUnMute();
		}
	}

#ifndef CFG_FUNC_I2S_DPLL_POLICY_EN
	AudioCoreSourceUnmute(I2SIN_SOURCE_NUM, TRUE, TRUE);
#endif
	return ret;
}
/**
 * @func        I2sInPlayEntrance
 * @brief       模式执行主体
 * @param       void * param  
 * @Output      None
 * @return      None
 * @Others      模式建立和结束过程
 * Record
 */
void I2SInPlayRun(uint16_t msgId)
{
#if (CFG_RES_I2S_MODE == 1)
	extern void AudioSpdifOut_SampleRateChange(uint32_t SampleRate);
	if (I2S_SampleRateCheckInterruptGet(CFG_RES_I2S_MODULE) && (gCtrlVars.HwCt.I2S0Ct.CLK_Source < 2))
	{
#if defined (CFG_RES_AUDIO_SPDIFOUT_EN) && defined (CFG_I2S_SLAVE_TO_SPDIFOUT_EN)
		{
			Clock_PllLock(225792);
		}//Add the above actions to make I2S_SampleRateGet right
#endif
		CurrentSampleRate = I2S_SampleRateGet(CFG_RES_I2S_MODULE);
//		AudioI2S_SampleRateChange(CFG_RES_I2S_MODULE, CurrentSampleRate);
		APP_DBG("I2SIn samplerate change to:%ld\n", CurrentSampleRate);
#if defined (CFG_RES_AUDIO_SPDIFOUT_EN) && defined (CFG_I2S_SLAVE_TO_SPDIFOUT_EN)
		AudioSpdifOut_SampleRateChange(CurrentSampleRate);
		SyncModule_Reset();

		extern bool AudioEffectModeSel(EFFECT_MODE effectMode, uint8_t sel);
		AudioEffectModeSel(mainAppCt.EffectMode, 1);
#elif defined (CFG_AUDIO_OUT_AUTO_SAMPLE_RATE_44100_48000)
		AudioOutSampleRateSet(CurrentSampleRate);
#endif
		AudioI2S_SampleRateChange(CFG_RES_I2S_MODULE, CurrentSampleRate);
		DelayMs(1);

#ifdef CFG_I2SIN_TDM_MODE_EN
		AudioCoreSourceChange(I2STDM0_SOURCE_NUM, 0, CurrentSampleRate);
		AudioCoreSourceChange(I2STDM1_SOURCE_NUM, 0, CurrentSampleRate);
#else
		AudioCoreSourceChange(I2SIN_SOURCE_NUM, 0, CurrentSampleRate);
#endif
//#ifdef CFG_RES_AUDIO_I2SOUT_EN
//		AudioCoreSinkChange(AUDIO_I2SOUT_SINK_NUM, 0, CurrentSampleRate);
//#endif

		I2S_SampleRateCheckInterruptClr(CFG_RES_I2S_MODULE);
	}
#endif


	switch(msgId)//警告：在此段代码，禁止新增提示音插播位置。
	{
/*	case MSG_REMIND_SOUND_PLAY_START:
			break;

		case MSG_REMIND_SOUND_PLAY_DONE://提示音播放结束
		case MSG_REMIND_SOUND_PLAY_REQUEST_FAIL:
			//AudioCoreSourceUnmute(APP_SOURCE_NUM, TRUE, TRUE);
			break;
*/
		default:
			CommonMsgProccess(msgId);
			break;
	}
}

bool I2SInPlayDeinit(void)
{
	if(sI2SInPlayCt == NULL)
	{
		return TRUE;
	}
	APP_DBG("I2SIn Play deinit\n");
#ifdef AUDIO_STREAM_UNINTERRUPT
	if(IsResRamRelease == TRUE)
#endif
	{	
		if(IsAudioPlayerMute() == FALSE)
		{
			HardWareMuteOrUnMute();
		}	
	}
#ifdef CFG_FUNC_I2S_DPLL_POLICY_EN
	I2SDpllPolicy_ReleaseOwner(I2S_DPLL_OWNER_I2SIN_MODE, CFG_RES_I2S_MODULE);
	I2SDpllPolicy_RegisterStreamCallback(I2S_DPLL_OWNER_I2SIN_MODE, NULL, NULL);
#endif
	//Kill used services
#if	0//defined(CFG_FUNC_REMIND_SOUND_EN)
	AudioCoreSourceDisable(REMIND_SOURCE_NUM);
	AudioCoreSourceDisable(PLAYBACK_SOURCE_NUM);
	DecoderServiceDeinit(DECODER_REMIND_CHANNEL);
	while(GetDecoderServiceState()!=TaskStateStopped)
	{
		APP_DBG("I2S IN:%d\n",GetDecoderServiceState());
		osTaskDelay(1);
	}
#endif
#ifdef I2S_MCLK_GPIO
	SET_I2S_PIN_MODE((I2S_MCLK_GPIO), 0);//mclk
#endif
	SET_I2S_PIN_MODE((I2S_LRCLK_GPIO),0);//lrclk
	SET_I2S_PIN_MODE((I2S_BCLK_GPIO), 0);//bclk
#ifdef I2S_DIN_GPIO
	SET_I2S_PIN_MODE((I2S_DIN_GPIO), 0);//di
#endif
#if defined(CFG_FUNC_I2SIN_TX_EN)
	SET_I2S_PIN_MODE((I2S_DOUT_GPIO), 0);//dout
	AudioCoreSinkDisable(I2SIN_SINK_NUM);
#endif
	AudioCoreSourceDisable(I2SIN_SOURCE_NUM);
#ifdef AUDIO_STREAM_UNINTERRUPT
	if(IsResRamRelease == TRUE)
#endif
	{
		PauseAudioCore();
		AudioCoreProcessConfig((void*)AudioNoAppProcess);
	}
	
	I2SInPlayResFree();
#ifdef AUDIO_STREAM_UNINTERRUPT
	if(IsResRamRelease == TRUE)
#endif
	{
		ModeCommonDeinit();//通路全部释放
	}

	osPortFree(sI2SInPlayCt);
	sI2SInPlayCt = NULL;

	return TRUE;
}

#endif//#ifdef CFG_APP_I2SIN_MODE_EN
