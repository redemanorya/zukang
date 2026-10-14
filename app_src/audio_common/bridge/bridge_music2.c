/*
 * bridge_music2.c
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

#ifdef BRIDGE_MUSIC2_EN
Bridge_Context * pBridgeMusic2Ct;

uint16_t BridgeMusic2_SinkDataSet(void* Buf, uint16_t Len)
{
	uint16_t PutLen = 0;
	if(Buf == NULL || pBridgeMusic2Ct == NULL || pBridgeMusic2Ct->BridgeBuffer == NULL) return 0;
#ifdef BRIDGE_MUSIC2_HWSRC_EN
	if(pBridgeMusic2Ct->pBridgeSrcCt
			&& pBridgeMusic2Ct->pBridgeSrcCt->InBuf
			&& pBridgeMusic2Ct->pBridgeSrcCt->OutBuf
			&& pBridgeMusic2Ct->FifoSampleRate != pBridgeMusic2Ct->SinkSampleRate)
	{
		if(SRC_IsBlockDone(CFG_BRIDGE_HW_SRC_PORT))
		{
			PutLen = SRC_BlockApplyDone((SRC_MODULE)CFG_BRIDGE_HW_SRC_PORT, pBridgeMusic2Ct->pBridgeSrcCt, (int16_t*)pBridgeMusic2Ct->pBridgeSrcCt->OutBuf);
			if(MCUCircular_GetSpaceLen(&pBridgeMusic2Ct->BridgeFifoCt) > PutLen * (PCM_BYTE(BRIDGE_MUSIC2_WIDTH) * BRIDGE_MUSIC2_CHN))
			{
				MCUCircular_PutData(&pBridgeMusic2Ct->BridgeFifoCt, pBridgeMusic2Ct->pBridgeSrcCt->OutBuf, PutLen * (PCM_BYTE(BRIDGE_MUSIC2_WIDTH) * BRIDGE_MUSIC2_CHN));
			}
		}

		SRC_BlockLenSet(CFG_BRIDGE_HW_SRC_PORT, Len);
		SRC_BlockApplyStart(CFG_BRIDGE_HW_SRC_PORT, pBridgeMusic2Ct->pBridgeSrcCt, (int16_t*)Buf, Len);
	}
	else
#endif
	if(MCUCircular_GetSpaceLen(&pBridgeMusic2Ct->BridgeFifoCt) > Len * (PCM_BYTE(BRIDGE_MUSIC2_WIDTH) * BRIDGE_MUSIC2_CHN))
	{
		MCUCircular_PutData(&pBridgeMusic2Ct->BridgeFifoCt, Buf, Len * (PCM_BYTE(BRIDGE_MUSIC2_WIDTH) * BRIDGE_MUSIC2_CHN));
		PutLen = Len;
	}
	return PutLen;
}

uint16_t BridgeMusic2_SinkGetSpaceLen(void)
{
	if(pBridgeMusic2Ct == NULL || pBridgeMusic2Ct->BridgeBuffer == NULL) return 0;
#ifdef BRIDGE_MUSIC2_HWSRC_EN
	if(pBridgeMusic2Ct->SinkSampleRate != pBridgeMusic2Ct->FifoSampleRate && pBridgeMusic2Ct->pBridgeSrcCt != NULL)
	{
		return (uint64_t)pBridgeMusic2Ct->SinkSampleRate * MCUCircular_GetSpaceLen(&pBridgeMusic2Ct->BridgeFifoCt) /(pBridgeMusic2Ct->FifoSampleRate * (PCM_BYTE(BRIDGE_MUSIC2_WIDTH) * BRIDGE_MUSIC2_CHN));
	}
	else
#endif
	{
		return MCUCircular_GetSpaceLen(&pBridgeMusic2Ct->BridgeFifoCt) / (PCM_BYTE(BRIDGE_MUSIC2_WIDTH) * BRIDGE_MUSIC2_CHN);
	}
}

uint16_t BridgeMusic2_SouceDataGet(void* Buf, uint16_t Len)
{
	uint16_t OutLen = 0;
	if(Buf == NULL || pBridgeMusic2Ct == NULL || pBridgeMusic2Ct->BridgeBuffer == NULL) return 0;
#ifdef BRIDGE_MUSIC2_HWSRC_EN
	uint16_t Length;
	uint16_t SampleLen;
	if(pBridgeMusic2Ct->pBridgeSrcCt
			&& pBridgeMusic2Ct->pBridgeSrcCt->InBuf
			&& pBridgeMusic2Ct->pBridgeSrcCt->OutBuf
			&& pBridgeMusic2Ct->FifoSampleRate != pBridgeMusic2Ct->SourceSampleRate)
	{
		Length = MCUCircular_GetDataLen(&pBridgeMusic2Ct->BridgeFifoCt);
		SampleLen = (uint64_t)Length * pBridgeMusic2Ct->SourceSampleRate /(pBridgeMusic2Ct->FifoSampleRate * (PCM_BYTE(BRIDGE_MUSIC2_WIDTH) * BRIDGE_MUSIC2_CHN));//SRCout Sample
		if(SampleLen > Len)
		{
			SampleLen = Len;
		}
		SampleLen = (uint64_t)SampleLen * pBridgeMusic2Ct->FifoSampleRate / pBridgeMusic2Ct->SourceSampleRate;//Tartet SRCIn Sample
		Length = SampleLen * (PCM_BYTE(BRIDGE_MUSIC2_WIDTH) * BRIDGE_MUSIC2_CHN);

		if(SRC_IsBlockDone(CFG_BRIDGE_HW_SRC_PORT))
		{
			OutLen = SRC_BlockApplyDone((SRC_MODULE)CFG_BRIDGE_HW_SRC_PORT, pBridgeMusic2Ct->pBridgeSrcCt, (int16_t*)Buf);
		}
#if 0
		else if(AudioCoreSinkIsEnable(USB_AUDIO_SINK_NUM))//Mask //UsbAudioMic.AltSet
			DBG("HWSRCLostF\n");
#endif
		MCUCircular_GetData(&pBridgeMusic2Ct->BridgeFifoCt, pBridgeMusic2Ct->pBridgeSrcCt->InBuf, Length);
		SRC_BlockLenSet(CFG_BRIDGE_HW_SRC_PORT, SampleLen);
		SRC_BlockApplyStart(CFG_BRIDGE_HW_SRC_PORT, pBridgeMusic2Ct->pBridgeSrcCt, (int16_t*)pBridgeMusic2Ct->pBridgeSrcCt->InBuf, SampleLen);
	}
	else
#endif
	{
		OutLen = MCUCircular_GetData(&pBridgeMusic2Ct->BridgeFifoCt, Buf, Len * (PCM_BYTE(BRIDGE_MUSIC2_WIDTH) * BRIDGE_MUSIC2_CHN)) / (PCM_BYTE(BRIDGE_MUSIC2_WIDTH) * BRIDGE_MUSIC2_CHN);
	}
	return OutLen;
}

uint16_t BridgeMusic2_SouceGetDataLen(void)
{
	if(pBridgeMusic2Ct == NULL || pBridgeMusic2Ct->BridgeBuffer == NULL) return 0;
#ifdef BRIDGE_MUSIC2_HWSRC_EN
	if(pBridgeMusic2Ct->SourceSampleRate != pBridgeMusic2Ct->FifoSampleRate && pBridgeMusic2Ct->pBridgeSrcCt != NULL)
	{
		return (uint64_t)pBridgeMusic2Ct->SourceSampleRate * MCUCircular_GetDataLen(&pBridgeMusic2Ct->BridgeFifoCt) /(pBridgeMusic2Ct->FifoSampleRate * (PCM_BYTE(BRIDGE_MUSIC2_WIDTH) * BRIDGE_MUSIC2_CHN));
	}
	else
#endif
	{
		return  MCUCircular_GetDataLen(&pBridgeMusic2Ct->BridgeFifoCt) /((PCM_BYTE(BRIDGE_MUSIC2_WIDTH) * BRIDGE_MUSIC2_CHN));
	}
}
#ifdef BRIDGE_MUSIC2_HWSRC_EN
//输入输出采样率不许相同
void BridgeMusic2SrcInit(void)
{
    //src set
	if(pBridgeMusic2Ct
			&& pBridgeMusic2Ct->pBridgeSrcCt
			&& pBridgeMusic2Ct->pBridgeSrcCt->InBuf
			&& pBridgeMusic2Ct->pBridgeSrcCt->OutBuf)
	{
		pBridgeMusic2Ct->pBridgeSrcCt->Mode = MODE_BLOCK;
#if BRIDGE_MUSIC2_WIDTH == WIDTH_16BIT
		pBridgeMusic2Ct->pBridgeSrcCt->Width = SRC_16BIT;
#elif (BRIDGE_MUSIC2_WIDTH == WIDTH_L24BIT)
		pBridgeMusic2Ct->pBridgeSrcCt->Width = SRC_24BIT;
#else
	#error Conflict: BridgeMusic2Src Width error!!!
#endif
		pBridgeMusic2Ct->pBridgeSrcCt->NumChannels = BRIDGE_MUSIC2_CHN;
		pBridgeMusic2Ct->pBridgeSrcCt->BlockSamples = pBridgeMusic2Ct->pBridgeSrcCt->InBufLen;
	if(pBridgeMusic2Ct->SinkSampleRate != pBridgeMusic2Ct->FifoSampleRate)
		SRC_BlockInit(CFG_BRIDGE_HW_SRC_PORT, pBridgeMusic2Ct->pBridgeSrcCt, pBridgeMusic2Ct->SinkSampleRate, pBridgeMusic2Ct->FifoSampleRate);
	else if(pBridgeMusic2Ct->FifoSampleRate != pBridgeMusic2Ct->SinkSampleRate)
		SRC_BlockInit(CFG_BRIDGE_HW_SRC_PORT, pBridgeMusic2Ct->pBridgeSrcCt, pBridgeMusic2Ct->FifoSampleRate, pBridgeMusic2Ct->SinkSampleRate);
	}
}
#endif

bool BridgeMusic2Init(void)
{
	AudioCoreIO AudioIOSet;
	uint16_t SinkFrameUs, SourceFrameUs, FifoDepthUs;


	if(((AudioEffect_Parambin_GetSinkBuffer(&EffectParambin, BRIDGE_MUSIC2_SINK) && AudioEffect_Parambin_GetSourceBuffer(&EffectParambin2, BRIDGE_MUSIC2_SOURCE)))
			|| (AudioEffect_Parambin_GetSinkBuffer(&EffectParambin2, BRIDGE_MUSIC2_SINK) && (AudioEffect_Parambin_GetSourceBuffer(&EffectParambin, BRIDGE_MUSIC2_SOURCE))))
	{
		pBridgeMusic2Ct = (Bridge_Context *)osPortMalloc(sizeof(Bridge_Context));
		if(pBridgeMusic2Ct == NULL)
		{
			APP_DBG("Bridge_Context Error!\n");
			return FALSE;
		}
		memset(pBridgeMusic2Ct, 0, sizeof(Bridge_Context));
		pBridgeMusic2Ct->SinkFlowNet = AudioCoreGetNetBySinkChannelNUM(BRIDGE_MUSIC2_SINK);//尚未挂载，需从图上获取配置
		pBridgeMusic2Ct->SinkSampleRate = AudioCoreMixSampleRateGet(pBridgeMusic2Ct->SinkFlowNet);
		SinkFrameUs = AudioCoreMixStepUsGet(pBridgeMusic2Ct->SinkFlowNet);
		pBridgeMusic2Ct->SourceFlowNet = AudioCoreGetNetBySourceChannelNUM(BRIDGE_MUSIC2_SOURCE);//尚未挂载，需从图上获取配置
		pBridgeMusic2Ct->SourceSampleRate = AudioCoreMixSampleRateGet(pBridgeMusic2Ct->SourceFlowNet);
		SourceFrameUs = AudioCoreMixStepUsGet(pBridgeMusic2Ct->SourceFlowNet);
		FifoDepthUs = SourceFrameUs + SinkFrameUs + BRIDGE_FIFO_JITTER;//深度控制，可加长
		if(pBridgeMusic2Ct->SinkSampleRate != pBridgeMusic2Ct->SourceSampleRate)
		{
	#ifdef BRIDGE_MUSIC2_HWSRC_EN
			uint32_t SinkFrameSize = AudioCoreFrameSizeGet(pBridgeMusic2Ct->SinkFlowNet);
			uint32_t SourceFrameSize = AudioCoreFrameSizeGet(pBridgeMusic2Ct->SourceFlowNet);
			pBridgeMusic2Ct->pBridgeSrcCt = (SRC_Context *)osPortMalloc(sizeof(SRC_Context));
			if(pBridgeMusic2Ct->pBridgeSrcCt == NULL)
			{
				APP_DBG("pBridgeMusic2Ct->BridgeSrcCt Error!\n");
				return FALSE;
			}
			memset(pBridgeMusic2Ct->pBridgeSrcCt, 0, sizeof(SRC_Context));
			/**FIFO 衔接 HWSRC 常规配在set Sink,如时隙过短会产生丢帧；如配置GetSource要满足等长转采,否则帧间Out波动不好管理**/
			if((SourceFrameUs > SinkFrameUs) && (((uint64_t)SourceFrameSize * pBridgeMusic2Ct->SinkSampleRate) % pBridgeMusic2Ct->SourceSampleRate) == 0)
			{
				pBridgeMusic2Ct->FifoSampleRate = pBridgeMusic2Ct->SinkSampleRate;
				pBridgeMusic2Ct->pBridgeSrcCt->InBufLen = ((uint64_t)SourceFrameSize * pBridgeMusic2Ct->SinkSampleRate) / pBridgeMusic2Ct->SourceSampleRate;
				pBridgeMusic2Ct->pBridgeSrcCt->OutBufLen = SinkFrameSize;
			}
			else//Sink Set @HWSRC 输出留些余量
			{
				pBridgeMusic2Ct->FifoSampleRate = pBridgeMusic2Ct->SourceSampleRate;
				pBridgeMusic2Ct->pBridgeSrcCt->InBufLen = ((uint64_t)SinkFrameSize * pBridgeMusic2Ct->SinkSampleRate) / pBridgeMusic2Ct->SourceSampleRate;
				pBridgeMusic2Ct->pBridgeSrcCt->OutBufLen = ((uint64_t)SinkFrameSize * pBridgeMusic2Ct->SourceSampleRate * 12) / (pBridgeMusic2Ct->SinkSampleRate * 10);
			}
			pBridgeMusic2Ct->pBridgeSrcCt->InBuf = (uint32_t *)osPortMalloc(pBridgeMusic2Ct->pBridgeSrcCt->InBufLen * (PCM_BYTE(BRIDGE_MUSIC2_WIDTH) * BRIDGE_MUSIC2_CHN));
			if(pBridgeMusic2Ct->pBridgeSrcCt->InBuf == NULL)
			{
				APP_DBG("pBridgeMusic2Ct->pBridgeSrcCt->InBuf Error!\n");
				return FALSE;
			}
			pBridgeMusic2Ct->pBridgeSrcCt->OutBuf = (uint32_t *)osPortMalloc(pBridgeMusic2Ct->pBridgeSrcCt->OutBufLen * (PCM_BYTE(BRIDGE_MUSIC2_WIDTH) * BRIDGE_MUSIC2_CHN));
			if(pBridgeMusic2Ct->pBridgeSrcCt->OutBuf == NULL)
			{
				APP_DBG("pBridgeMusic2Ct->pBridgeSrcCt->OutBuf Error!\n");
				return FALSE;
			}
			BridgeMusic2SrcInit();
	#else
			//此处管理软件SRC配在Source或sink 常规短时隙延时要求高,转采样配置长时隙方向。
			if(SourceFrameUs > SinkFrameUs)
			{
				pBridgeMusic2Ct->FifoSampleRate = pBridgeMusic2Ct->SinkSampleRate;
			}
			else if(SourceFrameUs < SinkFrameUs)
			{
				pBridgeMusic2Ct->FifoSampleRate = pBridgeMusic2Ct->SourceSampleRate;
			}
			else //低RAM先于延时控制时，可去除上述条件
			{
				pBridgeMusic2Ct->FifoSampleRate = MIN(pBridgeMusic2Ct->SinkSampleRate, pBridgeMusic2Ct->SourceSampleRate);
			}
	#endif
		}
		else
		{
			pBridgeMusic2Ct->FifoSampleRate = pBridgeMusic2Ct->SinkSampleRate;
		}

		pBridgeMusic2Ct->FifoSampleLen = (uint64_t)pBridgeMusic2Ct->FifoSampleRate * (FifoDepthUs) / 1000000;
		pBridgeMusic2Ct->BridgeBuffer = (uint32_t *)osPortMalloc(pBridgeMusic2Ct->FifoSampleLen * (PCM_BYTE(BRIDGE_MUSIC2_WIDTH) * BRIDGE_MUSIC2_CHN));
		if(pBridgeMusic2Ct->BridgeBuffer == NULL)
		{
			APP_DBG("pBridgeMusic2Ct->BridgeBuffer Error!\n");
			return FALSE;
		}

		//for 双图 sink和 source挂接
		MCUCircular_Config(&pBridgeMusic2Ct->BridgeFifoCt, pBridgeMusic2Ct->BridgeBuffer, pBridgeMusic2Ct->FifoSampleLen * (PCM_BYTE(BRIDGE_MUSIC2_WIDTH) * BRIDGE_MUSIC2_CHN));

		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
		AudioIOSet.PcmWidth	= BRIDGE_MUSIC2_WIDTH;
		if(!AudioCoreSinkIsInit(BRIDGE_MUSIC2_SINK))
		{
			APP_DBG("BridgeMusic2 SinkInit\n");
			AudioIOSet.Sync = FALSE;
			AudioIOSet.Channels = BRIDGE_MUSIC2_CHN;
			AudioIOSet.Net = pBridgeMusic2Ct->SinkFlowNet;
			AudioIOSet.Depth = (uint64_t)pBridgeMusic2Ct->SinkSampleRate * (FifoDepthUs) / 1000000;
	#ifndef BRIDGE_MUSIC2_HWSRC_EN
			if(pBridgeMusic2Ct->SinkSampleRate != pBridgeMusic2Ct->FifoSampleRate)
			{
				AudioIOSet.Adapt = SRC_ONLY;
				AudioIOSet.SampleRate = pBridgeMusic2Ct->FifoSampleRate;
			}
			else
	#endif
			{
				AudioIOSet.Adapt = STD;
			}
			AudioIOSet.DataIOFunc = BridgeMusic2_SinkDataSet;
			AudioIOSet.LenGetFunc = BridgeMusic2_SinkGetSpaceLen;

			if(!AudioCoreSinkInit(&AudioIOSet, BRIDGE_MUSIC2_SINK))
			{
				DBG(" SINK init error\n");
				return FALSE;
			}
			AudioCoreSinkEnable(BRIDGE_MUSIC2_SINK);
		}

		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
		AudioIOSet.PcmWidth	= BRIDGE_MUSIC2_WIDTH;
		if(!AudioCoreSourceIsInit(BRIDGE_MUSIC2_SOURCE))
		{
			APP_DBG("BridgeMusic2 SourceInit\n");

			AudioIOSet.Sync = FALSE;
			AudioIOSet.Channels = BRIDGE_MUSIC2_CHN;
			AudioIOSet.Net = pBridgeMusic2Ct->SourceFlowNet;
			AudioIOSet.Depth = (uint64_t)pBridgeMusic2Ct->SourceSampleRate * (FifoDepthUs) / 1000000;
	#ifndef BRIDGE_MUSIC2_HWSRC_EN
			if(pBridgeMusic2Ct->FifoSampleRate != pBridgeMusic2Ct->SourceSampleRate)
			{
				AudioIOSet.Adapt = SRC_ONLY;
				AudioIOSet.SampleRate = pBridgeMusic2Ct->FifoSampleRate;
			}
			else
	#endif
			{
				AudioIOSet.Adapt = STD;
			}
			AudioIOSet.DataIOFunc = BridgeMusic2_SouceDataGet;
			AudioIOSet.LenGetFunc = BridgeMusic2_SouceGetDataLen;

			if(!AudioCoreSourceInit(&AudioIOSet, BRIDGE_MUSIC2_SOURCE))
			{
				DBG("SOURCE init error\n");
				return FALSE;
			}
			AudioCoreSourceEnable(BRIDGE_MUSIC2_SOURCE);
		}
	}
	return TRUE;
}
void BridgeMusic2Deinit(void)
{
	if(pBridgeMusic2Ct != NULL)
	{
		if(AudioCoreSourceIsInit(BRIDGE_MUSIC2_SOURCE))
		{
			AudioCoreSourceDisable(BRIDGE_MUSIC2_SOURCE);
			AudioCoreSourceDeinit(BRIDGE_MUSIC2_SOURCE);

		}
		if(AudioCoreSinkIsInit(BRIDGE_MUSIC2_SINK))
		{
			AudioCoreSinkDisable(BRIDGE_MUSIC2_SINK);
			AudioCoreSinkDeinit(BRIDGE_MUSIC2_SINK);
		}
	#ifdef BRIDGE_MUSIC2_HWSRC_EN
		if(pBridgeMusic2Ct->pBridgeSrcCt != NULL)
		{
			if(pBridgeMusic2Ct->pBridgeSrcCt->InBuf != NULL)
			{
				if(pBridgeMusic2Ct->pBridgeSrcCt->OutBuf)
				{
					Reset_Function2Reset((SRC0_FUNC_SEPA) << CFG_BRIDGE_HW_SRC_PORT);
					Reset_Register2Reset(SRC0_REG_SEPA << CFG_BRIDGE_HW_SRC_PORT);
					osPortFree(pBridgeMusic2Ct->pBridgeSrcCt->OutBuf);
					pBridgeMusic2Ct->pBridgeSrcCt->OutBuf = NULL;
				}
				osPortFree(pBridgeMusic2Ct->pBridgeSrcCt->InBuf);
				pBridgeMusic2Ct->pBridgeSrcCt->InBuf = NULL;
			}
			osPortFree(pBridgeMusic2Ct->pBridgeSrcCt);
			pBridgeMusic2Ct->pBridgeSrcCt = NULL;
		}
	#endif
		if(pBridgeMusic2Ct->BridgeBuffer != NULL)
		{
			osPortFree(pBridgeMusic2Ct->BridgeBuffer);
			pBridgeMusic2Ct->BridgeBuffer = NULL;
		}

		osPortFree(pBridgeMusic2Ct);
		pBridgeMusic2Ct = NULL;
	}
}

#endif //#ifdef BRIDGE_MUSIC2_EN

