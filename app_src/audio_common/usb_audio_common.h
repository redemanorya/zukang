/*
 * usb_audio_common.h
 *
 *  Created on: May 15, 2026
 *      Author: piwang
 */

#ifndef _USB_AUDIO_COMMON_H_
#define _USB_AUDIO_COMMON_H_

void UsbHSDevicePlayResRelease(void);

bool UsbHSDevicePlayResMalloc(uint8_t SourceId, uint8_t SinkId);

bool UsbHSDevicePlayResInit(uint8_t SourceId, uint8_t SinkId);

void UsbFSDevicePlayResRelease(void);

bool UsbFSDevicePlayResMalloc(uint8_t SourceId, uint8_t SinkId);

bool UsbFSDevicePlayResInit(uint8_t SourceId, uint8_t SinkId);

void UsbAudioDeviceHardwareInit(USB_DeviceType_t dev_type);

#endif /* _USB_AUDIO_COMMON_H_ */
