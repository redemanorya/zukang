/**
  *****************************************************************************
  * @file:			spdif.h
  * @author			Cecilia
  * @version		V1.0.0
  * @data			2015-7-20
  * @Brief			SPDIF driver interface
  ******************************************************************************
 */
/**
 * @addtogroup SPDIF
 * @{
 * @defgroup spdif spdif.h
 * @{
 */
 
#ifndef __SPDIF_H__
#define __SPDIF_H__
 
#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#include "type.h"

typedef enum __SPDIF0_ANA_CH_SEL
{
	SPDIF_ANA_INPUT_B0 = 0,

	SPDIF_ANA_INPUT_B1,

	SPDIF_ANA_INPUT_B2,

	SPDIF_ANA_INPUT_B3

} SPDIF0_ANA_CH_SEL;

typedef enum __SPDIF_ANA_INPUT_LEVEL
{
	SPDIF_ANA_LEVEL_300mVpp = 0,

	SPDIF_ANA_LEVEL_150mVpp

} SPDIF_ANA_LEVEL;

typedef enum __SPDIF_ANA_INPUT_MODE
{
	SPDIF_ANA_LVL = 0,

	SPDIF_ANA_EARC

} SPDIF_ANA_MODE;

typedef enum __SPDIF_DATA_WORDLTH
{
    SPDIF_WORDLTH_16BIT = 16,
    SPDIF_WORDLTH_20BIT = 20,
    SPDIF_WORDLTH_24BIT = 24,

} SPDIF_DATA_WORDLTH;    
    
typedef enum __SPDIF_FLAG_STATUS
{
    PARITY_FLAG_STATUS = 21,
    UNDER_FLAG_STATUS  = 22,
    OVER_FLAG_STATUS   = 23,
    EMPTY_FLAG_STATUS  = 24,
    AEMPTY_FLAG_STATUS = 25,
    FULL_FLAG_STATUS   = 26,
    AFULL_FLAG_STATUS  = 27,
    SYNC_FLAG_STATUS   = 28,
    LOCK_FLAG_STATUS   = 29,
    BEGIN_FLAG_STATUS  = 30,
	RIGHT_LEFT_FLAG_STATUS = 31,
    
} SPDIF_STATUS_TYPE;    

typedef enum __SPDIF_INT_TYPE
{
    PARITY_ERROR_INT   = 1 << 21,    //校验错误中断
    UNDER_ERROR_INT    = 1 << 22,    //
    OVER_ERROR_INT     = 1 << 23,    //
    EMPTY_ERROR_INT    = 1 << 24,    //FIFO空中断
    AEMPTY_ERROR_INT   = 1 << 25,    //FIFO接近空但不为空中断
    FULL_ERROR_INT     = 1 << 26,
    AFULL_ERROR_INT    = 1 << 27,
    SYNC_ERROR_INT     = 1 << 28,
    LOCK_INT           = 1 << 29,
    BEGIN_FRAME_INT    = 1 << 30,
    ALL_SPDIF_INDIV_INT = 1 << 31     //以上中断独立控制

} SPDIF_INT_TYPE;    

typedef enum __SPDIF_CLOCK_SOURCE_INDEX
{

	SPDIF_CLK_SOURCE_PLL = 0,	/*设置SPDIF的时钟源为PLL*/
	SPDIF_CLK_SOURCE_AUPLL		/*设置SPDIF的时钟源为AUPLL*/

} SPDIF_CLOCK_SOURCE_INDEX;

typedef enum __SPDIF_MUTE_CTRL_MODULE
{
	AUTO_MUTE_I2S0_ENABLE = 1 << 2,
	AUTO_MUTE_I2S1_ENABLE = 1 << 3,
	AUTO_MUTE_I2S2_ENABLE = 1 << 4,
	AUTO_MUTE_DAC0_ENABLE = 1 << 5,
	AUTO_MUTE_DAC1_ENABLE = 1 << 6,
	AUTO_MUTE_DAC2_ENABLE = 1 << 7,
	AUTO_MUTE_SPDIF_EN    = 1 << 8,

} SPDIF_MUTE_CTRL_MODULE;


typedef enum __SPDIF_AUDIO_DATA_TYPE
{
	SPDIF_AUDIO_NULL_DATA_TYPE 				= 0,
	SPDIF_AUDIO_AC_3_DATA_TYPE 				= 1,
	SPDIF_AUDIO_TIME_STAMP_DATA_TYPE 		= 2,
	SPDIF_AUDIO_PAUSE_DATA_TYPE				= 3,
	SPDIF_AUDIO_MPEG_1_LAYER_1_DATA_TYPE 	= 4,
	SPDIF_AUDIO_MPEG_1_LAYER_2_3_DATA_TYPE 	= 5,
	SPDIF_AUDIO_MPEG_2_DATA_TYPE			= 6,
	SPDIF_AUDIO_MPEG_2_AAC_DATA_TYPE 		= 7,
	SPDIF_AUDIO_MPEG_2_LAYER_1_DATA_TYPE 	= 8,
	SPDIF_AUDIO_MPEG_2_LAYER_2_DATA_TYPE 	= 9,
	SPDIF_AUDIO_MPEG_2_LAYER_3_DATA_TYPE 	= 10,
	SPDIF_AUDIO_DTS_TYPE_1_DATA_TYPE 		= 11,
	SPDIF_AUDIO_DTS_TYPE_2_DATA_TYPE 		= 12,
	SPDIF_AUDIO_DTS_TYPE_3_DATA_TYPE 		= 13,
	SPDIF_AUDIO_ATRAC_DATA_TYPE 			= 14,
	SPDIF_AUDIO_ATRAC_2_3_DATA_TYPE 		= 15,
	SPDIF_AUDIO_ATRAC_X_DATA_TYPE 			= 16,
	SPDIF_AUDIO_DTS_TYPE_4_DATA_TYPE 		= 17,
	SPDIF_AUDIO_WMA_DATA_TYPE 				= 18,
	SPDIF_AUDIO_MPEG_2_AAC_LOW_DATA_TYPE	= 19,
	SPDIF_AUDIO_MPEG_4_AAC_LOW_DATA_TYPE    = 20,
	SPDIF_AUDIO_EN_AC_3_DATA_TYPE 			= 21,
	SPDIF_AUDIO_MAT_DATA_TYPE 				= 22,
	SPDIF_AUDIO_SMPTE_KLV_DATA_TYPE 		= 27,
	SPDIF_AUDIO_DOLBY_E_DATA_TYPE 			= 28,
	SPDIF_AUDIO_CAPTIONING_DATA_TYPE 		= 29,
	SPDIF_AUDIO_USER_DEFINED_DATA_TYPE 		= 30,

	SPDIF_AUDIO_PCM_DATA_TYPE 				= 100,

} SPDIF_AUDIO_DATA_TYPE;

typedef enum __SPDIF_MODULE
{
	SPDIF0 = 0,
	SPDIF1 = 1,

} SPDIF_MODULE;

typedef struct __SPDIF_TYPE_STR
{
    uint8_t              audio_type;
    uint8_t				 bitstream_number;
    uint8_t              IsNewFrame;//0-该帧没有帧头，1-该帧有帧头， 2-该帧有第二帧的帧头
    int32_t             total_length;
    int32_t             output_length;

} SPDIF_TYPE_STR;

/**
 * @brief 20bit和24bit时，数据存放格式
 */
typedef enum _SPDIF_20BIT_24BIT_ALIGN_MODE
{
	SPDIF_HIGH_BITS_ACTIVE = 0,
	SPDIF_LOW_BITS_ACTIVE,
} SPDIF_20BIT_24BIT_ALIGN_MODE;

/**
 * @brief  对SPDIFx进行功能复位
 * @param  spdif_index： SPDIF0 or SPDIF1
 * @return 无
 */
void SPDIF_ModuleRst(SPDIF_MODULE spdif_index);

/**
 * @brief  使能spdif0输入前端电平转换电路,仅适用于SPDIF0
 * @param  ChannelIO： 通道号 详见：#SPDIF0_ANA_CH_SEL
 * @param  Level: SPDIF_ANA_LEVEL_300mVpp: 输入最小信号压差支持300mV
 * 				  SPDIF_ANA_LEVEL_200mVpp: 输入最小信号压差支持200mV
 * @param  Mode: SPDIF_ANA_LVL: 同轴
 * 				 SPDIF_ANA_EARC: HDMI EARC
 * @return 无
 */
void SPDIF0_AnalogModuleEnable(SPDIF0_ANA_CH_SEL ChannelIO, SPDIF_ANA_LEVEL Level, SPDIF_ANA_MODE Mode);


/**
 * @brief  禁能spdif0输入前端电平转换电路
 * @param  无
 * @return 无
 */
void SPDIF0_AnalogModuleDisable(void);

/**
 * @brief  读取当前SPDIF的采样率
 * @param  spdif_index： SPDIF0 or SPDIF1
 * @return 当前采样率
 */
uint32_t SPDIF_SampleRateGet(SPDIF_MODULE spdif_index);

/**
 * @brief  设置发送时的采样率
 * @param  spdif_index： SPDIF0 or SPDIF1
 * @param  SpdifSampleRate: 发送采样率
 * @return 无
 */
void SPDIF_SampleRateSet(SPDIF_MODULE spdif_index, uint32_t SpdifSampleRate);

/**
 * @brief  将SPDIF设置为发送模式并进行初始化
 * @param  spdif_index： SPDIF0 or SPDIF1
 * @param  ParitySrc：生成极性元的源选择，0 - 软件配置 1 - 硬件生成
 * @param  IsValidityEn: 有效位是否进行检测，0 - 有效位不检测，全部写入FIFO中， 
                                           1 - 只有有效位正确的写入FIFO中
 * @param  ChannelMoe: 0: 模式0 - SPDIF作为接收模块或发送模块时，均为双声道数据
                       1: 模式1 - SPDIF作为接收模块时，只把左声道的数据写入FIFO中；作为发送模块时，左声道数据正常，右声道数据为左声道拷贝数据
                       2: 模式2 - SPDIF作为接收模块时，同模式1相同；作为发送模块时，左声道数据正常，右声道数据填0
 * @param  SyncFrameDelay: 同步帧在reset之后的第SyncFrameDelay帧之后发送，如果为0，则Reset之后立即发送同步帧
 * @return 无
 */
void SPDIF_TXInit(SPDIF_MODULE spdif_index, uint8_t ParitySrc, uint8_t IsValidityEn, uint8_t ChannelMode, uint8_t SyncFrameDelay);

/**
 * @brief  将SPDIF设置为接收模式并进行初始化
 * @param  spdif_index： SPDIF0 or SPDIF1
 * @param  ParitySrc：生成极性元的源选择，0 - 软件配置 1 - 硬件生成
 * @param  IsValidityEn: 有效位是否进行检测，0 - 有效位不检测，全部写入FIFO中， 
                                            1 - 只有有效位正确的写入FIFO中
 * @param  ChannelMoe: 0: 模式0 - SPDIF作为接收模块或发送模块时，均为双声道数据
                       1: 模式1 - SPDIF作为接收模块时，只把左声道的数据写入FIFO中；作为发送模块时，左声道数据正常，右声道数据填0
                       2: 模式2 - SPDIF作为接收模块时，同模式1相同；作为发送模块时，左声道数据正常，右声道数据为左声道拷贝数据
 * @return 无
 */
void SPDIF_RXInit(SPDIF_MODULE spdif_index, uint8_t ParitySrc, uint8_t IsValidityEn, uint8_t ChannelMode);

/**
 * @brief  使能SPDIF模块
 * @param  spdif_index： SPDIF0 or SPDIF1
 * @return 无
 */
void SPDIF_ModuleEnable(SPDIF_MODULE spdif_index);

/**
 * @brief  禁能SPDIF模块
 * @param  spdif_index： SPDIF0 or SPDIF1
 * @return 无
 */
void SPDIF_ModuleDisable(SPDIF_MODULE spdif_index);

/**
 * @brief  获取SPDIF状态标志位
 * @param  spdif_index： SPDIF0 or SPDIF1
 * @param  FlagType： SPDIF标志类型，详见 #SPDIF_STATUS_TYPE
 * @return 标志状态
 */
bool SPDIF_FlagStatusGet(SPDIF_MODULE spdif_index, SPDIF_STATUS_TYPE FlagType);

// *******************************************************************************************************************
//                                                  以下是中断部分
// *******************************************************************************************************************
/**
 * @brief  开启或关闭SPDIF中断
 * @param  spdif_index： SPDIF0 or SPDIF1
 * @param  IntType: SPDIF中断类型，详见 #SPDIF_INT_TYPE
 * @param  IsEnable: 0 - 禁能该类型中断，1 - 开启该类型中断
 * @return 无
 */
void SPDIF_SetInt(SPDIF_MODULE spdif_index, SPDIF_INT_TYPE IntType, bool IsEnable);

/**
 * @brief  获取中断标志
 * @param  spdif_index： SPDIF0 or SPDIF1
 * @param  IntType: SPDIF中断类型，详见 #SPDIF_INT_TYPE
 * @return 中断标志
 */
bool SPDIF_GetIntFlg(SPDIF_MODULE spdif_index, SPDIF_INT_TYPE IntType);

/**
 * @brief  清除中断标志
 * @param  spdif_index： SPDIF0 or SPDIF1
 * @param  IntType: SPDIF中断类型，详见 #SPDIF_INT_TYPE
 * @return 无
 */
void SPDIF_ClearIntFlg(SPDIF_MODULE spdif_index, SPDIF_INT_TYPE IntType);

// *******************************************************************************************************************
//                                        以下是PCM格式与SPDIF格式之间的转换
// *******************************************************************************************************************

/**
 * @brief  PCM格式数据转SPDIF格式数据
 * @param  spdif_index： SPDIF0 or SPDIF1
 * @param  PcmBuf：PCM双声道数据格式，LRLR...
 * @param  Length: PCM数据的长度，单位为：Byte
 * @param  SpdifBuf: SPDIF格式数据
 * @param  Wordlth: 数据宽度，详见#SPDIF_DATA_WORDLTH
 * @return SPDIF数据长度，单位：Byte
 */
uint32_t SPDIF_PCMDataToSPDIFData(SPDIF_MODULE spdif_index, int32_t *PcmBuf, uint32_t Length, int32_t *SpdifBuf, SPDIF_DATA_WORDLTH Wordlth);

/**
 * @brief  SPDIF格式数据转PCM格式数据
 * @param  spdif_index： SPDIF0 or SPDIF1
 * @param  SpdifBuf：存放SPDIF数据的首地址
 * @param  Length: SPDIF数据的长度，单位为：Byte
 * @param  PcmBuf: 存放PCM数据的首地址
 * @param  Wordlth: 数据宽度，详见#SPDIF_DATA_WORDLTH
 * @return >0: PCM数据长度，单位：Byte;
 */
int32_t SPDIF_SPDIFDataToPCMData(SPDIF_MODULE spdif_index, int32_t *SpdifBuf, uint32_t Length, int32_t *PcmBuf, SPDIF_DATA_WORDLTH Wordlth);

/**
 * @brief  在20bit和24bit时，数据格式选择
 * @param  AlignMode I2S_20BIT_24BIT_ALIGN_MODE
 *
 * @return NONE
 */
void SPDIF_AlignModeSet(SPDIF_20BIT_24BIT_ALIGN_MODE AlignMode);

/**
 * @brief  SPDIF格式数据转Audio格式数据,包括线性和非线性。
 * @param  SpdifBuf：存放SPDIF数据的首地址
 * @param  Length: SPDIF数据的长度，单位为：Byte
 * @param  AudioBuf: 存放AudioBuf数据的首地址
 * @param  Wordlth: 数据宽度，详见#SPDIF_DATA_WORDLTH
 * @param  p: 存放当前输出帧的大小（Byte），通道号，类型等相关信息
 * @return 无;
 */
void SPDIF_SPDIFDatatoAudioData(int32_t *SpdifBuf, int32_t InLength, int32_t *AudioBuf, SPDIF_DATA_WORDLTH Wordlth, SPDIF_TYPE_STR *p);




void SPDIF_MuteModeSet(SPDIF_MODULE spdif_index, SPDIF_MUTE_CTRL_MODULE module, bool isAutoMute);
void SPDIF_MuteStatusFlagClear(SPDIF_MODULE spdif_index);
void SPDIF0_FramePulseFlagEnable(bool isEnable);
void SPDIF0_FramePulseIntoClock(bool intoClock);
void SPDIF0_BlockPulseFlagEnable(bool isEnable);
void SPDIF0_BlockPulseIntoClock(bool intoClock);
bool SPDIF0_BlockCountReadyFlagGet(void);
uint32_t SPDIF0_BlockCyclesDoutGet(void);
void SPDIF0_CycleCntEnable(uint8_t isEnable);
bool SPDIF0_FrameCountOverFlagGet(void);
bool SPDIF0_BlockCountOverFlagGet(void);
uint32_t SPDIF0_FrameCyclesGet(void);
uint32_t SPDIF0_BlockCyclesGet(void);

// *******************************************************************************************************************
//                               以下是DPLL0 以 SPDIF0 的 block flag 为外部参考时钟使用
// *******************************************************************************************************************
/**
 * @brief  初始化DPLL锁相通路：SPDIF0 block送入DPLL参考，SPDIF PLL时钟源预置APLL
 * @param  无
 * @return 无
 * @note   需在SPDIF接收已建立有效信号后调用
 */
void SPDIF_DpllLockInit(void);
/**
 * @brief  释放DPLL锁相通路：停止block送时钟，释放DPLL0锁定，恢复APLL时钟源
 * @param  无
 * @return 无
 * @note   用于采样率改变、信号丢失重建等场景；释放后可通过Init+Start重新锁定
 */
void SPDIF_DpllLockDeInit(void);

/**
 * @brief  以SPDIF0采样率为参考启动DPLL0锁定（无有效信号则直接返回）
 * @param  无
 * @return 无
 * @note   内部先读取SPDIF0采样率，若为0（无有效信号）则直接返回，不启动锁定
 */
void SPDIF_DpllLockStart(void);
/**
 * @brief  查询DPLL0是否已锁定
 * @param  无
 * @return TRUE: 已锁定; FALSE: 未锁定
 */
bool SPDIF_GetDpllLockFlag(void);
/**
 * @brief  DPLL锁定后置HOLD保持输出，并将SPDIF PLL时钟源切到DPLL
 * @param  无
 * @return 无
 */
void SPDIF_DpllLockHold(void);

/**
 * @brief  设置DPLL0的HOLD保持状态
 * @param  en: 非0使能保持（维持当前锁定输出）; 0解除保持（继续跟踪参考）
 * @return 无
 */
void SPDIF_DpllLockHoldSet(uint8_t en);
/**
 * @brief  查询DPLL0是否处于HOLD
 * @param  无
 * @return TRUE: 保持中; FALSE: 未保持
 */
bool SDPIF_DpllLockHoldGet(void);



#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__SPDIF_H__

/**
 * @}
 * @}
 */

