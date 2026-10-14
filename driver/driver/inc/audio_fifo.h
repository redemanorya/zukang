/**
 **************************************************************************************
 * @file    audio_fifo.h
 * @brief   
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2025-09-03 13:32:38$
 *
 * @copyright Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

/**
 * @addtogroup AUDIOFIFO
 * @{
 * @defgroup audio_fifo audio_fifo.h
 * @{
 */

#ifndef __FFT_H__
#define __FFT_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#include "type.h"

void AudioFifo_Config(bool width, uint32_t SampleLen);

void AudioFifo_IntEnable(void);

void AudioFifo_Clear(void);

bool AudioFifo_IsDataDone(void);


uint32_t AudioFifo_DataLenGet(void);

void AudioFifo_DataGet(uint32_t * buf, uint32_t Len);

void AufifoInterrupt(void);


#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//audio_fifo

/**
 * @}
 * @}
 */
