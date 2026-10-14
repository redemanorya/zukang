/**
 **************************************************************************************
 * @file    uarts_interface.h
 * @brief   uarts_interface
 *
 * @author  Sam
 * @version V1.1.0
 *
 * $Created: 2018-06-05 15:17:05$
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#ifndef __UARTS_INTERFACE_H__
#define __UARTS_INTERFACE_H__

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus 

#include "type.h"
#include "uarts.h"
#include "dma.h"

/*UART MCU mode*/
/**
 * @brief      初始化串口函数
 *             配置基础参数，波特率，数据位，校验位，停止位配置
 * @param[in]  UART_MODULE      UART硬件模块选择
 * @param[in]  BaudRate   串口波特率配置，DPLL@288MHz或者 AUPLL@240MHz模式下:波特率范围1200bps~3000000bps；RC@12M波特率范围1200bps~115200bps
 * @param[in]  DataBits   数据位长度选择，支持5-8bit，及参数输入（5，6，7，8）；
 * @param[in]  Parity     校验位选择，输入参数分别对应 0:无校验 1:奇校验 2:偶校验
 * @param[in]  StopBits   停止位选择，输入参数分别对应1：1位停止位 2：2位停止位
 * @return     			  FALSE参数错误	TRUE参数初始化完成
 */
#define UARTS_Init(UartNum,BaudRate,DataBits,Parity,StopBits)  UART_Init(UartNum,BaudRate,DataBits,Parity,StopBits)

/**
 * @brief      串口控制位操作函数
 *             通过选择串口控制位Cmd以及传入的参数Arg来设置相关配置或者读取相关配置
 * @param[in]  UART_MODULE      UART硬件模块选择
 * @param[in]  Cmd  	控制指令选择，选择UART_IOCTL_CMD_T枚举量
 * @param[in]  Arg  	指令参数，根据Cmd指令写入需要的参数
 * @return     在对控制指令操作成功的前提下，会根据情况返回下列结果：
 * 				如UART_IOCTL_TXSTAT_GET等获取状态：即返回状态值
 * 				如UART_IOCTL_TXINT_SET等设置控制位，即返回0，操作成功
 * 			   返回EINVAL（22）即非法操作，请检查Cmd与Arg参数是否正确
 */
#define UARTS_IOCTL(UartNum,cmd,Arg)  UART_IOCtl(UartNum,cmd,Arg)

/**
 * @brief      串口多字节接收函数
 * @param[in]  UART_MODULE      UART硬件模块选择
 * @param[in]  RecvBuf  	接收数据缓冲区地址
 * @param[in]  BufLen   	接收数据缓冲区字节长度
 * @param[in]  TimeOut   	超时退出时间
 * @return     接收到的数据长度
 */
#define UARTS_Recv(UartNum, RecvBuf, BufLen,TimeOut) UART_Recv(UartNum, RecvBuf, BufLen,TimeOut)

/**
 * @brief      串口单字节接收函数
 * @param[in]  UartNum  UART硬件模块选择，0:UART0 1:UART1
 * @param[in]  Val  	接收数据存放地址
 * @return     FALSE    未接收到数据	TRUE接收到数据
 */
#define UARTS_RecvByte(UartNum, Val) UART_RecvByte(UartNum, Val)

/**
 * @brief      串口多字节发送函数
 * @param[in]  UART_MODULE      UART硬件模块选择
 * @param[in]  SendBuf  	发送数据缓冲区地址
 * @param[in]  BufLen   	发送数据字节长度
 * @param[in]  TimeOut   	超时退出时间
 * @return     发送长度
 */
uint32_t UARTS_Send(UART_PORT_T UART_MODULE,uint8_t* SendBuf, uint32_t BufLen,uint32_t TimeOut);

/**
 * @brief      串口单字节发送函数
 * @param[in]  UART_MODULE      UART硬件模块选择
 * @param[in]  SendByte  	需发送的数据
 * @return     无返回值
 */
void UARTS_SendByte(UART_PORT_T UART_MODULE,uint8_t SendByte);

/**
 * @brief      串口单字节发送
 *             注：发送后不需要等待发送是否完成，而当发送完成后，会发起发送完成中断。（在中断函数中需要注意清TX DONE中断）
 * @param[in]  UART_MODULE      UART硬件模块选择
 * @param[in]  SendByte  	需发送的数据
 * @return     无返回值
 */
void UARTS_SendByte_In_Interrupt(UART_PORT_T UART_MODULE,uint8_t SendByte);

/*UART DMA mode*/

/**
 * @brief      UART DMA 接收模式初始化，配置基础参数，接收缓存地址，接收缓存长度，中断触发门槛值，回调函数
 * @param[in]  UART_MODULE      UART硬件模块选择
 * @param[in]  RxBufAddr  	接收缓冲区地址
 * @param[in]  RxBufLen   	接收缓存区长度设置
 * @param[in]  ThresholdLen 中断触发门槛值设置，注意设置值小于RxBufLen
 * @param[in]  CallBack   	中断回调函数
 * @return     FALSE初始化配置失败	TRUE初始化设置成功
 */
bool UARTS_DMA_RxInit(UART_PORT_T UART_MODULE,void* RxBufAddr, uint16_t RxBufLen, uint16_t ThresholdLen, INT_FUNC CallBack);

/**
 * @brief      UART DMA 传送模式初始化，配置基础参数，发送缓存地址，发送缓存长度，中断触发门槛值，回调函数
 * @param[in]  UART_MODULE      UART硬件模块选择
 * @param[in]  TxBufAddr  	传送缓冲区地址
 * @param[in]  TxBufLen   	传送缓存区长度设置
 * @param[in]  ThresholdLen 中断触发门槛值设置，注意设置值小于RxBufLen
 * @param[in]  CallBack   	中断回调函数
 * @return     FALSE初始化配置失败	TRUE初始化设置成功
 */
bool UARTS_DMA_TxInit(UART_PORT_T UART_MODULE,void* TxBufAddr, uint16_t TxBufLen, uint16_t ThresholdLen, INT_FUNC CallBack);

/**
 * @brief      UART DMA 传送模式初始化，发送缓存区地址，发送缓存长度，中断触发门槛值，回调函数
 * @param[in]  UART_MODULE      UART硬件模块选择
 * @param[in]  TxBufAddr  	传送缓冲区地址
 * @param[in]  TxBufLen   	传送字符长度
 * @param[in]  TimeOut 		超时时间设置，即达到设置的超时时间还未发送完成，则放弃发送
 * @return     实际发送数据长度
 */
uint32_t UARTS_DMA_Send(UART_PORT_T UART_MODULE,uint8_t* SendBuf, uint16_t BufLen, uint32_t TimeOut);

/**
 * @brief      UART DMA 接收数据，设置接收缓存地址，接收字符长度，接收超时设置
 * @param[in]  UART_MODULE      UART硬件模块选择
 * @param[in]  RecvBuf  	接收缓冲区地址
 * @param[in]  BufLen   	接收长度设置
 * @param[in]  TimeOut		超时时间设置，规定时间内接收没有达到预期长度或者没收到收据则放弃接收
 * @return     接收到数据的实际长度
 */
uint32_t UARTS_DMA_Recv(UART_PORT_T UART_MODULE,uint8_t* RecvBuf, uint16_t BufLen, uint32_t TimeOut);

/**
 * @brief      UART DMA 传送数据，设置传入传送缓存区地址，传送字符长度
 * @param[in]  UART_MODULE      UART硬件模块选择
 * @param[in]  RecvBuf  	传送缓冲区地址
 * @param[in]  BufLen   	传送字符长度设置
 * @return     无返回值
 */
void UARTS_DMA_SendDataStart(UART_PORT_T UART_MODULE,uint8_t* SendBuf, uint16_t BufLen);

/**
 * @brief      判断数据传送是否完成
 * @param[in]  UART_MODULE      UART硬件模块选择
 * @return     TURE传送完成 	 FALSE传送未完成
  */
bool UARTS_DMA_TxIsTransferDone(UART_PORT_T UART_MODULE);

/**
 * @brief      UART DMA 	非阻塞方式使能接收，设置接收缓存地址，接收字符长度
 * @param[in]  UART_MODULE      UART硬件模块选择
 * @param[in]  RecvBuf  	接收缓冲区地址
 * @param[in]  BufLen   	接收长度设置
 * @return     接收到数据的实际长度
 */
int32_t UARTS_DMA_RecvDataStart(UART_PORT_T UART_MODULE,uint8_t* RecvBuf, uint16_t BufLen);

/**
 * @brief      判断是否传输完成
 * @param[in]  UART_MODULE      UART硬件模块选择
 * @return     TURE传送完成 	 FALSE传送未完成
  */
bool UARTS_DMA_RxIsTransferDone(UART_PORT_T UART_MODULE);

/**
 * @brief      查询DMA接收缓存区数据长度
 * @param[in]  UART_MODULE      UART硬件模块选择
 * @return     返回数据长度
  */
int32_t UARTS_DMA_RxDataLen(UART_PORT_T UART_MODULE);

/**
 * @brief      查询DMA发送缓存区数据长度
 * @param[in]  UART_MODULE      UART硬件模块选择
 * @return     返回数据长度
  */
int32_t UARTS_DMA_TxDataLen(UART_PORT_T UART_MODULE);

/**
 * @brief      注册UART_DMA模式下的中断回调函数，注意配置中断类型
 * @param[in]  UART_MODULE      UART硬件模块选择
 * @param[in]  IntType  	中断类型设置: 1.DMA_DONE_INT无中断    2.DMA_THRESHOLD_INT阈值触发中断    3.DMA_ERROR_INT错误中断
 * param[in]   CallBack		需注册的中断回掉函数，若写入NULL，则关闭回调功能
 * @return     返回数据长度
  */
void UARTS_DMA_RxSetInterruptFun(UART_PORT_T UART_MODULE,DMA_INT_TYPE IntType, INT_FUNC CallBack);
void UARTS_DMA_TxSetInterruptFun(UART_PORT_T UART_MODULE,DMA_INT_TYPE IntType, INT_FUNC CallBack);

/**
 * @brief      UART_DMA使能函数
 * @param[in]  UART_MODULE      UART硬件模块选择
 * @return
  */
void UARTS_DMA_RxChannelEn(UART_PORT_T UART_MODULE);
void UARTS_DMA_TxChannelEn(UART_PORT_T UART_MODULE);

/**
 * @brief      UART_DMA禁用函数
 * @param[in]  UART_MODULE      UART硬件模块选择
 * @return
  */
void UARTS_DMA_RxChannelDisable(UART_PORT_T UART_MODULE);
void UARTS_DMA_TxChannelDisable(UART_PORT_T UART_MODULE);

/**
 * @brief      UART_DMA 中断标志位清除
 * @param[in]  UART_MODULE      UART硬件模块选择
 * @param[in]  IntType  	中断类型设置	DMA_DONE_INT无中断DMA_THRESHOLD_INT阈值触发中断DMA_ERROR_INT错误中断
 * @return  	DMA_ERROR
 * @note	中断标志在不开启中断的情况下也会置位
  */
int32_t UARTS_DMA_TxIntFlgClr(UART_PORT_T UART_MODULE,DMA_INT_TYPE IntType);
//RX flag clear
int32_t UARTS_DMA_RxIntFlgClr(UART_PORT_T UART_MODULE,DMA_INT_TYPE IntType);
//add new APIs here...

#ifdef __cplusplus
}
#endif // __cplusplus 

#endif//__UARTS_INTERFACE_H__
