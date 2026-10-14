/**
 *****************************************************************************
 * @file     otg_detect.h
 * @author   Owen
 * @version  V1.0.0
 * @date     27-03-2017
 * @brief    otg port detect module driver interface
 *****************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2015 MVSilicon </center></h2>
 */

/**
 * @addtogroup OTG
 * @{
 * @defgroup otg_detect otg_detect.h
 * @{
 */
#ifndef __OTG_DETECT_H__
#define	__OTG_DETECT_H__

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus 


#include "type.h"
#ifdef CFG_APP_CONFIG
	#include "app_config.h"
#endif

// ---------------------------- FS 函数声明 ----------------------------
/**
 * @brief  Port连接检测函数
 * @param  NONE
 * @return NONE
 */
void OTG_FS_PortLinkCheck(void);

/**
 * @brief  检查Port端口上是否有一个U盘设备连接
 * @param  NONE
 * @return 1-有U盘设备连接，0-无U盘设备连接
 */
bool OTG_FS_PortHostIsLink(void);

/**
 * @brief  检查Port端口上是否有一个PC连接
 * @param  NONE
 * @return 1-有PC连接，0-无PC连接
 */
bool OTG_FS_PortDeviceIsLink(void);

/**
 * @brief  使能port端口检查U盘连接
 * @param  HostEnable: 是否检测U盘连接
 * @param  DeviceEnable: 是否检测PC连接
 * @return NONE
 */
void OTG_FS_PortSetDetectMode(bool HostEnable, bool DeviceEnable);

/**
 * @brief  使能port端口检查暂停
 * @param  PauseEnable: 是否暂停检测
 * @return NONE
 */
void OTG_FS_PortSetDetectPause(bool PauseEnable);

/**
 * @brief  usb快速检测设备插入
 * @param  DetectQuickEnable: 是否使能快速检测(默认使能)
 * @return NONE
 */
void OTG_FS_PortSetDetectQuick(bool DetectQuickEnable);


// ---------------------------- HS 函数声明 ----------------------------
/**
 * @brief  Port连接检测函数
 * @param  NONE
 * @return NONE
 * work with USB_HS_IRQn
 */
void OTG_HS_PortLinkCheck(void);

/**
 * @brief  检查Port端口上是否有一个U盘设备连接
 * @param  NONE
 * @return 1-有U盘设备连接，0-无U盘设备连接
 */
bool OTG_HS_PortHostIsLink(void);

/**
 * @brief  检查Port端口上是否有一个PC连接
 * @param  NONE
 * @return 1-有PC连接，0-无PC连接
 */
bool OTG_HS_PortDeviceIsLink(void);

/**
 * @brief  使能port端口检查U盘连接
 * @param  HostEnable: 是否检测U盘连接
 * @param  DeviceEnable: 是否检测PC连接
 * @return NONE
 */
void OTG_HS_PortSetDetectMode(bool HostEnable, bool DeviceEnable);

///**
// * @brief  使能port端口检查暂停
// * @param  PauseEnable: 是否暂停检测
// * @return NONE
// */
//void OTG_HS_PortSetDetectPause(bool PauseEnable);
//
/**
 * @brief  port端口 HS Mode 取消    默认开启高速
 * @param
 * @return NONE
 */
void OTG_HS_PortHsModeDisable(void);


// ---------------------------- 接口选择宏 ----------------------------
#if defined(OTG_HOST_PORT_FS) || defined(OTG_DEVICE_PORT_FS)
#define OTG_PortLinkCheck             						OTG_FS_PortLinkCheck
#define OTG_PortSetDetectMode(HostEnable,DeviceEnable) 		OTG_FS_PortSetDetectMode(HostEnable,DeviceEnable)
#define OTG_PortSetDetectPause(PauseEnable) 				OTG_FS_PortSetDetectPause(PauseEnable)
#define OTG_PortSetDetectQuick(DetectQuickEnable) 			OTG_FS_PortSetDetectQuick(DetectQuickEnable)
#endif
//FS HS detect 共存，宏映射适配代码继承，HS相关API直接调用

#ifdef OTG_HOST_PORT_FS
#define OTG_PortHostIsLink            						OTG_FS_PortHostIsLink
#else
#define OTG_PortHostIsLink            						OTG_HS_PortHostIsLink
#endif

#ifdef OTG_DEVICE_PORT_FS
#define OTG_PortDeviceIsLink          						OTG_FS_PortDeviceIsLink
#else
#define OTG_PortDeviceIsLink          						OTG_HS_PortDeviceIsLink
#endif

#define OTG_PortHsModeDisable         						OTG_HS_PortHsModeDisable

typedef bool (*OTG_HostPortIsLink_t)(void);

#ifdef __cplusplus
}
#endif // __cplusplus 

#endif //__OTG_DETECT_H__

/**
 * @}
 * @}
 */
