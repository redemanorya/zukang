/**
 **************************************************************************************
 * @file    protocol_queue.c
 * @brief   协议消息队列模块
 *
 * @author  Jarvis
 * @version V1.0.0
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include "protocol_queue.h"
#include <string.h>
#include "bt_config.h"
#ifdef MV_APP_CONTROL
/*=============================================================================
 * 调试宏
 *============================================================================*/
#define QUEUE_DEBUG_ENABLE    1

#if QUEUE_DEBUG_ENABLE
#include <stdio.h>
#define QUEUE_DEBUG_LOG(...)   printf("[QUEUE] " __VA_ARGS__)
#else
#define QUEUE_DEBUG_LOG(...)
#endif

/*=============================================================================
 * API函数实现
 *============================================================================*/

void ProtocolQueue_Init(ProtocolQueue* queue) {
    if (queue == NULL) return;
    
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    memset(queue->msgs, 0, sizeof(queue->msgs));
    
    QUEUE_DEBUG_LOG("Queue initialized, size=%d\n", PROTOCOL_QUEUE_SIZE);
}

bool ProtocolQueue_Enqueue(ProtocolQueue* queue, const uint8_t* frame, uint16_t frameLen) {
    if (queue == NULL || frame == NULL || frameLen == 0) {
        return false;
    }
    
    if (queue->count >= PROTOCOL_QUEUE_SIZE) {
        QUEUE_DEBUG_LOG("Queue full! Drop frame, len=%d\n", frameLen);
        return false;
    }
    
    if (frameLen > MAX_FRAME_SIZE) {
        QUEUE_DEBUG_LOG("Frame too large: %d > %d\n", frameLen, MAX_FRAME_SIZE);
        return false;
    }
    
    uint16_t tail = queue->tail;
    ProtocolMessage* msg = &queue->msgs[tail];
    
    msg->frameLen = frameLen;
    memcpy(msg->frame, frame, frameLen);
    
    tail++;
    if (tail >= PROTOCOL_QUEUE_SIZE) {
        tail = 0;
    }
    queue->tail = tail;
    queue->count++;
    
    QUEUE_DEBUG_LOG("Enqueue, len=%d, count=%d\n", frameLen, queue->count);
    
    return true;
}

bool ProtocolQueue_Dequeue(ProtocolQueue* queue, uint8_t* frame, uint16_t* frameLen) {
    if (queue == NULL || frame == NULL || frameLen == NULL) {
        return false;
    }
    
    if (queue->count == 0) {
        return false;
    }
    
    uint16_t head = queue->head;
    ProtocolMessage* msg = &queue->msgs[head];
    
    *frameLen = msg->frameLen;
    memcpy(frame, msg->frame, msg->frameLen);
    
    // 清空已取出的消息
    memset(msg, 0, sizeof(ProtocolMessage));
    
    head++;
    if (head >= PROTOCOL_QUEUE_SIZE) {
        head = 0;
    }
    queue->head = head;
    queue->count--;
    
    QUEUE_DEBUG_LOG("Dequeue, len=%d, count=%d\n", *frameLen, queue->count);
    
    return true;
}

uint16_t ProtocolQueue_GetCount(ProtocolQueue* queue) {
    if (queue == NULL) return 0;
    return queue->count;
}

bool ProtocolQueue_IsEmpty(ProtocolQueue* queue) {
    if (queue == NULL) return true;
    return (queue->count == 0);
}

bool ProtocolQueue_IsFull(ProtocolQueue* queue) {
    if (queue == NULL) return true;
    return (queue->count >= PROTOCOL_QUEUE_SIZE);
}

void ProtocolQueue_Clear(ProtocolQueue* queue) {
    if (queue == NULL) return;
    
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    memset(queue->msgs, 0, sizeof(queue->msgs));
    
    QUEUE_DEBUG_LOG("Queue cleared\n");
}
#endif