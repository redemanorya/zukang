#include <string.h>
#include "type.h"
#include "dma.h"
#include "spis.h"
#include "gpio.h"
#include "timeout.h"

#ifdef FUNC_OS_EN
#include "rtos_api.h"
#endif

static uint8_t spis_dma_id[4] = {
	PERIPHERAL_0_ID_SPIS0_TX,
	PERIPHERAL_0_ID_SPIS0_RX,
	PERIPHERAL_1_ID_SPIS1_TX,
	PERIPHERAL_1_ID_SPIS1_RX
};

/**
 * @brief
 *		 配置SPIS接口的GPIO引脚复用功能
 * @param	PortSel 配置SPIS使用GPIO引脚
 * 					0: 配置SPIS0使用GPIOA4-A7引脚
 * 					1: 配置SPIS0使用GPIOA10-A13引脚
 * 					2: 配置SPIS1使用GPIOB0-B3引脚
 * @return
 *		无返回值
 * @note
 */
bool GPIO_SpisIoConfig(uint8_t PortSel)
{
    if(PortSel == 0)
    {
        GPIO_PortAModeSet(GPIOA4, 3);  // spis0_cs_0
        GPIO_PortAModeSet(GPIOA5, 6);  // spis0_miso_0
        GPIO_PortAModeSet(GPIOA6, 1);  // spis0_clk_0(i)
        GPIO_PortAModeSet(GPIOA7, 4);  // spis0_mosi_0(i)
    }
    else if(PortSel == 1)
    {
        GPIO_PortAModeSet(GPIOA13, 2);  // spis0_cs_1(i)
        GPIO_PortAModeSet(GPIOA12, 10); // spis0_miso_1(o)
        GPIO_PortAModeSet(GPIOA11, 1);  // spis0_clk_1(i)
        GPIO_PortAModeSet(GPIOA10, 3);  // spis0_mosi_1(i)
    }
    else if(PortSel == 2)
    {
        GPIO_PortBModeSet(GPIOB3, 2);   // spis1_cs_0(i)
        GPIO_PortBModeSet(GPIOB2, 11);  // spis1_miso_0(o)
        GPIO_PortBModeSet(GPIOB1, 1);   // spis1_clk_0(i)
        GPIO_PortBModeSet(GPIOB0, 3);   // spis1_mosi_0(i)
    }
    else
    {
        return FALSE;
    }
    
    return TRUE;
}


bool SPIS_DMA_Init(SPIS_PORT_T SPIModule, void* RxBufferAddr, uint16_t RxBufferLen, void* TxBufferAddr, uint16_t TxBufferLen)
{
    uint8_t RxChannelID, TxChannelID;
    if (SPIModule == SPIS0)
    {
        RxChannelID = PERIPHERAL_0_ID_SPIS0_RX;
        TxChannelID = PERIPHERAL_0_ID_SPIS0_TX;
    }
    else if (SPIModule == SPIS1)
    {
        RxChannelID = PERIPHERAL_1_ID_SPIS1_RX;
        TxChannelID = PERIPHERAL_1_ID_SPIS1_TX;
    }
    else
    {
        return FALSE;
    }

    if (DMA_CircularConfig(RxChannelID, RxBufferLen / 2, RxBufferAddr, RxBufferLen) != DMA_OK)
    {
        return FALSE;
    }

    if (DMA_CircularConfig(TxChannelID, TxBufferLen / 2, TxBufferAddr, TxBufferLen) != DMA_OK)
    {
        return FALSE;
    }

    DMA_ChannelEnable(RxChannelID);
    DMA_ChannelEnable(TxChannelID);

    return TRUE;
}


uint32_t SPIS_DMA_Send(SPIS_PORT_T SPIModule, uint8_t* SendBuf, uint32_t BufLen, uint32_t TimeOut)
{
    TIMER TimerTx;
    uint8_t TxChannelID;
    if (SPIModule == SPIS0)
    {
        TxChannelID = PERIPHERAL_0_ID_SPIS0_TX;
    }
    else if (SPIModule == SPIS1)
    {
        TxChannelID = PERIPHERAL_1_ID_SPIS1_TX;
    }
    else
    {
        return 0;
    }
    TimeOutSet(&TimerTx, TimeOut);
    SPIS_TxDmaModeSet(SPIModule, 1);
    DMA_CircularFIFOClear(TxChannelID);
    DMA_CircularDataPut(TxChannelID, SendBuf, BufLen);
	while(SPIS_GetCSStatus(SPIModule));
    while (DMA_CircularDataLenGet(TxChannelID) &&! IsTimeOut(&TimerTx));
	#ifdef FUNC_OS_EN
	{
		osTaskDelay(1);
	}
	#endif
    while (!SPIS_GetTXFIFOEmptyStatus(SPIModule) &&! IsTimeOut(&TimerTx));
    return BufLen - DMA_CircularDataLenGet(TxChannelID);
}


uint32_t SPIS_DMA_Recv(SPIS_PORT_T SPIModule, uint8_t* RecvBuf, uint32_t BufLen, uint32_t TimeOut)
{
	TIMER TimerRx;
    uint8_t RxChannelID;

    if (SPIModule == SPIS0)
    {
        RxChannelID = PERIPHERAL_0_ID_SPIS0_RX;
    }
    else if (SPIModule == SPIS1)
    {
        RxChannelID = PERIPHERAL_1_ID_SPIS1_RX;
    }
    else
    {
        return 0;
    }
	TimeOutSet(&TimerRx, TimeOut);
	SPIS_RxDmaModeSet(SPIModule, 1);
    while ((DMA_CircularDataLenGet(RxChannelID) < BufLen) && !IsTimeOut(&TimerRx));
	#ifdef FUNC_OS_EN
	{
		osTaskDelay(1);
	}
	#endif
    return DMA_CircularDataGet(RxChannelID, RecvBuf, BufLen);
}


void SPIS_DMA_StartSendRecv(SPIS_PORT_T SPIModule, uint8_t* SendBuf, uint8_t* RecvBuf, uint32_t BufLen)
{
	SPIS_ClrRxFIFO(SPIModule);
	SPIS_ClrTxFIFO(SPIModule);
	DMA_BlockBufSet(spis_dma_id[SPIModule*2], SendBuf, BufLen);
	DMA_BlockBufSet(spis_dma_id[SPIModule*2+1], RecvBuf, BufLen);
	DMA_ChannelEnable(spis_dma_id[SPIModule*2+1]);
	DMA_ChannelEnable(spis_dma_id[SPIModule*2]);
	SPIS_TxDmaModeSet(SPIModule, 1);
	SPIS_RxDmaModeSet(SPIModule, 1);
}
