/**
 **************************************************************************************
 * @file    ipc_interface.h
 * @brief   ipc interface
 *
 * @author  jiaozhenqiang
 * @version V1.0.0
 *
 * $Created: 2025-10-14 18:53:05$
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#ifndef __IPC_INTERFACE_H__
#define __IPC_INTERFACE_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#include "dualcore.h"

//#ifdef FUNC_OS_EN
//#include "rtos_api.h"
//extern osMutexId DCoreIPCMutex;
//#endif

/**
 * @brief  IPC function initialization. It enabled IPC interrupt and then masked it.
 * @param  None
 * @return None
 * @note   This function should be called before using any other IPC interface and called earlier from core1 than from core2.
 */
void DCOS_IPC_Init(void);

/**
 * @brief  Disable IPC function.
 * @param  None
 * @return None
 */
void DCOS_IPC_Disable(void);

/**
 * @brief  Set the masking status of IPC interrupt.
 * @param  doMask: Indicate the operation to be carried out.
 * @return None
 */
void DCOS_IPC_SetRcvIntMask(bool doMask);

/**
 * @brief  Check whether the IPC interrupt is masked.
 * @param  None
 * @return TRUE: Masked; FALSE£ºNot masked;
 */
bool DCOS_IPC_IsRcvIntMasked(void);

/**
 * @brief  Get the receiving count of IPC. The unit of measurement is a 32-bit word.
 * @param  None
 * @return Receiving count, maximum value is 8.
 */
uint32_t DCOS_IPC_GetRcvCount(void);

/**
 * @brief  Get the free space of IPC sending FIFO.
 * @param  None
 * @return The free space, maximum value is 8.
 */
uint32_t DCOS_IPC_GetFreeSndSpace(void);

/**
 * @brief  Receive data from IPC.
 * @param  pBuf: Pointer to the receive buffer.
 * @param  rcvCount: Buffer length of 32bit word.
 * @return The number of 32bit words received from IPC and stored in the buffer.
 */
uint32_t DCOS_IPC_Recv(uint32_t* pBuf, uint32_t rcvCount);

/**
 * @brief  Send data to IPC.
 * @param  pData: Pointer to the data to be sent.
 * @param  sndCount: The count of data to be sent.
 * @return The number of 32bit words that have been sent to the IPC FIFO.
 */
uint32_t DCOS_IPC_Send(uint32_t* pData, uint32_t sndCount);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif//
