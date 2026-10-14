/**
 **************************************************************************************
 * @file    chip_interrupt.h
 * @brief
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2026-1-16 19:46:00$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

/**
 * @addtogroup SYSTEM
 * @{
 * @defgroup chip_interrupt chip_interrupt.h
 * @{
 */
 
#ifndef __CHIP_INTERRUPT_H__
#define __CHIP_INTERRUPT_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

typedef enum _CORE_INT
{
	INT_GPIO = 0,
	INT_USB_FS,
	INT_SRC0,
	INT_USB_HS,
	INT_PLMT,
	INT_SW,
	INT_Timer1,
	INT_Timer2,
	INT_Timer5,
	INT_Timer6,
	INT_SPIM0,	//10
	INT_SPIS0,
	INT_UART0,
	INT_SDIO,
	INT_Reserved1,//
	INT_IR,
	INT_DM,		//rwip_isr
	INT_BT,		//rwbt_isr
	INT_BLE,	//rwble_isr
	INT_DMA1_D,
	INT_DMA1_T,		//20
	INT_Timer7,
	INT_Timer8,
	INT_Dprods8,
	INT_FFT,		//SDK中没有使用
	INT_KeyScan,
	INT_CAN1,
	INT_RTC,
	INT_IPC,
	INT_DMA0_D,
	INT_DMA0_T,		//30
	INT_DMA0_E,
	INT_Touch,		//32
	INT_WakeUp,
	INT_SRC1,
	INT_Reserved2,
	INT_DVPC,
	INT_Reserved3,
	INT_Reserved4,
	INT_SRC2,
	INT_Reserved5,	//40
	INT_Reserved6,
	INT_SPIM1,//
	INT_SPIS1,
	INT_UART1,
	INT_UART2,
	INT_CAN0,
	INT_I2C0,
	INT_I2C1,
	INT_Reserved7,
	INT_Reserved8,	//50
	INT_Timer3,
	INT_Timer4,//
	INT_Reserved9,
	INT_DMA2_T,
	INT_DMA2_D,
	INT_Reserved10,
	INT_Reserved11,
	INT_Reserved12,
	INT_Reserved13,
	INT_HSEM,		//60
	INT_Reserved14,
	INT_DMA2_E,//
	INT_DMA1_E,
}CORE_INT;

/**
 * @brief	core外设中断使能
 * @param	IntBit 外设枚举值
 * @return	无
 */
void CoreInterruptEnable(CORE_INT IntBit);
/**
 * @brief	core外设中断禁能
 * @param	IntBit 外设枚举值
 * @return	无
 */
void CoreInterruptDisable(CORE_INT IntBit);
/**
 * @brief	core外设中断使能/禁能状态获取
 * @param	IntBit 外设枚举值
 * @return	无
 */
bool CoreInterruptStateGet(CORE_INT IntBit);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif /*__CHIP_INTERRUPT_H__*/

/**
 * @}
 * @}
 */

