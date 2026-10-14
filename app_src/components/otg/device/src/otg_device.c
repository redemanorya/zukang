/**
 *****************************************************************************
 * @file     otg_device.c
 * @author   Shanks
 * @version  V1.0.0
 * @date     7-September-2015
 * @brief    USB 设备框架核心层
 *****************************************************************************
 * @attention
 * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
 *****************************************************************************
 * @brief
 *  1. 管理 USB 设备上下文（多设备实例支持）
 *  2. 提供控制端点 IN/OUT 传输通用接口
 *  3. 自动绑定 HS/FS 底层总线操作函数
 *  4. 提供设备类数据注册接口
 *****************************************************************************
 */
//driver
#include <string.h>
#include <math.h>
#include "otg_device_hcd.h"
#include "irqn.h"
#include "chip_interrupt.h"
//application
#include "app_config.h"
#include "otg_device_audio.h"
#include "otg_device_descriptor.h"
#include "otg_device_audio10_request.h"
#include "otg_device_audio20_request.h"
#include "otg_device_stor_request.h"
#include "otg_device.h"
#include "debug.h"

//内部管理资源
static DeviceCtx_t s_device_inst[USB_DEVICE_MAX];

/**
 * @brief  内部函数：获取设备上下文
 * @param  dev_type 对外设备类型
 * @return 上下文指针（NULL表示无效）
 */
DeviceCtx_t* Device_GetCtx(USB_DeviceType_t dev_type)
{
    if (dev_type >= USB_DEVICE_MAX) {
        OTG_DBG("无效的设备类型：%d\n", dev_type);
        return NULL;
    }
    return &s_device_inst[dev_type];
}

/**
 * @brief 准备IN传输（设备→主机）：填充数据缓冲区并切换到DATA_IN状态
 * @param ep0：端点0控制结构体指针
 * @param data_buf：待发送数据的缓冲区指针
 * @param total_data_len：待发送的总数据长度（≤ Setup包的wLength）
 * @return 0：成功；-1：参数无效
 */
int32_t OTG_DeviceControlInTransfer(USB_DeviceType_t dev_type,uint8_t* data_buf, uint32_t total_data_len)
{
    if (data_buf == NULL || total_data_len == 0) {
        return -1; // 无效参数（缓冲区为空或长度为0）
    }
	DeviceCtx_t *ctx = Device_GetCtx(dev_type);

    // 初始化IN传输参数
	ctx->ep0.data_buf = data_buf;         // 绑定发送缓冲区
	ctx->ep0.total_data_len = total_data_len; // 记录总发送长度
	ctx->ep0.processed_len = 0;           // 重置已发送长度
    return 0;
}
/**
 * @brief 准备OUT传输（主机→设备）：绑定接收缓冲区并切换到DATA_OUT状态
 * @param ep0：端点0控制结构体指针
 * @param data_buf：接收数据的缓冲区指针
 * @param total_data_len：预期接收的总数据长度（通常等于Setup包的wLength）
 * @return 0：成功；-1：参数无效
 */
int32_t OTG_DeviceControlOutTransfer(USB_DeviceType_t dev_type,uint8_t* data_buf, uint32_t total_data_len)
{
	if (data_buf == NULL || total_data_len == 0) {
        return -1; // 无效参数（缓冲区为空）
    }
	DeviceCtx_t *ctx = Device_GetCtx(dev_type);

    // 初始化OUT传输参数
	ctx->ep0.data_buf = data_buf;         // 绑定接收缓冲区
	ctx->ep0.total_data_len = total_data_len; // 记录总接收长度
	ctx->ep0.processed_len = 0;           // 重置已接收长度
    return 0;
}

/**
  * @brief  USB OTG 设备上下文初始化函数
  * @param  dev_type: USB设备类型（USB_DEVICE_HS/USB_DEVICE_FS）
  * @param  DriverCfg: 驱动配置结构体指针，不能为空
  * @param  bus_ops: 总线操作接口指针，可为NULL（使用默认接口）
  * @retval TRUE: 初始化成功，FALSE: 初始化失败
  */
bool OTG_DeviceCtxInit(USB_DeviceType_t dev_type,USB_DevConfig_t *DriverCfg,USB_DriverOps_t *bus_ops)
{
	DeviceCtx_t *ctx = Device_GetCtx(dev_type);
    if (!ctx) {
        return FALSE;
    }
    // 清空上下文
    memset(ctx, 0, sizeof(DeviceCtx_t));
    // 绑定核心资源
    ctx->type = dev_type;
    if(DriverCfg)
    {
        memcpy(&ctx->cfg, DriverCfg, sizeof(USB_DevConfig_t));
    }
    else
    {
    	return FALSE;
    }

    if(bus_ops)
    {
        memcpy(&ctx->bus_ops, bus_ops, sizeof(USB_DriverOps_t));
    }
    else
    {
    	if(ctx->type == USB_DEVICE_HS)
        {
    		ctx->bus_ops.SetupReceive = OTG_HS_DeviceSetupReceive;
    		ctx->bus_ops.ControlReceive = OTG_HS_DeviceControlReceive;
    		ctx->bus_ops.ControlSend = OTG_HS_DeviceControlSend;
    		ctx->bus_ops.DeviceBusEventGet = OTG_HS_DeviceBusEventGet;
    		ctx->bus_ops.DevicePortSpeedGet = OTG_HS_DevicePortSpeedGet;
    		ctx->bus_ops.StallSend = OTG_HS_DeviceStallSend;
    		ctx->bus_ops.AddressSet = OTG_HS_DeviceAddressSet;
    		ctx->bus_ops.BulkReceive = OTG_HS_DeviceBulkReceive;
    		ctx->bus_ops.BulkSend = OTG_HS_DeviceBulkSend;
        }
    	else if(ctx->type == USB_DEVICE_FS)
        {
    		ctx->bus_ops.SetupReceive = OTG_FS_DeviceSetupReceive;
    		ctx->bus_ops.ControlReceive = OTG_FS_DeviceControlReceive;
    		ctx->bus_ops.ControlSend = OTG_FS_DeviceControlSend;
    		ctx->bus_ops.DeviceBusEventGet = OTG_FS_DeviceBusEventGet;
    		ctx->bus_ops.StallSend = OTG_FS_DeviceStallSend;
    		ctx->bus_ops.AddressSet = OTG_FS_DeviceAddressSet;
    		ctx->bus_ops.BulkReceive = OTG_FS_DeviceBulkReceive;
    		ctx->bus_ops.BulkSend = OTG_FS_DeviceBulkSend;
        }
    }
    return TRUE;
}

/**
 * @brief 注册USB类数据到设备上下文
 * @param dev_type 设备类
 * @param class_data 类数据指针（不能为空）
 * @retval TRUE 成功
 * @retval FALSE 失败（参数无效/类数据类型错误）
 */
bool OTG_DeviceRegisterClassData(USB_DeviceType_t dev_type, void *class_data)
{
    if (class_data == NULL) {
        return FALSE;
    }
	DeviceCtx_t *ctx = Device_GetCtx(dev_type);
    if (!ctx) {
        return FALSE;
    }
    ctx->pClassData = class_data;
    return TRUE;
}



void OTG_DeviceAudioModeSel(USB_DeviceType_t dev_type,uint8_t Mode,uint16_t UsbVid,uint16_t UsbPid)
{
	if(dev_type == USB_DEVICE_HS)
	{
		OTG_HS_DeviceAudioModeSel(Mode,UsbVid,UsbPid);
	}
	else
	{
		OTG_FS_DeviceAudioModeSel(Mode,UsbVid,UsbPid);
	}
}

//usb device 模式硬件相关初始化
void OTG_DeviceHardwareInit(USB_DeviceType_t dev_type,uint8_t Mode,uint16_t UsbVid,uint16_t UsbPid)
{
	APP_DBG("OTG_DeviceHardwareInit %d\n",dev_type);
	if(dev_type == USB_DEVICE_HS)
	{
		OTG_HS_DeviceAudioModeSel(Mode, UsbVid, UsbPid);
		OTG_HS_DeviceFifoInit();
		OTG_HS_DeviceInit();
		NVIC_EnableIRQ(USB_HS_IRQn);
	}
	else if(dev_type == USB_DEVICE_FS)
	{
		OTG_FS_DeviceAudioModeSel(Mode, UsbVid, UsbPid);
		OTG_FS_DeviceFifoInit();
		OTG_FS_DeviceInit();
		CoreInterruptEnable(INT_USB_FS);
		NVIC_EnableIRQ(USB_FS_WAKEUP_IRQn);
	}
}

void UsbHidHardwareint(void)
{
#if defined(CFG_COMMUNICATION_BY_HS_USB) && defined(CHIP_PIN_HSOTG) && !((CFG_PARA_HOST_HS_DET || CFG_PARA_DEVICE_HS_DET)) \
	&& !defined(CFG_APP_USB_HS_PLAY_MODE_EN) && !defined(CFG_RES_USB_HS_MIX_AUDIO_EN)
	OTG_DeviceHardwareInit(USB_DEVICE_HS, HID, USB_VID, USBPID(HID));
#endif
#if defined(CFG_COMMUNICATION_BY_FS_USB) &&  !((CFG_PARA_HOST_FS_DET || CFG_PARA_DEVICE_FS_DET)) \
	&& !defined(CFG_APP_USB_FS_AUDIO_MODE_EN) && !defined(CFG_RES_USB_FS_MIX_AUDIO_EN)
	OTG_DeviceHardwareInit(USB_DEVICE_FS, HID, USB_VID, USBPID(HID));
#endif
}

/**
 * @brief  将 ASCII 字符串转换为 USB Unicode 字符串描述符格式
 * @param ascii    ASCII 字符串输入（以 '\0' 结尾，最多 127 字符）
 * @param unicode  Unicode 字符串描述符输出缓冲区
 *                 [0] = 总长度（字节），[1] = 0x03（USB 字符串描述符类型），
 *                 后续每字符占 2 字节（ASCII 码 + 0x00 填充）
 * @param len      输出 Unicode 字符串的总字节数
 */
void asciiToUnicode(uint8_t *ascii, uint8_t *unicode, uint16_t *len)
{
	uint8_t idx = 0U;
	if (ascii == NULL || unicode == NULL || len == NULL)
	{
		return;
	}
	for(idx = 0; idx < 127; idx++)
	{
		if(ascii[idx] == '\0')
		{
			break;
		}
		unicode[2 + idx * 2 + 0] = ascii[idx];
		unicode[2 + idx * 2 + 1] = 0x00;
	}

	idx = idx * 2 + 2;
	unicode[0] = idx;
	unicode[1] = 0x03;
	*len = idx;
}

