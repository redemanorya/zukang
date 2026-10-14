/**
  *****************************************************************************
  * @file:			spis.h
  * @author			grayson
  * @version		V1.0.0
  * @data			18-June-2017
  * @Brief			SPI Slave driver header file.
  * @note			For sdio and spi can't access memory simultaneously
  ******************************************************************************
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, MVSILICON SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
  * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
  */

/**
 * @addtogroup SPIS
 * @{
 * @defgroup spis spis.h
 * @{
 */
#ifndef __SPIS_H__
#define __SPIS_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

/**
 * @brief      SPIS mode
 */
typedef enum
{
	SPIS0 = 0,
	SPIS1,
}SPIS_PORT_T;


///////////////////////////////////////////////////////////////////

/**
 * @brief
 *    初始化SPIS模块
 * @param	SPIModule
 * 				选择SPIM0还是SPIM1
 * @param	ClkMode
 *					0 - CPOL = 0 & CPHA = 0, idle为低电平,第一个边沿采样(上升沿)
 *					1 - CPOL = 0 & CPHA = 1, idle为低电平,第二个边沿采样(下降沿)
 *					2 - CPOL = 1 & CPHA = 0, idle为高电平,第一个边沿采样(下降沿)
 *					3 - CPOL = 1 & CPHA = 1, idle为高电平,第一个边沿采样(上升沿)
 * @return
 *   无
 * @note
 */
void SPIS_Init(SPIS_PORT_T SPIModule, uint8_t ClkMode);


//--------------------------------MCU MODE-------------------------------------

/**
 * @brief
 *    SPIS0 MCU方式发送数据单位Byte
 * @param	SPIModule
 * 				选择SPIM0还是SPIM1
 * @param	SendBuf			数据首地址
 * @param	BufLen			发送数据长度
 * @param	TimeOut			超时时间
 *
 * @return
 *   返回实际发送数据长度单位Byte
 * @note
 */
uint32_t SPIS_Send(SPIS_PORT_T SPIModule, uint8_t* SendBuf, uint32_t BufLen, uint32_t TimeOut);


/**
 * @brief
 *    SPIS MCU方式接收数据，在TimeOut时间内等待接收完成，超时返回实际接收数据个数，单位Byte
 * @param	SPIModule
 * 				选择SPIM0还是SPIM1
 * @param	RecvBuf			数据首地址
 * @param	BufLen			接收数据长度
 * @param	TimeOut			超时时间
 *
 * @return
 *   返回实际接收数据长度
 * @note
 */
uint32_t SPIS_Recv(SPIS_PORT_T SPIModule, uint8_t* RecvBuf, uint32_t BufLen, uint32_t TimeOut);


/**
 * @brief
 *      获取 SPIS 模块的 CS 状态
 * @param	SPIModule
 * 				选择SPIS0还是SPIS1
 * @return
 *      返回 CS 状态
 *      	0 spis 未被选中
 *      	1 spis 已被选中
 * @note
 */
bool SPIS_GetCSStatus(SPIS_PORT_T SPIModule);


/**
 * @brief
 *      获取 SPIS 模块的 TX FIFO 空状态
 * @param[in]   SPIModule    SPIS 模块选择，指定要查询的 SPIS 模块
 * @return
 *      返回 TX FIFO 状态
 *      	0 FIFO为空
 *      	1 FIFO不为空
 * @note
 */
bool SPIS_GetTXFIFOEmptyStatus(SPIS_PORT_T SPIModule);


/**
 * @brief
 *    接收中断使能
 *
 * @param	SPIModule
 * 				选择SPIS0还是SPIS1
 * @param	En			1：开启中断，0：关闭中断
 *
 * @return
 *   无
 * @note
 *	 每接收一个Byte，起一次中断
 */
void SPIS_RxIntEn(SPIS_PORT_T SPIModule, bool En);


/**
 * @brief
 *    获取接收中断标志
 * @param	SPIModule
 * 				选择SPIS0还是SPIS1
 * @return
 *   返回接收中断标志
 * @note
 *	 每接收一个Byte，起一次中断
 */
bool SPIS_GetRxIntFlag(SPIS_PORT_T SPIModule);


/**
 * @brief
 *   清除接收中断标志
 * @param	SPIModule
 * 				选择SPIS0还是SPIS1
 * @return
 *   无
 * @note
 *	 每接收一个Byte，起一次中断
 */
void SPIS_ClrRxIntFlag(SPIS_PORT_T SPIModule);


/**
 * @brief
 *    获取接收溢出标志
 * @param	SPIModule
 * 				选择SPIS0还是SPIS1
 * @return
 *   返回接收溢出标志
 * @note
 *	 SPIS 内部4Byte 硬件Rx FIFO满后再次收到一个Byte，起接收溢出标志
 */
bool SPIS_GetRxOverrunFlag(SPIS_PORT_T SPIModule);


/**
 * @brief
 *    清除接收溢出标志
 * @param	SPIModule
 * 				选择SPIS0还是SPIS1
 * @return
 *   无
 * @note
 *	 SPIS 内部4Byte 硬件Rx FIFO满后再次收到一个Byte，起接收溢出标志
 */
void SPIS_ClrRxOverrunFlag(SPIS_PORT_T SPIModule);


/**
 * @brief
 *    LSB/MSB传输设置
 * @param	SPIModule
 * 				选择SPIS0还是SPIS1
 * @param	1--LSB先行传输
 *          0--MSB先行传输
 * @return
 *   无
 * @note
 *   在SPIS_Init()之后设置，若不设置则默认MSB先行传输
 */
void SPIS_LSBFirstSet(SPIS_PORT_T SPIModule, bool EN);


/**
 * @brief
 *   清空接收缓冲区FIFO
 * @param	SPIModule
 * 				选择SPIS0还是SPIS1
 *   无
 * @return
 *   无
 * @note
 *
 */
void SPIS_ClrRxFIFO(SPIS_PORT_T SPIModule);


/**
 * @brief
 *   清空发送缓冲区FIFO
 * @param	SPIModule
 * 				选择SPIS0还是SPIS1
 * @return
 *   无
 * @note
 *
 */
void SPIS_ClrTxFIFO(SPIS_PORT_T SPIModule);


//--------------------------------DMA MODE-------------------------------------


/**
 * @brief
 *    SPIS DMA方式发送数据
 * @param	SPIModule
 * 				选择SPIS0还是SPIS1
 * @param	SendBuf			数据首地址
 * @param	BufLen			发送数据长度
 * @param	TimeOut			超时时间
 *
 * @return
 *   返回实际发送数据长度单位Byte
 * @note
 */
uint32_t SPIS_DMA_Send(SPIS_PORT_T SPIModule, uint8_t* SendBuf, uint32_t BufLen, uint32_t TimeOut);

/**
 * @brief
 *    SPIS DMA方式接收数据
 * @param	SPIModule
 * 				选择SPIS0还是SPIS1
 * @param	RecvBuf			数据首地址
 * @param	BufLen			接收数据长度
 * @param	TimeOut			超时时间
 *
 * @return
 *   返回实际接收数据长度单位Byte
 * @note
 */
uint32_t SPIS_DMA_Recv(SPIS_PORT_T SPIModule, uint8_t* RecvBuf, uint32_t BufLen, uint32_t TimeOut);


/**
 * @brief
 *      设置 SPIS 模块为 DMA 模式
 * @param	SPIModule
 * 				选择SPIS0还是SPIS1
 * @return
 *      无返回值
 * @note
 */
void SPIS_TxDmaModeSet(SPIS_PORT_T SPIModule, bool EN);


/**
 * @brief
 *    DMA方式接收数据模式设置
 * @param	SPIModule
 * 				选择SPIS0还是SPIS1
 * @param	1--使用DMA接收数据，
 *          0--不使用DMA接收数据
 * @return
 *   无
 * @note
 *
 */
void SPIS_RxDmaModeSet(SPIS_PORT_T SPIModule, bool EN);


/**
 * @brief
 *   初始化SPIS收发的BlockDMA模式
 * @param	SPIModule
 * 				选择SPIS0还是SPIS1
 * @return
 *   无
 * @note
 *
 */
bool SPIS_BlockDMA_Init(SPIS_PORT_T SPIModule);

/**
 * @brief
 *   查询SPIS使用DMA方式接收数据状态
 * @param
 * 无
 * @return
 *   0：没有产生DMA数据接收完成标志  1：产生DMA数据接收完成标志
 * @note
 *
 */
bool SPIS_DMA_RecvState(SPIS_PORT_T SPIModule);

/**
 * @brief
 *   查询SPIS使用DMA方式接收和发送数据状态
 * @param
 * 无
 * @return
 *   0：没有产生DMA数据接收完成标志  1：产生DMA数据接收和发送完成标志
 * @note
 *
 */
bool SPIS_DMA_SendRecvState(SPIS_PORT_T SPIModule);
#ifdef  __cplusplus
}
#endif//__cplusplus

#endif //__SPIS_H__

/**
 * @}
 * @}
 */

