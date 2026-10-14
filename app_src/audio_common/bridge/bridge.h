/*
 * bridge.h
 *
 *  Created on: Jul 13, 2026
 *      Author: piwang
 */
#ifndef _BRIDGE_H_
#define _BRIDGE_H_

#include "type.h"
#include "src.h"
#include "mcu_circular_buf.h"

typedef struct _Bridge_Context
{
	MCU_CIRCULAR_CONTEXT BridgeFifoCt;
	void * 		BridgeBuffer;
	uint8_t		SinkFlowNet;
	uint32_t	SinkSampleRate;
	uint8_t		SourceFlowNet;
	uint32_t	SourceSampleRate;
	uint32_t	FifoSampleRate;
	uint32_t	FifoSampleLen;
	SRC_Context *pBridgeSrcCt;
} Bridge_Context;

bool BridgeSinkSourceInit(void);

void BridgeSinkSourceDeinit(void);

uint16_t BridgeMusic_SinkDataSet(void* Buf, uint16_t Len);

uint16_t BridgeMusic_SinkGetSpaceLen(void);

uint16_t BridgeMusic_SouceDataGet(void* Buf, uint16_t Len);

uint16_t BridgeMusic_SouceGetDataLen(void);

void BridgeMusicSrcInit(void);

bool BridgeMusicInit(void);

void BridgeMusicDeinit(void);



uint16_t BridgeMusic2_SinkDataSet(void* Buf, uint16_t Len);

uint16_t BridgeMusic2_SinkGetSpaceLen(void);

uint16_t BridgeMusic2_SouceDataGet(void* Buf, uint16_t Len);

uint16_t BridgeMusic2_SouceGetDataLen(void);

void BridgeMusic2SrcInit(void);

bool BridgeMusic2Init(void);

void BridgeMusic2Deinit(void);


uint16_t BridgeMic_SinkDataSet(void* Buf, uint16_t Len);

uint16_t BridgeMic_SinkGetSpaceLen(void);

uint16_t BridgeMic_SouceDataGet(void* Buf, uint16_t Len);

uint16_t BridgeMic_SouceGetDataLen(void);

void BridgeMicSrcInit(void);

bool BridgeMicInit(void);

void BridgeMicDeinit(void);


uint16_t BridgeMic2_SinkDataSet(void* Buf, uint16_t Len);

uint16_t BridgeMic2_SinkGetSpaceLen(void);

uint16_t BridgeMic2_SouceDataGet(void* Buf, uint16_t Len);

uint16_t BridgeMic2_SouceGetDataLen(void);

void BridgeMic2SrcInit(void);

bool BridgeMic2Init(void);

void BridgeMic2Deinit(void);

#endif /* _BRIDGE_H_ */
