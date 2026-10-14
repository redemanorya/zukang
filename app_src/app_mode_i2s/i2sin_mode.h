/**
 **************************************************************************************
 * @file    i2sin_mode.h
 * @brief   i2s in play mode 
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2019-1-4 17:27:00$
 *
 * @Copyright (C) 2018, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __I2SIN_MODE_H__
#define __I2SIN_MODE_H__


#ifdef __cplusplus
extern "C"{
#endif // __cplusplus 

typedef struct _I2SInPlayContext
{
	uint32_t			*I2SFIFO1;			//I2S的DMA循环fifo
#if defined(CFG_FUNC_I2SIN_TX_EN)
	uint32_t			*I2STxFIFO;			//I2S Out循环fifo
#endif

	//play
	uint32_t 			SampleRate; //带提示音时，如果不重采样，要避免采样率配置冲突

}I2SInPlayContext;

bool  I2SInPlayInit(void);
bool I2SInPlayDeinit(void);
void I2SInPlayRun(uint16_t msgId);
#ifdef __cplusplus
}
#endif // __cplusplus 

#endif // __I2SIN_MODE_H__
