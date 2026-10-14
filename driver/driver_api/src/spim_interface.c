////////////////////////////////////////////////////////////////////////////////
//                   Mountain View Silicon Tech. Inc.
///////////////////////////////////////////////////////////////////////////////
#include <string.h>
#include "type.h"
#include "dma.h"
#include "spim.h"
#include "gpio.h"
#include "spim_interface.h"
#include "core_d1088.h"

#ifdef FUNC_OS_EN
#include "rtos_api.h"
#endif

static uint8_t spim_dma_id[4] =
{
	PERIPHERAL_0_ID_SPIM0_TX,
	PERIPHERAL_0_ID_SPIM0_RX,
	PERIPHERAL_1_ID_SPIM1_TX,
	PERIPHERAL_1_ID_SPIM1_RX
};

//DMA方式接口函数
/**
 * @brief
 *		DMA完成标志
 * @param	PeripheralID		接收数据DMA通道
 * @return
 *		DMA完成标志
 * @note
 */
bool SPIM_DMA_Done(uint8_t PeripheralID)
{
	return DMA_InterruptFlagGet(PeripheralID, DMA_DONE_INT);
}


/**
 * @brief
 *		清除DMA完成标志
 * @param	PeripheralID		接收数据DMA通道
 * @return
 *		无
 * @note
 */
void SPIM_DMA_Done_Clear(uint8_t PeripheralID)
{
	DMA_InterruptFlagClear(PeripheralID,DMA_DONE_INT);
}


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
void SPIM_DMA_Send_Start(SPIM_PORT_T SPIModule, uint8_t PeripheralID, uint8_t* SendBuf, uint32_t Length, uint8_t TransferMode)
{
	DMA_ChannelDisable(PeripheralID);
	DMA_InterruptFlagClear(PeripheralID, DMA_DONE_INT);
	DMA_BlockConfig(PeripheralID);//PERIPHERAL_1_ID_SPIM1_TX or PERIPHERAL_0_ID_SPIM0_TX
	DMA_BlockBufSet(PeripheralID, SendBuf, Length);
	DMA_ChannelEnable(PeripheralID);

	SPIM_DMAStart(SPIModule, Length, 1, 0, TransferMode);
}


/**
 * @brief
 *		启动DMA模式收数据
 * @param	SPIModule    SPIM模块选择，指定要使用的SPIM模块
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
void SPIM_DMA_Recevie_Start(SPIM_PORT_T SPIModule, uint8_t PeripheralID, uint8_t*RevBuf, uint32_t Length, uint8_t TransferMode)
{
	DMA_ChannelDisable(PeripheralID);
	DMA_InterruptFlagClear(PeripheralID, DMA_DONE_INT);
	DMA_BlockConfig(PeripheralID);//PERIPHERAL_1_ID_SPIM1_TX or PERIPHERAL_0_ID_SPIM0_TX
	DMA_BlockBufSet(PeripheralID, RevBuf, Length);
	DMA_ChannelEnable(PeripheralID);

	SPIM_DMAStart(SPIModule, Length, 0, 1, TransferMode);
}


//全双工配置
/**
 * @brief
 *		启动DMA模式收发数据
 * @param	SPIModule    SPIM模块选择，指定要使用的SPIM模块
 * @param	RevPeripheralID DMA接收通道ID，指定DMA通道
 * @param	TxPeripheralID DMA接收通道ID，指定DMA通道
 * @param	RevBuf       接收数据的首地址，指向要发送的数据缓冲区
 * @param	SendBuf      发送数据的首地址，指向要发送的数据缓冲区
 * @param   Length       发送数据的长度，单位为字节
 * @param TransferMode
 *      SPI传输模式选择：
 *          0 - 标准模式
 *          1 - 4bit模式
 * @return
 *		无返回值
 * @note
 */
void SPIM_DMA_Send_Receive_Start(SPIM_PORT_T SPIModule, uint8_t RevPeripheralID, uint8_t TxPeripheralID, uint8_t* SendBuf, uint8_t*RecvBuf, uint32_t Length, uint8_t TransferMode)
{
	DMA_ChannelDisable(RevPeripheralID);
	DMA_InterruptFlagClear(RevPeripheralID, DMA_DONE_INT);
	DMA_BlockConfig(RevPeripheralID);//PERIPHERAL_1_ID_SPIM1_RX or PERIPHERAL_0_ID_SPIM0_RX
	DMA_BlockBufSet(RevPeripheralID, RecvBuf, Length);
	DMA_ChannelEnable(RevPeripheralID);
	DMA_ChannelDisable(TxPeripheralID);
	DMA_InterruptFlagClear(TxPeripheralID, DMA_DONE_INT);
	DMA_BlockConfig(TxPeripheralID);//PERIPHERAL_1_ID_SPIM1_TX or PERIPHERAL_0_ID_SPIM0_TX
	DMA_BlockBufSet(TxPeripheralID, SendBuf, Length);
	DMA_ChannelEnable(TxPeripheralID);

	SPIM_DMAStart(SPIModule, Length, 1, 1, TransferMode);
}


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
void SpimDataRecvByDMA(SPIM_PORT_T SPIModule, uint8_t* RecvBuf, uint32_t Length, uint8_t TransferMode)
{
	SPIM_DMA_Recevie_Start(SPIModule ,spim_dma_id[SPIModule*2+1], RecvBuf, Length, TransferMode);
	while(!DMA_InterruptFlagGet(spim_dma_id[SPIModule*2+1], DMA_DONE_INT));//DMA完成标志
#ifdef FUNC_OS_EN
	{
		osTaskDelay(1);
	}
#endif
	DataCacheRangeInvalid((uint32_t)RecvBuf, Length);
}


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
void SpimDataSendByDMA(SPIM_PORT_T SPIModule, uint8_t* SendBuf, uint32_t Length, uint8_t TransferMode)
{
	SPIM_DMA_Send_Start(SPIModule, spim_dma_id[SPIModule*2], SendBuf, Length, TransferMode);
	while(!DMA_InterruptFlagGet(spim_dma_id[SPIModule*2], DMA_DONE_INT));//DMA完成标志
#ifdef FUNC_OS_EN
	{
		osTaskDelay(1);
	}
#endif
	while(!SPIM_TX_FIFO_EMPTY(SPIModule));
}


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
void SpimDataSendRecvByDMA(SPIM_PORT_T SPIModule, uint8_t* SendBuf, uint8_t* RecvBuf, uint32_t Length, uint8_t TransferMode)
{
	SPIM_DMA_Send_Receive_Start(SPIModule, spim_dma_id[SPIModule*2+1], spim_dma_id[SPIModule*2], SendBuf, RecvBuf, Length, TransferMode);
	while(!DMA_InterruptFlagGet(spim_dma_id[SPIModule*2+1], DMA_DONE_INT));//DMA完成标志
#ifdef FUNC_OS_EN
	{
		osTaskDelay(1);
	}
#endif
	DataCacheRangeInvalid((uint32_t)RecvBuf, Length);
}


/**
 * @brief
 *		配置SPIM接口的GPIO引脚复用功能
 * @param	PortSel 配置SPIM使用GPIO引脚
 * 					0: 配置SPIM0使用GPIOA4-A7引脚
 * 					1: 配置SPIM1使用GPIOA10-A13引脚
 * 					2: 配置SPIM1使用GPIOB0-B3引脚
 * 					3: 配置SPIM0使用GPIOA2-A3,A5-A7引脚
 * 					4: 配置SPIM1使用GPIOA10-A15引脚
 * 					5: 配置SPIM1使用GPIOB0-B5引脚
 * @return
 *		SPIM_NONE_ERR: 配置成功
  		ERR_SPIM_DATALEN_OUT_OF_RANGE: 参数GPIO_Mode超出有效范围
 * @note
 */
bool GPIO_SpimIoConfig(uint8_t GPIO_Mode)
{
    if(GPIO_Mode == 0)
    {
        GPIO_PortAModeSet(GPIOA5, 3);  // MISO: spim0_io1_0
        GPIO_PortAModeSet(GPIOA6, 9);  // CLK: spim0_clk_0
        GPIO_PortAModeSet(GPIOA7, 3);  // MOSI: spim0_io0_0
    }
    else if(GPIO_Mode == 1)
    {
        GPIO_PortAModeSet(GPIOA10, 2);  // MOSI: spim1_io0_0
        GPIO_PortAModeSet(GPIOA11, 13); // CLK: spim1_clk_0
        GPIO_PortAModeSet(GPIOA12, 2);  // MISO: spim1_io1_0
    }
    else if(GPIO_Mode == 2)
    {
        GPIO_PortBModeSet(GPIOB0, 2);  // MOSI: spim1_io0_1
        GPIO_PortBModeSet(GPIOB1, 13); // CLK: spim1_clk_1
        GPIO_PortBModeSet(GPIOB2, 2);  // MISO: spim1_io1_1
    }
    else if(GPIO_Mode == 3)
    {
        GPIO_PortAModeSet(GPIOA2, 3);  // IO3: spim0_io3_0
        GPIO_PortAModeSet(GPIOA3, 2);  // IO2: spim0_io2_0
        GPIO_PortAModeSet(GPIOA5, 3);  // IO1: spim0_io1_0
        GPIO_PortAModeSet(GPIOA6, 9);  // CLK: spim0_clk_0
        GPIO_PortAModeSet(GPIOA7, 3);  // IO0: spim0_io0_0
    }
    else if(GPIO_Mode == 4) 
    {
        GPIO_PortAModeSet(GPIOA10, 2); // IO0: spim1_io0_0
        GPIO_PortAModeSet(GPIOA11, 13); // CLK: spim1_clk_0
        GPIO_PortAModeSet(GPIOA12, 2); // IO1: spim1_io1_0
        GPIO_PortAModeSet(GPIOA14, 2); // IO2: spim1_io2_0
        GPIO_PortAModeSet(GPIOA15, 2); // IO3: spim1_io3_0
    }
    else if(GPIO_Mode == 5)
    {
        GPIO_PortBModeSet(GPIOB0, 2);  // IO0: spim1_io0_1
        GPIO_PortBModeSet(GPIOB1, 13); // CLK: spim1_clk_1
        GPIO_PortBModeSet(GPIOB2, 2);  // IO1: spim1_io1_1
        GPIO_PortBModeSet(GPIOB4, 2);  // IO2: spim1_io2_1
        GPIO_PortBModeSet(GPIOB5, 2);  // IO3: spim1_io3_1
    }
    else
    {
        return FALSE;
    }
    
    return TRUE;
}
