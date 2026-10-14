/**
 **************************************************************************************
 * @file	ipc_interface.c
 * @brief	ipc interface
 *
 * @author	jiaozhenqiang
 * @version V1.0.0
 *
 * $Created: 2025-11-14 55:50$
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */



#include "type.h"
#include "watchdog.h"
#include "core_d1088.h"

#ifdef CFG_APP_CONFIG
	#include "app_config.h"
#endif

#ifdef SDK_DUALCORE_DEMO
#include "project_config.h"
#endif

#ifdef FUNC_OS_EN
#include "rtos_api.h"
#endif

#ifdef DUAL_CORE_EN


#include "ipc_interface.h"

#ifdef FUNC_OS_EN
osMutexId DCoreIPCMutex = NULL;
#endif

static void DCOS_IpcLock(void)
{
#ifdef FUNC_OS_EN
//	if (NULL != DCoreIPCMutex)
	{
		osMutexLock(DCoreIPCMutex);
	}
#endif
}

static void DCOS_IpcUnlock(void)
{
#ifdef FUNC_OS_EN
//	if (NULL != DCoreIPCMutex)
	{
		osMutexUnlock(DCoreIPCMutex);
	}
#endif
}

void DCOS_IPC_Init(void)
{
	DCOS_IpcLock();
	IPC_Init();
	DCOS_IpcUnlock();
}

void DCOS_IPC_Disable(void)
{
	DCOS_IpcLock();
	IPC_Disable();
	DCOS_IpcUnlock();
}

void DCOS_IPC_SetRcvIntMask(bool doMask)
{
	DCOS_IpcLock();
	IPC_SetRcvIntMask(doMask);
	DCOS_IpcUnlock();
}

bool DCOS_IPC_IsRcvIntMasked(void)
{
	bool bRet;

	DCOS_IpcLock();
	bRet = IPC_IsRcvIntMasked();
	DCOS_IpcUnlock();

	return bRet;
}

uint32_t DCOS_IPC_GetRcvCount(void)
{
	uint32_t nRet;

	DCOS_IpcLock();
	nRet = IPC_GetRcvCount();
	DCOS_IpcUnlock();

	return nRet;
}

uint32_t DCOS_IPC_GetFreeSndSpace(void)
{
	uint32_t nRet;

	DCOS_IpcLock();
	nRet = IPC_GetFreeSndSpace();
	DCOS_IpcUnlock();

	return nRet;
}


uint32_t DCOS_IPC_Recv(uint32_t* pBuf, uint32_t rcvCount)
{
	uint32_t nRet;

	DCOS_IpcLock();
	nRet = IPC_Recv(pBuf, rcvCount);
	DCOS_IpcUnlock();

	return nRet;
}

uint32_t DCOS_IPC_Send(uint32_t* pData, uint32_t sndCount)
{
	uint32_t nRet;

	DCOS_IpcLock();
	nRet = IPC_Send(pData, sndCount);
	DCOS_IpcUnlock();

	return nRet;
}

#endif

