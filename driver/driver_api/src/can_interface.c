/**
 **************************************************************************************
 * @file    i2c_interface.h
 * @brief   i2c interface
 *
 * @author  hunanyun
 * @version V1.0.0
 *
 * $Created: 2025-9-4 12:15:00$
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#include "can_interface.h"
#include "gpio.h"
#include "can.h"
#include "reset.h"

/**
 * @brief  CAN端口模式选择函数
 * @param  pin        端口模式选择
 * @return 无
 * @Note   必须在CAN_Init初始化后调用，CAN0模块只能选择0、1、2引脚，CAN1模块只能选择4、5、6引脚
 */
void CAN_PortSelect(CAN_PORT_MODE pin)
{
	switch (pin)
	{
        case CAN0_PORT_A0_A1:
            GPIO_PortAModeSet(GPIOA0,0b111);
            GPIO_PortAModeSet(GPIOA1,0b10);
            break;
        case CAN0_PORT_A16_A17:
            GPIO_PortAModeSet(GPIOA16,0b1111);
            GPIO_PortAModeSet(GPIOA17,0b1000);
            break;
        case CAN0_PORT_B0_B1:
            GPIO_PortBModeSet(GPIOB0,0b10001);
            GPIO_PortBModeSet(GPIOB1,0b111);
            break;
        case CAN1_PORT_A14_A15:
            GPIO_PortAModeSet(GPIOA14,0b10000);
            GPIO_PortAModeSet(GPIOA15,0b110);
            break;
        case CAN1_PORT_A20_A21:
            GPIO_PortAModeSet(GPIOA20,0b1100);
            GPIO_PortAModeSet(GPIOA21,0b11);
            break;
        case CAN1_PORT_B4_B5:
            GPIO_PortBModeSet(GPIOB4,0b1101);
            GPIO_PortBModeSet(GPIOB5,0b101);
            break;
        default:
            break;
	}
}

static const uint8_t Can_BaudRate[RATE_MAX_TABLE][5] =
{
// PHSEG1	PHSEG2	SAM   TQ_BRP	SJW
	{12,	5,		1,		11,		0 },	//50K
	{12,	5,		1,		5,		0 },	//100K
	{10,	3,		1,		5,		0 },	//125K
	{10,	3,		1,		2,		1 },	//250K
	{15,	6,		1,		0,		1 },	//500K
	{7,		2,		1,		0,		1 },	//1M
};

/**
 * @brief  CAN模块初始化函数
 * @param  port       CAN端口模式选择
 * @param  baudrate   CAN波特率设置
 * @param  pin        CAN引脚模式选择
 * @return 无
 * @note   必须在CAN_Init初始化后调用，CAN0模块只能选择0、1、2引脚，CAN1模块只能选择4、5、6引脚
 *         函数会配置CAN端口选择、时钟选择、波特率参数、错误计数等
 */
void CAN_ModuleInit(CAN_PORT_MODE port, CAN_BAUDRATE baudrate, CAN_PORT_MODE pin)
{
	CAN_INIT_STRUCT can_init;

	CAN_PortSelect(pin);
	CAN_ClkSelect(port, CAN_CLK_OSC_24M);

	// Reset_FunctionReset(CAN_FUNC_SEPA);

	CAN_SetModeCmd(port, CAN_MODE_RST_SELECT);

	can_init.PHSEG1  = Can_BaudRate[baudrate][0];
	can_init.PHSEG2  = Can_BaudRate[baudrate][1];
	can_init.SAM     = Can_BaudRate[baudrate][2];
	can_init.TQ_BRP	 = Can_BaudRate[baudrate][3];
	can_init.SJW	 = Can_BaudRate[baudrate][4];
    //Acceptance code and mask
	can_init.CAN_ACPC= 0x00;
	can_init.CAN_ACPM= 0xffffffff;
	CAN_Init(port, &can_init);

	//CAN_IntTypeEnable(CAN_INT_RX_EN | CAN_INT_WAKEUP_EN);
	//CAN中断和SPDIF复用一个中断号 22
	//NVIC_EnableIRQ(SPDIF_IRQn);//22
	//GIE_ENABLE();

	CAN_RXTX_ERR_CNT cnt;
	cnt.ERR_WRN_LMT = 0x80;
	cnt.RX_ERR_CNT  = 0;
	cnt.TX_ERR_CNT  = 0;
	CAN_SetRxTxErrCnt(port, &cnt);

	//CAN_SetModeCmd(CAN_MODE_ABOM_DISABLE);
	//CAN_SetModeCmd(CAN_MODE_AUWK_MODE_DISABLE);
	//CAN_SetModeCmd(CAN_MODE_AUWK_MODE);
	//CAN_SetModeCmd(CAN_MODE_SLEEP_SEL);
	CAN_SetModeCmd(port, CAN_MODE_RST_DISABLE);
}

/**
 * @brief  CAN数据发送测试函数
 * @param  port       CAN端口模式选择
 * @return 无
 * @note   发送一个固定格式的测试数据帧，ID为0x55，数据为8字节(0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x80)
 *         数据帧为标准帧(非扩展帧)、普通帧(非远程帧)
 */
void CanDataSendTest(CAN_PORT_MODE port)
{
	CAN_DATA_MSG Msg;

	Msg.Id		= 0x55;
	Msg.EFF 	= 0;  	// 1 - Extend frame 0 - Standard frame
	Msg.RTR 	= 0;	// 1 - remote frame 0 - normal frame
	Msg.Data[0]	= 0x10;
	Msg.Data[1]	= 0x20;
	Msg.Data[2]	= 0x30;
	Msg.Data[3]	= 0x40;
	Msg.Data[4]	= 0x50;
	Msg.Data[5]	= 0x60;
	Msg.Data[6]	= 0x70;
	Msg.Data[7]	= 0x80;
	Msg.DATALENGTH = 8;

	CAN_Send(port, &Msg,500);
}
