/**
 *******************************************************************************
 * @file    dac.h
 * @author  Mike
 * @version V1.0.0
 * @date    2025/01/16
 * @brief
 *******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2014 MVSilicon </center></h2>
 */

/**
 * @addtogroup DAC
 * @{
 * @defgroup dac dac.h
 * @{
 */

#ifndef __DAC_H__
#define __DAC_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

/**
 * DAC Module Select
 */
typedef enum _AUDIODAC_MODULE
{
	DAC0_MODULE = (1 << 0),  //1
	DAC1_MODULE = (1 << 1),  //2
	DAC2_MODULE = (1 << 2),  //4
	DAC0_DAC1_MODULE = DAC0_MODULE | DAC1_MODULE,  //3
	DAC0_DAC2_MODULE = DAC0_MODULE | DAC2_MODULE,  //5
	DAC1_DAC2_MODULE = DAC1_MODULE | DAC2_MODULE,  //6
	DAC0_DAC1_DAC2_MODULE = DAC0_MODULE | DAC1_MODULE | DAC2_MODULE  //7
}AUDIODAC_MODULE;

/* Preprocessor-visible aliases (enum 不能被预处理器识别) */
#define DAC0_MODULE_VAL  (1 << 0)
#define DAC1_MODULE_VAL  (1 << 1)
#define DAC2_MODULE_VAL  (1 << 2)
#define DAC0_DAC1_MODULE_VAL    (DAC0_MODULE_VAL | DAC1_MODULE_VAL)
#define DAC0_DAC2_MODULE_VAL    (DAC0_MODULE_VAL | DAC2_MODULE_VAL)
#define DAC1_DAC2_MODULE_VAL    (DAC1_MODULE_VAL | DAC2_MODULE_VAL)
#define DAC0_DAC1_DAC2_MODULE_VAL  (DAC0_MODULE_VAL | DAC1_MODULE_VAL | DAC2_MODULE_VAL)

typedef enum _SCRAMBLE_MODULE
{
	DWA = 0,
	RANDOM_DWA = 1,
	POS_NEG = 2
}SCRAMBLE_MODULE;

typedef enum _AUDIODAC_DOUT_MODE
{
    MODE0 = 0,// L_o=L_i;R_o=R_i;
    MODE1 = 1,// L_o=R_i;R_o=L_i; L R swap;
    MODE2 = 2,// L_o=(L_i+R_i)/2;R_o=(L_i+R_i)/2;
    MODE3 = 3,// L_o=(L_i+R_i)/2;R_o=-(L_i+R_i)/2;
}AUDIODAC_DOUT_MODE;

typedef enum _AUDIODAC_DOUT_WIDTH
{
	DAC_16BIT = 0,
	DAC_24BIT_HIGHALIGN = 2,
	DAC_24BIT_LOWALIGN = 3,
}AUDIODAC_BitWidth;

typedef enum _EDGE
{
	POSEDGE = 0,
	NEGEDGE = 1
}EDGE;

typedef enum _AUDIODAC_Mode
{
	DAC_Single,   //单端
	DAC_Diff      //差分
} AUDIODAC_Mode;

typedef enum _AUDIODAC_LoadStatus
{
	DAC_NOLoad = 0, //不带负载
	DAC_Load     	//带负载
} AUDIODAC_LoadStatus;

typedef enum PVDD_Model
{
	PVDD16 = 0, //使用1.6V电源
	PVDD33     	//使用3.3V电源
} PVDD_Model;

typedef enum _AUDIODAC_EnergyModel
{
	DACLowEnergy = 0, //低功耗模式
	DACCommonEnergy
} AUDIODAC_EnergyModel;

typedef struct __AUDIODAC_ParamCt
{
	uint32_t 				SampleRate;
	AUDIODAC_Mode 			DACMode;
	AUDIODAC_LoadStatus 	DACLoadStatus;
	AUDIODAC_EnergyModel 	DACEnergyModel;
	AUDIODAC_BitWidth 		DACBitWidth;
	AUDIODAC_DOUT_MODE 		DoutMode;
	void *					DacFifo;
	uint16_t         		DacFifoLen;
	bool					FIRMode;
	bool					OCPEnable;    //过流保护
} AUDIODAC_ParamCt;

/**
 * @brief  Audio DAC module enable
 * @param  DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @return NONE
 */
void AudioDAC_Enable(AUDIODAC_MODULE DACModule);

/**
 * @brief  Audio DAC module disable
 * @param  DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @return NONE
 */
void AudioDAC_Disable(AUDIODAC_MODULE DACModule);

/**
 * @brief  Audio DAC module dither enable
 * @param  DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @return NONE
 */
void AudioDAC_DitherEnable(AUDIODAC_MODULE DACModule);

/**
 * @brief  Audio DAC module dither disable
 * @param  DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @return NONE
 */
void AudioDAC_DitherDisable(AUDIODAC_MODULE DACModule);

/**
 * @brief  Audio DAC module scramble enable
 * @param  DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @return NONE
 */
void AudioDAC_ScrambleEnable(AUDIODAC_MODULE DACModule);

/**
 * @brief  Audio DAC module scramble disable
 * @param  DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @return NONE
 */
void AudioDAC_ScrambleDisable(AUDIODAC_MODULE DACModule);

/**
 * @brief  Audio DAC pause,stop ask data from DMA
 * @param  DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @return NONE
 */
void AudioDAC_Pause(AUDIODAC_MODULE DACModule);

/**
 * @brief  Audio DAC disable pause, go on asking data from DMA
 * @param  DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @return NONE
 */
void AudioDAC_Run(AUDIODAC_MODULE DACModule);

/**
 * @brief  Audio DAC output mode set
 * @param  DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @param  DoutOut 	MODE0 = 0,// L_o=L_i;R_o=R_i;
					MODE1 = 1,// L_o=R_i;R_o=L_i;
					MODE2 = 2,// L_o=(L_i+R_i)/2;R_o=(L_i+R_i)/2;
					MODE3 = 3// L_o=(L_i+R_i)/2;R_o=-(L_i+R_i)/2;
 * @param DoutWidth
 * @return NONE
 */
void AudioDAC_DoutModeSet(AUDIODAC_MODULE DACModule, AUDIODAC_DOUT_MODE DoutOut, AUDIODAC_BitWidth DoutWidth);

/**
 * @param  DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @brief  Get Audio DAC output Width
 * @return AUDIODAC_BitWidth
 */
AUDIODAC_BitWidth AudioDAC_BitWidthGet(AUDIODAC_MODULE DACModule);

/**
 * @brief  Audio DAC sample edge mode config
 * @param  DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @param  Edge posedge or negedge to sample data
 * @return NONE
 */
void AudioDAC_EdgeSet(AUDIODAC_MODULE DACModule, EDGE Edge);

/**
 * @brief  Audio DAC scramble mode config
 * @param  DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @param  ScrambleMode DWA
 *                      RANDOM_DWA
 *                      BUTTERFLY_DWA
 * @return NONE
 */
void AudioDAC_ScrambleModeSet(AUDIODAC_MODULE DACModule, SCRAMBLE_MODULE ScrambleMode);

/**
 * @brief  Audio DAC dsm outdis mode config
 * @param  DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @param  mode 0 the DSM out drive the scramble module as normal
 *              1 auto mute the scramble input data  when the continuous N points of the PCM * VOL data values are 0, or the VOL is 0,the N is set by mdac0_zeros_num_sel
 * @return NONE
 */
void AudioDAC_DsmOutdisModeSet(AUDIODAC_MODULE DACModule,uint8_t mode);

/**
 * @brief  Audio DAC zero num set
 * @param  DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @param  sel the continuous PCM data as 0, and the 0's number limit
 *             	0: zeros number value: 512
				1: zeros number value: 1024
				2: zeros number value: 2048
				3: zeros number value: 4096
				4: zeros number value: 8192
				5: zeros number value: 16384
				6: zeros number value: 32768
				7: zeros number value: 65535
 * @return NONE
 */
void AudioDAC_ZeroNumSet(AUDIODAC_MODULE DACModule,uint8_t sel);

/**
 * @brief  Use optimized DSM
 * @param  DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @return NONE
 */
//void AudioDAC_DsmOptimizeSet(void);
void AudioDAC_DsmOptimizeSet(AUDIODAC_MODULE DACModule);

/**
 * @brief  Audio DAC dither pow set
 * @param  DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @param  DitherPow you will set this value to dither pow
 * @return NONE
 */
void AudioDAC_DitherPowSet(AUDIODAC_MODULE DACModule,uint32_t DitherPow);

/**
 * @brief  Audio DAC get dither pow
 * @param  DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @return Dither Pow you used
 */
uint32_t AudioDAC_DitherPowGet(AUDIODAC_MODULE DACModule);

/**
 * @brief  Audio DAC get sampler rate
 * @param  DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @return Sample Rate you used
 */
uint32_t AudioDAC_SampleRateGet(AUDIODAC_MODULE DACModule);

/**
 * @brief  Audio DAC sampler rate set
 * @param  DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @param  SampleRate 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000,
 * 					  64000, 88200, 96000, 176400, 192000, 352800, 384000 are supported.
 * @return NONE
 */
void AudioDAC_SampleRateSet(AUDIODAC_MODULE DACModule, uint32_t SampleRate);

/**
 * @brief  Audio DAC reset the reg data and mem data
 * @param  DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @return NONE
 */
void AudioDAC_Reset(AUDIODAC_MODULE DACModule);

/**
 * @brief  Audio DAC clear mdac_load_error
 * @param  DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @return NONE
 */
void AudioDAC_ErrorFlagClear(AUDIODAC_MODULE DACModule);

/**
 * @brief  Audio DAC  mdac load error, when load next data ,but data is not ready
 * @param  DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @return  ErrorFlag
 */
bool AudioDAC_ErrorFlagGet(AUDIODAC_MODULE DACModule);

/**
 * @brief Audio DAC fade time set
 * @param DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @param FadeTime unit is ms
 * @return NONE
 */
void AudioDAC_FadeTimeSet(AUDIODAC_MODULE DACModule,uint8_t FadeTime);

/**
 * @brief Audio DAC fadein fadeout enable
 * @param DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @return NONE
 */
void AudioDAC_FadeEnable(AUDIODAC_MODULE DACModule);

/**
 * @brief Audio DAC fadein fadeout disable
 * @param DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @return NONE
 */
void AudioDAC_FadeDisable(AUDIODAC_MODULE DACModule);

/**
 * @brief Audio DAC Digital mute,DAC0 has left channel and right channel,DAC1 has just one channel we use LeftMuteEn to control
 * @param DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @param LeftMuteEn left channel for DAC0,it is valid for DAC1
 * @param RightMuteEn right channel for DAC0,it is invalid for DAC1
 * @return NONE
 * @note You must have already used SysTickInit before using this API
 */
void AudioDAC_DigitalMute(AUDIODAC_MODULE DACModule,bool LeftMuteEn, bool RightMuteEn);

/**
 * @brief Audio DAC soft mute,DAC0 has left channel and right channel,DAC1 has just one channel we use LeftMuteEn to control
 * @param DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @param LeftMuteEn left channel for DAC0,it is valid for DAC1
 * @param RightMuteEn right channel for DAC0,it is invalid for DAC1
 * @return NONE
 */
void AudioDAC_SoftMute(AUDIODAC_MODULE DACModule, bool LeftMuteEn, bool RightMuteEn);

/**
 * @brief Audio DAC volume set,DAC0 has left channel and right channel,DAC1 has just one channel we use LeftVol to control
 * @param DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @param LeftVol left channel for DAC0,it is valid for DAC1,its scope is form 0 to 0x1000
 * @param RightVol right channel for DAC0,it is invalid for DAC1,its scope is form 0 to 0x1000
 * @return NONE
 */
void AudioDAC_VolSet(AUDIODAC_MODULE DACModule,uint16_t LeftVol, uint16_t RightVol);

/**
 * @brief Audio DAC volume Get,DAC0 has left channel and right channel,DAC1 has just one channel we use LeftVol to control
 * @param DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @param *LeftVol left channel for DAC0,it is valid for DAC1,its scope is form 0 to 0x1000
 * @param *RightVol right channel for DAC0,it is invalid for DAC1,its scope is form 0 to 0x1000
 * @return NONE
 */
void AudioDAC_VolGet(AUDIODAC_MODULE DACModule,uint16_t* LeftVol, uint16_t* RightVol);

/**
 * @brief enable DAC REG clock
 * @param DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @param Enable TRUE for enable, FALSE for disable
 * @return NONE
 */
void AudioDAC_REGClkEnable(AUDIODAC_MODULE DACModule, bool Enable);

/**
 * @brief enable DAC clock
 * @param DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @param Enable TRUE for enable, FALSE for disable
 * @return NONE
 */
void AudioDAC_ClkEnable(AUDIODAC_MODULE DACModule, bool Enable);

/**
 * @brief function reset for DAC
 * @param DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @return NONE
 */
void AudioDAC_FuncReset(AUDIODAC_MODULE DACModule);

/**
 * @brief register reset for DAC, this will reset ALL REGISTER including DAC0 、DAC1 and DAC2
 * @param DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @return NONE
 * @param NONE
 * @return NONE
 */
//void AudioDAC_RegReset(void);
void AudioDAC_RegReset(AUDIODAC_MODULE DACModule);

#if 0
/**
 * @brief VMID starts charging with programmed current, and power down BIAS
 * @param NONE
 * @return NONE
 */
void AudioDAC_VIMDControl(void);

/**
 * @brief BIAS power on/off control
 * @param OnOff TRUE for BIAS Power on, FALSE for BIAS Power off
 * @return NONE
 */
void AudioDAC_BIASPowerCtrl(bool OnOff);

/**
 * @brief control SCF_VREFP current for left/right channel
 * @param DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @param VrefpL: form 0 to 0x3
 * @param VrefpR: form 0 to 0x3
 * @return NONE
 */
void AudioDAC_IBSelect(AUDIODAC_MODULE DACModule, uint8_t VrefpL, uint8_t VrefpR);
#endif

/**
 * @brief All Analog power on DAC
 * @param DACModule: DAC0_MODULE / DAC1_MODULE / DAC2_MODULE / DAC0_DAC1_MODULE / 
 * 					DAC0_DAC2_MODULE /DAC1_DAC2_MODULE / DAC0_DAC1_DAC2_MODULE
 * @param AUDIODAC_ParamCt: DAC参数结构体
 * @param DACMode: DAC_Single 单端，DAC_Diff 差分
 * @param DAC_LoadStatus: DAC_NOLoad 不带负载，DAC_Load 带负载
 * @param DACEnergyModel：LowEnergy 低功耗模式; CommonEnergy 普通功耗模式
 * @return TRUE:sucess,FALSE:failure
 */
bool AudioDAC_PowerOn(AUDIODAC_MODULE DACMODULE, AUDIODAC_ParamCt *DAC0, AUDIODAC_ParamCt *DAC1, AUDIODAC_ParamCt *DAC2);

/**
 * @brief dac power down
 * @param DACModule: DAC0_MODULE / DAC1_MODULE / DAC2_MODULE / DAC0_DAC1_MODULE / 
 * 					DAC0_DAC2_MODULE /DAC1_DAC2_MODULE / DAC0_DAC1_DAC2_MODULE
 * @return TRUE:sucess,FALSE:failure
 */
bool AudioDAC_PowerDown(AUDIODAC_MODULE DACModule);

/**
 * @brief dac All Analog power down
 * @return TRUE:sucess,FALSE:failure
 */
bool AudioDAC_AllPowerDown(void);

/**
 * @brief Filter selection
 * @param DACModule which AudioDac you will use, Only DAC0_MODULE、DAC1_MODULE 、DAC2_MODULE are supported
 * @return NONE
 */
void AudioDAC_FirSel(AUDIODAC_MODULE DACModule, bool FIRMode);

/**
 * @brief TX request counter, in 24-bit mode, 
 * 		  MDAC dig_en should be set to 0 only after this value reads 0 to avoid unexpected channel swap
 * @param DACMODULE: DAC0_MODULE / DAC1_MODULE / DAC2_MODULE
 * @return RequestCount value
 */
uint8_t AudioDAC_RequestCountGet(AUDIODAC_MODULE DACModule);

/**
 * @brief VMID温度类型
 */
typedef enum _VMID_TEMP_TYPE
{
    VMID_TEMP_NORMAL = 0,  // 常温
    VMID_TEMP_HIGH   = 1   // 高温
} VMID_TEMP_TYPE;

/**
 * @brief Dynamic adjustment of vmid voltage
 * @param temp_type Temperature type, see VMID_TEMP_TYPE
 * @return NONE
 * @note When the codec is in use, it needs to be executed periodically
 */
void vmid_Dynamic_change(VMID_TEMP_TYPE temp_type);

/*
 * @brief DAC2 Replace VCOM configuration
 * @return NONE
 * @note dac2 needs to be configured in DAC_Load of AUDIODAC_LoadStatus
 */
void AudioDAC2_ReplaceVCOM(void);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif //__DAC_H__

/**
 * @}
 * @}
 */
