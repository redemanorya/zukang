/**
 **************************************************************************************
 * @file    protocol_queue.h
 * @brief   协议消息队列模块
 * @details 提供消息队列的初始化、入队、出队等功能
 *
 * @author  Jarvis
 * @version V1.0.0
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#ifndef PROTOCOL_QUEUE_H
#define PROTOCOL_QUEUE_H

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
#define PROTOCOL_QUEUE_SIZE         32          // 队列最大消息数
#define MAX_FRAME_SIZE              512         // 最大帧长度

/*=============================================================================
 * 结构体定义
 *============================================================================*/
// 消息节点
typedef struct {
    uint8_t         frame[MAX_FRAME_SIZE];  // 完整的帧数据
    uint16_t        frameLen;               // 帧长度
} ProtocolMessage;

// 消息队列
typedef struct {
    ProtocolMessage msgs[PROTOCOL_QUEUE_SIZE];
    volatile uint16_t head;         // 队列头（读位置）
    volatile uint16_t tail;         // 队列尾（写位置）
    volatile uint16_t count;        // 队列中消息数量
} ProtocolQueue;

/*=============================================================================
 * API函数声明
 *============================================================================*/

/**
 * @brief 初始化消息队列
 */
void ProtocolQueue_Init(ProtocolQueue* queue);

/**
 * @brief 消息入队
 * @param queue     队列句柄
 * @param frame     帧数据指针
 * @param frameLen  帧长度
 * @return true=入队成功，false=队列已满
 */
bool ProtocolQueue_Enqueue(ProtocolQueue* queue, const uint8_t* frame, uint16_t frameLen);

/**
 * @brief 消息出队
 * @param queue 队列句柄
 * @param frame 输出帧数据缓冲区
 * @param frameLen 输出帧长度指针
 * @return true=出队成功，false=队列为空
 */
bool ProtocolQueue_Dequeue(ProtocolQueue* queue, uint8_t* frame, uint16_t* frameLen);

/**
 * @brief 获取队列中的消息数量
 */
uint16_t ProtocolQueue_GetCount(ProtocolQueue* queue);

/**
 * @brief 判断队列是否为空
 */
bool ProtocolQueue_IsEmpty(ProtocolQueue* queue);

/**
 * @brief 判断队列是否已满
 */
bool ProtocolQueue_IsFull(ProtocolQueue* queue);

/**
 * @brief 清空队列
 */
void ProtocolQueue_Clear(ProtocolQueue* queue);
#endif

#ifdef __cplusplus
}
#endif

#endif /* PROTOCOL_QUEUE_H */