#include <stdio.h>
#include <nds32_intrinsic.h>
#include <string.h>
#include "type.h"
#include "uarts_interface.h"
#include "debug.h"
#include "irqn.h"
#include "FreeRTOS.h"
#include "task.h"

#include "shell.h"
#include "mode_task.h"

#ifdef CFG_APP_CONFIG
	#include "app_config.h"
	#if 0//GET_DEBUG_GPIO_UARTPORT(CFG_FUNC_SHELL_RX_PORT) //for B5x sdk  //Port
		#define CFG_PARA_SHELL_COM1
	#else
		/* [MOD] 原为 #define CFG_PARA_SHELL_COM0 (shell 走 UART0)。
		 * 现改走 UART1(A14 RX / A15 TX), 与日志口 CFG_UART_RX/TX_PORT 保持一致:
		 * 不定义 CFG_PARA_SHELL_COM0, 下面 #ifdef 即进入 UART1 分支。
		 * 双核(DualCore)下中断只能用 UART_IRQn / UART_Interrupt 覆盖 weak 向量;
		 * UART0_IRQn 在双核不存在, 保留 COM0 会编译失败。 */
		//#define CFG_PARA_SHELL_COM0
	#endif
#else
	#define CFG_PARA_SHELL_COM1
	//#define CFG_PARA_SHELL_COM0
#endif

#ifdef CFG_FUNC_SHELL_EN

#define BUFSIZE		32
uint8_t	UARTBuffer[BUFSIZE];
uint32_t	gRxQueueHead = 0;
uint32_t	gRxQueueTail = 0;

#define CMD_ARG_MAX_COUNT	3
#define ONE_ARG_MAX_LEN		10
char cmdbuffer[CMD_ARG_MAX_COUNT][ONE_ARG_MAX_LEN];

//多出ONE_ARG_MAX_LEN,目的有参数之间的间隔' '，还有cmd长度可以大于ONE_ARG_MAX_LEN,因此多出ONE_ARG_MAX_LEN长度预留
uint8_t shellbuffer[(CMD_ARG_MAX_COUNT+1)*ONE_ARG_MAX_LEN];

/**
 * @func        char_to_num
 * @brief       Bcd字符串转换成数值
 * @param       char *buffer：不超过10个数字字符
 * @Output      None
 * @return      int
 * @Others      比如“1234567890” 返回1234567890数值。
 * Record
 */
int char_to_num(char *buffer)
{
    int i;
    int templen;
    int sum = 0;
    templen = strlen(buffer);
    if(templen > 10)
    {
        DBG("CMD ERROR\n");
        return -1;
    }
    for(i=0;i<templen;i++)
    {
        buffer[i] -= 0x30;
        if((buffer[i]>=0)&&(buffer[i]<=9))
        {
        	//DBG("%x",buffer[i]);
        	sum *= 10;
        	sum += buffer[i];
        }
        else
        {
        	DBG("ERROR");
            return -1;
        }
    }

    return sum;
}

uint8_t shell_getchar(void)
{
	if(gRxQueueHead != gRxQueueTail)//非空
	{
		unsigned char	val;
		val = UARTBuffer[gRxQueueHead];
		gRxQueueHead = ((gRxQueueHead + 1) % BUFSIZE);
		return val;
	}
	return 0;
}
#ifdef CFG_PARA_SHELL_COM0
void UART0_Interrupt(void)
{
	if(UARTS_IOCTL(UART_PORT0, UART_IOCTL_RXSTAT_GET, 1) & 0x01) { //接收中断
		int i;
		uint8_t Temp[1];
		for(i=0;i<4;i++)
		{
			if(UARTS_RecvByte(UART_PORT0,Temp) == 0)
			{
				break;
			}
			else
			{
				UARTBuffer[gRxQueueTail] = Temp[0];
				if (((gRxQueueTail + 1) % BUFSIZE) != gRxQueueHead)		//queue is not full
				{
					gRxQueueTail = ((gRxQueueTail + 1) % BUFSIZE);			//queue tail++
				}
			}
		}
		UARTS_IOCTL(UART_PORT0, UART_IOCTL_RXINT_CLR, 1);
	}
    if(UARTS_IOCTL(UART_PORT0, UART_IOCTL_RXSTAT_GET, 0) & 0x3C) { //错误中断
        //kprintf("Uart0 RX error(0x%02X)\n", (uint32_t)(UARTS_IOCTL(UART_PORT0, UART_IOCTL_RXSTAT_GET, 0) & 0x3C));
        UARTS_IOCTL(UART_PORT0, UART_IOCTL_RXINT_CLR, 1);
    }

//    if(UARTS_IOCTL(UART_PORT0, UART_IOCTL_TXSTAT_GET, 0)) { //发送中断
//        UARTS_IOCTL(UART_PORT0, UART_IOCTL_TXINT_CLR, 1);
//    }
}

#else
#ifdef DUAL_CORE_EN
void UART_Interrupt(void)
#else
void UART1_Interrupt(void)
#endif
{
	if(UARTS_IOCTL(UART_PORT1, UART_IOCTL_RXSTAT_GET, 1) & 0x01) { //接收中断
		int i;
		uint8_t Temp[1];
		for(i=0;i<4;i++)
		{
			if(UARTS_RecvByte(UART_PORT1,Temp) == 0)
			{
				break;
			}
			else
			{
				UARTBuffer[gRxQueueTail] = Temp[0];
				if (((gRxQueueTail + 1) % BUFSIZE) != gRxQueueHead)		//queue is not full
				{
					gRxQueueTail = ((gRxQueueTail + 1) % BUFSIZE);			//queue tail++
				}
			}
		}
		UARTS_IOCTL(UART_PORT1, UART_IOCTL_RXINT_CLR, 1);
	}
    if(UARTS_IOCTL(UART_PORT1, UART_IOCTL_RXSTAT_GET, 0) & 0x3C) { //错误中断
        //kprintf("Uart0 RX error(0x%02X)\n", (uint32_t)(UARTS_IOCTL(UART_PORT0, UART_IOCTL_RXSTAT_GET, 0) & 0x3C));
        UARTS_IOCTL(UART_PORT1, UART_IOCTL_RXINT_CLR, 1);
    }

//    if(UARTS_IOCTL(UART_PORT1, UART_IOCTL_TXSTAT_GET, 0)) { //发送中断
//        UARTS_IOCTL(UART_PORT1, UART_IOCTL_TXINT_CLR, 1);
//    }
}
#endif
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void cmd_test(int argc, char *argv[]);
void cmd_src_switch(int argc, char *argv[]);
static void DisplayMemInf(void);
static void DisplayRunTimeStatesInf(void);
static void DisplayTaskInf(void);
static void Displayhelp(void);
extern void clear_net_cfg(void);
extern void creat_wifi_connect_task(void);
extern void edp_task_start();
extern void mqtt_run();
extern void DisplayMcpsInfo(void);
shell_cmds mv_shell[] =
{
    { "test",   				(shell_cmd_func)cmd_test,       			"for test"},
    { "displaytaskinfo",   		(shell_cmd_func)DisplayTaskInf,       		"show task info"},
    { "displayruntimestats",   	(shell_cmd_func)DisplayRunTimeStatesInf,	"show run time states"},
    { "src",                                      (shell_cmd_func)cmd_src_switch,                 "source switch: src bt | src linein"},
    { "DisplayMemInf",   		(shell_cmd_func)DisplayMemInf,				"show heap memory size"},
    { "help",   				(shell_cmd_func)Displayhelp,				"help"},
    { NULL, NULL, NULL },
};

void cmd_test(int argc, char *argv[])
{
	int i;
	for(i=0;i<argc;i++)
	{
		if(i==0)
		{
			DBG("%s\n",argv[i]);
		}
		else
		{
			DBG("%d\n",char_to_num(argv[i]));
		}
	}
}


//static
void DisplayMemInf(void)
{
	DBG("\nRemainingMemoryInBytes: %d\r\n",(int)xPortGetFreeHeapSize());
}

static void DisplayRunTimeStatesInf(void)
{
	uint8_t *buf= pvPortMalloc(4096);
	memset(buf,0,4096);
	vTaskGetRunTimeStats((char*)buf);
	DBG("\nTask           \tAbs Time\tPercent\r\n****************************************\r\n");
	DBG("%s\n",buf);
	vPortFree(buf);
}

void cmd_src_switch(int argc, char *argv[])
{
	if(argc < 2)
	{
		DBG("src usage: src bt | src linein\n");
		return;
	}
	if(argv[1][0] == 'b' || argv[1][0] == 'B')   /* bt */
	{
		DBG("switch source -> BT\n");
		SysModeEnter(ModeBtAudioPlay);
	}
	else if(argv[1][0] == 'l' || argv[1][0] == 'L')  /* linein */
	{
		DBG("switch source -> LineIn\n");
		SysModeEnter(ModeLineAudioPlay);
	}
	else
	{
		DBG("src unknown arg: %s (use bt or linein)\n", argv[1]);
	}
}

static void DisplayTaskInf(void)
{
	uint8_t *buf= pvPortMalloc(4096);
	memset(buf,0,4096);
	vTaskList((char*)buf);
	DBG("\nTask           State  Prio+3  FreeStack(word)  PID\n***************************************************\r\n");
	DBG("%s\n",buf);
	vPortFree(buf);
}


static void Displayhelp(void)
{
	int i=0;
	DBG("\n***************************************************\nmv shell cmd list:\n");
	DBG("\tcmd\t\thelpinfo\t\n");
	while(1)
	{
		if(mv_shell[i].Cmd == NULL)
		{
			break;
		}
		DBG("\t%s\t\t%s\t\n",mv_shell[i].Cmd,mv_shell[i].CmdUsage);
		i++;
	}
	DBG("cmd example:test 123 456\n");
	DBG("***************************************************\n");
}


void cmd_proess()
{
	int i = 0;
	int j = 0;
	uint8_t k = 0;
	int ii = 0;
	int argc=0;
	char *argv[CMD_ARG_MAX_COUNT];
	char temp[ONE_ARG_MAX_LEN];

        /* ignore empty line (e.g. extra LF after CR or idle timeout) */
        if(strlen((char*)shellbuffer) == 0)
        {
            return;
        }
	while(1)
	{
		if(mv_shell[i].Cmd == NULL)
		{
			if(j==0)
			{
				DBG("未知命令\n");
			}
			return;
		}
		if(memcmp(shellbuffer,mv_shell[i].Cmd,strlen(mv_shell[i].Cmd)) == 0)
		{
			argc=0;
			ii = 0;
			memset(temp,0,sizeof(temp));
			for(k=0;k<strlen((char*)shellbuffer);k++)
			{
				if((shellbuffer[k] == 0x20)||(shellbuffer[k] == 0x0D))
				{
					memcpy(cmdbuffer+argc,temp,sizeof(temp));
					memset(temp,0,sizeof(temp));
					argv[argc] = (char*)(cmdbuffer+argc);
					ii = 0;
					argc++;
					if(argc == CMD_ARG_MAX_COUNT)
					{
						break;
					}
				}
				else
				{
					temp[ii] = shellbuffer[k];
					ii++;
					if(ii >= ONE_ARG_MAX_LEN)
					{
						ii=10;
					}
				}
			}
			(mv_shell[i].CmdHandler)(argc,argv);
			j++;
		}
		i++;
	}
}


void shell_init(void)
{
#ifdef CFG_PARA_SHELL_COM0
    UARTS_IOCTL(UART_PORT0, UART_IOCTL_RXFIFO_CLR, 1);
    UARTS_IOCTL(UART_PORT0, UART_IOCTL_TXFIFO_CLR, 1);
    UARTS_IOCTL(UART_PORT0, UART_IOCTL_RXINT_SET, 1);
    UARTS_IOCTL(UART_PORT0, UART_IOCTL_RXFIFO_TRGRDEP_SET, 0);
    memset(UARTBuffer,0,sizeof(UARTBuffer));
    NVIC_EnableIRQ(UART0_IRQn);
    NVIC_SetPriority(UART0_IRQn,2);
#else
#ifdef DUAL_CORE_EN
    UARTS_IOCTL(UART_PORT1, UART_IOCTL_RXFIFO_CLR, 1);
    UARTS_IOCTL(UART_PORT1, UART_IOCTL_TXFIFO_CLR, 1);
    UARTS_IOCTL(UART_PORT1, UART_IOCTL_RXINT_SET, 1);
    UARTS_IOCTL(UART_PORT1, UART_IOCTL_RXFIFO_TRGRDEP_SET, 0);
    memset(UARTBuffer,0,sizeof(UARTBuffer));
    NVIC_EnableIRQ(UART_IRQn);
    NVIC_SetPriority(UART_IRQn,2);
#else
    UARTS_IOCTL(UART_PORT1, UART_IOCTL_RXFIFO_CLR, 1);
    UARTS_IOCTL(UART_PORT1, UART_IOCTL_TXFIFO_CLR, 1);
    UARTS_IOCTL(UART_PORT1, UART_IOCTL_RXINT_SET, 1);
    UARTS_IOCTL(UART_PORT1, UART_IOCTL_RXFIFO_TRGRDEP_SET, 0);
    memset(UARTBuffer,0,sizeof(UARTBuffer));
    NVIC_EnableIRQ(UART1_IRQn);
    NVIC_SetPriority(UART1_IRQn,2);
#endif
#endif
}


void mv_shell_task(void * param)
{
	uint8_t count = 0;
	uint8_t cmdstr;
	TickType_t last = 0;
	DBG("mv_shell_task START\n");
	while(1)
	{
		cmdstr = shell_getchar();
		if(cmdstr)
		{
			DBG("%c",cmdstr);
			shellbuffer[count] = cmdstr;
			if((cmdstr == 0x0A)||(cmdstr == 0x0D))
			{
				DBG("\n");
				cmd_proess();
				count = 0;
				memset(shellbuffer,0,sizeof(shellbuffer));
			}
			else
			{
				count++;
				last = xTaskGetTickCount();   /* reset idle timer */
			}
		}
		else
		{
			/* execute command when input is idle for a while,
			 * so serial tools that send no <CR><LF> still work */
			if((count > 0) && ((xTaskGetTickCount() - last) > pdMS_TO_TICKS(200)))
			{
				cmd_proess();
				count = 0;
				memset(shellbuffer,0,sizeof(shellbuffer));
			}
		}
		osTaskDelay(10);   /* was 1ms; relax slightly, still responsive */
	}
}

#endif //CFG_FUNC_SHELL_EN


