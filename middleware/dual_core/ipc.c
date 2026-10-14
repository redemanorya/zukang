/*
 * ipc.c
 *
 *  Created on: Sep 28, 2025
 *      Author: jiaozhenqiang
 */
#include "dualcore_api.h"
#include "irqn.h"
#include "gpio.h"

#ifdef CFG_APP_CONFIG
#include "app_config.h"

#ifdef CUR_CORE1
#include "app_message.h"
#include "audio_core_service.h"
#endif
#endif

#ifdef DUAL_CORE_EN

#ifdef FUNC_OS_EN
#include "rtos_api.h"
#include "ipc_interface.h"
#ifdef RTOS_MSGBUFF_EN
#include "ipcMessageBuffer.h"
#endif

SemaphoreHandle_t gIPCAppSem;
SemaphoreHandle_t gIPCAppDSPSem;//

#endif

#define IPCMSG_BUF_LEN       (32)
static uint32_t g_u32a_ipcMsg[IPCMSG_BUF_LEN];
static uint8_t g_u8_ipcMsg_rIdx;
static uint8_t g_u8_ipcMsg_wIdx;

void IpcInit(void)
{
#ifdef FUNC_OS_EN
	gIPCAppSem = xSemaphoreCreateBinary();
	gIPCAppDSPSem = xSemaphoreCreateBinary();

	DCOS_IPC_Init();
	if (DCOS_IPC_IsRcvIntMasked())
	{
		DCOS_IPC_SetRcvIntMask(FALSE);
	}
#else
	IPC_Init();
	if (IPC_IsRcvIntMasked())
	{
		IPC_SetRcvIntMask(FALSE);
	}
#endif
	g_u8_ipcMsg_rIdx = 0;
	g_u8_ipcMsg_wIdx = 0;
}

#ifdef FUNC_OS_EN
__attribute__((section(".dcdriver.isr"))) __attribute__((weak))
void IPC_ProcessForISR(uint32_t* pipcItem, BaseType_t* pxHigherPriorityTaskWoken)
{
#ifdef RTOS_MSGBUFF_EN
			TU_IPCITEM cmd;
			cmd.value = *pipcItem;
			if (E_CMDTYPE_OS == cmd.item.type)
			{
				switch (cmd.item.cmd)
				{
					case E_OSCMD_MSGBUFFINIT:
						if (COREID_CORE2 == DualCore_GetCurCoreID())
						{
							IPCMsgbuf_Init();
						}
						break;

					case E_OSCMD_MSGBUFFINIT_OK:
						IPCMsgBuf_SetInitFlag(1);
						break;

					case E_OSCMD_MSGBUFFINIT_FAILED:
						IPCMsgBuf_SetInitFlag(2);
						break;

					case E_OSCMD_MSGSND:
//						DBGPRT("%s got E_OSCMD_MSGSND\n", __func__);
						xMessageBufferSendCompletedFromISR(IPCMsgbuf_GetRcvHandle(), pxHigherPriorityTaskWoken);
						break;

					case E_OSCMD_MSGRCV:
//						DBGPRT("%s got E_OSCMD_MSGRCV\n", __func__);
						xMessageBufferReceiveCompletedFromISR(IPCMsgbuf_GetSndHandle(), pxHigherPriorityTaskWoken);
						break;

					default:
						break;

				}
			}
			else if (E_CMDTYPE_APP == cmd.item.type)
			{

				g_u32a_ipcMsg[g_u8_ipcMsg_wIdx] = *pipcItem;
				g_u8_ipcMsg_wIdx = (g_u8_ipcMsg_wIdx + 1) % IPCMSG_BUF_LEN;
			}
			else
			{
				// unkonwn msg type, skip
			}
#else
			g_u32a_ipcMsg[g_u8_ipcMsg_wIdx] = *pipcItem;
			g_u8_ipcMsg_wIdx = (g_u8_ipcMsg_wIdx + 1) % IPCMSG_BUF_LEN;

#endif
}

#else
__attribute__((section(".dcdriver.isr"))) __attribute__((weak))
void IPC_ProcessForISR(uint32_t* pipcItem)
{
	g_u32a_ipcMsg[g_u8_ipcMsg_wIdx] = *pipcItem;
	g_u8_ipcMsg_wIdx = (g_u8_ipcMsg_wIdx + 1) % IPCMSG_BUF_LEN;
}
#endif

__attribute__((section(".dcdriver.isr")))
void IPC_Interrupt(void)
{
	uint8_t u8_msgCount;
	uint8_t i = 0;
	uint32_t u32BufArr[8];

//	GPIO_RegOneBitSet(GPIO_A_OE, GPIO_INDEX28);
//	GPIO_RegOneBitSet(GPIO_A_OUT, GPIO_INDEX28);
#ifdef FUNC_OS_EN
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
#endif
	u8_msgCount = IPC_Recv(u32BufArr, 8);
	for (i = 0; i < u8_msgCount; i++)
	{
		if (((g_u8_ipcMsg_wIdx + 1) % IPCMSG_BUF_LEN) != g_u8_ipcMsg_rIdx)
		{
#ifdef FUNC_OS_EN
			IPC_ProcessForISR(&u32BufArr[i], &xHigherPriorityTaskWoken);
#else
			IPC_ProcessForISR(&u32BufArr[i]);
#endif
		}
	}
//	GPIO_RegOneBitClear(GPIO_A_OUT, GPIO_INDEX28);
#ifdef FUNC_OS_EN
//	xSemaphoreGiveFromISR(gIPCAppSem, &xHigherPriorityTaskWoken);
#ifdef use_MCPS_ANALYSIS
	trace_dcore_out();
#endif
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
#endif
}

bool IpcReadOneItem(uint32_t* pItem)
{
	bool bRet = FALSE;
	if (pItem)
	{
		if (g_u8_ipcMsg_wIdx != g_u8_ipcMsg_rIdx)
		{
			*pItem = g_u32a_ipcMsg[g_u8_ipcMsg_rIdx];
			g_u8_ipcMsg_rIdx = (g_u8_ipcMsg_rIdx + 1) % IPCMSG_BUF_LEN;

			bRet = TRUE;
		}
	}

	return bRet;
}

bool IpcSendOneItem(uint32_t item)
{
#ifdef FUNC_OS_EN
	if (DCOS_IPC_Send((uint32_t *)&item, 1))
#else
	if (IPC_Send((uint32_t *)&item, 1))
#endif
	{
#ifdef use_MCPS_ANALYSIS
		trace_dcore_in();
#endif
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

#ifdef RTOS_MSGBUFF_EN

bool IpcSendCmd(uint32_t cmd)
{
	if (DCOS_IPC_Send(&cmd, 1))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

bool IpcSendOSCmd(uint32_t cmd, uint32_t param)
{
	TU_IPCITEM uCMD;
	uCMD.item.type = E_CMDTYPE_OS;
	uCMD.item.cmd = cmd;
	uCMD.item.param = param;

	return IpcSendCmd(uCMD.value);
}

bool IpcSendAppCmd(uint32_t cmd, uint32_t param)
{
	TU_IPCITEM uCMD;
	uCMD.item.type = E_CMDTYPE_APP;
	uCMD.item.cmd = cmd;
	uCMD.item.param = param;

	return IpcSendCmd(uCMD.value);
}
#endif

#endif
