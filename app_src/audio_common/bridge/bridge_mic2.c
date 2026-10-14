/*
 * bridge_mic2.c
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

#ifdef BRIDGE_MIC2_EN
Bridge_Context * pBridgeMic2Ct;
uint16_t BridgeMic2_SinkDataSet(void* Buf, uint16_t Len)
{
	uint16_t PutLen = 0;
	if(Buf == NULL || pBridgeMic2Ct == NULL || pBridgeMic2Ct->BridgeBuffer == NULL) return 0;
#ifdef BRIDGE_MIC2_HWSRC_EN
	if(pBridgeMic2Ct->pBridgeSrcCt
			&& pBridgeMic2Ct->pBridgeSrcCt->InBuf
			&& pBridgeMic2Ct->pBridgeSrcCt->OutBuf
			&& pBridgeMic2Ct->FifoSampleRate != pBridgeMic2Ct->SinkSampleRate)
	{
		if(SRC_IsBlockDone(CFG_BRIDGE_HW_SRC_PORT))
		{
			PutLen = SRC_BlockApplyDone((SRC_MODULE)CFG_BRIDGE_HW_SRC_PORT, pBridgeMic2Ct->pBridgeSrcCt, (int16_t*)pBridgeMic2Ct->pBridgeSrcCt->OutBuf);
			if(MCUCircular_GetSpaceLen(&pBridgeMic2Ct->BridgeFifoCt) > PutLen * (PCM_BYTE(BRIDGE_MIC2_WIDTH) * BRIDGE_MIC2_CHN))
			{
				MCUCircular_PutData(&pBridgeMic2Ct->BridgeFifoCt, pBridgeMic2Ct->pBridgeSrcCt->OutBuf, PutLen * (PCM_BYTE(BRIDGE_MIC2_WIDTH) * BRIDGE_MIC2_CHN));
			}
		}

		SRC_BlockLenSet(CFG_BRIDGE_HW_SRC_PORT, Len);
		SRC_BlockApplyStart(CFG_BRIDGE_HW_SRC_PORT, pBridgeMic2Ct->pBridgeSrcCt, (int16_t*)Buf, Len);
	}
	else
#endif
	if(MCUCircular_GetSpaceLen(&pBridgeMic2Ct->BridgeFifoCt) > Len * (PCM_BYTE(BRIDGE_MIC2_WIDTH) * BRIDGE_MIC2_CHN))
	{
		MCUCircular_PutData(&pBridgeMic2Ct->BridgeFifoCt, Buf, Len * (PCM_BYTE(BRIDGE_MIC2_WIDTH) * BRIDGE_MIC2_CHN));
		PutLen = Len;
	}
	return PutLen;
}

uint16_t BridgeMic2_SinkGetSpaceLen(void)
{
	if(pBridgeMic2Ct == NULL || pBridgeMic2Ct->BridgeBuffer == NULL) return 0;
#ifdef BRIDGE_MIC2_HWSRC_EN
	if(pBridgeMic2Ct->SinkSampleRate != pBridgeMic2Ct->FifoSampleRate && pBridgeMic2Ct->pBridgeSrcCt != NULL)
	{
		return (uint64_t)pBridgeMic2Ct->SinkSampleRate * MCUCircular_GetSpaceLen(&pBridgeMic2Ct->BridgeFifoCt) /(pBridgeMic2Ct->FifoSampleRate * (PCM_BYTE(BRIDGE_MIC2_WIDTH) * BRIDGE_MIC2_CHN));
	}
	else
#endif
	{
		return MCUCircular_GetSpaceLen(&pBridgeMic2Ct->BridgeFifoCt) / (PCM_BYTE(BRIDGE_MIC2_WIDTH) * BRIDGE_MIC2_CHN);
	}
}

uint16_t BridgeMic2_SouceDataGet(void* Buf, uint16_t Len)
{
	uint16_t OutLen = 0;

	if(Buf == NULL || pBridgeMic2Ct == NULL || pBridgeMic2Ct->BridgeBuffer == NULL) return 0;
#ifdef BRIDGE_MIC2_HWSRC_EN
	uint16_t Length;
	uint16_t SampleLen;
	if(pBridgeMic2Ct->pBridgeSrcCt
			&& pBridgeMic2Ct->pBridgeSrcCt->InBuf
			&& pBridgeMic2Ct->pBridgeSrcCt->OutBuf
			&& pBridgeMic2Ct->FifoSampleRate != pBridgeMic2Ct->SourceSampleRate)
	{
		Length = MCUCircular_GetDataLen(&pBridgeMic2Ct->BridgeFifoCt);
		SampleLen = (uint64_t)Length * pBridgeMic2Ct->SourceSampleRate /(pBridgeMic2Ct->FifoSampleRate * (PCM_BYTE(BRIDGE_MIC2_WIDTH) * BRIDGE_MIC2_CHN));//SRCout Sample
		if(SampleLen > Len)
		{
			SampleLen = Len;
		}
		SampleLen = (uint64_t)SampleLen * pBridgeMic2Ct->FifoSampleRate / pBridgeMic2Ct->SourceSampleRate;//Tartet SRCIn Sample
		Length = SampleLen * (PCM_BYTE(BRIDGE_MIC2_WIDTH) * BRIDGE_MIC2_CHN);

		if(SRC_IsBlockDone(CFG_BRIDGE_HW_SRC_PORT))
		{
			OutLen = SRC_BlockApplyDone((SRC_MODULE)CFG_BRIDGE_HW_SRC_PORT, pBridgeMic2Ct->pBridgeSrcCt, (int16_t*)Buf);
		}
#if 0
		else if(AudioCoreSinkIsEnable(USB_AUDIO_SINK_NUM))//Mask //UsbAudioMic.AltSet
			DBG("HWSRCLostF\n");
#endif
		MCUCircular_GetData(&pBridgeMic2Ct->BridgeFifoCt, pBridgeMic2Ct->pBridgeSrcCt->InBuf, Length);
		SRC_BlockLenSet(CFG_BRIDGE_HW_SRC_PORT, SampleLen);
		SRC_BlockApplyStart(CFG_BRIDGE_HW_SRC_PORT, pBridgeMic2Ct->pBridgeSrcCt, (int16_t*)pBridgeMic2Ct->pBridgeSrcCt->InBuf, SampleLen);
	}
	else
#endif
	{
		OutLen = MCUCircular_GetData(&pBridgeMic2Ct->BridgeFifoCt, Buf, Len * (PCM_BYTE(BRIDGE_MIC2_WIDTH) * BRIDGE_MIC2_CHN)) / (PCM_BYTE(BRIDGE_MIC2_WIDTH) * BRIDGE_MIC2_CHN);
	}
	return OutLen;
}

uint16_t BridgeMic2_SouceGetDataLen(void)
{
	if(pBridgeMic2Ct == NULL || pBridgeMic2Ct->BridgeBuffer == NULL) return 0;
#ifdef BRIDGE_MIC2_HWSRC_EN
	if(pBridgeMic2Ct->SourceSampleRate != pBridgeMic2Ct->FifoSampleRate && pBridgeMic2Ct->pBridgeSrcCt != NULL)
	{
		return (uint64_t)pBridgeMic2Ct->SourceSampleRate * MCUCircular_GetDataLen(&pBridgeMic2Ct->BridgeFifoCt) /(pBridgeMic2Ct->FifoSampleRate * (PCM_BYTE(BRIDGE_MIC2_WIDTH) * BRIDGE_MIC2_CHN));
	}
	else
#endif
	{
		return  MCUCircular_GetDataLen(&pBridgeMic2Ct->BridgeFifoCt) /((PCM_BYTE(BRIDGE_MIC2_WIDTH) * BRIDGE_MIC2_CHN));
	}
}
#ifdef BRIDGE_MIC2_HWSRC_EN
//输入输出采样率不许相同
void BridgeMic2SrcInit(void)
{
    //src set
	if(pBridgeMic2Ct
			&& pBridgeMic2Ct->pBridgeSrcCt
			&& pBridgeMic2Ct->pBridgeSrcCt->InBuf
			&& pBridgeMic2Ct->pBridgeSrcCt->OutBuf)
	{
		pBridgeMic2Ct->pBridgeSrcCt->Mode = MODE_BLOCK;
#if BRIDGE_MIC2_WIDTH == WIDTH_16BIT
		pBridgeMic2Ct->pBridgeSrcCt->Width = SRC_16BIT;
#elif (BRIDGE_MIC2_WIDTH == WIDTH_L24BIT)
		pBridgeMic2Ct->pBridgeSrcCt->Width = SRC_24BIT;
#else
	#error Conflict: BridgeMic2Src Width error!!!
#endif
		pBridgeMic2Ct->pBridgeSrcCt->NumChannels = BRIDGE_MIC2_CHN;
		pBridgeMic2Ct->pBridgeSrcCt->BlockSamples = pBridgeMic2Ct->pBridgeSrcCt->InBufLen;
	if(pBridgeMic2Ct->SinkSampleRate != pBridgeMic2Ct->FifoSampleRate)
		SRC_BlockInit(CFG_BRIDGE_HW_SRC_PORT, pBridgeMic2Ct->pBridgeSrcCt, pBridgeMic2Ct->SinkSampleRate, pBridgeMic2Ct->FifoSampleRate);
	else if(pBridgeMic2Ct->FifoSampleRate != pBridgeMic2Ct->SinkSampleRate)
		SRC_BlockInit(CFG_BRIDGE_HW_SRC_PORT, pBridgeMic2Ct->pBridgeSrcCt, pBridgeMic2Ct->FifoSampleRate, pBridgeMic2Ct->SinkSampleRate);
	}
}
#endif

bool BridgeMic2Init(void)
{
	AudioCoreIO AudioIOSet;
	uint16_t SinkFrameUs, SourceFrameUs, FifoDepthUs;
	uint32_t SinkFrameSize,SourceFrameSize;

	if((AudioEffect_Parambin_GetSinkBuffer(&EffectParambin, BRIDGE_MIC2_SINK) && AudioEffect_Parambin_GetSourceBuffer(&EffectParambin2, BRIDGE_MIC2_SOURCE))
			|| (AudioEffect_Parambin_GetSinkBuffer(&EffectParambin2, BRIDGE_MIC2_SINK) && (AudioEffect_Parambin_GetSourceBuffer(&EffectParambin, BRIDGE_MIC2_SOURCE))))
	{

		pBridgeMic2Ct = (Bridge_Context *)osPortMalloc(sizeof(Bridge_Context));
		if(pBridgeMic2Ct == NULL)
		{
			APP_DBG("Bridge_Context Error!\n");
			return FALSE;
		}
		memset(pBridgeMic2Ct, 0, sizeof(Bridge_Context));
		pBridgeMic2Ct->SinkFlowNet = AudioCoreGetNetBySinkChannelNUM(BRIDGE_MIC2_SINK);//尚未挂载，需从图上获取配置
		pBridgeMic2Ct->SinkSampleRate = AudioCoreMixSampleRateGet(pBridgeMic2Ct->SinkFlowNet);
		SinkFrameSize = AudioCoreFrameSizeGet(pBridgeMic2Ct->SinkFlowNet);
		SinkFrameUs = AudioCoreMixStepUsGet(pBridgeMic2Ct->SinkFlowNet);
		pBridgeMic2Ct->SourceFlowNet = AudioCoreGetNetBySourceChannelNUM(BRIDGE_MIC2_SOURCE);//尚未挂载，需从图上获取配置
		pBridgeMic2Ct->SourceSampleRate = AudioCoreMixSampleRateGet(pBridgeMic2Ct->SourceFlowNet);
		SourceFrameSize = AudioCoreFrameSizeGet(pBridgeMic2Ct->SourceFlowNet);
		SourceFrameUs = AudioCoreMixStepUsGet(pBridgeMic2Ct->SourceFlowNet);
		FifoDepthUs = SourceFrameUs + SinkFrameUs + BRIDGE_FIFO_JITTER;//深度控制，可加长
		if(pBridgeMic2Ct->SinkSampleRate != pBridgeMic2Ct->SourceSampleRate)
		{
	#ifdef BRIDGE_MIC2_HWSRC_EN
			pBridgeMic2Ct->pBridgeSrcCt = (SRC_Context *)osPortMalloc(sizeof(SRC_Context));
			if(pBridgeMic2Ct->pBridgeSrcCt == NULL)
			{
				APP_DBG("pBridgeMic2Ct->BridgeSrcCt Error!\n");
				return FALSE;
			}
			memset(pBridgeMic2Ct->pBridgeSrcCt, 0, sizeof(SRC_Context));
			/**FIFO 衔接 HWSRC 常规配在set Sink,如时隙过短会产生丢帧；如配置GetSource要满足等长转采,否则帧间Out波动不好管理**/
			if((SourceFrameUs > SinkFrameUs) && (((uint64_t)SourceFrameSize * pBridgeMic2Ct->SinkSampleRate) % pBridgeMic2Ct->SourceSampleRate) == 0)
			{
				pBridgeMic2Ct->FifoSampleRate = pBridgeMic2Ct->SinkSampleRate;
				pBridgeMic2Ct->pBridgeSrcCt->InBufLen = ((uint64_t)SourceFrameSize * pBridgeMic2Ct->SinkSampleRate) / pBridgeMic2Ct->SourceSampleRate;
				pBridgeMic2Ct->pBridgeSrcCt->OutBufLen = SinkFrameSize;
			}
			else//Sink Set @HWSRC 输出留些余量
			{
				pBridgeMic2Ct->FifoSampleRate = pBridgeMic2Ct->SourceSampleRate;
				pBridgeMic2Ct->pBridgeSrcCt->InBufLen = ((uint64_t)SinkFrameSize * pBridgeMic2Ct->SinkSampleRate) / pBridgeMic2Ct->SourceSampleRate;
				pBridgeMic2Ct->pBridgeSrcCt->OutBufLen = ((uint64_t)SinkFrameSize * pBridgeMic2Ct->SourceSampleRate * 12) / (pBridgeMic2Ct->SinkSampleRate * 10);
			}
			pBridgeMic2Ct->pBridgeSrcCt->InBuf = (uint32_t *)osPortMalloc(pBridgeMic2Ct->pBridgeSrcCt->InBufLen * (PCM_BYTE(BRIDGE_MIC2_WIDTH) * BRIDGE_MIC2_CHN));
			if(pBridgeMic2Ct->pBridgeSrcCt->InBuf == NULL)
			{
				APP_DBG("pBridgeMic2Ct->pBridgeSrcCt->InBuf Error!\n");
				return FALSE;
			}
			pBridgeMic2Ct->pBridgeSrcCt->OutBuf = (uint32_t *)osPortMalloc(pBridgeMic2Ct->pBridgeSrcCt->OutBufLen * (PCM_BYTE(BRIDGE_MIC2_WIDTH) * BRIDGE_MIC2_CHN));
			if(pBridgeMic2Ct->pBridgeSrcCt->OutBuf == NULL)
			{
				APP_DBG("pBridgeMic2Ct->pBridgeSrcCt->OutBuf Error!\n");
				return FALSE;
			}
			BridgeMic2SrcInit();
	#else
			//此处管理软件SRC配在Source或sink 常规短时隙延时要求高,转采样配置长时隙方向。
			if(SourceFrameUs > SinkFrameUs)
			{
				pBridgeMic2Ct->FifoSampleRate = pBridgeMic2Ct->SinkSampleRate;
			}
			else if(SourceFrameUs < SinkFrameUs)
			{
				pBridgeMic2Ct->FifoSampleRate = pBridgeMic2Ct->SourceSampleRate;
			}
			else //低RAM先于延时控制时，可去除上述条件
			{
				pBridgeMic2Ct->FifoSampleRate = MIN(pBridgeMic2Ct->SinkSampleRate, pBridgeMic2Ct->SourceSampleRate);
			}
	#endif
		}
		else
		{
			pBridgeMic2Ct->FifoSampleRate = pBridgeMic2Ct->SinkSampleRate;
		}

		pBridgeMic2Ct->FifoSampleLen = (uint64_t)pBridgeMic2Ct->FifoSampleRate * (FifoDepthUs) / 1000000;
		pBridgeMic2Ct->BridgeBuffer = (uint32_t *)osPortMalloc(pBridgeMic2Ct->FifoSampleLen * (PCM_BYTE(BRIDGE_MIC2_WIDTH) * BRIDGE_MIC2_CHN));
		if(pBridgeMic2Ct->BridgeBuffer == NULL)
		{
			APP_DBG("pBridgeMic2Ct->BridgeBuffer Error!\n");
			return FALSE;
		}

		//for 双图 sink和 source挂接
		MCUCircular_Config(&pBridgeMic2Ct->BridgeFifoCt, pBridgeMic2Ct->BridgeBuffer, pBridgeMic2Ct->FifoSampleLen * (PCM_BYTE(BRIDGE_MIC2_WIDTH) * BRIDGE_MIC2_CHN));

		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
		AudioIOSet.PcmWidth	= BRIDGE_MIC2_WIDTH;
		if(!AudioCoreSinkIsInit(BRIDGE_MIC2_SINK))
		{
			APP_DBG("BridgeMic2 SinkInit\n");
			AudioIOSet.Sync = FALSE;
			AudioIOSet.Channels = BRIDGE_MIC2_CHN;
			AudioIOSet.Net = pBridgeMic2Ct->SinkFlowNet;
			AudioIOSet.Depth = (uint64_t)pBridgeMic2Ct->SinkSampleRate * (FifoDepthUs) / 1000000;
	#ifndef BRIDGE_MIC2_HWSRC_EN
			if(pBridgeMic2Ct->SinkSampleRate != pBridgeMic2Ct->FifoSampleRate)
			{
				AudioIOSet.Adapt = SRC_ONLY;
				AudioIOSet.SampleRate = pBridgeMic2Ct->FifoSampleRate;
			}
			else
	#endif
			{
				AudioIOSet.Adapt = STD;
			}
			AudioIOSet.DataIOFunc = BridgeMic2_SinkDataSet;
			AudioIOSet.LenGetFunc = BridgeMic2_SinkGetSpaceLen;

			if(!AudioCoreSinkInit(&AudioIOSet, BRIDGE_MIC2_SINK))
			{
				DBG(" SINK init error\n");
				return FALSE;
			}
			AudioCoreSinkEnable(BRIDGE_MIC2_SINK);
		}

		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
		AudioIOSet.PcmWidth	= BRIDGE_MIC2_WIDTH;
		if(!AudioCoreSourceIsInit(BRIDGE_MIC2_SOURCE))
		{
			APP_DBG("BridgeMic2 SourceInit\n");

			AudioIOSet.Sync = FALSE;
			AudioIOSet.Channels = BRIDGE_MIC2_CHN;
			AudioIOSet.Net = pBridgeMic2Ct->SourceFlowNet;
			AudioIOSet.Depth = (uint64_t)pBridgeMic2Ct->SourceSampleRate * (FifoDepthUs) / 1000000;
	#ifndef BRIDGE_MIC2_HWSRC_EN
			if(pBridgeMic2Ct->FifoSampleRate != pBridgeMic2Ct->SourceSampleRate)
			{
				AudioIOSet.Adapt = SRC_ONLY;
				AudioIOSet.SampleRate = pBridgeMic2Ct->FifoSampleRate;
			}
			else
	#endif
			{
				AudioIOSet.Adapt = STD;
			}
			AudioIOSet.DataIOFunc = BridgeMic2_SouceDataGet;
			AudioIOSet.LenGetFunc = BridgeMic2_SouceGetDataLen;

			if(!AudioCoreSourceInit(&AudioIOSet, BRIDGE_MIC2_SOURCE))
			{
				DBG("SOURCE init error\n");
				return FALSE;
			}
			AudioCoreSourceEnable(BRIDGE_MIC2_SOURCE);
		}
	}
	return TRUE;
}

void BridgeMic2Deinit(void)
{
	if(pBridgeMic2Ct != NULL)
	{
		if(AudioCoreSourceIsInit(BRIDGE_MIC2_SOURCE))
		{
			AudioCoreSourceDisable(BRIDGE_MIC2_SOURCE);
			AudioCoreSourceDeinit(BRIDGE_MIC2_SOURCE);

		}
		if(AudioCoreSinkIsInit(BRIDGE_MIC2_SINK))
		{
			AudioCoreSinkDisable(BRIDGE_MIC2_SINK);
			AudioCoreSinkDeinit(BRIDGE_MIC2_SINK);
		}
	#ifdef BRIDGE_MIC2_HWSRC_EN
		if(pBridgeMic2Ct->pBridgeSrcCt != NULL)
		{
			if(pBridgeMic2Ct->pBridgeSrcCt->InBuf != NULL)
			{
				if(pBridgeMic2Ct->pBridgeSrcCt->OutBuf)
				{
					Reset_Function2Reset((SRC0_FUNC_SEPA) << CFG_BRIDGE_HW_SRC_PORT);
					Reset_Register2Reset(SRC0_REG_SEPA << CFG_BRIDGE_HW_SRC_PORT);
					osPortFree(pBridgeMic2Ct->pBridgeSrcCt->OutBuf);
					pBridgeMic2Ct->pBridgeSrcCt->OutBuf = NULL;
				}
				osPortFree(pBridgeMic2Ct->pBridgeSrcCt->InBuf);
				pBridgeMic2Ct->pBridgeSrcCt->InBuf = NULL;
			}
			osPortFree(pBridgeMic2Ct->pBridgeSrcCt);
			pBridgeMic2Ct->pBridgeSrcCt = NULL;
		}
	#endif
		if(pBridgeMic2Ct->BridgeBuffer != NULL)
		{
			osPortFree(pBridgeMic2Ct->BridgeBuffer);
			pBridgeMic2Ct->BridgeBuffer = NULL;
		}
		osPortFree(pBridgeMic2Ct);
		pBridgeMic2Ct = NULL;
	}
}

#endif//#ifdef BRIDGE_MIC2_EN
