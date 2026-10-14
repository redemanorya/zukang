/*
 * retarget.c
 *
 *  Created on: Mar 8, 2017
 *      Author: peter
 */

#include <stdio.h>
#include "type.h"
#include "remap.h"
#include "dma.h"
#include "uarts.h"
#include "uarts_interface.h"
#if defined(CFG_APP_CONFIG)
	#include "app_config.h"
	#include "rtos_api.h"
#endif
#include <stdarg.h>
uint32_t inHandlerMode (void);
extern uint8_t DebugPrintPort;
volatile static bool is_uart_switch = FALSE;
volatile static bool s_uart_protocol_mode = FALSE;

#ifdef CFG_FUNC_USBDEBUG_EN
uint8_t 	usb_buffer[4096];
MCU_CIRCULAR_CONTEXT usb_fifo;

void usb_hid_printf_init(void)
{
	MCUCircular_Config(&usb_fifo,usb_buffer,sizeof(usb_buffer));
}

/* Never wait for the USB host. Reserve one byte so R == W remains the
 * unambiguous empty state; excess new log data is dropped. */
static void usb_hid_printf_put(const void *buf, uint32_t len)
{
	int32_t space = MCUCircular_GetSpaceLen(&usb_fifo);
	if((buf == NULL) || (len == 0) || (space <= 1))
	{
		return;
	}
	if(len >= (uint32_t)space)
	{
		len = (uint32_t)space - 1;
	}
	MCUCircular_PutData(&usb_fifo, (void *)buf, len);
}
#endif

void set_uart_switch(bool flag)
{
	is_uart_switch = flag;
}

#if defined(CFG_FUNC_DEBUG_EN) || defined(CFG_FUNC_USBDEBUG_EN)
	/**库printf不支持重入,替换 */
	osMutexId DBGMutex = NULL;

	/**for Buf**/
	__attribute__((used))
	int printf(const char *format, ...)
	{
		va_list arg;
		int done;
		if(s_uart_protocol_mode)
			return 0;
		if(GetIPSR() && (is_uart_switch == FALSE))
			return EOF;
	#ifdef FUNC_OS_EN
		osMutexLock(DBGMutex);
	#endif
		va_start (arg, format);
		done = vprintf (format,  arg);
		va_end (arg);
	#ifdef FUNC_OS_EN
		osMutexUnlock(DBGMutex);
	#endif
		return done;
	}
	__attribute__((used))
	void nds_write(const unsigned char *buf, int size)
	{
		if(s_uart_protocol_mode)
			return;
	#ifdef CFG_UART_LOG_EN
		for (int i = 0; i < size; i++)
		{
			putchar(buf[i]);
		}
	#endif
	#ifdef CFG_FUNC_USBDEBUG_EN
		usb_hid_printf_put(buf, (uint32_t)size);
	#endif
	}

	/**for String**/
	__attribute__((used))
	int	puts(const char * format)
	{
		size_t len = 0;
		if(s_uart_protocol_mode)
			return 0;
		if((format == NULL) || (GetIPSR() && (is_uart_switch == FALSE)))
			return EOF;
	#ifdef FUNC_OS_EN
		osMutexLock(DBGMutex);
	#endif
	#ifdef CFG_FUNC_DEBUG_EN
		while (format[len] != '\0')
		{
			putchar(format[len++]);
		}
		putchar('\n');
	#endif
	#ifdef CFG_FUNC_USBDEBUG_EN
		#ifndef CFG_FUNC_DEBUG_EN
			len = strlen(format);
		#endif
		usb_hid_printf_put(format, (uint32_t)len);
		usb_hid_printf_put("\n", 1);
	#endif
	#ifdef FUNC_OS_EN
		osMutexUnlock(DBGMutex);
	#endif
		return len+1;
	}
#endif

#ifdef CFG_UART_LOG_EN
	uint8_t DebugPrintPort = UART_PORT0;

#ifdef CFG_FUNC_DEBUG_USE_TIMER
	#include "mcu_circular_buf.h"
	static struct
	{
		MCU_CIRCULAR_CONTEXT log_CircularBuf;
		uint8_t log_buf[1024*3];
		volatile bool uart_switch;
	}TimerLog = {0};
	static uint16_t sQueuePrintfLock = 0;

	void UartLogUseTimer(bool enable)
	{
		TimerLog.uart_switch = enable;
	}

	void PrintfAllInBuffer(void)
	{
		UartLogUseTimer(0);
		while(MCUCircular_GetDataLen(&TimerLog.log_CircularBuf))
		{
			uint8_t data;
			MCUCircular_GetData_Printf(&TimerLog.log_CircularBuf,&data,1);
			putchar(data);
		}
	}
	__attribute__((section(".ilm_section")))
	void uart_log_out(void)
	{
		int i;
		volatile const uint32_t  UART_CTRL[3] 	= {0x4003101C,	0x4003201C, 0x4001B01C};
		volatile const uint32_t  UART_DATA[3] 	= {0x40031018,	0x40032018, 0x4001B018};
		volatile const uint32_t  UART_STATUS[3]	= {0x40031014, 	0x40032014, 0x4001B014};

		if(!TimerLog.uart_switch)
			return;
		else
		{
			uint32_t reg_status = *(uint32_t*)UART_STATUS[DebugPrintPort];
			for(i=0;i<4;i++)
			{
				if(reg_status&0x100)//tx_data_int
				{
					*(uint8_t*)UART_CTRL[DebugPrintPort] &= ~4;//tx_int_clr
				}
				if(reg_status&0x200)//tx_mcu_allow
				{
					if(MCUCircular_GetDataLen(&TimerLog.log_CircularBuf))
					{
						uint8_t tab[1];
						MCUCircular_GetData_Printf(&TimerLog.log_CircularBuf,tab,1);
						*(uint32_t*)UART_DATA[DebugPrintPort] = tab[0];
					}
				}
				if(reg_status&0x100)//tx_data_int
				{
					*(uint8_t*)UART_CTRL[DebugPrintPort] &= ~4;//tx_int_clr
				}
			}
		}
	}
#endif
	__attribute__((used))
	int putchar(int c)
	{
#ifdef CFG_FUNC_DEBUG_USE_TIMER
		if(TimerLog.uart_switch)
		{
			if(sQueuePrintfLock)return 0;
			sQueuePrintfLock = 1;
			{
				uint8_t buf[1];
				if(c == '\n')
				{
					buf[0] = '\r';
					if(MCUCircular_GetSpaceLen(&TimerLog.log_CircularBuf )> 1)
					{
						MCUCircular_PutData_Printf(&TimerLog.log_CircularBuf,buf,1);
					}
				}
				buf[0] = c;
				if(MCUCircular_GetSpaceLen(&TimerLog.log_CircularBuf) > 1)
				{
					MCUCircular_PutData_Printf(&TimerLog.log_CircularBuf,buf,1);
				}
			}
			sQueuePrintfLock = 0;
			return c;
		}
#endif

		if (c == '\n')
		{
			UART_SendByte_In_Interrupt(DebugPrintPort, '\r');
			UART_SendByte_In_Interrupt(DebugPrintPort, '\n');
		}
		else
		{
			UART_SendByte_In_Interrupt(DebugPrintPort, (uint8_t)c);
		}

		return c;
	}

	int DbgUartInit(int Which, unsigned int BaudRate, unsigned char DatumBits, unsigned char Parity, unsigned char StopBits)
	{
#ifdef CFG_FUNC_DEBUG_USE_TIMER
		MCUCircular_Config(&TimerLog.log_CircularBuf,TimerLog.log_buf,sizeof(TimerLog.log_buf));
#endif
		DebugPrintPort = Which;
		return UART_Init(Which, BaudRate, DatumBits,  Parity,  StopBits);
	}

	void UartLogEnterProtocolMode(void)
	{
		if(s_uart_protocol_mode)
		{
			return;
		}

	#ifdef FUNC_OS_EN
		osMutexLock(DBGMutex);
	#endif
	#ifdef CFG_FUNC_DEBUG_USE_TIMER
		PrintfAllInBuffer();
	#endif
		/* A UART FIFO byte takes about 87 us at 115200 baud.  Allow the
		 * hardware FIFO to become empty before binary protocol traffic. */
	#ifdef FUNC_OS_EN
		osTaskDelay(3);
	#endif
		s_uart_protocol_mode = TRUE;
	#ifdef FUNC_OS_EN
		osMutexUnlock(DBGMutex);
	#endif
	}
#endif
