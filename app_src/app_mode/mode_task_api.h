/*
 * mode_task_api.h
 *
 *  Created on: Mar 30, 2021
 *      Author: piwang
 */

#ifndef _MODE_TASK_API_H_
#define _MODE_TASK_API_H_
#include "type.h"

#ifdef AUDIO_STREAM_UNINTERRUPT
extern uint32_t IsResRamReallocate;//正常情况下不需要释放
extern uint32_t IsResRamRelease;//通话模式需要释放资源，所以需要做处理
#endif

//配置系统标准通路
bool ModeCommonInit(void);

//清理标准通路配置，模式退出后未调用时 标准通路后台化，切模式期间无音效。
void ModeCommonDeinit(void);

//各模式下的通用消息处理
void CommonMsgProccess(uint16_t Msg);

bool AudioIoCommonForHfp(uint16_t gain);

void PauseAudioCore(void);

#ifdef CFG_AUDIO_OUT_AUTO_SAMPLE_RATE_44100_48000
void AudioOutSampleRateSet(uint32_t SampleRate);
#endif

#endif /* _MODE_TASK_API_H_ */
