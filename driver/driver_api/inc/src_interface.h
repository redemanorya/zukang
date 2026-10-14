/**
 **************************************************************************************
 * @file    src_interface.h
 * @brief   audio src interface
 *
 * @author  Sam
 * @version V1.1.0
 *
 * $Created: 2025-09-15 17:01:05$
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#ifndef		__SRC_INTERFACE_H__
#define		__SRC_INTERFACE_H__

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus 
#include "type.h"
#include "src.h"
#include "dma.h"
/***********************************
 * Apply T (uS) = 56448 * Len(Sample) * RealDiv / (SampleRateIn(K) * PLL(M))
 * ʵ�⣺
 * Tdiv1: ~ 112uS (Len:128;SampleRateIn:192K;APLL:336M)
 * Tdiv2: ~ 2.37 * Tdiv1
 * Tdiv3: 3.5 * Tdiv1
 * Tdiv4: 4.5 * Tdiv1
 * *********************************/
int16_t SRC_BlockInit(SRC_MODULE ModuleIndex, SRC_Context* srcContext, int32_t SampleRateIn, int32_t SampleRateOut);

int16_t SRC_BlockApply(SRC_MODULE ModuleIndex, SRC_Context* srcContext, int16_t *pcm_in, int16_t *pcm_out, int32_t n);

void SRC_BlockApplyStart(SRC_MODULE ModuleIndex, SRC_Context* srcContext, int16_t *pcm_in, int32_t n);

int16_t SRC_BlockApplyDone(SRC_MODULE ModuleIndex, SRC_Context* srcContext, int16_t *pcm_out);

#ifdef __cplusplus
}
#endif // __cplusplus 

#endif //__SRC_INTERFACE_H__

