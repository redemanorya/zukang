/**
 *****************************************************************************
 * @file     otg_host_hcd.h
 * @author   owen
 * @version  V1.0.0
 * @date     27-03-2017
 * @brief    otg host hardware driver interface
 *****************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2015 MVSilicon </center></h2>
 */

/**
 * @addtogroup OTG
 * @{
 * @defgroup otg_host_hcd otg_host_hcd.h
 * @{
 */
 
#ifndef __OTG_HOST_HCD_H__
#define	__OTG_HOST_HCD_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus
	
#include "type.h"
#ifdef CFG_APP_CONFIG
	#include "app_config.h"
#endif

#define	HOST_CONTROL_EP			0
#define	HOST_BULK_IN_EP			1
#define	HOST_BULK_OUT_EP		1
#define	HOST_ISO_FEEDBACK_IN_EP	1
#define	HOST_INT_IN_EP			2
#define	HOST_INT_OUT_EP			2
#define	HOST_ISO_IN_EP			3
#define	HOST_ISO_OUT_EP			3


#define	HOST_FS_CONTROL_MPS		64
#define	HOST_FS_INT_IN_MPS		64
#define	HOST_FS_BULK_IN_MPS		64
#define	HOST_FS_BULK_OUT_MPS	64
#define	HOST_FS_ISO_OUT_MPS		192
#define	HOST_FS_ISO_IN_MPS		768

#define ENDPOINT_TYPE_CONTROL	0x00
#define ENDPOINT_TYPE_ISO		0x01
#define ENDPOINT_TYPE_BULK		0x02
#define ENDPOINT_TYPE_INT		0x03


typedef enum _OTG_HOST_ERR_CODE
{
	HOST_NONE_ERR = 0,
	HOST_UNLINK_ERR,
	
	CONTROL_WRITE_SETUP_ERR,
	CONTROL_WRITE_OUT_ERR,
	CONTROL_WRITE_IN_ERR,
	
	CONTROL_READ_SETUP_ERR,
	CONTROL_READ_OUT_ERR,
	CONTROL_READ_IN_ERR,
	
	BULK_READ_ERR,
	BULK_WRITE_ERR,

	INT_READ_ERR,
	INT_WRITE_ERR,

	ISO_READ_ERR,
	ISO_WRITE_ERR,
}OTG_HOST_ERR_CODE;

	
typedef struct _USB_CTRL_SETUP_REQUEST{
    uint8_t bmRequest;//D7(0:H->D;1:D->H);D6~D5(0:标准,1:类,2:厂商,3:rsv)；D4~D0(0:设备,1:接口,2:端点,3:其他,4_31:RSV)
    uint8_t bRequest;//请求代码
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
}USB_CTRL_SETUP_REQUEST, *PUSB_CTRL_SETUP_REQUEST;


typedef struct _PIPE_INFO
{
	uint8_t	EpNum;//endpoint number
	uint8_t	EpType;//endpoint type
	uint16_t MaxPacketSize;	//max packet size
	uint8_t	bInterval;
} PIPE_INFO;

// ---------------------------- FS Host模式函数声明（保留注释） ----------------------------
/**
 * @brief  usb host init
 * @param  NONE
 * @return NONE
 */
void OTG_FS_HostControlInit(void);

/**
 * @brief  总线复位
 * @param  NONE
 * @return NONE
 */
void OTG_FS_HostPortReset(void);

/**
 * @brief  设置地址
 * @param  地址
 * @return NONE
 */
void OTG_FS_HostAddressSet(uint8_t Address);

/**
 * @brief  从控制端点完成一次控制写传输或者无数据写传输,SETUP OUTDATA INDATA或者SETUP IN
 * @param  SetupPacket setup请求cmd
 * @param  Buf OUT数据缓冲区指针
 * @param  Len OUT数据长度
 * @param  TimeOut 传输超时值，单位为毫秒
 * @return OTG_HOST_ERR_CODE
 */
OTG_HOST_ERR_CODE OTG_FS_HostControlWrite(USB_CTRL_SETUP_REQUEST SetupPacket, uint8_t *Buf, uint32_t Len, uint32_t TimeOut);

/**
 * @brief  从控制端点完成一次控制读传输，SETUP INDATA OUT
 * @param  SetupPacket setup请求数据
 * @param  Buf IN数据缓冲区指针
 * @param  Len IN数据长度
 * @param  TimeOut 传输超时值，单位为毫秒
 * @return OTG_HOST_ERR_CODE
 */
OTG_HOST_ERR_CODE OTG_FS_HostControlRead(USB_CTRL_SETUP_REQUEST SetupPacket, uint8_t *Buf, uint32_t Len, uint32_t TimeOut);

/**
 * @brief  Host以BULK传输方式接收一个数据包
 * @param  Pipe pipe指针
 * @param  Buf 接收数据缓冲区指针
 * @param  Len 接收数据长度
 * @param  TimeOut 接收数据超时时间，单位为毫秒
 * @return OTG_HOST_ERR_CODE
 */
OTG_HOST_ERR_CODE OTG_FS_HostBulkWrite(PIPE_INFO* Pipe, uint8_t *Buf, uint32_t Len, uint32_t TimeOut);

/**
 * @brief  Host以BULK传输方式发送一个数据包
 * @param  Pipe pipe指针
 * @param  Buf 发送数据缓冲区指针
 * @param  Len 发送数据长度
 * @param  pTransferLen 实际接收到的数据长度
 * @param  TimeOut 接收数据超时时间，单位为毫秒
 * @return OTG_HOST_ERR_CODE
 */
OTG_HOST_ERR_CODE OTG_FS_HostBulkRead(PIPE_INFO* Pipe, uint8_t* Buf, uint32_t Len, uint32_t *pTransferLen, uint16_t TimeOut);

/**
 * @brief  Host以interrupt传输方式发送一个数据包
 * @param  Pipe pipe指针
 * @param  Buf 发送数据缓冲区指针
 * @param  Len 发送数据长度
 * @param  TimeOut 接收数据超时时间，单位为毫秒
 * @return OTG_HOST_ERR_CODE
 */
OTG_HOST_ERR_CODE OTG_FS_HostInterruptWrite(PIPE_INFO* Pipe, uint8_t *Buf, uint32_t Len, uint32_t TimeOut);

/**
 * @brief  Host以interrupt传输方式接收一个数据包
 * @param  Pipe pipe指针
 * @param  Buf 发送数据缓冲区指针
 * @param  Len 发送数据长度
 * @param  pTransferLen 实际接收到的数据长度
 * @param  TimeOut 接收数据超时时间，单位为毫秒
 * @return OTG_HOST_ERR_CODE
 */
OTG_HOST_ERR_CODE OTG_FS_HostInterruptRead(PIPE_INFO* Pipe, uint8_t *Buf, uint32_t Len, uint32_t *pTransferLen, uint32_t TimeOut);

/**
 * @brief  Host以iso传输方式写一个数据包
 * @param  Pipe pipe指针
 * @param  Buf 发送数据缓冲区指针
 * @param  Len 发送数据长度
 * @param  TimeOut 接收数据超时时间，单位为毫秒
 * @return OTG_HOST_ERR_CODE
 */
OTG_HOST_ERR_CODE OTG_FS_HostISOWrite(PIPE_INFO* Pipe, uint8_t *Buf, uint32_t Len, uint32_t TimeOut);

/**
 * @brief  Host以iso传输方式接收一个数据包
 * @param  Pipe pipe指针
 * @param  Buf 发送数据缓冲区指针
 * @param  Len 发送数据长度
 * @param  *pTransferLen 发送数据实际长度
 * @param  TimeOut 接收数据超时时间，单位为毫秒
 * @return OTG_HOST_ERR_CODE
 */
OTG_HOST_ERR_CODE OTG_FS_HostISORead(PIPE_INFO* Pipe, uint8_t *Buf, uint32_t Len, uint32_t *pTransferLen, uint32_t TimeOut);

/**
 * @brief  启动主机IN端点(接收)
 * @param  HostEp: 内部端点索引编号
 * @param  Pipe: 端点信息(端点号、端点类型、传输协议、包长、轮询间隔等)
 * @retval 无
 * @note   配置主机内部端点参数，发送首次包请求。
 */
void OTG_FS_HostEpReadStart(uint8_t HostEp,PIPE_INFO* Pipe);
/**
 * @brief  主机IN端点单包接收函数
 * @param  HostEp: 内部端点索引编号
 * @param  Buf: 接收数据缓冲区指针
 * @param  Len: 缓冲区最大长度
 * @param  pTransferLen: 输出实际接收到的数据长度
 * @retval OTG_HOST_ERR_CODE
 * @note   在OTG_FS_HostEpReadStart后使用，读取FIFO数据后自动请求下一包。
 */
OTG_HOST_ERR_CODE OTG_FS_HostEpReadPacket(uint8_t HostEp, uint8_t *Buf, uint32_t Len, uint32_t *pTransferLen);

/**
 * @brief  启动主机OUT端点(发送)
 * @param  HostEp: 内部端点索引编号
 * @param  Pipe: 端点信息(端点号、端点类型、传输协议、包长、轮询间隔等)
 * @retval 无
 * @note   配置主机内部端点参数，等待上层填充数据发送
 */
void OTG_FS_HostEpSendStart(uint8_t HostEp,PIPE_INFO* Pipe);

/**
 * @brief  主机OUT端点单包发送函数
 * @param  HostEp: 内部端点索引
 * @param  Buf: 待发送数据缓冲区
 * @param  Len: 待发送数据长度
 * @retval 无
 * @note   在OTG_FS_HostEpSendStart后使用，将数据加载到FIFO，标记发送就绪，硬件自动周期发送
 */
void OTG_FS_HostEpSendPacket(uint8_t HostEp, uint8_t *Buf, uint32_t Len);


void OTG_FS_HostFifoInit(void);


// ---------------------------- HS Host模式函数声明（保留注释） ----------------------------
/**
 * @brief  usb host init
 * @param  NONE
 * @return NONE
 */
void OTG_HS_HostControlInit(void);

/**
 * @brief  总线复位
 * @param  NONE
 * @return NONE
 */
void OTG_HS_HostPortReset(void);

/**
 * @brief  HOST模式下Port端口上设备速度获取
 * @param  NONE
 * @return 0-无设备连接，1-LS设备连接，2-FS设备连接，3-HS设备连接
 */
uint8_t OTG_HS_HostPortSpeedGet(void);
/**
 * @brief  设置地址
 * @param  地址
 * @return NONE
 */
void OTG_HS_HostAddressSet(uint8_t Address);

/**
 * @brief  从控制端点完成一次控制写传输或者无数据写传输,SETUP OUTDATA INDATA或者SETUP IN
 * @param  SetupPacket setup请求cmd
 * @param  Buf OUT数据缓冲区指针
 * @param  Len OUT数据长度
 * @param  TimeOut 传输超时值，单位为毫秒
 * @return OTG_HOST_ERR_CODE
 */
OTG_HOST_ERR_CODE OTG_HS_HostControlWrite(USB_CTRL_SETUP_REQUEST SetupPacket, uint8_t *Buf, uint32_t Len, uint32_t TimeOut);

/**
 * @brief  从控制端点完成一次控制读传输，SETUP INDATA OUT
 * @param  SetupPacket setup请求数据
 * @param  Buf IN数据缓冲区指针
 * @param  Len IN数据长度
 * @param  TimeOut 传输超时值，单位为毫秒
 * @return OTG_HOST_ERR_CODE
 */
OTG_HOST_ERR_CODE OTG_HS_HostControlRead(USB_CTRL_SETUP_REQUEST SetupPacket, uint8_t *Buf, uint32_t Len, uint32_t TimeOut);

/**
 * @brief  Host以BULK传输方式接收一个数据包
 * @param  Pipe pipe指针
 * @param  Buf 接收数据缓冲区指针
 * @param  Len 接收数据长度
 * @param  TimeOut 接收数据超时时间，单位为毫秒
 * @return OTG_HOST_ERR_CODE
 */
OTG_HOST_ERR_CODE OTG_HS_HostBulkWrite(PIPE_INFO* Pipe, uint8_t *Buf, uint32_t Len, uint32_t TimeOut);

/**
 * @brief  Host以BULK传输方式发送一个数据包
 * @param  Pipe pipe指针
 * @param  Buf 发送数据缓冲区指针
 * @param  Len 发送数据长度
 * @param  pTransferLen 实际接收到的数据长度
 * @param  TimeOut 接收数据超时时间，单位为毫秒
 * @return OTG_HOST_ERR_CODE
 */
OTG_HOST_ERR_CODE OTG_HS_HostBulkRead(PIPE_INFO* Pipe, uint8_t* Buf, uint32_t Len, uint32_t *pTransferLen, uint16_t TimeOut);

/**
 * @brief  Host以interrupt传输方式发送一个数据包
 * @param  Pipe pipe指针
 * @param  Buf 发送数据缓冲区指针
 * @param  Len 发送数据长度
 * @param  TimeOut 接收数据超时时间，单位为毫秒
 * @return OTG_HOST_ERR_CODE
 */
OTG_HOST_ERR_CODE OTG_HS_HostInterruptWrite(PIPE_INFO* Pipe, uint8_t *Buf, uint32_t Len, uint32_t TimeOut);

/**
 * @brief  Host以interrupt传输方式接收一个数据包
 * @param  Pipe pipe指针
 * @param  Buf 发送数据缓冲区指针
 * @param  Len 发送数据长度
 * @param  pTransferLen 实际接收到的数据长度
 * @param  TimeOut 接收数据超时时间，单位为毫秒
 * @return OTG_HOST_ERR_CODE
 */
OTG_HOST_ERR_CODE OTG_HS_HostInterruptRead(PIPE_INFO* Pipe, uint8_t *Buf, uint32_t Len, uint32_t *pTransferLen, uint32_t TimeOut);

/**
 * @brief  Host以iso传输方式写一个数据包
 * @param  Pipe pipe指针
 * @param  Buf 发送数据缓冲区指针
 * @param  Len 发送数据长度
 * @param  TimeOut 接收数据超时时间，单位为毫秒
 * @return OTG_HOST_ERR_CODE
 */
OTG_HOST_ERR_CODE OTG_HS_HostISOWrite(PIPE_INFO* Pipe, uint8_t *Buf, uint32_t Len, uint32_t TimeOut);

/**
 * @brief  Host以iso传输方式接收一个数据包
 * @param  Pipe pipe指针
 * @param  Buf 发送数据缓冲区指针
 * @param  Len 发送数据长度
 * @param  *pTransferLen 发送数据实际长度
 * @param  TimeOut 接收数据超时时间，单位为毫秒
 * @return OTG_HOST_ERR_CODE
 */
OTG_HOST_ERR_CODE OTG_HS_HostISORead(PIPE_INFO* Pipe, uint8_t *Buf, uint32_t Len, uint32_t *pTransferLen, uint32_t TimeOut);

/**
 * @brief  启动主机IN端点(接收)
 * @param  HostEp: 内部端点索引编号
 * @param  Pipe: 端点信息(端点号、端点类型、传输协议、包长、轮询间隔等)
 * @retval 无
 * @note   配置主机内部端点参数，发送首次包请求。
 */
void OTG_HS_HostEpReadStart(uint8_t HostEp,PIPE_INFO* Pipe);
/**
 * @brief  主机IN端点单包接收函数
 * @param  HostEp: 内部端点索引编号
 * @param  Buf: 接收数据缓冲区指针
 * @param  Len: 缓冲区最大长度
 * @param  pTransferLen: 输出实际接收到的数据长度
 * @retval OTG_HOST_ERR_CODE
 * @note   在OTG_HS_HostEpReadStart后使用，读取FIFO数据后自动请求下一包。
 */
OTG_HOST_ERR_CODE OTG_HS_HostEpReadPacket(uint8_t HostEp, uint8_t *Buf, uint32_t Len, uint32_t *pTransferLen);

/**
 * @brief  启动主机OUT端点(发送)
 * @param  HostEp: 内部端点索引编号
 * @param  Pipe: 端点信息(端点号、端点类型、传输协议、包长、轮询间隔等)
 * @retval 无
 * @note   配置主机内部端点参数，等待上层填充数据发送
 */
void OTG_HS_HostEpSendStart(uint8_t HostEp,PIPE_INFO* Pipe);
/**
 * @brief  主机OUT端点单包发送函数
 * @param  HostEp: 内部端点索引
 * @param  Buf: 待发送数据缓冲区
 * @param  Len: 待发送数据长度
 * @retval 无
 * @note   在OTG_HS_HostEpSendStart后使用，将数据加载到FIFO，标记发送就绪，硬件自动周期发送
 */
void OTG_HS_HostEpSendPacket(uint8_t HostEp, uint8_t *Buf, uint32_t Len);


void OTG_HS_HostFifoInit(void);

// ---------------------------- 接口选择宏 ----------------------------
#ifdef OTG_HOST_PORT_FS
	#define	OTG_HostFifoInit											OTG_FS_HostFifoInit
	#define OTG_HostControlInit                          				OTG_FS_HostControlInit
	#define OTG_HostPortReset                            				OTG_FS_HostPortReset
	#define OTG_HostAddressSet                           				OTG_FS_HostAddressSet
	#define OTG_HostControlWrite                         				OTG_FS_HostControlWrite
	#define OTG_HostControlRead                          				OTG_FS_HostControlRead
	#define OTG_HostBulkWrite                            				OTG_FS_HostBulkWrite
	#define OTG_HostBulkRead                             				OTG_FS_HostBulkRead
	#define OTG_HostInterruptWrite                       				OTG_FS_HostInterruptWrite
	#define OTG_HostInterruptRead                        				OTG_FS_HostInterruptRead
	#define OTG_HostISOWrite                             				OTG_FS_HostISOWrite
	#define OTG_HostISORead                              				OTG_FS_HostISORead
	#define OTG_HostISOReadStart										OTG_FS_HostISOReadStart
	#define OTG_HostInterruptReadStart									OTG_FS_HostInterruptReadStart

	#define OTG_HostEndpointInterruptEnable       						OTG_FS_EndpointInterruptEnable
	#define OTG_HostEndpointInterruptDisable      						OTG_FS_EndpointInterruptDisable
	#define OTG_HostBusEventInterruptEnable       						OTG_FS_BusEventInterruptEnable
	#define OTG_HostBusEventInterruptDisable      						OTG_FS_BusEventInterruptDisable
#else
	#define	OTG_HostFifoInit											OTG_HS_HostFifoInit
	#define OTG_HostControlInit                          				OTG_HS_HostControlInit
	#define OTG_HostPortReset                            				OTG_HS_HostPortReset
	#define OTG_HostPortSpeedGet          								OTG_HS_HostPortSpeedGet
	#define OTG_HostAddressSet                           				OTG_HS_HostAddressSet
	#define OTG_HostControlWrite                         				OTG_HS_HostControlWrite
	#define OTG_HostControlRead                          				OTG_HS_HostControlRead
	#define OTG_HostBulkWrite                            				OTG_HS_HostBulkWrite
	#define OTG_HostBulkRead                             				OTG_HS_HostBulkRead
	#define OTG_HostInterruptWrite                       				OTG_HS_HostInterruptWrite
	#define OTG_HostInterruptRead                        				OTG_HS_HostInterruptRead
	#define OTG_HostISOWrite                             				OTG_HS_HostISOWrite
	#define OTG_HostISORead                              				OTG_HS_HostISORead

	#define OTG_HostEndpointInterruptEnable       						OTG_HS_EndpointInterruptEnable
	#define OTG_HostEndpointInterruptDisable      						OTG_HS_EndpointInterruptDisable
	#define OTG_HostBusEventInterruptEnable       						OTG_HS_BusEventInterruptEnable
	#define OTG_HostBusEventInterruptDisable      						OTG_HS_BusEventInterruptDisable
#endif
typedef void (*OTG_HostControlInit_t)(void);
typedef void (*OTG_HostPortReset_t)(void);
typedef uint8_t (*OTG_HostPortSpeedGet_t)(void);
typedef void (*OTG_HostAddressSet_t)(uint8_t Address);
typedef OTG_HOST_ERR_CODE (*OTG_HostControlWrite_t)(USB_CTRL_SETUP_REQUEST SetupPacket, uint8_t *Buf, uint32_t Len, uint32_t TimeOut);
typedef OTG_HOST_ERR_CODE (*OTG_HostControlRead_t)(USB_CTRL_SETUP_REQUEST SetupPacket, uint8_t *Buf, uint32_t Len, uint32_t TimeOut);
typedef OTG_HOST_ERR_CODE (*OTG_HostBulkWrite_t)(PIPE_INFO* Pipe, uint8_t *Buf, uint32_t Len, uint32_t TimeOut);
typedef OTG_HOST_ERR_CODE (*OTG_HostBulkRead_t)(PIPE_INFO* Pipe, uint8_t* Buf, uint32_t Len, uint32_t *pTransferLen, uint16_t TimeOut);
typedef OTG_HOST_ERR_CODE (*OTG_HostInterruptWrite_t)(PIPE_INFO* Pipe, uint8_t *Buf, uint32_t Len, uint32_t TimeOut);
typedef OTG_HOST_ERR_CODE (*OTG_HostInterruptRead_t)(PIPE_INFO* Pipe, uint8_t *Buf, uint32_t Len, uint32_t *pTransferLen, uint32_t TimeOut);
typedef OTG_HOST_ERR_CODE (*OTG_HostISOWrite_t)(PIPE_INFO* Pipe, uint8_t *Buf, uint32_t Len, uint32_t TimeOut);
typedef OTG_HOST_ERR_CODE (*OTG_HostISORead_t)(PIPE_INFO* Pipe, uint8_t *Buf, uint32_t Len, uint32_t *pTransferLen, uint32_t TimeOut);
typedef void (*OTG_HostEndpointInterruptEnable_t)(uint8_t EpNum, FPCALLBACK Func);
typedef void (*OTG_HostEndpointInterruptDisable_t)(uint8_t EpNum);
typedef void (*OTG_HostBusEventInterruptEnable_t)(uint8_t EventId, FPCALLBACK Func);
typedef void (*OTG_HostBusEventInterruptDisable_t)(uint8_t EventId);
#ifdef __cplusplus
}
#endif//__cplusplus

#endif //__OTG_HOST_HCD_H__
/**
 * @}
 * @}
 */
