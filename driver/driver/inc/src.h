/**
 **************************************************************************************
 * @file    src.h
 * @brief   SRC Module API
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2025-9-13 15:27:00$
 *
 * @copyright Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
 
/**
 * @addtogroup SRC
 * @{
 * @defgroup src src.h
 * @{
 */
 
#ifndef __SRC_H__
#define __SRC_H__
 
#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#include "type.h"
/**
 * @brief SRC模块
 */
typedef enum _SRC_MODULE
{
	SRC0_MODULE = 0,
	SRC1_MODULE,
	SRC2_MODULE
} SRC_MODULE;

typedef enum _SRC_MODE
{
	MODE_STREAMING = 0,
	MODE_CIRCULAR,
	MODE_BLOCK
} SRC_MODE;

typedef enum _SRC_WIDTH
{
	SRC_16BIT = 0,
	SRC_24BIT = 2,
} SRC_WIDTH;

typedef struct _SRC_Context
{
	uint32_t* InBuf;
	uint16_t  InBufLen;
	uint32_t* OutBuf;
	uint16_t  OutBufLen;
	uint16_t  Mode;
	uint16_t  NumChannels; //1:单声道；2: 双声道
	uint16_t  Width;
	uint16_t  BlockSamples;//
} SRC_Context;

/**
 * @brief  SRC输入采样率设置
 * @param  ModuleIndex SRC模块；
 * @param  SampleRate 采样率值
 * @return 无
 * @note   支持的采样率：8000/11025/12000/16000/22050/24000/32000/44100/48000/88200/96000/176400/192000
 */
void SRCInSampleRateSet(SRC_MODULE ModuleIndex, uint32_t SampleRate);

/**
 * @brief  SRC输出采样率设置
 * @param  ModuleIndex SRC模块
 * @param  SampleRate 采样率值
 * @return 无
 * @note   支持的采样率：8000/11025/12000/16000/22050/24000/32000/44100/48000/88200/96000/176400/192000
 */
void SRCOutSampleRateSet(SRC_MODULE ModuleIndex, uint32_t SampleRate);

/**
 * @brief  SRC模式设置
 * @param  ModuleIndex SRC模块；
 * @param  Mode 工作模式
 * @return 无
 */
void SRC_ModeSet(SRC_MODULE ModuleIndex, uint16_t Mode);

/**
 * @brief  SRC声道设置
 * @param  ModuleIndex SRC模块；
 * @param  Channels 1：单声道；2：立体声
 * @return 无
 */
void SRC_ChannelsSet(SRC_MODULE ModuleIndex, uint16_t Channels);

/**
 * @brief  SRC位宽设置
 * @param  ModuleIndex SRC模块；
 * @param  Width SRC_16BIT/SRC_24BIT
 * @return 无
 */
void SRC_WidthSet(SRC_MODULE ModuleIndex, SRC_WIDTH Width);

/**
 * @brief  SRC转换长度设置
 * @param  ModuleIndex SRC模块；
 * @param  Len 长度
 * @return 无
 * @note   单位samples
 */
void SRC_BlockLenSet(SRC_MODULE ModuleIndex, uint16_t Len);

/**
 * @brief  SRC转换长度设置
 * @param  ModuleIndex SRC模块；
 * @param  SRC_Context SRC结构体
 * @param  sr_original 输入采样率
 * @param  sr_target 输出采样率
 * @return 无
 */
void SRC_Init(SRC_MODULE ModuleIndex, SRC_Context *ct, int32_t sr_original, int32_t sr_target);

/**
 * @brief  SRC Block 启动SRC
 * @param  ModuleIndex SRC模块；
 * @return 无
 */
void SRC_BlockStart(SRC_MODULE ModuleIndex);

/**
 * @brief  SRC Block done标志清理
 * @param  ModuleIndex SRC模块；
 * @return 无
 */
void SRC_BlockDoneFlagClean(SRC_MODULE ModuleIndex);

/**
 * @brief  SRC Block 转换完成
 * @param  ModuleIndex SRC模块；
 * @return 无
 */
bool SRC_IsBlockDone(SRC_MODULE ModuleIndex);

/**
 * @brief  SRC Block 转换数据长度获取
 * @param  ModuleIndex SRC模块；
 * @return 无
 */
uint16_t SRC_BlockDataLenGet(SRC_MODULE ModuleIndex);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__SRC_H__

/**
 * @}
 * @}
 */

