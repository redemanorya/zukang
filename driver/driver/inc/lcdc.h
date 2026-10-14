/*
 * lcdc.h
 *
 *  Created on: Aug 26, 2025
 *      Author: CSSWPC313
 */

/**
 * @addtogroup LCDC
 * @{
 * @defgroup lcdc lcdc.h
 * @{
 */
 
#ifndef DRIVER_DRIVER_INC_LCDC_H_
#define DRIVER_DRIVER_INC_LCDC_H_

typedef enum __LCDC_MODE
{
    LCDC_8080_MODE = 0,//8080 mode
	LCDC_6800_MODE = 1,//6800 mode
	LCDC_RGB_MODE = 2, //RGB mode
} LCDC_MODE;

typedef enum __LCDC_BYPASS_CTRL_SEL
{
    LCDC_CTRL_RD = 0, 	//8080->RD 		6800->E 		RGB->DE
	LCDC_CTRL_WR,     	//8080->WR 		6800->R/W 		RGB->HSYNC
	LCDC_CTRL_RS,		//8080->DC/RS 	6800->DC/RS		RGB->VSYNC
	LCDC_CTRL_CS,		//8080->CS 		6800->CS 		RGB->DOTCLK
//	LCDC_CTRL_EN
} LCDC_BYPASS_CTRL_SEL;

typedef enum __LCDC_CTRL_BUS_OEN
{
    LCDC_CTRL_BUS_ACTIVE = 0,		//控制线使能
	LCDC_CTRL_BUS_INACTIVE = 0xf,	//控制线失能
} LCDC_CTRL_BUS_OEN;

typedef enum __LCDC_DATA_BUS_OEN
{
    LCDC_DATA_BUS_ACTIVE = 0,		//data线使能
	LCDC_DATA_BUS_INACTIVE = 1,		//data线失能
} LCDC_DATA_BUS_OEN;

typedef struct __LCDC_StructInit
{
	uint8_t Endian; 			 /*when data bus width is 8 bits
								 0: [ 31  :  24 ], [ 23  :  16 ], [ 15  :   8 ], [  7  :  0  ]
									data[7:0]_3rd, data[7:0]_2nd, data[7:0]_1st, data[7:0]_0th
								 1: [ 23  :  16 ], [ 31  :  24 ], [  7  :  0  ], [ 15  :   8 ]
									data[7:0]_3rd, data[7:0]_2nd, data[7:0]_1st, data[7:0]_0th
								 when data bus width is 16 bits
								 0: {[31:24],[23:16]}, {[15:8],[ 7:0]}
									  data[7:0]_1st,    data[7:0]_0th
								 1: {[23:16],[31:24]}, {[ 7:0],[15:8]}*/
	uint8_t DataWidth;			 //0: 8 bits, data[15:8] invalid, data[7:0] valid 1: 16 bits, data[15:0] valid
	uint8_t CtrlBusIdlePol;/**   空闲电平，只有bit[2]的值在有效状态时与配置的值一致，其余在生效时的电平与配置状态相反
	                             6800模式时，bit[1]空闲状态电平为读时电平
								 control bus mapping   8080    6800     RGB
								 control bus bit[3] ->  CS      CS    DOTCLK
								 control bus bit[2] -> DC/RS   DC/RS  VSYNC
								 control bus bit[1] ->  WR      R/W   HSYNC
								 control bus bit[0] ->  RD       E      DE*/
	uint8_t IntfMode;			//0: 8080 1: 6800 2: RGB SYNC-DE mode
	uint8_t ClkDivCof;			//Flcdc = Fsys/(ClkDivCof+1)  range: 1-511
	uint8_t IntEnable;			//中断使能
	uint8_t RwDir;				//读写方向  0: read 1:write
	uint8_t RwHost;				// only in non-bypass mode 0: DMA read/write 1: MCU read/write
	uint8_t BypassEnable;
}LCDC_StructInit;


typedef struct __LCDC_RGB_Param_StructInit
{
	uint8_t VFP; 			  // vertical front porch		default:0x4
	uint8_t VBP;			  // vertical back porch		default:0x2
	uint8_t VSW;              // vertical sync width		default:0x2
	uint8_t HFP;			  //horizontal front porch		default:0xa
	uint8_t HBP;			  // horizontal back porch		default:0x14
	uint8_t HSW;			  // horizontal sync width		default:0xa
}LCDC_RGB_Param_StructInit;

/**
 * @brief      将GPIO复用为LCDC引脚.
 * 			   GPIOA0->lcdc_cs GPIOA1->lcdc_dc/lcdc_vsync GPIOA10->lcdc_wr/lcdc_hsync GPIOA11->lcdc_rd/lcdc_DE
 *
 * @param[in]  DataWidth数据位  0：8位 GPIOA11-GPIOA19->lcdc_data[0:7] 1：16位 GPIOA20-GPIOA27->lcdc_data[8:15]
 *
*/
void LcdcGpioInit(uint8_t DataWidth);

/**
 * @brief   lcdc 参数 配置
 * @param[in]    LcdcStruct lcdc初始化参数
 */
void LcdcConfig(LCDC_StructInit *LcdcStruct);

/**
 * @brief   lcdcRGB 参数 配置
 * @param[in]   lcdcRGB参数,参见LCDC_RGB_Param_StructInit
 */
void LcdcRGBParamCfg(LCDC_RGB_Param_StructInit *param);

/**
 * @brief   lcdcRGB 屏幕大小参数 配置
 * @param[in]    ver:pixels in vertical direction
 * 				 hor:pixels in horizontal direction
 */
void LcdcRGBDiscrSet(uint16_t ver,uint16_t hor);
/**
 * @brief  enable or disable lcdc ctrl bus and data bus
 * @param[in]    CtrlOen：ctrl bus
 * @param[in]    DataOen：data bus
 */
void LcdcSetBusOen(LCDC_CTRL_BUS_OEN CtrlOen,LCDC_DATA_BUS_OEN DataOen);

/**
 * @brief   配置lcdc 读/写方向
 * @param[in]    dir 0：读  1：写
 */
void LcdcSetDir(uint8_t dir);

/**
 * @brief  控制bypass 模式下的ctrl bus拉高拉低，使能或者失能bypass模式
 * @param[in]    CtrlSel:控制类型，参见LCDC_BYPASS_CTRL_SEL
 * @param[in]    statues： 0或1，当CtrlSel为ctrl bus时代表电平，为LCDC_CTRL_EN时代表使能或失能
 */
void LcdcSetBypassCtrl(LCDC_BYPASS_CTRL_SEL CtrlSel,uint8_t statues);

/**
 * @brief  set LCDC write/read transfer length in non-bypass mode
 * @param[in]    len数据长度，单位：word
 */
void SetXferWriteLen(uint32_t len);

/**
 * @brief  bypass 模式下输出数据
 * @param[in]    data:bypass要输出的数据内容
 */
void LcdcBypassTxByte(uint16_t data);

/**
 * @brief  bypass 模式下接收数据
 * @return[out]    bypass要接收到的数据内容
 */
uint16_t LcdcBypassRxByte(void);
/**
 * @brief  开始发送或者读取，只在non-bypass模式生效
 */
void LcdcRwStart(void);

/**
 * @brief  mcu模式发送数据(阻塞式)
 * @param[in] data 要发送的数据buf
 * @param[in] count 要发送的数据长度，单位为word
 */
void LcdcWriteDataByMcu(uint32_t *data,uint32_t count);
/**
 * @brief  mcu模式接收数据(阻塞式)
 * @param[in]recvdata 接收的数据buf
 * @param[in] len 要接收的数据长度，单位为word
 */
uint32_t LcdcReadDataByMcu(uint32_t *recvdata,uint32_t len);

/**
 * @brief  mcu模式发送数据(非阻塞式)
 * @param[in] data 要发送的数据buf
 * @param[in] count 要发送的数据长度，单位为word,最大为64
 */
void LcdcWriteDataNoBlock(uint32_t *data,uint32_t count);
/**
 * @brief  mcu模式接收数据(非阻塞式)
 * @param[in]data 接收的数据buf
 * @param[in] count 要接收的数据长度，单位为word，最大为64
 */
void LcdcReadDataNoBlock(uint32_t *data,uint32_t count);

/**
 * @brief  dma模式发送数据
 * @param[in] data 要发送的数据buf
 * @param[in] count 要发送的数据长度，单位为word,最大为16383
 */
void LcdcDmaSend(uint32_t *data,uint32_t count);

/**
 * @brief  dma模式接收数据
 * @param[in]data 接收的数据buf
 * @param[in] count 要接收的数据长度，单位为word，最大为16383
 */
void LcdcDmaRead(uint32_t *data,uint32_t count);

/**
 * @brief  使能或失能lcdc中断
 * @param[in] enable  0：失能 1：使能
 */
void LcdcIntEnable(bool enable);

/**
 * @brief  清除lcdc中断标志位
 */
void LcdcIntClear(void);

/**
 * @brief  获取cdc中断标志位
 */
uint8_t LcdcIntFlagGet(void);

#endif /* DRIVER_DRIVER_INC_LCDC_H_ */

/**
 * @}
 * @}
 */
