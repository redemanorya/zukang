/**
 *****************************************************************************
 * @file     otg_device.h
 * @author   Owen
 * @version  V1.0.0
 * @date     24-June-2015
 * @brief    audio device interface
 *****************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
 */

#ifndef __OTG_DEVICE_H__
#define	__OTG_DEVICE_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus
	
#include "type.h"

typedef struct _UsbAudioClass UsbAudioClass;
typedef struct _UsbStor UsbStorClass;

// 复合设备总类（所有功能类的容器）
typedef struct {
    // 音频类实例
	UsbAudioClass*	pAudio;
    // 读卡器(MSC)类实例
    UsbStorClass*	pMSC;
    // 以后加其他功能（如CDC），继续加指针即可
} UsbCompositeClass;

/**
 * @brief USB设备类型枚举
 */
typedef enum {
	USB_DEVICE_FS = 0,  // 全速设备
	USB_DEVICE_HS = 1,  // 高速设备
	USB_DEVICE_MAX
} USB_DeviceType_t;

/* Following USB Device Speed */
typedef enum
{
  USB_SPEED_NULL  = 0U,
  USB_SPEED_LOW   = 1U,
  USB_SPEED_FULL  = 2U,
  USB_SPEED_HIGH  = 3U,
} USB_SpeedType_t;

// USB 设备全局状态枚举（标准状态机）
typedef enum
{
    DEV_STATE_DEFAULT,      // 默认状态（上电后）
    DEV_STATE_RESET,     	// 复位
    DEV_STATE_ADDRESSED,    // 已分配地址
    DEV_STATE_CONFIGURED,   // 已枚举完成、已配置（工作状态）
    DEV_STATE_SUSPEND,      // 挂起
	DEV_STATE_RESUME,		// 恢复
} USB_DevStateTypeDef;

struct _DeviceCtx_t;
/**
 * @brief USB驱动操作抽象接口（适配HS/FS不同底层）
 */
typedef struct {
    // 端点0相关
	OTG_DEVICE_ERR_CODE (*SetupReceive)(uint8_t* Buf, uint32_t MaxLen ,uint32_t *pTransferLen);
	OTG_DEVICE_ERR_CODE (*ControlSend)(uint8_t* Buf, uint32_t Len, uint32_t TimeOut);
	OTG_DEVICE_ERR_CODE (*ControlReceive)(uint8_t* Buf, uint32_t MaxLen, uint32_t *pTransferLen, uint32_t TimeOut);
	OTG_DEVICE_ERR_CODE (*BulkReceive)(uint8_t EpNum, uint8_t* Buf, uint16_t MaxLen, uint32_t *pTransferLen ,uint32_t TimeOut);
	OTG_DEVICE_ERR_CODE (*BulkSend)(uint8_t EpNum, uint8_t* Buf, uint32_t Len, uint32_t TimeOut);
    uint8_t (*DeviceBusEventGet)(void);
    uint8_t (*DevicePortSpeedGet)(void);
    void (*StallSend)(uint8_t ep);
    void (*AddressSet)(uint8_t addr);
    // 设备配置
    void (*EndpointReset)(uint8_t ep, uint8_t type);
    void (*EndpointPacketSizeSet)(uint8_t ep, uint32_t size);
    void (*EndpointInterruptEnable)(uint8_t ep, void (*cb)(void));
} USB_DriverOps_t;
/**
 * @brief USB设备配置（差异化的核心参数）
 */
typedef struct {
    uint8_t *(*GetDeviceDescriptor)(USB_SpeedType_t speed, uint16_t *length);
    uint8_t *(*GetConfigurationStrDescriptor)(USB_SpeedType_t speed, uint16_t *length);
    uint8_t *(*GetDeviceQualifierDescriptor)(USB_SpeedType_t speed,uint16_t *length);
    uint8_t *(*GetStrDescriptor)(USB_SpeedType_t speed, uint8_t idx, uint16_t *length);
    uint8_t *(*GetHidReportDescriptor)(USB_SpeedType_t speed, uint8_t idx, uint16_t *length);
    uint8_t *(*GetBOSDescriptor)(USB_SpeedType_t speed, uint16_t *length);

    // 初始化/中断
    void (*SoftwareInit)(void);
    void (*SoftwareClassRequest)(struct _DeviceCtx_t *ctx);
    void (*SoftwareSetInterest)(uint8_t Interest,uint8_t Alt);
    void (*SoftwareReset)(USB_SpeedType_t speed);
	void (*SoftwareSuspend)(USB_SpeedType_t speed);
	void (*SoftwareResume)(USB_SpeedType_t speed);
} USB_DevConfig_t;

// USB Setup包结构（8字节标准格式）
typedef struct {
	uint8_t bmRequestType;    // 请求类型（方向、类型、接收者）
	uint8_t bRequest;         // 具体请求
	uint16_t wValue;          // 请求参数值
	uint16_t wIndex;          // 请求参数索引（通常为接口/端点号）
	uint16_t wLength;         // 数据阶段长度
} USB_SetupPacket;

// 端点0 状态机状态定义
typedef enum {
	EP0_STATE_IDLE,              // 空闲状态，等待Setup包
	EP0_STATE_SETUP,             // 处理Setup包阶段
	EP0_STATE_DATA_IN,           // 主机读数据阶段（IN传输）
	EP0_STATE_DATA_OUT,          // 主机写数据阶段（OUT传输）
	EP0_STATE_STATUS_IN,         // 状态阶段（主机读状态）
	EP0_STATE_STATUS_OUT,        // 状态阶段（主机写状态）
	EP0_STATE_STALL,             // 端点暂停状态（错误处理）
//	EP0_STATE_WAIT_ADDRESS_ACK   // 等待地址设置确认（特殊状态）
} EP0_State;

// 端点0 控制结构体
typedef struct {
	EP0_State       state;           // 当前状态
	USB_SetupPacket setup_pkt;       // 存储接收到的Setup包
	uint8_t*        data_buf;        // 数据缓冲区（用于IN/OUT传输）
	uint32_t        total_data_len;  // 总数据长度（IN传输时为待发送长度，OUT为待接收长度）
	uint32_t        processed_len;   // 已处理的数据长度（发送/接收）
	uint8_t         device_addr;     // 设备地址（SET_ADDRESS请求用）
} EP0_HandleTypeDef;

/**
 * @brief USB设备上下文（包含所有设备信息，HS/FS独立实例）
 */
typedef struct _DeviceCtx_t{
	// 设备基础信息
	USB_DeviceType_t type;          // 设备类型（HS/FS）
	USB_DevStateTypeDef state;		// USB设备当前全局状态
	USB_SpeedType_t dev_speed;		// USB设备当前速率
	USB_DriverOps_t bus_ops;     	// 绑定的底层驱动接口
	USB_DevConfig_t cfg;            // 设备差异化配置（VID/PID/描述符/端点等）
	// 端点0状态机
	EP0_HandleTypeDef ep0;          // 端点0状态机
	// 业务缓冲区
	uint8_t Setup[8];               // Setup包缓冲区
	uint8_t Request[256];           // 控制传输缓冲区
	uint8_t Resp[256];              // 应答缓冲区

	void *pClassData;				// 类数据
} DeviceCtx_t;


/**
 * @brief 准备IN传输（设备→主机）：填充数据缓冲区并切换到DATA_IN状态
 * @param ep0：端点0控制结构体指针
 * @param data_buf：待发送数据的缓冲区指针
 * @param total_data_len：待发送的总数据长度（≤ Setup包的wLength）
 * @return 0：成功；-1：参数无效
 */
int32_t OTG_DeviceControlInTransfer(USB_DeviceType_t dev_type,uint8_t* data_buf, uint32_t total_data_len);

/**
 * @brief 准备OUT传输（主机→设备）：绑定接收缓冲区并切换到DATA_OUT状态
 * @param ep0：端点0控制结构体指针
 * @param data_buf：接收数据的缓冲区指针
 * @param total_data_len：预期接收的总数据长度（通常等于Setup包的wLength）
 * @return 0：成功；-1：参数无效
 */
int32_t OTG_DeviceControlOutTransfer(USB_DeviceType_t dev_type,uint8_t* data_buf, uint32_t total_data_len);
/**
 * @brief 注册USB类数据到设备上下文
 * @param dev_type 设备类
 * @param class_data 类数据指针（不能为空）
 * @retval TRUE 成功
 * @retval FALSE 失败（参数无效/类数据类型错误）
 */
bool OTG_DeviceRegisterClassData(USB_DeviceType_t dev_type, void *class_data);

/**
 * @brief  初始化设备上下文
 * @param  dev_type 对外设备类型
 * @param  bus_ops 总线操作函数指针
 * @return 成功/失败
 */
bool OTG_DeviceCtxInit(USB_DeviceType_t dev_type,USB_DevConfig_t *DriverCfg,USB_DriverOps_t *bus_ops);

/**
 * @brief  获取设备上下文
 * @param  dev_type 对外设备类型
 * @return 上下文指针（NULL表示无效）
 */
DeviceCtx_t* Device_GetCtx(USB_DeviceType_t dev_type);

void OTG_FS_DeviceAudioModeSel(uint8_t Mode,uint16_t UsbVid,uint16_t UsbPid);

void OTG_HS_DeviceAudioModeSel(uint8_t Mode,uint16_t UsbVid,uint16_t UsbPid);

void OTG_DeviceAudioModeSel(USB_DeviceType_t dev_type,uint8_t Mode,uint16_t UsbVid,uint16_t UsbPid);

void OTG_DeviceHardwareInit(USB_DeviceType_t dev_type,uint8_t Mode,uint16_t UsbVid,uint16_t UsbPid);

//TODO 以下函数暂时存放在这里
extern void OTG_DeviceCustomHidRequest(DeviceCtx_t *ctx);
void asciiToUnicode(uint8_t *ascii, uint8_t *unicode, uint16_t *len);
#ifdef  __cplusplus
}
#endif//__cplusplus

#endif
