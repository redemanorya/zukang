/**
 **************************************************************************************
 * @file    split_gain.h
 * @brief   interface for user defined effect algorithm
 *
 * @author  Castle Cai
 * @version V1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __SIMPLE_GAIN_H__
#define __SIMPLE_GAIN_H__

#include "roboeffect_api.h"
#include "roboeffect_config.h"
typedef struct _simple_gain_struct
{
	int16_t data_a;
	int16_t data_b;
	int32_t *ptr;
} simple_gain_struct;

#endif/*__SIMPLE_GAIN_H__*/
