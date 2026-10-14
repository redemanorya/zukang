
/**
 *****************************************************************************
 * @file     sdio_card1.c
 * @author   owen
 * @version  V1.0.0
 * @date     2017-05-15
 * @brief    sd card module driver interface
 *****************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
 */
#include <string.h>
#include "type.h"
#include "debug.h"
#include "delay.h"
#include "timeout.h"
#include "sdio.h"
#include "sd_card.h"
#include "dma.h"
#include "core_d1088.h"
#include "gpio.h"

#ifdef FUNC_OS_EN
#include "rtos_api.h" //add for mutex declare

osMutexId SDIOMutex = NULL;
osMutexId SDIOSendCommandMutex = NULL;
#endif

/**<传输状态 */
#define SD_STATUS_ACCEPTED				0x02
#define SD_STATUS_CRC_ERROR				0x05
#define SD_STATUS_WRITE_ERROR			0x06

/**<卡类型*/
#define	MC_MCDMAS_MMC					1
#define	MC_MCDMAS_SD					2

#define	SD_BLOCK_SIZE					512             /**<块大小固定为512字节*/

/**<卡状态定义*/
#define	CURRENT_STATE_IDLE				0
#define	CURRENT_STATE_READY				1
#define	CURRENT_STATE_IDENT				2              /**<identification*/
#define	CURRENT_STATE_STBY				3
#define	CURRENT_STATE_TRAN				4
#define	CURRENT_STATE_DATA				5              /**<sending data*/
#define	CURRENT_STATE_RCV				6
#define	CURRENT_STATE_PRG				7
#define	CURRENT_STATE_DIS				8              /**<disconnect*/

#define	VOLTAGE_WINDOWS					0x40FF8000     /**<2.7v~3.6v*/
#define VOLTAGE_PATTERN                 0x000001AA     /**<0x01:2.7-3.6V,0xAA:check pattern,pattern can use any 8-bit*/

#define	BUS_WIDTH_1BIT					0x00
#define	BUS_WIDTH_4BIT					0x02

#define CARD_DBG	printf

SD_CARD		    SDCard;
SD_CARD_ID 		SDCardId;
static uint8_t  SDPort;
#ifndef CFG_SDIO_BYTE_MODE_ENABLE
static uint32_t SD_WordModeBuf[4096/4];  //SDIO WORD方式读写buf,SDIO读写时传入的buf不是WORD对齐的情况下用于临时缓存数据
#endif
void CardPortInit(uint8_t SdioPort)
{
	SDPort = SdioPort;
	switch(SdioPort)
	{
		case SDIO_B0_TO_B5:
//			GPIO_RegOneBitClear(GPIO_B_PD, GPIOB3);
//			GPIO_RegOneBitClear(GPIO_B_PD, GPIOB4);
//			GPIO_RegOneBitClear(GPIO_B_PD, GPIOB5);
//			GPIO_RegOneBitSet(GPIO_B_PU, GPIOB3);
//			GPIO_RegOneBitSet(GPIO_B_PU, GPIOB4);
//			GPIO_RegOneBitSet(GPIO_B_PU, GPIOB5);

			GPIO_RegBitsClear(GPIO_B_PD,GPIOB3|GPIOB4|GPIOB5);
			GPIO_RegBitsSet(GPIO_B_PU,GPIOB3|GPIOB4|GPIOB5);

			GPIO_PortBModeSet(GPIOB3, 0x01);//sd_dat1_2
			GPIO_PortBModeSet(GPIOB4, 0x01);//sd_dat2_2
			GPIO_PortBModeSet(GPIOB5, 0x01);//sd_dat3_2

		case SDIO_B0_B1_B2:
//			GPIO_RegOneBitClear(GPIO_B_PD, GPIOB0);
//			GPIO_RegOneBitClear(GPIO_B_PD, GPIOB1);
//			GPIO_RegOneBitClear(GPIO_B_PD, GPIOB2);
//			GPIO_RegOneBitSet(GPIO_B_PU, GPIOB0);
//			GPIO_RegOneBitSet(GPIO_B_PU, GPIOB1);
//			GPIO_RegOneBitSet(GPIO_B_PU, GPIOB2);

			GPIO_RegBitsClear(GPIO_B_PD,GPIOB0|GPIOB1|GPIOB2);
			GPIO_RegBitsSet(GPIO_B_PU,GPIOB0|GPIOB1|GPIOB2);

			GPIO_PortBModeSet(GPIOB0, 0x01);//sd_dat0_2
			GPIO_PortBModeSet(GPIOB1, 0x0C);//sd_clk_2
			GPIO_PortBModeSet(GPIOB2, 0x01);//sd_cmd_2
			break;

		case SDIO_A10_TO_A15:
//			GPIO_RegOneBitClear(GPIO_A_PD, GPIOA13);
//			GPIO_RegOneBitClear(GPIO_A_PD, GPIOA14);
//			GPIO_RegOneBitClear(GPIO_A_PD, GPIOA15);
//			GPIO_RegOneBitSet(GPIO_A_PU, GPIOA13);
//			GPIO_RegOneBitSet(GPIO_A_PU, GPIOA14);
//			GPIO_RegOneBitSet(GPIO_A_PU, GPIOA15);

			GPIO_RegBitsClear(GPIO_A_PD,GPIOA13|GPIOA14|GPIOA15);
			GPIO_RegBitsSet(GPIO_A_PU,GPIOA13|GPIOA14|GPIOA15);

			GPIO_PortAModeSet(GPIOA13, 0x01);//sd_dat1_1
			GPIO_PortAModeSet(GPIOA14, 0x01);//sd_dat2_1
			GPIO_PortAModeSet(GPIOA15, 0x01);//sd_dat3_1

		case SDIO_A10_A11_A12:
//			GPIO_RegOneBitClear(GPIO_A_PD, GPIOA10);
//			GPIO_RegOneBitClear(GPIO_A_PD, GPIOA11);
//			GPIO_RegOneBitClear(GPIO_A_PD, GPIOA12);
//			GPIO_RegOneBitSet(GPIO_A_PU, GPIOA10);
//			GPIO_RegOneBitSet(GPIO_A_PU, GPIOA11);
//			GPIO_RegOneBitSet(GPIO_A_PU, GPIOA12);

			GPIO_RegBitsClear(GPIO_A_PD,GPIOA10|GPIOA11|GPIOA12);
			GPIO_RegBitsSet(GPIO_A_PU,GPIOA10|GPIOA11|GPIOA12);

			GPIO_PortAModeSet(GPIOA10, 0x01);//sd_dat0_1
			GPIO_PortAModeSet(GPIOA11, 0x0c);//sd_clk_1
			GPIO_PortAModeSet(GPIOA12, 0x01);//sd_cmd_1
			break;

		case SDIO_A2_TO_A7:
//			GPIO_RegOneBitClear(GPIO_A_PD, GPIOA2);
//			GPIO_RegOneBitClear(GPIO_A_PD, GPIOA3);
//			GPIO_RegOneBitClear(GPIO_A_PD, GPIOA4);
//			GPIO_RegOneBitSet(GPIO_A_PU, GPIOA2);
//			GPIO_RegOneBitSet(GPIO_A_PU, GPIOA3);
//			GPIO_RegOneBitSet(GPIO_A_PU, GPIOA4);

			GPIO_RegBitsClear(GPIO_A_PD,GPIOA2|GPIOA3|GPIOA4);
			GPIO_RegBitsSet(GPIO_A_PU,GPIOA2|GPIOA3|GPIOA4);

			GPIO_PortAModeSet(GPIOA2, 0x02);//sd_dat3_0
			GPIO_PortAModeSet(GPIOA3, 0x01);//sd_dat2_0
			GPIO_PortAModeSet(GPIOA4, 0x02);//sd_dat1_0

		case SDIO_A5_A6_A7:
//			GPIO_RegOneBitClear(GPIO_A_PD, GPIOA5);
//			GPIO_RegOneBitClear(GPIO_A_PD, GPIOA6);
//			GPIO_RegOneBitClear(GPIO_A_PD, GPIOA7);
//			GPIO_RegOneBitSet(GPIO_A_PU, GPIOA5);
//			GPIO_RegOneBitSet(GPIO_A_PU, GPIOA6);
//			GPIO_RegOneBitSet(GPIO_A_PU, GPIOA7);
			GPIO_RegBitsClear(GPIO_A_PD,GPIOA5|GPIOA6|GPIOA7);
			GPIO_RegBitsSet(GPIO_A_PU,GPIOA5|GPIOA6|GPIOA7);

			GPIO_PortAModeSet(GPIOA5, 0x02);//sd_dat0_0
			GPIO_PortAModeSet(GPIOA6, 0x08);//sd_clk_0
			GPIO_PortAModeSet(GPIOA7, 0x02);//sd_cmd_0
			break;
	}
}

void SDCardDeinit(uint8_t SdioPort)
{
	switch(SdioPort)
	{
		case SDIO_B0_TO_B5:
//			GPIO_RegOneBitClear(GPIO_B_PU, GPIOB3);
//			GPIO_RegOneBitClear(GPIO_B_PU, GPIOB4);
//			GPIO_RegOneBitClear(GPIO_B_PU, GPIOB5);
			GPIO_RegBitsClear(GPIO_B_PU, GPIOB3|GPIOB4|GPIOB5);

			GPIO_PortBModeSet(GPIOB3, 0);//sd_dat1_2
			GPIO_PortBModeSet(GPIOB4, 0);//sd_dat2_2
			GPIO_PortBModeSet(GPIOB5, 0);//sd_dat3_2
		case SDIO_B0_B1_B2:
//			GPIO_RegOneBitClear(GPIO_B_PU, GPIOB0);
//			GPIO_RegOneBitClear(GPIO_B_PU, GPIOB1);
//			GPIO_RegOneBitClear(GPIO_B_PU, GPIOB2);
			GPIO_RegBitsClear(GPIO_B_PU, GPIOB0|GPIOB1|GPIOB2);

			GPIO_PortBModeSet(GPIOB0, 0);//sd_dat0_2
			GPIO_PortBModeSet(GPIOB1, 0);//sd_clk_2
			GPIO_PortBModeSet(GPIOB2, 0);//sd_cmd_2
			break;

		case SDIO_A10_TO_A15:
//			GPIO_RegOneBitClear(GPIO_A_PU, GPIOA13);
//			GPIO_RegOneBitClear(GPIO_A_PU, GPIOA14);
//			GPIO_RegOneBitClear(GPIO_A_PU, GPIOA15);
			GPIO_RegBitsClear(GPIO_A_PU, GPIOA13|GPIOA14|GPIOA15);

			GPIO_PortAModeSet(GPIOA13, 0);//sd_dat1_1
			GPIO_PortAModeSet(GPIOA14, 0);//sd_dat2_1
			GPIO_PortAModeSet(GPIOA15, 0);//sd_dat3_1

		case SDIO_A10_A11_A12:
//			GPIO_RegOneBitClear(GPIO_A_PU, GPIOA10);
//			GPIO_RegOneBitClear(GPIO_A_PU, GPIOA11);
//			GPIO_RegOneBitClear(GPIO_A_PU, GPIOA12);
			GPIO_RegBitsClear(GPIO_A_PU, GPIOA10|GPIOA11|GPIOA12);

			GPIO_PortAModeSet(GPIOA10, 0);//sd_dat0_1
			GPIO_PortAModeSet(GPIOA11, 0);//sd_clk_1
			GPIO_PortAModeSet(GPIOA12, 0);//sd_cmd_1
			break;

		case SDIO_A2_TO_A7:
//			GPIO_RegOneBitClear(GPIO_A_PU, GPIOA2);
//			GPIO_RegOneBitClear(GPIO_A_PU, GPIOA3);
//			GPIO_RegOneBitClear(GPIO_A_PU, GPIOA4);
			GPIO_RegBitsClear(GPIO_A_PU, GPIOA2|GPIOA3|GPIOA4);

			GPIO_PortAModeSet(GPIOA2, 0);//sd_dat3_0
			GPIO_PortAModeSet(GPIOA3, 0);//sd_dat2_0
			GPIO_PortAModeSet(GPIOA4, 0);//sd_dat1_0

		case SDIO_A5_A6_A7:
//			GPIO_RegOneBitClear(GPIO_A_PU, GPIOA5);
//			GPIO_RegOneBitClear(GPIO_A_PU, GPIOA6);
//			GPIO_RegOneBitClear(GPIO_A_PU, GPIOA7);
			GPIO_RegBitsClear(GPIO_A_PU, GPIOA5|GPIOA6|GPIOA7);

			GPIO_PortAModeSet(GPIOA5, 0);//sd_dat0_0
			GPIO_PortAModeSet(GPIOA6, 0);//sd_clk_0
			GPIO_PortAModeSet(GPIOA7, 0);//sd_cmd_0
			break;
	}
}


/**
 * @brief  初始化SDIO控制器
 * @param  NONE
 * @return NONE
 * @note   这里将读写模式默认初始化为连续读写方式
 */
void SDCard_ControllerInit(void)
{
	SDCard.RCA = 0;
	SDCard.IsSDHC = FALSE;
	SDCard.IsCardInit = FALSE;
#ifdef FUNC_OS_EN
	if(SDIOMutex == NULL)
		SDIOMutex = osMutexCreate();
#endif
	SDIO_SysToSdioDivSet(15);
	SDIO_Init();
	SDIO_ClkSet(15);
#ifdef CFG_SDIO_BYTE_MODE_ENABLE
	SDIO_ByteModeEnable();
#else
	SDIO_ByteModeDisable();
#endif
}
uint32_t volatile  cmd_timeout_count=0;
extern uint32_t gSysTick;

/**
 * @brief  SDIO发送命令
 * @param  SDIO需要发送的命令编号
 * @param  对于命令的参数
 * @return 发送命令成功返回NO_ERR，错误返回SEND_CMD_TIME_OUT_ERR
 * @note
 */
bool SDIO_CmdSend(uint8_t Cmd, uint32_t Param, uint16_t TimeOut)
{
	TIMER timer;
	SDIO_CmdStart(Cmd,Param);
	TimeOutSet(&timer, TimeOut);
	while(!SDIO_CmdIsDone())
	{
		if(Cmd == 0 || Cmd == 4 || Cmd == 15)
		{
	#ifdef FUNC_OS_EN
		osTaskDelay(1);
	#else
		__udelay(1000);
	#endif
		}
		if(IsTimeOut(&timer))
		{
			SDIO_CmdStop();
			return SEND_CMD_TIME_OUT_ERR;
		}
	#ifdef FUNC_OS_EN
		osTaskDelay(1);
	#endif
	}
	SDIO_CmdStop();
	return NO_ERR;
}



static SD_CARD_ERR_CODE SDCard_SendAppCommand(uint8_t Cmd, uint32_t Param)
{
	if(SDIO_CmdSend(CMD55_APP_CMD, SDCard.RCA, 20) == NO_ERR)
	{
		DelayUs(5);// > 1us between two cmd  by pi 2017.12.21
		if(SDIO_CmdSend(Cmd, Param, 20) == NO_ERR)
		{
			return NONE_ERR;
		}
		return CMD_SEND_TIME_OUT_ERR;
	}
	return CMD55_SEND_ERR;
}
/**
 * @brief  SD卡检测
 * @param  NONE
 * @return NONE
 * @note
 */
SD_CARD_ERR_CODE SDCard_Detect(void)
{
	if(SDCard.IsCardInit)//卡已初始化
	{
		if(SDIO_CmdSend(CMD13_SEND_STATUS, SDCard.RCA, 20)  == NO_ERR)              	//read card status
		{
			return NONE_ERR;
		}
		else
		{
			SDCard_ControllerInit();
			return CMD13_SEND_ERR;
		}
	}
	else//卡未初始化
	{
		SDCard_ControllerInit();
		SDIO_CmdSend(CMD0_GO_IDLE_STATE, 0, 20);
		SDIO_CmdSend(CMD8_SEND_IF_COND, VOLTAGE_PATTERN, 20);
		if(SDCard_SendAppCommand(ACMD41_SD_SEND_OP_COND, VOLTAGE_WINDOWS) == NONE_ERR)
		{
			DBG("SD link!\n");
			SDCard.CardType = MC_MCDMAS_SD;
			return NONE_ERR;
		}

		//check mmc card
		SDIO_CmdSend(CMD0_GO_IDLE_STATE, 0, 20);
		SDIO_CmdSend(CMD8_SEND_IF_COND, VOLTAGE_PATTERN, 20);
		if(SDIO_CmdSend(CMD1_SEND_OP_COND, VOLTAGE_WINDOWS, 20) ==NO_ERR)
		{
			DBG("MMC link!\n");
			SDCard.CardType = MC_MCDMAS_MMC;
			return NONE_ERR;
		}
		return NOCARD_LINK_ERR;
	}
}

SD_CARD_ERR_CODE SDCard_Identify(void)
{
	TIMER Timer;
	uint8_t  resp[16];
	uint8_t  i;

	//设置电压窗口
	TimeOutSet(&Timer, 2000);
	while(1)
	{
		DelayUs(10);//兼容性延时
		if(SDCard.CardType == MC_MCDMAS_SD)
		{
			if(SDCard_SendAppCommand(ACMD41_SD_SEND_OP_COND, VOLTAGE_WINDOWS))
			{
				return ACMD41_SEND_ERR;
			}
		}
		if(SDCard.CardType == MC_MCDMAS_MMC)
		{
			if(SDIO_CmdSend(CMD1_SEND_OP_COND, VOLTAGE_WINDOWS, 20))
			{
				return CMD1_SEND_ERR;
			}
		}
		SDIO_CmdRespGet(resp, SDIO_RESP_TYPE_R3);

		if(*(uint32_t *)resp & 0x80000000)//ocr register bit definition, bit31-Card power up status bit (busy) bit30-Card Capacity Status (CCS)
		{
			if(*(uint32_t *)resp & 0x40000000)//for SDHC
			{
				SDCard.IsSDHC = TRUE;
			}
			break;
		}

		if(IsTimeOut(&Timer))
		{
			DBG("CMD_SEND_TIME_OUT_ERR\n");
			return CMD_SEND_TIME_OUT_ERR;
		}
	}
	DelayUs(10);
	//获取CID(unique card identification)
	if(SDIO_CmdSend(CMD2_ALL_SEND_CID, 0, 20))      //success card will from ready switch to identification
	{
		DBG("SdCardInit() ERROR 005!\n");
		return CMD2_SEND_ERR;
	}
	SDIO_CmdRespGet(resp, SDIO_RESP_TYPE_R2);
	for(i = 0; i < 15; i++)
	{
		*((uint8_t *)(&(SDCardId)) + i + 1) = resp[14 - i];
	}

	//获取相对地址，最多重试10次
	for(i = 0; i < 10; i++)
	{
		if(SDIO_CmdSend(CMD3_SEND_RELATIVE_ADDR, 0, 20) == NO_ERR)
		{
			SDIO_CmdRespGet(resp, SDIO_RESP_TYPE_R6);
			if(((*(uint32_t *)(resp)) & 0x00001E00) >> 9 == CURRENT_STATE_STBY)
			{
				//SDCard.RCA = *(uint32_t *)resp;
				SDCard.RCA = (*(uint32_t *)resp) & 0xFFFF0000;
				break;
			}
		}
	}

	DBG("RCA: %08lx\n", SDCard.RCA);

	//获取CSD寄存器（Card Specific Data）里面的描述值，譬如，块长度，卡容量信息等。
	if(SDIO_CmdSend(CMD9_SEND_CSD, SDCard.RCA, 20))
	{
		return CMD9_SEND_ERR;
	}
	SDIO_CmdRespGet(resp, SDIO_RESP_TYPE_R2);
	//根据CSD计算BlockNum
	if(resp[14])	//CSD V2
	{
		SDCard.BlockNum = (((resp[7] & 0x3F) << 16) + (resp[6] << 8) + resp[5] + 1) << 10;	// memory capacity = (C_SIZE+1) * 512KByte
	}
	else			//CSD V1
	{
		uint8_t exp;
		SDCard.BlockNum = ((resp[8] & 0x03) << 10) + (resp[7] << 2) + ((resp[6] & 0xC0) >> 6) + 1;

		exp = ((resp[5] & 0x03) << 1) + ((resp[4] & 0x80) >> 7) + 2 + (resp[9] & 0x0F) - 9;
		SDCard.BlockNum <<= exp;	// (C_SIZE + 1) * 2 ^ (C_SIZE_MULT + 2)
	}
	DBG("BlockNum:%lu\n", SDCard.BlockNum);
	//get card max transfer data speed
	i = (resp[11] >> 3) & 0x0F;

	DBG("resp:%X\n", resp[11]);
	if(i == 0 )
	{
		DBG("Csd error!\n");
	}
	//默认sdio模块时钟  sys_clk/(ClkDiv+1) = 120M/(2+1) = 40M
	//clk = 40M /(2*(ClkDiv+1))
	if((resp[11] & 0x07) == 1)	//base 1M
	{
		if(i == 0x0F)//8M
		{
			SDCard.MaxTransSpeed = 0x02;	//频率设为6.67MHz = 40/(2*(2+1))
		}
		else if(i > 0x08)//4.0M,4.5M,5.0M,5.5M,6.0M,7.0M
		{
			SDCard.MaxTransSpeed = 0x03;	//频率设为5MHz = 40/(2*(3+1))
		}
		else if(i > 0x04)//2.0M,2.5M,3.0M,3.5M
		{
			SDCard.MaxTransSpeed = 0x07;	//频率设为2.5MHz = 40/(2*(7+1))
		}
		else //1.0M,1.2M,1.3M,1.5M
		{
			SDCard.MaxTransSpeed = 16;	//频率设为1.18MHz = 40/(2*(16+1))
		}
	}
	else if((resp[11] & 0x07) == 2)	//base 10M
	{
		if(i < 0x04)//10M, 12M, 13M
		{
			SDCard.MaxTransSpeed = 0x02;	//频率设为6.67MHz = 40/(2*(2+1))
		}
		else if(i < 0x07)//15M,20M,25M
		{
			SDCard.MaxTransSpeed = 0x01;	//频率设为10MHz = 40/(2*(1+1))
		}
		else
		{
			SDCard.MaxTransSpeed = 0x00;	//频率设为20MHz = 40/(2*(0+1))
		}
	}
	else
	{
		DBG("Csd max speed error!\n");
	}
	DBG("SDCard.MaxTransSpeed:%d\n", SDCard.MaxTransSpeed);
	//进入transfer state
	if(SDIO_CmdSend(CMD7_SELECT_DESELECT_CARD, SDCard.RCA, 20))
	{
		DBG("CMD7_SELECT_DESELECT_CARD\n");
		return CMD7_SEND_ERR;
	}
	DelayUs(5);
	SDIO_CmdRespGet(resp, SDIO_RESP_TYPE_R1);
	DBG("resp 0x%08lx\n",*(uint32_t *)resp);
	if((*(uint32_t *)(resp)) & CARD_IS_LOCKED)	//CARD_IS_LOCKED
	{
		DBG("CARD_IS_LOCKED\n");
	}
	else
	{
		//端口支持4DAT
		if((SDPort == SDIO_B0_TO_B5) || (SDPort == SDIO_A10_TO_A15) || (SDPort == SDIO_A2_TO_A7))
		{
			if(SDCard_SendAppCommand(ACMD6_SET_BUS_WIDTH, BUS_WIDTH_4BIT) == NONE_ERR)
			{
				DBG("SD BUS_WIDTH_4BIT\n");
				SDIO_Dat4ModeEnable();
			}
		}
		else
		{
			if(SDCard_SendAppCommand(ACMD6_SET_BUS_WIDTH, BUS_WIDTH_1BIT) == NONE_ERR)
			{
				DBG("SD BUS_WIDTH_1BIT\n");
				SDIO_Dat4ModeDisable();
			}
		}
	}
	//获取状态
	if(SDIO_CmdSend(CMD13_SEND_STATUS, SDCard.RCA, 20))
	{
		DBG("CMD13_SEND_STATUS\n");
		return CMD13_SEND_ERR;
	}
	SDIO_CmdRespGet(resp, SDIO_RESP_TYPE_R1);
	DBG("resp 0x%08lx\n",*(uint32_t *)resp);
	if(((*(uint32_t *)(resp)) & 0x00001E00) >> 9  != CURRENT_STATE_TRAN)
	{
		DBG("CMD13_SEND_STATUS SDIO_RESP_TYPE_R1\n");
		return GET_RESPONSE_STATUS_ERR;
	}
	//设置块长度
	if(SDIO_CmdSend(CMD16_SET_BLOCKLEN, SD_BLOCK_SIZE, 20))
	{
		DBG("NONSPPORT CMD16_SET_BLOCKLEN  \n");
	}
	return NONE_ERR;
}

SD_CARD_ERR_CODE SDCard_Init(void)
{
	uint8_t Retry = 4;
#ifdef FUNC_OS_EN
	osMutexLock(SDIOMutex);
#endif
	SDCard_ControllerInit();

	//根据以往SD卡兼容性经验，插入卡需延时一段时间
#ifdef FUNC_OS_EN
	osTaskDelay(150);
#else
	WaitMs(150);
#endif
	while(1)
	{
		SDCard_Detect();//主要是探测是否有卡插入
		if(SDCard_Identify() == NONE_ERR)
		{
			DBG("CardGetInfo OK\n");
			break;
		}
		if(!(--Retry))
		{
			DBG("retry 4 times was still failed\n");
#ifdef FUNC_OS_EN
			osMutexUnlock(SDIOMutex);
#endif
			return GET_SD_CARD_INFO_ERR;
		}
	}

	SDCard.IsCardInit = TRUE;
	SDIO_SysToSdioDivSet(2);
	SDIO_ClkSet(SDCard.MaxTransSpeed);	//1:10m
	// SDIO_ClkSet(0);	//0:20m
	SDIO_ClkEnable();
#ifdef FUNC_OS_EN
	osMutexUnlock(SDIOMutex);
#endif
	return NONE_ERR;
}



SD_CARD_ERR_CODE SDCard_ReadBlock(uint32_t Block, uint8_t* Buffer,uint8_t Size)
{
	TIMER Timer;
	SD_CARD_ERR_CODE ret = NONE_ERR;
#ifndef CFG_SDIO_BYTE_MODE_ENABLE
	uint32_t  SD_WordModeBufFlag = 0;
#endif
	if(Block > SDCard.BlockNum)
	{
		return BLOCK_NUM_EXCEED_BOUNDARY;
	}
#ifndef CFG_SDIO_BYTE_MODE_ENABLE
	if(((uint32_t)Buffer & 0x03)) //不是WORD对齐，需要临时拷贝到SD_WordModeBuf，然后在传回Buffer
	{
		if((SD_BLOCK_SIZE*Size) <= sizeof(SD_WordModeBuf))
		{
			SD_WordModeBufFlag = 1;
		}
		else
		{
			DBG("SD_WordModeBuf not enough!\n");
		}
	}
#endif
#ifdef FUNC_OS_EN
		osMutexLock(SDIOMutex);
#endif
	if(!SDCard.IsSDHC)
	{
		Block *= SD_BLOCK_SIZE;
	}
	TimeOutSet(&Timer, 350 + 20 * Size);  //某张卡1bit Nac达370ms，为兼容，提高上限。
	if(Size == 1)
	{
		DMA_ChannelDisable(PERIPHERAL_0_ID_SDIO_RX);
		DMA_InterruptFlagClear(PERIPHERAL_0_ID_SDIO_RX, DMA_DONE_INT);
		DMA_BlockConfig(PERIPHERAL_0_ID_SDIO_RX);
#ifndef CFG_SDIO_BYTE_MODE_ENABLE
		if(SD_WordModeBufFlag)
			DMA_BlockBufSet(PERIPHERAL_0_ID_SDIO_RX, SD_WordModeBuf, SD_BLOCK_SIZE);
		else
			DMA_BlockBufSet(PERIPHERAL_0_ID_SDIO_RX, Buffer, SD_BLOCK_SIZE);
#else
		DMA_BlockBufSet(PERIPHERAL_0_ID_SDIO_RX, Buffer, SD_BLOCK_SIZE);
		DMA_ConfigDwidth(PERIPHERAL_0_ID_SDIO_RX, DMA_DWIDTH_BYTE);
#endif
		DMA_ChannelEnable(PERIPHERAL_0_ID_SDIO_RX);
		SDIO_SingleBlockConfig(SDIO_DIR_RX,SD_BLOCK_SIZE);
		SDIO_DataTransfer(1);
		SDIO_CmdSend(CMD17_READ_SINGLE_BLOCK, Block, 20);
		while(1)
		{
			if(DMA_InterruptFlagGet(PERIPHERAL_0_ID_SDIO_RX, DMA_DONE_INT)&&SDIO_DataIsDone())
			{
				DMA_ChannelDisable(PERIPHERAL_0_ID_SDIO_RX);
				DMA_InterruptFlagClear(PERIPHERAL_0_ID_SDIO_RX, DMA_DONE_INT);
				break;
			}
			if(IsTimeOut(&Timer))
			{
				DMA_ChannelDisable(PERIPHERAL_0_ID_SDIO_RX);
				DMA_InterruptFlagClear(PERIPHERAL_0_ID_SDIO_RX, DMA_DONE_INT);
				SDIO_DataTransfer(0);
			#ifdef FUNC_OS_EN
					osMutexUnlock(SDIOMutex);
			#endif
					DBG("1 sector read timeout\n");
					return READ_SD_CARD_TIME_OUT_ERR;
			}
		#ifdef FUNC_OS_EN
			osTaskDelay(1);
		#endif
		}
		SDIO_DataTransfer(0);
		SDIO_ClkDisable();
	}
	else
	{
		DMA_ChannelDisable(PERIPHERAL_0_ID_SDIO_RX);
		DMA_InterruptFlagClear(PERIPHERAL_0_ID_SDIO_RX, DMA_DONE_INT);
		DMA_BlockConfig(PERIPHERAL_0_ID_SDIO_RX);
#ifndef CFG_SDIO_BYTE_MODE_ENABLE
		if(SD_WordModeBufFlag)
			DMA_BlockBufSet(PERIPHERAL_0_ID_SDIO_RX, SD_WordModeBuf, SD_BLOCK_SIZE*Size);
		else
			DMA_BlockBufSet(PERIPHERAL_0_ID_SDIO_RX, Buffer, SD_BLOCK_SIZE*Size);
#else
		DMA_BlockBufSet(PERIPHERAL_0_ID_SDIO_RX, Buffer, SD_BLOCK_SIZE*Size);
		DMA_ConfigDwidth(PERIPHERAL_0_ID_SDIO_RX, DMA_DWIDTH_BYTE);
#endif
		DMA_ChannelEnable(PERIPHERAL_0_ID_SDIO_RX);
		SDIO_MultiBlockConfig(SDIO_DIR_RX,SD_BLOCK_SIZE,Size);
		SDIO_AutoKillRXClkDisable();
		SDIO_DataTransfer(1);
		SDIO_CmdSend(CMD18_READ_MULTIPLE_BLOCK, Block, 20);
		while(1)
		{
			if(DMA_InterruptFlagGet(PERIPHERAL_0_ID_SDIO_RX, DMA_DONE_INT)&&SDIO_MultiBlockTransferDone())
			{
				DMA_ChannelDisable(PERIPHERAL_0_ID_SDIO_RX);
				DMA_InterruptFlagClear(PERIPHERAL_0_ID_SDIO_RX, DMA_DONE_INT);
				break;
			}
			if(IsTimeOut(&Timer))
			{
				SDIO_ClkEnable();
				SDIO_CmdDoneCheckBusy(1);
				SDIO_CmdSend(CMD12_STOP_TRANSMISSION, 0, 300);
				SDIO_CmdDoneCheckBusy(0);
				SDIO_DataTransfer(0);
				DMA_ChannelDisable(PERIPHERAL_0_ID_SDIO_RX);
				DMA_InterruptFlagClear(PERIPHERAL_0_ID_SDIO_RX, DMA_DONE_INT);
			#ifdef FUNC_OS_EN
				osMutexUnlock(SDIOMutex);
			#endif
				DBG("%d sectors read timeout\n", Size);
				return READ_SD_CARD_TIME_OUT_ERR;
			}
		#ifdef FUNC_OS_EN
			osTaskDelay(1);
		#endif
		}
		SDIO_ClkEnable();
		SDIO_CmdDoneCheckBusy(1);
		SDIO_CmdSend(CMD12_STOP_TRANSMISSION, 0, 300);
		SDIO_CmdDoneCheckBusy(0);
		SDIO_DataTransfer(0);
		SDIO_ClkDisable();
	}
#ifdef FUNC_OS_EN
	osMutexUnlock(SDIOMutex);
#endif
#ifndef CFG_SDIO_BYTE_MODE_ENABLE
	if(SD_WordModeBufFlag)
		memcpy(Buffer, SD_WordModeBuf,SD_BLOCK_SIZE*Size);
#endif
	return ret;
}





SD_CARD_ERR_CODE SDCard_WriteBlock(uint32_t Block, uint8_t* Buffer, uint8_t Size)
{
	TIMER Timer;
	SD_CARD_ERR_CODE ret = NONE_ERR;
#ifndef CFG_SDIO_BYTE_MODE_ENABLE
	uint32_t  SD_WordModeBufFlag = 0;
#endif
	if(Block > SDCard.BlockNum)
	{
		return BLOCK_NUM_EXCEED_BOUNDARY;
	}
#ifndef CFG_SDIO_BYTE_MODE_ENABLE
	if(((uint32_t)Buffer & 0x03)) //不是WORD对齐，需要临时拷贝到SD_WordModeBuf
	{
		if((SD_BLOCK_SIZE*Size) <= sizeof(SD_WordModeBuf))
		{
			SD_WordModeBufFlag = 1;
			memcpy(SD_WordModeBuf,Buffer,SD_BLOCK_SIZE*Size);
		}
		else
		{
			DBG("SD_WordModeBuf not enough!\n");
		}
	}
#endif
#ifdef FUNC_OS_EN
	osMutexLock(SDIOMutex);
#endif

	if(!SDCard.IsSDHC)
	{
		Block *= SD_BLOCK_SIZE;
	}
	SDIO_ClkEnable();
	TimeOutSet(&Timer,50);
	while(SDIO_IsDataLineBusy() && !IsTimeOut(&Timer));
	if(SDIO_IsDataLineBusy())
	{
		ret = WRITE_SD_CARD_TIME_OUT_ERR;
	}
	else
	{
		if(Size == 1)
		{
			SDIO_ClearClkHalt();
			SDIO_CmdSend(CMD24_WRITE_BLOCK, Block, 50);
			DMA_ChannelDisable(PERIPHERAL_0_ID_SDIO_TX);
			DMA_InterruptFlagClear(PERIPHERAL_0_ID_SDIO_TX, DMA_DONE_INT);
			DMA_BlockConfig(PERIPHERAL_0_ID_SDIO_TX);
#ifndef CFG_SDIO_BYTE_MODE_ENABLE
			if(SD_WordModeBufFlag)
				DMA_BlockBufSet(PERIPHERAL_0_ID_SDIO_TX, SD_WordModeBuf, SD_BLOCK_SIZE);
			else
				DMA_BlockBufSet(PERIPHERAL_0_ID_SDIO_TX, (uint8_t*)(Buffer), SD_BLOCK_SIZE);
#else
			DMA_BlockBufSet(PERIPHERAL_0_ID_SDIO_TX, (uint8_t*)(Buffer), SD_BLOCK_SIZE);
			DMA_ConfigDwidth(PERIPHERAL_0_ID_SDIO_TX, DMA_DWIDTH_BYTE);
#endif
			DMA_ChannelEnable(PERIPHERAL_0_ID_SDIO_TX);
			SDIO_SingleBlockConfig(SDIO_DIR_TX,SD_BLOCK_SIZE);
			SDIO_DataTransfer(1);
			TimeOutSet(&Timer,300);
			while(1)
			{
				if(DMA_InterruptFlagGet(PERIPHERAL_0_ID_SDIO_TX, DMA_DONE_INT)&&SDIO_DataIsDone())
				{
					DMA_ChannelDisable(PERIPHERAL_0_ID_SDIO_TX);
					DMA_InterruptFlagClear(PERIPHERAL_0_ID_SDIO_TX, DMA_DONE_INT);
					break;
				}
				if(IsTimeOut(&Timer))
				{
					SDIO_DataTransfer(0);
					SDIO_ClkDisable();
					DMA_ChannelDisable(PERIPHERAL_0_ID_SDIO_TX);
					DMA_InterruptFlagClear(PERIPHERAL_0_ID_SDIO_TX, DMA_DONE_INT);
				#ifdef FUNC_OS_EN
					osMutexUnlock(SDIOMutex);
				#endif
					DBG("1 sector write timeout\n");
					return WRITE_SD_CARD_TIME_OUT_ERR;
				}
			#ifdef FUNC_OS_EN
				osTaskDelay(1);
			#endif
			}
			SDIO_DataTransfer(0);
			SDIO_ClkDisable();
		#ifdef FUNC_OS_EN
			osMutexUnlock(SDIOMutex);
		#endif
			return ret;
		}
		else
		{
			SDIO_ClearClkHalt();
			ret = SDIO_CmdSend(CMD25_WRITE_MULTIPLE_BLOCK, Block, 50);
			DMA_ChannelDisable(PERIPHERAL_0_ID_SDIO_TX);
			DMA_InterruptFlagClear(PERIPHERAL_0_ID_SDIO_TX, DMA_DONE_INT);
			DMA_BlockConfig(PERIPHERAL_0_ID_SDIO_TX);
#ifndef CFG_SDIO_BYTE_MODE_ENABLE
			if(SD_WordModeBufFlag)
				DMA_BlockBufSet(PERIPHERAL_0_ID_SDIO_TX, SD_WordModeBuf, SD_BLOCK_SIZE*Size);
			else
				DMA_BlockBufSet(PERIPHERAL_0_ID_SDIO_TX, (uint8_t*)(Buffer), SD_BLOCK_SIZE*Size);
#else
			DMA_BlockBufSet(PERIPHERAL_0_ID_SDIO_TX, (uint8_t*)(Buffer), SD_BLOCK_SIZE*Size);
			DMA_ConfigDwidth(PERIPHERAL_0_ID_SDIO_TX, DMA_DWIDTH_BYTE);
#endif
			DMA_ChannelEnable(PERIPHERAL_0_ID_SDIO_TX);
			SDIO_MultiBlockConfig(SDIO_DIR_TX,SD_BLOCK_SIZE,Size);
			SDIO_DataTransfer(1);
			TimeOutSet(&Timer,250 + 50 * Size);
			while(1)
			{
				if((DMA_InterruptFlagGet(PERIPHERAL_0_ID_SDIO_TX, DMA_DONE_INT))&&(SDIO_MultiBlockTransferDone()))
				{
					DMA_ChannelDisable(PERIPHERAL_0_ID_SDIO_TX);
					DMA_InterruptFlagClear(PERIPHERAL_0_ID_SDIO_TX, DMA_DONE_INT);
					break;
				}
				if(IsTimeOut(&Timer))
				{
					SDIO_ClearClkHalt();
					SDIO_CmdDoneCheckBusy(1);
					SDIO_CmdSend(CMD12_STOP_TRANSMISSION, 0, 300);
					SDIO_CmdDoneCheckBusy(0);
					SDIO_DataTransfer(0);
					SDIO_ClkDisable();
					DMA_ChannelDisable(PERIPHERAL_0_ID_SDIO_TX);
					DMA_InterruptFlagClear(PERIPHERAL_0_ID_SDIO_TX, DMA_DONE_INT);
				#ifdef FUNC_OS_EN
					osMutexUnlock(SDIOMutex);
				#endif
					DBG("%d sectors write timeout\n", Size);
					return WRITE_SD_CARD_TIME_OUT_ERR;
				}
			#ifdef FUNC_OS_EN
					osTaskDelay(1);
			#endif
			}
			SDIO_ClearClkHalt();
			SDIO_CmdDoneCheckBusy(1);
			ret=SDIO_CmdSend(CMD12_STOP_TRANSMISSION, 0, 300);
			SDIO_CmdDoneCheckBusy(0);
			SDIO_DataTransfer(0);
			SDIO_ClkDisable();
		}
	}
#ifdef FUNC_OS_EN
		osMutexUnlock(SDIOMutex);
#endif
	return ret;
}

SD_CARD* SDCard_GetCardInfo()
{
	return &SDCard;
}

uint32_t SDCard_CapacityGet(void)
{
	return SDCard.BlockNum;
}

void SDCard_Force_Exit(void)
{
	SDIO_ClkEnable();
	SDIO_CmdDoneCheckBusy(1);
	SDIO_CmdSend(CMD12_STOP_TRANSMISSION, 0, 300);
	SDIO_CmdDoneCheckBusy(0);
	SDIO_DataTransfer(0);
	DMA_ChannelDisable(PERIPHERAL_0_ID_SDIO_RX);
	DMA_InterruptFlagClear(PERIPHERAL_0_ID_SDIO_RX, DMA_DONE_INT);
}




