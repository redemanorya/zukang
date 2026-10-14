/**
 *****************************************************************************
 * @file     otg_device_descriptor.c
 * @author   Shanks
 * @version  V1.0.0
 * @date     7-September-2015
 * @brief    USB 设备描述符定义文件
 *****************************************************************************
 * @attention
 * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
 *****************************************************************************
 * @brief  文件功能说明：
 *  1. 定义 USB 标准设备描述符、设备限定符、BOS、语言ID描述符
 *  2. 提供多套配置描述符：纯音频(AUDIO_ONLY)、纯麦(MIC_ONLY)、音频+麦(AUDIO_MIC)、HID、大容量存储(READER)
 *  3. 支持 UAC1.0 / UAC2.0 双协议，兼容 USB FullSpeed / HighSpeed
 *  4. 定义多媒体控制 HID 报告描述符、自定义数据 HID 报告描述符
 *  5. 提供描述符查表管理结构，支持动态切换设备模式
 *****************************************************************************
 */
#include "type.h"
#include "otg_device_hcd.h"
#include "otg_device_descriptor.h"
#include "otg_device_audio.h"

#if defined(CFG_UAC_DEVICE_HS_EN) || defined(CFG_UAC_DEVICE_FS_EN) || defined(CFG_COMMUNICATION_BY_HS_USB) || defined(CFG_COMMUNICATION_BY_FS_USB)

const uint8_t  DeviceQualifier[10] = {10,6,0x10,0x01,0,0,0,64,1,0};
const uint8_t  DeviceBos[12] = {
		0x05,        // bLength
		0x0F,        // bDescriptorType (Unknown)
		0x0C, 0x00, 0x01,
		0x07,        // bLength
		0x10,        // bDescriptorType (Unknown)
		0x02, 0x00, 0x00, 0x00, 0x00,
		// 12 bytes
};
const uint8_t DeviceString_LangID[4] = {0x04, 0x03, 0x09, 0x04};

//音乐控制命令 HID报告描述符
#ifndef CFG_FUNC_USB_TYPEC_HEADSET_CONFIG
const uint8_t AudioCtrlReportDescriptor[67] =
{
	0x05, 0x0C,        // Usage Page (Consumer)
	0x09, 0x01,        // Usage (Consumer Control)
	0xA1, 0x01,        // Collection (Application)
	0x85, 0x01,        //   Report ID (1)
	0x15, 0x00,        //   Logical Minimum (0)
	0x25, 0x01,        //   Logical Maximum (1)
	0x75, 0x01,        //   Report Size (1)
	0x95, 0x01,        //   Report Count (1)
	0x09, 0xE9,        //   Usage (Volume Increment)
	0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
	0x09, 0xEA,        //   Usage (Volume Decrement)
	0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
	0x09, 0xCD,        //   Usage (Play/Pause)
	0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
	0x09, 0xB5,        //   Usage (Scan Next Track)
	0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
	0x09, 0xB6,        //   Usage (Scan Previous Track)
	0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
	0x09, 0xB7,        //   Usage (Stop)
	0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
	0x09, 0xB3,        //   Usage (Fast Forward)
	0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
	0x09, 0xB4,        //   Usage (Rewind)
	0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
	0x05, 0x0B,        //   Usage Page (Telephony)
	0x09, 0x24,        //   Usage (Redial)
	0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
	0x09, 0x20,        //   Usage (Hook Switch)
	0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
	0x09, 0x2F,        //   Usage (Phone Mute)
	0x81, 0x06,        //   Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
	0x95, 0x05,        //   Report Count (5)
	0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
	0xC0,              // End Collection
};
#else
const uint8_t AudioCtrlReportDescriptor[47] =
{
	0x05, 0x0C,        // Usage Page (Consumer)
	0x09, 0x01,        // Usage (Consumer Control)
	0xA1, 0x01,        // Collection (Application)
	0x85, 0x01,        //   Report ID (1)
	0x15, 0x00,        //   Logical Minimum (0)
	0x25, 0x01,        //   Logical Maximum (1)
	0x75, 0x01,        //   Report Size (1)
	0x95, 0x01,        //   Report Count (1)
	0x05, 0x0C,        //   Usage Page (Consumer)
	0x09, 0xE9,        //   Usage (Volume Increment)
	0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
	0x09, 0xEA,        //   Usage (Volume Decrement)
	0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
	0x09, 0xCD,        //   Usage (Play/Pause)
	0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
	0x95, 0x06,        //   Report Count (6)
	0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
	0x95, 0x01,        //   Report Count (1)
	0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
	0x95, 0x04,        //   Report Count (4)
	0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
	0x95, 0x02,        //   Report Count (2)
	0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
	0xC0,              // End Collection

	// 47 bytes
};
#endif
//在线调音HID报告描述符
const uint8_t HidDataReportDescriptor[36] =
{
	0x06, 0x00, 0xFF,
	0x0A, 0xAA, 0x55,
	0xA1, 0x01,
	0x15, 0x00,
	0x26, 0xFF, 0x00,
	0x75, 0x08,

	0x96, 0x00, 0x01,
	0x09, 0x01,
	0x81, 0x02,

	0x96, 0x00, 0x01,
	0x09, 0x01,
	0x91, 0x02,

	0x95, 0x08,
	0x09, 0x01,
	0xB1, 0x02,
	0xC0
};
#if defined(CFG_FUNC_USBDEBUG_EN) && defined(CFG_COMMUNICATION_BY_FS_USB)
/* 独立的FS中断IN日志接口。每个报告固定64字节：4字节包头+最多60字节日志。 */
const uint8_t HidLogReportDescriptor[19] =
{
	0x06, 0x02, 0xFF,       // Usage Page (Vendor Defined 0xFF02)
	0x09, 0x01,             // Usage (1)
	0xA1, 0x01,             // Collection (Application)
	0x15, 0x00,             // Logical Minimum (0)
	0x26, 0xFF, 0x00,       // Logical Maximum (255)
	0x75, 0x08,             // Report Size (8 bits)
	0x95, 0x40,             // Report Count (64 bytes)
	0x81, 0x02,             // Input (Data,Var,Abs)
	0xC0
};
#endif
#ifdef CFG_DESCRIP_HID_HEART_BEAT_EN
const uint8_t HeartBeatReportDescriptor[24] =
{
	0x06, 0x01, 0xFF,  // Usage Page (Vendor Defined 0xFF01)
	0x0A, 0x00, 0x55,  // Usage (0x5500)
	0xA1, 0x01,        // Collection (Application)
	0x15, 0x00,        //   Logical Minimum (0)
	0x26, 0xFF, 0x00,  //   Logical Maximum (255)
	0x85, 0x01,        //   Report ID (1)
	0x75, 0x08,        //   Report Size (8 bits = 1字节)
	0x95, 0x01,        //   Report Count (1 → 仅1个字节)
	0x09, 0x01,        //   Usage (0x01)
	0x81, 0x02,        //   Input (Data,Var,Abs)
	0xC0              // End Collection
};
#endif
uint8_t DeviceDescriptor[18] =
{
	0x12, 		// bLength
	0x01, 		// bDescriptorType
	0x10, 0x01,	// bcdUSB
	0x00, 		// bDeviceClass
	0x00, 		// bDeviceSubClass
	0x00, 		// bDeviceProtocol
	0x40,		// bMaxPacketSize0
	0x00, 0x00, // idVendor
	0x00, 0x00,	// idProduct
	0x00, 0x01, // bcdDevice
	0x01, 		// iManufacturer
	0x02,		// iProduct
	0x03,		// iSerialNumber
	0x01,		// bNumConfigurations
};

uint8_t DeviceDescriptorMix[18] =
{
	0x12, 		// bLength
	0x01, 		// bDescriptorType
	0x10, 0x01,	// bcdUSB
	0x00, 		// bDeviceClass
	0x00, 		// bDeviceSubClass
	0x00, 		// bDeviceProtocol
	0x40,		// bMaxPacketSize0
	0x00, 0x00, // idVendor
	0x00, 0x00,	// idProduct
	0x00, 0x01, // bcdDevice
	0x01, 		// iManufacturer
	0x02,		// iProduct
	0x03,		// iSerialNumber
	0x01,		// bNumConfigurations
};



#if defined(CFG_DESCRIP_AUDIO_ONLY_EN)
	//声卡 MIC输入和AUDIO输出
	const uint8_t ConfigDescriptor_Tab(AUDIO_ONLY)[] =
	{
	#if HID_DATA_FUN_EN
		#ifdef HID_DATA_EP_EN
		USB_CFG_DESC_HEADER(0xA2 + USER_HS_CONFIG_DESCRIPTOR_UAC_20_SIZE +32,4),
		#else
		USB_CFG_DESC_HEADER(0xA2 + USER_HS_CONFIG_DESCRIPTOR_UAC_20_SIZE +18,4),
		#endif
	#else
		USB_CFG_DESC_HEADER(0xA2 + USER_HS_CONFIG_DESCRIPTOR_UAC_20_SIZE ,3),
	#endif

		UAC2_AC_TERMINAL_ASSOCIATION_DESC(0,2),

		UAC2_AC_STD_INTF_DESC(0x00,0x00),

		UAC2_AC_HEADER_DESC(AUDIO_HEADSET,(0x34+(PACKET_CHANNELS_NUM*4))),
		UAC2_AC_CLK_SOURCE_DESC(AUDIO_SPEAKER_CLK_ID),      													// 扬声器时钟源

		UAC2_AC_IT_DESC(AUDIO_SPEAKER_IT_ID, AUDIO_SPEAKER_CLK_ID, PACKET_CHANNELS_NUM, 0x0101), 				// 扬声器输入终端
		UAC2_AC_FU_DESC(AUDIO_SPEAKER_FU_ID, AUDIO_SPEAKER_IT_ID, PACKET_CHANNELS_NUM),             			// 扬声器功能单元
		UAC2_AC_OT_DESC(AUDIO_SPEAKER_OT_ID, AUDIO_SPEAKER_CLK_ID, AUDIO_SPEAKER_FU_ID, SPEAKER_TYPE),    			// 扬声器输出终端

		//扬声器AS Alt0（无流传输）
		UAC2_AS_INTERFACE_DESC(0x01, 0x00, 0x00),
		//扬声器AS Alt1
		UAC2_AS_INTERFACE_DESC(0x01, 0x01, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_SPEAKER_IT_ID, PACKET_CHANNELS_NUM),  								// 扬声器AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(SPEAKER_ALT1_BITS),                   									// 扬声器Alt1位宽描述符
		UAC2_AS_HS_ENDPOINT_DESC(DEVICE_ISO_OUT_EP, USBD_AUDIO_FREQ, PACKET_CHANNELS_NUM, SPEAKER_ALT1_BITS), 	// 扬声器Alt1端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 扬声器Alt1端点通用描述符

	#ifdef SPEAKER_ALT2_EN
		//扬声器AS Alt2
		UAC2_AS_INTERFACE_DESC(0x01, 0x02, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_SPEAKER_IT_ID, PACKET_CHANNELS_NUM),  								// 扬声器AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(SPEAKER_ALT2_BITS),                   									// 扬声器Alt2位宽描述符
		UAC2_AS_HS_ENDPOINT_DESC(DEVICE_ISO_OUT_EP, USBD_AUDIO_FREQ, PACKET_CHANNELS_NUM, SPEAKER_ALT2_BITS), 	// 扬声器Alt2端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 扬声器Alt2端点通用描述符
	#endif
	#ifdef SPEAKER_ALT3_EN
		//扬声器AS Alt3
		UAC2_AS_INTERFACE_DESC(0x01, 0x03, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_SPEAKER_IT_ID, PACKET_CHANNELS_NUM),  								// 扬声器AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(SPEAKER_ALT3_BITS),                   									// 扬声器Alt3位宽描述符
		UAC2_AS_HS_ENDPOINT_DESC(DEVICE_ISO_OUT_EP, USBD_AUDIO_FREQ, PACKET_CHANNELS_NUM, SPEAKER_ALT3_BITS), 	// 扬声器Alt3端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 扬声器Alt3端点通用描述符
	#endif
		//3 hid 播放控制
		HID_INTERFACE_DESC(2,0,1),
		HID_DESC(sizeof(AudioCtrlReportDescriptor)),
		USB_ENDPOINT_GENERAL_DESC(DEVICE_INT_IN_EP,3,3,4),
	#if HID_DATA_FUN_EN
		#ifdef HID_DATA_EP_EN
		HID_INTERFACE_DESC(3,0,2),
		HID_DESC(sizeof(HidDataReportDescriptor)),
		USB_ENDPOINT_GENERAL_DESC(DEVICE_INT_IN_EP2,3,EP_MAX_SZE(DEVICE_HS_INT_IN_MPS),4),
		USB_ENDPOINT_GENERAL_DESC(DEVICE_INT_OUT_EP2,3,EP_MAX_SZE(DEVICE_HS_INT_OUT_MPS),4),
		#else
		HID_INTERFACE_DESC(3,0,0),
		HID_DESC(sizeof(HidDataReportDescriptor)),
		#endif
	#endif
	};
	const uint8_t InterFaceNum_Tab(AUDIO_ONLY)[] = {0xFF,0x00,0x01,0xFF,0x02,0x03};
#endif//#if defined(CFG_DESCRIP_AUDIO_MIC_EN) && (USB_AUDIO_PROTOCOL == AUDIO_UAC_20)

#if defined(CFG_DESCRIP_AUDIO_ONLY_EN)
	//声卡 MIC输入和AUDIO输出
	const uint8_t ConfigDescriptor_Tab(AUDIO_ONLY_10)[] =
	{
	#if HID_DATA_FUN_EN
		//一共4个接口
		0x09,0x02,W_TOTAL_LENGTH(0x7A+USER_HS_CONFIG_DESCRIPTOR_UAC_10_SIZE+18),0x04,0x01,0x00,0x80,0x32,
	#else
		0x09,0x02,W_TOTAL_LENGTH(0x7A+USER_HS_CONFIG_DESCRIPTOR_UAC_10_SIZE),0x03,0x01,0x00,0x80,0x32,
	#endif
		//0 audio control Interface
		0x09,0x04,0x00,0x00,0x00,0x01,0x01,0x00,0x00,
		0x09,0x24,0x01,0x00,0x01,W_TOTAL_LENGTH(0x26+PACKET_CHANNELS_NUM),0x01,0x01,
		0x0C,0x24,0x02,0x01,0x01,0x01,0x00,PACKET_CHANNELS_NUM,CHANNEL_CONFIG(PACKET_CHANNELS_NUM),0x00,0x00,0x00,
		UAC1_AC_FU_DESC(0x02,0x01,PACKET_CHANNELS_NUM),
		0x09,0x24,0x03,0x03,DEVICE_TYPE(SPEAKER_TYPE),0x00,0x02,0x00,
		//1 audio stream Interface, speeaker
		0x09,0x04,0x01,0x00,0x00,0x01,0x02,0x00,0x00,

		0x09,0x04,0x01,0x01,0x01,0x01,0x02,0x00,0x00,
		0x07,0x24,0x01,0x01,0x01,0x01,0x00,
		SPEAKER_FREQ_DESCRIPTOR_UAC_10_SIZE,0x24,0x02,0x01,PACKET_CHANNELS_NUM,SPEAKER_ALT1_BITS,SPEAKER_ALT1_BITS*8,USBD_AUDIO10_SPEAKER_FREQ_NUM,
		SAMPLE_FREQ_ARRAY(USBD_AUDIO10_FREQ,USBD_AUDIO10_SPEAKER_FREQ_NUM),
		0x09, 0x05, DEVICE_ISO_OUT_EP, 0x09, ISO_EP_MAX_SZE(USBD_AUDIO10_FREQ,PACKET_CHANNELS_NUM,SPEAKER_ALT1_BITS,HS_UAC_10_FRAM_PRT_SECOND), HS_UAC_10_ISO_INTERVAL, 0x00, 0x00,
		0x07,0x25,0x01,0x01,0x00,0x00,0x00,
	#ifdef SPEAKER_ALT2_EN
		0x09,0x04,0x01,0x02,0x01,0x01,0x02,0x00,0x00,
		0x07,0x24,0x01,0x01,0x01,0x01,0x00,
		SPEAKER_FREQ_DESCRIPTOR_UAC_10_SIZE,0x24,0x02,0x01,PACKET_CHANNELS_NUM,SPEAKER_ALT2_BITS,SPEAKER_ALT2_BITS*8,USBD_AUDIO10_SPEAKER_FREQ_NUM,
		SAMPLE_FREQ_ARRAY(USBD_AUDIO10_FREQ,USBD_AUDIO10_SPEAKER_FREQ_NUM),
		0x09, 0x05, DEVICE_ISO_OUT_EP, 0x09, ISO_EP_MAX_SZE(USBD_AUDIO10_FREQ,PACKET_CHANNELS_NUM,SPEAKER_ALT2_BITS,HS_UAC_10_FRAM_PRT_SECOND), HS_UAC_10_ISO_INTERVAL, 0x00, 0x00,
		0x07,0x25,0x01,0x01,0x00,0x00,0x00,
	#endif
		//3 hid 播放控制
		0x09,0x04,0x02,0x00,0x01,0x03,0x00,0x00,0x00,
		0x09,0x21,0x01,0x02,0x00,0x01,0x22,sizeof(AudioCtrlReportDescriptor),0x00,
		0x07,0x05,DEVICE_INT_IN_EP,0x03,DEVICE_FS_INT_IN_MPS,0x00,0x04,
	#if HID_DATA_FUN_EN
		0x09,0x04,0x03,0x00,0x00,0x03,0x00,0x00,0x00,
		0x09,0x21,0x01,0x02,0x00,0x01,0x22,sizeof(HidDataReportDescriptor),0x00,
	#endif
	};
	const uint8_t InterFaceNum_Tab(AUDIO_ONLY_10)[] = {0xFF,0x00,0x01,0xFF,0x02,0x03};
#endif//#if defined(CFG_DESCRIP_AUDIO_MIC_EN) && (USB_AUDIO_PROTOCOL == AUDIO_UAC_20)

#if defined(CFG_DESCRIP_AUDIO_ONLY_EN)
	//声卡 MIC输入和AUDIO输出
	const uint8_t ConfigDescriptor_Tab(FS_AUDIO_ONLY)[] =
	{
	#if HID_DATA_FUN_EN
		#ifdef HID_DATA_EP_EN
		USB_CFG_DESC_HEADER(0xA2+USER_FS_CONFIG_DESCRIPTOR_UAC_20_SIZE+32,4),
		#else
		USB_CFG_DESC_HEADER(0xA2+USER_FS_CONFIG_DESCRIPTOR_UAC_20_SIZE+18,4),
		#endif
	#else
		USB_CFG_DESC_HEADER(0xA2+USER_FS_CONFIG_DESCRIPTOR_UAC_20_SIZE,4),
	#endif

		UAC2_AC_TERMINAL_ASSOCIATION_DESC(0,2),

		UAC2_AC_STD_INTF_DESC(0x00,0x00),

		UAC2_AC_HEADER_DESC(AUDIO_HEADSET,(0x34+(PACKET_CHANNELS_NUM*4))),
		UAC2_AC_CLK_SOURCE_DESC(AUDIO_SPEAKER_CLK_ID),      															// 扬声器时钟源

		UAC2_AC_IT_DESC(AUDIO_SPEAKER_IT_ID, AUDIO_SPEAKER_CLK_ID, PACKET_CHANNELS_NUM, 0x0101), 				// 扬声器输入终端
		UAC2_AC_FU_DESC(AUDIO_SPEAKER_FU_ID, AUDIO_SPEAKER_IT_ID, PACKET_CHANNELS_NUM),             			// 扬声器功能单元
		UAC2_AC_OT_DESC(AUDIO_SPEAKER_OT_ID, AUDIO_SPEAKER_CLK_ID, AUDIO_SPEAKER_FU_ID, SPEAKER_TYPE),    			// 扬声器输出终端

		//扬声器AS Alt0（无流传输）
		UAC2_AS_INTERFACE_DESC(0x01, 0x00, 0x00),
		//扬声器AS Alt1
		UAC2_AS_INTERFACE_DESC(0x01, 0x01, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_SPEAKER_IT_ID, PACKET_CHANNELS_NUM),  								// 扬声器AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(SPEAKER_ALT1_BITS),                   									// 扬声器Alt1位宽描述符
		UAC2_AS_FS_ENDPOINT_DESC(DEVICE_ISO_OUT_EP, USBD_AUDIO10_FREQ, PACKET_CHANNELS_NUM, SPEAKER_ALT1_BITS), // 扬声器Alt1端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 扬声器Alt1端点通用描述符

	#ifdef SPEAKER_ALT2_EN
		//扬声器AS Alt2
		UAC2_AS_INTERFACE_DESC(0x01, 0x02, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_SPEAKER_IT_ID, PACKET_CHANNELS_NUM),  								// 扬声器AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(SPEAKER_ALT2_BITS),                   									// 扬声器Alt1位宽描述符
		UAC2_AS_FS_ENDPOINT_DESC(DEVICE_ISO_OUT_EP, USBD_AUDIO10_FREQ, PACKET_CHANNELS_NUM, SPEAKER_ALT2_BITS), // 扬声器Alt1端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 扬声器Alt1端点通用描述符
	#endif
		//3 hid 播放控制
		HID_INTERFACE_DESC(2,0,1),
		HID_DESC(sizeof(AudioCtrlReportDescriptor)),
		USB_ENDPOINT_GENERAL_DESC(DEVICE_INT_IN_EP,3,3,1),
	#if HID_DATA_FUN_EN
		#ifdef HID_DATA_EP_EN
		HID_INTERFACE_DESC(3,0,2),
		HID_DESC(sizeof(HidDataReportDescriptor)),
		USB_ENDPOINT_GENERAL_DESC(DEVICE_INT_IN_EP2,3,EP_MAX_SZE(DEVICE_FS_INT_IN_MPS),4),
		USB_ENDPOINT_GENERAL_DESC(DEVICE_INT_OUT_EP2,3,EP_MAX_SZE(DEVICE_FS_INT_OUT_MPS),4),
		#else
		HID_INTERFACE_DESC(3,0,0),
		HID_DESC(sizeof(HidDataReportDescriptor)),
		#endif
	#endif
	};
	const uint8_t InterFaceNum_Tab(FS_AUDIO_ONLY)[] = {0xFF,0x00,0x01,0xFF,0x02,0x03};
#endif

#if defined(CFG_DESCRIP_AUDIO_ONLY_EN)
	//声卡 MIC输入和AUDIO输出
	const uint8_t ConfigDescriptor_Tab(FS_AUDIO_ONLY_10)[] =
	{
	#if HID_DATA_FUN_EN
		//一共4个接口
		0x09,0x02,W_TOTAL_LENGTH(0x7A+USER_FS_CONFIG_DESCRIPTOR_UAC_10_SIZE+18),0x04,0x01,0x00,0x80,0x32,
	#else
		0x09,0x02,W_TOTAL_LENGTH(0x7A+USER_FS_CONFIG_DESCRIPTOR_UAC_10_SIZE),0x03,0x01,0x00,0x80,0x32,
	#endif
		//0 audio control Interface
		0x09,0x04,0x00,0x00,0x00,0x01,0x01,0x00,0x00,
		0x09,0x24,0x01,0x00,0x01,W_TOTAL_LENGTH(0x26+PACKET_CHANNELS_NUM),0x01,0x01,
		0x0C,0x24,0x02,0x01,0x01,0x01,0x00,PACKET_CHANNELS_NUM,CHANNEL_CONFIG(PACKET_CHANNELS_NUM),0x00,0x00,0x00,
		UAC1_AC_FU_DESC(0x02,0x01,PACKET_CHANNELS_NUM),
		0x09,0x24,0x03,0x03,DEVICE_TYPE(SPEAKER_TYPE),0x00,0x02,0x00,
		//1 audio stream Interface, speeaker
		0x09,0x04,0x01,0x00,0x00,0x01,0x02,0x00,0x00,

		0x09,0x04,0x01,0x01,0x01,0x01,0x02,0x00,0x00,
		0x07,0x24,0x01,0x01,0x01,0x01,0x00,
		SPEAKER_FREQ_DESCRIPTOR_UAC_10_SIZE,0x24,0x02,0x01,PACKET_CHANNELS_NUM,SPEAKER_ALT1_BITS,SPEAKER_ALT1_BITS*8,USBD_AUDIO10_SPEAKER_FREQ_NUM,
		SAMPLE_FREQ_ARRAY(USBD_AUDIO10_FREQ,USBD_AUDIO10_SPEAKER_FREQ_NUM),
		0x09, 0x05, DEVICE_ISO_OUT_EP, 0x09, AUDIO_EP_MAX_SZE(USBD_AUDIO10_FREQ,PACKET_CHANNELS_NUM,SPEAKER_ALT1_BITS), FS_ISO_INTERVAL, 0x00, 0x00,
		0x07,0x25,0x01,0x01,0x00,0x00,0x00,
	#ifdef SPEAKER_ALT2_EN
		0x09,0x04,0x01,0x02,0x01,0x01,0x02,0x00,0x00,
		0x07,0x24,0x01,0x01,0x01,0x01,0x00,
		SPEAKER_FREQ_DESCRIPTOR_UAC_10_SIZE,0x24,0x02,0x01,PACKET_CHANNELS_NUM,SPEAKER_ALT2_BITS,SPEAKER_ALT2_BITS*8,USBD_AUDIO10_SPEAKER_FREQ_NUM,
		SAMPLE_FREQ_ARRAY(USBD_AUDIO10_FREQ,USBD_AUDIO10_SPEAKER_FREQ_NUM),
		0x09, 0x05, DEVICE_ISO_OUT_EP, 0x09, AUDIO_EP_MAX_SZE(USBD_AUDIO10_FREQ,PACKET_CHANNELS_NUM,SPEAKER_ALT2_BITS), FS_ISO_INTERVAL, 0x00, 0x00,
		0x07,0x25,0x01,0x01,0x00,0x00,0x00,
	#endif
		//3 hid 播放控制
		0x09,0x04,0x02,0x00,0x01,0x03,0x00,0x00,0x00,
		0x09,0x21,0x01,0x02,0x00,0x01,0x22,sizeof(AudioCtrlReportDescriptor),0x00,
		0x07,0x05,DEVICE_INT_IN_EP,0x03,DEVICE_FS_INT_IN_MPS,0x00,0x01,
	#if HID_DATA_FUN_EN
		0x09,0x04,0x03,0x00,0x00,0x03,0x00,0x00,0x00,
		0x09,0x21,0x01,0x02,0x00,0x01,0x22,sizeof(HidDataReportDescriptor),0x00,
	#endif
	};
	const uint8_t InterFaceNum_Tab(FS_AUDIO_ONLY_10)[] = {0xFF,0x00,0x01,0xFF,0x02,0x03};
#endif

#if defined(CFG_DESCRIP_MIC_ONLY_EN)
	//声卡 MIC输入和AUDIO输出
	const uint8_t ConfigDescriptor_Tab(MIC_ONLY)[] =
	{
	#if HID_DATA_FUN_EN
		#ifdef HID_DATA_EP_EN
		USB_CFG_DESC_HEADER(0xA2 + USER_HS_CONFIG_DESCRIPTOR_UAC_20_SIZE +32,4),
		#else
		USB_CFG_DESC_HEADER(0xA2 + USER_HS_CONFIG_DESCRIPTOR_UAC_20_SIZE +18,4),
		#endif
	#else
		USB_CFG_DESC_HEADER(0xA2 + USER_HS_CONFIG_DESCRIPTOR_UAC_20_SIZE ,3),
	#endif

		UAC2_AC_TERMINAL_ASSOCIATION_DESC(0,2),

		UAC2_AC_STD_INTF_DESC(0x00,0x00),

		UAC2_AC_HEADER_DESC(AUDIO_MICROPHONE,(0x34+(MIC_CHANNELS_NUM*4))),
		UAC2_AC_CLK_SOURCE_DESC(AUDIO_MIC_CLK_ID),  															// 麦克风时钟源
		UAC2_AC_IT_DESC(AUDIO_MIC_IT_ID, AUDIO_MIC_CLK_ID, MIC_CHANNELS_NUM, MIC_TYPE), 							// 麦克风输入终端
		UAC2_AC_FU_DESC(AUDIO_MIC_FU_ID, AUDIO_MIC_IT_ID, MIC_CHANNELS_NUM),                        			// 麦克风功能单元
		UAC2_AC_OT_DESC(AUDIO_MIC_OT_ID, AUDIO_MIC_CLK_ID, AUDIO_MIC_FU_ID, 0x0101), 							// 麦克风输出终端

		//麦克风AS Alt0（无流传输）
		UAC2_AS_INTERFACE_DESC(0x01, 0x00, 0x00),
		//麦克风AS Alt1
		UAC2_AS_INTERFACE_DESC(0x01, 0x01, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_MIC_OT_ID, MIC_CHANNELS_NUM),     									// 麦克风AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(MIC_ALT1_BITS),                      									// 麦克风Alt1位宽描述符
		UAC2_AS_HS_ENDPOINT_DESC(DEVICE_ISO_IN_EP, USBD_AUDIO_MIC_FREQ, MIC_CHANNELS_NUM, MIC_ALT1_BITS), 		// 麦克风Alt1端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 麦克风Alt1端点通用描述符

	#ifdef MIC_ALT2_EN
		//麦克风AS Alt2
		UAC2_AS_INTERFACE_DESC(0x01, 0x02, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_MIC_OT_ID, MIC_CHANNELS_NUM),     									// 麦克风AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(MIC_ALT2_BITS),                      									// 麦克风Alt2位宽描述符
		UAC2_AS_HS_ENDPOINT_DESC(DEVICE_ISO_IN_EP, USBD_AUDIO_MIC_FREQ, MIC_CHANNELS_NUM, MIC_ALT2_BITS), 		// 麦克风Alt2端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 麦克风Alt2端点通用描述符
	#endif
	#ifdef MIC_ALT3_EN
		//麦克风AS Alt3
		UAC2_AS_INTERFACE_DESC(0x01, 0x03, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_MIC_OT_ID, MIC_CHANNELS_NUM),     									// 麦克风AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(MIC_ALT3_BITS),                      									// 麦克风Alt3位宽描述符
		UAC2_AS_HS_ENDPOINT_DESC(DEVICE_ISO_IN_EP, USBD_AUDIO_MIC_FREQ, MIC_CHANNELS_NUM, MIC_ALT3_BITS), 		// 麦克风Alt3端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 麦克风Alt3端点通用描述符
	#endif
		//3 hid 播放控制
		HID_INTERFACE_DESC(2,0,1),
		HID_DESC(sizeof(AudioCtrlReportDescriptor)),
		USB_ENDPOINT_GENERAL_DESC(DEVICE_INT_IN_EP,3,3,4),
	#if HID_DATA_FUN_EN
		#ifdef HID_DATA_EP_EN
		HID_INTERFACE_DESC(3,0,2),
		HID_DESC(sizeof(HidDataReportDescriptor)),
		USB_ENDPOINT_GENERAL_DESC(DEVICE_INT_IN_EP2,3,EP_MAX_SZE(DEVICE_HS_INT_IN_MPS),4),
		USB_ENDPOINT_GENERAL_DESC(DEVICE_INT_OUT_EP2,3,EP_MAX_SZE(DEVICE_HS_INT_OUT_MPS),4),
		#else
		HID_INTERFACE_DESC(3,0,0),
		HID_DESC(sizeof(HidDataReportDescriptor)),
		#endif
	#endif
	};
	const uint8_t InterFaceNum_Tab(MIC_ONLY)[] = {0xFF,0x00,0xFF,0x01,0x02,0x03};
#endif//#if defined(CFG_DESCRIP_MIC_ONLY_EN) && (USB_AUDIO_PROTOCOL == AUDIO_UAC_20)
#if defined(CFG_DESCRIP_MIC_ONLY_EN)
	//声卡 MIC输入和AUDIO输出
	const uint8_t ConfigDescriptor_Tab(FS_MIC_ONLY)[] =
	{
	#if HID_DATA_FUN_EN
		#ifdef HID_DATA_EP_EN
		USB_CFG_DESC_HEADER(0xA2+USER_FS_CONFIG_DESCRIPTOR_UAC_20_SIZE+32,4),
		#else
		USB_CFG_DESC_HEADER(0xA2+USER_FS_CONFIG_DESCRIPTOR_UAC_20_SIZE+18,4),
		#endif
	#else
		USB_CFG_DESC_HEADER(0xA2+USER_FS_CONFIG_DESCRIPTOR_UAC_20_SIZE,3),
	#endif

		UAC2_AC_TERMINAL_ASSOCIATION_DESC(0,2),

		UAC2_AC_STD_INTF_DESC(0x00,0x00),

		UAC2_AC_HEADER_DESC(AUDIO_MICROPHONE,(0x34+(MIC_CHANNELS_NUM*4))),
		UAC2_AC_CLK_SOURCE_DESC(AUDIO_MIC_CLK_ID),  															// 麦克风时钟源
		UAC2_AC_IT_DESC(AUDIO_MIC_IT_ID, AUDIO_MIC_CLK_ID, MIC_CHANNELS_NUM, MIC_TYPE), 							// 麦克风输入终端
		UAC2_AC_FU_DESC(AUDIO_MIC_FU_ID, AUDIO_MIC_IT_ID, MIC_CHANNELS_NUM),                        			// 麦克风功能单元
		UAC2_AC_OT_DESC(AUDIO_MIC_OT_ID, AUDIO_MIC_CLK_ID, AUDIO_MIC_FU_ID, 0x0101), 							// 麦克风输出终端

		//麦克风AS Alt0（无流传输）
		UAC2_AS_INTERFACE_DESC(0x01, 0x00, 0x00),
		//麦克风AS Alt1
		UAC2_AS_INTERFACE_DESC(0x01, 0x01, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_MIC_OT_ID, MIC_CHANNELS_NUM),     									// 麦克风AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(MIC_ALT1_BITS),                      									// 麦克风Alt1位宽描述符
		UAC2_AS_FS_ENDPOINT_DESC(DEVICE_ISO_IN_EP, USBD_AUDIO10_MIC_FREQ, MIC_CHANNELS_NUM, MIC_ALT1_BITS), 		// 麦克风Alt1端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 麦克风Alt1端点通用描述符

	#ifdef MIC_ALT2_EN
		//麦克风AS Alt2
		UAC2_AS_INTERFACE_DESC(0x01, 0x02, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_MIC_OT_ID, MIC_CHANNELS_NUM),     									// 麦克风AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(MIC_ALT2_BITS),                      									// 麦克风Alt2位宽描述符
		UAC2_AS_FS_ENDPOINT_DESC(DEVICE_ISO_IN_EP, USBD_AUDIO10_MIC_FREQ, MIC_CHANNELS_NUM, MIC_ALT2_BITS), 		// 麦克风Alt2端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 麦克风Alt2端点通用描述符
	#endif
		//3 hid 播放控制
		HID_INTERFACE_DESC(2,0,1),
		HID_DESC(sizeof(AudioCtrlReportDescriptor)),
		USB_ENDPOINT_GENERAL_DESC(DEVICE_INT_IN_EP,3,3,1),
	#if HID_DATA_FUN_EN
		#ifdef HID_DATA_EP_EN
		HID_INTERFACE_DESC(3,0,2),
		HID_DESC(sizeof(HidDataReportDescriptor)),
		USB_ENDPOINT_GENERAL_DESC(DEVICE_INT_IN_EP2,3,EP_MAX_SZE(DEVICE_FS_INT_IN_MPS),1),
		USB_ENDPOINT_GENERAL_DESC(DEVICE_INT_OUT_EP2,3,EP_MAX_SZE(DEVICE_FS_INT_OUT_MPS),1),
		#else
		HID_INTERFACE_DESC(3,0,0),
		HID_DESC(sizeof(HidDataReportDescriptor)),
		#endif
	#endif
	};
	const uint8_t InterFaceNum_Tab(FS_MIC_ONLY)[] = {0xFF,0x00,0xFF,0x01,0x02,0x03};
#endif//#if defined(CFG_DESCRIP_MIC_ONLY_EN) && (USB_AUDIO_PROTOCOL == AUDIO_UAC_20)
#if defined(CFG_DESCRIP_MIC_ONLY_EN)
	//声卡 MIC输入和AUDIO输出
	const uint8_t ConfigDescriptor_Tab(MIC_ONLY_10)[] =
	{
	#if HID_DATA_FUN_EN
		//一共4个接口
		0x09,0x02,W_TOTAL_LENGTH(0x7A+USER_HS_CONFIG_DESCRIPTOR_UAC_10_SIZE+18),0x04,0x01,0x00,0x80,0x32,
	#else
		0x09,0x02,W_TOTAL_LENGTH(0x7A+USER_HS_CONFIG_DESCRIPTOR_UAC_10_SIZE),0x03,0x01,0x00,0x80,0x32,
	#endif
		//0 audio control Interface
		0x09,0x04,0x00,0x00,0x00,0x01,0x01,0x00,0x00,
		0x09,0x24,0x01,0x00,0x01,W_TOTAL_LENGTH(0x26+MIC_CHANNELS_NUM),0x01,0x01,

		0x0C,0x24,0x02,0x04,DEVICE_TYPE(MIC_TYPE),0x00,MIC_CHANNELS_NUM,CHANNEL_CONFIG(MIC_CHANNELS_NUM),0x00,0x00,0x00,
		UAC1_AC_FU_DESC(0x05,0x04,MIC_CHANNELS_NUM),
		0x09,0x24,0x03,0x06,0x01,0x01,0x00,0x05,0x00,

		//1 audio stream Interface, microphone
		0x09,0x04,0x01,0x00,0x00,0x01,0x02,0x00,0x00,

		0x09,0x04,0x01,0x01,0x01,0x01,0x02,0x00,0x00,
		0x07,0x24,0x01,0x06,0x01,0x01,0x00,
		MIC_FREQ_DESCRIPTOR_UAC_10_SIZE,0x24,0x02,0x01,MIC_CHANNELS_NUM,MIC_ALT1_BITS,MIC_ALT1_BITS*8,USBD_AUDIO10_MIC_FREQ_NUM,
		SAMPLE_FREQ_ARRAY(USBD_AUDIO10_MIC_FREQ,USBD_AUDIO10_MIC_FREQ_NUM),
		0x09, 0x05, DEVICE_ISO_IN_EP, 0x09, ISO_EP_MAX_SZE(USBD_AUDIO10_MIC_FREQ,MIC_CHANNELS_NUM,MIC_ALT1_BITS,HS_UAC_10_FRAM_PRT_SECOND), HS_UAC_10_ISO_INTERVAL, 0x00, 0x00,
		0x07,0x25,0x01,0x01,0x00,0x00,0x00,
	#ifdef MIC_ALT2_EN
		0x09,0x04,0x01,0x02,0x01,0x01,0x02,0x00,0x00,
		0x07,0x24,0x01,0x06,0x01,0x01,0x00,
		MIC_FREQ_DESCRIPTOR_UAC_10_SIZE,0x24,0x02,0x01,MIC_CHANNELS_NUM,MIC_ALT2_BITS,MIC_ALT2_BITS*8,USBD_AUDIO10_MIC_FREQ_NUM,
		SAMPLE_FREQ_ARRAY(USBD_AUDIO10_MIC_FREQ,USBD_AUDIO10_MIC_FREQ_NUM),
		0x09, 0x05, DEVICE_ISO_IN_EP, 0x09, ISO_EP_MAX_SZE(USBD_AUDIO10_MIC_FREQ,MIC_CHANNELS_NUM,MIC_ALT2_BITS,HS_UAC_10_FRAM_PRT_SECOND), HS_UAC_10_ISO_INTERVAL, 0x00, 0x00,
		0x07,0x25,0x01,0x01,0x00,0x00,0x00,
	#endif
		//3 hid 播放控制
		0x09,0x04,0x02,0x00,0x01,0x03,0x00,0x00,0x00,
		0x09,0x21,0x01,0x02,0x00,0x01,0x22,sizeof(AudioCtrlReportDescriptor),0x00,
		0x07,0x05,DEVICE_INT_IN_EP,0x03,DEVICE_FS_INT_IN_MPS,0x00,0x04,
	#if HID_DATA_FUN_EN
		0x09,0x04,0x03,0x00,0x00,0x03,0x00,0x00,0x00,
		0x09,0x21,0x01,0x02,0x00,0x01,0x22,sizeof(HidDataReportDescriptor),0x00,
	#endif
	};
	const uint8_t InterFaceNum_Tab(MIC_ONLY_10)[] = {0xFF,0x00,0xFF,0x01,0x02,0x03};
#endif
#if defined(CFG_DESCRIP_MIC_ONLY_EN)
	//声卡 MIC输入和AUDIO输出
	const uint8_t ConfigDescriptor_Tab(FS_MIC_ONLY_10)[] =
	{
	#if HID_DATA_FUN_EN
		//一共4个接口
		0x09,0x02,W_TOTAL_LENGTH(0x7A+USER_FS_CONFIG_DESCRIPTOR_UAC_10_SIZE+18),0x04,0x01,0x00,0x80,0x32,
	#else
		0x09,0x02,W_TOTAL_LENGTH(0x7A+USER_FS_CONFIG_DESCRIPTOR_UAC_10_SIZE),0x03,0x01,0x00,0x80,0x32,
	#endif
		//0 audio control Interface
		0x09,0x04,0x00,0x00,0x00,0x01,0x01,0x00,0x00,
		0x09,0x24,0x01,0x00,0x01,W_TOTAL_LENGTH(0x26+MIC_CHANNELS_NUM),0x01,0x01,

		0x0C,0x24,0x02,0x04,DEVICE_TYPE(MIC_TYPE),0x00,MIC_CHANNELS_NUM,CHANNEL_CONFIG(MIC_CHANNELS_NUM),0x00,0x00,0x00,
		UAC1_AC_FU_DESC(0x05,0x04,MIC_CHANNELS_NUM),
		0x09,0x24,0x03,0x06,0x01,0x01,0x00,0x05,0x00,

		//2 audio stream Interface, microphone
		0x09,0x04,0x01,0x00,0x00,0x01,0x02,0x00,0x00,

		0x09,0x04,0x01,0x01,0x01,0x01,0x02,0x00,0x00,
		0x07,0x24,0x01,0x06,0x01,0x01,0x00,
		MIC_FREQ_DESCRIPTOR_UAC_10_SIZE,0x24,0x02,0x01,MIC_CHANNELS_NUM,MIC_ALT1_BITS,MIC_ALT1_BITS*8,USBD_AUDIO10_MIC_FREQ_NUM,
		SAMPLE_FREQ_ARRAY(USBD_AUDIO10_MIC_FREQ,USBD_AUDIO10_MIC_FREQ_NUM),
		0x09, 0x05, DEVICE_ISO_IN_EP, 0x09, AUDIO_EP_MAX_SZE(USBD_AUDIO10_MIC_FREQ,MIC_CHANNELS_NUM,MIC_ALT1_BITS), FS_ISO_INTERVAL, 0x00, 0x00,
		0x07,0x25,0x01,0x01,0x00,0x00,0x00,
	#ifdef MIC_ALT2_EN
		0x09,0x04,0x01,0x02,0x01,0x01,0x02,0x00,0x00,
		0x07,0x24,0x01,0x06,0x01,0x01,0x00,
		MIC_FREQ_DESCRIPTOR_UAC_10_SIZE,0x24,0x02,0x01,MIC_CHANNELS_NUM,MIC_ALT2_BITS,MIC_ALT2_BITS*8,USBD_AUDIO10_MIC_FREQ_NUM,
		SAMPLE_FREQ_ARRAY(USBD_AUDIO10_MIC_FREQ,USBD_AUDIO10_MIC_FREQ_NUM),
		0x09, 0x05, DEVICE_ISO_IN_EP, 0x09, AUDIO_EP_MAX_SZE(USBD_AUDIO10_MIC_FREQ,MIC_CHANNELS_NUM,MIC_ALT2_BITS), FS_ISO_INTERVAL, 0x00, 0x00,
		0x07,0x25,0x01,0x01,0x00,0x00,0x00,
	#endif
		//3 hid 播放控制
		0x09,0x04,0x02,0x00,0x01,0x03,0x00,0x00,0x00,
		0x09,0x21,0x01,0x02,0x00,0x01,0x22,sizeof(AudioCtrlReportDescriptor),0x00,
		0x07,0x05,DEVICE_INT_IN_EP,0x03,DEVICE_FS_INT_IN_MPS,0x00,0x01,
	#if HID_DATA_FUN_EN
		0x09,0x04,0x03,0x00,0x00,0x03,0x00,0x00,0x00,
		0x09,0x21,0x01,0x02,0x00,0x01,0x22,sizeof(HidDataReportDescriptor),0x00,
	#endif
	};
	const uint8_t InterFaceNum_Tab(FS_MIC_ONLY_10)[] = {0xFF,0x00,0xFF,0x01,0x02,0x03};
#endif

#if defined(CFG_DESCRIP_AUDIO_MIC_EN)
	const uint8_t ConfigDescriptor_Tab(AUDIO_MIC)[] =
	{
	#if HID_DATA_FUN_EN
		#ifdef HID_DATA_EP_EN
		USB_CFG_DESC_HEADER(0x108+USER_HS_CONFIG_DESCRIPTOR_UAC_20_SIZE+32,5),
		#else
		USB_CFG_DESC_HEADER(0x108+USER_HS_CONFIG_DESCRIPTOR_UAC_20_SIZE+18,5),
		#endif
	#else
		USB_CFG_DESC_HEADER(0x108+USER_HS_CONFIG_DESCRIPTOR_UAC_20_SIZE,4),
	#endif

		UAC2_AC_TERMINAL_ASSOCIATION_DESC(0,3),

		UAC2_AC_STD_INTF_DESC(0x00,0x00),

		UAC2_AC_HEADER_DESC(AUDIO_HEADSET,(103+(PACKET_CHANNELS_NUM*4)+(MIC_CHANNELS_NUM*4))),
		UAC2_AC_CLK_SOURCE_DESC(AUDIO_SPEAKER_CLK_ID),      															// 扬声器时钟源
		UAC2_AC_CLK_SOURCE_DESC(AUDIO_MIC_CLK_ID),  															// 麦克风时钟源

		UAC2_AC_IT_DESC(AUDIO_SPEAKER_IT_ID, AUDIO_SPEAKER_CLK_ID, PACKET_CHANNELS_NUM, 0x0101), 				// 扬声器输入终端
		UAC2_AC_FU_DESC(AUDIO_SPEAKER_FU_ID, AUDIO_SPEAKER_IT_ID, PACKET_CHANNELS_NUM),             			// 扬声器功能单元
		UAC2_AC_OT_DESC(AUDIO_SPEAKER_OT_ID, AUDIO_SPEAKER_CLK_ID, AUDIO_SPEAKER_FU_ID, SPEAKER_TYPE),    		// 扬声器输出终端

		UAC2_AC_IT_DESC(AUDIO_MIC_IT_ID, AUDIO_MIC_CLK_ID, MIC_CHANNELS_NUM, MIC_TYPE), 						// 麦克风输入终端
		UAC2_AC_FU_DESC(AUDIO_MIC_FU_ID, AUDIO_MIC_IT_ID, MIC_CHANNELS_NUM),                        			// 麦克风功能单元
		UAC2_AC_OT_DESC(AUDIO_MIC_OT_ID, AUDIO_MIC_CLK_ID, AUDIO_MIC_FU_ID, 0x0101), 							// 麦克风输出终端

		//扬声器AS Alt0（无流传输）
		UAC2_AS_INTERFACE_DESC(0x01, 0x00, 0x00),
		//扬声器AS Alt1
		UAC2_AS_INTERFACE_DESC(0x01, 0x01, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_SPEAKER_IT_ID, PACKET_CHANNELS_NUM),  								// 扬声器AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(SPEAKER_ALT1_BITS),                   									// 扬声器Alt1位宽描述符
		UAC2_AS_HS_ENDPOINT_DESC(DEVICE_ISO_OUT_EP, USBD_AUDIO_FREQ, PACKET_CHANNELS_NUM, SPEAKER_ALT1_BITS), 	// 扬声器Alt1端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 扬声器Alt1端点通用描述符

	#ifdef SPEAKER_ALT2_EN
		//扬声器AS Alt2
		UAC2_AS_INTERFACE_DESC(0x01, 0x02, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_SPEAKER_IT_ID, PACKET_CHANNELS_NUM),  								// 扬声器AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(SPEAKER_ALT2_BITS),                   									// 扬声器Alt2位宽描述符
		UAC2_AS_HS_ENDPOINT_DESC(DEVICE_ISO_OUT_EP, USBD_AUDIO_FREQ, PACKET_CHANNELS_NUM, SPEAKER_ALT2_BITS), 	// 扬声器Alt2端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 扬声器Alt2端点通用描述符
	#endif
	#ifdef SPEAKER_ALT3_EN
		//扬声器AS Alt2
		UAC2_AS_INTERFACE_DESC(0x01, 0x03, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_SPEAKER_IT_ID, PACKET_CHANNELS_NUM),  								// 扬声器AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(SPEAKER_ALT3_BITS),                   									// 扬声器Alt2位宽描述符
		UAC2_AS_HS_ENDPOINT_DESC(DEVICE_ISO_OUT_EP, USBD_AUDIO_FREQ, PACKET_CHANNELS_NUM, SPEAKER_ALT3_BITS), 	// 扬声器Alt2端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 扬声器Alt2端点通用描述符
	#endif
		//麦克风AS Alt0（无流传输）
		UAC2_AS_INTERFACE_DESC(0x02, 0x00, 0x00),
		//麦克风AS Alt1
		UAC2_AS_INTERFACE_DESC(0x02, 0x01, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_MIC_OT_ID, MIC_CHANNELS_NUM),     									// 麦克风AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(MIC_ALT1_BITS),                      									// 麦克风Alt1位宽描述符
		UAC2_AS_HS_ENDPOINT_DESC(DEVICE_ISO_IN_EP, USBD_AUDIO_MIC_FREQ, MIC_CHANNELS_NUM, MIC_ALT1_BITS), 		// 麦克风Alt1端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 麦克风Alt1端点通用描述符

	#ifdef MIC_ALT2_EN
		//麦克风AS Alt1
		UAC2_AS_INTERFACE_DESC(0x02, 0x02, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_MIC_OT_ID, MIC_CHANNELS_NUM),     									// 麦克风AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(MIC_ALT2_BITS),                      									// 麦克风Alt1位宽描述符
		UAC2_AS_HS_ENDPOINT_DESC(DEVICE_ISO_IN_EP, USBD_AUDIO_MIC_FREQ, MIC_CHANNELS_NUM, MIC_ALT2_BITS), 		// 麦克风Alt1端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 麦克风Alt1端点通用描述符
	#endif
	#ifdef MIC_ALT3_EN
		//麦克风AS Alt1
		UAC2_AS_INTERFACE_DESC(0x02, 0x03, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_MIC_OT_ID, MIC_CHANNELS_NUM),     									// 麦克风AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(MIC_ALT3_BITS),                      									// 麦克风Alt1位宽描述符
		UAC2_AS_HS_ENDPOINT_DESC(DEVICE_ISO_IN_EP, USBD_AUDIO_MIC_FREQ, MIC_CHANNELS_NUM, MIC_ALT3_BITS), 		// 麦克风Alt1端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 麦克风Alt1端点通用描述符
	#endif
		//3 hid 播放控制
		HID_INTERFACE_DESC(3,0,1),
		HID_DESC(sizeof(AudioCtrlReportDescriptor)),
		USB_ENDPOINT_GENERAL_DESC(DEVICE_INT_IN_EP,3,3,4),
	#if HID_DATA_FUN_EN
		#ifdef HID_DATA_EP_EN
		HID_INTERFACE_DESC(4,0,2),
		HID_DESC(sizeof(HidDataReportDescriptor)),
		USB_ENDPOINT_GENERAL_DESC(DEVICE_INT_IN_EP2,3,EP_MAX_SZE(DEVICE_HS_INT_IN_MPS),4),
		USB_ENDPOINT_GENERAL_DESC(DEVICE_INT_OUT_EP2,3,EP_MAX_SZE(DEVICE_HS_INT_OUT_MPS),4),
		#else
		HID_INTERFACE_DESC(4,0,0),
		HID_DESC(sizeof(HidDataReportDescriptor)),
		#endif
	#endif
	};
	const uint8_t InterFaceNum_Tab(AUDIO_MIC)[] = {0xFF,0x00,0x01,0x02,0x03,0x04};

	//声卡 MIC输入和AUDIO输出
	const uint8_t ConfigDescriptor_Tab(AUDIO_MIC_10)[] =
	{
	#if HID_DATA_FUN_EN
		//一共4个接口
		0x09,0x02,W_TOTAL_LENGTH(0xC1+USER_HS_CONFIG_DESCRIPTOR_UAC_10_SIZE+18),0x05,0x01,0x00,0x80,0x32,
	#else
		0x09,0x02,W_TOTAL_LENGTH(0xC1+USER_HS_CONFIG_DESCRIPTOR_UAC_10_SIZE),0x04,0x01,0x00,0x80,0x32,
	#endif
		//0 audio control Interface
		0x09,0x04,0x00,0x00,0x00,0x01,0x01,0x00,0x00,
		0x0A,0x24,0x01,0x00,0x01,W_TOTAL_LENGTH(0x44+PACKET_CHANNELS_NUM+MIC_CHANNELS_NUM),0x02,0x01,0x02,
		0x0C,0x24,0x02,0x01,0x01,0x01,0x00,PACKET_CHANNELS_NUM,CHANNEL_CONFIG(PACKET_CHANNELS_NUM),0x00,0x00,0x00,
		UAC1_AC_FU_DESC(0x02,0x01,PACKET_CHANNELS_NUM),
		0x09,0x24,0x03,0x03,DEVICE_TYPE(SPEAKER_TYPE),0x00,0x02,0x00,
		0x0C,0x24,0x02,0x04,DEVICE_TYPE(MIC_TYPE),0x00,MIC_CHANNELS_NUM,CHANNEL_CONFIG(MIC_CHANNELS_NUM),0x00,0x00,0x00,
		UAC1_AC_FU_DESC(0x05,0x04,MIC_CHANNELS_NUM),
		0x09,0x24,0x03,0x06,0x01,0x01,0x00,0x05,0x00,
		//1 audio stream Interface, speeaker
		0x09,0x04,0x01,0x00,0x00,0x01,0x02,0x00,0x00,

		0x09,0x04,0x01,0x01,0x01,0x01,0x02,0x00,0x00,
		0x07,0x24,0x01,0x01,0x01,0x01,0x00,
		SPEAKER_FREQ_DESCRIPTOR_UAC_10_SIZE,0x24,0x02,0x01,PACKET_CHANNELS_NUM,SPEAKER_ALT1_BITS,SPEAKER_ALT1_BITS*8,USBD_AUDIO10_SPEAKER_FREQ_NUM,
		SAMPLE_FREQ_ARRAY(USBD_AUDIO10_FREQ,USBD_AUDIO10_SPEAKER_FREQ_NUM),
		0x09, 0x05, DEVICE_ISO_OUT_EP, 0x09, ISO_EP_MAX_SZE(USBD_AUDIO10_FREQ,PACKET_CHANNELS_NUM,SPEAKER_ALT1_BITS,HS_UAC_10_FRAM_PRT_SECOND), HS_UAC_10_ISO_INTERVAL, 0x00, 0x00,
		0x07,0x25,0x01,0x01,0x00,0x00,0x00,
	#ifdef SPEAKER_ALT2_EN
		0x09,0x04,0x01,0x02,0x01,0x01,0x02,0x00,0x00,
		0x07,0x24,0x01,0x01,0x01,0x01,0x00,
		SPEAKER_FREQ_DESCRIPTOR_UAC_10_SIZE,0x24,0x02,0x01,PACKET_CHANNELS_NUM,SPEAKER_ALT2_BITS,SPEAKER_ALT2_BITS*8,USBD_AUDIO10_SPEAKER_FREQ_NUM,
		SAMPLE_FREQ_ARRAY(USBD_AUDIO10_FREQ,USBD_AUDIO10_SPEAKER_FREQ_NUM),
		0x09, 0x05, DEVICE_ISO_OUT_EP, 0x09, ISO_EP_MAX_SZE(USBD_AUDIO10_FREQ,PACKET_CHANNELS_NUM,SPEAKER_ALT2_BITS,HS_UAC_10_FRAM_PRT_SECOND), HS_UAC_10_ISO_INTERVAL, 0x00, 0x00,
		0x07,0x25,0x01,0x01,0x00,0x00,0x00,
	#endif
		//2 audio stream Interface, microphone
		0x09,0x04,0x02,0x00,0x00,0x01,0x02,0x00,0x00,

		0x09,0x04,0x02,0x01,0x01,0x01,0x02,0x00,0x00,
		0x07,0x24,0x01,0x06,0x01,0x01,0x00,
		MIC_FREQ_DESCRIPTOR_UAC_10_SIZE,0x24,0x02,0x01,MIC_CHANNELS_NUM,MIC_ALT1_BITS,MIC_ALT1_BITS*8,USBD_AUDIO10_MIC_FREQ_NUM,
		SAMPLE_FREQ_ARRAY(USBD_AUDIO10_MIC_FREQ,USBD_AUDIO10_MIC_FREQ_NUM),
		0x09, 0x05, DEVICE_ISO_IN_EP, 0x09, ISO_EP_MAX_SZE(USBD_AUDIO10_MIC_FREQ,MIC_CHANNELS_NUM,MIC_ALT1_BITS,HS_UAC_10_FRAM_PRT_SECOND), HS_UAC_10_ISO_INTERVAL, 0x00, 0x00,
		0x07,0x25,0x01,0x01,0x00,0x00,0x00,
	#ifdef MIC_ALT2_EN
		0x09,0x04,0x02,0x02,0x01,0x01,0x02,0x00,0x00,
		0x07,0x24,0x01,0x06,0x01,0x01,0x00,
		MIC_FREQ_DESCRIPTOR_UAC_10_SIZE,0x24,0x02,0x01,MIC_CHANNELS_NUM,MIC_ALT2_BITS,MIC_ALT2_BITS*8,USBD_AUDIO10_MIC_FREQ_NUM,
		SAMPLE_FREQ_ARRAY(USBD_AUDIO10_MIC_FREQ,USBD_AUDIO10_MIC_FREQ_NUM),
		0x09, 0x05, DEVICE_ISO_IN_EP, 0x09, ISO_EP_MAX_SZE(USBD_AUDIO10_MIC_FREQ,MIC_CHANNELS_NUM,MIC_ALT2_BITS,HS_UAC_10_FRAM_PRT_SECOND), HS_UAC_10_ISO_INTERVAL, 0x00, 0x00,
		0x07,0x25,0x01,0x01,0x00,0x00,0x00,
	#endif
		//3 hid 播放控制
		0x09,0x04,0x03,0x00,0x01,0x03,0x00,0x00,0x00,
		0x09,0x21,0x01,0x02,0x00,0x01,0x22,sizeof(AudioCtrlReportDescriptor),0x00,
		0x07,0x05,DEVICE_INT_IN_EP,0x03,DEVICE_FS_INT_IN_MPS,0x00,0x04,
	#if HID_DATA_FUN_EN
		0x09,0x04,0x04,0x00,0x00,0x03,0x00,0x00,0x00,
		0x09,0x21,0x01,0x02,0x00,0x01,0x22,sizeof(HidDataReportDescriptor),0x00,
	#endif
	};
	const uint8_t InterFaceNum_Tab(AUDIO_MIC_10)[] = {0xFF,0x00,0x01,0x02,0x03,0x04};
#endif//#if defined(CFG_DESCRIP_AUDIO_MIC_EN) && (USB_AUDIO_PROTOCOL == AUDIO_UAC_20)


#ifdef CFG_DESCRIP_AUDIO_MIC_AUDIO_EN
	const uint8_t ConfigDescriptor_Tab(AUDIO_MIC_AUDIO)[] =
    {
 	#if HID_DATA_FUN_EN
		#ifdef HID_DATA_EP_EN
		USB_CFG_DESC_HEADER(0x108+0x80+USER_HS_CONFIG_DESCRIPTOR_UAC_20_SIZE+32,7),
		#else
		USB_CFG_DESC_HEADER(0x108+0x80+USER_HS_CONFIG_DESCRIPTOR_UAC_20_SIZE+18,7),
		#endif
	#else
		USB_CFG_DESC_HEADER(0x108+0x80+USER_HS_CONFIG_DESCRIPTOR_UAC_20_SIZE,6),
	#endif

		UAC2_AC_TERMINAL_ASSOCIATION_DESC(0,3),

		UAC2_AC_STD_INTF_DESC(0x00,0x00),

		UAC2_AC_HEADER_DESC(AUDIO_HEADSET,(103+(PACKET_CHANNELS_NUM*4)+(MIC_CHANNELS_NUM*4))),
		UAC2_AC_CLK_SOURCE_DESC(AUDIO_SPEAKER_CLK_ID),      															// 扬声器时钟源
		UAC2_AC_CLK_SOURCE_DESC(AUDIO_MIC_CLK_ID),  															// 麦克风时钟源

		UAC2_AC_IT_DESC(AUDIO_SPEAKER_IT_ID, AUDIO_SPEAKER_CLK_ID, PACKET_CHANNELS_NUM, 0x0101), 				// 扬声器输入终端
		UAC2_AC_FU_DESC(AUDIO_SPEAKER_FU_ID, AUDIO_SPEAKER_IT_ID, PACKET_CHANNELS_NUM),             			// 扬声器功能单元
		UAC2_AC_OT_DESC(AUDIO_SPEAKER_OT_ID, AUDIO_SPEAKER_CLK_ID, AUDIO_SPEAKER_FU_ID, SPEAKER_TYPE),    			// 扬声器输出终端

		UAC2_AC_IT_DESC(AUDIO_MIC_IT_ID, AUDIO_MIC_CLK_ID, MIC_CHANNELS_NUM, MIC_TYPE), 							// 麦克风输入终端
		UAC2_AC_FU_DESC(AUDIO_MIC_FU_ID, AUDIO_MIC_IT_ID, MIC_CHANNELS_NUM),                        			// 麦克风功能单元
		UAC2_AC_OT_DESC(AUDIO_MIC_OT_ID, AUDIO_MIC_CLK_ID, AUDIO_MIC_FU_ID, 0x0101), 							// 麦克风输出终端

		//扬声器AS Alt0（无流传输）
		UAC2_AS_INTERFACE_DESC(0x01, 0x00, 0x00),
		//扬声器AS Alt1
		UAC2_AS_INTERFACE_DESC(0x01, 0x01, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_SPEAKER_IT_ID, PACKET_CHANNELS_NUM),  								// 扬声器AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(SPEAKER_ALT1_BITS),                   									// 扬声器Alt1位宽描述符
		UAC2_AS_HS_ENDPOINT_DESC(DEVICE_ISO_OUT_EP, USBD_AUDIO_FREQ, PACKET_CHANNELS_NUM, SPEAKER_ALT1_BITS), 	// 扬声器Alt1端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 扬声器Alt1端点通用描述符

	#ifdef SPEAKER_ALT2_EN
		//扬声器AS Alt2
		UAC2_AS_INTERFACE_DESC(0x01, 0x02, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_SPEAKER_IT_ID, PACKET_CHANNELS_NUM),  								// 扬声器AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(SPEAKER_ALT2_BITS),                   									// 扬声器Alt2位宽描述符
		UAC2_AS_HS_ENDPOINT_DESC(DEVICE_ISO_OUT_EP, USBD_AUDIO_FREQ, PACKET_CHANNELS_NUM, SPEAKER_ALT2_BITS), 	// 扬声器Alt2端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 扬声器Alt2端点通用描述符
	#endif
	#ifdef SPEAKER_ALT3_EN
		//扬声器AS Alt2
		UAC2_AS_INTERFACE_DESC(0x01, 0x03, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_SPEAKER_IT_ID, PACKET_CHANNELS_NUM),  								// 扬声器AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(SPEAKER_ALT3_BITS),                   									// 扬声器Alt2位宽描述符
		UAC2_AS_HS_ENDPOINT_DESC(DEVICE_ISO_OUT_EP, USBD_AUDIO_FREQ, PACKET_CHANNELS_NUM, SPEAKER_ALT3_BITS), 	// 扬声器Alt2端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 扬声器Alt2端点通用描述符
	#endif
		//麦克风AS Alt0（无流传输）
		UAC2_AS_INTERFACE_DESC(0x02, 0x00, 0x00),
		//麦克风AS Alt1
		UAC2_AS_INTERFACE_DESC(0x02, 0x01, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_MIC_OT_ID, MIC_CHANNELS_NUM),     									// 麦克风AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(MIC_ALT1_BITS),                      									// 麦克风Alt1位宽描述符
		UAC2_AS_HS_ENDPOINT_DESC(DEVICE_ISO_IN_EP, USBD_AUDIO_MIC_FREQ, MIC_CHANNELS_NUM, MIC_ALT1_BITS), 		// 麦克风Alt1端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 麦克风Alt1端点通用描述符

	#ifdef MIC_ALT2_EN
		//麦克风AS Alt1
		UAC2_AS_INTERFACE_DESC(0x02, 0x02, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_MIC_OT_ID, MIC_CHANNELS_NUM),     									// 麦克风AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(MIC_ALT2_BITS),                      									// 麦克风Alt1位宽描述符
		UAC2_AS_HS_ENDPOINT_DESC(DEVICE_ISO_IN_EP, USBD_AUDIO_MIC_FREQ, MIC_CHANNELS_NUM, MIC_ALT2_BITS), 		// 麦克风Alt1端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 麦克风Alt1端点通用描述符
	#endif
	#ifdef MIC_ALT3_EN
		//麦克风AS Alt1
		UAC2_AS_INTERFACE_DESC(0x02, 0x03, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_MIC_OT_ID, MIC_CHANNELS_NUM),     									// 麦克风AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(MIC_ALT3_BITS),                      									// 麦克风Alt1位宽描述符
		UAC2_AS_HS_ENDPOINT_DESC(DEVICE_ISO_IN_EP, USBD_AUDIO_MIC_FREQ, MIC_CHANNELS_NUM, MIC_ALT3_BITS), 		// 麦克风Alt1端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 麦克风Alt1端点通用描述符
	#endif

		//3 hid 播放控制
		HID_INTERFACE_DESC(3,0,1),
		HID_DESC(sizeof(AudioCtrlReportDescriptor)),
		USB_ENDPOINT_GENERAL_DESC(DEVICE_INT_IN_EP,3,3,4),
	#if HID_DATA_FUN_EN
		#ifdef HID_DATA_EP_EN
		HID_INTERFACE_DESC(4,0,2),
		HID_DESC(sizeof(HidDataReportDescriptor)),
		USB_ENDPOINT_GENERAL_DESC(DEVICE_INT_IN_EP2,3,EP_MAX_SZE(DEVICE_HS_INT_IN_MPS),4),
		USB_ENDPOINT_GENERAL_DESC(DEVICE_INT_OUT_EP2,3,EP_MAX_SZE(DEVICE_HS_INT_OUT_MPS),4),
		#else
		HID_INTERFACE_DESC(4,0,0),
		HID_DESC(sizeof(HidDataReportDescriptor)),
		#endif
	#endif

		//声卡2
		UAC2_AC_TERMINAL_ASSOCIATION_DESC(5,2),
		UAC2_AC_STD_INTF_DESC(0x05,0x04),

		UAC2_AC_HEADER_DESC(AUDIO_HEADSET,(0x34+(PACKET1_CHANNELS_NUM*4))),
		UAC2_AC_CLK_SOURCE_DESC(AUDIO_SPEAKER1_CLK_ID),      													// 扬声器时钟源

		UAC2_AC_IT_DESC(AUDIO_SPEAKER1_IT_ID, AUDIO_SPEAKER1_CLK_ID, PACKET1_CHANNELS_NUM, 0x0101), 				// 扬声器输入终端
		UAC2_AC_FU_DESC(AUDIO_SPEAKER1_FU_ID, AUDIO_SPEAKER1_IT_ID, PACKET1_CHANNELS_NUM),             			// 扬声器功能单元
		UAC2_AC_OT_DESC(AUDIO_SPEAKER1_OT_ID, AUDIO_SPEAKER1_CLK_ID, AUDIO_SPEAKER1_FU_ID, 0x0301),    			// 扬声器输出终端
		//扬声器AS Alt0（无流传输）
		UAC2_AS_INTERFACE_DESC(0x06, 0x00, 0x00),
		//扬声器AS Alt1
		UAC2_AS_INTERFACE_DESC(0x06, 0x01, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_SPEAKER1_IT_ID, PACKET1_CHANNELS_NUM),  								// 扬声器AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(SPEAKER1_ALT1_BITS),                   									// 扬声器Alt1位宽描述符
		UAC2_AS_HS_ENDPOINT_DESC(DEVICE_ISO_OUT_EP1, USBD_AUDIO1_FREQ, PACKET1_CHANNELS_NUM, SPEAKER1_ALT1_BITS), 	// 扬声器Alt1端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 扬声器Alt1端点通用描述符

	#ifdef SPEAKER1_ALT2_EN
		//扬声器AS Alt2
		UAC2_AS_INTERFACE_DESC(0x06, 0x02, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_SPEAKER1_IT_ID, PACKET1_CHANNELS_NUM),  								// 扬声器AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(SPEAKER1_ALT2_BITS),                   									// 扬声器Alt2位宽描述符
		UAC2_AS_HS_ENDPOINT_DESC(DEVICE_ISO_OUT_EP1, USBD_AUDIO1_FREQ, PACKET1_CHANNELS_NUM, SPEAKER1_ALT2_BITS), 	// 扬声器Alt2端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 扬声器Alt2端点通用描述符
	#endif
	#ifdef SPEAKER1_ALT3_EN
		//扬声器AS Alt3
		UAC2_AS_INTERFACE_DESC(0x06, 0x03, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_SPEAKER1_IT_ID, PACKET1_CHANNELS_NUM),  								// 扬声器AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(SPEAKER1_ALT3_BITS),                   									// 扬声器Alt3位宽描述符
		UAC2_AS_HS_ENDPOINT_DESC(DEVICE_ISO_OUT_EP1, USBD_AUDIO1_FREQ, PACKET1_CHANNELS_NUM, SPEAKER1_ALT3_BITS), 	// 扬声器Alt3端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 扬声器Alt3端点通用描述符
	#endif
    };
    const uint8_t InterFaceNum_Tab(AUDIO_MIC_AUDIO)[]        = {0xFF,0x00,0x01,0x02,0x03,0x04,0x05,0x06};

    const uint8_t ConfigDescriptor_Tab(AUDIO_MIC_AUDIO_10)[] =
    {
  	#if HID_DATA_FUN_EN
		//一共4个接口
		0x09,0x02,W_TOTAL_LENGTH(0xC1+0x58+USER_HS_CONFIG_DESCRIPTOR_UAC_10_SIZE+18),0x07,0x01,0x00,0x80,0x32,
	#else
		0x09,0x02,W_TOTAL_LENGTH(0xC1+0x58+USER_HS_CONFIG_DESCRIPTOR_UAC_10_SIZE),0x06,0x01,0x00,0x80,0x32,
	#endif
		//0 audio control Interface
		0x09,0x04,0x00,0x00,0x00,0x01,0x01,0x00,0x00,
		0x0A,0x24,0x01,0x00,0x01,W_TOTAL_LENGTH(0x44+PACKET_CHANNELS_NUM+MIC_CHANNELS_NUM),0x02,0x01,0x02,
		0x0C,0x24,0x02,0x01,0x01,0x01,0x00,PACKET_CHANNELS_NUM,CHANNEL_CONFIG(PACKET_CHANNELS_NUM),0x00,0x00,0x00,
		UAC1_AC_FU_DESC(0x02,0x01,PACKET_CHANNELS_NUM),
		0x09,0x24,0x03,0x03,DEVICE_TYPE(SPEAKER_TYPE),0x00,0x02,0x00,
		0x0C,0x24,0x02,0x04,DEVICE_TYPE(MIC_TYPE),0x00,MIC_CHANNELS_NUM,CHANNEL_CONFIG(MIC_CHANNELS_NUM),0x00,0x00,0x00,
		UAC1_AC_FU_DESC(0x05,0x04,MIC_CHANNELS_NUM),
		0x09,0x24,0x03,0x06,0x01,0x01,0x00,0x05,0x00,
		//1 audio stream Interface, speeaker
		0x09,0x04,0x01,0x00,0x00,0x01,0x02,0x00,0x00,

		0x09,0x04,0x01,0x01,0x01,0x01,0x02,0x00,0x00,
		0x07,0x24,0x01,0x01,0x01,0x01,0x00,
		SPEAKER_FREQ_DESCRIPTOR_UAC_10_SIZE,0x24,0x02,0x01,PACKET_CHANNELS_NUM,SPEAKER_ALT1_BITS,SPEAKER_ALT1_BITS*8,USBD_AUDIO10_SPEAKER_FREQ_NUM,
		SAMPLE_FREQ_ARRAY(USBD_AUDIO10_FREQ,USBD_AUDIO10_SPEAKER_FREQ_NUM),
		0x09, 0x05, DEVICE_ISO_OUT_EP, 0x09, ISO_EP_MAX_SZE(USBD_AUDIO10_FREQ,PACKET_CHANNELS_NUM,SPEAKER_ALT1_BITS,HS_UAC_10_FRAM_PRT_SECOND), HS_UAC_10_ISO_INTERVAL, 0x00, 0x00,
		0x07,0x25,0x01,0x01,0x00,0x00,0x00,
	#ifdef SPEAKER_ALT2_EN
		0x09,0x04,0x01,0x02,0x01,0x01,0x02,0x00,0x00,
		0x07,0x24,0x01,0x01,0x01,0x01,0x00,
		SPEAKER_FREQ_DESCRIPTOR_UAC_10_SIZE,0x24,0x02,0x01,PACKET_CHANNELS_NUM,SPEAKER_ALT2_BITS,SPEAKER_ALT2_BITS*8,USBD_AUDIO10_SPEAKER_FREQ_NUM,
		SAMPLE_FREQ_ARRAY(USBD_AUDIO10_FREQ,USBD_AUDIO10_SPEAKER_FREQ_NUM),
		0x09, 0x05, DEVICE_ISO_OUT_EP, 0x09, ISO_EP_MAX_SZE(USBD_AUDIO10_FREQ,PACKET_CHANNELS_NUM,SPEAKER_ALT2_BITS,HS_UAC_10_FRAM_PRT_SECOND), HS_UAC_10_ISO_INTERVAL, 0x00, 0x00,
		0x07,0x25,0x01,0x01,0x00,0x00,0x00,
	#endif
		//2 audio stream Interface, microphone
		0x09,0x04,0x02,0x00,0x00,0x01,0x02,0x00,0x00,

		0x09,0x04,0x02,0x01,0x01,0x01,0x02,0x00,0x00,
		0x07,0x24,0x01,0x06,0x01,0x01,0x00,
		MIC_FREQ_DESCRIPTOR_UAC_10_SIZE,0x24,0x02,0x01,MIC_CHANNELS_NUM,MIC_ALT1_BITS,MIC_ALT1_BITS*8,USBD_AUDIO10_MIC_FREQ_NUM,
		SAMPLE_FREQ_ARRAY(USBD_AUDIO10_MIC_FREQ,USBD_AUDIO10_MIC_FREQ_NUM),
		0x09, 0x05, DEVICE_ISO_IN_EP, 0x09, ISO_EP_MAX_SZE(USBD_AUDIO10_MIC_FREQ,MIC_CHANNELS_NUM,MIC_ALT1_BITS,HS_UAC_10_FRAM_PRT_SECOND), HS_UAC_10_ISO_INTERVAL, 0x00, 0x00,
		0x07,0x25,0x01,0x01,0x00,0x00,0x00,
	#ifdef MIC_ALT2_EN
		0x09,0x04,0x02,0x02,0x01,0x01,0x02,0x00,0x00,
		0x07,0x24,0x01,0x06,0x01,0x01,0x00,
		MIC_FREQ_DESCRIPTOR_UAC_10_SIZE,0x24,0x02,0x01,MIC_CHANNELS_NUM,MIC_ALT2_BITS,MIC_ALT2_BITS*8,USBD_AUDIO10_MIC_FREQ_NUM,
		SAMPLE_FREQ_ARRAY(USBD_AUDIO10_MIC_FREQ,USBD_AUDIO10_MIC_FREQ_NUM),
		0x09, 0x05, DEVICE_ISO_IN_EP, 0x09, ISO_EP_MAX_SZE(USBD_AUDIO10_MIC_FREQ,MIC_CHANNELS_NUM,MIC_ALT2_BITS,HS_UAC_10_FRAM_PRT_SECOND), HS_UAC_10_ISO_INTERVAL, 0x00, 0x00,
		0x07,0x25,0x01,0x01,0x00,0x00,0x00,
	#endif
		//3 hid 播放控制
		0x09,0x04,0x03,0x00,0x01,0x03,0x00,0x00,0x00,
		0x09,0x21,0x01,0x02,0x00,0x01,0x22,sizeof(AudioCtrlReportDescriptor),0x00,
		0x07,0x05,DEVICE_INT_IN_EP,0x03,DEVICE_FS_INT_IN_MPS,0x00,0x04,
	#if HID_DATA_FUN_EN
		0x09,0x04,0x04,0x00,0x00,0x03,0x00,0x00,0x00,
		0x09,0x21,0x01,0x02,0x00,0x01,0x22,sizeof(HidDataReportDescriptor),0x00,
	#endif

		//声卡2
		//5 audio control Interface
		0x09,0x04,0x05,0x00,0x00,0x01,0x01,0x00,0x04,
		0x09,0x24,0x01,0x00,0x01,W_TOTAL_LENGTH(0x26+PACKET1_CHANNELS_NUM),0x01,0x06,
		0x0C,0x24,0x02,AUDIO_SPEAKER1_IT_ID,0x01,0x01,0x00,PACKET1_CHANNELS_NUM,CHANNEL_CONFIG(PACKET1_CHANNELS_NUM),0x00,0x00,0x00,
		UAC1_AC_FU_DESC(AUDIO_SPEAKER1_FU_ID,AUDIO_SPEAKER1_IT_ID,PACKET1_CHANNELS_NUM),
		0x09,0x24,0x03,AUDIO_SPEAKER1_OT_ID,DEVICE_TYPE(SPEAKER_TYPE),0x00,AUDIO_SPEAKER1_FU_ID,0x00,

		//6 audio stream Interface, speeaker
		0x09,0x04,0x06,0x00,0x00,0x01,0x02,0x00,0x00,
		0x09,0x04,0x06,0x01,0x01,0x01,0x02,0x00,0x00,
		0x07,0x24,0x01,AUDIO_SPEAKER1_IT_ID,0x01,0x01,0x00,
		SPEAKER_FREQ_DESCRIPTOR_UAC_10_SIZE,0x24,0x02,0x01,PACKET1_CHANNELS_NUM,SPEAKER1_ALT1_BITS,SPEAKER1_ALT1_BITS*8,USBD_AUDIO10_SPEAKER1_FREQ_NUM,
		SAMPLE_FREQ_ARRAY(USBD_AUDIO10_SPEAKER1_FREQ,USBD_AUDIO10_SPEAKER1_FREQ_NUM),
		0x09, 0x05, DEVICE_ISO_OUT_EP1, 0x09, ISO_EP_MAX_SZE(USBD_AUDIO10_SPEAKER1_FREQ,PACKET1_CHANNELS_NUM,SPEAKER1_ALT1_BITS,HS_UAC_10_FRAM_PRT_SECOND), HS_UAC_10_ISO_INTERVAL, 0x00, 0x00,
		0x07,0x25,0x01,0x01,0x00,0x00,0x00,
	#ifdef SPEAKER1_ALT2_EN
		0x09,0x04,0x06,0x02,0x01,0x01,0x02,0x00,0x00,
		0x07,0x24,0x01,AUDIO_SPEAKER1_IT_ID,0x01,0x01,0x00,
		SPEAKER_FREQ_DESCRIPTOR_UAC_10_SIZE,0x24,0x02,0x01,PACKET1_CHANNELS_NUM,SPEAKER1_ALT2_BITS,SPEAKER1_ALT2_BITS*8,USBD_AUDIO10_SPEAKER1_FREQ_NUM,
		SAMPLE_FREQ_ARRAY(USBD_AUDIO10_SPEAKER1_FREQ,USBD_AUDIO10_SPEAKER1_FREQ_NUM),
		0x09, 0x05, DEVICE_ISO_OUT_EP1, 0x09, ISO_EP_MAX_SZE(USBD_AUDIO10_SPEAKER1_FREQ,PACKET1_CHANNELS_NUM,SPEAKER1_ALT2_BITS,HS_UAC_10_FRAM_PRT_SECOND), HS_UAC_10_ISO_INTERVAL, 0x00, 0x00,
		0x07,0x25,0x01,0x01,0x00,0x00,0x00,
	#endif
    };
    const uint8_t InterFaceNum_Tab(AUDIO_MIC_AUDIO_10)[]   = {0xFF,0x00,0x01,0x02,0x03,0x04,0x05,0x06};

    const uint8_t ConfigDescriptor_Tab(FS_AUDIO_MIC_AUDIO)[] =
    {
	#if HID_DATA_FUN_EN
		#ifdef HID_DATA_EP_EN
		USB_CFG_DESC_HEADER(0x108+0x80+USER_FS_CONFIG_DESCRIPTOR_UAC_20_SIZE+32,7),
		#else
		USB_CFG_DESC_HEADER(0x108+0x80+USER_FS_CONFIG_DESCRIPTOR_UAC_20_SIZE+18,7),
		#endif
	#else
		USB_CFG_DESC_HEADER(0x108+0x80+USER_FS_CONFIG_DESCRIPTOR_UAC_20_SIZE,6),
	#endif

		UAC2_AC_TERMINAL_ASSOCIATION_DESC(0,3),

		UAC2_AC_STD_INTF_DESC(0x00,0x00),

		UAC2_AC_HEADER_DESC(AUDIO_HEADSET,(103+(PACKET_CHANNELS_NUM*4)+(MIC_CHANNELS_NUM*4))),
		UAC2_AC_CLK_SOURCE_DESC(AUDIO_SPEAKER_CLK_ID),      															// 扬声器时钟源
		UAC2_AC_CLK_SOURCE_DESC(AUDIO_MIC_CLK_ID),  															// 麦克风时钟源

		UAC2_AC_IT_DESC(AUDIO_SPEAKER_IT_ID, AUDIO_SPEAKER_CLK_ID, PACKET_CHANNELS_NUM, 0x0101), 				// 扬声器输入终端
		UAC2_AC_FU_DESC(AUDIO_SPEAKER_FU_ID, AUDIO_SPEAKER_IT_ID, PACKET_CHANNELS_NUM),             			// 扬声器功能单元
		UAC2_AC_OT_DESC(AUDIO_SPEAKER_OT_ID, AUDIO_SPEAKER_CLK_ID, AUDIO_SPEAKER_FU_ID, SPEAKER_TYPE),    			// 扬声器输出终端

		UAC2_AC_IT_DESC(AUDIO_MIC_IT_ID, AUDIO_MIC_CLK_ID, MIC_CHANNELS_NUM, MIC_TYPE), 							// 麦克风输入终端
		UAC2_AC_FU_DESC(AUDIO_MIC_FU_ID, AUDIO_MIC_IT_ID, MIC_CHANNELS_NUM),                        			// 麦克风功能单元
		UAC2_AC_OT_DESC(AUDIO_MIC_OT_ID, AUDIO_MIC_CLK_ID, AUDIO_MIC_FU_ID, 0x0101), 							// 麦克风输出终端

		//扬声器AS Alt0（无流传输）
		UAC2_AS_INTERFACE_DESC(0x01, 0x00, 0x00),
		//扬声器AS Alt1
		UAC2_AS_INTERFACE_DESC(0x01, 0x01, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_SPEAKER_IT_ID, PACKET_CHANNELS_NUM),  								// 扬声器AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(SPEAKER_ALT1_BITS),                   									// 扬声器Alt1位宽描述符
		UAC2_AS_FS_ENDPOINT_DESC(DEVICE_ISO_OUT_EP, USBD_AUDIO10_FREQ, PACKET_CHANNELS_NUM, SPEAKER_ALT1_BITS), // 扬声器Alt1端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 扬声器Alt1端点通用描述符

	#ifdef SPEAKER_ALT2_EN
		//扬声器AS Alt2
		UAC2_AS_INTERFACE_DESC(0x01, 0x02, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_SPEAKER_IT_ID, PACKET_CHANNELS_NUM),  								// 扬声器AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(SPEAKER_ALT2_BITS),                   									// 扬声器Alt1位宽描述符
		UAC2_AS_FS_ENDPOINT_DESC(DEVICE_ISO_OUT_EP, USBD_AUDIO10_FREQ, PACKET_CHANNELS_NUM, SPEAKER_ALT2_BITS), // 扬声器Alt1端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 扬声器Alt1端点通用描述符
	#endif

		//麦克风AS Alt0（无流传输）
		UAC2_AS_INTERFACE_DESC(0x02, 0x00, 0x00),
		//麦克风AS Alt1
		UAC2_AS_INTERFACE_DESC(0x02, 0x01, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_MIC_OT_ID, MIC_CHANNELS_NUM),     									// 麦克风AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(MIC_ALT1_BITS),                      									// 麦克风Alt1位宽描述符
		UAC2_AS_FS_ENDPOINT_DESC(DEVICE_ISO_IN_EP, USBD_AUDIO10_MIC_FREQ, MIC_CHANNELS_NUM, MIC_ALT1_BITS), 	// 麦克风Alt1端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 麦克风Alt1端点通用描述符

	#ifdef MIC_ALT2_EN
		//麦克风AS Alt1
		UAC2_AS_INTERFACE_DESC(0x02, 0x02, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_MIC_OT_ID, MIC_CHANNELS_NUM),     									// 麦克风AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(MIC_ALT2_BITS),                      									// 麦克风Alt1位宽描述符
		UAC2_AS_FS_ENDPOINT_DESC(DEVICE_ISO_IN_EP, USBD_AUDIO10_MIC_FREQ, MIC_CHANNELS_NUM, MIC_ALT2_BITS), 	// 麦克风Alt1端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 麦克风Alt1端点通用描述符
	#endif
		//3 hid 播放控制
		HID_INTERFACE_DESC(3,0,1),
		HID_DESC(sizeof(AudioCtrlReportDescriptor)),
		USB_ENDPOINT_GENERAL_DESC(DEVICE_INT_IN_EP,3,3,1),
	#if HID_DATA_FUN_EN
		#ifdef HID_DATA_EP_EN
		HID_INTERFACE_DESC(4,0,2),
		HID_DESC(sizeof(HidDataReportDescriptor)),
		USB_ENDPOINT_GENERAL_DESC(DEVICE_INT_IN_EP2,3,EP_MAX_SZE(DEVICE_FS_INT_IN_MPS),4),
		USB_ENDPOINT_GENERAL_DESC(DEVICE_INT_OUT_EP2,3,EP_MAX_SZE(DEVICE_FS_INT_OUT_MPS),4),
		#else
		HID_INTERFACE_DESC(4,0,0),
		HID_DESC(sizeof(HidDataReportDescriptor)),
		#endif
	#endif

			//声卡2
		UAC2_AC_TERMINAL_ASSOCIATION_DESC(5,2),
		UAC2_AC_STD_INTF_DESC(0x05,0x04),

		UAC2_AC_HEADER_DESC(AUDIO_HEADSET,(0x34+(PACKET1_CHANNELS_NUM*4))),
		UAC2_AC_CLK_SOURCE_DESC(AUDIO_SPEAKER1_CLK_ID),      													// 扬声器时钟源

		UAC2_AC_IT_DESC(AUDIO_SPEAKER1_IT_ID, AUDIO_SPEAKER1_CLK_ID, PACKET1_CHANNELS_NUM, 0x0101), 				// 扬声器输入终端
		UAC2_AC_FU_DESC(AUDIO_SPEAKER1_FU_ID, AUDIO_SPEAKER1_IT_ID, PACKET1_CHANNELS_NUM),             			// 扬声器功能单元
		UAC2_AC_OT_DESC(AUDIO_SPEAKER1_OT_ID, AUDIO_SPEAKER1_CLK_ID, AUDIO_SPEAKER1_FU_ID, 0x0301),    			// 扬声器输出终端
		//扬声器AS Alt0（无流传输）
		UAC2_AS_INTERFACE_DESC(0x06, 0x00, 0x00),
		//扬声器AS Alt1
		UAC2_AS_INTERFACE_DESC(0x06, 0x01, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_SPEAKER1_IT_ID, PACKET1_CHANNELS_NUM),  								// 扬声器AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(SPEAKER1_ALT1_BITS),                   									// 扬声器Alt1位宽描述符
		UAC2_AS_FS_ENDPOINT_DESC(DEVICE_ISO_OUT_EP1, USBD_AUDIO10_SPEAKER1_FREQ, PACKET1_CHANNELS_NUM, SPEAKER1_ALT1_BITS), 	// 扬声器Alt1端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 扬声器Alt1端点通用描述符

	#ifdef SPEAKER1_ALT2_EN
		//扬声器AS Alt2
		UAC2_AS_INTERFACE_DESC(0x06, 0x02, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_SPEAKER1_IT_ID, PACKET1_CHANNELS_NUM),  								// 扬声器AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(SPEAKER1_ALT2_BITS),                   									// 扬声器Alt2位宽描述符
		UAC2_AS_FS_ENDPOINT_DESC(DEVICE_ISO_OUT_EP1, USBD_AUDIO10_SPEAKER1_FREQ, PACKET1_CHANNELS_NUM, SPEAKER1_ALT2_BITS), 	// 扬声器Alt2端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 扬声器Alt2端点通用描述符
	#endif
    };
    const uint8_t InterFaceNum_Tab(FS_AUDIO_MIC_AUDIO)[]    = {0xFF,0x00,0x01,0x02,0x03,0x04,0x05,0x06};

    const uint8_t ConfigDescriptor_Tab(FS_AUDIO_MIC_AUDIO_10)[] =
    {
	#if HID_DATA_FUN_EN
		//一共4个接口
		0x09,0x02,W_TOTAL_LENGTH(0xC1+0x58+USER_FS_CONFIG_DESCRIPTOR_UAC_10_SIZE+18),0x07,0x01,0x00,0x80,0x32,
	#else
		0x09,0x02,W_TOTAL_LENGTH(0xC1+0x58+USER_FS_CONFIG_DESCRIPTOR_UAC_10_SIZE),0x06,0x01,0x00,0x80,0x32,
	#endif
		//0 audio control Interface
		0x09,0x04,0x00,0x00,0x00,0x01,0x01,0x00,0x00,
		0x0A,0x24,0x01,0x00,0x01,W_TOTAL_LENGTH(0x44+PACKET_CHANNELS_NUM+MIC_CHANNELS_NUM),0x02,0x01,0x02,
		0x0C,0x24,0x02,0x01,0x01,0x01,0x00,PACKET_CHANNELS_NUM,CHANNEL_CONFIG(PACKET_CHANNELS_NUM),0x00,0x00,0x00,
		UAC1_AC_FU_DESC(0x02,0x01,PACKET_CHANNELS_NUM),
		0x09,0x24,0x03,0x03,DEVICE_TYPE(SPEAKER_TYPE),0x00,0x02,0x00,
		0x0C,0x24,0x02,0x04,DEVICE_TYPE(MIC_TYPE),0x00,MIC_CHANNELS_NUM,CHANNEL_CONFIG(MIC_CHANNELS_NUM),0x00,0x00,0x00,
		UAC1_AC_FU_DESC(0x05,0x04,MIC_CHANNELS_NUM),
		0x09,0x24,0x03,0x06,0x01,0x01,0x00,0x05,0x00,
		//1 audio stream Interface, speeaker
		0x09,0x04,0x01,0x00,0x00,0x01,0x02,0x00,0x00,

		0x09,0x04,0x01,0x01,0x01,0x01,0x02,0x00,0x00,
		0x07,0x24,0x01,0x01,0x01,0x01,0x00,
		SPEAKER_FREQ_DESCRIPTOR_UAC_10_SIZE,0x24,0x02,0x01,PACKET_CHANNELS_NUM,SPEAKER_ALT1_BITS,SPEAKER_ALT1_BITS*8,USBD_AUDIO10_SPEAKER_FREQ_NUM,
		SAMPLE_FREQ_ARRAY(USBD_AUDIO10_FREQ,USBD_AUDIO10_SPEAKER_FREQ_NUM),
		0x09, 0x05, DEVICE_ISO_OUT_EP, 0x09, AUDIO_EP_MAX_SZE(USBD_AUDIO10_FREQ,PACKET_CHANNELS_NUM,SPEAKER_ALT1_BITS), FS_ISO_INTERVAL, 0x00, 0x00,
		0x07,0x25,0x01,0x01,0x00,0x00,0x00,
	#ifdef SPEAKER_ALT2_EN
		0x09,0x04,0x01,0x02,0x01,0x01,0x02,0x00,0x00,
		0x07,0x24,0x01,0x01,0x01,0x01,0x00,
		SPEAKER_FREQ_DESCRIPTOR_UAC_10_SIZE,0x24,0x02,0x01,PACKET_CHANNELS_NUM,SPEAKER_ALT2_BITS,SPEAKER_ALT2_BITS*8,USBD_AUDIO10_SPEAKER_FREQ_NUM,
		SAMPLE_FREQ_ARRAY(USBD_AUDIO10_FREQ,USBD_AUDIO10_SPEAKER_FREQ_NUM),
		0x09, 0x05, DEVICE_ISO_OUT_EP, 0x09, AUDIO_EP_MAX_SZE(USBD_AUDIO10_FREQ,PACKET_CHANNELS_NUM,SPEAKER_ALT2_BITS), FS_ISO_INTERVAL, 0x00, 0x00,
		0x07,0x25,0x01,0x01,0x00,0x00,0x00,
	#endif
		//2 audio stream Interface, microphone
		0x09,0x04,0x02,0x00,0x00,0x01,0x02,0x00,0x00,

		0x09,0x04,0x02,0x01,0x01,0x01,0x02,0x00,0x00,
		0x07,0x24,0x01,0x06,0x01,0x01,0x00,
		MIC_FREQ_DESCRIPTOR_UAC_10_SIZE,0x24,0x02,0x01,MIC_CHANNELS_NUM,MIC_ALT1_BITS,MIC_ALT1_BITS*8,USBD_AUDIO10_MIC_FREQ_NUM,
		SAMPLE_FREQ_ARRAY(USBD_AUDIO10_MIC_FREQ,USBD_AUDIO10_MIC_FREQ_NUM),
		0x09, 0x05, DEVICE_ISO_IN_EP, 0x09, AUDIO_EP_MAX_SZE(USBD_AUDIO10_MIC_FREQ,MIC_CHANNELS_NUM,MIC_ALT1_BITS), FS_ISO_INTERVAL, 0x00, 0x00,
		0x07,0x25,0x01,0x01,0x00,0x00,0x00,
	#ifdef MIC_ALT2_EN
		0x09,0x04,0x02,0x02,0x01,0x01,0x02,0x00,0x00,
		0x07,0x24,0x01,0x06,0x01,0x01,0x00,
		MIC_FREQ_DESCRIPTOR_UAC_10_SIZE,0x24,0x02,0x01,MIC_CHANNELS_NUM,MIC_ALT2_BITS,MIC_ALT2_BITS*8,USBD_AUDIO10_MIC_FREQ_NUM,
		SAMPLE_FREQ_ARRAY(USBD_AUDIO10_MIC_FREQ,USBD_AUDIO10_MIC_FREQ_NUM),
		0x09, 0x05, DEVICE_ISO_IN_EP, 0x09, AUDIO_EP_MAX_SZE(USBD_AUDIO10_MIC_FREQ,MIC_CHANNELS_NUM,MIC_ALT2_BITS), FS_ISO_INTERVAL, 0x00, 0x00,
		0x07,0x25,0x01,0x01,0x00,0x00,0x00,
	#endif
		//3 hid 播放控制
		0x09,0x04,0x03,0x00,0x01,0x03,0x00,0x00,0x00,
		0x09,0x21,0x01,0x02,0x00,0x01,0x22,sizeof(AudioCtrlReportDescriptor),0x00,
		0x07,0x05,DEVICE_INT_IN_EP,0x03,DEVICE_FS_INT_IN_MPS,0x00,0x01,
	#if HID_DATA_FUN_EN
		0x09,0x04,0x04,0x00,0x00,0x03,0x00,0x00,0x00,
		0x09,0x21,0x01,0x02,0x00,0x01,0x22,sizeof(HidDataReportDescriptor),0x00,
	#endif

			//声卡2
		//5 audio control Interface
		0x09,0x04,0x05,0x00,0x00,0x01,0x01,0x00,0x04,
		0x09,0x24,0x01,0x00,0x01,W_TOTAL_LENGTH(0x26+PACKET1_CHANNELS_NUM),0x01,0x06,
		0x0C,0x24,0x02,AUDIO_SPEAKER1_IT_ID,0x01,0x01,0x00,PACKET1_CHANNELS_NUM,CHANNEL_CONFIG(PACKET1_CHANNELS_NUM),0x00,0x00,0x00,
		UAC1_AC_FU_DESC(AUDIO_SPEAKER1_FU_ID,AUDIO_SPEAKER1_IT_ID,PACKET1_CHANNELS_NUM),
		0x09,0x24,0x03,AUDIO_SPEAKER1_OT_ID,DEVICE_TYPE(SPEAKER_TYPE),0x00,AUDIO_SPEAKER1_FU_ID,0x00,

		//6 audio stream Interface, speeaker
		0x09,0x04,0x06,0x00,0x00,0x01,0x02,0x00,0x00,
		0x09,0x04,0x06,0x01,0x01,0x01,0x02,0x00,0x00,
		0x07,0x24,0x01,AUDIO_SPEAKER1_IT_ID,0x01,0x01,0x00,
		SPEAKER_FREQ_DESCRIPTOR_UAC_10_SIZE,0x24,0x02,0x01,PACKET1_CHANNELS_NUM,SPEAKER1_ALT1_BITS,SPEAKER1_ALT1_BITS*8,USBD_AUDIO10_SPEAKER1_FREQ_NUM,
		SAMPLE_FREQ_ARRAY(USBD_AUDIO10_SPEAKER1_FREQ,USBD_AUDIO10_SPEAKER1_FREQ_NUM),
		0x09, 0x05, DEVICE_ISO_OUT_EP1, 0x09, AUDIO_EP_MAX_SZE(USBD_AUDIO10_SPEAKER1_FREQ,PACKET1_CHANNELS_NUM,SPEAKER1_ALT1_BITS), FS_ISO_INTERVAL, 0x00, 0x00,
		0x07,0x25,0x01,0x01,0x00,0x00,0x00,
	#ifdef SPEAKER1_ALT2_EN
		0x09,0x04,0x06,0x02,0x01,0x01,0x02,0x00,0x00,
		0x07,0x24,0x01,AUDIO_SPEAKER1_IT_ID,0x01,0x01,0x00,
		SPEAKER_FREQ_DESCRIPTOR_UAC_10_SIZE,0x24,0x02,0x01,PACKET1_CHANNELS_NUM,SPEAKER1_ALT2_BITS,SPEAKER1_ALT2_BITS*8,USBD_AUDIO10_SPEAKER1_FREQ_NUM,
		SAMPLE_FREQ_ARRAY(USBD_AUDIO10_SPEAKER1_FREQ,USBD_AUDIO10_SPEAKER1_FREQ_NUM),
		0x09, 0x05, DEVICE_ISO_OUT_EP1, 0x09, AUDIO_EP_MAX_SZE(USBD_AUDIO10_SPEAKER1_FREQ,PACKET1_CHANNELS_NUM,SPEAKER1_ALT2_BITS), FS_ISO_INTERVAL, 0x00, 0x00,
		0x07,0x25,0x01,0x01,0x00,0x00,0x00,
	#endif
    };
    const uint8_t InterFaceNum_Tab(FS_AUDIO_MIC_AUDIO_10)[] = {0xFF,0x00,0x01,0x02,0x03,0x04,0x05,0x06};
#endif //#ifdef CFG_DESCRIP_AUDIO_MIC_AUDIO_EN

#ifdef CFG_DESCRIP_READER_EN
	const uint8_t ConfigDescriptor_Tab(READER)[] =
	{

	#if HID_DATA_FUN_EN
		//一共4个接口
		0x09,0x02,W_TOTAL_LENGTH(0x32),0x02,0x01,0x00,0x80,0x32,
	#else
		//一共1个接口
		0x09,0x02,W_TOTAL_LENGTH(0x20),0x01,0x01,0x00,0x80,0x32,
	#endif
		//mass-storage Interface
		0x09,0x04,0x00,0x00,0x02,0x08,0x06,0x50,0x00,
		0x07,0x05,DEVICE_BULK_IN_EP,0x02, EP_MAX_SZE(DEVICE_HS_BULK_IN_MPS),0x01,
		0x07,0x05,DEVICE_BULK_OUT_EP,0x02,EP_MAX_SZE(DEVICE_HS_BULK_OUT_MPS),0x01,

	#if HID_DATA_FUN_EN
		0x09,0x04,0x04,0x00,0x00,0x03,0x00,0x00,0x00,
		0x09,0x21,0x01,0x02,0x00,0x01,0x22,sizeof(HidDataReportDescriptor),0x00,
	#endif
	};
	const uint8_t InterFaceNum_Tab(READER)[] = {0x00,0xFF,0xFF,0xFF,0xFF,0x04};
#endif
#ifdef CFG_DESCRIP_READER_EN
	const uint8_t ConfigDescriptor_Tab(FS_READER)[] =
	{

	#if HID_DATA_FUN_EN
		//一共4个接口
		0x09,0x02,W_TOTAL_LENGTH(0x32),0x02,0x01,0x00,0x80,0x32,
	#else
		//一共1个接口
		0x09,0x02,W_TOTAL_LENGTH(0x20),0x01,0x01,0x00,0x80,0x32,
	#endif
		//mass-storage Interface
		0x09,0x04,0x00,0x00,0x02,0x08,0x06,0x50,0x00,
		0x07,0x05,DEVICE_BULK_IN_EP,0x02, EP_MAX_SZE(DEVICE_FS_BULK_IN_MPS),0x01,
		0x07,0x05,DEVICE_BULK_OUT_EP,0x02,EP_MAX_SZE(DEVICE_FS_BULK_OUT_MPS),0x01,

	#if HID_DATA_FUN_EN
		0x09,0x04,0x04,0x00,0x00,0x03,0x00,0x00,0x00,
		0x09,0x21,0x01,0x02,0x00,0x01,0x22,sizeof(HidDataReportDescriptor),0x00,
	#endif
	};
	const uint8_t InterFaceNum_Tab(FS_READER)[] = {0x00,0xFF,0xFF,0xFF,0xFF,0x04};
#endif


#ifdef CFG_DESCRIP_HID_EN
	//HID数据传输
	const uint8_t ConfigDescriptor_Tab(FS_HID)[] =
	{
	#if defined(CFG_FUNC_USBDEBUG_EN) && defined(CFG_COMMUNICATION_BY_FS_USB)
		//接口0保留在线调音/升级控制传输，接口1用于标准HID中断IN日志
		0x09, 0x02, 0x34, 0x00, 0x02, 0x01, 0x00, 0x80, 0x32,
	#else
		//一共1个接口
		0x09, 0x02, 0x1B, 0x00, 0x01, 0x01, 0x00, 0x80, 0x32,
	#endif

	#if HID_DATA_FUN_EN
		//HID自定数据传输接口
		0x09,0x04,0x00,0x00,0x00,0x03,0x00,0x00,0x00,
		0x09,0x21,0x01,0x02,0x00,0x01,0x22,sizeof(HidDataReportDescriptor),0x00,
	#endif
	#if defined(CFG_FUNC_USBDEBUG_EN) && defined(CFG_COMMUNICATION_BY_FS_USB)
		0x09,0x04,0x01,0x00,0x01,0x03,0x00,0x00,0x00,
		0x09,0x21,0x01,0x02,0x00,0x01,0x22,sizeof(HidLogReportDescriptor),0x00,
		USB_ENDPOINT_GENERAL_DESC(DEVICE_INT_IN_EP2,3,DEVICE_FS_INT_IN_MPS,4)
	#endif
	};
	const uint8_t InterFaceNum_Tab(FS_HID)[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0x00};

	//HID数据传输
	const uint8_t ConfigDescriptor_Tab(HID)[] =
	{
	#ifdef CFG_DESCRIP_HID_HEART_BEAT_EN
			//一共2个接口
			0x09, 0x02, W_TOTAL_LENGTH(0x1B+0x19), 0x02, 0x01, 0x00, 0x80, 0x32,

		#if HID_DATA_FUN_EN
			//HID自定数据传输接口
			0x09,0x04,0x00,0x00,0x00,0x03,0x00,0x00,0x00,
			0x09,0x21,0x01,0x02,0x00,0x01,0x22,sizeof(HidDataReportDescriptor),0x00,
		#endif
			//HID自定义心跳包接口
			0x09,0x04,0x01,0x00,0x01,0x03,0x00,0x00,0x00,
			0x09,0x21,0x01,0x02,0x00,0x01,0x22,sizeof(HeartBeatReportDescriptor),0x00,
			0x07,0x05,DEVICE_INT_IN_EP1,0x03, EP_MAX_SZE(0x02),0x08
	#else
			//一共1个接口
			0x09, 0x02, 0x1B, 0x00, 0x01, 0x01, 0x00, 0x80, 0x32,

		#if HID_DATA_FUN_EN
			//HID自定数据传输接口
			0x09,0x04,0x00,0x00,0x00,0x03,0x00,0x00,0x00,
			0x09,0x21,0x01,0x02,0x00,0x01,0x22,sizeof(HidDataReportDescriptor),0x00
		#endif
	#endif
	};
	const uint8_t InterFaceNum_Tab(HID)[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0x00};
#endif

#if defined(CFG_DESCRIP_AUDIO_MIC_EN)
	//声卡 MIC输入和AUDIO输出
	const uint8_t ConfigDescriptor_Tab(FS_AUDIO_MIC)[] =
	{
	#if HID_DATA_FUN_EN
		#ifdef HID_DATA_EP_EN
		USB_CFG_DESC_HEADER(0x108+USER_FS_CONFIG_DESCRIPTOR_UAC_20_SIZE+32,5),
		#else
		USB_CFG_DESC_HEADER(0x108+USER_FS_CONFIG_DESCRIPTOR_UAC_20_SIZE+18,5),
		#endif
	#else
		USB_CFG_DESC_HEADER(0x108+USER_FS_CONFIG_DESCRIPTOR_UAC_20_SIZE,4),
	#endif

		UAC2_AC_TERMINAL_ASSOCIATION_DESC(0,3),

		UAC2_AC_STD_INTF_DESC(0x00,0x00),

		UAC2_AC_HEADER_DESC(AUDIO_HEADSET,(103+(PACKET_CHANNELS_NUM*4)+(MIC_CHANNELS_NUM*4))),
		UAC2_AC_CLK_SOURCE_DESC(AUDIO_SPEAKER_CLK_ID),      															// 扬声器时钟源
		UAC2_AC_CLK_SOURCE_DESC(AUDIO_MIC_CLK_ID),  															// 麦克风时钟源

		UAC2_AC_IT_DESC(AUDIO_SPEAKER_IT_ID, AUDIO_SPEAKER_CLK_ID, PACKET_CHANNELS_NUM, 0x0101), 				// 扬声器输入终端
		UAC2_AC_FU_DESC(AUDIO_SPEAKER_FU_ID, AUDIO_SPEAKER_IT_ID, PACKET_CHANNELS_NUM),             			// 扬声器功能单元
		UAC2_AC_OT_DESC(AUDIO_SPEAKER_OT_ID, AUDIO_SPEAKER_CLK_ID, AUDIO_SPEAKER_FU_ID, SPEAKER_TYPE),    			// 扬声器输出终端

		UAC2_AC_IT_DESC(AUDIO_MIC_IT_ID, AUDIO_MIC_CLK_ID, MIC_CHANNELS_NUM, MIC_TYPE), 							// 麦克风输入终端
		UAC2_AC_FU_DESC(AUDIO_MIC_FU_ID, AUDIO_MIC_IT_ID, MIC_CHANNELS_NUM),                        			// 麦克风功能单元
		UAC2_AC_OT_DESC(AUDIO_MIC_OT_ID, AUDIO_MIC_CLK_ID, AUDIO_MIC_FU_ID, 0x0101), 							// 麦克风输出终端

		//扬声器AS Alt0（无流传输）
		UAC2_AS_INTERFACE_DESC(0x01, 0x00, 0x00),
		//扬声器AS Alt1
		UAC2_AS_INTERFACE_DESC(0x01, 0x01, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_SPEAKER_IT_ID, PACKET_CHANNELS_NUM),  								// 扬声器AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(SPEAKER_ALT1_BITS),                   									// 扬声器Alt1位宽描述符
		UAC2_AS_FS_ENDPOINT_DESC(DEVICE_ISO_OUT_EP, USBD_AUDIO10_FREQ, PACKET_CHANNELS_NUM, SPEAKER_ALT1_BITS), // 扬声器Alt1端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 扬声器Alt1端点通用描述符

	#ifdef SPEAKER_ALT2_EN
		//扬声器AS Alt2
		UAC2_AS_INTERFACE_DESC(0x01, 0x02, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_SPEAKER_IT_ID, PACKET_CHANNELS_NUM),  								// 扬声器AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(SPEAKER_ALT2_BITS),                   									// 扬声器Alt1位宽描述符
		UAC2_AS_FS_ENDPOINT_DESC(DEVICE_ISO_OUT_EP, USBD_AUDIO10_FREQ, PACKET_CHANNELS_NUM, SPEAKER_ALT2_BITS), // 扬声器Alt1端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 扬声器Alt1端点通用描述符
	#endif

		//麦克风AS Alt0（无流传输）
		UAC2_AS_INTERFACE_DESC(0x02, 0x00, 0x00),
		//麦克风AS Alt1
		UAC2_AS_INTERFACE_DESC(0x02, 0x01, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_MIC_OT_ID, MIC_CHANNELS_NUM),     									// 麦克风AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(MIC_ALT1_BITS),                      									// 麦克风Alt1位宽描述符
		UAC2_AS_FS_ENDPOINT_DESC(DEVICE_ISO_IN_EP, USBD_AUDIO10_MIC_FREQ, MIC_CHANNELS_NUM, MIC_ALT1_BITS), 	// 麦克风Alt1端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 麦克风Alt1端点通用描述符

	#ifdef MIC_ALT2_EN
		//麦克风AS Alt1
		UAC2_AS_INTERFACE_DESC(0x02, 0x02, 0x01),
		UAC2_AS_FORMAT_TYPE_I_PCM_DESC(AUDIO_MIC_OT_ID, MIC_CHANNELS_NUM),     									// 麦克风AS通用格式描述符
		UAC2_AS_FORMAT_TYPE_I_BITS_DESC(MIC_ALT2_BITS),                      									// 麦克风Alt1位宽描述符
		UAC2_AS_FS_ENDPOINT_DESC(DEVICE_ISO_IN_EP, USBD_AUDIO10_MIC_FREQ, MIC_CHANNELS_NUM, MIC_ALT2_BITS), 	// 麦克风Alt1端点
		UAC2_AS_EP_GENERAL_DESC(0x00, 0x0000),                                									// 麦克风Alt1端点通用描述符
	#endif
		//3 hid 播放控制
		HID_INTERFACE_DESC(3,0,1),
		HID_DESC(sizeof(AudioCtrlReportDescriptor)),
		USB_ENDPOINT_GENERAL_DESC(DEVICE_INT_IN_EP,3,3,1),
	#if HID_DATA_FUN_EN
		#ifdef HID_DATA_EP_EN
		HID_INTERFACE_DESC(4,0,2),
		HID_DESC(sizeof(HidDataReportDescriptor)),
		USB_ENDPOINT_GENERAL_DESC(DEVICE_INT_IN_EP2,3,EP_MAX_SZE(DEVICE_FS_INT_IN_MPS),4),
		USB_ENDPOINT_GENERAL_DESC(DEVICE_INT_OUT_EP2,3,EP_MAX_SZE(DEVICE_FS_INT_OUT_MPS),4),
		#else
		HID_INTERFACE_DESC(4,0,0),
		HID_DESC(sizeof(HidDataReportDescriptor)),
		#endif
	#endif
	};
	const uint8_t InterFaceNum_Tab(FS_AUDIO_MIC)[] = {0xFF,0x00,0x01,0x02,0x03,0x04};


	//声卡 MIC输入和AUDIO输出
	const uint8_t ConfigDescriptor_Tab(FS_AUDIO_MIC_10)[] =
	{
	#if HID_DATA_FUN_EN
		//一共4个接口
		0x09,0x02,W_TOTAL_LENGTH(0xC1+USER_FS_CONFIG_DESCRIPTOR_UAC_10_SIZE+18),0x05,0x01,0x00,0x80,0x32,
	#else
		0x09,0x02,W_TOTAL_LENGTH(0xC1+USER_FS_CONFIG_DESCRIPTOR_UAC_10_SIZE),0x04,0x01,0x00,0x80,0x32,
	#endif
		//0 audio control Interface
		0x09,0x04,0x00,0x00,0x00,0x01,0x01,0x00,0x00,
		0x0A,0x24,0x01,0x00,0x01,W_TOTAL_LENGTH(0x44+PACKET_CHANNELS_NUM+MIC_CHANNELS_NUM),0x02,0x01,0x02,
		0x0C,0x24,0x02,0x01,0x01,0x01,0x00,PACKET_CHANNELS_NUM,CHANNEL_CONFIG(PACKET_CHANNELS_NUM),0x00,0x00,0x00,
		UAC1_AC_FU_DESC(0x02,0x01,PACKET_CHANNELS_NUM),
		0x09,0x24,0x03,0x03,DEVICE_TYPE(SPEAKER_TYPE),0x00,0x02,0x00,
		0x0C,0x24,0x02,0x04,DEVICE_TYPE(MIC_TYPE),0x00,MIC_CHANNELS_NUM,CHANNEL_CONFIG(MIC_CHANNELS_NUM),0x00,0x00,0x00,
		UAC1_AC_FU_DESC(0x05,0x04,MIC_CHANNELS_NUM),
		0x09,0x24,0x03,0x06,0x01,0x01,0x00,0x05,0x00,
		//1 audio stream Interface, speeaker
		0x09,0x04,0x01,0x00,0x00,0x01,0x02,0x00,0x00,

		0x09,0x04,0x01,0x01,0x01,0x01,0x02,0x00,0x00,
		0x07,0x24,0x01,0x01,0x01,0x01,0x00,
		SPEAKER_FREQ_DESCRIPTOR_UAC_10_SIZE,0x24,0x02,0x01,PACKET_CHANNELS_NUM,SPEAKER_ALT1_BITS,SPEAKER_ALT1_BITS*8,USBD_AUDIO10_SPEAKER_FREQ_NUM,
		SAMPLE_FREQ_ARRAY(USBD_AUDIO10_FREQ,USBD_AUDIO10_SPEAKER_FREQ_NUM),
		0x09, 0x05, DEVICE_ISO_OUT_EP, 0x09, AUDIO_EP_MAX_SZE(USBD_AUDIO10_FREQ,PACKET_CHANNELS_NUM,SPEAKER_ALT1_BITS), FS_ISO_INTERVAL, 0x00, 0x00,
		0x07,0x25,0x01,0x01,0x00,0x00,0x00,
	#ifdef SPEAKER_ALT2_EN
		0x09,0x04,0x01,0x02,0x01,0x01,0x02,0x00,0x00,
		0x07,0x24,0x01,0x01,0x01,0x01,0x00,
		SPEAKER_FREQ_DESCRIPTOR_UAC_10_SIZE,0x24,0x02,0x01,PACKET_CHANNELS_NUM,SPEAKER_ALT2_BITS,SPEAKER_ALT2_BITS*8,USBD_AUDIO10_SPEAKER_FREQ_NUM,
		SAMPLE_FREQ_ARRAY(USBD_AUDIO10_FREQ,USBD_AUDIO10_SPEAKER_FREQ_NUM),
		0x09, 0x05, DEVICE_ISO_OUT_EP, 0x09, AUDIO_EP_MAX_SZE(USBD_AUDIO10_FREQ,PACKET_CHANNELS_NUM,SPEAKER_ALT2_BITS), FS_ISO_INTERVAL, 0x00, 0x00,
		0x07,0x25,0x01,0x01,0x00,0x00,0x00,
	#endif
		//2 audio stream Interface, microphone
		0x09,0x04,0x02,0x00,0x00,0x01,0x02,0x00,0x00,

		0x09,0x04,0x02,0x01,0x01,0x01,0x02,0x00,0x00,
		0x07,0x24,0x01,0x06,0x01,0x01,0x00,
		MIC_FREQ_DESCRIPTOR_UAC_10_SIZE,0x24,0x02,0x01,MIC_CHANNELS_NUM,MIC_ALT1_BITS,MIC_ALT1_BITS*8,USBD_AUDIO10_MIC_FREQ_NUM,
		SAMPLE_FREQ_ARRAY(USBD_AUDIO10_MIC_FREQ,USBD_AUDIO10_MIC_FREQ_NUM),
		0x09, 0x05, DEVICE_ISO_IN_EP, 0x09, AUDIO_EP_MAX_SZE(USBD_AUDIO10_MIC_FREQ,MIC_CHANNELS_NUM,MIC_ALT1_BITS), FS_ISO_INTERVAL, 0x00, 0x00,
		0x07,0x25,0x01,0x01,0x00,0x00,0x00,
	#ifdef MIC_ALT2_EN
		0x09,0x04,0x02,0x02,0x01,0x01,0x02,0x00,0x00,
		0x07,0x24,0x01,0x06,0x01,0x01,0x00,
		MIC_FREQ_DESCRIPTOR_UAC_10_SIZE,0x24,0x02,0x01,MIC_CHANNELS_NUM,MIC_ALT2_BITS,MIC_ALT2_BITS*8,USBD_AUDIO10_MIC_FREQ_NUM,
		SAMPLE_FREQ_ARRAY(USBD_AUDIO10_MIC_FREQ,USBD_AUDIO10_MIC_FREQ_NUM),
		0x09, 0x05, DEVICE_ISO_IN_EP, 0x09, AUDIO_EP_MAX_SZE(USBD_AUDIO10_MIC_FREQ,MIC_CHANNELS_NUM,MIC_ALT2_BITS), FS_ISO_INTERVAL, 0x00, 0x00,
		0x07,0x25,0x01,0x01,0x00,0x00,0x00,
	#endif
		//3 hid 播放控制
		0x09,0x04,0x03,0x00,0x01,0x03,0x00,0x00,0x00,
		0x09,0x21,0x01,0x02,0x00,0x01,0x22,sizeof(AudioCtrlReportDescriptor),0x00,
		0x07,0x05,DEVICE_INT_IN_EP,0x03,DEVICE_FS_INT_IN_MPS,0x00,0x01,
	#if HID_DATA_FUN_EN
		0x09,0x04,0x04,0x00,0x00,0x03,0x00,0x00,0x00,
		0x09,0x21,0x01,0x02,0x00,0x01,0x22,sizeof(HidDataReportDescriptor),0x00,
	#endif
	};
	const uint8_t InterFaceNum_Tab(FS_AUDIO_MIC_10)[] = {0xFF,0x00,0x01,0x02,0x03,0x04};
#endif//#if defined(CFG_DESCRIP_AUDIO_MIC_EN) && (USB_AUDIO_PROTOCOL == AUDIO_UAC_20)


const uint8_t * const ConfigDescriptorTab[] =
{
#ifdef CFG_DESCRIP_HID_EN
	[HID] 				= ConfigDescriptor_Tab(HID),
	[FS_HID] 			= ConfigDescriptor_Tab(FS_HID),
#endif
#ifdef CFG_DESCRIP_AUDIO_ONLY_EN
	[AUDIO_ONLY] 		= ConfigDescriptor_Tab(AUDIO_ONLY),
	[AUDIO_ONLY_10] 	= ConfigDescriptor_Tab(AUDIO_ONLY_10),
#endif
#ifdef CFG_DESCRIP_AUDIO_ONLY_EN
	[FS_AUDIO_ONLY] 	= ConfigDescriptor_Tab(FS_AUDIO_ONLY),
	[FS_AUDIO_ONLY_10] 	= ConfigDescriptor_Tab(FS_AUDIO_ONLY_10),
#endif
#ifdef CFG_DESCRIP_MIC_ONLY_EN
	[MIC_ONLY] 			= ConfigDescriptor_Tab(MIC_ONLY),
	[MIC_ONLY_10] 		= ConfigDescriptor_Tab(MIC_ONLY_10),
#endif
#ifdef CFG_DESCRIP_MIC_ONLY_EN
	[FS_MIC_ONLY] 		= ConfigDescriptor_Tab(FS_MIC_ONLY),
	[FS_MIC_ONLY_10] 	= ConfigDescriptor_Tab(FS_MIC_ONLY_10),
#endif
#ifdef CFG_DESCRIP_AUDIO_MIC_EN
	[AUDIO_MIC] 		= ConfigDescriptor_Tab(AUDIO_MIC),
	[AUDIO_MIC_10] 		= ConfigDescriptor_Tab(AUDIO_MIC_10),
#endif
#ifdef CFG_DESCRIP_AUDIO_MIC_EN
	[FS_AUDIO_MIC] 		= ConfigDescriptor_Tab(FS_AUDIO_MIC),
	[FS_AUDIO_MIC_10] 	= ConfigDescriptor_Tab(FS_AUDIO_MIC_10),
#endif
#ifdef CFG_DESCRIP_AUDIO_MIC_AUDIO_EN
	[AUDIO_MIC_AUDIO] 	= ConfigDescriptor_Tab(AUDIO_MIC_AUDIO),
	[AUDIO_MIC_AUDIO_10] 	= ConfigDescriptor_Tab(AUDIO_MIC_AUDIO_10),
#endif
#ifdef CFG_DESCRIP_AUDIO_MIC_AUDIO_EN
	[FS_AUDIO_MIC_AUDIO] 	= ConfigDescriptor_Tab(FS_AUDIO_MIC_AUDIO),
	[FS_AUDIO_MIC_AUDIO_10] 	= ConfigDescriptor_Tab(FS_AUDIO_MIC_AUDIO_10),
#endif
#ifdef CFG_DESCRIP_READER_EN
	[READER] 			= ConfigDescriptor_Tab(READER),
	[FS_READER] 		= ConfigDescriptor_Tab(FS_READER),
#endif
};
const uint16_t const ConfigDescriptorLen[] =
{
#ifdef CFG_DESCRIP_HID_EN
	[HID] 				= sizeof(ConfigDescriptor_Tab(HID)) / sizeof(ConfigDescriptor_Tab(HID)[0]),
	[FS_HID] 			= sizeof(ConfigDescriptor_Tab(FS_HID)) / sizeof(ConfigDescriptor_Tab(FS_HID)[0]),
#endif
#ifdef CFG_DESCRIP_AUDIO_ONLY_EN
	[AUDIO_ONLY] 		= sizeof(ConfigDescriptor_Tab(AUDIO_ONLY)) / sizeof(ConfigDescriptor_Tab(AUDIO_ONLY)[0]),
	[AUDIO_ONLY_10] 	= sizeof(ConfigDescriptor_Tab(AUDIO_ONLY_10)) / sizeof(ConfigDescriptor_Tab(AUDIO_ONLY_10)[0]),
#endif
#ifdef CFG_DESCRIP_AUDIO_ONLY_EN
	[FS_AUDIO_ONLY] 	= sizeof(ConfigDescriptor_Tab(FS_AUDIO_ONLY)) / sizeof(ConfigDescriptor_Tab(FS_AUDIO_ONLY)[0]),
	[FS_AUDIO_ONLY_10] 	= sizeof(ConfigDescriptor_Tab(FS_AUDIO_ONLY_10)) / sizeof(ConfigDescriptor_Tab(FS_AUDIO_ONLY_10)[0]),
#endif
#ifdef CFG_DESCRIP_MIC_ONLY_EN
	[MIC_ONLY] 			= sizeof(ConfigDescriptor_Tab(MIC_ONLY)) / sizeof(ConfigDescriptor_Tab(MIC_ONLY)[0]),
	[MIC_ONLY_10] 		= sizeof(ConfigDescriptor_Tab(MIC_ONLY_10)) / sizeof(ConfigDescriptor_Tab(MIC_ONLY_10)[0]),
#endif
#ifdef CFG_DESCRIP_MIC_ONLY_EN
	[FS_MIC_ONLY] 		= sizeof(ConfigDescriptor_Tab(FS_MIC_ONLY)) / sizeof(ConfigDescriptor_Tab(FS_MIC_ONLY)[0]),
	[FS_MIC_ONLY_10] 	= sizeof(ConfigDescriptor_Tab(FS_MIC_ONLY_10)) / sizeof(ConfigDescriptor_Tab(FS_MIC_ONLY_10)[0]),
#endif
#ifdef CFG_DESCRIP_AUDIO_MIC_EN
	[AUDIO_MIC] 		= sizeof(ConfigDescriptor_Tab(AUDIO_MIC)) / sizeof(ConfigDescriptor_Tab(AUDIO_MIC)[0]),
	[AUDIO_MIC_10] 		= sizeof(ConfigDescriptor_Tab(AUDIO_MIC_10)) / sizeof(ConfigDescriptor_Tab(AUDIO_MIC_10)[0]),
#endif
#ifdef CFG_DESCRIP_AUDIO_MIC_EN
	[FS_AUDIO_MIC] 		= sizeof(ConfigDescriptor_Tab(FS_AUDIO_MIC)) / sizeof(ConfigDescriptor_Tab(FS_AUDIO_MIC)[0]),
	[FS_AUDIO_MIC_10] 	= sizeof(ConfigDescriptor_Tab(FS_AUDIO_MIC_10)) / sizeof(ConfigDescriptor_Tab(FS_AUDIO_MIC_10)[0]),
#endif
#ifdef CFG_DESCRIP_AUDIO_MIC_AUDIO_EN
	[AUDIO_MIC_AUDIO] 	= sizeof(ConfigDescriptor_Tab(AUDIO_MIC_AUDIO)) / sizeof(ConfigDescriptor_Tab(AUDIO_MIC_AUDIO)[0]),
	[AUDIO_MIC_AUDIO_10] 	= sizeof(ConfigDescriptor_Tab(AUDIO_MIC_AUDIO_10)) / sizeof(ConfigDescriptor_Tab(AUDIO_MIC_AUDIO_10)[0]),
#endif
#ifdef CFG_DESCRIP_AUDIO_MIC_AUDIO_EN
	[FS_AUDIO_MIC_AUDIO] 	= sizeof(ConfigDescriptor_Tab(FS_AUDIO_MIC_AUDIO)) / sizeof(ConfigDescriptor_Tab(FS_AUDIO_MIC_AUDIO)[0]),
	[FS_AUDIO_MIC_AUDIO_10] 	= sizeof(ConfigDescriptor_Tab(FS_AUDIO_MIC_AUDIO_10)) / sizeof(ConfigDescriptor_Tab(FS_AUDIO_MIC_AUDIO_10)[0]),
#endif
#ifdef CFG_DESCRIP_READER_EN
	[READER] 			= sizeof(ConfigDescriptor_Tab(READER)) / sizeof(ConfigDescriptor_Tab(READER)[0]),
#endif
#ifdef CFG_DESCRIP_READER_EN
	[FS_READER] 		= sizeof(ConfigDescriptor_Tab(FS_READER)) / sizeof(ConfigDescriptor_Tab(FS_READER)[0]),
#endif
};
const uint16_t AudioCtrlReportDescriptorLen = sizeof(AudioCtrlReportDescriptor);

const uint8_t * const InterFaceNumTab[] =
{
#ifdef CFG_DESCRIP_HID_EN
	[HID] 				= InterFaceNum_Tab(HID),
	[FS_HID] 			= InterFaceNum_Tab(FS_HID),
#endif
#ifdef CFG_DESCRIP_AUDIO_ONLY_EN
	[AUDIO_ONLY] 		= InterFaceNum_Tab(AUDIO_ONLY),
	[AUDIO_ONLY_10] 	= InterFaceNum_Tab(AUDIO_ONLY_10),
#endif
#ifdef CFG_DESCRIP_AUDIO_ONLY_EN
	[FS_AUDIO_ONLY] 	= InterFaceNum_Tab(FS_AUDIO_ONLY),
	[FS_AUDIO_ONLY_10] 	= InterFaceNum_Tab(FS_AUDIO_ONLY_10),
#endif
#ifdef CFG_DESCRIP_MIC_ONLY_EN
	[MIC_ONLY] 			= InterFaceNum_Tab(MIC_ONLY),
	[MIC_ONLY_10] 		= InterFaceNum_Tab(MIC_ONLY_10),
#endif
#ifdef CFG_DESCRIP_MIC_ONLY_EN
	[FS_MIC_ONLY] 		= InterFaceNum_Tab(FS_MIC_ONLY),
	[FS_MIC_ONLY_10] 	= InterFaceNum_Tab(FS_MIC_ONLY_10),
#endif
#ifdef CFG_DESCRIP_AUDIO_MIC_EN
	[AUDIO_MIC] 		= InterFaceNum_Tab(AUDIO_MIC),
	[AUDIO_MIC_10] 		= InterFaceNum_Tab(AUDIO_MIC_10),
	[FS_AUDIO_MIC] 		= InterFaceNum_Tab(FS_AUDIO_MIC),
	[FS_AUDIO_MIC_10] 	= InterFaceNum_Tab(FS_AUDIO_MIC_10),
#endif
#ifdef CFG_DESCRIP_AUDIO_MIC_AUDIO_EN
	[AUDIO_MIC_AUDIO] 	= InterFaceNum_Tab(AUDIO_MIC_AUDIO),
	[AUDIO_MIC_AUDIO_10] 	= InterFaceNum_Tab(AUDIO_MIC_AUDIO_10),
#endif
#ifdef CFG_DESCRIP_AUDIO_MIC_AUDIO_EN
	[FS_AUDIO_MIC_AUDIO] 	= InterFaceNum_Tab(FS_AUDIO_MIC_AUDIO),
	[FS_AUDIO_MIC_AUDIO_10] 	= InterFaceNum_Tab(FS_AUDIO_MIC_AUDIO_10),
#endif
#ifdef CFG_DESCRIP_READER_EN
	[READER] 			= InterFaceNum_Tab(READER),
	[FS_READER] 		= InterFaceNum_Tab(FS_READER),
#endif
};

#endif//#if defined(CFG_APP_USB_AUDIO_MODE_EN)
