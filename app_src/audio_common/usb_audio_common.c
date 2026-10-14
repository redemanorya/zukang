/*
 * usb_audio_common.c
 *
 *  Created on: May 7, 2026
 *      Author: piwang
 */
//driver、lib
#include <string.h>
#include "dma.h"
#include "clk.h"
#include "core_d1088.h"
#include "src.h"
#include "reset.h"
#include "src_interface.h"
#include "otg_detect.h"
#include "otg_device_hcd.h"

//applicaion
#include "app_config.h"
#include "main_task.h"
#include "device_detect.h"
#include "otg_device_hcd.h"
#include "otg_device_audio.h"
#include "otg_device_standard_request.h"
#include "otg_device_stor_request.h"
#include "ctrlvars.h"
#include "audio_vol.h"
#include "audio_effect.h"
#include "audio_effect_control.h"
#include "debug.h"
#include "device_detect.h"
#include "communication.h"
#ifdef DUAL_CORE_EN
	#include "dualcore_api.h"
	#include "dualcore_dsp.h"
#endif


#if defined CFG_FUNC_USB_SPEAKER_HW_SRC_EN || defined CFG_USB_MIC_HW_SRC_PORT

	#ifdef CFG_FUNC_USB_SPEAKER_HW_SRC_EN
		extern SRC_Context USBSpeakersrcContext;
		extern bool IsSpeakerSRCFlag;
	#endif
	#ifdef CFG_USB_MIC_HW_SRC_PORT
		extern SRC_Context USBMicsrcContext;
		extern bool IsMicSRCFlag;
	#endif

#endif

//#ifdef CFG_APP_USB_AUDIO_MODE_EN

#if defined CFG_APP_USB_AUDIO_MODE_EN || defined CFG_RES_USB_HS_MIX_AUDIO_EN
void UsbHSDevicePlayResRelease(void)
{
	APP_DBG("UsbHSDevicePlayResRelease\n");
#ifdef CFG_OTG_MODE_AUDIO1_EN
	if(UsbAudioSpeaker1.PCMBuffer != NULL)
	{
		APP_DBG("UsbAudioSpeaker1.PCMBuffer free\n");
		osPortFree(UsbAudioSpeaker1.PCMBuffer);
		UsbAudioSpeaker1.PCMBuffer = NULL;
	}
#endif
#ifdef CFG_OTG_MODE_AUDIO_EN
	#ifdef CFG_FUNC_USB_SPEAKER_HW_SRC_EN
		Reset_Function2Reset((SRC0_FUNC_SEPA) << CFG_USB_SPEAKER_HW_SRC_PORT);
		Reset_Register2Reset(SRC0_REG_SEPA << CFG_USB_SPEAKER_HW_SRC_PORT);
		IsSpeakerSRCFlag = 0;
		if(USBSpeakersrcContext.InBuf != NULL)
		{
			APP_DBG("SpeakerSrcIn free\n");
			osPortFree(USBSpeakersrcContext.InBuf );
			USBSpeakersrcContext.InBuf  = NULL;
			USBSpeakersrcContext.InBufLen = 0;
		}
		if(USBSpeakersrcContext.OutBuf != NULL)
		{
			APP_DBG("SpeakerSrcOut free\n");
			osPortFree(USBSpeakersrcContext.OutBuf );
			USBSpeakersrcContext.OutBuf  = NULL;
			USBSpeakersrcContext.OutBufLen = 0;
		}
	#endif
	if(UsbAudioSpeaker.PCMBuffer != NULL)
	{
		APP_DBG("UsbAudioSpeaker.PCMBuffer free\n");
		osPortFree(UsbAudioSpeaker.PCMBuffer);
		UsbAudioSpeaker.PCMBuffer = NULL;
	}
#endif
#ifdef CFG_OTG_MODE_MIC_EN
	#ifdef CFG_FUNC_USB_MIC_HW_SRC_EN
		Reset_Function2Reset((SRC0_FUNC_SEPA) << CFG_USB_MIC_HW_SRC_PORT);
		Reset_Register2Reset(SRC0_REG_SEPA << CFG_USB_MIC_HW_SRC_PORT);
		IsMicSRCFlag = 0;
		if(USBMicsrcContext.InBuf != NULL)
		{
			APP_DBG("UsbMicSrcIn free\n");
			osPortFree(USBMicsrcContext.InBuf );
			USBMicsrcContext.InBuf  = NULL;
			USBMicsrcContext.InBufLen = 0;
		}
		if(USBMicsrcContext.OutBuf != NULL)
		{
			APP_DBG("UsbMicSrcOut free\n");
			osPortFree(USBMicsrcContext.OutBuf );
			USBMicsrcContext.OutBuf  = NULL;
			USBMicsrcContext.OutBufLen = 0;
		}
	#endif
	if(UsbAudioMic.PCMBuffer != NULL)
	{
		APP_DBG("UsbAudioMic.PCMBuffer free\n");
		osPortFree(UsbAudioMic.PCMBuffer);
		UsbAudioMic.PCMBuffer = NULL;
	}
#endif

}

bool UsbHSDevicePlayResMalloc(uint8_t SourceId, uint8_t SinkId)
{
	uint32_t buf_len;

//pc->chip
	#define SPEAKER_FIFO_FRAME		3
#ifdef CFG_OTG_MODE_AUDIO1_EN
	uint8_t Speaker1_Net = AudioCoreGetNetBySourceChannelNUM(USB_AUDIO_SOURCE1_NUM);
	//Speaker FIFO
	buf_len = (((uint64_t)AudioCoreMixStepUsGet(Speaker1_Net) * SPEAKER_FIFO_FRAME * USBD_AUDIO1_FREQ)  /1000000)  * PCM_BYTE(CFG_DEFAULT_WIDTH) * PACKET1_CHANNELS_NUM;
#ifdef AUDIO_STREAM_UNINTERRUPT
	UsbAudioSpeaker1.PCMBuffer = osPortMallocFromEnd(buf_len);
#else
	UsbAudioSpeaker1.PCMBuffer = osPortMalloc(buf_len);
#endif
	if(UsbAudioSpeaker1.PCMBuffer == NULL)
	{
		APP_DBG("UsbAudioSpeaker1.PCMBuffer memory error\n");
		return FALSE;
	}
	memset(UsbAudioSpeaker1.PCMBuffer, 0, buf_len);
	MCUCircular_Config(&UsbAudioSpeaker1.CircularBuf, UsbAudioSpeaker1.PCMBuffer, buf_len);
#endif//end of CFG_RES_USB_IN_EN

#ifdef CFG_OTG_MODE_AUDIO_EN
	uint8_t Speaker_Net = AudioCoreGetNetBySourceChannelNUM(SourceId);
	uint32_t sampleRate = AudioCoreMixSampleRateGet(Speaker_Net);
	//Speaker FIFO
	#ifdef CFG_FUNC_USB_SPEAKER_HW_SRC_EN
		buf_len = AudioCoreFrameSizeGet(Speaker_Net) * SPEAKER_FIFO_FRAME * PCM_BYTE(CFG_DEFAULT_WIDTH) * PACKET_CHANNELS_NUM;
	#else
		buf_len = (((uint64_t)AudioCoreMixStepUsGet(Speaker_Net) * SPEAKER_FIFO_FRAME * USBD_AUDIO_FREQ) /1000000) * PCM_BYTE(CFG_DEFAULT_WIDTH) * PACKET_CHANNELS_NUM;
	#endif
	DBG("SpeakerFifoLen:%lu\n", buf_len);
#ifdef AUDIO_STREAM_UNINTERRUPT
	UsbAudioSpeaker.PCMBuffer = osPortMallocFromEnd(buf_len);
#else
	UsbAudioSpeaker.PCMBuffer = osPortMalloc(buf_len);
#endif
	if(UsbAudioSpeaker.PCMBuffer == NULL)
	{
		APP_DBG("UsbSpeaker.PCMBuffer memory error\n");
		return FALSE;
	}
	memset(UsbAudioSpeaker.PCMBuffer, 0, buf_len);
	MCUCircular_Config(&UsbAudioSpeaker.CircularBuf, UsbAudioSpeaker.PCMBuffer, buf_len);

	#ifdef CFG_FUNC_USB_SPEAKER_HW_SRC_EN

		//src set
		USBSpeakersrcContext.InBufLen = MAX(USBD_AUDIO_FREQ/HS_UAC_20_FRAM_PRT_SECOND,  USBD_AUDIO10_FREQ / HS_UAC_10_FRAM_PRT_SECOND);
		USBSpeakersrcContext.OutBufLen = CFG_SPEAKER_SRCOUT(USBSpeakersrcContext.InBufLen, sampleRate);//Samples
#ifdef AUDIO_STREAM_UNINTERRUPT
		USBSpeakersrcContext.InBuf = (uint32_t*)osPortMallocFromEnd(USBSpeakersrcContext.InBufLen * 8);
		USBSpeakersrcContext.OutBuf = (uint32_t*)osPortMallocFromEnd(USBSpeakersrcContext.OutBufLen * 8);
#else
		USBSpeakersrcContext.InBuf = (uint32_t*)osPortMalloc(USBSpeakersrcContext.InBufLen * 8);
		USBSpeakersrcContext.OutBuf = (uint32_t*)osPortMalloc(USBSpeakersrcContext.OutBufLen * 8);
#endif
		if((USBSpeakersrcContext.InBuf == NULL) || (USBSpeakersrcContext.OutBuf == NULL))
		{
			APP_DBG("UsbSpeakerSRCIn/Out buf error\n");
			return FALSE;
		}
		USBSpeakersrcContext.Mode = MODE_BLOCK;
		USBSpeakersrcContext.Width = SRC_24BIT;
		USBSpeakersrcContext.NumChannels = 2;
		USBSpeakersrcContext.BlockSamples = USBSpeakersrcContext.InBufLen;
		if(!SoftFlagGet(SoftFlagSysCurModeReboot) && !UsbAudioSpeaker.AudioSampleRate)
		{
			UsbAudioSpeaker.AudioSampleRate = USBD_AUDIO_FREQ;//预配
		}

		IsSpeakerSRCFlag = (UsbAudioSpeaker.AudioSampleRate != sampleRate) ? 1: 0;
		SRC_BlockInit(CFG_USB_SPEAKER_HW_SRC_PORT, &USBSpeakersrcContext, UsbAudioSpeaker.AudioSampleRate, sampleRate);//设置一个初始的采样率转换值
	#endif

#endif//end of CFG_RES_USB_IN_EN

#ifdef CFG_OTG_MODE_MIC_EN
	#define USBMIC_FIFO_FRAME		3
	uint8_t Mic_Net = AudioCoreGetNetBySinkChannelNUM(SinkId);
	uint32_t MicSampleRate = AudioCoreMixSampleRateGet(Mic_Net);
	uint16_t MicSampleLen = AudioCoreFrameSizeGet(Mic_Net);
	//MIC FIFO
	buf_len = (((uint64_t)AudioCoreMixStepUsGet(Mic_Net) * USBMIC_FIFO_FRAME * USBD_AUDIO_MIC_FREQ) /1000000) * PCM_BYTE(CFG_DEFAULT_WIDTH) * MIC_CHANNELS_NUM;

	DBG("UsbMicFifoLen:%lu\n", buf_len);
#ifdef AUDIO_STREAM_UNINTERRUPT
	UsbAudioMic.PCMBuffer = osPortMallocFromEnd(buf_len);
#else
	UsbAudioMic.PCMBuffer = osPortMalloc(buf_len);
#endif
	if(UsbAudioMic.PCMBuffer == NULL)
	{
		APP_DBG("UsbAudioMic.PCMBuffer memory error\n");
		return FALSE;
	}
	memset(UsbAudioMic.PCMBuffer, 0, buf_len);
	MCUCircular_Config(&UsbAudioMic.CircularBuf, UsbAudioMic.PCMBuffer,buf_len);


	#ifdef CFG_FUNC_USB_MIC_HW_SRC_EN
		//src set
		USBMicsrcContext.InBufLen = MicSampleLen;//Samples
		USBMicsrcContext.OutBufLen = CFG_USBMIC_SRCOUT(USBMicsrcContext.InBufLen, MicSampleRate,USBD_AUDIO_MIC_FREQ);//Samples
#ifdef AUDIO_STREAM_UNINTERRUPT
		USBMicsrcContext.InBuf = (uint32_t*)osPortMallocFromEnd(USBMicsrcContext.InBufLen * 8);
		USBMicsrcContext.OutBuf = (uint32_t*)osPortMallocFromEnd(USBMicsrcContext.OutBufLen * 8);
#else
		USBMicsrcContext.InBuf = (uint32_t*)osPortMalloc(USBMicsrcContext.InBufLen * 8);
		USBMicsrcContext.OutBuf = (uint32_t*)osPortMalloc(USBMicsrcContext.OutBufLen * 8);
#endif
		if((USBMicsrcContext.InBuf == NULL) || (USBMicsrcContext.OutBuf == NULL))
		{
			APP_DBG("UsbMicSRCIn/Out buf error\n");
			return FALSE;
		}

		USBMicsrcContext.Mode = MODE_BLOCK;
		USBMicsrcContext.Width = SRC_24BIT;
		USBMicsrcContext.NumChannels = 2;
		USBMicsrcContext.BlockSamples = USBMicsrcContext.InBufLen;
		if(!SoftFlagGet(SoftFlagSysCurModeReboot) && !UsbAudioMic.AudioSampleRate)
		{
			UsbAudioMic.AudioSampleRate = USBD_AUDIO_MIC_FREQ;//预配
		}
		IsMicSRCFlag = (UsbAudioMic.AudioSampleRate != MicSampleRate) ? 1: 0;
		SRC_BlockInit(CFG_USB_MIC_HW_SRC_PORT, &USBMicsrcContext, MicSampleRate, UsbAudioMic.AudioSampleRate);
	#endif
#endif///end of CFG_REGS_AUDIO_USB_OUT_EN
	return TRUE;
}

bool UsbHSDevicePlayResInit(uint8_t SourceId, uint8_t SinkId)
{
	APP_DBG("UsbHSDevicePlayResInit\n");
	//Core Source1 para
	AudioCoreIO	AudioIOSet;
#ifdef CFG_OTG_MODE_AUDIO1_EN
	uint8_t Speaker1_Net = AudioCoreGetNetBySourceChannelNUM(USB_AUDIO_SOURCE1_NUM);
	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
	AudioIOSet.Adapt = SRC_SRA;
	AudioIOSet.SrcQuality = SRC_QUALITY;
	AudioIOSet.Sync = FALSE;//TRUE;//
	AudioIOSet.Channels = PACKET1_CHANNELS_NUM;
	AudioIOSet.Net = Speaker1_Net;
	AudioIOSet.DataIOFunc = UsbAudioSpeaker1DataGet;
	AudioIOSet.LenGetFunc = UsbAudioSpeaker1DataLenGet;
	AudioIOSet.Depth = UsbAudioSpeaker1DepthGet();
	AudioIOSet.LowLevelCent = 40;
	AudioIOSet.HighLevelCent = 60;
	if(UsbAudioSpeaker1.AudioSampleRate) //已枚举
	{
		AudioIOSet.SampleRate = UsbAudioSpeaker1.AudioSampleRate;
	}
	else
	{
		AudioIOSet.SampleRate = AudioCoreMixSampleRateGet(Speaker1_Net);//初始值
	}
	#ifdef CFG_AUDIO_OUT_AUTO_SAMPLE_RATE_44100_48000
		AudioOutSampleRateSet(AudioIOSet.SampleRate);
	#endif
	AudioIOSet.PcmWidth	= CFG_DEFAULT_WIDTH;
	if(!AudioCoreSourceInit(&AudioIOSet, USB_AUDIO_SOURCE1_NUM))
	{
		DBG("Usbin source error!\n");
		return FALSE;
	}
	AudioCoreSourceAdjust(USB_AUDIO_SOURCE1_NUM, TRUE);//仅在init通路配置微调后，通路使能时 有效
#endif

#ifdef CFG_OTG_MODE_AUDIO_EN
	uint8_t Speaker_Net = AudioCoreGetNetBySourceChannelNUM(SourceId);
	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
	#if defined(CFG_PARA_AUDIO_USB_IN_SYNC)//需微调   启用硬件时需要和Out协同
		#if defined(CFG_FUNC_USB_SPEAKER_HW_SRC_EN) && !defined(CFG_FUNC_USB_MCLK_ADJUST_EN)
			AudioIOSet.Adapt = SRA_ONLY;
		#elif defined(CFG_FUNC_USB_SPEAKER_HW_SRC_EN) && defined(CFG_FUNC_USB_MCLK_ADJUST_EN)
			AudioIOSet.Adapt = CLK_ADJUST_ONLY;
		#elif defined(CFG_FUNC_USB_MCLK_ADJUST_EN)
			AudioIOSet.Adapt = SRC_ADJUST;
		#else
			AudioIOSet.Adapt = SRC_SRA;
		#endif
	#elif defined(CFG_FUNC_USB_SPEAKER_HW_SRC_EN)
		AudioIOSet.Adapt = STD;
	#else
		AudioIOSet.Adapt = SRC_ONLY;
	#endif
	AudioIOSet.SrcQuality = SRC_QUALITY;
	AudioIOSet.Sync = FALSE;//TRUE;//
	AudioIOSet.Channels = PACKET_CHANNELS_NUM;
	AudioIOSet.Net = Speaker_Net;
	AudioIOSet.DataIOFunc = UsbAudioSpeakerDataGet;
	AudioIOSet.LenGetFunc = UsbAudioSpeakerDataLenGet;
	AudioIOSet.Depth = UsbAudioSpeakerDepthGet();
	AudioIOSet.LowLevelCent = 40;
	AudioIOSet.HighLevelCent = 60;
	if(UsbAudioSpeaker.AudioSampleRate) //已枚举
	{
		AudioIOSet.SampleRate = UsbAudioSpeaker.AudioSampleRate;
	}
	else
	{
		AudioIOSet.SampleRate = AudioCoreMixSampleRateGet(Speaker_Net);//初始值
	}
	#ifdef CFG_AUDIO_OUT_AUTO_SAMPLE_RATE_44100_48000
		AudioOutSampleRateSet(AudioIOSet.SampleRate);
	#endif
	AudioIOSet.PcmWidth	= CFG_DEFAULT_WIDTH;
	if(!AudioCoreSourceInit(&AudioIOSet, SourceId))
	{
		DBG("Usbin source error!\n");
		return FALSE;
	}
	AudioCoreSourceAdjust(SourceId, TRUE);//仅在init通路配置微调后，通路使能时 有效
#endif
#ifdef CFG_OTG_MODE_MIC_EN
	uint8_t Mic_Net = AudioCoreGetNetBySinkChannelNUM(SinkId);
	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
	#if defined(CFG_PARA_AUDIO_USB_OUT_SYNC)//微调  启用硬件时需要和Out协同
		#if defined(CFG_FUNC_USB_MIC_HW_SRC_EN) && !defined(CFG_FUNC_USB_MCLK_ADJUST_EN)
			AudioIOSet.Adapt = SRA_ONLY;//CLK_ADJUST_ONLY;
		#elif (defined(CFG_FUNC_USB_MIC_HW_SRC_EN) && defined(CFG_FUNC_USB_MCLK_ADJUST_EN))
			AudioIOSet.Adapt = CLK_ADJUST_ONLY;
		#elif defined(CFG_FUNC_USB_MCLK_ADJUST_EN)
			AudioIOSet.Adapt = SRC_ADJUST;
		#else
			AudioIOSet.Adapt = SRC_SRA;
		#endif
	#elif defined(CFG_FUNC_USB_MIC_HW_SRC_EN)//无微调同步
		AudioIOSet.Adapt = STD;
	#else
		AudioIOSet.Adapt = SRC_ONLY;
	#endif
	AudioIOSet.SrcQuality = SRC_QUALITY;
	AudioIOSet.Sync = FALSE;//TRUE;//
	AudioIOSet.Channels = MIC_CHANNELS_NUM;
	AudioIOSet.Net = Mic_Net;
	AudioIOSet.DataIOFunc = UsbAudioMicDataSet;
	AudioIOSet.LenGetFunc = UsbAudioMicSpaceLenGet;
	AudioIOSet.Depth = UsbAudioMicDepthGet();
	AudioIOSet.LowLevelCent = 40;
	AudioIOSet.HighLevelCent = 60;

	if(UsbAudioMic.AudioSampleRate) //已枚举
	{
		AudioIOSet.SampleRate = UsbAudioMic.AudioSampleRate;
	}
	else
	{
		AudioIOSet.SampleRate = USBD_AUDIO_MIC_FREQ1;//初始值
	}
	AudioIOSet.PcmWidth	= CFG_DEFAULT_WIDTH;
	if(!AudioCoreSinkInit(&AudioIOSet, USB_AUDIO_SINK_NUM))
	{
		DBG("Usbout sink error!\n");
	}
	AudioCoreSinkAdjust(USB_AUDIO_SINK_NUM,TRUE);
#endif
	return TRUE;
}
#endif //#if defined CFG_APP_USB_AUDIO_MODE_EN || defined CFG_RES_USB_HS_MIX_AUDIO_EN


#if defined CFG_APP_USB_FS_AUDIO_MODE_EN || defined CFG_RES_USB_FS_MIX_AUDIO_EN

void UsbFSDevicePlayResRelease(void)
{
	APP_DBG("UsbFSDevicePlayResRelease\n");
#ifdef CFG_OTG_FS_MODE_AUDIO1_EN
	if(UsbAudioMixSpeaker1.PCMBuffer != NULL)
	{
		APP_DBG("UsbAudioMixSpeaker1.PCMBuffer free\n");
		osPortFree(UsbAudioMixSpeaker1.PCMBuffer);
		UsbAudioMixSpeaker1.PCMBuffer = NULL;
	}
#endif
#ifdef CFG_OTG_FS_MODE_AUDIO_EN
	#if 0//def CFG_FUNC_USB_SPEAKER_HW_SRC_EN
		Reset_Function2Reset((SRC0_FUNC_SEPA) << CFG_USB_SPEAKER_HW_SRC_PORT);
		Reset_Register2Reset(SRC0_REG_SEPA << CFG_USB_SPEAKER_HW_SRC_PORT);
		IsSpeakerSRCFlag = 0;
		if(USBSpeakersrcContext.InBuf != NULL)
		{
			APP_DBG("SpeakerSrcIn free\n");
			osPortFree(USBSpeakersrcContext.InBuf );
			USBSpeakersrcContext.InBuf  = NULL;
			USBSpeakersrcContext.InBufLen = 0;
		}
		if(USBSpeakersrcContext.OutBuf != NULL)
		{
			APP_DBG("SpeakerSrcOut free\n");
			osPortFree(USBSpeakersrcContext.OutBuf );
			USBSpeakersrcContext.OutBuf  = NULL;
			USBSpeakersrcContext.OutBufLen = 0;
		}
	#endif
	if(UsbAudioMixSpeaker.PCMBuffer != NULL)
	{
		APP_DBG("UsbAudioMixSpeaker.PCMBuffer free\n");
		osPortFree(UsbAudioMixSpeaker.PCMBuffer);
		UsbAudioMixSpeaker.PCMBuffer = NULL;
	}
#endif
#ifdef CFG_OTG_FS_MODE_MIC_EN
	#if 0//def CFG_FUNC_USB_MIC_HW_SRC_EN
		Reset_Function2Reset((SRC0_FUNC_SEPA) << CFG_USB_MIC_HW_SRC_PORT);
		Reset_Register2Reset(SRC0_REG_SEPA << CFG_USB_MIC_HW_SRC_PORT);
		IsMicSRCFlag = 0;
		if(USBMicsrcContext.InBuf != NULL)
		{
			APP_DBG("UsbMicSrcIn free\n");
			osPortFree(USBMicsrcContext.InBuf );
			USBMicsrcContext.InBuf  = NULL;
			USBMicsrcContext.InBufLen = 0;
		}
		if(USBMicsrcContext.OutBuf != NULL)
		{
			APP_DBG("UsbMicSrcOut free\n");
			osPortFree(USBMicsrcContext.OutBuf );
			USBMicsrcContext.OutBuf  = NULL;
			USBMicsrcContext.OutBufLen = 0;
		}
	#endif
	if(UsbAudioMixMic.PCMBuffer != NULL)
	{
		APP_DBG("UsbAudioMixMic.PCMBuffer free\n");
		osPortFree(UsbAudioMixMic.PCMBuffer);
		UsbAudioMixMic.PCMBuffer = NULL;
	}
#endif
}

bool UsbFSDevicePlayResMalloc(uint8_t SourceId, uint8_t SinkId)
{
	uint32_t buf_len;

//pc->chip
	#define SPEAKER_FIFO_FRAME		3
#ifdef CFG_OTG_FS_MODE_AUDIO1_EN
	uint8_t Speaker1_Net = AudioCoreGetNetBySourceChannelNUM(USB_AUDIO_SOURCE1_NUM);
	//Speaker FIFO
	buf_len = (((uint64_t)AudioCoreMixStepUsGet(Speaker1_Net) * SPEAKER_FIFO_FRAME * USBD_AUDIO1_FREQ) / 1000000) * PCM_BYTE(CFG_DEFAULT_WIDTH) * PACKET1_CHANNELS_NUM;
#ifdef AUDIO_STREAM_UNINTERRUPT
	UsbAudioMixSpeaker1.PCMBuffer = osPortMallocFromEnd(buf_len);
#else
	UsbAudioMixSpeaker1.PCMBuffer = osPortMalloc(buf_len);
#endif
	if(UsbAudioMixSpeaker1.PCMBuffer == NULL)
	{
		APP_DBG("UsbAudioMixSpeaker1.PCMBuffer memory error\n");
		return FALSE;
	}
	memset(UsbAudioMixSpeaker1.PCMBuffer, 0, buf_len);
	MCUCircular_Config(&UsbAudioMixSpeaker1.CircularBuf, UsbAudioMixSpeaker1.PCMBuffer, buf_len);
#endif//end of CFG_RES_USB_IN_EN

#ifdef CFG_OTG_FS_MODE_AUDIO_EN
	uint8_t Speaker_Net = AudioCoreGetNetBySourceChannelNUM(SourceId);
	#if 0
	uint32_t sampleRate = AudioCoreMixSampleRateGet(Speaker_Net);
	#endif
	//Speaker FIFO
	//#ifdef CFG_FUNC_USB_SPEAKER_HW_SRC_EN
	#if 0
		buf_len = AudioCoreFrameSizeGet(Speaker_Net) * SPEAKER_FIFO_FRAME * PCM_BYTE(CFG_DEFAULT_WIDTH) * PACKET_CHANNELS_NUM;
	#else
		buf_len = (((uint64_t)AudioCoreMixStepUsGet(Speaker_Net) * SPEAKER_FIFO_FRAME * USBD_AUDIO_FREQ) /1000000) * PCM_BYTE(CFG_DEFAULT_WIDTH) * PACKET_CHANNELS_NUM;
	#endif
	DBG("\nUsbFSSpeakerFifoLen:%lu\n", buf_len);
#ifdef AUDIO_STREAM_UNINTERRUPT
	UsbAudioMixSpeaker.PCMBuffer = osPortMallocFromEnd(buf_len);
#else
	UsbAudioMixSpeaker.PCMBuffer = osPortMalloc(buf_len);
#endif
	if(UsbAudioMixSpeaker.PCMBuffer == NULL)
	{
		APP_DBG("UsbAudioMixSpeaker.PCMBuffer memory error\n");
		return FALSE;
	}
	memset(UsbAudioMixSpeaker.PCMBuffer, 0, buf_len);
	MCUCircular_Config(&UsbAudioMixSpeaker.CircularBuf, UsbAudioMixSpeaker.PCMBuffer, buf_len);

	#if 0//def CFG_FUNC_USB_SPEAKER_HW_SRC_EN

		//src set
		USBSpeakersrcContext.InBufLen = MAX(USBD_AUDIO_FREQ/HS_UAC_20_FRAM_PRT_SECOND,  USBD_AUDIO10_FREQ / HS_UAC_10_FRAM_PRT_SECOND);
		USBSpeakersrcContext.OutBufLen = CFG_SPEAKER_SRCOUT(USBSpeakersrcContext.InBufLen, sampleRate);//Samples
#ifdef AUDIO_STREAM_UNINTERRUPT
		USBSpeakersrcContext.InBuf = (uint32_t*)osPortMallocFromEnd(USBSpeakersrcContext.InBufLen * 8);
		USBSpeakersrcContext.OutBuf = (uint32_t*)osPortMallocFromEnd(USBSpeakersrcContext.OutBufLen * 8);
#else
		USBSpeakersrcContext.InBuf = (uint32_t*)osPortMalloc(USBSpeakersrcContext.InBufLen * 8);
		USBSpeakersrcContext.OutBuf = (uint32_t*)osPortMalloc(USBSpeakersrcContext.OutBufLen * 8);
#endif
		if((USBSpeakersrcContext.InBuf == NULL) || (USBSpeakersrcContext.OutBuf == NULL))
		{
			APP_DBG("UsbSpeakerSRCIn/Out buf error\n");
			return FALSE;
		}
		USBSpeakersrcContext.Mode = MODE_BLOCK;
		USBSpeakersrcContext.Width = SRC_24BIT;
		USBSpeakersrcContext.NumChannels = 2;
		USBSpeakersrcContext.BlockSamples = USBSpeakersrcContext.OutBufLen;
		if(!SoftFlagGet(SoftFlagSysCurModeReboot))
		{
			UsbAudioSpeaker.AudioSampleRate = USBD_AUDIO_FREQ;//预配
		}

		IsSpeakerSRCFlag = (UsbAudioSpeaker.AudioSampleRate != sampleRate) ? 1: 0;
		SRC_BlockInit(CFG_USB_SPEAKER_HW_SRC_PORT, &USBSpeakersrcContext, UsbAudioSpeaker.AudioSampleRate, sampleRate);//设置一个初始的采样率转换值
	#endif
#endif//end of CFG_RES_USB_IN_EN

#ifdef CFG_OTG_FS_MODE_MIC_EN
	#define USBMIC_FIFO_FRAME		3
	uint8_t Mic_Net = AudioCoreGetNetBySinkChannelNUM(SinkId);
	#if 0
	uint32_t MicSampleRate = AudioCoreMixSampleRateGet(Mic_Net);
	uint16_t MicSampleLen = AudioCoreFrameSizeGet(Mic_Net);
	#endif
	//MIC FIFO
	buf_len = (((uint64_t)AudioCoreMixStepUsGet(Mic_Net) * USBMIC_FIFO_FRAME * USBD_AUDIO_MIC_FREQ) /1000000) * PCM_BYTE(CFG_DEFAULT_WIDTH) * MIC_CHANNELS_NUM;

	DBG("\nUsbFSMicFifoLen:%lu\n", buf_len);
#ifdef AUDIO_STREAM_UNINTERRUPT
	UsbAudioMixMic.PCMBuffer = osPortMallocFromEnd(buf_len);
#else
	UsbAudioMixMic.PCMBuffer = osPortMalloc(buf_len);
#endif
	if(UsbAudioMixMic.PCMBuffer == NULL)
	{
		APP_DBG("UsbAudioMixMic.PCMBuffer memory error\n");
		return FALSE;
	}
	memset(UsbAudioMixMic.PCMBuffer, 0, buf_len);
	MCUCircular_Config(&UsbAudioMixMic.CircularBuf, UsbAudioMixMic.PCMBuffer, buf_len);
	APP_DBG("buf_len = %ld\n", buf_len);
#if 0 //def CFG_FUNC_USB_MIC_HW_SRC_EN
	//src set
	USBMicsrcContext.InBufLen = MicSampleLen;//Samples
	USBMicsrcContext.OutBufLen = CFG_USBMIC_SRCOUT(USBMicsrcContext.InBufLen, MicSampleRate,USBD_AUDIO10_MIC_FREQ);//Samples
#ifdef AUDIO_STREAM_UNINTERRUPT
	USBMicsrcContext.InBuf = (uint32_t*)osPortMallocFromEnd(USBMicsrcContext.InBufLen * 8);
	USBMicsrcContext.OutBuf = (uint32_t*)osPortMallocFromEnd(USBMicsrcContext.OutBufLen * 8);
#else
	USBMicsrcContext.InBuf = (uint32_t*)osPortMalloc(USBMicsrcContext.InBufLen * 8);
	USBMicsrcContext.OutBuf = (uint32_t*)osPortMalloc(USBMicsrcContext.OutBufLen * 8);
#endif
	if((USBMicsrcContext.InBuf == NULL) || (USBMicsrcContext.OutBuf == NULL))
	{
		APP_DBG("UsbMicSRCIn/Out buf error\n");
		return FALSE;
	}

	USBMicsrcContext.Mode = MODE_BLOCK;
	USBMicsrcContext.Width = SRC_24BIT;
	USBMicsrcContext.NumChannels = 2;
	USBMicsrcContext.BlockSamples = USBMicsrcContext.InBufLen;
	if(!SoftFlagGet(SoftFlagSysCurModeReboot))
	{
		UsbAudioMic.AudioSampleRate = USBD_AUDIO10_MIC_FREQ;//预配
	}
	IsMicSRCFlag = (UsbAudioMic.AudioSampleRate != MicSampleRate) ? 1: 0;
	SRC_BlockInit(CFG_USB_MIC_HW_SRC_PORT, &USBMicsrcContext, MicSampleRate, UsbAudioMic.AudioSampleRate);
#endif
#endif///end of CFG_REGS_AUDIO_USB_OUT_EN
	return TRUE;
}

bool UsbFSDevicePlayResInit(uint8_t SourceId, uint8_t SinkId)
{
	APP_DBG("UsbFSDevicePlayResInit\n");
	//Core Source1 para
	AudioCoreIO	AudioIOSet;
#ifdef CFG_OTG_FS_MODE_AUDIO1_EN
	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
	AudioIOSet.Adapt = SRC_SRA;//STD;//SRC_ADJUST;
	AudioIOSet.SrcQuality = SRC_QUALITY;
	AudioIOSet.Sync = TRUE;
	AudioIOSet.Channels = PACKET1_CHANNELS_NUM;
	AudioIOSet.Net = AudioCoreGetNetBySourceChannelNUM(USB_AUDIO_SOURCE1_NUM);
	AudioIOSet.DataIOFunc = UsbFSAudioMixSpeaker1DataGet;
	AudioIOSet.LenGetFunc = UsbFSAudioMixSpeaker1DataLenGet;
	AudioIOSet.Depth = UsbFSAudioMixSpeaker1DepthGet();
	AudioIOSet.LowLevelCent = 40;
	AudioIOSet.HighLevelCent = 60;
	if(UsbAudioMixSpeaker1.AudioSampleRate) //已枚举
	{
		AudioIOSet.SampleRate = UsbAudioMixSpeaker1.AudioSampleRate;
	}
	else
	{
		AudioIOSet.SampleRate = AudioCoreMixSampleRateGet(AudioIOSet.Net);//初始值
	}
//#ifdef CFG_AUDIO_OUT_AUTO_SAMPLE_RATE_44100_48000
//	AudioOutSampleRateSet(AudioIOSet.SampleRate);
//#endif
	AudioIOSet.PcmWidth	= CFG_DEFAULT_WIDTH;
	if(!AudioCoreSourceInit(&AudioIOSet, USB_AUDIO_SOURCE1_NUM))
	{
		DBG("Usbin source error!\n");
		return FALSE;
	}
	AudioCoreSourceAdjust(USB_AUDIO_SOURCE1_NUM, TRUE);//仅在init通路配置微调后，通路使能时 有效
#endif

#ifdef CFG_OTG_FS_MODE_AUDIO_EN
	uint8_t Speaker_Net = AudioCoreGetNetBySourceChannelNUM(SourceId);
	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
	#if defined(CFG_PARA_AUDIO_USB_FS_IN_SYNC)//需微调   启用硬件时需要和Out协同
		#if 0//暂无HWSRC、双OTG硬件微调必二选一。
			AudioIOSet.Adapt = SRC_ADJUST;
		#else
			AudioIOSet.Adapt = SRC_SRA;
		#endif
	#else
		AudioIOSet.Adapt = SRC_ONLY;
	#endif
	AudioIOSet.SrcQuality = SRC_QUALITY;
	AudioIOSet.Sync = FALSE;//TRUE;
	AudioIOSet.Channels = PACKET_CHANNELS_NUM;
	AudioIOSet.Net = Speaker_Net;
	AudioIOSet.DataIOFunc = UsbFSAudioMixSpeakerDataGet;
	AudioIOSet.LenGetFunc = UsbFSAudioMixSpeakerDataLenGet;
	AudioIOSet.Depth = UsbFSAudioMixSpeakerDepthGet();

	AudioIOSet.LowLevelCent = 40;
	AudioIOSet.HighLevelCent = 60;
	if(UsbAudioMixSpeaker.AudioSampleRate) //已枚举
	{
		AudioIOSet.SampleRate = UsbAudioMixSpeaker.AudioSampleRate;
	}
	else
	{
		AudioIOSet.SampleRate = AudioCoreMixSampleRateGet(Speaker_Net);//初始值
	}
	#ifdef CFG_AUDIO_OUT_AUTO_SAMPLE_RATE_44100_48000
		AudioOutSampleRateSet(AudioIOSet.SampleRate);
	#endif
	AudioIOSet.PcmWidth	= CFG_DEFAULT_WIDTH;
	if(!AudioCoreSourceInit(&AudioIOSet, SourceId))
	{
		DBG("Usbin source error!\n");
		return FALSE;
	}
	AudioCoreSourceAdjust(SourceId, TRUE);//仅在init通路配置微调后，通路使能时 有效
#endif
#ifdef CFG_OTG_FS_MODE_MIC_EN
	uint8_t Mic_Net = AudioCoreGetNetBySinkChannelNUM(SinkId);
	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
	#if defined(CFG_PARA_AUDIO_USB_FS_OUT_SYNC)//微调  启用硬件时需要和Out协同
		#if 0//暂无HWSRC、双OTG硬件微调必二选一。
			AudioIOSet.Adapt = SRC_ADJUST;
		#else
			AudioIOSet.Adapt = SRC_SRA;
		#endif
	#else
		AudioIOSet.Adapt = SRC_ONLY;
	#endif
	AudioIOSet.SrcQuality = SRC_QUALITY;
	AudioIOSet.Sync = FALSE;//TRUE;//
	AudioIOSet.Channels = MIC_CHANNELS_NUM;
	AudioIOSet.Net = Mic_Net;
	AudioIOSet.DataIOFunc = UsbFSAudioMixMicDataSet;
	AudioIOSet.LenGetFunc = UsbFSAudioMixMicSpaceLenGet;
	AudioIOSet.Depth = UsbFSAudioMixMicDepthGet();
	AudioIOSet.LowLevelCent = 40;
	AudioIOSet.HighLevelCent = 60;

	if(UsbAudioMixMic.AudioSampleRate) //已枚举
	{
		AudioIOSet.SampleRate = UsbAudioMixMic.AudioSampleRate;
	}
	else
	{
		AudioIOSet.SampleRate = USBD_AUDIO_MIC_FREQ1;//初始值
	}
	AudioIOSet.PcmWidth	= CFG_DEFAULT_WIDTH;
	if(!AudioCoreSinkInit(&AudioIOSet, SinkId))
	{
		DBG("Usbout sink error!\n");
	}
	AudioCoreSinkAdjust(SinkId,TRUE);
#endif
	return TRUE;
}
#endif //#if defined CFG_APP_USB_FS_AUDIO_MODE_EN || defined CFG_RES_USB_FS_MIX_AUDIO_EN

#if defined CFG_APP_USB_AUDIO_MODE_EN || defined CFG_RES_USB_HS_MIX_AUDIO_EN || defined CFG_APP_USB_FS_AUDIO_MODE_EN || defined CFG_RES_USB_FS_MIX_AUDIO_EN
//usb声卡模式硬件相关初始化
void UsbAudioDeviceHardwareInit(USB_DeviceType_t dev_type)
{
	if(dev_type == USB_DEVICE_HS)
	{
	#if defined CFG_APP_USB_AUDIO_MODE_EN || defined CFG_RES_USB_HS_MIX_AUDIO_EN
		OTG_DeviceHardwareInit(USB_DEVICE_HS, CFG_PARA_USB_MODE, USB_VID, USBPID(CFG_PARA_USB_MODE));
	#endif
	}
	else if(dev_type == USB_DEVICE_FS)
	{
	#if defined CFG_APP_USB_FS_AUDIO_MODE_EN || defined CFG_RES_USB_FS_MIX_AUDIO_EN
		OTG_DeviceHardwareInit(USB_DEVICE_FS, CFG_PARA_USB_FS_MODE, USB_VID, USBPID(CFG_PARA_USB_FS_MODE));
	#endif
	}
}
#endif
