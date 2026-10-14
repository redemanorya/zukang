/**
 **************************************************************************************
 * @file    Key.h
 * @brief   key 
 *
 * @author  pi
 * @version V1.0.0
 *
 * $Created: 2018-1-11 17:40:00$
 *
 * @Copyright (C) 2018, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#include "app_config.h"
#include "code_key.h"
#include "type.h"
#include "app_message.h"

#ifndef __KEY_H__
#define __KEY_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

typedef struct _DBKeyMsg_1_
{
	uint16_t dbclick_en;
	uint16_t dbclick_timeout;

	uint32_t KeyMsg;
	uint32_t dbclick_msg;
}KEYBOARD_MSG;

typedef enum _KeyMsgType
{
	KEY_UNKOWN_TYPE = 0,
	KEY_PRESSED,
	KEY_RELEASED,
	KEY_LONG_PRESSED,
	KEY_LONG_PRESS_HOLD,
	KEY_LONG_RELEASED,
}KeyMsgType;

typedef enum _KeySourceType
{
	UNKOWN_TYPE_SOURCE = 0,
	ADC_KEY_SOURCE,
	IR_KEY_SOURCE,
	IO_KEY_SOURCE,
	ADC_LEVEL_KEY_SOURCE,
	CODE_KEY_SOURCE,
}KeySourceType;

typedef struct _KeyScanMsg
{
    uint16_t 		index;
    KeyMsgType 		type;
    KeySourceType	source;
}KeyScanMsg;

#define 	KEY_MSG_INDEX_EMPTY             0xFF

void KeyInit(void);
MessageId KeyScan(void);
void BeepEnable(void);
MessageId GetGlobalKeyMessageId(KeyScanMsg 	msg);

#ifdef __cplusplus
}
#endif//__cplusplus


#endif




