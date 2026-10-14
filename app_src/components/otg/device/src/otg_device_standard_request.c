/**
 *****************************************************************************
 * @file     otg_device_standard_request.c
 * @author   Shanks
 * @version  V1.0.0
 * @date     7-September-2015
 * @brief    USB 设备标准请求协议控制层驱动
 *****************************************************************************
 * @attention
 * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
 *****************************************************************************
 * @brief
 *  1. 实现 USB 2.0 标准设备请求（Get Status/Set Address/Get Descriptor等）
 *  2. 提供控制端点0（EP0）完整状态机处理：IDLE→SETUP→DATA→STATUS
 *  3. 解析并分发标准请求、类请求、厂商请求
 *****************************************************************************
 */
//driver
#include <string.h>
#include "type.h"
#include "otg_device_hcd.h"

//application
#include "debug.h"
#include "otg_device_standard_request.h"
#include "otg_device.h"
#ifdef CFG_APP_CONFIG
	#include "app_config.h"
#endif
/**
  * @brief  初始化USB OTG设备端点0（EP0）状态机
  * @param  ep0: 指向端点0控制句柄的指针
  * @param  data_buf: 端点0数据传输缓冲区起始地址
  * @retval 无
  */
static void OTG_DeviceStateMachineInit(EP0_HandleTypeDef* ep0, uint8_t* data_buf)
{
	ep0->state = EP0_STATE_IDLE;
	ep0->data_buf = data_buf;
	ep0->total_data_len = 0;
	ep0->processed_len = 0;
	ep0->device_addr = 0;  // 默认地址0
}

/**
 * @brief  处理获取描述符命令
 * @param  NONE
 * @return NONE
 */
static void OTG_DeviceGetDescriptor(DeviceCtx_t *ctx)
{
	uint8_t*	UsbSendPtr = 0;
	uint16_t	Len = 0;
	switch(ctx->Setup[3])
	{
		case USB_DT_DEVICE:
		//	OTG_DBG("USB_DT_DEVICE\n");
			UsbSendPtr = ctx->cfg.GetDeviceDescriptor(ctx->dev_speed, &Len);
			break;

		case USB_DT_CONFIG:
		//	OTG_DBG("USB_DT_CONFIG\n");
			UsbSendPtr = ctx->cfg.GetConfigurationStrDescriptor(ctx->dev_speed, &Len);
			break;

		case USB_DT_STRING:
		//	OTG_DBG("USB_DT_STRING\n");
			{
				UsbSendPtr = ctx->cfg.GetStrDescriptor(ctx->dev_speed,ctx->Setup[2], &Len);
			}
			break;

		case USB_DT_INTERFACE:
			//PC默认不会发送该命令
			//OTG_DBG("USB_DT_INTERFACE\n");
			break;

		case USB_DT_ENDPOINT:
			//PC默认不会发送该命令
		//	OTG_DBG("USB_DT_ENDPOINT\n");
			break;
			
		case USB_DT_DEVICE_QUALIFIER:
			UsbSendPtr = ctx->cfg.GetDeviceQualifierDescriptor(ctx->dev_speed, &Len);
			break;
		case USB_DT_BOS:
			UsbSendPtr = ctx->cfg.GetBOSDescriptor(ctx->dev_speed, &Len);
			break;
		case USB_HID_REPORT:
			UsbSendPtr = ctx->cfg.GetHidReportDescriptor(ctx->dev_speed,ctx->Setup[4],&Len);
			break;

		default:
			ctx->bus_ops.StallSend(DEVICE_CONTROL_EP);
			return;
	}

	if(Len > (ctx->Setup[7] * 256 + ctx->Setup[6]))
	{

		Len = ctx->Setup[7] * 256 + ctx->Setup[6];
	}
	OTG_DeviceControlInTransfer(ctx->type,UsbSendPtr,Len);
}

#ifdef OTG_HS_TEST_MODE
extern void OTG_HS_TestMode(uint8_t mode);
static void OTG_DeviceHsTestMode(uint8_t TestSelector)
{
	#define Test_J						0x01
	#define Test_K						0x02
	#define Test_SE0_NAK				0x03
	#define Test_Packet					0x04
	#define Test_Force_Enable			0x05

	uint8_t TestPacket[53] = {	//LSB
		0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,					//JKJKJKJK * 9		 	//00000000 * 9
		0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,						//JJKKJJKK * 8		 	//01010101 * 8
		0xEE,0xEE,0xEE,0xEE,0xEE,0xEE,0xEE,0xEE,						//JJJJKKKK * 8		 	//01110111 * 8
		0xFE,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,	//JJJJJJJKKKKKKK * 8 	//0, {111111S *15}, 111111
		0x7F,0xBF,0xDF,0xEF,0xF7,0xFB,0xFD,								//JJJJJJJK * 8		 	//S, 111111S, {0111111S * 7}
		0xFC,0x7E,0xBF,0xDF,0xEF,0xF7,0xFB,0xFD,0x7E,					//{JKKKKKKK * 10}, JK	//00111111, {S0111111 * 9}, S0
	};
	printf("OTG_HS_TestMode %d\n",TestSelector);
	switch(TestSelector)
	{
	case Test_Packet:
		OTG_HS_DeviceControlSend(TestPacket,sizeof(TestPacket),0);
	case Test_J:
	case Test_K:
	case Test_SE0_NAK:
		OTG_HS_TestMode(TestSelector);
		break;
	default:
		OTG_HS_DeviceStallSend(DEVICE_CONTROL_EP);
		break;
	}
}
#endif

static void OTG_DeviceStandardRequest(DeviceCtx_t *ctx)
{
	//OTG_DBG("\nSetup[1] = %d\n\n", Setup[1]);
	switch(ctx->Setup[1])
	{
		case USB_REQ_GET_STATUS:
			//OTG_DBG("GetStatus\n");
			//用于获取USB设备接口端点的状态
			ctx->Resp[0] = 0x00;
			ctx->Resp[1] = 0x00;
			OTG_DeviceControlInTransfer(ctx->type,(uint8_t*)&ctx->Resp,2);
			break;

		case USB_REQ_CLEAR_FEATURE:
			//OTG_DBG("ClearFeature\n");
			//用于清除或者禁止USB设备00,接口01,端点02,的某些特性，无数据传输
			break;

		case USB_REQ_SET_FEATURE:
		//	OTG_DBG("SetFeature\n");
			//用于设置或者使USB设备00,接口01,端点02,的某些特性，无数据传输
#ifdef OTG_HS_TEST_MODE
			if(ctx->dev_speed == USB_SPEED_HIGH && ctx->Setup[2] == 2)	//FeatureSelector TEST_MODE
			{
				OTG_DeviceHsTestMode(ctx->Setup[5]); //TestSelector
			}
			else
#endif
			ctx->bus_ops.StallSend(ctx->Setup[4]);
			break;

		case USB_REQ_SET_ADDRESS:
		//	OTG_DBG("SetAddress\n");
			ctx->state = DEV_STATE_ADDRESSED;
			ctx->bus_ops.AddressSet(ctx->Setup[2] & 0x7F);
			break;

		case USB_REQ_GET_DESCRIPTOR:
		//	OTG_DBG("GetDescriptor\n");
			OTG_DeviceGetDescriptor(ctx);
			break;
		
		case USB_REQ_SET_DESCRIPTOR:
		//	OTG_DBG("GetDescriptor111\n");
			break;

		case USB_REQ_GET_CONFIGURATION:
		//	OTG_DBG("GetConfiguration\n");
			ctx->Resp[0] = 0x01;
			OTG_DeviceControlInTransfer(ctx->type,(uint8_t*)&ctx->Resp,1);
			break;

		case USB_REQ_SET_CONFIGURATION:
			{
				ctx->state = DEV_STATE_CONFIGURED;
				ctx->cfg.SoftwareInit();
			}
			break;

		case USB_REQ_GET_INTERFACE:
			OTG_DeviceControlInTransfer(ctx->type,(uint8_t*)&ctx->Resp,1);
			break;

		case USB_REQ_SET_INTERFACE:
			ctx->cfg.SoftwareSetInterest(ctx->Setup[4],ctx->Setup[2]);
			break;

		case USB_REQ_SYNCH_FRAME:
			//OTG_DBG("SYNC FRAME\n");
			break;

		default:
			ctx->bus_ops.StallSend(DEVICE_CONTROL_EP);
			break;
	}
}
/**
 * @brief  处理PC发来的控制命令
 * @param  NONE
 * @return NONE
 */
static void OTG_DeviceProcessSetupPacket(DeviceCtx_t *ctx)
{
	uint8_t ReqType;
	ReqType = (ctx->Setup[0]&0x60)>>5;
	//DBG("ReqType:%d\n",ReqType);
	switch(ReqType)
	{
		case 0:
			//标准请求
			OTG_DeviceStandardRequest(ctx);
			break;

		case 1:
		case 2:
		case 3:
			ctx->cfg.SoftwareClassRequest(ctx);
			break;
	}
}

/**
  * @brief  USB OTG 设备控制端点0（EP0）状态机主处理函数
  * @note   该函数为USB设备控制传输核心处理逻辑，需周期性调用，实现USB标准控制传输流程
  *         包含：SETUP阶段 -> DATA IN/OUT阶段 -> STATUS阶段 -> 回到IDLE的完整状态流转
  * @param  ctx: 指向USB设备上下文结构体指针，包含端点0、总线操作接口、配置等信息
  * @retval 无
  */

static void OTG_DeviceStateMachine(DeviceCtx_t *ctx)
{
	uint32_t DataLeng;
	EP0_HandleTypeDef *ep0 = &ctx->ep0;
    switch (ep0->state)
    {
        case EP0_STATE_IDLE:
        	if(ctx->bus_ops.SetupReceive((uint8_t*)&ep0->setup_pkt, 8, &DataLeng) != DEVICE_NONE_ERR)
        	{
        		return;
        	}
        	else if(DataLeng != 8)
        	{
        //		DBG("Setup Packet error!\n");
        		return;
        	}
        	memcpy(ctx->Setup,&ep0->setup_pkt,8);		//TODO 应用层很多接口用的Setup数组
        	ep0->state = EP0_STATE_SETUP;

        case EP0_STATE_SETUP:
            //处理Setup包，决定下一步状态
        	ep0->state = EP0_STATE_IDLE;  		//零长度数据SETUP包状态
        	if((ep0->setup_pkt.bmRequestType&0x80) == 0)
        	{
        		if(ep0->setup_pkt.wLength)	 	//如果是OUT 且需要接收数据，则先接收数据，后面再处理指令
        		{
					memset(ctx->Request,0,sizeof(ctx->Request));
        			OTG_DeviceControlOutTransfer(ctx->type,ctx->Request,ep0->setup_pkt.wLength);
        			ep0->state = EP0_STATE_DATA_OUT;
        			break;
        		}
        	}
        	else
        	{
        		if(ep0->setup_pkt.wLength)	 	//如果是IN 防止处理流程内没有对数据进行处理
        		{
					memset(ctx->Resp,0,sizeof(ctx->Resp));
        			OTG_DeviceControlInTransfer(ctx->type,ctx->Resp,ep0->setup_pkt.wLength);
        			ep0->state = EP0_STATE_DATA_IN;
        		}
        	}
        	OTG_DeviceProcessSetupPacket(ctx);
            break;
        case EP0_STATE_DATA_OUT:
			// 数据OUT阶段：主机向设备发送数据
			if (ep0->processed_len < ep0->total_data_len)
			{
				uint32_t recv_len;  // 实际接收长度
				// 计算最大接收长度（不超过端点最大包长）
//				uint32_t max_recv_len = (ep0->total_data_len - ep0->processed_len) > 64 ? 64 : (ep0->total_data_len - ep0->processed_len);
//				// 调用OTG接收函数
//				int32_t ret = ctx->bus_ops.ControlReceive(&ep0->data_buf[ep0->processed_len],max_recv_len,&recv_len,0);

				uint32_t max_recv_len = (ep0->total_data_len - ep0->processed_len);
				// 调用OTG接收函数
				int32_t ret = ctx->bus_ops.ControlReceive(&ep0->data_buf[ep0->processed_len],max_recv_len,&recv_len,1000);
				if (ret == 0) {  // 接收成功
					ep0->processed_len += recv_len;
					// 数据接收完毕（达到总长度或主机发送零长度包）
					if (ep0->processed_len >= ep0->total_data_len || recv_len == 0) {
						ep0->state = EP0_STATE_STATUS_IN;  // 进入状态IN阶段
					}
				} else {  // 接收失败，暂停端点
					ctx->bus_ops.StallSend(DEVICE_CONTROL_EP);
					ep0->state = EP0_STATE_STALL;
				}
			}
			else
			{
				// 数据长度为0，直接进入状态IN
				ep0->state = EP0_STATE_STATUS_IN;
			}
        	break;
        case EP0_STATE_STATUS_IN:
            ep0->state = EP0_STATE_IDLE;  		// 回到空闲状态
        	OTG_DeviceProcessSetupPacket(ctx);		//收到数据后再处理指令
            break;

        case EP0_STATE_STATUS_OUT:
             ep0->state = EP0_STATE_IDLE;  		// 回到空闲状态
            break;

        case EP0_STATE_STALL:
            break;
        case EP0_STATE_DATA_IN:					//消除警告
            break;
    }
    if(ep0->state == EP0_STATE_DATA_IN)
    {
		// 数据IN阶段：设备向主机发送数据
//		if (ep0->processed_len < ep0->total_data_len) {
//			// 计算本次发送长度（不超过端点最大包长，假设64字节）
//			uint32_t send_len = (ep0->total_data_len - ep0->processed_len) > 64 ? 64 : (ep0->total_data_len - ep0->processed_len);
//			// 调用OTG发送函数（从已处理位置的下一段开始发送）
//			int32_t ret = ctx->bus_ops.ControlSend(&ep0->data_buf[ep0->processed_len], send_len, 0);
//			if (ret == 0) {  // 发送成功
//				ep0->processed_len += send_len;
//				// 所有数据发送完毕，进入状态OUT阶段（主机发送确认）
//				if (ep0->processed_len >= ep0->total_data_len) {
//					ep0->state = EP0_STATE_IDLE;
//				}
//			}
//		}
		if (ep0->processed_len < ep0->total_data_len) {
			// 计算本次发送长度（不超过端点最大包长，假设64字节）
			uint32_t send_len = ep0->total_data_len - ep0->processed_len;
			// 调用OTG发送函数（从已处理位置的下一段开始发送）
			int32_t ret = ctx->bus_ops.ControlSend(&ep0->data_buf[ep0->processed_len], send_len, 1000);
			if (ret == 0) {  // 发送成功
				ep0->processed_len += send_len;
				// 所有数据发送完毕，进入状态OUT阶段（主机发送确认）
				if (ep0->processed_len >= ep0->total_data_len) {
					ep0->state = EP0_STATE_IDLE;
				}
			}
		}
		else
		{
			// 数据长度为0（无数据阶段），直接进入状态OUT
			ep0->state = EP0_STATE_IDLE;
		}
    }
}

/**
  * @brief  USB OTG 设备请求处理总入口函数
  * @note   该函数为USB设备主处理流程，需周期性调用，处理总线复位、状态机调度等核心事务
  *         1. 检测USB总线复位事件
  *         2. 复位时初始化端点0状态机
  *         3. 设置设备速度（全速/高速）
  *         4. 调用设备控制端点0状态机
  * @param  dev: USB设备类型（全速/高速）
  * @retval 无
  */
void OTG_DeviceRequestProcess(USB_DeviceType_t dev)
{
	DeviceCtx_t *ctx = Device_GetCtx(dev);

	uint8_t BusEvent = ctx->bus_ops.DeviceBusEventGet();
	if(BusEvent & 0x01)
	{
		ctx->state = DEV_STATE_SUSPEND;
		if(ctx->cfg.SoftwareSuspend)
		{
			ctx->cfg.SoftwareSuspend(ctx->dev_speed);
		}
	}
	if(BusEvent & 0x02)
	{
		ctx->state = DEV_STATE_RESUME;
		if(ctx->cfg.SoftwareResume)
		{
			ctx->cfg.SoftwareResume(ctx->dev_speed);
		}
	}
	if(BusEvent & 0x04)
	{
		OTG_DeviceStateMachineInit(&ctx->ep0,ctx->Resp);
		ctx->dev_speed = USB_SPEED_FULL;
		if(dev == USB_DEVICE_HS)
		{
			ctx->dev_speed = ctx->bus_ops.DevicePortSpeedGet();
		}
		ctx->state = DEV_STATE_RESET;
		if(ctx->cfg.SoftwareReset)
		{
			ctx->cfg.SoftwareReset(ctx->dev_speed);
		}
	}
	OTG_DeviceStateMachine(ctx);
}
