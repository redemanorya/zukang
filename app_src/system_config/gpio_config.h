/*
 * gpio_config.h
 *
 *  Created on: Sep 12, 2025
 *      Author: piwang
 */

#ifndef APP_GPIO_H_
#define APP_GPIO_H_
#include "gpio.h"
#include "ir.h"
#include "i2s_interface.h"


/*************************************************
 * 功能：
 * 1、适配app_config.h配置的音频与参数
 * 2、适配chip_config.h配置的芯片封装引脚
 * 3、IO user调配
 * 4、IO 冲突提示
 *
 * 仅在app_config.h底部引用
 ***********************************************/


/**********************************************
 * 			Uart Module
 * Uart0:Default Log
 * Uart1:可选,Uart调音/Upgrade/Dump
 * Uart2:备选 Log
 * B6所有IO口均支持UART0 TX，建议作为Log端口;gpio_config.h列表未定义时，自行扩展@gpio.h
 * *******************************************/
#ifdef CFG_UART_LOG_EN
	#define CFG_FLASHBOOT_UART_EN          		(1)//Porting CFG_FLASHBOOT_DEBUG_EN
#else //for flash_boot.h
	#define CFG_FLASHBOOT_UART_EN          		(0)
#endif
#define CFG_UART_TX_PORT  RES_UART1_TX_A15
#define CFG_UART_RX_PORT  RES_UART1_RX_A14
/**Low baudrate may be Dac pop 不建议低波特率,时序有干扰**/
#define CFG_UART_BANDRATE  UART_BAUDRATE_115200

/**Uart调音,不可复用Log模块**/
#ifdef CFG_COMMUNICATION_BY_UART
	#define CFG_FUNC_COMMUNICATION_RX_PORT 			RES_UART1_RX_A14
	#define CFG_FUNC_COMMUNICATION_TX_PORT 			RES_UART1_TX_A15
	#define CFG_FUNC_COMMUNICATION_UART_BAUDRATE 	UART_BAUDRATE_2000000
	#define	CFG_FUNC_COMMUNICATION_RX_DMA_PORT		PERIPHERAL_0_ID_UART1_RX
	#define	CFG_FUNC_COMMUNICATION_TX_DMA_PORT		PERIPHERAL_0_ID_UART1_TX
#endif
/**Uart升级,不可复用Log模块**/
#ifdef CFG_UPGRADE_BY_UART
	#define CFG_FUNC_UPGRADE_TX_PORT 			RES_UART1_TX_A15
	#define CFG_FUNC_UPGRADE_RX_PORT 			RES_UART1_RX_A14
	#define CFG_FUNC_UPGRADE_UART_BAUDRATE 		UART_BAUDRATE_2000000
#endif
/**Uart Dump,不可复用Log模块**/
#ifdef CFG_DUMP_DEBUG_EN
	#define CFG_DUMP_UART_TX_PORT 				RES_UART1_TX_A15
	#define CFG_DUMP_UART_BANDRATE 				(2000000)
#endif


/***************************************************************************************
 * 			I2S Module
 * 	1、I2S输出也使能时端口选择和模式需要注意保持一致;
 * 	2、缺省配置为Master，I2S外设音频使用Master的MCLK,否则对方不微调时钟就会有pop音。
 * 	3、 0:master 1:slave。master 和 slave的角色和MCLK输入和输出没有关系，请手动配置
 * 	4、谨慎开启I2S slave，I2S外设必须提供稳定时钟，外设连线不稳定，设置AudioIOSet.Sync = FALSE；自行DetectLock调整Sync
 * 	5、PLURALCH/TDM_MODE多通路建议Master+同采样率 STD挂接,否者SRC + SRA， Mcps/ram飙升
 *  6、I2S多线/多通路模式且使用音效的情况下，需要手动修改音效图的source、sink（也可参照或直接audioeffect_res\audio_effect_for_read\I2S1MIX_多线.bin）
 * 	   使用方法：将audioeffect_res\audio_effect_for_read\I2S1MIX_多线.bin改名为audio_effect.bin并copy覆盖到audioeffect_res\audio_effect.bin
 ***************************************************************************************/

/* ============================================================
 * 音频线数常量（统一，线数 = 数据线数量）
 * I2S 模式: ch数 = 线数 x 2
 *   AUDIO_LINE_SINGLE(1线) -> 2ch
 *   AUDIO_LINE_DUAL  (2线) -> 4ch
 *   AUDIO_LINE_QUAD  (4线) -> 8ch
 * TDM 模式: ch数 = TdmChNum（见 TDM1CHNUM / TDM2CHNUM）
 *   AUDIO_LINE_SINGLE(单线) / AUDIO_LINE_DUAL(双线)
 *   [!!] TDM 双线时 TdmChNum 必须 >= 8
 * ============================================================ */
#define AUDIO_LINE_SINGLE       1   // 单线
#define AUDIO_LINE_DUAL         2   // 双线
#define AUDIO_LINE_QUAD         4   // 四线


#if defined(CFG_APP_I2SIN_MODE_EN)
	/* ===== 自动推导 ===== */
	#define CFG_FUNC_I2SIN

	#include"i2s.h"

	/* ===== 用户可配置: 时钟 GPIO ===== */
	/**i2s gpio配置，必须统一模块**/
	/**0:master 1:slave。master 和 slave的角色和MCLK输入和输出没有关系，请手动配置**/
//	#define I2S_MCLK_GPIO					MCLK_OUT_A20 //MCLK_OUT/MCLK_IN脚  可选
	#define I2S_LRCLK_GPIO					I2S1_LRCLK_A17//I2S2_LRCLK_A17//
	#define I2S_BCLK_GPIO					I2S1_BCLK_A16//I2S2_BCLK_A16//

	/* ===== 用户可配置: 数据 GPIO ===== */
//	#define CFG_FUNC_I2SIN_TX_EN			//I2S In mode模式下可开I2S Tx out 映射APP_SINK
	#ifdef CFG_FUNC_I2SIN_TX_EN
		#define I2S_DOUT_GPIO				I2S1_DOUT0_A12//I2S2_DOUT0_A18//
	#endif
	#define I2S_DIN_GPIO					I2S1_DIN0_A13//I2S2_DIN0_A20//

	/* ===== 用户可配置: 模块参数 ===== */
	#define CFG_RES_I2S_MODE				I2S_MASTER_MODE

	#define CFG_PARA_I2S_SAMPLERATE			44100
	#define CFG_FUNC_I2S_IN_SYNC_EN			//缺省为SRA
	#define CFG_FUNC_I2S_OUT_SYNC_EN

	/* ===== 自动推导: I2S模块号 ===== */
	#define	CFG_RES_I2S_MODULE				GET_I2S_I2S_PORT(I2S_BCLK_GPIO)	//根据I2S_BCLK_GPIO自动配置i2s1/i2s0
#endif


#if defined(CFG_I2S0_MIX_IN_EN) || defined(CFG_I2S0_MIX_OUT_EN) //defined(CFG_RES_AUDIO_I2S_MIX_IN_EN) || defined(CFG_RES_AUDIO_I2S_MIX_OUT_EN)
	#define CFG_FUNC_I2S0_MIX
	#define I2S0_MIX_MCLK_GPIO					MCLK_OUT_A14 //MCLK_OUT/MCLK_IN脚 可选
	#define I2S0_MIX_LRCLK_GPIO					I2S0_LRCLK_A11
	#define I2S0_MIX_BCLK_GPIO					I2S0_BCLK_A10

	#ifdef CFG_I2S0_MIX_IN_EN
		#define I2S0_MIX_DIN_GPIO				I2S0_DIN_A5
	#endif
	#ifdef CFG_I2S0_MIX_OUT_EN
		#define I2S0_MIX_DOUT_GPIO				I2S0_DOUT_A4
	#endif

	/**0:master 1:slave。master 和 slave的角色和MCLK输入和输出没有关系，请手动配置**/
	#define CFG_I2S0_MIX_MODE					I2S_MASTER_MODE//I2S_SLAVE_MODE//I2S_SLAVE_MODE // 0:master 1:slave
	#define CFG_I2S0_MIX_SAMPLERATE				44100//48000
	#define CFG_FUNC_I2S0_MIX_SYNC_EN			//缺省为SRA
#endif

#if defined(CFG_I2S1_MIX_IN_EN) || defined(CFG_I2S1_MIX_OUT_EN)
	/* ===== 自动推导 ===== */
	#define CFG_FUNC_I2S1_MIX

	/* ===== 用户可配置: 时钟 GPIO ===== */
	#define I2S1_MIX_MCLK_GPIO					MCLK_OUT_A20 //I2S1_MCLK_OUT_A6//MCLK_OUT/MCLK_IN脚  可选
	#define I2S1_MIX_LRCLK_GPIO  I2S1_LRCLK_A11
	#define I2S1_MIX_BCLK_GPIO  I2S1_BCLK_A10

	/* ===== 用户可配置: 格式选择 ===== */
	//#define CFG_FUNC_I2S1_TDM_MODE_EN						 // I2S模块使用TDM格式,注释掉即为使用I2S格式
		#ifdef CFG_FUNC_I2S1_TDM_MODE_EN
			/* [!!] TDM 不支持4线, 双线时 TdmChNum 必须 >= 8 */
			/* 可选: TDM_CH4 / TDM_CH8 / TDM_CH16 / TDM_CH32*/
			#define TDM1CHNUM                       TDM_CH8
		#endif

	#ifdef CFG_I2S1_MIX_IN_EN
		/* ===== 用户可配置: IN 线数模式 ===== */
		/* 可选: AUDIO_LINE_SINGLE(1线) / AUDIO_LINE_DUAL(2线) / AUDIO_LINE_QUAD(4线) */
		/* [!!] TDM 不支持4线, 双线时 TdmChNum 必须 >= 8 */
		#define I2S1_MIX_IN_LINE_MODE           AUDIO_LINE_SINGLE

		#if defined(CFG_FUNC_I2S1_TDM_MODE_EN) && (I2S1_MIX_IN_LINE_MODE == AUDIO_LINE_QUAD)
			#error "TDM 不支持4线(AUDIO_LINE_QUAD)，请改用 AUDIO_LINE_SINGLE 或 AUDIO_LINE_DUAL"
		#endif

		/* ===== 用户可配置: IN GPIO ===== */
		/* 单线 DIN（线数=1使用，TDM/I2S通用） */
		#if (I2S1_MIX_IN_LINE_MODE == AUDIO_LINE_SINGLE)
			#define I2S1_MIX_DIN_GPIO              I2S1_DIN0_A13
		#endif

		/* 多线 DIN（线数>=2使用，TDM/I2S通用） */
		/* I2S多线模式,某个GPIO配置注释掉,即为该立体声不使用 */
		#if (I2S1_MIX_IN_LINE_MODE >= AUDIO_LINE_DUAL)
			#define I2S1_MIX_DIN0_GPIO         I2S1_DIN0_A12
			#define I2S1_MIX_DIN1_GPIO         I2S1_DIN1_A13
		#endif

		/* 四线 DIN(线数 >= 4) */
		#if (I2S1_MIX_IN_LINE_MODE >= AUDIO_LINE_QUAD)
			#define I2S1_MIX_DIN2_GPIO         I2S1_DIN2_A14
			#define I2S1_MIX_DIN3_GPIO         I2S1_DIN3_A15
		#endif

		/* 多线输入使用多路处理功能，打开此功能下可支持使用多线输入SRA、SRC功能，但需注意内存资源 */
		/* [!!] 在开启TDM功能时，超过TDM_CH8则需手动屏蔽多路处理功能，防止由于SRA、SRC资源造成内存溢出 */
		#if (I2S1_MIX_IN_LINE_MODE >= AUDIO_LINE_DUAL)
			#define CFG_I2S1_MIX_PLURAL_IN_FIFO_EN
		#endif

		/* ===== 自动推导: IN 线数/通道数 ===== */
		#define I2S1_MIX_IN_LINE_CNT            I2S1_MIX_IN_LINE_MODE

		#ifdef CFG_FUNC_I2S1_TDM_MODE_EN
			#define I2S1_MIX_IN_CHNUM          TDM1CHNUM
		#else
			#define I2S1_MIX_IN_CHNUM          (I2S1_MIX_IN_LINE_CNT * 2)
		#endif
	#endif

	#ifdef CFG_I2S1_MIX_OUT_EN
		/* ===== 用户可配置: OUT 线数模式 ===== */
		/* 可选: AUDIO_LINE_SINGLE(1线) / AUDIO_LINE_DUAL(2线) / AUDIO_LINE_QUAD(4线) */
		/* [!!] TDM 不支持4线, 双线时 TdmChNum 必须 >= 8 */
		#define I2S1_MIX_OUT_LINE_MODE           AUDIO_LINE_SINGLE

		#if defined(CFG_FUNC_I2S1_TDM_MODE_EN) && (I2S1_MIX_OUT_LINE_MODE == AUDIO_LINE_QUAD)
			#error "TDM 不支持4线(AUDIO_LINE_QUAD)，请改用 AUDIO_LINE_SINGLE 或 AUDIO_LINE_DUAL"
		#endif

		/* ===== 用户可配置: OUT GPIO ===== */
		/* 单线 DOUT（线数=1使用，TDM/I2S通用） */
		#if (I2S1_MIX_OUT_LINE_MODE == AUDIO_LINE_SINGLE)
			#define I2S1_MIX_DOUT_GPIO				I2S1_DOUT0_A12
		#endif

		/* 多线 DOUT（线数>=2使用，TDM/I2S通用） */
		/* I2S多线模式,某个GPIO配置注释掉,即为该立体声不使用 */
		#if (I2S1_MIX_OUT_LINE_MODE >= AUDIO_LINE_DUAL)
			#define I2S1_MIX_DOUT0_GPIO				I2S1_DOUT0_B8
			#define I2S1_MIX_DOUT1_GPIO				I2S1_DOUT1_B9
		#endif

		/* 四线 DOUT(线数 >= 4) */
		#if (I2S1_MIX_OUT_LINE_MODE >= AUDIO_LINE_QUAD)
			#define I2S1_MIX_DOUT2_GPIO				I2S1_DOUT2_B10
			#define I2S1_MIX_DOUT3_GPIO				I2S1_DOUT3_B11
		#endif

		/* 多线输出使用多路处理功能，打开此功能下可支持使用多线输出SRA、SRC功能，但需注意内存资源 */
		/* [!!] 在开启TDM功能时，超过TDM_CH8则需手动屏蔽多路处理功能，防止由于SRA、SRC资源造成内存溢出 */
		#if (I2S1_MIX_OUT_LINE_MODE >= AUDIO_LINE_DUAL)
			#define CFG_I2S1_MIX_PLURAL_OUT_FIFO_EN
		#endif

		/* ===== 自动推导: OUT 线数/通道数 ===== */
		#define I2S1_MIX_OUT_LINE_CNT            I2S1_MIX_OUT_LINE_MODE

		#ifdef CFG_FUNC_I2S1_TDM_MODE_EN
			#define I2S1_MIX_OUT_CHNUM          TDM1CHNUM
		#else
			#define I2S1_MIX_OUT_CHNUM          (I2S1_MIX_OUT_LINE_CNT * 2)
		#endif
	#endif

	/* ===== 用户可配置: 模块参数 I2S模式 ===== */
	#define CFG_I2S1_MIX_MODE					I2S_MASTER_MODE
	#define CFG_I2S1_MIX_SAMPLERATE  48000//48000//
	#define CFG_FUNC_I2S1_MIX_SYNC_EN			//缺省为SRA
#endif

#if defined(CFG_I2S2_MIX_IN_EN) || defined(CFG_I2S2_MIX_OUT_EN)
	/* ===== 自动推导 ===== */
	#define CFG_FUNC_I2S2_MIX

	/* ===== 用户可配置: 时钟 GPIO ===== */
	#define I2S2_MIX_LRCLK_GPIO					I2S2_LRCLK_A17
	#define I2S2_MIX_BCLK_GPIO					I2S2_BCLK_A16

	/* ===== 用户可配置: 格式选择 ===== */
	//#define CFG_FUNC_I2S2_TDM_MODE_EN						   // I2S模块使用TDM格式,注释掉即为使用I2S格式
		#ifdef CFG_FUNC_I2S2_TDM_MODE_EN
			/* [!!] TDM 不支持4线, 双线时 TdmChNum 必须 >= 8 */
			/* 可选: TDM_CH4 / TDM_CH8 / TDM_CH16 / TDM_CH32*/
			#define TDM2CHNUM                       TDM_CH8
		#endif

	#ifdef CFG_I2S2_MIX_IN_EN
		/* ===== 用户可配置: IN 线数模式 ===== */
		/* 可选: AUDIO_LINE_SINGLE(1线) / AUDIO_LINE_DUAL(2线) / AUDIO_LINE_QUAD(4线) */
		/* [!!] TDM 不支持4线, 双线时 TdmChNum 必须 >= 8 */
		#define I2S2_MIX_IN_LINE_MODE           AUDIO_LINE_SINGLE

		#if defined(CFG_FUNC_I2S2_TDM_MODE_EN) && (I2S2_MIX_IN_LINE_MODE == AUDIO_LINE_QUAD)
			#error "TDM 不支持4线(AUDIO_LINE_QUAD)，请改用 AUDIO_LINE_SINGLE 或 AUDIO_LINE_DUAL"
		#endif

		/* ===== 用户可配置: IN GPIO ===== */
		/* 单线 DIN（线数=1使用，TDM/I2S通用） */
		#if (I2S2_MIX_IN_LINE_MODE == AUDIO_LINE_SINGLE)
			#define I2S2_MIX_DIN_GPIO              I2S2_DIN0_A20
		#endif

		/* 多线 DIN（线数>=2使用，TDM/I2S通用） */
		/* I2S多线模式,某个GPIO配置注释掉,即为该立体声不使用 */
		#if (I2S2_MIX_IN_LINE_MODE >= AUDIO_LINE_DUAL)
			#define I2S2_MIX_DIN0_GPIO         I2S2_DIN0_A18
			#define I2S2_MIX_DIN1_GPIO         I2S2_DIN1_A19
		#endif

		/* 四线 DIN(线数 >= 4) */
		#if (I2S2_MIX_IN_LINE_MODE >= AUDIO_LINE_QUAD)
			#define I2S2_MIX_DIN2_GPIO         I2S2_DIN2_A20
			#define I2S2_MIX_DIN3_GPIO         I2S2_DIN3_A21
		#endif

		/* 多线输入使用多路处理功能，打开此功能下可支持使用多线输入SRA、SRC功能，但需注意内存资源 */
		/* [!!] 在开启TDM功能时，超过TDM_CH8则需手动屏蔽多路处理功能，防止由于SRA、SRC资源造成内存溢出 */
		#if (I2S2_MIX_IN_LINE_MODE >= AUDIO_LINE_DUAL)
			#define CFG_I2S2_MIX_PLURAL_IN_FIFO_EN
		#endif

		/* ===== 自动推导: IN 线数/通道数 ===== */
		#define I2S2_MIX_IN_LINE_CNT            I2S2_MIX_IN_LINE_MODE

		#ifdef CFG_FUNC_I2S2_TDM_MODE_EN
			#define I2S2_MIX_IN_CHNUM          TDM2CHNUM
		#else
			#define I2S2_MIX_IN_CHNUM          (I2S2_MIX_IN_LINE_CNT * 2)
		#endif
	#endif

	#ifdef CFG_I2S2_MIX_OUT_EN
		/* ===== 用户可配置: OUT 线数模式 ===== */
		/* 可选: AUDIO_LINE_SINGLE(1线) / AUDIO_LINE_DUAL(2线) / AUDIO_LINE_QUAD(4线) */
		/* [!!] TDM 不支持4线, 双线时 TdmChNum 必须 >= 8 */
		#define I2S2_MIX_OUT_LINE_MODE           AUDIO_LINE_SINGLE

		#if defined(CFG_FUNC_I2S2_TDM_MODE_EN) && (I2S2_MIX_OUT_LINE_MODE == AUDIO_LINE_QUAD)
			#error "TDM 不支持4线(AUDIO_LINE_QUAD)，请改用 AUDIO_LINE_SINGLE 或 AUDIO_LINE_DUAL"
		#endif

		/* ===== 用户可配置: OUT GPIO ===== */
		/* 单线 DOUT（线数=1使用，TDM/I2S通用） */
		#if (I2S2_MIX_OUT_LINE_MODE == AUDIO_LINE_SINGLE)
			#define I2S2_MIX_DOUT_GPIO				I2S2_DOUT0_A18
		#endif

		/* 多线 DOUT（线数>=2使用，TDM/I2S通用） */
		/* I2S多线模式,某个GPIO配置注释掉,即为该立体声不使用 */
		#if (I2S2_MIX_OUT_LINE_MODE >= AUDIO_LINE_DUAL)
			#define I2S2_MIX_DOUT0_GPIO				I2S2_DOUT0_B2
			#define I2S2_MIX_DOUT1_GPIO				I2S2_DOUT1_B3
		#endif

		/* 四线 DOUT(线数 >= 4) */
		#if (I2S2_MIX_OUT_LINE_MODE >= AUDIO_LINE_QUAD)
			#define I2S2_MIX_DOUT2_GPIO				I2S2_DOUT2_B4
			#define I2S2_MIX_DOUT3_GPIO				I2S2_DOUT3_B5
		#endif

		/* 多线输出使用多路处理功能，打开此功能下可支持使用多线输出SRA、SRC功能，但需注意内存资源 */
		/* [!!] 在开启TDM功能时，超过TDM_CH8则需手动屏蔽多路处理功能，防止由于SRA、SRC资源造成内存溢出 */
		#if (I2S2_MIX_OUT_LINE_MODE >= AUDIO_LINE_DUAL)
			#define CFG_I2S2_MIX_PLURAL_OUT_FIFO_EN
		#endif

		/* ===== 自动推导: OUT 线数/通道数 ===== */
		#define I2S2_MIX_OUT_LINE_CNT            I2S2_MIX_OUT_LINE_MODE

		#ifdef CFG_FUNC_I2S2_TDM_MODE_EN
			#define I2S2_MIX_OUT_CHNUM          TDM2CHNUM
		#else
			#define I2S2_MIX_OUT_CHNUM          (I2S2_MIX_OUT_LINE_CNT * 2)
		#endif
	#endif

	/* ===== 用户可配置: 模块参数 ===== */
	#define CFG_I2S2_MIX_MODE					I2S_MASTER_MODE
	#define CFG_I2S2_MIX_SAMPLERATE				44100//48000//
	#define CFG_FUNC_I2S2_MIX_SYNC_EN		//缺省为SRA
#endif


 /****************************************************************************************
  * 				SDIO Module
****************************************************************************************/
#if defined(CFG_APP_CARD_PLAY_MODE_EN) || defined(CFG_OTG_MODE_READER_EN) || defined(CFG_OTG_FS_MODE_READER_EN)
	#define	CFG_RES_CARD_USE  //启用SD电路
	#include "sd_card.h"
	#if (!CHIP_IO_ERR(RES_GPIOA5) && !CHIP_IO_ERR(RES_GPIOA6) && !CHIP_IO_ERR(RES_GPIOA7))
		#define	CFG_RES_CARD_GPIO					SDIO_A5_A6_A7
	#elif (!CHIP_IO_ERR(RES_GPIOA10) && !CHIP_IO_ERR(RES_GPIOA11) && !CHIP_IO_ERR(RES_GPIOA12))
		#define	CFG_RES_CARD_GPIO					SDIO_A10_A11_A12
	#elif (!CHIP_IO_ERR(RES_GPIOB0) && !CHIP_IO_ERR(RES_GPIOB1) && !CHIP_IO_ERR(RES_GPIOB2))
		#define	CFG_RES_CARD_GPIO					SDIO_B0_B1_B2
	#else //usr check
		#define	CFG_RES_CARD_GPIO					SDIO_A2_TO_A7  //SDIO_A10_TO_A15  //SDIO_B0_TO_B5 //
	#endif

	#define	CFG_FUNC_CARD_DETECT //检测sd卡插入弹出，电路配合

	#if (CFG_RES_CARD_GPIO == SDIO_B0_TO_B5) || (CFG_RES_CARD_GPIO == SDIO_B0_B1_B2)
		#define CARD_DETECT_GPIO					RES_GPIOB1
	#elif (CFG_RES_CARD_GPIO == SDIO_A10_TO_A15) || (CFG_RES_CARD_GPIO == SDIO_A10_A11_A12)
		#define CARD_DETECT_GPIO					RES_GPIOA11
	#elif (CFG_RES_CARD_GPIO == SDIO_A2_TO_A7) || (CFG_RES_CARD_GPIO == SDIO_A5_A6_A7)
		#define CARD_DETECT_GPIO					RES_GPIOA6
	#endif
#endif


/****************************************************************************************
 *						SPDIF光纤同轴功能相关配置参数
****************************************************************************************/
#define SPDIF_0 0
#define SPDIF_1 1
#if (defined (CFG_APP_OPTICAL_MODE_EN)) || (defined (CFG_APP_COAXIAL_MODE_EN))
	#ifdef CFG_APP_OPTICAL_MODE_EN
		#define CFG_SPDIF_OPTICAL_MODULE				SPDIF_1
		#if (CFG_SPDIF_OPTICAL_MODULE == SPDIF_0)
			#define CFG_SPDIF_OPTICAL_RX_DMA_CHANNEL	PERIPHERAL_0_ID_SPDIF0_RX
		#elif (CFG_SPDIF_OPTICAL_MODULE == SPDIF_1)
			#define CFG_SPDIF_OPTICAL_RX_DMA_CHANNEL	PERIPHERAL_1_ID_SPDIF1_RX
		#endif
		// GPIOB0 or GPIOB2 when SPDIF_0, GPIOB1 or GPIOB3 when SPDIF_1
		#if (CFG_SPDIF_OPTICAL_MODULE == SPDIF_0)
			#define SPDIF_OPTICAL_INDEX					GPIOB0
			#define SPDIF_OPTICAL_PORT_MODE				10
//			#define SPDIF_OPTICAL_INDEX					GPIOB2
//			#define SPDIF_OPTICAL_PORT_MODE				6
		#elif (CFG_SPDIF_OPTICAL_MODULE == SPDIF_1)
			#define SPDIF_OPTICAL_INDEX					GPIOB1
			#define SPDIF_OPTICAL_PORT_MODE				8
//			#define SPDIF_OPTICAL_INDEX					GPIOB3
//			#define SPDIF_OPTICAL_PORT_MODE				6
		#endif
	#endif
	#ifdef CFG_APP_COAXIAL_MODE_EN
		#define CFG_SPDIF_COAXIAL_MODULE				SPDIF_0		//coaxial just support spdif0 in
		#define CFG_SPDIF_COAXIAL_RX_DMA_CHANNEL		PERIPHERAL_0_ID_SPDIF0_RX
		#define SPDIF_COAXIAL_PORT_ANA_INPUT			SPDIF_ANA_INPUT_B2
		#define SPDIF_COAXIAL_INDEX						GPIOB2
	#endif
#endif

// enable one SPDIF module TX or RX, not support TX + RX
#ifdef CFG_RES_AUDIO_SPDIF0OUT_EN
	#define SPDIF0_OUT_MODULE				SPDIF_0
	#define SPDIF0_OUT_DMA_ID				PERIPHERAL_0_ID_SPDIF0_TX
	#define SPDIF0_OUT_INDEX				GPIOB0
	#define SPDIF0_OUT_PORT_MODE			18
//	#define SPDIF0_OUT_INDEX				GPIOB2
//	#define SPDIF0_OUT_PORT_MODE			14
#endif
#ifdef CFG_RES_AUDIO_SPDIF1OUT_EN
	#define SPDIF1_OUT_MODULE				SPDIF_1
	#define SPDIF1_OUT_DMA_ID				PERIPHERAL_1_ID_SPDIF1_TX
	#define SPDIF1_OUT_INDEX				GPIOB1
	#define SPDIF1_OUT_PORT_MODE			17
//	#define SPDIF1_OUT_INDEX				GPIOB3
//	#define SPDIF1_OUT_PORT_MODE			16
#endif


/****************************************************************************************
 *                            HDMI功能相关配置参数
 *					CEC管脚对ARC管脚有影响，需要注意PCB走线
****************************************************************************************/
#ifdef  CFG_APP_HDMIIN_MODE_EN
	#define HDMI_SPDIF0 0 //B6仅SPDIF0支持HDMI

	/**ARC**/
	#define HDMI_ARC_RECV_IO_PIN			RES_GPIOB2
	#define HDMI_ARC_RECV_ANA_PIN           SPDIF_ANA_INPUT_B2

	/**hdmi spdif 配置**/
	#define CFG_HDMI_SPDIF_NUM 				HDMI_SPDIF0
	#define CFG_HDMI_DMA_CHANNEL 			PERIPHERAL_0_ID_SPDIF0_RX
	/**CEC**/
	//timer3可以使用rc时钟或者系统时钟，
	//PWC可以复用所有GPIO口，而PWM只有指定GPIO口可以用，所以二者接口参数用法不一样，CEC接收IO无需设置RECV_IO和RECV_IO_PIN
	#define HDMI_CEC_CLK_MODE				RC_CLK_MODE//SYSTEM_CLK_MODE//RC_CLK_MODE /*启用cec唤醒功能时，只能用rc*/
	#define HDMI_CEC_IO_PIN					RES_GPIOA0		// cec口设置

	#define HDMI_CEC_SEND_IO	            TIMER5_PWM0_A0_A6_A12_A18_A24_A30_B4_B10 /*dma table配合*/
	#define HDMI_CEC_SEND_IO_PIN			(0)
    #define HDMI_CEC_RECV_DATA_ADDR	    	TIMER5_PWC_DATA_ADDR
    #define HDMI_CEC_SEND_DATA_ADDR         TIMER5_PWM_DUTY_ADDR

	#define CFG_PARA_WAKEUP_GPIO_CEC		WAKEUP_GPIOA0

	#define	TIMER5_PWC_DATA_ADDR			(0x40050034)
	#define TIMER5_PWM_DUTY_ADDR			(0x40050024)

	#define	TIMER6_PWC_DATA_ADDR			(0x40050834)
	#define TIMER6_PWM_DUTY_ADDR			(0x40050824)

	#define HDMI_CEC_RECV_TIMER_ID	        TIMER5
	#define HDMI_CEC_RECV_DMA_ID	        PERIPHERAL_0_ID_TIMER5
	#define HDMI_CEC_RECV_DMA_ADDR	        TIMER5_PWC_DATA_ADDR

	#define HDMI_CEC_SEND_TIMER_ID	        TIMER5
	#define HDMI_CEC_SEND_DMA_ID	        PERIPHERAL_0_ID_TIMER5
	#define HDMI_CEC_SEND_DMA_ADDR	        TIMER5_PWM_DUTY_ADDR

	#define HDMI_CEC_ARBITRATION_TIMER_ID   TIMER6
	#define HDMI_CEC_ARBITRATION_TIMER_IRQ  TMR6_IRQn
	#define HDMI_CEC_ARBITRATION_TIMER_FUNC Timer6_Interrupt

	/**HPD**/
	#define HDMI_HPD_CHECK_DETECT_EN        //HDMI后插先播功能选项

	#define HDMI_HPD_CHECK_STATUS_IO_PIN   	RES_GPIOA24		//检测口配置
	#define HDMI_HPD_ACTIVE_LEVEL			1  //高电平有效

	#define HdmiScl_IO_PORT					RES_GPIOB7	// IIC clk口
	#define HdmiSda_IO_PORT					RES_GPIOB6	// IIC dat口
	#define HDMI_DDC_IO_PIN					I2C0_PORT_B6_B7

	/**DDC**/
	#define DDC_USE_SW_I2C
	#ifndef DDC_USE_SW_I2C
	   #define HDMI_DDC_DATA_IO_INIT()		GPIO_RegOneBitSet(PORT_PU_REG(HdmiSda_IO_PORT), GET_IO_PIN(HdmiSda_IO_PORT)),\
										    GPIO_RegOneBitClear(PORT_PD_REG(HdmiSda_IO_PORT), GET_IO_PIN(HdmiSda_IO_PORT))
	   #define HDMI_DDC_CLK_IO_INIT()		GPIO_RegOneBitSet(PORT_PU_REG(HdmiScl_IO_PORT), GET_IO_PIN(HdmiScl_IO_PORT)),\
										    GPIO_RegOneBitClear(PORT_PD_REG(HdmiScl_IO_PORT), GET_IO_PIN(HdmiScl_IO_PORT))
    #endif
#endif


/**************************************************************************
 * 								Key IO
 ***************************************************************************/
/**ADC按键**/
#ifdef CFG_RES_ADC_KEY_SCAN
	#define CFG_RES_POWERKEY_ADC_EN		  //power key脚上adc key功能使能
	#if (!CHIP_IO_ERR(RES_GPIOA22))	//检查引脚
		#define	CFG_RES_ADC_KEY_CH1_IO		RES_GPIOA22
	#endif
	#if defined(CFG_RES_ADC_KEY_CH1_IO)
		#define CFG_RES_ADC_KEY_CH1			GET_RES_ADC_CHN(CFG_RES_ADC_KEY_CH1_IO)
		#define CFG_PARA_WAKEUP_GPIO_ADCKEY	WAKEUP_GPIOA22 //同步设置唤醒端口
	#endif //CFG_RES_ADC_KEY_CH1

	#if (!CHIP_IO_ERR(RES_GPIOA23)) //检查引脚
		#define	CFG_RES_ADC_KEY_CH2_IO		RES_GPIOA23
	#endif
	#ifdef CFG_RES_ADC_KEY_CH2_IO
		#define CFG_RES_ADC_KEY_CH2			GET_RES_ADC_CHN(CFG_RES_ADC_KEY_CH2_IO)
	#endif

	#if defined(CFG_FUNC_DEBUG_EN)
//		#define CFG_ADC_KEY_DEBUG//Key Value List
	#endif
#endif //CFG_RES_ADC_KEY_SCAN

/**IR PIN**/
#if defined(CFG_RES_IR_KEY_SCAN) || defined(CFG_PARA_WAKEUP_SOURCE_IR)
	#define CFG_RES_IR_PIN					IR_GPIOA13
#endif

/*GPIO中断源和touch中断源复用一个端口，中断服务函数需要注意*/
/**编码旋钮按键**/
#ifdef CFG_RES_CODE_KEY_USE
	#define CFG_CODE_KEY1P_BANK				'A'
	#define CFG_CODE_KEY1P_PIN				(28)
	#define CFG_CODE_KEY1N_BANK				'A'
	#define CFG_CODE_KEY1N_PIN				(27)
#endif

/**GPIO按键**/
#ifdef CFG_RES_IO_KEY_SCAN
//	#define  CFG_SOFT_POWER_KEY_EN                          //外围硬件自锁式软开关宏开关,开机后，做普通GPIO KEY0使用；
    #define  CFG_GPIO_KEY1_EN                               //GPIO KEY1使能
	#define  CFG_GPIO_KEY2_EN                               //GPIO KEY2使能

    #define CFG_PARA_WAKEUP_GPIO_IOKEY1			WAKEUP_GPIOA25 //同步设置唤醒端口
	#define CFG_PARA_WAKEUP_GPIO_IOKEY2			WAKEUP_GPIOA26 //同步设置唤醒端口
#ifdef	CFG_SOFT_POWER_KEY_EN
	 //SOFT POWER KEY, GPIOA27做POWER KEY检测
	#define POWER_KEY_PIN              			RES_GPIOA27
#endif
#ifdef CFG_GPIO_KEY1_EN
	#define	CFG_RES_GPIO_KEY1_IO				RES_GPIOA25
#endif
#ifdef CFG_GPIO_KEY2_EN
	#define	CFG_RES_GPIO_KEY2_IO				RES_GPIOA26
#endif

#endif

//****************************************************************************************
// 				POWER MONITOR功能配置
// 定义电能监视(适用于带电池系统)的功能宏和选项宏
// 电能监视包括，电池电压检测及低电压后的系统行为以及充电指示等
// 电池电压检测，是指LDOIN输入端的电压检测功能
// 该功能宏打开后，默认包含电池电压检测功能
//****************************************************************************************
#ifdef CFG_FUNC_POWER_MONITOR_EN

	#define	 CFG_FUNC_OPTION_CHARGER_DETECT		//打开该宏定义，支持GPIO检测充电设备插入功能
		#ifdef CFG_FUNC_OPTION_CHARGER_DETECT
			#define CHARGE_DETECT_GPIO			RES_GPIOA31		//充电检测端口设置
		#endif

	#define BAT_VOL_DET_LRADC //打开该宏定义则为ADC检测电池电量  关闭为默认的LDOIN检测电池电量
		#ifdef BAT_VOL_DET_LRADC
			#define BAT_VOL_LRADC_PORT			RES_GPIOA30		//充电检测ADC口设置
		#endif
#endif

/***********************************************************************************
 * 							8段LED显示操作
 * LED显存刷新需要在Timer1ms中断进行，读写flash操作时会关闭中断
 * 所以需要做特殊处理，请关注该宏包含的代码段
 * 注意timer中断服务函数和调用到的API必须进入TCM，含调用的所有api，库函数请咨询支持
 * 开启此宏，要关注所有使用NVIC_SetPriority 设置为0的代码，避免对应中断调用非TCM代码引起死机复位
*****************************************************************************************/
#ifdef DISP_DEV_7_LED
  #define	CFG_FUNC_LED_REFRESH
#endif

/****************************************IO conflict warning, chipset error**************************************************
 * SDK典型性配置冲突管理
 * ************************************************************************************************************************/
#if defined(CFG_RES_ADC_KEY_CH1_IO) && (CHIP_IO_ERR(CFG_RES_ADC_KEY_CH1_IO) || RES_IO_ADC_ERR(CFG_RES_ADC_KEY_CH1_IO))
	#error Conflict: AdckeyCH1 Pin异常!!!
#endif
#if defined(CFG_RES_ADC_KEY_CH2_IO) && (CHIP_IO_ERR(CFG_RES_ADC_KEY_CH2_IO) || RES_IO_ADC_ERR(CFG_RES_ADC_KEY_CH2_IO))
	#error Conflict: AdckeyCH2 Pin异常!!!
#endif

#if (defined(I2S1_MIX_IN_LINE_CNT) && (I2S1_MIX_IN_LINE_CNT > AUDIO_LINE_SINGLE)) || (defined(I2S2_MIX_IN_LINE_CNT) && (I2S2_MIX_IN_LINE_CNT > AUDIO_LINE_SINGLE)) || (defined(I2S1_MIX_OUT_LINE_CNT) && (I2S1_MIX_OUT_LINE_CNT > AUDIO_LINE_SINGLE)) || (defined(I2S2_MIX_OUT_LINE_CNT) && (I2S2_MIX_OUT_LINE_CNT > AUDIO_LINE_SINGLE))
	//#error "使用i2S 4线模式，需要手动将tools\merge_script\audio_effect_for_read\I2S1MIX_4线.bin替换为tools\merge_script\audio_effect.bin"
#endif

#if (defined(I2S1_MIX_IN_LINE_CNT) && (I2S1_MIX_IN_LINE_CNT > AUDIO_LINE_SINGLE)) && (defined(I2S2_MIX_IN_LINE_CNT) && (I2S2_MIX_IN_LINE_CNT > AUDIO_LINE_SINGLE))
	#error "SDK资源限制：i2S 4线模式不支持同时2个in同时使用"
#endif

#if (defined(I2S1_MIX_OUT_LINE_CNT) && (I2S1_MIX_OUT_LINE_CNT > AUDIO_LINE_SINGLE)) && (defined(I2S2_MIX_OUT_LINE_CNT) && (I2S2_MIX_OUT_LINE_CNT > AUDIO_LINE_SINGLE))
	#error "SDK资源限制：i2S 4线模式不支持同时2个out同时使用"
#endif

#if defined(CFG_FUNC_I2S2_MIX) && defined(ROBOEFFECT_DOUBLE_ENGINE_EN)
	#warning I2S2Mix缺省在图2,要Master;两者采样率配置要一致,改图切记!!!
#endif

/**Display**/
#if defined(DISP_DEV_SLED) && defined(DISP_DEV_7_LED) && defined(LED_IO_TOGGLE)
   #error Conflict: display setting error //不能同时选择两种显示模式
#endif

/***********************************************************************************
 *                              Headphone (HP) 模块引脚                          *
 * HP_DETC  : PB9   耳机插入检测 1=插入
 * AD_EN    : PB8   ADC参考电压使能 1=开(3.3V)
 * HP_AD_R  : PA30  AD0 右声道阻抗回采 (16ohm=0.16V / 32ohm=0.32V)
 * HP_AD_L  : PA31  AD1 左声道阻抗回采
 * 注意: 以上引脚由 headphone.h/headphone.c 直接使用,
 *       如需改用其它引脚, 修改 headphone.c 顶部宏即可。
 **********************************************************************************/
#ifdef CFG_FUNC_HEADPHONE_EN
	#define HP_DETECT_GPIO					RES_GPIOB9		// 耳机插入检测 高=插入
	#define HP_AD_EN_GPIO					RES_GPIOB8		// ADC参考3.3V使能 高=开
	#define HP_ADC_RIGHT_PORT				RES_GPIOA30		// 右声道阻抗 AD0
	#define HP_ADC_LEFT_PORT				RES_GPIOA31		// 左声道阻抗 AD1
	#define HP_EN_GPIO						RES_GPIOB10		// 耳放使能 高=开 低=关
#endif

#endif /* APP_GPIO_H_ */
