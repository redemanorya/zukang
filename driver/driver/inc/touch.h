
/**
 *******************************************************************************
 * @file    touch.h
 * @brief	Touch driver interface
 *
 * @author  Eddie
 * @version V1.0.0
 *
 * $Created: 2026-03-17 10:46:11$
 *
 * @Copyright (C) 2026, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *******************************************************************************
 */


/**
 * @addtogroup TOUCH
 * @{
 * @defgroup touch touch.h
 * @{
 */

#ifndef __TOUCH_H__
#define __TOUCH_H__

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

#include "type.h"

typedef enum __TOUCH_Channel
{
    TOUCH_0,        /**GPIOB17*/
    TOUCH_1,        /**GPIOB16*/
    TOUCH_2,        /**GPIOB15*/
    TOUCH_3,        /**GPIOB14*/
    TOUCH_4,        /**GPIOB13*/
    TOUCH_5,        /**GPIOB12*/
    TOUCH_6,        /**GPIOB11*/
    TOUCH_7,        /**GPIOB10*/
    TOUCH_8,        /**GPIOB9*/
    TOUCH_9,        /**GPIOB8*/
    TOUCH_10,       /**GPIOB7*/
    TOUCH_11,       /**GPIOB6*/
	TOUCH_12,       /**GPIOB5*/
	TOUCH_13,       /**GPIOB4*/
	TOUCH_14,       /**GPIOB3*/
	TOUCH_15,       /**GPIOB2*/
}TOUCH_Channel;


typedef enum __Touch_ErrorState
{
	TOUCH_OK=0,
	TOUCH_TIMEOUT,
}Touch_ErrorState;

typedef enum __Touch_IntType
{
	TOUCH_TrgInt = 0x2,      /**indicate previous trigger has been done*/
	TOUCH_EventInt = 0x8,    /**indicate a touch event has occurred*/
    TOUCH_RecyclInt = 0x20,   /**indicate the current cycle of touch channel has been completed, and it's going to recycle*/
    TOUCH_SmplInt = 0x80,     /**indicate a SARADC dout value has been sampled by touch digital*/
}Touch_IntType;

typedef enum __Touch_TrgMode
{
	TOUCH_TrgDisable,           /**disable, touch_trg_ctrl invalid*/
	TOUCH_TrgHardware,          /**enable only hardware trigger source*/
	TOUCH_TrgSoftware,          /** enable only software trigger source*/
	TOUCH_TrgHardwareSoftware,  /**enable both of hardware and software trigger sources*/
}Touch_TrgMode;

/**select a timer as hardware trigger source*/
typedef enum __Touch_TrgTimerSel
{
	TOUCH_TrgTimer9,
	TOUCH_TrgTimer6,
	TOUCH_TrgTimer7,
	TOUCH_TrgTimer8,
}Touch_TrgTimerSel;

typedef struct __Touch_Info
{
	unsigned long 	DOUT_DIFF         :  12; 	/** difference of "doutg - dout" corresponding to the current channel*/
	unsigned long 	RESERVE           :  2;		/** Ignore me*/
	unsigned long 	CHIPID            :  4;		/** indicate which channel has been readed currently*/
	unsigned long 	RESERVE_1         :  14;	/** Ignore me*/
}Touch_Info __ATTRIBUTE__(BITBAND);

/**
 * @brief  设置Touch工作时钟分频，固定与adc clk 为4 ： 1
 * @param  无
 * @return 无
 */
void Touch_ClockDivSet(void);

/**
 Touch channel select
	6'd0: touch0
	6'd1: touch1
	......
	6'd5: touch5
**/
/**
 * @brief  Touch 通道启用
 * @param  ChannalNum   0~0xFFFF
 * @return 无
 */
void Touch_Enable(uint16_t ChannalNum);

/**
 * @brief  Touch 关闭
 * @param  无
 * @return 无
 */
void Touch_Disable(void);

/**
 * @brief  Touch 设置Data读值的通道
 * @param  ChannalNum
 * @return 无
 */
void Touch_ChannelReadSel(TOUCH_Channel ChannalNum);

/**
 * @brief  Touch data参考值设定，默认Max 0xe65 Min 0xccc
 * @param  Max   0~0xFFF DOUTG上限值
 * @param  Min   0~0xFFF DOUTG下限值
 * @return 无
 */
void Touch_DataRefSet(uint16_t Max, uint16_t Min);

/**
 * @brief  设置DiffValue和Event触摸事件触发的阈值
 * @param  无
 * @return 无
 */
void Touch_DiffValueSensSet(uint16_t Sens);

/**
 * @brief  Touch      硬件校准
 * @param Touch_Sel   通道选择
 * @param Timeout     超时时间设置
 * @return state 详见 # Touch_ErrorState
 */
Touch_ErrorState Touch_HardWareCalibration(uint32_t Timeout);

/**
 * @brief  Touch 软件校准
 * @param Touch_Sel         通道选择
 * @param Pulse   0~0xF     脉冲时间
 * @param Current 0~0x3F    电流值
 * @param Timeout           超时时间设置
 * @return state 详见 # Touch_ErrorState
 */
Touch_ErrorState Touch_SoftWareCalibration(TOUCH_Channel Touch_Sel, uint8_t Pulse, uint8_t Current, uint32_t Timeout);

/**
 * @brief  获取Touch data的参考值
 * @param  无
 * @return touch 基准值
 * @note Touch_ChannelReadSel 配合使用
 */
uint16_t Touch_DataRefGet(void);

/**
 * @brief  获取通道的Pulse
 * @param  无
 * @return 脉冲时间
 * @note Touch_ChannelReadSel 配合使用
 */
uint8_t Touch_PulseGet(void);

/**
 * @brief  获取选择通道的current
 * @param  无
 * @return 电流值
 * @note Touch_ChannelReadSel 配合使用
 */
uint16_t Touch_CurrentGet(void);

/**
 * @brief  获取TOUCH采样数据
 * @param  无
 * @return ADC采样量化后数据[0-4095]
 * @note TOUCH_Channel,Touch_Calibration 2个函数配合使用
 */
uint16_t Touch_DataGet(void);

/**
 * @brief  触发的条件是sens > dataref- data
 * @param TouchInfo  通道的差值和通道号
 * @param Timeout    超时时间设置
 * @return state 详见 # Touch_ErrorState
 * @note TOUCH_Channel,Touch_Calibration/Touch_SoftWareCalibration Touch_DiffValueSensSet 3个函数配合使用
 */
Touch_ErrorState Touch_DiffValueGet(Touch_Info *TouchInfo,uint32_t Timeout);

/**
 * @brief   Touch 当前触摸事件中同时发生的触摸事件数
 * @param   无
 * @return  触摸事件数
 */
uint8_t Touch_EventSimulNumGet(void);

/**
 * @brief   Touch 当前触摸事件中 dataRef- data 差值最大的触摸通道号
 * @param   无
 * @return  通道号
 */
uint8_t Touch_EventDiffMaxChannelGet(void);

/**
 * @brief   Touch 当sens > diffref-data 触摸事件触发标志位
 * @param   无
 * @return  触发标志位
 */
bool Touch_EventFlagGet(void);

/**
 * @brief   Touch 一次采样结束的标志位
 * @param   无
 * @return  采样完成标志位
 */
bool Touch_RecyclDoneFlagGet(void);

/**
 * @brief   清除状态标志位
 * @param   无
 * @return  无
 * @note Touch_DataGet/Touch_DiffValueGet 后使用此函数
 */
void Touch_DtatFlagClear(void);

/**
 * @brief   打开中断源
 * @param   IntType 中断源选择
 * @return  无
 */
void Touch_InterrputEnable(Touch_IntType IntType);

/**
 * @brief   判断touch中断
 * @param   无
 * @return  touch中断返回0，gpio中断返回1
 */
uint8_t Touch_CoreInterruptStateGet(void);

/**
 * @brief   清除中断源标志位
 * @param   IntType 中断源选择，可以选择多个
 * @return  无
 */
void Touch_InterruptFlagClear(Touch_IntType IntType);

/**
 * @brief   关闭中断源
 * @param   IntType 中断源选择
 * @return  无
 */
void Touch_InterrputDisable(Touch_IntType IntType);

/**
 * @brief   使能DMA
 * @param   无
 * @return  无
 */
void Touch_DMAEnable(void);

/**
 * @brief   关闭DMA
 * @param   无
 * @return  无
 */
void Touch_DMADiable(void);

/**
 * @brief   Touch 触发源配置
 * @param   TrgMode    触发源选择
 * @param   TrgNum     触摸通道采样周期数
 * @return  无
 */
void Touch_TrgConfig(Touch_TrgMode TrgMode, uint8_t TrgNum);

/**
 * @brief   Touch 选择作用于硬件触发源的定时器
 * @param   Timer    定时器选择
 * @return  无
 */
void Touch_TrgHardwareTimerSel(Touch_TrgTimerSel Timer);

/**
 * @brief   Touch 开始软件触发模式的触摸采样
 * @param   无
 * @return  无
 */
void Touch_TrgSoftwareStart(void);

/**
 * @brief   Touch 触发源完成的标志位
 * @param   无
 * @return  标志位
 */
bool Touch_TrgDoneFlagGet(void);

/**
 * @brief   Touch 校准
 * @param   无
 * @return  无
 */
void Touch_Calibration(void);
#ifdef __cplusplus
}
#endif // __cplusplus
#endif
/**
 * @}
 * @}
 */
