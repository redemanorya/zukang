/*
 * usb_audio_mix.c
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


// 高速混合模式
#ifdef CFG_RES_USB_HS_MIX_AUDIO_EN
bool UsbHSDevicePlayMixInit()
{
	APP_DBG("UsbDevice Play Mix Init\n");

	if(!UsbHSDevicePlayResMalloc(USB_AUDIO_SOURCE_NUM, USB_AUDIO_SINK_NUM))
	{
		APP_DBG("UsbDevicePlayResMalloc Res Error!\n");
		return FALSE;
	}
	UsbHSDevicePlayResInit(USB_AUDIO_SOURCE_NUM, USB_AUDIO_SINK_NUM);
#ifdef CFG_OTG_MODE_AUDIO1_EN
	AudioCoreSourceUnmute(USB_AUDIO_SOURCE1_NUM,TRUE,TRUE);
#endif
#ifdef CFG_OTG_MODE_AUDIO_EN
	AudioCoreSourceUnmute(USB_AUDIO_SOURCE_NUM,TRUE,TRUE);
#endif
	return TRUE;
}
bool UsbHSDevicePlayMixDeinit(void)
{
	APP_DBG("UsbDevice Play Mix Deinit\n");
	NVIC_DisableIRQ(USB_HS_IRQn);
#ifdef CFG_OTG_MODE_AUDIO1_EN
	AudioCoreSourceMute(USB_AUDIO_SOURCE1_NUM,TRUE,TRUE);
	AudioCoreSourceDisable(USB_AUDIO_SOURCE1_NUM);
	AudioCoreSourceDeinit(USB_AUDIO_SOURCE1_NUM);
	UsbAudioSpeaker1.FirstThrhld = FALSE;
#endif
#ifdef CFG_OTG_MODE_AUDIO_EN
	AudioCoreSourceMute(USB_AUDIO_SOURCE_NUM,TRUE,TRUE);
	AudioCoreSourceDisable(USB_AUDIO_SOURCE_NUM);
	AudioCoreSourceDeinit(USB_AUDIO_SOURCE_NUM);
	UsbAudioSpeaker.FirstThrhld = FALSE;
#endif
#ifdef CFG_OTG_MODE_MIC_EN
	AudioCoreSinkDisable(USB_AUDIO_SINK_NUM);
	AudioCoreSinkDeinit(USB_AUDIO_SINK_NUM);
	UsbAudioMic.FirstThrhld = FALSE;
#endif
	NVIC_EnableIRQ(USB_HS_IRQn);
	UsbHSDevicePlayResRelease();
	return TRUE;
}

#endif

// 全速混合模式
#ifdef CFG_RES_USB_FS_MIX_AUDIO_EN

bool UsbFSDevicePlayMixInit(void)
{
	//return 0;
	APP_DBG("UsbFSDevice Play Mix Init\n");

	if(!UsbFSDevicePlayResMalloc(USB_FS_AUDIO_SOURCE_NUM, USB_FS_AUDIO_SINK_NUM))
	{
		APP_DBG("UsbDevicePlayResMalloc Res Error!\n");
		return FALSE;
	}
	UsbFSDevicePlayResInit(USB_FS_AUDIO_SOURCE_NUM, USB_FS_AUDIO_SINK_NUM);
#ifdef CFG_OTG_FS_MODE_AUDIO1_EN
	AudioCoreSourceUnmute(USB_AUDIO_SOURCE1_NUM,TRUE,TRUE);
#endif
#ifdef CFG_OTG_FS_MODE_AUDIO_EN
	AudioCoreSourceUnmute(USB_FS_AUDIO_SOURCE_NUM,TRUE,TRUE);
#endif
	return TRUE;
}

bool UsbFSDevicePlayMixDeinit(void)
{
	APP_DBG("UsbFSDevice Play Mix Deinit\n");
	NVIC_DisableIRQ(USB_FS_WAKEUP_IRQn);
#ifdef CFG_OTG_FS_MODE_AUDIO1_EN
	AudioCoreSourceMute(USB_AUDIO_SOURCE1_NUM,TRUE,TRUE);
	AudioCoreSourceDisable(USB_AUDIO_SOURCE1_NUM);
	AudioCoreSourceDeinit(USB_AUDIO_SOURCE1_NUM);
	UsbAudioSpeaker1.FirstThrhld = FALSE;
#endif
#ifdef CFG_OTG_FS_MODE_AUDIO_EN
	AudioCoreSourceMute(USB_FS_AUDIO_SOURCE_NUM,TRUE,TRUE);
	AudioCoreSourceDisable(USB_FS_AUDIO_SOURCE_NUM);
	AudioCoreSourceDeinit(USB_FS_AUDIO_SOURCE_NUM);
	UsbAudioMixSpeaker.FirstThrhld = FALSE;
#endif
#ifdef CFG_OTG_FS_MODE_MIC_EN
	AudioCoreSinkDisable(USB_FS_AUDIO_SINK_NUM);
	AudioCoreSinkDeinit(USB_FS_AUDIO_SINK_NUM);
	UsbAudioMixMic.FirstThrhld = FALSE;
#endif
	NVIC_EnableIRQ(USB_FS_WAKEUP_IRQn);
	UsbFSDevicePlayResRelease();
	return TRUE;
}
#endif
