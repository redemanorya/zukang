/**
 **************************************************************************************
 * @file    i2s_interface.h
 * @brief   audio i2s interface
 *
 * @author  
 * @version V1.1.0
 *
 * $Created: 2019-01-04 14:01:05$
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#ifndef		__I2S_INTERFACE_H__
#define		__I2S_INTERFACE_H__

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus 
#include "dma.h"
#include "i2s.h"
#include "gpio.h"
#include "mcu_circular_buf.h"
#include "clk.h"

typedef enum _LINE_MODE
{
	Single_Mode = 0,
	Dual_Mode,
	Quad_Mode,
}LINE_MODE;

typedef struct _I2SParamCt
{
	uint8_t          IsMasterMode;    //0: master, 1: slave

	uint32_t         SampleRate;      //sample rate

	I2S_DATA_FORMAT  I2sFormat;       //I2S_DATA_FORMAT

	I2S_20BIT_24BIT_ALIGN_MODE   ALIGN_MODE;

	I2S_DATA_LENGTH  I2sBits;         //I2S_DATA_LENGTH
	
	uint8_t          I2sTxRxEnable;   // 0: disable, 1: only tx enable, 2: only rx enable, 3: tx & rx enable
	
	//DMA
	uint8_t          TxPeripheralID;  //i2s tx peripheralID

	void            *TxBuf;           //i2s tx buf

	uint16_t         TxLen;           //i2s tx len
	
	uint8_t          RxPeripheralID;  //i2s rx peripheralID

	void            *RxBuf;           //i2s rx buf

	uint16_t         RxLen;           //i2s rx len

	LINE_MODE		 LineMode;

	TDM_CH_NUM       TdmChNum;

} I2SParamCt;

typedef enum _AUDIO_I2S_DPLL_FOLLOW_MASK
{
	AUDIO_I2S_DPLL_FOLLOW_NONE   = 0,
	AUDIO_I2S_DPLL_FOLLOW_DAC0   = (1UL << 0),
	AUDIO_I2S_DPLL_FOLLOW_DAC1   = (1UL << 1),
	AUDIO_I2S_DPLL_FOLLOW_DAC2   = (1UL << 2),
	AUDIO_I2S_DPLL_FOLLOW_ADC0   = (1UL << 3),
	AUDIO_I2S_DPLL_FOLLOW_ADC1   = (1UL << 4),
	AUDIO_I2S_DPLL_FOLLOW_I2S0   = (1UL << 5),
	AUDIO_I2S_DPLL_FOLLOW_I2S1   = (1UL << 6),
	AUDIO_I2S_DPLL_FOLLOW_I2S2   = (1UL << 7),
	AUDIO_I2S_DPLL_FOLLOW_SPDIF0 = (1UL << 8),
	AUDIO_I2S_DPLL_FOLLOW_SPDIF1 = (1UL << 9),
} AUDIO_I2S_DPLL_FOLLOW_MASK;


void AudioI2S_Init(I2S_MODULE Module, I2SParamCt *ct);
void 	 AudioI2S_DeInit(I2S_MODULE Module);
void AudioI2S_RXDeInit(I2S_MODULE Module);
void AudioI2S_TXDeInit(I2S_MODULE Module);
uint8_t  AudioI2S_MasterModeGet(I2S_MODULE Module);
I2S_DATA_LENGTH AudioI2S_WordLengthGet(I2S_MODULE Module);
uint16_t AudioI2S_DataLenGet(I2S_MODULE Module);
uint16_t AudioI2S_DataGet(I2S_MODULE Module, void* Buf, uint16_t Len);
uint16_t AudioI2S_DataSpaceLenGet(I2S_MODULE Module);
void AudioI2S_DataSet(I2S_MODULE Module, void *Buf, uint32_t Len);


uint16_t AudioI2S0_DataLenGet(void);
uint16_t AudioI2S1_DataLenGet(void);
uint16_t AudioI2S2_DataLenGet(void);
uint16_t AudioI2S0_DataGet(void* Buf, uint16_t Len);
uint16_t AudioI2S1_DataGet(void* Buf, uint16_t Len);
uint16_t AudioI2S2_DataGet(void* Buf, uint16_t Len);
uint16_t AudioI2S0_DataSpaceLenGet(void);
uint16_t AudioI2S1_DataSpaceLenGet(void);
uint16_t AudioI2S2_DataSpaceLenGet(void);
uint16_t AudioI2S0_DataSet(void *Buf, uint16_t Len);
uint16_t AudioI2S1_DataSet(void *Buf, uint16_t Len);
uint16_t AudioI2S2_DataSet(void *Buf, uint16_t Len);

uint16_t AudioI2S0_TX_DataLenGet(void);
uint16_t AudioI2S1_TX_DataLenGet(void);
uint16_t AudioI2S2_TX_DataLenGet(void);

void AudioI2S_SampleRateChange(I2S_MODULE Module,uint32_t SampleRate);

uint32_t AudioI2SDpllLockFreqGet(uint32_t SampleRate);
uint32_t AudioI2SDpllOutputFreqGet(uint32_t SampleRate);
void AudioI2SDpllTargetFreqConfig(uint32_t SampleRate);
void AudioI2SDpllRefInit(I2S_MODULE Module);
void AudioI2SDpllRefDeInit(void);
bool AudioI2SDpllRefStart(I2S_MODULE Module, uint32_t SampleRate);
bool AudioI2SDpllRefLockStatusGet(void);
void AudioI2SDpllRefHold(void);
void AudioI2SDpllRefHoldSet(bool IsEnable);
bool AudioI2SDpllRefHoldGet(void);
bool AudioI2SDpllFollowModuleMatchSampleRate(AUDIO_MODULE Module, uint32_t SampleRate);
void AudioI2SDpllFollowEnable(uint32_t FollowMask);
void AudioI2SDpllFollowDisable(uint32_t FollowMask);
void AudioI2SDpllFollowDisableAll(void);

/**
 * @brief  I2S 多channle数据处理
 * @param  Modul： which I2S MODULE you will use
 * @param  ChNum：I2S channle数
 * @param  LineMask：线使能掩码，bit[a]=1表示第a条线有效，bit[a]=0表示该线被屏蔽（RX:丢弃数据 / TX:填0）
 * I2S_Switch_Buf：DMA处理buf
 * Len 帧长
 * @note I2S0 TDM不支持输出
 */
uint16_t AudioI2SDataRXCarry(I2S_MODULE Module, uint8_t ChNum, uint8_t LineMask, int32_t* I2S_Switch_Buf, uint16_t Len);

/**
 * @brief  I2S 多channle据处理
 * @param  Modul： which I2S MODULE you will use
 * @param  ChNum：I2S channle数
 * @param  LineMask：线使能掩码，bit[a]=1表示第a条线有效，bit[a]=0表示该线被屏蔽（RX:丢弃数据 / TX:填0）
 * I2S_Switch_Buf：DMA处理buf
 * Len 帧长
 * @note I2S0 TDM不支持输出
 */
void AudioI2SDataTXCarry(I2S_MODULE Module, uint8_t ChNum, uint8_t LineMask, int32_t* I2S_Switch_Buf, uint16_t Len);

/**
 * @brief  I2S MCLK out初始化，调用之后对应的IO端口输出MCLK时钟
 * @param  Modul： I2S0/I2S1
 * @param  SampleRate：采样率，48KHz输出12.228M，44.1KHz输出11.2896MHz
 * @note 如果I2S模块有配置，则直接再I2S内部使能配置
 * @note MCLK out IO复用端口见函数实现前的宏定义
 */
void AudioI2S_MclkOutInit(I2S_MODULE Module,uint32_t SampleRate);

//0:master mode ;1:slave mode 外设未接不要配slave
#define	I2S_MASTER_MODE			0
#define	I2S_SLAVE_MODE			1
// IO组定义
#define IO_GROUP_A             0
#define IO_GROUP_B             1
//i2s_port: 0 ---> i2s0  1 ---> i2s1  2 ---> i2s2
//gpio_mode: GPIO复用功能配置
//gpio_index：GPIO引脚号
#define SET_I2S_GPIO_ALL_VAL(i2s_mode, io_group, i2s_port, gpio_mode, gpio_index)		((i2s_mode << 28) | (io_group << 24) | (i2s_port << 16) | (gpio_mode << 8) | (gpio_index))
#define SET_I2S_GPIO_VAL(io_group, i2s_port, gpio_mode, gpio_index)						((io_group << 24) | (i2s_port << 16) | (gpio_mode << 8) | (gpio_index))

#define GET_I2S_MODE(val)													((val >> 28) & 0x01)
#define GET_I2S_IO_GROUP(val)												((val >> 24) & 0x0f)
#define GET_I2S_I2S_PORT(val)												((val >> 16) & 0x03)
#define GET_I2S_GPIO_MODE(val)												((val >> 8) & 0x1f)
#define GET_I2S_GPIO_INDEX(val)												(val & 0x1f)
#define GET_I2S_GPIO_PORT(val)												(1 << (val & 0x1f))

//mclk out gpio I2S_MASTER_MODE
#define MCLK_OUT_A4										SET_I2S_GPIO_ALL_VAL(I2S_MASTER_MODE,IO_GROUP_A,0,0x0a,4)
#define MCLK_OUT_A6										SET_I2S_GPIO_ALL_VAL(I2S_MASTER_MODE,IO_GROUP_A,0,0x0a,6)
#define MCLK_OUT_A14									SET_I2S_GPIO_ALL_VAL(I2S_MASTER_MODE,IO_GROUP_A,0,0x0e,14)
#define MCLK_OUT_A20									SET_I2S_GPIO_ALL_VAL(I2S_MASTER_MODE,IO_GROUP_A,1,0x09,20)
#define MCLK_OUT_B4										SET_I2S_GPIO_ALL_VAL(I2S_MASTER_MODE,IO_GROUP_B,1,0x0b,4)
#define MCLK_OUT_B10									SET_I2S_GPIO_ALL_VAL(I2S_MASTER_MODE,IO_GROUP_B,1,0x08,10)
//mclk in gpio  I2S_SLAVE_MODE
#define MCLK_IN_A4										SET_I2S_GPIO_ALL_VAL(I2S_SLAVE_MODE,IO_GROUP_A,0,0x05,4)
#define MCLK_IN_A6										SET_I2S_GPIO_ALL_VAL(I2S_SLAVE_MODE,IO_GROUP_A,0,0x03,6)
#define MCLK_IN_A14										SET_I2S_GPIO_ALL_VAL(I2S_SLAVE_MODE,IO_GROUP_A,0,0x05,14)
#define MCLK_IN_A20										SET_I2S_GPIO_ALL_VAL(I2S_SLAVE_MODE,IO_GROUP_A,1,0x01,20)
#define MCLK_IN_B4										SET_I2S_GPIO_ALL_VAL(I2S_SLAVE_MODE,IO_GROUP_B,1,0x05,4)
#define MCLK_IN_B10										SET_I2S_GPIO_ALL_VAL(I2S_SLAVE_MODE,IO_GROUP_B,1,0x01,10)
//lrclk gpio
#define I2S0_LRCLK_A6									SET_I2S_GPIO_VAL(IO_GROUP_A,0,0x02,6)
#define I2S0_LRCLK_A11									SET_I2S_GPIO_VAL(IO_GROUP_A,0,0x06,11)
#define I2S1_LRCLK_A11									SET_I2S_GPIO_VAL(IO_GROUP_A,1,0x05,11)
#define I2S1_LRCLK_A17									SET_I2S_GPIO_VAL(IO_GROUP_A,1,0x06,17)
#define I2S1_LRCLK_B1									SET_I2S_GPIO_VAL(IO_GROUP_B,1,0x06,1)
#define I2S1_LRCLK_B7									SET_I2S_GPIO_VAL(IO_GROUP_B,1,0x02,7)
#define I2S2_LRCLK_A17									SET_I2S_GPIO_VAL(IO_GROUP_A,2,0x05,17)
#define I2S2_LRCLK_B1									SET_I2S_GPIO_VAL(IO_GROUP_B,2,0x05,1)
#define I2S0OUT_I2S1IN_LRCLK_A11						SET_I2S_GPIO_VAL(IO_GROUP_A,0,0x02,11)
#define I2S0IN_I2S1OUT_LRCLK_A11						SET_I2S_GPIO_VAL(IO_GROUP_A,0,0x03,11)
#define I2S0IN_I2S1IN_LRCLK_A11							SET_I2S_GPIO_VAL(IO_GROUP_A,0,0x04,11)
#define I2S1OUT_I2S2IN_LRCLK_A17						SET_I2S_GPIO_VAL(IO_GROUP_A,1,0x02,17)
#define I2S1IN_I2S2OUT_LRCLK_A17						SET_I2S_GPIO_VAL(IO_GROUP_A,1,0x03,17)
#define I2S1IN_I2S2IN_LRCLK_A17							SET_I2S_GPIO_VAL(IO_GROUP_A,1,0x04,17)
#define I2S1OUT_I2S2IN_LRCLK_B1							SET_I2S_GPIO_VAL(IO_GROUP_B,1,0x02,1)
#define I2S1IN_I2S2OUT_LRCLK_B1							SET_I2S_GPIO_VAL(IO_GROUP_B,1,0x03,1)
#define I2S1IN_I2S2IN_LRCLK_B1							SET_I2S_GPIO_VAL(IO_GROUP_B,1,0x04,1)
//bclk gpio
#define I2S0_BCLK_A7									SET_I2S_GPIO_VAL(IO_GROUP_A,0,0x05,7)
#define I2S0_BCLK_A10									SET_I2S_GPIO_VAL(IO_GROUP_A,0,0x08,10)
#define I2S1_BCLK_A10									SET_I2S_GPIO_VAL(IO_GROUP_A,1,0x07,10)
#define I2S1_BCLK_A16									SET_I2S_GPIO_VAL(IO_GROUP_A,1,0x06,16)
#define I2S1_BCLK_B0									SET_I2S_GPIO_VAL(IO_GROUP_B,1,0x08,0)
#define I2S1_BCLK_B6									SET_I2S_GPIO_VAL(IO_GROUP_B,1,0x02,6)
#define I2S2_BCLK_A16									SET_I2S_GPIO_VAL(IO_GROUP_A,2,0x05,16)
#define I2S2_BCLK_B0									SET_I2S_GPIO_VAL(IO_GROUP_B,2,0x07,0)
#define I2S0OUT_I2S1IN_BCLK_A10							SET_I2S_GPIO_VAL(IO_GROUP_A,0,0x04,10)
#define I2S0IN_I2S1OUT_BCLK_A10							SET_I2S_GPIO_VAL(IO_GROUP_A,0,0x05,10)
#define I2S0IN_I2S1IN_BCLK_A10							SET_I2S_GPIO_VAL(IO_GROUP_A,0,0x06,10)
#define I2S1OUT_I2S2IN_BCLK_A16							SET_I2S_GPIO_VAL(IO_GROUP_A,1,0x02,16)
#define I2S1IN_I2S2OUT_BCLK_A16							SET_I2S_GPIO_VAL(IO_GROUP_A,1,0x03,16)
#define I2S1IN_I2S2IN_BCLK_A16							SET_I2S_GPIO_VAL(IO_GROUP_A,1,0x04,16)
#define I2S1OUT_I2S2IN_BCLK_B0							SET_I2S_GPIO_VAL(IO_GROUP_B,1,0x04,0)
#define I2S1IN_I2S2OUT_BCLK_B0							SET_I2S_GPIO_VAL(IO_GROUP_B,1,0x05,0)
#define I2S1IN_I2S2IN_BCLK_B0							SET_I2S_GPIO_VAL(IO_GROUP_B,1,0x06,0)
//dout gpio
#define I2S0_DOUT_A4									SET_I2S_GPIO_VAL(IO_GROUP_A,0,0x09,4) //I2S单线模式
#define I2S0_DOUT_A5									SET_I2S_GPIO_VAL(IO_GROUP_A,0,0x07,5) //I2S单线模式
#define I2S1_DOUT0_A12									SET_I2S_GPIO_VAL(IO_GROUP_A,1,0x0b,12)//I2S单线模式
#define I2S1_DOUT1_A13									SET_I2S_GPIO_VAL(IO_GROUP_A,1,0x0b,13)
#define I2S1_DOUT2_A14									SET_I2S_GPIO_VAL(IO_GROUP_A,1,0x0d,14)
#define I2S1_DOUT3_A15									SET_I2S_GPIO_VAL(IO_GROUP_A,1,0x0c,15)
#define I2S1_DOUT0_B8									SET_I2S_GPIO_VAL(IO_GROUP_B,1,0x07,8) //I2S单线模式
#define I2S1_DOUT1_B9									SET_I2S_GPIO_VAL(IO_GROUP_B,1,0x06,9)
#define I2S1_DOUT2_B10									SET_I2S_GPIO_VAL(IO_GROUP_B,1,0x09,10)
#define I2S1_DOUT3_B11									SET_I2S_GPIO_VAL(IO_GROUP_B,1,0x05,11)
#define I2S2_DOUT0_A18									SET_I2S_GPIO_VAL(IO_GROUP_A,2,0x0a,18)//I2S单线模式
#define I2S2_DOUT1_A19									SET_I2S_GPIO_VAL(IO_GROUP_A,2,0x09,19)
#define I2S2_DOUT2_A20									SET_I2S_GPIO_VAL(IO_GROUP_A,2,0x0a,20)
#define I2S2_DOUT3_A21									SET_I2S_GPIO_VAL(IO_GROUP_A,2,0x08,21)
#define I2S2_DOUT0_B2									SET_I2S_GPIO_VAL(IO_GROUP_B,2,0x0c,2) //I2S单线模式
#define I2S2_DOUT1_B3									SET_I2S_GPIO_VAL(IO_GROUP_B,2,0x0c,3)
#define I2S2_DOUT2_B4									SET_I2S_GPIO_VAL(IO_GROUP_B,2,0x0a,4)
#define I2S2_DOUT3_B5									SET_I2S_GPIO_VAL(IO_GROUP_B,2,0x09,5)
//din gpio
#define I2S0_DIN_A4										SET_I2S_GPIO_VAL(IO_GROUP_A,0,0x04,4) //I2S单线模式
#define I2S0_DIN_A5										SET_I2S_GPIO_VAL(IO_GROUP_A,0,0x04,5) //I2S单线模式
#define I2S1_DIN0_A12									SET_I2S_GPIO_VAL(IO_GROUP_A,1,0x03,12)//I2S单线模式
#define I2S1_DIN0_A13									SET_I2S_GPIO_VAL(IO_GROUP_A,1,0x04,13)//I2S单线模式
#define I2S1_DIN0_A14									SET_I2S_GPIO_VAL(IO_GROUP_A,1,0x04,14)//I2S单线模式
#define I2S1_DIN0_A16									SET_I2S_GPIO_VAL(IO_GROUP_A,1,0x07,16)//I2S单线模式
#define I2S1_DIN0_B8									SET_I2S_GPIO_VAL(IO_GROUP_B,1,0x02,8) //I2S单线模式
#define I2S1_DIN0_B9									SET_I2S_GPIO_VAL(IO_GROUP_B,1,0x03,9) //I2S单线模式
#define I2S1_DIN0_B10									SET_I2S_GPIO_VAL(IO_GROUP_B,1,0x03,10)//I2S单线模式
#define I2S1_DIN1_A13									SET_I2S_GPIO_VAL(IO_GROUP_A,1,0x03,13)
#define I2S1_DIN1_A15									SET_I2S_GPIO_VAL(IO_GROUP_A,1,0x04,15)
#define I2S1_DIN1_A17									SET_I2S_GPIO_VAL(IO_GROUP_A,1,0x07,17)
#define I2S1_DIN1_B9									SET_I2S_GPIO_VAL(IO_GROUP_B,1,0x02,9)
#define I2S1_DIN1_B11									SET_I2S_GPIO_VAL(IO_GROUP_B,1,0x02,11)
#define I2S1_DIN2_A14									SET_I2S_GPIO_VAL(IO_GROUP_A,1,0x03,14)
#define I2S1_DIN2_A18									SET_I2S_GPIO_VAL(IO_GROUP_A,1,0x02,18)
#define I2S1_DIN2_B10									SET_I2S_GPIO_VAL(IO_GROUP_B,1,0x02,10)
#define I2S1_DIN3_A15									SET_I2S_GPIO_VAL(IO_GROUP_A,1,0x03,15)
#define I2S1_DIN3_A19									SET_I2S_GPIO_VAL(IO_GROUP_A,1,0x02,19)
#define I2S1_DIN3_B11									SET_I2S_GPIO_VAL(IO_GROUP_B,1,0x01,11)
#define I2S2_DIN0_A18									SET_I2S_GPIO_VAL(IO_GROUP_A,2,0x03,18)//I2S单线模式
#define I2S2_DIN0_A19									SET_I2S_GPIO_VAL(IO_GROUP_A,2,0x04,19)//I2S单线模式
#define I2S2_DIN0_A20									SET_I2S_GPIO_VAL(IO_GROUP_A,2,0x03,20)//I2S单线模式
#define I2S2_DIN0_B2									SET_I2S_GPIO_VAL(IO_GROUP_B,2,0x03,2) //I2S单线模式
#define I2S2_DIN0_B3									SET_I2S_GPIO_VAL(IO_GROUP_B,2,0x04,3) //I2S单线模式
#define I2S2_DIN0_B4									SET_I2S_GPIO_VAL(IO_GROUP_B,2,0x04,4) //I2S单线模式
#define I2S2_DIN0_B6									SET_I2S_GPIO_VAL(IO_GROUP_B,2,0x01,6) //I2S单线模式
#define I2S2_DIN1_A19									SET_I2S_GPIO_VAL(IO_GROUP_A,2,0x03,19)
#define I2S2_DIN1_A21									SET_I2S_GPIO_VAL(IO_GROUP_A,2,0x02,21)
#define I2S2_DIN1_B3									SET_I2S_GPIO_VAL(IO_GROUP_B,2,0x03,3)
#define I2S2_DIN1_B5									SET_I2S_GPIO_VAL(IO_GROUP_B,2,0x04,5)
#define I2S2_DIN1_B7									SET_I2S_GPIO_VAL(IO_GROUP_B,2,0x01,7)
#define I2S2_DIN2_A20									SET_I2S_GPIO_VAL(IO_GROUP_A,2,0x02,20)
#define I2S2_DIN2_B4									SET_I2S_GPIO_VAL(IO_GROUP_B,2,0x03,4)
#define I2S2_DIN2_B8									SET_I2S_GPIO_VAL(IO_GROUP_B,2,0x01,8)
#define I2S2_DIN3_A21									SET_I2S_GPIO_VAL(IO_GROUP_A,2,0x01,21)
#define I2S2_DIN3_B5									SET_I2S_GPIO_VAL(IO_GROUP_B,2,0x03,5)
#define I2S2_DIN3_B9									SET_I2S_GPIO_VAL(IO_GROUP_B,2,0x01,9)

#define SET_I2S_PIN_MODE(X, Mode)	(GET_I2S_IO_GROUP(X) ? (GPIO_PortBModeSet(GET_I2S_GPIO_PORT(X), (Mode))) : GPIO_PortAModeSet(GET_I2S_GPIO_PORT(X), (Mode)))

#ifdef __cplusplus
}
#endif // __cplusplus 

#endif //__ADC_INTERFACE_H__

