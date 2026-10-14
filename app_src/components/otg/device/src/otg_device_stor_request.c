/**
 *****************************************************************************
 * @file     otg_device_stor_request.c
 * @author   Shanks
 * @version  V1.0.0
 * @date     7-September-2015
 * @brief    USB 大容量存储设备（MSC/BOT）协议控制层驱动
 *****************************************************************************
 * @attention
 * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
 *****************************************************************************
 * @brief
 *  1. 实现 USB Mass Storage Bulk-Only Transport 协议
 *  2. 解析并处理标准 SCSI 命令：INQUIRY / READ10 / WRITE10 / READ_CAPACITY 等
 *****************************************************************************
 */
#include "app_config.h"
#include <string.h>
#include "type.h"
#include "timeout.h"
#include "debug.h"
#include "delay.h"
#include "mvintrinsics.h"
#include "otg_device_hcd.h"

#include "otg_device.h"
#include "otg_device_stor_request.h"
#ifdef FUNC_OS_EN
#include "rtos_api.h" //add for mutex declare
#endif

#if defined CFG_OTG_MODE_READER_EN || defined CFG_OTG_FS_MODE_READER_EN

const uint8_t gInquiryData[] =
{
	0x00, 		//Direct Access Device
	0x80, 		//RMB
	0x02, 		//ISO/ECMA/ANSI
	0x02, 		//Response Data Format
	0x1F, 		//Additional Length
	0x00, 		//Reserved
	0x00, 		//Reserved
	0x00, 		//Reserved
	'M', 'V', 'S', 'I', 0, 0, 0, 0,				//Vendor Information
	'C', 'a', 'r', 'd', ' ', 'R', 'e', 'a', 'd', 'e', 'r', 0, 0, 0, 0, 0,		//Product Identification
	'V', '1', '.', '0'		//Product Revision Level
};
const uint8_t gModeSenseProtectPage[] = {0x03, 0x00, 0x00, 0x00};
const uint8_t gRequestSenseNotReady[] =
{
	0x70, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x0A,
	0x00, 0x00, 0x00, 0x00, 0x3A, 0x00, 0x00, 0x00,
	0x00, 0x00
};
const uint8_t gRequestSenseReady[] =
{
	0x70, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x0A,
	0x00, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00,
	0x00, 0x00
};
////////////////////////////Mass Storage Request//////////////////////////////////////
/**
 * @brief  发送CSW
 * @param  Status CSW状态
 * @return NONE
 */
void DeviceStorSendCSW(DeviceCtx_t *ctx,uint8_t Status)
{
	UsbStorClass* pMSC = ((UsbCompositeClass*)ctx->pClassData)->pMSC;
	BOT_CSW *csw = (BOT_CSW *)pMSC->gCSW;
	csw->Residue = 0;
	csw->Signature = STOR_CSW_SIGNATURE;
	csw->Status = Status;
	ctx->bus_ops.BulkSend(pMSC->BulkInEp,pMSC->gCSW, sizeof(pMSC->gCSW), 100000);
}

/**
 * @brief  处理ReadFmtCapacity命令
 * @param  NONE
 * @return NONE
 */
int8_t DeviceStorReadFmtCapacity(DeviceCtx_t *ctx)
{
	UsbStorClass* pMSC = ((UsbCompositeClass*)ctx->pClassData)->pMSC;
	uint32_t Temp;
	uint8_t gFmtCapacityData[12];
	if(!pMSC->ops->DeviceLink())
	{
		ctx->bus_ops.StallSend(pMSC->BulkInEp);
		return -1;
	}

	memset(gFmtCapacityData,0,sizeof(gFmtCapacityData));

	Temp = pMSC->ops->CapacityGet() - 1;

	gFmtCapacityData[3] = 0x08U;
	gFmtCapacityData[4] = (uint8_t)(Temp >> 24);
	gFmtCapacityData[5] = (uint8_t)(Temp >> 16);
	gFmtCapacityData[6] = (uint8_t)(Temp >> 8);
	gFmtCapacityData[7] = (uint8_t)(Temp);

	Temp = pMSC->ops->get_block_size();

	gFmtCapacityData[8] = 0x03U;
	gFmtCapacityData[9] = (uint8_t)(Temp >>  16);
	gFmtCapacityData[10] = (uint8_t)(Temp >>  8);
	gFmtCapacityData[11] = (uint8_t)(Temp);

	ctx->bus_ops.BulkSend(pMSC->BulkInEp, gFmtCapacityData, sizeof(gFmtCapacityData), 10000);
	return 0;
}

/**
 * @brief  处理ReadCapacity命令
 * @param  NONE
 * @return NONE
 */
//extern uint8_t Setup[];

int8_t DeviceStorReadCapacity(DeviceCtx_t *ctx)
{
	UsbStorClass* pMSC = ((UsbCompositeClass*)ctx->pClassData)->pMSC;
	uint32_t Temp;
	uint8_t gCapacityData[8];

	if(!pMSC->ops->DeviceLink())
	{
		ctx->bus_ops.StallSend(pMSC->BulkInEp);
		return -1;
	}
	memset(gCapacityData,0,sizeof(gCapacityData));

	Temp = pMSC->ops->CapacityGet() - 1;
	gCapacityData[0] = (uint8_t)(Temp >> 24);
	gCapacityData[1] = (uint8_t)(Temp >> 16);
	gCapacityData[2] = (uint8_t)(Temp >>  8);
	gCapacityData[3] = (uint8_t)Temp;

	Temp = pMSC->ops->get_block_size();
	gCapacityData[4] = (uint8_t)(Temp >>  24);
	gCapacityData[5] = (uint8_t)(Temp >>  16);
	gCapacityData[6] = (uint8_t)(Temp >>   8);
	gCapacityData[7] = (uint8_t)(Temp);

	ctx->bus_ops.BulkSend(pMSC->BulkInEp, gCapacityData, sizeof(gCapacityData), 1000);
	return 0;
}

/**
 * @brief  通用 SCSI Read10 处理（支持任意块设备）
 * @param  ctx: USB 设备上下文
 * @return 0=成功 / -1=失败
 */
int8_t DeviceStorRead10(DeviceCtx_t *ctx)
{
	UsbStorClass* pMSC = ((UsbCompositeClass*)ctx->pClassData)->pMSC;

    uint32_t lba = Be32ToCpu(*(uint32_t*)(&pMSC->gCBW[17]));
    uint16_t cnt = Be16ToCpu(*(uint16_t*)(&pMSC->gCBW[22]));

    // 直接调用硬件实现的 Read10
    return pMSC->ops->read_block(ctx, lba, cnt);
}

/**
 * @brief  通用 SCSI Write10 处理（支持任意块设备）
 * @param  ctx: USB 设备上下文
 * @return 0=成功 / -1=失败
 */
int8_t DeviceStorWrite10(DeviceCtx_t *ctx)
{
	UsbStorClass* pMSC = ((UsbCompositeClass*)ctx->pClassData)->pMSC;

    uint32_t lba = Be32ToCpu(*(uint32_t*)(&pMSC->gCBW[17]));
    uint16_t cnt = Be16ToCpu(*(uint16_t*)(&pMSC->gCBW[22]));

    // 直接调用硬件实现的 Write10
    return pMSC->ops->Write_block(ctx, lba, cnt);
}

/**
 * @brief  处理ModeSense命令
 * @param  NONE
 * @return NONE
 */
//extern uint8_t Setup[];
int8_t DeviceStorModeSense(DeviceCtx_t *ctx)
{
	UsbStorClass* pMSC = ((UsbCompositeClass*)ctx->pClassData)->pMSC;

	uint32_t Temp;
	uint8_t	gModeSenseAllPage[12];

	if(!pMSC->ops->DeviceLink())
	{
		ctx->bus_ops.StallSend(pMSC->BulkInEp);
		return -1;
	}

	switch(pMSC->gCBW[17])
	{
		case 0x1C:		//mode sense, Timer and Protect Page
			ctx->bus_ops.BulkSend(pMSC->BulkInEp, (uint8_t*)gModeSenseProtectPage, sizeof(gModeSenseProtectPage), 10000);
			break;

		case 0x3F:		//mode sense, Return all pages
			memset(gModeSenseAllPage,0,sizeof(gModeSenseAllPage));

			Temp = pMSC->ops->CapacityGet() - 1;
			gModeSenseAllPage[0] = 0x0BU;
			gModeSenseAllPage[3] = 0x08U;

			gModeSenseAllPage[5] = (uint8_t)(Temp >> 16);
			gModeSenseAllPage[6] = (uint8_t)(Temp >>  8);
			gModeSenseAllPage[7] = (uint8_t)(Temp);

			Temp = pMSC->ops->get_block_size();

			gModeSenseAllPage[9] = (uint8_t)(Temp >>  16);
			gModeSenseAllPage[10] = (uint8_t)(Temp >>  8);
			gModeSenseAllPage[11] = (uint8_t)(Temp);

			ctx->bus_ops.BulkSend(pMSC->BulkInEp, gModeSenseAllPage, MIN(sizeof(gModeSenseAllPage),pMSC->gCBW[19]), 10000);
			break;

		default:
			ctx->bus_ops.BulkSend(pMSC->BulkInEp, 0, 0, 10000);
			break;
	}
	return 0;
}

///**
// * @brief  判断当前是否处于保护状态
// * @param  NONE
// * @return 1-保护，0-非保护
// */
//bool DeviceStorIsPrevent(void)
//{
//	UsbStorClass* pMSC = ((UsbCompositeClass*)ctx->pClassData)->pMSC;
//
//	if((!pMSC->DeviceStorPreventFlag) || IsTimeOut(&pMSC->DeviceStorPreventTimer))
//	{
//		return FALSE;
//	}
//	else
//	{
//		return TRUE;
//	}
//}

///**
// * @brief  判断PC端是否执行了“弹出”操作
// * @param  NONE
// * @return 1-已弹出，0-未弹出
// */
//bool DeviceStorIsStopped(void)
//{
//	if(pMSC->DeviceStorStoppedFlag)
//	{
//		return TRUE;
//	}
//	else if(pMSC->DeviceStorCmdFlag && IsTimeOut(&pMSC->DeviceStorCmdTimer))
//	{
//		return TRUE;
//	}
//	else
//	{
//		return FALSE;
//	}
//}

/**
 * @brief  读卡器状态初始化
 * @param  NONE
 * @return NONE
 */
void OTG_DeviceStorStateInit(UsbStorClass* pMSC)
{
	pMSC->sReaderState = READER_INIT;
	pMSC->DeviceStorStoppedFlag = FALSE;
	pMSC->DeviceStorCmdFlag = FALSE;
	pMSC->sReaderState = READER_READY;
}

/**
 * @brief  读卡器模式命令处理总入口
 * @param  NONE
 * @return NONE
 */
void OTG_DeviceStorProcess(USB_DeviceType_t dev)
{
	DeviceCtx_t *ctx = Device_GetCtx(dev);
	UsbStorClass* pMSC = ((UsbCompositeClass*)ctx->pClassData)->pMSC;
	int8_t ret = -1;
	uint32_t DataLen = 0;

    if(ctx->state != DEV_STATE_CONFIGURED) return;

	if(ctx->bus_ops.BulkReceive(pMSC->BulkOutEp, (uint8_t*)&pMSC->gCBW, 31, &DataLen, 1) != DEVICE_NONE_ERR)
	{
		return;
	}
	if(memcmp(pMSC->gCBW, "USBC", 4))
	{
		DBG("(memcmp(gCBW, USBC, 4) != 0\n");
		return;
	}
	// set CSW tag
	pMSC->gCSW[4] = pMSC->gCBW[4];
	pMSC->gCSW[5] = pMSC->gCBW[5];
	pMSC->gCSW[6] = pMSC->gCBW[6];
	pMSC->gCSW[7] = pMSC->gCBW[7];

	switch(pMSC->gCBW[15])
	{
		case INQUIRY:
//			DBG("INQUIRY\n");
			pMSC->sReaderState = READER_INQUIRY;
			ctx->bus_ops.BulkSend(pMSC->BulkInEp, (uint8_t*)gInquiryData, sizeof(gInquiryData), 10000);
			ret = 0;
			break;

		case READ_FORMAT_CAPACITY:
//			DBG("READ_FORMAT_CAPACITY\n");
			pMSC->sReaderState = READER_READ_FORMAT_CAPACITY;
			ret = DeviceStorReadFmtCapacity(ctx);
			break;

		case READ_CAPACITY:
//			DBG("READ_CAPACITY\n");
			pMSC->sReaderState = READER_READ_CAPACITY;
			ret = DeviceStorReadCapacity(ctx);
			break;

		case READ_10:
//			DBG("R\n");//DBG("R10\n");
			pMSC->sReaderState = READER_READ;
			ret = DeviceStorRead10(ctx);
//			DBG("End\n");
			break;

		case WRITE_10:
//			DBG("W\n");//DBG("W10\n");
			pMSC->sReaderState = READER_WIRTE;
			ret = DeviceStorWrite10(ctx);
			TimeOutSet(&pMSC->DeviceStorPreventTimer, 2000);
//			DBG("End\n");
			break;

		case TEST_UNIT_READY:
			if(!pMSC->ops->DeviceLink())
			{
				pMSC->sReaderState = READER_UNREADY;
				pMSC->DeviceStorStoppedFlag = FALSE;	//卡被拔除后，清楚“弹出标志”，以便下次插上卡时，能够被电脑识别到
				ret = -1;
			}
			else
			{
				if(pMSC->DeviceStorStoppedFlag)		//卡未拔除，但在电脑上执行了“弹出”命令
				{
					pMSC->sReaderState = READER_UNREADY;
					ret = -1;
				}
				else
				{
					pMSC->sReaderState = READER_READY;
					ret = 0;
				}
			}
			break;

		case MODE_SENSE:
//			DBG("MODE_SENSE\n");
			ret = DeviceStorModeSense(ctx);
			break;

		case ALLOW_MEDIUM_REMOVAL:
//			DBG("ALLOW_MEDIUM_REMOVAL\n");
			if((pMSC->gCBW[19] == 0) && pMSC->ops->DeviceLink())
			{
				DBG("allow\n");
				pMSC->sReaderState = READER_NOT_ALLOW_REMOVAL;
				pMSC->DeviceStorPreventFlag = FALSE;
				ret = 0;
			}
			else
			{
				DBG("prevent\n");
				pMSC->sReaderState = READER_ALLOW_REMOVAL;
				pMSC->DeviceStorPreventFlag = TRUE;
				ret = 0;
			}
			break;

		case REQUEST_SENSE:
//			DBG("REQUEST_SENSE\n");
			if((!pMSC->ops->DeviceLink()) || pMSC->DeviceStorStoppedFlag)
			{
				ctx->bus_ops.BulkSend(pMSC->BulkInEp, (uint8_t*)gRequestSenseNotReady, sizeof(gRequestSenseNotReady), 10000);
			}
			else
			{
				ctx->bus_ops.BulkSend(pMSC->BulkInEp, (uint8_t*)gRequestSenseReady, sizeof(gRequestSenseReady), 10000);
			}
			ret = 0;
			break;

		case VERYFY:
//			DBG("VERYFY\n");
			ret = 0;
			break;

		case START_STOP_UNIT:
			//DBG("START_STOP_UNIT\n");
			if(pMSC->gCBW[19] == 0)
			{
				//DBG("STOP_UNIT\n");
				pMSC->DeviceStorStoppedFlag = TRUE;
				pMSC->sReaderState = READER_UNREADY;
				ret = 0;
			}
			else if(pMSC->gCBW[19] == 1)
			{
				//DBG("START_UNIT\n");
				pMSC->DeviceStorStoppedFlag = FALSE;
				pMSC->sReaderState = READER_READY;
				ret = 0;
			}
			break;

		default:
			DBG("default\n");
			uint32_t temp = 0;
			temp = ((pMSC->gCBW[8]) | (pMSC->gCBW[9]<<8) | (pMSC->gCBW[10]<<16) | (pMSC->gCBW[11]<<24));
			if(temp > 0)
			{
				DBG("CMD %x send DATA %ld\n",pMSC->gCBW[15],temp);
				ctx->bus_ops.BulkSend(pMSC->BulkInEp, NULL, 0, 10000);
			}
			ret = -1;
			break;
	}

	DeviceStorSendCSW(ctx, ret < 0 ? STOR_CSW_CMD_FAILED : STOR_CSW_CMD_PASSED);
	//如果1.2秒没有接收到命令，则认为PC端已安全删除设备
	pMSC->DeviceStorCmdFlag = TRUE;
	TimeOutSet(&pMSC->DeviceStorCmdTimer, 1500);
}
#endif
