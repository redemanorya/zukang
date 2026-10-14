/**
 *******************************************************************************
 * @file    watchdog.h
 * @brief	watchdog module driver interface

 * @author  Sam
 * @version V1.0.0

 * $Created: 2017-10-31 16:51:05$
 * @Copyright (C) 2017, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *******************************************************************************
 */

/**
 * @addtogroup WATCHDOG
 * @{
 * @defgroup watchdog watchdog.h
 * @{
 */
 
#ifndef __WATCH_DOG_H__
#define __WATCH_DOG_H__
 
#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

/*B6 include WDG0 and WDG1. WDG0 is used for core 1, WDG1 is used for core 2*/

/**
 *  定义看门狗的复位周期
 */
typedef enum _WDG_STEP_SEL
{
    WDG_STEP_1S = 0,/**<看门狗的复位周期:1S*/
    WDG_STEP_3S = 1,/**<看门狗的复位周期:3S*/
    WDG_STEP_4S = 2 /**<看门狗的复位周期:4S*/
} WDG_STEP_SEL;


/**
 * @brief  看门狗0使能
 * @param  Mode 设置看门狗的复位周期
 *     @arg WDG_STEP_1S
 *     @arg WDG_STEP_3S
 *     @arg WDG_STEP_4S
 * @return 无
 */
void WDG0_Enable(WDG_STEP_SEL Mode);


/**
 * @brief  看门狗0禁止
 * @param  无
 * @return 无
 */
void WDG0_Disable(void);


/**
 * @brief  喂狗0
 * @param  无
 * @return 无
 */
void WDG0_Feed(void);

/**
* @brief  获取看门狗0的半周期中断是否被触发
* @param  void
* @return void
* @note   none
*/
bool WDG0_IsHalfTimeIntReported();

/**
 * @brief  看门狗1使能
 * @param  Mode 设置看门狗的复位周期
 *     @arg WDG_STEP_1S
 *     @arg WDG_STEP_3S
 *     @arg WDG_STEP_4S
 * @return 无
 */     
void WDG1_Enable(WDG_STEP_SEL Mode);     

/**             
 * @brief  看门狗1禁止
 * @param  无
 * @return 无
 */
void WDG1_Disable(void);

/**                         
 * @brief  喂狗1
 * @param  无
 * @return 无
 */
void WDG1_Feed(void);

/**
 * @brief  看门狗1使能复位所有核
 * @param  void
 * @return void
 * @note   none
 */
void WDG1_EnableRestAllCore(void);

/**
 * @brief  使能当前核上的看门狗
 * @param  Mode 设置看门狗的复位周期
 *	   @arg WDG_STEP_1S
 *	   @arg WDG_STEP_3S
 *	   @arg WDG_STEP_4S
 * @return 无
 */
void WDG_Enable(WDG_STEP_SEL Mode);

/**
 * @brief  关闭当前核上的看门狗
 * @param  无
 * @return 无
 */
void WDG_Disable(void);

/**
 * @brief  获取当前核的看门狗使能状态
 * @param	无
 * @return Return 0: 看门狗禁止, 1: 看门狗使能
 */
bool WDG_EnableStatusGet(void);

/**
 * @brief  当前核看门狗喂狗
 * @param  无
 * @return 无
 */
void WDG_Feed(void);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif //__WATCH_DOG_H__

/**
 * @}
 * @}
 */
