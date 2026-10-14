/*
 * RTOS_API.c
 *
 *  Created on: Aug 30, 2016
 *      Author: peter
 */
#include <stdint.h>
#include <stddef.h>
#include <nds32_intrinsic.h>
#include "type.h"
#include "string.h"
#include "rtos_api.h"
#include "remap.h"
#include "task.h"
#include "clk.h"

#ifdef CFG_APP_CONFIG
	#include "app_config.h"
#endif

#ifdef SDK_DUALCORE_DEMO
	#include "project_config.h"
#endif

#include "dualcore.h"

#ifdef RTOS_MSGBUFF_EN
	#include "project_config.h"
	#include "ipcMessageBuffer.h"
#endif

#if defined(CFG_RTOS_HEAP5_DCORE_MV) && !defined(CUR_CORE1) && !defined(CUR_CORE2)
	#perror "You should define CUR_CORE1 or CUR_CORE2 in your project configuration!!!!!"
#endif

extern void vPortDefineHeapRegions( const HeapRegion_t * const pxHeapRegions ); //defined by heap_5s.c add for warning by pi
uint32_t GetIPSR( void );
/* Determine whether we are in thread mode or handler mode. */
uint32_t inHandlerMode (void)
{
	//return 0;
	return GetIPSR();
}

bool MessageSend(MessageHandle msgHandle,  MessageContext * msgContext)
{
	portBASE_TYPE taskWoken = pdFALSE;

	if(msgHandle == NULL)
	{
		return  FALSE;
	}
	if (inHandlerMode())
	{
		if (xQueueSendFromISR(msgHandle, msgContext, &taskWoken) != pdTRUE)
		{
			return FALSE;
		}
		portEND_SWITCHING_ISR(taskWoken);
	}
	else
	{
		if (xQueueSend(msgHandle, msgContext, 0) != pdTRUE)
		{
			return FALSE;
		}
	}

	return TRUE;
}

bool MessageSendFromISR(MessageHandle msgHandle,  MessageContext * msgContext)
{
	portBASE_TYPE taskWoken = pdFALSE;

	if (xQueueSendFromISR(msgHandle, msgContext, &taskWoken) != pdTRUE)
	{
		return FALSE;
	}
	portEND_SWITCHING_ISR(taskWoken);
	
	return TRUE;
}

void MessageSendx(MessageHandle msgHandle,  MessageContext * msgContext)
{
	if(msgHandle == NULL)
	{
		return ;
	}

	xQueueSend(msgHandle, msgContext, 0xFFFFFFFF);
}


bool MessageRecv(MessageHandle msgHandle, MessageContext * msgContext, uint32_t millisec)
{
	portBASE_TYPE	taskWoken;
	bool			ret = FALSE;

	if (msgHandle == NULL)
	{
		return FALSE;
	}

	taskWoken = pdFALSE;
	msgContext->msgId = MSG_INVAILD;

	if (inHandlerMode())
	{
		if (xQueueReceiveFromISR(msgHandle, msgContext, &taskWoken) == pdTRUE)
		{
			/* We have mail */
			ret = TRUE;
		}
		else
		{
			ret = FALSE;
		}
		portEND_SWITCHING_ISR(taskWoken);
	}
	else 
	{
		if (xQueueReceive(msgHandle, msgContext, millisec) == pdTRUE)
		{
			/* We have mail */
			ret = TRUE;
		}
		else 
		{
			ret = FALSE;
		}
	}

	return ret;
}

//系统使用的ram结束地址和em的使用大小有关系
void prvInitialiseHeap(void)
{
	extern char _end;
	HeapRegion_t xHeapRegions[2];

	xHeapRegions[0].pucStartAddress = (uint8_t*)&_end;

#ifdef RTOS_MSGBUFF_EN
	if (DualCore_GetCurCoreID() == COREID_CORE1)
	{
		xHeapRegions[0].xSizeInBytes = SRAM_END_ADDR-(uint32_t)&_end;
	}
	else
	{
		xHeapRegions[0].xSizeInBytes = SRAM_END_ADDR_CORE2 - IPC_MSGBUF_LEN -(uint32_t)&_end;
	}
#else

#ifdef CFG_APP_CONFIG
	xHeapRegions[0].xSizeInBytes = BB_MPU_START_ADDR -(uint32_t)&_end;
#else
	xHeapRegions[0].xSizeInBytes = SRAM_END_ADDR-(uint32_t)&_end;
#endif

#endif

#ifdef CFG_RTOS_HEAP5_DCORE_MV
#ifdef CUR_CORE2
	{
		extern char _HEAP_HIGH_;
		xHeapRegions[0].pucStartAddress = (void *)&_HEAP_HIGH_;
	}
#endif
	xHeapRegions[0].xSizeInBytes = config_DCORE_HEAP_INIT_SIZE;

#endif

	xHeapRegions[1].pucStartAddress = NULL;
	xHeapRegions[1].xSizeInBytes = 0;

	vPortDefineHeapRegions( (HeapRegion_t *)xHeapRegions );
}

#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
extern osMutexId Parambin1RWMutex;
extern osMutexId Parambin2RWMutex;
#endif
//note: 该函数必须在prvInitialiseHeap()函数调用之后
void osSemaphoreMutexCreate(void)
{
#if defined(CFG_FUNC_DEBUG_EN) || defined(CFG_FUNC_USBDEBUG_EN)
	if(DBGMutex == NULL)
	{
		DBGMutex = xSemaphoreCreateMutex();
	}
#endif
    if(UARTMutex[0] == NULL)
    {
    	UARTMutex[0] = xSemaphoreCreateMutex();
    }
	if(UARTMutex[1] == NULL)
	{
		UARTMutex[1] = xSemaphoreCreateMutex();
	}
	if(UARTMutex[2] == NULL)
	{
		UARTMutex[2] = xSemaphoreCreateMutex();
	}
//#ifdef CFG_RES_SarADC_USE
	if(SarADCMutex == NULL)
	{
		SarADCMutex = xSemaphoreCreateMutex();
	}
//#endif
#ifdef	FUNC_OS_EN
	if(FlashMutex == NULL)
	{
		FlashMutex = xSemaphoreCreateMutex();
	}
#endif
#ifdef	CFG_RES_CARD_USE
	if(SDIOMutex == NULL)
	{
		SDIOMutex = osMutexCreate();
	}
	if(SDIOSendCommandMutex == NULL)
	{
		SDIOSendCommandMutex = osMutexCreate();
	}
#endif
//#ifdef CFG_RES_UDISK_FS_USE
//	if(UDiskMutex == NULL)
//	{
//		UDiskMutex = osMutexCreate();
//	}
//#endif
//#ifdef CFG_RES_UDISK_HS_USE
//	if(UDiskHsMutex == NULL)
//	{
//		UDiskHsMutex = osMutexCreate();
//	}
//#endif
#if (defined(CFG_APP_BT_MODE_EN))
	if(SbcDecoderMutex == NULL)
	{
		SbcDecoderMutex = osMutexCreate();
	}
#endif
//#ifdef CFG_FUNC_ROBOEFFECT_EN
//	if(AudioEffectMutex == NULL)
//	{
//		AudioEffectMutex = osMutexCreate();
//	}
//#endif
#if defined(CFG_APP_CONFIG) && defined(FUNC_OS_EN)
	if(SysModeMutex == NULL)
	{
		SysModeMutex = osMutexCreate();
	}
#endif
#ifdef CFG_FUNC_REMIND_SOUND_EN
	if(RemindMutex == NULL)
	{
		RemindMutex = osMutexCreate();
	}
#endif
#ifdef DUAL_CORE_EN
	if(DCoreIPCMutex == NULL)
	{
		DCoreIPCMutex = osMutexCreate();
	}
#endif
#ifdef CFG_FUNC_ROBOEFFECT_EN
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
	if(Parambin1RWMutex == NULL)
	{
		Parambin1RWMutex = xSemaphoreCreateMutex();
	}
#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
	if(Parambin2RWMutex == NULL)
	{
		Parambin2RWMutex = xSemaphoreCreateMutex();
	}
#endif
#endif
#endif
}

void osSemaphoreMutexUnlock(void)//bkd add 2021.04.25
{
#if defined(CFG_FUNC_DEBUG_EN) || defined(CFG_FUNC_USBDEBUG_EN)
	if(DBGMutex != NULL)
	{
		osMutexUnlock(DBGMutex);
	}
#endif
	if(UARTMutex[0] != NULL)
	{
		osMutexUnlock(UARTMutex[0]);
	}
	if(UARTMutex[1] != NULL)
	{
		osMutexUnlock(UARTMutex[1]);
	}
	if(UARTMutex[2] != NULL)
	{
		osMutexUnlock(UARTMutex[2]);
	}
//#ifdef	CFG_RES_SarADC_USE
	if(SarADCMutex != NULL)
	{
		osMutexUnlock(SarADCMutex);
	}
//#endif
#ifdef	FUNC_OS_EN
	if(FlashMutex != NULL)
	{
		osMutexUnlock(FlashMutex);
	}
#endif
#ifdef	CFG_RES_CARD_USE
	if(SDIOMutex != NULL)
	{
		osMutexUnlock(SDIOMutex);
	}
	if(SDIOSendCommandMutex != NULL)
	{
		osMutexUnlock(SDIOSendCommandMutex);
	}

#endif
//#ifdef CFG_RES_UDISK_FS_USE
//	if(UDiskMutex != NULL)
//	{
//		osMutexUnlock(UDiskMutex);
//	}
//#endif
//#ifdef CFG_RES_UDISK_HS_USE
//	if(UDiskHsMutex != NULL)
//	{
//		osMutexUnlock(UDiskHsMutex);
//	}
//#endif
#ifdef CFG_APP_BT_MODE_EN
	if(SbcDecoderMutex != NULL)
	{
		osMutexUnlock(SbcDecoderMutex);
	}
#endif
//#ifdef CFG_FUNC_ROBOEFFECT_EN
//	if(AudioEffectMutex != NULL)
//	{
//		osMutexUnlock(AudioEffectMutex);
//	}
//#endif
#if defined(CFG_APP_CONFIG) && defined(FUNC_OS_EN)
	if(SysModeMutex != NULL)
	{
		osMutexUnlock(SysModeMutex);
	}
#endif
#ifdef CFG_FUNC_REMIND_SOUND_EN
	if(RemindMutex != NULL)
	{
		osMutexUnlock(RemindMutex);
	}
#endif
#ifdef CFG_FUNC_ROBOEFFECT_EN
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
	if(Parambin1RWMutex != NULL)
	{
		osMutexUnlock(Parambin1RWMutex);
	}
#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
	if(Parambin2RWMutex != NULL)
	{
		osMutexUnlock(Parambin2RWMutex);
	}
#endif
#endif
#endif
	DecoderOslockUnlock();
}

BaseType_t osMutexLockFun(osMutexId mutex_id)
{
	if(mutex_id == NULL)
	{
		//printf("1#############################################################\n");
		return TRUE;
	}
	return xSemaphoreTake(( mutex_id ), ( osWaitForever ));

}

BaseType_t osMutexLock_1000msFun(osMutexId mutex_id)
{
	if(mutex_id == NULL)
	{
		//printf("2#############################################################\n");
		return TRUE;
	}
	return xSemaphoreTake(( mutex_id ), ( 1000 ));

}


BaseType_t osMutexLock_1msFun(osMutexId mutex_id)
{
	if(mutex_id == NULL)
	{
		//printf("3#############################################################\n");
		return TRUE;
	}
	return xSemaphoreTake(( mutex_id ), ( 1 ));
}


BaseType_t osMutexUnlockFun(osMutexId mutex_id)
{
	if(mutex_id == NULL)
	{
		//printf("4#############################################################\n");
		return TRUE;
	}
	return xSemaphoreGive( mutex_id );
}

/**
* @brief Allocate a memory block from a memory pool
* @param  osWantedSize Allocate memory word(1word=4byte) size
* @retval  address of the allocated memory block or NULL in case of no memory available.
*/
void *osPortMalloc(uint32_t osWantedSize)
{
	void *ospvReturn = NULL;
	vPortEnterCritical();
	ospvReturn=pvPortMalloc(osWantedSize);
	vPortExitCritical();
	//DBG("Malloc11:%d->%d, Add=%x\n", (int)osWantedSize, (int)xPortGetFreeHeapSize(), ospvReturn);
	return ospvReturn;
}

#ifndef CFG_RTOS_HEAP5_DCORE_MV
	void *pvPortMallocFromEnd( size_t xWantedSize );

	/**
	* @brief Allocate a memory block from a memory pool
	* @param  osWantedSize Allocate memory word(1word=4byte) size
	* @retval  address of the allocated memory block or NULL in case of no memory available.
	*/
	void *osPortMallocFromEnd(uint32_t osWantedSize)
	{
		void *ospvReturn = NULL;

		//DBG("\nMalloc22:%d, %d\n", xPortGetFreeHeapSize(), osWantedSize);
		ospvReturn = pvPortMallocFromEnd(osWantedSize);
		//DBG("Malloc33:%d->%d, Add=%x\n", (int)osWantedSize, (int)xPortGetFreeHeapSize(), ospvReturn);
		return ospvReturn;
	}
#endif

int osPortRemainMem(void)
{
	return (int)xPortGetFreeHeapSize();
}

/**
* @brief Free a memory block
* @param address of the allocated memory block.
*/
void osPortFree(void *ospv)
{
	//DBG("Add=%x\n", ospv);
	vPortEnterCritical();
	vPortFree(ospv);
	vPortExitCritical();
}

void osTaskDelay(uint32_t Cnt)
{
	vTaskDelay(pdMS_TO_TICKS(Cnt));
}

//PRIVILEGED_DATA static volatile UBaseType_t uxCurrentNumberOfTasks 	= ( UBaseType_t ) 0U;

uint32_t SystemMips;
#ifdef DUAL_CORE_EN
	uint32_t Core2Mips;
#endif
__attribute__((section(".ilm_rtos_section")))
void vApplicationIdleHook(void)
{
#ifdef CFG_GOTO_SLEEP_USE
	//#include <nds32_intrinsic.h>
		if(__nds32__mfsr(NDS32_SR_INT_PEND2))
		{
			__nds32__mtsr(__nds32__mfsr(NDS32_SR_INT_PEND2), NDS32_SR_INT_PEND2);
		}
		__nds32__standby_no_wake_grant();
		__nds32__isb();
#endif
}


#ifdef use_MCPS_ANALYSIS

static uint32_t AllCycle = 0;
static uint32_t SwitchInCycle = 0;
static uint32_t SwitchOutCycle = 0;
static uint32_t IdleTaskCycle = 0;
static uint32_t IdleOutCycle = 0;

#ifdef DUAL_CORE_EN
static uint32_t Core2Cycle;
static uint32_t Core2InCycle;
void trace_dcore_in(void)
{
	Core2InCycle = __nds32__mfsr(NDS32_SR_PFMC0);
}
void trace_dcore_out(void)
{
	Core2Cycle += (__nds32__mfsr(NDS32_SR_PFMC0) - Core2InCycle);
}
#endif

void trace_TASK_SWITCHED_IN(void)
{
	SwitchInCycle = __nds32__mfsr(NDS32_SR_PFMC0);
	AllCycle += SwitchInCycle - SwitchOutCycle;
#ifdef TASK_TGL
	DbgTaskTGL_in();
#endif
}


void trace_TASK_SWITCHED_OUT(void)
{
	uint32_t CycleNum;
// 应用层启动
//	__nds32__mtsr(0, NDS32_SR_PFM_CTL);
//	temp = __nds32__mfsr(NDS32_SR_PFMC0);
	SwitchOutCycle = __nds32__mfsr(NDS32_SR_PFMC0);
	CycleNum = SwitchOutCycle - SwitchInCycle;

	if(memcmp(pcTaskGetTaskName(NULL), "IDLE", 4) == 0)
	{
		IdleTaskCycle += CycleNum;
		IdleOutCycle = SwitchOutCycle;
	}
	AllCycle += CycleNum;
	if(AllCycle >= Clock_CoreClockFreqGet() * 2)
	{
#ifdef DUAL_CORE_EN
		Core2Mips = (float)Core2Cycle  * (Clock_CoreClockFreqGet() / 1000000) / AllCycle;
		Core2Cycle = 0;
#endif
		SystemMips = (unsigned)(((float)AllCycle - IdleTaskCycle) * (Clock_CoreClockFreqGet() / 1000000) / AllCycle) ;
		IdleTaskCycle = 0;
		AllCycle=0;
	}

#ifdef TASK_TGL
	DbgTaskTGL_out();
#endif
}
#endif//#ifdef use_MCPS_ANALYSIS
