/**
 **************************************************************************************
 * @file    IO_key.h
 * @brief   IO key 
 *
 * @author  kenbu/bkd
 * @version V0.0.1
 *
 * $Created: 2018-09-03 
 *
 * @Copyright (C) 2018, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */


#ifndef __IO_KEY_H__
#define __IO_KEY_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus
#include "type.h"
#include "key.h"


#define 	IO_CHANNEL_EMPTY               KEY_MSG_INDEX_EMPTY

KeyScanMsg IOKeyScan(void);

void IOKeyInit(void);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif //__IO_KEY_H__

