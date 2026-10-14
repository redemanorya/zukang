#ifndef __FLASH_BOOT_H__
#define __FLASH_BOOT_H__
/*
版本说明：当前为V5.5.0版本
日期：2026年3月5日
*/
#ifdef CFG_APP_CONFIG
	#include "upgrade.h"
	#include "app_config.h"
//	#include "flash_table.h"
	#include "chip_info.h"
#endif
#define FLASH_CAPACTITY_TYPE			CHIP_FLASH_CAPACTITY //1M		//TODO 定义位置待优化


#define FLASH_BOOT_NONE			0 //无flashboot
#define FLASH_BOOT_BASIC		1 //基础版flashboot
#define FLASH_BOOT_OTA_PRO		2 //Pro版 flashboot

#ifdef CFG_FUNC_BT_OTA_EN
	#define FLASH_BOOT_EN     	FLASH_BOOT_OTA_PRO		//带OTA功能
#else
	#define FLASH_BOOT_EN    	FLASH_BOOT_BASIC
#endif

//需要和debug.h中定义的波特率一一对应
typedef enum __UART_BAUDRATE
{
	BOOT_UART_BAUDRATE_9600 = 0,
	BOOT_UART_BAUDRATE_115200,
	BOOT_UART_BAUDRATE_256000,
	BOOT_UART_BAUDRATE_1000000,
	BOOT_UART_BAUDRATE_2000000,
	BOOT_UART_BAUDRATE_3000000,
}UART_BAUDRATE;


//波特率配置
#define  DEBUG_CONNECT(x, y)			x ## y
#define  DEBUG_STRING_CONNECT(x, y)		DEBUG_CONNECT(x, y)
#define  CFG_BOOT_UART_BANDRATE   		DEBUG_STRING_CONNECT(BOOT_,CFG_UART_BANDRATE)
#define  BOOT_UART_CONFIG				((CFG_FLASHBOOT_UART_EN<<7) + (CFG_BOOT_UART_BANDRATE<<4))
#define  BOOT_UART_DBUG_PORT			CFG_UART_TX_PORT


/*  JUDGEMENT_STANDARD说明
 * 分高4bit与低4bit：
 *   高4bit：
 *      为F则code按版本号升级
 *      为5则按code的CRC进行升级
 *   低4bit:
 *     为F则在升级code需要用到多大空间即擦除多大空间
 *     为5时则标识升级code前全部擦除芯片数据，即擦除“全片”（即除开flash的0地址开始flashboot占用空间不擦除以及最后8K不擦除）
 * 例如：0x5F 则为比较CODE CRC判断是否需要升级；升级时仅部分擦除，不进行全擦除
 */
#define JUDGEMENT_STANDARD		0x55

#ifdef CFG_RES_CARD_GPIO
#define SDIO_UP_CONFIG				CFG_RES_CARD_GPIO
#else
#define SDIO_UP_CONFIG				0
#endif

#ifdef CFG_FUNC_BT_OTA_EN
	#define UP_PORT				(CHN_MASK_USBCDC + CHN_MASK_BLE) //根据应用情况决定打开那些升级接口
#else
	#define UP_PORT				(CHN_MASK_USBCDC + CHN_MASK_SDCARD + CHN_MASK_UDISK + CHN_MASK_UDISK_FS + CHN_MASK_USBCDC_FS) //根据应用情况决定打开那些升级接口
#endif


typedef enum __UART_UP
{
	// UART0 PORT
	UART0_PORT_A0_A1 = 0,     // RX-GPIOA0, TX-GPIOA1
	UART0_PORT_A10_A11,       // RX-GPIOA10, TX-GPIOA11
	UART0_PORT_A20_A21,       // RX-GPIOA20, TX-GPIOA21
	UART0_PORT_A30_A31,       // RX-GPIOA30, TX-GPIOA31
	UART0_PORT_B8_B9,         // RX-GPIOB8, TX-GPIOB9
	UART0_PORT_B18_B19,       // RX-GPIOB18, TX-GPIOB19
	UART0_PORT_A8_A9,         // RX-GPIOA8, TX-GPIOA9

	// UART1 PORT
	UART1_PORT_A4_A5,         // RX-GPIOA4, TX-GPIOA5
	UART1_PORT_A14_A15,       // RX-GPIOA14, TX-GPIOA15
	UART1_PORT_A24_A25,       // RX-GPIOA24, TX-GPIOA25
	UART1_PORT_B2_B3,         // RX-GPIOB2, TX-GPIOB3
	UART1_PORT_B12_B13,       // RX-GPIOB12, TX-GPIOB13

	// UART2 PORT
	UART2_PORT_A8_A9,         // RX-GPIOA8, TX-GPIOA9
	UART2_PORT_A18_A19,       // RX-GPIOA18, TX-GPIOA19
	UART2_PORT_A28_A29,       // TX-GPIOA28, RX-GPIOA29
	UART2_PORT_B6_B7,         // RX-GPIOB6, TX-GPIOB7
	UART2_PORT_B16_B17,       // RX-GPIOB16, TX-GPIOB17
	UART2_PORT_B20_B21,       // RX-GPIOB20, TX-GPIOB21
}UART_UP_PORT;

typedef enum __UART_UP_BAUDRATE
{
	UART_UP_BAUDRATE_115200 = 0,
	UART_UP_BAUDRATE_256000,
	UART_UP_BAUDRATE_1000000,
	UART_UP_BAUDRATE_2000000,
}UART_UP_BAUDRATE;

/* UART_UP_CONFIG 说明
	bit[4:0]:端口配置
		参考 UART_UP_PORT 枚举参数
	bit[7:5]: 波特率配置
		参考 UART_BAUDRATE 枚举参数
*/
#define CFG_UART_UP_PORT		UART2_PORT_A8_A9			////打印串口和升级串口不能使用同一组。
#define CFG_UART_UP_BAUD_RATE	UART_UP_BAUDRATE_115200
#define UART_UP_CONFIG			((CFG_UART_UP_BAUD_RATE<<5)+CFG_UART_UP_PORT)

#define SPIMFLASH_UP_CONFIG 	0		//TODO	待实现

/*  BOOT_SYSTEM_CONFIGURATIOM 说明
	bit[1:0]:系统时钟配置
		2’b00: HSE OSC 24M
		2’b01: RC_32K
	bit[3:2]:代码校验模式配置
		2’b00: SYMBOL_CHECK,			//中断向量表+特征校验
		2’b01: CRC_PART_CODE_CHECK,		//CRC部分  user code 校验
		2’b10: CRC_ALL_CODE_CHECK,		//CRC全部  user code 校验
	bit[7:4]:保留
 */
typedef enum {
	BOOT_CLK_HSE_OSC_24M,						//外部24m晶体
	BOOT_CLK_RC_32K,								//内部RC32K
}BOOT_CLK_MODE;

typedef enum {
	SYMBOL_CHECK,						//中断向量表+特征校验
	CRC_PART_CODE_CHECK,				//CRC部分  user code 校验
	CRC_ALL_CODE_CHECK,					//CRC全部  user code 校验
}UPGRAD_CHECK_MODE;


#ifdef USB_CRYSTA_FREE_EN
#define BOOT_CLK 	BOOT_CLK_RC_32K
#else
#define BOOT_CLK 	BOOT_CLK_HSE_OSC_24M			//外部24m晶体
#endif

#define CODE_CHECK 	CRC_ALL_CODE_CHECK	//CRC全部  user code 校验

#define BOOT_SYSTEM_CONFIGURATIOM		((CODE_CHECK<<2) + BOOT_CLK)

#define CFG_SDK_MAGIC_NUMBER            (0xB0BEBDC9)

#define COMPILE_IS_UP_PORT_ENABLED(chn_mask)  (((UP_PORT) & (chn_mask)) ? 1 : 0)
#endif

