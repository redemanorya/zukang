#include "rtos_api.h"

#ifndef __SAFE_MODE_H__
#define __SAFE_MODE_H__

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus 


bool SafeModeInit(void);

bool SafeModeDeinit(void);

void SafeModeRun(uint16_t msgId);


#ifdef __cplusplus
}
#endif // __cplusplus 

#endif // __SAFE_MODE_H__



