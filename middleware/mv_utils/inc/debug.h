////////////////////////////////////////////////////////////////////////////////
//                   Mountain View Silicon Tech. Inc.
//		Copyright 2011, Mountain View Silicon Tech. Inc., ShangHai, China
//                   All rights reserved.
//
//		Filename	:debug.h
//
//		Description	:
//					Define debug ordinary print & debug routine
//
//		Changelog	:
///////////////////////////////////////////////////////////////////////////////

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h>
#include <string.h>
#include "type.h"
#include "gpio.h"

/******
 * 消警告格式化 ：
 * int64_t	-- %lld
 * int32_t 	-- %ld
 * int16_t 	-- %d
 * int8_t 	-- %d
 * uint64_t	-- %lld、%llu、0x%llx
 * uint32_t -- %ld、%lu、0x%lx
 * uint16_t	-- %d、%u、0x%x
 * uint8_t 	-- %d、%u、0x%x
 */
#ifndef DBG_MACRO
	#define DBG_MACRO(format, ...)	printf(format, ##__VA_ARGS__)
#endif

#define	APP_DBG(format, ...)	DBG_MACRO(format, ##__VA_ARGS__)
#define	DBG(format, ...)		DBG_MACRO(format, ##__VA_ARGS__)
#define	OTG_DBG(format, ...)	//DBG_MACRO(format, ##__VA_ARGS__)
#define	BT_DBG(format, ...)		DBG_MACRO(format, ##__VA_ARGS__)
#define	BT2_DBG(format, ...)	//DBG_MACRO(format, ##__VA_ARGS__)	//BT debug for mv

int DbgUartInit(int Which, unsigned int BaudRate, unsigned char DatumBits, unsigned char Parity, unsigned char StopBits);

//通过DMA打印，节省打印时间，需要在DbgUartInit之前调用
void EnableDmaPrint(void);

/* Flush boot logs and permanently hand UART1 TX to the binary protocol. */
void UartLogEnterProtocolMode(void);


/************************** TOGGLE DEBUG**********************************
 *
 * 调试时序，debug.c/.h提供IO toggle配置序列
 * 1. DBG_IO_TGL TGL_PORT_LIST为映射序列0~n，
 * 2、TGL_OFFSET OS或Int toggle序列偏移起始编号，下文编号为X
 * 3、INT_TGL 中断进出时间片, 占用TGL_INT_LIST + 1序号，led X ~ X+n
 * 4、TASK_TGL Os任务进出时间片，TGL_TASK_LIST名字表为序号，排在INT_TGL之后
 * 测试IO,常用于程序执行时序可视化，标准SDK不使用
 * 输出高低电平，上升沿，下降沿，翻转等
 * 调用DbgPortInit初始化端口配置
 *************************************************************/
//#define DBG_IO_TGL //使用IO toggle调试 逻分观测 配合 DbgPortInit()

#ifdef DBG_IO_TGL
	void DbgPortInit(void);
	void DbgPortOn(uint8_t Index);//是否生效取决于TGL_PORT_LIST
	void DbgPortOff(uint8_t Index);
	void DbgPortToggle(uint8_t Index);
	void DbgPortRise(uint8_t Index);
	void DbgPortDown(uint8_t Index);
	bool DbgPortGet(uint8_t Index);//TRUE:高电平，FALSE:低电平，index无效时 默认FALSE

	/**启动对应端口和序号 0~n-1，规避端口复用**/
	#define TGL_PORT_LIST	{RES_GPIOA0, RES_GPIOA1, RES_GPIOA2, RES_GPIOA3, RES_GPIOA4, RES_GPIOA5, RES_GPIOA6, RES_GPIOA7, RES_GPIOA8, RES_GPIOA9, RES_GPIOA10, RES_GPIOA11, RES_GPIOA12, RES_GPIOA13, RES_GPIOA14, RES_GPIOA15}
	/**,RES_GPIOA16, RES_GPIOA17, RES_GPIOA18, RES_GPIOA19, RES_GPIOA20, RES_GPIOA21, RES_GPIOA22, RES_GPIOA23, RES_GPIOA24, RES_GPIOA25, RES_GPIOA26, RES_GPIOA27, RES_GPIOA28, RES_GPIOA29, RES_GPIOA30, RES_GPIOA31**/
	#define	TGL_OFFSET			0//INT_TGL或TASK_TGL起始序列编号,即X配置，基于TGL_PORT_LIST,

	/**中断进出Toggle 表征执行时间片
	 * 	修改OS_Trap_Int_Comm @crt0.S
		!TGL_INT_COM
		jral $r1
		改为
		TGL_INT_COM
		!jral $r1
	 ********************************/
//	#define INT_TGL

	#ifdef INT_TGL
		#include "irqn.h"
		/**中断号toggle序列，其他中断用后续一IO
		 * 序列不为空，示例B5,参见enum IRQn_Type @irqn.h
		 * ***************************/
		#define	TGL_INT_LIST	{USB_HS_IRQn}//, USB_FS_WAKEUP_IRQn, BT_IRQn
		void dbg_int_in(uint32_t int_num);
		void dbg_int_out(uint32_t int_num);
	#endif

	/**OS任务切换IO toggle，表征执行时间片
	 * 开INT_TGL时，排在TGL_INT_LIST + 1之后
	 *******************************/
//	#define TASK_TGL

	#ifdef TASK_TGL
		/**toggle的task Name头部字符串;序列不为空，占用TGL_TASK_LIST对应端口数**/
		#define TGL_TASK_LIST	{{"AudioCore"}, {"EffProc"}, {"SysMode"}, {"MainApp"}, {"IDLE"}}//{"BtStack"},
		void DbgTaskTGL_in();
		void DbgTaskTGL_out();
	#endif

	#ifdef CFG_APP_CONFIG
		#include "resource_defs.h"
	#else
		/****************************************Uart*******************************************/
		#define RES_UARTX_0							0
		#define RES_UARTX_1							1
		#define RES_UARTX_2							2

		#define SET_UARTIO_VAL(uart_port,gpio_mode,gpio_port,gpio_index)	\
													((uart_port<<24)|(gpio_mode<<16)|(gpio_port<<8)|(gpio_index))
		/***Param Val:RES_UART_TX_???; Return:RES_UARTX_?***/
		#define GET_UARTIO_UARTX(val)				((val>>24)&0x03)
		#define GET_UARTIO_MODE(val)				((val>>16)&0xff)
		/***Param Val:RES_UART_TX_???; Return:RES_PORT_A or RES_PORT_B***/
		#define GET_UARTIO_PORT(val)				((val>>8)&0x01)
		/***Param Val:RES_UART_TX_???; Return:IO index****/
		#define GET_UARTIO_INDEX(val)				((val)&0x1f)
		/***Param Val:RES_UART_TX_???; Return:@GPIO_INDEX / GPIO_PortA / GPIO_PortB****/
		#define GET_UARTIO_PIN(val)					(1<<((val)&0x1f))


		/*B6所有IO口均支持UART0 TX，建议作为Log端口，下列不符合需求时，自行扩展@gpio.h*/
																  //UART port, 复用    , 端口Bank,	IO Index
		#define RES_UART0_TX_A5						SET_UARTIO_VAL(RES_UARTX_0, 0x0A, RES_PORT_A, 5)
		#define RES_UART2_TX_A9  					SET_UARTIO_VAL(RES_UARTX_2, 0x02, RES_PORT_A, 9)
		#define RES_UART0_TX_A15  					SET_UARTIO_VAL(RES_UARTX_0, 0x0E, RES_PORT_A, 15)
		#define RES_UART0_TX_A19  					SET_UARTIO_VAL(RES_UARTX_0, 0x0B, RES_PORT_A, 19)
		#define RES_UART0_TX_B6  					SET_UARTIO_VAL(RES_UARTX_0, 0x09, RES_PORT_B, 6)

		enum
		{
			UART_BAUDRATE_9600 						= 9600,
			UART_BAUDRATE_115200 					= 115200,
			UART_BAUDRATE_256000					= 256000,
			UART_BAUDRATE_1000000					= 1000000,
			UART_BAUDRATE_2000000					= 2000000,
			UART_BAUDRATE_3000000 					= 3000000,
		};

		/***********************************GPIO******************************************************/
		#define RES_PORT_A							0
		#define RES_PORT_B							1

		#define SET_IO_VAL(gpio_port,gpio_index)	((gpio_port<<8)|(gpio_index))
		/***Param Val:RES_GPIO???; Return:RES_PORT_A or RES_PORT_B***/
		#define GET_IO_PORT(val)					((val>>8)&0x01)
		/***Param Val:RES_GPIO???; Return:IO index****/
		#define GET_IO_INDEX(val)					((val)&0x1f)
		/***Param Val:RES_GPIO???; Return:@GPIO_INDEX / GPIO_PortA / GPIO_PortB****/
		#define GET_IO_PIN(val)						(1<<((val)&0x1f))

		#define RES_GPIOA0							SET_IO_VAL(RES_PORT_A, 0)
		#define RES_GPIOA1							SET_IO_VAL(RES_PORT_A, 1)
		#define RES_GPIOA2							SET_IO_VAL(RES_PORT_A, 2)
		#define RES_GPIOA3							SET_IO_VAL(RES_PORT_A, 3)
		#define RES_GPIOA4							SET_IO_VAL(RES_PORT_A, 4)
		#define RES_GPIOA5							SET_IO_VAL(RES_PORT_A, 5)
		#define RES_GPIOA6							SET_IO_VAL(RES_PORT_A, 6)
		#define RES_GPIOA7							SET_IO_VAL(RES_PORT_A, 7)
		#define RES_GPIOA8							SET_IO_VAL(RES_PORT_A, 8)
		#define RES_GPIOA9							SET_IO_VAL(RES_PORT_A, 9)
		#define RES_GPIOA10							SET_IO_VAL(RES_PORT_A, 10)
		#define RES_GPIOA11							SET_IO_VAL(RES_PORT_A, 11)
		#define RES_GPIOA12							SET_IO_VAL(RES_PORT_A, 12)
		#define RES_GPIOA13							SET_IO_VAL(RES_PORT_A, 13)
		#define RES_GPIOA14							SET_IO_VAL(RES_PORT_A, 14)
		#define RES_GPIOA15							SET_IO_VAL(RES_PORT_A, 15)
		#define RES_GPIOA16							SET_IO_VAL(RES_PORT_A, 16)
		#define RES_GPIOA17							SET_IO_VAL(RES_PORT_A, 17)
		#define RES_GPIOA18							SET_IO_VAL(RES_PORT_A, 18)
		#define RES_GPIOA19							SET_IO_VAL(RES_PORT_A, 19)
		#define RES_GPIOA20							SET_IO_VAL(RES_PORT_A, 20)
		#define RES_GPIOA21							SET_IO_VAL(RES_PORT_A, 21)
		#define RES_GPIOA22							SET_IO_VAL(RES_PORT_A, 22)
		#define RES_GPIOA23							SET_IO_VAL(RES_PORT_A, 23)
		#define RES_GPIOA24							SET_IO_VAL(RES_PORT_A, 24)
		#define RES_GPIOA25							SET_IO_VAL(RES_PORT_A, 25)
		#define RES_GPIOA26							SET_IO_VAL(RES_PORT_A, 26)
		#define RES_GPIOA27							SET_IO_VAL(RES_PORT_A, 27)
		#define RES_GPIOA28							SET_IO_VAL(RES_PORT_A, 28)
		#define RES_GPIOA29							SET_IO_VAL(RES_PORT_A, 29)
		#define RES_GPIOA30							SET_IO_VAL(RES_PORT_A, 30)
		#define RES_GPIOA31							SET_IO_VAL(RES_PORT_A, 31)
		#define RES_GPIOB0							SET_IO_VAL(RES_PORT_B, 0)
		#define RES_GPIOB1							SET_IO_VAL(RES_PORT_B, 1)
		#define RES_GPIOB2							SET_IO_VAL(RES_PORT_B, 2)
		#define RES_GPIOB3							SET_IO_VAL(RES_PORT_B, 3)
		#define RES_GPIOB4							SET_IO_VAL(RES_PORT_B, 4)
		#define RES_GPIOB5							SET_IO_VAL(RES_PORT_B, 5)
		#define RES_GPIOB6							SET_IO_VAL(RES_PORT_B, 6)
		#define RES_GPIOB7							SET_IO_VAL(RES_PORT_B, 7)
		#define RES_GPIOB8							SET_IO_VAL(RES_PORT_B, 8)
		#define RES_GPIOB9							SET_IO_VAL(RES_PORT_B, 9)
		#define RES_GPIOB10							SET_IO_VAL(RES_PORT_B, 10)
		#define RES_GPIOB11							SET_IO_VAL(RES_PORT_B, 11)
		#define RES_GPIOB12							SET_IO_VAL(RES_PORT_B, 12)
		#define RES_GPIOB13							SET_IO_VAL(RES_PORT_B, 13)
		#define RES_GPIOB14							SET_IO_VAL(RES_PORT_B, 14)
		#define RES_GPIOB15							SET_IO_VAL(RES_PORT_B, 15)
		#define RES_GPIOB16							SET_IO_VAL(RES_PORT_B, 16)
		#define RES_GPIOB17							SET_IO_VAL(RES_PORT_B, 17)
		#define RES_GPIOB18							SET_IO_VAL(RES_PORT_B, 18)
		#define RES_GPIOB19							SET_IO_VAL(RES_PORT_B, 19)
		#define RES_GPIOB20							SET_IO_VAL(RES_PORT_B, 20)
		#define RES_GPIOB21							SET_IO_VAL(RES_PORT_B, 21)
		#define RES_GPIOB22							SET_IO_VAL(RES_PORT_B, 22)
		#define RES_GPIOB23							SET_IO_VAL(RES_PORT_B, 23)
		#define RES_GPIOB24							SET_IO_VAL(RES_PORT_B, 24)
		#define RES_GPIOB25							SET_IO_VAL(RES_PORT_B, 25)
		#define RES_GPIOB26							SET_IO_VAL(RES_PORT_B, 26)
		#define RES_GPIOB27							SET_IO_VAL(RES_PORT_B, 27)

		/**param X:RES_GPIO???; Return:Registor Index@gpio.h***/
		#define PORT_IN_REG(X)						(GET_IO_PORT(X) ? GPIO_B_IN : GPIO_A_IN)
		#define PORT_OUT_REG(X)						(GET_IO_PORT(X) ? GPIO_B_OUT : GPIO_A_OUT)
		#define PORT_SET_REG(X)						(GET_IO_PORT(X) ? GPIO_B_SET : GPIO_A_SET)
		#define PORT_CLR_REG(X)						(GET_IO_PORT(X) ? GPIO_B_CLR : GPIO_A_CLR)
		#define PORT_TGL_REG(X)						(GET_IO_PORT(X) ? GPIO_B_TGL : GPIO_A_TGL)
		#define PORT_IE_REG(X)						(GET_IO_PORT(X) ? GPIO_B_IE : GPIO_A_IE)
		#define PORT_OE_REG(X)						(GET_IO_PORT(X) ? GPIO_B_OE : GPIO_A_OE)
		#define PORT_DS_REG(X)						(GET_IO_PORT(X) ? GPIO_B_DS : GPIO_A_DS)
		#define PORT_PU_REG(X)						(GET_IO_PORT(X) ? GPIO_B_PU : GPIO_A_PU)
		#define PORT_PD_REG(X)						(GET_IO_PORT(X) ? GPIO_B_PD : GPIO_A_PD)
		#define PORT_ANA_REG(X)						(GET_IO_PORT(X) ? GPIO_B_ANA_EN : GPIO_A_ANA_EN)
		#define PORT_PULLDOWN_REG(X)				(GET_IO_PORT(X) ? GPIO_B_PULLDOWN : GPIO_A_PULLDOWN)
		#define PORT_CORE_REG(X)					(GET_IO_PORT(X) ? GPIO_B_CORE_OUT_MASK : GPIO_A_CORE_OUT_MASK)
		#define PORT_DMA_REG(X)						(GET_IO_PORT(X) ? GPIO_B_DMA_OUT_MASK : GPIO_A_DMA_OUT_MASK)
		#define SET_PIN_MODE(X, Mode)				(GET_IO_PORT(X) ? (GPIO_PortBModeSet(GET_IO_PIN(X), (Mode))) : GPIO_PortAModeSet(GET_IO_PIN(X), (Mode)))
		#define PIN_MODE_GET(X)						(GET_IO_PORT(X) ? GPIO_PortBModeGet(GET_IO_PIN(X)： GPIO_PortAModeGet(GET_IO_PIN(X)))
	#endif
#endif //DBG_IO_TGL



#define SINE_48_MONO																										{	\
0x0001,	0x085b,	0x1090,	0x187e,	0x2000,	0x26f6,	0x2d41,	0x32c6, 0x376d,	0x3b21,	0x3dd2,	0x3f74,	0x4000,	0x3f74,	0x3dd2,	0x3b21,	\
0x376d,	0x32c6,	0x2d41,	0x26f6,	0x2000,	0x187e,	0x1090,	0x085b,	0x0000,	0xf7a5,	0xef70,	0xe782,	0xe000,	0xd90a,	0xd2bf,	0xcd3a,	\
0xc893,	0xc4df,	0xc22e,	0xc08c,	0xc000,	0xc08c,	0xc22e,	0xc4df,	0xc893,	0xcd3a,	0xd2bf,	0xd90a,	0xe000,	0xe782,	0xef70,	0xf7a5};

#define SINE_32_0DB_MONO 																								{	\
0,	3204,	6284,	9123,	11612,	13655,	15173,	16107,	16422,	16107,	15173,	13655,	11612,	9124,	6285,	3203,	\
0,	-3204,	-6285,	-9124,	-11612,	-13655,	-15173,	-16107,	-16422,	-16107,	-15172,	-13655,	-11613,	-9123,	-6285,	-3204}

#define SINE_256_MONO																										{	\
0,		320,	640,	959,	1279,	1597,	1914,	2231,	2546,	2858,	3170,	3479,	3787,	4092,	4395,	4695,	\
4992,	5286,	5578,	5865,	6149,	6430,	6707,	6978,	7248,	7511,	7771,	8025,	8276,	8521,	8761,	8994,	\
9224,	9448,	9666,	9878,	10084,	10285,	10478,	10665,	10846,	11021,	11189,	11350,	11505,	11652,	11792,	11926,	\
12052,	12171,	12282,	12386,	12483,	12572,	12654,	12728,	12794,	12853,	12904,	12947,	12982,	13009,	13029,	13040,	\
13045,	13041,	13029,	13009,	12982,	12947,	12904,	12853,	12794,	12728,	12654,	12571,	12482,	12385,	12282,	12171,	\
12052,	11926,	11792,	11651,	11504,	11349,	11188,	11021,	10846,	10664,	10476,	10282,	10082,	9876,	9664,	9446,	\
9222,	8994,	8758,	8519,	8273,	8024,	7769,	7509,	7245,	6977,	6704,	6427,	6146,	5862,	5575,	5283,	\
4989,	4692,	4392,	4089,	3783,	3476,	3166,	2855,	2541,	2226,	1910,	1593,	1275,	955,	636,	316,	\
-4,		-324,	-644,	-964,	-1283,	-1601,	-1919,	-2234,	-2549,	-2862,	-3173,	-3484,	-3792,	-4096,	-4400,	-4700,	\
-4997,	-5292,	-5582,	-5869,	-6154,	-6435,	-6711,	-6984,	-7253,	-7516,	-7776,	-8031,	-8280,	-8525,	-8766,	-9000,	\
-9229,	-9452,	-9670,	-9882,	-10088,	-10288,	-10482,	-10670,	-10850,	-11025,	-11193,	-11354,	-11508,	-11654,	-11796,	-11929,	\
-12056,	-12174,	-12285,	-12389,	-12486,	-12575,	-12656,	-12729,	-12796,	-12854,	-12905,	-12948,	-12982,	-13010,	-13030,	-13040,	\
-13044,	-13040,	-13028,	-13008,	-12981,	-12945,	-12903,	-12850,	-12792,	-12725,	-12651,	-12569,	-12480,	-12383,	-12279,	-12167,	\
-12047,	-11921,	-11788,	-11647,	-11499,	-11344,	-11183,	-11014,	-10840,	-10658,	-10471,	-10276,	-10076,	-9869,	-9657,	-9439,	\
-9216,	-8986,	-8751,	-8510,	-8265,	-8015,	-7760,	-7500,	-7236,	-6967,	-6694,	-6418,	-6138,	-5853,	-5564,	-5274,	\
-4979,	-4682,	-4381,	-4078,	-3773,	-3464,	-3155,	-2843,	-2530,	-2214,	-1899,	-1581,	-1263,	-944,	-624,	-304,	}

//#define AUDIO_SINE_TEST_EN

#ifdef AUDIO_SINE_TEST_EN
	#define TEST_PERIOD		256//48			//32
	#define	TEST_DATA		SINE_256_MONO//SINE_48_MONO //SINE_32_0DB_MONO//
	extern int16_t SineMono[TEST_PERIOD];
	extern uint32_t FrameOffset;
	//示例：SineMono[FrameOffset++%TEST_PERIOD];
#endif//AUDIO_SINE_TEST_EN

#endif //__DBG_H__ 

