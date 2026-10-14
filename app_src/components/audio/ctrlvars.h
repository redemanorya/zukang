/**
 **************************************************************************************
 * @file    ctrlvars.h
 * @brief   Control Variables Definition
 * 
 * @author  Aissen Li
 * @version V1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
 
#ifndef __CTRLVARS_H__
#define __CTRLVARS_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#include <stdint.h>
#include "sys.h"
#include "clk.h"
#include "timer.h"
#include "irqn.h"
#include "i2s_interface.h"
#include "adc_interface.h"
#include "audio_effect_library.h"
#include "app_config.h"
#include "bt_config.h"
#include "audio_core_api.h"
#include "rtos_api.h"

//#define  MAX_MIC_DIG_STEP               (16)
//#define  MAX_MUSIC_DIG_STEP             (16)
//#define  MAX_MIC_EFFECT_DELAY_STEP      (16)

#if 0
extern int16_t EqMode;
extern int16_t MicBassStep;
extern int16_t MicTrebStep;
extern int16_t MicVolume;
extern int16_t MusicBassStep;
extern int16_t MusicTrebStep;
extern int16_t MusicVolume;
extern int16_t ReverbStep;
#endif

#if 0  //2026.1.7
typedef enum _EFFECT_MODE
{
	EFFECT_MODE_DEFAULT = 0,
	/**********uac mode****************/
	EFFECT_MODE_UAC,
	/**********two audio mode****************/
	EFFECT_MODE_TWO_AUDIO,
	/**********mic mode****************/
	EFFECT_MODE_MIC,
	/**********music mode**************/
	EFFECT_MODE_MUSIC,
	/**********micusbAI mode***********/
	EFFECT_MODE_MICUSBAI,
	/**********bypass mode*************/
	EFFECT_MODE_BYPASS,
	/**********hfp mode****************/
	EFFECT_MODE_HFP_AEC,
	/**********karaoke mode************/
    EFFECT_MODE_HunXiang,
    EFFECT_MODE_DianYin,
    EFFECT_MODE_MoYin,
    EFFECT_MODE_HanMai,
    EFFECT_MODE_NanBianNv,
    EFFECT_MODE_NvBianNan,
    EFFECT_MODE_WaWaYin,

	EFFECT_MODE_BIS,
	EFFECT_MODE_LE_AUDIO,
	EFFECT_MODE_LE_SINK_AUDIO,
	/**********tdm mode**************/
	EFFECT_MODE_TDM,

	EFFECT_MODE_COUNT,
    //User can add other effect mode
} EFFECT_MODE;
#endif

#if 0  //2026.1.7
//实际有效的音效轮转模式数量
#if (BT_HFP_SUPPORT)
#define	EFFECT_MODE_NUM_ACTIVCE 	2
#else
#define	EFFECT_MODE_NUM_ACTIVCE 	1
#endif

typedef enum _REMIND_TYPE
{
    REMIND_TYPE_KEY,
	REMIND_TYPE_BACKGROUND, 

} REMIND_TYPE;
#endif

#if 0
typedef enum _EQ_MODE
{
    EQ_MODE_FLAT,
	EQ_MODE_CLASSIC,	
	EQ_MODE_POP,
	EQ_MODE_ROCK,
	EQ_MODE_JAZZ,
	EQ_MODE_VOCAL_BOOST,
} EQ_MODE;
#endif

//for ADC0 PGA      0x03
typedef struct _ADC0PGAContext
{
	uint8_t	  			sync_head;
	uint8_t	  			len;
    uint16_t            MICL_EN;
    uint16_t            MICR_EN;
    uint16_t            LineinL_EN;
    uint16_t            LineinR_EN;
    uint16_t            MICL_Mode;
    uint16_t            MICR_Mode;
    uint16_t            MICL_Single_Gain;
    uint16_t            MICR_Single_Gain;
    uint16_t			MICL_DIFF_Gain;
    uint16_t            MICR_DIFF_Gain;
    uint16_t            LineinL_Gain;
    uint16_t            LineinR_Gain;
}ADC0PGAContext;

//for ADC0 DIGITAL  0x04
typedef struct _ADCDigitalContext
{
	uint8_t	  			sync_head;
	uint8_t	  			len;
    uint16_t         	Enable;
	uint16_t  			Sample_Rate;
    uint16_t  			CLK_Source;
    uint16_t  			Channel_Swap;
    uint16_t  			DC_Block_Enable;
	uint16_t  			L_Volume;
	uint16_t  			R_Volume;
	uint16_t            L_Mute;
	uint16_t            R_Mute;
}ADCDigitalContext;

typedef struct _ADCAGCContext
{
	uint8_t	  			sync_head;
	uint8_t	  			len;
	uint16_t            AGC_EN;
	uint16_t            Max_Level;
	uint16_t            Target_Level;
	uint16_t            Max_Gain;
	uint16_t            Min_Gain;
	uint16_t            Gain_Offset;
	uint16_t            Frame_Time;
	uint16_t            Hold_N_Frame;
	uint16_t            Attack_Time;
	uint16_t            Decay_Time;
	uint16_t            NG_Threshold;
	uint16_t            NG_Rref;
	uint16_t            NG_Hold_N_Frame;
}ADCAGCContext;

typedef struct _ADC1PGAContext
{
	uint8_t	  			sync_head;
	uint8_t	  			len;
    uint16_t            MICL_EN;
    uint16_t            MICR_EN;
    uint16_t            AUX0L_B20_EN;
    uint16_t            AUX0R_B21_EN;
    uint16_t            AUX1L_B18_EN;
    uint16_t            AUX1R_B19_EN;
    uint16_t            MICL_Mode;
    uint16_t            MICR_Mode;
    uint16_t            MICL_Single_Gain;
    uint16_t            MICR_Single_Gain;
    uint16_t            MICL_DIFF_Gain;
    uint16_t            MICR_DIFF_Gain;
    uint16_t            AUX0L_B20_Gain;
    uint16_t            AUX0R_B21_Gain;
    uint16_t			AUX1L_B18_Gain;
    uint16_t            AUX1R_B19_Gain;
}ADC1PGAContext;

typedef struct _DAC0_1Context
{
	uint8_t	  			sync_head;
	uint8_t	  			len;
	uint16_t            Enable;
	uint16_t            DAC_Mode;
	uint16_t            Sample_Rate;
	uint16_t            CLK_Source;
	uint16_t            Dither_En;
	uint16_t            Dither_ValueH15;
	uint16_t            Dither_ValueL8;
	uint16_t            Scramble_En;
	uint16_t            Scramble_Mode;
	uint16_t            Out_Mode;
	uint16_t            L_Volume;
	uint16_t            R_Volume;
	uint16_t            L_Mute;
	uint16_t            R_Mute;
}DAC0_1Context;

typedef struct _DACxContext
{
	uint8_t	  			sync_head;
	uint8_t	  			len;
	uint16_t            Enable;
	uint16_t            DAC_Mode;
	uint16_t            Sample_Rate;
	uint16_t            CLK_Source;
	uint16_t            Dither_En;
	uint16_t            Dither_ValueH15;
	uint16_t            Dither_ValueL8;
	uint16_t            Scramble_En;
	uint16_t            Scramble_Mode;
	uint16_t            Volume;
	uint16_t            Mute;
}DACxContext;

typedef struct _I2S0Context
{
	uint8_t	  			sync_head;
	uint8_t	  			len;
	uint16_t            TX_Enable;
	uint16_t            RX_Enable;
	uint16_t            Sample_Rate;
	uint16_t            CLK_Source;
	uint16_t            Mode;
	uint16_t            Format;
	uint16_t            Wordlth;
	uint16_t            BCLK_INV;
	uint16_t            LRCLK_INV;
	uint16_t            TX_Mute;
}I2S0Context;

typedef struct _I2S1_2Context
{
	uint8_t	  			sync_head;
	uint8_t	  			len;
	uint16_t            TX_Enable;
	uint16_t            RX_Enable;
	uint16_t            Sample_Rate;
	uint16_t            CLK_Source;
	uint16_t            Mode;
	uint16_t            Format;
	uint16_t            TDM_Type;
	uint16_t            TDM_Amount_Cnt;
	uint16_t            Wordlth;
	uint16_t            BCLK_INV;
	uint16_t            LRCLK_INV;
	uint16_t            Dual_Mode_En;
	uint16_t            Quad_Mode_En;
	uint16_t            TX_Mute;
}I2S1_2Context;

typedef struct _SPDIFContext
{
	uint8_t	  			sync_head;
	uint8_t	  			len;
	uint16_t            Enable;
	uint16_t            Sample_Rate;
	uint16_t            Channle_Mode;
	uint16_t            IO_Select;
	uint16_t            Lock_Status;
}SPDIFContext;

typedef struct _HardwareConfigContext
{
	//for ADC0 PGA      0x03
	ADC0PGAContext		ADC0PGACt;

    //for ADC0 DIGITAL  0x04
	ADCDigitalContext	ADC0DigitalCt;

	//for AGC0 ADC0     0x05
	ADCAGCContext		ADC0AGCCt;

	//for ADC1 PGA      0x06
	ADC1PGAContext		ADC1PGACt;

	//for ADC1 DIGITAL  0x07
	ADCDigitalContext	ADC1DigitalCt;

	//for AGC1  ADC1    0x08
	ADCAGCContext		ADC1AGCCt;

	//for DAC0          0x09
	DAC0_1Context		DAC0Ct;
	//for DAC1          0x0a
	DAC0_1Context       DAC1Ct;
	//for DAC2			0x0b
	DACxContext         DAC2Ct;

						//0x0c
	I2S0Context			I2S0Ct;
						//0x0d
	I2S1_2Context		I2S1Ct;
						//0x0e
	I2S1_2Context		I2S2Ct;

						//0x0F
	SPDIFContext		SPDIF0Ct;
						//0x10
	SPDIFContext		SPDIF1Ct;
}HardwareConfigContext;

typedef enum _AutoRefreshType
{
	AutoRefresh_NONE = 0,			//不需要刷新
	AutoRefresh_ALL_PARA = 1,		//刷新0x00开始的所有参数
	AutoRefresh_ALL_EFFECTS_PARA = 2,//刷新0x81开始所有音效参数

	//3-255 刷新指定地址的参数
	AutoRefresh_0x03_PARA,
	AutoRefresh_0x04_PARA,
}AutoRefreshType;

//-----system var--------------------------//
typedef struct _ControlVariablesContext
{
	//for system control 0x01
	//for System status 0x02
	AutoRefreshType 			AutoRefresh;
	MessageHandle 				RefreshMsgHandle;

	HardwareConfigContext HwCt;

    //for system define

	uint16_t            sample_rate_index;
	uint32_t            sample_rate;

	#ifdef CFG_FUNC_DETECT_MIC_SEG_EN
	uint8_t             MicSegment;
	#endif
	#ifdef CFG_FUNC_DETECT_PHONE_EN
	uint8_t             EarPhoneOnlin;
	#endif
}ControlVariablesContext;

#ifdef CFG_I2S_SLAVE_TO_SPDIFOUT_EN
typedef struct _SyncModuleContext
{
	uint64_t gI2sBclkCnt;
	uint64_t gDacMclkCnt;
	uint8_t  gRefreshFlag;
	uint8_t  gClearDoneFlag;
	uint64_t gMClkFromBCLK;
	uint64_t gMClkFromDPLL;
	uint32_t gSyncTimerCnt;
	uint16_t defVal_bak;
	uint16_t dat;
	uint16_t clkRatio;
}SyncModuleContext;

#define SYNC_TIMER_INDEX        TIMER4  //定时器
#define SYNC_TIMER_OUT_VALUE    100000 //定时器的间隔值，单位us
#define BCLK_MCLK_RATIO         4       //MCLK和BCLK的比值， 48K与12.288M是4， 44.1与11.2896M是4.。。
#define MCLK_DIV_VALUE          20

extern SyncModuleContext gSyncModule;
#endif
extern ControlVariablesContext gCtrlVars;

extern const uint16_t HPCList[3];

uint16_t SampleRateIndexGet(uint32_t SampleRate);
void CtrlVarsInit(void);
void DefaultParamgsInit(void);
void Line3MicPinSet(void);
void UsbLoadAudioMode(uint16_t len,uint8_t *buff);
void AudioAnaChannelApply(int8_t ana_input_ch);
void AudioLineChannelSet(int8_t ana_input_ch);
void AudioMicChannelSet(void);

// 将 gCtrlVars 中的音频硬件配置全量同步到寄存器
// 涵盖 ADC PGA/AGC/Digital Volume、DAC Volume/Scramble/Dither 等
void AudioCodecHardwareSync(void);

#ifdef CFG_I2S_SLAVE_TO_SPDIFOUT_EN
void SyncModule_Init(void);
void SyncModule_Reset(void);
void SyncModule_Get(void);
void SyncModule_Process(void);
#endif

extern const int16_t DigVolTab_32[32];
extern const int16_t DigVolTab_16[16];


#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__CTRLVARS_H__

