/*
 * audio_core_adapt.h
 *
 *  Created on: Mar 11, 2021
 *      Author: piwang
 */

#ifndef BT_AUDIO_APP_SRC_INC_AUDIO_CORE_ADAPT_H_
#define BT_AUDIO_APP_SRC_INC_AUDIO_CORE_ADAPT_H_
#include "type.h"
#include "timeout.h"
#include "resampler_polyphase.h"
#include "resampler.h"
#include "mcu_circular_buf.h"
#include "resampler_farrow.h"
#include "src.h"
#include "app_config.h"
#if defined(AUDIOCORE_HWSRC_EN)
#include "reset.h"
#endif
#define DefaultNet		0
#define SeparateNet		1
#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
	#define MaxNet		2
#else
	#define MaxNet		1
#endif

typedef enum _AUDIO_ADAPT_
{
	STD = 0,				//one frame /Direct
	SRC_ONLY,				//ONLY 软件转采样
	SRA_ONLY,				//ONLY 软件采样率微调
	CLK_ADJUST_ONLY,		//ONLY 硬件采样率微调
	SRC_SRA,				//软件转采样+软件采样率微调
	SRC_ADJUST,				//软件转采样+硬件采样率微调
#if defined(AUDIOCORE_HWSRC_EN)
	HWSRC_ONLY,				//ONLY 硬件SRC
	HWSRC_SRA,				//硬件转采样+软件采样率微调
	HWSRC_ADJUST,			//硬件转采样+硬件采样率微调
#endif
} AUDIO_ADAPT;
#if defined(AUDIOCORE_HWSRC_EN)
	#define AUDIO_ADAPT_MAX		9
#else
	#define AUDIO_ADAPT_MAX		6
#endif

typedef struct _SRC_ADAPTER_
{
	uint32_t					SampleRate;
	MCU_CIRCULAR_CONTEXT		SrcBufHandler;
	uint8_t						SrcQuality;//0~10:better SNR but higher Mcps/memory
#if defined(AUDIOCORE_HWSRC_EN)
	uint8_t						HwSrcIndex;//< RES_SRC_NUM && HwSrcCase双确认
#endif
#if defined(CFG_FUNC_SRC_SINC_EN)
	uint8_t *					SincCt;
#elif defined(CFG_FUNC_SRC_POLYPHASE_EN)
	ResamplerPolyphaseContext	SrcCt;
#endif

} SRC_ADAPTER;

typedef struct _SRA_ADAPTER_
{
	bool						Enable;//使能开关，通路开启时有效
	ResamplerFarrowContext		SraResFarCt;
	MCU_CIRCULAR_CONTEXT		SraBufHandler;
	uint32_t					TotalNum;//微调水位统计--旨在多次求平均
	uint16_t					Count;//微调水位统计计数器
	int8_t						AdjustVal;//软件微调 插点和删点数值，源于周期性水位判断
	int8_t						GroupAdj;//多帧微调值@ADJUST_GROUP_FRAME,帧组合期间不可变更参数
	uint32_t					ValidDepth;//基于Mix samplerate通路深度 - Framelen
	uint8_t						HighLevelCent;
	uint8_t						LowLevelCent;
} SRA_ADAPTER;

typedef struct _CLK_ADJUST_ADAPTER
{
	bool						Enable;//使能开关，通路开启时有效
	uint32_t					TotalNum;//微调水位统计--旨在多次求平均
	uint16_t					Count;//微调水位统计计数器
	uint32_t					ValidDepth;//基于Mix samplerate通路深度 - Framelen
	uint8_t						HighLevelCent;
	uint8_t						LowLevelCent;
	uint32_t					LastLevel;//前一次平均水位
	int16_t						RiseTimes;//连续上涨次数或下跌次数
	int16_t						AdjustVal;//分频寄存器小数部分
	uint32_t					AdjTickVal;//HW Adj监测周期起始time
} CLK_ADJUST_ADAPTER;

typedef struct _AudioCoreIO_
{
	uint8_t						Channels;//1/2 注意：init必须配置最大值，通道续存期间可以随音源改变。
	bool						Sync; //TRUE:每帧同步，不足时混音通路阻塞。 FALSE:帧操作，Source数据不足时填零，Sink：空间不足时丢弃
	uint8_t						Net; //缺省：DefaultNet，混音通路分组
	AUDIO_ADAPT					Adapt; //缺省:STD,通路采样和微调功能设置
	uint8_t						SrcQuality;//0~10:better SNR but higher Mcps/memory
#if defined(AUDIOCORE_HWSRC_EN)
	uint8_t						HwSrcIndex;//< RES_SRC_NUM 由HwSrcIdleGet获取确认
#endif
	uint8_t						PcmWidth;
	void						*DataIOFunc;//音频输入输出 必须配置  @sample
	void						*LenGetFunc;//source数据长度 sink空间长度 必须配置 @sample
	//微调参数设置，Adapt不支持微调时缺省0
	uint8_t						HighLevelCent;//高水位%：建议60
	uint8_t						LowLevelCent;//低水位%：建议40
	uint32_t					Depth;//LenGetFunc()最大值 @sample
	//转采样参数，Adapt不支持转采样时缺省0
	uint32_t					SampleRate;//DataIOFunc()所在fifo数据的采样率
	bool						Resident;//sink，不跟随应用频繁申请释放帧buf时 设TRUE
} AudioCoreIO;//音频通路接口参数 组合

#if defined(AUDIOCORE_HWSRC_EN)
/**下列硬件SRC配置场景,可自行扩展**/
typedef enum _HWSRC_CASE
{
	//0 ~ 0x7f:Adapt分配Audiocore soucre/sink归一化编号
	HWSRC_IDLE = 0x80,//
	HWSRC_MODE_SPEAKER,
	HWSRC_MODE_USBMIC,
	HWSRC_MODE_MAX,
	HWSRC_HS_MIX_SPEAKER,
	HWSRC_HS_MIX_USBMIC,
	HWSRC_BRIDGE, //典型for AEC 192K->16K
	HWSRC_CASE_MAX,//
} HWSRC_CASE;

typedef struct _AdaptHwSrc
{
	uint8_t			SrcCase;//HwSrc挂接登记 @HWSRC_CASE
	SRC_Context		SrcContext;
	uint16_t		SampleLenStartIn;
}AdaptHwSrc;
#endif
typedef struct _AdaptHwResource
{
	//Hw adjust
	TIMER			AdjTimer;//for HW Mclk Adjust
	uint8_t			AdjustIO;//HW Mclk Adjust Active source/Sink
#if defined(AUDIOCORE_HWSRC_EN)
	//Hw Src
	AdaptHwSrc		HwSrcCt[RES_SRC_NUM];//HwSrc管理登记
#endif
}AdaptHwResource;

extern AdaptHwResource AdaptHwCt;

/**source/sink 归一IO编号,source低，sink高**/
#define SOURCE_GET_IO(Source)				(Source)
#define IO_GET_SOURCE(Index)				((Index < AUDIO_CORE_SOURCE_MAX_NUM) ? Index : (AUDIO_CORE_SOURCE_MAX_NUM))
#define SINK_GET_IO(Sink)					(AUDIO_CORE_SOURCE_MAX_NUM + Sink)
#define IO_GET_SINK(Index)					((Index >= AUDIO_CORE_SOURCE_MAX_NUM) ? (Index - AUDIO_CORE_SOURCE_MAX_NUM) : (AUDIO_CORE_SINK_MAX_NUM))
#define IO_IS_SOURCE(Index)					(Index < AUDIO_CORE_SOURCE_MAX_NUM)
#define IO_IS_SINK(Index)					((Index >= AUDIO_CORE_SOURCE_MAX_NUM) && (Index < (AUDIO_CORE_SOURCE_MAX_NUM + AUDIO_CORE_SINK_MAX_NUM)))

//使用通路 位段组合和MASK
#define SOURCE_BIT_EN(Value, Index) 		(Value) |= (1ull<<(Index))
#define SOURCE_BIT_DIS(Value, Index)		(Value) &= ~(1ull<<(Index))
#define	SOURCE_BIT_GET(Value, Index)		((((uint64_t)(Value)) >> (Index)) & 0x01)
#define	SOURCE_BIT_MASK(Value)				((Value) & ((1ull<<AUDIO_CORE_SOURCE_MAX_NUM) - 1))
#define SINK_BIT_EN(Value, Index) 			(Value) |= (1ull<<(SINK_GET_IO(Index)))
#define SINK_BIT_DIS(Value, Index)			(Value) &= ~(1ull<<(SINK_GET_IO(Index)))
#define	SINK_BIT_GET(Value, Index)			((((uint64_t)(Value)) >> (SINK_GET_IO(Index))) & 0x01)
#define SINK_BIT_MASK(Value)				((Value) & (((1ull<<(AUDIO_CORE_SOURCE_MAX_NUM + AUDIO_CORE_SINK_MAX_NUM)) - 1) - ((1ull<<AUDIO_CORE_SOURCE_MAX_NUM) - 1)))

//#define SOURCES(Index)	(AudioCore.AudioSource[Index])
//#define SINKS(Index)	(AudioCore.AudioSink[Index])
#define SOURCEFRAME(Index)					(AudioCore.NetCt[AudioCore.AudioSource[Index].Net].FrameSize)
#define SINKFRAME(Index)					(AudioCore.NetCt[AudioCore.AudioSink[Index].Net].FrameSize)
#define SOURCESAMPLERATE(Index)				(AudioCore.NetCt[AudioCore.AudioSource[Index].Net].FrameSize)
#define SINKSAMPLERATE(Index)				(AudioCore.NetCt[AudioCore.AudioSink[Index].Net].FrameSize)
#define SOURCEREADY(Index)					(AudioCore.NetCt[AudioCore.AudioSource[Index].Net].FrameReady)
#define SINKREADY(Index)					(AudioCore.NetCt[AudioCore.AudioSink[Index].Net].FrameReady)

#define ADAPTER_MAX_CHG 					2
#define ADAPTER_INPUT_BLOCK					128

#define POLYNOMIAL_ORDER	 				1//2// (1~4 @resampler_farrow.h SRA)

#define SRC_QUALITY							0//2//3//(0~10 @resample_sinc.h SRC)

#define	ADJUST_PERIOD						(512*3)		//以采样点计数 监测消抖周期，平衡波动和速度，SRA:增减一点的周期,对应频繁幅度
#define ADJUST_APLL_PERIOD					(20)		//mS 监测消抖周期，用于硬件微调。
//#define ADJUST_APLL_STEP					(32)			//调整步长，分频精度为单位。
#define ADJUST_APLL_IDLE					(AUDIO_CORE_SOURCE_MAX_NUM + AUDIO_CORE_SINK_MAX_NUM)		//硬件微调通路缺省标记
#define ADJUST_DIV_MAX						(378)//(400)			//硬件微调频偏限幅，源于分频小数。
#define	ADJUST_SHRESHOLD					2			//2~4 Low～high之间 抑制硬件微调的震荡，连续单向水位变化，反向抑制。
#define ADJUST_GROUP_FRAME(AdjustVal)		(AdjustVal == 0 ? 1 : 10) //(AdjustVal == 0 ? 1 : ADJUST_PERIOD/ADAPTER_INPUT_BLOCK)
#if defined(AUDIOCORE_HWSRC_EN)
#define	HWSRC_BUF_TIME						6000//uS,基于采样点时隙申请in/out Buf
#define HWSRC_MIN_TIME						1100//最小数据处理节拍
#endif
#define SRC_INPUT_MIN						8//低于这两个数值，不做转换处理 减少累积误差
#define SRC_OUPUT_MIN						4
#define SRC_OUPUT_JITTER					3//输出采样点偏差 正常是舍入偏差1点， 第一帧可能delay引入2点左右


#define SRC_SCALE_MAX						(12)			//192000	<--	16000 RESAMPLER_POLYPHASE_SRC_RATIO_6_1  @file	resampler_polyphase.h
#define	SRC_SCALE_MIN						(147/640)	 //44100 <-- 192000	RESAMPLER_POLYPHASE_SRC_RATIO_147_640

#define SRC_FIFO_SIZE(FRAME)				(FRAME + ADAPTER_INPUT_BLOCK + SRC_INPUT_MIN * SRC_SCALE_MAX + SRC_OUPUT_JITTER)

//预防帧小时接口fifo配2帧 微调Low 40%故障，要求后级fifo要放大
#define SRA_FIFO_SIZE(FRAME)				(((FRAME) > ADAPTER_INPUT_BLOCK) ? ((FRAME) + ADAPTER_INPUT_BLOCK + ADAPTER_MAX_CHG + 1) : ((FRAME) + ADAPTER_INPUT_BLOCK * 2))

extern const uint32_t AudioCoreSampleRateTBL[];
#define SAMPLERATE_TBL_CNT					(sizeof(AudioCoreSampleRateTBL) / sizeof(uint32_t))

//配置通路参数，申请所需buf
bool AudioCoreSourceInit(AudioCoreIO * AudioIO, uint8_t Index);

//清理参数，释放动态buf
void AudioCoreSourceDeinit(uint8_t Index);

//配置通路参数，申请所需buf
bool AudioCoreSinkInit(AudioCoreIO * AudioIO, uint8_t Index);

//清理参数，释放动态buf
void AudioCoreSinkDeinit(uint8_t Index);

//实现audiocore适配层Adapter数据迁移，数据帧监测
void AudioCoreIOLenProcess(uint8_t FlowNet);

//混音通路 同步检测，驱动不同混音通路组合 进入AudioProcessMain
bool AudioCoreSyncing(uint8_t FlowNet);

//通路节拍处理输出
bool AudioCoreSynced(uint8_t FlowNet);

/**多通路硬件微调协同监测**/
void AudioCoreAdjustIORenew(void);

//水位监测最大深度标定
void AudioCoreSourceValidDepthRenew(uint8_t Index);

//水位监测最大深度标定
void AudioCoreSinkValidDepthRenew(uint8_t Index);

//警告：通路存在mono/Stereo切换时, init时必须配Stereo,关联fifo size
void AudioCoreSourceChange(uint8_t Index, uint8_t Channels, uint32_t SampleRate);

//警告：通路存在mono/Stereo切换时, init时必须配Stereo,关联fifo size
void AudioCoreSinkChange(uint8_t Index, uint8_t Channels, uint32_t SampleRate);

void AudioCoreSourceAdapterReset(uint8_t Index);

void AudioCoreSinkAdapterReset(uint8_t Index);

//调整LenGetFunc()对应的最大值  @sample
void AudioCoreSourceDepthChange(uint8_t Index, uint32_t NewDepth);

//调整LenGetFunc()对应的最大值  @sample
void AudioCoreSinkDepthChange(uint8_t Index, uint32_t NewDepth);

//通路已配置，实际以帧buf申请与否为基准
bool AudioCoreSourceIsInit(uint8_t Index);

bool AudioCoreSinkIsInit(uint8_t Index);

//通路微调开关，需init时Adapt类型及其参数配合，通路关闭时不生效。
void AudioCoreSourceAdjust(uint8_t Index, bool OnOff);

//通路微调开关，需init时Adapt类型及其参数配合，通路关闭时不生效。
void AudioCoreSinkAdjust(uint8_t Index, bool OnOff);

bool AudioCoreSinkAdjustIsEnable(uint8_t Index);

//mixnet采样率、帧长、位宽设置, 不带合法性自检
void AudioCoreMixParamSet(uint8_t FlowNet, uint32_t SampleRate, uint16_t FrameSize, uint8_t Width);

uint16_t AudioCoreFrameSizeGet(uint8_t FlowNet);

uint32_t AudioCoreMixStepUsGet(uint8_t MixNet);

uint32_t AudioCoreMixSampleRateGet(uint8_t FlowNet);

/****************************************************
 * 重要：通路Init挂载后才可使用，否则是假信息;挂载前使用音效图获取 FlowNet
 * AudioCoreGetNetBySourceChannelNUM(Index)
 * AudioCoreGetNetBySinkChannelNUM(Index)
 **************************************************/
uint8_t AudioCoreSourceBitWidthGet(uint8_t Index);

uint8_t AudioCoreSinkBitWidthGet(uint8_t Index);

uint8_t AudioCoreSourceMixNetGet(uint8_t Index);

uint8_t AudioCoreSinkMixNetGet(uint8_t Index);

uint16_t AudioCoreSourceFrameSizeGet(uint8_t Index);

uint16_t AudioCoreSinkFrameSizeGet(uint8_t Index);

uint32_t AudioCoreSourceMixSampleRateGet(uint8_t Index);

uint32_t AudioCoreSinkMixSampleRateGet(uint8_t Index);


/**第一音频帧时，需预填,避免OS mS波动引起播空,fifo重置时可配 **/
void AudioCoreSinkAudioPreset(uint8_t Index);

/**Mixnet/音效图采样率/帧长/位宽合法性**/
bool MixSupport(uint32_t SampleRate, uint16_t FrameSize, uint8_t Width);

#if defined(AUDIOCORE_HWSRC_EN)
void HwSrcAppModePreSet(uint8_t AppMode);

void HwSrcAppModeClear(uint8_t AppMode);

/**return：0 ~ RES_SRC_NUM - 1有效; HWSRC_IDLE:无效**/
uint8_t HwSrcIdleGet(uint8_t AudioNum, bool IsSource);
#endif

/**调音工具第二档显示Default帧Mcps,双图分双核时显示Core2帧Mcps**/
uint16_t FrameMcps_Communication(void);

void FrameMcpsLog(void);
#endif /* BT_AUDIO_APP_SRC_INC_AUDIO_CORE_ADAPT_H_ */
