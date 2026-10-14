/*
 * bridge_mic.c
 *
 *  Created on: Jul 13, 2026
 *      Author: piwang
 */

#include "mvintrinsics.h"
#include "src.h"
#include "reset.h"
#include "resource_defs.h"
#include "src_interface.h"
#include "audio_effect.h"
#include "rtos_api.h"
#include "app_config.h"
#include "bridge/bridge.h"

#ifdef BRIDGE_MIC_EN
Bridge_Context * pBridgeMicCt;
uint16_t BridgeMic_SinkDataSet(void* Buf, uint16_t Len)
{
	uint16_t PutLen = 0;
	if(Buf == NULL || pBridgeMicCt == NULL || pBridgeMicCt->BridgeBuffer == NULL) return 0;
#ifdef BRIDGE_MIC_HWSRC_EN
	if(pBridgeMicCt->pBridgeSrcCt
			&& pBridgeMicCt->pBridgeSrcCt->InBuf
			&& pBridgeMicCt->pBridgeSrcCt->OutBuf
			&& pBridgeMicCt->FifoSampleRate != pBridgeMicCt->SinkSampleRate)
	{
		if(SRC_IsBlockDone(CFG_BRIDGE_HW_SRC_PORT))
		{
			PutLen = SRC_BlockApplyDone((SRC_MODULE)CFG_BRIDGE_HW_SRC_PORT, pBridgeMicCt->pBridgeSrcCt, (int16_t*)pBridgeMicCt->pBridgeSrcCt->OutBuf);
			if(MCUCircular_GetSpaceLen(&pBridgeMicCt->BridgeFifoCt) > PutLen * (PCM_BYTE(BRIDGE_MIC_WIDTH) * BRIDGE_MIC_CHN))
			{
				MCUCircular_PutData(&pBridgeMicCt->BridgeFifoCt, pBridgeMicCt->pBridgeSrcCt->OutBuf, PutLen * (PCM_BYTE(BRIDGE_MIC_WIDTH) * BRIDGE_MIC_CHN));
			}
		}

		SRC_BlockLenSet(CFG_BRIDGE_HW_SRC_PORT, Len);
		SRC_BlockApplyStart(CFG_BRIDGE_HW_SRC_PORT, pBridgeMicCt->pBridgeSrcCt, (int16_t*)Buf, Len);
	}
	else
#endif
	if(MCUCircular_GetSpaceLen(&pBridgeMicCt->BridgeFifoCt) > Len * (PCM_BYTE(BRIDGE_MIC_WIDTH) * BRIDGE_MIC_CHN))
	{
		MCUCircular_PutData(&pBridgeMicCt->BridgeFifoCt, Buf, Len * (PCM_BYTE(BRIDGE_MIC_WIDTH) * BRIDGE_MIC_CHN));
		PutLen = Len;
	}
	return PutLen;
}

uint16_t BridgeMic_SinkGetSpaceLen(void)
{
	if(pBridgeMicCt == NULL || pBridgeMicCt->BridgeBuffer == NULL) return 0;
#ifdef BRIDGE_MIC_HWSRC_EN
	if(pBridgeMicCt->SinkSampleRate != pBridgeMicCt->FifoSampleRate && pBridgeMicCt->pBridgeSrcCt != NULL)
	{
		return (uint64_t)pBridgeMicCt->SinkSampleRate * MCUCircular_GetSpaceLen(&pBridgeMicCt->BridgeFifoCt) /(pBridgeMicCt->FifoSampleRate * (PCM_BYTE(BRIDGE_MIC_WIDTH) * BRIDGE_MIC_CHN));
	}
	else
#endif
	{
		return MCUCircular_GetSpaceLen(&pBridgeMicCt->BridgeFifoCt) / (PCM_BYTE(BRIDGE_MIC_WIDTH) * BRIDGE_MIC_CHN);
	}
}

uint16_t BridgeMic_SouceDataGet(void* Buf, uint16_t Len)
{
	uint16_t OutLen = 0;

	if(Buf == NULL || pBridgeMicCt == NULL || pBridgeMicCt->BridgeBuffer == NULL) return 0;
#ifdef BRIDGE_MIC_HWSRC_EN
	uint16_t Length;
	uint16_t SampleLen;
	if(pBridgeMicCt->pBridgeSrcCt
			&& pBridgeMicCt->pBridgeSrcCt->InBuf
			&& pBridgeMicCt->pBridgeSrcCt->OutBuf
			&& pBridgeMicCt->FifoSampleRate != pBridgeMicCt->SourceSampleRate)
	{
		Length = MCUCircular_GetDataLen(&pBridgeMicCt->BridgeFifoCt);
		SampleLen = (uint64_t)Length * pBridgeMicCt->SourceSampleRate /(pBridgeMicCt->FifoSampleRate * (PCM_BYTE(BRIDGE_MIC_WIDTH) * BRIDGE_MIC_CHN));//SRCout Sample
		if(SampleLen > Len)
		{
			SampleLen = Len;
		}
		SampleLen = (uint64_t)SampleLen * pBridgeMicCt->FifoSampleRate / pBridgeMicCt->SourceSampleRate;//Tartet SRCIn Sample
		Length = SampleLen * (PCM_BYTE(BRIDGE_MIC_WIDTH) * BRIDGE_MIC_CHN);

		if(SRC_IsBlockDone(CFG_BRIDGE_HW_SRC_PORT))
		{
			OutLen = SRC_BlockApplyDone((SRC_MODULE)CFG_BRIDGE_HW_SRC_PORT, pBridgeMicCt->pBridgeSrcCt, (int16_t*)Buf);
		}
#if 0
		else if(AudioCoreSinkIsEnable(USB_AUDIO_SINK_NUM))//Mask //UsbAudioMic.AltSet
			DBG("HWSRCLostF\n");
#endif
		MCUCircular_GetData(&pBridgeMicCt->BridgeFifoCt, pBridgeMicCt->pBridgeSrcCt->InBuf, Length);
		SRC_BlockLenSet(CFG_BRIDGE_HW_SRC_PORT, SampleLen);
		SRC_BlockApplyStart(CFG_BRIDGE_HW_SRC_PORT, pBridgeMicCt->pBridgeSrcCt, (int16_t*)pBridgeMicCt->pBridgeSrcCt->InBuf, SampleLen);
	}
	else
#endif
	{
		OutLen = MCUCircular_GetData(&pBridgeMicCt->BridgeFifoCt, Buf, Len * (PCM_BYTE(BRIDGE_MIC_WIDTH) * BRIDGE_MIC_CHN)) / (PCM_BYTE(BRIDGE_MIC_WIDTH) * BRIDGE_MIC_CHN);
	}
	return OutLen;
}

uint16_t BridgeMic_SouceGetDataLen(void)
{
	if(pBridgeMicCt == NULL || pBridgeMicCt->BridgeBuffer == NULL) return 0;
#ifdef BRIDGE_MIC_HWSRC_EN
	if(pBridgeMicCt->SourceSampleRate != pBridgeMicCt->FifoSampleRate && pBridgeMicCt->pBridgeSrcCt != NULL)
	{
		return (uint64_t)pBridgeMicCt->SourceSampleRate * MCUCircular_GetDataLen(&pBridgeMicCt->BridgeFifoCt) /(pBridgeMicCt->FifoSampleRate * (PCM_BYTE(BRIDGE_MIC_WIDTH) * BRIDGE_MIC_CHN));
	}
	else
#endif
	{
		return  MCUCircular_GetDataLen(&pBridgeMicCt->BridgeFifoCt) /((PCM_BYTE(BRIDGE_MIC_WIDTH) * BRIDGE_MIC_CHN));
	}
}
#ifdef BRIDGE_MIC_HWSRC_EN
//输入输出采样率不许相同
void BridgeMicSrcInit(void)
{
    //src set
	if(pBridgeMicCt
			&& pBridgeMicCt->pBridgeSrcCt
			&& pBridgeMicCt->pBridgeSrcCt->InBuf
			&& pBridgeMicCt->pBridgeSrcCt->OutBuf)
	{
		pBridgeMicCt->pBridgeSrcCt->Mode = MODE_BLOCK;
#if BRIDGE_MIC_WIDTH == WIDTH_16BIT
		pBridgeMicCt->pBridgeSrcCt->Width = SRC_16BIT;
#elif (BRIDGE_MIC_WIDTH == WIDTH_L24BIT)
		pBridgeMicCt->pBridgeSrcCt->Width = SRC_24BIT;
#else
	#error Conflict: BridgeMicSrc Width error!!!
#endif
		pBridgeMicCt->pBridgeSrcCt->NumChannels = BRIDGE_MIC_CHN;
		pBridgeMicCt->pBridgeSrcCt->BlockSamples = pBridgeMicCt->pBridgeSrcCt->InBufLen;
	if(pBridgeMicCt->SinkSampleRate != pBridgeMicCt->FifoSampleRate)
		SRC_BlockInit(CFG_BRIDGE_HW_SRC_PORT, pBridgeMicCt->pBridgeSrcCt, pBridgeMicCt->SinkSampleRate, pBridgeMicCt->FifoSampleRate);
	else if(pBridgeMicCt->FifoSampleRate != pBridgeMicCt->SinkSampleRate)
		SRC_BlockInit(CFG_BRIDGE_HW_SRC_PORT, pBridgeMicCt->pBridgeSrcCt, pBridgeMicCt->FifoSampleRate, pBridgeMicCt->SinkSampleRate);
	}
}
#endif

bool BridgeMicInit(void)
{
	AudioCoreIO AudioIOSet;
	uint16_t SinkFrameUs, SourceFrameUs, FifoDepthUs;

	if(((AudioEffect_Parambin_GetSinkBuffer(&EffectParambin, BRIDGE_MIC_SINK) && AudioEffect_Parambin_GetSourceBuffer(&EffectParambin2, BRIDGE_MIC_SOURCE)))
			|| (AudioEffect_Parambin_GetSinkBuffer(&EffectParambin2, BRIDGE_MIC_SINK) && (AudioEffect_Parambin_GetSourceBuffer(&EffectParambin, BRIDGE_MIC_SOURCE))))
	{

		pBridgeMicCt = (Bridge_Context *)osPortMalloc(sizeof(Bridge_Context));
		if(pBridgeMicCt == NULL)
		{
			APP_DBG("Bridge_Context Error!\n");
			return FALSE;
		}
		memset(pBridgeMicCt, 0, sizeof(Bridge_Context));
		pBridgeMicCt->SinkFlowNet = AudioCoreGetNetBySinkChannelNUM(BRIDGE_MIC_SINK);//尚未挂载，需从图上获取配置
		pBridgeMicCt->SinkSampleRate = AudioCoreMixSampleRateGet(pBridgeMicCt->SinkFlowNet);
		SinkFrameUs = AudioCoreMixStepUsGet(pBridgeMicCt->SinkFlowNet);
		pBridgeMicCt->SourceFlowNet = AudioCoreGetNetBySourceChannelNUM(BRIDGE_MIC_SOURCE);//尚未挂载，需从图上获取配置
		pBridgeMicCt->SourceSampleRate = AudioCoreMixSampleRateGet(pBridgeMicCt->SourceFlowNet);
		SourceFrameUs = AudioCoreMixStepUsGet(pBridgeMicCt->SourceFlowNet);
		FifoDepthUs = SourceFrameUs + SinkFrameUs + BRIDGE_FIFO_JITTER;//深度控制，可加长
		if(pBridgeMicCt->SinkSampleRate != pBridgeMicCt->SourceSampleRate)
		{
	#ifdef BRIDGE_MIC_HWSRC_EN
			uint32_t SinkFrameSize = AudioCoreFrameSizeGet(pBridgeMicCt->SinkFlowNet);
			uint32_t SourceFrameSize = AudioCoreFrameSizeGet(pBridgeMicCt->SourceFlowNet);
			pBridgeMicCt->pBridgeSrcCt = (SRC_Context *)osPortMalloc(sizeof(SRC_Context));
			if(pBridgeMicCt->pBridgeSrcCt == NULL)
			{
				APP_DBG("pBridgeMicCt->BridgeSrcCt Error!\n");
				return FALSE;
			}
			memset(pBridgeMicCt->pBridgeSrcCt, 0, sizeof(SRC_Context));
			/**FIFO 衔接 HWSRC 常规配在set Sink,如时隙过短会产生丢帧；如配置GetSource要满足等长转采,否则帧间Out波动不好管理**/
			if((SourceFrameUs > SinkFrameUs) && (((uint64_t)SourceFrameSize * pBridgeMicCt->SinkSampleRate) % pBridgeMicCt->SourceSampleRate) == 0)
			{
				pBridgeMicCt->FifoSampleRate = pBridgeMicCt->SinkSampleRate;
				pBridgeMicCt->pBridgeSrcCt->InBufLen = ((uint64_t)SourceFrameSize * pBridgeMicCt->SinkSampleRate) / pBridgeMicCt->SourceSampleRate;
				pBridgeMicCt->pBridgeSrcCt->OutBufLen = SinkFrameSize;
			}
			else//Sink Set @HWSRC 输出留些余量
			{
				pBridgeMicCt->FifoSampleRate = pBridgeMicCt->SourceSampleRate;
				pBridgeMicCt->pBridgeSrcCt->InBufLen = ((uint64_t)SinkFrameSize * pBridgeMicCt->SinkSampleRate) / pBridgeMicCt->SourceSampleRate;
				pBridgeMicCt->pBridgeSrcCt->OutBufLen = ((uint64_t)SinkFrameSize * pBridgeMicCt->SourceSampleRate * 12) / (pBridgeMicCt->SinkSampleRate * 10);
			}
			pBridgeMicCt->pBridgeSrcCt->InBuf = (uint32_t *)osPortMalloc(pBridgeMicCt->pBridgeSrcCt->InBufLen * (PCM_BYTE(BRIDGE_MIC_WIDTH) * BRIDGE_MIC_CHN));
			if(pBridgeMicCt->pBridgeSrcCt->InBuf == NULL)
			{
				APP_DBG("pBridgeMicCt->pBridgeSrcCt->InBuf Error!\n");
				return FALSE;
			}
			pBridgeMicCt->pBridgeSrcCt->OutBuf = (uint32_t *)osPortMalloc(pBridgeMicCt->pBridgeSrcCt->OutBufLen * (PCM_BYTE(BRIDGE_MIC_WIDTH) * BRIDGE_MIC_CHN));
			if(pBridgeMicCt->pBridgeSrcCt->OutBuf == NULL)
			{
				APP_DBG("pBridgeMicCt->pBridgeSrcCt->OutBuf Error!\n");
				return FALSE;
			}
			BridgeMicSrcInit();
	#else
			//此处管理软件SRC配在Source或sink 常规短时隙延时要求高,转采样配置长时隙方向。
			if(SourceFrameUs > SinkFrameUs)
			{
				pBridgeMicCt->FifoSampleRate = pBridgeMicCt->SinkSampleRate;
			}
			else if(SourceFrameUs < SinkFrameUs)
			{
				pBridgeMicCt->FifoSampleRate = pBridgeMicCt->SourceSampleRate;
			}
			else //低RAM先于延时控制时，可去除上述条件
			{
				pBridgeMicCt->FifoSampleRate = MIN(pBridgeMicCt->SinkSampleRate, pBridgeMicCt->SourceSampleRate);
			}
	#endif
		}
		else
		{
			pBridgeMicCt->FifoSampleRate = pBridgeMicCt->SinkSampleRate;
		}

		pBridgeMicCt->FifoSampleLen = (uint64_t)pBridgeMicCt->FifoSampleRate * (FifoDepthUs) / 1000000;
		pBridgeMicCt->BridgeBuffer = (uint32_t *)osPortMalloc(pBridgeMicCt->FifoSampleLen * (PCM_BYTE(BRIDGE_MIC_WIDTH) * BRIDGE_MIC_CHN));
		if(pBridgeMicCt->BridgeBuffer == NULL)
		{
			APP_DBG("pBridgeMicCt->BridgeBuffer Error!\n");
			return FALSE;
		}

		//for 双图 sink和 source挂接
		MCUCircular_Config(&pBridgeMicCt->BridgeFifoCt, pBridgeMicCt->BridgeBuffer, pBridgeMicCt->FifoSampleLen * (PCM_BYTE(BRIDGE_MIC_WIDTH) * BRIDGE_MIC_CHN));

		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
		AudioIOSet.PcmWidth	= BRIDGE_MIC_WIDTH;
		if(!AudioCoreSinkIsInit(BRIDGE_MIC_SINK))
		{
			APP_DBG("BridgeMic SinkInit\n");
			AudioIOSet.Sync = FALSE;
			AudioIOSet.Channels = BRIDGE_MIC_CHN;
			AudioIOSet.Net = pBridgeMicCt->SinkFlowNet;
			AudioIOSet.Depth = (uint64_t)pBridgeMicCt->SinkSampleRate * (FifoDepthUs) / 1000000;
	#ifndef BRIDGE_MIC_HWSRC_EN
			if(pBridgeMicCt->SinkSampleRate != pBridgeMicCt->FifoSampleRate)
			{
				AudioIOSet.Adapt = SRC_ONLY;
				AudioIOSet.SampleRate = pBridgeMicCt->FifoSampleRate;
			}
			else
	#endif
			{
				AudioIOSet.Adapt = STD;
			}
			AudioIOSet.DataIOFunc = BridgeMic_SinkDataSet;
			AudioIOSet.LenGetFunc = BridgeMic_SinkGetSpaceLen;

			if(!AudioCoreSinkInit(&AudioIOSet, BRIDGE_MIC_SINK))
			{
				DBG(" SINK init error\n");
				return FALSE;
			}
			AudioCoreSinkEnable(BRIDGE_MIC_SINK);
		}

		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
		AudioIOSet.PcmWidth	= BRIDGE_MIC_WIDTH;
		if(!AudioCoreSourceIsInit(BRIDGE_MIC_SOURCE))
		{
			APP_DBG("BridgeMic SourceInit\n");

			AudioIOSet.Sync = FALSE;
			AudioIOSet.Channels = BRIDGE_MIC_CHN;
			AudioIOSet.Net = pBridgeMicCt->SourceFlowNet;
			AudioIOSet.Depth = (uint64_t)pBridgeMicCt->SourceSampleRate * (FifoDepthUs) / 1000000;
	#ifndef BRIDGE_MIC_HWSRC_EN
			if(pBridgeMicCt->FifoSampleRate != pBridgeMicCt->SourceSampleRate)
			{
				AudioIOSet.Adapt = SRC_ONLY;
				AudioIOSet.SampleRate = pBridgeMicCt->FifoSampleRate;
			}
			else
	#endif
			{
				AudioIOSet.Adapt = STD;
			}
			AudioIOSet.DataIOFunc = BridgeMic_SouceDataGet;
			AudioIOSet.LenGetFunc = BridgeMic_SouceGetDataLen;

			if(!AudioCoreSourceInit(&AudioIOSet, BRIDGE_MIC_SOURCE))
			{
				DBG("SOURCE init error\n");
				return FALSE;
			}
			AudioCoreSourceEnable(BRIDGE_MIC_SOURCE);
		}
	}
	return TRUE;
}

void BridgeMicDeinit(void)
{
	if(pBridgeMicCt != NULL)
	{
		if(AudioCoreSourceIsInit(BRIDGE_MIC_SOURCE))
		{
			AudioCoreSourceDisable(BRIDGE_MIC_SOURCE);
			AudioCoreSourceDeinit(BRIDGE_MIC_SOURCE);

		}
		if(AudioCoreSinkIsInit(BRIDGE_MIC_SINK))
		{
			AudioCoreSinkDisable(BRIDGE_MIC_SINK);
			AudioCoreSinkDeinit(BRIDGE_MIC_SINK);
		}
	#ifdef BRIDGE_MIC_HWSRC_EN
		if(pBridgeMicCt->pBridgeSrcCt != NULL)
		{
			if(pBridgeMicCt->pBridgeSrcCt->InBuf != NULL)
			{
				if(pBridgeMicCt->pBridgeSrcCt->OutBuf)
				{
					Reset_Function2Reset((SRC0_FUNC_SEPA) << CFG_BRIDGE_HW_SRC_PORT);
					Reset_Register2Reset(SRC0_REG_SEPA << CFG_BRIDGE_HW_SRC_PORT);
					osPortFree(pBridgeMicCt->pBridgeSrcCt->OutBuf);
					pBridgeMicCt->pBridgeSrcCt->OutBuf = NULL;
				}
				osPortFree(pBridgeMicCt->pBridgeSrcCt->InBuf);
				pBridgeMicCt->pBridgeSrcCt->InBuf = NULL;
			}
			osPortFree(pBridgeMicCt->pBridgeSrcCt);
			pBridgeMicCt->pBridgeSrcCt = NULL;
		}
	#endif
		if(pBridgeMicCt->BridgeBuffer != NULL)
		{
			osPortFree(pBridgeMicCt->BridgeBuffer);
			pBridgeMicCt->BridgeBuffer = NULL;
		}
		osPortFree(pBridgeMicCt);
		pBridgeMicCt = NULL;
	}
}

#endif//#ifdef BRIDGE_MIC_EN
