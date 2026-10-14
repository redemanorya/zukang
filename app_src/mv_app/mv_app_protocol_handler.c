/**
 **************************************************************************************
 * @file    mv_app_protocol_handler.c
 * @brief   应用层协议处理模块
 * @details 实现与APP约定的通信协议，包括帧解析、命令分发、响应生成等功能
 *          协议格式：帧头(2B) + 控制字(1B) + 命令(1B) + 帧长度(2B) + 数据段(NB) + 校验和(1B)
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
#include "mv_app_protocol_handler.h"
#include "mv_app_protocol_commands.h"
#include <string.h>
#include "ble_gs_service.h"
#ifdef MV_APP_CONTROL
/*=============================================================================
 * 调试宏
 *============================================================================*/
#define PROTOCOL_DEBUG_ENABLE    1

#if PROTOCOL_DEBUG_ENABLE
#include <stdio.h>
#define PROTOCOL_PRINTF(...)     printf(__VA_ARGS__)
#define PROTOCOL_DEBUG_LOG(...)  printf("[PROTO] " __VA_ARGS__)
#else
#define PROTOCOL_PRINTF(...)
#define PROTOCOL_DEBUG_LOG(...)
#endif

static ProtocolHandler protocol;

/*=============================================================================
 * 内部函数声明
 *============================================================================*/
static bool ParseFrame(const uint8_t* frame, uint16_t frameLen, 
                       CommandCode* cmd, uint8_t* subCmd, 
                       const uint8_t** params, uint16_t* paramLen);
static bool BuildFrame(uint8_t* frame, uint16_t* frameLen, CommandCode cmd,
                       const uint8_t* data, uint16_t dataLen);
static bool ValidateFrame(const uint8_t* frame, uint16_t frameLen);

/*=============================================================================
 * 函数实现
 *============================================================================*/

/**
 * @brief 计算校验和（包含帧头）
 * @param data 数据指针（从帧头开始）
 * @param len 数据长度（从帧头到数据段结束，不含校验和本身）
 * @return 校验和值
 */
uint8_t Protocol_CalcChecksum(const uint8_t* data, uint16_t len) {
    uint8_t checksum = 0;
    for (uint16_t i = 0; i < len; i++) {
        checksum += data[i];
    }
    return checksum;
}

/**
 * @brief 验证帧完整性
 */
static bool ValidateFrame(const uint8_t* frame, uint16_t frameLen) {
    if (frameLen < 7) {
        PROTOCOL_DEBUG_LOG("Frame too short: %d\n", frameLen);
        return false;
    }
    
    // 检查帧头
    if (frame[0] != FRAME_HEADER1 || frame[1] != FRAME_HEADER2) {
        PROTOCOL_DEBUG_LOG("Invalid header: 0x%02X 0x%02X\n", frame[0], frame[1]);
        return false;
    }
    
    // 检查控制字
    if (frame[2] != CONTROL_WORD) {
        PROTOCOL_DEBUG_LOG("Invalid control word: 0x%02X\n", frame[2]);
        return false;
    }
    
    // 【修改点】帧长度解析：小端序（低字节在前）
    // 原：大端序 uint16_t dataLen = (frame[4] << 8) | frame[5];
    // 新：小端序
    uint16_t dataLen = frame[4] | (frame[5] << 8);  // 低字节在前
    uint16_t expectedLen = 7 + dataLen;
    
    PROTOCOL_DEBUG_LOG("dataLen=%d, expectedLen=%d, actualLen=%d\n", 
                       dataLen, expectedLen, frameLen);
    
    if (frameLen != expectedLen) {
        PROTOCOL_DEBUG_LOG("Length mismatch: expected=%d, actual=%d\n", expectedLen, frameLen);
        return false;
    }
    
    // 验证校验和（从帧头开始到数据段结束，即 frameLen - 1 字节）
    uint8_t calcChecksum = Protocol_CalcChecksum(frame, frameLen - 1);
    uint8_t receivedChecksum = frame[frameLen - 1];
    
    if (calcChecksum != receivedChecksum) {
        PROTOCOL_DEBUG_LOG("Checksum mismatch: calc=0x%02X, recv=0x%02X\n", 
                           calcChecksum, receivedChecksum);
        return false;
    }
    
    return true;
}

/**
 * @brief 解析帧，提取命令和参数
 */
static bool ParseFrame(const uint8_t* frame, uint16_t frameLen,
                       CommandCode* cmd, uint8_t* subCmd,
                       const uint8_t** params, uint16_t* paramLen) {
    if (!ValidateFrame(frame, frameLen)) {
        return false;
    }
    
    // 提取命令码
    *cmd = (CommandCode)frame[3];
    
    // 【修改点】提取数据段长度：小端序（低字节在前）
    uint16_t dataLen = frame[4] | (frame[5] << 8);
    
    PROTOCOL_DEBUG_LOG("cmd=0x%02X, dataLen=%d\n", *cmd, dataLen);
    
    if (dataLen == 0) {
        *subCmd = 0;
        *params = NULL;
        *paramLen = 0;
        return true;
    }
    
    // 数据段：第6字节开始
    const uint8_t* dataSegment = &frame[6];
    *subCmd = dataSegment[0];              // 第1字节是子命令码
    
    if (dataLen > 1) {
        *params = &dataSegment[1];         // 剩余字节是参数
        *paramLen = dataLen - 1;
    } else {
        *params = NULL;
        *paramLen = 0;
    }
    
    return true;
}

/**
 * @brief 构建完整帧
 */
static bool BuildFrame(uint8_t* frame, uint16_t* frameLen, CommandCode cmd,
                       const uint8_t* data, uint16_t dataLen) {
    if (dataLen > MAX_DATA_SIZE) {
        PROTOCOL_DEBUG_LOG("Data too large: %d > %d\n", dataLen, MAX_DATA_SIZE);
        return false;
    }
    
    uint16_t idx = 0;
    
    // 1. 填充帧头
    frame[idx++] = FRAME_HEADER1;      // 0xA5
    frame[idx++] = FRAME_HEADER2;      // 0x5A
    
    // 2. 填充控制字
    frame[idx++] = CONTROL_WORD;       // 0x24
    
    // 3. 填充命令码
    frame[idx++] = (uint8_t)cmd;
    
    // 4. 【修改点】填充帧长度：小端序（低字节在前）
    frame[idx++] = dataLen & 0xFF;           // 低8位在前
    frame[idx++] = (dataLen >> 8) & 0xFF;    // 高8位在后
    
    // 5. 填充数据段
    if (dataLen > 0 && data != NULL) {
        memcpy(&frame[idx], data, dataLen);
        idx += dataLen;
    }
    
    // 6. 计算并填充校验和（从帧头开始到数据段结束，即 idx 字节）
    uint8_t checksum = Protocol_CalcChecksum(frame, idx);
    frame[idx++] = checksum;
    
    *frameLen = idx;
    return true;
}

/**
 * @brief 初始化协议处理器
 */
void Protocol_Init(ProtocolHandler* handler, 
                   ProtocolSendCallback sendCb,
                   ProtocolEventCallback eventCb) {
    if (handler == NULL) return;
    
    memset(handler, 0, sizeof(ProtocolHandler));
    handler->sendCallback = sendCb;
    handler->eventCallback = eventCb;
    
    PROTOCOL_DEBUG_LOG("Protocol handler initialized\n");
}

/**
 * @brief 注册命令处理器
 */
void Protocol_RegisterHandler(ProtocolHandler* handler, 
                              CommandCode cmd, 
                              CommandHandler handlerFunc) {
    if (handler == NULL) return;
    
    if (cmd >= 0 && cmd < 256) {
        handler->handlers[cmd] = handlerFunc;
        PROTOCOL_DEBUG_LOG("Registered handler for cmd=0x%02X\n", cmd);
    }
}

/**
 * @brief 处理接收到的数据包
 */
bool Protocol_ProcessPacket(ProtocolHandler* handler, 
                            const uint8_t* data, 
                            uint16_t len) {
    if (handler == NULL || data == NULL || len == 0) {
        return false;
    }
    
    PROTOCOL_DEBUG_LOG("Rx packet, len=%d\n", len);
    
    // 解析帧
    CommandCode cmd;
    uint8_t subCmd;
    const uint8_t* params;
    uint16_t paramLen;
    
    if (!ParseFrame(data, len, &cmd, &subCmd, &params, &paramLen)) {
        PROTOCOL_DEBUG_LOG("Frame parse failed\n");
        return false;
    }
    
    PROTOCOL_DEBUG_LOG("Parsed: cmd=0x%02X, subCmd=0x%02X, paramLen=%d\n", 
                       cmd, subCmd, paramLen);
    
    // 查找并执行命令处理器
    if (handler->handlers[cmd] != NULL) {
        uint8_t response[MAX_DATA_SIZE];
        uint16_t responseLen = 0;
        
        bool success = handler->handlers[cmd](subCmd, params, paramLen, 
                                               response, &responseLen);
        
        if (success) {
            Protocol_SendResponse(handler, cmd, STATUS_OK, response, responseLen);
        } else {
            Protocol_SendResponse(handler, cmd, STATUS_INVALID_PARAM, NULL, 0);
        }
    } else {
        PROTOCOL_DEBUG_LOG("No handler for cmd=0x%02X\n", cmd);
        Protocol_SendResponse(handler, cmd, STATUS_INVALID_CMD, NULL, 0);
    }
    
    // 触发事件回调（可选）
    if (handler->eventCallback != NULL) {
        handler->eventCallback(cmd, subCmd, params, paramLen);
    }
    
    return true;
}

/**
 * @brief 发送请求帧
 */
bool Protocol_SendRequest(ProtocolHandler* handler,
                          CommandCode cmd,
                          uint8_t subCmd,
                          const uint8_t* params,
                          uint16_t paramLen) {
    if (handler == NULL || handler->sendCallback == NULL) {
        return false;
    }
    
    // 构建数据段：子命令码 + 参数
    uint8_t data[MAX_DATA_SIZE];
    data[0] = subCmd;
    
    if (paramLen > 0 && params != NULL) {
        if (paramLen > MAX_DATA_SIZE - 1) {
            paramLen = MAX_DATA_SIZE - 1;
        }
        memcpy(&data[1], params, paramLen);
    }
    
    uint16_t totalDataLen = 1 + paramLen;
    
    // 构建帧
    uint8_t frame[MAX_FRAME_SIZE];
    uint16_t frameLen;
    
    if (!BuildFrame(frame, &frameLen, cmd, data, totalDataLen)) {
        return false;
    }
    
    // 发送
    handler->sendCallback(frame, frameLen);
    
    PROTOCOL_DEBUG_LOG("Sent request: cmd=0x%02X, subCmd=0x%02X, len=%d\n", 
                       cmd, subCmd, frameLen);
    
    return true;
}

/**
 * @brief 发送响应帧
 */
bool Protocol_SendResponse(ProtocolHandler* handler,
                           CommandCode cmd,
                           ProtocolStatus status,
                           const uint8_t* extraData,
                           uint16_t extraDataLen) {
    if (handler == NULL || handler->sendCallback == NULL) {
        return false;
    }
    
    // 构建数据段：状态码 + 附加数据
    uint8_t data[MAX_DATA_SIZE];
    data[0] = (uint8_t)status;
    
    if (extraDataLen > 0 && extraData != NULL) {
        if (extraDataLen > MAX_DATA_SIZE - 1) {
            extraDataLen = MAX_DATA_SIZE - 1;
        }
        memcpy(&data[1], extraData, extraDataLen);
    }
    
    uint16_t totalDataLen = 1 + extraDataLen;
    
    // 构建帧
    uint8_t frame[MAX_FRAME_SIZE];
    uint16_t frameLen;
    
    if (!BuildFrame(frame, &frameLen, cmd, data, totalDataLen)) {
        return false;
    }
    
    // 发送
    handler->sendCallback(frame, frameLen);
    
    PROTOCOL_DEBUG_LOG("Sent response: cmd=0x%02X, status=0x%02X, len=%d\n", 
                       cmd, status, frameLen);
    
    return true;
}

#include "protocol_queue.h"

ProtocolHandler g_protocolHandler;
ProtocolQueue g_sendQueue;        // 全局发送队列

/*=============================================================================
 * 压入队列的回调函数（这个函数传给Protocol_Init）
 * 作用：将协议模块要发送的消息压入队列，而不是直接发送
 *============================================================================*/
static void EnqueueToQueue(const uint8_t* data, uint16_t len) {
    ProtocolQueue_Enqueue(&g_sendQueue, data, len);
}


// 初始化
void Mv_AppProtocol_Init(void) {
    
    ProtocolQueue_Init(&g_sendQueue);

    // 1. 初始化
    Protocol_Init(&protocol, EnqueueToQueue, NULL);
    
    // 2. 注册所有命令处理器
    Protocol_RegisterHandler(&protocol, CMD_HANDSHAKE, HandshakeCommandHandler);
    Protocol_RegisterHandler(&protocol, CMD_COMMON_CONTROL, CommonControlCommandHandler);
    Protocol_RegisterHandler(&protocol, CMD_LED_EFFECT, LedEffectCommandHandler);
    Protocol_RegisterHandler(&protocol, CMD_MIC_EFFECT, MicEffectCommandHandler);
    Protocol_RegisterHandler(&protocol, CMD_MUSIC_EFFECT, MusicEffectCommandHandler);
    Protocol_RegisterHandler(&protocol, CMD_QUERY_STATE, QueryStateCommandHandler);
    Protocol_RegisterHandler(&protocol, CMD_LE_AUDIO, LeAudioCommandHandler);
}


/**
 * @brief BLE数据接收回调
 * @param data 接收到的数据指针
 * @param len 数据长度
 * 
 * 这个函数由BLE协议栈在收到数据时调用
 * 你需要将BLE接收到的数据传入这里
 */
void mvble_data_received(const uint8_t* data, uint16_t len) {
    // 直接调用协议处理函数
    // 这里会解析帧、调用命令处理器、自动发送响应
    Protocol_ProcessPacket(&protocol, data, len);
}
#endif