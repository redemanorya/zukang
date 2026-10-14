/**
 **************************************************************************************
 * @file    i2c_interface.c
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
 
#include "type.h"
//#include "app_config.h"
#include "uarts.h"
#include "gpio.h"
#include "timeout.h"
#include "delay.h"
#include "i2c_interface.h"
#include "dma.h"
#include "i2c.h"
#define I2C_ERR_RECOVER_TIME 	10//出现锁死超时恢复时间

/**
 * @brief I2C 配置结构体
 */
typedef struct _I2C_CFG
{
	uint8_t I2cClkParam;
	I2C_PORT_MODE port;
	uint8_t OwnAddr;
} I2C_CFG;

/**
 * @brief I2C 全局配置实例
 */
I2C_CFG I2C_Cfg;

static I2C_PORT_MODE s_I2C_PortMode[2] = {I2C0_PORT_A0_A1, I2C1_PORT_A2_A3};

/** @brief I2C 等待中断标志
 *  @param port I2C端口
 *  @param timeout 超时时间
 *  @return I2C错误状态
 */
static I2C_ErrorState I2C_WaitIntFlag(I2C_PORT_T port, uint32_t timeout)
{
    TIMER I2CTimer;
    TimeOutSet(&I2CTimer, timeout);
    while(!I2C_GetIntFlag(port))
    {
        if(IsTimeOut(&I2CTimer))
        {
            return ERROR_TIMEOUT;
        }
    }
    return ERROR_OK;
}

/** @brief I2C 端口模式设置
 *  @param pin I2C端口模式
 *  @return 无
 */
static void I2C_SetPortMode(I2C_PORT_MODE pin)
{
    switch(pin)
    {
        case I2C0_PORT_A0_A1:
        case I2C0_PORT_A6_A7:
        case I2C0_PORT_A14_A15:
        case I2C0_PORT_A22_A23:
        case I2C0_PORT_A30_A31:
        case I2C0_PORT_B6_B7:
        case I2C0_PORT_B14_B15:
            s_I2C_PortMode[I2C_PORT0] = pin;
            break;
        case I2C1_PORT_A2_A3:
        case I2C1_PORT_A10_A11:
        case I2C1_PORT_A18_A19:
        case I2C1_PORT_A26_A25:
        case I2C1_PORT_B2_B3:
        case I2C1_PORT_B10_B11:
        case I2C1_PORT_B18_B19:
            s_I2C_PortMode[I2C_PORT1] = pin;
            break;
        default:
            break;
    }
}

/** @brief I2C 锁死恢复
 *  @param port I2C端口
 *  @return 无
 */
static void I2C_Clock_Recovery(I2C_PORT_T port)
{
    uint32_t gpio_mode = 0;
    uint32_t sda = 0;
    uint32_t scl = 0;

    if(port == I2C_PORT0)
    {
        switch(s_I2C_PortMode[0])
        {
            case I2C0_PORT_A0_A1:
                gpio_mode = GPIO_A_START; sda = GPIOA0; scl = GPIOA1; break;
            case I2C0_PORT_A6_A7:
                gpio_mode = GPIO_A_START; sda = GPIOA6; scl = GPIOA7; break;
            case I2C0_PORT_A14_A15:
                gpio_mode = GPIO_A_START; sda = GPIOA14; scl = GPIOA15; break;
            case I2C0_PORT_A22_A23:
                gpio_mode = GPIO_A_START; sda = GPIOA22; scl = GPIOA23; break;
            case I2C0_PORT_A30_A31:
                gpio_mode = GPIO_A_START; sda = GPIOA30; scl = GPIOA31; break;
            case I2C0_PORT_B6_B7:
                gpio_mode = GPIO_B_START; sda = GPIOB6; scl = GPIOB7; break;
            case I2C0_PORT_B14_B15:
                gpio_mode = GPIO_B_START; sda = GPIOB14; scl = GPIOB15; break;
            default:
                return;
        }
    }
    else if(port == I2C_PORT1)
    {
        switch(s_I2C_PortMode[1])
        {
            case I2C1_PORT_A2_A3:
                gpio_mode = GPIO_A_START; sda = GPIOA2; scl = GPIOA3; break;
            case I2C1_PORT_A10_A11:
                gpio_mode = GPIO_A_START; sda = GPIOA10; scl = GPIOA11; break;
            case I2C1_PORT_A18_A19:
                gpio_mode = GPIO_A_START; sda = GPIOA18; scl = GPIOA19; break;
            case I2C1_PORT_A26_A25:
                gpio_mode = GPIO_A_START; sda = GPIOA26; scl = GPIOA25; break;
            case I2C1_PORT_B2_B3:
                gpio_mode = GPIO_B_START; sda = GPIOB2; scl = GPIOB3; break;
            case I2C1_PORT_B10_B11:
                gpio_mode = GPIO_B_START; sda = GPIOB10; scl = GPIOB11; break;
            case I2C1_PORT_B18_B19:
                gpio_mode = GPIO_B_START; sda = GPIOB18; scl = GPIOB19; break;
            default:
                return;
        }
    }
    else
    {
        return;
    }

    if(gpio_mode == GPIO_B_START)
    {
        GPIO_PortBModeSet(sda, 0);
        GPIO_PortBModeSet(scl, 0);
    }
    else
    {
        GPIO_PortAModeSet(sda, 0);
        GPIO_PortAModeSet(scl, 0);
    }

    GPIO_RegOneBitSet((gpio_mode + 0x05), sda);
    GPIO_RegOneBitClear((gpio_mode + 0x06), sda);
    GPIO_RegOneBitSet((gpio_mode + 0x06), scl);
    GPIO_RegOneBitClear((gpio_mode + 0x05), scl);

    for(uint8_t i = 0; i < 9; i++)
    {
        GPIO_RegOneBitClear((gpio_mode + 0x01), scl);
        DelayUs(5);
        GPIO_RegOneBitSet((gpio_mode + 0x01), scl);
        DelayUs(5);
    }
}

/** @brief I2C 主机复位函数
 *  @param port I2C端口
 *  @return 无
 */
static void I2C_MasterReset(I2C_PORT_T port)
{
    I2C_Disable(port);
    I2C_Clock_Recovery(port);
    
    // 使用保存的配置重新初始化 I2C
    I2C_Config(I2C_Cfg.port,s_I2C_PortMode[I2C_Cfg.port], I2C_Cfg.I2cClkParam, I2C_Cfg.OwnAddr);

    // I2C_SendStop(port);
}

/**
 * @brief  I2C功能引脚选择函数
 * @param  pin:  I2C端口模式选择
 * @return 无
 * @Note   必须在I2C_Init初始化后调用
 */
void IIC_PortSelect(I2C_PORT_MODE pin)
{
	I2C_SetPortMode(pin);
	
	switch(pin)
	{
		case I2C0_PORT_A0_A1:
			GPIO_PortAModeSet(GPIOA0,0b10);//sda
			GPIO_PortAModeSet(GPIOA1,0b1);//scl
			GPIO_RegOneBitSet(GPIO_A_PU, GPIO_INDEX0);
			GPIO_RegOneBitClear(GPIO_A_PD, GPIO_INDEX0);
			GPIO_RegOneBitSet(GPIO_A_PU, GPIO_INDEX1);
			GPIO_RegOneBitClear(GPIO_A_PD, GPIO_INDEX1);
			break;

		case I2C0_PORT_A6_A7:
			GPIO_PortAModeSet(GPIOA6,0b101);//sda
			GPIO_PortAModeSet(GPIOA7,0b110);//scl
			GPIO_RegOneBitSet(GPIO_A_PU, GPIO_INDEX6);
			GPIO_RegOneBitClear(GPIO_A_PD, GPIO_INDEX6);
			GPIO_RegOneBitSet(GPIO_A_PU, GPIO_INDEX7);
			GPIO_RegOneBitClear(GPIO_A_PD, GPIO_INDEX7);
	        break;

		case I2C0_PORT_A14_A15:
			GPIO_PortAModeSet(GPIOA14,0b111);//sda
			GPIO_PortAModeSet(GPIOA15,0b101);//scl
			GPIO_RegOneBitSet(GPIO_A_PU, GPIO_INDEX14);
			GPIO_RegOneBitClear(GPIO_A_PD, GPIO_INDEX14);
			GPIO_RegOneBitSet(GPIO_A_PU, GPIO_INDEX15);
			GPIO_RegOneBitClear(GPIO_A_PD, GPIO_INDEX15);
			break;

		case I2C0_PORT_A22_A23:
			GPIO_PortAModeSet(GPIOA22,0b11);//sda
			GPIO_PortAModeSet(GPIOA23,0b10);//scl
			GPIO_RegOneBitSet(GPIO_A_PU, GPIO_INDEX22);
			GPIO_RegOneBitClear(GPIO_A_PD, GPIO_INDEX22);
			GPIO_RegOneBitSet(GPIO_A_PU, GPIO_INDEX23);
			GPIO_RegOneBitClear(GPIO_A_PD, GPIO_INDEX23);
	        break;

		case I2C0_PORT_A30_A31:
			GPIO_PortAModeSet(GPIOA30,0b10);//sda
			GPIO_PortAModeSet(GPIOA31,0b1);//scl
			GPIO_RegOneBitSet(GPIO_A_PU, GPIO_INDEX30);
			GPIO_RegOneBitClear(GPIO_A_PD, GPIO_INDEX30);
			GPIO_RegOneBitSet(GPIO_A_PU, GPIO_INDEX31);
			GPIO_RegOneBitClear(GPIO_A_PD, GPIO_INDEX31);
			break;

		case I2C0_PORT_B6_B7:
			GPIO_PortBModeSet(GPIOB6,0b100);//sda
			GPIO_PortBModeSet(GPIOB7,0b11);//scl
			GPIO_RegOneBitSet(GPIO_B_PU, GPIO_INDEX6);
			GPIO_RegOneBitClear(GPIO_B_PD, GPIO_INDEX6);
			GPIO_RegOneBitSet(GPIO_B_PU, GPIO_INDEX7);
			GPIO_RegOneBitClear(GPIO_B_PD, GPIO_INDEX7);
			break;

		case I2C0_PORT_B14_B15:
			GPIO_PortBModeSet(GPIOB14,0b10);//sda
			GPIO_PortBModeSet(GPIOB15,0b1);//scl
			GPIO_RegOneBitSet(GPIO_B_PU, GPIO_INDEX14);
			GPIO_RegOneBitClear(GPIO_B_PD, GPIO_INDEX14);
			GPIO_RegOneBitSet(GPIO_B_PU, GPIO_INDEX15);
			GPIO_RegOneBitClear(GPIO_B_PD, GPIO_INDEX15);
			break;

		case I2C1_PORT_A2_A3:
			GPIO_PortAModeSet(GPIOA2,0b101);//sda
			GPIO_PortAModeSet(GPIOA3,0b11);
			GPIO_RegOneBitSet(GPIO_A_PU, GPIO_INDEX2);
			GPIO_RegOneBitClear(GPIO_A_PD, GPIO_INDEX2);
			GPIO_RegOneBitSet(GPIO_A_PU, GPIO_INDEX3);
			GPIO_RegOneBitClear(GPIO_A_PD, GPIO_INDEX3);
			break;

		case I2C1_PORT_A10_A11:
			GPIO_PortAModeSet(GPIOA10,0b1010);//sda
			GPIO_PortAModeSet(GPIOA11,0b111);
			GPIO_RegOneBitSet(GPIO_A_PU, GPIO_INDEX10);
			GPIO_RegOneBitClear(GPIO_A_PD, GPIO_INDEX10);
			GPIO_RegOneBitSet(GPIO_A_PU, GPIO_INDEX11);
			GPIO_RegOneBitClear(GPIO_A_PD, GPIO_INDEX11);
			break;

		case I2C1_PORT_A18_A19:
			GPIO_PortAModeSet(GPIOA18,0b101);//sda
			GPIO_PortAModeSet(GPIOA19,0b101);//scl
			GPIO_RegOneBitSet(GPIO_A_PU, GPIO_INDEX18);
			GPIO_RegOneBitClear(GPIO_A_PD, GPIO_INDEX18);
			GPIO_RegOneBitSet(GPIO_A_PU, GPIO_INDEX19);
			GPIO_RegOneBitClear(GPIO_A_PD, GPIO_INDEX19);
			break;

		case I2C1_PORT_A26_A25:
			GPIO_PortAModeSet(GPIOA25,0b10);//scl
			GPIO_PortAModeSet(GPIOA26,0b11);//sda
			GPIO_RegOneBitSet(GPIO_A_PU, GPIO_INDEX25);
			GPIO_RegOneBitClear(GPIO_A_PD, GPIO_INDEX25);
			GPIO_RegOneBitSet(GPIO_A_PU, GPIO_INDEX26);
			GPIO_RegOneBitClear(GPIO_A_PD, GPIO_INDEX26);
			break;

		case I2C1_PORT_B2_B3:
			GPIO_PortBModeSet(GPIOB2,0b101);//sda
			GPIO_PortBModeSet(GPIOB3,0b101);//scl
			GPIO_RegOneBitSet(GPIO_B_PU, GPIO_INDEX2);
			GPIO_RegOneBitClear(GPIO_B_PD, GPIO_INDEX2);
			GPIO_RegOneBitSet(GPIO_B_PU, GPIO_INDEX3);
			GPIO_RegOneBitClear(GPIO_B_PD, GPIO_INDEX3);
			break;

		case I2C1_PORT_B10_B11:
			GPIO_PortBModeSet(GPIOB10,0b101);//sda
			GPIO_PortBModeSet(GPIOB11,0b11);//scl
			GPIO_RegOneBitSet(GPIO_B_PU, GPIO_INDEX10);
			GPIO_RegOneBitClear(GPIO_B_PD, GPIO_INDEX10);
			GPIO_RegOneBitSet(GPIO_B_PU, GPIO_INDEX11);
			GPIO_RegOneBitClear(GPIO_B_PD, GPIO_INDEX11);
			break;

		case I2C1_PORT_B18_B19:
			GPIO_PortBModeSet(GPIOB18,0b10);//sda
			GPIO_PortBModeSet(GPIOB19,0b1);//scl
			GPIO_RegOneBitSet(GPIO_B_PU, GPIO_INDEX18);
			GPIO_RegOneBitClear(GPIO_B_PD, GPIO_INDEX18);
			GPIO_RegOneBitSet(GPIO_B_PU, GPIO_INDEX19);
			GPIO_RegOneBitClear(GPIO_B_PD, GPIO_INDEX19);
			break;

	}

}

/**
 * @brief  I2C 初始化配置函数
 * @param  port: I2C 端口模式选择
 * @param  pin: I2C 引脚模式选择
 * @param  I2cClkParam: 时钟分频参数 (范围 0x00-0x3f)
 * @param  OwnAddr: 自身地址 (做 slave 时配置，做 master 时可随意配置)
 * @return 无
 * @note   保存配置到 I2C_Cfg 结构体，并调用底层 I2C_Init
 */
void I2C_Config(I2C_PORT_T port, I2C_PORT_MODE pin, uint8_t I2cClkParam, uint8_t OwnAddr)
{
	I2C_Cfg.I2cClkParam = I2cClkParam;
	I2C_Cfg.port = port;
	I2C_Cfg.OwnAddr = OwnAddr;
	
	// 调用底层 I2C 初始化函数
	I2C_Init(port, I2cClkParam, OwnAddr);
	IIC_PortSelect(pin);	//GPIO设置
}

/**
 * @brief  I2C DMA接收初始化函数
 * @param  port: I2C端口号，取值范围I2C_PORT_T
 * @param  RxBufAddr: 接收缓冲区地址
 * @param  RxBufLen: 接收缓冲区长度
 * @param  ThresholdLen: 阈值中断的长度
 * @param  CallBack: 中断回调函数指针
 * @return bool: 初始化成功返回TRUE，失败返回FALSE
 * @note   使用循环模式进行DMA接收，当接收到ThresholdLen字节数据时产生中断
 */
bool I2C_DMA_RxInit(I2C_PORT_T port,void* RxBufAddr, uint16_t RxBufLen, uint16_t ThresholdLen, INT_FUNC CallBack)
{
	DMA_PERIPHERAL_ID dma_id[] = {PERIPHERAL_0_ID_I2C0_RX,PERIPHERAL_0_ID_I2C1_RX};
    if(DMA_CircularConfig(dma_id[port], 0, RxBufAddr, RxBufLen) != DMA_OK)
    {
        return FALSE;
    }
	
    DMA_CircularThresholdLenSet(dma_id[port], ThresholdLen);
	DMA_InterruptFunSet(dma_id[port], DMA_THRESHOLD_INT, CallBack);
	DMA_InterruptEnable(dma_id[port], DMA_THRESHOLD_INT, 1);
    DMA_ChannelEnable(dma_id[port]);
	I2C_DMAEnable(port);
    return TRUE;
}

/**
 * @brief  I2C DMA发送初始化函数
 * @param  port: I2C端口号，取值范围I2C_PORT_T
 * @param  TxBufAddr: 发送缓冲区地址
 * @param  TxBufLen: 发送缓冲区长度
 * @param  ThresholdLen: 阈值中断的长度
 * @param  CallBack: 中断回调函数指针
 * @return bool: 初始化成功返回TRUE，失败返回FALSE
 * @note   使用循环模式进行DMA发送，当发送ThresholdLen字节数据时产生中断
 */
bool I2C_DMA_TxInit(I2C_PORT_T port,void* TxBufAddr, uint16_t TxBufLen, uint16_t ThresholdLen, INT_FUNC CallBack)
{
	DMA_PERIPHERAL_ID dma_id[] = {PERIPHERAL_0_ID_I2C0_TX,PERIPHERAL_0_ID_I2C1_TX};
    if(DMA_CircularConfig(dma_id[port], 0, TxBufAddr, TxBufLen) != DMA_OK)
    {
        return FALSE;
    }
	
    DMA_CircularThresholdLenSet(dma_id[port], ThresholdLen);
	DMA_InterruptFunSet(dma_id[port], DMA_THRESHOLD_INT, CallBack);
	DMA_InterruptEnable(dma_id[port], DMA_THRESHOLD_INT, 1);
    DMA_ChannelEnable(dma_id[port]);
	I2C_DMAEnable(port);
    return TRUE;
}

/**
 * @brief  I2C DMA块发送初始化函数
 * @param  port: I2C端口号，取值范围I2C_PORT_T
 * @param  CallBack: 中断回调函数指针
 * @return bool: 初始化成功返回TRUE，失败返回FALSE
 * @note   使用块模式进行DMA发送，数据发送完成时产生中断
 */
bool I2C_DMA_BlockTxInit(I2C_PORT_T port,INT_FUNC CallBack)
{
	DMA_PERIPHERAL_ID dma_id[] = {PERIPHERAL_0_ID_I2C0_TX,PERIPHERAL_0_ID_I2C1_TX};
	if(DMA_BlockConfig(dma_id[port])!=DMA_OK)
	{
		return FALSE;
	}
	DMA_InterruptFunSet(dma_id[port], DMA_DONE_INT, CallBack);
	DMA_InterruptEnable(dma_id[port], DMA_DONE_INT, 1);
    DMA_ChannelEnable(dma_id[port]);
    I2C_DMAEnable(port);
    return TRUE;
}

/**
 * @brief  I2C DMA块发送函数
 * @param  port: I2C端口号，取值范围I2C_PORT_T
 * @param  SendBuf: 发送数据缓冲区地址
 * @param  BufLen: 发送数据长度
 * @return uint32_t: 返回状态，0表示发送完成
 * @note   使用DMA块模式发送，一次性发送BufLen字节数据
 */
uint32_t I2C_DMA_BlockSend(I2C_PORT_T port,uint8_t* SendBuf, uint16_t BufLen)
{
	DMA_PERIPHERAL_ID dma_id[] = {PERIPHERAL_0_ID_I2C0_TX,PERIPHERAL_0_ID_I2C1_TX};
	DMA_BlockBufSet(dma_id[port],SendBuf,BufLen);
	DMA_ChannelEnable(dma_id[port]);
	return 0;
}

/**
 * @brief     I2C主模式下发送数据
 *
 * @param[in] SlaveAddr 从设备地址
 * @param[in] SendBuf   发送数据缓存
 * @param[in] BufLen    数据长度
 * @param[in] timeout   超时时间设置
 *
 * @return    i2c state  详见 I2C_ErrorState
 */
I2C_ErrorState I2C_MasterSendBuffer(I2C_PORT_T port,uint8_t SlaveAddr, void* SendBuf, uint32_t BufLen, uint32_t timeout)
{
	TIMER I2CTimer;
	uint32_t i;
	I2C_ErrorState ret = ERROR_OK;

	TimeOutSet(&I2CTimer, timeout);
	while(I2C_IsBusy(port))
	{
		if(IsTimeOut(&I2CTimer))
		{
			ret = ERROR_BUSY;
			goto i2c_send_buff_err;
		}
	}

	I2C_MasterConfig(port);

	// send slave address
	I2C_SendByte(port, SlaveAddr << 1);
	if(I2C_WaitIntFlag(port, timeout) != ERROR_OK)
	{
		ret = ERROR_TIMEOUT;
		goto i2c_send_buff_err;
	}
	I2C_IntClr(port);
	if(I2C_ReceiveAcknowledge(port))
	{
		I2C_SendStop(port);
		ret = ERROR_NOACK;
		goto i2c_send_buff_err;
	}

	// send data
	for(i = 0; i < BufLen; i++)
	{
		I2C_SendByte(port, ((uint8_t*)SendBuf)[i]);
		if(I2C_WaitIntFlag(port, timeout) != ERROR_OK)
		{
			ret = ERROR_TIMEOUT;
			goto i2c_send_buff_err;
		}
		I2C_IntClr(port);
		if(I2C_ReceiveAcknowledge(port))
		{
			I2C_SendStop(port);
			ret = ERROR_NOACK;
			goto i2c_send_buff_err;
		}
	}
	I2C_SendStop(port);
	return ERROR_OK;

i2c_send_buff_err:
	TimeOutSet(&I2CTimer, I2C_ERR_RECOVER_TIME);
	while(I2C_IsBusy(port))
	{
		if(IsTimeOut(&I2CTimer) || ret == ERROR_BUSY)
		{
			I2C_MasterReset(port);
			return ERROR_BUSY;
		}
	}
	return ret;
}

/**
 * @brief     I2C主模式下发送数据
 *
 * @param[in] SlaveAddr 从设备地址
 * @param[in] RegAddr   从设备寄存器地址
 * @param[in] SendBuf   发送数据缓存
 * @param[in] BufLen    数据长度
 * @param[in] timeout   超时时间设置
 *
 * @return    i2c state  详见 I2C_ErrorState
 */
I2C_ErrorState I2C_MasterSendData(I2C_PORT_T port,uint8_t SlaveAddr, uint8_t RegAddr, void* SendBuf, uint32_t BufLen, uint32_t timeout)
{
	TIMER I2CTimer;
	uint32_t i;
	I2C_ErrorState ret = ERROR_OK;

	TimeOutSet(&I2CTimer, timeout);
	while(I2C_IsBusy(port))
	{
		if(IsTimeOut(&I2CTimer))
		{
			ret = ERROR_BUSY;
			goto i2c_send_err;
		}
	}

	I2C_MasterConfig(port);

	// send slave address
	I2C_SendByte(port, SlaveAddr << 1);
	if(I2C_WaitIntFlag(port, timeout) != ERROR_OK)
	{
		ret = ERROR_TIMEOUT;
		goto i2c_send_err;
	}
	I2C_IntClr(port);
	if(I2C_ReceiveAcknowledge(port))
	{
		I2C_SendStop(port);
		ret = ERROR_NOACK;
		goto i2c_send_err;
	}

	// send register address
	I2C_SendByte(port, RegAddr);
	if(I2C_WaitIntFlag(port, timeout) != ERROR_OK)
	{
		ret = ERROR_TIMEOUT;
		goto i2c_send_err;
	}
	I2C_IntClr(port);
	if(I2C_ReceiveAcknowledge(port))
	{
		I2C_SendStop(port);
		ret = ERROR_NOACK;
		goto i2c_send_err;
	}

	// send data
	for(i=0; i<BufLen; i++)
	{
		I2C_SendByte(port, ((uint8_t *)SendBuf)[i]);
		if(I2C_WaitIntFlag(port, timeout) != ERROR_OK)
		{
			ret = ERROR_TIMEOUT;
			goto i2c_send_err;
		}
		I2C_IntClr(port);
		if(I2C_ReceiveAcknowledge(port))
		{
			I2C_SendStop(port);
			ret = ERROR_NOACK;
			goto i2c_send_err;
		}
	}
	I2C_SendStop(port);
	return ERROR_OK;

i2c_send_err:
	TimeOutSet(&I2CTimer, I2C_ERR_RECOVER_TIME);
	while(I2C_IsBusy(port))
	{
		if(IsTimeOut(&I2CTimer) || ret == ERROR_BUSY)
		{
			I2C_MasterReset(port);
			return ERROR_BUSY;
		}
	}
	return ret;
}

/**
 * @brief     I2C主模式下接收数据
 *
 * @param[in] SlaveAddr 从设备地址
 * @param[in] SendBuf   发送数据缓存
 * @param[in] BufLen    数据长度
 * @param[in] timeout   超时时间设置
 *
 * @return    i2c state  详见 I2C_ErrorState
 */
I2C_ErrorState I2C_MasterReceiveBuffer(I2C_PORT_T port,uint8_t SlaveAddr, void* RecvBuf, uint32_t BufLen, uint32_t timeout)
{
	TIMER I2CTimer;
	uint32_t i;
	I2C_ErrorState ret = ERROR_OK;
	
	TimeOutSet(&I2CTimer, timeout);
	while(I2C_IsBusy(port))
	{
		if(IsTimeOut(&I2CTimer))
		{
			ret = ERROR_BUSY;
			goto i2c_recv_buff_err;
		}
	}

	I2C_MasterConfig(port);

	// send slave address(read)
	I2C_SendByte(port,(SlaveAddr<<1) | I2C_READ);
	if(I2C_WaitIntFlag(port, timeout) != ERROR_OK)
	{
		ret = ERROR_TIMEOUT;
		goto i2c_recv_buff_err;
	}
	I2C_IntClr(port);
	if(I2C_ReceiveAcknowledge(port))
	{
		I2C_SendStop(port);
		ret = ERROR_NOACK;
		goto i2c_recv_buff_err;
	}
	
	I2C_SendAck(port);
	I2C_ReceiveStart(port);

	((uint8_t *)RecvBuf)[0] = I2C_ReceiveByte(port);//dummy read for trigger recv

	for(i=0; i<BufLen-1; i++)
	{
		I2C_SendAck(port);
		if(I2C_WaitIntFlag(port, timeout) != ERROR_OK)
		{
			ret = ERROR_TIMEOUT;
			goto i2c_recv_buff_err;
		}
		I2C_IntClr(port);
		((uint8_t *)RecvBuf)[i] = I2C_ReceiveByte(port);
	}
	
	I2C_SendNoAck(port);
	if(I2C_WaitIntFlag(port, timeout) != ERROR_OK)
	{
		ret = ERROR_TIMEOUT;
		goto i2c_recv_buff_err;
	}
	I2C_IntClr(port);
	I2C_SendStop(port);
	while(I2C_IsBusy(port));
	((uint8_t *)RecvBuf)[i] = I2C_ReceiveByte(port);
	return ERROR_OK;

i2c_recv_buff_err:
	TimeOutSet(&I2CTimer, I2C_ERR_RECOVER_TIME);
	while(I2C_IsBusy(port))
	{
		if(IsTimeOut(&I2CTimer) || ret == ERROR_BUSY)
		{
			I2C_MasterReset(port);
			return ERROR_BUSY;
		}
	}
	return ret;
}

/**
 * @brief     I2C主模式下接收数据
 *
 * @param[in] SlaveAddr 从设备地址
 * @param[in] RegAddr   从设备寄存器地址
 * @param[in] SendBuf   发送数据缓存
 * @param[in] BufLen    数据长度
 * @param[in] timeout   超时时间设置
 *
 * @return    i2c state  详见 I2C_ErrorState
 */
I2C_ErrorState I2C_MasterReceiveData(I2C_PORT_T port,uint8_t SlaveAddr, uint8_t RegAddr, void* RecvBuf, uint32_t BufLen, uint32_t timeout)
{
	TIMER I2CTimer;
	uint32_t i;
	
	TimeOutSet(&I2CTimer, timeout);
	while(I2C_IsBusy(port))
	{
		if(IsTimeOut(&I2CTimer))
		{
			return ERROR_BUSY;
		}
	}

	I2C_MasterConfig(port);

	//send slave address(write)
	I2C_SendByte(port,SlaveAddr<<1);
	while(!I2C_GetIntFlag(port));
	I2C_IntClr(port);
	if(I2C_ReceiveAcknowledge(port))
	{
		I2C_SendStop(port);
		return ERROR_NOACK;
	}
	
	//send register address
	I2C_SendByte(port,RegAddr);
	while(!I2C_GetIntFlag(port));
	I2C_IntClr(port);
	if(I2C_ReceiveAcknowledge(port))
	{
		I2C_SendStop(port);
		return ERROR_NOACK;
	}

	//send restart
	I2C_SendReStart(port);
	
	//send slave address(read)
	I2C_SendByte(port,(SlaveAddr<<1) | I2C_READ);
	while(!I2C_GetIntFlag(port));
	I2C_IntClr(port);
	if(I2C_ReceiveAcknowledge(port))
	{
		I2C_SendStop(port);
		return ERROR_NOACK;
	}

	I2C_SendAck(port);
	I2C_ReceiveStart(port);
	((uint8_t *)RecvBuf)[0] = I2C_ReceiveByte(port);//dummy read for trigger recv

	for(i=0; i<BufLen-1; i++)
	{
		I2C_SendAck(port);
		while(!I2C_GetIntFlag(port));
		I2C_IntClr(port);
		((uint8_t *)RecvBuf)[i] = I2C_ReceiveByte(port);
	}
	
	I2C_SendNoAck(port);
	while(!I2C_GetIntFlag(port));
	I2C_IntClr(port);
	I2C_SendStop(port);
	while(I2C_IsBusy(port));
	((uint8_t *)RecvBuf)[i] = I2C_ReceiveByte(port);
	return ERROR_OK;
}

/**
 * @brief     I2C从模式下发送数据
 *
 * @param[in] SlaveAddr 从设备地址
 * @param[in] SendBuf   发送数据缓存
 * @param[in] BufLen    数据长度
 * @param[in] timeout   超时时间设置
 *
 * @return    i2c state  详见 I2C_ErrorState
 */
I2C_ErrorState I2C_SlaveSendBuffer(I2C_PORT_T port,uint8_t *SendBuf, uint32_t BufLen, uint32_t timeout)
{
	uint32_t i;
	TIMER I2CTimer;
	I2C_ErrorState ret;
	TimeOutSet(&I2CTimer, timeout);

	I2C_SlaveConfig(port);

	ret = I2C_SlaveRequestWrite(port);
	if(ret)
	{
		return ret;
	}
	
	I2C_SendStart(port);
	for(i = 0;i < BufLen; i++)
	{
		I2C_SendByte(port,SendBuf[i]);
		while(!I2C_GetIntFlag(port));
		I2C_IntClr(port);
		if(I2C_ReceiveAcknowledge(port))
		{
			break;
		}
	}

	I2C_SlaveReleaseI2C(port);
	while(I2C_IsBusy(port));

	return ERROR_OK;
}

/**
 * @brief     I2C从模式下接收数据
 *
 * @param[in] SlaveAddr 从设备地址
 * @param[in] SendBuf   发送数据缓存
 * @param[in] BufLen    数据长度
 * @param[in] timeout   超时时间设置
 *
 * @return    i2c state  详见 I2C_ErrorState
 */
I2C_ErrorState I2C_SlaveReceiveBuffer(I2C_PORT_T port,uint8_t *RecvBuf, uint32_t BufLen, uint32_t timeout)
{
	uint32_t i;
	TIMER I2CTimer;
	I2C_ErrorState ret;
	TimeOutSet(&I2CTimer, timeout);	

	I2C_SlaveConfig(port);

	ret = I2C_SlaveRequestRead(port);
	if(ret)
	{
		return ret;
	}
	
	RecvBuf[0] = I2C_ReceiveByte(port);//dummy read for trigger recv
	for(i = 0; i < BufLen; i++)
	{
		while(!I2C_GetIntFlag(port));
		I2C_IntClr(port);
		RecvBuf[i] = I2C_ReceiveByte(port);
		if(!I2C_IsBusy(port))
		{	//end receive data
			break;
		}
	}
	return ERROR_OK;
}




