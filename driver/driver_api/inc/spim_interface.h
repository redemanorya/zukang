/* @file    Spim_interface.h
 * @brief   Spi  interface
 *
 * @author  Owen
 * @version V1.0.0 	initial release
 *
 * $Id$
 * $Created: 2018-11-29 17:31:10$
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */


#ifndef SPIM_INTERFACE_H_
#define SPIM_INTERFACE_H_


/**
 * @brief
 *      选择SPIM模块功能引脚
 * @param PortSel
 *      0  -
 *          spim MISO: A5
 *          spim CLK:  A6
 *          spim MOSI: A7
 *      1  -
 *          spim MISO: A12
 *          spim CLK:  A11
 *          spim MOSI: A10
 *      2  -
 *          spim MISO: B2
 *          spim CLK:  B1
 *          spim MOSI: B0
 *      3  -
 *          spim io0:  A7
 *          spim io1:  A5
 *          spim io2:  A3
 *          spim io3:  A2
 *          spim CLK:  A6
 *      4  -
 *          spim io0:  A10
 *          spim io1:  A12
 *          spim io2:  A14
 *          spim io3:  A15
 *          spim CLK:  A11
 *      5  -
 *          spim io0:  B0
 *          spim io1:  B2
 *          spim io2:  B4
 *          spim io3:  B5
 *          spim CLK:  B1
 * @return
 *      无
 * @note
 */
bool GPIO_SpimIoConfig(uint8_t GPIO_Mode);


/**
 * @brief
 *		DMA完成标志
 * @param	PeripheralID		接收数据DMA通道
 * @return
 *		DMA完成标志
 * @note
 */
bool SPIM_DMA_Done(uint8_t PeripheralID);


/**
 * @brief
 *		清除DMA完成标志
 * @param	PeripheralID		接收数据DMA通道
 * @return
 *		无
 * @note
 */
void SPIM_DMA_Done_Clear(uint8_t PeripheralID);


/**
 * @brief
 *      启动 SPIM 模块的 DMA 发送操作。
 * @param SPIModule
 *      SPIM 模块选择：
 *          SPIM0 - 选择 SPIM0 模块
 *          SPIM1 - 选择 SPIM1 模块
 * @param PeripheralID   外设 ID，用于指定 DMA 通道。
 * @param SendBuf        发送数据的首地址，指向要发送的数据缓冲区。
 * @param Length         发送数据的长度（单位：Byte），表示要发送的数据字节数。
 * @param TransferMode
 *      SPI传输模式选择：
 *          0 - 标准模式
 *          1 - 4bit模式
 * @return
 *      无
 * @note
 */
void SPIM_DMA_Send_Start(SPIM_PORT_T SPIModule, uint8_t PeripheralID, uint8_t* SendBuf, uint32_t Length, uint8_t TransferMode);


/**
 * @brief
 *		启动DMA模式收数据
 * @param SPIModule
 *      SPIM 模块选择：
 *          SPIM0 - 选择 SPIM0 模块
 *          SPIM1 - 选择 SPIM1 模块
 * @param	PeripheralID DMA通道ID，指定DMA通道
 * @param	RevBuf       接收数据的首地址，指向要发送的数据缓冲区
 * @param   Length       接收数据的长度，单位为字节
 * @param TransferMode
 *      SPI传输模式选择：
 *          0 - 标准模式
 *          1 - 4bit模式
 * @return
 *		无返回值
 * @note
 */
void SPIM_DMA_Recevie_Start(SPIM_PORT_T SPIModule, uint8_t PeripheralID, uint8_t*RevBuf, uint32_t Length, uint8_t TransferMode);


/**
 * @brief
 *		通过 SPIM 模块启动 DMA 模式同时发送和接收数据
 * @param	SPIModule	指定的 SPIM 模块
 * @param	RevPeripheralID	接收外设 ID
 * @param	TxPeripheralID	发送外设 ID
 * @param	SendBuf	发送数据缓冲区指针
 * @param	RecvBuf	接收数据缓冲区指针
 * @param	Length	数据长度（单位：Byte），最大65536 byte
 * @param	TransferMode
 *      SPI传输模式选择：
 *          0 - 标准模式
 *          1 - 4bit模式
 * @return
 *		无
 * @Note
 *		1. SPIM 中断标志置位表示收发数据完成，可以通过查询或中断方式处理。
 *		2. 发送和接收缓冲区必须在 DMA 可访问的内存范围内。
 */
void SPIM_DMA_Send_Receive_Start(SPIM_PORT_T SPIModule, uint8_t RevPeripheralID, uint8_t TxPeripheralID, uint8_t* SendBuf, uint8_t*RecvBuf, uint32_t Length, uint8_t TransferMode);


/**
 * @brief
 *		启动DMA模式收发数据（全双工）
 * @param	SPIModule    SPIM模块选择，指定要使用的SPIM模块
 * @param	SendBuf      发送数据的首地址，指向要发送的数据缓冲区
 * @param	RecvBuf      接收数据的首地址，指向要接收的数据缓冲区
 * @param	Length       收发数据的长度，单位为字节
 * @param	TransferMode
 *      SPI传输模式选择：
 *          0 - 标准模式
 *          1 - 4bit模式
 * @return
 *		无返回值
 * @note
 */
void SpimDataSendRecvByDMA(SPIM_PORT_T SPIModule, uint8_t* SendBuf, uint8_t* RecvBuf, uint32_t Length, uint8_t TransferMode);


/**
 * @brief
 *		启动DMA模式发送数据
 * @param	SPIModule    SPIM模块选择，指定要使用的SPIM模块
 * @param	SendBuf      发送数据的首地址，指向要发送的数据缓冲区
 * @param	Length       发送数据的长度，单位为字节
 * @param	TransferMode
 *      SPI传输模式选择：
 *          0 - 标准模式
 *          1 - 4bit模式
 * @return
 *		无返回值
 * @note
 */
void SpimDataSendByDMA(SPIM_PORT_T SPIModule, uint8_t* SendBuf, uint32_t Length, uint8_t TransferMode);


/**
 * @brief
 *		启动DMA模式接收数据
 * @param	SPIModule    SPIM模块选择，指定要使用的SPIM模块
 * @param	RecvBuf      接收数据的首地址，指向要接收的数据缓冲区
 * @param	Length       接收数据的长度，单位为字节
 * @param	TransferMode
 *      SPI传输模式选择：
 *          0 - 标准模式
 *          1 - 4bit模式
 * @return
 *		无返回值
 * @note
 */
void SpimDataRecvByDMA(SPIM_PORT_T SPIModule, uint8_t* RecvBuf, uint32_t Length, uint8_t TransferMode);

#endif /* SPIM_INTERFACE_H_ */
