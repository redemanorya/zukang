/**
  ******************************************************************************
  * @file	 irqn.h
  * @author  Peter Zhai
  * @version V1.0
  * @date	 2017-10-27
  * @brief
  ******************************************************************************
  * @attention
  *
  ******************************************************************************
  */

/**
 * @addtogroup SYSTEM
 * @{
 * @defgroup irqn irqn.h
 * @{
 */
#include "type.h"

#ifndef IRQN_H_
#define IRQN_H_

/**
 * @brief	外设中断ID
 */
#if 1

//CORE1_GROUP0 & CORE1_GROUP1
typedef enum CORE1_IRQn
{
	/*
	 * internal interrupt(Exception/Fault)
	 */
	RST_IRQn		= -9,
	TLB_Fill_IRQn		= -8,
	PTE_IRQn	= -7,
	TLB_Misc_IRQn		= -6,//programmable as below
	TLB_VLPT_Miss_IRQn		= -5,
	Machine_ERR_IRQn		= -4,
	Debug_IRQn		= -3,
	General_Exception_IRQn		= -2,
	Syscall_IRQn		= -1,
	/*
	 * SOC interrupt(External Interrupt)
	 */
	Gpio_Touch_IRQn						= 0,				//Touch Default C2
	USB_FS_WAKEUP_IRQn					= 1,
	SRC_IRQn							= 2,//SRC0/1/2		//SRC1/2 Default C2
	USB_HS_IRQn							= 3,//USB HS
	PLMT_IRQn							= 4,
	SWI_IRQn							= 5,
	TMR1_IRQn							= 6,
	TMR2_IRQn							= 7,
	TMR5_IRQn							= 8,
	TMR6_IRQn							= 9,
	SPIM_IRQn							= 10,//SPIM0,SPIM1	//SPIM1 Default C2
	SPIS_IRQn							= 11,//SPIS0,SPIS1	//SPIS1 Default C2
	UART_IRQn							= 12,//UART0/1/2	//UART1/2 Default C2
	SDIO0_IRQn							= 13,
	CAN0_IRQn							= 14,				//CAN0 Default C2
	IR_I2C_IRQn							= 15,//I2C0,I2C1	//I2C0/1 Default C2
	DM_IRQn								= 16,
	BT_IRQn								= 17,
	BLE_IRQn							= 18,
	TMR3_DMAC1DINT_IRQn					= 19,				//TMR3 Default C2
	TMR4_DMAC1TINT_IRQn 				= 20,				//TMR4 Default C2
	TMR7_IRQn							= 21,
	TMR8_DMAC2TINT_IRQn					= 22,				//DMAC2TINT Default C2
	DPRODS8_DMAC2DINT_IRQn				= 23,				//DMAC2DINT Default C2
	FFT_LCDC_DVPC_IRQn					= 24,				//DVPC Default C2
	KEYSCAN_AUFIFO_IRQn 				= 25,
	CAN1_IRQn							= 26,
	RTC_IRQn							= 27,
	IPC_HSEMINT0_IRQn					= 28,				//HSEMINT0 Default C2
	DMAC0_DINT_IRQn 					= 29,
	DMAC0_TINT_IRQn 					= 30,
	DMAC_EINT_IRQn						= 31,//DMAEint0/1/2	//DMAEint1/2 Default C2
} IRQn_Type;

typedef enum CORE2_IRQn
{
	/*
	 * internal interrupt(Exception/Fault)
	 */
	/*
	RST_IRQn		= -9,
	TLB_Fill_IRQn		= -8,
	PTE_IRQn	= -7,
	TLB_Misc_IRQn		= -6,//programmable as below
	TLB_VLPT_Miss_IRQn		= -5,
	Machine_ERR_IRQn		= -4,
	Debug_IRQn		= -3,
	General_Exception_IRQn		= -2,
	Syscall_IRQn		= -1,
	*/
	/*
	 * SOC interrupt(External Interrupt)
	 */
	Gpio_C2_IRQn		= 0,  //						//Default C1
	SDIO0_C2_IRQn		= 1,  //						//Default C1
	PLMT_C2_IRQn		= 2,							//Default C1
	SRC1_C2_IRQn		= 3,
	FFT_C2_IRQn			= 4,							//Default C1
	SWI_C2_IRQn			= 5,							//Default C1
	DPROD__C2_IRQn		= 6,							//Default C1
	AUFIFO_C2_IRQn		= 7,
	Timer3_C2_IRQn		= 8,
	Timer4_C2_IRQn		= 9,
	Timer7_C2_IRQn		= 10,							//Default C1
	Timer8_C2_IRQn		= 11,							//Default C1
	SPDIF1_C2_IRQn		= 12,
	SPIM1_C2_IRQn		= 13,
	SRC2_C2_IRQn		= 14,
	SPIS1_C2_IRQn		= 15,
	UART2_C2_IRQn		= 16,
	MDAC_C2_IRQn		= 17,
	I2S_C2_IRQn			= 18,
	IPC_C2_IRQn			= 19,							//Default C1
	Timer9_C2_IRQn		= 20,
	Timer10_C2_IRQn		= 21,
	HSEM_C2_IRQn		= 22,
	DMAC2_DINT_C2_IRQn	= 23,
	DMAC2_TINT_C2_IRQn	= 24,
	DMAC2_EINT_C2_IRQn	= 25,
	DMAC1_DINT_C2_IRQn	= 26,							//Default C1
	DMAC1_TINT_C2_IRQn	= 27,							//Default C1
	DMAC1_EINT_C2_IRQn	= 28,
	DMAC0_DINT_C2_IRQn	= 29,							//Default C1
	DMAC0_TINT_C2_IRQn	= 30,							//Default C1
	DMAC0_EINT_C2_IRQn	= 31,
} CORE2_IRQn_Type;



#else
typedef enum IRQn
{
	/*
	 * internal interrupt(Exception/Fault)
	 */
	RST_IRQn		= -9,
	TLB_Fill_IRQn		= -8,
	PTE_IRQn	= -7,
	TLB_Misc_IRQn		= -6,//programmable as below
	TLB_VLPT_Miss_IRQn		= -5,
	Machine_ERR_IRQn		= -4,
	Debug_IRQn		= -3,
	General_Exception_IRQn		= -2,
	Syscall_IRQn		= -1,
	/*
	 * SOC interrupt(External Interrupt)
	 */
	Gpio_IRQn		= 0,  //
	Wakeup_IRQn		= 1,  //
	POWERKEY_IRQn	= 2,
	Usb_IRQn		= 3,
	FFTInt_IRQn		= 4,
	SWI_IRQn		= 5,
	TMR1_IRQn		= 6,
	Timer2_IRQn		= 7,
	Timer3_IRQn		= 8,
	Timer4_IRQn		= 9,
	Timer5_IRQn		= 10,
	Timer6_IRQn		= 11,
	RTC_IRQn		= 12,
	HOSCCNT_IRQn	= 13,
	DM_IRQn			= 14,
	BT_IRQn			= 15,
	BLE_IRQn		= 16,
	SPIM_IRQn		= 17,
	SPIS_IRQn		= 18,
	UART0_IRQn		= 19,
	UART1_IRQn		= 20,
	SDIO0_IRQn		= 21,
	SPDIF_IRQn		= 22,
	IR_IRQn			= 23,
	I2S_IRQn		= 24,
	I2C_IRQn		= 25,
	DMA0_IRQn		= 26,
	DMA1_IRQn		= 27,
	DMA2_IRQn		= 28,
	DMA3_IRQn		= 29,
} IRQn_Type;


#endif

///**
// * @brief	外设默认中断优先级
// */
//typedef enum IRQn_PRIO
//{
//	  /*
//	   * SOC interrupt(External Interrupt)
//	   */
//	  TMR1_IRQn_PRIO		= 3,
//	  Wakeup_IRQn_PRIO	= 3,
//	  Gpio_IRQn_PRIO		= 3,
//	  SWI_IRQn_PRIO		= 3,
//	I2C_IRQn_PRIO		= 3,
//	UART0_IRQn_PRIO		= 3,
//	Timer2_IRQn_PRIO	= 3,
//	  DMA0_IRQn_PRIO		= 3,
//	  DMA1_IRQn_PRIO		= 3,
//	  DMA2_IRQn_PRIO		= 3,
//	  DMA3_IRQn_PRIO		= 3,
//	DM_IRQn_PRIO		= 3,
//	  BT_IRQn_PRIO		= 3,
//	  BLE_IRQn_PRIO		= 3,
//	Usb_IRQn_PRIO		= 3,
//	  SPIM_IRQn_PRIO		= 3,
//	I2S_IRQn_PRIO		= 3,
//	  Timer3_IRQn_PRIO	= 3,
//	Timer4_IRQn_PRIO	= 3,
//	Timer5_IRQn_PRIO	= 3,
//	Timer6_IRQn_PRIO	= 3,
//	  SDIO0_IRQn_PRIO		= 3,
//	  FFTInt_IRQn_PRIO	= 3,
//	POWERKEY_IRQn_PRIO	= 3
//} IRQn_PRIO_TYPE;

/**
 * @brief	设置CPU Powerbrake时使用
 */
#define NDS_THROTTLE_LEVEL_0  (0x00)
#define NDS_THROTTLE_LEVEL_1  (0x10)
#define NDS_THROTTLE_LEVEL_2  (0x20)
#define NDS_THROTTLE_LEVEL_3  (0x30)
#define NDS_THROTTLE_LEVEL_4  (0x40)
#define NDS_THROTTLE_LEVEL_5  (0x50)
#define NDS_THROTTLE_LEVEL_6  (0x60)
#define NDS_THROTTLE_LEVEL_7  (0x70)
#define NDS_THROTTLE_LEVEL_8  (0x80)
#define NDS_THROTTLE_LEVEL_9  (0x90)
#define NDS_THROTTLE_LEVEL_10 (0xA0)
#define NDS_THROTTLE_LEVEL_11 (0xB0)
#define NDS_THROTTLE_LEVEL_12 (0xC0)
#define NDS_THROTTLE_LEVEL_13 (0xD0)
#define NDS_THROTTLE_LEVEL_14 (0xE0)
#define NDS_THROTTLE_LEVEL_15 (0xF0)

/**
 * @brief	使能全局中断
 * @param	None
 * @return	None
 * @note
 */
void GIE_ENABLE(void);

/**
 * @brief	禁能全局中断
 * @param	None
 * @return	None
 * @note
 */
void GIE_DISABLE(void);

/**
 * @brief	获取全局中断
 * @param	None
 * @return	None
 * @note
 */
bool GIE_STATE_GET(void);

/**
 * @brief	使能PowerBrake功能
 * @param	None
 * @return	None
 * @note
 */
void Throttle_Enable(void);

/**
 * @brief	禁能PowerBrake功能
 * @param	None
 * @return	None
 * @note
 */
void Throttle_Disable(void);

/**
 * @brief	设置Throttle level
 * @param	setting
 *			NDS_THROTTLE_LEVEL_0，   16个CPU clock全部有效，具有最高的性能
 *			...
 *			NDS_THROTTLE_LEVEL_15，16个CPU clock，只有1个clock起作用,具有最低的性能
 * @return	None
 * @note
 */
void Throttle_Setup(uint32_t setting);

/**
 * @brief	使能外设中断
 * @param	IRQn	外设ID
 * @return	None
 * @note
 */
void NVIC_EnableIRQ(IRQn_Type IRQn);

/**
 * @brief	禁能外设中断
 * @param	IRQn	外设ID
 * @return	None
 * @note
 */
void NVIC_DisableIRQ(IRQn_Type IRQn);

/**
 * @brief	设置外设的中断优先级
 * @param	IRQn	外设ID
 * @param	priority
 *			0,1,2,3 0=highest 3=lowest
 * @return	None
 * @note
 */
void NVIC_SetPriority(IRQn_Type IRQn, uint32_t priority);

/**
 * @brief	获取外设的中断优先级
 * @param	IRQn	外设ID
 * @return	中断优先级
 * @note
 */
uint32_t NVIC_GetPriority(IRQn_Type IRQn);

#endif /* IRQN_H_ */

/**
 * @}
 * @}
 */
 