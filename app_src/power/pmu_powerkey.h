#ifndef _PMU_POWERKEY_H_
#define _PMU_POWERKEY_H_

#include "pmu.h"

#define		POWERKEY_MODE_BYPASS 				0
#define		POWERKEY_MODE_PUSH_BUTTON			1
#define		POWERKEY_MODE_SLIDE_SWITCH_LPD 		2//硬开关高唤醒
#define		POWERKEY_MODE_SLIDE_SWITCH_HPD		3//硬开关低唤醒

void PMU_PowerKey8SResetSet(void);
void SystemPowerDown(void);
void SystemPowerKeyIdleModeInit(void);

#endif
