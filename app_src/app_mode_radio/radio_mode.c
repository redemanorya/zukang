/**
 **************************************************************************************
 * @file    Radio_mode.c
 * @brief
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2017-12-26 13:06:47$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include <string.h>
#include "type.h"
#include "irqn.h"
#include "gpio.h"
#include "dma.h"
#include "rtos_api.h"
#include "app_message.h"
#include "app_config.h"
#include "debug.h"
#include "delay.h"
#include "audio_adc.h"
#include "dac.h"
#include "adc_interface.h"
#include "dac_interface.h"
#include "audio_effect.h"
#include "radio_api.h"
#ifdef FUNC_RADIO_RDA5807_EN
#include "RDA5807.h"
#endif
#ifdef FUNC_RADIO_QN8035_EN
  #include "QN8035.h"
#endif
#include "audio_core_api.h"
#include "audio_core_service.h"
#include "powercontroller.h"
//#include "deepsleep.h"
#include "breakpoint.h"
//#include "decoder.h"
#include "remind_sound_item.h"
#include "remind_sound.h"
#include "audio_vol.h"
#include "clk.h"
#include "ctrlvars.h"
#include "main_task.h"
#include "reset.h"
#include "mode_task_api.h"
#ifdef DUAL_CORE_EN
#include "dualcore_api.h"
#include "dualcore_dsp.h"
#endif

#ifdef CFG_APP_RADIOIN_MODE_EN

#define RADIO_PLAY_TASK_STACK_SIZE		512//1024
#define RADIO_PLAY_TASK_PRIO			3
#define RADIO_NUM_MESSAGE_QUEUE			10

#define RADIO_SOURCE_NUM				APP_SOURCE_NUM

typedef struct _RadioPlayContext
{
	AUDIOADC_ParamCt 	HwCt;

//	uint32_t			*ADCFIFO;			//ADC的DMA循环fifo
//	uint32_t			ADCFIFO_len;		//ADC的DMA循环fifo len

}RadioPlayContext;

/****************************************
 * DMA CTRL 仅8通道，超出无效：
 * 常驻设备排前部,所有mode序号一致
 * *************************************/
static const uint8_t Dma0ChannelMap[16] =
{
	MAP0_COMMON_FIRST
	/*在此新增独有设备,确保不超*/
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,//8 CH-default
};

static const uint8_t Dma1ChannelMap[16] =
{
	MAP1_COMMON_FIRST

	RADIO_DMA_PERI_ID_SEL,
	/*在此新增独有设备,确保不超*/
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,//8 CH-default
};


static RadioPlayContext			*sRadioPlayCt = NULL;
RADIO_CONTROL 					*gRadioControl = NULL;

void RadioMute(bool mute)
{
#ifdef FUNC_RADIO_RDA5807_EN
	RDA5807Mute(mute);
#endif
#ifdef FUNC_RADIO_QN8035_EN
	QN8035Mute(mute);
#endif
}

void RadioSetFreq(uint16_t freq)
{
#ifdef FUNC_RADIO_RDA5807_EN
	RDA5807SetFreq(freq);
#endif
#ifdef FUNC_RADIO_QN8035_EN
	QN8035SetFreq(freq);
#endif
}

bool RadioPowerOn(void)
{
#ifdef FUNC_RADIO_RDA5807_EN
	return RDA5807PowerOn();
#endif
#ifdef FUNC_RADIO_QN8035_EN
	return QN8035PowerOn();
#endif
	return 0;
}

void RadioInit(void)
{
#ifdef FUNC_RADIO_RDA5807_EN
	RDA5807Init();
#endif
#ifdef FUNC_RADIO_QN8035_EN
	QN8035Init();
#endif
}

void RadioPowerDown(void)
{
#ifdef FUNC_RADIO_RDA5807_EN
	RDA5807PowerDown();
#endif
#ifdef FUNC_RADIO_QN8035_EN
	QN8035PowerDown();
#endif
}

uint8_t RadioSearchRead(uint16_t freq)
{
#ifdef FUNC_RADIO_RDA5807_EN
	return RDA5807SearchRead(freq);
#endif
#ifdef FUNC_RADIO_QN8035_EN
	return QN8035SearchRead(freq);
#endif
	return 0;
}


static uint16_t RadioGetBaseFreq(void)
{
	switch(gRadioControl->CurFreqArea)
	{
		case FM_AREA_CHINA:
			return CHINA_BASE_FREQ;
		case FM_AREA_JAPAN:
			return JAPAN_BASE_FREQ;
		case FM_AREA_RUSSIA:
			return RUSSIA_BASE_FREQ;
		default:
			return OTHER_BASE_FREQ;
	}
}

#ifdef CFG_FUNC_BREAKPOINT_EN
void RadioGetBreakPoint(void)
{
	BP_RADIO_INFO *pBpRadioInfo;

	pBpRadioInfo = (BP_RADIO_INFO *)BP_GetInfo(BP_RADIO_INFO_TYPE);

	gRadioControl->CurFreqArea = BP_GET_ELEMENT(pBpRadioInfo->CurBandIdx);
	gRadioControl->Freq = BP_GET_ELEMENT(pBpRadioInfo->CurFreq);
	gRadioControl->ChlCount = BP_GET_ELEMENT(pBpRadioInfo->StationCount);
	if(gRadioControl->ChlCount > 0)
	{
		uint32_t i;
		for(i = 0; i < gRadioControl->ChlCount; i++)
		{
			gRadioControl->Channel[i] = (uint16_t)(BP_GET_ELEMENT(pBpRadioInfo->StationList[i]));
			if((gRadioControl->Channel[i] + RadioGetBaseFreq()) == gRadioControl->Freq)
			{
				gRadioControl->CurStaIdx = (uint8_t)i;
			}
		}
	}
}
#endif

void RadioPlay(void)
{
	if(gRadioControl->State == RADIO_STATUS_SEEKING || gRadioControl->State == RADIO_STATUS_PREVIEW)
	{
		RadioMute(TRUE);
		gRadioControl->State = RADIO_STATUS_IDLE;
	}

	RadioMute(TRUE);
	RadioSetFreq(gRadioControl->Freq);
	APP_DBG("play Freq = %d\n", gRadioControl->Freq);
	RadioMute(FALSE);
	gRadioControl->State = RADIO_STATUS_PLAYING;
}

void RadioSeekEnd(void)
{
	if(gRadioControl->ChlCount > 0)
	{
		gRadioControl->Freq = gRadioControl->Channel[gRadioControl->CurStaIdx] + RadioGetBaseFreq();
	}
	else
	{
		gRadioControl->Freq = gRadioControl->MinFreq;
	}
	APP_DBG("ChlCount = %d\n", gRadioControl->ChlCount);
	APP_DBG("Freq = %d\n", gRadioControl->Freq);
	RadioPlay();
#ifdef CFG_FUNC_BREAKPOINT_EN
	BackupInfoUpdata(BACKUP_RADIO_INFO);
#endif
}

bool RadioChannelAdd(void)
{
	if(gRadioControl->ChlCount == MAX_RADIO_CHANNEL_NUM)
	{
		return FALSE;
	}
	gRadioControl->Channel[gRadioControl->ChlCount++] = gRadioControl->Freq - RadioGetBaseFreq();

#ifdef CFG_FUNC_BREAKPOINT_EN
	//RadioPlayBPUpdata();
	BackupInfoUpdata(BACKUP_RADIO_INFO);
#endif

	return TRUE;
}

void RadioSeekStart(void)
{
	RadioMute(TRUE);

	if(!(RADIO_STATUS_SEEKING == gRadioControl->State || RADIO_STATUS_PREVIEW == gRadioControl->State))
	{
		gRadioControl->Freq = gRadioControl->MinFreq;
		gRadioControl->State = RADIO_STATUS_SEEKING;
		gRadioControl->ChlCount = 0;
		gRadioControl->CurStaIdx = 0;
		memset(gRadioControl->Channel, 0, sizeof(gRadioControl->Channel));
	}
	RadioSetFreq(gRadioControl->Freq);

	//TimeOutSet(&gRadioControl->TimerHandle, 50);
}

void RadioTimerCB(void* unused)
{
	switch(gRadioControl->State)
	{
		case RADIO_STATUS_PLAYING:
			break;

		case RADIO_STATUS_SEEKING_DOWN:
			if(RadioSearchRead(gRadioControl->Freq))
			{
				RadioPlay();
			}
			else
			{
				if(gRadioControl->Freq > gRadioControl->MinFreq)
				 {
				   gRadioControl->Freq--;
				   RadioSetFreq(gRadioControl->Freq);
				 }
				else
				 RadioPlay();
			}
			break;

		case RADIO_STATUS_SEEKING_UP:
			if(RadioSearchRead(gRadioControl->Freq))
			{
				RadioPlay();
			}
			else
			{
				if(gRadioControl->Freq < gRadioControl->MaxFreq)
				 {
				   gRadioControl->Freq++;
				   RadioSetFreq(gRadioControl->Freq);
				 }
				else
				 RadioPlay();
			}
			break;

		case RADIO_STATUS_SEEKING:
			if(RadioSearchRead(gRadioControl->Freq) && (gRadioControl->Freq < gRadioControl->MaxFreq))//
			{
				if(RadioChannelAdd())
				{
					if(gRadioControl->Freq >= gRadioControl->MaxFreq)//
					{
						RadioSeekEnd();
						break;
					}
					else
					{
						gRadioControl->Freq++;
						RadioSeekStart();
						//APP_DBG("gRadioControl->Freq = %d \n",gRadioControl->Freq);
					}
				}
				else
				{
					RadioSeekEnd();
				}
			}
			else
			{
				if(gRadioControl->Freq >= gRadioControl->MaxFreq)//
				{
					RadioSeekEnd();
					break;
				}
				else
				{
					gRadioControl->Freq++;
					RadioSeekStart();
				}
			}
			break;

		case RADIO_STATUS_PREVIEW: //

			break;

		case RADIO_STATUS_IDLE:
		default:
			break;
	}
}

bool RadioSwitchChannel(uint32_t Msg)
{
	if(gRadioControl->State == RADIO_STATUS_SEEKING)
	{
		return FALSE;
	}
	if(gRadioControl->ChlCount == 0)
	{
		return FALSE;
	}

	if((Msg == MSG_RADIO_PLAY_NEXT) ||(Msg == MSG_NEXT))
	{
		gRadioControl->CurStaIdx++;
		gRadioControl->CurStaIdx %= gRadioControl->ChlCount;
	}
	else
	{
		if(gRadioControl->CurStaIdx > 0)
		{
			gRadioControl->CurStaIdx--;
		}
		else
		{
			gRadioControl->CurStaIdx = (uint8_t)(gRadioControl->ChlCount - 1);
		}
	}

	gRadioControl->Freq = gRadioControl->Channel[gRadioControl->CurStaIdx] + RadioGetBaseFreq();
	RadioPlay();

#ifdef CFG_FUNC_BREAKPOINT_EN
	//RadioPlayBPUpdata();
	BackupInfoUpdata(BACKUP_RADIO_INFO);
#endif

	return TRUE;
}

void RadioPlayResFree(void)
{
	if(sRadioPlayCt->HwCt.ADCFifo != NULL)
	{
		APP_DBG("ADCFIFO\n");
		osPortFree(sRadioPlayCt->HwCt.ADCFifo);
		sRadioPlayCt->HwCt.ADCFifo = NULL;
	}

	if(gRadioControl != NULL)
	{
		osPortFree(gRadioControl);
		gRadioControl = NULL;
	}
}

bool RadioPlayResMalloc(uint16_t SampleLen)
{
#ifdef AUDIO_STREAM_UNINTERRUPT
	sRadioPlayCt = (RadioPlayContext*)osPortMallocFromEnd(sizeof(RadioPlayContext));
#else
	sRadioPlayCt = (RadioPlayContext*)osPortMalloc(sizeof(RadioPlayContext));
#endif

	if(sRadioPlayCt == NULL)
	{
		return FALSE;
	}
	memset(sRadioPlayCt, 0, sizeof(RadioPlayContext));
#ifdef AUDIO_STREAM_UNINTERRUPT
	gRadioControl = (RADIO_CONTROL *)osPortMallocFromEnd(sizeof(RADIO_CONTROL));
#else
	gRadioControl = (RADIO_CONTROL *)osPortMalloc(sizeof(RADIO_CONTROL));
#endif
	if(gRadioControl == NULL)
	{
		return FALSE;
	}
	memset(gRadioControl, 0, sizeof(RADIO_CONTROL));

	sRadioPlayCt->HwCt.ADCFifoLen = SampleLen * sizeof(PCM_DATA_TYPE) * 2 * 2;
#ifdef AUDIO_STREAM_UNINTERRUPT
	sRadioPlayCt->HwCt.ADCFifo = (uint32_t*)osPortMallocFromEnd(sRadioPlayCt->HwCt.ADCFifoLen);
#else
	sRadioPlayCt->HwCt.ADCFifo = (uint32_t*)osPortMalloc(sRadioPlayCt->HwCt.ADCFifoLen);
#endif
	if(sRadioPlayCt->HwCt.ADCFifo == NULL)
	{
		return FALSE;
	}
	memset(sRadioPlayCt->HwCt.ADCFifo, 0, sRadioPlayCt->HwCt.ADCFifoLen);

	return TRUE;
}

void RadioPlayResInit(void)
{
	//Soure1.
	AudioCoreIO	AudioIOSet;
	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
	AudioIOSet.Adapt = STD;
	AudioIOSet.Sync = TRUE;
	AudioIOSet.Channels = 2;
	AudioIOSet.Net = AudioCoreGetNetBySourceChannelNUM(RADIO_SOURCE_NUM);
	AudioIOSet.DataIOFunc = RADIO_DATAGET_SEL;
	AudioIOSet.LenGetFunc = RADIO_LENGET_SEL;
	AudioIOSet.PcmWidth	= CFG_DEFAULT_WIDTH;
	AudioIOSet.SampleRate = AudioCoreMixSampleRateGet(AudioCoreGetNetBySourceChannelNUM(RADIO_SOURCE_NUM));

	if(!AudioCoreSourceInit(&AudioIOSet, RADIO_SOURCE_NUM))
	{
		DBG("Radioplay source error!\n");
	}

#if (CFG_RES_MIC_SELECT) && defined(CFG_FUNC_MIC_EN)
	sRadioPlayCt->HwCt.MICBias_EN					= CFG_MICBIAS_EN_VAL,
#endif
	sRadioPlayCt->HwCt.AUDIOADC_AnaLeft.INPUT = RADIO_LEFT_SEL;
	sRadioPlayCt->HwCt.AUDIOADC_AnaLeft.Mode = ADC_Linein;
	sRadioPlayCt->HwCt.AUDIOADC_AnaLeft.PGAGain = 31 - RADIO_LEFT_GAIN_VAR;
	sRadioPlayCt->HwCt.AUDIOADC_AnaRight.INPUT = RADIO_RIGHT_SEL;
	sRadioPlayCt->HwCt.AUDIOADC_AnaRight.Mode = ADC_Linein;
	sRadioPlayCt->HwCt.AUDIOADC_AnaRight.PGAGain = 31 - RADIO_RIGHT_GAIN_VAR;
	sRadioPlayCt->HwCt.ADCEnergyModel = ADC_ENERGY_SEL;
	sRadioPlayCt->HwCt.ADCBitWidth = AUDIOADC_WIDTH_SEL;
	sRadioPlayCt->HwCt.SampleRate = AudioCoreMixSampleRateGet(AudioCoreGetNetBySourceChannelNUM(RADIO_SOURCE_NUM));

	AudioLineChannelSet(RADIO_INPUT_CHANNEL);

#if (RADIO_INPUT_CHANNEL == ANA_INPUT_CH_AUX1)
	GPIO_PortBModeSet(GPIOB18, 0);
	GPIO_PortBModeSet(GPIOB19, 0);
	GPIO_RegBitsClear(GPIO_B_IE, GPIOB18);
	GPIO_RegBitsClear(GPIO_B_OE, GPIOB18);
	GPIO_RegBitsClear(GPIO_B_IE, GPIOB19);
	GPIO_RegBitsClear(GPIO_B_OE, GPIOB19);
#elif (RADIO_INPUT_CHANNEL == ANA_INPUT_CH_AUX0)
//	DBG("Delay for SW burner\n");
//	DelayMs(3000);//SW/linein, for burner
	GPIO_PortBModeSet(GPIOB20,0);
	GPIO_PortBModeSet(GPIOB21,0);
	GPIO_RegBitsClear(GPIO_B_IE,GPIOB20);
	GPIO_RegBitsClear(GPIO_B_OE,GPIOB20);
	GPIO_RegBitsClear(GPIO_B_IE,GPIOB21);
	GPIO_RegBitsClear(GPIO_B_OE,GPIOB21);
#endif

	AudioADC_Init(RADIO_ADC_SEL, &sRadioPlayCt->HwCt);
	RADIO_CTRLDIGITAL_SEL.Sample_Rate = SampleRateIndexGet(AudioADC_SampleRateGet(RADIO_ADC_SEL));
}

//Radio模式参数配置，资源初始化
bool RadioPlayInit(void)
{
	APP_DBG("Radio Mode init\n");
	DMA_ChannelAllocTableSet(DMA0, (uint8_t* )Dma0ChannelMap);
	DMA_ChannelAllocTableSet(DMA1, (uint8_t* )Dma1ChannelMap);
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
	if(!RadioPlayResMalloc(AudioCoreFrameSizeGet(AudioCoreGetNetBySourceChannelNUM(RADIO_SOURCE_NUM))))
	{
		APP_DBG("RadioPlayResMalloc Res Error!\n");
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
	RadioPlayResInit();

	AudioCodecHardwareSync();//update hardware config

#ifdef	CFG_FUNC_BREAKPOINT_EN
	RadioGetBreakPoint();
#endif

	gRadioControl->State = RADIO_STATUS_IDLE;
	gRadioControl->CurFreqArea = FM_AREA_CHINA;

	// Audio core config
	TimeOutSet(&gRadioControl->TimerHandle, 100);
	switch(gRadioControl->CurFreqArea)
	{
		case FM_AREA_CHINA:
			gRadioControl->MinFreq = RADIO_DEF_MIN_FREQ_VALUE;
			gRadioControl->MaxFreq = RADIO_DEF_MAX_FREQ_VALUE;
			break;
		case FM_AREA_JAPAN:
			gRadioControl->MinFreq = RADIO_JAP_MIN_FREQ_VALUE;
			gRadioControl->MaxFreq = RADIO_JAP_MAX_FREQ_VALUE;
			break;
		case FM_AREA_RUSSIA:
			gRadioControl->MinFreq = RADIO_RUS_MIN_FREQ_VALUE;
			gRadioControl->MaxFreq = RADIO_RUS_MAX_FREQ_VALUE;
			break;
		default:
			gRadioControl->MinFreq = RADIO_OTH_MIN_FREQ_VALUE;
			gRadioControl->MaxFreq = RADIO_OTH_MAX_FREQ_VALUE;
			break;
	}

	if(gRadioControl->Freq < gRadioControl->MinFreq)
	{
		gRadioControl->Freq = gRadioControl->MinFreq;
	}
	else if(gRadioControl->Freq > gRadioControl->MaxFreq)
	{
		gRadioControl->Freq = gRadioControl->MaxFreq;
	}
	APP_DBG("gRadioControl->Freq = %d \n",gRadioControl->Freq);

#ifdef CFG_RADIO_CLK_M12
	Clock_Clock0GPIOOutSel(GPIO_CLK_OUT_A29, RC_CLK);
#endif

	if(RadioPowerOn())
	{
		APP_DBG("Radio PowerOn\n");
		RadioInit();
		RadioMute(TRUE);
		//gRadioControl->Freq = 971;
		RadioSetFreq(gRadioControl->Freq);
		RadioMute(FALSE);
		AudioCoreSourceEnable(RADIO_SOURCE_NUM);
	}
	else
	{
		APP_DBG("err: radio 硬件异常!!!\n");
	}

	#ifdef CFG_FUNC_REMIND_SOUND_EN
	if(!SoftFlagGet(SoftFlagSysCurModeReboot) && RemindSoundServiceItemRequest(SOUND_REMIND_FMMODE, REMIND_ATTR_NEED_MUTE_APP_SOURCE) == FALSE)
	#endif
	{
		if(IsAudioPlayerMute() == TRUE)
		{
			HardWareMuteOrUnMute();
		}
	}

	AudioCoreSourceUnmute(RADIO_SOURCE_NUM, TRUE,TRUE);
	return TRUE;
}


void RadioPlayRun(uint16_t msgId)
{
	if(IsTimeOut(&gRadioControl->TimerHandle))
	{
		TimeOutSet(&gRadioControl->TimerHandle, 100);
		RadioTimerCB(NULL);
	}

	switch(msgId)
	{
		case MSG_PLAY_PAUSE:
		case MSG_RADIO_PLAY_SCAN:
			if(gRadioControl->State == RADIO_STATUS_SEEKING)
			{
				RadioSeekEnd();
			}
			else
			{
				RadioSeekStart();
			}
			break;
		case MSG_FB_START:
		case MSG_RADIO_PLAY_SCAN_DN:
			if(gRadioControl->State == RADIO_STATUS_SEEKING)
			{
				RadioSeekEnd();
			}
			else
			{
			   if(gRadioControl->Freq > gRadioControl->MinFreq)
				{
				  RadioMute(TRUE);
				  gRadioControl->Freq--;
				  RadioSetFreq(gRadioControl->Freq);
				  gRadioControl->State = RADIO_STATUS_SEEKING_DOWN;
				}
			}
			break;
		case MSG_FF_START:
		case MSG_RADIO_PLAY_SCAN_UP:
			if(gRadioControl->State == RADIO_STATUS_SEEKING)
			{
				RadioSeekEnd();
			}
			else
			{
			   if(gRadioControl->Freq < gRadioControl->MaxFreq)
				{
				   RadioMute(TRUE);
				  gRadioControl->Freq++;
				  RadioSetFreq(gRadioControl->Freq);
				  gRadioControl->State = RADIO_STATUS_SEEKING_UP;
				}
			}
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
			break;
#endif
		case MSG_PRE:
		case MSG_RADIO_PLAY_PRE:
			RadioSwitchChannel(msgId);
			break;
		case MSG_NEXT:
		case MSG_RADIO_PLAY_NEXT:
			RadioSwitchChannel(msgId);
			break;

		default:
			CommonMsgProccess(msgId);
			break;
	}
}

bool RadioPlayDeinit(void)
{
	APP_DBG("Radio Mode Deinit\n");
	if(sRadioPlayCt == NULL)
	{
		return TRUE;
	}
#ifdef AUDIO_STREAM_UNINTERRUPT
	if(IsResRamRelease == TRUE)
#endif
	{
		if(IsAudioPlayerMute() == FALSE)
		{
			HardWareMuteOrUnMute();
		}
		PauseAudioCore();
		AudioCoreProcessConfig((void*)AudioNoAppProcess) ;
	}
	//注意：AudioCore父任务调整到mainApp下，此处只关闭AudioCore通道，不关闭任务
	AudioCoreSourceDisable(RADIO_SOURCE_NUM);
	AudioCoreSourceDeinit(RADIO_SOURCE_NUM);

	RadioPowerDown();
#ifdef CFG_RADIO_CLK_M12
	Clock_Clock0GPIOOutDisable(GPIO_CLK_OUT_A29);
#endif

	AudioLineChannelSet(ANA_INPUT_CH_NONE);
	AudioADC_DeInit(RADIO_ADC_SEL);

#if (RADIO_INPUT_CHANNEL == ANA_INPUT_CH_AUX1)
	GPIO_RegBitsClear(GPIO_B_ANA_EN, GPIOB18);
	GPIO_RegBitsClear(GPIO_B_ANA_EN, GPIOB19);
#elif (RADIO_INPUT_CHANNEL == ANA_INPUT_CH_AUX0)
	GPIO_PortBModeSet(GPIOB20,4);
	GPIO_PortBModeSet(GPIOB21,1);
	GPIO_RegBitsClear(GPIO_B_ANA_EN, GPIOB20);
	GPIO_RegBitsClear(GPIO_B_ANA_EN, GPIOB21);
#endif

	RadioPlayResFree();
#ifdef AUDIO_STREAM_UNINTERRUPT
	if(IsResRamRelease == TRUE)
#endif
	{
		ModeCommonDeinit();//通路全部释放
	}
	osPortFree(sRadioPlayCt);
	sRadioPlayCt = NULL;

	return TRUE;
}

#endif //CFG_APP_RADIOIN_MODE_EN
