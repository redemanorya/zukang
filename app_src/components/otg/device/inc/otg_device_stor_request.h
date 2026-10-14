/**
 *****************************************************************************
 * @file     otg_device_stor.h
 * @author   Owen
 * @version  V1.0.0
 * @date     24-June-2015
 * @brief    stor device interface
 *****************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
 */

#ifndef __OTG_DEVICE_STOR_REQUEST_H__
#define	__OTG_DEVICE_STOR_REQUEST_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus
	
#include "type.h"
#include "timeout.h"
#include "otg_device.h"
// SCSI opcodes
#define TEST_UNIT_READY				0x00
#define REQUEST_SENSE				0x03
#define READ_6						0x08
#define WRITE_6						0x0A
#define SEEK_6						0x0B
#define INQUIRY						0x12
#define MODE_SELECT					0x15
#define MODE_SENSE					0x1A
#define START_STOP_UNIT				0x1B
#define ALLOW_MEDIUM_REMOVAL		0x1E
#define	READ_FORMAT_CAPACITY		0x23
#define READ_CAPACITY				0x25
#define READ_10						0x28
#define WRITE_10					0x2A
#define SEEK_10						0x2B
#define VERYFY						0x2F
#define MODE_SELECT_10				0x55
#define MODE_SENSE_10				0x5A
#define READ_12						0xA8
#define WRITE_12					0xAA
#define READ_DISC_STRUCTURE			0xAD
#define DVD_MACHANISM_STATUS  		0xBD
#define REPORT_KEY			  		0xA4
#define SEND_KEY			  		0xA3
#define READ_TOC			  		0x43
#define READ_MSF			  		0xB9



#define READER_UNREADY				0 // 未就绪
#define READER_INIT					1 // 初始化
#define READER_READY				2 // 就绪
#define READER_READ					3 // 读数据
#define READER_WIRTE				4 // 写数据
#define READER_INQUIRY				5 // 查询读卡器信息
#define READER_READ_FORMAT_CAPACITY 6 // 查询格式化容量
#define READER_READ_CAPACITY		7 // 查询磁盘容量
#define READER_NOT_ALLOW_REMOVAL	8 // 不允许删除设备
#define READER_ALLOW_REMOVAL		9 // 允许删除设备

#define STOR_CSW_CMD_PASSED         0x00
#define STOR_CSW_CMD_FAILED         0x01

#define STOR_CBW_SIGNATURE             0x43425355U
#define STOR_CSW_SIGNATURE             0x53425355U


#pragma pack(1)
/**
 * command block wrapper
 */
typedef struct _BOT_CBW
{
	uint32_t	Signature;
	uint32_t	Tag;
	uint32_t	DataTransferLength;
	uint8_t		Flags;
	uint8_t		Lun;
	uint8_t		Length;
	uint8_t		CDB[16];
} BOT_CBW;
#pragma pack()
/**
 * command status wrapper
 */
typedef struct _BOT_CSW
{
	uint32_t	Signature;
	uint32_t	Tag;
	uint32_t	Residue;
	uint8_t		Status;
} BOT_CSW;


typedef struct {
	bool (*DeviceLink)(void);
	uint32_t (*CapacityGet)(void);
    uint32_t (*get_block_size)(void);
    int8_t (*read_block)(DeviceCtx_t *ctx, uint32_t lba, uint16_t cnt);
    int8_t (*Write_block)(DeviceCtx_t *ctx, uint32_t lba, uint16_t cnt);
} BlockDevOps;

typedef struct _UsbStor
{
	uint8_t BulkInEp;
	uint8_t BulkOutEp;
	uint8_t gCBW[31];
	uint8_t gCSW[13];
	bool DeviceStorPreventFlag;
	TIMER DeviceStorPreventTimer;
	bool DeviceStorStoppedFlag;
	bool DeviceStorCmdFlag;
	TIMER DeviceStorCmdTimer;
	bool DeviceStorIsCardInitOK;
	uint8_t sReaderState;

	BlockDevOps *ops;
}UsbStorClass;

#define	DEVICE_STOR_BLOCK_SIZE	512
void OTG_DeviceStorStateInit(UsbStorClass* pMSC);
void OTG_DeviceStorProcess(USB_DeviceType_t dev);
extern BlockDevOps sd_ops;

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif
