/**
 *****************************************************************************
 * @file     otg_device_audio_hs.c
 * @author   Shanks
 * @version  V1.0.0
 * @date     7-September-2015
 * @brief    USB OTG HS(High-Speed) 高速音频设备驱动
 *****************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
 *****************************************************************************
 * @brief  文件功能说明：
 *  1. 基于 USB High-Speed 实现 UAC 高速音频设备驱动
 *  2. 支持 Speaker 播放 + Mic 录音双向ISO同步数据传输
 *  3. 支持硬件SRC采样率自适应转换（播放/录音）
 *  4. 支持 16bit/24bit/32bit PCM 格式自动转换与位深适配
 *  5. 环形缓冲区实现USB与音频系统的数据缓冲与同步
 *  6. 支持音量调节、静音控制、增益淡入淡出处理
 *  7. 支持多AltSet模式、多通道、多采样率动态切换
 *  8. 音频流启停、缓存水位自动流控
 *****************************************************************************
 */
//driver
#include <string.h>
#include <math.h>
#include "otg_device_hcd.h"
#include "core_d1088.h"
#include "src.h"
#include "reset.h"
#include "src_interface.h"

//application
#include "app_config.h"
#include "audio_core_api.h"
#include "otg_device_audio_api.h"
#include "otg_device.h"
#include "debug.h"
#include "rtos_api.h"
#include "audio_effect_control.h"

#if defined CFG_FUNC_USB_SPEAKER_HW_SRC_EN || defined CFG_FUNC_USB_MIC_HW_SRC_EN
	#ifdef CFG_FUNC_USB_SPEAKER_HW_SRC_EN
	SRC_Context USBSpeakersrcContext;
	bool IsSpeakerSRCFlag = 0;
	#endif

	#ifdef CFG_FUNC_USB_MIC_HW_SRC_EN
	SRC_Context USBMicsrcContext;
	bool IsMicSRCFlag = 0;
	#endif
#endif
#if defined CFG_APP_USB_AUDIO_MODE_EN || defined CFG_COMMUNICATION_BY_HS_USB || defined CFG_RES_USB_HS_MIX_AUDIO_EN

#ifdef CFG_OTG_MODE_MIC_EN
	UsbAudio UsbAudioMic;
	const uint8_t MicByteSet[] = {0,MIC_ALT1_BITS,MIC_ALT2_BITS,MIC_ALT3_BITS};
#endif
#ifdef CFG_OTG_MODE_AUDIO_EN
	UsbAudio UsbAudioSpeaker;
	const uint8_t SpeakerByteSet[] = {0,SPEAKER_ALT1_BITS,SPEAKER_ALT2_BITS,SPEAKER_ALT3_BITS};
#endif
#ifdef CFG_OTG_MODE_AUDIO1_EN
	UsbAudio UsbAudioSpeaker1;
	const uint8_t Speaker1ByteSet[] = {0,SPEAKER1_ALT1_BITS,SPEAKER1_ALT2_BITS,SPEAKER_ALT3_BITS};
#endif
static uint8_t iso_buf[PCM_LEN_MAX];
static uint8_t AudioCmd[3] = {1,0,0};
/**
 * @brief  USB声卡模式下，发送反向控制命令
 * @param  Cmd 反向控制命令
 * @return 1-成功，0-失败
 */
bool OTG_HS_DeviceAudioSendPcCmd(uint16_t Cmd)
{
	AudioCmd[1] = Cmd;
	AudioCmd[2] = Cmd >> 8;
	OTG_HS_DeviceInterruptSend(DEVICE_INT_IN_EP,AudioCmd, 3,1);
	AudioCmd[1] = 0;
	AudioCmd[2] = 0;
	OTG_HS_DeviceInterruptSend(DEVICE_INT_IN_EP,AudioCmd, 3,1);
	return TRUE;
}
/**
 * @brief  发送 HID 按键按下事件（反向控制命令）
 * @param  Cmd 按键命令码
 * @return 1-成功，0-失败
 */
bool OTG_HS_DeviceAudioSendHidKeyDown(uint16_t Cmd)
{
	AudioCmd[1] = Cmd;
	AudioCmd[2] = Cmd >> 8;
	OTG_HS_DeviceInterruptSend(DEVICE_INT_IN_EP,AudioCmd, 3,1);
	return TRUE;
}
/**
 * @brief  发送 HID 按键释放事件（清空反向控制命令）
 * @return 1-成功，0-失败
 */
bool OTG_HS_DeviceAudioSendHidKeyUp(void)
{
	AudioCmd[1] = 0;
	AudioCmd[2] = 0;
	OTG_HS_DeviceInterruptSend(DEVICE_INT_IN_EP,AudioCmd, 3,1);
	return TRUE;
}

#ifdef CFG_OTG_MODE_AUDIO1_EN
/**
 * @brief  从 Speaker1 音频缓冲区读取 PCM 数据（PC→芯片）
 * @param  Buffer 输出数据缓冲区指针
 * @param  Len    期望读取的 PCM 样本数（每通道）
 * @return 实际读取的 PCM 样本数（每通道）
 */
//pc->chip 从缓存区获取数据
uint16_t UsbAudioSpeaker1DataGet(void *Buffer, uint16_t Len)
{
	uint16_t ret = UsbAudioBufDataGet(&UsbAudioSpeaker1, Buffer, Len);
#ifdef CFG_RES_AUDIO_USB_VOL_SET_EN
	UsbAudioGainApply(&UsbAudioSpeaker1, Buffer, ret);
#endif
	return ret;
}

/**
 * @brief  获取 Speaker1 音频缓冲区中可读 PCM 样本数（每通道）
 * @return 可读 PCM 样本数（每通道）
 */
//pc->chip 获取缓存区数据长度
uint16_t UsbAudioSpeaker1DataLenGet(void)
{
	return UsbAudioBufDataLen(&UsbAudioSpeaker1);
}
/**
 * @brief  获取 Speaker1 音频缓冲区深度（总容量）
 * @return 缓冲区深度（采样点数）
 */
uint16_t UsbAudioSpeaker1DepthGet(void)
{
	return UsbAudioBufDepth(&UsbAudioSpeaker1, PACKET1_CHANNELS_NUM);
}
#endif

#ifdef CFG_OTG_MODE_AUDIO_EN
/**
 * @brief  从 Speaker 音频缓冲区读取 PCM 数据（PC→芯片）
 * @param  Buffer 输出数据缓冲区指针
 * @param  Len    期望读取的 PCM 样本数（每通道）
 * @return 实际读取的 PCM 样本数（每通道）
 */
//pc->chip 从缓存区获取数据
uint16_t UsbAudioSpeakerDataGet(void *Buffer, uint16_t Len)
{
	uint16_t ret = UsbAudioBufDataGet(&UsbAudioSpeaker, Buffer, Len);
#ifdef CFG_RES_AUDIO_USB_VOL_SET_EN
	UsbAudioGainApply(&UsbAudioSpeaker, Buffer, ret);
#endif
	return ret;
}

/**
 * @brief  获取 Speaker 音频缓冲区中可读 PCM 样本数（每通道）
 * @return 可读 PCM 样本数（每通道）
 */
//pc->chip 获取缓存区数据长度
uint16_t UsbAudioSpeakerDataLenGet(void)
{
	return UsbAudioBufDataLen(&UsbAudioSpeaker);
}
/**
 * @brief  获取 Speaker 音频缓冲区深度（总容量）
 * @return 缓冲区深度（采样点数）
 */
uint16_t UsbAudioSpeakerDepthGet(void)
{
	return UsbAudioBufDepth(&UsbAudioSpeaker, PACKET_CHANNELS_NUM);
}
#endif

#ifdef CFG_OTG_MODE_MIC_EN
/**
 * @brief  将 Mic PCM 数据写入音频缓冲区（芯片→PC）
 * @param  Buffer 输入数据缓冲区指针
 * @param  Len    待写入的 PCM 样本数（每通道）
 * @return 实际写入的 PCM 样本数（每通道）
 */
//chip->pc 保存数据到缓存区
uint16_t UsbAudioMicDataSet(void *Buffer, uint16_t Len)
{
#ifdef CFG_RES_AUDIO_USB_VOL_SET_EN
	UsbAudioGainApply(&UsbAudioMic, Buffer, Len);
#endif

#ifdef CFG_FUNC_USB_MIC_HW_SRC_EN
	if(IsMicSRCFlag)
	{
		if(!SRC_IsBlockDone(CFG_USB_MIC_HW_SRC_PORT))
			osTaskDelay(1);
		if(SRC_IsBlockDone(CFG_USB_MIC_HW_SRC_PORT))
		{
			uint32_t OutLen = SRC_BlockApplyDone((SRC_MODULE)CFG_USB_MIC_HW_SRC_PORT, &USBMicsrcContext, (int16_t*)USBMicsrcContext.OutBuf);

			UsbAudioBufDataSet(&UsbAudioMic, (uint8_t*)USBMicsrcContext.OutBuf, OutLen);
		}
		else
			DBG("SRCLost\n");
		SRC_BlockLenSet(CFG_USB_MIC_HW_SRC_PORT, Len);
		SRC_BlockApplyStart(CFG_USB_MIC_HW_SRC_PORT, &USBMicsrcContext, (int16_t*)Buffer, Len);
	}
	else
#endif
	{
		UsbAudioBufDataSet(&UsbAudioMic, Buffer, Len);
	}


	return Len;
}

/**
 * @brief  获取 Mic 音频缓冲区剩余可用 PCM 样本数（每通道）
 * @return 可用 PCM 样本数（每通道）
 */
//chip->pc 数据缓存区剩余空间
uint16_t UsbAudioMicSpaceLenGet(void)
{
	return UsbAudioBufSpaceLen(&UsbAudioMic);
}

/**
 * @brief  获取 Mic 音频缓冲区深度（总容量）
 * @return 缓冲区深度（采样点数）
 */
uint16_t UsbAudioMicDepthGet(void)
{
	return UsbAudioBufDepth(&UsbAudioMic, MIC_CHANNELS_NUM);
}
#endif

#ifdef CFG_OTG_MODE_AUDIO1_EN
/**
 * @brief  Speaker1 ISO 输出端点接收回调
 *         接收 PC 端音频 ISO 数据包，转为内部 PCM 格式写入 Speaker1 缓冲区
 */
//注意一下需要4字节对齐
__attribute__((section(".tcm_section")))
void OnDeviceAudioRcvIsoPacket1(void)
{
	uint32_t Len;
	int32_t s;
	uint32_t PcmNum = 0;//= sample * channels
//	uint32_t SamplesNum = 0;
	OTG_HS_DeviceISOReceive(DEVICE_ISO_OUT_EP1, (uint8_t*)iso_buf, sizeof(iso_buf), &Len);
	if(Len == 0 || !AudioCoreSourceIsInit(USB_AUDIO_SOURCE1_NUM)) return;
	if(UsbAudioSpeaker1.FramCount == 0)
	{
		UsbAudioSpeaker1.FirstThrhld = FALSE;
	}
	UsbAudioSpeaker1.FramCount++;

	PcmNum = Len/UsbAudioSpeaker1.ByteSet;
	UsbAudioPcmIsoToInternal(iso_buf, PcmNum, UsbAudioSpeaker1.ByteSet);
	UsbAudioBufDataSet(&UsbAudioSpeaker1, (uint8_t*)iso_buf, PcmNum / UsbAudioSpeaker1.Channels);

	if(!AudioCoreSourceIsEnable(USB_AUDIO_SOURCE1_NUM)
			&& !UsbAudioSpeaker1.FirstThrhld
			&& MCUCircular_GetDataLen(&UsbAudioSpeaker1.CircularBuf) >= UsbAudioSpeaker1.CircularBuf.BufDepth / 2)
	{
//		AudioCoreSourceAdapterReset(USB_AUDIO_SOURCE1_NUM);
		AudioCoreSourceEnable(USB_AUDIO_SOURCE1_NUM);
		UsbAudioSpeaker1.FirstThrhld = TRUE;
	}
}
#endif

#ifdef CFG_OTG_MODE_AUDIO_EN
/**
 * @brief  Speaker ISO 输出端点接收回调
 *         接收 PC 端音频 ISO 数据包，转为内部 PCM 格式写入 Speaker 缓冲区
 *         支持硬件 SRC 采样率自适应转换
 */
__attribute__((section(".tcm_section")))
void OnDeviceHsAudioRcvIsoPacket(void)
{
	uint32_t Len;
	uint32_t PcmNum = 0;//= sample * channels

	OTG_HS_DeviceISOReceive(DEVICE_ISO_OUT_EP, (uint8_t*)iso_buf, sizeof(iso_buf), &Len);
	if(Len == 0 || !AudioCoreSourceIsInit(USB_AUDIO_SOURCE_NUM)) return;
	if(UsbAudioSpeaker.FramCount == 0)
	{
		UsbAudioSpeaker.FirstThrhld = FALSE;
	}
	UsbAudioSpeaker.FramCount++;
	PcmNum = Len/UsbAudioSpeaker.ByteSet;
	//从USB接收调整PCM数据格式（USB->内部）
	UsbAudioPcmIsoToInternal(iso_buf, PcmNum, UsbAudioSpeaker.ByteSet);
#ifdef CFG_FUNC_USB_SPEAKER_HW_SRC_EN
	if(IsSpeakerSRCFlag)
	{
		if(SRC_IsBlockDone(CFG_USB_SPEAKER_HW_SRC_PORT))
		{
			Len = SRC_BlockApplyDone((SRC_MODULE)CFG_USB_SPEAKER_HW_SRC_PORT, &USBSpeakersrcContext, (int16_t*)USBSpeakersrcContext.OutBuf);
			UsbAudioBufDataSet(&UsbAudioSpeaker, (uint8_t*)USBSpeakersrcContext.OutBuf, Len);
		}
		SRC_BlockLenSet(CFG_USB_SPEAKER_HW_SRC_PORT, PcmNum / UsbAudioSpeaker.Channels);
		SRC_BlockApplyStart(CFG_USB_SPEAKER_HW_SRC_PORT, &USBSpeakersrcContext, (int16_t*)iso_buf, PcmNum / UsbAudioSpeaker.Channels);
	}
	else
#endif
	{
		UsbAudioBufDataSet(&UsbAudioSpeaker, (uint8_t*)iso_buf, PcmNum / UsbAudioSpeaker.Channels);
	}

	if(!AudioCoreSourceIsEnable(USB_AUDIO_SOURCE_NUM)
			&& !UsbAudioSpeaker.FirstThrhld
			&& MCUCircular_GetDataLen(&UsbAudioSpeaker.CircularBuf) >= UsbAudioSpeaker.CircularBuf.BufDepth / 2)
	{
//		AudioCoreSourceAdapterReset(USB_AUDIO_SOURCE_NUM);
		AudioCoreSourceEnable(USB_AUDIO_SOURCE_NUM);
		UsbAudioSpeaker.FirstThrhld = TRUE;
	}
}
#endif
#ifdef CFG_OTG_MODE_MIC_EN
/**
 * @brief  Mic ISO 输入端点发送回调
 *         从 Mic 缓冲区读取 PCM 数据并转为 USB ISO 格式发送到 PC
 */
__attribute__((section(".tcm_section")))
void OnDeviceHsAudioSendIsoPacket(void)
{
	uint32_t RealLen = 0;
	if(UsbAudioMic.FramCount == 0)
	{
		UsbAudioMic.FirstThrhld = FALSE;
	}
	UsbAudioMic.FramCount++;
	RealLen = UsbAudioMic.AudioSampleRate/UsbAudioMic.FramesPerSecond;
	UsbAudioMic.Accumulator += UsbAudioMic.AudioSampleRate%UsbAudioMic.FramesPerSecond;
	if(UsbAudioMic.Accumulator > UsbAudioMic.FramesPerSecond)
	{
		UsbAudioMic.Accumulator -= UsbAudioMic.FramesPerSecond;
		RealLen += 1;
	}

	if(AudioCoreSinkIsInit(USB_AUDIO_SINK_NUM) && !UsbAudioMic.FirstThrhld)
	{
		memset(iso_buf,0,RealLen*sizeof(PCM_DATA_TYPE)*UsbAudioMic.Channels);
		if(!AudioCoreSinkIsEnable(USB_AUDIO_SINK_NUM))
		{
//			AudioCoreSinkAdapterReset(USB_AUDIO_SINK_NUM);
			AudioCoreSinkEnable(USB_AUDIO_SINK_NUM);
		}
		else if(MCUCircular_GetDataLen(&UsbAudioMic.CircularBuf) > UsbAudioMic.CircularBuf.BufDepth * 45 / 100)
		{
			UsbAudioMic.FirstThrhld = TRUE;
		}
	}
	else if(AudioCoreSinkIsInit(USB_AUDIO_SINK_NUM)
			&& UsbAudioMic.FirstThrhld
			&& (MCUCircular_GetDataLen(&UsbAudioMic.CircularBuf) >= RealLen*sizeof(PCM_DATA_TYPE)*UsbAudioMic.Channels))
	{
		MCUCircular_GetData(&UsbAudioMic.CircularBuf, iso_buf,RealLen*sizeof(PCM_DATA_TYPE)*UsbAudioMic.Channels);
	}
	else
	{
		memset(iso_buf,0,RealLen*sizeof(PCM_DATA_TYPE)*UsbAudioMic.Channels);
	}

	//内部->USB 调整PCM数据格式
	UsbAudioPcmInternalToIso(iso_buf, RealLen*UsbAudioMic.Channels, UsbAudioMic.ByteSet);

	OTG_HS_DeviceISOSend(DEVICE_ISO_IN_EP,(uint8_t*)iso_buf, RealLen*UsbAudioMic.ByteSet*UsbAudioMic.Channels);
}
#endif
#ifdef CFG_DESCRIP_HID_HEART_BEAT_EN
/**
 * @brief  发送 HID 心跳包，维持 USB 连接活性
 */
__attribute__((section(".tcm_section")))
void OnDeviceAudioSendIntHeartBeatPacket(void)
{
	uint8_t HeartBeat[2];
	HeartBeat[0] = 0x01;
	HeartBeat[1] = 0x00;
	OTG_HS_DeviceInterruptSendStart(DEVICE_INT_IN_EP1,HeartBeat, 2);
}
#endif
static uint32_t FramCount = 0;
/**
 * @brief  1ms 定时器处理函数，用于音频流监控
 *         检测 Speaker/Mic 数据流状态，超时或断开时自动停止音频流
 */
void UsbAudioHsTimer1msProcess(void)
{
	FramCount++;
	if(FramCount % 2)//2ms
	{
		return;
	}
#ifdef CFG_OTG_MODE_AUDIO1_EN
	if(UsbAudioSpeaker1.AltSet && UsbAudioSpeaker1.FramCount != UsbAudioSpeaker1.TempFramCount)
	{//monitor
		UsbAudioSpeaker1.TempFramCount = UsbAudioSpeaker1.FramCount;
	}
	else//disconnect or stop：disable stream
	{
		UsbAudioSpeaker1.FramCount = 0;
		UsbAudioSpeaker1.TempFramCount = 0;
//		UsbAudioSpeaker1.CircularBuf.R = 0;
//		UsbAudioSpeaker1.CircularBuf.W = 0;
#ifdef CFG_FUNC_USB_AUDIO_MIX_MODE
		if(!IsBtHfpSrc())
#endif
		if(AudioCoreSourceIsEnable(USB_AUDIO_SOURCE1_NUM))
		AudioCoreSourceDisable(USB_AUDIO_SOURCE1_NUM);
	}
#endif

#ifdef CFG_OTG_MODE_AUDIO_EN
	if(UsbAudioSpeaker.AltSet && UsbAudioSpeaker.FramCount != UsbAudioSpeaker.TempFramCount)
	{//monitor
		UsbAudioSpeaker.TempFramCount = UsbAudioSpeaker.FramCount;
	}
	else//disconnect or stop：disable stream
	{
		UsbAudioSpeaker.FramCount = 0;
		UsbAudioSpeaker.TempFramCount = 0;
//		UsbAudioSpeaker.CircularBuf.R = 0;
//		UsbAudioSpeaker.CircularBuf.W = 0;
#ifdef CFG_FUNC_USB_AUDIO_MIX_MODE
		if(!IsBtHfpSrc())
#endif
		if(AudioCoreSourceIsEnable(USB_AUDIO_SOURCE_NUM))
		AudioCoreSourceDisable(USB_AUDIO_SOURCE_NUM);
	}
#endif

#ifdef CFG_OTG_MODE_MIC_EN
	if(UsbAudioMic.AltSet && UsbAudioMic.FramCount != UsbAudioMic.TempFramCount)//open stream
	{//monitor
		UsbAudioMic.TempFramCount = UsbAudioMic.FramCount;
	}
	else//disconnect or stop：disable stream
	{
		UsbAudioMic.Accumulator = 0;
		UsbAudioMic.FramCount = 0;
		UsbAudioMic.TempFramCount = 0;
#ifdef CFG_FUNC_USB_AUDIO_MIX_MODE
		if(!IsBtHfpSink())
#endif
		if(AudioCoreSinkIsEnable(USB_AUDIO_SINK_NUM))
		AudioCoreSinkDisable(USB_AUDIO_SINK_NUM);
	}
#endif
}
#ifdef CFG_OTG_MODE_AUDIO_EN
/**
 * @brief  设置 Speaker 的 Alternate Setting，切换音频参数配置
 * @param  Alt Alternate Setting 值（0=关闭，非0=对应配置索引）
 */
	void UsbHsSpeakerAlt(uint8_t Alt)
	{
		if(UsbAudioSpeaker.AltSet != Alt)
		{
			UsbAudioSpeaker.AltSet = Alt;
			UsbAudioSpeaker.ByteSet = SpeakerByteSet[UsbAudioSpeaker.AltSet];
			if(UsbAudioSpeaker.AltSet)
			{
				AudioCoreSourceChange(USB_AUDIO_SOURCE_NUM, UsbAudioSpeaker.Channels, UsbAudioSpeaker.AudioSampleRate);
				OTG_HS_DeviceEndpointPacketSizeSet(DEVICE_ISO_OUT_EP,GET_MAX_PACK(UsbAudioSpeaker.SampleRateS[0],UsbAudioSpeaker.Channels,UsbAudioSpeaker.ByteSet,UsbAudioSpeaker.FramesPerSecond));
				UsbAudioSpeaker.CircularBuf.R = 0;
				UsbAudioSpeaker.CircularBuf.W = 0;
				UsbAudioSpeaker.FirstThrhld = FALSE;
	#ifdef 	CFG_FUNC_USB_SPEAKER_HW_SRC_EN
				uint32_t MixSampleRate =  AudioCoreMixSampleRateGet(AudioCoreGetNetBySourceChannelNUM(USB_AUDIO_SOURCE_NUM));
				if(UsbAudioSpeaker.AudioSampleRate != MixSampleRate)
				{
					extern SRC_Context USBSpeakersrcContext;
					Reset_Function2Reset((SRC0_FUNC_SEPA) << CFG_USB_SPEAKER_HW_SRC_PORT);
					Reset_Register2Reset(SRC0_REG_SEPA << CFG_USB_SPEAKER_HW_SRC_PORT);
					SRC_BlockInit(CFG_USB_SPEAKER_HW_SRC_PORT, &USBSpeakersrcContext, UsbAudioSpeaker.AudioSampleRate, MixSampleRate);//设置一个初始的采样率转换值
					IsSpeakerSRCFlag = 1;//需要硬件SRC
					DBG("UacSpeakerSR = %lu To %lu\n", UsbAudioSpeaker.AudioSampleRate, MixSampleRate);
				}
				else
					IsSpeakerSRCFlag = 0;
	#endif
			}
			else
			{
				UsbAudioSpeaker.FirstThrhld = FALSE;
			}
		}
	}
#endif

#ifdef CFG_OTG_MODE_AUDIO1_EN
/**
 * @brief  设置 Speaker1 的 Alternate Setting，切换音频参数配置
 * @param  Alt Alternate Setting 值（0=关闭，非0=对应配置索引）
 */
	void UsbHsSpeaker1Alt(uint8_t Alt)
	{
		if(UsbAudioSpeaker1.AltSet != Alt)
		{
			UsbAudioSpeaker1.AltSet = Alt;
			UsbAudioSpeaker1.ByteSet = Speaker1ByteSet[UsbAudioSpeaker1.AltSet];
			if(UsbAudioSpeaker1.AltSet)
			{
				AudioCoreSourceChange(USB_AUDIO_SOURCE1_NUM, UsbAudioSpeaker1.Channels, UsbAudioSpeaker1.AudioSampleRate);
				OTG_HS_DeviceEndpointPacketSizeSet(DEVICE_ISO_OUT_EP1,GET_MAX_PACK(UsbAudioSpeaker1.SampleRateS[0],UsbAudioSpeaker1.Channels,UsbAudioSpeaker1.ByteSet,UsbAudioSpeaker1.FramesPerSecond));
				UsbAudioSpeaker1.CircularBuf.R = 0;
				UsbAudioSpeaker1.CircularBuf.W = 0;
				UsbAudioSpeaker1.FirstThrhld = FALSE;
			}
			else
			{
				UsbAudioSpeaker1.FirstThrhld = FALSE;
			}
		}
	}
#endif

#ifdef CFG_OTG_MODE_MIC_EN
/**
 * @brief  设置 Mic 的 Alternate Setting，切换音频参数配置
 * @param  Alt Alternate Setting 值（0=关闭，非0=对应配置索引）
 */
	void UsbHsMicAlt(uint8_t Alt)
	{
		if(UsbAudioMic.AltSet != Alt)
		{
			UsbAudioMic.AltSet = Alt;
			UsbAudioMic.ByteSet = MicByteSet[UsbAudioMic.AltSet];
			if(UsbAudioMic.AltSet)
			{
	#ifdef CFG_FUNC_USB_MIC_HW_SRC_EN
				uint32_t MixSampleRate = AudioCoreMixSampleRateGet(AudioCoreGetNetBySinkChannelNUM(USB_AUDIO_SINK_NUM));
				UsbAudioMic.CircularBuf.R = 0;
				UsbAudioMic.CircularBuf.W = 0;
				if(UsbAudioMic.AudioSampleRate != MixSampleRate)
				{
					extern SRC_Context USBMicsrcContext;
					Reset_Function2Reset((SRC0_FUNC_SEPA) << CFG_USB_MIC_HW_SRC_PORT);
					Reset_Register2Reset(SRC0_REG_SEPA << CFG_USB_MIC_HW_SRC_PORT);
					SRC_BlockInit(CFG_USB_MIC_HW_SRC_PORT, &USBMicsrcContext, MixSampleRate, UsbAudioMic.AudioSampleRate);//设置一个初始的采样率转换值
					IsMicSRCFlag = 1;//需要硬件SRC
					DBG("UacMicSR = %lu To %lu\n", MixSampleRate, UsbAudioMic.AudioSampleRate);
				}
				else
					IsMicSRCFlag = 0;
	#endif
				AudioCoreSinkChange(USB_AUDIO_SINK_NUM, UsbAudioMic.Channels, UsbAudioMic.AudioSampleRate);
				OTG_HS_DeviceEndpointPacketSizeSet(DEVICE_ISO_IN_EP,GET_MAX_PACK(UsbAudioMic.SampleRateS[0],UsbAudioMic.Channels,UsbAudioMic.ByteSet,UsbAudioMic.FramesPerSecond));
				OTG_HS_EndpointInterruptEnable(DEVICE_ISO_IN_EP,OnDeviceHsAudioSendIsoPacket);
//				OTG_HS_DeviceISOSend(DEVICE_ISO_IN_EP,0,0);
				OnDeviceHsAudioSendIsoPacket();
			}
			else
			{
				OTG_HS_EndpointInterruptDisable(DEVICE_ISO_IN_EP);
				OTG_HS_DeviceEndpointReset(DEVICE_ISO_IN_EP,TYPE_ISO_IN);
				UsbAudioMic.FirstThrhld = FALSE;
			}
		}
	}
#endif

#ifdef CFG_OTG_MODE_AUDIO_EN
/**
 * @brief  设置 Speaker 采样率，触发音频核心重配置
 *         支持硬件 SRC 模式下的采样率自适应转换
 */
void UsbHsSpeakerSampleRateSet(void)
{
	DBG("UsbSpeakerSampleRateSet %ld\n",UsbAudioSpeaker.AudioSampleRate);
#ifdef 	CFG_FUNC_USB_SPEAKER_HW_SRC_EN
	uint32_t MixSampleRate = AudioCoreMixSampleRateGet(AudioCoreGetNetBySourceChannelNUM(USB_AUDIO_SOURCE_NUM));
	if(UsbAudioSpeaker.AudioSampleRate == MixSampleRate)
	{
		IsSpeakerSRCFlag = 0;
		DBG("Pause HwSRC\n");
	}
	else
	{
		IsSpeakerSRCFlag = 1;//需要硬件SRC
		Reset_Function2Reset((SRC0_FUNC_SEPA) << CFG_USB_SPEAKER_HW_SRC_PORT);
		//function reset之后，采样率值需要配置2次才能生效
		SRCInSampleRateSet(CFG_USB_SPEAKER_HW_SRC_PORT, MixSampleRate);
		SRCInSampleRateSet(CFG_USB_SPEAKER_HW_SRC_PORT, UsbAudioSpeaker.AudioSampleRate);
		SRCOutSampleRateSet(CFG_USB_SPEAKER_HW_SRC_PORT, 8000);
		SRCOutSampleRateSet(CFG_USB_SPEAKER_HW_SRC_PORT, MixSampleRate);
		//SRC_BlockInit(SRC0_MODULE, &srcContext, UsbAudioSpeaker.AudioSampleRate, AudioCoreMixSampleRateGet(DefaultNet));//设置一个初始的采样率转换值
	}
#else
	AudioCoreSourceChange(USB_AUDIO_SOURCE_NUM, UsbAudioSpeaker.Channels, UsbAudioSpeaker.AudioSampleRate);
#endif
}
/**
 * @brief  设置 Speaker 音量/静音状态（调试用）
 * @param  channel 通道索引（0xFF=查询静音状态，其他=指定通道）
 */
void UsbHsSpeakerVolumeSet(uint8_t channel)
{
	if(channel == 0xFF)
	{
//		DBG("mute=%d\n",UsbAudioSpeaker.Mute);
	}
	else
	{
//		DBG("channel=%d dB=%d\n",channel,UsbAudioGetChannelDb(&UsbAudioSpeaker,channel));
	}
}
#endif

#ifdef CFG_OTG_MODE_AUDIO1_EN
/**
 * @brief  设置 Speaker1 采样率，触发音频核心重配置
 */
void UsbHsSpeaker1SampleRateSet(void)
{
	DBG("UsbHsSpeaker1SampleRateSet %ld\n",UsbAudioSpeaker1.AudioSampleRate);
	AudioCoreSourceChange(USB_AUDIO_SOURCE1_NUM, UsbAudioSpeaker1.Channels, UsbAudioSpeaker1.AudioSampleRate);
}
/**
 * @brief  设置 Speaker1 音量/静音状态（调试用）
 * @param  channel 通道索引（0xFF=查询静音状态，其他=指定通道）
 */
void UsbHsSpeaker1VolumeSet(uint8_t channel)
{
	if(channel == 0xFF)
	{
//		DBG("mute=%d\n",UsbAudioSpeaker1.Mute);
	}
	else
	{
//		DBG("channel=%d dB=%d\n",channel,UsbAudioGetChannelDb(&UsbAudioSpeaker1,channel));
	}
}
#endif

#ifdef CFG_OTG_MODE_MIC_EN
/**
 * @brief  设置 Mic 采样率，触发音频核心重配置
 *         支持硬件 SRC 模式下的采样率自适应转换
 */
void UsbHsMicSampleRateSet(void)
{
	DBG("UsbMicSampleRateSet %ld\n",UsbAudioMic.AudioSampleRate);
#ifdef CFG_FUNC_USB_MIC_HW_SRC_EN
	extern bool IsMicSRCFlag;
	uint32_t MixSampleRate = AudioCoreMixSampleRateGet(AudioCoreGetNetBySinkChannelNUM(USB_AUDIO_SINK_NUM));
	if(UsbAudioMic.AudioSampleRate == MixSampleRate)
	{
		IsMicSRCFlag = 0;
		DBG("Pause HwSRC\n");
	}
	else
	{
		IsMicSRCFlag = 1;//需要硬件SRC
		Reset_Function2Reset((SRC0_FUNC_SEPA) << CFG_USB_MIC_HW_SRC_PORT);//SRC1
		//function reset之后，采样率值需要配置2次才能生效
		SRCInSampleRateSet(CFG_USB_MIC_HW_SRC_PORT, 8000);
		SRCInSampleRateSet(CFG_USB_MIC_HW_SRC_PORT, MixSampleRate);
		SRCOutSampleRateSet(CFG_USB_MIC_HW_SRC_PORT, MixSampleRate);
		SRCOutSampleRateSet(CFG_USB_MIC_HW_SRC_PORT, UsbAudioMic.AudioSampleRate);
	}
#else
	AudioCoreSinkChange(USB_AUDIO_SINK_NUM, UsbAudioMic.Channels, UsbAudioMic.AudioSampleRate);
#endif
}
/**
 * @brief  设置 Mic 音量/静音状态（调试用）
 * @param  channel 通道索引（0xFF=查询静音状态，其他=指定通道）
 */
void UsbHsMicVolumeSet(uint8_t channel)
{
	if(channel == 0xFF)
	{
//		DBG("mute=%d\n",UsbAudioMic.Mute);
	}
	else
	{
//		DBG("channel=%d dB=%d\n",channel,UsbAudioGetChannelDb(&UsbAudioMic,channel));
	}
}
#endif

#endif //#ifdef CFG_APP_USB_AUDIO_MODE_EN
