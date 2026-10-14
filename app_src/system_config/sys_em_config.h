
///////////////////////////////////////////////////////////////////////////////
//               Mountain View Silicon Tech. Inc.
//  Copyright 2012, Mountain View Silicon Tech. Inc., Shanghai, China
//                       All rights reserved.
//  Filename: sys_em_config.h
//  maintainer: kk
///////////////////////////////////////////////////////////////////////////////

#ifndef SYSTEM_EM_CONFIG_H_
#define SYSTEM_EM_CONFIG_H_

#include "bt_config.h"
#include "chip_config.h"

//****************************************************************************************
// 芯片RAM宏定义，请勿随意修改
//****************************************************************************************
//#define CFG_D16K_MEM16K_EN		0		//1 ---> D-Cache拿出16K给RAM用,D-Cache 16KB
//										//0 ---> D-Cache 32KB
//#define CFG_D16KMEM16K_RAM_SIZE	(CFG_D16K_MEM16K_EN*16*1024)

#define CFG_CHIP_RAM_SIZE		    (640*1024)

#define CFG_CHIP_RAM_START		    (0x20000000)


//****************************************************************************************
// BB_EM, TCM, STACK config
//****************************************************************************************
#define BB_EM_MAP_ADDR				0x80000000

#define	TCM_SIZE		24*1024
#define	STACK_SIZE		4*1024

#if ((BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT) && (BT_TWS_SUPPORT) && (BT_SUPPORT))
#define BB_EM_SIZE					(40*1024)
#elif ((BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT) && (BT_SUPPORT))
#define BB_EM_SIZE					(36*1024)
#elif ((BT_TWS_SUPPORT) && (BT_SUPPORT) && (BLE_SUPPORT))
#define BB_EM_SIZE					(36*1024)
#elif ((BT_TWS_SUPPORT) && (BT_SUPPORT) && (!BLE_SUPPORT))
#define BB_EM_SIZE					(24*1024)
#elif( (BLE_SUPPORT) && (BT_SUPPORT))
#define BB_EM_SIZE					(32*1024)
#elif( (!BLE_SUPPORT) && (BT_SUPPORT))
#define BB_EM_SIZE					(20*1024)
#elif( (BLE_SUPPORT) && (!BT_SUPPORT))
#define BB_EM_SIZE					(20*1024)
#else
#define BB_EM_SIZE					0
#endif

#ifdef DUAL_CORE_EN
#define	CORE2_RAM_SIZE		8*1024
#else
#define	CORE2_RAM_SIZE		0
#endif

#define BB_EM_START_PARAMS			(CORE2_RAM_SIZE)

#define BB_MPU_START_ADDR			(CFG_CHIP_RAM_START + CFG_CHIP_RAM_SIZE)



//****************************************************************************************
// 蓝牙库使用判定
//****************************************************************************************
#if ((BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT) && (BT_TWS_SUPPORT) && BT_SUPPORT )
	#define	USE_BT_BIS_TWS_LIB
#elif ((BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT) && BT_SUPPORT )
	#define	USE_BT_BIS_LIB
#elif ((BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT) && BT_SUPPORT )
	#define	USE_BT_CIS_LIB
#elif ((BT_TWS_SUPPORT) && (BLE_SUPPORT) && BT_SUPPORT)
	#define	USE_BT_TWS_LIB
#elif ((BT_TWS_SUPPORT) && (BLE_SUPPORT == DISABLE) && BT_SUPPORT)
	#define	USE_BT_TWS_NO_BLE_LIB
#elif ((BLE_SUPPORT == DISABLE) && (BT_SUPPORT == ENABLE))
    #define USE_BT_NO_BLE_LIB
#elif ((BLE_SUPPORT == ENABLE) && (BT_SUPPORT == DISABLE) && (BT_LE_AUDIO_SUPPORT == 0))
    #define USE_BLE_LIB
#else
	#define USE_BT_STD_LIB
#endif


#endif /*__BT_EM_CFG_H__*/

