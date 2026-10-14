/*
 * dualcore_api.h
 *
 *  Created on: Sep 28, 2025
 *      Author: jiaozhenqiang
 */

#ifndef __DUALCORE_API_H__
#define __DUALCORE_API_H__

#include "dualcore.h"

//core2 UART setting
#define UART2_NULL			(0)
#define UART2_TA19_RA18		(1)
#define UART2_TA29_RA28		(2)//默认用A29
#define UART2_TA9_RA8		(3)
#define UART2_TB7_RB6		(4)
#define UART2_TB21_RB20		(5)//AUX 会使用，确认没有冲突才配置

#ifdef SDK_DUALCORE_DEMO
#include "project_config.h"
#endif

void IpcInit(void);
bool IpcReadOneItem(uint32_t* pItem);
bool IpcSendOneItem(uint32_t item);

#ifdef RTOS_MSGBUFF_EN
bool IpcSendOSCmd(uint32_t cmd, uint32_t param);
bool IpcSendAppCmd(uint32_t cmd, uint32_t param);
#endif

void HsemInit(void);
void HsemLockByQuery(int idx);
void HsemLockWithInterrupt(int idx);
void HsemUnlock(int idx);

void DualCoreModuleInit(void);

void UartInit_Core2(void);

#endif /* __DUALCORE_API_H__ */
