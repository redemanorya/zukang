/*
 * dvpc.h
 *
 *  Created on: Aug 7, 2025
 *      Author: CSSWPC313
 */
/**
 * @addtogroup DVPC
 * @{
 * @defgroup dvpc dvpc.h
 * @{
 */
 
#include "type.h"

#ifndef DRIVER_DRIVER_INC_DVPC_H_
#define DRIVER_DRIVER_INC_DVPC_H_

typedef enum {
	DVPC_DPLL0_CLK_MODE=0,	/*dpll0 source*/
	DVPC_APLL_CLK_MODE,	/*APLL source*/
}DVPC_CLK_MODE;

typedef enum __PCLK_POLARITY
{
	PCLK_POLARITY_RAISING  = 0,  /**< 上升沿捕获 */
	PCLK_POLARITY_FALLING  = 1   /**< 下降沿捕获 */

} PCLK_POLARITY;
typedef enum __HSYNC_ACTIVE
{
	HSYNC_ACTIVE_HIGH  = 0,  /**< 高有效 */
	HSYNC_ACTIVE_LOW  = 1   /**< 低有效 */

} HSYNC_ACTIVE;
typedef enum __VSYNC_ACTIVE
{
	VSYNC_ACTIVE_LOW  = 0, /**< 低有效 */
	VSYNC_ACTIVE_HIGH  = 1 /**< 高有效 */
} VSYNC_ACTIVE;


typedef struct __DVPC_StructInit
{
	uint8_t PclkActiveEge;	// 0: active rising edge 1: active falling edge
	uint8_t HsyncActive;	//0: active high 1: active low
	uint8_t VsyncActive; 	//0: active low 1: active high
	uint8_t StoForm;		// 0: all pixel data are stored  1: only Y data in YUV mode are stored
	uint8_t StoOrder;		/*when sto_form = 0   0: 1-word = {MSB ... LSB} = {pixel_1 data, pixel_0 data} 1: 1-word = {MSB ... LSB} = {pixel_0 data, pixel_1 data}
	 	 	 	 	 	 	  when sto_form = 1   0: 1-word = {MSB ... LSB} = {pixel_3 data, pixel_2 data, pixel_1 data, pixel_0 data}1: 1-word = {MSB ... LSB} = {pixel_0 data, pixel_1 data, pixel_2 data, pixel_3 data}*/
	uint8_t StoIntelval;	// 1 in N=sto_interval+1 frames are stored to memoery
	uint8_t DmaEnable;
	uint8_t CropEnable;
	uint8_t  FramNum;		//0:countinue mode
	uint16_t LineNum;		//line number of each frame, for data check
	uint16_t PixNum;		//pixel number of each line, for data check
	uint16_t VertStart;		// vertical start line count
	uint16_t HorrStart;		// horrizontal offset count
	uint16_t VertCount;		//how many lines in a frame to capture
	uint16_t HorrCount;		//how many bytes in a line to capture
}Dvpc_StructInit;

typedef enum __DVPC_INTERRUPT_SRC
{
    FRAME_INTERRUPT_SRC = 1,   //one frame interrupt
	TRAN_INTERRUPT_SRC = 2,    //whole transfer interrupt
	ERROR_INTERRUPT_SRC = 4,   //Error Int includes  pixel data amount check,Line Error ,FIFO Overwrite Error
} DVPC_INTERRUPT_SRC;

typedef enum _DVPC_IOCTRL_CMD
{
	DVPC_TRANS_STATUS_GET = 0,    /**< DVPC TRANS INT FLAG获取 */
	DVPC_FRAM_STATUS_GET,         /**< DVPC FRAM INT FLAG获取 */
	DVPC_BUSY_STATUS_GET,         /**< DVPC BUSY FLAG获取 */
	DVPC_FRAM_COUNT_GET,          /**< DVPC 接收了多少帧数据获取 */

	DVPC_RFIFO_WFULL_GET,         /**< DVPC READ FIFO满状态获取 */
	DVPC_RFIFO_REMPTY_GET,        /**< DVPC READ FIFO 空 状态获取 */

	DVPC_OVWR_STATUS_GET,         /**< DVPC READ FIFO over write 状态获取  */
	DVPC_FIFO_LENGTH_GET,         /**< DVPC READ FIFO剩余byte数获取  */
	DVPC_ERROR_GET,               /**< DVPC ERR获取  [0]:DATA_PIX_ERR [1]:DATA_LINE_ERR [7:2]:ERR_FRA_NUM哪一帧发生错误 [20:8]:ERR_LINE_NUM哪一行发生错误*/

	DVPC_RX_STATUS_CLR,          /**< DVPC RX FIFO CLEAR  */
} DVPC_IOCTRL_CMD;

/**
 * @brief   dvpc mclk 配置
 * @param[in]    clkmode 时钟源选择
 * @param[in]    divnum dvpc 时钟分频系数 ：[0-0x1f] the divided frequency is pll_frequency/(N+1)
 */
void Dvpc_ClkConfig(DVPC_CLK_MODE clkmode,uint8_t divnum);

/**
 * @brief      将GPIO复用为DVPC引脚.
 * @param[in]    hsync_pin 0:A0,1:A21
 * @param[in]    vsync_pin 0:A1,1:A20
 * A10->PCLK A11->MCLK A12-A19->DATA[0:7]
 *
*/
void DvpcGpioConfig(uint8_t hsync_pin,uint8_t vsync_pin);

/**
 * @brief   dvpc 参数 配置
 * @param[in]    DvpcStruct dvpc初始化参数
 */
void Dvpc_Config(Dvpc_StructInit *DvpcStruct);

/**
 * @brief      将dvpc开始接收.
*/
void DvpcStart(void);

/**
 * @brief      将dvpc停止接收.
*/
void DvpcEnd(void);

/**
 * @brief      使能中断源
 * @param[in]  IntSrc:         中断源选择，可以选择多个
 */
void DvpcInterruptSrcEnable(DVPC_INTERRUPT_SRC IntSrc);

/**
 * @brief      关闭中断源
 * @param[in]  IntSrc:         中断源选择，可以选择多个
 */
void DvpcInterruptSrcDisable(DVPC_INTERRUPT_SRC IntSrc);

/**
 * @brief      清除中断标志状态
 * @param[in]  IntSrc:         中断源选择
 */
void DvpcInterruptFlagClear(DVPC_INTERRUPT_SRC IntSrc);
/**
 * @brief      根据对应命令进行相关操作
 * @param[in]  cmd:命令，对照DVPC_IOCTRL_CMD
 * @param[in]  返回命令对应值
 */
uint16_t Dvpc_IOCTRL(DVPC_IOCTRL_CMD cmd);

#endif /* DRIVER_DRIVER_INC_DVPC_H_ */

/**
 * @}
 * @}
 */