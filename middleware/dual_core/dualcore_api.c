/*
 * dualcore_api.c
 *
 *  Created on: Sep 28, 2025
 *      Author: jiaozhenqiang
 */
#include <stdlib.h>
#include <string.h>
#include "gpio.h"
#include "uarts.h"
#include "debug.h"
#include "irqn.h"
#include "delay.h"
#include "dualcore_api.h"

#ifdef CFG_APP_CONFIG
	#include "app_config.h"
#endif

#ifdef DUAL_CORE_EN

//this code should be called from core1 only
void UartInit_Core2(void)
{
	if (DualCore_IsCurCore1())
	{
#if (CORE2_UART == UART2_TA19_RA18)
		GPIO_PortAModeSet(GPIOA18, 4);//UART2 Rx
		GPIO_PortAModeSet(GPIOA19, 10);//UART2 Tx
		UART_Init(2, 2000000, 8, 0, 1);
#elif (CORE2_UART == UART2_TA29_RA28)
		GPIO_PortAModeSet(GPIOA28, 1);//UART2 Rx
		GPIO_PortAModeSet(GPIOA29, 1);//UART2 Tx
		UART_Init(2, 2000000, 8, 0, 1);
#elif (CORE2_UART == UART2_TA9_RA8)
		GPIO_PortAModeSet(GPIOA8, 1);//UART2 Rx
		GPIO_PortAModeSet(GPIOA9, 2);//UART2 Tx
		UART_Init(2, 2000000, 8, 0, 1);
#elif (CORE2_UART == UART2_TB7_RB6)
		GPIO_PortBModeSet(GPIOB6, 3);//UART2 Rx
		GPIO_PortBModeSet(GPIOB7, 8);//UART2 Tx
		UART_Init(2, 2000000, 8, 0, 1);
#elif (CORE2_UART == UART2_TB21_RB20)
		GPIO_PortBModeSet(GPIOB20, 1);//UART2 Rx
		GPIO_PortBModeSet(GPIOB21, 2);//UART2 Tx
		UART_Init(2, 2000000, 8, 0, 1);
#endif
	}
}

void DualCoreModuleInit(void)
{
	//IPC init
	IpcInit();

	//HSEM init
	HsemInit();

	//enable interrupt
	if (DualCore_IsCurCore1())
	{
		NVIC_EnableIRQ(IPC_HSEMINT0_IRQn);
		NVIC_EnableIRQ(PLMT_IRQn);
	}
	else
	{
		NVIC_EnableIRQ(IPC_C2_IRQn);
		NVIC_EnableIRQ(HSEM_C2_IRQn);
		NVIC_EnableIRQ(PLMT_C2_IRQn);
	}
}

#endif
