/**
 *****************************************************************************
 * @file     otg_device_custom_hid_request.c
 * @author   Shanks
 * @version  V1.0.0
 * @date     7-September-2015
 * @brief    USB 自定义 HID 设备控制层驱动
 *****************************************************************************
 * @attention
 * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
 *****************************************************************************
 * @brief  文件功能说明：
 *  1. 实现 USB 自定义 HID 类设备数据收发控制
 *  2. 处理 HID 标准请求：SetReport / GetReport / 中断传输
 *  3. 支持上位机指令解析（参数调参、CRC 校验、固件升级）
 *  4. 支持 USB 虚拟 DEBUG 日志回传
 *  5. 支持 AT 指令透传传输
 *  6. 实现 CODE 固件在线升级触发逻辑
 *  7. 对接上层应用：HID 通信、Flash 参数、系统模式切换
 *****************************************************************************
 */
//driver
#include <string.h>
#include "otg_device_hcd.h"
//application
#include "app_config.h"
#include "otg_device.h"
#include "debug.h"

#ifdef CFG_FUNC_FLASH_PARAM_ONLINE_TUNING_EN
#include "pc_online_param.h"
#endif
#if FLASH_BOOT_EN
#include "mode_task.h"
extern void start_up_grate(uint32_t UpdateResource);
#endif
extern void HIDUsb_Rx(uint8_t *buf,uint16_t len);

uint8_t hid_tx_buf[256];
uint32_t pc_upgrade = 0;
#ifdef CFG_FUNC_USBDEBUG_EN
	extern MCU_CIRCULAR_CONTEXT usb_fifo;
	const uint8_t usbdebug_head[] = {0xA5,0x5A,0x30,0x01};
#endif
#ifdef MVAPP_UPDATA_EN
extern void upgrade_process_data(const uint8_t *buf, uint16_t len);
#endif
/**
 * @brief  接收并处理来自主机的 HID 数据
 *         解析帧头/控制字，分发至各功能模块（在线调参、USB 命令、固件升级、AT 指令透传）
 * @param  Request  接收到的 HID 数据缓冲区指针（长度 256 字节）
 * @return 无
 */
void hid_recive_data(uint8_t *Request)
{
#if defined(CFG_COMMUNICATION_BY_HS_USB)||defined(CFG_COMMUNICATION_BY_FS_USB)
	HIDUsb_Rx(Request,256);
#endif

#ifdef CFG_FUNC_FLASH_PARAM_ONLINE_TUNING_EN
	if((Request[0] == 0xA5) && (Request[1] == 0x5A))	//帧头
	{
		switch(Request[2]) //控制字
		{
			case PC_PARAM_CMD:
				FlashSn_Rx(Request+3,256-3);
				break;
			// case PC_CRC_CMD:
			// 	FlashCRC_Rx(Request+3,256-3);
			// 	break;
			case PC_USB_CMD:
				FlashParamUsb_Rx(Request+3,256-3);
				break;
			case PC_UPGRADE_CMD:
			#ifdef MVAPP_UPDATA_EN
				upgrade_process_data(Request,256);
			#endif
				break;
			default:
				break;
		}
	}
#endif
#if BT_LE_AUDIO_SUPPORT
	if((Request[0] == 'A') && (Request[1] == 'T') && (Request[2] == '#'))
	{
		printf("HID_RECV: AT CMD\n");
		extern void hidParseHostCmd(uint8_t *buf, uint16_t len);
		hidParseHostCmd(Request+3,256-3);
	}
#endif
}

/**
 * @brief  自定义 HID 类设备标准请求与类请求处理入口
 *         处理 SetReport(Out)、GetReport/中断传输(In)、SetReport(Feature)、GetReport(Feature)
 *         包含 USB 调试日志回传、CODE 固件升级触发逻辑
 * @param  ctx  USB 设备上下文指针（含 Setup 包、数据缓冲区）
 * @return 无
 */
void OTG_DeviceCustomHidRequest(DeviceCtx_t *ctx)
{
	uint8_t *Setup = ctx->Setup;
	uint8_t *Request = ctx->Request;
	if((Setup[3] == 0x02)&&(Setup[0] == 0x21))//out
	{
		hid_recive_data(Request);
	}
	else if((Setup[3] == 0x01)&&(Setup[0] == 0xA1))//int
	{
#ifdef CFG_FUNC_USBDEBUG_EN
		extern bool hid_tx_buf_is_used;
		if(!hid_tx_buf_is_used)
		{
			memcpy(hid_tx_buf,usbdebug_head,sizeof(usbdebug_head));
			uint32_t datalen = MCUCircular_GetData(&usb_fifo,&hid_tx_buf[4],252);
			if(datalen < 252)
			{
				memset(&hid_tx_buf[4]+datalen,0,252-datalen);
			}
		}
		hid_tx_buf_is_used = 0;
#endif
		memcpy(ctx->Resp, hid_tx_buf, 256);
		memset(hid_tx_buf, 0, 256);
		OTG_DeviceControlInTransfer(ctx->type, ctx->Resp, 256);
	}
#if FLASH_BOOT_EN
	else if((Setup[3] == 0x03)&&(Setup[0] == 0xA1))//GetReport (Feature Report)
	{
	#ifdef CFG_FUNC_FLASH_PARAM_ONLINE_TUNING_EN
		if(Request[3] == 0x01 &&  Request[0] == 0xA1)
		{
			extern bool FlashParamUsb_Tx(DeviceCtx_t *ctx);

			if(!FlashParamUsb_Tx(ctx))
			OTG_DeviceControlInTransfer(ctx->type,hid_tx_buf,256);
		}
		else
	#endif
		{
//			DBG("pc_upgrade start 1\n");
			if(pc_upgrade)
			{
//				DBG("pc_upgrade start 2\n");
				Setup[0] = 0x55;
				OTG_DeviceControlInTransfer(ctx->type,Setup,Setup[7]*256+Setup[6]);
				if(ctx->type == USB_DEVICE_HS)
				{
					start_up_grate(SysResourceUsbDevice);		//TODO 应该等待usb通讯完毕再启动升级流程
				}
				else
				{
					start_up_grate(SysResourceUsbDeviceFs);		//TODO 应该等待usb通讯完毕再启动升级流程
				}

			}
			else
			{
				Setup[0] = 0;
				OTG_DeviceControlInTransfer(ctx->type,Setup,Setup[7]*256+Setup[6]);
			}
		}
	}
	else if((Setup[3] == 0x03)&&(Setup[0] == 0x21))//SetReport (Feature Report)
	{
		pc_upgrade = 0;
//		DBG("pc_upgrade start 0\n");
		if(Request[0] == 0x55)//有CODE升级
		{
			//uint8_t *p = (uint8_t *)(0x10000 + 0xB8);
			//p = Request+1;// bkd // 2019.5.7
			if(memcmp((uint8_t*)(0x10000 + 0xB8),Request + 1,4) != 0)//不需要升级了
			{
				pc_upgrade = 1;
			}
		}
		else if(Request[0] == 0xAA)//无code升级，无条件升级
		{
			pc_upgrade = 1;
		}
	}
#endif
	else
	{
	   // OTG_DBG("Others Cmd\n");
	}
}

#if defined(CFG_FUNC_USBDEBUG_EN) && defined(CFG_COMMUNICATION_BY_FS_USB)
/**
 * @brief 通过FS HID中断IN端点发送一包日志，供WebHID/通用HID工具接收。
 *        原256字节控制GetReport路径继续保留，二者使用相同包头。
 * @note  仅在FIFO有数据时尝试一次，端点未就绪时不丢弃FIFO数据。
 */
void UsbHidLogPoll(void)
{
	static uint8_t log_packet[DEVICE_FS_INT_IN_MPS];
	uint32_t data_len;
	extern bool GetUSBFsDeviceInitState(void);

	if(!GetUSBFsDeviceInitState() || (MCUCircular_GetDataLen(&usb_fifo) == 0))
	{
		return;
	}

	memcpy(log_packet, usbdebug_head, sizeof(usbdebug_head));
	data_len = MCUCircular_ReadData(&usb_fifo,
								&log_packet[sizeof(usbdebug_head)],
								DEVICE_FS_INT_IN_MPS - sizeof(usbdebug_head));
	if(data_len == 0)
	{
		return;
	}
	if(data_len < (DEVICE_FS_INT_IN_MPS - sizeof(usbdebug_head)))
	{
		memset(&log_packet[sizeof(usbdebug_head) + data_len], 0,
			   DEVICE_FS_INT_IN_MPS - sizeof(usbdebug_head) - data_len);
	}

	if(OTG_FS_DeviceInterruptSend(DEVICE_INT_IN_EP2, log_packet,
								 DEVICE_FS_INT_IN_MPS, 1) == DEVICE_NONE_ERR)
	{
		MCUCircular_AbortData(&usb_fifo, data_len);
	}
}
#endif

