/**
  ******************************************************************************
  * @file    reset.h
  * @author  Peter Zhai
  * @version V1.0
  * @date    2017-10-27
  * @brief
  ******************************************************************************
  * @attention
  *
  ******************************************************************************
  */

/**
 * @addtogroup SYSTEM
 * @{
 * @defgroup reset reset.h
 * @{
 */

#ifndef __RESET_H__
#define __RESET_H__

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus 


/**
 * reset module(Register)1 macro define
 */
typedef enum __RESET_REGISTER_MODULE1
{
	AUFIFO_REG_SEPA = (1 << 0),				/**<AUFIFO register separate reset*/
	ASDM_REG_SEPA = (1 << 1),				/**<ASDM register separate reset*/
	CAN0_REG_SEPA = (1 << 2),				/**<CAN0 register separate reset*/
	CAN1_REG_SEPA = (1 << 3),      			/**<CAN1 register separate reset */
	DMAC0_REG_SEPA = (1 << 4),				/**<DMAC0 register separate reset */
	DMAC1_REG_SEPA = (1 << 5),      		/**<DMAC1 register separate reset */
	DMAC2_REG_SEPA = (1 << 6),              /**<DMAC2 register separate reset */
	DPROD_S8_REG_SEPA = (1 << 7),			/**<DPROD_S8 register separate reset */
	EFUSE_REG_SEPA = (1 << 8),       		/**<EFUSE register separate reset */
	FFT_REG_SEPA = (1 << 9),				/**<FFT register separate reset */
	FSHC0_REG_SEPA = (1 << 10),				/**<FSHC0 register separate reset */
	IPC_REG_SEPA = (1 << 11),				/**<IPC register separate reset */
	IR_REG_SEPA = (1 << 12),				/**<IR register separate reset */
	I2S0_REG_SEPA = (1 << 13),				/**<I2S0 register separate reset */
	I2S1_REG_SEPA = (1 << 14),          	/**<I2S1 register separate reset */
	I2S2_REG_SEPA = (1 << 15),           	/**<I2S2 register separate reset */
	MDAC0_REG_SEPA = (1 << 16),           	/**<MDAC0 register separate reset */
	MDAC1_REG_SEPA = (1 << 17),   			/**<MDAC1 register separate reset */
	PWR_REG_SEPA = (1 << 18),           	/**<Power register separate reset */
	SADC_REG_SEPA = (1 << 19),              /**<SADC register separate reset */
	SDIO_REG_SEPA = (1 << 20),              /**<SDIO register separate reset */
	SPIM0_REG_SEPA = (1 << 21),          	/**<SPIM0 register separate reset */
	SPIM1_REG_SEPA = (1 << 22),          	/**<SPIM1 register separate reset */
	SPIS0_REG_SEPA = (1 << 23),          	/**<SPIS0 register separate reset */
	SPIS1_REG_SEPA = (1 << 24),          	/**<SPIS1 register separate reset */
	TIMER1_REG_SEPA = (1 << 25),          	/**<TIMER1 register separate reset */
	TIMER3_REG_SEPA = (1 << 25),          	/**<TIMER3 register separate reset */
	TIMER5_REG_SEPA = (1 << 27),          	/**<TIMER5 register separate reset */
	TIMER7_REG_SEPA = (1 << 28),          	/**<TIMER7 register separate reset */
	UART0_REG_SEPA = (1 << 29),          	/**<UART0 register separate reset */
	UART1_REG_SEPA = (1 << 30),          	/**<UART1 register separate reset */
	UART2_REG_SEPA = (1 << 31),          	/**<UART2 register separate reset */

}RESET_REGISTER_MODULE1;

/**
 * reset module(Register)2 macro define
 */
typedef enum __RESET_REGISTER_MODULE2
{
	AUMIXER_REG_SEPA = (1 << 0),				/**<AUMIXER register separate reset*/
	DVPC_REG_SEPA = (1 << 1),					/**<DVPC register separate reset*/
	FSHC1_REG_SEPA = (1 << 2),					/**<FSHC1 control register separate reset*/
	HSEM_REG_SEPA = (1 << 3),     				/**<HSEM register separate reset */
	KS_REG_SEPA = (1 << 4),						/**<KS register separate reset */
	LCDC_REG_SEPA = (1 << 5),      				/**<LCDC register separate reset */
	MTIMER_REG_SEPA = (1 << 6),               	/**<MTIMER register separate reset */
	SRC0_REG_SEPA = (1 << 7),					/**<SRC0 register separate reset */
	SRC1_REG_SEPA = (1 << 8),       			/**<SRC1 register separate reset */
	SRC2_REG_SEPA = (1 << 9),					/**<SRC2 register separate reset */
	TOUCH_REG_SEPA = (1 << 10),					/**<TOUCH register separate reset */
	TIMER9_REG_SEPA = (1 << 11),				/**<TIMER9 register separate reset */
	USB_HS_REG_SEPA = (1 << 12),				/**<USB register separate reset */
	MDAC2_REG_SEPA = (1 << 13),					/**<MDAC2 register separate reset */
	DSD_REG_SEPA = (1 << 14),          			/**<DSD register separate reset */
	DPSRC_REG_SEPA = (1 << 15),           		/**<DPSRC register separate reset */

}RESET_REGISTER_MODULE2;
/**
 * reset module(Function)1 macro define
 */
typedef enum __RESET_FUNC_MODULE1
{
	ASDM0_FUNC_SEPA = (1 << 0),				/**<ASDM0 function separate reset*/
	ASDM1_FUNC_SEPA = (1 << 1),				/**<ASDM1 function separate reset*/
	BTDM_FUNC_SEPA = (1 << 2),		        /**<BTDM function separate reset*/
	CAN0_FUNC_SEPA = (1 << 3),		        /**<CAN0 function separate reset*/
	DMAC0_FUNC_SEPA = (1 << 4),             /**<DMAC0 function separate reset */
	FFT_FUNC_SEPA = (1 << 5),		        /**<FFT function separate reset */
	FSHC0_FUNC_SEPA = (1 << 6),       		/**<FSHC0 function separate reset */
	MDAC0_FUNC_SEPA = (1 << 7),				/**<MDAC0 function separate reset */
	MDM_FUNC_SEPA = (1 << 8),      	     	/**<MDM function separate reset */
	MCLKCNT_FUNC_SEPA = (1 << 9),			/**<MCLKCNT function separate reset */
	I2C0_FUNC_SEPA = (1 << 10),	            /**<I2C0 function separate reset */
	I2S0_FUNC_SEPA = (1 << 11),		    	/**<I2S0 function separate reset */
	I2S1_FUNC_SEPA = (1 << 12),				/**<I2S1 function separate reset */
	IR_FUNC_SEPA = (1 << 13),				/**<IR function separate reset */
	SADC_FUNC_SEPA = (1 << 14),           	/**<SADC function separate reset */
	SDIO_FUNC_SEPA = (1 << 15),           	/**<SDIO function separate reset */
	SPDIF0_FUNC_SEPA = (1 << 16),           /**<SPDIF0 function separate reset */
	SPDIF1_FUNC_SEPA = (1 << 17),   		/**<SPDIF1 function separate reset */
	SPIM0_FUNC_SEPA = (1 << 18),           	/**<SPIM0 function separate reset */
	SPIS0_FUNC_SEPA = (1 << 19),            /**<SPIS0 function separate reset */
	TIMER1_FUNC_SEPA = (1 << 20),           /**<TIMER1 function separate reset */
	TIMER2_FUNC_SEPA = (1 << 21),          	/**<TIMER2 function separate reset */
	TIMER3_FUNC_SEPA = (1 << 22),          	/**<TIMER3 function separate reset */
	TIMER4_FUNC_SEPA = (1 << 23),          	/**<TIMER4 function separate reset */
	TIMER5_FUNC_SEPA = (1 << 24),          	/**<TIMER5 function separate reset */
	TIMER6_FUNC_SEPA = (1 << 25),          	/**<TIMER6 function separate reset */
	TIMER7_FUNC_SEPA = (1 << 26),          	/**<TIMER7 function separate reset */
	TIMER8_FUNC_SEPA = (1 << 27),           /**<TIMER8 function separate reset */
	UART0_FUNC_SEPA = (1 << 28),          	/**<UART0 function separate reset */
	USB_FUNC_SEPA = (1 << 29),          	/**<USB function separate reset */
	UART1_FUNC_SEPA = (1 << 30),          	/**<UART1 function separate reset */
	USB_FS_HM_EN = (1 << 31),         	 	/**<USB function separate reset */

}RESET_FUNC_MODULE1;

/**
 * reset module(Function)2 macro define
 */
typedef enum __RESET_FUNC_MODULE2
{
	AUFIFO_FUNC_SEPA = (1 << 0),			/**<AUFIFO function separate reset*/
	AUMIXER_FUNC_SEPA = (1 << 1),			/**<AUMIXER function separate reset*/
	CAN1_FUNC_SEPA = (1 << 2),		        /**<CAN1 function separate reset*/
	DMAC1_FUNC_SEPA = (1 << 3),		        /**<DMAC1 function separate reset*/
	DMAC2_FUNC_SEPA = (1 << 4),             /**<DMAC2 function separate reset */
	DPROD_S8_FUNC_SEPA = (1 << 5),		    /**<DPROD_S8 function separate reset */
	DVPC_FUNC_SEPA = (1 << 6),       		/**<DVPC function separate reset */
	FSHC1_FUNC_SEPA = (1 << 7),				/**<FSHC1 function separate reset */
	I2C1_FUNC_SEPA = (1 << 8),      	    /**<I2C1 function separate reset */
	I2S2_FUNC_SEPA = (1 << 9),				/**<I2S2 function separate reset */
	IPC_FUNC_SEPA = (1 << 10),	     		/**<IPC function separate reset */
	KS_FUNC_SEPA = (1 << 11),		    	/**<KS control function separate reset */
	LCDC_FUNC_SEPA = (1 << 12),				/**<LCDC function separate reset */
	MDAC1_FUNC_SEPA = (1 << 13),			/**<MDAC1 function separate reset */
	MTIMER_FUNC_SEPA = (1 << 14),           /**<MTIMER function separate reset */
	SPIM1_FUNC_SEPA = (1 << 15),           	/**<SPIM1 function separate reset */
	SPIS1_FUNC_SEPA = (1 << 16),           	/**<SPIS1 function separate reset */
	SRC0_FUNC_SEPA = (1 << 17),   			/**<SRC0 function separate reset */
	SRC1_FUNC_SEPA = (1 << 18),           	/**<SRC1 function separate reset */
	SRC2_FUNC_SEPA = (1 << 19),             /**<SRC2 function separate reset */
	TOUCH_FUNC_SEPA = (1 << 20),           	/**<TOUCH function separate reset */
	UART2_FUNC_SEPA = (1 << 21),          	/**<UART2 function separate reset */
	USB_HS_FUNC_SEPA = (1 << 22),          	/**<USB function separate reset */
	TIMER9_FUNC_SEPA = (1 << 23),          	/**<TIMER9 function separate reset */
	TIMER10_FUNC_SEPA = (1 << 24),          /**<TIMER10 function separate reset */
	MDAC2_FUNC_SEPA = (1 << 25),          	/**<MDAC2 function separate reset */
	DSD_FUNC_SEPA = (1 << 26),          	/**<DSD function separate reset */
	DPSRC_FUNC_SEPA = (1 << 27),          	/**<DPSRC function separate reset */
	HPMDM_FUNC_SEPA = (1 << 28),          	/**<HPMDM function separate reset */
	HDTMDM_FUNC_SEPA = (1 << 29),          	/**<HDTMDM function separate reset */
}RESET_FUNC_MODULE2;


/**
 * @brief	获取复位源
 * @param	None
 * @return	返回复位源
 * 			bit0=1	Power On Reset
 * 			bit1=1	Pin Reset
 * 			bit2=1	Powerkey reset
 * 			bit3=1	Core2Dog Reset
 * 			bit4=1	Core1Dog Reset
 * 			bit5=1	LVD(VDD33) Reset
 * 			bit6=1	CPU Debug Reset
 * 			bit7=1	System Reset
 * @note
 */
uint16_t Reset_FlagGet(void);

/**
 * @brief	Clear复位标志
 * @param	None
 * @return	None
 * @note
 */
void Reset_FlagClear(void);

/**
 * @brief	系统复位
 * @param	None
 * @return	None
 * @note
 */
void Reset_McuSystem(void);

/**
 * @brief	模块寄存器复位
 * @param	Module：模块
 * @return	None
 * @note
 */
void Reset_Register1Reset(RESET_REGISTER_MODULE1 Module);

/**
 * @brief	模块寄存器复位
 * @param	Module：模块
 * @return	None
 * @note
 */
void Reset_Register2Reset(RESET_REGISTER_MODULE2 Module);

/**
 * @brief	模块功能复位
 * @param	Module：模块
 * @return	None
 * @note
 */
void Reset_Function1Reset(RESET_FUNC_MODULE1 Module);

/**
 * @brief	模块功能复位
 * @param	Module：模块
 * @return	None
 * @note
 */
void Reset_Function2Reset(RESET_FUNC_MODULE2 Module);

#ifdef __cplusplus
}
#endif // __cplusplus 

#endif //__RESET_H__

/**
 * @}
 * @}
 */
