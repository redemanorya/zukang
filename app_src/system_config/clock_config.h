
///////////////////////////////////////////////////////////////////////////////
//               Mountain View Silicon Tech. Inc.
//  Copyright 2012, Mountain View Silicon Tech. Inc., Shanghai, China
//                       All rights reserved.
//  Filename: clock_config.h
//  maintainer: kk
///////////////////////////////////////////////////////////////////////////////
#include "app_config.h"

#ifndef CLOCK_CONFIG_H_
#define CLOCK_CONFIG_H_


#define SYS_CRYSTAL_FREQ			24*1000*1000	//晶体频率 Hz

//系统时钟可以选择APLL，或者DPLL1
#if (FUNC_RF_EN || defined(FUNC_WIRELESS2_SUPPORT))
#define APLL_FREQ					336* 1000000
#else
#define APLL_FREQ					432* 1000000
#endif

#if defined(FUNC_WIRELESS2_SUPPORT) || defined(CFG_FUNC_EFFECT_CORE_DUAL_EN)
#define DPLL1_FREQ					384* 1000000
#else
#define DPLL1_FREQ					336* 1000000
#endif

#define	CFG_DPLL0_SUPPORT			FALSE//TRUE
//#define	CFG_DPLL1_SPREADSPECTRUM_EN	//DPLL1进行展频，默认关闭

#endif /*__BT_EM_CFG_H__*/

