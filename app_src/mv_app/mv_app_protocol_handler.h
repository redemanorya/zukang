/**
 **************************************************************************************
 * @file    mv_app_protocol_handler.h
 * @brief   应用层协议处理模块
 * @details 定义与APP通信协议的数据结构、枚举类型及API接口
 *          支持BLE/SPP两种传输方式，采用请求-响应机制
 *          接收方式：一次性接收完整数据包
 *
 * @author  Jarvis
 * @version V1.0.0
 *
 * $Created: 2026-01-01
 * $Revision: 1.0
 * $Date: 2026-01-01
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#ifndef MV_APP_PROTOCOL_HANDLER_H
#define MV_APP_PROTOCOL_HANDLER_H

#include <stdint.h>
#include <stdbool.h>
#include "bt_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef MV_APP_CONTROL
/*=============================================================================
 * 宏定义
 *============================================================================*/
#define FRAME_HEADER1               0xA5
#define FRAME_HEADER2               0x5A
#define CONTROL_WORD                0x24

#define PROTOCOL_VERSION            "V1.0.0"

#define MAX_FRAME_SIZE              512
#define MAX_DATA_SIZE               (MAX_FRAME_SIZE - 7)

/*=============================================================================
 * 枚举定义
 *============================================================================*/
// 状态码
typedef enum {
    STATUS_OK                  = 0x00,     // 成功
    STATUS_INVALID_CMD         = 0x01,     // 命令无效
    STATUS_INVALID_PARAM       = 0x02,     // 参数错误
    STATUS_BUSY                = 0x03,     // 设备忙
    STATUS_CHECKSUM_ERR        = 0x04,     // 校验错误
} ProtocolStatus;

// 命令码
typedef enum {
    CMD_HANDSHAKE              = 0x01,     // 握手与版本查询
    CMD_COMMON_CONTROL         = 0x02,     // 常用控制
    CMD_LED_EFFECT             = 0x03,     // 灯效控制
    CMD_MIC_EFFECT             = 0x04,     // 麦克风音效控制
    CMD_MUSIC_EFFECT           = 0x05,     // 音乐音效控制
    CMD_QUERY_STATE            = 0x06,     // 设备状态查询
    CMD_LE_AUDIO               = 0x07,     // LE Audio Assistant
    CMD_RESERVED_START         = 0x08,
    CMD_RESERVED_END           = 0xFF,
} CommandCode;

/*=============================================================================
 * 回调函数类型定义
 *============================================================================*/
// 数据发送回调（底层BLE/SPP发送函数）
typedef void (*ProtocolSendCallback)(const uint8_t* data, uint16_t len);

// 事件通知回调（可选，用于日志或异步通知）
typedef void (*ProtocolEventCallback)(CommandCode cmd, uint8_t subCmd, 
                                       const uint8_t* params, uint16_t paramLen);

// 命令处理器函数类型
// 返回值：true=处理成功，false=处理失败
// response: 输出响应数据（不含状态码，状态码由框架自动添加）
// responseLen: 输出响应数据长度
typedef bool (*CommandHandler)(uint8_t subCmd, const uint8_t* params, uint16_t paramLen,
                                uint8_t* response, uint16_t* responseLen);

/*=============================================================================
 * 结构体定义
 *============================================================================*/
typedef struct {
    ProtocolSendCallback   sendCallback;      // 发送回调
    ProtocolEventCallback  eventCallback;     // 事件回调（可选，可为NULL）
    CommandHandler         handlers[256];     // 命令处理器映射表
} ProtocolHandler;

/*=============================================================================
 * API函数声明
 *============================================================================*/

/**
 * @brief 初始化协议处理器
 * @param handler       协议处理器句柄
 * @param sendCb        发送回调函数（必须）
 * @param eventCb       事件回调函数（可选，传NULL）
 */
void Protocol_Init(ProtocolHandler* handler, 
                   ProtocolSendCallback sendCb,
                   ProtocolEventCallback eventCb);

/**
 * @brief 注册命令处理器
 * @param handler       协议处理器句柄
 * @param cmd           命令码
 * @param handlerFunc   处理函数
 */
void Protocol_RegisterHandler(ProtocolHandler* handler, 
                              CommandCode cmd, 
                              CommandHandler handlerFunc);

/**
 * @brief 处理接收到的数据包（一次性接收完整包）
 * @param handler       协议处理器句柄
 * @param data          接收到的数据指针
 * @param len           数据长度
 * @return true=处理成功（帧有效），false=处理失败
 */
bool Protocol_ProcessPacket(ProtocolHandler* handler, 
                            const uint8_t* data, 
                            uint16_t len);

/**
 * @brief 发送请求帧（主动向APP发送请求）
 * @param handler       协议处理器句柄
 * @param cmd           命令码
 * @param subCmd        子命令码
 * @param params        参数数据
 * @param paramLen      参数长度
 * @return true=发送成功，false=发送失败
 */
bool Protocol_SendRequest(ProtocolHandler* handler,
                          CommandCode cmd,
                          uint8_t subCmd,
                          const uint8_t* params,
                          uint16_t paramLen);

/**
 * @brief 发送响应帧（回复APP的请求）
 * @param handler       协议处理器句柄
 * @param cmd           命令码（与请求相同）
 * @param status        状态码
 * @param extraData     附加数据
 * @param extraDataLen  附加数据长度
 * @return true=发送成功，false=发送失败
 */
bool Protocol_SendResponse(ProtocolHandler* handler,
                           CommandCode cmd,
                           ProtocolStatus status,
                           const uint8_t* extraData,
                           uint16_t extraDataLen);

/**
 * @brief 计算校验和
 * @param data          数据指针
 * @param len           数据长度
 * @return 校验和值
 */
uint8_t Protocol_CalcChecksum(const uint8_t* data, uint16_t len);



void Mv_AppProtocol_Init(void);

void mvble_data_received(const uint8_t* data, uint16_t len);
#endif 

#ifdef __cplusplus
}
#endif

#endif /* MV_APP_PROTOCOL_HANDLER_H */