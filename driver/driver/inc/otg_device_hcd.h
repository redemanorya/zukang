/**
 *****************************************************************************
 * @file     otg_device_hcd.h
 * @author   Owen
 * @version  V1.0.0
 * @date     27-03-2017
 * @brief    device module driver interface
 *****************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
 */
 
/**
 * @addtogroup OTG
 * @{
 * @defgroup otg_device_hcd otg_device_hcd.h
 * @{
 */
 
#ifndef __OTG_DEVICE_HCD_H__
#define	__OTG_DEVICE_HCD_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#include "type.h"
#ifdef CFG_APP_CONFIG
	#include "app_config.h"
#endif

// Max packet size. Fixed, user should not modify.
#define	DEVICE_FS_CONTROL_MPS		64
#define	DEVICE_FS_INT_IN_MPS		64
#define	DEVICE_FS_INT_OUT_MPS		64
#define	DEVICE_FS_BULK_IN_MPS		64
#define	DEVICE_FS_BULK_OUT_MPS		64
//#define	DEVICE_FS_ISO_IN_MPS		192
//#define	DEVICE_FS_ISO_OUT_MPS		192


#define	DEVICE_HS_CONTROL_MPS		64
#define	DEVICE_HS_INT_IN_MPS		512
#define	DEVICE_HS_INT_OUT_MPS		512
#define	DEVICE_HS_BULK_IN_MPS		512
#define	DEVICE_HS_BULK_OUT_MPS		512
//#define	DEVICE_HS_ISO_IN_MPS		151
//#define	DEVICE_HS_ISO_OUT_MPS		151

// Endpoint number. Fixed, user should not modify.
//HS FS
#define	DEVICE_CONTROL_EP			0x00
#define	DEVICE_BULK_IN_EP			0x81
#define	DEVICE_BULK_OUT_EP			0x01
#define	DEVICE_INT_IN_EP			0x82
#define	DEVICE_INT_OUT_EP			0x02
#define	DEVICE_ISO_IN_EP			0x83
#define	DEVICE_ISO_OUT_EP			0x03
#define	DEVICE_ISO_IN_EP1			0x81
#define	DEVICE_ISO_OUT_EP1			0x01
#define	DEVICE_ISO_FEEDBACK_IN_EP	0x81
#define	DEVICE_ISO_FEEDBACK_OUT_EP	0x01
//HS
#define	DEVICE_INT_IN_EP1			0x84
#define	DEVICE_INT_OUT_EP1			0x04
#define	DEVICE_INT_IN_EP2			0x85
#define	DEVICE_INT_OUT_EP2			0x05

typedef enum _DEVICE_TRANSFER_TYPE
{
    TYPE_SETUP 			= 0xFF,
    TYPE_CONTROL_IN		= 0xFE,
    TYPE_CONTROL_OUT	= 0xFD,
	TYPE_ISO_IN 		= 0x81,
	TYPE_ISO_OUT 		= 0x01,
	TYPE_BULK_IN 		= 0x82,
	TYPE_BULK_OUT 		= 0x02,
	TYPE_INT_IN 		= 0x83,
	TYPE_INT_OUT 		= 0x03,
} DEVICE_TRANSFER_TYPE;




typedef enum _OTG_DEVICE_ERR_CODE
{
	DEVICE_NONE_ERR = 0,
	DEVICE_UNLINK_ERR,
	
	CONTROL_SEND_ERR,
	CONTROL_RCV_ERR,
	CONTROL_SETUP_ERR,
	
	BULK_SEND_ERR,
	BULK_RCV_ERR0,
	BULK_RCV_ERR1,
	BULK_RCV_ERR2,

	INT_SEND_ERR,
	INT_RCV_ERR0,
	INT_RCV_ERR1,
	INT_RCV_ERR2,
}OTG_DEVICE_ERR_CODE;

// ---------------------------- FS 设备模式函数声明 ----------------------------
/**
 * @brief  Device initial
 * @param  NONE
 * @return NONE
 */
void OTG_FS_DeviceInit(void);

/**
 * @brief  Device software connect.
 * @param  NONE
 * @return NONE
 */
void OTG_FS_DeviceConnect(void);

/**
 * @brief  Device software disconnect.
 * @param  NONE
 * @return NONE
 */
void OTG_FS_DeviceDisConnect(void);

/**
 * @brief  返回STALL应答
 * @param  EndpointNum 端点号
 * @return NONE
 */
void OTG_FS_DeviceStallSend(uint8_t EndpointNum);

/**
 * @brief  复位某个端点
 * @param  EndpointNum 端点号
 * @param  EndpointType 端点类型
 * @return NONE
 */
void OTG_FS_DeviceEndpointReset(uint8_t EndpointNum, uint8_t EndpointType);

/**
 * @brief  设置端点包大小
 */
void OTG_FS_DeviceEndpointPacketSizeSet(uint8_t EndpointNum, uint16_t EndpointSize);

/**
 * @brief  设置设备地址
 * @brief  总线复位后，设置设备地址为0，后续根据PC端发的SetAddress命令设置指定的设备地址
 * @param  Address 设备地址
 * @return NONE
 */
void OTG_FS_DeviceAddressSet(uint8_t Address);

/**
 * @brief  获取总线上的事件
 * @param  NONE
 * @return 总线事件编码，USB_RESET-总线复位事件，...
 */
uint8_t OTG_FS_DeviceBusEventGet(void);

/**
 * @brief  从控制端点发送数据
 * @param  Buf 数据缓冲区指针
 * @param  Len 数据长度
 * @param  TimeOut超时时间，ms
 * @return OTG_DEVICE_ERR_CODE
 */
OTG_DEVICE_ERR_CODE OTG_FS_DeviceControlSend(uint8_t* Buf, uint32_t Len, uint32_t TimeOut);

/**
 * @brief  从控制端点接收OUT数据
 * @param  Buf 数据缓冲区指针
 * @param  MaxLen 最大数据长度
 * @param  *pTransferLen实际接收到的数据长度
 * @param  TimeOut超时时间，ms
 * @return OTG_DEVICE_ERR_CODE
 */
OTG_DEVICE_ERR_CODE OTG_FS_DeviceControlReceive(uint8_t* Buf, uint32_t MaxLen, uint32_t *pTransferLen, uint32_t TimeOut);

/**
 * @brief  从控制端点接收SETUP数据
 * @param  Buf 数据缓冲区指针
 * @param  MaxLen 最大数据长度，一般SETUP数据包长度都是8字节
 * @param  *pTransferLen实际接收到的数据长度
 * @return OTG_DEVICE_ERR_CODE
 */
OTG_DEVICE_ERR_CODE OTG_FS_DeviceSetupReceive(uint8_t* Buf, uint32_t MaxLen ,uint32_t *pTransferLen);

/**
 * @brief  从BULK IN端点发送数据
 * @param  EpNum 端点号
 * @param  Buf 数据缓冲区指针
 * @param  Len 数据长度
 * @param  TimeOut超时时间，ms
 * @return OTG_DEVICE_ERR_CODE
 */
OTG_DEVICE_ERR_CODE OTG_FS_DeviceBulkSend(uint8_t EpNum, uint8_t* Buf, uint32_t Len, uint32_t TimeOut);

/**
 * @brief  从BULK OUT端点接收数据
 * @param  EpNum 端点号
 * @param  Buf 数据缓冲区指针
 * @param  MaxLen 最大数据长度
 * @param  *pTransferLen实际接收到的数据长度
 * @param  TimeOut超时时间，ms
 * @return OTG_DEVICE_ERR_CODE
 */
OTG_DEVICE_ERR_CODE OTG_FS_DeviceBulkReceive(uint8_t EpNum, uint8_t* Buf, uint16_t MaxLen, uint32_t *pTransferLen ,uint32_t TimeOut);

/**
 * @brief  从ISO IN端点发送数据
 * @param  EpNum 端点号
 * @param  Buf 数据缓冲区指针
 * @param  Len 数据长度，不能大于DEVICE_FS_ISO_IN_MPS字节
 * @return OTG_DEVICE_ERR_CODE
 */
OTG_DEVICE_ERR_CODE OTG_FS_DeviceISOSend(uint8_t EpNum, uint8_t* Buf, uint32_t Len);

/**
 * @brief  从ISO OUT端点接收数据
 * @param  EpNum 端点号
 * @param  Buf 数据缓冲区指针
 * @param  MaxLen 最大数据长度
 * @param  *pTransferLen实际接收到的数据长度
 * @return OTG_DEVICE_ERR_CODE
 */
OTG_DEVICE_ERR_CODE OTG_FS_DeviceISOReceive(uint8_t EpNum, uint8_t* Buf, uint32_t MaxLen ,uint32_t *pTransferLen);

/**
 * @brief  从INT IN端点发送数据
 * @param  EpNum 端点号
 * @param  Buf 数据缓冲区指针
 * @param  Len 数据长度，不能大于DEVICE_FS_BULK_IN_MPS字节
 * @param  TimeOut超时时间，ms
 * @return OTG_DEVICE_ERR_CODE
 */
OTG_DEVICE_ERR_CODE OTG_FS_DeviceInterruptSend(uint8_t EpNum, uint8_t* Buf, uint32_t Len, uint32_t TimeOut);

/**
 * @brief  从INT OUT端点接收数据
 * @param  EpNum 端点号
 * @param  Buf 数据缓冲区指针
 * @param  MaxLen 最大数据长度
 * @param  *pTransferLen实际接收到的数据长度
 * @param  TimeOut超时时间，ms
 * @return OTG_DEVICE_ERR_CODE
 */
OTG_DEVICE_ERR_CODE OTG_FS_DeviceInterruptReceive(uint8_t EpNum, uint8_t* Buf, uint16_t MaxLen, uint32_t *pTransferLen ,uint32_t TimeOut);

/**
 * @brief  USB FS模块中断入口
 * @brief  在此函数中判断是具体哪一个端点的中断，然后调用相应端点的中断回调函数
 * @param  NONE
 * @return NONE
 */
void UsbFsInterrupt(void);

/**
 * @brief  DEVICE模式下使能某个端点中断
 * @param  Pipe Pipe指针
 * @param  Func 中断回调函数指针
 * @return NONE
 */
void OTG_FS_EndpointInterruptEnable(uint8_t EpNum, FPCALLBACK Func);

/**
 * @brief  DEVICE模式下禁止某个端点中断
 * @param  EpNum 端点号
 * @return NONE
 */
void OTG_FS_EndpointInterruptDisable(uint8_t EpNum);

/**
 * @brief  DEVICE模式下使能某个事件中断
 * @param  事件id
 * @param  Func 中断回调函数指针
 * @return NONE
 */
void OTG_FS_BusEventInterruptEnable(uint8_t EventId, FPCALLBACK Func);

/**
 * @brief  DEVICE模式下禁止某个事件中断
 * @param  事件id
 * @return NONE
 */
void OTG_FS_BusEventInterruptDisable(uint8_t EventId);

/**
 * @brief  OTG PowerDown
 * @param  NONE
 * @return NONE
 */
void OTG_FS_PowerDown(void);

/**
 * @brief  OTG Pause detect & Backup reg &Powerdown
 * @param  NONE
 * @return NONE
 */
void OTG_FS_DeepSleepBackup(void);

/**
 * @brief  OTG resume Reg/PowerOn & Detect
 * @param  NONE
 * @return NONE
 */
void OTG_FS_WakeupResume(void);

void OTG_FS_DeviceFifoInit(void);

// ---------------------------- HS 设备模式函数声明 ----------------------------
/**
 * @brief  Device initial
 * @param  NONE
 * @return NONE
 */
void OTG_HS_DeviceInit(void);

/**
 * @brief  Device software connect.
 * @param  NONE
 * @return NONE
 */
void OTG_HS_DeviceConnect(void);

/**
 * @brief  Device software disconnect.
 * @param  NONE
 * @return NONE
 */
void OTG_HS_DeviceDisConnect(void);

/**
 * @brief  返回STALL应答
 * @param  EndpointNum 端点号
 * @return NONE
 */
void OTG_HS_DeviceStallSend(uint8_t EndpointNum);

/**
 * @brief  复位某个端点
 * @param  EndpointNum 端点号
 * @param  EndpointType 端点类型
 * @return NONE
 */
void OTG_HS_DeviceEndpointReset(uint8_t EndpointNum, uint8_t EndpointType);

/**
 * @brief  Device模式下Port端口上设备速度获取
 * @param  NONE
 * @return 2-FS设备连接，3-HS设备连接
 */
uint8_t OTG_HS_DevicePortSpeedGet(void);
/**
 * @brief  设置端点包大小
 */
void OTG_HS_DeviceEndpointPacketSizeSet(uint8_t EndpointNum, uint16_t EndpointSize);

/**
 * @brief  设置设备地址
 * @brief  总线复位后，设置设备地址为0，后续根据PC端发的SetAddress命令设置指定的设备地址
 * @param  Address 设备地址
 * @return NONE
 */
void OTG_HS_DeviceAddressSet(uint8_t Address);

/**
 * @brief  获取总线上的事件
 * @param  NONE
 * @return 总线事件编码，USB_RESET-总线复位事件，...
 */
uint8_t OTG_HS_DeviceBusEventGet(void);

/**
 * @brief  从控制端点发送数据
 * @param  Buf 数据缓冲区指针
 * @param  Len 数据长度
 * @param  TimeOut超时时间，ms
 * @return OTG_DEVICE_ERR_CODE
 */
OTG_DEVICE_ERR_CODE OTG_HS_DeviceControlSend(uint8_t* Buf, uint32_t Len, uint32_t TimeOut);

/**
 * @brief  从控制端点接收OUT数据
 * @param  Buf 数据缓冲区指针
 * @param  MaxLen 最大数据长度
 * @param  *pTransferLen实际接收到的数据长度
 * @param  TimeOut超时时间，ms
 * @return OTG_DEVICE_ERR_CODE
 */
OTG_DEVICE_ERR_CODE OTG_HS_DeviceControlReceive(uint8_t* Buf, uint32_t MaxLen, uint32_t *pTransferLen, uint32_t TimeOut);

/**
 * @brief  从控制端点接收SETUP数据
 * @param  Buf 数据缓冲区指针
 * @param  MaxLen 最大数据长度，一般SETUP数据包长度都是8字节
 * @param  *pTransferLen实际接收到的数据长度
 * @return OTG_DEVICE_ERR_CODE
 */
OTG_DEVICE_ERR_CODE OTG_HS_DeviceSetupReceive(uint8_t* Buf, uint32_t MaxLen ,uint32_t *pTransferLen);

/**
 * @brief  从BULK IN端点发送数据
 * @param  EpNum 端点号
 * @param  Buf 数据缓冲区指针
 * @param  Len 数据长度
 * @param  TimeOut超时时间，ms
 * @return OTG_DEVICE_ERR_CODE
 */
OTG_DEVICE_ERR_CODE OTG_HS_DeviceBulkSend(uint8_t EpNum, uint8_t* Buf, uint32_t Len, uint32_t TimeOut);

/**
 * @brief  从BULK OUT端点接收数据
 * @param  EpNum 端点号
 * @param  Buf 数据缓冲区指针
 * @param  MaxLen 最大数据长度
 * @param  *pTransferLen实际接收到的数据长度
 * @param  TimeOut超时时间，ms
 * @return OTG_DEVICE_ERR_CODE
 */
OTG_DEVICE_ERR_CODE OTG_HS_DeviceBulkReceive(uint8_t EpNum, uint8_t* Buf, uint16_t MaxLen, uint32_t *pTransferLen ,uint32_t TimeOut);

/**
 * @brief  从ISO IN端点发送数据
 * @param  EpNum 端点号
 * @param  Buf 数据缓冲区指针
 * @param  Len 数据长度，不能大于DEVICE_HS_ISO_IN_MPS字节
 * @return OTG_DEVICE_ERR_CODE
 */
OTG_DEVICE_ERR_CODE OTG_HS_DeviceISOSend(uint8_t EpNum, uint8_t* Buf, uint32_t Len);

/**
 * @brief  从ISO OUT端点接收数据
 * @param  EpNum 端点号
 * @param  Buf 数据缓冲区指针
 * @param  MaxLen 最大数据长度
 * @param  *pTransferLen实际接收到的数据长度
 * @return OTG_DEVICE_ERR_CODE
 */
OTG_DEVICE_ERR_CODE OTG_HS_DeviceISOReceive(uint8_t EpNum, uint8_t* Buf, uint32_t MaxLen ,uint32_t *pTransferLen);

/**
 * @brief  从INT IN端点发送数据
 * @param  EpNum 端点号
 * @param  Buf 数据缓冲区指针
 * @param  Len 数据长度，不能大于DEVICE_HS_BULK_IN_MPS字节
 * @param  TimeOut超时时间，ms
 * @return OTG_DEVICE_ERR_CODE
 */
OTG_DEVICE_ERR_CODE OTG_HS_DeviceInterruptSend(uint8_t EpNum, uint8_t* Buf, uint32_t Len, uint32_t TimeOut);

/**
 * @brief  从INT OUT端点接收数据
 * @param  EpNum 端点号
 * @param  Buf 数据缓冲区指针
 * @param  MaxLen 最大数据长度
 * @param  *pTransferLen实际接收到的数据长度
 * @param  TimeOut超时时间，ms
 * @return OTG_DEVICE_ERR_CODE
 */
OTG_DEVICE_ERR_CODE OTG_HS_DeviceInterruptReceive(uint8_t EpNum, uint8_t* Buf, uint16_t MaxLen, uint32_t *pTransferLen ,uint32_t TimeOut);

/**
 * @brief  USB HS模块中断入口
 * @brief  在此函数中判断是具体哪一个端点的中断，然后调用相应端点的中断回调函数
 * @param  NONE
 * @return NONE
 */
void UsbHsInterrupt(void);

/**
 * @brief  DEVICE模式下使能某个端点中断
 * @param  Pipe Pipe指针
 * @param  Func 中断回调函数指针
 * @return NONE
 */
void OTG_HS_EndpointInterruptEnable(uint8_t EpNum, FPCALLBACK Func);

/**
 * @brief  DEVICE模式下禁止某个端点中断
 * @param  EpNum 端点号
 * @return NONE
 */
void OTG_HS_EndpointInterruptDisable(uint8_t EpNum);

/**
 * @brief  DEVICE模式下使能某个事件中断
 * @param  事件id
 * @param  Func 中断回调函数指针
 * @return NONE
 */
void OTG_HS_BusEventInterruptEnable(uint8_t EventId, FPCALLBACK Func);

/**
 * @brief  DEVICE模式下禁止某个事件中断
 * @param  事件id
 * @return NONE
 */
void OTG_HS_BusEventInterruptDisable(uint8_t EventId);

/**
 * @brief  OTG PowerDown
 * @param  NONE
 * @return NONE
 */
void OTG_HS_PowerDown(void);

/**
 * @brief  OTG Pause detect & Backup reg &Powerdown
 * @param  NONE
 * @return NONE
 */
void OTG_HS_DeepSleepBackup(void);

/**
 * @brief  OTG resume Reg/PowerOn & Detect
 * @param  NONE
 * @return NONE
 */
void OTG_HS_WakeupResume(void);

void OTG_HS_DeviceFifoInit(void);

// ---------------------------- 接口选择宏 ----------------------------
#ifdef OTG_DEVICE_PORT_FS
	#define	OTG_DeviceFifoInit													OTG_FS_DeviceFifoInit
	#define OTG_DeviceInit                          							OTG_FS_DeviceInit
	#define OTG_DeviceConnect                       							OTG_FS_DeviceConnect
	#define OTG_DeviceDisConnect                    							OTG_FS_DeviceDisConnect
	#define OTG_DeviceStallSend                     							OTG_FS_DeviceStallSend
	#define OTG_DeviceEndpointReset                 							OTG_FS_DeviceEndpointReset
	#define OTG_DeviceEndpointPacketSizeSet         							OTG_FS_DeviceEndpointPacketSizeSet
	#define OTG_DeviceAddressSet                    							OTG_FS_DeviceAddressSet
	#define OTG_DeviceBusEventGet                   							OTG_FS_DeviceBusEventGet
	#define OTG_DeviceControlSend                   							OTG_FS_DeviceControlSend
	#define OTG_DeviceControlReceive                							OTG_FS_DeviceControlReceive
	#define OTG_DeviceSetupReceive                  							OTG_FS_DeviceSetupReceive
	#define OTG_DeviceBulkSend                      							OTG_FS_DeviceBulkSend
	#define OTG_DeviceBulkReceive                   							OTG_FS_DeviceBulkReceive
	#define OTG_DeviceISOSend                       							OTG_FS_DeviceISOSend
	#define OTG_DeviceISOReceive                    							OTG_FS_DeviceISOReceive
	#define OTG_DeviceInterruptSend                 							OTG_FS_DeviceInterruptSend
	#define OTG_DeviceInterruptReceive              							OTG_FS_DeviceInterruptReceive
	#define OTG_DeviceEndpointInterruptEnable       							OTG_FS_EndpointInterruptEnable
	#define OTG_DeviceEndpointInterruptDisable      							OTG_FS_EndpointInterruptDisable
	#define OTG_DeviceBusEventInterruptEnable       							OTG_FS_BusEventInterruptEnable
	#define OTG_DeviceBusEventInterruptDisable      							OTG_FS_BusEventInterruptDisable
	#define OTG_PowerDown                           							OTG_FS_PowerDown
	#define OTG_DeepSleepBackup                     							OTG_FS_DeepSleepBackup
	#define OTG_WakeupResume                        							OTG_FS_WakeupResume
#else
	#define	OTG_DeviceFifoInit													OTG_HS_DeviceFifoInit
	#define OTG_DeviceInit                          							OTG_HS_DeviceInit
	#define OTG_DeviceConnect                       							OTG_HS_DeviceConnect
	#define OTG_DeviceDisConnect                    							OTG_HS_DeviceDisConnect
	#define OTG_DeviceStallSend                     							OTG_HS_DeviceStallSend
	#define OTG_DeviceEndpointReset                 							OTG_HS_DeviceEndpointReset
	#define OTG_DevicePortSpeedGet												OTG_HS_DevicePortSpeedGet
	#define OTG_DeviceEndpointPacketSizeSet         							OTG_HS_DeviceEndpointPacketSizeSet
	#define OTG_DeviceAddressSet                    							OTG_HS_DeviceAddressSet
	#define OTG_DeviceBusEventGet                   							OTG_HS_DeviceBusEventGet
	#define OTG_DeviceControlSend                   							OTG_HS_DeviceControlSend
	#define OTG_DeviceControlReceive                							OTG_HS_DeviceControlReceive
	#define OTG_DeviceSetupReceive                  							OTG_HS_DeviceSetupReceive
	#define OTG_DeviceBulkSend                      							OTG_HS_DeviceBulkSend
	#define OTG_DeviceBulkReceive                   							OTG_HS_DeviceBulkReceive
	#define OTG_DeviceISOSend                       							OTG_HS_DeviceISOSend
	#define OTG_DeviceISOReceive                    							OTG_HS_DeviceISOReceive
	#define OTG_DeviceInterruptSend                 							OTG_HS_DeviceInterruptSend
	#define OTG_DeviceInterruptReceive              							OTG_HS_DeviceInterruptReceive
	#define OTG_DeviceEndpointInterruptEnable       							OTG_HS_EndpointInterruptEnable
	#define OTG_DeviceEndpointInterruptDisable      							OTG_HS_EndpointInterruptDisable
	#define OTG_DeviceBusEventInterruptEnable       							OTG_HS_BusEventInterruptEnable
	#define OTG_DeviceBusEventInterruptDisable      							OTG_HS_BusEventInterruptDisable
	#define OTG_PowerDown                           							OTG_HS_PowerDown
	#define OTG_DeepSleepBackup                     							OTG_HS_DeepSleepBackup
	#define OTG_WakeupResume                        							OTG_HS_WakeupResume
#endif

// 通用OTG设备模式函数指针类型定义
typedef void (*OTG_DeviceInit_t)(void);
typedef void (*OTG_DeviceConnect_t)(void);
typedef void (*OTG_DeviceDisConnect_t)(void);
typedef void (*OTG_DeviceStallSend_t)(uint8_t EndpointNum);
typedef void (*OTG_DeviceEndpointReset_t)(uint8_t EndpointNum, uint8_t EndpointType);
typedef uint8_t (*OTG_DevicePortSpeedGet_t)(void);
typedef void (*OTG_DeviceEndpointPacketSizeSet_t)(uint8_t EndpointNum, uint16_t EndpointSize);
typedef void (*OTG_DeviceAddressSet_t)(uint8_t Address);
typedef uint8_t (*OTG_DeviceBusEventGet_t)(void);
typedef OTG_DEVICE_ERR_CODE (*OTG_DeviceControlSend_t)(uint8_t* Buf, uint32_t Len, uint32_t TimeOut);
typedef OTG_DEVICE_ERR_CODE (*OTG_DeviceControlReceive_t)(uint8_t* Buf, uint32_t MaxLen, uint32_t *pTransferLen, uint32_t TimeOut);
typedef OTG_DEVICE_ERR_CODE (*OTG_DeviceSetupReceive_t)(uint8_t* Buf, uint32_t MaxLen ,uint32_t *pTransferLen);
typedef OTG_DEVICE_ERR_CODE (*OTG_DeviceBulkSend_t)(uint8_t EpNum, uint8_t* Buf, uint32_t Len, uint32_t TimeOut);
typedef OTG_DEVICE_ERR_CODE (*OTG_DeviceBulkReceive_t)(uint8_t EpNum, uint8_t* Buf, uint16_t MaxLen, uint32_t *pTransferLen ,uint32_t TimeOut);
typedef OTG_DEVICE_ERR_CODE (*OTG_DeviceISOSend_t)(uint8_t EpNum, uint8_t* Buf, uint32_t Len);
typedef OTG_DEVICE_ERR_CODE (*OTG_DeviceISOReceive_t)(uint8_t EpNum, uint8_t* Buf, uint32_t MaxLen ,uint32_t *pTransferLen);
typedef OTG_DEVICE_ERR_CODE (*OTG_DeviceInterruptSend_t)(uint8_t EpNum, uint8_t* Buf, uint32_t Len, uint32_t TimeOut);
typedef OTG_DEVICE_ERR_CODE (*OTG_DeviceInterruptReceive_t)(uint8_t EpNum, uint8_t* Buf, uint16_t MaxLen, uint32_t *pTransferLen ,uint32_t TimeOut);
typedef void (*OTG_DeviceEndpointInterruptEnable_t)(uint8_t EpNum, FPCALLBACK Func);
typedef void (*OTG_DeviceEndpointInterruptDisable_t)(uint8_t EpNum);
typedef void (*OTG_DeviceBusEventInterruptEnable_t)(uint8_t EventId, FPCALLBACK Func);
typedef void (*OTG_DeviceBusEventInterruptDisable_t)(uint8_t EventId);
typedef void (*OTG_PowerDown_t)(void);
typedef void (*OTG_DeepSleepBackup_t)(void);
typedef void (*OTG_WakeupResume_t)(void);
#ifdef __cplusplus
}
#endif//__cplusplus

#endif //__OTG_DEVICE_HCD_H__
/**
 * @}
 * @}
 */
