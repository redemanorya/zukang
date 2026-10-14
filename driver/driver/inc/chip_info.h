/**
 **************************************************************************************
 * @file    chip_info.h
 * @brief	define all chip info
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2018-1-15 19:46:00$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

/**
 * @addtogroup SYSTEM
 * @{
 * @defgroup chip_info chip_info.h
 * @{
 */
 
#ifndef __CHIP_INFO_H__
#define __CHIP_INFO_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#include "type.h"

#define		CHIP_VERSION_ECO0		0xFF
#define		CHIP_VERSION_ECO1		0xFE

/**
 * @brief	获取芯片ECO版本
 * @param	无
 * @return	ECO版本
 */
uint16_t Chip_Version(void);

/**
 * @brief	芯片上电硬件初始化
 * @param	无
 * @return	无
 * @note    芯片上电main函数里必须调用
 */
void Chip_Init(void);

/**
 * @brief	芯片温度传感器模拟通道使能
 * @param	无
 * @return	无
 */
void Chip_TmpSensorEn(void);

/**
 * @brief	芯片温度传感器模拟通道禁能
 * @param	无
 * @return	无
 */
void Chip_TmpSensorDis(void);

/**
 * @brief	dirver库版本号获取
 * @param	无
 * @return	芯片版本号（字符串）
 */
const unsigned char *GetLibVersionDriver(void);

/**
 * @brief	lrc库版本号获取
 * @param	无
 * @return	芯片版本号（字符串）
 */
const unsigned char *GetLibVersionLrc(void);

/**
 * @brief	rtc库版本号获取
 * @param	无
 * @return	芯片版本号（字符串）
 */
const unsigned char *GetLibVersionRTC(void);

/**
 * @brief	fatfsacc库版本号获取
 * @param	无
 * @return	芯片版本号（字符串）
 */
const unsigned char *GetLibVersionFatfsACC(void);

/**
 * @brief	获取芯片的唯一ID号
 * @param	64bit ID号
 * @return	无
 */
void Chip_IDGet(uint64_t* IDNum);

/**
 * @brief	获取芯片用户自定义数据
 * @param	自定义数据
 * @return	无
 */
uint32_t Chip_UserDataGet(void);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif /*__CHIP_INFO_H__*/

/**
 * @}
 * @}
 */

