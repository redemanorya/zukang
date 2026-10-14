/**
 *****************************************************************************
 * @file     otg_device_audio10_request.c
 * @author   Shanks
 * @version  V1.0.0
 * @date     7-September-2015
 * @brief    USB 音频设备类(UAC1.0) 协议控制层驱动
 *****************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
 *****************************************************************************
 * @brief  文件功能说明：
 *  1. 实现 USB Audio Class 1.0 标准协议请求处理
 *  2. 处理 PC 主机下发的音量、静音等控制命令
 *  3. 实现采样率获取、设置与自动切换
 *  4. 音频接口(Interface)、端点(Endpoint)请求解析
 *  5. 多音频流（Speaker/Mic）统一管理
 *  6. 提供音量值与硬件增益自动转换接口
 *****************************************************************************
 */
//driver
#include <string.h>
#include "otg_device_hcd.h"
//application
#include "otg_device_audio.h"
#include "otg_device.h"
#include "debug.h"

#define RequestCmd	((Setup[0] << 8) | Setup[1])
#define Channel		Setup[2]
#define Control		Setup[3]
#define Entity		Setup[5]
#define wLength		((Setup[7] << 8) | Setup[6])

/**
 * @brief  UAC1.0 音量/静音控制请求处理
 * @param ctx   USB设备上下文指针
 * @param Stream 音频流实例指针
 * @return 无
 */
static void Audio10VolumeRequest(DeviceCtx_t *ctx,UsbAudio *Stream)
{
	uint8_t *Setup = ctx->Setup;
	uint8_t *Request = ctx->Request;

	if(Control == AUDIO_CONTROL_MUTE)
	{
		if(RequestCmd == GET_CUR)
		{
			OTG_DeviceControlInTransfer(ctx->type,&Stream->Mute,1);
		}
		else if(RequestCmd == SET_CUR)
		{
			Stream->Mute=Request[0];
			if(Stream->VolumeSet)
			{
				Stream->VolumeSet(0xFF);
			}
		}
		else
		{
			//APP_DBG("%s %d\n",__FILE__,__LINE__);
		}
	}
	else if(Control == AUDIO_CONTROL_VOLUME)
	{
		if(RequestCmd == GET_MIN)
		{
			OTG_DeviceControlInTransfer(ctx->type,(uint8_t*)&Stream->VolMin,2);
		}
		else if(RequestCmd == GET_MAX)
		{
			OTG_DeviceControlInTransfer(ctx->type,(uint8_t*)&Stream->VolMax,2);
		}
		else if(RequestCmd == GET_RES)
		{
			OTG_DeviceControlInTransfer(ctx->type,(uint8_t*)&Stream->VolRes,2);
		}
		else if(RequestCmd == GET_CUR)
		{
			uint8_t ch_idx = (Channel == 0x00) ? 0 : (Channel - 1);
			if(ch_idx < Stream->Channels)
			{
				OTG_DeviceControlInTransfer(ctx->type,(uint8_t*)&Stream->Vol[ch_idx],2);
			}
		}
		else if(RequestCmd == SET_CUR)
		{
			uint8_t i;
			int16_t Temp = 0;
			Temp = Request[1]* 256 + Request[0];
			if(Channel == 0x00 && Setup[2] == 0x00)
			{
				for(i = 0; i < Stream->Channels; i++)
				{
					Stream->Vol[i] = Temp;
					Stream->Gain[i] = UsbValToMcuGain(Temp);
					if(Stream->VolumeSet)
					{
						Stream->VolumeSet(i);
					}
				}
			}
			else if(Channel >= 0x01 && Channel <= Stream->Channels)
			{
				Stream->Vol[Channel - 1] = Temp;
				Stream->Gain[Channel - 1] = UsbValToMcuGain(Temp);
				if(Stream->VolumeSet)
				{
					Stream->VolumeSet(Channel - 1);
				}
			}
		}
		else
		{
			//APP_DBG("%s %d\n",__FILE__,__LINE__);
		}
	}
}
/**
 * @brief  UAC1.0 采样率控制请求处理（获取/设置/范围查询）
 * @param ctx   USB设备上下文指针
 * @param Stream 音频流实例指针
 * @return 无
 */
static void Audio10SampleRateRequest(DeviceCtx_t *ctx,UsbAudio *Stream)
{
	uint8_t *Setup = ctx->Setup;
	uint8_t *Request = ctx->Request;
	if(RequestCmd == SET_CUR_EP)
	{
		if(Stream->AudioSampleRate != SWAP_BUF_TO_U32(Request))
		{
			Stream->AudioSampleRate = SWAP_BUF_TO_U32(Request);
			if(Stream->SampleRateSet)
			{
				Stream->SampleRateSet();
			}
		}
	}
	if(RequestCmd == GET_CUR_EP)
	{
		OTG_DeviceControlInTransfer(ctx->type,(uint8_t*)&Stream->AudioSampleRate,3);
	}
}
/**
 * @brief  UAC1.0 音频控制接口请求分发处理（音量+采样率）
 * @param ctx USB设备上下文指针
 * @return 无
 */
void OTG_DeviceAudio10InterfaceRequest(DeviceCtx_t *ctx)
{
    UsbAudioClass* UsbAudio = ((UsbCompositeClass*)ctx->pClassData)->pAudio;

	uint8_t *Setup = ctx->Setup;
	uint8_t i = 0;
	for(i=0; i<UsbAudio->registered_count; i++)
	{
		if(Entity == UsbAudio->registered_audio[i]->FuId)
		{
			Audio10VolumeRequest(ctx,UsbAudio->registered_audio[i]);
		}
	}
}

/**
 * @brief  UAC1.0 音频流接口请求处理
 * @param ctx USB设备上下文指针
 * @return 无
 */
void OTG_DeviceAudio10StreamInterfaceRequest(DeviceCtx_t *ctx)
{

}

/**
 * @brief  UAC1.0 端点请求处理（采样率相关）
 * @param ctx USB设备上下文指针
 * @return 无
 */
void OTG_DeviceAudio10EpRequest(DeviceCtx_t *ctx)
{
    UsbAudioClass* UsbAudio = ((UsbCompositeClass*)ctx->pClassData)->pAudio;

	uint8_t *Setup = ctx->Setup;
	uint8_t i = 0;
	for(i=0; i<UsbAudio->registered_count; i++)
	{
		if(Setup[4] == UsbAudio->registered_audio[i]->EpNum)
		{
			Audio10SampleRateRequest(ctx,UsbAudio->registered_audio[i]);
		}
	}
}


