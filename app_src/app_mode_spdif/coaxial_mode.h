/**
 **************************************************************************************
 * @file    coaxial_mode.h
 * @brief   coaxial
 *
 * @author  Yangyu
 * @version V1.0.0
 *
 * $Created: 2026-1-26$
 *
 * @Copyright (C) 2026, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __COAXIAL_MODE_H__
#define __COAXIAL_MODE_H__

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

bool SpdifCoaxialPlayInit(void);
void SpdifCoaxialPlayRun(uint16_t msgId);
bool SpdifCoaxialPlayDeinit(void);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __COAXIAL_MODE_H__
