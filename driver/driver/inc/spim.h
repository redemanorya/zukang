/**
  *****************************************************************************
  * @file:			spim.c
  * @author			Lilu
  * @version		V1.0.0
  * @data			2014-11-20
  * @Brief			SPI Master driver interface
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
 * @addtogroup SPIM
 * @{
 * @defgroup spim spim.h
 * @{
 */
 
 
#ifndef __SPIM_H__
#define __SPIM_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


///////////////////////////////////////////////////////////////////

/**
 * @brief      SPIM mode
 */
//SPIM时钟=(PLL_CLK/2/(REG_SPIM_CLK_DIV_NUM+1))/2*(SREG_SPIM_CTRL.CRAT+1)     REG_SPIM_CLK_DIV_NUM默认等于1
/*注意：此处宏定义均建立在PLL时钟为288M情况下*/
//Max 30Mhz
#define	SPIM_CLK_DIV_30M	0x0			/**< SPI master clock Div 30MHz*/
#define	SPIM_CLK_DIV_18M	0x1			/**< SPI master clock Div 18MHz*/
#define	SPIM_CLK_DIV_12M 	0x2			/**< SPI master clock Div 12MHz*/
#define	SPIM_CLK_DIV_9M     0x3			/**< SPI master clock Div 9MHz*/
#define	SPIM_CLK_DIV_7M2	0x4			/**< SPI master clock Div 7.2MHz*/
#define	SPIM_CLK_DIV_6M		0x5			/**< SPI master clock Div 6MHz*/
#define	SPIM_CLK_DIV_5M		0x6			/**< SPI master clock Div 5MHz*/
#define	SPIM_CLK_DIV_4M5	0x7			/**< SPI master clock Div 4.5MHz*/
#define	SPIM_CLK_DIV_4M	    0x8			/**< SPI master clock Div 4MHz*/
#define	SPIM_CLK_DIV_3M6	0x9			/**< SPI master clock Div 3.6MHz*/
#define	SPIM_CLK_DIV_3M27	0xa			/**< SPI master clock Div 3.27MHz*/
#define	SPIM_CLK_DIV_3M		0xb			/**< SPI master clock Div 3MHz*/
#define	SPIM_CLK_DIV_2M77	0xc			/**< SPI master clock Div 2.77MHz*/
#define	SPIM_CLK_DIV_2M57	0xd			/**< SPI master clock Div 2.57MHz*/
#define	SPIM_CLK_DIV_2M4	0xe			/**< SPI master clock Div 2.4MHz*/
#define	SPIM_CLK_DIV_2M25	0xf			/**< SPI master clock Div 2.25MHz*/

typedef enum
{
	SPIM0 = 0,
	SPIM1,
}SPIM_PORT_T;

typedef enum
{
	StandardMode = 0,
	QuadMode,
}FLASH_MODE_T;

/**
 * err code define
 */
typedef enum _SPI_MASTER_ERR_CODE
{
    ERR_SPIM_TIME_OUT = -255,			/**<function execute time out*/
    ERR_SPIM_DATALEN_OUT_OF_RANGE,		/**<data len is out of range < 0*/
    SPIM_NONE_ERR = 0,
} SPI_MASTER_ERR_CODE;


/**
 * @brief
 *		初始化SPIM模块
 * @param	SPI_PORT
 * 				选择SPIM0还是SPIM1
 * @param	Mode
 *				0 - CPOL = 0 & CPHA = 0, idle为低电平,第一个边沿采样(上升沿)
 *				1 - CPOL = 0 & CPHA = 1, idle为低电平,第二个边沿采样(下降沿)
 *				2 - CPOL = 1 & CPHA = 0, idle为高电平,第一个边沿采样(下降沿)
 *				3 - CPOL = 1 & CPHA = 1, idle为高电平,第一个边沿采样(上升沿)
 * @param	ClkDiv 	分频系数:0 ~ 15
* 				spi clock = clock / (2 * (ClkDiv + 1)).
 * @return
 *		成功-true 失败-false
 * @note
 */
bool SPIM_Init(SPIM_PORT_T SPIModule,uint8_t Mode ,uint8_t ClkDiv);


/**
 * @brief 反初始化SPI Master模块

 * @param	SPI_PORT
 * 				选择SPIM0还是SPIM1
 * @return
 *		成功-true 失败-false
 * @note
 */
bool SPIM_Deinit(SPIM_PORT_T SPIModule);


//--------------------------------MCU MODE-------------------------------------
/**
 * @brief
 *		SPIM MCU模式发送数据
 * @param	SPI_PORT
 * 				选择SPIM0还是SPIM1
 * @param	SendBuf		发送数据首地址
 * @param	Length		发送数据长度（单位：Byte）
 * @param Spi_Mode
 *      SPI模式选择：
 *          0 - 标准模式
 *          1 - 4bit模式
 * @return
 *		无
 * @note
 */
void SPIM_Send(SPIM_PORT_T SPIModule, uint8_t* SendBuf, uint32_t Length, uint8_t Spi_Mode);


/**
 * @brief
 *		SPIM MCU模式接收数据
 * @param	SPI_PORT
 * 				选择SPIM0还是SPIM1
 * @param	RecvBuf		接收数据首地址
 * @param	Length		接收数据长度（单位：Byte）
 * @param Spi_Mode
 *      SPI模式选择：
 *          0 - 标准模式
 *          1 - 4bit模式
 * @return
 *		无
 * @note
 */
void SPIM_Recv(SPIM_PORT_T SPIModule, uint8_t* RecvBuf, uint32_t Length, uint8_t Spi_Mode);


/**
 * @brief
 *      通过 SPIM 模块发送和接收数据。
 * @param	SPI_PORT
 * 				选择SPIM0还是SPIM1
 * @param SendBuf
 *      发送数据的首地址。指向要发送的数据缓冲区。
 * @param RecvBuf       接收数据的首地址。指向用于存储接收数据的缓冲区。
 * @param SendLength    发送数据的长度（单位：Byte）。表示要发送的数据字节数。
 * @param RecvLength    接收数据的长度（单位：Byte）。表示预期接收的数据字节数。
 * @return
 *      无
 * @note
 */
void SPIM_Send_Recv(SPIM_PORT_T SPIModule, uint8_t *SendBuf, uint8_t *RecvBuf, uint32_t SendLength, uint32_t RecvLength);


//--------------------------------DMA MODE-------------------------------------
/**
 * @brief
 *		通过 SPIM 模块 启动DMA模式传输数据
 * @param	SPI_PORT
 * 				选择SPIM0还是SPIM1
 * @param	Length	数据长度（单位：Byte），最大65536 byte
 * @param	TXEN    DMA发送数据开启
 * @param	RXEN    DMA接收数据开启
 * @param TransferMode
 *      SPI传输模式选择：
 *          0 - 标准模式
 *          1 - 4bit模式
 * @return
 *		无
 * @Note SPIM中断标志置位标志收发数据完成，可使用查询或中断方式
 */
void SPIM_DMAStart(SPIM_PORT_T SPIModule, uint32_t Length, bool TXEN, bool RXEN, uint8_t TransferMode);


/**
 * @brief
 *		中断使能设置
 * @param	SPI_PORT
 * 				选择SPIM0还是SPIM1
 * @param	EN           中断使能控制
 *                      true - 使能中断
 *                      false - 禁止中断
 * @return
 *		无
 */
void SPIM_SetIntEn(SPIM_PORT_T SPI_PORT, bool EN);


/**
 * @brief
 *		获取中断标志状态
 * @param	SPI_PORT
 * 				选择SPIM0还是SPIM1
 * @return
 *		true - 中断发生
 *		false - 无中断
 */
bool SPIM_GetIntFlag(SPIM_PORT_T SPI_PORT);


/**
 * @brief
 *		清除中断标志
 * @param	SPI_PORT
 * 				选择SPIM0还是SPIM1
 * @return
 *		无
 */
void SPIM_ClrIntFlag(SPIM_PORT_T SPI_PORT);


/**
 * @brief
 *		获取SPI忙状态
 * @param	SPI_PORT
 * 				选择SPIM0还是SPIM1
 * @return
 *		true - SPI正在传输数据
 *		false - SPI空闲
 */
bool SPIM_GetBusy(SPIM_PORT_T SPI_PORT);


/**
 * @brief
 *		设置MSB/LSB传输顺序
 * @param	SPI_PORT
 * 				选择SPIM0还是SPIM1
 * @param	EN           MSB使能控制
 *                      true - MSB优先
 *                      false - LSB优先
 * @return
 *		无
 */
void SPIM_SetMSB(SPIM_PORT_T SPI_PORT, bool EN);


/**
 * @brief
 *		获取当前传输顺序
 * @param	SPI_PORT
 * 				选择SPIM0还是SPIM1
 * @return
 *		true - MSB优先
 *		false - LSB优先
 */
bool SPIM_GetMSB(SPIM_PORT_T SPI_PORT);


/**
 * @brief
 *		清除所有状态标志
 * @param	SPI_PORT
 * 				选择SPIM0还是SPIM1
 * @return
 *		无
 */
void SPIM_ClrAllStatus(SPIM_PORT_T SPI_PORT);


/**
 * @brief
 *		DMA使能设置
 * @param	SPI_PORT
 * 				选择SPIM0还是SPIM1
 * @param	EN           DMA使能控制
 *                      true - 使能DMA
 *                      false - 禁止DMA
 * @return
 *		无
 */
void SPIM_SetDmaEn(SPIM_PORT_T SPI_PORT, bool EN);


/**
 * @brief
 *		获取发送溢出标志
 * @param	SPI_PORT
 * 				选择SPIM0还是SPIM1
 * @return
 *		true - 发送溢出发生
 *		false - 无发送溢出
 */
bool SPIM_GetTxOverwr(SPIM_PORT_T SPI_PORT);


/**
 * @brief
 *		获取接收溢出标志
 * @param	SPI_PORT
 * 				选择SPIM0还是SPIM1
 * @return
 *		true - 接收溢出发生
 *		false - 无接收溢出
 */
bool SPIM_GetRxOverwr(SPIM_PORT_T SPI_PORT);


/**
 * @brief
 *		清除溢出标志
 * @param	SPI_PORT
 * 				选择SPIM0还是SPIM1
 * @return
 *		无
 */
void SPIM_ClrOverwr(SPIM_PORT_T SPI_PORT);


/**
 * @brief
 *		高速模式使能设置
 * @param	SPI_PORT
 * 				选择SPIM0还是SPIM1
 * @param	EN           高速模式使能控制
 *                      true - 使能高速模式
 *                      false - 禁止高速模式
 * @return
 *		true - 设置成功
 *		false - 设置失败
 */
bool SPIM_HS_EN(SPIM_PORT_T SPI_PORT, bool EN);


/**
 * @brief
 *		获取TX FIFO空状态
 * @param	SPI_PORT
 * 				选择SPIM0还是SPIM1
 * @return
 *		true - TX FIFO为空（没有数据等待发送）
 *		false - TX FIFO非空（还有数据等待发送）
 */
bool SPIM_TX_FIFO_EMPTY(SPIM_PORT_T SPIModule);


#endif

/**
 * @}
 * @}
 */
