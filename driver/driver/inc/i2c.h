/**
 **************************************************************************************
 * @file    i2c.h
 * @brief   I2c Module API
 *
 * @author  Jerry
 * @version V1.0.0
 *
 * $Created: 2015-03-09 13:11:35$
 *
 * @copyright Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
/**
 * @addtogroup I2C
 * @{
 * @defgroup i2c i2c.h
 * @{
 */

#ifndef __I2C_H__
#define __I2C_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


#define	I2C_WRITE	        0x00    //I2C write command    
#define	I2C_READ	        0x01    //I2C read command


/**
 * @brief I2C端口
 */
typedef enum
{
	I2C_PORT0 = 0,
	I2C_PORT1,
}I2C_PORT_T;


/**
 * @brief I2C 时钟拉伸模式定义
 * 
 * 时钟拉伸（Clock Stretching）是I2C从设备在需要更多时间处理数据时，
 * 通过拉低SCL线来暂停主机操作的机制。以下枚举定义了不同的时钟拉伸行为：
 * 
 * - I2C_STRETCH_MODE_ORIGINAL: 在整个数据传输过程中都执行时钟拉伸，
 *   包括等待用户处理的阶段
 *   
 * - I2C_STRETCH_MODE_IMPROVED: 仅在完成一个字节的数据传输并等待用户处理时
 *   执行时钟拉伸，减少了总线占用时间
 *   
 * - I2C_STRETCH_MODE_NONE: 不执行时钟拉伸，用户只需读/写数据而无需配置
 *   其他寄存器，适用于高速应用
 */
typedef enum
{
    I2C_STRETCH_MODE_ORIGINAL = 0x00,  /*!< 原始时钟拉伸模式: 在整个数据传输和等待用户处理期间都执行时钟拉伸 */
    I2C_STRETCH_MODE_IMPROVED = 0x01,  /*!< 改进时钟拉伸模式: 仅在字节传输完成等待用户处理时执行时钟拉伸 */
    I2C_STRETCH_MODE_NONE     = 0x02   /*!< 无时钟拉伸模式: 不执行时钟拉伸，用户只需读/写数据 */
} I2C_StretchModeTypeDef;

/**
 * i2c define
 */
 
 /*
typedef enum 
{
	ENABLE					= 1,
	DISABLE					= 0
}I2C_CONFIG;


*/
/**
 * i2c mode define
 */
typedef enum 
{
	MASTER					= 1,
	SLAVE					= 0
}I2C_CONFIG_MODE;

/**
 * i2c state define
 */
typedef enum 
{
	I2C_OK       			= 0x00,
	I2C_ERROR    			= 0x01,
	I2C_BUSY     			= 0x02,
	I2C_TIMEOUT  			= 0x03
}I2C_StatusTypeDef;

/**
 * err state define
 */
typedef enum 
{
	ERROR_OK       			= 0x00,
	ERROR_BUSY    			= 0x01,
	ERROR_TIMEOUT    		= 0x02,
	ERROR_NOACK				= 0x03,
	ERROR_UNMATCH			= 0x04,
	ERROR_SLAVEMODE			= 0x05,
	ERROR_DIV_OVERSTEP		= 0x06
}I2C_ErrorState;


/**
 * i2c速率计算公式:
 * PLL模式下: i2c_rate = OSC-PLL/(SYS_CLK_DIV_NUM+1)/(SYS_CLK_DIV_NUM1+1)/(APB_CLK_DIV_NUM+1)/I2C_CLK_DIV
 * RC模式下: i2c_rate = OSC-RC/(SYS_CLK_DIV_NUM+1)/(SYS_CLK_DIV_NUM1+1)/(APB_CLK_DIV_NUM+1)/I2C_CLK_DIV
 * 默认设置: OSC-PLL = 288M
 *           OSC-RC = 12M
 *           SYS_CLK_DIV_NUM=0
 *           SYS_CLK_DIV_NUM1=1
 *           APB_CLK_DIV_NUM = 7
 * 参考参数:
 *     (1)I2C_CLK_DIV_48(0x28) i2c_rate = 375KHz
 *     (2)I2C_CLK_DIV_80(0x2C) i2c_rate = 225KHz
 * 注: i2c速率不要超过400K
 * i2c 分频比和设置参数关系表如下:
 ******************************************************************************
 ****** Div | Param **** Div  | Param **** Div | Param **** Div  | Param ******
 ****** 30  | 0x00  **** 288  | 0x10  **** 22  | 0x20  **** 160  | 0x30  ******
 ****** 32  | 0x01  **** 320  | 0x11  **** 24  | 0x21  **** 192  | 0x31  ******
 ****** 36  | 0x02  **** 384  | 0x12  **** 26  | 0x22  **** 224  | 0x32  ******
 ****** 42  | 0x03  **** 480  | 0x13  **** 28  | 0x23  **** 256  | 0x33  ******
 ****** 48  | 0x04  **** 576  | 0x14  **** 32  | 0x24  **** 320  | 0x34  ******
 ****** 52  | 0x05  **** 640  | 0x15  **** 36  | 0x25  **** 384  | 0x35  ******
 ****** 60  | 0x06  **** 768  | 0x16  **** 40  | 0x26  **** 448  | 0x36  ******
 ****** 72  | 0x07  **** 960  | 0x17  **** 44  | 0x27  **** 512  | 0x37  ******
 ****** 80  | 0x08  **** 1152 | 0x18  **** 48  | 0x28  **** 640  | 0x38  ******
 ****** 88  | 0x09  **** 1280 | 0x19  **** 56  | 0x29  **** 768  | 0x39  ******
 ****** 104 | 0x0A  **** 1536 | 0x1A  **** 64  | 0x2A  **** 896  | 0x3A  ******
 ****** 128 | 0x0B  **** 1920 | 0x1B  **** 72  | 0x2B  **** 1024 | 0x3B  ******
 ****** 144 | 0x0C  **** 2304 | 0x1C  **** 80  | 0x2C  **** 1280 | 0x3C  ******
 ****** 160 | 0x0D  **** 2560 | 0x1D  **** 96  | 0x2D  **** 1536 | 0x3D  ******
 ****** 192 | 0x0E  **** 3072 | 0x1E  **** 112 | 0x2E  **** 1792 | 0x3E  ******
 ****** 240 | 0x0F  **** 3840 | 0x1F  **** 128 | 0x2F  **** 2048 | 0x3F  ******
 ******************************************************************************
 */

/**
 * @brief     i2c 初始化
 * @param[in] port      I2C硬件端口选择，详见 # I2C_PORT_T
 * @param[in] I2cClkParam 分频参数设置(范围0x00-0x3f)，详见 # i2c div table
 * @param[in] OwnAddr   i2c做slave，需要配置自身地址
 *                      i2c做master，可随意配置
 *
 * @return    state     详见 # I2C_ErrorState
 */
I2C_ErrorState I2C_Init(I2C_PORT_T port,uint8_t I2cClkParam, uint8_t OwnAddr);

/**
 * @brief   i2c使能
 * @param[in] port      I2C硬件端口选择，详见 # I2C_PORT_T
 * @param	无
 *
 * @return  无 
 */
void I2C_Enable(I2C_PORT_T port);

/**
 * @brief     i2c禁用
 *
 * @param[in] port      I2C硬件端口选择，详见 # I2C_PORT_T
 *
 * @return  无  
 */
void I2C_Disable(I2C_PORT_T port);

/**
 * @brief     i2c使能DMA
 *
 * @param[in] port      I2C硬件端口选择，详见 # I2C_PORT_T
 *
 * @return
 */
void I2C_DMAEnable(I2C_PORT_T port);


/**
 * @brief     i2c禁用DMA
 *
 * @param[in] port      I2C硬件端口选择，详见 # I2C_PORT_T
 *
 * @return
 */
void I2C_DMADisable(I2C_PORT_T port);

/**
 * @brief     开启i2c中断使能
 *
 * @param[in] flag    使能标志
 *
 * @return   无 
 */
void I2C_IntEn(I2C_PORT_T port,uint8_t flag);

/**
 * @brief     清除i2c中断标志
 *
 * @param[in] port      I2C硬件端口选择，详见 # I2C_PORT_T
 *
 * @return   无 
 */
void I2C_IntClr(I2C_PORT_T port);

/**
 * @brief     获取i2c中断标志
 *
 * @param[in] port      I2C硬件端口选择，详见 # I2C_PORT_T
 *
 * @return   获取中断标志。 
 *			  0：无中断信号产生
 *            1：有中断信号产生
 */
bool I2C_GetIntFlag(I2C_PORT_T port);

/**
 * @brief     查询i2c空闲状态
 *
 * @param[in] 无
 *
 * @return    I2C总线状态。
 *				0：I2C空闲状态
 *            	1：I2C忙状态
 */
bool I2C_IsBusy(I2C_PORT_T port);

/**
 * @brief     i2c通讯地址是否匹配
 *
 * @param[in] port      I2C硬件端口选择，详见 # I2C_PORT_T
 *
 * @return    i2c通讯地址匹配情况
 *				0： 地址不匹配
 *	            1： 地址匹配
 */
bool I2C_IsAddressMatched(I2C_PORT_T port);

/**
 * @brief     i2c仲裁是否丢失
 *
 * @param[in] 无
 *
 * @return    i2c仲裁情况。
 *				0:仲裁未丢失;1：仲裁丢失
 */
bool I2C_IsArbitrationLost(I2C_PORT_T port);

/**
 * @brief     等待i2c空闲状态
 *
 * @param[in] port      I2C硬件端口选择，详见 # I2C_PORT_T
 * @param[in] timeout    超时时间设置
 *
 * @return    i2c status 详见 I2C_StatusTypeDef
 */
I2C_StatusTypeDef I2C_WaitForIdle(I2C_PORT_T port,uint32_t timeout);

/**
 * @brief     i2c发送一个开始信号
 * 
 * @param[in] port      I2C硬件端口选择，详见 # I2C_PORT_T
 * @param[in] 无
 *
 * @return    无  
 */
void I2C_SendStart(I2C_PORT_T port);

/**
 * @brief     i2c发送一个重复开始信号
 *
 * @param[in] port      I2C硬件端口选择，详见 # I2C_PORT_T
 *
 * @return    无
 */
void I2C_SendReStart(I2C_PORT_T port);

/**
 * @brief     i2c发送一个停止信号
 *
 * @param[in] port      I2C硬件端口选择，详见 # I2C_PORT_T
 *
 * @return    无
 */
void I2C_SendStop(I2C_PORT_T port);

/**
 * @brief     i2c发送一个ACK信号
 *
 * @param[in] port      I2C硬件端口选择，详见 # I2C_PORT_T
 *
 * @return    无
 */
void I2C_SendAck(I2C_PORT_T port);

/**
 * @brief     i2c发送一个NoACK信号
 *
 * @param[in] port      I2C硬件端口选择，详见 # I2C_PORT_T
 *
 * @return    无
 */
void I2C_SendNoAck(I2C_PORT_T port);

/**
 * @brief     i2c接收ACK信号
 *
 * @param[in] port      I2C硬件端口选择，详见 # I2C_PORT_T
 *
 * @return    i2c接收ACK信号
 *				0：接收到NoACK信号
 *            	1：接收到ACK信号
 */
bool I2C_ReceiveAcknowledge(I2C_PORT_T port);

/**
 * @brief     i2c发送一字节数据
 *
 * @param[in] port      I2C硬件端口选择，详见 # I2C_PORT_T
 * @param[in] value     单位: byte
 *
 * @return    无
 */
void I2C_SendByte(I2C_PORT_T port,uint8_t value);

/**
 * @brief     i2c接收一字节数据
 *
 * @param[in] port      I2C硬件端口选择，详见 # I2C_PORT_T
 * @param[in] 无
 *
 * @return    value     单位: byte
 */
uint8_t I2C_ReceiveByte(I2C_PORT_T port);

/*************************** I2C 主模式 ****************************/

/**
 * @brief     i2c主模式使能
 *
 * @param[in] port      I2C硬件端口选择，详见 # I2C_PORT_T
 *
 * @return    无
 */
void I2C_MasterEnable(I2C_PORT_T port);

/**
 * @brief     i2c主模式配置
 *
 * @param[in] port      I2C硬件端口选择，详见 # I2C_PORT_T
 *
 * @return    无
 */
void I2C_MasterConfig(I2C_PORT_T port);

/**
 * @brief     I2C主模式下,开始接收从设备发送的数据
 *
 * @param[in] port      I2C硬件端口选择，详见 # I2C_PORT_T
 *
 * @return    无
 */
void I2C_ReceiveStart(I2C_PORT_T port);


/*************************** I2C 从模式 ****************************/

/**
 * @brief     i2c从模式使能
 *
 * @param[in] port      I2C硬件端口选择，详见 # I2C_PORT_T
 *
 * @return    无
 */
void I2C_SlaveEnable(I2C_PORT_T port);

/**
 * @brief     i2c从模式配置
 *
 * @param[in] port      I2C硬件端口选择，详见 # I2C_PORT_T
 *
 * @return    无
 */
void I2C_SlaveConfig(I2C_PORT_T port);

/**
 * @brief     i2c从模式配置启动和停止事件响应时间宽度（滤除毛刺）
 *
 * @param[in] value：取值范围为0-15
 *
 * @return
 */
void I2C_SlaveWidSet(I2C_PORT_T port,uint8_t start_value,uint8_t stop_value);

/**
 * @brief     获取i2c接收/发送状态
 *
 * @param[in] port      I2C硬件端口选择，详见 # I2C_PORT_T
 * @param[in] 无
 *
 * @return    0 从模式接收状态, 接收主模式发送的数据
 *            1 从模式发送状态, 发送数据给主模式
 */
bool I2C_SlaveReadWriteGet(I2C_PORT_T port);

/**
 * @brief     释放i2c
 *
 * @param[in] port      I2C硬件端口选择，详见 # I2C_PORT_T
 *
 * @return    无
 */
void I2C_SlaveReleaseI2C(I2C_PORT_T port);

/**
 * @brief     I2C从模式下,等待写请求
 *
 * @param[in] 无
 *
 * @return    i2c state   详见 I2C_ErrorState
 */
I2C_ErrorState I2C_SlaveRequestWrite(I2C_PORT_T port);

/**
 * @brief     I2C从模式下,等待读请求
 *
 * @param[in] port      I2C硬件端口选择，详见 # I2C_PORT_T
 *
 * @return    i2c state   详见 I2C_ErrorState
 */
I2C_ErrorState I2C_SlaveRequestRead(I2C_PORT_T port);

/**
 * @brief     I2C时钟拉伸模式设置
 *
 * @param[in] port      I2C硬件端口选择，详见 #I2C_PORT_T
 * @param[in] mode      时钟拉伸模式选择，详见 #I2C_StretchModeTypeDef
 *                      - I2C_STRETCH_MODE_ORIGINAL: 原始模式，全程时钟拉伸
 *                      - I2C_STRETCH_MODE_IMPROVED: 改进模式，仅字节传输完成后拉伸
 *                      - I2C_STRETCH_MODE_NONE: 无时钟拉伸
 *
 * @return    无
 */
void I2C_StretchModeSet(I2C_PORT_T port,I2C_StretchModeTypeDef mode);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif //__I2C_H__

/**
 * @}
 * @}
 */
