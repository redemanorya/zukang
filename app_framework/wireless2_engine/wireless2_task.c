/**
 **************************************************************************************
 * @file    wireless2_task.c
 * @brief
 *
 * @author  Tony
 * @version V1.0.0
 *
 * $Created: 2026-04-07
 *
 * @Copyright (C) 2026, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

//Port#include <components/soft_watchdog/soft_watch_dog.h>
//driver、lib
#include "type.h"
#include <string.h>
#include "watchdog.h"
#include "debug.h"
#include "mixer.h"

//application
#include "wireless2_task.h"
#include "app_config.h"
#include "wireless2_audiomain.h"
#include "wireless2_wiremain.h"
#include "mvwireless2_config.h"
#include "audio_vol.h"
#include "rtos_api.h"
#include "main_task.h"
#include "audio_effect.h"
#include "audio_core_api.h"
#include "audio_effect_control.h"
#include "lc3p_audiocodec_api.h"
#include "audio_association.h"
#ifdef DUAL_CORE_EN
#include "dualcore_dsp.h"
#endif

#ifdef FUNC_WIRELESS2_SUPPORT

#define WIREAUDIO_SOURCE_NUM					APP_SOURCE_NUM
#define WIREAUDIO_SINK_NUM						AUDIO_APP_SINK_NUM

#define MVWIRE2_MODE_NUM_MESSAGE_QUEUE			20
#define MVWIRE2_MODE_MSG_TIMEOUT 				1
#define MVWIRE2_MODE_TASK_PRIO					5
#define MVWIRE2_MODE_TASK_STACK_SIZE			4*1024//1024

static Wireless2ModeState   ModeState = Wire2StateDeinit;
static Wireless2ModeContext Wireless2ModeCt = {NULL,NULL};
osMutexId Wireless2ModeMutex = NULL;

static void SysModeDeinit(void);
static void Wireless2ModeInit(void);
static void Wireless2ModeInit(void);
static void Wireless2ModeEntrance(void * param);

bool     WireSinkAudioPlayResMalloc(uint16_t SampleLen);
uint16_t WireSinkAudioPlayDataSet(void *Buffer, uint16_t Len);
uint16_t WireSinkAudioPlayDepthGet(void);
uint16_t WireSinkAudioPlaySpaceLenGet(void);

bool     WireSrcAudioPlayResMalloc(uint16_t SampleLen);
uint16_t WireSrcAudioPlayDataGet(void *Buffer, uint16_t Len);
uint16_t WireSrcAudioPlayDataLenGet(void);

//--------audio-----------------
typedef struct _WirelessPlayContextSink
{
	uint8_t 				Channels;
	uint16_t         		FifoLen;
	uint32_t 				SampleRate;
	void *					FifoAddr;
	MCU_CIRCULAR_CONTEXT 	CircularBuf;
}WirelessPlayContextSink;


WirelessPlayContextSink *sWirelessPlayCt;

typedef struct _WirelessPlayContextSrc
{
	uint8_t 				Channels;
	uint16_t         		FifoLen;
	uint32_t 				SampleRate;
	void *					FifoAddr;
	MCU_CIRCULAR_CONTEXT 	CircularBuf;
}WirelessPlayContextSrc;


WirelessPlayContextSrc *srcWirelessPlayCt;

//---------------sink buffer
bool WireSinkAudioPlayResMalloc(uint16_t SampleLen)
{
	sWirelessPlayCt = (WirelessPlayContextSink*)osPortMalloc(sizeof(WirelessPlayContextSink));
	if(sWirelessPlayCt == NULL)
	{
		return FALSE;
	}
	memset(sWirelessPlayCt, 0, sizeof(WirelessPlayContextSink));

	sWirelessPlayCt->FifoLen = SampleLen * sizeof(PCM_DATA_TYPE) * 2 * 3;
	sWirelessPlayCt->FifoAddr = (uint32_t*)osPortMalloc(sWirelessPlayCt->FifoLen);
	if(sWirelessPlayCt->FifoAddr == NULL)
	{
		return FALSE;
	}
	memset(sWirelessPlayCt->FifoAddr, 0, sWirelessPlayCt->FifoLen);

	MCUCircular_Config(&sWirelessPlayCt->CircularBuf, sWirelessPlayCt->FifoAddr,sWirelessPlayCt->FifoLen);

	return TRUE;
}

uint16_t WireSinkAudioPlaySpaceLenGet(void)
{
	uint32_t Len;

	if(!sWirelessPlayCt)
	{
		return 0;
	}
	Len = MCUCircular_GetSpaceLen(&(sWirelessPlayCt->CircularBuf));
	Len = Len / (sizeof(PCM_DATA_TYPE) * sWirelessPlayCt->Channels);
	return Len;
}

uint8_t remote_cmd = 0xff;
uint8_t encoder_buffer[RFAUDIO_ENCODE_LEN*2+PACKET_CNT_LEN+PACKET_CRC_LEN];
void WireSinkAudioPlayDataCore2_Set()
{
	uint8_t encoder_len = 0;
	static unsigned char send_cnt = 0;
	extern unsigned char audio_init_isready;

#ifdef	WIRELESS2_EFFECTTESTMODE
	return;
#endif

	if(audio_init_isready)
	{
		static unsigned preencoder_cnt = 0;
#if (RFAUDIO_ENCODE_CH==1)
		if((wireless_AudioParityCntGet()!=0xff))
		{
			DualCoreDspEncoderAddrSet(1,encoder_buffer,(uint32_t*)&encoder_len,NULL);
			DualCoreDspEncoderCoreRunDone();
			DualCoreDspEncoderGetOutdata(encoder_buffer,NULL);
			audio_PackBuild(encoder_buffer,send_cnt,remote_cmd);
			if((preencoder_cnt>=3)&&(wireless_AudioParityCntGet()!=0xff)&&(MVWIRE2_GetDevice1ConnState()))
			{
				if(Wireless_TransSpaceLen()>(RFAUDIO_ENCODE_LEN+PACKET_CNT_LEN+PACKET_CRC_LEN))
					Wireless_TransBufWrite(encoder_buffer,RFAUDIO_ENCODE_LEN+PACKET_CNT_LEN+PACKET_CRC_LEN);
			}
			else if((preencoder_cnt<3))
				preencoder_cnt++;
		}
		else
		{
			preencoder_cnt = 0;
		}
#elif (RFAUDIO_ENCODE_CH==2)
		if((wireless_AudioParityCntGet()!=0xff))
		{
			DualCoreDspEncoderAddrSet(3,encoder_buffer,(uint32_t*)&encoder_len,NULL);
			DualCoreDspEncoderCoreRunDone();
			DualCoreDspEncoderGetOutdata(encoder_buffer,encoder_buffer+RFAUDIO_ENCODE_LEN);
			audio_PackBuild(encoder_buffer,send_cnt,remote_cmd);
			if((preencoder_cnt>=3)&&(wireless_AudioParityCntGet()!=0xff)&&
			   (MVWIRE2_GetDevice1ConnState()||MVWIRE2_GetDevice2ConnState())
			  )
			{
				if(Wireless_TransSpaceLen()>(RFAUDIO_ENCODE_LEN*2+PACKET_CNT_LEN+PACKET_CRC_LEN))
					Wireless_TransBufWrite(encoder_buffer,RFAUDIO_ENCODE_LEN*2+PACKET_CNT_LEN+PACKET_CRC_LEN);
			}
			else if((preencoder_cnt<3))
				preencoder_cnt++;

		}
		else
		{
			preencoder_cnt = 0;
		}
#endif
		send_cnt++;
	}
}

uint16_t WireSinkAudioPlayDataSet(void *Buffer, uint16_t Len)
{
#ifdef	WIRELESS2_EFFECTTESTMODE
	static unsigned char send_cnt = 0;
	uint32_t encoder_len = 0;

	downmix_2to1_apply24((int32_t*)Buffer,  (int32_t*)Buffer, ONE_FRAME);
	wireless_encoder_apply(1,encoder_buffer,&encoder_len,Buffer);
	audio_PackBuild(encoder_buffer,send_cnt,0xff);

	device1.ConStatus = CONNECT_AUDIO;
	device1.handle = 0x80;

	AudioAssociationRec(0,encoder_buffer,RFAUDIO_ENCODE_LEN+PACKET_CNT_LEN);
	send_cnt++;
#else
	#if (RFAUDIO_ENCODE_CH==1)
	downmix_2to1_apply24((int32_t*)Buffer,  (int32_t*)Buffer, ONE_FRAME);
	memset(&wireless_encoderbuff_ct, 0, sizeof(WirelessEncoderBuffer_CT));
	memcpy(wireless_encoderbuff_ct.dsp_input_addr,Buffer,ONE_FRAME*4);
	#elif (RFAUDIO_ENCODE_CH==2)
	int i = 0;
	int32_t *pcm_in = Buffer;
	int32_t *pcm_out = wireless_encoderbuff_ct.dsp_input_addr;
	int32_t *pcm_out_R = wireless_encoderbuff_ct.dsp_input_addr_R;

	memset(&wireless_encoderbuff_ct, 0, sizeof(WirelessEncoderBuffer_CT));
	for (i = 0 ; i < ONE_FRAME; i++)
		pcm_out[i] = pcm_in[i * 2];
	for (i = 0 ; i < ONE_FRAME; i++)
		pcm_out_R[i] = pcm_in[i * 2 + 1];
	#endif
#endif//WIRELESS2_EFFECTTESTMODE
	return Len;
}

uint16_t WireSinkAudioPlayDepthGet(void)
{
	uint32_t Len;
	Len = sWirelessPlayCt->FifoLen;
	Len = Len / (sizeof(PCM_DATA_TYPE) * sWirelessPlayCt->Channels);
	return Len;
}

//---------------src buffer
bool WireSrcAudioPlayResMalloc(uint16_t SampleLen)
{
	srcWirelessPlayCt = (WirelessPlayContextSrc*)osPortMalloc(sizeof(WirelessPlayContextSrc));
	if(srcWirelessPlayCt == NULL)
	{
		return FALSE;
	}
	memset(srcWirelessPlayCt, 0, sizeof(WirelessPlayContextSrc));

	srcWirelessPlayCt->FifoLen = SampleLen * sizeof(PCM_DATA_TYPE) * 2 * 3;
	srcWirelessPlayCt->FifoAddr = (uint32_t*)osPortMalloc(srcWirelessPlayCt->FifoLen);
	if(srcWirelessPlayCt->FifoAddr == NULL)
	{
		return FALSE;
	}
	memset(srcWirelessPlayCt->FifoAddr, 0, srcWirelessPlayCt->FifoLen);

	MCUCircular_Config(&srcWirelessPlayCt->CircularBuf, srcWirelessPlayCt->FifoAddr,srcWirelessPlayCt->FifoLen);

	return TRUE;
}

uint16_t WireSrcAudioPlayDataLenGet(void)
{
	uint32_t Len;

	if(!srcWirelessPlayCt)
	{
		return 0;
	}

	Len = Wireless_TransDataLen()/RFAUDIO_ENCODE_LEN*ONE_FRAME;
	return Len;
}
uint32_t DEC_bufferrL[ONE_FRAME];
uint32_t DEC_bufferrR[ONE_FRAME];
uint32_t DEC_bufferr[ONE_FRAME*2];
void WireAudioPlayDecoderProcess()
{
	memset(DEC_bufferr,0,ONE_FRAME*8);
	if(AudioAssociationProcess(DEC_bufferrL,DEC_bufferrR))
	{
		uint16_t WirelessinFrameRead = 0;

		for(; WirelessinFrameRead < ONE_FRAME; WirelessinFrameRead++)
		{
			DEC_bufferr[2 * WirelessinFrameRead    ] = DEC_bufferrL[WirelessinFrameRead];
			DEC_bufferr[2 * WirelessinFrameRead + 1] = DEC_bufferrR[WirelessinFrameRead];
		}
		if(Wireless_DcoderPcmSpaceLen()>=(ONE_FRAME*8))
		{
			Wireless_DcoderPcmBufWrite(DEC_bufferr,ONE_FRAME*8);
		}
#ifdef	WIRELESS2_EFFECTTESTMODE
		if(Device1Info.AudioSycn != SYNC_PACKET_NONE)
		{
			DAC_DATASET_SEL(DEC_bufferr,ONE_FRAME);
		}
#endif
	}
}

uint16_t WireSrcAudioPlayDataGet(void *Buffer, uint16_t Len)
{
	Len = ONE_FRAME;

	memset(Buffer,0,ONE_FRAME*8);
#ifndef	WIRELESSAUDIOSYNCDAC
	if(Wireless_DcoderPcmDataLen()>=(Len*8))
	{
		Wireless_DcoderPcmBufRead(Buffer,Len*8);
	}
#endif
	return Len;
}


void Wireless2PlayAudioCoreInit(void)
{
	AudioCoreIO	AudioIOSet;

//---------------sink-------------------
	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));

	AudioIOSet.Net = AudioCoreGetNetBySinkChannelNUM(WIREAUDIO_SINK_NUM);
	AudioIOSet.SampleRate = AudioCoreMixSampleRateGet(AudioCoreGetNetBySinkChannelNUM(WIREAUDIO_SINK_NUM));
	AudioIOSet.Adapt = STD;
	AudioIOSet.Sync = FALSE;
	AudioIOSet.Channels = 2;

	sWirelessPlayCt->Channels = AudioIOSet.Channels;
	sWirelessPlayCt->SampleRate = AudioIOSet.SampleRate;

	AudioIOSet.Depth = WireSinkAudioPlayDepthGet();
	AudioIOSet.DataIOFunc = WireSinkAudioPlayDataSet;
	AudioIOSet.LenGetFunc = WireSinkAudioPlaySpaceLenGet;
	AudioIOSet.PcmWidth	= CFG_DEFAULT_WIDTH;

	//AudioIOSet.
	if(!AudioCoreSinkInit(&AudioIOSet, WIREAUDIO_SINK_NUM))
	{
		AudioCoreSinkDisable(WIREAUDIO_SINK_NUM);
		DBG("####WIREAUDIO Sink error!!!\n");
		return;
	}
	AudioCoreSinkEnable(WIREAUDIO_SINK_NUM);

//---------------srouce-------------------
	//Soure0.
	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));

	AudioIOSet.Net = AudioCoreGetNetBySourceChannelNUM(WIREAUDIO_SOURCE_NUM);
	AudioIOSet.SampleRate = AudioCoreMixSampleRateGet(AudioCoreGetNetBySourceChannelNUM(WIREAUDIO_SOURCE_NUM));
	AudioIOSet.Adapt = STD;
	AudioIOSet.Sync = FALSE;
	AudioIOSet.Channels = 2;

	srcWirelessPlayCt->Channels = AudioIOSet.Channels;
	srcWirelessPlayCt->SampleRate = AudioIOSet.SampleRate;

	AudioIOSet.DataIOFunc = WireSrcAudioPlayDataGet;
	AudioIOSet.LenGetFunc = WireSrcAudioPlayDataLenGet;
	AudioIOSet.PcmWidth	= CFG_DEFAULT_WIDTH;

	if(!AudioCoreSourceInit(&AudioIOSet, WIREAUDIO_SOURCE_NUM))
	{
		DBG("####WIREAUDIO Source error!!!");
		return;
	}

	AudioCoreSourceEnable(WIREAUDIO_SOURCE_NUM);
}

void WireAudioPlayResFree(void)
{
	AudioCoreSinkDisable(WIREAUDIO_SINK_NUM);

	if(sWirelessPlayCt->FifoAddr != NULL)
	{
		APP_DBG("FifoAddr\n");
		osPortFree(sWirelessPlayCt->FifoAddr);
		sWirelessPlayCt->FifoAddr = NULL;
	}
	AudioCoreSinkDeinit(WIREAUDIO_SINK_NUM);

	osPortFree(sWirelessPlayCt);
	sWirelessPlayCt = NULL;
	APP_DBG("WireAudio Free Ct\n");
}

bool Wireless2AudioPlayInit(void)
{
#ifdef WIRELESS_TX_DEVICE
	APP_DBG("\r\n\r\nWireless2 %s Tx Play Init\r\n\r\n",TURNKEY_NAME);
#else
	APP_DBG("\r\n\r\nWireless2 %s Rx Play Init\r\n\r\n",TURNKEY_NAME);
#endif

	if(!ModeCommonInit())
	{
		ModeCommonDeinit();
		return FALSE;
	}

	if(!WireSinkAudioPlayResMalloc(AudioCoreFrameSizeGet(AudioCoreGetNetBySinkChannelNUM(WIREAUDIO_SINK_NUM))))
	{
		APP_DBG("WireSink Res Error!\n");
		return FALSE;
	}

	if(!WireSrcAudioPlayResMalloc(AudioCoreFrameSizeGet(AudioCoreGetNetBySourceChannelNUM(WIREAUDIO_SOURCE_NUM))))
	{
		APP_DBG("WireSrc Res Error!\n");
		return FALSE;
	}

	//Core Process
#ifdef CFG_FUNC_ROBOEFFECT_EN

//音效功能时，bypassmode不能使用roboeffect src sink buffer
	#ifdef	EFFECT_BYPASSMODE
	AudioCoreProcessConfig((void*)AudioBypassProcess);
	#else
	AudioCoreProcessConfig((void*)AudioMusicProcess);
	#endif

#else
	AudioCoreProcessConfig((void*)AudioBypassProcess);
#endif

#ifdef CFG_FUNC_ROBOEFFECT_EN
#ifdef DUAL_CORE_EN
	DualCoreDspAddrInit();
#endif
	AudioEffect_Params_Sync(&EffectParambin);
#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
	AudioEffect_Params_Sync(&EffectParambin2);
#endif
#endif

	AudioCodecHardwareSync();

	Wireless2PlayAudioCoreInit();

	if(IsAudioPlayerMute() == TRUE)
	{
		HardWareMuteOrUnMute();
	}

	AudioCoreSourceUnmute(WIREAUDIO_SOURCE_NUM, TRUE,TRUE);
	return TRUE;
}


bool Wireless2AudioPlayDeinit(void)
{
	APP_DBG("Wireless2 Play Deinit\n");
	if(sWirelessPlayCt == NULL)
	{
		return TRUE;
	}
	if(IsAudioPlayerMute() == FALSE)
	{
		HardWareMuteOrUnMute();
	}

	PauseAuidoCore();

	//注意：AudioCore父任务调整到mainApp下，此处只关闭AudioCore通道，不关闭任务
	AudioCoreProcessConfig((void*)AudioNoAppProcess);

	WireAudioPlayResFree();
	ModeCommonDeinit();//通路全部释放
	return TRUE;
}


/**
 * @brief mode task creat
 * @param none
 * @return none
 */
void Wireless2ModeTaskCreat(void)
{	
	if(Wireless2ModeCt.taskHandle == NULL)
	{
		ModeState = Wire2StateInit;
		if(xTaskCreate(Wireless2ModeEntrance, "Wireless2Mode", MVWIRE2_MODE_TASK_STACK_SIZE, NULL, MVWIRE2_MODE_TASK_PRIO, &Wireless2ModeCt.taskHandle) != pdTRUE)
		{
			vTaskDelay(50);
			APP_DBG("system error ,Wireless2Mode task create fail\n");
		}
	}
	else
	{
		APP_DBG("system Wireless2Mode already init\n");
	}
}

/**
 * @brief mode task delete
 * @param none
 * @return none
 */
static void Wireless2ModeTaskKill(void)
{
	MessageHandle		msgHandleTemp = NULL;
	
	if(Wireless2ModeCt.taskHandle == NULL)
	{
		return;
	}
	msgHandleTemp =	Wireless2ModeCt.msgHandle;
	Wireless2ModeCt.msgHandle = NULL;
	if(msgHandleTemp != NULL)
	{
		MessageClear(msgHandleTemp);
	}
	vTaskDelete(Wireless2ModeCt.taskHandle);
	Wireless2ModeCt.taskHandle = NULL;
}

/**
 * @brief get mode message handle
 * @param none
 * @return message handle
 */
MessageHandle GetWireless2ModeMsgHandle(void)
{
	return Wireless2ModeCt.msgHandle;
}

/**
 * @brief send message to sysmode
 * @param msgId
 * @return
 */
bool SendWireless2ModeMsg(uint16_t msgId)
{
	MessageContext		msgSend;
	if((msgId>=MSG_WIRELESS_EVENT)&&(msgId<MSG_WIRELESS_EVENT_END))
	{
		msgSend.msgId		= msgId;
		return MessageSend(Wireless2ModeCt.msgHandle, &msgSend);
	}
	return FALSE;
}

/**
 * @brief send mode msg to main task
 * @param none
 * @return none
 */
void SendWireless2ModeKeyMsg(void)
{
	MessageContext		msgSend;
	msgSend.msgId		= MSG_SOFT_MODE;
	MessageSend(GetMainMessageHandle(), &msgSend);
}

/**
 * @brief mode de initialization
 * @param none
 * @return none
 */
static void SysModeDeinit(void)
{
}

/**
 * @brief mode  initialization
 * @param none
 * @return none
 */
static void Wireless2ModeInit(void)
{
	Wireless_AudioAssociationInit();
	Wireless2AudioPlayInit();
	//wirless in fifo
	Wireless_TransBufInit();
	Wireless_DcoderPcmBufInit();
	wireless_encoder_init(1);
#if (RFAUDIO_ENCODE_CH==2)
	wireless_encoder_init(2);
#endif
	DualCoreDspEncoderAddrInit();
}

/**
 * @brief mode  task
 * @param none
 * @return none
 */
static void Wireless2ModeEntrance(void * param)
{
	MessageContext		msg;
	extern unsigned char audio_init_isready;
	
	APP_DBG("start Wireless2ModeEntrance\n");
	vTaskDelay(100);
	Wireless2ModeInit();
	Wireless2ModeCt.msgHandle = MessageRegister(MVWIRE2_MODE_NUM_MESSAGE_QUEUE);
	ModeState = Wire2StateRunning;
	audio_init_isready = 1;
	while(1)
	{
		MessageRecv(Wireless2ModeCt.msgHandle, &msg, MVWIRE2_MODE_MSG_TIMEOUT);
		WDG0_Feed();
		WirelessInfo_Process();
		if(msg.msgId == MSG_TASK_STOP)
		{
			SysModeDeinit();
			Wireless2ModeTaskKill();
			APP_DBG("Kill Wireless2ModeEntrance\n");
		}
		else
		{
			if(msg.msgId == MSG_WIRELESS_EVENT)
			{
				remote_cmd++;
				APP_DBG("Wireless cmd send [0x%x]\n",remote_cmd);
			}
		}
	}
}

#endif
