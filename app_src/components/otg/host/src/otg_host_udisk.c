#include <string.h>
#include "type.h"
#include "timeout.h"
#include "delay.h"
#include "debug.h"
#include "otg_detect.h"
#include "otg_host_hcd.h"
#include "otg_host_udisk.h"
#ifdef FUNC_OS_EN
#include "rtos_api.h"
#endif

extern int kprintf(const char *fmt, ...);
#undef  OTG_DBG
#define	OTG_DBG(format, ...)		printf(format, ##__VA_ARGS__)

#ifndef UDISK_BULK_TIMEOUT
	#define	UDISK_BULK_TIMEOUT		10000//ms 3000~10000可配,兼容性与加快读盘异常退出成反比
#endif

// U盘核心上下文（内部使用）
typedef struct {
    // U盘基础信息
    UDISK UDisk;
    uint8_t UDiskBuf[40];
    CBW cbw;
    uint16_t HeadLen;
    uint16_t DataLen;
    uint16_t TailLen;
    bool UDiskInitOK;
    bool UDiskNotReadyErrFlag;
    uint32_t CmdErrCnt;

    // 总线专属资源
    HOST_DEV_TYPE dev_type;  // 映射到对外设备类型
    Host_BusOps_t bus_ops;     // 总线操作函数指针
    OTG_HOST_INFO *host_info; // 总线Host信息指针

    // 同步资源
#ifdef FUNC_OS_EN
    osMutexId UDiskMutex;
#endif
} UDiskCtx_t;
//内部管理资源
static UDiskCtx_t s_udisk_inst[HOST_DEV_MAX];
// 总线类型字符串映射（内部调试用）
static const char* UDiskDevTypeStr[] = {"FS", "HS"};
/**
 * @brief  内部函数：获取设备上下文
 * @param  dev_type 对外设备类型
 * @return 上下文指针（NULL表示无效）
 */
static UDiskCtx_t* UDisk_GetCtx(HOST_DEV_TYPE dev_type)
{
    if (dev_type >= HOST_DEV_MAX) {
        OTG_DBG("无效的设备类型：%d\n", dev_type);
        return NULL;
    }
    return &s_udisk_inst[dev_type];
}

/**
 * @brief  内部函数：获取存储设备最大LUN
 */
static uint8_t UDiskGetMaxLUN(UDiskCtx_t *ctx)
{
    if (!ctx || !ctx->bus_ops.HostControlRead) {
        return 0;
    }

    USB_CTRL_SETUP_REQUEST SetupPacket;
    uint8_t CmdGetMaxLun[8] = {0xA1, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00};
    ctx->UDisk.MaxLUN = 0;
    memcpy((uint8_t *)&SetupPacket, CmdGetMaxLun, 8);

    if (ctx->bus_ops.HostControlRead(SetupPacket, &ctx->UDisk.MaxLUN, 1, 1000) != HOST_NONE_ERR) {
        return ctx->UDisk.MaxLUN;
    }
    return 0;
}
/**
 * @brief  discard packets
 * @param  nBytes 长度
 * @return 1-成功，0-失败
 */
static bool UDiskDiscardPackets(UDiskCtx_t *ctx,uint16_t nBytes)
{
	uint8_t Buf[64];
	uint16_t Len;
	uint32_t RxLen;

    if (!ctx || !ctx->bus_ops.HostBulkRead) {
        return 0;
    }

	if(ctx->cbw.CDB[0] == READ_10)
	{
		while(nBytes > 0)
		{
			Len = (nBytes > 512) ? 512 : nBytes;
			nBytes -= Len;

			if(ctx->bus_ops.HostBulkRead(&ctx->UDisk.BulkInEp, Buf, Len,&RxLen,10000) != HOST_NONE_ERR)
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}
/**
 * @brief  内部函数：批量传输
 */
static bool UDiskBulkTransport(UDiskCtx_t *ctx, uint8_t* Buf, uint32_t Len)
{
    if (!ctx || !ctx->bus_ops.HostBulkRead || !ctx->bus_ops.HostBulkWrite) {
        return FALSE;
    }

	CSW csw;
//	uint8_t Retry = 2;
	uint32_t RxLen;
	TIMER ComTimer;
	int32_t RemainTime;
	TimeOutSet(&ComTimer, 3000);
	memset(&csw,0,sizeof(CSW));
	ctx->cbw.DataTransferLength = CpuToLe32(Len);

	//支持扇区大小不是512字节的U盘
	if(ctx->cbw.CDB[0] == READ_10)
	{
		Len = ctx->DataLen;
	}

	if(ctx->bus_ops.HostBulkWrite(&ctx->UDisk.BulkOutEp,(uint8_t*)&ctx->cbw,sizeof(CBW),3000) != HOST_NONE_ERR)
	{
		DBG("cmd error\n");
		ctx->CmdErrCnt++;
#ifdef FUNC_OS_EN
		osTaskDelay(1);// sam add 20200916
#endif
//		if(IsTimeOut(&ComTimer))
		{
			return FALSE;
		}
		//continue;
	}
	if(Len)
	{
		if((ctx->cbw.Flags & 0x80) == US_BULK_FLAG_IN)
		{
			//IN PACKET
			//DBG("I");
			UDiskDiscardPackets(ctx,ctx->HeadLen);
			RemainTime = UDISK_BULK_TIMEOUT - PastTimeGet(&ComTimer);
			if(RemainTime > 0  && ctx->bus_ops.HostBulkRead(&ctx->UDisk.BulkInEp, Buf, Len, &RxLen, RemainTime) != HOST_NONE_ERR)
			{
#ifdef FUNC_OS_EN
				osTaskDelay(1);// sam add 20200916
#endif
				//DBG("IN E\n");
				if(IsTimeOut(&ComTimer))
				{
					return FALSE;
				}
			}
			UDiskDiscardPackets(ctx,ctx->TailLen);
		}
		else
		{
			//OUT PACKET
			//DBG("O");
#ifdef FUNC_OS_EN
			osTaskDelay(1);// bkd del 2020/03/24
#else
			DelayMs(1);
#endif

			RemainTime = UDISK_BULK_TIMEOUT - PastTimeGet(&ComTimer);
			if(RemainTime > 0 && ctx->bus_ops.HostBulkWrite(&ctx->UDisk.BulkOutEp, Buf, Len, RemainTime) != HOST_NONE_ERR)
			{
#ifdef FUNC_OS_EN
				osTaskDelay(1);// sam add 20200916
#endif
				//DBG("OUT E\n");
				if(IsTimeOut(&ComTimer))
				{
					return FALSE;
				}
			}
		}
	}
	RemainTime = UDISK_BULK_TIMEOUT - PastTimeGet(&ComTimer);
	if(RemainTime > 0 && ctx->bus_ops.HostBulkRead(&ctx->UDisk.BulkInEp,(uint8_t*)&csw,sizeof(CSW),&RxLen, RemainTime) != HOST_NONE_ERR)
	{
		//DBG("OTG read block\n");
		return FALSE;
	}

	if(csw.Signature == 0x53425355 && csw.Tag == ctx->cbw.Tag && csw.Status == 0)
	{
		//DBG("B OK\n");
		ctx->CmdErrCnt = 0;
		return TRUE;	//SUCCESS
	}
    return FALSE;
}
/**
 * @brief  inquiry scsi command
 * @param  Lun logic unit number
 * @return 1-成功，0-失败
 */
static bool UDiskInquiry(UDiskCtx_t *ctx,uint8_t Lun)
{
	memset(ctx->cbw.CDB, 0x00, sizeof(ctx->cbw.CDB));
	ctx->cbw.CDB[0] = INQUIRY;
	ctx->cbw.CDB[4] = 36;
	ctx->cbw.Signature = 0x43425355;
	ctx->cbw.Flags = US_BULK_FLAG_IN;
	ctx->cbw.Lun = Lun;
	ctx->cbw.Length = 6;

	return UDiskBulkTransport(ctx,ctx->UDiskBuf, 36);
}
/**
 * @brief  read format capacity scsi command
 * @param  Lun logic unit number
 * @return 1-成功，0-失败
 */
static bool UDiskReadFormatCapacity(UDiskCtx_t *ctx,uint8_t Lun)
{
	memset(ctx->cbw.CDB, 0x00, sizeof(ctx->cbw.CDB));
	ctx->cbw.CDB[0] = READ_FORMAT_CAPACITY;
	ctx->cbw.CDB[4] = 12;
	ctx->cbw.Flags = US_BULK_FLAG_IN;
	ctx->cbw.Lun = Lun;
	ctx->cbw.Length = 10;
	return UDiskBulkTransport(ctx,ctx->UDiskBuf, 12);
}
/**
 * @brief  read capacity scsi command
 * @param  Lun logic unit number
 * @return 1-成功，0-失败
 */
static bool UDiskReadCapacity(UDiskCtx_t *ctx,uint8_t Lun)
{
//	DBG("StorReadCapacity()\n");
	memset(ctx->cbw.CDB, 0x00, sizeof(ctx->cbw.CDB));
	ctx->cbw.CDB[0] = READ_CAPACITY;
//	cbw.CDB[4] = 8;
	ctx->cbw.Flags = US_BULK_FLAG_IN;
	ctx->cbw.Lun = Lun;
	ctx->cbw.Length = 10;

	return UDiskBulkTransport(ctx,ctx->UDiskBuf, 8);

}

/**
 * @brief  request sense scsi command
 * @param  Lun logic unit number
 * @return 1-成功，0-失败
 */
static bool UDiskRequestSense(UDiskCtx_t *ctx,uint8_t Lun)
{
//	DBG("StorRequestSense()\n");
	memset(ctx->cbw.CDB, 0x00, sizeof(ctx->cbw.CDB));
	ctx->cbw.CDB[0] = REQUEST_SENSE;
	ctx->cbw.CDB[4] = 18;
	ctx->cbw.Flags = US_BULK_FLAG_IN;
	ctx->cbw.Lun = Lun;
	ctx->cbw.Length = 12;
	if(UDiskBulkTransport(ctx,ctx->UDiskBuf, 18))
	{
		if(ctx->UDiskBuf[2] == 0x02)	//Not ready
		{
			return FALSE;
		}
	}
	return TRUE;
}

/**
 * @brief  test unit ready scsi command
 * @param  Lun logic unit number
 * @return 1-成功，0-失败
 */
static bool UDiskTestUnitReady(UDiskCtx_t *ctx,uint8_t Lun)
{
//	DBG("StorTestUnitReady()\n");
	memset(ctx->cbw.CDB, 0x00, sizeof(ctx->cbw.CDB));
	ctx->cbw.CDB[0] = TEST_UNIT_READY;
	//cbw.CDB[4] = 0;
	ctx->cbw.Flags = US_BULK_FLAG_OUT;
	ctx->cbw.Lun = Lun;
	ctx->cbw.Length = 6;

	return UDiskBulkTransport(ctx,ctx->UDiskBuf, 0);
}


/**
 * @brief  初始化设备上下文
 * @param  dev_type 对外设备类型
 * @param  host_info 总线Host信息
 * @param  bus_ops 总线操作函数指针
 * @return 成功/失败
 */
bool UDisk_CtxInit(HOST_DEV_TYPE dev_type,OTG_HOST_INFO *host_info, Host_BusOps_t *bus_ops)
{
    UDiskCtx_t *ctx = UDisk_GetCtx(dev_type);
    if (!ctx || !host_info) {
        return FALSE;
    }

    // 清空上下文
    memset(ctx, 0, sizeof(UDiskCtx_t));

    // 绑定核心资源
    ctx->dev_type = dev_type;
    ctx->host_info = host_info;
    if(bus_ops)
    {
        memcpy(&ctx->bus_ops, bus_ops, sizeof(Host_BusOps_t));
    }
    else
    {
        // FS设备（HOST_DEV_FS = 0）
    	if(ctx->dev_type == HOST_DEV_FS)
        {
    		ctx->bus_ops.HostControlRead = OTG_FS_HostControlRead;
    		ctx->bus_ops.HostBulkRead = OTG_FS_HostBulkRead;
    		ctx->bus_ops.HostBulkWrite = OTG_FS_HostBulkWrite;
    		ctx->bus_ops.HostPortIsLink = OTG_FS_PortHostIsLink;

        }
    	else if(ctx->dev_type == HOST_DEV_HS)
        // HS设备（HOST_DEV_HS = 1）
        {
    		ctx->bus_ops.HostControlRead = OTG_HS_HostControlRead;
    		ctx->bus_ops.HostBulkRead = OTG_HS_HostBulkRead;
    		ctx->bus_ops.HostBulkWrite = OTG_HS_HostBulkWrite;
    		ctx->bus_ops.HostPortIsLink = OTG_HS_PortHostIsLink;
        }
    }
    OTG_DBG("[%s] 设备上下文初始化成功\n", UDiskDevTypeStr[dev_type]);
    return TRUE;
}
/**
 * @brief  初始化U盘设备（FS/HS）
 * @return 成功/失败
 */
bool UDisk_Init(HOST_DEV_TYPE dev,OTG_HOST_INFO *host_info)
{
	UDisk_CtxInit(dev,host_info,NULL);

	UDiskCtx_t *ctx = UDisk_GetCtx(dev);
	if (!ctx) {
		return FALSE;
	}

    // 1. 创建互斥锁（仅第一次调用时创建）
#ifdef FUNC_OS_EN
    if (ctx->UDiskMutex == NULL) {
        ctx->UDiskMutex = osMutexCreate();
        if (ctx->UDiskMutex == NULL) {
            OTG_DBG("[%s] 互斥锁创建失败\n", UDiskDevTypeStr[dev]);
            return FALSE;
        }
    }
#endif

    // 2. 查找USB MASS设备接口
    uint32_t i;
    uint32_t MassStorDeviceCount = 0;
    uint32_t MassStorDeviceInterface = 0;
    uint32_t EndPointFlag = 0;
    uint32_t EndPointCount = 0;
    bool InquiryOkFlag = FALSE;
    bool ReadFormatCapacityOkFlag = FALSE;
    TIMER WaitStorReadyTimer1;
    TIMER WaitStorReadyTimer2;

    // 解析Interface描述符
    for (i = 0; i < ctx->host_info->ConfigDesCriptor.bNumInterfaces; i++) {
        PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor =
            (PUSB_INTERFACE_DESCRIPTOR)ctx->host_info->UsbInterface[i].pData;

        // 仅支持Mass Storage类、BBB协议
        if ((InterfaceDescriptor->bInterfaceClass == USB_CLASS_MASS_STORAGE) &&
            ((InterfaceDescriptor->bInterfaceSubClass == 5) || (InterfaceDescriptor->bInterfaceSubClass == 6)) &&
            (InterfaceDescriptor->bInterfaceProtocol == 0x50)) {
            MassStorDeviceCount++;
            MassStorDeviceInterface = i;
        }
    }

    // 校验Mass Storage接口数量
    if (MassStorDeviceCount == 0) {
        OTG_DBG("[%s] 未找到MassStor设备接口\n", UDiskDevTypeStr[dev]);
        return FALSE;
    } else if (MassStorDeviceCount > 1) {
        OTG_DBG("[%s] 找到%ld个MassStor设备接口（仅支持1个）\n",UDiskDevTypeStr[dev], MassStorDeviceCount);
        return FALSE;
    }
    OTG_DBG("[%s] 找到%ld个MassStor设备接口\n",UDiskDevTypeStr[dev], MassStorDeviceCount);

    // 解析端点描述符（Bulk-Only协议需2个BULK端点）
    uint8_t *pBuf = ctx->host_info->UsbInterface[MassStorDeviceInterface].pData + 9;
    i = 0;
    while (1) {
        // 检查是否为BULK端点描述符
        if ((pBuf[i] == 0x07) && (pBuf[i+1] == 0x05) && (pBuf[i+3] == 0x02)) {
            if (pBuf[i+2] & 0x80) {
                // Bulk IN端点
                OTG_DBG("[%s] 找到Bulk IN端点\n", UDiskDevTypeStr[dev]);
                EndPointFlag |= 0x01;
                EndPointCount++;
                ctx->UDisk.BulkInEp.EpNum = pBuf[i+2];
                ctx->UDisk.BulkInEp.MaxPacketSize = pBuf[i+4] + (pBuf[i+5] << 8);
            } else {
                // Bulk OUT端点
                OTG_DBG("[%s] 找到Bulk OUT端点\n", UDiskDevTypeStr[dev]);
                EndPointFlag |= 0x02;
                EndPointCount++;
                ctx->UDisk.BulkOutEp.EpNum = pBuf[i+2];
                ctx->UDisk.BulkOutEp.MaxPacketSize = pBuf[i+4] + (pBuf[i+5] << 8);
            }
        }

        i += pBuf[i];

        // 校验解析结果
        if ((EndPointCount == 2) && (EndPointFlag == 3)) {
            OTG_DBG("[%s] 端点描述符解析成功\n", UDiskDevTypeStr[dev]);
            OTG_DBG("[%s] Bulk IN: 0x%02X, MaxPacketSize: %d\n",UDiskDevTypeStr[dev],
            		ctx->UDisk.BulkInEp.EpNum,
					ctx->UDisk.BulkInEp.MaxPacketSize);
            OTG_DBG("[%s] Bulk OUT: 0x%02X, MaxPacketSize: %d\n",UDiskDevTypeStr[dev],
            		ctx->UDisk.BulkOutEp.EpNum,
					ctx->UDisk.BulkOutEp.MaxPacketSize);
            break;
        }

        if (i == (ctx->host_info->UsbInterface[MassStorDeviceInterface].Length - 9)) {
            OTG_DBG("[%s] 端点错误  %lu %lu\n", UDiskDevTypeStr[dev],EndPointCount,EndPointFlag);
            return FALSE;
        }

        if (i > (ctx->host_info->UsbInterface[MassStorDeviceInterface].Length - 9)) {
            OTG_DBG("[%s] 端点描述符解析越界\n", UDiskDevTypeStr[dev]);
            return FALSE;
        }
    }

    // 获取最大LUN数
    OTG_DBG("[%s] 获取MaxLUN\n", UDiskDevTypeStr[dev]);
    UDiskGetMaxLUN(ctx);

    // 设备查询（Inquiry）
    OTG_DBG("[%s] 设备Inquiry\n", UDiskDevTypeStr[dev]);
    for (i = 0; i < (ctx->UDisk.MaxLUN + 1) && ctx->bus_ops.HostPortIsLink(); i++) {
        bool Ret = UDiskInquiry(ctx, i);

#ifdef FUNC_OS_EN
        osTaskDelay(5);
#else
        WaitMs(5);
#endif

        if (Ret && (ctx->UDiskBuf[0] == 0)) {
            if (!InquiryOkFlag) {
                InquiryOkFlag = TRUE;
                ctx->UDisk.LuNum = i;
            }

            // 多LUN设备处理
            if (ctx->UDisk.MaxLUN > 0) {
                UDiskReadFormatCapacity(ctx, i);
                if (UDiskReadCapacity(ctx, i)) {
                    uint32_t *pp = (uint32_t*)ctx->UDiskBuf;
                    if ((!ReadFormatCapacityOkFlag) && (Be32ToCpu(*pp) > 4096)) {
                        ReadFormatCapacityOkFlag = TRUE;
                        ctx->UDisk.LuNum = i;
                    }
                }
            }
        }

        UDiskRequestSense(ctx, i);
    }

    if (!InquiryOkFlag) {
        OTG_DBG("[%s] Inquiry失败\n", UDiskDevTypeStr[dev]);
        return FALSE;
    }

#ifdef FUNC_OS_EN
    osTaskDelay(10);
#else
    WaitMs(10);  // 兼容部分U盘（如130号U盘）
#endif

    // 等待设备就绪
    TimeOutSet(&WaitStorReadyTimer1, 10000);
    TimeOutSet(&WaitStorReadyTimer2, 2000);
    while (1) {
        // 设备已拔出
        if (!ctx->bus_ops.HostPortIsLink()) {
            OTG_DBG("[%s] 设备已拔出\n", UDiskDevTypeStr[dev]);
            return FALSE;
        }

        // 设备就绪
        if (UDiskTestUnitReady(ctx, ctx->UDisk.LuNum)) {
            break;
        }

        // 请求感知失败且超时
        if (!UDiskRequestSense(ctx, ctx->UDisk.LuNum)) {
            if (IsTimeOut(&WaitStorReadyTimer2)) {
                ctx->UDiskNotReadyErrFlag = TRUE;
                OTG_DBG("[%s] 设备未就绪超时\n", UDiskDevTypeStr[dev]);
                return FALSE;
            }
        }

        // 总超时
        if (IsTimeOut(&WaitStorReadyTimer1)) {
            OTG_DBG("[%s] 设备就绪等待超时\n", UDiskDevTypeStr[dev]);
            return FALSE;
        }

        // 短延时
#ifdef FUNC_OS_EN
        osTaskDelay(200);
#else
        WaitMs(200);
#endif
    }

    // 读取容量（兼容72号U盘，重试1次）
    if (!UDiskReadCapacity(ctx, ctx->UDisk.LuNum)) {
        if (!UDiskReadCapacity(ctx, ctx->UDisk.LuNum)) {
            OTG_DBG("[%s] 读取容量失败\n", UDiskDevTypeStr[dev]);
            return FALSE;
        }
    }

    // 解析容量信息
    uint16_t *PP16 = (uint16_t*)(&ctx->UDiskBuf[6]);
    uint32_t *PP32 = (uint32_t*)(&ctx->UDiskBuf[0]);
    ctx->UDisk.BlockSize = Be16ToCpu(*PP16);
    ctx->UDisk.LastLBA = Be32ToCpu(*PP32);

    // 初始化完成
    ctx->UDiskInitOK = TRUE;
    OTG_DBG("[%s] U盘初始化成功，BlockSize: %d, LastLBA: %lu\n",
            UDiskDevTypeStr[dev],
            ctx->UDisk.BlockSize, ctx->UDisk.LastLBA);

    return TRUE;
}

/**
 * @brief  read blocks
 * @param  BlockNum block number
 * @param  Buf 读缓冲区指针
 * @param  BlockCnt 要读的block个数
 * @return 1-成功，0-失败
 */
bool UDisk_ReadBlock(HOST_DEV_TYPE dev_type, uint32_t BlockNum, void* Buf, uint8_t BlockCnt)
{
	uint8_t		Ret;
	uint8_t		Offset;
	uint8_t		Mult;
	uint8_t     RetryTransCnt = 3;
	uint32_t	*pp = NULL;

	UDiskCtx_t *ctx = UDisk_GetCtx(dev_type);
	if (!ctx) {
		return FALSE;
	}

#ifdef FUNC_OS_EN
	osMutexLock(ctx->UDiskMutex);
#endif

	if(!ctx->UDiskInitOK || !ctx->bus_ops.HostPortIsLink())// Modify by lean.xiong @2013-06-21 设备拔除后不需要再重试
	{
		OTG_DBG("HostStorReadBlock() error!\n");

#ifdef FUNC_OS_EN
		osMutexUnlock(ctx->UDiskMutex);
#endif
		return FALSE;
	}
	ctx->DataLen = BlockCnt * 512;
	if(ctx->UDisk.BlockSize != 512)
	{
		Mult = (ctx->UDisk.BlockSize >> 9);
		Offset = ((uint8_t)BlockNum & (Mult - 1));

		BlockNum /= (ctx->UDisk.BlockSize / 512);
		ctx->HeadLen = (Offset << 9);

		ctx->TailLen = ((0 - Offset - BlockCnt) & (Mult - 1)) * 512;
		BlockCnt = (Offset + BlockCnt + (Mult - 1)) / Mult;
	}

	if(BlockNum > ctx->UDisk.LastLBA)
	{

#ifdef FUNC_OS_EN
		osMutexUnlock(ctx->UDiskMutex);
#endif
		return FALSE;
	}

	//DBG("HostStorReadBlock(%ld, %-.4X, %d)\n", lba, (uint16_t)buf, (uint16_t)size);
	memset(ctx->cbw.CDB, 0x00, sizeof(ctx->cbw.CDB));
	ctx->cbw.CDB[0] = READ_10;


	//((uint32_t*)(&cbw.CDB[2]))[0] = CpuToBe32(BlockNum);
	//cbw.CDB[2] = BlockNum>>24;
	//cbw.CDB[3] = BlockNum>>16;
	//cbw.CDB[4] = BlockNum>>8;
	//cbw.CDB[5] = BlockNum;

	pp = ((uint32_t*)(&ctx->cbw.CDB[2]));
	*pp=	CpuToBe32(BlockNum);
	//Cpu32bitSetValue(&cbw.CDB[2],BlockNum);

	ctx->cbw.CDB[8] = BlockCnt;

	ctx->cbw.Flags = US_BULK_FLAG_IN;
	ctx->cbw.Lun = ctx->UDisk.LuNum;
	ctx->cbw.Length = 10;
	Ret = UDiskBulkTransport(ctx,Buf, ctx->HeadLen + ctx->DataLen + ctx->TailLen);

	if(!Ret)
	{
		while(RetryTransCnt--)
		{
			//如果是设备被拔除，则不再重试。
			if(!ctx->bus_ops.HostPortIsLink())
			{
				OTG_DBG("UsbHostIsLink?\n");
				break;
			}
			memset(ctx->cbw.CDB, 0x00, sizeof(ctx->cbw.CDB));
			ctx->cbw.CDB[0] = READ_10;


//			((uint32_t*)(&cbw.CDB[2]))[0] = CpuToBe32(BlockNum);
//			cbw.CDB[2] = BlockNum>>24;
//			cbw.CDB[3] = BlockNum>>16;
//			cbw.CDB[4] = BlockNum>>8;
//			cbw.CDB[5] = BlockNum;
			//Cpu32bitSetValue(&cbw.CDB[2],BlockNum);
			pp = ((uint32_t*)(&ctx->cbw.CDB[2]));
			*pp=	CpuToBe32(BlockNum);
			ctx->cbw.CDB[8] = BlockCnt;

			ctx->cbw.Flags = US_BULK_FLAG_IN;
			ctx->cbw.Lun = ctx->UDisk.LuNum;
			ctx->cbw.Length = 10;

			Ret = UDiskBulkTransport(ctx,Buf, ctx->HeadLen + ctx->DataLen + ctx->TailLen);

			if(Ret == TRUE)
			{
				break;
			}
		}
	}
	ctx->HeadLen = 0;
	ctx->DataLen = 0;
	ctx->TailLen = 0;

#ifdef FUNC_OS_EN
		osMutexUnlock(ctx->UDiskMutex);
#endif
	return Ret;
}


/**
 * @brief  write blocks
 * @param  BlockNum block number
 * @param  Buf 写缓冲区指针
 * @param  BlockCnt 要写的block个数
 * @return 1-成功，0-失败
 */
bool UDisk_WriteBlock(HOST_DEV_TYPE dev_type,uint32_t BlockNum, void* Buf, uint8_t BlockCnt)
{
	uint8_t		Ret;
	uint32_t	*pp=NULL;

	UDiskCtx_t *ctx = UDisk_GetCtx(dev_type);
	if (!ctx) {
		return FALSE;
	}

#ifdef FUNC_OS_EN
	osMutexLock(ctx->UDiskMutex);
#endif

	memset(ctx->cbw.CDB, 0x00, sizeof(ctx->cbw.CDB));
	ctx->cbw.CDB[0] = WRITE_10;
	pp=((uint32_t*)(&ctx->cbw.CDB[2])) ;//= 0;//CpuToBe32(BlockNum);//BlockNum;
	*pp=CpuToBe32(BlockNum);
//	cbw.CDB[2] = BlockNum>>24;
//	cbw.CDB[3] = BlockNum>>16;
//	cbw.CDB[4] = BlockNum>>8;
//	cbw.CDB[5] = BlockNum;

	ctx->cbw.CDB[8] = BlockCnt;

	ctx->cbw.Flags = US_BULK_FLAG_OUT;
	ctx->cbw.Lun = ctx->UDisk.LuNum;
	ctx->cbw.Length = 10;


	Ret = UDiskBulkTransport(ctx,Buf, BlockCnt * 512);
#ifdef FUNC_OS_EN
	osMutexUnlock(ctx->UDiskMutex);
#endif

	return Ret;
}
/**
 * @brief  对外接口：获取U盘块大小
 * @param  dev_type 设备类型（HOST_DEV_FS/HOST_DEV_HS）
 * @return 块大小（字节）
 */
uint16_t UDisk_GetBlockSize(HOST_DEV_TYPE dev_type)
{
    UDiskCtx_t *ctx = UDisk_GetCtx(dev_type);
    return (ctx && ctx->UDiskInitOK) ? ctx->UDisk.BlockSize : 0;
}

/**
 * @brief  对外接口：获取U盘最后LBA
 * @param  dev_type 设备类型（HOST_DEV_FS/HOST_DEV_HS）
 * @return 最后LBA编号
 */
uint32_t UDisk_GetLastLBA(HOST_DEV_TYPE dev_type)
{
    UDiskCtx_t *ctx = UDisk_GetCtx(dev_type);
    return (ctx && ctx->UDiskInitOK) ? ctx->UDisk.LastLBA : 0;
}

/**
 * @brief  对外接口：检查U盘是否就绪
 * @param  dev_type 设备类型（HOST_DEV_FS/HOST_DEV_HS）
 * @return 就绪/未就绪
 */
bool UDisk_IsReady(HOST_DEV_TYPE dev_type)
{
    UDiskCtx_t *ctx = UDisk_GetCtx(dev_type);
    if (!ctx) return FALSE;
    return (ctx->UDiskInitOK && ctx->bus_ops.HostPortIsLink());
}

/**
 * @brief  对外接口：获取指定U盘设备的互斥锁ID
 * @param  dev_type 设备类型（HOST_DEV_FS/HOST_DEV_HS）
 * @return 成功返回对应设备的互斥锁ID，失败返回NULL
 * @note   1. 需确保U盘上下文初始化完成后调用
 */
#ifdef FUNC_OS_EN
osMutexId UDisk_GetMutexId(HOST_DEV_TYPE dev_type)
{
    // 获取U盘上下文
    UDiskCtx_t *ctx = UDisk_GetCtx(dev_type);
    if (!ctx)
    {
        // 上下文为空，返回NULL（可添加日志/断言便于调试）
#ifdef DEBUG_EN
        printf("UDisk_GetMutexId err: ctx is NULL (dev_type=%d)\r\n", dev_type);
#endif
        return NULL;
    }
    // 返回上下文内的互斥锁ID
    return ctx->UDiskMutex;
}
#endif
