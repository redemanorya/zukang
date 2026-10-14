/*
 * ipcMessageBuffer.c
 *
 *  Created on: Oct 15, 2025
 *      Author: jiaozhenqiang
 */
#include "dualcore_api.h"

#ifdef CFG_APP_CONFIG
#include "app_config.h"
#endif

#if (defined(FUNC_OS_EN) && defined(RTOS_MSGBUFF_EN))

#include "ipcMessageBuffer.h"
#include "task.h"

//#define DRV_DBG_ENABLE
#ifdef DRV_DBG_ENABLE
#define DBGPRT(format, ...)		printf(format, ##__VA_ARGS__)
#else
#define  DBGPRT(format, ...)
#endif

#define IPC_MSGBUF_SIZE				(0x200)	//512 bytes
#define IPC_MSGBUF_STARTADDR		(0x60000000)
#define IPC_MSGBUF_ENDADDR			(IPC_MSGBUF_STARTADDR + IPC_MSGBUF_LEN)

#define IPC_MSBBUF_HANDLE_ADDR		(IPC_MSGBUF_STARTADDR)
#define IPC_MSBBUF_CTL_ADDR			(IPC_MSBBUF_HANDLE_ADDR + 8)

#if ((IPC_MSGBUF_SIZE*2 + 0x58) > IPC_MSGBUF_LEN)
#error "Message buffer overflowed !!!!"
#endif

volatile static uint32_t gIPCMsgBufInitOkFlag = 0;  //0: not initialized; 1: init OK; 2: init failed;

bool IPCMsgbuf_GetInitFlag(void)
{
	return  (1 == gIPCMsgBufInitOkFlag)?TRUE:FALSE;
}

void IPCMsgBuf_SetInitFlag(uint32_t value)
{
	gIPCMsgBufInitOkFlag = value;
}

void vSendIPCSbSendCompleteCommand(void * xStreamBuffer)
{
	IpcSendOSCmd(E_OSCMD_MSGSND, 0);
	DBGPRT("Send E_OSCMD_MSGSND, Handle=0x%08X taskHandle=0x%08X\n", xStreamBuffer, GetCurrentTaskHandle());
}

void vSendIPCSbRcvCompleteCommand(void * xStreamBuffer)
{
	IpcSendOSCmd(E_OSCMD_MSGRCV, 0);
	DBGPRT("Send E_OSCMD_MSGRCV, Handle=0x%08X taskHandle=0x%08X\n", xStreamBuffer, GetCurrentTaskHandle());
}

//This function should be called before using any other IPCMsgBuffer interface and called earlier from core1 than from core2.
int IPCMsgbuf_Init(void)
{
	MessageBufferHandle_t handle;
	MessageBufferHandle_t *pHandle;
	uint8_t *pBuffAddr;
	uint8_t *pCtlAddr;
	int nRet = 0;
	uint32_t count = 0;

	if (DualCore_IsCurCore1())
	{
		pHandle = (MessageBufferHandle_t *)IPC_MSBBUF_HANDLE_ADDR;

		pCtlAddr = (uint8_t *)IPC_MSBBUF_CTL_ADDR;
		pBuffAddr = (uint8_t *)pCtlAddr + (sizeof(StaticMessageBuffer_t) * 2);

		handle = xMessageBufferCreateStatic(IPC_MSGBUF_SIZE, pBuffAddr, (StaticStreamBuffer_t *)pCtlAddr);
		if (!handle)
		{
			DBGPRT("xMessageBufferCreateStatic failed!\n");
			nRet = -1;
		}
		else
		{
			*pHandle++ = handle;
			pCtlAddr += sizeof(StaticMessageBuffer_t);
			pBuffAddr += IPC_MSGBUF_SIZE;
			gIPCMsgBufInitOkFlag = 0;

			handle = xMessageBufferCreateStatic(IPC_MSGBUF_SIZE, pBuffAddr, (StaticStreamBuffer_t *)pCtlAddr);

			if (!handle)
			{
				DBGPRT("xMessageBufferCreateStatic 2 failed!\n");
				nRet = -2;
			}
			else
			{
				*pHandle = handle;

			}

			//send cmd, core2 will do init
			IpcSendOSCmd(E_OSCMD_MSGBUFFINIT, 0);

			//wait the result of core2 init
			while(!gIPCMsgBufInitOkFlag)
			{
				osTaskDelay(10);
				if (count++ > 100)
				{
					//time out
					break;
				}
			}

			if (1 != gIPCMsgBufInitOkFlag)
			{
				nRet = -3;
			}
		}
	}
	else
	{
		pHandle = (MessageBufferHandle_t *)IPC_MSBBUF_HANDLE_ADDR;
		if ((((uint32_t)*pHandle) == IPC_MSBBUF_CTL_ADDR) && (((uint32_t)*(pHandle+1)) == (IPC_MSBBUF_CTL_ADDR + sizeof(StaticMessageBuffer_t))))
		{
			gIPCMsgBufInitOkFlag = 1;
			IpcSendOSCmd(E_OSCMD_MSGBUFFINIT_OK, 0);
		}
		else
		{
			gIPCMsgBufInitOkFlag = 2;
			IpcSendOSCmd(E_OSCMD_MSGBUFFINIT_FAILED, 0);
			nRet = -1;
		}
	}

	return nRet;
}

MessageBufferHandle_t IPCMsgbuf_GetSndHandle(void)
{
	if (gIPCMsgBufInitOkFlag)
	{
		if (DualCore_IsCurCore1())
		{
			return (MessageBufferHandle_t)(*(uint32_t *)IPC_MSGBUF_STARTADDR);
		}
		else
		{
			return (MessageBufferHandle_t)(*(uint32_t *)(IPC_MSGBUF_STARTADDR + sizeof(MessageBufferHandle_t)));
		}
	}
	else
	{
		return NULL;
	}
}

MessageBufferHandle_t IPCMsgbuf_GetRcvHandle(void)
{
	if (gIPCMsgBufInitOkFlag)
	{
		if (DualCore_IsCurCore1())
		{
			return (MessageBufferHandle_t)(*(uint32_t *)(IPC_MSGBUF_STARTADDR + sizeof(MessageBufferHandle_t)));
		}
		else
		{
			return (MessageBufferHandle_t)(*(uint32_t *)IPC_MSGBUF_STARTADDR);
		}
	}
	else
	{
		return NULL;
	}
}

#endif
