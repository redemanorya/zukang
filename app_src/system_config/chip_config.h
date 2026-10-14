#ifndef _CHIP_CONFIG_H_
#define _CHIP_CONFIG_H_

//#include "type.h"
#include "resource_defs.h"
#define	GET_PACKAGE(X)			((X) >> 8)
#define GET_SIP_FLASH(X)		(2 << ((X) & 0xF))
/****************************************************************************************
 * 芯片封装预定义
 * 26B:b00xx xxxx xxxx xxxx
 * 26A:b01xx xxxx xxxx xxxx
 * 16B:b1xxx xxxx xxxx xxxx
 * 16A:b11xx xxxx xxxx xxxx
 * PackagePin bit10~13: b0001:128; 40:b0010; 48:b0011; 52:b0100; 68:b0101; 88:b0110
 * PinPackage bit8~9: 0:A; 1:B; 2:C; 3:D;  Use New PackagePin for E、F、H...
 * bit4~7:预留 Psram、Flash2等
 * Sip Flash(b) bit0~3: 1:4M; 2:8M; 3:16M; 4:32M;...
 ****************************************************************************************/
#define CFG_CHIP_B26X128			0x0400		//128pin
#define CFG_CHIP_B2640A2			0x0803		//40pin
#define CFG_CHIP_B2648B2			0x0D03		//48pin
#define CFG_CHIP_B2648B4			0x0D04
#define CFG_CHIP_B2652B2			0x1103		//52pin
#define CFG_CHIP_B2652B4			0x1104
#define CFG_CHIP_B2668A2			0x1403		//68pin
#define CFG_CHIP_B2668A4			0x1404
#define CFG_CHIP_B2668D2			0x1703
#define CFG_CHIP_B2668D4			0x1704
#define CFG_CHIP_B2688A2			0x1803		//88pin
#define CFG_CHIP_B2688A4			0x1804


#define CFG_CHIP_A2640A2			0x4803		//40pin


#define CFG_CHIP_26					1

/****************************************************************************************
 *       芯片型号选择配置
 ****************************************************************************************/
#if CFG_CHIP_26// 0//  for 双核应用
	#define CFG_CHIP_SEL           		CFG_CHIP_B2688A2  //CFG_CHIP_A2640A2//CFG_CHIP_B2640A2//

	#define CORE1_ILM_REMAP_EN // core1 ILM enable

	#define DUAL_CORE_EN				//双核使能，开启后会启用core2用于DSP
	#ifdef DUAL_CORE_EN
		#define CORE_INFO_STR	"DualCore"
	#else
		#define CORE_INFO_STR	"SingleCore"
	#endif
	/**************************************************************************************
	 * 			芯片封装 出PIN
	 * 	出PIN定义不可增加
	 * 	末尾条目,配合板级电路事项,128开发板模拟其他封装需保留
	***************************************************************************************/
	#if CFG_CHIP_SEL == CFG_CHIP_B26X128
		#define CHIP_FLASH_CAPACTITY			32						//flash容量
		#define	CHIP_GPIOA_RESOURCE				(BITS(0, 31))
		#define CHIP_GPIOB_RESOURCE				(BITS(0, 27))
		#define CHIP_PIN_RF
		#define CHIP_PIN_HSOTG
		#define CHIP_PIN_DAC0
		#define CHIP_PIN_DAC1
		#define CHIP_PIN_DAC2
		#define CHIP_PIN_MIC0
		#define CHIP_PIN_MIC1
		#define CHIP_PIN_LINEIN
		#define CHIP_PIN_AUX0
		#define CHIP_PIN_AUX1

		#define CHIP_PIN_DCDC11
		#define CHIP_PIN_DCDC16
		#define CHIP_PIN_PA_VDD
	#elif GET_PACKAGE(CFG_CHIP_SEL) == GET_PACKAGE(CFG_CHIP_B2640A2)
		#define CHIP_FLASH_CAPACTITY			(GET_SIP_FLASH(CFG_CHIP_SEL))
		#define	CHIP_GPIOA_RESOURCE				(BITS(8, 15))
		#define CHIP_GPIOB_RESOURCE				(BITS(0, 3) | BITS(20, 21))
		#define CHIP_PIN_RF
		#define CHIP_PIN_HSOTG
		#define CHIP_PIN_DAC1
		#define CHIP_PIN_DAC2
		#define CHIP_PIN_MIC0
		#define CHIP_PIN_AUX0

		#define CHIP_PIN_DCDC11
		#define CHIP_PIN_DCDC16
	#elif GET_PACKAGE(CFG_CHIP_SEL) == GET_PACKAGE(CFG_CHIP_A2640A2)
		#define CHIP_FLASH_CAPACTITY			(GET_SIP_FLASH(CFG_CHIP_SEL))
		#define	CHIP_GPIOA_RESOURCE				(BITS(8, 15))
		#define CHIP_GPIOB_RESOURCE				(BITS(0, 3) | BITS(20, 21))
		#define CHIP_PIN_HSOTG
		#define CHIP_PIN_DAC1
		#define CHIP_PIN_DAC2
		#define CHIP_PIN_MIC0
		#define CHIP_PIN_AUX0

		#define CHIP_PIN_DCDC11
		#define CHIP_PIN_DCDC16
	#elif GET_PACKAGE(CFG_CHIP_SEL) == GET_PACKAGE(CFG_CHIP_B2648B2)
		#define CHIP_FLASH_CAPACTITY			(GET_SIP_FLASH(CFG_CHIP_SEL))
		#define	CHIP_GPIOA_RESOURCE				(BITS(4, 16) | BITS(30, 31))
		#define CHIP_GPIOB_RESOURCE				(BITS(0, 6) | BITS(18, 21))
		#define CHIP_PIN_RF
		#define CHIP_PIN_DAC1
		#define CHIP_PIN_DAC2
		#define CHIP_PIN_MIC0
		#define CHIP_PIN_AUX0
		#define CHIP_PIN_AUX1

		#define CHIP_PIN_DCDC16
		#define	CHIP_PIN_PA_VDD
	#elif GET_PACKAGE(CFG_CHIP_SEL) == GET_PACKAGE(CFG_CHIP_B2652B2)
		#define CHIP_FLASH_CAPACTITY			(GET_SIP_FLASH(CFG_CHIP_SEL))
		#define	CHIP_GPIOA_RESOURCE				(BITS(4, 15) | BITS(30, 31))
		#define CHIP_GPIOB_RESOURCE				(BITS(0, 7) | BITS(18, 21))
		#define CHIP_PIN_RF
		#define CHIP_PIN_HSOTG
		#define CHIP_PIN_DAC1
		#define CHIP_PIN_DAC2
		#define CHIP_PIN_MIC0
		#define CHIP_PIN_AUX0
		#define CHIP_PIN_AUX1

		#define CHIP_PIN_DCDC16
	#elif GET_PACKAGE(CFG_CHIP_SEL) == GET_PACKAGE(CFG_CHIP_B2668A2)
		#define CHIP_FLASH_CAPACTITY			(GET_SIP_FLASH(CFG_CHIP_SEL))
		#define	CHIP_GPIOA_RESOURCE				(BITS(0, 1) | BITS(8, 21) | BITS(29, 31))
		#define CHIP_GPIOB_RESOURCE				(BITS(0, 11) | BITS(18, 21))
		#define CHIP_PIN_RF
		#define CHIP_PIN_HSOTG
		#define CHIP_PIN_DAC0
		#define CHIP_PIN_DAC1
		#define CHIP_PIN_MIC0
		#define CHIP_PIN_AUX0
		#define CHIP_PIN_AUX1

		#define CHIP_PIN_DCDC16
		#define CHIP_PIN_PA_VDD
	#elif GET_PACKAGE(CFG_CHIP_SEL) == GET_PACKAGE(CFG_CHIP_B2668D2)
		#define CHIP_FLASH_CAPACTITY			(GET_SIP_FLASH(CFG_CHIP_SEL))
		#define	CHIP_GPIOA_RESOURCE				(BITS(0, 15) | BITS(29, 31))
		#define CHIP_GPIOB_RESOURCE				(BITS(0, 11) | BITS(14, 15) | BITS(18, 21))
		#define CHIP_PIN_RF
		#define CHIP_PIN_HSOTG
		#define CHIP_PIN_DAC0
		#define CHIP_PIN_DAC1
		#define CHIP_PIN_MIC0
		#define CHIP_PIN_MIC1
		#define CHIP_PIN_AUX0
		#define CHIP_PIN_AUX1

		#define CHIP_PIN_DCDC16
		#define CHIP_PIN_PA_VDD
	#elif GET_PACKAGE(CFG_CHIP_SEL) == GET_PACKAGE(CFG_CHIP_B2688A2)
		#define CHIP_FLASH_CAPACTITY			(GET_SIP_FLASH(CFG_CHIP_SEL))
		#define	CHIP_GPIOA_RESOURCE				(BITS(0, 27) | BITS(29, 31))
		#define CHIP_GPIOB_RESOURCE				(BITS(0, 12) | BITS(18, 21))
		#define CHIP_PIN_RF
		#define CHIP_PIN_HSOTG
		#define CHIP_PIN_DAC0
		#define CHIP_PIN_DAC1
		#define CHIP_PIN_DAC2
		#define CHIP_PIN_MIC0
		#define CHIP_PIN_MIC1
		#define CHIP_PIN_AUX0
		#define CHIP_PIN_AUX1

		#define CHIP_PIN_DCDC16
		#define CHIP_PIN_PA_VDD
	#else
		#error "Undefined Chip type!!!!"
	#endif
#endif
#endif


