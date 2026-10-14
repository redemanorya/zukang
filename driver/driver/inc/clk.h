/**
 *******************************************************************************
 * @file    clk.h
 * @brief	Clock driver interface
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2015-11-05 10:46:11$
 *
 * @Copyright (C) 2015, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 ******************************************************************************* 
 */
 
 
/**
 * @addtogroup CLOCK
 * @{
 * @defgroup clk clk.h
 * @{
 */
 
#ifndef __CLK_H__
#define __CLK_H__
 
#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#include "spi_flash.h"
/**
 * CLK module switch macro define
 */

typedef enum __CLOCK_PLL_MODULE1_SWITCH
{
	FSHC0_PLL_CLK_EN = (1 << 0),	/**<flash0 control pll module clk switch */
	FSHC1_PLL_CLK_EN = (1 << 1),	/**<flash1 control pll module clk switch */
	USB_UART_PLL_CLK_EN = (1 << 2),	/**<USB/UART PLL module clk switch */
	USB_UART_APLL_CLK_EN = (1 << 3),/**<USB/UART APLL module clk switch */
	USB_UART_DPLL1_CLK_EN =(1 << 4),/**<USB/UART DPLL1 module clk switch */
	MCLK_PLL_CLK0_EN = (1 << 5),	/**<MCLK PLL clock port0 module clk switch */
	MCLK0_APLL_CLK_EN = (1 << 6),   /**<MCLK0 APLL clock module clk switch */
	MCLK0_DPLL0_CLK_EN = (1 << 7),	/**<MCLK0 DPLL0 clock module clk switch */
	MCLK_PLL_CLK1_EN = (1 << 8),    /**<MCLK PLL clock port1 module clk switch */
	MCLK1_APLL_CLK_EN = (1 << 9),	/**<MCLK1 APLL clock module clk switch */
	MCLK1_DPLL0_CLK_EN = (1 << 10),	/**<MCLK1 DPLL0 clock module clk switch */
	SPDIF0_PLL_CLK_EN = (1 << 11),	/**<SPDIF0 PLL module clk switch */
	SPDIF0_APLL_CLK_EN = (1 << 12),	/**<SPDIF0 APLL module clk switch */
	SPDIF0_DPLL0_CLK_EN = (1 << 13),/**<SPDIF0 DPLL0 module clk switch */
	SPDIF1_PLL_CLK_EN = (1 << 14),  /**<SPDIF1 PLL module clk switch */
	SPDIF1_APLL_CLK_EN = (1 << 15), /**<SPDIF1 APLL module clk switch */
	SPDIF1_DPLL0_CLK_EN = (1 << 16),/**<SPDIF1 DPLL0 module clk switch */
	MDM_PLL_CLK_EN = (1 << 17),     /**<HPMDM PLL module clk switch */
	DVPC_PLL_CLK_EN = (1 << 18),    /**<DVPC PLL module clk switch */
	DVPC_APLL_CLK_EN = (1 << 19),   /**<DVPC APLL module clk switch */
	DVPC_DPLL0_CLK_EN = (1 << 20),  /**<DVPC DPLL0 module clk switch */
	SRC0_PLL_CLK_EN = (1 << 21),    /**<SRC port0 PLL module clk switch */
	SRC0_APLL_CLK_EN = (1 << 22),   /**<SRC port0 APLL module clk switch */
	SRC0_DPLL0_CLK_EN = (1 << 23),  /**<SRC port0 DPLL0 module clk switch */
	SRC1_PLL_CLK_EN = (1 << 24),    /**<SRC port1 PLL module clk switch */
	SRC1_APLL_CLK_EN = (1 << 25),   /**<SRC port1 APLL module clk switch */
	SRC1_DPLL0_CLK_EN = (1 << 26),  /**<SRC port1 DPLL0 module clk switch */
	SRC2_PLL_CLK_EN = (1 << 27),    /**<SRC port2 PLL module clk switch */
	SRC2_APLL_CLK_EN = (1 << 28),   /**<SRC port2 APLL module clk switch */
	SRC2_DPLL0_CLK_EN = (1 << 29),  /**<SRC port2 DPLL0 module clk switch */

	ALL_MODULE1_CLK_PLL_SWITCH = (0x3FFFFFFF),/**<all module clk SWITCH*/
} CLOCK_PLL_MODULE1_SWITCH;

typedef enum __CLOCK_PLL_MODULE2_SWITCH
{
	SRC0_REFO_PLL_CLK_EN = (1 << 0),	/**<SRC0 refo clk module clk switch */
	SRC1_REFO_PLL_CLK_EN = (1 << 1),	/**<SRC1 refo clk module clk switch */
	SRC2_REFO_PLL_CLK_EN = (1 << 2),	/**<SRC2 refo clk module clk switch */

	ALL_MODULE2_CLK_PLL_SWITCH = (0x7),/**<all module clk SWITCH*/
} CLOCK_PLL_MODULE2_SWITCH;

typedef enum __CLOCK_MODULE1_SWITCH
{
	APLL_CLK_EN = (1 << 0),			/**<APLL module clk switch */
	DPLL0_CLK_EN = (1 << 1),		/**<DPLL0 module clk switch */
	DPLL1_CLK_EN = (1 << 2),		/**<DPLL1 module clk switch */
	HRC_CLK_EN = (1 << 3),  		/**<RC12M module clk switch */
	HOSC_CLK_EN = (1 << 4),			/**<HOSC(24M) module clk switch */
	FPU_CLK_EN = (1 << 5),			/**<FPU module clk switch */
	FSHC0_CLK_EN = (1 << 6),       	/**<Flash Control module clk switch */
	CORE2_MCU_CLK_EN = (1 << 7),	/**<Core2 MCU clock module clk switch */
	CORE2_SYS_CLK_EN = (1 << 8),    /**<Core2 system clock module clk switch */
	FPU2_CLK_EN = (1 << 9),			/**<FPU2 module clk switch */
	FSHC1_CLK_EN = (1 << 10),		/**<Flash Control1 module clk switch */
	BT32K_CLK_EN = (1 << 11),		/**<BT 32K module clk switch */

	ALL_MODULE1_CLK_SWITCH = (0xFFF),/**<all module clk SWITCH*/
} CLOCK_MODULE1_SWITCH;

typedef enum __CLOCK_MODULE2_SWITCH
{
	AUMIXER_CLK_EN = (1 << 0),		/**<Audio mixer module clk switch */
	AUFIFO_REG_CLK_EN = (1 << 1),	/**<AUFIFO module clk switch */
	ASDM0_CLK_EN = (1 << 2),		/**<Audio ADC0 module clk switch */
	ASDM1_CLK_EN = (1 << 3),  		/**<Audio ADC1 module clk switch */
	MDAC0_CLK_EN = (1 << 4),		/**<Audio DAC0 module clk switch */
	MDAC1_CLK_EN = (1 << 5),		/**<Audio DAC1 port0 module clk switch */
	MDAC2_CLK_EN = (1 << 6),        /**<Audio DAC2 port1 module clk switch */
	MDAC0_REG_CLK_EN = (1 << 7),	/**<Audio DAC0 Reg module clk switch */
	MDAC1_REG_CLK_EN = (1 << 8),    /**<Audio DAC1 Reg module clk switch */
	MDAC2_REG_CLK_EN = (1 << 9),	/**<Audio DAC2 Reg module clk switch */
	I2S0_CLK_EN = (1 << 10),		/**<I2S port0 module clk switch */
	I2S1_CLK_EN = (1 << 11),		/**<I2S port1 module clk switch */
	I2S2_CLK_EN = (1 << 12),		/**<I2S port2 module clk switch */
	I2S0_REG_CLK_EN = (1 << 13),	/**<I2S port0 Reg module clk switch */
	I2S1_REG_CLK_EN = (1 << 14),    /**<I2S port1 Reg module clk switch */
	I2S2_REG_CLK_EN = (1 << 15),    /**<I2S port2 Reg module clk switch */
	SPDIF0_CLK_EN = (1 << 16),      /**<SPDIF port0 module clk switch */
	SPDIF1_CLK_EN = (1 << 17),   	/**<SPDIF port1 module clk switch */
	SPDIF0_REG_CLK_EN = (1 << 18),  /**<SPDIF port0 Reg module clk switch */
	SPDIF1_REG_CLK_EN = (1 << 19),  /**<SPDIF port1 Reg module clk switch */
	SRC0_FUNC_CLK_EN = (1 << 20),	/**<SRC port0 module clk switch */
	SRC1_FUNC_CLK_EN = (1 << 21),   /**<SRC port1 module clk switch */
	SRC2_FUNC_CLK_EN = (1 << 22),   /**<SRC port2 module clk switch */
	SRC0_REG_CLK_EN = (1 << 23),    /**<SRC port0 Reg module clk switch */
	SRC1_REG_CLK_EN = (1 << 24),    /**<SRC port1 Reg module clk switch */
	SRC2_REG_CLK_EN = (1 << 25),    /**<SRC port2 Reg module clk switch */
	DSD_FUNC_CLK_EN = (1 << 26),    /**<DSD module clk switch */
	DSD_REG_CLK_EN = (1 << 27),     /**<DSD Reg module clk switch */
	ASDM_REG_CLK_EN = (1 << 28),    /**<Audio ADC Reg module clk switch */
	AUMIXER_REG_CLK_EN = (1 << 29), /**<Audio mixer Reg module clk switch */

	ALL_MODULE2_CLK_SWITCH = (0x3FFFFFFF),/**<all module clk SWITCH*/
} CLOCK_MODULE2_SWITCH;

typedef enum __CLOCK_MODULE3_SWITCH
{
	PMU_RC32K_CLK_EN = (1 << 0),	/**< PMU rc32k module clk switch */
	HOSCCNT_32K_OB_CLK_EN = (1 << 1),/**< module clk switch */
	BTDM_HCLK_EN = (1 << 2),		/**< module clk switch */
	BTDM_24M_CLK_EN = (1 << 3),		/**< module clk switch */
	BTDM_LP_CLK_EN = (1 << 4),		/**< module clk switch */
	MDM_12M_CLK_EN = (1 << 5),		/**< module clk switch */
	MDM_24M_CLK_EN = (1 << 6),		/**< module clk switch */
	MDM_48M_CLK_EN = (1 << 7),		/**< module clk switch */
	DMAC0_CLK_EN = (1 << 8),		/**<DMA control0 module clk switch */
	DMAC1_CLK_EN = (1 << 9),		/**<DMA control1 module clk switch */
	DMAC2_CLK_EN = (1 << 10),		/**<DMA control2 module clk switch */
	DPROD_S8_CLK_EN = (1 << 11),	/**< module clk switch */
	DPROD_S8_REG_CLK_EN = (1 << 12),/**< module clk switch */
	EFUSE_CLK_EN = (1 << 13),		/**<Efuse module clk switch */
	FFT_CLK_EN = (1 << 14),			/**<FFT module clk switch */
	MTIMER_CLK_EN = (1 << 15),		/**<Platform machine timer module clk switch */
	TIMER1_CLK_EN = (1 << 16),		/**<TIMER1 module clk switch */
	TIMER2_CLK_EN = (1 << 17),		/**<TIMER2 module clk switch */
	TIMER3_CLK_EN = (1 << 18),		/**<TIMER3 module clk switch */
	TIMER4_CLK_EN = (1 << 19),		/**<TIMER4 module clk switch */
	TIMER5_CLK_EN = (1 << 20),		/**<TIMER5 module clk switch */
	TIMER6_CLK_EN = (1 << 21),		/**<TIMER6 module clk switch */
	TIMER7_CLK_EN = (1 << 22),		/**<TIMER7 module clk switch */
	TIMER8_CLK_EN = (1 << 23),		/**<TIMER8 module clk switch */
	HSEM_REG_CLK_EN = (1 << 24),	/**< module clk switch */
	TIMER9_CLK_EN = (1 << 25),		/**<TIMER9 module clk switch */
	TIMER10_CLK_EN = (1 << 26),		/**<TIMER10 module clk switch */
	DCDC16_CLK_EN = (1 << 27),		/**< module clk switch */
	IPC_REG_CLK_EN = (1 << 28),		/**< module clk switch */

	ALL_MODULE3_CLK_SWITCH = (0x1FFFFFFF),/**<all module clk SWITCH*/
} CLOCK_MODULE3_SWITCH;

typedef enum __CLOCK_MODULE4_SWITCH
{
	CAN0_24M_CLK_EN = (1 << 0),		/**<Can port0 module clk switch */
	CAN1_24M_CLK_EN = (1 << 1),		/**<Can port1 module clk switch */
	DVPC_CLK_EN = (1 << 2),			/**< module clk switch */
	DVPC_REG_CLK_EN = (1 << 3),		/**< module clk switch */
	I2C0_CLK_EN = (1 << 4),			/**<I2C port0 module clk switch */
	I2C1_CLK_EN = (1 << 5),			/**<I2C port1 module clk switch */
	KS_CLK_EN = (1 << 6),			/**< module clk switch */
	KS_REG_CLK_EN = (1 << 7),		/**< module clk switch */
	LCDC_REG_CLK_EN = (1 << 8),		/**< module clk switch */
	SADC_CLK_EN = (1 << 9),			/**<Sar ADC module clk switch */
	SDIO_CLK_EN = (1 << 10),		/**<SDIO module clk switch */
	SDIO_REG_CLK_EN = (1 << 11),	/**<SDIO reg module clk switch */
	SPIM0_CLK_EN = (1 << 12),		/**<SPI master port0 module clk switch */
	SPIM0_REG_CLK_EN = (1 << 13),	/**<SPI master port0 Reg module clk switch */
	SPIM1_CLK_EN = (1 << 14),		/**<SPI master port1 module clk switch */
	SPIM1_REG_CLK_EN = (1 << 15),	/**<SPI master port1 Reg module clk switch */
	SPIS0_REG_CLK_EN = (1 << 16),	/**<SPI slave port0 module clk switch */
	SPIS1_REG_CLK_EN = (1 << 17),	/**<SPI slave port1 module clk switch */
	UART0_CLK_EN = (1 << 18),		/**<UART port0 module clk switch */
	UART0_REG_CLK_EN = (1 << 19),	/**<UART port0 Reg module clk switch */
	UART1_CLK_EN = (1 << 20),		/**<UART port1 module clk switch */
	UART1_REG_CLK_EN = (1 << 21),	/**<UART port1 Reg module clk switch */
	UART2_CLK_EN = (1 << 22),		/**<UART port2 module clk switch */
	UART2_REG_CLK_EN = (1 << 23),	/**<UART port2 Reg module clk switch */
	USB_CLK_EN = (1 << 24),			/**<USB module clk switch */
	USB_REG_CLK_EN = (1 << 25),		/**<USB Reg module clk switch */
	USB_HS_CLK_EN = (1 << 26),		/**<USB High speed module clk switch */
	USB_HS_REG_CLK_EN = (1 << 27),	/**<USB High speed Reg module clk switch */
	DPSRC_REG_CLK_EN = (1 << 28),	/**< module clk switch */
	DPSRC_CLK_EN = (1 << 29),		/**< module clk switch */

	ALL_MODULE4_CLK_SWITCH = (0x3FFFFFFF),/**<all module clk SWITCH*/
} CLOCK_MODULE4_SWITCH;

typedef enum __DPLL_REF_CLK_SRC_SEL{
	DPLL_REF_CLK_SRC_SPDIF0_BLOCK_FLAG = 0x02,	/**<DPLL参考时钟选择: SPDIF0 block flag */
	DPLL_REF_CLK_SRC_SPDIF1_BLOCK_FLAG = 0x04,	/**<DPLL参考时钟选择: SPDIF1 block flag */
	DPLL_REF_CLK_SRC_GPIO_I2S0_BCLK = 0x08,		/**<DPLL参考时钟选择: GPIO I2S0 BCLK组, 建议固定使用组起始值 */
	DPLL_REF_CLK_SRC_GPIO_I2S1_BCLK = 0x10,		/**<DPLL参考时钟选择: GPIO I2S1 BCLK组, 建议固定使用组起始值 */
	DPLL_REF_CLK_SRC_GPIO_I2S2_BCLK = 0x20,		/**<DPLL参考时钟选择: GPIO I2S2 BCLK组, 建议固定使用组起始值 */
}DPLL_REF_CLK_SRC_SEL;

typedef enum __CLK_DPLL_MODULE{
	DPLL0_MODULE,	/*DPLL0 */
	DPLL1_MODULE,	/*DPLL1 */
}CLK_DPLL_MODULE;

typedef enum __CLK_MODE{
	RC_CLK_MODE,	/*RC 12M source*/
	DPLL0_CLK_MODE,	/*DPLL0 source,For audio use only*/
	DPLL1_CLK_MODE,	/*DPLL1 source*/
	APLL_CLK_MODE,	/*APLL source*/
	SYSTEM_CLK_MODE,/*System Clock*/
	HOSC_CLK_MODE /*HOSC Clock 24M*/
}CLK_MODE;

typedef enum _CLK_32K_MODE{
	HOSC_DIV_32K_CLK_MODE,	/*HOSC DIV or RC 12M div source*/
	BT_RC_CLK32_MODE,		/*RC32K source(High precision)*/
	PMU_RC_32K_MODE,		/*RC32K*/
	LOSC_32K_MODE,			/*LOSC 32K source*/
}CLK_32K_MODE;

typedef enum __PLL_CLK_INDEX{
	PLL_CLK_1 = 0,	/**PLL分频时钟器1,软件配置生成为11.2896*2M， 软件可以修改*/
	PLL_CLK_2,		/**PLL分频时钟器2,软件配置生成为12.288*2M，  软件可以修改*/
	PLL_CLK_3,		/**PLL分频时钟器3,软件配置生成为11.2896*2M， 软件可以修改*,使用SRC0 div*/
	PLL_CLK_4,		/**PLL分频时钟器4,软件配置生成为12.288*2M，  软件可以修改*,使用SRC1 div*/
	PLL_CLK_5,		/**PLL分频时钟器5,软件配置生成为DPLL0当前动态频率（11.2896&12.288*2M），  软件可以修改*,使用SRC1 div*/
}PLL_CLK_INDEX;

typedef enum __MCLK_CLK_SEL{
	PLL_CLOCK1 = 0,		/**分频时钟1，11.2896*2M,	或者其他频率*/
	PLL_CLOCK2 = 1,		/**分频时钟2，12.288*2M,	或者其他频率*/
	PLL_CLOCK3 = 4,		/**分频时钟3，11.2896*2M,	或者其他频率*/
	PLL_CLOCK4 = 5,		/**分频时钟4，12.288*2M,	或者其他频率*/
	PLL_CLOCK5 = 6,		/**分频时钟5，11.2896&12.288*2M,	用于DPLL0外部时钟灌入定频*/
	GPIO_IN0 = 2,		/**GPIO MCLK in0输入*/
	GPIO_IN1 = 3,		/**GPIO MCLK in0输入*/
}MCLK_CLK_SEL;

typedef enum __AUDIO_MODULE{
	AUDIO_DAC0,
	AUDIO_DAC1,
	AUDIO_DAC2,
	AUDIO_ADC0,
	AUDIO_ADC1,
	AUDIO_I2S0,
	AUDIO_I2S1,
	AUDIO_I2S2,
	AUDIO_DSD,
}AUDIO_MODULE;

typedef enum __DLY_CLK_SRC_SEL{
	FR_ASDM0_CLK_MUX = 0,	/**音频时钟延时器，时钟来源选择： ASDM0路时钟*/
	FR_MDAC0_CLK_MUX,		/**音频时钟延时器，时钟来源选择： MDAC0路时钟*/
}DLY_CLK_SRC_SEL;

typedef enum __DLY_CLK_SEL{
	NO_DELAY_CODEC_CLK = 0,	/**选择默认音频时钟源 */
	DELAYED_CODEC_CLK,		/**选择音频时钟延时器为时钟源*/
}DLY_CLK_SEL;

typedef enum __DLY_CLK_CTRL{
	DLY_CLK_12_DELTA = 0,/** 12*delta */
	DLY_CLK_25_DELTA,
	DLY_CLK_37_DELTA,
	DLY_CLK_50_DELTA,
	DLY_CLK_62_DELTA,
	DLY_CLK_75_DELTA,
	DLY_CLK_87_DELTA,
	DLY_CLK_100_DELTA,
}DLY_CLK_CTRL;

typedef enum __CLOCK0_GPIO_Port
{
	GPIO_CLK_OUT_A0,		/**GPIO A0输出时钟观测端口*/
	GPIO_CLK_OUT_A13,		/**GPIO A13输出时钟观测端口*/
	GPIO_CLK_OUT_A16,		/**GPIO A16输出时钟观测端口*/
	GPIO_CLK_OUT_A29,		/**GPIO A29输出时钟观测端口*/
	GPIO_CLK_OUT_B3,		/**GPIO B3输出时钟观测端口*/
}CLK0_GPIO_Port;

typedef enum __CLOCK1_GPIO_Port
{
	GPIO_CLK_OUT_A1,		/**GPIO A1输出时钟观测端口*/
}CLK1_GPIO_Port;

typedef enum __CLOCK0_OUT_MODE
{
	MCLK_PLL_CLK_DIV = 1,//1:   mclk_pll_clk0_div
	Flash_Control_CLK,	//2:   fshc_clk
	USB_CLK,			//3:   usb_clk
	APB_CLK,			//4:   apb_clk
	SPIM_CLK,			//5:   spim_clk
	SDIO_CLK,			//6:   sdio_clk
	RC_CLK,				//7:   rc12M_clk
	BTDM_CLK,			//8:   btdm_hclk，12MHz，FM可用
	BTDM_24M_CLK,		//9:   hosc_clk_div
	BT_PMU_32K_CLK,		//10   hosccnt_32k_ob_clk
	DPLL0_CLK_DIV,		//11:  pll_clk_div
	SPDIF0_CLK,			//12:  spdif0_clk
	CODEC_CLK_IN,		//13
	MDAC0_CLK_SW,		//14
	MDAC1_CLK_SW,		//15
	ASDM0_CLK_SW,		//16
	ASDM1_CLK_SW,		//17
	APLL_CLK_DIV,		//18
	LOSC_32k,			//19 LOSC,硬件有32K晶体接C1和C2管脚，需要先调用RTC_ClockSrcSel(OSC_32K);FM可用
	MDM_24M_CLK,		//20
	DPLL1_CLK_DIV,		//21
	MDAC2_CLK_SW,		//22
	TOUCH_CLK,			//22
}CLOCK0_OUT_MODE;

typedef enum __CLOCK1_OUT_MODE
{
	OTG_1KHz_CLK = 1,	//1:   mclk_pll_clk0_div
	SPDIF0_BLOCK_FLAG,	//2:   spdif0_block_flag
	SPDIF0_FRAME_FLAG,	//3:   spdif0_frame_flag
	MDM_MUX_12M_CLK,	//4:   clk_12m_mdm_mux
	MDM_MUX_48M_CLK,	//5:   clk_48m_mdm_mux
	TOUCH_CLK_2,		//6:   touch_clk
	SPIM1_CLK,			//7:   spim1_clk
	DCDC_OUT_CLK,		//8:   DCDC_CLK_OUT
	SRC0_PLL_DIV_CLK,	//9:   src0_pll_div_mux
	SRC1_PLL_DIV_CLK,	//10   src1_pll_div_mux
	SRC2_PLL_DIV_CLK,	//11:  src2_pll_div_mux
	SPDIF1_BLOCK_FLAG,	//12:  spdif1_block_flag
	SPDIF1_FRAME_FLAG,	//13:  spdif1_frame_flag
	SPDIF1_CLK,			//14:  spdif1_clk
}CLOCK1_OUT_MODE;

typedef enum __INTERNAL_CLOCK_MODE
{
	bt_rc32k_clk,
	pmu_32k_clk,
	rc_12m_clk,
	losc_32k_clk,
	dcdc_clk,
}INTERNAL_CLOCK_MODE;

//typedef enum _SPDIF_MODULE
//{
//	SPDIF0 = 0,
//	SPDIF1,
//}SPDIF_MODULE;

//建议MCLK0 配置为11.2896*4M，MCLK1配置为12.288*4M
#define		AUDIO_PLL_CLK1_FREQ		11289600*2
#define		AUDIO_PLL_CLK2_FREQ		12288000*2
#define		AUDIO_PLL_CLK3_FREQ		11289600*2
#define		AUDIO_PLL_CLK4_FREQ		12288000*2
#define 	IsSelectMclkClk1(freq)  ((AUDIO_PLL_CLK1_FREQ%freq) == 0)


/**
 * @brief	系统参考时钟源配置选择
 * @param	IsOsc TURE：晶体时钟；FALSE：XIN端口外灌方波时钟
 * @param   Freq  系统参考时钟的工作频率：32.768K,1M,2M,...40M,单位HZ
 * @return	无
 * @note	如果使用PLL时钟则必须要先调用该函数
 */
void Clock_Config(bool IsOsc, uint32_t Freq);

/**
 * @brief	设置apll工作频率
 * @param	PllFreq pll频率,只能工作于晶体的整数倍频率
 * @return	无
 * @note	工作于晶体的整数倍频率
 */
bool Clock_APllLock(uint32_t PllFreq);
/**
 * @brief	设置dpll工作频率,等待pll lock
 * @param	DpllModule,dpll模块0，dpll模块1，其中只有DPLL1才能作为core时钟
 * @param	DPllFreq pll频率,单位KHz[120000K-288000K]
 * @return  DPLL锁定情况  TRUE:按设定目标频率锁定
 * @note    DPLL1作为core时钟；DPLL0作为音频时钟，可以关闭以节省芯片功耗
 */
bool Clock_DPllLock(CLK_DPLL_MODULE DpllModule, uint32_t PllFreq);

/**
 * @brief	获取内部时钟频率
 * @param	type 内部时钟种类
 * @param	IsReCount 是否再次获取硬件计数器值。TRUE：再次启动硬件计数器。FALSE：获取上次记录值。针对RC12M有效
 * @return  频率，单位Hz
 */
uint32_t Clock_InternalClockFreqGet(INTERNAL_CLOCK_MODE type, bool IsReCount);

/**
 * @brief	设置pll工作频率,快速锁定模式，等待pll lock
 * @param	PllFreq pll频率,单位KHz[240000K-480000K]
 * @param	K1 [0-15]
 * @param	OS [0-31]
 * @param	NDAC [0-4095]
 * @param	FC [0-2]
 * @param	Slope [0-16777216]
 * @return  PLL锁定情况  TRUE:按设定目标频率锁定
 */
bool Clock_DPllQuicklock(CLK_DPLL_MODULE DpllModule, uint32_t PllFreq, uint8_t K1, uint8_t OS, uint32_t NDAC, uint32_t FC, uint32_t Slope);

/**
 * @brief	设置pll工作频率,开环锁定模式(不需要晶体，无自校准)
 * @param	PllFreq pll频率,单位KHz[240000K-480000K]
 * @param	K1 [0-15]
 * @param	OS [0-31]
 * @param	NDAC [0-4095]
 * @param	FC [0-2]
 * @return  无
 */
void Clock_DPllFreeRun(CLK_DPLL_MODULE DpllModule, uint32_t PllFreq, uint32_t K1, uint32_t OS, uint32_t NDAC, uint32_t FC);

/**
 * @brief	pll free run 一般只需要调此函数即可
 * @param	无
 * @return  TRUE, 配置成功
 */
bool Clock_DPllFreeRunEfuse(CLK_DPLL_MODULE DpllModule);
/**
 * @brief	Dpll模块关闭
 * @param	DpllModule DPLL模块
 * @return  无
 */
void Clock_DPllClose(CLK_DPLL_MODULE DpllModule);


/**
 * @brief	Apll模块关闭
 * @param	无
 * @return  无
 */
void Clock_APllClose(void);

/**
 * @brief	获取dpll0频率
 * @param	无
 * @return  pll频率
 * @note    该函数调用需要应用确保dpll0已经lock，否则读数错误
 */
uint32_t Clock_DPll0FreqGet(void);

/**
 * @brief	获取dpll1频率
 * @param	无
 * @return  pll频率
 * @note    该函数调用需要应用确保dpll1已经lock，否则读数错误
 */
uint32_t Clock_DPll1FreqGet(void);

/**
 * @brief	获取apll频率
 * @param	无
 * @return  pll频率
 * @note    无
 */
uint32_t Clock_APllFreqGet(void);

/**
 * @brief	获取Core工作的时钟频率
 * @param	无
 * @return	Core工作频率
 * @note    Core时钟和系统时钟可以同频
 */
uint32_t Clock_CoreClockFreqGet(void);

/**
 * @brief	获取系统工作的时钟频率
 * @param	无
 * @return	系统工作频率
 * @note    Core时钟和系统时钟固定2分频
 */
uint32_t Clock_SysClockFreqGet(void);

/**
 * @brief  设置Core时钟分频系数
 * @param  DivVal [1-256]，Freq = Fpll/Div
 * @return 无
 * @note    Core时钟和系统时钟可以同频，此时最高运行144MHz
 */
void Clock_CoreClkDivSet(uint32_t DivVal);

/**
 * @brief   获取Core分频系数
 * @param   无
 * @return  系统分频系数[0-255]
 * @note    Core时钟和系统时钟可以同频，此时最高运行144MHz
 */
uint32_t Clock_CoreClkDivGet(void);

/**
 * @brief   设置APB总线分频系数，基于系统总线分频
 * @param   DivVal [2-15]
 * @return  无
 */
void Clock_ApbClkDivSet(uint32_t DivVal);

/**
 * @brief   获取APB总线分频系数，基于系统总线分频
 * @param   无
 * @return  APB总线分频系数[2-14]
 * @note	APB时钟不能大于37.5MHz
 */
uint32_t Clock_ApbClkDivGet(void);

/**
 * @brief   设置USB时钟分频系数，基于PLL时钟分频
 * @param   DivVal [2-16]
 * @return  无
 * #note	
 */
void Clock_USBClkDivSet(uint32_t DivVal);

/**
 * @brief   获取USB时钟分频系数，基于PLL时钟分频
 * @param   无
 * @return  USB时钟分频系数[2-16]
 */
uint32_t Clock_USBClkDivGet(void);

/**
 * @brief  	设置SPI0 master工作时钟分频系数，从系统时钟分频
 * @param  	DivVal 分频系数[2-15]
 * @return  无
 * @note	SPIM时钟不高于120M
 */
void Clock_SPIM0ClkDivSet(uint32_t DivVal);

/**
 * @brief  获取SPI0 master工作时钟分频系数，从系统时钟分频
 * @param  无
 * @return  分频系数[2-15]
 */
uint32_t Clock_SPIM0ClkDivGet(void);

/**
 * @brief  	设置SPI1 master工作时钟分频系数，从系统时钟分频
 * @param  	DivVal 分频系数[2-15]
 * @return  无
 * @note	SPIM时钟不高于120M
 */
void Clock_SPIM1ClkDivSet(uint32_t DivVal);

/**
 * @brief  获取SPI1 master工作时钟分频系数，从系统时钟分频
 * @param  无
 * @return  分频系数[2-15]
 */
uint32_t Clock_SPIM1ClkDivGet(void);

/**
 * @brief  	设置SPDIF0工作时钟分频系数
 * @param  	DivVal 分频系数[2-31]
 * @return  无
 */
void Clock_SPDIF0ClkDivSet(uint32_t DivVal);

/**
 * @brief  获取SPDIF0工作时钟分频系数
 * @param  无
 * @return  分频系数[2-31]
 */
uint32_t Clock_SPDIF0ClkDivGet(void);

/**
 * @brief   设置SDIO0工作时钟分频系数，从系统时钟分频
 * @param   DivVal 分频系数[2-15]
 * @return  无
 * @note	SDIO时钟不能大于60MHz
 */
void Clock_SDIOClkDivSet(uint32_t DivVal);

/**
 * @brief  获取SDIO0工作时钟分频系数，从系统时钟分频
 * @param  无
 * @return  分频系数[2-15]
 */
uint32_t Clock_SDIOClkDivGet(void);

/**
 * @brief  设置MDM工作时钟分频系数
 * @param  DivVal 分频系数[2-15]
 * @return  无
 */
void Clock_MDMClkDivSet(uint32_t DivVal);

/**
 * @brief  获取MDM工作时钟分频系数
 * @param  无
 * @return  分频系数[2-15]
 */
uint32_t Clock_MDMClkDivGet(void);

/**
 * @brief	设置OSC分频系数，分频之后的时钟提供给RTC使用
 * @param	DivVal 分频系数[1-2048]。
 * @return  无
 */
void Clock_OSCClkDivSet(uint32_t DivVal);

/**
 * @brief	获取OSC分频系数，分频之后的时钟提供给RTC使用
 * @param	无
 * @return  OSC分频系数[1-2048]
 */
uint32_t Clock_OSCClkDivGet(void);

/**
 * @brief	获取pll频率
 * @param	Win 计数器窗口【3:2048】
 * @return  无
 */
void Clock_HOSCCntWindowSet(uint32_t Win);

/**
 * @brief	关闭rc频率硬件自动更新功能
 * @param	无
 * @return  无
 */
void Clock_HOSCFreqAutoCntDisable(void);

/**
 * @brief	启动rc频率硬件自动更新功能
 * @param	无
 * @return  无
 */
void Clock_HOSCFreqAutoCntStart(void);

/**
 * @brief	获取rc频率（硬件自动更新）
 * @param	无
 * @return  无
 */
uint32_t Clock_HOSCFreqAutoCntGet(void);

/**
 * @brief	选择系统工作时钟
 * @param	ClkMode
 * 	@arg 	RC_CLK_MODE   : RC12M时钟;
 * 	@arg	DPLL1_CLK_MODE: DPLL1时钟;
 * 	@arg	APLL_CLK_MODE : APLL时钟
 * @return  是否成功切换系统时钟，TRUE：工程切换；FALSE：切换时钟失败。
 * @note    系统时钟工作于pll时钟时，为pll时钟的2分频
 * @note	系统时钟最高频率不能超过384MHz，如果选择的PLL时钟频率超过384MHz，需要配置分频比到合适频率；或者降低锁定的目标频率
 */
bool Clock_SysClkSelect(CLK_MODE ClkMode);

/**
 * @brief	USB FS模块时钟选择,dpll1时钟还是apll时钟
 * @param	ClkMode 时钟源选择
 *   @arg	DPLL1_CLK_MODE:DPLL时钟（PLL分频之后）
 *   @arg	APLL_CLK_MODE: APLL时钟（APLL分频之后）
 * @return  无
 * @note	不支持DPLL0
 */
void Clock_USBClkSelect(CLK_MODE ClkMode);

/**
 * @brief	Uart模块时钟选择,pll时钟还是RC时钟
 * @param	ClkMode 时钟源选择 
 *   @arg	0: RC时钟;
 *   @arg	1: DPLL时钟（和USB同源，USB时钟没有选择，默认是DPLL0）
 * @return  无
 * @note	注意该时钟源如果选择PLL时钟，是和USB FS 时钟源同源
 */
void Clock_UARTClkSelect(uint32_t ClkMode);

/**
 * @brief	Timer5模块时钟选择,系统时钟时钟还是RC时钟
 * @param	ClkMode SYSTEM_CLK_MODE: 系统时钟; RC_CLK_MODE:RC 12M时钟
 * @return  无
 */
void Clock_Timer5ClkSelect(CLK_MODE ClkMode);

/**
 * @brief	BTDM模块时钟选择,系统时钟时钟还是RC时钟
 * @param	ClkMode BTDM模块
 *   @arg	RC_CLK32_MODE: RC12M时钟分频到32K;
 *   @arg	OSC_32K_MODE: 晶体32K时钟(注意来源);
 * @return  无
 */
void Clock_BTDMClkSelect(CLK_32K_MODE ClkMode);

/**
 * @brief	Spdif0模块时钟选择,DPLL时钟还是APLL时钟
 * @param	ClkMode	时钟源选择
 *   @arg	DPLL0_CLK_MODE:DPLL0时钟（PLL分频之后）
 *   @arg	APLL_CLK_MODE:APLL时钟（APLL分频之后）
 * @return  无
 * @note	不支持DPLL1
 */
void Clock_Spdif0ClkSelect(CLK_MODE ClkMode);

/**
 * @brief	Spdif1模块时钟选择,DPLL时钟还是APLL时钟
 * @param	ClkMode	时钟源选择
 *   @arg	DPLL0_CLK_MODE:DPLL0时钟（PLL分频之后）
 *   @arg	APLL_CLK_MODE:APLL时钟（APLL分频之后）
 * @return  无
 * @note	不支持DPLL1
 */
void Clock_Spdif1ClkSelect(CLK_MODE ClkMode);

/**
 * @brief	PLL模块时钟使能
 * @param	ClkSel 模块源，根据CLOCK_PLL_MODULE1_SWITCH选择
 * @return  无
 */
void Clock_PLLModule1Enable(CLOCK_PLL_MODULE1_SWITCH ClkSel);

/**
 * @brief	PLL模块时钟禁能
 * @param	ClkSel 模块源，根据CLOCK_PLL_MODULE1_SWITCH选择
 * @return  无
 */
void Clock_PLLModule1Disable(CLOCK_PLL_MODULE1_SWITCH ClkSel);

/**
 * @brief	PLL模块2时钟使能（SRC）
 * @param	ClkSel 模块源，根据CLOCK_PLL_MODULE2_SWITCH选择
 * @return  无
 */
void Clock_PLLModule2Enable(CLOCK_PLL_MODULE2_SWITCH ClkSel);

/**
 * @brief	PLL模块2时钟禁能（SRC）
 * @param	ClkSel 模块源，根据CLOCK_PLL_MODULE2_SWITCH选择
 * @return  无
 */
void Clock_PLLModule2Disable(CLOCK_PLL_MODULE2_SWITCH ClkSel);

/**
 * @brief	模块时钟使能
 * @param	ClkSel 模块源，根据CLOCK_MODULE1_SWITCH选择
 * @return  无
 */
void Clock_Module1Enable(CLOCK_MODULE1_SWITCH ClkSel);

/**
 * @brief	模块时钟禁能
 * @param	ClkSel 模块源，根据CLOCK_MODULE1_SWITCH选择
 * @return  无
 */
void Clock_Module1Disable(CLOCK_MODULE1_SWITCH ClkSel);

/**
 * @brief	模块时钟使能
 * @param	ClkSel 模块源，根据CLOCK_MODULE2_SWITCH选择
 * @return  无
 */
void Clock_Module2Enable(CLOCK_MODULE2_SWITCH ClkSel);

/**
 * @brief	模块时钟禁能
 * @param	ClkSel 模块源，根据CLOCK_MODULE2_SWITCH选择
 * @return  无
 */
void Clock_Module2Disable(CLOCK_MODULE2_SWITCH ClkSel);

/**
 * @brief	模块时钟使能
 * @param	ClkSel 模块源，根据CLOCK_MODULE3_SWITCH选择
 * @return  无
 */
void Clock_Module3Enable(CLOCK_MODULE3_SWITCH ClkSel);

/**
 * @brief	模块时钟禁能
 * @param	ClkSel 模块源，根据CLOCK_MODULE3_SWITCH选择
 * @return  无
 */
void Clock_Module3Disable(CLOCK_MODULE3_SWITCH ClkSel);

/**
 * @brief	模块时钟使能
 * @param	ClkSel 模块源，根据CLOCK_MODULE3_SWITCH选择
 * @return  无
 */
void Clock_Module4Enable(CLOCK_MODULE4_SWITCH ClkSel);

/**
 * @brief	模块时钟禁能
 * @param	ClkSel 模块源，根据CLOCK_MODULE3_SWITCH选择
 * @return  无
 */
void Clock_Module4Disable(CLOCK_MODULE4_SWITCH ClkSel);

/**
 * @brief	音频PLL时钟生成
 * @param	CLK_MODE PLL源: PLL_CLK 还是 APLL_CLK
 * @param	Index PLL源，PLL_CLK_1:11.2896M;PLL_CLK_2:12.288M;
 * @param	TargetFreq 锁定的音频MCLK频率，推荐配置PLL1： 11 2896M；PLL2: 12.288M。Unit:Hz
 * @return  无
 */
void Clock_AudioPllClockSet(CLK_MODE ClkMode, PLL_CLK_INDEX Index, uint32_t TargetFreq);

/**
  * @brief  获取音频PLL时钟的频率
  * @param  Index: PLL源，根据PLL_CLK_INDEX选择
  * @retval 当前配置的音频PLL时钟频率（Hz），如果Index无效则返回0
  */
uint32_t Clock_AudioPllClockGet(PLL_CLK_INDEX Index);

/**
 * @brief	音频模块主时钟源选择
 * @param	Module 音频模块
 * @param	ClkSel 时钟来源类型选择
 * @return  无
 */
void Clock_AudioMclkSel(AUDIO_MODULE Module, MCLK_CLK_SEL ClkSel);

/**
 * @brief	获取音频模块主时钟源
 * @param	Module 音频模块
 * @return  时钟来源类型
 */
MCLK_CLK_SEL Clock_AudioMclkGet(AUDIO_MODULE Module);

/**
 * @brief	音频时钟源采样率微调，PLL1和PLL2
 * @param	Index PLL源，PLL_CLK_1:11.2896M;PLL_CLK_2:12.288M;
 * @param	Sign  0：调慢；1：调快
 * @param	Ppm 时钟微调参数，为0时硬件时钟无微调功能。
 * @return  无
 */
void Clock_AudioPllClockAdjust(PLL_CLK_INDEX Index,uint8_t Sign, uint16_t Ppm);

/**
 * @brief	使用DeepSleep功能时的系统时钟选择和flash时钟选择
 * @param	ClockSelect
 * @param	FlashClockSel
 * @param	IsEnterDeepSeep  TRUE:系统进入DeepSleep  FALSE:系统不进DeepSleep
 * @return  bool  TRUE:时钟切换成功     FALSE:时钟切换失败
 */
bool Clock_DeepSleepSysClkSelect(CLK_MODE SysClockSelect, FSHC_CLK_MODE FlashClockSel, bool IsEnterDeepSeep);

/**
 * @brief	配置高频晶体的内置起振电容
 * @param	XICap 晶体输入端起振电容
 * @param	XOCap 晶体输出端起振电容
 * @return  无
 */
void Clock_HOSCCapSet(uint32_t XICap, uint32_t XOCap);

/**
 * @brief	配置高频晶体的内置起振电容
 * @param	XICap 晶体输入端起振电容
 * @param	XOCap 晶体输出端起振电容
 * @return  无
 */
void Clock_HOSCMosCapSet(uint32_t XICap, uint32_t XOCap);

/**
 * @brief	配置高频晶体的内置起振电容偏置电流
 * @param	Current 电流值，【0x0-0xF】，0xF最大
 * @return  无
 * @note	注意，电流值的调整会影响24M晶体的震荡频率
 */
void Clock_HOSCCurrentSet(uint32_t Current);

/**
 * @brief	根据自动校准值修正CS 电容
 * @param
 * @return  无
 * @note	注意，调用此函数需要在PLL LOCK 之后
 */
void Clock_HOSCCurrentAutoSet(void);
/**
 * @brief	HOSC晶振关闭
 * @param	无
 * @return  无
 */
void Clock_HOSCDisable(void);

/**
 * @brief	LOSC晶振关闭
 * @param	无
 * @return  无
 */
void Clock_LOSCDisable(void);

/**
 * @brief	时钟输出到GPIO端口上
 * @param	CLK0_GPIO_Port GPIO复用输出时钟端口，GPIOA29，GPIOB6，GPIOB7
 * @param	CLOCK0_OUT_MODE 时钟输出的源
 * @return  无
 */
void Clock_Clock0GPIOOutSel(CLK0_GPIO_Port Port, CLOCK0_OUT_MODE mode);

/**
 * @brief	时钟输出到GPIO端口上
 * @param	CLK1_GPIO_Port GPIO复用输出时钟端口，GPIOA1
 * @param	CLOCK1_OUT_MODE 时钟输出的源
 * @return  无
 */
void Clock_Clock1GPIOOutSel(CLK1_GPIO_Port Port, CLOCK1_OUT_MODE mode);
/**
 * @brief	关闭GPIO端口上的时钟输出功能
 * @param	CLK_GPIO_Port GPIO复用输出时钟端口，GPIOA29，GPIOB6，GPIOB7
 * @return  无
 */
void Clock_Clock0GPIOOutDisable(CLK0_GPIO_Port Port);

/**
 * @brief	设置DPLL0锁定的频率值（寄存器）
 * @param	Freq: 频率值, 单位:KHz
 * @return  无
 */
void Clock_DPll0LockFreqSet(uint32_t Freq);

/**
 * @brief	设置DPLL1锁定的频率值（寄存器）
 * @param	Freq: 频率值, 单位:KHz
 * @return  无
 */
void Clock_DPll1LockFreqSet(uint32_t Freq);

/**
 * @brief	设置APLL锁定的频率值（寄存器）
 * @param	Freq: 频率值, 单位:KHz
 * @return  无
 */
void Clock_APllLockFreqSet(uint32_t Freq);

/**
 * @brief	获取DPLL0锁定的频率值（寄存器）
 * @param	无
 * @return  频率值, 单位:KHz
 */
uint32_t Clock_DPll0LockFreqGet(void);

/**
 * @brief	获取DPLL1锁定的频率值（寄存器）
 * @param	无
 * @return  频率值, 单位:KHz
 */
uint32_t Clock_DPll1LockFreqGet(void);

/**
 * @brief	设置APLL锁定的频率寄存器中
 * @param	无
 * @return  频率值, 单位:KHz
 */
uint32_t Clock_APllLockFreqGet(void);


void Clock_ADC0ClkDivSet(uint32_t DivVal);

uint32_t Clock_ADC0ClkDivGet(void);

void Clock_ADC1ClkDivSet(uint32_t DivVal);

uint32_t Clock_ADC1ClkDivGet(void);

void Clock_DAC0ClkDivSet(uint32_t DivVal);

uint32_t Clock_DAC0ClkDivGet(void);

void Clock_DAC1ClkDivSet(uint32_t DivVal);

uint32_t Clock_DAC1ClkDivGet(void);

void Clock_DAC2ClkDivSet(uint32_t DivVal);

uint32_t Clock_DAC2ClkDivGet(void);

void Clock_I2S0ClkDivSet(uint32_t DivVal);

uint32_t Clock_I2S0ClkDivGet(void);

void Clock_I2S1ClkDivSet(uint32_t DivVal);

uint32_t Clock_I2S1ClkDivGet(void);

void Clock_I2S2ClkDivSet(uint32_t DivVal);

uint32_t Clock_I2S2ClkDivGet(void);

/**
 * @brief	设置SRC时钟的分频值
 * @param	SRCIndex： SRC模块,【0, 1, 2】
 * @param	Div： 分频值，低16bit为小数部分，高8bit为整数部分。整数大于2时才有小数分频，不分配DIV填（1 << 16）
 * @return  无
 */
void Clock_SRCClkDivSet(uint32_t SRCIndex, uint32_t Div);

/**
 * @brief	选择SRC时钟的时钟源
 * @param	SRCIndex： SRC模块,【0, 1, 2】
 * @param	ClkMode： 时钟源，DPLL0_CLK_MODE:DPLL0；APLL_CLK_MODE：APLL
 * @return  无
 */
void Clock_SRCClkSelect(uint32_t SRCIndex, CLK_MODE ClkMode);

/**
 * @brief	获取fshc 工作频率
 * @param	idx： 0-fshc0; other - fshc1
 * @return  频率值
 */
uint32_t Clock_GetFSHCWorkFreq(uint32_t idx);

/**
 * @brief	DPLL1锁相环展频功能配置
 * @param	Mode： 时钟抖动方式
 * @param	Step： 步进
 * @param	FreqMod： 频率值
 * @return  无
 * @note	典型值【3，20, 8000】
 */
void Clock_DPLL1SpreadSpectrumConfig(uint8_t Mode, uint32_t Step, uint32_t FreqMod);

/**
 * @brief	DPLL1锁相环展频功能使能
 * @param	无
 * @return  无
 */
void Clock_DPLL1SpreadSpectrumEnable(void);

/**
 * @brief	DPLL1锁相环展频功能禁能
 * @param	无
 * @return  无
 */
void Clock_DPLL1SpreadSpectrumDisable(void);

/**
 * @brief   使用外部参考采样率描述参考时钟并启动DPLL0锁频
 * @param   ClkSrcSel DPLL0外部时钟源选择，详见DPLL_REF_CLK_SRC_SEL
 * @param   SampleRate 外部参考采样率，单位Hz
 * @param   PllFreq DPLL0目标锁频频率，单位Hz
 * @param   IsNoiseFilterEn TRUE: 使能外部时钟噪声滤波; FALSE: 关闭外部时钟噪声滤波
 * @return  TRUE: 锁频参数配置并启动成功; FALSE: 锁频配置失败
 * @note    SPDIF block按Fs/192处理，I2S block默认按BCLK=64*Fs处理
 */
bool Clock_Dpll0LockStart_ExtRef(DPLL_REF_CLK_SRC_SEL ClkSrcSel, uint32_t SampleRate, uint32_t PllFreq, bool IsNoiseFilterEn);

/**
 * @brief 	获取DPLL0外部参考锁频状态
 * @param 	无
 * @return  TRUE: DPLL0已锁定; FALSE: DPLL0未锁定
 */
bool Clock_DPll0LockStatusGet_ExtRef(void);
/**
 * @brief 	设置DPLL0外部参考锁频保持状态
 * @param 	IsEnable TRUE: 保持当前锁频结果; FALSE: 取消保持继续跟踪参考
 * @return 	无
 */
void Clock_DPll0HoldSet_ExtRef(bool IsEnable);
/**
 * @brief 	获取DPLL0外部参考锁频保持状态
 * @param 	无
 * @return  TRUE: 当前处于保持状态; FALSE: 当前未保持
 */
bool Clock_DPll0HoldGet_ExtRef(void);
/**
 * @brief 	释放DPLL0外部参考锁频配置
 * @param 	无
 * @return  无
 */
void Clock_DPll0Release_ExtRef(void);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif

/**
 * @}
 * @}
 */
 
