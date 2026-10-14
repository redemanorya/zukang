/**
 **************************************************************************************
 * @file    usb_audio_mode.c
 * @brief
 *
 * @author  Owen
 * @version V1.0.0
 *
 * $Created: 2018-04-27 13:06:47$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
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
#include "remind_sound.h"
#include "audio_vol.h"
#include "audio_effect.h"
#include "audio_effect_control.h"
#include "debug.h"
#include "device_detect.h"
#include "communication.h"
#include "usb_audio_common.h"
#ifdef DUAL_CORE_EN
#include "dualcore_api.h"
#include "dualcore_dsp.h"
#endif

#if defined CFG_APP_USB_AUDIO_MODE_EN || defined CFG_APP_USB_FS_AUDIO_MODE_EN
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
	/*在此新增独有设备,确保不超*/
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,//8 CH-default
};

USB_DeviceType_t DevCur;

bool UsbDevicePlayResMalloc(USB_DeviceType_t dev_type)
{
	if(dev_type == USB_DEVICE_HS)
	{
	#if defined CFG_APP_USB_AUDIO_MODE_EN || defined CFG_RES_USB_HS_MIX_AUDIO_EN
		return UsbHSDevicePlayResMalloc(USB_AUDIO_SOURCE_NUM, USB_AUDIO_SINK_NUM);
	#endif
	}
	else if(dev_type == USB_DEVICE_FS)
	{
	#if defined CFG_APP_USB_FS_AUDIO_MODE_EN || defined CFG_RES_USB_FS_MIX_AUDIO_EN
		return UsbFSDevicePlayResMalloc(USB_AUDIO_SOURCE_NUM, USB_AUDIO_SINK_NUM);
	#endif
	}
	return FALSE;
}

void UsbDevicePlayResRelease(USB_DeviceType_t dev_type)
{
	if(dev_type == USB_DEVICE_HS)
	{
	#if defined CFG_APP_USB_AUDIO_MODE_EN || defined CFG_RES_USB_HS_MIX_AUDIO_EN
		UsbHSDevicePlayResRelease();
	#endif
	}
	else if(dev_type == USB_DEVICE_FS)
	{
	#if defined CFG_APP_USB_FS_AUDIO_MODE_EN || defined CFG_RES_USB_FS_MIX_AUDIO_EN
		UsbFSDevicePlayResRelease();
	#endif
	}
}

bool UsbDevicePlayResInit(USB_DeviceType_t dev_type)
{
	if(dev_type == USB_DEVICE_HS)
	{
	#if defined CFG_APP_USB_AUDIO_MODE_EN || defined CFG_RES_USB_HS_MIX_AUDIO_EN
		return UsbHSDevicePlayResInit(USB_AUDIO_SOURCE_NUM, USB_AUDIO_SINK_NUM);
	#endif
	}
	else if(dev_type == USB_DEVICE_FS)
	{
	#if defined CFG_APP_USB_FS_AUDIO_MODE_EN || defined CFG_RES_USB_FS_MIX_AUDIO_EN
		return UsbFSDevicePlayResInit(USB_AUDIO_SOURCE_NUM, USB_AUDIO_SINK_NUM);
	#endif
	}
	return FALSE;
}

bool OTG_DeviceAudioSendPcCmd(USB_DeviceType_t dev_type,uint16_t Cmd)
{
	if(dev_type == USB_DEVICE_HS)
	{
	#if defined CFG_APP_USB_AUDIO_MODE_EN || defined CFG_RES_USB_HS_MIX_AUDIO_EN
		return OTG_HS_DeviceAudioSendPcCmd(Cmd);
	#endif
	}
	else if(dev_type == USB_DEVICE_FS)
	{
	#if defined CFG_APP_USB_FS_AUDIO_MODE_EN || defined CFG_RES_USB_FS_MIX_AUDIO_EN
		return OTG_FS_DeviceAudioSendPcCmd(Cmd);
	#endif
	}
	return FALSE;
}

bool OTG_DeviceAudioSendHidKeyDown(USB_DeviceType_t dev_type,uint16_t Cmd)
{
	if(dev_type == USB_DEVICE_HS)
	{
	#if defined CFG_APP_USB_AUDIO_MODE_EN || defined CFG_RES_USB_HS_MIX_AUDIO_EN
		return OTG_HS_DeviceAudioSendHidKeyDown(Cmd);
	#endif
	}
	else if(dev_type == USB_DEVICE_FS)
	{
	#if defined CFG_APP_USB_FS_AUDIO_MODE_EN || defined CFG_RES_USB_FS_MIX_AUDIO_EN
		return OTG_FS_DeviceAudioSendHidKeyDown(Cmd);
	#endif
	}
	return FALSE;
}
bool OTG_DeviceAudioSendHidKeyUp(USB_DeviceType_t dev_type)
{
	if(dev_type == USB_DEVICE_HS)
	{
	#if defined CFG_APP_USB_AUDIO_MODE_EN || defined CFG_RES_USB_HS_MIX_AUDIO_EN
		return OTG_HS_DeviceAudioSendHidKeyUp();
	#endif
	}
	else if(dev_type == USB_DEVICE_FS)
	{
	#if defined CFG_APP_USB_FS_AUDIO_MODE_EN || defined CFG_RES_USB_FS_MIX_AUDIO_EN
		return OTG_FS_DeviceAudioSendHidKeyUp();
	#endif
	}
	return FALSE;
}

//USB声卡模式参数配置，资源初始化
bool UsbDevicePlayInit(void)
{
	if(GetSystemMode() == ModeUsbDevicePlay)
	DevCur = USB_DEVICE_HS;
	else if(GetSystemMode() == ModeUsbFsDevicePlay)
	DevCur = USB_DEVICE_FS;

	APP_DBG("UsbDevice Play Init %d\n",DevCur);
	//1st：DMA通道初始化
	DMA_ChannelAllocTableSet(DMA0, (uint8_t* )Dma0ChannelMap);
	DMA_ChannelAllocTableSet(DMA1, (uint8_t* )Dma1ChannelMap);
	//2st：usb声卡相关硬件配置
	if(!SoftFlagGet(SoftFlagSysCurModeReboot))
	{
		UsbAudioDeviceHardwareInit(DevCur);
	}
	//3st：配置系统标准通路
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
	//4st：usb声卡通路资源申请及配置
	if(!UsbDevicePlayResMalloc(DevCur))
	{
		APP_DBG("UsbDevicePlayResMalloc Res Error!\n");
		return FALSE;
	}
	if(!UsbDevicePlayResInit(DevCur))
	{
		APP_DBG("UsbDevicePlayResInit Res Error!\n");
		return FALSE;
	}
#ifdef AUDIO_STREAM_UNINTERRUPT
	if(IsResRamReallocate == TRUE)
#endif
	{
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

	//Core Process
#ifdef CFG_FUNC_ROBOEFFECT_EN
	//AudioEffect_SourceGain_Update(APP_SOURCE_NUM, AudioVol_GetMaxLevel()); //设置到最大
	AudioEffect_PresetSet(&EffectParambin, EFFECT_MUSICGAIN, INDEX_GAIN_CONTROL_GAIN, VOL_PRESET, AudioVol_GetMaxLevel());
#ifdef AUDIO_STREAM_UNINTERRUPT
	if(IsResRamReallocate == TRUE)
#endif
	{
		AudioCoreProcessConfig((AudioCoreProcessFunc)AudioMusicProcess);
	}
#else
#ifdef AUDIO_STREAM_UNINTERRUPT
	if(IsResRamReallocate == TRUE)
#endif
	{
		AudioCoreProcessConfig((AudioCoreProcessFunc)AudioBypassProcess);
	}
#endif

	AudioCodecHardwareSync();//update hardware config

#if defined(CFG_COMMUNICATION_BY_HS_USB) && defined(CFG_PARA_DEVICE_HS_DET)
	if(GetSystemMode() == ModeUsbDevicePlay)
		SetUSBHsDeviceInitState(TRUE);
#endif
#if defined(CFG_COMMUNICATION_BY_FS_USB) && defined(CFG_PARA_DEVICE_FS_DET)
	if(GetSystemMode() == ModeUsbFsDevicePlay)
		SetUSBFsDeviceInitState(TRUE);
#endif

#ifdef CFG_FUNC_REMIND_SOUND_EN
	if(!SoftFlagGet(SoftFlagSysCurModeReboot) && RemindSoundServiceItemRequest(SOUND_REMIND_SHENGKAM, REMIND_PRIO_NORMAL) == FALSE)
#endif
	{
		if(IsAudioPlayerMute() == TRUE)
		{
			HardWareMuteOrUnMute();
		}
	}
#if defined CFG_OTG_MODE_AUDIO1_EN || defined CFG_OTG_FS_MODE_AUDIO1_EN
	AudioCoreSourceUnmute(USB_AUDIO_SOURCE1_NUM,TRUE,TRUE);
#endif
#if defined CFG_OTG_MODE_AUDIO_EN || defined CFG_OTG_FS_MODE_AUDIO_EN
	AudioCoreSourceUnmute(USB_AUDIO_SOURCE_NUM,TRUE,TRUE);
#endif

	return TRUE;
}

void UsbDevicePlayRun(uint16_t msgId)
{
	switch(msgId)
	{
		case MSG_HID_KEY1_DOWN:
			APP_DBG("KEY1_DOWN\n");
			OTG_DeviceAudioSendHidKeyDown(DevCur,AUDIO_VOL_UP);
			if(IsAudioPlayerMute() == TRUE)
			{
				HardWareMuteOrUnMute();
			}
			break;

		case MSG_HID_KEY2_DOWN:
			APP_DBG("KEY2_DOWN\n");
			OTG_DeviceAudioSendHidKeyDown(DevCur,AUDIO_VOL_DN);
			if(IsAudioPlayerMute() == TRUE)
			{
				HardWareMuteOrUnMute();
			}
			break;

		case MSG_HID_KEY3_DOWN:
			APP_DBG("KEY3_DOWN\n");
			OTG_DeviceAudioSendHidKeyDown(DevCur,AUDIO_PP);
			break;

		case MSG_HID_KEY_UP:
			APP_DBG("KEY_UP\n");
			OTG_DeviceAudioSendHidKeyUp(DevCur);
			break;

		case MSG_PLAY_PAUSE:
			APP_DBG("Play Pause\n");
			OTG_DeviceAudioSendPcCmd(DevCur,AUDIO_PP);
			break;

		case MSG_PRE:
			APP_DBG("PRE Song\n");
			OTG_DeviceAudioSendPcCmd(DevCur,AUDIO_PREV);
			break;

		case MSG_NEXT:
			APP_DBG("next Song\n");
			OTG_DeviceAudioSendPcCmd(DevCur,AUDIO_NEXT);
			break;

		case MSG_USB_MUSIC_VOLUP:
			APP_DBG("VOLUP\n");
			OTG_DeviceAudioSendPcCmd(DevCur,AUDIO_VOL_UP);
			if(IsAudioPlayerMute() == TRUE)
			{
				HardWareMuteOrUnMute();
			}
			break;

		case MSG_USB_MUSIC_VOLDOWN:
			APP_DBG("VOLDOWN\n");
			OTG_DeviceAudioSendPcCmd(DevCur,AUDIO_VOL_DN);
			if(IsAudioPlayerMute() == TRUE)
			{
				HardWareMuteOrUnMute();
			}
			break;

		default:
			CommonMsgProccess(msgId);
			break;
	}
#ifdef CFG_FUNC_USB_PROTOCOL_AUTO
	OTG_HS_DeviceAudioModeAutoSwitchProcess();
#endif

#ifdef USB_CRYSTA_FREE_EN
	Clock_USBCrystaFreeAdjustProcess();
#endif

	OTG_DeviceRequestProcess(DevCur);
#if defined CFG_OTG_MODE_READER_EN || defined CFG_OTG_FS_MODE_READER_EN
	OTG_DeviceStorProcess(DevCur);
#endif
}

bool UsbDevicePlayDeinit(void)
{
	APP_DBG("UsbDevice Play Deinit\n");
#if defined CFG_OTG_MODE_AUDIO1_EN || defined CFG_OTG_FS_MODE_AUDIO1_EN
	AudioCoreSourceMute(USB_AUDIO_SOURCE1_NUM,TRUE,TRUE);
#endif
#if defined CFG_OTG_MODE_AUDIO_EN || defined CFG_OTG_FS_MODE_AUDIO_EN
	AudioCoreSourceMute(USB_AUDIO_SOURCE_NUM,TRUE,TRUE);
#endif
#ifdef AUDIO_STREAM_UNINTERRUPT
	if(IsResRamRelease == TRUE)
#endif
	{
	if(IsAudioPlayerMute() == FALSE)
	{
		HardWareMuteOrUnMute();
	}	
	
	PauseAudioCore();

	//注意：AudioCore父任务调整到mainApp下，此处只关闭AudioCore通道，不关闭任务
	AudioCoreProcessConfig((void*)AudioNoAppProcess);
#if defined CFG_OTG_MODE_AUDIO1_EN || defined CFG_OTG_FS_MODE_AUDIO1_EN
	AudioCoreSourceDisable(USB_AUDIO_SOURCE1_NUM);
#endif
	}
#if defined CFG_OTG_MODE_AUDIO_EN || defined CFG_OTG_FS_MODE_AUDIO_EN
	AudioCoreSourceDisable(USB_AUDIO_SOURCE_NUM);
#endif
#if defined CFG_OTG_MODE_MIC_EN || defined CFG_OTG_FS_MODE_MIC_EN
	AudioCoreSinkDisable(USB_AUDIO_SINK_NUM);
#endif
	if(!SoftFlagGet(SoftFlagSysCurModeReboot))
	{
	#if defined(CFG_COMMUNICATION_BY_HS_USB)
		if(GetSystemMode() == ModeUsbDevicePlay)
		{
			OTG_DeviceHardwareInit(USB_DEVICE_HS, HID, USB_VID, USBPID(HID));
			DBG("OTG HS Detect:HID\n");
		}
	#endif
	#if defined(CFG_COMMUNICATION_BY_FS_USB)
		if(GetSystemMode() == ModeUsbFsDevicePlay)
		{
			OTG_DeviceHardwareInit(USB_DEVICE_FS, FS_HID, USB_VID, USBPID(FS_HID));
			DBG("OTG FS Detect:HID\n");
		}
	#endif
		//恢复其他模式的音量值
		AudioMusicVolSet(mainAppCt.MusicVolume);
	}
	UsbDevicePlayResRelease(DevCur);
#if defined CFG_OTG_MODE_AUDIO1_EN || defined CFG_OTG_FS_MODE_AUDIO1_EN
	AudioCoreSourceDeinit(USB_AUDIO_SOURCE1_NUM);
#endif
#if defined CFG_OTG_MODE_AUDIO_EN || defined CFG_OTG_FS_MODE_AUDIO_EN
	AudioCoreSourceDeinit(USB_AUDIO_SOURCE_NUM);
#endif
#if defined CFG_OTG_MODE_MIC_EN || defined CFG_OTG_FS_MODE_MIC_EN
	AudioCoreSinkDeinit(USB_AUDIO_SINK_NUM);
#endif
#ifdef AUDIO_STREAM_UNINTERRUPT
	if(IsResRamRelease == TRUE)
#endif
	{
		ModeCommonDeinit();//通路全部释放
	}

	return TRUE;
}

#endif //CFG_APP_USB_AUDIO_MODE_EN

