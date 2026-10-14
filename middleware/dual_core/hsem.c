/*
 * hsem.c
 *
 *  Created on: Sep 29, 2025
 *      Author: jiaozhenqiang
 */
#include "delay.h"
#include "dualcore_api.h"
#include "timeout.h"
#ifdef CFG_APP_CONFIG
	#include "app_config.h"
#endif

#ifdef DUAL_CORE_EN

#define HSEM_MAXIDX				(32)

#define HSEM_KEY				(0xABCD)
#define HSEM_NO					(2)

#define PROC_ID_CORE1			(0x50)
#define PROC_ID_CORE2			(0xA0)

volatile uint8_t gaU8IntLockOkFlag[HSEM_MAXIDX] = {0};
static uint8_t gU8ProcID = 0;

void HsemInit(void)
{
	if (DualCore_IsCurCore1())
	{
		gU8ProcID = PROC_ID_CORE1;
	}
	else
	{
		gU8ProcID = PROC_ID_CORE2;
	}
	HSEM_Init(HSEM_KEY);
}

void HsemLockByQuery(int idx)
{
	if (idx < HSEM_MAXIDX)
	{
		while (HSEM_Lock(idx, gU8ProcID+idx) != E_HSEMOPT_OK)
		{
			DelayUs(5);
		}
	}
}

void HsemLockWithInterrupt(int idx)
{
	if (idx < HSEM_MAXIDX)
	{
		gaU8IntLockOkFlag[idx] = 0;
		while (HSEM_LockWithIntHandle(HSEM_NO, gU8ProcID+idx) != E_HSEMOPT_OK)
		{
			while (!gaU8IntLockOkFlag[idx]);   //此处模拟 OS 操作中睡眠挂起等行为，等待事件唤醒
		}
	}
}

void HsemUnlock(int idx)
{
	if (idx < HSEM_MAXIDX)
	{
		HSEM_Unlock(idx, gU8ProcID+idx);
	}
}

//HSEM 中断处理 回调函数
__attribute__((section(".dcdriver.isr")))
void HSEM_LockSuccessCallback(int i)
{
	gaU8IntLockOkFlag[i] = 1;
}

#endif
