#ifndef __LE_AUDIO_MODE_H__
#define __LE_AUDIO_MODE_H__

#include "le_audio_config.h"

bool LeAudioSinkPlayInit(void);
bool LeAudioSinkPlayDeinit(void);
void LeAudioSinkPlayRunning(uint16_t msgId);

void LeAudioSinkModeEnter(void);
void LeAudioSinkModeExit(void);

#endif
