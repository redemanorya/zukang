/*
 * module_config.h
 *
 *  Created on: Sep 9, 2025
 *      Author: piwang
 */

#ifndef APP_MODULE_CONFIG_H_
#define APP_MODULE_CONFIG_H_

#include "clk.h"
#include "mvwireless2_config.h"
/*************************************************
 * 功能：
 * 1、适配app_config.h配置与参数
 * 2、适配chip_config.h配置的芯片封装资源
 * 3、管理SOC资源模块配置项(Pin可选模块见gpio_config.h)
 * 4、 仅列优先项,多路可用时,自行调配
 *
 * 仅在app_config.h底部引用
 ***********************************************/

/**音频系统Mix采样率列表**/
#ifdef CFG_192KHZ_EN
	#define CORE_SAMPLERATE_LIST		{16000, 44100, 48000, 96000, 192000}
#else
	#define CORE_SAMPLERATE_LIST		{16000, 44100, 48000, 96000}
#endif

/**********************************************
 * 			AD/DA Module
 * 	A20/21耦合SW,DelayMs(3000)防呆,量产可删除
 * *******************************************/
#if defined(CHIP_PIN_MIC0)
	#define MIC_INPUT_CHANNEL                                ANA_INPUT_CH_MIC0
#elif defined(CHIP_PIN_MIC1)
	#define MIC_INPUT_CHANNEL                                ANA_INPUT_CH_MIC1
#else
	#error Conflict: MIC端口异常！！！
#endif

/* MICBias_EN value: bit0=MICBIAS1_EN, bit1=MICBIAS2_EN. Default=0 */
#ifdef MICBIAS1_EN
	#define MICBIAS1_BIT 1
#else
	#define MICBIAS1_BIT 0
#endif
#ifdef MICBIAS2_EN
	#define MICBIAS2_BIT 2
#else
	#define MICBIAS2_BIT 0
#endif
#define CFG_MICBIAS_EN_VAL (MICBIAS1_BIT | MICBIAS2_BIT)

#if defined(CFG_APP_LINEIN_MODE_EN) || defined(CFG_FUNC_LINEIN_MIX_MODE)
	#if (defined(CHIP_PIN_LINEIN) && (MIC_INPUT_CHANNEL != ANA_INPUT_CH_MIC0))
		#define LINEIN_INPUT_CHANNEL                                ANA_INPUT_CH_LINEIN
	#elif (defined(CHIP_PIN_AUX1) && (MIC_INPUT_CHANNEL != ANA_INPUT_CH_MIC1))
		#define LINEIN_INPUT_CHANNEL                                ANA_INPUT_CH_AUX1
	#elif (defined(CHIP_PIN_AUX0) && (MIC_INPUT_CHANNEL != ANA_INPUT_CH_MIC1))
		#define LINEIN_INPUT_CHANNEL                                ANA_INPUT_CH_AUX0
	#else
		#error Conflict: LINEIN端口异常！！！
	#endif

//	#define CFG_LINEIN_DET_EN				//line in插拔检测，linein mix模式不可打开
	#ifdef	CFG_LINEIN_DET_EN
		#define LINEIN_DET_GPIO				RES_GPIOA28		// linein插拔检测IO设置，默认低有效
	#endif
#endif

#ifdef CFG_APP_RADIOIN_MODE_EN
	// 外挂5807/8035二选一
	#define FUNC_RADIO_RDA5807_EN
//	#define FUNC_RADIO_QN8035_EN			//芯片io时钟只支持12M，qn8035必须外挂晶振

	#define	CFG_RADIO_CLK_M12				//sdk 默认配置A29口输出12M clk
//	#define CFG_RADIO_I2C_SD_SAME_PIN_EN	//fm和sd复用data/clk

	#define RADIO_IIC_CLK_PORT				RES_GPIOB5	// IIC时钟口配置
	#define RADIO_IIC_DAT_PORT				RES_GPIOB4	// IIC数据口配置

	#if defined(CFG_APP_LINEIN_MODE_EN)
		#define RADIO_INPUT_CHANNEL			LINEIN_INPUT_CHANNEL //无MIC时可选其他
	#elif defined(CHIP_PIN_AUX1) \
			&& ((defined(CFG_FUNC_MIC_EN) && !defined(CFG_FUNC_LINEIN_MIX_MODE) && defined(MIC_INPUT_CHANNEL) && (MIC_INPUT_CHANNEL == ANA_INPUT_CH_MIC0)) \
			|| (!defined(CFG_FUNC_MIC_EN) && defined(CFG_FUNC_LINEIN_MIX_MODE) &&  (LINEIN_INPUT_CHANNEL == ANA_INPUT_CH_LINEIN)))
		#define RADIO_INPUT_CHANNEL		ANA_INPUT_CH_AUX1
	#elif defined(CHIP_PIN_AUX0) \
				&& ((defined(CFG_FUNC_MIC_EN) && !defined(CFG_FUNC_LINEIN_MIX_MODE) && defined(MIC_INPUT_CHANNEL) && (MIC_INPUT_CHANNEL == ANA_INPUT_CH_MIC0)) \
			|| (!defined(CFG_FUNC_MIC_EN) && defined(CFG_FUNC_LINEIN_MIX_MODE) &&  (LINEIN_INPUT_CHANNEL == ANA_INPUT_CH_LINEIN)))
		#define RADIO_INPUT_CHANNEL		ANA_INPUT_CH_AUX0
	#elif defined(CHIP_PIN_LINEIN) \
			&& ((defined(CFG_FUNC_MIC_EN) && !defined(CFG_FUNC_LINEIN_MIX_MODE) && defined(MIC_INPUT_CHANNEL) && (MIC_INPUT_CHANNEL == ANA_INPUT_CH_MIC1)) \
				|| (!defined(CFG_FUNC_MIC_EN) && defined(CFG_FUNC_LINEIN_MIX_MODE) && (LINEIN_INPUT_CHANNEL != ANA_INPUT_CH_LINEIN)))
		#define RADIO_INPUT_CHANNEL			ANA_INPUT_CH_LINEIN
	#else
		#error Conflict: RADIO AUDIO IN端口异常!!!
	#endif

	#define CFG_RADIO_IIC_ENABLE()			SET_PIN_MODE(RADIO_IIC_CLK_PORT, 0);\
											SET_PIN_MODE(RADIO_IIC_DAT_PORT, 0);
	#define CFG_RADIO_IIC_HANDLER()			I2cMasterCreate(GET_IO_PORT(RADIO_IIC_CLK_PORT), GET_IO_INDEX(RADIO_IIC_CLK_PORT), GET_IO_PORT(RADIO_IIC_DAT_PORT), GET_IO_INDEX(RADIO_IIC_DAT_PORT))
#endif

#if defined(RADIO_INPUT_CHANNEL) && RADIO_INPUT_CHANNEL == ANA_INPUT_CH_LINEIN
	#define RADIO_ADC_SEL					ADC0_MODULE
	#define RADIO_LEFT_GAIN_VAR				gCtrlVars.HwCt.ADC0PGACt.LineinL_Gain
	#define RADIO_RIGHT_GAIN_VAR			gCtrlVars.HwCt.ADC0PGACt.LineinR_Gain
	#define RADIO_CTRLDIGITAL_SEL			gCtrlVars.HwCt.ADC0DigitalCt
	#define RADIO_LEFT_SEL					ADC0_LineinL
	#define RADIO_RIGHT_SEL					ADC0_LineinR
	#define RADIO_DMA_PERI_ID_SEL			PERIPHERAL_1_ID_AUDIO_ADC0_RX
	#define RADIO_LENGET_SEL				AudioADC0_DataLenGet
	#define RADIO_DATAGET_SEL				AudioADC0_DataGet
#elif defined(RADIO_INPUT_CHANNEL) && (RADIO_INPUT_CHANNEL == ANA_INPUT_CH_AUX0)
	#define RADIO_ADC_SEL					ADC1_MODULE
	#define RADIO_LEFT_GAIN_VAR				gCtrlVars.HwCt.ADC1PGACt.AUX0L_B20_Gain
	#define RADIO_RIGHT_GAIN_VAR			gCtrlVars.HwCt.ADC1PGACt.AUX0R_B21_Gain
	#define RADIO_CTRLDIGITAL_SEL			gCtrlVars.HwCt.ADC1DigitalCt
	#define RADIO_LEFT_SEL					ADC1_AUX0L_B20
	#define RADIO_RIGHT_SEL					ADC1_AUX0R_B21
	#define RADIO_DMA_PERI_ID_SEL			PERIPHERAL_1_ID_AUDIO_ADC1_RX
	#define RADIO_LENGET_SEL				AudioADC1_DataLenGet
	#define RADIO_DATAGET_SEL				AudioADC1_DataGet
#elif defined(RADIO_INPUT_CHANNEL) && (RADIO_INPUT_CHANNEL == ANA_INPUT_CH_AUX1)
	#define RADIO_ADC_SEL					ADC1_MODULE
	#define RADIO_LEFT_GAIN_VAR				gCtrlVars.HwCt.ADC1PGACt.AUX1L_B18_Gain
	#define RADIO_RIGHT_GAIN_VAR			gCtrlVars.HwCt.ADC1PGACt.AUX1R_B19_Gain
	#define RADIO_CTRLDIGITAL_SEL			gCtrlVars.HwCt.ADC1DigitalCt
	#define RADIO_LEFT_SEL					ADC1_AUX1L_B18
	#define RADIO_RIGHT_SEL					ADC1_AUX1R_B19
	#define RADIO_DMA_PERI_ID_SEL			PERIPHERAL_1_ID_AUDIO_ADC1_RX
	#define RADIO_LENGET_SEL				AudioADC1_DataLenGet
	#define RADIO_DATAGET_SEL				AudioADC1_DataGet
#elif defined(RADIO_INPUT_CHANNEL) && (RADIO_INPUT_CHANNEL == ANA_INPUT_CH_MIC0)
	#define RADIO_ADC_SEL					ADC0_MODULE
	#define RADIO_LEFT_GAIN_VAR				gCtrlVars.HwCt.ADC0PGACt.MICL_EN
	#define RADIO_RIGHT_GAIN_VAR			gCtrlVars.HwCt.ADC0PGACt.MICR_EN
	#define RADIO_CTRLDIGITAL_SEL			gCtrlVars.HwCt.ADC0DigitalCt
	#define RADIO_LEFT_SEL					ADC0_MICL
	#define RADIO_RIGHT_SEL					ADC0_MICR
	#define RADIO_DMA_PERI_ID_SEL			PERIPHERAL_1_ID_AUDIO_ADC0_RX
	#define RADIO_LENGET_SEL				AudioADC0_DataLenGet
	#define RADIO_DATAGET_SEL				AudioADC0_DataGet
#elif defined(RADIO_INPUT_CHANNEL) && (RADIO_INPUT_CHANNEL == ANA_INPUT_CH_MIC1)
	#define RADIO_ADC_SEL					ADC1_MODULE
	#define RADIO_LEFT_GAIN_VAR				gCtrlVars.HwCt.ADC1PGACt.MICL_EN
	#define RADIO_RIGHT_GAIN_VAR			gCtrlVars.HwCt.ADC1PGACt.MICR_EN
	#define RADIO_CTRLDIGITAL_SEL			gCtrlVars.HwCt.ADC1DigitalCt
	#define RADIO_LEFT_SEL					ADC1_MICL
	#define RADIO_RIGHT_SEL					ADC1_MICR
	#define RADIO_DMA_PERI_ID_SEL			PERIPHERAL_1_ID_AUDIO_ADC1_RX
	#define RADIO_LENGET_SEL				AudioADC1_DataLenGet
	#define RADIO_DATAGET_SEL				AudioADC1_DataGet
#endif

#if (defined(CFG_FUNC_MIC_EN) || (defined(BT_HFP_SUPPORT) && BT_HFP_SUPPORT)) && defined(MIC_INPUT_CHANNEL)
	#define CFG_RES_MIC_SELECT      			(0x03)//1= MICL, 2= MICR, 3= MICStereo
	#if defined(CFG_RES_MIC_SELECT) && (MIC_INPUT_CHANNEL == ANA_INPUT_CH_MIC1)
		//#define CHIP_MIC1_USE_DIFF
		#define	MIC_ADC_SEL						ADC1_MODULE
		#define MIC_CTRLPGA_SEL					gCtrlVars.HwCt.ADC1PGACt
		#define MIC_CTRLDIGITAL_SEL				gCtrlVars.HwCt.ADC1DigitalCt
		#define MIC_DMA_PERI_ID_SEL				PERIPHERAL_1_ID_AUDIO_ADC1_RX
		#define MIC_LENGET_SEL					AudioADC1_DataLenGet
		#define MIC_DATAGET_SEL					AudioADC1_DataGet
		#if CFG_RES_MIC_SELECT & 1
			#define	MIC_LEFT_SEL				ADC1_MICL
		#else
			#define	MIC_LEFT_SEL				ADC_NONE
		#endif
		#if CFG_RES_MIC_SELECT & 2
			#define	MIC_RIGHT_SEL				ADC1_MICR
		#else
			#define	MIC_RIGHT_SEL				ADC_NONE
		#endif
	#elif defined(CFG_RES_MIC_SELECT) && (MIC_INPUT_CHANNEL == ANA_INPUT_CH_MIC0)
		//#define CHIP_MIC0_USE_DIFF
		#define	MIC_ADC_SEL						ADC0_MODULE
		#define MIC_CTRLPGA_SEL					gCtrlVars.HwCt.ADC0PGACt
		#define MIC_CTRLDIGITAL_SEL				gCtrlVars.HwCt.ADC0DigitalCt
		#define MIC_DMA_PERI_ID_SEL				PERIPHERAL_1_ID_AUDIO_ADC0_RX
		#define MIC_LENGET_SEL					AudioADC0_DataLenGet
		#define MIC_DATAGET_SEL					AudioADC0_DataGet
		#if CFG_RES_MIC_SELECT & 1
			#define	MIC_LEFT_SEL				ADC0_MICL
		#else
			#define	MIC_LEFT_SEL				ADC_NONE
		#endif
		#if CFG_RES_MIC_SELECT & 2
			#define	MIC_RIGHT_SEL				ADC0_MICR
		#else
			#define	MIC_RIGHT_SEL				ADC_NONE
		#endif
	#endif
#endif

#if defined(LINEIN_INPUT_CHANNEL) && LINEIN_INPUT_CHANNEL == ANA_INPUT_CH_LINEIN
	#define LINEIN_ADC_SEL					ADC0_MODULE
	#define LINEIN_LEFT_GAIN_VAR			gCtrlVars.HwCt.ADC0PGACt.LineinL_Gain
	#define LINEIN_RIGHT_GAIN_VAR			gCtrlVars.HwCt.ADC0PGACt.LineinR_Gain
	#define LINEIN_CTRLDIGITAL_SEL			gCtrlVars.HwCt.ADC0DigitalCt
	#define LINEIN_LEFT_SEL					ADC0_LineinL
	#define LINEIN_RIGHT_SEL				ADC0_LineinR
	#define LINEIN_DMA_PERI_ID_SEL			PERIPHERAL_1_ID_AUDIO_ADC0_RX
	#define LINEIN_LENGET_SEL				AudioADC0_DataLenGet
	#define LINEIN_DATAGET_SEL				AudioADC0_DataGet
#elif defined(LINEIN_INPUT_CHANNEL) && (LINEIN_INPUT_CHANNEL == ANA_INPUT_CH_AUX0)
	#define LINEIN_ADC_SEL					ADC1_MODULE
	#define LINEIN_LEFT_GAIN_VAR			gCtrlVars.HwCt.ADC1PGACt.AUX0L_B20_Gain
	#define LINEIN_RIGHT_GAIN_VAR			gCtrlVars.HwCt.ADC1PGACt.AUX0R_B21_Gain
	#define LINEIN_CTRLDIGITAL_SEL			gCtrlVars.HwCt.ADC1DigitalCt
	#define LINEIN_LEFT_SEL					ADC1_AUX0L_B20
	#define LINEIN_RIGHT_SEL				ADC1_AUX0R_B21
	#define LINEIN_DMA_PERI_ID_SEL			PERIPHERAL_1_ID_AUDIO_ADC1_RX
	#define LINEIN_LENGET_SEL				AudioADC1_DataLenGet
	#define LINEIN_DATAGET_SEL				AudioADC1_DataGet
#elif defined(LINEIN_INPUT_CHANNEL) && (LINEIN_INPUT_CHANNEL == ANA_INPUT_CH_AUX1)
	#define LINEIN_ADC_SEL					ADC1_MODULE
	#define LINEIN_LEFT_GAIN_VAR			gCtrlVars.HwCt.ADC1PGACt.AUX1L_B18_Gain
	#define LINEIN_RIGHT_GAIN_VAR			gCtrlVars.HwCt.ADC1PGACt.AUX1R_B19_Gain
	#define LINEIN_CTRLDIGITAL_SEL			gCtrlVars.HwCt.ADC1DigitalCt
	#define LINEIN_LEFT_SEL					ADC1_AUX1L_B18
	#define LINEIN_RIGHT_SEL				ADC1_AUX1R_B19
	#define LINEIN_DMA_PERI_ID_SEL			PERIPHERAL_1_ID_AUDIO_ADC1_RX
	#define LINEIN_LENGET_SEL				AudioADC1_DataLenGet
	#define LINEIN_DATAGET_SEL				AudioADC1_DataGet
#endif

#if defined(CHIP_PIN_DAC0)
	#define DAC_OUTPUT_CHANNEL                                MONITOR_DAC0
#elif defined(CHIP_PIN_DAC1)
	#define DAC_OUTPUT_CHANNEL                                MONITOR_DAC1
#elif defined(CHIP_PIN_DAC2)
	#define DAC_OUTPUT_CHANNEL                                MONITOR_DAC2
#else
	#error Conflict: DAC端口异常！！！
#endif

#if defined(CFG_RES_DAC_MONITOR_EN)
    #define CHIP_DAC_USE_DIFF                    // [HP] DAC0 diff 4-wire to HT97180——差分模式
	#if defined(CHIP_DAC_USE_DIFF)
		#define DAC_MODE_SEL	DAC_Diff
	#else
		#define DAC_MODE_SEL	DAC_Single
	#endif
	#define DAC_DOUT_MODE_SEL    MODE0  // DoutMode(MODE0~MODE3)
	#if DAC_OUTPUT_CHANNEL == MONITOR_DAC0
		#define	DAC_SEL							DAC0_MODULE
		#define DAC_AUDIO_MODULE				AUDIO_DAC0
		#define DAC_DMA_PERI_ID_SEL				PERIPHERAL_1_ID_AUDIO_DAC0_TX
		#define DAC_CTRLVAR_SEL					gCtrlVars.HwCt.DAC0Ct
		#define DAC_LENGET_SEL					AudioDAC0_DataSpaceLenGet
		#define DAC_LENGET_GET					AudioDAC0_DataLenGet
		#define DAC_DATASET_SEL					AudioDAC0_DataSet
	#elif DAC_OUTPUT_CHANNEL == MONITOR_DAC1
		#define	DAC_SEL							DAC1_MODULE
		#define DAC_AUDIO_MODULE				AUDIO_DAC1
		#define DAC_DMA_PERI_ID_SEL				PERIPHERAL_1_ID_AUDIO_DAC1_TX
		#define DAC_CTRLVAR_SEL					gCtrlVars.HwCt.DAC1Ct
		#define DAC_LENGET_SEL					AudioDAC1_DataSpaceLenGet
		#define DAC_LENGET_GET					AudioDAC1_DataLenGet
		#define DAC_DATASET_SEL					AudioDAC1_DataSet
	#endif
#endif

#if defined(CFG_RES_AUDIO_DAC1_MIX_OUT_EN) && (DAC_OUTPUT_CHANNEL != MONITOR_DAC1)
	//#define CHIP_DAC1_USE_DIFF
	#if defined(CHIP_DAC1_USE_DIFF)
		#define DAC1_MODE_SEL	DAC_Diff
	#else
		#define DAC1_MODE_SEL	DAC_Single
	#endif
	#define DAC1_DOUT_MODE_SEL   MODE0  // DoutMode(MODE0~MODE3)

	#define	DAC1_SEL						DAC1_MODULE
	#define DAC1_AUDIO_MODULE				AUDIO_DAC1
	#define DAC1_DMA_PERI_ID_SEL			PERIPHERAL_1_ID_AUDIO_DAC1_TX
	#define DAC1_CTRLVAR_SEL				gCtrlVars.HwCt.DAC1Ct
	#define DAC1_LENGET_SEL					AudioDAC1_DataSpaceLenGet
	#define DAC1_DATASET_SEL				AudioDAC1_DataSet
#endif

#if defined(CFG_RES_AUDIO_DAC2_MIX_OUT_EN) && (DAC_OUTPUT_CHANNEL != MONITOR_DAC2)
	//#define CHIP_DAC2_USE_DIFF
	#if defined(CHIP_DAC2_USE_DIFF)
		#define DAC2_MODE_SEL	DAC_Diff
	#else
		#define DAC2_MODE_SEL	DAC_Single
	#endif
	#define DAC2_DOUT_MODE_SEL   MODE0  // DoutMode(MODE0~MODE3)

	#define	DAC2_SEL						DAC2_MODULE
	#define DAC2_AUDIO_MODULE				AUDIO_DAC2
	#define DAC2_DMA_PERI_ID_SEL			PERIPHERAL_1_ID_AUDIO_DAC2_TX
	#define DAC2_CTRLVAR_SEL				gCtrlVars.HwCt.DAC2Ct
	#define DAC2_LENGET_SEL					AudioDAC2_DataSpaceLenGet
	#define DAC2_DATASET_SEL				AudioDAC2_DataSet
#endif

#if CFG_DEFAULT_WIDTH == WIDTH_L24BIT
	#define AUDIOADC_WIDTH_SEL				ADC_24BIT
	#define DAC_WIDTH_SEL					DAC_24BIT_LOWALIGN
#else
	#define AUDIOADC_WIDTH_SEL				ADC_16BIT
	#define DAC_WIDTH_SEL					DAC_16BIT
#endif


#ifdef CFG_FUNC_SYSTEM_LOW_POWER
	#define ADC_ENERGY_SEL  	 			ADCLowEnergy //ADC使用低功耗模式
	#define DAC_ENERGY_SEL  	 			DACLowEnergy //DAC使用低功耗模式
#else
	#define ADC_ENERGY_SEL  	 			ADCCommonEnergy
	#define DAC_ENERGY_SEL  	 			DACCommonEnergy
#endif

#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
	#define	TWS_DAC_SEL							DAC2_MODULE
	#define TWS_DAC_SEL_VAL						DAC2_MODULE_VAL
	#define TWS_DAC_AUDIO_MODULE				AUDIO_DAC2
	#define TWS_DAC_DMA_PERI_ID_SEL				PERIPHERAL_1_ID_AUDIO_DAC2_TX
	#define TWS_DAC_CTRLVAR_SEL					gCtrlVars.HwCt.DAC2Ct
	#define TWS_DAC_LENGET_SEL					AudioDAC2_DataSpaceLenGet
	#define TWS_DAC_DATASET_SEL					AudioDAC2_DataSet
	#define TWS_SAMPLERATE						48000//	48k/44.1k
#else

	#if DAC_OUTPUT_CHANNEL == MONITOR_DAC0
	#define	TWS_DAC_SEL							DAC0_MODULE
	#define TWS_DAC_SEL_VAL						DAC0_MODULE_VAL
	#define TWS_DAC_AUDIO_MODULE				AUDIO_DAC0
	#define TWS_DAC_DMA_PERI_ID_SEL				PERIPHERAL_1_ID_AUDIO_DAC0_TX
	#define TWS_DAC_CTRLVAR_SEL					gCtrlVars.HwCt.DAC0Ct
	#define TWS_DAC_LENGET_SEL					AudioDAC0_DataSpaceLenGet
	#define TWS_DAC_DATASET_SEL					AudioDAC0_DataSet
	#define TWS_SAMPLERATE						44100//	48k/44.1k
	#elif DAC_OUTPUT_CHANNEL == MONITOR_DAC1
	#define	TWS_DAC_SEL							DAC1_MODULE
	#define TWS_DAC_SEL_VAL						DAC1_MODULE_VAL
	#define TWS_DAC_AUDIO_MODULE				AUDIO_DAC1
	#define TWS_DAC_DMA_PERI_ID_SEL				PERIPHERAL_1_ID_AUDIO_DAC1_TX
	#define TWS_DAC_CTRLVAR_SEL					gCtrlVars.HwCt.DAC1Ct
	#define TWS_DAC_LENGET_SEL					AudioDAC1_DataSpaceLenGet
	#define TWS_DAC_DATASET_SEL					AudioDAC1_DataSet
	#define TWS_SAMPLERATE						44100//	48k/44.1k
	#elif DAC_OUTPUT_CHANNEL == MONITOR_DAC2
	#define	TWS_DAC_SEL							DAC2_MODULE
	#define TWS_DAC_SEL_VAL						DAC2_MODULE_VAL
	#define TWS_DAC_AUDIO_MODULE				AUDIO_DAC2
	#define TWS_DAC_DMA_PERI_ID_SEL				PERIPHERAL_1_ID_AUDIO_DAC2_TX
	#define TWS_DAC_CTRLVAR_SEL					gCtrlVars.HwCt.DAC2Ct
	#define TWS_DAC_LENGET_SEL					AudioDAC2_DataSpaceLenGet
	#define TWS_DAC_DATASET_SEL					AudioDAC2_DataSet
	#define TWS_SAMPLERATE						44100//	48k/44.1k
	#endif
#endif

/*******************************************************
 * 			Frame Step @ First Source DMA
 * 	短帧短延时 水位中断,依音效图挂载Source,通道可选
 * 	典型应用双核分双图, 非该场景效果一般
 ******************************************************/
#if defined(CFG_FUNC_EFFECT_CORE_DUAL_EN) || defined(CFG_LOW_DELAY_CTRL) || defined(FUNC_WIRELESS2_SUPPORT)
	#if (defined(CFG_FUNC_MIC_EN)) && defined(MIC_INPUT_CHANNEL)//缺省Mic_KaraOK主source
		#define MIC_THRESHOLD_TRIGER_EN
		#define FRAME_SYNC_TRIGER_EN
		#define FRAME_SYNC_SOURCE_NUM		MIC_SOURCE_NUM
		#define FRAME_SYNC_DMA_ID			MIC_DMA_PERI_ID_SEL
		#define DELAY_SOURCE_LEN_FUNC		MIC_LENGET_SEL
		#define DELAY_SOURCE_GET_FUNC		MIC_DATAGET_SEL
	#elif defined(CFG_I2S0_MIX_IN_EN) && (CFG_I2S0_MIX_MODE == I2S_MASTER_MODE)
		#define I2S0_MIX_IN_THRESHOLD_TRIGER_EN//dma Int trigger,帧效率高,条件:Master、同采样率、无其他triger
		#define FRAME_SYNC_TRIGER_EN
		#define FRAME_SYNC_SOURCE_NUM		I2S0_MIX_SOURCE_NUM
		#define FRAME_SYNC_DMA_ID			PERIPHERAL_1_ID_I2S0_RX
		#define DELAY_SOURCE_LEN_FUNC		AudioI2S0_DataLenGet
		#define DELAY_SOURCE_GET_FUNC		AudioI2S0_DataGet
	#elif defined(CFG_I2S1_MIX_IN_EN) && (CFG_I2S1_MIX_MODE == I2S_MASTER_MODE) && !defined(I2S1_MIX_IN_PLURALCH_MODE_EN)
		#define	I2S1_MIX_IN_THRESHOLD_TRIGER_EN
		#define FRAME_SYNC_TRIGER_EN
		#define FRAME_SYNC_SOURCE_NUM		I2S1_MIX_SOURCE_NUM
		#define FRAME_SYNC_DMA_ID			PERIPHERAL_1_ID_I2S1_RX
		#define DELAY_SOURCE_LEN_FUNC		AudioI2S1_DataLenGet
		#define DELAY_SOURCE_GET_FUNC		AudioI2S1_DataGet
	#elif defined(CFG_I2S2_MIX_IN_EN) && (CFG_I2S2_MIX_MODE == I2S_MASTER_MODE) && !(I2S2_MIX_IN_LINE_CNT > I2S_LINE_SINGLE)
		#define I2S2_MIX_IN_THRESHOLD_TRIGER_EN
		#define FRAME_SYNC_TRIGER_EN
		#define FRAME_SYNC_SOURCE_NUM		I2S2_MIX_SOURCE_NUM
		#define FRAME_SYNC_DMA_ID			PERIPHERAL_1_ID_I2S2_RX
		#define DELAY_SOURCE_LEN_FUNC		AudioI2S2_DataLenGet
		#define DELAY_SOURCE_GET_FUNC		AudioI2S2_DataGet
	#else
		#error Conflict:No frame sync source!!!
	#endif
#endif

/*******************************************************
 * 			Sink Delay @ DMA sample
 * 	依短延时音效图挂载Sink配置,通道可选
 * 	重要: FRAME_SYNC_SOURCE_NUM / DELAY_SINK_DAC2_MIX_EN 不在同flow图时,延时不正确
  ******************************************************/
#if defined(CFG_LOW_DELAY_CTRL)
	#if defined(CFG_RES_DAC_MONITOR_EN) && (DAC_OUTPUT_CHANNEL == MONITOR_DAC0)//缺省Mic_KaraOK主sink
		#define DELAY_SINK_DAC_MONITOR_EN
		#define DELAY_SINK_NUM			AUDIO_DAC_SINK_NUM
		#define DELAY_SINK_LEN_FUNC		AudioDAC0_DataLenGet
	#elif (defined(CFG_RES_DAC_MONITOR_EN) && (DAC_OUTPUT_CHANNEL == MONITOR_DAC1))
		#define DELAY_SINK_DAC_MONITOR_EN
		#define DELAY_SINK_NUM			AUDIO_DAC_SINK_NUM
		#define DELAY_SINK_LEN_FUNC		AudioDAC1_DataLenGet
	#elif defined(CFG_RES_AUDIO_DAC1_MIX_OUT_EN) && !(defined(CFG_RES_DAC_MONITOR_EN) && (DAC_OUTPUT_CHANNEL == MONITOR_DAC1))
		#define DELAY_SINK_DAC1_MIX_EN
		#define DELAY_SINK_NUM			AUDIO_DAC_MIX1_OUT_SINK_NUM
		#define DELAY_SINK_LEN_FUNC		AudioDAC1_DataLenGet
	#elif defined(CFG_RES_AUDIO_DAC1_MIX_OUT_EN)
		#define DELAY_SINK_DAC1_MIX_EN
		#define DELAY_SINK_NUM			AUDIO_DAC_MIX1_OUT_SINK_NUM
		#define DELAY_SINK_LEN_FUNC		AudioDAC1_DataLenGet
	#elif defined(CFG_I2S0_MIX_OUT_EN) && (CFG_I2S0_MIX_MODE == I2S_MASTER_MODE)
		#define DELAY_SINK_NUM			AUDIO_I2S0_MIX_OUT_SINK_NUM
		#define DELAY_SINK_LEN_FUNC		AudioI2S0_TX_DataLenGet
	#elif defined(CFG_I2S1_MIX_OUT_EN) && (CFG_I2S1_MIX_MODE == I2S_MASTER_MODE) && !defined(I2S1_MIX_OUT_PLURALCH_MODE_EN)
		#define DELAY_SINK_NUM			AUDIO_I2S1_MIX_OUT_SINK_NUM
		#define DELAY_SINK_LEN_FUNC		AudioI2S1_TX_DataLenGet
	#elif defined(CFG_I2S2_MIX_OUT_EN) && (CFG_I2S2_MIX_MODE == I2S_MASTER_MODE) && !(I2S2_MIX_OUT_LINE_CNT > I2S_LINE_SINGLE)
		#define DELAY_SINK_NUM			AUDIO_I2S2_MIX_OUT_SINK_NUM
		#define DELAY_SINK_LEN_FUNC		AudioI2S2_TX_DataLenGet
	#else
		#error Conflict:No delay sink!!!
	#endif
	/**Target = Frame + FramePayload + BlockJitter(1ms), 帧轻载时可下调**/
	#define	LOW_DELAY_SAMPLE_TARGET		(SOURCEFRAME(FRAME_SYNC_SOURCE_NUM)*2 + 30)
#endif

/**********************************************
 * 			USB Module
 * *******************************************/
/**Application Detect**/
#if(defined(CFG_APP_USB_PLAY_MODE_EN) || defined(USB_HOST_AUDIO_MIX_FS))
	#define CFG_FUNC_HOST_FS_DETECT
#endif
#if defined(CFG_APP_USB_PLAY_MODE_EN)
	#define CFG_RES_UDISK_FS_USE
#endif
#if (defined(CFG_APP_USB_HS_PLAY_MODE_EN) || defined(USB_HOST_AUDIO_MIX_HS))
	#define CFG_FUNC_HOST_HS_DETECT
#endif
#if (defined(CFG_APP_USB_HS_PLAY_MODE_EN))
	#define CFG_RES_UDISK_HS_USE
#endif
#if defined(CFG_UAC_DEVICE_HS_EN) || defined(CFG_COMMUNICATION_BY_HS_USB)
	#define CFG_FUNC_HS_USB_DEVICE_DETECT		//HS USB device模式连接检测
#endif
#if defined(CFG_UAC_DEVICE_FS_EN) || defined(CFG_COMMUNICATION_BY_FS_USB)
	#define CFG_FUNC_FS_USB_DEVICE_DETECT		//FS USB device模式连接检测
#endif

/**Bus Scan Host Role**/
#if defined(CFG_FUNC_HOST_FS_DETECT)
	#define CFG_PARA_HOST_FS_DET				1
#else
	#define CFG_PARA_HOST_FS_DET				0
#endif
#if defined(CFG_FUNC_HOST_HS_DETECT)
	#define CFG_PARA_HOST_HS_DET				1
#else
	#define CFG_PARA_HOST_HS_DET				0
#endif

/**Bus Scan Device Role**/
#if defined(CFG_FUNC_FS_USB_DEVICE_DETECT)
	#define CFG_PARA_DEVICE_FS_DET				1
#else
	#define CFG_PARA_DEVICE_FS_DET				0
#endif
#if defined(CFG_FUNC_HS_USB_DEVICE_DETECT) && defined(CHIP_PIN_HSOTG)
	#define CFG_PARA_DEVICE_HS_DET				1
#else
	#define CFG_PARA_DEVICE_HS_DET				0
#endif


/**********************************************
 * 			RGB Module
 * *******************************************/
#ifdef CFG_DMA_RGB_LED_EN

	#define USE_TIMER		TIMER10_PWM0	//timer及IO设置,注意管理对应的DMA配置冲突

#if(USE_TIMER == TIMER5_PWM0)//警告：Timer5 HDMI预配,RGB勿用
	#define TIMER_ID		TIMER5
	#define DMA_MASK		PERIPHERAL_0_ID_TIMER5
	#define TIMER_ADDR		TIMER5_PWM0_DUTY_ADDR
	#define PWM_INDEX		4	// 决定使用下面哪个IO输出，比如设置0则A0为输出口， 设置4则A24为输出口
	#define PWM_CHANNEL		TIMER5_PWM0_A0_A6_A12_A18_A24_A30_B4_B10
	#define PWM_OUT_CH		PWM_CHANNEL0
#elif (USE_TIMER == TIMER6_PWM0)
	#define TIMER_ID		TIMER6
	#define DMA_MASK		PERIPHERAL_0_ID_TIMER6
	#define TIMER_ADDR		TIMER6_PWM0_DUTY_ADDR
	#define PWM_INDEX		4	// 决定使用下面哪个IO输出，比如设置0:则A1为输出口， 设置4:则A25为输出口
	#define PWM_CHANNEL		TIMER6_PWM0_A1_A7_A13_A19_A25_B5_B11_B17
	#define PWM_OUT_CH		PWM_CHANNEL0
#elif (USE_TIMER == TIMER7_PWM0)
	#define TIMER_ID		TIMER7
	#define DMA_MASK		PERIPHERAL_1_ID_TIMER7
	#define TIMER_ADDR		TIMER7_PWM0_DUTY_ADDR
	#define PWM_INDEX		4	// 决定使用下面哪个IO输出，比如设置0:则A2为输出口， 设置4:则A26为输出口
	#define PWM_CHANNEL		TIMER7_PWM0_A2_A8_A14_A20_A26_B0_B6_B12_B18
	#define PWM_OUT_CH		PWM_CHANNEL0
#elif (USE_TIMER == TIMER7_PWM1)
	#define TIMER_ID		TIMER7
	#define DMA_MASK		PERIPHERAL_1_ID_TIMER7_1
	#define TIMER_ADDR		TIMER7_PWM1_DUTY_ADDR
	#define PWM_INDEX		0
	#define PWM_CHANNEL		TIMER7_PWM1_A15_B7
	#define PWM_OUT_CH		PWM_CHANNEL1
#elif (USE_TIMER == TIMER7_PWM2)
	#define TIMER_ID		TIMER7
	#define DMA_MASK		PERIPHERAL_1_ID_TIMER7_2
	#define TIMER_ADDR		TIMER7_PWM2_DUTY_ADDR
	#define PWM_INDEX		0
	#define PWM_CHANNEL		TIMER7_PWM2_A16_B8
	#define PWM_OUT_CH		PWM_CHANNEL2
#elif (USE_TIMER == TIMER7_PWM3)
	#define TIMER_ID		TIMER7
	#define DMA_MASK		PERIPHERAL_1_ID_TIMER7_3
	#define TIMER_ADDR		TIMER7_PWM3_DUTY_ADDR
	#define PWM_INDEX		0
	#define PWM_CHANNEL		TIMER7_PWM3_A17_B9
	#define PWM_OUT_CH		PWM_CHANNEL3
#elif (USE_TIMER == TIMER8_PWM0)
	#define TIMER_ID		TIMER8
	#define DMA_MASK		PERIPHERAL_1_ID_TIMER8
	#define TIMER_ADDR		TIMER8_PWM0_DUTY_ADDR
	#define PWM_INDEX		4	// 决定使用下面哪个IO输出，比如设置0则A3为输出口， 设置4则A27为输出口
	#define PWM_CHANNEL		TIMER8_PWM0_A3_A9_A15_A21_A27_B1_B7_B13_B19
	#define PWM_OUT_CH		PWM_CHANNEL0
#elif (USE_TIMER == TIMER8_PWM1)
	#define TIMER_ID		TIMER8
	#define DMA_MASK		PERIPHERAL_1_ID_TIMER8_1
	#define TIMER_ADDR		TIMER8_PWM1_DUTY_ADDR
	#define PWM_INDEX		0	//只支持A20口
	#define PWM_CHANNEL		TIMER8_PWM1_A20
	#define PWM_OUT_CH		PWM_CHANNEL1
#elif (USE_TIMER == TIMER8_PWM2)
	#define TIMER_ID		TIMER8
	#define DMA_MASK		PERIPHERAL_1_ID_TIMER8_2
	#define TIMER_ADDR		TIMER8_PWM2_DUTY_ADDR
	#define PWM_INDEX		0	//只支持A19口
	#define PWM_CHANNEL		TIMER8_PWM2_A19
	#define PWM_OUT_CH		PWM_CHANNEL2
#elif (USE_TIMER == TIMER8_PWM3)
	#define TIMER_ID		TIMER8
	#define DMA_MASK		PERIPHERAL_1_ID_TIMER8_3
	#define TIMER_ADDR		TIMER8_PWM3_DUTY_ADDR
	#define PWM_INDEX		0	//只支持A18口
	#define PWM_CHANNEL		TIMER8_PWM3_A18
	#define PWM_OUT_CH		PWM_CHANNEL3
#elif (USE_TIMER == TIMER9_PWM0)
	#define TIMER_ID		TIMER9
	#define DMA_MASK		PERIPHERAL_1_ID_TIMER9
	#define TIMER_ADDR		TIMER9_PWM0_DUTY_ADDR
	#define PWM_INDEX		4	// 决定使用下面哪个IO输出，比如设置0则A4为输出口， 设置4则A28为输出口
	#define PWM_CHANNEL		TIMER9_PWM0_A4_A10_A16_A22_A28_B2_B8_B14_B20
	#define PWM_OUT_CH		PWM_CHANNEL0
#elif (USE_TIMER == TIMER10_PWM0)
	#define TIMER_ID		TIMER10
	#define DMA_MASK		PERIPHERAL_1_ID_TIMER10
	#define TIMER_ADDR		TIMER10_PWM0_DUTY_ADDR
	#define PWM_INDEX		4	// 决定使用下面哪个IO输出，比如设置0则A5为输出口， 设置4则A29为输出口
	#define PWM_CHANNEL		TIMER10_PWM0_A5_A11_A17_A23_A29_B3_B9_B15_B21
	#define PWM_OUT_CH		PWM_CHANNEL0
#endif
#endif

/**********************************************
 * 			Timer Module
 * Timer1:
 * Timer2:System Monitor
 * Timer3(Default C2):CFG_FUNC_LED_REFRESH
 * Timer4(Default C2):CFG_I2S_SLAVE_TO_SPDIFOUT_EN
 * Timer5:HDMI(专用勿改,RC时钟)
 * Timer6:HDMI
 * Timer7:
 * Timer8:BT_LE_AUDIO_SUPPORT
 * *******************************************/


/**********************************************
 * 			DMA Common Map
 *常驻外设DMA通道号归一化管理
 *旨在规避切模式更新Map引发LED闪烁/无声/通信异常
 *音频流退模式时Deinit通道更安全
 *APP Mode私有外设勿配，仅相应Mode文件本地管理
 *New Item：
 *a、增常驻通道MAP宏,小心规则：外设枚举加逗号,或者指向空避免占资源
 *b、配置项添加到对应COMMON列表
 *c、外设和DMA控制器映射关系，不可跨区/重复配置
 * *******************************************/
/**Map0**/
#if defined(CFG_RES_CARD_USE)
	#define MAP_SDIO_RX				PERIPHERAL_0_ID_SDIO_RX,
#else
	#define MAP_SDIO_RX
#endif
#if defined(CFG_RES_CARD_USE) && (defined(CFG_FUNC_RECORD_SD_UDISK) || defined(CFG_OTG_MODE_READER_EN) || defined(CFG_OTG_FS_MODE_READER_EN))
	#define MAP_SDIO_TX				PERIPHERAL_0_ID_SDIO_TX,
#else
	#define MAP_SDIO_TX
#endif
#if defined(CFG_COMMUNICATION_BY_UART) && (GET_UARTIO_UARTX(CFG_FUNC_COMMUNICATION_RX_PORT) == RES_UARTX_0) \
	|| defined(CFG_UPGRADE_BY_UART) && (GET_UARTIO_UARTX(CFG_FUNC_UPGRADE_RX_DMA_PORT) == RES_UARTX_0)
	#define MAP_UART0_RX			PERIPHERAL_0_ID_UART0_RX,
#else
	#define MAP_UART0_RX
#endif
#if defined(CFG_COMMUNICATION_BY_UART) && (GET_UARTIO_UARTX(CFG_FUNC_COMMUNICATION_TX_PORT) == RES_UARTX_0) \
	|| defined(CFG_UPGRADE_BY_UART) && (GET_UARTIO_UARTX(CFG_FUNC_UPGRADE_TX_DMA_PORT) == RES_UARTX_0)	\
	|| defined(CFG_DUMP_DEBUG_EN) && (GET_UARTIO_UARTX(CFG_DUMP_UART_TX_PORT) == RES_UARTX_0)
	#define MAP_UART0_TX			PERIPHERAL_0_ID_UART0_TX,
#else
	#define MAP_UART0_TX
#endif
#if defined(CFG_COMMUNICATION_BY_UART) && (GET_UARTIO_UARTX(CFG_FUNC_COMMUNICATION_RX_PORT) == RES_UARTX_1) \
	|| defined(CFG_UPGRADE_BY_UART) && (GET_UARTIO_UARTX(CFG_FUNC_UPGRADE_RX_DMA_PORT) == RES_UARTX_1)
	#define MAP_UART1_RX			PERIPHERAL_0_ID_UART1_RX,
#else
	#define MAP_UART1_RX
#endif
#if defined(CFG_COMMUNICATION_BY_UART) && (GET_UARTIO_UARTX(CFG_FUNC_COMMUNICATION_TX_PORT) == RES_UARTX_1) \
	|| defined(CFG_UPGRADE_BY_UART) && (GET_UARTIO_UARTX(CFG_FUNC_UPGRADE_TX_DMA_PORT) == RES_UARTX_1)	\
	|| defined(CFG_DUMP_DEBUG_EN) && (GET_UARTIO_UARTX(CFG_DUMP_UART_TX_PORT) == RES_UARTX_1)
	#define MAP_UART1_TX			PERIPHERAL_0_ID_UART1_TX,
#else
	#define MAP_UART1_TX
#endif
#if defined(SPDIF0_OUT_DMA_ID)
	#define MAP_SPDIF0_TX			PERIPHERAL_0_ID_SPDIF0_TX,
#else
	#define MAP_SPDIF0_TX
#endif

#if defined(CFG_DMA_RGB_LED_EN) && (USE_TIMER < TIMER7_PWM0)
	#define MAP_RGB_RX0				DMA_MASK,
#else
	#define MAP_RGB_RX0
#endif

#define	MAP0_COMMON_FIRST \
			MAP_SDIO_RX \
			MAP_SDIO_TX \
			MAP_UART0_RX \
			MAP_UART0_TX \
			MAP_UART1_RX \
			MAP_UART1_TX \
			MAP_SPDIF0_TX \
			MAP_RGB_RX0

/**Map1**/
#if (CFG_RES_MIC_SELECT) && defined(CFG_FUNC_MIC_EN) && (MIC_INPUT_CHANNEL == ANA_INPUT_CH_MIC0) \
	|| defined(CFG_FUNC_LINEIN_MIX_MODE) && (LINEIN_INPUT_CHANNEL == ANA_INPUT_CH_LINEIN)
	#define MAP_ADC0_RX			PERIPHERAL_1_ID_AUDIO_ADC0_RX,
#else
	#define MAP_ADC0_RX
#endif
#if (CFG_RES_MIC_SELECT) && defined(CFG_FUNC_MIC_EN) && (MIC_INPUT_CHANNEL == ANA_INPUT_CH_MIC1) \
	|| defined(CFG_FUNC_LINEIN_MIX_MODE) && (LINEIN_INPUT_CHANNEL != ANA_INPUT_CH_LINEIN)
	#define MAP_ADC1_RX			PERIPHERAL_1_ID_AUDIO_ADC1_RX,
#else
	#define MAP_ADC1_RX
#endif
#if defined(CFG_RES_DAC_MONITOR_EN) && DAC_OUTPUT_CHANNEL == MONITOR_DAC0
	#define MAP_DAC0_TX			PERIPHERAL_1_ID_AUDIO_DAC0_TX,
#else
	#define MAP_DAC0_TX
#endif
#if defined(CFG_RES_DAC_MONITOR_EN) && DAC_OUTPUT_CHANNEL == MONITOR_DAC1 \
	|| defined(CFG_RES_AUDIO_DAC1_MIX_OUT_EN)
	#define MAP_DAC1_TX			PERIPHERAL_1_ID_AUDIO_DAC1_TX,
#else
	#define MAP_DAC1_TX
#endif
#if  defined(CFG_RES_AUDIO_DAC2_MIX_OUT_EN)
	#define MAP_DAC2_TX			PERIPHERAL_1_ID_AUDIO_DAC2_TX,
#else
	#define MAP_DAC2_TX
#endif
#ifdef CFG_I2S0_MIX_IN_EN
	#define MAP_I2S0_RX			PERIPHERAL_1_ID_I2S0_RX,
#else
	#define MAP_I2S0_RX
#endif
#ifdef CFG_I2S0_MIX_OUT_EN
	#define MAP_I2S0_TX			PERIPHERAL_1_ID_I2S0_TX,
#else
	#define MAP_I2S0_TX
#endif
#ifdef CFG_I2S1_MIX_IN_EN
	#define MAP_I2S1_RX			PERIPHERAL_1_ID_I2S1_RX,
#else
	#define MAP_I2S1_RX
#endif
#ifdef CFG_I2S1_MIX_OUT_EN
	#define MAP_I2S1_TX			PERIPHERAL_1_ID_I2S1_TX,
#else
	#define MAP_I2S1_TX
#endif
#ifdef CFG_I2S2_MIX_IN_EN
	#define MAP_I2S2_RX			PERIPHERAL_1_ID_I2S2_RX,
#else
	#define MAP_I2S2_RX
#endif
#ifdef CFG_I2S2_MIX_OUT_EN
	#define MAP_I2S2_TX			PERIPHERAL_1_ID_I2S2_TX,
#else
	#define MAP_I2S2_TX
#endif
#if defined(CFG_COMMUNICATION_BY_UART) && (GET_UARTIO_UARTX(CFG_FUNC_COMMUNICATION_RX_PORT) == RES_UARTX_2) \
	|| defined(CFG_UPGRADE_BY_UART) && (GET_UARTIO_UARTX(CFG_FUNC_UPGRADE_RX_DMA_PORT) == RES_UARTX_2)
	#define MAP_UART2_RX			PERIPHERAL_1_ID_UART2_RX,
#else
	#define MAP_UART2_RX
#endif
#if defined(CFG_COMMUNICATION_BY_UART) && (GET_UARTIO_UARTX(CFG_FUNC_COMMUNICATION_TX_PORT) == RES_UARTX_2) \
	|| defined(CFG_UPGRADE_BY_UART) && (GET_UARTIO_UARTX(CFG_FUNC_UPGRADE_TX_DMA_PORT) == RES_UARTX_2)	\
	|| defined(CFG_DUMP_DEBUG_EN) && (GET_UARTIO_UARTX(CFG_DUMP_UART_TX_PORT) == RES_UARTX_2)
	#define MAP_UART2_TX			PERIPHERAL_1_ID_UART2_TX,
#else
	#define MAP_UART2_TX
#endif
#if defined(SPDIF1_OUT_DMA_ID)
	#define MAP_SPDIF1_TX			PERIPHERAL_1_ID_SPDIF1_TX,
#else
	#define MAP_SPDIF1_TX
#endif

#if defined(CFG_DMA_RGB_LED_EN) && (USE_TIMER >= TIMER7_PWM0)
	#define MAP_RGB_RX1				DMA_MASK,
#else
	#define MAP_RGB_RX1
#endif

#define	MAP1_COMMON_FIRST \
			MAP_ADC0_RX \
			MAP_ADC1_RX \
			MAP_DAC0_TX \
			MAP_DAC1_TX \
			MAP_DAC2_TX \
			MAP_I2S0_RX \
			MAP_I2S0_TX \
			MAP_I2S1_RX \
			MAP_I2S1_TX \
			MAP_I2S2_RX \
			MAP_I2S2_TX \
			MAP_SPDIF1_TX \
			MAP_RGB_RX1


/**********************************************
 * 			HWSRC Module
 * 三路硬件转采样通道预 分配:
 * 1、应用锁定：Case FIX, 自行Init/Apply
 * 2、APPmode锁定：  SRCx_MODE_LIST 对应模式内自行 Init/Apply
 * 3、适配层调配：audio core adapt挂接和Apply
*********************************************/
#define AUDIOCORE_HWSRC_EN
#ifdef AUDIOCORE_HWSRC_EN
	#define SRC_MODE_LIST_MAX					2//每个SRC暂限2 mode锁定,按需调整
	#if defined(CFG_RES_USB_HS_MIX_AUDIO_EN) && defined(CFG_OTG_MODE_AUDIO_EN) && defined(CFG_FUNC_USB_SPEAKER_HW_SRC_EN) && (CFG_USB_SPEAKER_HW_SRC_PORT == RES_SRC_0)
		#define		HWSRC0_FIX_CASE				HWSRC_HS_MIX_SPEAKER
	#elif defined(CFG_APP_USB_AUDIO_MODE_EN) && defined(CFG_OTG_MODE_AUDIO_EN) && defined(CFG_FUNC_USB_SPEAKER_HW_SRC_EN) && (CFG_USB_SPEAKER_HW_SRC_PORT == RES_SRC_0)
		#define		HWSRC0_MODE_LIST			{{ModeUsbDevicePlay, HWSRC_MODE_SPEAKER}}
	#endif

	#if defined(CFG_RES_USB_HS_MIX_AUDIO_EN) && defined(CFG_OTG_MODE_MIC_EN) && defined(CFG_FUNC_USB_MIC_HW_SRC_EN) && (CFG_USB_MIC_HW_SRC_PORT == RES_SRC_1)
		#define		HWSRC1_FIX_CASE				HWSRC_HS_MIX_USBMIC
	#elif defined(CFG_APP_USB_AUDIO_MODE_EN) && defined(CFG_OTG_MODE_MIC_EN) && defined(CFG_FUNC_USB_MIC_HW_SRC_EN) && (CFG_USB_MIC_HW_SRC_PORT == RES_SRC_1)
		#define		HWSRC1_MODE_LIST			{{ModeUsbDevicePlay, HWSRC_MODE_USBMIC}}
	#endif

	#if defined(CFG_BRIDGE_HW_SRC_PORT) && (CFG_BRIDGE_HW_SRC_PORT == RES_SRC_2) \
										&& (defined(BRIDGE_MUSIC_HWSRC_EN) || defined(BRIDGE_MUSIC2_HWSRC_EN) || defined(BRIDGE_MIC_HWSRC_EN)|| defined(BRIDGE_MIC2_HWSRC_EN))
		#define		HWSRC2_FIX_CASE				HWSRC_BRIDGE
	#endif

	#if !(defined(HWSRC0_FIX_CASE) && defined(HWSRC1_FIX_CASE) && defined(HWSRC2_FIX_CASE))
		#define		HWSRC_MODE_CONFIG_EN //有资源可调配
	#endif
	#if defined(HWSRC0_MODE_LIST) || defined(HWSRC1_MODE_LIST) || defined(HWSRC2_MODE_LIST)
		#define		HWSRC_MODE_LIST_SCAN_EN //有模式相关锁定
	#endif
#endif//AUDIOCORE_HWSRC_EN

#define		OSTICK_RELOAD		1000//725//us

#if !defined(CFG_FUNC_DEBUG_EN) && !defined(CFG_FUNC_USBDEBUG_EN)
	#undef	DBG_MACRO
	#define DBG_MACRO(format, ...)
#endif //关映射,降codebin

/**据音频采样率调整系统,未移植适配多图**/
//Porting #define CFG_AUDIO_OUT_AUTO_SAMPLE_RATE_44100_48000

/****************************************Module conflict warning**************************************************
 * SDK应用配置冲突管理
 *****************************************************************************************************************/
#if defined(CFG_UART_LOG_EN) \
	&& (CHIP_IO_ERR(CFG_UART_TX_PORT) \
			|| (OTG_FS_IO(CFG_UART_TX_PORT) \
					&& (defined(CFG_APP_USB_PLAY_MODE_EN) \
						|| defined(CFG_APP_USB_FS_AUDIO_MODE_EN)\
						|| CFG_PARA_HOST_FS_DET \
						|| CFG_PARA_DEVICE_FS_DET \
						|| defined(CFG_COMMUNICATION_BY_FS_USB))))//HS/FS双调音
	#error Conflict: log Pin 无引脚或功能冲突!!!
#endif

#if defined(CFG_192KHZ_EN) && defined(CFG_FUNC_MULTISOURCE_EN)
	#warning Conflict: Multisource Mix / 192K Samplerate 扩展Source资源消耗高!!!
#endif

#if (defined(CFG_APP_USB_HS_PLAY_MODE_EN) ||defined(CFG_RES_USB_HS_MIX_AUDIO_EN) || CFG_PARA_DEVICE_HS_DET || CFG_PARA_HOST_HS_DET) && ! defined(CHIP_PIN_HSOTG)
	#error Conflict: chip not support HSOTG 芯片无高速USB口!!!
#endif
#if (defined(CFG_APP_USB_AUDIO_MODE_EN) && defined(CFG_RES_USB_HS_MIX_AUDIO_EN)) || (defined(CFG_APP_USB_FS_AUDIO_MODE_EN) && defined(CFG_RES_USB_FS_MIX_AUDIO_EN))
	#error Conflict: OTG Module 配置错误 !!!
#endif
#if defined(CFG_PARA_USB_FS_MODE) && (CFG_PARA_USB_FS_MODE < FS_DESC_BASE_OFFSET)
	#error Conflict: FS端口不能使用HS描述符!!!
#endif
/**AD、DA**/
#if (CFG_RES_MIC_SELECT) && defined(CFG_FUNC_MIC_EN) && defined(CFG_FUNC_LINEIN_MIX_MODE) && defined(CFG_APP_RADIOIN_MODE_EN)
	#error Conflict: AD Module不足!!! 谨慎配置
#endif
#if (DAC_OUTPUT_CHANNEL == MONITOR_DAC1) && defined(CFG_RES_AUDIO_DAC1_MIX_OUT_EN)
	#error Conflict: DAC_MONITOR/Mix冲突!!! 谨慎配置
#endif

/**I2S_MODULE提示, GPIO自行调配**/
#if defined(CFG_RES_I2S_MODULE) && (  ((CFG_RES_I2S_MODULE == 0) && defined(CFG_FUNC_I2S0_MIX)) \
		                       || ((CFG_RES_I2S_MODULE == 1) && defined(CFG_FUNC_I2S1_MIX))  \
							   || ((CFG_RES_I2S_MODULE == 2) && defined(CFG_FUNC_I2S2_MIX)))
	#error Conflict: I2S App Mode/Mix冲突!!! 谨慎配置
#endif

/**RF Module**/
#if FUNC_RF_EN && BT_LE_AUDIO_SUPPORT && !BLE_SUPPORT
	#error Conflict: BT_LE_AUDIO_SUPPORT / BLE_SUPPORT!!! 请调整
#endif
#if (FUNC_RF_EN == 1) && !defined(CHIP_PIN_RF)
	#error Conflict: chip not support RF 芯片不支持无线应用!!!
#endif
//CFG_RES_AUDIO_DAC2_MIX_OUT_EN
#if defined(ROBOEFFECT_DOUBLE_ENGINE_EN) && !defined(CFG_RES_AUDIO_DAC2_MIX_OUT_EN)
	#error Conflict: music_KaraOK DualFlow need SyncFrame 音频流节拍器和调试需求!!!
#endif
#endif /* APP_MODULE_CONFIG_H_ */
