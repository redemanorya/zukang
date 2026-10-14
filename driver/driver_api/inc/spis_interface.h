/*
 * spis_interface.h
 *
 *  Created on: Sep 2, 2025
 *      Author: mv_cs_01
 */

#ifndef DRIVER_DRIVER_API_INC_SPIS_INTERFACE_H_
#define DRIVER_DRIVER_API_INC_SPIS_INTERFACE_H_


/**
 * @brief
 *      SPIS功能IO初始化
 * @param PortSel
 *      0 - SPIS_PORT0_A4A5A6A7
 *      1 - SPIS_PORT1_A10A11A12A13
 *      其他 - 默认为 SPIS_PORT2_B0B1B2B3
 * @return
 *       TRUE   成功，  FALSE 失败
 * @note
 *      根据 PortSel 的值，选择对应的 SPIS 功能引脚配置。
 *      请确保在调用此函数之前，GPIO 模块已经正确初始化。
 */
bool GPIO_SpisIoConfig(uint8_t PortSel);


/**
 * @brief
 *    初始化SPIS DMA功能
 *     * @param SPIModule
 *      SPIS模块选择：
 *          SPIS0 - 选择SPIS0模块
 *          SPIS1 - 选择SPIS1模块
 * @param	RxBufferAddr  接收缓冲区首地址
 * @param	RxBufferLen   接收缓冲区长度
 * @param	TxBufferAddr  发送缓冲区首地址
 * @param   TxBufferLen   发送缓冲区长度
 * @return
 *   TRUE   成功，  FALSE 失败
 * @note
 */
bool SPIS_DMA_Init(SPIS_PORT_T SPIModule,void* RxBufferAddr, uint16_t RxBufferLen, void* TxBufferAddr, uint16_t TxBufferLen);


void SPIS_DMA_StartSendRecv(SPIS_PORT_T SPIModule, uint8_t* SendBuf, uint8_t* RecvBuf, uint32_t BufLen);

#endif /* DRIVER_DRIVER_API_INC_SPIS_INTERFACE_H_ */
