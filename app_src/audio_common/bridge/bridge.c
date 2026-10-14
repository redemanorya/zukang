#include "mvintrinsics.h"
#include "src.h"
#include "reset.h"
#include "resource_defs.h"
#include "src_interface.h"
#include "audio_effect.h"
#include "rtos_api.h"
#include "app_config.h"
#include "bridge/bridge.h"

#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN

bool BridgeSinkSourceInit(void)
{
#ifdef BRIDGE_MUSIC_EN
	if(BridgeMusicInit() != TRUE)
		return FALSE;
#endif //#ifdef BRIDGE_MUSIC_EN

#ifdef BRIDGE_MUSIC2_EN
	if(BridgeMusic2Init() != TRUE)
		return FALSE;
#endif //#ifdef BRIDGE_MUSIC2_EN

#ifdef BRIDGE_MIC_EN
	if(BridgeMicInit() != TRUE)
		return FALSE;
#endif //#ifdef BRIDGE_MIC_EN

#ifdef BRIDGE_MIC2_EN
	if(BridgeMic2Init() != TRUE)
		return FALSE;
#endif //#ifdef BRIDGE_MIC2_EN
	return TRUE;
}

void BridgeSinkSourceDeinit(void)
{
#ifdef BRIDGE_MUSIC_EN
	BridgeMusicDeinit();
#endif //#ifdef BRIDGE_MUSIC_EN

#ifdef BRIDGE_MUSIC2_EN
	BridgeMusic2Deinit();
#endif //#ifdef BRIDGE_MUSIC2_EN

#ifdef BRIDGE_MIC_EN
	BridgeMicDeinit();
#endif //#ifdef BRIDGE_MIC_EN

#ifdef BRIDGE_MIC2_EN
	BridgeMic2Deinit();
#endif //#ifdef BRIDGE_MIC2_EN
}

#ifdef BRIDGE_MUSIC_EN
Bridge_Context * pBridgeMusicCt;

#if (BT_LE_AUDIO_SUPPORT)
extern uint16_t get_bis_sync_need_samples(uint16_t	Depth);
static bool 	bis_drop_flag = 0;
static uint32_t bis_drop_samples = 0;

void BridgeSinkSourceBufferClear(void)
{
	printf("----BridgeSinkSourceBufferClear\n");
	pBridgeMusicCt->BridgeFifoCt.R = 0;
	pBridgeMusicCt->BridgeFifoCt.W = 0;
	bis_drop_flag = 0;
	bis_drop_samples = 0;
	AudioCoreSourceDisable(BRIDGE_MUSIC_SOURCE);
}

void BridgeSinkSourceBufferDrop(uint32_t DropSample)
{
	bis_drop_flag = 1;
	bis_drop_samples = DropSample;
}
#endif

uint16_t BridgeMusic_SinkDataSet(void* Buf, uint16_t Len)
{
	uint16_t PutLen = 0;
	if(Buf == NULL || pBridgeMusicCt == NULL || pBridgeMusicCt->BridgeBuffer == NULL) return 0;
#ifdef BRIDGE_MUSIC_HWSRC_EN
	if(pBridgeMusicCt->pBridgeSrcCt
			&& pBridgeMusicCt->pBridgeSrcCt->InBuf
			&& pBridgeMusicCt->pBridgeSrcCt->OutBuf
			&& pBridgeMusicCt->FifoSampleRate != pBridgeMusicCt->SinkSampleRate)
	{
		if(SRC_IsBlockDone(CFG_BRIDGE_HW_SRC_PORT))
		{
			PutLen = SRC_BlockApplyDone((SRC_MODULE)CFG_BRIDGE_HW_SRC_PORT, pBridgeMusicCt->pBridgeSrcCt, (int16_t*)pBridgeMusicCt->pBridgeSrcCt->OutBuf);
			if(MCUCircular_GetSpaceLen(&pBridgeMusicCt->BridgeFifoCt) > PutLen * (PCM_BYTE(BRIDGE_MUSIC_WIDTH) * BRIDGE_MUSIC_CHN))
			{
				MCUCircular_PutData(&pBridgeMusicCt->BridgeFifoCt, pBridgeMusicCt->pBridgeSrcCt->OutBuf, PutLen * (PCM_BYTE(BRIDGE_MUSIC_WIDTH) * BRIDGE_MUSIC_CHN));
			}
		}

		SRC_BlockLenSet(CFG_BRIDGE_HW_SRC_PORT, Len);
		SRC_BlockApplyStart(CFG_BRIDGE_HW_SRC_PORT, pBridgeMusicCt->pBridgeSrcCt, (int16_t*)Buf, Len);
	}
	else
#endif
	if(MCUCircular_GetSpaceLen(&pBridgeMusicCt->BridgeFifoCt) > Len * (PCM_BYTE(BRIDGE_MUSIC_WIDTH) * BRIDGE_MUSIC_CHN))
	{
		MCUCircular_PutData(&pBridgeMusicCt->BridgeFifoCt, Buf, Len * (PCM_BYTE(BRIDGE_MUSIC_WIDTH) * BRIDGE_MUSIC_CHN));
		PutLen = Len;
	}
	return PutLen;
}

uint16_t BridgeMusic_SinkGetSpaceLen(void)
{
	if(pBridgeMusicCt == NULL || pBridgeMusicCt->BridgeBuffer == NULL) return 0;
#ifdef BRIDGE_MUSIC_HWSRC_EN
	if(pBridgeMusicCt->SinkSampleRate != pBridgeMusicCt->FifoSampleRate && pBridgeMusicCt->pBridgeSrcCt != NULL)
	{
		return (uint64_t)pBridgeMusicCt->SinkSampleRate * MCUCircular_GetSpaceLen(&pBridgeMusicCt->BridgeFifoCt) /(pBridgeMusicCt->FifoSampleRate * (PCM_BYTE(BRIDGE_MUSIC_WIDTH) * BRIDGE_MUSIC_CHN));
	}
	else
#endif
	{
		return MCUCircular_GetSpaceLen(&pBridgeMusicCt->BridgeFifoCt) / (PCM_BYTE(BRIDGE_MUSIC_WIDTH) * BRIDGE_MUSIC_CHN);
	}
}

uint16_t BridgeMusic_SouceDataGet(void* Buf, uint16_t Len)
{
	uint16_t OutLen = 0;
	if(Buf == NULL || pBridgeMusicCt == NULL || pBridgeMusicCt->BridgeBuffer == NULL) return 0;
#if (BT_LE_AUDIO_SUPPORT)
	if(bis_drop_flag)
	{
		extern uint16_t BridgeMusic_SouceGetDataLen(void);
		bis_drop_flag = 0;
		printf("====BridgeMusic_SouceDataGet,Dac0Samples:%d,%d,%d\n", bis_drop_samples,Len,BridgeMusic_SouceGetDataLen());
		while(bis_drop_samples)
		{
			if(Len < bis_drop_samples)
			{
				MCUCircular_GetData(&pBridgeMusicCt->BridgeFifoCt, Buf, Len * (PCM_BYTE(BRIDGE_MUSIC_WIDTH) * BRIDGE_MUSIC_CHN)) / (PCM_BYTE(BRIDGE_MUSIC_WIDTH) * BRIDGE_MUSIC_CHN);
				bis_drop_samples -= Len;
			}
			else
			{
				MCUCircular_GetData(&pBridgeMusicCt->BridgeFifoCt, Buf, bis_drop_samples * (PCM_BYTE(BRIDGE_MUSIC_WIDTH) * BRIDGE_MUSIC_CHN)) / (PCM_BYTE(BRIDGE_MUSIC_WIDTH) * BRIDGE_MUSIC_CHN);
				bis_drop_samples = 0;
			}
		}
	}
#endif
#ifdef BRIDGE_MUSIC_HWSRC_EN
	uint16_t Length;
	uint16_t SampleLen;
	if(pBridgeMusicCt->pBridgeSrcCt
			&& pBridgeMusicCt->pBridgeSrcCt->InBuf
			&& pBridgeMusicCt->pBridgeSrcCt->OutBuf
			&& pBridgeMusicCt->FifoSampleRate != pBridgeMusicCt->SourceSampleRate)
	{
		Length = MCUCircular_GetDataLen(&pBridgeMusicCt->BridgeFifoCt);
		SampleLen = (uint64_t)Length * pBridgeMusicCt->SourceSampleRate /(pBridgeMusicCt->FifoSampleRate * (PCM_BYTE(BRIDGE_MUSIC_WIDTH) * BRIDGE_MUSIC_CHN));//SRCout Sample
		if(SampleLen > Len)
		{
			SampleLen = Len;
		}
		SampleLen = (uint64_t)SampleLen * pBridgeMusicCt->FifoSampleRate / pBridgeMusicCt->SourceSampleRate;//Tartet SRCIn Sample
		Length = SampleLen * (PCM_BYTE(BRIDGE_MUSIC_WIDTH) * BRIDGE_MUSIC_CHN);

		if(SRC_IsBlockDone(CFG_BRIDGE_HW_SRC_PORT))
		{
			OutLen = SRC_BlockApplyDone((SRC_MODULE)CFG_BRIDGE_HW_SRC_PORT, pBridgeMusicCt->pBridgeSrcCt, (int16_t*)Buf);
		}
#if 0
		else if(AudioCoreSinkIsEnable(USB_AUDIO_SINK_NUM))//Mask //UsbAudioMic.AltSet
			DBG("HWSRCLostF\n");
#endif
		MCUCircular_GetData(&pBridgeMusicCt->BridgeFifoCt, pBridgeMusicCt->pBridgeSrcCt->InBuf, Length);
		SRC_BlockLenSet(CFG_BRIDGE_HW_SRC_PORT, SampleLen);
		SRC_BlockApplyStart(CFG_BRIDGE_HW_SRC_PORT, pBridgeMusicCt->pBridgeSrcCt, (int16_t*)pBridgeMusicCt->pBridgeSrcCt->InBuf, SampleLen);
	}
	else
#endif
	{
		OutLen = MCUCircular_GetData(&pBridgeMusicCt->BridgeFifoCt, Buf, Len * (PCM_BYTE(BRIDGE_MUSIC_WIDTH) * BRIDGE_MUSIC_CHN)) / (PCM_BYTE(BRIDGE_MUSIC_WIDTH) * BRIDGE_MUSIC_CHN);
	}
	return OutLen;
}

uint16_t BridgeMusic_SouceGetDataLen(void)
{
	if(pBridgeMusicCt == NULL || pBridgeMusicCt->BridgeBuffer == NULL) return 0;
#ifdef BRIDGE_MUSIC_HWSRC_EN
	if(pBridgeMusicCt->SourceSampleRate != pBridgeMusicCt->FifoSampleRate && pBridgeMusicCt->pBridgeSrcCt != NULL)
	{
		return (uint64_t)pBridgeMusicCt->SourceSampleRate * MCUCircular_GetDataLen(&pBridgeMusicCt->BridgeFifoCt) /(pBridgeMusicCt->FifoSampleRate * (PCM_BYTE(BRIDGE_MUSIC_WIDTH) * BRIDGE_MUSIC_CHN));
	}
	else
#endif
	{
		return  MCUCircular_GetDataLen(&pBridgeMusicCt->BridgeFifoCt) /((PCM_BYTE(BRIDGE_MUSIC_WIDTH) * BRIDGE_MUSIC_CHN));
	}
}
#ifdef BRIDGE_MUSIC_HWSRC_EN
//输入输出采样率不许相同
void BridgeMusicSrcInit(void)
{
    //src set
	if(pBridgeMusicCt
			&& pBridgeMusicCt->pBridgeSrcCt
			&& pBridgeMusicCt->pBridgeSrcCt->InBuf
			&& pBridgeMusicCt->pBridgeSrcCt->OutBuf)
	{
		pBridgeMusicCt->pBridgeSrcCt->Mode = MODE_BLOCK;
#if BRIDGE_MUSIC_WIDTH == WIDTH_16BIT
		pBridgeMusicCt->pBridgeSrcCt->Width = SRC_16BIT;
#elif (BRIDGE_MUSIC_WIDTH == WIDTH_L24BIT)
		pBridgeMusicCt->pBridgeSrcCt->Width = SRC_24BIT;
#else
	#error Conflict: BridgeSrc Width error!!!
#endif
		pBridgeMusicCt->pBridgeSrcCt->NumChannels = BRIDGE_MUSIC_CHN;
		pBridgeMusicCt->pBridgeSrcCt->BlockSamples = pBridgeMusicCt->pBridgeSrcCt->InBufLen;
	if(pBridgeMusicCt->SinkSampleRate != pBridgeMusicCt->FifoSampleRate)
		SRC_BlockInit(CFG_BRIDGE_HW_SRC_PORT, pBridgeMusicCt->pBridgeSrcCt, pBridgeMusicCt->SinkSampleRate, pBridgeMusicCt->FifoSampleRate);
	else if(pBridgeMusicCt->FifoSampleRate != pBridgeMusicCt->SinkSampleRate)
		SRC_BlockInit(CFG_BRIDGE_HW_SRC_PORT, pBridgeMusicCt->pBridgeSrcCt, pBridgeMusicCt->FifoSampleRate, pBridgeMusicCt->SinkSampleRate);
	}
}
#endif
#endif //#ifdef BRIDGE_MUSIC_EN


bool BridgeMusicInit(void)
{
	AudioCoreIO AudioIOSet;
	uint16_t SinkFrameUs, SourceFrameUs, FifoDepthUs;

	if(((AudioEffect_Parambin_GetSinkBuffer(&EffectParambin, BRIDGE_MUSIC_SINK) && AudioEffect_Parambin_GetSourceBuffer(&EffectParambin2, BRIDGE_MUSIC_SOURCE)))
			|| (AudioEffect_Parambin_GetSinkBuffer(&EffectParambin2, BRIDGE_MUSIC_SINK) && (AudioEffect_Parambin_GetSourceBuffer(&EffectParambin, BRIDGE_MUSIC_SOURCE))))
	{
		pBridgeMusicCt = (Bridge_Context *)osPortMalloc(sizeof(Bridge_Context));
		if(pBridgeMusicCt == NULL)
		{
			APP_DBG("BridegMusicContext Error!\n");
			return FALSE;
		}
		memset(pBridgeMusicCt, 0, sizeof(Bridge_Context));
		pBridgeMusicCt->SinkFlowNet = AudioCoreGetNetBySinkChannelNUM(BRIDGE_MUSIC_SINK);//尚未挂载，需从图上获取配置
		pBridgeMusicCt->SinkSampleRate = AudioCoreMixSampleRateGet(pBridgeMusicCt->SinkFlowNet);
		SinkFrameUs = AudioCoreMixStepUsGet(pBridgeMusicCt->SinkFlowNet);
		pBridgeMusicCt->SourceFlowNet = AudioCoreGetNetBySourceChannelNUM(BRIDGE_MUSIC_SOURCE);//尚未挂载，需从图上获取配置
		pBridgeMusicCt->SourceSampleRate = AudioCoreMixSampleRateGet(pBridgeMusicCt->SourceFlowNet);
		SourceFrameUs = AudioCoreMixStepUsGet(pBridgeMusicCt->SourceFlowNet);
		FifoDepthUs = SourceFrameUs + SinkFrameUs + BRIDGE_FIFO_JITTER;//深度控制，可加长
		if(pBridgeMusicCt->SinkSampleRate != pBridgeMusicCt->SourceSampleRate)
		{
	#ifdef BRIDGE_MUSIC_HWSRC_EN
			uint32_t SinkFrameSize = AudioCoreFrameSizeGet(pBridgeMusicCt->SinkFlowNet);
			uint32_t SourceFrameSize = AudioCoreFrameSizeGet(pBridgeMusicCt->SourceFlowNet);
			pBridgeMusicCt->pBridgeSrcCt = (SRC_Context *)osPortMalloc(sizeof(SRC_Context));
			if(pBridgeMusicCt->pBridgeSrcCt == NULL)
			{
				APP_DBG("pBridgeMusicCt->BridgeSrcCt Error!\n");
				return FALSE;
			}
			memset(pBridgeMusicCt->pBridgeSrcCt, 0, sizeof(SRC_Context));
			/**FIFO 衔接 HWSRC 常规配在set Sink,如时隙过短会产生丢帧；如配置GetSource要满足等长转采,否则帧间Out波动不好管理**/
			if((SourceFrameUs > SinkFrameUs) && (((uint64_t)SourceFrameSize * pBridgeMusicCt->SinkSampleRate) % pBridgeMusicCt->SourceSampleRate) == 0)
			{
				pBridgeMusicCt->FifoSampleRate = pBridgeMusicCt->SinkSampleRate;
				pBridgeMusicCt->pBridgeSrcCt->InBufLen = ((uint64_t)SourceFrameSize * pBridgeMusicCt->SinkSampleRate) / pBridgeMusicCt->SourceSampleRate;
				pBridgeMusicCt->pBridgeSrcCt->OutBufLen = SinkFrameSize;
			}
			else//Sink Set @HWSRC 输出留些余量
			{
				pBridgeMusicCt->FifoSampleRate = pBridgeMusicCt->SourceSampleRate;
				pBridgeMusicCt->pBridgeSrcCt->InBufLen = ((uint64_t)SinkFrameSize * pBridgeMusicCt->SinkSampleRate) / pBridgeMusicCt->SourceSampleRate;
				pBridgeMusicCt->pBridgeSrcCt->OutBufLen = ((uint64_t)SinkFrameSize * pBridgeMusicCt->SourceSampleRate * 12) / (pBridgeMusicCt->SinkSampleRate * 10);
			}
			pBridgeMusicCt->pBridgeSrcCt->InBuf = (uint32_t *)osPortMalloc(pBridgeMusicCt->pBridgeSrcCt->InBufLen * (PCM_BYTE(BRIDGE_MUSIC_WIDTH) * BRIDGE_MUSIC_CHN));
			if(pBridgeMusicCt->pBridgeSrcCt->InBuf == NULL)
			{
				APP_DBG("pBridgeMusicCt->pBridgeSrcCt->InBuf Error!\n");
				return FALSE;
			}
			pBridgeMusicCt->pBridgeSrcCt->OutBuf = (uint32_t *)osPortMalloc(pBridgeMusicCt->pBridgeSrcCt->OutBufLen * (PCM_BYTE(BRIDGE_MUSIC_WIDTH) * BRIDGE_MUSIC_CHN));
			if(pBridgeMusicCt->pBridgeSrcCt->OutBuf == NULL)
			{
				APP_DBG("pBridgeMusicCt->pBridgeSrcCt->OutBuf Error!\n");
				return FALSE;
			}
			BridgeMusicSrcInit();
	#else
			//此处管理软件SRC配在Source或sink 常规短时隙延时要求高,转采样配置长时隙方向。
			if(SourceFrameUs > SinkFrameUs)
			{
				pBridgeMusicCt->FifoSampleRate = pBridgeMusicCt->SinkSampleRate;
			}
			else if(SourceFrameUs < SinkFrameUs)
			{
				pBridgeMusicCt->FifoSampleRate = pBridgeMusicCt->SourceSampleRate;
			}
			else //低RAM先于延时控制时，可去除上述条件
			{
				pBridgeMusicCt->FifoSampleRate = MIN(pBridgeMusicCt->SinkSampleRate, pBridgeMusicCt->SourceSampleRate);
			}
	#endif
		}
		else
		{
			pBridgeMusicCt->FifoSampleRate = pBridgeMusicCt->SinkSampleRate;
		}

		pBridgeMusicCt->FifoSampleLen = (uint64_t)pBridgeMusicCt->FifoSampleRate * (FifoDepthUs) / 1000000;
#if (BT_LE_AUDIO_SUPPORT)
		pBridgeMusicCt->FifoSampleLen = get_bis_sync_need_samples(pBridgeMusicCt->FifoSampleLen);
#endif
		pBridgeMusicCt->BridgeBuffer = (uint32_t *)osPortMalloc(pBridgeMusicCt->FifoSampleLen * (PCM_BYTE(BRIDGE_MUSIC_WIDTH) * BRIDGE_MUSIC_CHN));
		if(pBridgeMusicCt->BridgeBuffer == NULL)
		{
			APP_DBG("pBridgeMusicCt->BridgeBuffer Error!\n");
			return FALSE;
		}

		//for 双图 sink和 source挂接
		MCUCircular_Config(&pBridgeMusicCt->BridgeFifoCt, pBridgeMusicCt->BridgeBuffer, pBridgeMusicCt->FifoSampleLen * (PCM_BYTE(BRIDGE_MUSIC_WIDTH) * BRIDGE_MUSIC_CHN));

		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
		AudioIOSet.PcmWidth	= BRIDGE_MUSIC_WIDTH;
		if(!AudioCoreSinkIsInit(BRIDGE_MUSIC_SINK))
		{
			APP_DBG("BridgeMusic SinkInit\n");
	#if (BT_LE_AUDIO_SUPPORT)
			AudioIOSet.Sync = TRUE;
	#else
			AudioIOSet.Sync = FALSE;
	#endif
			AudioIOSet.Channels = BRIDGE_MUSIC_CHN;
			AudioIOSet.Net = pBridgeMusicCt->SinkFlowNet;
			AudioIOSet.Depth = (uint64_t)pBridgeMusicCt->SinkSampleRate * (FifoDepthUs) / 1000000;
	#ifndef BRIDGE_MUSIC_HWSRC_EN
			if(pBridgeMusicCt->SinkSampleRate != pBridgeMusicCt->FifoSampleRate)
			{
				AudioIOSet.Adapt = SRC_ONLY;
				AudioIOSet.SampleRate = pBridgeMusicCt->FifoSampleRate;
			}
			else
	#endif
			{
				AudioIOSet.Adapt = STD;
			}
			AudioIOSet.DataIOFunc = BridgeMusic_SinkDataSet;
			AudioIOSet.LenGetFunc = BridgeMusic_SinkGetSpaceLen;

			if(!AudioCoreSinkInit(&AudioIOSet, BRIDGE_MUSIC_SINK))
			{
				DBG(" SINK init error\n");
				return FALSE;
			}
			AudioCoreSinkEnable(BRIDGE_MUSIC_SINK);
		}

		memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
		AudioIOSet.PcmWidth	= BRIDGE_MUSIC_WIDTH;
		if(!AudioCoreSourceIsInit(BRIDGE_MUSIC_SOURCE))
		{
			APP_DBG("BridgeMusic SourceInit\n");

	#if (BT_LE_AUDIO_SUPPORT)
			AudioIOSet.Sync = TRUE;
	#else
			AudioIOSet.Sync = FALSE;
	#endif
			AudioIOSet.Channels = BRIDGE_MUSIC_CHN;
			AudioIOSet.Net = pBridgeMusicCt->SourceFlowNet;
			AudioIOSet.Depth = (uint64_t)pBridgeMusicCt->SourceSampleRate * (FifoDepthUs) / 1000000;
	#ifndef BRIDGE_MUSIC_HWSRC_EN
			if(pBridgeMusicCt->FifoSampleRate != pBridgeMusicCt->SourceSampleRate)
			{
				AudioIOSet.Adapt = SRC_ONLY;
				AudioIOSet.SampleRate = pBridgeMusicCt->FifoSampleRate;
			}
			else
	#endif
			{
				AudioIOSet.Adapt = STD;
			}
			AudioIOSet.DataIOFunc = BridgeMusic_SouceDataGet;
			AudioIOSet.LenGetFunc = BridgeMusic_SouceGetDataLen;

			if(!AudioCoreSourceInit(&AudioIOSet, BRIDGE_MUSIC_SOURCE))
			{
				DBG("SOURCE init error\n");
				return FALSE;
			}
			AudioCoreSourceEnable(BRIDGE_MUSIC_SOURCE);
		}
	}
	return TRUE;
}

void BridgeMusicDeinit(void)
{
	if(pBridgeMusicCt != NULL)
	{
		if(AudioCoreSourceIsInit(BRIDGE_MUSIC_SOURCE))
		{
			AudioCoreSourceDisable(BRIDGE_MUSIC_SOURCE);
			AudioCoreSourceDeinit(BRIDGE_MUSIC_SOURCE);

		}
		if(AudioCoreSinkIsInit(BRIDGE_MUSIC_SINK))
		{
			AudioCoreSinkDisable(BRIDGE_MUSIC_SINK);
			AudioCoreSinkDeinit(BRIDGE_MUSIC_SINK);
		}
	#ifdef BRIDGE_MUSIC_HWSRC_EN
		if(pBridgeMusicCt->pBridgeSrcCt != NULL)
		{
			if(pBridgeMusicCt->pBridgeSrcCt->InBuf != NULL)
			{
				if(pBridgeMusicCt->pBridgeSrcCt->OutBuf)
				{
					Reset_Function2Reset((SRC0_FUNC_SEPA) << CFG_BRIDGE_HW_SRC_PORT);
					Reset_Register2Reset(SRC0_REG_SEPA << CFG_BRIDGE_HW_SRC_PORT);
					osPortFree(pBridgeMusicCt->pBridgeSrcCt->OutBuf);
					pBridgeMusicCt->pBridgeSrcCt->OutBuf = NULL;
				}
				osPortFree(pBridgeMusicCt->pBridgeSrcCt->InBuf);
				pBridgeMusicCt->pBridgeSrcCt->InBuf = NULL;
			}
			osPortFree(pBridgeMusicCt->pBridgeSrcCt);
			pBridgeMusicCt->pBridgeSrcCt = NULL;
		}
	#endif
		if(pBridgeMusicCt->BridgeBuffer != NULL)
		{
			osPortFree(pBridgeMusicCt->BridgeBuffer);
			pBridgeMusicCt->BridgeBuffer = NULL;
		}

		osPortFree(pBridgeMusicCt);
		pBridgeMusicCt = NULL;
	}
}

#endif //#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN

#if defined(BRIDGE_MUSIC_HWSRC_EN) && defined(BRIDGE_MIC_HWSRC_EN)
	|| defined(BRIDGE_MUSIC2_HWSRC_EN) && defined(BRIDGE_MIC_HWSRC_EN)
	|| defined(BRIDGE_MUSIC_HWSRC_EN) && defined(BRIDGE_MUSIC2_HWSRC_EN)
	#error Conflict: Bridge HWSRC配置冲突!!!
#endif
