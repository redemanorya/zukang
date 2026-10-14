/**
 *************************************************************************************
 * @file	adc_levels.h
 * @author	ken bu/bkd
 * @version	v0.0.1
 * @date    2019/04/24
 * @brief	 for  Sliding rheostat
 * @ maintainer: 
 * Copyright (C) Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 *************************************************************************************
 */
#include "app_config.h"
#include "key.h"

#ifdef CFG_ADC_LEVEL_KEY_EN

#define how_many_times_have_effect	3

void ADCLevelsKeyInit(void);
KeyScanMsg AdcLevelKeyProcess(void);


#endif//__POTENTIOMETER_KEY_H__



