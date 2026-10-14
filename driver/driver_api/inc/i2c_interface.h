/**
 **************************************************************************************
 * @file    i2c_interface.h
 * @brief   i2c interface
 *
 * @author  Huny
 * @version V1.0.1
 *
 * $Created: 2026-03-31 19:15:00$
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
 
#ifndef __I2C_INTERFACE_H__
#define __I2C_INTERFACE_H__

#include "type.h"
#include "i2c.h"
#include "dma.h"
/**
 * i2c port define
 */
typedef enum _I2C_PORT_MODE
{
	I2C0_PORT_A0_A1 = 0,
	I2C0_PORT_A6_A7,
	I2C0_PORT_A14_A15,
	I2C0_PORT_A22_A23,
	I2C0_PORT_A30_A31,
	I2C0_PORT_B6_B7,
	I2C0_PORT_B14_B15,

	I2C1_PORT_A2_A3,
	I2C1_PORT_A10_A11,
	I2C1_PORT_A18_A19,
	I2C1_PORT_A26_A25,
	I2C1_PORT_B2_B3,
	I2C1_PORT_B10_B11,
	I2C1_PORT_B18_B19,
}I2C_PORT_MODE;


/**
 * @brief  I2C 初始化配置函数
 * @param  I2cClkParam: 时钟分频参数 (范围 0x00-0x3f)
 * @param  port: I2C 端口模式选择
 * @param  OwnAddr: 自身地址 (做 slave 时配置，做 master 时可随意配置)
 * @return 无
 * @note   保存配置到 I2C_Cfg 结构体，并调用底层 I2C_Init
 */
void I2C_Config(I2C_PORT_T port, I2C_PORT_MODE pin, uint8_t I2cClkParam, uint8_t OwnAddr);

/**
 * @brief  I2C 功能引脚选择函数
 * @param  pin:  I2C 端口模式选择
 * @return 无
 */
void IIC_PortSelect( I2C_PORT_MODE pin);

/**
 * @brief  I2C DMA 接收初始化函数
 * @param  port: I2C 端口号，取值范围 I2C_PORT_T
 * @param  RxBufAddr: 接收缓冲区地址
 * @param  RxBufLen: 接收缓冲区长度
 * @param  ThresholdLen: 触发阈值中断的长度
 * @param  CallBack: 中断回调函数指针
 * @return bool: 初始化成功返回 TRUE，失败返回 FALSE
 * @note   使用循环模式配置 DMA 接收，当接收到 ThresholdLen 字节数据时触发中断
 */
bool I2C_DMA_RxInit(I2C_PORT_T port,void* RxBufAddr, uint16_t RxBufLen, uint16_t ThresholdLen, INT_FUNC CallBack);

/**
 * @brief  I2C DMA 发送初始化函数
 * @param  port: I2C 端口号，取??范围 I2C_PORT_T
 * @param  TxBufAddr: 发送缓冲区地址
 * @param  TxBufLen: 发送缓冲区长度
 * @param  ThresholdLen: 阈值中断的长度
 * @param  CallBack: 中断回调函数指针
 * @return bool: 初始化成功返回 TRUE，失败返回 FALSE
 * @note   使用循环模式进行 DMA 发送，当发送 ThresholdLen 字节数据时产生中断
 */
bool I2C_DMA_TxInit(I2C_PORT_T port,void* TxBufAddr, uint16_t TxBufLen, uint16_t ThresholdLen, INT_FUNC CallBack);

/**
 * @brief  I2C DMA 块发送初始化函数
 * @param  port: I2C 端口号，取值范围 I2C_PORT_T
 * @param  CallBack: 中断回调函数指针
 * @return bool: 初始化成功返回 TRUE，失败返回 FALSE
 * @note   使用块模式配置 DMA 发送，当发送完成时触发中断
 */
bool I2C_DMA_BlockTxInit(I2C_PORT_T port,INT_FUNC CallBack);

/**
 * @brief  I2C DMA 块发送函数
 * @param  port: I2C 端口号，取值范围 I2C_PORT_T
 * @param  SendBuf: 发送数据缓冲区地址
 * @param  BufLen: 发送数据长度
 * @return uint32_t: 发送状态，0 表示发送完成
 * @note   配置并启动 DMA 块发送，发送 BufLen 字节数据
 */
uint32_t I2C_DMA_BlockSend(I2C_PORT_T port,uint8_t* SendBuf, uint16_t BufLen);

/**
 * @brief     I2C 主模式下发送数据
 *
 * @param[in] SlaveAddr 从设备地址
 * @param[in] SendBuf   发送数据缓存
 * @param[in] BufLen    数据长度
 * @param[in] timeout   超时时间设置
 *
 * @return    i2c state  详见 I2C_ErrorState
 */
I2C_ErrorState I2C_MasterSendBuffer(I2C_PORT_T port,uint8_t SlaveAddr, void* SendBuf, uint32_t BufLen, uint32_t timeout);

/**
 * @brief     I2C 主模式下发送数据
 *
 * @param[in] SlaveAddr 从设备地址
 * @param[in] RegAddr   从设备寄存器地址
 * @param[in] SendBuf   发送数据缓存
 * @param[in] BufLen    数据长度
 * @param[in] timeout   超时时间设置
 *
 * @return    i2c state  详见 I2C_ErrorState
 */
I2C_ErrorState I2C_MasterSendData(I2C_PORT_T port,uint8_t SlaveAddr, uint8_t RegAddr, void* SendBuf, uint32_t BufLen, uint32_t timeout);

/**
 * @brief     I2C 主模式下接收数据
 *
 * @param[in] SlaveAddr 从设备地址
 * @param[in] SendBuf   发送数据缓存
 * @param[in] BufLen    数据长度
 * @param[in] timeout   超时时间设置
 *
 * @return    i2c state  详见 I2C_ErrorState
 */
I2C_ErrorState I2C_MasterReceiveBuffer(I2C_PORT_T port,uint8_t SlaveAddr, void* RecvBuf, uint32_t BufLen, uint32_t timeout);

/**
 * @brief     I2C 主模式下接收数据
 *
 * @param[in] SlaveAddr 从设备地址
 * @param[in] RegAddr   从设备寄存器地址
 * @param[in] SendBuf   发送数据缓存
 * @param[in] BufLen    数据长度
 * @param[in] timeout   超时时间设置
 *
 * @return    i2c state  详见 I2C_ErrorState
 */
I2C_ErrorState I2C_MasterReceiveData(I2C_PORT_T port,uint8_t SlaveAddr, uint8_t RegAddr, void* RecvBuf, uint32_t BufLen, uint32_t timeout);

/**
 * @brief     I2C 从模式下发送数据
 *
 * @param[in] SlaveAddr 从设备地址
 * @param[in] SendBuf   发送数据缓存
 * @param[in] BufLen    数据长度
 * @param[in] timeout   超时时间设置
 *
 * @return    i2c state  详见 I2C_ErrorState
 */
I2C_ErrorState I2C_SlaveSendBuffer(I2C_PORT_T port,uint8_t *SendBuf, uint32_t BufLen, uint32_t timeout);

/**
 * @brief     I2C 从模式下接收数据
 *
 * @param[in] SlaveAddr 从设备地址
 * @param[in] SendBuf   发送数据缓存
 * @param[in] BufLen    数据长度
 * @param[in] timeout   超时时间设置
 *
 * @return    i2c state  详见 I2C_ErrorState
 */
I2C_ErrorState I2C_SlaveReceiveBuffer(I2C_PORT_T port,uint8_t *RecvBuf, uint32_t BufLen, uint32_t timeout);


#endif/*__I2C_INTERFACE_H__*/
