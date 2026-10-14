/**
 **************************************************************************************
 * @file    dsd.h
 * @brief   DSD Module API
 *
 * @author  Yangyu
 * @version V1.0.0
 *
 * $Created: 2026-2-6 15:32:38$
 *
 * @copyright Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#ifndef __DSD_H__
#define __DSD_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#include "type.h"
/**
 * @brief DSD格式
 */
typedef enum _DSD_TYPE_E
{
	DSD64 = 0,
	DSD128,
	DSD256,
	DSD512
} DSD_TYPE;

/**
 * @brief	 初始化DSD模块
 * @param	dsd_type：DSD64(64*44.1K)  DSD128(128*44.1K)   DSD256(256*44.1K)   DSD512(512*44.1K)
 * @param	lr_swap : 0:L/R 不交换，1:L/R交换
 * @param	clk_edge：0:下沿发  外部DSDDAC用上沿采， 1：上沿发，外部DSDDAC用下沿采
 * @param   enddian：0：  LSB 先发,1: MSB先发
 * @return  无
 */
void DSD_Init(DSD_TYPE dsd_type, bool lr_swap, bool clk_edge, bool enddian);

/**
 * @brief	使能 DSD模块
 * @param	无
 * @return  无
 */
void DSD_Enable();

/**
 * @brief	关闭DSD模块
 * @param	无
 * @return  无
 * @note  先调用  DSD_Pause,再关闭
 */
void DSD_Disable();
/**
 * @brief	暂停DSD模块
 * @param	pause_pattern：暂停期间发的数据
 * @return  无
 */
void DSD_Pause(uint8_t pause_pattern);


// IO组选择
#define IO_GROUP_A             0
#define IO_GROUP_B             1
//gpio_mode: GPIO输入输出模式
//gpio_index：GPIO引脚
#define SET_DSD_GPIO_VAL(io_group, gpio_mode, gpio_index)               ((io_group << 24) | (gpio_mode << 8) | (gpio_index))

#define GET_DSD_IO_GROUP(val)                                     ((val >> 24) & 0x0f)
#define GET_DSD_GPIO_MODE(val)                                    ((val >> 8) & 0x1f)
#define GET_DSD_GPIO_INDEX(val)                                   (val & 0x1f)
#define GET_DSD_GPIO_PORT(val)                                    (1 << (val & 0x1f))

//dsd_dclk gpio
#define DSD_DCLK_GPIOA7              SET_DSD_GPIO_VAL(IO_GROUP_A,0x08,7)   // dsd_dclk_0(o)
#define DSD_DCLK_GPIOA10             SET_DSD_GPIO_VAL(IO_GROUP_A,0x10,10)  // dsd_dclk_1(o)
#define DSD_DCLK_GPIOB0              SET_DSD_GPIO_VAL(IO_GROUP_B,0x0f,0)   // dsd_dclk_2(o)
#define DSD_DCLK_GPIOB6              SET_DSD_GPIO_VAL(IO_GROUP_B,0x08,6)   // dsd_dclk_3(o)

//dsd_l gpio
#define DSD_L_GPIOA5                 SET_DSD_GPIO_VAL(IO_GROUP_A,0x08,5)   // dsd_l_0(o)
#define DSD_L_GPIOA13                SET_DSD_GPIO_VAL(IO_GROUP_A,0x0c,13)  // dsd_l_1(o)
#define DSD_L_GPIOB3                 SET_DSD_GPIO_VAL(IO_GROUP_B,0x0d,3)   // dsd_l_2(o)
#define DSD_L_GPIOB9                 SET_DSD_GPIO_VAL(IO_GROUP_B,0x07,9)   // dsd_l_3(o)

//dsd_r gpio
#define DSD_R_GPIOA6                 SET_DSD_GPIO_VAL(IO_GROUP_A,0x0b,6)   // dsd_r_0(o)
#define DSD_R_GPIOA11                SET_DSD_GPIO_VAL(IO_GROUP_A,0x0e,11)  // dsd_r_1(o)
#define DSD_R_GPIOB1                 SET_DSD_GPIO_VAL(IO_GROUP_B,0x0e,1)   // dsd_r_2(o)
#define DSD_R_GPIOB7                 SET_DSD_GPIO_VAL(IO_GROUP_B,0x07,7)   // dsd_r_3(o)

#define SET_DSD_PIN_MODE(X, Mode)	(GET_DSD_IO_GROUP(X) ? (GPIO_PortBModeSet(GET_DSD_GPIO_PORT(X), (Mode))) : GPIO_PortAModeSet(GET_DSD_GPIO_PORT(X), (Mode)))

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__DSD_H__
