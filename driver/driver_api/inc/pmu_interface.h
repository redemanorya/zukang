/**
 **************************************************************************************
 * @file    pmu_interface.h
 * @brief   pmu interface
 *
 * @author  
 * @version 
 *
 * $Created: 2024-01-18 16:30:04$
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#ifndef		__PMU_INTERFACE_H__
#define		__PMU_INTERFACE_H__

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus 

#include "pmu.h"

typedef enum
{
	E_PWRKEY_MODE_BYPASS = 0,
	E_PWRKEY_MODE_PUSHBUTTON,
	E_PWRKEY_MODE_SLIDESWITCH_HON,
	E_PWRKEY_MODE_SLIDESWITCH_LON
} TE_PWRKEY_MODE;

typedef enum
{
	E_PWRKEYINIT_OK = 0,
	E_PWRKEYINIT_NONE,
	E_PWRKEYINIT_NOTSUPPORT,
	E_PWRKEYINIT_NOT_FIRST_PWRON,
	E_PWRKEYINIT_UNKNOWN_MODE,
	E_PWRKEYINIT_SPTIME_ERR,
	E_PWRKEYINIT_LPTIME_ERR,
	E_PWRKEYINIT_LPRSTTIME_ERR,
	E_PWRKEYINIT_STIME_GE_LPTIME_ERR,
	E_PWRKEYINIT_LTIME_GE_RSTTIME_ERR
} TE_PWRKEYINIT_RET;

typedef enum
{
	E_KEYDETTIME_512MS = 0,
	E_KEYDETTIME_1024MS,
	E_KEYDETTIME_1536MS,
	E_KEYDETTIME_2048MS,
	E_KEYDETTIME_3072MS,
	E_KEYDETTIME_4096MS
} TE_KEYDET_TIME;

/**
 * @brief     清除PowerKey按键状态标志
 *
 * @param     None
 * @return    None
 */
void SystemPowerKeyStateClear(void);

/**
 * @brief     获取当前PowerKey设置模式
 *
 * @param     None
 * @return    TE_PWRKEY_MODE
 */
TE_PWRKEY_MODE SystemPowerKeyGetMode(void);

/**
 * @brief     Powerkey初始化
 *
 * @param[in] eMode	 	Powerkey模式
 * @param     eTime     设置PUSHBUTTON模式时按键检测时长. SLIDESWITCH模式时固定为128ms
 *
 * @return    TE_PWRKEYINIT_RET
 */
TE_PWRKEYINIT_RET SystemPowerKeyInit(TE_PWRKEY_MODE eMode, TE_KEYDET_TIME eTime);

/**
 * @brief     检测PowerKey按键事件
 *
 * @return    TRUE:检测到事件; FLASE:未检测到
 */
bool SystemPowerKeyDetect(void);

/**
 * @brief     GPIOC0/1/2做为Powerup key 设置
 * @param     ePin 按键
 * @param     eType 类型
 * @return    TRUE 设置成功; FALSE:设置失败;
 */
bool SystemGPIOCPowerUpSet(GPIOC_POWERUP_PIN ePin, GPIOC_POWERUP_TYPE eType);

#ifdef __cplusplus
}
#endif // __cplusplus 

#endif //__PMU_INTERFACE_H__

