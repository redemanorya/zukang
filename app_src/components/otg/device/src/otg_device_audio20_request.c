/**
 *****************************************************************************
 * @file     otg_device_audio20_request.c
 * @author   Shanks
 * @version  V1.0.0
 * @date     7-September-2015
 * @brief    USB 音频设备类(UAC2.0) 协议控制层驱动
 *****************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
 *****************************************************************************
 * @brief  文件功能说明：
 *  1. 实现 USB Audio Class 2.0 标准协议请求处理
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
 * @brief  将16位值以小端序写入缓冲区
 * @param address 目标缓冲区指针
 * @param value   待写入的16位数据
 * @return 无
 */
void data_uint16_put(uint8_t * address, uint16_t value)
{
    *address++ =  (uint8_t) (value & 0xff);
    *address = (uint8_t) ((value >> 8) & 0xff);
    return;
}

/**
 * @brief  将32位值以小端序写入缓冲区
 * @param address 目标缓冲区指针
 * @param value   待写入的32位数据
 * @return 无
 */
void data_uint32_put(uint8_t * address, uint32_t value)
{
    *address++ =  (uint8_t) (value & 0xff);
    *address++ =  (uint8_t) ((value >> 8) & 0xff);
    *address++ =  (uint8_t) ((value >> 16) & 0xff);
    *address =    (uint8_t) ((value >> 24) & 0xff);
    return;
}

/**
 * @brief  UAC2.0 音量/静音控制请求处理
 * @param ctx   USB设备上下文指针
 * @param Stream 音频流实例指针
 * @return 无
 */
static void Audio20VolumeRequest(DeviceCtx_t *ctx,UsbAudio *Stream)
{
	uint8_t *Setup = ctx->Setup;
	uint8_t *Request = ctx->Request;

	if(Control == AUDIO_CONTROL_MUTE)
	{
		//Speaker mute的操作
		if(RequestCmd == GET_CUR_2)
		{
			OTG_DeviceControlInTransfer(ctx->type,&Stream->Mute,1);
		}
		else if(RequestCmd == SET_CUR)
		{
//			APP_DBG("mute: %d\n", Request[0]);
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
		if(RequestCmd == GET_RANGE)
		{
			data_uint16_put(ctx->Resp,0x01);
			data_uint16_put(ctx->Resp+2,Stream->VolMin);
			data_uint16_put(ctx->Resp+4,Stream->VolMax);
			data_uint16_put(ctx->Resp+6,Stream->VolRes);
			OTG_DeviceControlInTransfer(ctx->type,ctx->Resp,8);
		}
		else if(RequestCmd == GET_CUR_2)
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
			uint16_t Temp = 0;
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
	}
}

/**
 * @brief  UAC2.0 采样率控制请求处理（获取/设置/范围查询）
 * @param ctx   USB设备上下文指针
 * @param Stream 音频流实例指针
 * @return 无
 */
static void Audio20SampleRateRequest(DeviceCtx_t *ctx,UsbAudio *Stream)
{
	uint8_t *Setup = ctx->Setup;
	uint8_t *Request = ctx->Request;
	if(RequestCmd == SET_CUR)
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
	else if(RequestCmd == GET_CUR_2)
	{
		if(Control == CS_CLOCK_VALID_CONTROL)
		{
			if(Stream->AudioSampleRate == 0)
			{
				Stream->SampleRateValid = 0;
			}else{
				Stream->SampleRateValid = 1;
			}
			OTG_DeviceControlInTransfer(ctx->type,(uint8_t*)&Stream->SampleRateValid,wLength);
		}
		else
		{
			OTG_DeviceControlInTransfer(ctx->type,(uint8_t*)&Stream->AudioSampleRate,wLength);
		}
	}
	else if(RequestCmd == GET_RANGE)
	{
		int i;
		uint8_t block_size;
		for(i=0; i<Stream->SampleRateNum; i++)
		{
			data_uint32_put(ctx->Resp+(i*12+2),Stream->SampleRateS[i]);	/* dMIN(1) */
			data_uint32_put(ctx->Resp+(i*12+6),Stream->SampleRateS[i]);	/* dMAX(1) */
			data_uint32_put(ctx->Resp+(i*12+10),0);						/* dMIN(1) */
		}
		data_uint16_put(ctx->Resp,Stream->SampleRateNum);
		block_size = Stream->SampleRateNum*12+2;
        if(wLength > block_size)
        {
        	OTG_DeviceControlInTransfer(ctx->type,ctx->Resp,block_size);
        }
        else
        {
        	OTG_DeviceControlInTransfer(ctx->type,ctx->Resp,wLength);
        }
	}
}
/**
 * @brief  UAC2.0 音频控制接口请求分发处理（音量+采样率）
 * @param ctx USB设备上下文指针
 * @return 无
 */
void OTG_DeviceAudio20InterfaceRequest(DeviceCtx_t *ctx)
{
    UsbAudioClass* UsbAudio = ((UsbCompositeClass*)ctx->pClassData)->pAudio;

	uint8_t *Setup = ctx->Setup;
	uint8_t i = 0;
	for(i=0; i<UsbAudio->registered_count; i++)
	{
		if(Entity == UsbAudio->registered_audio[i]->FuId)
		{
			Audio20VolumeRequest(ctx,UsbAudio->registered_audio[i]);
		}
	}

	for(i=0; i<UsbAudio->registered_count; i++)
	{
		if(Entity == UsbAudio->registered_audio[i]->ClkId)
		{
			Audio20SampleRateRequest(ctx,UsbAudio->registered_audio[i]);
		}
	}
}
const uint8_t para_block_Alt[] = {
	0x01,
#ifdef MIC_ALT2_EN
	0x07
#else
	0x03
#endif
};

/**
 * @brief  UAC2.0 音频流接口请求处理（备用设置查询）
 * @param ctx USB设备上下文指针
 * @return 无
 */
void OTG_DeviceAudio20StreamInterfaceRequest(DeviceCtx_t *ctx)
{
	uint8_t *Setup = ctx->Setup;
	if((Entity == 0))
	{
		if(Channel == 0 && Control == AS_VAL_ALT_SETTINGS_CONTROL)
		{
			if(RequestCmd == GET_CUR_2)
			{
				OTG_DeviceControlInTransfer(ctx->type,(uint8_t*)para_block_Alt,sizeof(para_block_Alt));
			}
		}
	}
}

/**
 * @brief  UAC2.0 端点请求处理
 * @note   暂未定义具体指令
 * @param ctx USB设备上下文指针
 * @return 无
 */
void OTG_DeviceAudio20EpRequest(DeviceCtx_t *ctx)
{
	//usb声卡2.0协议暂无端点指令
}
