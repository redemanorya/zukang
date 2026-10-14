/**
 *****************************************************************************
 * @file     otg_host_hid.h
 * @author   Shanks
 * @version  V1.0.0
 * @date     2024.1.11
 * @brief    host audio V1.0 module driver interface
 *****************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2024 MVSilicon </center></h2>
 */
#ifndef __OTG_HOST_HID_H__
#define __OTG_HOST_HID_H__

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

#include <string.h>
#include "type.h"
#include "otg_host_hcd.h"
#include "otg_host.h"
#define MAX_NUM_HID_INTERFACE        5U

#define	HID_CONSUMER 							0x0C	//Usage Page (Consumer)
#define HID_CONSUMER_VOLUME_INCREMENT           0xE9	//Usage (Volume Increment)
#define HID_CONSUMER_VOLUME_DECREMENT           0xEA	//Usage (Volume Decrement)
#define HID_CONSUMER_PLAY_PAUSE                 0xCD	//Usage (Play/Pause)
#define HID_CONSUMER_NEXT_TRACK                 0xB5	//Usage (Scan Next Track)
#define HID_CONSUMER_PREV_TRACK                 0xB6	//Usage (Scan Previous Track)

typedef struct USBH_HidReportCallbackInfoStruct
{
	uint32_t 	report_id;
	uint32_t 	report_value;
	uint32_t 	report_usage;
} USBH_HidReportCallbackInfo;

typedef struct
{
	PIPE_INFO            Pipe;
	uint16_t             interface;				//接口
	uint8_t              AltSettings;			//接口设置
	uint8_t              bDescriptorType;		//描述符类型
	uint16_t             wDescriptorLength;		//描述符长度
	uint8_t              supported;				//是否支持
} HID_InterfaceControlPropTypeDef;

typedef struct
{
	PIPE_INFO Pipe;
	uint8_t supported;				//是否支持
	uint8_t	*ReportDescriptor;		//报告描述符
	uint16_t wDescriptorLength;		//描述符长度
	uint16_t HidInterfaceNum;

	HID_InterfaceControlPropTypeDef   Interface[MAX_NUM_HID_INTERFACE];
} HID_ClassSpecificDescTypedef;
typedef void (*USBH_HidInputReportCallback) (USBH_HidReportCallbackInfo *);
USBH_StatusTypeDef USBH_HidSetIdle(uint16_t interface);
USBH_StatusTypeDef USBH_HidGetReport(uint16_t interface, uint8_t *buf,uint16_t size);
USBH_StatusTypeDef USBH_HidDescriptorParse(OTG_HOST_INFO *HostInfo,HID_ClassSpecificDescTypedef *Hid_Handle);
void  USBH_HidInputReportDataDecode(void *report_buffer,uint8_t report_buffer_len);
void  USBH_HidInputReportCallbackRegister(USBH_HidInputReportCallback call_back);
uint8_t  USBH_HidReportDescriptorParse(uint8_t *descriptor, uint32_t length);
bool  USBH_HidInputReportHasUsage(uint32_t Usage);
#ifdef __cplusplus
}
#endif // __cplusplus

#endif //__UDISK_H__

