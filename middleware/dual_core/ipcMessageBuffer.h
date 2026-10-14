/*
 * ipcMessageBuffer.h
 *
 *  Created on: Jan 4, 2026
 *      Author: jiaozhenqiang
 */

#ifndef __IPCMESSAGEBUFFER_H__
#define __IPCMESSAGEBUFFER_H__

#ifdef RTOS_MSGBUFF_EN

#include "type.h"
#include "FreeRTOS.h"
#include "message_buffer.h"
#include "dualcore_api.h"
#include "project_config.h"

typedef enum
{
	E_CMDTYPE_OS = 0,
	E_CMDTYPE_APP,
} TE_CMD_TYPE;

typedef enum
{
	E_OSCMD_MSGBUFFINIT = 0,
	E_OSCMD_MSGBUFFINIT_OK,
	E_OSCMD_MSGBUFFINIT_FAILED,
	E_OSCMD_MSGSND,
	E_OSCMD_MSGRCV,

} TE_IPC_OSCMD;

typedef struct {
	unsigned long type            :1;
	unsigned long cmd             :7;
	unsigned long param           :24;
} TS_IPCITEM;

typedef union
{
	TS_IPCITEM item;
	uint32_t value;
} TU_IPCITEM;

int IPCMsgbuf_Init(void);
bool IPCMsgbuf_GetInitFlag(void);
void IPCMsgBuf_SetInitFlag(uint32_t value);
MessageBufferHandle_t IPCMsgbuf_GetSndHandle(void);
MessageBufferHandle_t IPCMsgbuf_GetRcvHandle(void);

#endif

#endif /* __IPCMESSAGEBUFFER_H__ */
