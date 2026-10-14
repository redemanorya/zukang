

#ifndef __OTG_DEVICE_AUDIO20_REQUEST_H__
#define	__OTG_DEVICE_AUDIO20_REQUEST_H__

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus 
#include "type.h"
#include "otg_device.h"

//usb声卡端点相关请求处理
void OTG_DeviceAudio20EpRequest(DeviceCtx_t *ctx);

//usb声卡接口相关请求处理
void OTG_DeviceAudio20InterfaceRequest(DeviceCtx_t *ctx);

//usb声卡音频流接口相关请求处理
void OTG_DeviceAudio20StreamInterfaceRequest(DeviceCtx_t *ctx);
#ifdef __cplusplus
}
#endif // __cplusplus 

#endif //__OTG_DEVICE_AUDIO10_REQUEST_H__

