/*
 * dualcore.h
 *
 *  Created on: Sep 12, 2025
 *      Author: jiaozhenqiang
 */

#ifndef __DUALCORE_H__
#define __DUALCORE_H__

#include "type.h"

typedef enum
{
	COREID_UNKNOWN = 0,
	COREID_CORE1,
	COREID_CORE2
} TE_COREID;

/**
 * @brief  Get current core ID
 * @param  None
 * @return COREID_CORE1: core1
           COREID_CORE2: core2
           COREID_UNKNOWN: unable to judge the core id
 */
TE_COREID DualCore_GetCurCoreID(void);

/**
 * @brief  Determine whether it is currently running on core1
 * @param  None
 * @return TRUE or FALSE
 */
bool DualCore_IsCurCore1(void);

/**
 * @brief  Set the startup address of core2 and release it to run 
 * @param  u32Addr: Core2 startup address, it should be 64K byte aligned.
 * @return None
 * @note   This code should be called from core1 only.
 */
void DualCore_ReleaseCore2ToRun(uint32_t u32Addr);


/************************************* IPC Function API **************************************/


/**
 * @brief  IPC function initialization. It enabled IPC interrupt and then masked it.
 * @param  None
 * @return None
 * @note   This function should be called before using any other IPC interface and called earlier from core1 than from core2.
 */
void IPC_Init(void);

/**
 * @brief  Disable IPC function.
 * @param  None
 * @return None
 */
void IPC_Disable(void);

/**
 * @brief  Set the masking status of IPC interrupt.
 * @param  doMask: Indicate the operation to be carried out.
 * @return None
 */
void IPC_SetRcvIntMask(bool doMask);

/**
 * @brief  Check whether the IPC interrupt is masked.
 * @param  None
 * @return TRUE: Masked; FALSE£ºNot masked;
 */
bool IPC_IsRcvIntMasked(void);

/**
 * @brief  Get the receiving count of IPC. The unit of measurement is a 32-bit word. 
 * @param  None
 * @return Receiving count, maximum value is 8.
 */
uint32_t IPC_GetRcvCount(void);

/**
 * @brief  Get the free space of IPC sending FIFO. 
 * @param  None
 * @return The free space, maximum value is 8.
 */
uint32_t IPC_GetFreeSndSpace(void);

/**
 * @brief  Receive data from IPC.
 * @param  pBuf: Pointer to the receive buffer.
 * @param  rcvCount: Buffer length of 32bit word.
 * @return The number of 32bit words received from IPC and stored in the buffer.
 */
uint32_t IPC_Recv(uint32_t* pBuf, uint32_t rcvCount);

/**
 * @brief  Send data to IPC.
 * @param  pData: Pointer to the data to be sent.
 * @param  sndCount: The count of data to be sent.
 * @return The number of 32bit words that have been sent to the IPC FIFO.
 */
uint32_t IPC_Send(uint32_t* pData, uint32_t sndCount);


/************************************* PLMT Function API **************************************/


typedef enum
{
	E_PLMT_CLK_HOSC = 0,
	E_PLMT_CLK_HRC = 1
} TE_PLMT_CLKSRC;


/**
 * @brief  PLMT initialization.
 * @param  clkSrc: Clock source.
 * @param  countCycleUs: Counting cycle duration, unit: us.
 * @param  c1IntIntervalUs: Time interval between the occurrence of core1 interrupt events starting from the beginning of the counting cycle.
 * @param  c2IntIntervalUs: Time interval between the occurrence of core2 interrupt events starting from the beginning of the counting cycle.
 * @param  bIsOPM: Set the operation mode to single-shot mode or continuous mode. TRUE - single-shot mode.
 * @param  bC1IntEna: Set whether to enable the core1 interrupt.
 * @param  bC2IntEna: Set whether to enable the core1 interrupt
 * @return None.
 * @note   This function is only effective when called from core1.
 */
void PLMT_Init(TE_PLMT_CLKSRC clkSrc, uint32_t countCycleUs, uint32_t c1IntIntervalUs, uint32_t c2IntIntervalUs, bool bIsOPM, bool bC1IntEna, bool bC2IntEna);

/**
 * @brief PLMT initialization.
* @param  None
* @return None
* @note   This function is only effective when called from core1.
*/
void PLMT_Disable(void);

/**
 * @brief  Enable or disable PLMT interrupt on current core.
 * @param  bIsEna£º TRUE - enable; FALSE - disable.
 * @return None
 */
void PLMT_SetInterrupt(bool bIsEna);

/**
* @brief  Set the PLMT interrupt interval of the current core.
* @param  intIntervalUS: Interval, unit:us.
* @return None
*/
void PLMT_SetIntInterval(uint32_t intIntervalUS);

/**
* @brief  Clear the current core's PLMT interrupt flag. 
* @param  None
* @return None
*/
void PLMT_ClearIntFlag(void);

/**
* @brief  Get the current counter value of the PLMT timer. 
* @param  None
* @return PLMT counter value.
*/
uint32_t PLMT_GetCurCntValue(void);



/************************************* HSEM Function API **************************************/


typedef enum
{
	E_HSEMOPT_OK = 0,
	E_HSEMOPT_INVALID_IDX,
	E_HSEMOPT_LOCKED_BY_ANOTHER_CORE,
	E_HSEMOPT_LOCKED_BY_ANOTHER_PROC,
	E_HSEMOPT_IGNOREPROC_FLAG_NOT_MATCH,
	E_HSEMOPT_LOCK_FAIL,
	E_HSEMOPT_UNLOCK_FAIL,
} TE_HSEMOpt_Status;

//core1 ÏÈµ÷
/**
 * @brief  HSEM initialization.
 * @param  clearKey£ºA key than can be used with HSEM_UnlockByClearKey() to unlock all HSEM that locked by current core. 
 * @return None.
 * @note   This function should be called before using any other IPC interface and called earlier from core1 than from core2.
 */
void HSEM_Init(uint16_t clearKey);

/**
 * @brief  Disable HSEM function.
 * @param  None
 * @return None
 */
void HSEM_Disable(void);

/**
 * @brief  Enable HSEM interrupt.
 * @param  u8_idx: The index number of HSEM.
 * @return None
 */
void HSEM_IntEna(uint8_t u8_idx);


/**
 * @brief  Disable HSEM interrupt.
 * @param  u8_idx: The index number of HSEM.
 * @return None
 */
void HSEM_IntDis(uint8_t u8_idx);

/**
 * @brief  Clear the interrupt flag of one HSEM.
 * @param  u8_idx: The index number of HSEM.
 * @return None
 */
void HSEM_IntClr(uint8_t u8_idx);

/**
 * @brief  Clear the interrupt flag of all HSEM.
 * @param  None
 * @return None
 */
void HSEM_IntClrAll(void);


/**
 * @brief  Check the interrupt flag of one HSEM.
 * @param  u8_idx: The index number of HSEM.
 * @return None
 */
bool HSEM_IsIntMISR(uint8_t u8_idx);

/**
 * @brief  Try to get lock using one HSEM.
 * @param  u8_idx: The index number of HSEM.
 * @param  u8_procid: A 8bit ID code to represent the current task.
 * @return Operation result code.
 */
TE_HSEMOpt_Status HSEM_Lock(uint8_t u8_idx, uint8_t u8_procid);

/**
 * @brief  Try to get lock using one HSEM with interrupt handler.
 * @param  u8_idx: The index number of HSEM.
 * @param  u8_procid: A 8bit ID code to represent the current task.
 * @return Operation result code
 */
TE_HSEMOpt_Status HSEM_LockWithIntHandle(uint8_t u8_idx, uint8_t u8_procid);

/**
 * @brief  Try to unlock one HSEM.
 * @param  u8_idx: The index number of HSEM.
 * @param  u8_procid: A 8bit ID code to represent the current task.
 * @return Operation result code
 */
TE_HSEMOpt_Status HSEM_Unlock(uint8_t u8_idx, uint8_t u8_procid);

/**
 * @brief  Try to unlock all HSEM that locked by current core with a clearkey.
 * @param  clearKey: A 16bit key.
 * @return None
 */
void HSEM_UnlockByClearKey(uint16_t clearKey);

/********************************* MEM Debug Funciton ****************************************/

typedef enum _BUS_MASTER_ID
{
	E_BUS_MASTER_ID_NONE = 0,
	E_BUS_MASTER_ID_CORE1_DUBS = 3,
	E_BUS_MASTER_ID_DMA0 = 4,
	E_BUS_MASTER_ID_DMA1 = 5,
	E_BUS_MASTER_ID_CORE2_DUBS = 10,
	E_BUS_MASTER_ID_DMA2 = 11
} TE_BUS_MASTER_ID;

typedef enum
{
	E_MEM_SEG_0 = 0,
	E_MEM_SEG_1,
	E_MEM_SEG_2,
	E_MEM_SEG_3,
	E_MEM_SEG_4,
	E_MEM_SEG_5,
	E_MEM_SEG_6,
	E_MEM_SEG_7,
	E_MEM_SEG_8,
	E_MEM_SEG_9,
	E_MEM_SEG_10
} TE_MEM_SEG_NO;

/**
 * @brief  Set the write access limit ID of memory segment.
 * @param  eMem: The memory segment to set. 
 * @param  eMasterId: The bus master ID to limit write access
 * @return Setting result: TRUE-success; FLASE - failed.
 */
bool MEMDBG_SetLimitID(TE_MEM_SEG_NO eMem, TE_BUS_MASTER_ID eMasterId);

/**
 * @brief  Get the write access limit ID of memory segment.
 * @param  eMem: The memory segment to query. 
 * @return Bus master ID
 */
TE_BUS_MASTER_ID MEMDBG_GetLimitID(TE_MEM_SEG_NO eMem);

#endif /* __DUALCORE_H__ */

