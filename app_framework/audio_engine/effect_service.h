/*
 * effect_service.h
 *
 *  Created on: Mar 3, 2026
 *      Author: piwang
 */

#ifndef EFFECT_SERVICE_H_
#define EFFECT_SERVICE_H_

#include "type.h"
#include "rtos_api.h"
#include "audio_core_api.h"

#define EFFECT_SERVICE_SIZE					512//1024
#define EFFECT_SERVICE_PRIO					6
#define EFFECT_SERVICE_TIMEOUT				1000		/** 1 s */
#define EFFECT_SERVICE_NUM_MESSAGE_QUEUE	20

/**
 * @brief	Create effect proc service.
 * @param	 NONE
 * @return
 */
int32_t EffectServiceCreate(void);

/**
 * @brief	Start effect proc service.
 * @param	 NONE
 * @return
 */
int32_t EffectServiceStart(void);

/**
 * @brief	Exit effect proc service.
 * @param	NONE
 * @return
 */
void EffectServiceStop(void);

int32_t EffectServiceKill(void);

/**
 * @brief	send message to effect service
 * @param	NONE
 * @return	bool
 */
bool SendEffectServiceMsg(uint16_t msgId);


void EffectProcessConfig(AudioCoreProcessFunc Process, uint8_t FlowNet);


AudioCoreProcessFunc GetEffectProcess(void);

#endif /* EFFECT_SERVICE_H_ */
