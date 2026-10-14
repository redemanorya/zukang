/**
 *****************************************************************************
 * @file     otg_device_stor_sdcard.c
 * @author   Shanks
 * @version  V1.0.0
 * @date     7-September-2015
 * @brief    USB Mass Storage SD卡设备驱动
 *****************************************************************************
 * @attention
 * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
 *****************************************************************************
 * @brief
 *  1. 实现 USB 大容量存储设备底层 SD 卡物理层驱动
 *  2. 基于 SDIO 接口 + DMA 双缓冲，实现高速 Read10 / Write10 数据传输
 *  3. 提供标准块设备操作接口（检测、容量、读写）
 *****************************************************************************
 */

#include "app_config.h"
#include <string.h>
#include "type.h"
#include "timeout.h"
#include "debug.h"
#include "delay.h"
#include "otg_device_hcd.h"
#include "sd_card.h"
#include "sdio.h"
#include "dma.h"
#include "gpio.h"
#include "reset.h"

#include "device_detect.h"
#include "otg_device.h"
#include "otg_device_stor_request.h"
#ifdef FUNC_OS_EN
#include "rtos_api.h" //add for mutex declare
#endif

#if defined CFG_OTG_MODE_READER_EN || defined CFG_OTG_FS_MODE_READER_EN

extern SD_CARD		    SDCard;

uint32_t CARD_BUF_A[512/4];  /**< SD卡双缓冲A */
uint32_t CARD_BUF_B[512/4];  /**< SD卡双缓冲B */

/**
 * @brief  检测SD卡设备是否连接在位
 * @param  None
 * @return true-已连接 false-未连接
 */
bool SDCard_DeviceLink(void)
{
	if(GetCardState() != DETECT_STATE_IN)
	{
		return 0;
	}
	return 1;
}

//// 获取设备总容量（块数量）sd_card.c存在
//uint32_t SDCard_CapacityGet(void)
//{
//
//}

/**
 * @brief  获取SD卡块大小
 * @param  None
 * @return 块大小（固定512字节）
 */
uint32_t SDCard_GetBlockSize(void)
{
	return SD_BLOCK_SIZE;
}

/**
 * @brief  SCSI Read10 命令完整处理流程（SD卡读取 + USB Bulk 发送）
 * @param  ctx: USB设备上下文指针，包含总线操作接口
 * @param  ReqLBA: 读取起始逻辑块地址
 * @param  ReqCnt: 需要读取的块数量
 * @return 0-成功，-1-失败
 * @note   内部处理SDIO多块读、DMA双缓冲、USB BulkIn发送
 */
int8_t SDCard_Read10_Process(DeviceCtx_t *ctx, uint32_t ReqLBA, uint16_t ReqCnt)
{
	TIMER Timer;
	if(!SDCard.IsSDHC)
	{
		ReqLBA *= SD_BLOCK_SIZE;
	}
#ifdef FUNC_OS_EN
	osMutexLock(SDIOMutex);
#endif
	DMA_ChannelDisable(PERIPHERAL_0_ID_SDIO_RX);
	DMA_InterruptFlagClear(PERIPHERAL_0_ID_SDIO_RX, DMA_DONE_INT);
	DMA_BlockConfig(PERIPHERAL_0_ID_SDIO_RX);
	DMA_BlockBufSet(PERIPHERAL_0_ID_SDIO_RX, (ReqCnt % 2) ? CARD_BUF_A : CARD_BUF_B, SD_BLOCK_SIZE);
#ifdef CFG_SDIO_BYTE_MODE_ENABLE
	DMA_ConfigDwidth(PERIPHERAL_0_ID_SDIO_RX, DMA_DWIDTH_BYTE);
#endif
	DMA_ChannelEnable(PERIPHERAL_0_ID_SDIO_RX);
	SDIO_MultiBlockDisable();
	SDIO_AutoKillRXClkEnable();
	SDIO_SingleBlockConfig(SDIO_DIR_RX,SD_BLOCK_SIZE);
	SDIO_DataTransfer(1);
	SDIO_CmdSend(CMD18_READ_MULTIPLE_BLOCK, ReqLBA, 20);
	TimeOutSet(&Timer, 2500);
	while(!DMA_InterruptFlagGet(PERIPHERAL_0_ID_SDIO_RX, DMA_DONE_INT) && !IsTimeOut(&Timer));
	if(!IsTimeOut(&Timer))//for SD Plus out
	{
		DMA_InterruptFlagClear(PERIPHERAL_0_ID_SDIO_RX, DMA_DONE_INT);

		while(ReqCnt-- > 1)
		{
			SDIO_DataTransfer(0);
			DMA_ChannelDisable(PERIPHERAL_0_ID_SDIO_RX);
			DMA_InterruptFlagClear(PERIPHERAL_0_ID_SDIO_RX, DMA_DONE_INT);
			DMA_BlockConfig(PERIPHERAL_0_ID_SDIO_RX);
			DMA_BlockBufSet(PERIPHERAL_0_ID_SDIO_RX, (ReqCnt % 2) ? CARD_BUF_A : CARD_BUF_B, SD_BLOCK_SIZE);//0,2,4,6 A 1357B
#ifdef CFG_SDIO_BYTE_MODE_ENABLE
			DMA_ConfigDwidth(PERIPHERAL_0_ID_SDIO_RX, DMA_DWIDTH_BYTE);
#endif
			DMA_ChannelEnable(PERIPHERAL_0_ID_SDIO_RX);
			SDIO_DataTransfer(1);
			SDIO_ClkEnable();
			ctx->bus_ops.BulkSend(DEVICE_BULK_IN_EP, (ReqCnt % 2) ? (uint8_t*)CARD_BUF_B : (uint8_t*)CARD_BUF_A, 512, 10000);//0,2,4,6 B 1357A
			TimeOutSet(&Timer, 250);
			while(!DMA_InterruptFlagGet(PERIPHERAL_0_ID_SDIO_RX, DMA_DONE_INT) && !IsTimeOut(&Timer))
			{
				;
			}
			if(IsTimeOut(&Timer))//for SD Plus out
			{
				break;
			}
			DMA_InterruptFlagClear(PERIPHERAL_0_ID_SDIO_RX, DMA_DONE_INT);
		}
		ctx->bus_ops.BulkSend(DEVICE_BULK_IN_EP, (ReqCnt % 2) ? (uint8_t*)CARD_BUF_B : (uint8_t*)CARD_BUF_A, 512, 10000);
		SDIO_DataTransfer(0);
		SDIO_ClkEnable();
		SDIO_CmdDoneCheckBusy(1);
		SDIO_CmdSend(CMD12_STOP_TRANSMISSION, 0, 300);
	}

	SDIO_CmdDoneCheckBusy(0);
	SDIO_DataTransfer(0);
	SDIO_ClkDisable();
	Reset_Function1Reset(SDIO_FUNC_SEPA);
#ifdef FUNC_OS_EN
	osMutexUnlock(SDIOMutex);
#endif
	return 0;
}




/**
 * @brief  SCSI Write10 命令完整处理流程（USB Bulk 接收 + SD卡写入）
 * @param  ctx: USB设备上下文指针，包含总线操作接口
 * @param  ReqLBA: 写入起始逻辑块地址
 * @param  ReqCnt: 需要写入的块数量
 * @return 0-成功，-1-失败
 * @note   内部处理USB BulkOut接收、SDIO多块写、DMA双缓冲
 */
int8_t SDCard_Write10_Process(DeviceCtx_t *ctx, uint32_t ReqLBA, uint16_t ReqCnt)
{
	TIMER Timer;
	uint32_t DataLength = 0;
	if(!SDCard.IsSDHC)
	{
		ReqLBA *= SD_BLOCK_SIZE;
	}
#ifdef FUNC_OS_EN
	osMutexLock(SDIOMutex);
#endif
	Reset_Function1Reset(SDIO_FUNC_SEPA);
	while(SDIO_IsDataLineBusy());
	SDIO_ClearClkHalt();
	SDIO_MultiBlockDisable();
	SDIO_AutoKillTXClkEnable();
	SDIO_DataTransfer(0);
	ctx->bus_ops.BulkReceive(DEVICE_BULK_OUT_EP, (ReqCnt%2) ? (uint8_t*)CARD_BUF_A : (uint8_t*)CARD_BUF_B, 512, &DataLength, 10000);
	SDIO_CmdSend(CMD25_WRITE_MULTIPLE_BLOCK, ReqLBA, 50);
	while(ReqCnt--)
	{
		while(SDIO_IsDataLineBusy())
		{
			;
		}
		SDIO_DataTransfer(0);
		DMA_ChannelDisable(PERIPHERAL_0_ID_SDIO_TX);
		DMA_InterruptFlagClear(PERIPHERAL_0_ID_SDIO_TX, DMA_DONE_INT);
		DMA_BlockConfig(PERIPHERAL_0_ID_SDIO_TX);
		DMA_BlockBufSet(PERIPHERAL_0_ID_SDIO_TX, (uint8_t*)((ReqCnt%2) ? (uint8_t*)CARD_BUF_B : (uint8_t*)CARD_BUF_A), SD_BLOCK_SIZE);
#ifdef CFG_SDIO_BYTE_MODE_ENABLE
		DMA_ConfigDwidth(PERIPHERAL_0_ID_SDIO_TX, DMA_DWIDTH_BYTE);
#endif
		DMA_ChannelEnable(PERIPHERAL_0_ID_SDIO_TX);
		SDIO_SingleBlockConfig(SDIO_DIR_TX,SD_BLOCK_SIZE);
		SDIO_DataTransfer(1);
		SDIO_ClkEnable();

		if(ReqCnt != 0)
		{
			ctx->bus_ops.BulkReceive(DEVICE_BULK_OUT_EP, (ReqCnt % 2) ? (uint8_t*)CARD_BUF_A : (uint8_t*)CARD_BUF_B, 512, &DataLength, 10000);
		}
		TimeOutSet(&Timer, 500);
		while(!IsTimeOut(&Timer) && (!DMA_InterruptFlagGet(PERIPHERAL_0_ID_SDIO_TX, DMA_DONE_INT) || !SDIO_DataIsDone()))
		{
				;
		}
		if(IsTimeOut(&Timer))//for SD Plus out
		{
			DBG("sdio Write10 time out!!!\n");
			break;
		}
		DMA_InterruptFlagClear(PERIPHERAL_0_ID_SDIO_TX, DMA_DONE_INT);
	}
	SDIO_DataTransfer(0);
	SDIO_ClkDisable();

	SDIO_ClearClkHalt();
	SDIO_CmdDoneCheckBusy(1);
	SDIO_CmdSend(CMD12_STOP_TRANSMISSION, 0, 300);
	SDIO_CmdDoneCheckBusy(0);
#ifdef FUNC_OS_EN
	osMutexUnlock(SDIOMutex);
#endif
	Reset_Function1Reset(SDIO_FUNC_SEPA);
	while(SDIO_IsDataLineBusy())
	{
		DBG("SDIO_IsDataLineBusy 2\n");
	}
	//DBG("6");
	return 0;
}

/**
 * @brief SD卡块设备操作接口集合
 */
BlockDevOps sd_ops = {
    .DeviceLink     = SDCard_DeviceLink,
    .CapacityGet    = SDCard_CapacityGet,
    .get_block_size = SDCard_GetBlockSize,
    .read_block     = SDCard_Read10_Process,
    .Write_block    = SDCard_Write10_Process,
};

#endif
