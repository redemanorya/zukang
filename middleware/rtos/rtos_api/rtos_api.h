/*
 * RTOS_API.c
 *
 *  Created on: Aug 30, 2016
 *      Author: peter
 */
 
#ifndef __RTOS_API_H__
#define __RTOS_API_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#include "type.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "semphr.h"
#include "list.h"
//#include "locks.h"

#define MessageHandle		QueueHandle_t

#define MSG_INVAILD			0x0000

typedef struct _MessageContext
{
	uint16_t		msgId;
}MessageContext;


/**
* @brief	Register a message queue
* @param	maxMsgQueueNumber
*	The max number of message that the queue can receive.
* @retval	MessageHandle
*/
MessageHandle MessageRegister(uint16_t maxMsgQueueNumber);
#define MessageRegister(num) 			xQueueCreate((num), sizeof(MessageContext))

/**
* @brief	Deregister a message queue
* @param	msgHandle
* @retval	NONE
*/
void MessageDeregister(MessageHandle msgHandle);
#define MessageDeregister(handle)		vQueueDelete((handle))

/**
* @brief	Clear all of message in the queue
* @param	msgHandle
*	The message queue which to be cleared
* @retval	NONE
*/
void MessageClear(MessageHandle msgHandle);
#define MessageClear(handle)			xQueueReset((handle))


/**
* @brief	Send Message
* @param	msgHandle The message queue which will receive this message
* @param	msgContext The message to send
* @retval	TRUE or FALSE
*/
bool MessageSend(MessageHandle msgHandle,  MessageContext * msgContext);
bool MessageSendFromISR(MessageHandle msgHandle,  MessageContext * msgContext);


/**
* @brief
*	Send Message extension function. 
*	It will not return until the message has been sent successful.
*	This function CAN NOT be used in interrupt functions
* @param 	msgHandle The message queue which will receive this message
* @param	msgContext The message to send
* @retval	NONE
*/
void MessageSendx(MessageHandle msgHandle,  MessageContext * msgContext);

/**
* @brief	Get a Message or Wait for a Message from a Queue.
* @param 	msgHandle The message queue handle
* @param 	millisec Timeout value or 0 in case of no time-out.
* @param	msgContext The message to send
* @retval	TRUE means received message and FALSE means timeout
*/
bool MessageRecv(MessageHandle msgHandle, MessageContext * msgContext, uint32_t millisec);

/**
* @brief	Init heap.
* @param 	NONE
* @retval	NONE
*/
void prvInitialiseHeap(void);

void osSemaphoreMutexCreate(void);
void osSemaphoreMutexUnlock(void);
#ifndef CFG_RTOS_HEAP5_DCORE_MV
void *osPortMallocFromEnd(uint32_t osWantedSize);
#endif
void *osPortMalloc(uint32_t osWantedSize);
void osPortFree( void *ospv );
void osTaskDelay(uint32_t Cnt);
int osPortRemainMem(void);
void trace_dcore_in(void);
void trace_dcore_out(void);

#define osWaitForever     			0xFFFFFFFF     ///< wait forever timeout value
#define osMutexCreate()				xSemaphoreCreateMutex()
#define osMutexLock( mutex_id )		xSemaphoreTake(( mutex_id ), ( osWaitForever ))
#define osMutexLock_1000ms( mutex_id ) osMutexLock_1000msFun(mutex_id)		//xSemaphoreTake(( mutex_id ), ( 1000 ))
#define osMutexLock_1ms( mutex_id )		osMutexLock_1msFun(mutex_id)//xSemaphoreTake(( mutex_id ), ( 1 ))
#define osMutexUnlock( mutex_id )	xSemaphoreGive( mutex_id )

typedef SemaphoreHandle_t osMutexId;

//
extern osMutexId SDIOMutex;//defined @device_service
extern osMutexId SDIOSendCommandMutex;
extern osMutexId UDiskMutex;
extern osMutexId MuteReadBlock;
extern osMutexId MuteWriteBlock;
extern osMutexId UARTMutex[3];
extern osMutexId FlashMutex;
//extern osMutexId AudioEffectMutex;
extern osMutexId SbcDecoderMutex;
extern osMutexId SysModeMutex;
extern osMutexId RemindMutex;
extern osMutexId DCoreIPCMutex;
extern osMutexId SarADCMutex;
extern osMutexId DBGMutex;

extern SemaphoreHandle_t gIPCAppSem;
extern SemaphoreHandle_t gIPCAppDSPSem;
void DecoderOslockUnlock(void);

extern uint32_t SystemMips;
extern uint32_t Core2Mips;

/**********************
 * Is Interrupt service process
 * 0:非中断; 1：中断;
 ***********************/
uint32_t GetIPSR( void );

/**
 * OS时间片出让延时函数
 * ceil: 0 < vTaskDelay(1) <= 1
 */
void vTaskDelay( const TickType_t xTicksToDelay ) PRIVILEGED_FUNCTION;

/**
* @brief   Read instantaneous MCPS situation, instantaneous MCPS will be cleared to 0 after read
* @param   NONE
* @retval  (uS) Max duration of full load
*/
uint32_t InstantMcpsFull(void);

extern BaseType_t osMutexLock_1msFun(osMutexId mutex_id);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif /*__RTOS_API_H__*/
