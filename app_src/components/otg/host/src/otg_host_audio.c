/**
 *****************************************************************************
 * @file     otg_host_audio.c
 * @author   Shanks
 * @version  V1.0.0
 * @date     2024.1.12
 * @brief    host mass-storage module driver interface
 *****************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2024 MVSilicon </center></h2>
 */

#include <string.h>
#include "debug.h"
#include "audio_core_api.h"
#include "otg_host_audio.h"
#include "otg_host_hcd.h"
#include "otg_device_hcd.h"
#include "main_task.h"
#include "irqn.h"
#include "otg_device_audio_api.h"
#include "otg_host_standard_enum.h"
#include "resampler_polyphase.h"
#include "chip_interrupt.h"
#undef  OTG_DBG
#define	OTG_DBG(format, ...)		printf(format, ##__VA_ARGS__)

#ifdef CFG_FUNC_USB_HOST_AUDIO_MIX_MODE

MCU_CIRCULAR_CONTEXT 	UsbAudioOutFiFoBuf;
MCU_CIRCULAR_CONTEXT 	UsbAudioInFiFoBuf;
MCU_CIRCULAR_CONTEXT 	UsbHidInFiFoBuf;
AUDIO_HandleTypeDef UacAudio;
uint8_t *PcmOutFiFoBuf;
uint8_t *PcmInFiFoBuf;
uint8_t *hidInFiFoBuf;

uint8_t *host_iso_buf;	//48K 32Bit 2Chn + 预留

#define	InterfaceDescriptor		((PUSB_INTERFACE_DESCRIPTOR)HostCtx->OtgHostInfo.UsbInterface[i].pData)


void HostUacModuleDebugLog(void)
{
	if(UacAudio.FeedbackSampleRate)
	{
		APP_DBG("FeedbackSampleRate %ld.%ld SpeakerAccumulator %ld\n", UacAudio.FeedbackSampleRate,UacAudio.FeedbackSampleRatemHz,UacAudio.SpeakerAccumulator);
	}
}

uint16_t UacOutPcmDateSet(void *Buffer,uint16_t Len)
{
	MCUCircular_PutData(&UsbAudioOutFiFoBuf, Buffer, Len * sizeof(PCM_DATA_TYPE) * UacAudio.Speaker.NrChannels);
	return Len;
}
uint16_t UacOutPcmDateSpaceLenGet(void)
{
	uint16_t NumSamples = 0;
	NumSamples = MCUCircular_GetSpaceLen(&UsbAudioOutFiFoBuf);
	return NumSamples / (sizeof(PCM_DATA_TYPE) * UacAudio.Speaker.NrChannels);
}

uint16_t UacOutPcmDateDepthGet(void)
{
	uint16_t NumSamples = 0;
	NumSamples = UsbAudioOutFiFoBuf.BufDepth;
	return NumSamples / (sizeof(PCM_DATA_TYPE) * UacAudio.Speaker.NrChannels);
}

uint16_t UacInPcmDateGet(void* Buf, uint16_t Len)
{
	uint16_t Length = 0;//Samples
	Length = MCUCircular_GetDataLen(&UsbAudioInFiFoBuf);
	if(Length > Len * (UacAudio.Mic.NrChannels*sizeof(PCM_DATA_TYPE)))
	{
		Length = Len * (UacAudio.Mic.NrChannels*sizeof(PCM_DATA_TYPE));
	}
	MCUCircular_GetData(&UsbAudioInFiFoBuf, Buf, Length);
    return Length / (UacAudio.Mic.NrChannels*sizeof(PCM_DATA_TYPE));
}
uint16_t UacInPcmDateLenGet(void)
{
	uint16_t NumSamples = 0;
    NumSamples = MCUCircular_GetDataLen(&UsbAudioInFiFoBuf);
	return NumSamples / (UacAudio.Mic.NrChannels*sizeof(PCM_DATA_TYPE));
}
uint16_t UacInPcmDateDepthGet(void)
{
	uint16_t NumSamples = 0;
    NumSamples = UsbAudioInFiFoBuf.BufDepth;
	return NumSamples / (UacAudio.Mic.NrChannels*sizeof(PCM_DATA_TYPE));
}

void OnHostHidRcvIntPacket(void)
{
	uint32_t recvlen;
	uint8_t HidInBuf[65];
#if defined(USB_HOST_AUDIO_MIX_FS)
	OTG_FS_HostEpReadPacket(HOST_INT_IN_EP,(uint8_t*)&HidInBuf[1],UacAudio.Hid.Pipe.MaxPacketSize,&recvlen);
#elif defined(USB_HOST_AUDIO_MIX_HS)
	OTG_HS_HostEpReadPacket(HOST_INT_IN_EP,(uint8_t*)&HidInBuf[1],UacAudio.Hid.Pipe.MaxPacketSize,&recvlen);
#endif
	if(recvlen > 0)
	{
		if(MCUCircular_GetSpaceLen(&UsbHidInFiFoBuf)>recvlen)
		{
			HidInBuf[0] = recvlen;
			MCUCircular_PutData(&UsbHidInFiFoBuf, (uint8_t*)HidInBuf,recvlen+1);
		}
	}
}

void OnHostIsoFeedbackRcvPacket(void)
{
	uint32_t Len;
    uint32_t feedback_value = 0;
#if defined(USB_HOST_AUDIO_MIX_FS)
    OTG_FS_HostEpReadPacket(HOST_ISO_FEEDBACK_IN_EP, (uint8_t*)&feedback_value,UacAudio.Speaker.FeedBackPipe.MaxPacketSize,&Len);
#elif defined(USB_HOST_AUDIO_MIX_HS)
    OTG_HS_HostEpReadPacket(HOST_ISO_FEEDBACK_IN_EP, (uint8_t*)&feedback_value,UacAudio.Speaker.FeedBackPipe.MaxPacketSize,&Len);
#endif

    if(Len < 3) return;

    if(feedback_value == 0) return;

    if(UacAudio.Speaker.FeedBackPipe.MaxPacketSize == 3 || Len == 3)
	{
		// 10.14 格式
		UacAudio.FeedbackRequest = feedback_value << 2;
	}
	else if(UacAudio.Speaker.FeedBackPipe.MaxPacketSize == 4)
	{
		// 16.16 格式
		UacAudio.FeedbackRequest = feedback_value;
	}

//	UacAudio.FeedbackSampleRate = ((uint64_t)UacAudio.FeedbackRequest * UacAudio.TimeBase) >> 16;
//	UacAudio.FeedbackSampleRatemHz = ((uint64_t)UacAudio.FeedbackRequest * UacAudio.TimeBase) & 0xFFFF;
//	UacAudio.FeedbackSampleRatemHz = (UacAudio.FeedbackSampleRatemHz * UacAudio.TimeBase) >> 16;

}


void OnHostAudioRcvIsoPacket(void)
{
	uint32_t Len;
	uint32_t sample = 0;
#if defined(USB_HOST_AUDIO_MIX_FS)
	OTG_FS_HostEpReadPacket(HOST_ISO_IN_EP, host_iso_buf, UacAudio.Mic.Pipe.MaxPacketSize, &Len);
#elif defined(USB_HOST_AUDIO_MIX_HS)
	OTG_HS_HostEpReadPacket(HOST_ISO_IN_EP, host_iso_buf, UacAudio.Mic.Pipe.MaxPacketSize, &Len);
#endif

	sample = Len / UacAudio.Mic.ByteSet;
	UsbAudioPcmIsoToInternal(host_iso_buf, sample, UacAudio.Mic.ByteSet);

	if(MCUCircular_GetSpaceLen(&UsbAudioInFiFoBuf) > sample*sizeof(PCM_DATA_TYPE))
	{
		MCUCircular_PutData(&UsbAudioInFiFoBuf,  (uint8_t*)host_iso_buf, sample*sizeof(PCM_DATA_TYPE));
	}

	if(!AudioCoreSourceIsEnable(USB_HOST_SOURCE_NUM)
			&& !UacAudio.MicStartFlag
			&& MCUCircular_GetDataLen(&UsbAudioInFiFoBuf) >= UsbAudioInFiFoBuf.BufDepth / 2)
	{
//		AudioCoreSourceAdapterReset(USB_HOST_SOURCE_NUM);
		AudioCoreSourceEnable(USB_HOST_SOURCE_NUM);
		UacAudio.MicStartFlag = TRUE;
	}
}

void OnHostAudioSendIsoPacket(void)
{
	uint32_t channel = UacAudio.Speaker.NrChannels;
	uint32_t SendLen = 0;

    if(!UacAudio.Speaker.FeedBackPipe.EpNum)
    {
    	SendLen = UacAudio.Speaker.frequency/UacAudio.SpeakerFramesPerSecond;
    	UacAudio.SpeakerAccumulator += UacAudio.Speaker.frequency%UacAudio.SpeakerFramesPerSecond;
    	if(UacAudio.SpeakerAccumulator >= UacAudio.SpeakerFramesPerSecond)
    	{
    		UacAudio.SpeakerAccumulator -= UacAudio.SpeakerFramesPerSecond;
    		SendLen += 1;
    	}
    }
    else
    {
        UacAudio.SpeakerAccumulator += UacAudio.FeedbackRequest*UacAudio.SpeakerIntervalFrames;
        SendLen = UacAudio.SpeakerAccumulator >> 16;
        UacAudio.SpeakerAccumulator &= 0xFFFF;
    }

	if(!UacAudio.SpeakerStartFlag)
	{
		memset(host_iso_buf,0,SendLen*sizeof(PCM_DATA_TYPE)*channel);
		if(!AudioCoreSinkIsEnable(USB_HOST_SINK_NUM))
		{
//			AudioCoreSinkAdapterReset(USB_HOST_SINK_NUM);
			AudioCoreSinkEnable(USB_HOST_SINK_NUM);
		}
		else if(MCUCircular_GetDataLen(&UsbAudioOutFiFoBuf) > UsbAudioOutFiFoBuf.BufDepth * 45 / 100)
		{
			UacAudio.SpeakerStartFlag = TRUE;
		}
	}
	else
	{
		if(MCUCircular_GetDataLen(&UsbAudioOutFiFoBuf) < SendLen*sizeof(PCM_DATA_TYPE)*channel)
		{
			memset(host_iso_buf,0,SendLen*sizeof(PCM_DATA_TYPE)*channel);
		}
		else
		{
			MCUCircular_GetData(&UsbAudioOutFiFoBuf, host_iso_buf,SendLen*sizeof(PCM_DATA_TYPE)*channel);
		}
	}

	UsbAudioPcmInternalToIso(host_iso_buf, SendLen*channel, UacAudio.Speaker.ByteSet);

#if defined(USB_HOST_AUDIO_MIX_FS)
	OTG_FS_HostEpSendPacket(HOST_ISO_OUT_EP,host_iso_buf,SendLen*UacAudio.Speaker.ByteSet*channel);
#elif defined(USB_HOST_AUDIO_MIX_HS)
	OTG_HS_HostEpSendPacket(HOST_ISO_OUT_EP,host_iso_buf,SendLen*UacAudio.Speaker.ByteSet*channel);
#endif
}

void  HidInputReportCallbackFunction(USBH_HidReportCallbackInfo * data)
{
//	printf("HidInputReportCallbackFunction\n");
//	printf("id : 0x%x\n",data->report_id);
//	printf("usage : 0x%x\n",data->report_usage);
//	printf("value : 0x%x\n",data->report_value);

	static uint8_t VolumeIncrement = 0;
	static uint8_t VolumeDecrement = 0;
	static uint8_t PlayPause = 0;
	static uint8_t NextTrack = 0;
	static uint8_t PrevTrack = 0;

    switch (data->report_usage)
    {
    case HID_CONSUMER<<16|HID_CONSUMER_VOLUME_INCREMENT:
    	if(VolumeIncrement != data->report_value)
    	{
    		VolumeIncrement = data->report_value;
        	if(data->report_value)
        	{
        		printf("VOLUME_INCREMENT DOWN\n");
    			USBH_AudioSetVolumeIncrement(&UacAudio.Speaker,5);	//Increment 5%
        	}
        	else
        	{
        		printf("VOLUME_INCREMENT UP\n");
        	}
    	}
        break;
    case HID_CONSUMER<<16|HID_CONSUMER_VOLUME_DECREMENT:
		if(VolumeDecrement != data->report_value)
		{
			VolumeDecrement = data->report_value;
			if(data->report_value)
			{
				printf("VOLUME_DECREMENT DOWN\n");
				USBH_AudioSetVolumeDecrement(&UacAudio.Speaker,5);	// Decrement 5%
			}
			else
			{
				printf("VOLUME_DECREMENT UP\n");
			}
		}
        break;
    case HID_CONSUMER<<16|HID_CONSUMER_PLAY_PAUSE:
		if(PlayPause != data->report_value)
		{
			PlayPause = data->report_value;
			if(data->report_value)
			{
				printf("PLAY_PAUSE DOWN\n");
				SendSysModeMsg(MSG_PLAY_PAUSE);
			}
			else
			{
				printf("PLAY_PAUSE UP\n");
			}
		}
        break;
    case HID_CONSUMER<<16|HID_CONSUMER_NEXT_TRACK:
    	if(NextTrack != data->report_value)
    	{
    		NextTrack = data->report_value;
        	if(data->report_value)
        	{
        		printf("NEXT_TRACK DOWN\n");
        		SendSysModeMsg(MSG_NEXT);
        	}
        	else
        	{
        		printf("NEXT_TRACK UP\n");
        	}
    	}
        break;
    case HID_CONSUMER<<16|HID_CONSUMER_PREV_TRACK:
		if(PrevTrack != data->report_value)
		{
			PrevTrack = data->report_value;
			if(data->report_value)
			{
				printf("PREV_TRACK DOWN\n");
				SendSysModeMsg(MSG_PRE);
			}
			else
			{
				printf("PREV_TRACK UP\n");
			}
		}
        break;
    default:
        break;
    }
}

void UsbHostHidDateProcess(void)
{
	uint8_t Length = 0;//Samples
	uint8_t hidLength = 0;//Samples
	uint8_t Buf[65];
	Length = MCUCircular_GetDataLen(&UsbHidInFiFoBuf);
	if(Length > 0)
	{
		hidLength = UsbHidInFiFoBuf.CircularBuf[UsbHidInFiFoBuf.R];
		if(Length >= hidLength + 1)
		{
			Length = hidLength + 1;
			MCUCircular_GetData(&UsbHidInFiFoBuf, &Buf, Length);
//			int i;
//			APP_DBG("HID_LEN: %d HID_DATA:",Buf[0]);
//			for(i=1;i<Length;i++)
//			{
//				APP_DBG("%02x ",Buf[i]);
//			}
//			APP_DBG("\n");
			USBH_HidInputReportDataDecode(&Buf[1],hidLength);
		}
	}
}

USBH_StatusTypeDef USBH_AudioSampleRateCheck(uint32_t FreqIn,uint32_t FreqOut)
{
	extern RESAMPLER_POLYPHASE_SRC_RATIO GetRatioEnum(uint32_t Scale1000);
	RESAMPLER_POLYPHASE_SRC_RATIO ret;

	ret = GetRatioEnum(1000 * FreqOut / FreqIn);
	APP_DBG("FreqIn %ld,FreqOut %ld GetRatioEnum %d\n",FreqIn,FreqOut,ret);

	if(ret == RESAMPLER_POLYPHASE_SRC_RATIO_UNSUPPORTED)
	{
		return USBH_NOT_SUPPORTED;
	}
	return USBH_OK;
}

bool UsbHostPlayHardwareInit(HOST_DEV_TYPE dev,HostCtx_t *HostCtx)
{
	uint32_t i;
	uint16_t IsoBufMaxLen = 0;
	//跳过8BIT
	//优先级1: ByteSet 和 采样率都匹配
	//优先级2: 仅采样率匹配（ByteSet不要求）
	//优先级3: ByteSet匹配，采样率最接近的
	//优先级4：采样率最接近的（ByteSet不要求）
	//默认参数配置 -- 按照优先级选择声卡配置。
	UacAudio.Mic.ByteSet = PCM16BIT;
	UacAudio.Mic.frequency = AudioCore.NetCt[DefaultNet].SampleRate;

	UacAudio.Speaker.ByteSet = PCM16BIT;
	UacAudio.Speaker.frequency = AudioCore.NetCt[DefaultNet].SampleRate;

	//采样率转换支持检测回调
	USBH_AudioSampleRateCheckCallbackSet(USBH_AudioSampleRateCheck);
	USBH_RegisterControlCallback(HostCtx->bus_ops.HostControlWrite,HostCtx->bus_ops.HostControlRead);

	//声卡枚举
	if(USBH_AudioClassDevEnum(&HostCtx->OtgHostInfo,&UacAudio.Speaker,&UacAudio.Mic) != USBH_OK)
	{
		OTG_DBG("声卡不支持\n");
		return FALSE;
	}
	if(UacAudio.Speaker.supported)
	{
		USBH_AudioSetVolume(&UacAudio.Speaker,100);	//100%
	//	USBH_AudioSetVolumeDB(&UacAudio.Speaker,0);	//0db

		IsoBufMaxLen = USBH_ISO_FRAME_SIZE(UacAudio.Speaker.Pipe.MaxPacketSize);
	}
	if(UacAudio.Mic.supported)
	{
		USBH_AudioSetVolume(&UacAudio.Mic,100);		//100%
	//	USBH_AudioSetVolumeDB(&UacAudio.Mic,0);		//0db

		IsoBufMaxLen = MAX(USBH_ISO_FRAME_SIZE(UacAudio.Mic.Pipe.MaxPacketSize),IsoBufMaxLen);
	}

	if(!UacAudio.Speaker.supported && !UacAudio.Mic.supported)
	{
		OTG_DBG("声卡端点不支持\n");
		return FALSE;
	}

	OTG_DBG("IsoBufMaxLen %d\n",IsoBufMaxLen);
	host_iso_buf = (uint8_t*)osPortMalloc(IsoBufMaxLen*2);
	if(host_iso_buf == NULL)
	{
		OTG_DBG("host_iso_buf == NULL\n");
		return FALSE;
	}

	//HID枚举
	USBH_HidDescriptorParse(&HostCtx->OtgHostInfo,&UacAudio.Hid);

	if(!UacAudio.Hid.HidInterfaceNum)
	{
		UacAudio.Hid.supported = 0;
		return TRUE;
	}
	OTG_DBG("UacAudio.Hid.wDescriptorLength %d\n",UacAudio.Hid.wDescriptorLength);
	UacAudio.Hid.ReportDescriptor = (uint8_t*)osPortMalloc(UacAudio.Hid.wDescriptorLength);
	if(UacAudio.Hid.ReportDescriptor == NULL)
	{
		OTG_DBG("UacAudio.Hid.Descriptor == NULL\n");
		return TRUE;
	}
	for(i = 0;i<UacAudio.Hid.HidInterfaceNum;i++)
	{
		if(UacAudio.Hid.Interface[i].supported == 1)
		{
			USBH_HidSetIdle(UacAudio.Hid.Interface[i].interface);
			USBH_HidGetReport(UacAudio.Hid.Interface[i].interface,UacAudio.Hid.ReportDescriptor,UacAudio.Hid.Interface[i].wDescriptorLength);
			USBH_HidReportDescriptorParse(UacAudio.Hid.ReportDescriptor,UacAudio.Hid.Interface[i].wDescriptorLength);
			if(USBH_HidInputReportHasUsage(HID_CONSUMER))	//寻找 Usage Page (Consumer)
			{
				USBH_HidInputReportCallbackRegister(HidInputReportCallbackFunction);
				memcpy(&UacAudio.Hid.Pipe,&UacAudio.Hid.Interface[i].Pipe,sizeof(PIPE_INFO));
				UacAudio.Hid.supported = 1;
				break;
			}
		}
	}
	return TRUE;
}
void UsbHostPlayStart(void)
{
	#if defined(USB_HOST_AUDIO_MIX_FS)
		CoreInterruptEnable(INT_USB_FS);
		NVIC_EnableIRQ(USB_FS_WAKEUP_IRQn);
	#endif
	if(UacAudio.Hid.supported == 1)
	{
	#if defined(USB_HOST_AUDIO_MIX_FS)
		OTG_FS_EndpointInterruptEnable(HOST_INT_IN_EP,OnHostHidRcvIntPacket);
	#elif defined(USB_HOST_AUDIO_MIX_HS)
		OTG_HS_EndpointInterruptEnable(HOST_INT_IN_EP,OnHostHidRcvIntPacket);
	#endif
		OTG_DBG("Hid.Pipe.EpNum %d\n",UacAudio.Hid.Pipe.EpNum);
		OTG_DBG("Hid.Pipe.EpType %d\n",UacAudio.Hid.Pipe.EpType);
		OTG_DBG("Hid.Pipe.MaxPacketSize %d\n",UacAudio.Hid.Pipe.MaxPacketSize);
		OTG_DBG("Hid.Pipe.bInterval %d\n",UacAudio.Hid.Pipe.bInterval);
	#if defined(USB_HOST_AUDIO_MIX_FS)
		OTG_FS_HostEpReadStart(HOST_INT_IN_EP,&UacAudio.Hid.Pipe);
	#elif defined(USB_HOST_AUDIO_MIX_HS)
		OTG_HS_HostEpReadStart(HOST_INT_IN_EP,&UacAudio.Hid.Pipe);
	#endif
	}

	if(UacAudio.Mic.supported == 1)
	{
	#if defined(USB_HOST_AUDIO_MIX_FS)
		OTG_FS_EndpointInterruptEnable(HOST_ISO_IN_EP,OnHostAudioRcvIsoPacket);
	#elif defined(USB_HOST_AUDIO_MIX_HS)
		OTG_HS_EndpointInterruptEnable(HOST_ISO_IN_EP,OnHostAudioRcvIsoPacket);
	#endif
		OTG_DBG("Mic.Pipe.EpNum %x\n",UacAudio.Mic.Pipe.EpNum);
		OTG_DBG("Mic.Pipe.EpType %d\n",UacAudio.Mic.Pipe.EpType);
		OTG_DBG("Mic.Pipe.MaxPacketSize %d\n",UacAudio.Mic.Pipe.MaxPacketSize);
		OTG_DBG("Mic.Pipe.bInterval %d\n",UacAudio.Mic.Pipe.bInterval);
		OTG_DBG("Mic.AltSettings %d\n",UacAudio.Mic.AltSettings);
		OTG_DBG("Mic.ByteSet %d\n",UacAudio.Mic.ByteSet);
//		UacAudio.Mic.Pipe.MaxPacketSize = 288; //bp10芯片需要
	#if defined(USB_HOST_AUDIO_MIX_FS)
		OTG_FS_HostEpReadStart(HOST_ISO_IN_EP,&UacAudio.Mic.Pipe);
	#elif defined(USB_HOST_AUDIO_MIX_HS)
		OTG_HS_HostEpReadStart(HOST_ISO_IN_EP,&UacAudio.Mic.Pipe);
	#endif
	}

	if(UacAudio.Speaker.supported == 1)
	{
	#if defined(USB_HOST_AUDIO_MIX_FS)
		OTG_FS_EndpointInterruptEnable(HOST_ISO_OUT_EP|0x80,OnHostAudioSendIsoPacket);
	#elif defined(USB_HOST_AUDIO_MIX_HS)
		OTG_HS_EndpointInterruptEnable(HOST_ISO_OUT_EP|0x80,OnHostAudioSendIsoPacket);
	#endif

	#if defined(USB_HOST_AUDIO_MIX_FS)
		UacAudio.PortSpeed = 2;
		UacAudio.TimeBase = 1000;
		UacAudio.SpeakerIntervalFrames = 1;
		UacAudio.SpeakerFramesPerSecond = 1000;
	#elif defined(USB_HOST_AUDIO_MIX_HS)
		UacAudio.PortSpeed = OTG_HS_HostPortSpeedGet();
		if(UacAudio.PortSpeed == 3)		//HS设备
		{
			UacAudio.SpeakerIntervalFrames = (1<<(UacAudio.Speaker.Pipe.bInterval-1));	//帧间隔
			UacAudio.TimeBase = 8000;
		}
		else if(UacAudio.PortSpeed == 2) //FS设备
		{
			UacAudio.SpeakerIntervalFrames = UacAudio.Speaker.Pipe.bInterval;			//帧间隔
			UacAudio.TimeBase = 1000;
		}
		UacAudio.SpeakerFramesPerSecond = UacAudio.TimeBase/UacAudio.Speaker.Pipe.bInterval;		//帧每秒
	#endif
		OTG_DBG("SpeakerIntervalFrames %d\n",UacAudio.SpeakerIntervalFrames);
		OTG_DBG("SpeakerFramesPerSecond %d\n",UacAudio.SpeakerFramesPerSecond);

		OTG_DBG("Speaker.Pipe.EpNum %x\n",UacAudio.Speaker.Pipe.EpNum);
		OTG_DBG("Speaker.Pipe.EpType %d\n",UacAudio.Speaker.Pipe.EpType);
		OTG_DBG("Speaker.Pipe.MaxPacketSize %d\n",UacAudio.Speaker.Pipe.MaxPacketSize);
		OTG_DBG("Speaker.Pipe.bInterval %d\n",UacAudio.Speaker.Pipe.bInterval);
		OTG_DBG("Speaker.AltSettings %d\n",UacAudio.Speaker.AltSettings);
		OTG_DBG("Speaker.ByteSet %d\n",UacAudio.Speaker.ByteSet);
	#if defined(USB_HOST_AUDIO_MIX_FS)
		OTG_FS_HostEpSendStart(HOST_ISO_OUT_EP,&UacAudio.Speaker.Pipe);
		OTG_FS_HostEpSendPacket(HOST_ISO_OUT_EP,host_iso_buf,UacAudio.Speaker.frequency/UacAudio.SpeakerFramesPerSecond*UacAudio.Speaker.ByteSet*UacAudio.Speaker.NrChannels);
	#elif defined(USB_HOST_AUDIO_MIX_HS)
		OTG_HS_HostEpSendStart(HOST_ISO_OUT_EP,&UacAudio.Speaker.Pipe);
		OTG_HS_HostEpSendPacket(HOST_ISO_OUT_EP,host_iso_buf,UacAudio.Speaker.frequency/UacAudio.SpeakerFramesPerSecond*UacAudio.Speaker.ByteSet*UacAudio.Speaker.NrChannels);
	#endif

		if(UacAudio.Speaker.FeedBackPipe.EpNum != 0)
		{
			UacAudio.FeedbackRequest = ((uint64_t)UacAudio.Speaker.frequency << 16) / UacAudio.TimeBase;

		#if defined(USB_HOST_AUDIO_MIX_FS)
			OTG_FS_EndpointInterruptEnable(HOST_ISO_FEEDBACK_IN_EP,OnHostIsoFeedbackRcvPacket);
		#elif defined(USB_HOST_AUDIO_MIX_HS)
			OTG_HS_EndpointInterruptEnable(HOST_ISO_FEEDBACK_IN_EP,OnHostIsoFeedbackRcvPacket);
		#endif
			OTG_DBG("FeedBackPipe.EpNum %x\n",UacAudio.Speaker.FeedBackPipe.EpNum);
			OTG_DBG("FeedBackPipe.EpType %d\n",UacAudio.Speaker.FeedBackPipe.EpType);
			OTG_DBG("FeedBackPipe.MaxPacketSize %d\n",UacAudio.Speaker.FeedBackPipe.MaxPacketSize);
			OTG_DBG("FeedBackPipe.bInterval %d\n",UacAudio.Speaker.FeedBackPipe.bInterval);
		#if defined(USB_HOST_AUDIO_MIX_FS)
			OTG_FS_HostEpReadStart(HOST_ISO_FEEDBACK_IN_EP,&UacAudio.Speaker.FeedBackPipe);
		#elif defined(USB_HOST_AUDIO_MIX_HS)
			OTG_HS_HostEpReadStart(HOST_ISO_FEEDBACK_IN_EP,&UacAudio.Speaker.FeedBackPipe);
		#endif
		}
	}
}
void UsbHostPlayResInit(void)
{
	AudioCoreIO	AudioIOSet;
	if(UacAudio.Mic.supported == 1)
	{
		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
		if(UacAudio.Mic.ResamplerEn)
		{
			AudioIOSet.Adapt = SRC_SRA;//SRC_ADJUST;
		}
		else
		{
			AudioIOSet.Adapt = SRA_ONLY;//STD;
		}
		AudioIOSet.SrcQuality = SRC_QUALITY;
		AudioIOSet.Sync = FALSE;
		AudioIOSet.Channels = UacAudio.Mic.NrChannels;
		AudioIOSet.Net = AudioCoreGetNetBySourceChannelNUM(USB_HOST_SOURCE_NUM);
		AudioIOSet.DataIOFunc = UacInPcmDateGet;
		AudioIOSet.LenGetFunc = UacInPcmDateLenGet;
		AudioIOSet.Depth = UacInPcmDateDepthGet();
		AudioIOSet.LowLevelCent = 40;
		AudioIOSet.HighLevelCent = 60;
		AudioIOSet.SampleRate = UacAudio.Mic.frequency;
		DBG("USB_HOST_SOURCE_NUM SampleRate %ld\n",AudioIOSet.SampleRate);
		AudioIOSet.PcmWidth	= CFG_DEFAULT_WIDTH;
		if(!AudioCoreSourceInit(&AudioIOSet, USB_HOST_SOURCE_NUM))
		{
			DBG("Usbin source error!\n");
		}
		AudioCoreSourceAdjust(USB_HOST_SOURCE_NUM, TRUE);//仅在init通路配置微调后，通路使能时 有效
	}

	if(UacAudio.Speaker.supported == 1)
	{
		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
		if(UacAudio.Speaker.ResamplerEn)
		{
			AudioIOSet.Adapt = SRC_SRA;//SRC_ADJUST;
		}
		else
		{
			AudioIOSet.Adapt = SRA_ONLY;//STD;
		}
		AudioIOSet.SrcQuality = SRC_QUALITY;
		AudioIOSet.Sync = FALSE;
		AudioIOSet.Channels = UacAudio.Speaker.NrChannels;
		AudioIOSet.Net = AudioCoreGetNetBySinkChannelNUM(USB_HOST_SINK_NUM);
		AudioIOSet.DataIOFunc = UacOutPcmDateSet;
		AudioIOSet.LenGetFunc = UacOutPcmDateSpaceLenGet;
		AudioIOSet.Depth = UacOutPcmDateDepthGet();
		AudioIOSet.LowLevelCent = 40;
		AudioIOSet.HighLevelCent = 60;
		AudioIOSet.SampleRate = UacAudio.Speaker.frequency;
		DBG("USB_HOST_SINK_NUM SampleRate %ld\n",AudioIOSet.SampleRate);
		AudioIOSet.PcmWidth	= CFG_DEFAULT_WIDTH;
		if(!AudioCoreSinkInit(&AudioIOSet, USB_HOST_SINK_NUM))
		{
			DBG("Usbout sink error!\n");
		}
		AudioCoreSinkAdjust(USB_HOST_SINK_NUM,TRUE);
	}
}
bool UsbHostPlayResMalloc(uint16_t SampleLen)
{
	APP_DBG("UsbHostPlayResMalloc %u\n", SampleLen);
	if(UacAudio.Mic.supported == 1)
	{
		PcmInFiFoBuf = (uint8_t*)osPortMalloc(SampleLen * sizeof(PCM_DATA_TYPE) * 2 * 3);
		if(PcmInFiFoBuf == NULL)
		{
			APP_DBG("PcmInFiFoBuf memory error\n");
			return FALSE;
		}
		memset(PcmInFiFoBuf, 0, SampleLen *  sizeof(PCM_DATA_TYPE) * 2 * 3);
		MCUCircular_Config(&UsbAudioInFiFoBuf, PcmInFiFoBuf, SampleLen * sizeof(PCM_DATA_TYPE) * 2 * 3);
	}
	if(UacAudio.Speaker.supported == 1)
	{
		PcmOutFiFoBuf = (uint8_t*)osPortMalloc(SampleLen * sizeof(PCM_DATA_TYPE) * 2 * 3);
		if(PcmOutFiFoBuf == NULL)
		{
			APP_DBG("PcmOutFiFoBuf memory error\n");
			return FALSE;
		}
		memset(PcmOutFiFoBuf, 0, SampleLen * sizeof(PCM_DATA_TYPE) * 2 * 3);
		MCUCircular_Config(&UsbAudioOutFiFoBuf, PcmOutFiFoBuf, SampleLen * sizeof(PCM_DATA_TYPE) * 2 * 3);
	}

	if(UacAudio.Hid.supported == 1)
	{
		hidInFiFoBuf = (uint8_t*)osPortMalloc(64);
		if(hidInFiFoBuf == NULL)
		{
			APP_DBG("PcmOutFiFoBuf memory error\n");
			return FALSE;
		}
		memset(hidInFiFoBuf, 0, 64);
		MCUCircular_Config(&UsbHidInFiFoBuf, hidInFiFoBuf,64);
	}
	return TRUE;
}

void UsbHostPlayResRelease(void)
{
	if(PcmOutFiFoBuf != NULL)
	{
		APP_DBG("PcmOutFiFoBuf\n");
		osPortFree(PcmOutFiFoBuf);
		PcmOutFiFoBuf = NULL;
	}
	if(PcmInFiFoBuf != NULL)
	{
		APP_DBG("PcmInFiFoBuf\n");
		osPortFree(PcmInFiFoBuf);
		PcmInFiFoBuf = NULL;
	}
	if(host_iso_buf != NULL)
	{
		APP_DBG("host_iso_buf\n");
		osPortFree(host_iso_buf);
		host_iso_buf = NULL;
	}
	if(UacAudio.Hid.ReportDescriptor != NULL)
	{
		APP_DBG("Hid.Descriptor\n");
		osPortFree(UacAudio.Hid.ReportDescriptor);
		UacAudio.Hid.ReportDescriptor = NULL;
	}
	if(hidInFiFoBuf != NULL)
	{
		APP_DBG("hidInFiFoBuf\n");
		osPortFree(hidInFiFoBuf);
		hidInFiFoBuf = NULL;
	}
	memset(&UacAudio,0,sizeof(UacAudio));
	APP_DBG("UsbHostPlayResRelease\n");
}
bool UsbHostPlayMixInit(void)
{
	if(!UsbHostPlayResMalloc(AudioCoreFrameSizeGet(DefaultNet)))
	{
		APP_DBG("UsbHostPlayResMalloc Res Error!\n");
		return FALSE;
	}
	UsbHostPlayResInit();

	AudioCoreSourceUnmute(USB_HOST_SOURCE_NUM,TRUE,TRUE);
	return TRUE;
}
bool UsbHostPlayMixDeinit(void)
{
	APP_DBG("Usb Host Play Mix Deinit\n");
#if defined(USB_HOST_AUDIO_MIX_FS)
	CoreInterruptDisable(INT_USB_FS);
	NVIC_DisableIRQ(USB_FS_WAKEUP_IRQn);//有多中断源需要注意，是否直接关掉内核中断源
	OTG_FS_EndpointInterruptDisable(HOST_INT_IN_EP);
	OTG_FS_EndpointInterruptDisable(HOST_ISO_IN_EP);
	OTG_FS_EndpointInterruptDisable(HOST_ISO_OUT_EP);

#elif defined(USB_HOST_AUDIO_MIX_HS)
	OTG_HS_EndpointInterruptDisable(HOST_INT_IN_EP);
	OTG_HS_EndpointInterruptDisable(HOST_ISO_IN_EP);
	OTG_HS_EndpointInterruptDisable(HOST_ISO_OUT_EP);

#endif

	AudioCoreSourceMute(USB_HOST_SOURCE_NUM,TRUE,TRUE);
	AudioCoreSourceDisable(USB_HOST_SOURCE_NUM);
	AudioCoreSinkDisable(USB_HOST_SINK_NUM);

	AudioCoreSourceDeinit(USB_HOST_SOURCE_NUM);
	AudioCoreSinkDeinit(USB_HOST_SINK_NUM);
	UsbHostPlayResRelease();
	return TRUE;
}
#endif
