 /**
 *****************************************************************************
 * @file     otg_host_stor_request.c
 * @author   owen
 * @version  V1.0.0
 * @date     7-September-2015
 * @brief    otg host standard enum driver interface
 *****************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2015 MVSilicon </center></h2>
 */

#include <string.h>

#include "type.h"
#include "debug.h"
#include "delay.h"
#include "app_config.h"
//#include "otg_hal.h"
#include "otg_host_hcd.h"
#include "otg_host_standard_enum.h"
#include "otg_host_udisk.h"
#ifdef FUNC_OS_EN
#include "rtos_api.h"
#endif
extern int printf(const char *fmt, ...);
#undef  OTG_DBG
#define	OTG_DBG(format, ...)		printf(format, ##__VA_ARGS__)  //bkd mark

//内部管理资源
static HostCtx_t s_host_inst[HOST_DEV_MAX];

/**
 * @brief  内部函数：获取设备上下文
 * @param  dev_type 对外设备类型
 * @return 上下文指针（NULL表示无效）
 */
static HostCtx_t* Host_GetCtx(HOST_DEV_TYPE dev_type)
{
    if (dev_type >= HOST_DEV_MAX) {
        OTG_DBG("无效的设备类型：%d\n", dev_type);
        return NULL;
    }
    return &s_host_inst[dev_type];
}

static bool OTG_HostGetDescriptor(HostCtx_t *ctx,uint8_t Type, uint8_t Index, uint8_t* Buf, uint16_t Size)
{
	USB_CTRL_SETUP_REQUEST SetupPacket;	
	uint8_t CmdGetDescriptor[8] = {0x80, 0x06, 0x00, 0x01, 0x00, 0x00, 0x08, 0x00};
	CmdGetDescriptor[2] = Index;
	CmdGetDescriptor[3] = Type;
	CmdGetDescriptor[6] = Size;
	CmdGetDescriptor[7] = Size>>8;

	if(Type == USB_DT_STRING)
	{
		CmdGetDescriptor[4] = 0x09;
		CmdGetDescriptor[5] = 0x04;
	}
	memcpy((uint8_t *)&SetupPacket,CmdGetDescriptor,8);
	if(ctx->bus_ops.HostControlRead(SetupPacket,Buf,Size,1000) == HOST_NONE_ERR)
	{
		return TRUE;
	}
	return FALSE;
}

static bool OTG_HostSetAddress(HostCtx_t *ctx,uint8_t Address)
{
	USB_CTRL_SETUP_REQUEST SetupPacket;		
	uint8_t CmdSetAddress[8] = {0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};

	CmdSetAddress[2] = Address;
	memcpy((uint8_t *)&SetupPacket,CmdSetAddress,8);
	
	if(ctx->bus_ops.HostControlWrite(SetupPacket, NULL, 0,1000) != HOST_NONE_ERR)
	{
#ifdef FUNC_OS_EN
		osTaskDelay(20);
#else
		WaitMs(20);
#endif
		if(ctx->bus_ops.HostControlWrite(SetupPacket, NULL, 0,1000) != HOST_NONE_ERR)
		{
#ifdef FUNC_OS_EN
			osTaskDelay(20);
#else
			WaitMs(20);
#endif
			if(ctx->bus_ops.HostControlWrite(SetupPacket, NULL, 0,1000) != HOST_NONE_ERR)
			{
				return FALSE;
			}
		}
	}
	
	ctx->bus_ops.HostAddressSet(Address);
#ifdef FUNC_OS_EN
	osTaskDelay(20);
#else
	WaitMs(20);//等待Device将ADDRESS设置OK
#endif
	return TRUE;
}

/**
 * @brief  设置配置
 * @param  ConfigurationNum 配置号
 * @return 1-成功，0-失败
 */
static bool OTG_HostSetConfiguration(HostCtx_t *ctx,uint8_t ConfigurationNum)
{
	USB_CTRL_SETUP_REQUEST SetupPacket;	
	uint8_t CmdSetConfig[8] = {0x00, 0x09, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};
	CmdSetConfig[2] = ConfigurationNum;
	memcpy((uint8_t *)&SetupPacket,CmdSetConfig,8);

	if(ctx->bus_ops.HostControlWrite(SetupPacket, NULL, 0, 1000) != HOST_NONE_ERR)
	{
		return FALSE;
	}
	return TRUE;
}
#if 0
/**
 * @brief  设置接口
 * @param  InterfaceNum 接口号
 * @return 1-成功，0-失败
 */
static bool OTG_HostSetInterface(HostCtx_t *ctx,uint8_t InterfaceNum)
{
	USB_CTRL_SETUP_REQUEST SetupPacket;	
	uint8_t CmdSetInterface[8] = {0x01, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	CmdSetInterface[4] = InterfaceNum;
	memcpy((uint8_t *)&SetupPacket,CmdSetInterface,8);

	if(ctx->bus_ops.HostControlWrite(SetupPacket, NULL, 0, 1000) != HOST_NONE_ERR)
	{
		return FALSE;
	}
	return TRUE;
}
#endif
/**
 * @brief  设置Ipod充电接口
 * @param  Current 充电电流   0~500ma
 * @return 1-成功，0-失败
 */
static bool OTG_HostSetIpodChargeCurrent(HostCtx_t *ctx,int16_t Current)
{
	USB_CTRL_SETUP_REQUEST SetupPacket;
	uint8_t CmdSetCurrent[8] = {0x40, 0x40, 0x00, 0x00, 0xf4, 0x01, 0x00, 0x00};
	CmdSetCurrent[4] = (uint8_t)(Current);
	CmdSetCurrent[5] = (uint8_t)(Current >> 8);
	memcpy((uint8_t *)&SetupPacket,CmdSetCurrent,8);
	if(ctx->bus_ops.HostControlWrite(SetupPacket, NULL, 0, 1000) != HOST_NONE_ERR)
	{
		return FALSE;
	}
	return TRUE;
}

/**
 * @brief  host枚举初始化
 * @param  NONE
 * @return 1-枚举成功，0-枚举失败
 */
static bool OTG_HostEnumDevice(HostCtx_t *ctx)
{
	uint16_t	DescriptorLen;
	uint8_t		DescriptorType;
	uint8_t		InterfaceNum = 0;
	uint16_t	i;
	uint8_t	*pBuf;

	OTG_DBG("HOST Start enum Device\n");
	OTG_DBG("Get Device  Descriptor\n");
	if(!OTG_HostGetDescriptor(ctx,USB_DT_DEVICE, 0, (uint8_t*)&ctx->OtgHostInfo.DeviceDesCriptor, 8))
	{
		if(!OTG_HostGetDescriptor(ctx,USB_DT_DEVICE, 0, (uint8_t*)&ctx->OtgHostInfo.DeviceDesCriptor, 8))
		{
			if(!OTG_HostGetDescriptor(ctx,USB_DT_DEVICE, 0, (uint8_t*)&ctx->OtgHostInfo.DeviceDesCriptor, 8))
			{
				OTG_DBG("GET DESCRIPTOR Error\n");
				return FALSE;
			}
		}
	}
	if(ctx->dev_type == HOST_DEV_FS)
	{
		extern uint8_t gFsMaxPacketSize0;
		gFsMaxPacketSize0 = ctx->OtgHostInfo.DeviceDesCriptor.bMaxPacketSize0;
	}
	else if(ctx->dev_type == HOST_DEV_HS)
	{
		extern uint8_t gHsMaxPacketSize0;
		gHsMaxPacketSize0 = ctx->OtgHostInfo.DeviceDesCriptor.bMaxPacketSize0;
	}


	DescriptorLen = ctx->OtgHostInfo.DeviceDesCriptor.bLength;
	OTG_DBG("Set Device  Address\n");
	if(!OTG_HostSetAddress(ctx,1))
	{
		return FALSE;
	}
	OTG_DBG("Get All Device  Descriptor\n");
	if(!OTG_HostGetDescriptor(ctx,USB_DT_DEVICE, 0, (uint8_t *)&ctx->OtgHostInfo.DeviceDesCriptor, DescriptorLen))
	{
		return FALSE;
	}
	
	//设备描述符
	OTG_DBG("Device DeviceDesCriptor:\n");
	OTG_DBG("bLength:\t%02X \n",ctx->OtgHostInfo.DeviceDesCriptor.bLength);
	OTG_DBG("bDescriptorType:\t%02X \n",ctx->OtgHostInfo.DeviceDesCriptor.bDescriptorType);
	OTG_DBG("bcdUSB:\t%04X \n",ctx->OtgHostInfo.DeviceDesCriptor.bcdUSB);
	OTG_DBG("bDeviceClass:\t%02X \n",ctx->OtgHostInfo.DeviceDesCriptor.bDeviceClass);
	OTG_DBG("bDeviceSubClass:\t%02X \n",ctx->OtgHostInfo.DeviceDesCriptor.bDeviceSubClass);
	OTG_DBG("bDeviceProtocol:\t%02X \n",ctx->OtgHostInfo.DeviceDesCriptor.bDeviceProtocol);
	OTG_DBG("bMaxPacketSize0:\t%02X \n",ctx->OtgHostInfo.DeviceDesCriptor.bMaxPacketSize0);
	OTG_DBG("idVendor:\t%04X \n",ctx->OtgHostInfo.DeviceDesCriptor.idVendor);
	OTG_DBG("idProduct:\t%04X \n",ctx->OtgHostInfo.DeviceDesCriptor.idProduct);
	OTG_DBG("bcdDevice:\t%02X \n",ctx->OtgHostInfo.DeviceDesCriptor.bcdDevice);
	OTG_DBG("iManufacturer:\t%02X \n",ctx->OtgHostInfo.DeviceDesCriptor.iManufacturer);
	OTG_DBG("iProduct:\t%02X \n",ctx->OtgHostInfo.DeviceDesCriptor.iProduct);
	OTG_DBG("iSerialNumber:\t%02X \n",ctx->OtgHostInfo.DeviceDesCriptor.iSerialNumber);
	OTG_DBG("bNumConfigurations:\t%02X \n",ctx->OtgHostInfo.DeviceDesCriptor.bNumConfigurations);

	if(ctx->OtgHostInfo.DeviceDesCriptor.idVendor == 0x05AC)			//Apple
	{
		OTG_HostSetIpodChargeCurrent(ctx,500);
	}

	if(ctx->OtgHostInfo.DeviceDesCriptor.bNumConfigurations != 1)
	{
		return FALSE;
	}

	OTG_DBG("Get Device ConfigCriptor\n");
	if(!OTG_HostGetDescriptor(ctx,USB_DT_CONFIG, 0, (uint8_t *)&ctx->OtgHostInfo.ConfigDesCriptor, 9))
	{
		return FALSE;
	}

	DescriptorLen = ctx->OtgHostInfo.ConfigDesCriptor.wTotalLength;

#ifdef FUNC_OS_EN
	if(ctx->OtgHostInfo.DesCriptorBuffer)
	{
		osPortFree(ctx->OtgHostInfo.DesCriptorBuffer);
		ctx->OtgHostInfo.DesCriptorBuffer = NULL;
	}
	ctx->OtgHostInfo.DesCriptorBuffer = osPortMalloc(DescriptorLen);
	if(!ctx->OtgHostInfo.DesCriptorBuffer)
	{
		OTG_DBG("DesCriptorBuffer ERROR\n");
		return FALSE;
	}
#else
	if(DescriptorLen > MAX_DESCRIPTOR_LEN)
	{
		OTG_DBG("DesCriptorBuffer ERROR %d byte\n",DescriptorLen);
		return FALSE;
	}
	ctx->OtgHostInfo.DesCriptorBuffer = ctx->DesCriptorBuffer;
#endif
	memset(ctx->OtgHostInfo.DesCriptorBuffer, 0, DescriptorLen);

	OTG_DBG("Get All Device ConfigCriptor %u byte\n",DescriptorLen);
	if(!OTG_HostGetDescriptor(ctx,USB_DT_CONFIG, 0, ctx->OtgHostInfo.DesCriptorBuffer, DescriptorLen))
	{
		return FALSE;
	}
	
	if(ctx->OtgHostInfo.ConfigDesCriptor.bNumInterfaces > MAXINTERFACE)
	{
		return FALSE;
	}
	
	pBuf = ctx->OtgHostInfo.DesCriptorBuffer + 9;
	ctx->OtgHostInfo.UsbInterfaceNum = 0;
	i = 0;
	OTG_DBG("解析配置描述符\n");
	while(1)
	{
		DescriptorLen = pBuf[i];
		if(DescriptorLen == 0)return FALSE;// bkd add for dead loop
		DescriptorType = pBuf[i+1];
		if((DescriptorType == USB_DT_INTERFACE) && (pBuf[i + 3] == 0))//接口atl setting = 0，表示一个新的接口
		{
			//InterfaceNum = pBuf[i+2];
			InterfaceNum = ctx->OtgHostInfo.UsbInterfaceNum;//Sam,20251223，有发现interface号不按顺序增加的情况。#16879
			ctx->OtgHostInfo.UsbInterface[InterfaceNum].pData = pBuf + i;
			ctx->OtgHostInfo.UsbInterface[InterfaceNum].Length = 0;
			ctx->OtgHostInfo.UsbInterfaceNum++;
		}
		ctx->OtgHostInfo.UsbInterface[InterfaceNum].Length = ctx->OtgHostInfo.UsbInterface[InterfaceNum].Length + DescriptorLen;
		i = i + DescriptorLen;
		if(i > ctx->OtgHostInfo.ConfigDesCriptor.wTotalLength)return FALSE;// bkd add for dead loop
		if(i == (ctx->OtgHostInfo.ConfigDesCriptor.wTotalLength - 9))
		{
			if(ctx->OtgHostInfo.UsbInterfaceNum == ctx->OtgHostInfo.ConfigDesCriptor.bNumInterfaces)
			{
				OTG_DBG("解析配置描述符 ok\n");
				if(OTG_HostSetConfiguration(ctx,ctx->OtgHostInfo.ConfigDesCriptor.bConfigurationValue))
				{
					return TRUE;
				}
				return FALSE;
			}
			OTG_DBG("解析配置描述符 Error 1\n");
			return FALSE;
		}
		else if(i > ctx->OtgHostInfo.ConfigDesCriptor.wTotalLength)
		{
			OTG_DBG("解析配置描述符 Error\n");
			return FALSE;			
		}
	}
}


/**
 * @brief  初始化设备上下文
 * @param  dev_type 对外设备类型
 * @param  bus_ops 总线操作函数指针
 * @return 成功/失败
 */
bool Host_CtxInit(HOST_DEV_TYPE dev_type,Host_BusOps_t *bus_ops)
{
	HostCtx_t *ctx = Host_GetCtx(dev_type);
    if (!ctx) {
        return FALSE;
    }
    // 清空上下文
    memset(ctx, 0, sizeof(HostCtx_t));
    // 绑定核心资源
    ctx->dev_type = dev_type;
    if(bus_ops)
    {
        memcpy(&ctx->bus_ops, bus_ops, sizeof(Host_BusOps_t));
    }
    else
    {
        // FS设备（HOST_DEV_FS = 0）
    	if(ctx->dev_type == HOST_DEV_FS)
        {
    		ctx->bus_ops.HostAddressSet = OTG_FS_HostAddressSet;
    		ctx->bus_ops.HostControlWrite = OTG_FS_HostControlWrite;
    		ctx->bus_ops.HostControlRead = OTG_FS_HostControlRead;
    		ctx->bus_ops.HostBulkRead = OTG_FS_HostBulkRead;
    		ctx->bus_ops.HostBulkWrite = OTG_FS_HostBulkWrite;
    		ctx->bus_ops.HostPortIsLink = OTG_FS_PortHostIsLink;

        }
    	else if(ctx->dev_type == HOST_DEV_HS)
        // HS设备（HOST_DEV_HS = 1）
        {
    		ctx->bus_ops.HostAddressSet = OTG_HS_HostAddressSet;
    		ctx->bus_ops.HostControlWrite = OTG_HS_HostControlWrite;
    		ctx->bus_ops.HostControlRead = OTG_HS_HostControlRead;
    		ctx->bus_ops.HostBulkRead = OTG_HS_HostBulkRead;
    		ctx->bus_ops.HostBulkWrite = OTG_HS_HostBulkWrite;
    		ctx->bus_ops.HostPortIsLink = OTG_HS_PortHostIsLink;
        }
    }
    return TRUE;
}
/**
 * @brief  usb host device init
 * @param  NONE
 * @return 每个bit表示一个功能
 */
uint8_t USBH_HostInit(HOST_DEV_TYPE dev)
{
	uint8_t FunFlage = 0;
    Host_CtxInit(dev,NULL);

	HostCtx_t *ctx = Host_GetCtx(dev);
    if (!ctx) {
        return FALSE;
    }
	if(!OTG_HostEnumDevice(ctx))
	{
		OTG_DBG("HostEnumDevice() false!\n");
		return FunFlage;
	}
	if(UDisk_Init(dev,&ctx->OtgHostInfo))//枚举MASSSTORAGE接口
	{
		return 0x01;
	}
#ifdef CFG_FUNC_USB_HOST_AUDIO_MIX_MODE
#if defined(USB_HOST_AUDIO_MIX_FS)
	if(dev == HOST_DEV_FS)
#elif defined(USB_HOST_AUDIO_MIX_HS)
	if(dev == HOST_DEV_HS)
#endif
	{
		if(UsbHostPlayHardwareInit(dev,ctx))
		{
			return 0x02;
		}
	}
#endif
	return FunFlage;
}

uint8_t USBH_HostUnInit(HOST_DEV_TYPE dev)
{
	HostCtx_t *ctx = Host_GetCtx(dev);
    if (!ctx) {
        return FALSE;
    }
#ifdef FUNC_OS_EN
	if(ctx->OtgHostInfo.DesCriptorBuffer)
	{
		OTG_DBG("DesCriptorBuffer\n");
		osPortFree(ctx->OtgHostInfo.DesCriptorBuffer);
		ctx->OtgHostInfo.DesCriptorBuffer = 0;
	}
#endif
	return TRUE;
}
