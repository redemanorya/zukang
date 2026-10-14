/**
 **************************************************************************************
 * @file    can_interface.h
 * @brief   can interface
 *
 * @author  hunanyun
 * @version V1.0.0
 *
 * $Created: 2025-9-4 12:15:00$
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __CAN_INTERFACE_H__
#define __CAN_INTERFACE_H__
typedef enum
{
	CAN0_PORT_A0_A1 = 0,	//TXD,RXD
	CAN0_PORT_A16_A17,
	CAN0_PORT_B0_B1,

    CAN1_PORT_A14_A15,
    CAN1_PORT_A20_A21,
    CAN1_PORT_B4_B5,

}CAN_PORT_MODE;

typedef enum _CAN_BAUDRATE_
{
	RATE_50KBPS = 0,
	RATE_100KBPS,
	RATE_125KBPS,
	RATE_250KBPS,
	RATE_500KBPS,
	RATE_1000KBPS,
	RATE_MAX_TABLE,
}CAN_BAUDRATE;

/**
 * @brief  CAN功能引脚选择函数
 * @param  port        引脚选择
 * @return 无
 * @Note   必须在CAN_Init初始化后调用
 */
void CAN_PortSelect(CAN_PORT_MODE port);

/**
 * @brief  CAN模块初始化函数
 * @param  port       CAN端口模式选择
 * @param  baudrate   CAN波特率设置
 * @param  pin        CAN引脚模式选择
 * @return 无
 * @note   需要在CAN_Init初始化函数后使用，CAN0模块只能选择0、1、2号引脚，CAN1模块只能选择4、5、6号引脚
 *         在初始化CAN端口选择时，选项、参数及引脚不能随意搭配，需要根据硬件设计合理选择
 */
void CAN_ModuleInit(CAN_PORT_MODE port, CAN_BAUDRATE baudrate, CAN_PORT_MODE pin);
#endif