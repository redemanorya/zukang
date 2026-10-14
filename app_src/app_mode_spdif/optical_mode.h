/**
 **************************************************************************************
 * @file    optical_mode.h
 * @brief   optical
 *
 * @author  Yangyu
 * @version V1.0.0
 *
 * $Created: 2026-1-26$
 *
 * @Copyright (C) 2026, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __OPTICAL_MODE_H__
#define __OPTICAL_MODE_H__

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

bool SpdifOpticalPlayInit(void);
void SpdifOpticalPlayRun(uint16_t msgId);
bool SpdifOpticalPlayDeinit(void);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __OPTICAL_MODE_H__
