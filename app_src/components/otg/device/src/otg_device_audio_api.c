/**
 *****************************************************************************
 * @file     otg_device_audio_api.c
 * @author   Shanks
 * @version  V1.0.0
 * @date     7-September-2015
 * @brief    USB 音频设备公共辅助 API
 *****************************************************************************
 */
//driver
#include <string.h>
#include <math.h>

//application
#include "app_config.h"
#include "otg_device_audio.h"
#include "audio_core_api.h"
#if defined(CFG_UAC_DEVICE_HS_EN) || defined(CFG_UAC_DEVICE_FS_EN)
const uint32_t Uac10SpeakerRates[FREQ_NUM_MAX] = {
    USBD_AUDIO10_FREQ, USBD_AUDIO10_FREQ1, USBD_AUDIO10_FREQ2,
    USBD_AUDIO10_FREQ3, USBD_AUDIO10_FREQ4, USBD_AUDIO10_FREQ5,
    USBD_AUDIO10_FREQ6, USBD_AUDIO10_FREQ7, USBD_AUDIO10_FREQ8,
    USBD_AUDIO10_FREQ9
};

const uint32_t Uac10Speaker1Rates[FREQ_NUM_MAX] = {
    USBD_AUDIO10_SPEAKER1_FREQ, USBD_AUDIO10_SPEAKER1_FREQ1, USBD_AUDIO10_SPEAKER1_FREQ2,
    USBD_AUDIO10_SPEAKER1_FREQ3, USBD_AUDIO10_SPEAKER1_FREQ4, USBD_AUDIO10_SPEAKER1_FREQ5,
    USBD_AUDIO10_SPEAKER1_FREQ6, USBD_AUDIO10_SPEAKER1_FREQ7, USBD_AUDIO10_SPEAKER1_FREQ8,
    USBD_AUDIO10_SPEAKER1_FREQ9
};

const uint32_t Uac10MicRates[FREQ_NUM_MAX] = {
    USBD_AUDIO10_MIC_FREQ, USBD_AUDIO10_MIC_FREQ1, USBD_AUDIO10_MIC_FREQ2,
    USBD_AUDIO10_MIC_FREQ3, USBD_AUDIO10_MIC_FREQ4, USBD_AUDIO10_MIC_FREQ5,
    USBD_AUDIO10_MIC_FREQ6, USBD_AUDIO10_MIC_FREQ7, USBD_AUDIO10_MIC_FREQ8,
    USBD_AUDIO10_MIC_FREQ9
};

const uint32_t Uac20SpeakerRates[FREQ_NUM_MAX] = {
    USBD_AUDIO_FREQ, USBD_AUDIO_FREQ1, USBD_AUDIO_FREQ2,
    USBD_AUDIO_FREQ3, USBD_AUDIO_FREQ4, USBD_AUDIO_FREQ5,
    USBD_AUDIO_FREQ6, USBD_AUDIO_FREQ7, USBD_AUDIO_FREQ8,
    USBD_AUDIO_FREQ9
};

const uint32_t Uac20Speaker1Rates[FREQ_NUM_MAX] = {
    USBD_AUDIO1_FREQ, USBD_AUDIO1_FREQ1, USBD_AUDIO1_FREQ2,
    USBD_AUDIO1_FREQ3, USBD_AUDIO1_FREQ4, USBD_AUDIO1_FREQ5,
    USBD_AUDIO1_FREQ6, USBD_AUDIO1_FREQ7, USBD_AUDIO1_FREQ8,
    USBD_AUDIO1_FREQ9
};

const uint32_t Uac20MicRates[FREQ_NUM_MAX] = {
    USBD_AUDIO_MIC_FREQ, USBD_AUDIO_MIC_FREQ1, USBD_AUDIO_MIC_FREQ2,
    USBD_AUDIO_MIC_FREQ3, USBD_AUDIO_MIC_FREQ4, USBD_AUDIO_MIC_FREQ5,
    USBD_AUDIO_MIC_FREQ6, USBD_AUDIO_MIC_FREQ7, USBD_AUDIO_MIC_FREQ8,
    USBD_AUDIO_MIC_FREQ9
};

/**
 * @brief  统一采样率初始化（UAC1.0 / UAC2.0 共用）
 * @param audio UsbAudio实例指针
 * @param rates 支持的采样率数组
 * @param count 采样率数量
 * @return 无
 */
void UsbAudioSampleRateInit(UsbAudio *audio,const uint32_t *rates, uint8_t count)
{
    audio->AudioSampleRate = (count > 0) ? rates[0] : 0;
    audio->SampleRateNum = count;
    for (uint8_t i = 0; i < count && i < sizeof(audio->SampleRateS)/sizeof(audio->SampleRateS[0]); i++) {
        audio->SampleRateS[i] = rates[i];
    }
}

/**
 * @brief  将USB音量值转换为MCU硬件增益值
 * @param UsbVal USB音量值（单位 1/256 dB）
 * @return 转换后的MCU增益值（Q12定点格式）
 */
int16_t UsbValToMcuGain(int16_t UsbVal)
{
	float Db;
	Db = (float)UsbVal/256;
	return (int16_t)roundf(powf(10.0f,(Db/20.0f)) * (1<<12));
}

/**
 * @brief  初始化音频流音量参数
 * @param audio  UsbAudio实例指针
 * @param volume 初始音量值（USB音量值，单位 1/256 dB）
 * @return 无
 */
void UsbAudioVolumeInit(UsbAudio *audio, int16_t volume)
{
	uint8_t ch;
	for(ch = 0; ch < audio->Channels; ch++) {
		audio->Gain[ch] = UsbValToMcuGain(volume);
		audio->Vol[ch]  = volume;
	}
	audio->VolMax    = AUDIO_MAX_VOLUME;
	audio->VolMin    = AUDIO_MIN_VOLUME;
	audio->VolRes    = AUDIO_RES_VOLUME;
}

int16_t UsbAudioGetChannelDb(UsbAudio *audio, uint8_t channel)
{
	if(channel >= audio->Channels)
		return 0;
	return audio->Vol[channel] / 256;  // USB音量值单位1/256dB → dB
}

/**
 * @brief  增益应用核心函数（逐样本逐通道应用定点增益）
 * @param pcm_buf PCM数据缓冲区（原地处理）
 * @param sample  样本数量
 * @param gains   各通道增益数组（Q12格式）
 * @param channel 通道数
 * @return 无
 * @note   内部调用函数，使用定点数乘法实现增益
 */
static void UsbAudioGainApplyCore(uint8_t *pcm_buf,uint32_t sample,int16_t *gains,uint8_t channel)
{
	int32_t s;
	uint8_t ch;
	for(s = 0; s<sample; s++)
	{
		pcm_int *pcm = (pcm_int *)pcm_buf;
		for(ch = 0; ch < channel; ch++)
		{
			pcm[channel * s + ch] = __nds32__clips(((((gain_int)pcm[channel * s + ch]) * gains[ch] + 2048) >> 12), (USB_AUDIO_WIDTH)-1);
		}
	}
}

/**
 * @brief  将USB ISO端点PCM数据转换为内部PCM格式（位宽适配）
 * @param buf     PCM数据缓冲区
 * @param pcmNum  PCM样本数量
 * @param byteSet 原始位宽（PCM16BIT/PCM24BIT/PCM32BIT）
 * @return 无
 * @note   根据CFG_DEFAULT_WIDTH配置进行定点位宽转换
 */
void UsbAudioPcmIsoToInternal(uint8_t *buf, uint32_t pcmNum, uint8_t byteSet)
{
	int32_t s;
	if(byteSet == PCM16BIT)
	{
#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
		for(s = pcmNum - 1; s >= 0; s--)
			((int32_t *)buf)[s] = ((int32_t)(((int16_t *)buf)[s]))<<8;
#endif
	}
	else if(byteSet == PCM24BIT)
	{
#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
		for(s = pcmNum - 1; s >= 0; s--)
			((int32_t *)buf)[s] = (*(int32_t *)(&(buf[3 * s]) - 1))>>8;
#else
		for(s = 0; s < pcmNum; s++)
			((int16_t *)buf)[s] = (int16_t)((*(int32_t *)(&(buf[3 * s]) - 1))>>16);
#endif
	}
	else if(byteSet == PCM32BIT)
	{
#if CFG_DEFAULT_WIDTH == WIDTH_L16BIT
		for(s = 0; s < pcmNum; s++)
			((int16_t *)buf)[s] = (int16_t)(((int32_t *)buf)[s] >> 16);
#elif CFG_DEFAULT_WIDTH == WIDTH_L24BIT
		for(s = pcmNum - 1; s >= 0; s--)
			((int32_t *)buf)[s] = ((int32_t *)buf)[s] >> 8;
#endif
	}
}

/**
 * @brief  将内部PCM格式转换为USB ISO端点PCM数据（位宽适配）
 * @param buf     PCM数据缓冲区
 * @param pcmNum  PCM样本数量
 * @param byteSet 目标位宽（PCM16BIT/PCM24BIT/PCM32BIT）
 * @return 无
 * @note   根据CFG_DEFAULT_WIDTH配置进行定点位宽转换
 */
void UsbAudioPcmInternalToIso(uint8_t *buf, uint32_t pcmNum, uint8_t byteSet)
{
	int32_t s;
	if(byteSet == PCM16BIT)
	{
#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
		int32_t *PcmBuf32 = (int32_t *)buf;
		int16_t *PcmBuf16 = (int16_t *)buf;
		for(s = 0; s < pcmNum; s++)
			PcmBuf16[s] = PcmBuf32[s] >> 8;
#endif
	}
	else if(byteSet == PCM24BIT)
	{
#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
		for(s = 0; s < pcmNum; s++)
			memcpy(&buf[s*3], &buf[s*4], 3);
#else
		for(s = pcmNum - 1; s >= 0; s--)
		{
			buf[s*3+2] = buf[s*2+1];
			buf[s*3+1] = buf[s*2];
		}
#endif
	}
	else if(byteSet == PCM32BIT)
	{
#if CFG_DEFAULT_WIDTH == WIDTH_L16BIT
		for(s = 0; s < pcmNum; s++)
			((int32_t *)buf)[s] = (int32_t)((int16_t *)buf)[s] << 16;
#elif CFG_DEFAULT_WIDTH == WIDTH_L24BIT
		for(s = pcmNum - 1; s >= 0; s--)
			((int32_t *)buf)[s] = (int32_t)((int32_t *)buf)[s] << 8;
#endif
	}
}

/**
 * @brief  从环形缓冲区读取PCM数据
 * @param audio  UsbAudio实例指针
 * @param Buffer 输出数据缓冲区指针
 * @param Len    期望读取的PCM样本数（每通道）
 * @return 实际读取的PCM样本数（每通道）
 */
uint16_t UsbAudioBufDataGet(UsbAudio *audio, void *Buffer, uint16_t Len)
{
	uint32_t Length = 0;
	if(!audio->PCMBuffer)
		return 0;
	Length = Len * sizeof(PCM_DATA_TYPE) * audio->Channels;
	Length = MCUCircular_GetData(&audio->CircularBuf, Buffer, Length);
	Length = Length/(sizeof(PCM_DATA_TYPE)*audio->Channels);

	return Length;
}

/**
 * @brief  获取环形缓冲区中可读的PCM样本数量
 * @param audio UsbAudio实例指针
 * @return 可读 PCM 样本数（每通道）
 */
uint16_t UsbAudioBufDataLen(UsbAudio *audio)
{
	uint32_t Len;
	if(!audio->PCMBuffer)
		return 0;
	Len = MCUCircular_GetDataLen(&audio->CircularBuf);
	Len = Len / (sizeof(PCM_DATA_TYPE) * audio->Channels);
	return Len;
}

/**
 * @brief  获取环形缓冲区总深度（样本数）
 * @param audio    UsbAudio实例指针
 * @param channels 通道数
 * @return 缓冲区总样本容量
 */
uint16_t UsbAudioBufDepth(UsbAudio *audio, uint8_t channels)
{
	uint32_t Len;
	Len = audio->CircularBuf.BufDepth;
	Len = Len / (sizeof(PCM_DATA_TYPE) * channels);
	return Len;
}

/**
 * @brief  向环形缓冲区写入PCM数据
 * @param audio  UsbAudio实例指针
 * @param Buffer 待写入数据缓冲区指针
 * @param Len    待写入的PCM样本数（每通道）
 * @return 请求写入的样本数（Len）；空间不足时写入被跳过仍返回Len
 */
uint16_t UsbAudioBufDataSet(UsbAudio *audio, void *Buffer, uint16_t Len)
{
	if(!audio->PCMBuffer)
		return 0;

	if(MCUCircular_GetSpaceLen(&audio->CircularBuf) > Len * sizeof(PCM_DATA_TYPE) * audio->Channels)
	{
		MCUCircular_PutData(&audio->CircularBuf, Buffer, Len * sizeof(PCM_DATA_TYPE) * audio->Channels);
	}
	return Len;
}

/**
 * @brief  获取环形缓冲区剩余可用空间（样本数）
 * @param audio UsbAudio实例指针
 * @return 可用 PCM 样本数（每通道）
 */
uint16_t UsbAudioBufSpaceLen(UsbAudio *audio)
{
	uint32_t Len;
	if(!audio->PCMBuffer)
		return 0;
	Len = MCUCircular_GetSpaceLen(&audio->CircularBuf);
	Len = Len / (sizeof(PCM_DATA_TYPE) * audio->Channels);
	return Len;
}

/**
 * @brief  对PCM音频缓冲区应用通道增益（含淡入淡出处理）
 * @param audio   UsbAudio实例指针
 * @param Buffer  PCM数据缓冲区
 * @param samples 每通道的PCM样本数（帧数）
 * @return 无
 * @note   静音时增益强制置零，内部调用UsbAudioGainApplyCore
 */
void UsbAudioGainApply(UsbAudio *audio, void *Buffer, uint32_t samples)
{
	uint8_t ch;
	int16_t pregain[CHANNEL_NUM_MAX];

	for(ch = 0; ch < audio->Channels; ch++)
	{
		pregain[ch] = audio->Gain[ch];
		if(audio->Mute)
			pregain[ch] = 0;
		MixerFadeVolume(audio->CurGain[ch], pregain[ch], 100);
	}
	UsbAudioGainApplyCore(Buffer, samples, audio->CurGain, audio->Channels);
}
#endif//#if defined(CFG_UAC_DEVICE_HS_EN) || defined(CFG_UAC_DEVICE_FS_EN)
