/**
 *****************************************************************************
 * @file     otg_device_audio_fs.c
 * @author   Shanks
 * @version  V1.0.0
 * @date     7-September-2015
 * @brief    USB OTG FS(Full-Speed) 全速音频设备驱动
 *****************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
 *******************************************************************************
 * @brief   核心功能说明：
 *  1. 基于 USB Full-Speed 实现 UAC 全速音频设备驱动
 *  2. 支持 Speaker 播放、Mic 录音 双方向ISO数据传输
 *  3. 环形缓冲区实现 USB 与音频系统的数据缓冲与同步
 *  4. 支持 16bit / 24bit PCM 位深自动格式转换
 *  5. 音量调节、静音控制、增益淡入淡出处理
 *  6. 多AltSet模式、多通道、多采样率动态切换
 *  7. 音频流启停、缓存阈值自动控制
 *******************************************************************************
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
#if defined CFG_APP_USB_FS_AUDIO_MODE_EN || defined CFG_COMMUNICATION_BY_FS_USB

#ifdef CFG_OTG_FS_MODE_MIC_EN
	UsbAudio UsbAudioMixMic;
	static const uint8_t MicByteSet[] = {0,MIC_ALT1_BITS,MIC_ALT2_BITS};
#endif
#ifdef CFG_OTG_FS_MODE_AUDIO_EN
	UsbAudio UsbAudioMixSpeaker;
	static const uint8_t SpeakerByteSet[] = {0,SPEAKER_ALT1_BITS,SPEAKER_ALT2_BITS};
#endif
#ifdef CFG_OTG_FS_MODE_AUDIO1_EN
	UsbAudio UsbAudioMixSpeaker1;
	static const uint8_t Speaker1ByteSet[] = {0,SPEAKER1_ALT1_BITS,SPEAKER1_ALT2_BITS};
#endif
static uint8_t iso_buf[PCM_LEN_MAX];
static uint8_t AudioCmd[3] = {1,0,0};
/**
 * @brief  USB声卡模式下，发送反向控制命令
 * @param  Cmd 反向控制命令
 * @return 1-成功，0-失败
 */
bool OTG_FS_DeviceAudioSendPcCmd(uint16_t Cmd)
{
	AudioCmd[1] = Cmd;
	AudioCmd[2] = Cmd >> 8;
	OTG_FS_DeviceInterruptSend(DEVICE_INT_IN_EP,AudioCmd, 3,1);
	AudioCmd[1] = 0;
	AudioCmd[2] = 0;
	OTG_FS_DeviceInterruptSend(DEVICE_INT_IN_EP,AudioCmd, 3,1);
	return TRUE;
}
/**
 * @brief  发送 HID 按键按下事件（反向控制命令）
 * @param  Cmd 按键命令码
 * @return 1-成功，0-失败
 */
bool OTG_FS_DeviceAudioSendHidKeyDown(uint16_t Cmd)
{
	AudioCmd[1] = Cmd;
	AudioCmd[2] = Cmd >> 8;
	OTG_FS_DeviceInterruptSend(DEVICE_INT_IN_EP,AudioCmd, 3,1);
	return TRUE;
}
/**
 * @brief  发送 HID 按键释放事件（清空反向控制命令）
 * @return 1-成功，0-失败
 */
bool OTG_FS_DeviceAudioSendHidKeyUp(void)
{
	AudioCmd[1] = 0;
	AudioCmd[2] = 0;
	OTG_FS_DeviceInterruptSend(DEVICE_INT_IN_EP,AudioCmd, 3,1);
	return TRUE;
}

#ifdef CFG_OTG_FS_MODE_AUDIO1_EN
/**
 * @brief  从 Speaker1 音频缓冲区读取 PCM 数据（PC→芯片）
 * @param  Buffer 输出数据缓冲区指针
 * @param  Len    期望读取的 PCM 样本数（每通道）
 * @return 实际读取的 PCM 样本数（每通道）
 */
uint16_t UsbFSAudioMixSpeaker1DataGet(void *Buffer, uint16_t Len)
{
	uint16_t ret = UsbAudioBufDataGet(&UsbAudioMixSpeaker1, Buffer, Len);
#ifdef CFG_RES_AUDIO_USB_FS_VOL_SET_EN
	UsbAudioGainApply(&UsbAudioMixSpeaker1, Buffer, ret);
#endif
	return ret;
}

/**
 * @brief  获取 Speaker1 音频缓冲区中可读 PCM 样本数（每通道）
 * @return 可读 PCM 样本数（每通道）
 */
uint16_t UsbFSAudioMixSpeaker1DataLenGet(void)
{
	return UsbAudioBufDataLen(&UsbAudioMixSpeaker1);
}
/**
 * @brief  获取 Speaker1 音频缓冲区深度（总容量）
 * @return 缓冲区深度（采样点数）
 */
uint16_t UsbFSAudioMixSpeaker1DepthGet(void)
{
	return UsbAudioBufDepth(&UsbAudioMixSpeaker1, PACKET1_CHANNELS_NUM);
}
#endif

#ifdef CFG_OTG_FS_MODE_AUDIO_EN
/**
 * @brief  从 Speaker 音频缓冲区读取 PCM 数据（PC→芯片）
 * @param  Buffer 输出数据缓冲区指针
 * @param  Len    期望读取的 PCM 样本数（每通道）
 * @return 实际读取的 PCM 样本数（每通道）
 */
uint16_t UsbFSAudioMixSpeakerDataGet(void *Buffer, uint16_t Len)
{
	uint16_t ret = UsbAudioBufDataGet(&UsbAudioMixSpeaker, Buffer, Len);
#ifdef CFG_RES_AUDIO_USB_FS_VOL_SET_EN
	UsbAudioGainApply(&UsbAudioMixSpeaker, Buffer, ret);
#endif
	return ret;
}

/**
 * @brief  获取 Speaker 音频缓冲区中可读 PCM 样本数（每通道）
 * @return 可读 PCM 样本数（每通道）
 */
uint16_t UsbFSAudioMixSpeakerDataLenGet(void)
{
	return UsbAudioBufDataLen(&UsbAudioMixSpeaker);
}
/**
 * @brief  获取 Speaker 音频缓冲区深度（总容量）
 * @return 缓冲区深度（采样点数）
 */
uint16_t UsbFSAudioMixSpeakerDepthGet(void)
{
	return UsbAudioBufDepth(&UsbAudioMixSpeaker, PACKET_CHANNELS_NUM);
}
#endif

#ifdef CFG_OTG_FS_MODE_MIC_EN
/**
 * @brief  将 Mic PCM 数据写入音频缓冲区（芯片→PC）
 * @param  Buffer 输入数据缓冲区指针
 * @param  Len    待写入的 PCM 样本数（每通道）
 * @return 实际写入的 PCM 样本数（每通道）
 */
uint16_t UsbFSAudioMixMicDataSet(void *Buffer, uint16_t Len)
{
#ifdef CFG_RES_AUDIO_USB_FS_VOL_SET_EN
	UsbAudioGainApply(&UsbAudioMixMic, Buffer, Len);
#endif
	return UsbAudioBufDataSet(&UsbAudioMixMic, Buffer, Len);
}

/**
 * @brief  获取 Mic 音频缓冲区剩余可用 PCM 样本数（每通道）
 * @return 可用 PCM 样本数（每通道）
 */
uint16_t UsbFSAudioMixMicSpaceLenGet(void)
{
	return UsbAudioBufSpaceLen(&UsbAudioMixMic);
}

/**
 * @brief  获取 Mic 音频缓冲区深度（总容量）
 * @return 缓冲区深度（采样点数）
 */
uint16_t UsbFSAudioMixMicDepthGet(void)
{
	return UsbAudioBufDepth(&UsbAudioMixMic, MIC_CHANNELS_NUM);
}
#endif

#ifdef CFG_OTG_FS_MODE_AUDIO1_EN
/**
 * @brief  Speaker1 ISO 输出端点接收回调
 *         接收 PC 端音频 ISO 数据包，转为内部 PCM 格式写入 Speaker1 缓冲区
 */
__attribute__((section(".tcm_section")))
void OnDeviceFsAudioRcvIsoPacket1(void)
{
	uint32_t Len;
	int32_t s;
	uint32_t PcmNum = 0;//= sample * channels

	OTG_FS_DeviceISOReceive(DEVICE_ISO_OUT_EP1, (uint8_t*)iso_buf, sizeof(iso_buf), &Len);

	if(Len == 0 || !AudioCoreSourceIsInit(USB_FS_AUDIO_SOURCE1_NUM)) return;
	if(UsbAudioMixSpeaker1.FramCount == 0)
	{
		UsbAudioMixSpeaker1.FirstThrhld = FALSE;
	}
	UsbAudioMixSpeaker1.FramCount++;
	PcmNum = Len/UsbAudioMixSpeaker1.ByteSet;
	UsbAudioPcmIsoToInternal(iso_buf, PcmNum, UsbAudioMixSpeaker1.ByteSet);
	UsbAudioBufDataSet(&UsbAudioMixSpeaker1, (uint8_t*)iso_buf, PcmNum / UsbAudioMixSpeaker1.Channels);

	if(!AudioCoreSourceIsEnable(USB_FS_AUDIO_SOURCE1_NUM)
			&& !UsbAudioMixSpeaker1.FirstThrhld
			&& MCUCircular_GetDataLen(&UsbAudioMixSpeaker1.CircularBuf) >= UsbAudioMixSpeaker1.CircularBuf.BufDepth / 2)
	{
//		AudioCoreSourceAdapterReset(USB_FS_AUDIO_SOURCE1_NUM);
		AudioCoreSourceEnable(USB_FS_AUDIO_SOURCE1_NUM);
		UsbAudioMixSpeaker1.FirstThrhld = TRUE;
	}
}
#endif

#ifdef CFG_OTG_FS_MODE_AUDIO_EN
/**
 * @brief  Speaker ISO 输出端点接收回调
 *         接收 PC 端音频 ISO 数据包，转为内部 PCM 格式写入 Speaker 缓冲区
 */
__attribute__((section(".tcm_section")))
void OnDeviceFsAudioRcvIsoPacket(void)
{
	uint32_t Len;
	uint32_t PcmNum = 0;//= sample * channels

	OTG_FS_DeviceISOReceive(DEVICE_ISO_OUT_EP, (uint8_t*)iso_buf, sizeof(iso_buf), &Len);

	if(Len == 0 || !AudioCoreSourceIsInit(USB_FS_AUDIO_SOURCE_NUM)) return;
	if(UsbAudioMixSpeaker.FramCount == 0)
	{
		UsbAudioMixSpeaker.FirstThrhld = FALSE;
	}
	UsbAudioMixSpeaker.FramCount++;
	PcmNum = Len/UsbAudioMixSpeaker.ByteSet;
	UsbAudioPcmIsoToInternal(iso_buf, PcmNum, UsbAudioMixSpeaker.ByteSet);
	UsbAudioBufDataSet(&UsbAudioMixSpeaker, (uint8_t*)iso_buf, PcmNum / UsbAudioMixSpeaker.Channels);
	
	if(!AudioCoreSourceIsEnable(USB_FS_AUDIO_SOURCE_NUM)
			&& !UsbAudioMixSpeaker.FirstThrhld
			&& MCUCircular_GetDataLen(&UsbAudioMixSpeaker.CircularBuf) >= UsbAudioMixSpeaker.CircularBuf.BufDepth / 2)
	{
//		AudioCoreSourceAdapterReset(USB_FS_AUDIO_SOURCE_NUM);
		AudioCoreSourceEnable(USB_FS_AUDIO_SOURCE_NUM);
		UsbAudioMixSpeaker.FirstThrhld = TRUE;
	}
}
#endif

#ifdef CFG_OTG_FS_MODE_MIC_EN
/**
 * @brief  Mic ISO 输入端点发送回调
 *         从 Mic 缓冲区读取 PCM 数据并转为 USB ISO 格式发送到 PC
 */
__attribute__((section(".tcm_section")))
void OnDeviceFsAudioSendIsoPacket(void)
{
	uint32_t RealLen = 0;
	if(UsbAudioMixMic.FramCount == 0)
	{
		UsbAudioMixMic.FirstThrhld = FALSE;
	}
	UsbAudioMixMic.FramCount++;
	RealLen = UsbAudioMixMic.AudioSampleRate/UsbAudioMixMic.FramesPerSecond;
	UsbAudioMixMic.Accumulator += UsbAudioMixMic.AudioSampleRate%UsbAudioMixMic.FramesPerSecond;
	if(UsbAudioMixMic.Accumulator > UsbAudioMixMic.FramesPerSecond)
	{
		UsbAudioMixMic.Accumulator -= UsbAudioMixMic.FramesPerSecond;
		RealLen += 1;
	}

	if(AudioCoreSinkIsInit(USB_FS_AUDIO_SINK_NUM) && !UsbAudioMixMic.FirstThrhld)
	{
		memset(iso_buf,0,RealLen*sizeof(PCM_DATA_TYPE)*UsbAudioMixMic.Channels);
		if(!AudioCoreSinkIsEnable(USB_FS_AUDIO_SINK_NUM))
		{
//			AudioCoreSinkAdapterReset(USB_FS_AUDIO_SINK_NUM);
			AudioCoreSinkEnable(USB_FS_AUDIO_SINK_NUM);
		}
		else if(MCUCircular_GetDataLen(&UsbAudioMixMic.CircularBuf) > UsbAudioMixMic.CircularBuf.BufDepth * 5 / 10)
		{
			UsbAudioMixMic.FirstThrhld = TRUE;
		}
	}
	else if(AudioCoreSinkIsInit(USB_FS_AUDIO_SINK_NUM)
			&& UsbAudioMixMic.FirstThrhld
			&& MCUCircular_GetDataLen(&UsbAudioMixMic.CircularBuf) >= RealLen*sizeof(PCM_DATA_TYPE)*UsbAudioMixMic.Channels)
	{
		MCUCircular_GetData(&UsbAudioMixMic.CircularBuf, iso_buf,RealLen*sizeof(PCM_DATA_TYPE)*UsbAudioMixMic.Channels);
	}
	else
	{
		memset(iso_buf,0,RealLen*sizeof(PCM_DATA_TYPE)*UsbAudioMixMic.Channels);
	}
	//内部->USB 调整PCM数据格式
	UsbAudioPcmInternalToIso(iso_buf, RealLen*UsbAudioMixMic.Channels, UsbAudioMixMic.ByteSet);

	OTG_FS_DeviceISOSend(DEVICE_ISO_IN_EP,(uint8_t*)iso_buf, RealLen*UsbAudioMixMic.ByteSet*UsbAudioMixMic.Channels);
}
#endif

static uint32_t FramCount = 0;
/**
 * @brief  1ms 定时器处理函数，用于音频流监控
 *         检测 Speaker/Mic 数据流状态，超时或断开时自动停止音频流
 */
void UsbAudioFsTimer1msProcess(void)
{
	FramCount++;
	if(FramCount % 2)//2ms
	{
		return;
	}
#ifdef CFG_OTG_FS_MODE_AUDIO1_EN
	if(UsbAudioMixSpeaker1.AltSet && UsbAudioMixSpeaker1.FramCount != UsbAudioMixSpeaker1.TempFramCount)
	{//monitor
		UsbAudioMixSpeaker1.TempFramCount = UsbAudioMixSpeaker1.FramCount;
	}
	else//disconnect or stop：disable stream
	{
		UsbAudioMixSpeaker1.FramCount = 0;
		UsbAudioMixSpeaker1.TempFramCount = 0;
//		UsbAudioMixSpeaker1.CircularBuf.R = 0;
//		UsbAudioMixSpeaker1.CircularBuf.W = 0;
	#ifdef CFG_FUNC_USB_AUDIO_MIX_MODE
		if(!IsBtHfpSrc())
	#endif
		if(AudioCoreSourceIsEnable(USB_FS_AUDIO_SOURCE1_NUM))
		AudioCoreSourceDisable(USB_FS_AUDIO_SOURCE1_NUM);
	}
#endif
#ifdef CFG_OTG_FS_MODE_AUDIO_EN
	if(UsbAudioMixSpeaker.AltSet && UsbAudioMixSpeaker.FramCount != UsbAudioMixSpeaker.TempFramCount)
	{//monitor
		UsbAudioMixSpeaker.TempFramCount = UsbAudioMixSpeaker.FramCount;
	}
	else//disconnect or stop：disable stream
	{
		UsbAudioMixSpeaker.FramCount = 0;
		UsbAudioMixSpeaker.TempFramCount = 0;
//		UsbAudioMixSpeaker.CircularBuf.R = 0;
//		UsbAudioMixSpeaker.CircularBuf.W = 0;
	#ifdef CFG_FUNC_USB_AUDIO_MIX_MODE
		if(!IsBtHfpSrc())
	#endif
		if(AudioCoreSourceIsEnable(USB_FS_AUDIO_SOURCE_NUM))
		AudioCoreSourceDisable(USB_FS_AUDIO_SOURCE_NUM);
	}
#endif

#ifdef CFG_OTG_FS_MODE_MIC_EN
	if(UsbAudioMixMic.AltSet && UsbAudioMixMic.FramCount != UsbAudioMixMic.TempFramCount)//open stream
	{//monitor
		UsbAudioMixMic.TempFramCount = UsbAudioMixMic.FramCount;
	}
	else//disconnect or stop：disable stream
	{
		UsbAudioMixMic.Accumulator = 0;
		UsbAudioMixMic.FramCount = 0;
		UsbAudioMixMic.TempFramCount = 0;
	#ifdef CFG_FUNC_USB_AUDIO_MIX_MODE
		if(!IsBtHfpSink())
	#endif
		if(AudioCoreSinkIsEnable(USB_FS_AUDIO_SINK_NUM))
		AudioCoreSinkDisable(USB_FS_AUDIO_SINK_NUM);
	}
#endif
}

#ifdef CFG_OTG_FS_MODE_AUDIO_EN
/**
 * @brief  设置 Speaker 的 Alternate Setting，切换音频参数配置
 * @param  Alt Alternate Setting 值（0=关闭，非0=对应配置索引）
 */
	void UsbFsSpeakerAlt(uint8_t Alt)
	{
		if(UsbAudioMixSpeaker.AltSet != Alt)
		{
			UsbAudioMixSpeaker.AltSet = Alt;
			UsbAudioMixSpeaker.ByteSet = SpeakerByteSet[UsbAudioMixSpeaker.AltSet];
			if(UsbAudioMixSpeaker.AltSet)
			{
				AudioCoreSourceChange(USB_FS_AUDIO_SOURCE_NUM, UsbAudioMixSpeaker.Channels, UsbAudioMixSpeaker.AudioSampleRate);
				OTG_FS_DeviceEndpointPacketSizeSet(DEVICE_ISO_OUT_EP,GET_MAX_PACK_FS(UsbAudioMixSpeaker.SampleRateS[0],UsbAudioMixSpeaker.Channels,UsbAudioMixSpeaker.ByteSet,UsbAudioMixSpeaker.FramesPerSecond));
				UsbAudioMixSpeaker.CircularBuf.R = 0;
				UsbAudioMixSpeaker.CircularBuf.W = 0;
				UsbAudioMixSpeaker.FirstThrhld = FALSE;
			}
			else
			{
				UsbAudioMixSpeaker.FirstThrhld = FALSE;
				AudioCoreSourceDisable(USB_FS_AUDIO_SOURCE_NUM);
			}
		}
	}
#endif

#ifdef CFG_OTG_FS_MODE_AUDIO1_EN
/**
 * @brief  设置 Speaker1 的 Alternate Setting，切换音频参数配置
 * @param  Alt Alternate Setting 值（0=关闭，非0=对应配置索引）
 */
	void UsbFsSpeaker1Alt(uint8_t Alt)
	{
		if(UsbAudioMixSpeaker1.AltSet != Alt)
		{
			UsbAudioMixSpeaker1.AltSet = Alt;
			UsbAudioMixSpeaker1.ByteSet = Speaker1ByteSet[UsbAudioMixSpeaker1.AltSet];
			if(UsbAudioMixSpeaker1.AltSet)
			{
				AudioCoreSourceChange(USB_FS_AUDIO_SOURCE1_NUM, UsbAudioMixSpeaker1.Channels, UsbAudioMixSpeaker1.AudioSampleRate);
				OTG_FS_DeviceEndpointPacketSizeSet(DEVICE_ISO_OUT_EP1,GET_MAX_PACK_FS(UsbAudioMixSpeaker1.SampleRateS[0],UsbAudioMixSpeaker1.Channels,UsbAudioMixSpeaker1.ByteSet,UsbAudioMixSpeaker1.FramesPerSecond));
				UsbAudioMixSpeaker1.CircularBuf.R = 0;
				UsbAudioMixSpeaker1.CircularBuf.W = 0;
				UsbAudioMixSpeaker1.FirstThrhld = FALSE;
			}
			else
			{
				UsbAudioMixSpeaker1.FirstThrhld = FALSE;
				AudioCoreSourceDisable(USB_FS_AUDIO_SOURCE1_NUM);
			}
		}
	}
#endif

#ifdef CFG_OTG_FS_MODE_MIC_EN
/**
 * @brief  设置 Mic 的 Alternate Setting，切换音频参数配置
 * @param  Alt Alternate Setting 值（0=关闭，非0=对应配置索引）
 */
	void UsbFsMicAlt(uint8_t Alt)
	{
		if(UsbAudioMixMic.AltSet != Alt)
		{
			UsbAudioMixMic.AltSet = Alt;
			UsbAudioMixMic.ByteSet = MicByteSet[UsbAudioMixMic.AltSet];
			if(UsbAudioMixMic.AltSet)
			{
				AudioCoreSinkChange(USB_FS_AUDIO_SINK_NUM, UsbAudioMixMic.Channels, UsbAudioMixMic.AudioSampleRate);
				UsbAudioMixMic.CircularBuf.R = 0;
				UsbAudioMixMic.CircularBuf.W = 0;
				OTG_FS_DeviceEndpointPacketSizeSet(DEVICE_ISO_IN_EP,GET_MAX_PACK_FS(UsbAudioMixMic.SampleRateS[0],UsbAudioMixMic.Channels,UsbAudioMixMic.ByteSet,UsbAudioMixMic.FramesPerSecond));
				OTG_FS_EndpointInterruptEnable(DEVICE_ISO_IN_EP,OnDeviceFsAudioSendIsoPacket);
//				OTG_FS_DeviceISOSend(DEVICE_ISO_IN_EP,0,0);
				OnDeviceFsAudioSendIsoPacket();
			}
			else
			{
				OTG_FS_EndpointInterruptDisable(DEVICE_ISO_IN_EP);
				OTG_FS_DeviceEndpointReset(DEVICE_ISO_IN_EP,TYPE_ISO_IN);
				UsbAudioMixMic.FirstThrhld = FALSE;
				AudioCoreSinkDisable(USB_FS_AUDIO_SINK_NUM);
			}
		}
	}
#endif

#ifdef CFG_OTG_FS_MODE_AUDIO_EN
/**
 * @brief  设置 Speaker 采样率，触发音频核心重配置
 */
void UsbFsSpeakerSampleRateSet(void)
{
	DBG("UsbFsSpeakerSampleRateSet %ld\n",UsbAudioMixSpeaker.AudioSampleRate);
	AudioCoreSourceChange(USB_FS_AUDIO_SOURCE_NUM, UsbAudioMixSpeaker.Channels, UsbAudioMixSpeaker.AudioSampleRate);
}
/**
 * @brief  设置 Speaker 音量/静音状态（调试用）
 * @param  channel 通道索引（0xFF=查询静音状态，其他=指定通道）
 */
void UsbFsSpeakerVolumeSet(uint8_t channel)
{
	if(channel == 0xFF)
	{
//		DBG("mute=%d\n",UsbAudioMixSpeaker.Mute);
	}
	else
	{
//		DBG("channel=%d dB=%d\n",channel,UsbAudioGetChannelDb(&UsbAudioMixSpeaker,channel));
	}
}
#endif

#ifdef CFG_OTG_FS_MODE_AUDIO1_EN
/**
 * @brief  设置 Speaker1 采样率，触发音频核心重配置
 */
void UsbFsSpeaker1SampleRateSet(void)
{
	DBG("UsbFsSpeaker1SampleRateSet %ld\n",UsbAudioMixSpeaker1.AudioSampleRate);
	AudioCoreSourceChange(USB_FS_AUDIO_SOURCE1_NUM, UsbAudioMixSpeaker1.Channels, UsbAudioMixSpeaker1.AudioSampleRate);
}
/**
 * @brief  设置 Speaker1 音量/静音状态（调试用）
 * @param  channel 通道索引（0xFF=查询静音状态，其他=指定通道）
 */
void UsbFsSpeaker1VolumeSet(uint8_t channel)
{
	if(channel == 0xFF)
	{
//		DBG("mute=%d\n",UsbAudioMixSpeaker1.Mute);
	}
	else
	{
//		DBG("channel=%d dB=%d\n",channel,UsbAudioGetChannelDb(&UsbAudioMixSpeaker1,channel));
	}
}
#endif

#ifdef CFG_OTG_FS_MODE_MIC_EN
/**
 * @brief  设置 Mic 采样率，触发音频核心重配置
 */
void UsbFsMicSampleRateSet(void)
{
	DBG("UsbFsMicSampleRateSet %ld\n",UsbAudioMixMic.AudioSampleRate);
	AudioCoreSinkChange(USB_FS_AUDIO_SINK_NUM, UsbAudioMixMic.Channels, UsbAudioMixMic.AudioSampleRate);
}
/**
 * @brief  设置 Mic 音量/静音状态（调试用）
 * @param  channel 通道索引（0xFF=查询静音状态，其他=指定通道）
 */
void UsbFsMicVolumeSet(uint8_t channel)
{
	if(channel == 0xFF)
	{
//		DBG("mute=%d\n",UsbAudioMixMic.Mute);
	}
	else
	{
//		DBG("channel=%d dB=%d\n",channel,UsbAudioGetChannelDb(&UsbAudioMixMic,channel));
	}
}
#endif

#endif //#ifdef CFG_RES_USB_FS_MIX_AUDIO_EN
