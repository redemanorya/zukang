/**
 **************************************************************************************
 * @file    audio_core_api.h
 * @brief   audio core 
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2016-6-29 13:06:47$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __AUDIO_CORE_API_H__
#define __AUDIO_CORE_API_H__
#include "timeout.h"
#include "resampler.h"
#include "bt_config.h"
#ifdef CFG_APP_CONFIG
	#include "app_config.h"
#endif
#include "audio_core_adapt.h"
#include "audio_core_effect.h"

#include "audio_effect_parambin.h"

#ifdef CFG_FUNC_EFFECT_CORE_DUAL_EN
	#define CFG_PARA_CORE2_NET			SeparateNet//DefaultNet//
#endif

#define AUDIO_TOTAL_NUM			(AUDIO_CORE_SOURCE_MAX_NUM + AUDIO_CORE_SINK_MAX_NUM)
#define AUDIO_WORD_NUM			((AUDIO_TOTAL_NUM + 31) / 32)

#define AUDIO_HF_SCO_SINK_NUM 	AUDIO_APP_SINK_NUM

//不会并存的通路可以合并，特别是sink通路
#ifndef CFG_RES_USB_HS_MIX_AUDIO_EN
	#define USB_AUDIO_SOURCE_NUM		APP_SOURCE_NUM
	#define USB_AUDIO_SOURCE1_NUM		USB_HS_MIX_SOURCE_NUM
	//随模式调用的共用sink
	//#define AUDIO_HF_SCO_SINK_NUM 	AUDIO_APP_SINK_NUM
	#define USB_AUDIO_SINK_NUM			AUDIO_APP_SINK_NUM
#endif

#ifndef CFG_RES_USB_FS_MIX_AUDIO_EN
	#define USB_FS_AUDIO_SOURCE_NUM		APP_SOURCE_NUM
	#define USB_FS_AUDIO_SOURCE1_NUM	USB_HS_MIX_SOURCE_NUM
	#define USB_FS_AUDIO_SINK_NUM		AUDIO_APP_SINK_NUM
#endif

#ifdef CFG_RES_USB_FS_MIX_AUDIO_EN
	#define USB_FS_AUDIO_SOURCE_NUM		USB_FS_MIX_SOURCE_NUM
	#define USB_FS_AUDIO_SINK_NUM		USB_FS_MIX_SINK_NUM
#endif

#ifdef CFG_RES_USB_HS_MIX_AUDIO_EN
	#define USB_AUDIO_SOURCE_NUM		USB_HS_MIX_SOURCE_NUM
	#define USB_AUDIO_SINK_NUM			USB_HS_MIX_SINK_NUM
#endif

typedef uint16_t (*AudioCoreDataGetFunc)(void* Buf, uint16_t Samples);
typedef uint16_t (*AudioCoreDataLenFunc)(void);
typedef uint16_t (*AudioCoreDataSetFunc)(void* Buf, uint16_t Samples);
typedef uint16_t (*AudioCoreSpaceLenFunc)(void);

//每个模式混音通路组合及其音效处理的函数结构。
typedef void (*AudioCoreProcessFunc)(void*, uint8_t);//音效处理主函数

typedef bool (*SourceSyncMsgFunc)(uint16_t);

#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
	#define PCM_DATA_TYPE	int32_t
#else
	#define PCM_DATA_TYPE	int16_t
#endif

typedef enum
{
	MIX_STATE_IDLE,
	MIX_STATE_GOT,
	MIX_STATE_SYNC,//
	MIX_STATE_PROC,
	MIX_STATE_PUT,
}MIX_STATE;

typedef struct _AudioCoreSource
{
	bool						Enable;//通路启停
	bool						Sync;//TRUE:每帧同步，不足时混音通路阻塞。 FALSE:帧操作，Source数据不足时填零，Sink：空间不足时丢弃
	uint8_t						Channels;//接口fifo PCM声道
	uint8_t						PcmWidth;//接口fifo PCM Width value
	uint8_t						FlowCh;//Flow Channels
	uint8_t						FlowWidth;//@FlowIO Source
	bool						LeftMuteFlag;//静音标志
	bool						RightMuteFlag;//静音标志
	bool						MuteFlagbk;//静音标志
	bool						Active;//混音组合的数据/空间帧完备，发起AudioProcessMain，此通路激活。
	uint8_t						Net;//缺省DefaultNet/0,  使能条件下同步帧输入和输出
	AUDIO_ADAPT					Adapt;//缺省:STD,通路采样和微调功能设置
	AudioCoreDataGetFunc		DataGetFunc;//抽数据入口函数
	AudioCoreDataLenFunc		DataLenFunc;//数据sample数 函数
	uint32_t					LenFuncDepth;
	PCM_DATA_TYPE				*PcmInBuf;	//通路数据帧处理buf
	PCM_DATA_TYPE				*AdaptBuf;	//SRC&SRA缓存buf
	SRC_ADAPTER					*SrcAdapter; //转采样适配器
	void						*AdjAdapter;//微调适配器
	bool						InitFlag;	 //通路挂载标记
	bool						PcmBufFlag;	//为1的时候，PcmInBuf由audiocore申请，需要audiocore释放
}AudioCoreSource;


typedef struct _AudioCoreSink
{
	bool						Enable;//通路启停
	bool						Sync;//TRUE:每帧同步，不足时混音通路阻塞。 FALSE:帧操作，Source数据不足时填零，Sink：空间不足时丢弃
	uint8_t						Channels;//接口fifo PCM声道
	uint8_t						PcmWidth;//接口fifo PCM Width value
	uint8_t						FlowCh;//Flow Channels
	uint8_t						FlowWidth;//@FlowIO Sink
	bool						Active;//混音组合的数据/空间帧完备，发起AudioProcessMain，此通路激活。
	uint8_t						Net; //缺省DefaultNet/0,  使能条件下同步帧输入和输出
	AUDIO_ADAPT					Adapt;//缺省:STD,通路采样和微调功能设置
	bool						LeftMuteFlag;//静音标志
	bool						RightMuteFlag;//静音标志
	bool						MuteFlagbk;//静音标志
	AudioCoreDataSetFunc		DataSetFunc;//推流入口 内buf->外缓冲搬移函数
	AudioCoreSpaceLenFunc		SpaceLenFunc;//数据填充空间sample数 函数
	uint32_t					LenFuncDepth;
	PCM_DATA_TYPE				*PcmOutBuf;//通路数据帧处理buf
	PCM_DATA_TYPE				*AdaptBuf;//SRC&SRA缓存buf
	void						*AdjAdapter; //微调适配器
	SRC_ADAPTER					*SrcAdapter; //转采样适配器

	bool						InitFlag;	//通路挂载标记
	bool						PcmBufFlag;	//为1的时候，PcmOutBuf由audiocore申请，需要audiocore释放
	bool						PresetFlag;  //音频帧首次预填,fifo重置时可配置
}AudioCoreSink;


typedef struct _NetContext
{
	MIX_STATE		State;
	uint32_t		SampleRate;
	uint16_t		FrameSize;//采样帧
	uint32_t		FrameUs;//帧时隙uS--取整近似,供参考
	uint8_t			PcmWidth;
	uint64_t		FrameReady; //使用位段登记 数据/空间帧可用
	uint32_t		ReadyIO[AUDIO_WORD_NUM];
	uint32_t		SyncIO[AUDIO_WORD_NUM];
	uint32_t		NetIO[AUDIO_WORD_NUM];
	uint32_t		FrameCycle;
	uint32_t		FrameStart;
	uint32_t		FrameCnt;
	uint16_t		AudioMcps;
	uint32_t		NetBuf[(CFG_MAX_FRAME * sizeof(PCM_DATA_TYPE)) / 2];
#ifdef FRAME_SYNC_TRIGER_EN
	SourceSyncMsgFunc TrigerMsgFunc;
#endif
	bool			AdaptProcessed;
} NetContext;

typedef struct {
	uint32_t 	address; 		// 需要擦除的buff首地址
	uint32_t    framelength;    // 帧长
	uint8_t   	flownum;   		// 图的编号 0:图1/	1:图2
} AudioEraseSourceRecord;

typedef struct {
	uint32_t 	address; 		// 需要擦除的buff首地址
	uint32_t    framelength;    // 帧长
	uint8_t   	flownum;   		// 图的编号 0:图1/	1:图2
} AudioEraseSinkRecord;

typedef struct _AudioCoreContext
{
	uint8_t 		Pause;//1：Request;(1+MaxNet):Paused;
	uint32_t		AdaptIn[(CFG_MAX_FRAME * sizeof(PCM_DATA_TYPE)) / 2];//转采样和软件微调输入buf 4字节对齐便于dmafifo衔接
	uint32_t		AdaptOut[(CFG_MAX_FRAME * SRC_SCALE_MAX * sizeof(PCM_DATA_TYPE))/ 2];//转采样和软件微调输出buf
	uint8_t			ScanNet; //旨在 多通路异步处理和收发。
	NetContext		NetCt[MaxNet];
	AudioCoreSource AudioSource[AUDIO_CORE_SOURCE_MAX_NUM];
	AudioCoreProcessFunc AudioEffectProcess;			//****流处理入口
	AudioCoreSink   AudioSink[AUDIO_CORE_SINK_MAX_NUM];
	bool			AudioFlowRenew;//音效图节点、通路启停变更

	AudioEraseSourceRecord	AudioSourceRecord[AUDIO_CORE_SOURCE_MAX_NUM*2];
	AudioEraseSinkRecord	AudioSinkRecord	[AUDIO_CORE_SINK_MAX_NUM*2];
}AudioCoreContext;

extern AudioCoreContext		AudioCore;

//typedef void (*AudioCoreProcessFunc)(AudioCoreContext *AudioCore);
/**
 * @func        AudioCoreSourceFreqAdjustEnable
 * @brief       使能系统音频分频微调，使信道之间同步(与异步音源)
 * @param       uint8_t AsyncIndex   异步音频源混音信道编号
 * @param       uint16_t LevelLow   异步音频源混音信道低水位采样点值
 * @param       uint16_t LevelHigh   异步音频源混音信道高水位采样点值
 * @Output      None
 * @return      None
 * @Others
 * Record
 * 1.Date        : 20180518
 *   Author      : pi.wang
 *   Modification: Created function
 */

//音量淡入淡出
#define MixerFadeVolume(a, b, c)  	\
    if(a > b + c)		    \
	{						\
		a -= c;				\
	}						\
	else if(a + c < b)	   	\
	{						\
		a += c;				\
	}						\
	else					\
	{						\
		a = b;				\
	}

//随通路数据调整声道配置，通路init时需要配置最大声道。
void AudioCoreSourcePcmFormatConfig(uint8_t Index, uint16_t Format);

void AudioCoreSourceEnable(uint8_t Index);

void AudioCoreSourceDisable(uint8_t Index);

bool AudioCoreSourceIsEnable(uint8_t Index);

void AudioCoreSourceMute(uint8_t Index, bool IsLeftMute, bool IsRightMute);

void AudioCoreSourceUnmute(uint8_t Index, bool IsLeftUnmute, bool IsRightUnmute);

bool AudioCoreSourceMuteStatusGet(uint8_t Index);

void AudioCoreSourceConfig(uint8_t Index, AudioCoreSource* Source);

void AudioCoreSinkEnable(uint8_t Index);

void AudioCoreSinkDisable(uint8_t Index);

bool AudioCoreSinkIsEnable(uint8_t Index);

void AudioCoreSinkConfig(uint8_t Index, AudioCoreSink* Sink);

void AudioCoreProcessConfig(AudioCoreProcessFunc AudioEffectProcess);


bool AudioCoreInit(void);

void AudioCoreDeinit(void);

void AudioCoreRun(void);

void AudioCoreSourceEraseInfo(AudioEraseSourceRecord* out_records);

void AudioFlowSourceClear(uint8_t FlowNet);

void AudioCoreSinkEraseInfo(AudioEraseSinkRecord* out_records);

void AudioCoreSourceMuteApply(uint8_t FlowNet);
void AudioCoreSinkMuteApply(uint8_t FlowNet);

#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
uint8_t AudioCoreSourceBitWidthGet(uint8_t Index);
#endif

#endif //__AUDIO_CORE_API_H__
