/**
 *******************************************************************************
 * @file    powercontroller.h
 * @brief	powercontroller module driver interface

 * @author  Sean
 * @version V1.0.0

 * $Created: 2017-11-13 16:51:05$
 * @Copyright (C) 2017, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *******************************************************************************
 */

/**
 * @addtogroup POWERCONTROLLER
 * @{
 * @defgroup powercontroller powercontroller.h
 * @{
 */
 
#ifndef __POWERCONTROLLER_H__
#define __POWERCONTROLLER_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#include "type.h"

typedef enum __PWR_SYSWAKEUP_SOURCE_SEL
{
    SYSWAKEUP_SOURCE_NONE = 0,
    SYSWAKEUP_SOURCE0_GPIO = (1 << 0),         /**<system wakeup source0 macro,can be waked up by both edge*/
    SYSWAKEUP_SOURCE1_GPIO = (1 << 1),         /**<system wakeup source1 macro,can be waked up by both edge*/
    SYSWAKEUP_SOURCE2_GPIO = (1 << 2),         /**<system wakeup source2 macro,can be waked up by both edge*/
    SYSWAKEUP_SOURCE3_GPIO = (1 << 3),         /**<system wakeup source3 macro,can be waked up by both edge*/
    SYSWAKEUP_SOURCE4_GPIO = (1 << 4),         /**<system wakeup source4 macro,can be waked up by both edge*/
    SYSWAKEUP_SOURCE5_GPIO = (1 << 5),         /**<system wakeup source5 macro,can be waked up by both edge*/
    SYSWAKEUP_SOURCE6_POWERKEY = (1 << 6),     /**<system wakeup source6 macro,can be waked up by both edge*/
    SYSWAKEUP_SOURCE7_LVD = (1 << 7),          /**<system wakeup source7 macro,should be waked up by rise edge*/
    SYSWAKEUP_SOURCE8_UART0_RX = (1 << 8),     /**<system wakeup source8 macro,should be waked up by rise edge*/
    SYSWAKEUP_SOURCE9_RTC = (1 << 9),          /**<system wakeup source9 macro,should be waked up by rise edge*/
    SYSWAKEUP_SOURCE10_IR = (1 << 10),         /**<system wakeup source10 macro,should be waked up by rise edge*/
    SYSWAKEUP_SOURCE11_TIMER5_INT = (1 << 11), /**<system wakeup source11 macro,should be waked up by rise edge*/
    SYSWAKEUP_SOURCE12_UART1_RX = (1 << 12),   /**<system wakeup source12 macro,should be waked up by rise edge*/
    SYSWAKEUP_SOURCE13_CAN0 = (1 << 13),       /**<system wakeup source13 macro,should be waked up by rise edge*/
    SYSWAKEUP_SOURCE14_UART2_RX = (1 << 14),   /**<system wakeup source14 macro,should be waked up by rise edge*/
    SYSWAKEUP_SOURCE15_CAN1 = (1 << 15),       /**<system wakeup source14 macro,should be waked up by rise edge*/
    SYSWAKEUP_SOURCE16_BT = (1 << 16)          /**<system wakeup source16 macro,should be waked up by rise edge*/

} PWR_SYSWAKEUP_SOURCE_SEL;

// 定义枚举定义唤醒的GPIO枚举 GPIOA0~GPIOA31, GPIOB0~GPIOB27, GPIOC0~GPIOC2
typedef enum __PWR_WAKEUP_GPIO_SEL
{
    WAKEUP_GPIOA0 = 0, /**<wakeup by GPIOA0 macro*/
    WAKEUP_GPIOA1,     /**<wakeup by GPIOA1 macro*/
    WAKEUP_GPIOA2,     /**<wakeup by GPIOA2 macro*/
    WAKEUP_GPIOA3,     /**<wakeup by GPIOA3 macro*/
    WAKEUP_GPIOA4,     /**<wakeup by GPIOA4 macro*/
    WAKEUP_GPIOA5,     /**<wakeup by GPIOA5 macro*/
    WAKEUP_GPIOA6,     /**<wakeup by GPIOA6 macro*/
    WAKEUP_GPIOA7,     /**<wakeup by GPIOA7 macro*/
    WAKEUP_GPIOA8,     /**<wakeup by GPIOA8 macro*/
    WAKEUP_GPIOA9,     /**<wakeup by GPIOA9 macro*/
    WAKEUP_GPIOA10,    /**<wakeup by GPIOA10 macro*/
    WAKEUP_GPIOA11,    /**<wakeup by GPIOA11 macro*/
    WAKEUP_GPIOA12,    /**<wakeup by GPIOA12 macro*/
    WAKEUP_GPIOA13,    /**<wakeup by GPIOA13 macro*/
    WAKEUP_GPIOA14,    /**<wakeup by GPIOA14 macro*/
    WAKEUP_GPIOA15,    /**<wakeup by GPIOA15 macro*/
    WAKEUP_GPIOA16,    /**<wakeup by GPIOA16 macro*/
    WAKEUP_GPIOA17,    /**<wakeup by GPIOA17 macro*/
    WAKEUP_GPIOA18,    /**<wakeup by GPIOA18 macro*/
    WAKEUP_GPIOA19,    /**<wakeup by GPIOA19 macro*/
    WAKEUP_GPIOA20,    /**<wakeup by GPIOA20 macro*/
    WAKEUP_GPIOA21,    /**<wakeup by GPIOA21 macro*/
    WAKEUP_GPIOA22,    /**<wakeup by GPIOA22 macro*/
    WAKEUP_GPIOA23,    /**<wakeup by GPIOA23 macro*/
    WAKEUP_GPIOA24,    /**<wakeup by GPIOA24 macro*/
    WAKEUP_GPIOA25,    /**<wakeup by GPIOA25 macro*/
    WAKEUP_GPIOA26,    /**<wakeup by GPIOA26 macro*/
    WAKEUP_GPIOA27,    /**<wakeup by GPIOA27 macro*/
    WAKEUP_GPIOA28,    /**<wakeup by GPIOA28 macro*/
    WAKEUP_GPIOA29,    /**<wakeup by GPIOA29 macro*/
    WAKEUP_GPIOA30,    /**<wakeup by GPIOA30 macro*/
    WAKEUP_GPIOA31,    /**<wakeup by GPIOA31 macro*/
    WAKEUP_GPIOB0,     /**<wakeup by GPIOB0 macro*/
    WAKEUP_GPIOB1,     /**<wakeup by GPIOB1 macro*/
    WAKEUP_GPIOB2,     /**<wakeup by GPIOB2 macro*/
    WAKEUP_GPIOB3,     /**<wakeup by GPIOB3 macro*/
    WAKEUP_GPIOB4,     /**<wakeup by GPIOB4 macro*/
    WAKEUP_GPIOB5,     /**<wakeup by GPIOB5 macro*/
    WAKEUP_GPIOB6,     /**<wakeup by GPIOB6 macro*/
    WAKEUP_GPIOB7,     /**<wakeup by GPIOB7 macro*/
    WAKEUP_GPIOB8,     /**<wakeup by GPIOB8 macro*/
    WAKEUP_GPIOB9,     /**<wakeup by GPIOB9 macro*/
    WAKEUP_GPIOB10,    /**<wakeup by GPIOB10 macro*/
    WAKEUP_GPIOB11,    /**<wakeup by GPIOB11 macro*/
    WAKEUP_GPIOB12,    /**<wakeup by GPIOB12 macro*/
    WAKEUP_GPIOB13,    /**<wakeup by GPIOB13 macro*/
    WAKEUP_GPIOB14,    /**<wakeup by GPIOB14 macro*/
    WAKEUP_GPIOB15,    /**<wakeup by GPIOB15 macro*/
    WAKEUP_GPIOB16,    /**<wakeup by GPIOB16 macro*/
    WAKEUP_GPIOB17,    /**<wakeup by GPIOB17 macro*/
    WAKEUP_GPIOB18,    /**<wakeup by GPIOB18 macro*/
    WAKEUP_GPIOB19,    /**<wakeup by GPIOB19 macro*/
    WAKEUP_GPIOB20,    /**<wakeup by GPIOB20 macro*/
    WAKEUP_GPIOB21,    /**<wakeup by GPIOB21 macro*/
    WAKEUP_GPIOB22,    /**<wakeup by GPIOB22 macro*/
    WAKEUP_GPIOB23,    /**<wakeup by GPIOB23 macro*/
    WAKEUP_GPIOB24,    /**<wakeup by GPIOB24 macro*/
    WAKEUP_GPIOB25,    /**<wakeup by GPIOB25 macro*/
    WAKEUP_GPIOB26,    /**<wakeup by GPIOB26 macro*/
    WAKEUP_GPIOB27,    /**<wakeup by GPIOB27 macro*/
    WAKEUP_GPIOC0,     /**<wakeup by GPIOC0 macro*/
    WAKEUP_GPIOC1,     /**<wakeup by GPIOC1 macro*/
    WAKEUP_GPIOC2      /**<wakeup by GPIOC2 macro*/
} PWR_WAKEUP_GPIO_SEL;

typedef enum __PWR_SYSWAKEUP_SOURCE_EDGE_SEL
{
    SYSWAKEUP_SOURCE_NEGE_TRIG = 0,  /**negedge trigger*/
    SYSWAKEUP_SOURCE_POSE_TRIG,      /**posedge trigger*/
    SYSWAKEUP_SOURCE_BOTH_EDGES_TRIG /**both edges trigger*/
                                        /**只在SourSel为[0~6]才有上升沿或者下降沿触发之分，其他SourSel都是默认上升沿触发*/

} PWR_SYSWAKEUP_SOURCE_EDGE_SEL;

typedef enum __PWR_LVD_Threshold_SEL
{
    PWR_LVD_Threshold_2V3 = 0, /**LVD threshold select:2.3V */
    PWR_LVD_Threshold_2V4,     /**LVD threshold select:2.4V*/
    PWR_LVD_Threshold_2V5,     /**LVD threshold select:2.5V*/
    PWR_LVD_Threshold_2V6,     /**LVD threshold select:2.6V*/
    PWR_LVD_Threshold_2V7,     /**LVD threshold select:2.7V*/
    PWR_LVD_Threshold_2V8,     /**LVD threshold select:2.8V*/
    PWR_LVD_Threshold_2V9,     /**LVD threshold select:2.9V*/
    PWR_LVD_Threshold_3V0      /**LVD threshold select:3.0V*/
} PWR_LVD_Threshold_SEL;

typedef enum _PWR_LDO11_LVL_SEL
{
    PWD_LDO11_LVL_0V85 = 0, // 0.85V
    PWD_LDO11_LVL_0V90,     // 0.90V
    PWD_LDO11_LVL_0V95,     // 0.95V
    PWD_LDO11_LVL_1V0,      // 1.0V
    PWD_LDO11_LVL_1V03,     // 1.03V
    PWD_LDO11_LVL_1V05,     // 1.05V
    PWD_LDO11_LVL_1V08,     // 1.08V
    PWD_LDO11_LVL_1V10,     // 1.10V
    PWD_LDO11_LVL_1V13,     // 1.13V
    PWD_LDO11_LVL_1V15,     // 1.15V
    PWD_LDO11_LVL_1V18,     // 1.18V
    PWD_LDO11_LVL_1V20,     // 1.20V
    PWD_LDO11_LVL_1V22,     // 1.22V
    PWD_LDO11_LVL_1V25,     // 1.25V
    PWD_LDO11_LVL_MAX       // 请不要用这个值配置！！！仅用来做有效性判断，实际配置请用PWD_LDO11_LVL_0V95~PWD_LDO11_LVL_1V25
} PWR_LDO11_LVL_SEL;

typedef enum _PWR_LDO33_LVL_SEL
{
    PWD_LDO33_LVL_2V7 = 0, // 2.7V
    PWD_LDO33_LVL_2V8,     // 2.8V
    PWD_LDO33_LVL_2V9,     // 2.9V
    PWD_LDO33_LVL_3V0,     // 3.0V
    PWD_LDO33_LVL_3V1,     // 3.1V
    PWD_LDO33_LVL_3V2,     // 3.2V
    PWD_LDO33_LVL_3V3,     // 3.3V
    PWD_LDO33_LVL_MAX      // 请不要用这个值配置！！！仅用来做有效性判断，实际配置请用PWD_LDO33_LVL_2V7~PWD_LDO33_LVL_3V3
} PWR_LDO33_LVL_SEL;

typedef enum _PWR_LDO16_LVL_SEL
{
    PWD_LDO16_LVL_1V6 = 0, // 1.6V
    PWD_LDO16_LVL_1V7,     // 1.7V
    PWD_LDO16_LVL_1V8,     // 1.8V
    PWD_LDO16_LVL_MAX      // 请不要用这个值配置！！！仅用来做有效性判断，实际配置请用PWD_LDO16_LVL_1V6~PWD_LDO16_LVL_1V8
} PWR_LDO16_LVL_SEL;

typedef enum _PWR_DCDC11_LVL_SEL
{
    PWD_DCDC11_LVL_0V9 = 0, // 0.9V
    PWD_DCDC11_LVL_0V95,     // 0.95V
    PWD_DCDC11_LVL_1V0,     // 1.0V
    PWD_DCDC11_LVL_1V03,     // 1.03V
    PWD_DCDC11_LVL_1V06,     // 1.06V
    PWD_DCDC11_LVL_1V1,     // 1.1V
    PWD_DCDC11_LVL_1V13,     // 1.13V
    PWD_DCDC11_LVL_1V16,     // 1.16V
    PWD_DCDC11_LVL_1V2,     // 1.2V
    PWD_DCDC11_LVL_1V23,     // 1.23V
    PWD_DCDC11_LVL_1V26,     // 1.26V
    PWD_DCDC11_LVL_1V3,     // 1.3V
    PWD_DCDC11_LVL_MAX      // 请不要用这个值配置！！！仅用来做有效性判断，实际配置请用PWD_DCDC11_LVL_0V9~PWD_DCDC11_LVL_1V35
} PWR_DCDC11_LVL_SEL;

typedef enum _PWR_DCDC16_LVL_SEL
{
    PWD_DCDC16_LVL_1V3,  // 1.3V
    PWD_DCDC16_LVL_1V35,  // 1.35V
    PWD_DCDC16_LVL_1V4,  // 1.4V
    PWD_DCDC16_LVL_1V45, // 1.45V
    PWD_DCDC16_LVL_1V5,  // 1.5V
    PWD_DCDC16_LVL_1V55, // 1.55V
    PWD_DCDC16_LVL_1V6,  // 1.6V
    PWD_DCDC16_LVL_1V65, // 1.65V
    PWD_DCDC16_LVL_1V7,  // 1.7V
    PWD_DCDC16_LVL_1V75, // 1.75V
    PWD_DCDC16_LVL_1V8,  // 1.8V
    PWD_DCDC16_LVL_1V85, // 1.85V
    PWD_DCDC16_LVL_1V9,  // 1.9V
    PWD_DCDC16_LVL_MAX   // 请不要用这个值配置！！！仅用来做有效性判断，实际配置请用PWD_DCDC16_LVL_1V35~PWD_DCDC16_LVL_1V95
} PWER_DCDC16_LVL_SEL;

typedef enum _PWR_DCDC_FREQ_SEL
{
    PWR_DCDC_FREQ_1M = 0,   //1M
    PWR_DCDC_FREQ_2M2,      //2.2M
} PWR_DCDC_FREQ_SEL;

/**
* @brief  DCDC11配置频率
* @param  freq 频率选择：1M或者2.2M
* @return void
* @note   none
*/
void Power_DCDC11ConfigFreq(PWR_DCDC_FREQ_SEL freq);

/**
* @brief  DCDC16配置频率
* @param  freq 频率选择：1M或者2.2M
* @return void
* @note   none
*/
void Power_DCDC16ConfigFreq(PWR_DCDC_FREQ_SEL freq);

/**
 * @brief  Config LDO11D voltage
 * @param  PWR_LDO11_LVL_SEL level
 * @return void
 * @note   none
 */
void Power_LDO11DConfig(PWR_LDO11_LVL_SEL level);

/**
 * @brief  获取LDO11D配置的电压值
 * @param  void
 * @return uint16_t 电压值，单位：mV
 * @note   none
 */
uint16_t Power_GetLDO11DConfigVoltage(void);

/**
 * @brief  config power LDO33D voltage
 * @param  PWR_LDO33_LVL_SEL level
 * @return void
 * @note   芯片个体之间会有稍许差异和偏差，偏差范围在0mV~50mV之间；低于3.0V的挡位误差比较大，建议使用3.0V以上挡位。
 */
void Power_LDO33DConfig(PWR_LDO33_LVL_SEL level);

/**
 * @brief  config power LDO33A voltage
 * @param  PWR_LDO33_LVL_SEL level
 * @return void
 * @note   芯片个体之间会有稍许差异和偏差，偏差范围在0mV~50mV之间；低于3.0V的挡位误差比较大，建议使用3.0V以上挡位。
 */
void Power_LDO33AConfig(PWR_LDO33_LVL_SEL level);

/**
 * @brief  获取LDO33D配置的电压值
 * @param  void
 * @return uint16_t 电压值，单位：mV
 * @note   none
 */
uint16_t Power_GetLDO33DConfigVoltage(void);

/**
 * @brief  Power config LDO16 voltage
 * @param  PWR_LDO16_LVL_SEL level
 * @return void
 * @note   none
 */
void Power_LDO16Config(PWR_LDO16_LVL_SEL level);

/**
 * @brief  获取LDO16配置的电压值
 * @param  void
 * @return uint16_t 电压值，单位：mV
 * @note   none
 */
uint16_t Power_GetLDO16ConfigVoltage(void);

/**
 * @brief  系统进入sleep模式
 * @param  无
 * @return 无
 */
void Power_GotoSleep(void);

/**
 * @brief  系统进入deepsleep模式
 * @param  无
 * @return 无
 */
void Power_GotoDeepSleep(void);

#if 1 // bt deepsleep with external trigger
void Power_BtWakeupEnable(PWR_SYSWAKEUP_SOURCE_SEL SourSel, PWR_SYSWAKEUP_SOURCE_EDGE_SEL Edge);
void Power_BtWakeupDisable(PWR_SYSWAKEUP_SOURCE_SEL SourSel);
uint32_t Power_BtGetPending(void);
void Power_BtClearPending(void);
#endif

/**
 * @brief  配置DeepSleep唤醒源
 * @param  SourSel 配置唤醒源,分别为：GPIO，PowerKey,RTC,LVD(一般不用),IR,BT为蓝牙事件唤醒
 * @param  Gpio   配置gpio唤醒引脚，[0~41],分别对应GPIOA[31:0]、GPIOB[8:0]、GPIOC[0]
 * @param  Edge   gpio的边沿触发方式选择：1上升沿0下降沿
 * @note   参数gpio，    只在SourSel为GPIO才有效
 *         参数edge，    只在SourSel为GPIO和PowerKey才有上升沿或者下降沿触发之分，其他SourSel都是默认上升沿触发
 * @return 无
 */
void Power_WakeupSourceSet(PWR_SYSWAKEUP_SOURCE_SEL SourSel, PWR_WAKEUP_GPIO_SEL Gpio, PWR_SYSWAKEUP_SOURCE_EDGE_SEL Edge);

/**
 * @brief  设置某个通道唤醒使能
 * @param  SourSel 唤醒源:GPIO，PowerKey,RTC,LVD(一般不用),IR,BT为蓝牙事件唤醒
 * @return 无
 */
void Power_WakeupEnable(PWR_SYSWAKEUP_SOURCE_SEL SourSel);

/**
 * @brief  设置某个通道唤醒禁止
 * @param  SourSel 唤醒源:GPIO，PowerKey,RTC,LVD(一般不用),IR,BT为蓝牙事件唤醒
 * @return 无
 */
void Power_WakeupDisable(PWR_SYSWAKEUP_SOURCE_SEL SourSel);

/**
 * @brief  获取唤醒通道标志
 * @param  无
 * @return 获取唤醒通道标志
 */
uint32_t Power_WakeupSourceGet(void);

/**
 * @brief  清除唤醒通道标志
 * @param  无
 * @return 无
 */
void Power_WakeupSourceClear(void);

/**
 * @brief  通过唤醒源标志查询gpio唤醒引脚：[0~41],分别对应GPIOA[31:0]、GPIOB[8:0]、GPIOC[0]
 * @param  SourSel为所获取的唤醒源，分别为：GPIO，PowerKey,RTC,LVD(一般不用),IR,Timer3中断（定时或者PWC），UART0或者UART1
 * @return gpio唤醒引脚
 * @note   必须先获取唤醒源
 *         只在SourSel为GPIO才有效
 */
uint32_t Power_WakeupGpioGet(PWR_SYSWAKEUP_SOURCE_SEL SourSel);

/**
 * @brief  通过唤醒源标志查询gpio唤醒的触发边沿
 * @param  SourSel:
 * @return 唤醒源的触发边沿      0：下降沿； 1：上升沿
 * @note   必须先获取唤醒源
 *         只在SourSel为GPIO和PowerKey才有上升沿或者下降沿触发之分，其他SourSel都是默认上升沿触发
 */
uint8_t Power_WakeupEdgeGet(PWR_SYSWAKEUP_SOURCE_SEL SourSel);

/**
 * @brief  使能LVD低电压检测（VIN电压）
 * @param  无
 * @return 无
 * @note   无
 */
void Power_LVDEnable(void);

/**
 * @brief  禁能LVD低电压检测（VIN电压）
 * @param  无
 * @return 无
 * @note   无
 */
void Power_LVDDisable(void);

/**
 * @brief  使能AVDD33
 * @param  无
 * @return 无
 * @note   无
 */
void Power_AVDD33Enable(void);

/**
 * @brief  禁能AVDD33
 * @param  无
 * @return 无
 * @note   无
 */
void Power_AVDD33Disable(void);

/**
 * @brief  LVD低电压检测的水位选择（VIN电压）
 * @param  Lvd_Threshold_Sel  水位选择档位[2.3~3.0]（默认水位为2.3V）
 * @return 无
 * @note   无
 */
void Power_LVDThresholdSel(PWR_LVD_Threshold_SEL Lvd_Threshold_Sel);

/**
 * @brief  使用DeepSleep功能时配置LVD低电压检测的水位（VIN电压）
 * @param  Lvd_Threshold_Sel  LVD低电压检测的水位选择（默认水位为2.3V）
 * @return 无
 * @note   无
 */
void Power_LVDWakeupConfig(PWR_LVD_Threshold_SEL Lvd_Threshold_Sel);

/**
 * @brief Power_ConfigDCDC11
 * @param  PWR_DCDC11_LVL_SEL lvl
 * @return void
 * @note   none
 */
void Power_ConfigDCDC11(PWR_DCDC11_LVL_SEL lvl);

/**
 * @brief  Power_ConfigDCDC16
 * @param  PWER_DCDC16_LVL_SEL lvl
 * @return void
 * @note   none
 */
void Power_ConfigDCDC16(PWER_DCDC16_LVL_SEL lvl);

/**
 * @brief  LDO16_Switch2DCDC16 切换LDO16到DCDC16,并设置电压1.6V
 * @param  void
 * @return void
 * @note   该API中用到的延时接口依赖于时钟，需要在系统时钟配置后使用，否则延时不准引起整体耗时增加。
 */
void LDO16_Switch2DCDC16(void);

/**
 * @brief  从DCDC16模式切换到LDO16模式，并设置电压1.7V
 * @param  void
 * @return void
 * @note   该API中用到的延时接口依赖于时钟，需要在系统时钟配置后使用，否则延时不准引起整体耗时增加。
 */
void Power_switchDCDC16toLDO16(void);

/**
 * @brief  LDO11_Switch2DCDC11 切换LDO11到DCDC11，并设置电压1.1V
 * @param  void
 * @return void
 * @note   该API中用到的延时接口依赖于时钟，需要在系统时钟配置后使用，否则延时不准引起整体耗时增加。
 */
void LDO11_Switch2DCDC11(void);

/**
 * @brief  从DCDC11模式切换到LDO11模式,并设置电压1.2V
 * @param  void
 * @return void
 * @note   该API中用到的延时接口依赖于时钟，需要在系统时钟配置后使用，否则延时不准引起整体耗时增加。
 */
void Power_switchDCDC11toLDO11(void);

/**
 * @brief  配置内存进入睡眠模式
 * @param  void
 * @return void
 **/
void Power_MemoryEnterSleep(void);

/**
 * @brief  配置内存退出睡眠模式
 * @param  void
 * @return void
 **/
void Power_MemoryExitSleep(void);

/**
 * @brief  使能唤醒中断
 * @param  void
 * @return void
 * @note   none
 */
void Power_EnableWakeupIRQ(void);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif //__POWERCONTROLLER_H__

/**
 * @}
 * @}
 */
