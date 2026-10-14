/*
 * flow_config.h
 *
 *  Created on: Dec 26, 2025
 *      Author: piwang
 */

#ifndef _EFFECTFLOW_CONFIG_H_
#define _EFFECTFLOW_CONFIG_H_

#include "app_config.h"
/*************************************************
 * 			音效图flow
 * 1、适配app_config.h功能配置
 * 2、管理音效图启用、切换列表
 * 3、音效文件:tools\merge_script\audio_effect.bin
 * 4、tools\merge_script包含音效文件版本插件和文本化配置信息
 * audio_effect_control.c落实配置
 * 图名不可重复,但单双图之间可复用
 * 每张音效图Flow有效挂载通路里至少含一路Sync = TRUE：音频帧主节拍器
 * MapList首个有效的图名是上电缺省音效图
 * 双图要点：
 * 1、帧节拍、Source/SinkFifo深度协同(@mS)
 * 2、快节拍Flow Fifo要大于慢节拍Flow帧时间,防阻塞播空或溢出
 * 3、双图Souce/Sink不可重复配置,否则挂接异常
 * 4、开CFG_FUNC_EFFECT_CORE_DUAL_EN时,Core2的Flow通路不建议含SRC/SRA/读写flash/编解码,典型如提示音
 * 5、条目左图对DefaultNet不可空；右图对SeparateNet,配mic/dac等DMA直通通路(小心：App_Soucre挂接DefaultNet)
 * Item：
 *a、如需宏管理图名，新增MAP宏指向图名,或者指向空避免占资源,再将MAP宏加入List
 *b、增加常用图名,可直接加入List。
 ***********************************************/
#ifdef CFG_192KHZ_EN
	#define	MAP_FLOW_USB_HS				"B6_USB_HS",
	#define	MAP_DUALFLOW_AEC_USB_HS		{"USB_HS_DoubleFlow", "Mic_DoubleFlow"},
#else
	#define	MAP_FLOW_USB_HS
	#define	MAP_DUALFLOW_AEC_USB_HS
#endif
#if BT_TWS_SUPPORT
	#define	MAP_FLOW_TWS				"tws",
#else
	#define	MAP_FLOW_TWS
#endif
#if (BT_LE_AUDIO_SUPPORT)
	#define	MAP_FLOW_LEAUDIO			"Le_Audio",
#else
	#define	MAP_FLOW_LEAUDIO
#endif
#ifdef FUNC_WIRELESS2_SUPPORT
	#define	MAP_FLOW_WIRELESS2			"wireless2",
#else
	#define	MAP_FLOW_WIRELESS2
#endif
#if defined(BT_HFP_SUPPORT) && (BT_HFP_SUPPORT)
	#define	MAP_FLOW_HFP				"Hfp",
	#define	MAP_DUALFLOW_HFP			{"Hfp",""},
#else
	#define	MAP_FLOW_HFP
	#define	MAP_DUALFLOW_HFP
#endif


#define	MAP_FLOW_LIST 	\
				MAP_FLOW_USB_HS 	\
				MAP_FLOW_TWS		\
				MAP_FLOW_LEAUDIO	\
				MAP_FLOW_WIRELESS2	\
				"Karaoke",			\
				"mic",				\
				MAP_FLOW_HFP		\



#define	MAP_DUALFLOW_LIST	\
				MAP_DUALFLOW_AEC_USB_HS				\
				{"music_KaraOK", "mic_KaraOK"},		\
				{"Karaoke", ""},					\
				MAP_DUALFLOW_HFP					\




/*************************************************
 * 		映射枚举列表与flow节点名字符串：
 * AUDIO_SOURCE_ENUM_MAP(XX)--音源通路全部列表
 * AUDIO_SINK_ENUM_MAP(XX)--输出音频通路全部列表
 * AUDIO_EFFECT_NAME_ENUM_MAP(XX)--应用可控的音效图内音效名称
 * AUDIO_EFFECT_PRESET_ENUM_MAP(XX)--应用可调节音效预设参数
 * 调音界面-预设消息-参数预设 管理音效图SDK调节Event
 * audio_effect_parambin.h、audio_core_api.h落实配置
 * 图与代码不匹配返回值Log，查ErrorCode@roboeffect_api.h
 * Source+Sink不可超64,缩减可节约100ByteRAM/路
 ***********************************************/
#define AUDIO_SOURCE_ENUM_MAP(XX) \
    XX(MIC_SOURCE_NUM,            "MIC_SOURCE") 			\
	XX(APP_SOURCE_NUM,            "APP_SOURCE") 			\
    XX(REMIND_SOURCE_NUM,         "REMIND_SOURCE") 			\
    XX(PLAYBACK_SOURCE_NUM,       "REC_SOURCE") 			\
    XX(I2S0_MIX_SOURCE_NUM,       "I2S0_MIX_SOURCE") 		\
    XX(I2S1_MIX_SOURCE_NUM,       "I2S1_MIX_SOURCE") 		\
	XX(I2S2_MIX_SOURCE_NUM,       "I2S2_MIX_SOURCE") 		\
    XX(USB_FS_MIX_SOURCE_NUM,     "USB_FS_MIX_SOURCE") 		\
    XX(USB_HS_MIX_SOURCE_NUM,     "USB_HS_MIX_SOURCE") 		\
    XX(LINEIN_MIX_SOURCE_NUM,     "LINEIN_MIX_SOURCE") 		\
	XX(SPDIF_MIX_SOURCE_NUM,      "SPDIF_MIX_SOURCE") 		\
	XX(TWS_SOURCE_NUM,            "TWS_SOURCE") 			\
	XX(BRIDGE_MUSIC_SOURCE_NUM,   "BRIDGE_MUSIC_SOURCE")	\
	XX(BRIDGE_MUSIC2_SOURCE_NUM,  "BRIDGE_MUSIC2_SOURCE") 	\
	XX(BRIDGE_MIC_SOURCE_NUM,     "BRIDGE_MIC_SOURCE") 		\
	XX(BRIDGE_MIC2_SOURCE_NUM,    "BRIDGE_MIC2_SOURCE") 	\
	XX(I2SPLURALCH0_SOURCE_NUM,   "I2SPLURALCH0_SOURCE") 	\
	XX(I2SPLURALCH1_SOURCE_NUM,   "I2SPLURALCH1_SOURCE") 	\
	XX(I2SPLURALCH2_SOURCE_NUM,   "I2SPLURALCH2_SOURCE") 	\
	XX(I2SPLURALCH3_SOURCE_NUM,   "I2SPLURALCH3_SOURCE") 	\
	XX(I2SPLURALCH4_SOURCE_NUM,   "I2SPLURALCH4_SOURCE") 	\
	XX(I2SPLURALCH5_SOURCE_NUM,   "I2SPLURALCH5_SOURCE") 	\
	XX(I2SPLURALCH6_SOURCE_NUM,   "I2SPLURALCH6_SOURCE") 	\
	XX(I2SPLURALCH7_SOURCE_NUM,   "I2SPLURALCH7_SOURCE")

#define AUDIO_SINK_ENUM_MAP(XX) \
    XX(AUDIO_DAC_SINK_NUM,           "DAC_SINK") 			\
	XX(AUDIO_DAC_MIX1_OUT_SINK_NUM,  "DAC1_MIX_SINK") 		\
	XX(AUDIO_DAC_MIX2_OUT_SINK_NUM,  "DAC2_MIX_SINK") 		\
	XX(AUDIO_APP_SINK_NUM,           "APP_SINK")			\
    XX(AUDIO_RECORDER_SINK_NUM,      "REC_SINK") 			\
    XX(AUDIO_STEREO_SINK_NUM,        "STEREO_SINK") 		\
    XX(AUDIO_I2S0_MIX_OUT_SINK_NUM,  "I2S0_MIX_SINK") 		\
	XX(AUDIO_I2S1_MIX_OUT_SINK_NUM,  "I2S1_MIX_SINK") 		\
	XX(AUDIO_I2S2_MIX_OUT_SINK_NUM,  "I2S2_MIX_SINK") 		\
	XX(USB_FS_MIX_SINK_NUM,			 "USB_FS_MIX_SINK") 	\
    XX(USB_HS_MIX_SINK_NUM,			 "USB_HS_MIX_SINK") 	\
    XX(AUDIO_SPDIF0_SINK_NUM,        "SPDIF0_SINK") 		\
    XX(AUDIO_SPDIF1_SINK_NUM,        "SPDIF1_SINK") 		\
	XX(LE_AUDIO_SINK_NUM,    	 	 "LE_SINK") 			\
    XX(TWS_SINK_NUM,    	 	     "TWS_SINK") 			\
	XX(BRIDGE_MUSIC_SINK_NUM,    	 "BRIDGE_MUSIC_SINK") 	\
	XX(BRIDGE_MUSIC2_SINK_NUM,    	 "BRIDGE_MUSIC2_SINK") \
	XX(BRIDGE_MIC_SINK_NUM,    	 	 "BRIDGE_MIC_SINK") 	\
	XX(BRIDGE_MIC2_SINK_NUM,    	 "BRIDGE_MIC2_SINK") 	\
	XX(I2SPLURALCH0_SINK_NUM,        "I2SPLURALCH0_SINK") 	\
	XX(I2SPLURALCH1_SINK_NUM,    	 "I2SPLURALCH1_SINK") 	\
	XX(I2SPLURALCH2_SINK_NUM,    	 "I2SPLURALCH2_SINK") 	\
	XX(I2SPLURALCH3_SINK_NUM,    	 "I2SPLURALCH3_SINK") 	\
	XX(I2SPLURALCH4_SINK_NUM,        "I2SPLURALCH4_SINK") 	\
	XX(I2SPLURALCH5_SINK_NUM,    	 "I2SPLURALCH5_SINK") 	\
	XX(I2SPLURALCH6_SINK_NUM,    	 "I2SPLURALCH6_SINK") 	\
	XX(I2SPLURALCH7_SINK_NUM,    	 "I2SPLURALCH7_SINK")


#define AUDIO_EFFECT_NAME_ENUM_MAP(XX) \
    XX(EFFECT_MUSICGAIN,		"music_gain") \
    XX(EFFECT_MICGAIN,			"mic_gain") \
    XX(EFFECT_PREGAIN,			"preGain") \
	XX(EFFECT_REMIND_GAIN,		"remind_gain") \
    XX(EFFECT_MUSICEQ,			"music_EQ") \
    XX(EFFECT_MICEQ,			"mic_EQ") \
	XX(EFFECT_RECGAIN,			"rec_gain") \
    XX(EFFECT_MUSICSNDET,		"silence_detector_music") \
    XX(EFFECT_MICSNDET,			"silence_detector_mic") \
	XX(EFFECT_ECHO,				"echo0") \
	XX(EFFECT_REVERB,			"reverb_plate_me0")
	//要设置的目标音效

// === 全参预设表（param_index = 0xff，效果器全部参数一次性写入）===
#define AUDIO_EFFECT_PRESET_FULLPARAM_ENUM_MAP(XX) \
    XX(PRESET_EQ_FLAT,				"FLAT") \
    XX(PRESET_EQ_CLASSIC,			"CLASSIC") \
    XX(PRESET_EQ_POP,				"POP") \
    XX(PRESET_EQ_ROCK,				"ROCK") \
    XX(PRESET_EQ_JAZZ,				"JAZZ") \
    XX(PRESET_EQ_VOCALBOOST,		"VOCAL_BOOST")

// === 个参预设表（param_index = 具体参数索引，单个参数独立调节）===
#define AUDIO_EFFECT_PRESET_SINGLEPARAM_ENUM_MAP(XX) \
    XX(PRESET_EQ_BASS_VOL,			"BASS") \
	XX(PRESET_EQ_TREB_VOL,			"TREB") \
    XX(PRESET_GAIN_VOL,				"VOL") \
	XX(PRESET_GAIN_BT_VOL,			"BT_VOL") \
	XX(PRESET_ECHO_delay,			"ECHO_delay") \
	XX(PRESET_ECHO_attenuation,		"ECHO_attenuation") \
	XX(PRESET_reverb_decay,			"reverb_decay") \
	XX(PRESET_reverb_wetdry,		"reverb_wetdry")

// === 全量预设组合表 ===
#define AUDIO_EFFECT_PRESET_ENUM_MAP(XX) \
    AUDIO_EFFECT_PRESET_FULLPARAM_ENUM_MAP(XX) \
    AUDIO_EFFECT_PRESET_SINGLEPARAM_ENUM_MAP(XX)

#endif /* _EFFECTFLOW_CONFIG_H_ */
