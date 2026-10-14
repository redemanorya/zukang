/******************************************************************************
 * @file    gpio.h
 * @author  BKD
 * @version V1.0.0
 * @date    2020-6-11
 * @brief   gpio register description and function declare
 * @TypicalAppication
 *   Gpio can be configured as general io pin also be reused for other pin.
 *   They are configured as general io pin using the following functions:
 *   1.Digital output:  
	--Disable input function: Clr GPIO_A_IE, GPIO_B_IE
		--Enable Output function: Set GPIO_A_OE, GPIO_B_OE
		--Strong or week output:  Set or clr GPIO_A_OUTDS, GPIO_B_OUTDS
		--Pull up or down function: Set or clr {PU,PD} 
		--strong or week pull: Set or clr GPIO_A_DS, GPIO_B_DS
		--Output High level or Low level: Set or clr GPIO_A_OUT, GPIO_B_OUT 
		
	 2.Digital input:
		--Disable output function: Clr GPIO_A_OE, GPIO_B_OE
		--Enable input function: Set GPIO_A_IE, GPIO_B_IE
		--Pull up or down function: Set or clr {PU,PD} 
		--strong or week pull: Set or clr GPIO_A_DS, GPIO_B_DS
		--Read input status
	 
	 3.Analog input:
		--Clr PU: Clr GPIO_A_PU, GPIO_B_PU
		--Set PD: Set GPIO_A_PD, GPIO_B_PD
		--Clr OE: Clr GPIO_A_OE, GPIO_B_OE
		--Clr IE: Clr GPIO_A_IE, GPIO_B_IE
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
 * 
 */

/**
 * @addtogroup GPIO
 * @{
 * @defgroup gpio gpio.h
 * @{
 */
 
#ifndef __GPIO_H__
#define	__GPIO_H__

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

#include "type.h"
#include "timer.h"

#define     RESTORE_TO_GENERAL_IO     			   		(0xFF)  /**<this mcaro will be used as a para value to restore the multiplex io to general io*/

/**
 * RegIndex for Bank A
 */
#define		GPIO_A_START					(0x00)
#define		GPIO_A_IN						(0x00 + GPIO_A_START)	/**<GPIOA input data register macro*/
#define		GPIO_A_OUT						(0x01 + GPIO_A_START)	/**<GPIOA output data register macro*/
#define     GPIO_A_SET						(0x02 + GPIO_A_START)
#define    	GPIO_A_CLR						(0x03 + GPIO_A_START)
#define    	GPIO_A_TGL						(0x04 + GPIO_A_START)
#define		GPIO_A_IE						(0x05 + GPIO_A_START)	/**<GPIOA input enable register macro.0: disable, 1: enable*/
#define		GPIO_A_OE						(0x06 + GPIO_A_START)	/**<GPIOA output enable register macro.0: disable, 1: enable*/
#define		GPIO_A_ANA_EN					(0x07 + GPIO_A_START)
#define		GPIO_A_PU						(0x08 + GPIO_A_START)	/**<GPIOA pull up register macro*/
#define		GPIO_A_PD						(0x09 + GPIO_A_START)	/**<GPIOA pull down register macro.{pu,pd} 1'b1: pull-up, 1'b1: pull-down, 1'b0: no pull-up, no pull-down, */
#define		GPIO_A_CORE_OUT_MASK			(0x0a + GPIO_A_START)
#define		GPIO_A_DMA_OUT_MASK				(0x0b + GPIO_A_START)
#define     GPIO_A_OUT_MASK                 GPIO_A_CORE_OUT_MASK
#define		GPIO_A_END						GPIO_A_DMA_OUT_MASK
#define		GPIO_A_REG_RPU					(0x35 + GPIO_A_START)	 /**10k pull-up, 1:10K; 0:100K*/

/**
 * RegIndex for Bank B
 */
#define		GPIO_B_START					(GPIO_A_END + 1)
#define		GPIO_B_IN						(0x00 + GPIO_B_START)	/**<GPIOB input data register macro*/
#define		GPIO_B_OUT						(0x01 + GPIO_B_START)	/**<GPIOB output data register macro*/
#define     GPIO_B_SET						(0x02 + GPIO_B_START)
#define     GPIO_B_CLR						(0x03 + GPIO_B_START)
#define    	GPIO_B_TGL						(0x04 + GPIO_B_START)
#define		GPIO_B_IE						(0x05 + GPIO_B_START)	/**<GPIOB input enable register macro.0: disable, 1: enable*/
#define		GPIO_B_OE						(0x06 + GPIO_B_START)	/**<GPIOB output enable register macro.0: disable, 1: enable*/
#define		GPIO_B_ANA_EN					(0x07 + GPIO_B_START)
#define		GPIO_B_PU						(0x08 + GPIO_B_START)	/**<GPIOB pull up register macro*/
#define		GPIO_B_PD						(0x09 + GPIO_B_START)	/**<GPIOB pull down register macro.{pu,pd} 2'b00: pull-up, 2'b01: AIN, 2'b10: no pull-up, no pull-down, 2'b11: pull-down*/
#define		GPIO_B_CORE_OUT_MASK			(0x0a + GPIO_B_START)
#define		GPIO_B_DMA_OUT_MASK				(0x0b + GPIO_B_START)
#define     GPIO_B_OUT_MASK                 GPIO_B_CORE_OUT_MASK
#define		GPIO_B_END						GPIO_B_DMA_OUT_MASK
#define		GPIO_B_REG_RPU					(0x38 + GPIO_A_START)	/**10k pull-up, 1:10K; 0:100K*/

/**
 * RegIndex for others
 */
#define		GPIO_GLB_INTE					(GPIO_B_END + 1)
/**
 * RegIndex for separate interrupt of GPIO Bank A
 */
#define		GPIO_A_INT			        	(GPIO_B_END + 2)
#define		GPIO_A_SEP_INTE					(0x00 + GPIO_A_INT)//  GPIOA[31:0] separate interrupt enable
#define		GPIO_A_SEP_NTRIG				(0x01 + GPIO_A_INT)
#define		GPIO_A_SEP_PTRIG				(0x02 + GPIO_A_INT)
#define		GPIO_A_SEP_INTC					(0x03 + GPIO_A_INT)
#define		GPIO_A_SEP_INTS					(0x04 + GPIO_A_INT)
#define		GPIO_A_INT_END					GPIO_A_SEP_INTS

/**
 * RegIndex for separate interrupt of GPIO Bank B
 */
#define		GPIO_B_INT						(GPIO_A_INT_END + 1)
#define		GPIO_B_SEP_INTE					(0x00 + GPIO_B_INT)	//  GPIOB[7:0] separate interrupt enable
#define		GPIO_B_SEP_NTRIG				(0x01 + GPIO_B_INT)
#define		GPIO_B_SEP_PTRIG				(0x02 + GPIO_B_INT)
#define		GPIO_B_SEP_INTC					(0x03 + GPIO_B_INT)
#define		GPIO_B_SEP_INTS					(0x04 + GPIO_B_INT)
#define		GPIO_B_INT_END					GPIO_B_SEP_INTS

#define		GPIO_C_START					(0x47+GPIO_A_START)
#define		GPIO_C_SEP_INTE					(0+GPIO_C_START)
#define		GPIO_C_SEP_NTRIG				(1+GPIO_C_START)
#define		GPIO_C_SEP_PTRIG				(2+GPIO_C_START)
#define		GPIO_C_SEP_INTC					(3+GPIO_C_START)
#define		GPIO_C_SEP_INTS					(4+GPIO_C_START)
#define		GPIO_C_REG_I					(5+GPIO_C_START)

#define		GPIO_FSHC_REG_START				(GPIO_C_REG_I+1)
#define		GPIO_FSHC_SCK_CTRL				(0+GPIO_FSHC_REG_START)
#define		GPIO_FSHC_CS_CTRL				(1+GPIO_FSHC_REG_START)
#define		GPIO_FSHC_MOSI_CTRL				(2+GPIO_FSHC_REG_START)
#define		GPIO_FSHC_MISO_CTRL				(3+GPIO_FSHC_REG_START)
#define		GPIO_FSHC_WP_CTRL				(4+GPIO_FSHC_REG_START)
#define		GPIO_FSHC_HOLD_CTRL				(5+GPIO_FSHC_REG_START)

#define		GPIO_GTIMER_SUB_CTRL			(1+GPIO_FSHC_HOLD_CTRL)

#define		GPIO_BT_DEBUG_IO				(1+GPIO_GTIMER_SUB_CTRL) 

#define		GPIO_MUX_SEL0					(1+GPIO_BT_DEBUG_IO) 
#define		GPIO_MUX_SEL1					(1+GPIO_MUX_SEL0) 
#define		GPIO_MUX_SEL2					(1+GPIO_MUX_SEL1) 
#define		GPIO_MUX_SEL3					(1+GPIO_MUX_SEL2) 
#define		GPIO_MUX_SEL4					(1+GPIO_MUX_SEL3) 

#define	    GPIO_A_INT_FLAG	                 GPIO_A_INT
#define	   	GPIO_B_INT_FLAG	                 GPIO_B_INT

/**
 * Define GPIO interrupt trigger type
 */
typedef enum __GPIO_TriggerType{
	GPIO_POS_EDGE_TRIGGER = 1,  /**<GPIO interrupt mode:posedge trigger, you can use it with function GPIO_INTEnable as the third para*/
	GPIO_NEG_EDGE_TRIGGER = 2,  /**<GPIO interrupt mode:negedge trigger, you can use it with function GPIO_INTEnable as the third para*/
	GPIO_EDGE_TRIGGER = 3,      /**<GPIO interrupt mode:edge trigger, you can use it with function GPIO_INTEnable as the third para*/
}GPIO_TriggerType;

/**
 * Define GPIO output Mask Type
 */
typedef enum __GPIO_OUTMaskType{
	GPIO_OUTMASK_CORE= (1<<0),
	GPIO_OUTMASK_DMA = (1<<1),

}GPIO_OUTMaskType;

typedef enum __GPIO_INDEX{
	GPIO_INDEX0 = (1<<0),                        /**<GPIO bit0 macro*/
	GPIO_INDEX1 = (1<<1),                        /**<GPIO bit1 macro*/
	GPIO_INDEX2 = (1<<2),                        /**<GPIO bit2 macro*/
	GPIO_INDEX3 = (1<<3),                        /**<GPIO bit3 macro*/
	GPIO_INDEX4 = (1<<4),                        /**<GPIO bit4 macro*/
	GPIO_INDEX5 = (1<<5),                        /**<GPIO bit5 macro*/
	GPIO_INDEX6 = (1<<6),                        /**<GPIO bit6 macro*/
	GPIO_INDEX7 = (1<<7),                        /**<GPIO bit7 macro*/
	GPIO_INDEX8 = (1<<8),                        /**<GPIO bit8 macro*/
	GPIO_INDEX9 = (1<<9),                        /**<GPIO bit9 macro*/
	GPIO_INDEX10 = (1<<10),                      /**<GPIO bit10 macro*/
	GPIO_INDEX11 = (1<<11),                      /**<GPIO bit11 macro*/
	GPIO_INDEX12 = (1<<12),                      /**<GPIO bit12 macro*/
	GPIO_INDEX13 = (1<<13),                      /**<GPIO bit13 macro*/
	GPIO_INDEX14 = (1<<14),                      /**<GPIO bit14 macro*/
	GPIO_INDEX15 = (1<<15),                      /**<GPIO bit15 macro*/
	GPIO_INDEX16 = (1<<16),                      /**<GPIO bit16 macro*/
	GPIO_INDEX17 = (1<<17),                      /**<GPIO bit17 macro*/
	GPIO_INDEX18 = (1<<18),                      /**<GPIO bit18 macro*/
	GPIO_INDEX19 = (1<<19),                      /**<GPIO bit19 macro*/
	GPIO_INDEX20 = (1<<20),                      /**<GPIO bit20 macro*/
	GPIO_INDEX21 = (1<<21),                      /**<GPIO bit21 macro*/
	GPIO_INDEX22 = (1<<22),                      /**<GPIO bit22 macro*/
	GPIO_INDEX23 = (1<<23),                      /**<GPIO bit23 macro*/
	GPIO_INDEX24 = (1<<24),                      /**<GPIO bit24 macro*/
	GPIO_INDEX25 = (1<<25),                      /**<GPIO bit25 macro*/
	GPIO_INDEX26 = (1<<26),                      /**<GPIO bit26 macro*/
	GPIO_INDEX27 = (1<<27),                      /**<GPIO bit27 macro*/
	GPIO_INDEX28 = (1<<28),                      /**<GPIO bit28 macro*/
	GPIO_INDEX29 = (1<<29),                      /**<GPIO bit29 macro*/
	GPIO_INDEX30 = (1<<30),                      /**<GPIO bit30 macro*/
	GPIO_INDEX31 = (1<<31),                      /**<GPIO bit31 macro*/
}GPIO_INDEX;

typedef enum __GPIO_TimerRemapIndex{
	GPIO_TimerRemapIndexA0 = 0,                        /**<GPIO Timer Remap IndexA0 macro*/
	GPIO_TimerRemapIndexA1 = 1,                        /**<GPIO Timer Remap IndexA1 macro*/
	GPIO_TimerRemapIndexA2 = 2,                        /**<GPIO Timer Remap IndexA2 macro*/
	GPIO_TimerRemapIndexA3 = 3,                        /**<GPIO Timer Remap IndexA3 macro*/
	GPIO_TimerRemapIndexA4 = 4,                        /**<GPIO Timer Remap IndexA4 macro*/
	GPIO_TimerRemapIndexA5 = 5,                        /**<GPIO Timer Remap IndexA5 macro*/
	GPIO_TimerRemapIndexA6 = 6,                        /**<GPIO Timer Remap IndexA6 macro*/
	GPIO_TimerRemapIndexA7 = 7,                        /**<GPIO Timer Remap IndexA7 macro*/
	GPIO_TimerRemapIndexA8 = 8,                        /**<GPIO Timer Remap IndexA8 macro*/
	GPIO_TimerRemapIndexA9 = 9,                        /**<GPIO Timer Remap IndexA9 macro*/
	GPIO_TimerRemapIndexA10 = 10,                      /**<GPIO Timer Remap IndexA10 macro*/
	GPIO_TimerRemapIndexA11 = 11,                      /**<GPIO Timer Remap IndexA11 macro*/
	GPIO_TimerRemapIndexA12 = 12,                      /**<GPIO Timer Remap IndexA12 macro*/
	GPIO_TimerRemapIndexA13 = 13,                      /**<GPIO Timer Remap IndexA13 macro*/
	GPIO_TimerRemapIndexA14 = 14,                      /**<GPIO Timer Remap IndexA14 macro*/
	GPIO_TimerRemapIndexA15 = 15,                      /**<GPIO Timer Remap IndexA15 macro*/
	GPIO_TimerRemapIndexA16 = 16,                      /**<GPIO Timer Remap IndexA16 macro*/
	
}GPIO_TimerRemapIndex;

typedef enum __GPIO_PULLDOWN_MODE{
	GPIO_PULLDOWN_CLOSE = 0,           /**<GPIO PortA PULLDOWN Mode:0/1.6/3.2/4.8mA (bit:A0~A31) macro*/
	GPIO_PULLDOWN_1MA6 = 1,			   /**<GPIO PortB PULLDOWN Mode:0/1.6/3.2/4.8mA (bit:b0~b9) macro*/
	GPIO_PULLDOWN_3MA2 = 2,
	GPIO_PULLDOWN_4MA8 = 3,
}GPIO_PULLDOWN_MODE;

/**
 * Define GPIO output Driving Strength
 */
typedef enum __GPIO_OUTDS_MODE{
	GPIO_OUTDS_3MA = 0,          /**<GPIO PortA digital output driving strength mode select:3/6/9/18mA(bit:A0~A31) macro*/
	GPIO_OUTDS_6MA = 1,		   	 /**<GPIO PortB digital output driving strength mode select:3/6/9/18mA(bit:B0~B27) macro*/
	GPIO_OUTDS_9MA = 2,
	GPIO_OUTDS_18MA = 3,
}GPIO_OUTDS_MODE;

typedef enum
{
	GPIOA0 = (1 << 0),	/** GpioMode
						0:gpio default register value
						1:uart0_rxd_0(i)
						10:i2c0_sda_0(io)
						11:dvpc_hsync_0(i)
						100:sw1_clk_0(i)
						101:btdm_rf_debug[0](io)
						110:uart0_txd_6(o)
						111:can0_txd_0(o)
						1000:tim5_pwm_0(o)
						1001:lcdc_cs(o)
						1010:clk0_out_0(o)
						1011:rf_txen_0(o)
						1100:otghs_debug[0](o)
						1101:mcu_deepsleep(o)
					    */
	GPIOA1 = (1 << 1),	/** GpioMode
						0:gpio default register value
						1:i2c0_scl_0(io)
						10:can0_rxd_0(i)
						11:dvpc_vsync_0(i)
						100:sw1_d_0(io)
						101:ir_in_0(i)
						110:btdm_rf_debug[1](io)
						111:uart0_txd_0(o)
						1000:tim6_pwm_0(o)
						1001:lcdc_dc(o)
						1010:clk1_out_0(o)
						1011:rf_rxen_0(o)
						1100:otghs_debug[1](o)
					    */
	GPIOA2 = (1 << 2),	/** GpioMode
						0:gpio default register value
						1:fshc1_wp(io)
						10:sd_dat3_0(io)
						11:spim0_io3_0(io)
						100:uart0_cts_0(i)
						101:i2c1_sda_0(io)
						110:btdm_rf_debug[2](io)
						111:uart0_txd_7(o)
						1000:tim7_pwm0_0(o)
						1001:otghs_debug[2](o)
					    */
	GPIOA3 = (1 << 3),	/** GpioMode
						0:gpio default register value
						1:sd_dat2_0(io)
						10:spim0_io2_0(io)
						11:i2c1_scl_0(io)
						100:btdm_rf_debug[3](io)
						101:fshc1_cs(o)
						110:uart0_rts_0(o)
						111:uart0_txd_8(o)
						1000:tim8_pwm0_0(o)
						1001:ddr_dq_test[0](o)
						1010:otghs_debug[3](o)
					    */
	GPIOA4 = (1 << 4),	/** GpioMode
						0:gpio default register value
						1:fshc1_miso(io)
						10:sd_dat1_0(io)
						11:spis0_cs_0(i)
						100:i2s0_din_0(i) //I2S单线模式
						101:i2s0_mclk_in_0(i)
						110:uart1_rxd_0(i)
						111:btdm_rf_debug[4](io)
						1000:spim0_cs_0(o)
						1001:i2s0_dout_1(o)
						1010:i2s0_mclk_out_0(o)
						1011:uart0_txd_9(o)
						1100:tim9_pwm_0(o)
						1101:ddr_dq_test[1](o)
						1110:otghs_debug[4](o)
					    */
	GPIOA5 = (1 << 5),	/** GpioMode
						0:gpio default register value
						1:fshc1_mosi(io)
						10:sd_dat0_0(io)
						11:spim0_io1_0(io)
						100:i2s0_din_1(i)
						101:btdm_rf_debug[5](io)
						110:spis0_miso_0(o)
						111:i2s0_dout_0(o) //I2S单线模式
						1000:dsd_l_0(o)
						1001:uart1_txd_0(o)
						1010:uart0_txd_10(o)
						1011:tim10_pwm_0(o)
						1100:ddr_dq_test[2](o)
						1101:otghs_debug[5](o)
					    */
	GPIOA6 = (1 << 6),	/** GpioMode
						0:gpio default register value
						1:spis0_clk_0(i)
						10:i2s0_lrck_0(io)
						11:i2s0_mclk_in_1(i)
						100:uart1_cts_0(i)
						101:i2c0_sda_1(io)
						110:btdm_rf_debug[6](io)
						111:fshc1_sck(o)
						1000:sd_clk_0(o)
						1001:spim0_clk_0(o)
						1010:i2s0_mclk_out_1(o)
						1011:dsd_r_0(o)
						1100:uart0_txd_11(o)
						1101:tim5_pwm_1(o)
						1110:ddr_dq_test[3](o)
						1111:otghs_debug[6](o)
					    */
	GPIOA7 = (1 << 7),	/** GpioMode
						0:gpio default register value
						1:fshc1_hold(io)
						10:sd_cmd_0(io)
						11:spim0_io0_0(io)
						100:spis0_mosi_0(i)
						101:i2s0_bclk_0(io)
						110:i2c0_scl_1(io)
						111:btdm_rf_debug[7](io)
						1000:dsd_dclk_0(o)
						1001:uart1_rts_0(o)
						1010:uart0_txd_12(o)
						1011:tim6_pwm_1(o)
						1100:ddr_csb_test(o)
						1101:otghs_debug[7](o)
					    */
	GPIOA8 = (1 << 8),	/** GpioMode
						0:gpio default register value
						1:uart2_rxd_0(i)
						10:uart0_rxd_14(i)
						11:btdm_rf_debug[8](io)
						100:uart0_txd_13(o)
						101:tim7_pwm0_1(o)
						110:fshc0_clk_test_1(o)
						111:fshc1_clk_test_1(o)
						1000:otghs_debug[8](o)
					    */
	GPIOA9 = (1 << 9),	/** GpioMode
						0:gpio default register value
						1:btdm_rf_debug[9](io)
						10:uart2_txd_0(o)
						11:uart0_txd_14(o)
						100:tim8_pwm0_1(o)
						101:fshc0_hold_test_1(o)
						110:fshc0_miso_test_1(o)
						111:fshc0_wp_test_1(o)
						1000:fshc0_mosi_test_1(o)
						1001:fshc0_cs_test_1(o)
						1010:fshc1_hold_test_1(o)
						1011:fshc1_miso_test_1(o)
						1100:fshc1_wp_test_1(o)
						1101:fshc1_mosi_test_1(o)
						1110:fshc1_cs_test_1(o)
						1111:otghs_debug[9](o)
					    */
	GPIOA10 = (1 << 10),/** GpioMode
						0:gpio default register value
						1:sd_dat0_1(io)
						10:spim1_io0_0(io)
						11:spis0_mosi_1(i)
						100:i2s1_bclk_0(i)/i2s0_bclk_1(o)
						101:i2s1_bclk_0(o)/i2s0_bclk_1(i)
						110:i2s1_bclk_0(i)/i2s0_bclk_1(i)
						111:i2s1_bclk_0(io)
						1000:i2s0_bclk_1(io)
						1001:uart0_rxd_1(i)
						1010:i2c1_sda_1(io)
						1011:keyscan_[0](io)
						1100:dvpc_pclk(i)
						1101:wlan_tx(i)
						1110:btdm_rf_debug[10](io)
						1111:phy_vpo(i)
						10000:dsd_dclk_1(o)
						10001:uart0_txd_15(o)
						10010:tim9_pwm_1(o)
						10011:ledc_clk(o)
						10100:lcdc_wr(o)
						10101:antsel_[0]_0(o)
						10110:rf_txen_1(o)
						10111:fshc1_clk_test_0(o)
						11000:ddr_clk_test(o)
						11001:otghs_debug[10](o)
						 */
	GPIOA11 = (1 << 11),/** GpioMode
						0:gpio default register value
						1:spis0_clk_1(i)
						10:i2s1_lrck_0(i)/i2s0_lrck_1(o)
						11:i2s1_lrck_0(o)/i2s0_lrck_1(i)
						100:i2s1_lrck_0(i)/i2s0_lrck_1(i)
						101:i2s1_lrck_0(io)
						110:i2s0_lrck_1(io)
						111:i2c1_scl_1(io)
						1000:keyscan_[1](io)
						1001:wlan_rx(i)
						1010:btdm_rf_debug[11](io)
						1011:phy_vmo(i)
						1100:sd_clk_1(o)
						1101:spim1_clk_0(o)
						1110:dsd_r_1(o)
						1111:uart0_txd_1(o)
						10000:tim10_pwm_1(o)
						10001:ledc_out[0](o)
						10010:dvpc_wclk(o)
						10011:lcdc_rd(o)
						10100:antsel_[1]_0(o)
						10101:rf_rxen_1(o)
						10110:fshc1_hold_test_0(o)
						10111:ddr_dq_test[4](o)
						11000:otghs_debug[11](o)
						 */
	GPIOA12 = (1 << 12),/** GpioMode
						0:gpio default register value
						1:sd_cmd_1(io)
						10:spim1_io1_0(io)
						11:i2s1_din0_0(i)	//I2S单线模式
						100:uart0_cts_1(i)
						101:keyscan_[2](io)
						110:dvpc_pdata[0](i)
						111:lcdc_data[0](io)
						1000:btdm_rf_debug[12](io)
						1001:phy_oen(i)
						1010:spis0_miso_1(o)
						1011:i2s1_dout0_0(o)//I2S单线模式
						1100:uart0_txd_16(o)
						1101:tim5_pwm_2(o)
						1110:ledc_out[1](o)
						1111:antsel_[2]_0(o)
						10000:wlan_dm_tx(o)
						10001:fshc1_miso_test_0(o)
						10010:ddr_dq_test[5](o)
						10011:otghs_debug[12](o)
						 */
	GPIOA13 = (1 << 13),/** GpioMode
						0:gpio default register value
						1:sd_dat1_1(io)
						10:spis0_cs_1(i)
						11:i2s1_din1_0(i)
						100:i2s1_din0_1(i)//I2S单线模式
						101:keyscan_[3](io)
						110:dvpc_pdata[1](i)
						111:lcdc_data[1](io)
						1000:ir_in_1(i)
						1001:btdm_rf_debug[13](io)
						1010:spim1_cs_0(o)
						1011:i2s1_dout1_0(o)
						1100:dsd_l_1(o)
						1101:uart0_rts_1(o)
						1110:uart0_txd_17(o)
						1111:tim6_pwm_2(o)
						10000:ledc_out[2](o)
						10001:clk0_out_1(o)
						10010:wlan_dm_rx(o)
						10011:fshc1_wp_test_0(o)
						10100:ddr_dq_test[6](o)
						10101:otghs_debug[13](o)
						10110:phy_vp(o)
						 */
	GPIOA14 = (1 << 14),/** GpioMode
						0:gpio default register value
						1:sd_dat2_1(io)
						10:spim1_io2_0(io)
						11:i2s1_din2_0(i)
						100:i2s1_din0_2(i)//I2S单线模式
						101:i2s0_mclk_in_2(i)
						110:uart1_rxd_1(i)
						111:i2c0_sda_2(io)
						1000:keyscan_[4](io)
						1001:dvpc_pdata[2](i)
						1010:lcdc_data[2](io)
						1011:sadc_soc(io)
						1100:btdm_rf_debug[14](io)
						1101:i2s1_dout2_0(o)
						1110:i2s0_mclk_out_2(o)
						1111:uart0_txd_18(o)
						10000:can1_txd_0(o)
						10001:tim7_pwm0_2(o)
						10010:ledc_out[3](o)
						10011:wlan_dm_sync(o)
						10100:fshc1_mosi_test_0(o)
						10101:ddr_dq_test[7](o)
						10110:otghs_debug[14](o)
						10111:phy_vm(o)
						 */
	GPIOA15 = (1 << 15),/** GpioMode
						0:gpio default register value
						1:sd_dat3_1(io)
						10:spim1_io3_0(io)
						11:i2s1_din3_0(i)
						100:i2s1_din1_2(i)
						101:i2c0_scl_2(io)
						110:can1_rxd_0(i)
						111:keyscan_[5](io)
						1000:dvpc_pdata[3](i)
						1001:lcdc_data[3](io)
						1010:sadc_clk(io)
						1011:btdm_rf_debug[15](io)
						1100:i2s1_dout3_0(o)
						1101:uart1_txd_1(o)
						1110:uart0_txd_19(o)
						1111:tim8_pwm0_2(o)
						10000:tim7_pwm1_2(o)
						10001:ledc_out[4](o)
						10010:wlan_dm_pti[3](o)
						10011:fshc1_cs_test_0(o)
						10100:ddr_dqs_test(o)
						10101:otghs_debug[15](o)
						10110:phy_rcv(o)
						 */
	GPIOA16 = (1 << 16),/** GpioMode
						0:gpio default register value
						1:DDR_DQ[0](io)
						10:i2s2_bclk_0(i)/i2s1_bclk_1(o)
						11:i2s2_bclk_0(o)/i2s1_bclk_1(i)
						100:i2s2_bclk_0(i)/i2s1_bclk_1(i)
						101:i2s2_bclk_0(io)
						110:i2s1_bclk_1(io)
						111:i2s1_din0_3(i)
						1000:uart1_cts_1(i)
						1001:keyscan_[6](io)
						1010:dvpc_pdata[4](i)
						1011:lcdc_data[4](io)
						1100:btdm_rf_debug[16](io)
						1101:phy_suspend(i)
						1110:uart0_txd_20(o)
						1111:can0_txd_1(o)
						10000:tim9_pwm_2(o)
						10001:tim7_pwm2_2(o)
						10010:ledc_out[5](o)
						10011:clk0_out_2(o)
						10100:rf_txen_2(o)
						10101:wlan_dm_pti[2](o)
						10110:sadc_test[0](o)
						10111:otghs_debug[16](o)
						 */
	GPIOA17 = (1 << 17),/** GpioMode
						0:gpio default register value
						1:DDR_DQ[1](io)
						10:i2s2_lrck_0(i)/i2s1_lrck_1(o)
						11:i2s2_lrck_0(o)/i2s1_lrck_1(i)
						100:i2s2_lrck_0(i)/i2s1_lrck_1(i)
						101:i2s2_lrck_0(io)
						110:i2s1_lrck_1(io)
						111:i2s1_din1_3(i)
						1000:can0_rxd_1(i)
						1001:keyscan_[7](io)
						1010:dvpc_pdata[5](i)
						1011:lcdc_data[5](io)
						1100:btdm_rf_debug[17](io)
						1101:uart1_rts_1(o)
						1110:uart0_txd_21(o)
						1111:tim10_pwm_2(o)
						10000:tim7_pwm3_2(o)
						10001:ledc_out[6](o)
						10010:rf_rxen_2(o)
						10011:wlan_dm_pti[1](o)
						10100:sadc_test[1](o)
						10101:otghs_debug[17](o)
						 */
	GPIOA18 = (1 << 18),/** GpioMode
						0:gpio default register value
						1:DDR_DQ[2](io)
						10:i2s1_din2_3(i)
						11:i2s2_din0_0(i)//I2S单线模式
						100:uart2_rxd_1(i)
						101:i2c1_sda_2(io)
						110:keyscan_[8](io)
						111:dvpc_pdata[6](i)
						1000:lcdc_data[6](io)
						1001:btdm_rf_debug[18](io)
						1010:i2s2_dout0_0(o)//I2S单线模式
						1011:uart0_txd_22(o)
						1100:tim5_pwm_3(o)
						1101:tim8_pwm3_3(o)
						1110:ledc_out[7](o)
						1111:wlan_dm_pti[0](o)
						10000:sadc_test[2](o)
						10001:otghs_debug[18](o)
						 */
	GPIOA19 = (1 << 19),/** GpioMode
						0:gpio default register value
						1:DDR_DQ[3](io)
						10:i2s1_din3_3(i)
						11:i2s2_din1_0(i)
						100:i2s2_din0_1(i)//I2S单线模式
						101:i2c1_scl_2(io)
						110:dvpc_pdata[7](i)
						111:lcdc_data[7](io)
						1000:btdm_rf_debug[19](io)
						1001:i2s2_dout1_0(o)
						1010:uart2_txd_1(o)
						1011:uart0_txd_23(o)
						1100:tim6_pwm_3(o)
						1101:tim8_pwm2_3(o)
						1110:wlan_nbt_ble(o)
						1111:sadc_test[3](o)
						10000:otghs_debug[19](o)
						 */
	GPIOA20 = (1 << 20),/** GpioMode
						0:gpio default register value
						1:i2s1_mclk_in_0(i)
						10:i2s2_din2_0(i)//I2S单线模式
						11:i2s2_din0_2(i)//I2S单线模式
						100:uart0_rxd_2(i)
						101:dvpc_vsync_1(i)
						110:lcdc_data[8](io)
						111:btdm_rf_debug[20](io)
						1000:DDR_CSB(o)
						1001:i2s1_mclk_out_0(o)
						1010:i2s2_dout2_0(o)
						1011:uart0_txd_24(o)
						1100:can1_txd_1(o)
						1101:tim7_pwm0_3(o)
						1110:tim8_pwm1_3(o)
						1111:wlan_dm_in_process(o)
						10000:sadc_test[4](o)
						 */
	GPIOA21 = (1 << 21),/** GpioMode
						0:gpio default register value
						1:i2s2_din3_0(i)
						10:i2s2_din1_2(i)
						11:can1_rxd_1(i)
						100:dvpc_hsync_1(i)
						101:lcdc_data[9](io)
						110:btdm_rf_debug[21](io)
						111:DDR_CLK(o)
						1000:i2s2_dout3_0(o)
						1001:uart0_txd_2(o)
						1010:tim8_pwm0_3(o)
						1011:sadc_test[5](o)
						 */
	GPIOA22 = (1 << 22),/** GpioMode
						0:gpio default register value
						1:DDR_DQ[4](io)
						10:uart0_cts_2(i)
						11:i2c0_sda_3(io)
						100:lcdc_data[10](io)
						101:btdm_rf_debug[22](io)
						110:uart0_txd_25(o)
						111:tim9_pwm_3(o)
						1000:sadc_test[6](o)
						 */
	GPIOA23 = (1 << 23),/** GpioMode
						0:gpio default register value
						1:DDR_DQ[5](io)
						10:i2c0_scl_3(io)
						11:lcdc_data[11](io)
						100:btdm_rf_debug[23](io)
						101:uart0_rts_2(o)
						110:uart0_txd_26(o)
						111:tim10_pwm_3(o)
						1000:sadc_test[7](o)
						 */
	GPIOA24 = (1 << 24),/** GpioMode
						0:gpio default register value
						1:DDR_DQ[6](io)
						10:uart1_rxd_2(i)
						11:lcdc_data[12](io)
						100:btdm_rf_debug[24](io)
						101:uart0_txd_27(o)
						110:tim5_pwm_4(o)
						111:sadc_test[8](o)
						 */
	GPIOA25 = (1 << 25),/** GpioMode
						0:gpio default register value
						1:DDR_DQ[7](io)
						10:i2c1_scl_3(io)
						11:lcdc_data[13](io)
						100:btdm_rf_debug[25](io)
						101:uart1_txd_2(o)
						110:uart0_txd_28(o)
						111:tim6_pwm_4(o)
						1000:sadc_test[9](o)
						 */
	GPIOA26 = (1 << 26),/** GpioMode
						0:gpio default register value
						1:DDR_DQS(io)
						10:uart1_cts_2(i)
						11:i2c1_sda_3(io)
						100:lcdc_data[14](io)
						101:btdm_rf_debug[26](io)
						110:uart0_txd_29(o)
						111:tim7_pwm0_4(o)
						1000:sadc_test[10](o)
						 */
	GPIOA27 = (1 << 27),/** GpioMode
						0:gpio default register value
						1:lcdc_data[15](io)
						10:btdm_rf_debug[27](io)
						11:DDR_RSTB(o)
						100:uart1_rts_2(o)
						101:uart0_txd_30(o)
						110:tim8_pwm0_4(o)
						111:sadc_test[11](o)
						 */
	GPIOA28 = (1 << 28),/** GpioMode
						0:gpio default register value
						1:uart2_rxd_2(i)
						10:btdm_rf_debug[28](io)
						11:uart0_txd_31(o)
						100:tim9_pwm_4(o)
						101:sadc_test[12](o)
						110:ddr_rstb_test(o)
						 */
	GPIOA29 = (1 << 29),/** GpioMode
						0:gpio default register value
						1:uart2_txd_2(o)
						10:uart0_txd_32(o)
						11:tim10_pwm_4(o)
						100:clk0_out_3(o)
						101:bist_end(o)
						 */
	GPIOA30 = (1 << 30),/** GpioMode
						0:gpio default register value
						1:uart0_rxd_3(i)
						10:i2c0_sda_4(io)
						11:uart0_txd_33(o)
						100:tim5_pwm_5(o)
						 */
	GPIOA31 = (1 << 31),/** GpioMode
						0:gpio default register value
						1:i2c0_scl_4(io)
						10:uart0_txd_3(o)
						11:tim6_pwm_5(o)
						 */
}GPIO_PortA;

typedef enum
{
	GPIOB0 = (1 << 0),	/** GpioMode
						0:gpio default register value
						1:sd_dat0_2(io)
						10:spim1_io0_1(io)
						11:spis1_mosi_0(i)
						100:i2s2_bclk_1(i)/i2s1_bclk_2(o)
						101:i2s2_bclk_1(o)/i2s1_bclk_2(i)
						110:i2s2_bclk_1(i)/i2s1_bclk_2(i)
						111:i2s2_bclk_1(io)
						1000:i2s1_bclk_2(io)
						1001:uart0_cts_3(i)
						1010:spdif0_di_0(i)
						1011:dmic0_dat_0(i)
						1100:mdac_test[0](io)
						1101:asdm_test[0](io)
						1110:btdm_rf_debug[29](io)
						1111:dsd_dclk_2(o)
						10000:uart0_txd_34(o)
						10001:can0_txd_2(o)
						10010:spdif0_do_0(o)
						10011:tim7_pwm0_5(o)
						10100:antsel_[0]_1(o)
						10101:rf_txen_3(o)
						10110:touch_test[0](o)
						10111:fshc0_clk_test(o)
						 */
	GPIOB1 = (1 << 1),	/** GpioMode
						0:gpio default register value
						1:spis1_clk_0(i)
						10:i2s2_lrck_1(i)/i2s1_lrck_2(o)
						11:i2s2_lrck_1(o)/i2s1_lrck_2(i)
						100:i2s2_lrck_1(i)/i2s1_lrck_2(i)
						101:i2s2_lrck_1(io)
						110:i2s1_lrck_2(io)
						111:can0_rxd_2(i)
						1000:spdif1_di_0(i)
						1001:mdac_test[1](io)
						1010:asdm_test[1](io)
						1011:btdm_rf_debug[30](io)
						1100:sd_clk_2(o)
						1101:spim1_clk_1(o)
						1110:dsd_r_2(o)
						1111:uart0_rts_3(o)
						10000:uart0_txd_35(o)
						10001:spdif1_do_0(o)
						10010:dmic0_clk_0(o)
						10011:tim8_pwm0_5(o)
						10100:antsel_[1]_1(o)
						10101:rf_rxen_3(o)
						10110:touch_test[1](o)
						10111:fshc0_hold_test(o)
						 */
	GPIOB2 = (1 << 2),	/** GpioMode
						0:gpio default register value
						1:sd_cmd_2(io)
						10:spim1_io1_1(io)
						11:i2s2_din0_3(i)//I2S单线模式
						100:uart1_rxd_3(i)
						101:i2c1_sda_4(io)
						110:spdif0_di_1(i)
						111:dmic1_dat_0(i)
						1000:mdac_test[2](io)
						1001:asdm_test[2](io)
						1010:btdm_rf_debug[31](io)
						1011:spis1_miso_0(o)
						1100:i2s2_dout0_3(o)//I2S单线模式
						1101:uart0_txd_36(o)
						1110:spdif0_do_1(o)
						1111:tim9_pwm_5(o)
						10000:antsel_[2]_1(o)
						10001:touch_test[2](o)
						10010:fshc0_miso_test(o)
						 */
	GPIOB3 = (1 << 3),	/** GpioMode
						0:gpio default register value
						1:sd_dat1_2(io)
						10:spis1_cs_0(i)
						11:i2s2_din1_3(i)
						100:i2s2_din0_4(i)//I2S单线模式
						101:i2c1_scl_4(io)
						110:spdif1_di_1(i)
						111:ir_in_2(i)
						1000:mdac_test[3](io)
						1001:asdm_test[3](io)
						1010:btdm_rf_debug[32](io)
						1011:spim1_cs_1(o)
						1100:i2s2_dout1_3(o)
						1101:dsd_l_2(o)
						1110:uart1_txd_3(o)
						1111:uart0_txd_37(o)
						10000:spdif1_do_1(o)
						10001:dmic1_clk_0(o)
						10010:tim10_pwm_5(o)
						10011:clk0_out_4(o)
						10100:touch_test[3](o)
						10101:fshc0_wp_test(o)
						 */
	GPIOB4 = (1 << 4),	/** GpioMode
						0:gpio default register value
						1:sd_dat2_2(io)
						10:spim1_io2_1(io)
						11:i2s2_din2_3(i)
						100:i2s2_din0_5(i)//I2S单线模式
						101:i2s1_mclk_in_1(i)
						110:uart1_cts_3(i)
						111:mdac_test[4](io)
						1000:asdm_test[4](io)
						1001:btdm_rf_debug[33](io)
						1010:i2s2_dout2_3(o)
						1011:i2s1_mclk_out_1(o)
						1100:uart0_txd_38(o)
						1101:can1_txd_2(o)
						1110:tim5_pwm_6(o)
						1111:touch_test[4](o)
						10000:fshc0_mosi_test(o)
						 */
	GPIOB5 = (1 << 5),	/** GpioMode
						0:gpio default register value
						1:sd_dat3_2(io)
						10:spim1_io3_1(io)
						11:i2s2_din3_3(i)
						100:i2s2_din1_5(i)
						101:can1_rxd_2(i)
						110:mdac_test[5](io)
						111:asdm_test[5](io)
						1000:btdm_rf_debug[34](io)
						1001:i2s2_dout3_3(o)
						1010:uart1_rts_3(o)
						1011:uart0_txd_39(o)
						1100:tim6_pwm_6(o)
						1101:touch_test[5](o)
						1110:fshc0_cs_test(o)
						 */
	GPIOB6 = (1 << 6),	/** GpioMode
						0:gpio default register value
						1:i2s2_din0_6(i)//I2S单线模式
						10:i2s1_bclk_3(io)
						11:uart2_rxd_3(i)
						100:i2c0_sda_5(io)
						101:mdac_test[6](io)
						110:asdm_test[6](io)
						111:btdm_rf_debug[35](io)
						1000:dsd_dclk_3(o)
						1001:uart0_txd_40(o)
						1010:tim7_pwm0_6(o)
						1011:touch_test[6](o)
	 	 	 	 	 	 */
	GPIOB7 = (1 << 7),	/** GpioMode
						0:gpio default register value
						1:i2s2_din1_6(i)
						10:i2s1_lrck_3(io)
						11:i2c0_scl_5(io)
						100:mdac_test[7](io)
						101:asdm_test[7](io)
						110:btdm_rf_debug[36](io)
						111:dsd_r_3(o)
						1000:uart2_txd_3(o)
						1001:uart0_txd_41(o)
						1010:tim8_pwm0_6(o)
						1011:tim7_pwm1_6(o)
						1100:touch_test[7](o)
						 */
	GPIOB8 = (1 << 8),	/** GpioMode
						0:gpio default register value
						1:i2s2_din2_6(i)
						10:i2s1_din0_4(i)//I2S单线模式
						11:uart0_rxd_4(i)
						100:mdac_test[8](io)
						101:asdm_test[8](io)
						110:btdm_rf_debug[37](io)
						111:i2s1_dout0_4(o)//I2S单线模式
						1000:uart0_txd_42(o)
						1001:tim9_pwm_6(o)
						1010:tim7_pwm2_6(o)
						1011:touch_test[8](o)
						 */
	GPIOB9 = (1 << 9),	/** GpioMode
						0:gpio default register value
						1:i2s2_din3_6(i)
						10:i2s1_din1_4(i)
						11:i2s1_din0_5(i)//I2S单线模式
						100:mdac_test[9](io)
						101:btdm_rf_debug[38](io)
						110:i2s1_dout1_4(o)
						111:dsd_l_3(o)
						1000:uart0_txd_4(o)
						1001:tim10_pwm_6(o)
						1010:tim7_pwm3_6(o)
						1011:touch_test[9](o)
						 */
	GPIOB10 = (1 << 10),/** GpioMode
						0:gpio default register value
						1:i2s1_mclk_in_2(i)
						10:i2s1_din2_4(i)
						11:i2s1_din0_6(i)//I2S单线模式
						100:uart0_cts_4(i)
						101:i2c1_sda_5(io)
						110:mdac_test[10](io)
						111:btdm_rf_debug[39](io)
						1000:i2s1_mclk_out_2(o)
						1001:i2s1_dout2_4(o)
						1010:uart0_txd_43(o)
						1011:tim5_pwm_7(o)
						1100:touch_test[10](o)
						 */
	GPIOB11 = (1 << 11),/** GpioMode
						0:gpio default register value
						1:i2s1_din3_4(i)
						10:i2s1_din1_6(i)
						11:i2c1_scl_5(io)
						100:mdac_test[11](io)
						101:i2s1_dout3_4(o)
						110:uart0_rts_4(o)
						111:uart0_txd_44(o)
						1000:tim6_pwm_7(o)
						1001:touch_test[11](o)
						 */
	GPIOB12 = (1 << 12),/** GpioMode
						0:gpio default register value
						1:uart1_rxd_4(i)
						10:mdac_test[12](io)
						11:uart0_txd_45(o)
						100:tim7_pwm0_7(o)
						101:clk0_out_5(o)
						110:touch_test[12](o)
						 */
	GPIOB13 = (1 << 13),/** GpioMode
						0:gpio default register value
						1:mdac_test[13](io)
						10:uart1_txd_4(o)
						11:uart0_txd_46(o)
						100:tim8_pwm0_7(o)
						101:touch_test[13](o)
						 */
	GPIOB14 = (1 << 14),/** GpioMode
						0:gpio default register value
						1:uart1_cts_4(i)
						10:i2c0_sda_6(io)
						11:mdac_test[14](io)
						100:uart0_txd_47(o)
						101:tim9_pwm_7(o)
						110:touch_test[14](o)
						 */
	GPIOB15 = (1 << 15),/** GpioMode
						0:gpio default register value
						1:i2c0_scl_6(io)
						10:mdac_test[15](io)
						11:uart1_rts_4(o)
						100:uart0_txd_48(o)
						101:tim10_pwm_7(o)
						110:touch_test[15](o)
						 */
	GPIOB16 = (1 << 16),/** GpioMode
						0:gpio default register value
						1:uart2_rxd_4(i)
						10:mdac_test[16](io)
						11:uart0_txd_49(o)
						100:tim5_pwm_8(o)
						 */
	GPIOB17 = (1 << 17),/** GpioMode
						0:gpio default register value
						1:mdac_test[17](io)
						10:uart2_txd_4(o)
						11:uart0_txd_50(o)
						100:tim6_pwm_8(o)
						 */
	GPIOB18 = (1 << 18),/** GpioMode
						0:gpio default register value
						1:uart0_rxd_5(i)
						10:i2c1_sda_6(io)
						11:dmic0_dat_1(i)
						100:dmic1_dat_2(i)
						101:mdac_test[18](io)
						110:uart0_txd_51(o)
						111:tim7_pwm0_8(o)
						1000:rf_txen_4(o)
						 */
	GPIOB19 = (1 << 19),/** GpioMode
						0:gpio default register value
						1:i2c1_scl_6(io)
						10:ir_in_3(i)
						11:uart0_txd_5(o)
						100:dmic0_clk_1(o)
						101:dmic1_clk_2(o)
						110:tim8_pwm0_8(o)
						111:clk0_out_6(o)
						1000:rf_rxen_4(o)
						 */
	GPIOB20 = (1 << 20),/** GpioMode
						0:gpio default register value
						1:uart2_rxd_5(i)
						10:dmic1_dat_1(i)
						11:dmic0_dat_2(i)
						100:sw0_clk_0(i)
						101:uart0_txd_52(o)
						110:tim9_pwm_8(o)
						 */
	GPIOB21 = (1 << 21),/** GpioMode
						0:gpio default register value
						1:sw0_d_0(io)
						10:uart2_txd_5(o)
						11:uart0_txd_53(o)
						100:dmic1_clk_1(o)
						101:dmic0_clk_2(o)
						110:tim10_pwm_8(o)
						 */
	GPIOB22 = (1 << 22),/** GpioMode
						0:gpio default register value
						1:fshc0_cs(o)
						 */
	GPIOB23 = (1 << 23),/** GpioMode
						0:gpio default register value
						1:fshc0_miso(io)
						 */
	GPIOB24 = (1 << 24),/** GpioMode
						0:gpio default register value
						1:fshc0_mosi(io)
						 */
	GPIOB25 = (1 << 25),/** GpioMode
						0:gpio default register value
						1:fshc0_sck(o)
						 */
	GPIOB26 = (1 << 26),/** GpioMode
						0:gpio default register value
						1:fshc0_wp(io)
						 */
	GPIOB27 = (1 << 27),/** GpioMode
						0:gpio default register value
						1:fshc0_hold(io)
						 */
}GPIO_PortB;

/**************************************************************************************************
                                 通用GPIO设置或获取状态部分
**************************************************************************************************/

//************************* 1、 只设置或读取 1 bit  *****************************//

/**
 * @brief      只置位 1bit
 *
 * @param[in]  RegIndex: GPIO地址索引，例如： GPIO_A_OUT
 * @param[in]  GpioIndex：GPIO bit位，例如：GPIO_INDEX1
 *
 * @return     无
 */
void GPIO_RegOneBitSet(uint32_t	RegIndex, GPIO_INDEX GpioIndex);

/**
 * @brief      只清零 1bit
 *
 * @param[in]  RegIndex: GPIO地址索引，例如： GPIO_A_OUT
 * @param[in]  GpioIndex：GPIO bit位，例如：GPIO_INDEX1
 *
 * @return     无
 */
void GPIO_RegOneBitClear(uint8_t RegIndex, GPIO_INDEX GpioIndex);

/**
 * @brief      读取 1bit数据
 *
 * @param[in]  RegIndex: GPIO地址索引，例如： GPIO_A_IN
 * @param[in]  GpioIndex：GPIO bit位，例如：GPIO_INDEX1
 *
 * @return     1 or 0
 */
bool GPIO_RegOneBitGet(uint8_t RegIndex, GPIO_INDEX GpioIndex);


//************************* 2、 设置或读取多 bit  ***************************** //

/**
 * @brief      置位多 bits
 *
 * @param[in]  RegIndex: GPIO地址索引，例如： GPIO_A_OUT
 * @param[in]  mask：GPIO bit位，例如：GPIO_INDEX1
 *
 * @return     无
 */
void GPIO_RegBitsSet(uint8_t RegIndex, GPIO_INDEX GpioIndex);

/**
 * @brief      清除多 bits
 *
 * @param[in]  RegIndex: GPIO地址索引，例如： GPIO_A_OUT
 * @param[in]  GpioIndex bit位，例如：GPIO_INDEX1
 *
 * @return     无
 */
void GPIO_RegBitsClear(uint8_t RegIndex, GPIO_INDEX GpioIndex);


//************************* 3、 设置或读取一组  ***************************** //

/**
 * @brief      设置一组GPIO数据
 *
 * @param[in]  RegIndex: GPIO地址索引，例如： GPIO_A_OUT
 * @param[in]  mask：GPIO bit位，例如：GPIO_INDEX1 | GPIO_INDEX2
 *
 * @return     无
 */
void GPIO_RegSet(uint8_t RegIndex, GPIO_INDEX GpioIndex);

/**
 * @brief      读取一组GPIO数据
 *
 * @param[in]  RegIndex: GPIO地址索引，例如： GPIO_A_OUT
 *
 * @return     GPIO值
 */
uint32_t GPIO_RegGet(uint8_t RegIndex);


//*************************** 4、  GPIO中断  ******************************* //

/**
 * @brief      使能GPIO某一bit的中断功能
 *
 * @param[in]  RegIndex: GPIO中断地址索引，例如： GPIO_A_INT
 * @param[in]  GpioIndex: bit位，例如GPIO_INDEX1
 * @param[in]  TrigType: 触发极性，例如GPIO_NEG_EDGE_TRIGGER触发
 *
 * @return     无
 */
void GPIO_INTEnable(uint8_t RegIndex, GPIO_INDEX GpioIndex, GPIO_TriggerType TrigType);

/**
 * @brief      禁能GPIO某一bit的中断功能
 *
 * @param[in]  RegIndex: GPIO中断地址索引，例如： GPIO_A_INT
 * @param[in]  GpioIndex: bit位，例如GPIO_INDEX1
 *
 * @return     无
 */
void GPIO_INTDisable(uint32_t RegIndex, GPIO_INDEX GpioIndex);

/**
 * @brief      获取GPIO的中断状态
 *
 * @param[in]  RegIndex: GPIO中断地址索引，例如： GPIO_A_INT_FLAG
 *
 * @return     一组GPIO的中断状态
 */
uint32_t GPIO_INTFlagGet(uint32_t RegIndex);

/**
 * @brief      清除某一位GPIO的中断状态
 *
 * @param[in]  RegIndex: GPIO中断地址索引，例如： GPIO_A_INT_FLAG
 * @param[in]  GpioIndex: GPIO bit位,例如GPIO_INDEX1
 * @return     无
 */
void GPIO_INTFlagClear(uint32_t RegIndex, GPIO_INDEX GpioIndex);

//*************************** 5、  GPIO bit mask  ******************************* //
/**
 * @brief    GPIO output mask bit,  take affect when CPU/DMA access A_REG_O/B_REG_O register
 *
 * @param[in]  RegIndex: GPIO中断地址索引，例如： GPIO_A_INT_FLAG
 * @param[in]  OutMask: GPIO_OUTMASK_CORE(take affect when CPU access A_REG_O/B_REG_O register)
 *                      GPIO_OUTMASK_DMA(take affect when DMA access A_REG_O/B_REG_O register)
 * @param[in]  GpioIndex: GPIO bit位,例如GPIO_INDEX1
 *
 * @return     无
 */
bool GPIO_OutMaskSet(uint8_t RegIndex, GPIO_OUTMaskType OutMask, GPIO_INDEX MaskBits);

/**
 * @brief    Reg GPIO output mask clear
 *
 * @param[in]  RegIndex: GPIO中断地址索引，例如： GPIO_A_INT_FLAG
 * @param[in]  OutMask: GPIO_OUTMASK_CORE(take affect when CPU access A_REG_O/B_REG_O register)
 *                      GPIO_OUTMASK_DMA(take affect when DMA access A_REG_O/B_REG_O register)
 *
 * @return     无
 */
bool GPIO_OutMaskClear(uint8_t RegIndex, GPIO_OUTMaskType OutMask);

/**************************************************************************************************
                                 GPIO与其他功能的复用部分
**************************************************************************************************/

/**
 * @brief      设置GPIOA端口复用关系
 *
 * @param[in]  port: GPIO port，例如： GPIOA10
 * @param[in]  GpioMode: GPIO 模式， 0 代表使用为普通GPIO口，其他值参考 GPIO_PortA 注释。
 *
 * @return     无
 */
void GPIO_PortAModeSet(GPIO_PortA Port, uint8_t GpioMode);

/**
 * @brief      设置GPIOB端口复用关系
 *
 * @param[in]  port: GPIO port，例如： GPIOB0
 * @param[in]  GpioMode: GPIO 模式， 0 代表使用为普通GPIO口，其他值参考 GPIO_PortB 注释。
 *
 * @return     无
 */
void GPIO_PortBModeSet(GPIO_PortB Port, uint8_t GpioMode);

/**
 * @brief      获取GPIO模式
 *
 * @param[in]  port: GPIO A port，例如： GPIOA10
 *
 * @return     GpioMode: GPIO 模式， 0 代表使用为普通GPIO口，255 表示模拟应用，其他值参考 GpioPort 注释。
 */
uint8_t GPIO_PortAModeGet(GPIO_PortA Port);

/**
 * @brief      获取GPIO模式
 *
 * @param[in]  port: GPIO B port，例如： GPIOB1
 *
 * @return     GpioMode: GPIO 模式， 0 代表使用为普通GPIO口，其他值参考 GpioPort 注释。
 */
uint8_t GPIO_PortBModeGet(GPIO_PortB Port);

/**
 * @brief      gpio pwc input select
 *
 * @param[in]  TimerSel: Timer port select  [Timer3~Timer6](TIMER_INDEX枚举类型)
 * @param[in]  GpioTimerRemapIndex: (GPIO_TimerRemapIndex枚举类型)
 *
 * @return     TRUE:设置成功
 *             FALSE:设置失败
 * @note       all gpio can be PWC input and should enable IE register witch bit is the selected gpio by software
 */
bool GPIO_TimerRemapConfig(TIMER_INDEX TimerSel, GPIO_TimerRemapIndex GpioTimerRemapIndex);


//*************************   GPIO下拉电流源设置  ***************************** //
/**
 * @brief      获取GPIO模式
 *
 * @param[in]  Port: GPIO A port，例如： GPIOA1
 *
 * @param[in]  PullDownModeSel: 下拉电流源的 模式：
 *                                   GPIO_PortA_PULLDOWN_Close: 0mA(bit:A0~A31);
 * @return     TRUE:设置成功
 *             FALSE:设置失败
 */
bool GPIO_PortAPulldownSet(GPIO_PortA Port, GPIO_PULLDOWN_MODE PulldownModeSel);

/**
 * @brief      获取GPIO模式
 *
 * @param[in]  Port: GPIO B port，例如： GPIOB1
 *
 * @param[in]  PullDownModeSel: 下拉电流源的 模式：
 *                                   GPIO_PortB_PULLDOWN_Close: 0mA(bit:B0~B27);
 * @return     TRUE:设置成功
 *             FALSE:设置失败
 */
bool GPIO_PortBPulldownSet(GPIO_PortB Port, GPIO_PULLDOWN_MODE PulldownModeSel);

//*************************   GPIO输出驱动能力设置  ***************************** //
/**
 * @brief      获取GPIO模式
 *
 * @param[in]  PortAGpioIndex: 例如： GPIO_INDEX1(即为GPIO_A1)
 *
 * @param[in]  PortAOutDsModeSel: 输出驱动能力的模式选择：
 *                                   GPIO_OUTDS_3MA : 3mA;
 *                                   GPIO_OUTDS_6MA : 6mA;
 *                                   GPIO_OUTDS_9MA : 9mA;
 *                                   GPIO_OUTDS_18MA : 18mA;
 *
 * @return     TRUE:设置成功
 *             FALSE:设置失败
 */
bool GPIO_PortAOutDsSet(GPIO_INDEX PortAGpioIndex, GPIO_OUTDS_MODE PortAOutDsModeSel);

/**
 * @brief      获取GPIO模式
 *
 * @param[in]  PortBGpioIndex: GPIO_INDEX1(即为GPIO_B1)
 *
 * @param[in]  GPIO_OUTDS_MODE: 输出电流的驱动能力：
 *                                   GPIO_OUTDS_3MA : 3mA;
 *                                   GPIO_OUTDS_6MA : 6mA;
 *                                   GPIO_OUTDS_9MA : 9mA;
 *                                   GPIO_OUTDS_18MA : 18mA;
 * @return     TRUE:设置成功
 *             FALSE:设置失败
 */
bool GPIO_PortBOutDsSet(GPIO_INDEX PortBGpioIndex, GPIO_OUTDS_MODE PortBOutDsModeSel);

/**
 * @brief      Avoid unwanted reset source to reset gpio register and only reset gpio register at power on reset
 *
 * @param[in]  NONE
 *
 * @return     NONE
 */
void GPIO_RegisterResetMask(void);


#ifdef __cplusplus
}
#endif // __cplusplus


#endif//__GPIO_H__

/**
 * @}
 * @}
 */
 