/**
 **************************************************************************************
 * @file    pwc.h
 * @brief   Pulse Width Capture (Reuse with General Timer 3&4 ) API
 *
 * @author  Grayson Chen
 * @version V1.0.0
 *
 * $Created: 2017-09-29 13:25:30$
 *
 * @copyright Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

/**
 * @addtogroup PWC
 * @{
 * @defgroup pwc pwc.h
 * @{
 */

#ifndef __PWC_H__
#define __PWC_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#include "type.h"
#include "timer.h"

#define PWC_MAX_TIMESCALE   0xffff
    
//PWC 输出通道GPIO复用关系
    
#define PWC_IO_TOTAL		62
    
/**
 * PWM IO Mode select definition
 */ 
typedef enum __PWC_IO_MODE
{
    PWC_IO_MODE_NONE = 0,    //还原为GPIO
    PWC_IO_MODE_IN  = 1     //直接输出   
}PWC_IO_MODE;
    
/**
 * @brief  PWC输入极性
 */
typedef enum __PWC_POLARITY
{
    PWC_POLARITY_BOTH     = 1,  /**< 双边沿捕获 */
    PWC_POLARITY_RAISING  = 2,  /**< 上升沿捕获 */
    PWC_POLARITY_FALLING  = 3   /**< 下降沿捕获 */

} PWC_POLARITY;

/**
 * @brief  PWC捕获模式
 */
typedef enum _PWC_CAPTURE_MODE
{
	PWC_CAPTURE_CONTINUES = 0,	/**< 连续捕获 */

	PWC_CAPTURE_ONCE = 1		/**< 只捕获一次 */

} PWC_CAPTURE_MODE;

/**
 * @brief  PWC输入滤波器采样时钟分频比
 */
typedef enum __PWC_FILTER_CLK_DIV
{   
    FILTER_CLK_DIV1 = 0,//不分频,等于系统时钟，即Fsys
    FILTER_CLK_DIV2,    //系统时钟的2分频，即 Fsys / 2 
    FILTER_CLK_DIV3,    //系统时钟的4分频，即 Fsys / 4
    FILTER_CLK_DIV4,    //系统时钟的8分频，即 Fsys / 8
    
}PWC_FILTER_CLK_DIV;


/**
 * @brief  PWC错误号定义
 */
typedef enum __PWC_ERROR_CODE
{
    PWC_ERROR_INVALID_PWC_INDEX = -128,		//定时器索引错误
	PWC_ERROR_INVALID_PWC_CH,				//PWC通道选择错误
    PWC_ERROR_INVALID_PWC_POLARITY,			//输入极性选择错误
    PWC_ERROR_OK = 0
}PWC_ERROR_CODE;


/** 
 * @brief  PWC初始化结构体定义
 */  
typedef struct __PWC_StructInit
{
    uint8_t  SingleGet;          //捕获次数： 1 -- 只读取1次， 0 -- 连续读取
    
    uint16_t TimeScale;          //PWC测量的量程(时钟分频系数)，取值范围[1,65535], 即可以测量的范围（1/Fsys * TimeScale  ~  65535/Fsys * TimeScale）
    
    uint8_t  DMAReqEnable;       //是否使能DMA将读取的数据搬运MEM中，参数取值： 1 -- 使能DMA请求； 0 -- 禁能DMA请求
    
    uint8_t  FilterTime;         //滤波时间，范围：1/Fpwc ~ 128/Fpwc
    
    uint8_t  Polarity;           //捕获极性，参数取值范围：边缘到边缘， 上升沿到上升沿，下降沿到下降沿，详细请见 #PWC_POLARITY

    uint8_t  PwcSourceSelect;    //0: gpio  1:pwm

    uint8_t  PwcOpt;           	 //0: first valid edge not generate dma req  1: first valid edge generate dma req
          
    uint8_t  MasterModeSel;      //000: Reset - timer3_ug is used as trigger out
     							 //001: Update - the update event is used as trigger out
   								 //010: PWM - pwm_o1 is used as trigger out.
    uint8_t  MasterSlaveSel;     //0: trigger input is no action
                                //1: the counter is reset by the posedge of trigger input
/*									TriggerInSrc
 *  								Timer6:
 *                               				00: Internal trigger 0 (itr0), Timer7
 *												01: Internal trigger 1 (itr1), Timer8
 *												10: Internal trigger 2 (itr2), Timer9
 *												10: Internal trigger 3 (itr3), Timer10
 *									Timer7:
 *                               				00: Internal trigger 0 (itr0), Timer6
 *												01: Internal trigger 1 (itr1), Timer8
 *												10: Internal trigger 2 (itr2), Timer9
 *												11: Internal trigger 3 (itr3), Timer10
 *									Timer8:
 *												00: Internal trigger 0 (itr0), Timer6
 *												01: Internal trigger 1 (itr1), Timer7
 *												10: Internal trigger 2 (itr2), Timer9
 *												11: Internal trigger 3 (itr3), Timer10
 *									Timer9:
 *												00: Internal trigger 0 (itr0), Timer6
 *												01: Internal trigger 1 (itr1), Timer7
 *												10: Internal trigger 2 (itr2), Timer8
 *												11: Internal trigger 3 (itr3), Timer10
 *									Timer10:
 *												00: Internal trigger 0 (itr0), Timer6
 *												01: Internal trigger 1 (itr1), Timer7
 *												10: Internal trigger 2 (itr2), Timer8
 *												11: Internal trigger 3 (itr3), Timer9
 */
    uint8_t  TriggerInSrc;

    uint8_t  CcxmAndCccUpdataSel;  //0: the timerX_ccxm and timerX_ccc only update when timerX_ug or timerX_com set
    							   //1:  the timerX_ccxm and timerX_ccc is update when timerX_ug or timerX_com set, or posedge of trigger input happen

}PWC_StructInit;  

/**
 * @brief PWC IOCTROL 命令
 */
typedef enum _PWC_IOCTRL_CMD
{
	PWC_DATA_GET = 0,                    /**< PWC数据获取 */
	PWC_DONE_STATUS_GET,                 /**< PWC捕获完成状态获取 */
	PWC_OVER_CAPTURE_STATUS_GET,         /**< PWC重复捕获状态获取 */
	PWC_ERR_STATUS_GET,                  /**< PWC错误状态获取 */

	PWC_OVER_CAPTURE_STATUS_CLR,         /**< PWC重复捕获状态清除 */
	PWC_ERR_STATUS_CLR,                  /**< PWC错误状态清除 */

    PWC_POLARITY_UPDATE,                 /**< PWC输入极性设置 */

} PWC_IOCTRL_CMD;

/**
 * @brief PWC IOCTROL 参数
 */
typedef struct _PWC_IOCTRL_ARG
{

	PWC_POLARITY    PWCPolarity;    /**< PWC输入极性 */

} PWC_IOCTRL_ARG;

/**
 * @brief      将GPIO复用为PWC引脚或还原为GPIO引脚.
 *
 * @param[in]  TimerId  定时器选择，注意：PWC只适用GTIMER: TIMER5/TIMER6/TIMER7/TIMER8/TIMER9/TIMER10
 * @param[in]  PWCIoSel  gpio pwc input select
						   note:all gpio can be PWC input and should enable IE witch is the selected gpio by software
						   GPIOPIN---PWCIoSel
						   GPIOA0----0
						   GPIOA1----1
						   GPIOA2----2
						   GPIOA3----3
						   ...
						   GPIOA29--29
						   GPIOA30--30
						   GPIOA31--31
						   GPIOB0---0+32
						   GPIOB1---1+32
						   GPIOB2---2+32
						   GPIOB3---3+32
						   ...
						   GPIOB25---25+32
						   GPIOB26---26+32
						   GPIOB27---27+32
 *
 * @return     PWC_ERROR_CODE
 */
PWC_ERROR_CODE PWC_GpioConfig(TIMER_INDEX TimerId, uint8_t PWCIoSel);

/**
 * @brief     选择一个定时器下PWC1通道并配置参数
 *
 * @param[in] TimerIdx  定时器索引号，支持 TIMER5/TIMER6/TIMER7/TIMER8/TIMER9/TIMER10
 * @param[in] PWCParam  PWC初始化参数，详细参考 PWCInfo
 *
 * @return    错误号：0 - 正确，其他为错误，详细参考PWC_ERROR_CODE
 */
PWC_ERROR_CODE PWC_Config(TIMER_INDEX TimerIdx, PWC_StructInit *PWCParam);

/**
 * @brief     使能TimerIdx下的PWC通道进行采集
 *
 * @param[in] TimerIdx  定时器索引号，支持 TIMER5/TIMER6/TIMER7/TIMER8/TIMER9/TIMER10
 *
 * @return    错误号：0 - 正确，其他为错误，详细参考PWC_ERROR_CODE
 */
PWC_ERROR_CODE PWC_Enable(TIMER_INDEX TimerIdx);

/**
 * @brief     关闭TimerIdx下的PWC通道进行采集
 *
 * @param[in] TimerIdx  定时器索引号，支持 TIMER5/TIMER6/TIMER7/TIMER8/TIMER9/TIMER10
 *
 * @return    错误号：0 - 正确，其他为错误，详细参考PWC_ERROR_CODE
 */
PWC_ERROR_CODE PWC_Disable(TIMER_INDEX TimerIdx);

/**
 * @brief     读取TimerIdx下的一路PWC捕获的数据
 *
 * @param[in] TimerIdx  定时器索引号，支持 TIMER5/TIMER6/TIMER7/TIMER8/TIMER9/TIMER10
 *
 * @return    0xffff -- 数据溢出，其他：PWC获取到的数据
 */
uint32_t PWC_CaptureValueGet(TIMER_INDEX TimerIdx);

/**
 * @brief     TimerIdx下的PWC操作
 *
 * @param[in] TimerIdx  定时器索引号，支持 TIMER5/TIMER6/TIMER7/TIMER8/TIMER9/TIMER10
 *
 * @return    >=0: 正确数值， < 0: 错误返回值
 */
PWC_ERROR_CODE PWC_IOCTRL(TIMER_INDEX TimerIdx,PWC_IOCTRL_CMD Cmd, PWC_IOCTRL_ARG *Arg);


#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__PWC_H__

/**
 * @}
 * @}
 */

