/**
 **************************************************************************************
 * @file   wireless2_task.h
 * @brief
 *
 * @author  Tony
 * @version V1.0.0
 *
 * $Created: 2026-04-07
 *
 * @Copyright (C) 2026, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#ifndef __WIRELESS2_TASK_H__
#define __WIRELESS2_TASK_H__
#include "rtos_api.h"
#include "app_message.h"

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


typedef struct _Wireless2ModeContext
{
	xTaskHandle			taskHandle;
	MessageHandle		msgHandle;
}Wireless2ModeContext;

typedef enum
{
	Wire2StateSusend	= 0,
	Wire2StateReady,
	Wire2StateInit,
	Wire2StateRunning,
	Wire2StateDeinit,
}Wireless2ModeState;

#ifdef __cplusplus
}
#endif//__cplusplus

#endif //__WIRELESS2_TASK_H__

