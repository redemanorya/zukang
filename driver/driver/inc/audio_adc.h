/**
 *******************************************************************************
 * @file    audio_adc.h
 * @brief	模拟∑Δ调制A/D转换器（ASDM）驱动程序接口
 *
 * @author  Mike
 * @version V1.1.0
 *
 * $Created: 2025-01-16 13:27:11$
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 *******************************************************************************
 */

/**
 * @addtogroup AUDIO_ADC
 * @{
 * @defgroup audio_adc audio_adc.h
 * @{
 */

#ifndef __AUDIO_ADC_H__
#define __AUDIO_ADC_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

////////////////////////////////////////////
//     Gain Volume Table for PGA(dB)      //
////////////////////////////////////////////
//	Vol // Gain of Mic 单端(600ohm)// Gain of Mic 差分(600ohm)  // Gain of LineIn/AUX //
//  00	//     28.18 		       //    34.987 	         	//    16.669      //
//  01	//     25.84  		       //    31.293 	         	//    14.785      //
//  02	//     23.66  		       //    28.271 	         	//    12.822      //
//  03	//     21.47  		       //    25.461              	//    10.883      //
//  04	//     19.37  		       //    22.924 	         	//    9.021       //
//  05	//     17.28	  		   //    20.513  		        //    7.044       //
//  06	//     15.287	  		   //    18.272  		        //    5.098       //
//  07	//     13.281	  		   //    16.08  		        //    3.103       //
//  08	//     11.225     		   //    13.878    		     	//    1.135       //
//  09	//     9.19	  		       //    11.725  		        //    -0.822      //
//  10	//     7.209	  		   //    9.657  		        //    -2.848      //
//  11	//     5.184	  		   //    7.562  		        //    -4.854      //
//  12	//     3.175	  		   //    5.499  		        //    -6.865      //
//  13	//     1.169	  		   //    3.449  		        //    -8.874      //
//  14	//     -0.843	  		   //    1.407  		        //    -10.873     //
//  15	//     -2.81	  		   //    -0.155 		        //    -12.878     //
//  16	//     -4.856      		   //    -2.654  		     	//                //
//  17	//     -6.857      		   //    -4.672 	         	//                //
//  18	//     -8.862      		   //    -6.692 	     		//                //
//  19	//     -10.864      	   //    -8.703   				//                //
//  20	//     -12.865      	   //    -10.713   				//                //
//  21	//     -14.864      	   //    -12.721   				//                //
//  22	//     -16.863      	   //    -14.728 				//                //
//  23	//     -18.864      	   //    -16.738     			//                //
//  24	//     -20.86      		   //    -18.73     			//                //
//  25	//     -22.848     	       //    -20.722  				//                //
//  26	//     -24.83      		   //    -22.7     				//                //
//  27	//     -26.85     		   //    -24.73     			//                //
//  28	//     -28.81     		   //    -26.71     			//                //
//  29	//     -30.785     		   //    -28.67   				//                //
//  30	//     -32.753             //    -30.65       			//                //
//  31	//     -34.789      	   //    -32.76       			//                //
////////////////////////////////////////////

/**
 * ADC 模块
 */
typedef enum _AUDIOADC_MODULE
{
    ADC0_MODULE = 0,
    ADC1_MODULE
} AUDIOADC_MODULE;

typedef enum _MICBIAS_MODULE
{
    MICBIAS1 = 0,
    MICBIAS2
} MICBIAS_MODULE;

typedef enum _AUDIOADC_CHANNEL
{
    ADC_LEFT = 0,
    ADC_RIGHT
} AUDIOADC_CHANNEL;

typedef enum _AUDIOADC_DMIC_DSR
{
    DOWN_SR_64 = 0,
	DOWN_SR_128
} AUDIOADC_DMIC_DSR;

typedef enum AUDIOADC_INPUT
{
	ADC_NONE = 0,				//none，用于关闭当前PGA下的channel选择

	ADC0_MICL,
	ADC0_LineinL,

	ADC0_MICR,
	ADC0_LineinR,

	ADC1_MICL,
	ADC1_AUX0L_B20,
	ADC1_AUX1L_B18,

	ADC1_MICR,
	ADC1_AUX0R_B21,
	ADC1_AUX1R_B19,
} AUDIOADC_INPUT;

/**
 * AGC 模式使能选择
 */
typedef enum _AGC_CHANNEL
{
    AGC_DISABLE	        = 0x00,       /**<禁止AGC功能 */
    AGC_RIGHT_ONLY      = 0x01,       /**<仅右通道使能AGC功能*/
    AGC_LEFT_ONLY       = 0x02,       /**<仅左通道使能AGC功能*/
    AGC_STEREO_OPEN     = 0x03        /**<左右双通道使能AGC功能*/
} AGC_CHANNEL;

/**
 * ADC 位宽选择
 */
typedef enum _AUDIOADC_BitWidth
{
	ADC_16BIT = 0,
	ADC_24BIT,
	ADC_32BIT
} AUDIOADC_BitWidth;

/**
 * MIC 模式选择
 */
typedef enum _AUDIOADC_Mode
{
	ADC_MIC_Single = 0,   //单端
	ADC_MIC_Diff,     	  //差分
	ADC_Linein
} AUDIOADC_Mode;

typedef enum _AUDIOADC_EnergyModel
{
	ADCLowEnergy = 0, //低功耗模式
	ADCCommonEnergy
} AUDIOADC_EnergyModel;

/**
 * @brief  ADC 模块左右通道使能选择
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  IsLeftEn     TRUE,左通道能使; FALSE,左通道关闭
 * @param  IsRightEn    TRUE,右通道能使; FALSE,右通道关闭
 * @return 无
 */
void AudioADC_LREnable(AUDIOADC_MODULE ADCModule, bool IsLeftEn, bool IsRightEn);

/**
 * @brief  ADC 模块使能（总开关）
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @return 无
 */
void AudioADC_Enable(AUDIOADC_MODULE ADCModule);

/**
 * @brief  ADC 模块关闭
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @return 无
 */
void AudioADC_Disable(AUDIOADC_MODULE ADCModule);

/**
 * @brief  ADC 模块是否交换左右通道输入
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  IsSwap       TRUE,左右通道交换输入; FALSE,左右通道正常输入
 * @return 无
 */
void AudioADC_ChannelSwap(AUDIOADC_MODULE ADCModule, bool IsSwap);

/**
 * @brief  ADC 模块高通滤波器截止频率参数配置
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  Coefficient  滤波器系数，14bit位宽，默认值0x3FF8。
 * @arg  Coefficient = 0x3FF8
 * @arg  Coefficient = 0x3FFC
 * @arg  Coefficient = 0x3FFE
 * @arg  Coefficient = 0x3FFF
 * @return 无
 * @Note 该函数必须在AudioADC_Enable()函数调用之后，recommended value(FR: 3.7hz --- -3dB for all fs):
 */
void AudioADC_HighPassFilterConfig(AUDIOADC_MODULE ADCModule, uint16_t Coefficient);

/**
 * @brief  ADC 模块高通滤波是否使能（去除直流偏量）
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  IsHpfEn      TRUE,能使高通滤波器，FALSE,关闭高通滤波器
 * @return 无
 */
void AudioADC_HighPassFilterSet(AUDIOADC_MODULE ADCModule, bool IsHpfEn);

/**
 * @brief  ADC 模块清除寄存器和内存中的数值
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @return 无
 */
void AudioADC_Clear(AUDIOADC_MODULE ADCModule);

/**
 * @brief  adc 位宽设置
 * @param  ADCModule: ADC0_MODULE,  ADC1_MODULE
 * @param  BitWidth  ADC_32BIT：32位；ADC_24BIT：24位; ADC_16BIT：16位
 * @return 无
 */
void AudioADC_BitWidthSet(AUDIOADC_MODULE ADCModule, AUDIOADC_BitWidth BitWidth);

/**
 * @brief  获取adc 位宽
 * @param  ADCModule: ADC0_MODULE,  ADC1_MODULE
 * @return ADC_32BIT：32位；ADC_24BIT：24位; ADC_16BIT：16位
 */
AUDIOADC_BitWidth AudioADC_BitWidthGet(AUDIOADC_MODULE ADCModule);

/**
 * @brief  ADC 模块采样率配置
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  SampleRate   ADC采样率值  8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000,
 * 					  	64000, 88200, 96000, 176400, 192000, 352800, 384000 are supported.
 * @return 无
 */
void AudioADC_SampleRateSet(AUDIOADC_MODULE ADCModule, uint32_t SampleRate);

/**
 * @brief  获取ADC 模块采样率配置值
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @return 当前采样率配置值
 */
uint32_t AudioADC_SampleRateGet(AUDIOADC_MODULE ADCModule);

/**
 * @brief  ADC 模块淡入淡出时间设置
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  FadeTime     淡入淡出时间, 单位:Ms
 * @return 无
 * @Note   建议时间为10Ms，时间不能设置为0，如果想关闭淡入淡出功能请调用函数AudioADC_FadeDisable();
 */
void AudioADC_FadeTimeSet(AUDIOADC_MODULE ADCModule, uint8_t FadeTime);

/**
 * @brief  ADC 模块淡入淡出功能使能
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @return 无
 */
void AudioADC_FadeEnable(AUDIOADC_MODULE ADCModule);

/**
 * @brief  ADC 模块淡入淡出功能禁用
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @return 无
 */
void AudioADC_FadeDisable(AUDIOADC_MODULE ADCModule);

/**
 * @brief  ADC 模块数字静音控制，左右通道分别独立控制
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  LeftMuteEn   TRUE,左通道静音使能; FALSE,左通道取消静音
 * @param  RightMuteEn  TRUE,右通道静音使能; FALSE,右通道取消静音
 * @return 无
 * @Note   该函数内部不带延时，配置硬件寄存器之后立即退出。如果需要等待静音动作完成，则要在函数外部做延时
 */
void AudioADC_DigitalMute(AUDIOADC_MODULE ADCModule, bool LeftMuteEn, bool RightMuteEn);

/**
 * @brief  ADC 模块软件静音控制，左右通道分别独立控制
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  LeftMuteEn   TRUE,左通道静音使能; FALSE,左通道取消静音
 * @param  RightMuteEn  TRUE,右通道静音使能; FALSE,右通道取消静音
 * @note   调用此API之前必须已经成功调用SysTickInit
 * @return 无
 * @Note   该函数内部带延时，如果是静音操作，会延时等待音量降低到0之后退出该函数。
 */
void AudioADC_SoftMute(AUDIOADC_MODULE ADCModule, bool LeftMuteEn, bool RightMuteEn);

/**
 * @brief  功能重置
 * @param  ADCModule 	0,ADC0模块; 1,ADC1模块
 * @return 无
 */
void AudioADC_FuncReset(AUDIOADC_MODULE ADCModule);

/**
 * @brief  寄存器重置
 * @param  无
 * @return 无
 */
void AudioADC_RegReset(void);

/**
 * @brief  ADC 模块音量设置
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  LeftVol      左声道音量值，0x00:静音, 0x001:-72dB, 0x1000:0dB
 * @param  RightVol     右声道音量值，0x00:静音, 0x001:-72dB, 0x1000:0dB
 * @return 无
 */
void AudioADC_VolSet(AUDIOADC_MODULE ADCModule, uint16_t LeftVol, uint16_t RightVol);

/**
 * @brief  ADC 模块音量设置（左右声道独立设置）
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  ChannelSel   左右声道选择，0x00: 无；0x1：左声道；0x2：右声道
 * @param  Vol     		音量值，0x00:静音, 0x001:-72dB, 0x1000:0dB、
 * @note   当ChannelSel为3时，同时选择了左右声道，此时Vol配置对左右声道均有效，左右声道音量值一致
 * @return 无
 */
void AudioADC_VolSetChannel(AUDIOADC_MODULE ADCModule, AUDIOADC_CHANNEL ChannelSel, uint16_t Vol);

/**
 * @brief  ADC 模块音量获取
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  *LeftVol      左声道音量值，0x00:静音, 0x001:-72dB, 0x1000:0dB
 * @param  *RightVol     右声道音量值，0x00:静音, 0x001:-72dB, 0x1000:0dB
 * @return 无
 */
void AudioADC_VolGet(AUDIOADC_MODULE ADCModule, uint16_t *LeftVol, uint16_t *RightVol);

/**
 * @brief  ADC PAG通道选择,
 * @param  ADCModule	0,ADC0模块; 1,ADC1模块
 * @param  ChannelSel   CHANNEL_LEFT:Left;	CHANNEL_RIGHT:Right
 * @param  InputSel 	PGA输入通路选择。具体见AUDIO_ADC_INPUT枚举值含义
 * @note   ADC1只支持MIC，没有AUX选通
 * @return 无
 */
void AudioADC_PGASel(AUDIOADC_MODULE ADCModule, AUDIOADC_CHANNEL ChannelSel, AUDIOADC_INPUT InputSel);

/**
 * @brief  ADC PAG增益配置
 * @param  ADCModule	0,ADC0模块; 1,ADC1模块
 * @param  ChannelSel   CHANNEL_LEFT:Left;	CHANNEL_RIGHT:Right
 * @param  InputSel 	InputSel PGA输入通路选择。具体见AUDIO_ADC_INPUT枚举值含义
 * @param  Gain 		PGA增益设置。配置范围【0-31】。
 * @return 无
 */
void AudioADC_PGAGainSet(AUDIOADC_MODULE ADCModule, AUDIOADC_CHANNEL ChannelSel, AUDIOADC_INPUT InputSel, uint16_t Gain);

/**
 * @brief  ADC模块PGA模块模式选择
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  ChannelSel	CHANNEL_LEFT:Left;	CHANNEL_RIGHT:Right
 * @param  AUDIOMode  	Single：Mic单端; Diff：Mic差分; ADC_Linein: Linein/AUX使用
 * @return 无
 */
void AudioADC_PGAMode(AUDIOADC_MODULE ADCModule, AUDIOADC_CHANNEL ChannelSel, AUDIOADC_Mode AUDIOMode);

/**
 * @brief  AGC模块PGA模块过零检测使能
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  IsLeftEn:  	左通道使能
 * @param  IsRightEn: 	右通道使能
 * @return 无
 */
void AudioADC_PGAZeroCrossEnable(AUDIOADC_MODULE ADCModule, bool IsLeftEn, bool IsRightEn);

/**
 * @brief  ASDM模块使能DMIC功能
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @return 无
 */
void AudioADC_DMICEnable(AUDIOADC_MODULE ADCModule);

/**
 * @brief  ASDM模块禁能DMIC功能
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @return 无
 */
void AudioADC_DMICDisable(AUDIOADC_MODULE ADCModule);

/**
 * @brief  DMIC降采样率倍数选择
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  DownSampleRate    DOWN_SR_64,64倍降采样; DOWN_SR_128,128倍降采样
 * @return 无
 * @note   当DownSampleRate为DOWN_SR_128时，确保DMIC输出时钟(128*fs)不大于DMIC设备限制
 */
void AudioADC_DMICDownSampleSel(AUDIOADC_MODULE ADCModule, AUDIOADC_DMIC_DSR DownSampleRate);

/**
 * @brief  Dynamic-Element-Matching enable signal
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  IsLeftEn  	左通道使能
 * @param  IsRightEn	右通道使能
 * @return 无
 */
void AudioADC_DynamicElementMatch(AUDIOADC_MODULE ADCModule, bool IsLeftEn, bool IsRightEn);

/**
 * @brief  查询ADC采样过程是否数据溢出
 * @param  ADCModule 	0,ADC0模块; 1,ADC1模块
 * @return TREU：有溢出过
 */
bool AudioADC_IsOverflow(AUDIOADC_MODULE ADCModule);

/**
 * @brief 清除ADC采样过程是否数据溢出标志
 * @param  ADCModule 	0,ADC0模块; 1,ADC1模块
 * @return 无
 */
void AudioADC_OverflowClear(AUDIOADC_MODULE ADCModule);

/**
 * @brief  ADC 模块AGC模块通道选择
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  IsLeftEn  	左通道使能
 * @param  IsRightEn	右通道使能
 * @return 无
 */
void AudioADC_AGCChannelSel(AUDIOADC_MODULE ADCModule, bool IsLeftEn, bool IsRightEn);

/**
 * @brief  ADC 模块用于整理AGC增益
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  GainOffset 	增益偏移设置。【8 ~ 15】-->【-4dB ~ -0.5dB】;【0 ~ 7】-->【0dB ~ 3.5dB】.
 * @return 无
 */
void AudioADC_AGCGainOffset(AUDIOADC_MODULE ADCModule, uint8_t GainOffset);

/**
 * @brief  ADC 模块AGC模块增益最大水平
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  MaxLevel     AGC模块增益最大水平.【0 ~ 31】-->【-3 ~ -34dB】
 * @return 无
 */
void AudioADC_AGCMaxLevel(AUDIOADC_MODULE ADCModule, uint8_t MaxLevel);

/**
 * @brief  AGC模块增益目标水平
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  TargetLevel	AGC模块增益目标水平.【0 ~ 31】-->【-3 ~ -34dB】
 * @return 无
 */
void AudioADC_AGCTargetLevel(AUDIOADC_MODULE ADCModule, uint8_t TargetLevel);

/**
 * @brief  AGC模模块可调节的最大增益
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  MaxGain		AGC模块最大增益.【0 ~ 31】-->【 32.75 ~ -6dB】,step:-1.25dB.
 * @return 无
 */
void AudioADC_AGCMaxGain(AUDIOADC_MODULE ADCModule, uint8_t MaxGain);

/**
 * @brief  AGC模模块可调节的最小增益
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  MinGain		AGC模块最小增益.【0 ~ 31】-->【 32.75 ~ -6dB】,step:-1.25dB.
 * @return 无
 */
void AudioADC_AGCMinGain(AUDIOADC_MODULE ADCModule, uint8_t MinGain);

/**
 * @brief  AGC模块帧时间
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  FrameTime	帧时间设置。 单位：ms。 范围【 1 ~ 4096】
 * @return 无
 */
void AudioADC_AGCFrameTime(AUDIOADC_MODULE ADCModule, uint16_t FrameTime);

/**
 * @brief  AGC模块保持时间，开始AGC算法自适应增益之前的保持时间。
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  HoldFrames	AGC开始算法之前的保持时间。单位：FrameTime的倍数。范围【0 ~ 31】
 * @return 无
 */
void AudioADC_AGCHoldFrames(AUDIOADC_MODULE ADCModule, uint32_t HoldFrames);

/**
 * @brief  AGC模块当输入信号太大时，AGC增益衰减的步进时间设置。
 * @param  ADCModule    	0,ADC0模块; 1,ADC1模块
 * @param  AttackStepTime 	AGC增益增强的步进时间，单位为ms,范围为1 ~ 4096 ms
 * @return 无
 */
void AudioADC_AGCAttackStepTime(AUDIOADC_MODULE ADCModule, uint16_t AttackStepTime);

/**
 * @brief  AGC模块当输入信号太小时，AGC增益增强的步进时间设置。
 * @param  ADCModule    	0,ADC0模块; 1,ADC1模块
 * @param  DecayStepTime 	AGC增益增强的步进时间，单位为ms,范围为1 ~ 4096 ms
 * @return 无
 */
void AudioADC_AGCDecayStepTime(AUDIOADC_MODULE ADCModule, uint16_t DecayStepTime);

/**
 * @brief  AGC模块AGC噪声阈值设置
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  NoiseThreshold： 噪音阈值设置,范围：【0 ~ 31】对应值：【-90dB ~ -28dB】，step：2dB
 * 						      默认为 01111,即-60 dB
 * @return 无
 */
void AudioADC_AGCNoiseThreshold(AUDIOADC_MODULE ADCModule, uint8_t NoiseThreshold);

/**
 * @brief  AGC模块AGC模块噪声模式选择,
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  NoiseMode：	0: ADC输出数据电平与噪声阈值判断，确定该输入是否为噪声；
 * 						1: ADC输入数据电平与噪声阈值判断，确定该输入是否为噪声。
 * @return 无
 */
void AudioADC_AGCNoiseMode(AUDIOADC_MODULE ADCModule, uint8_t NoiseMode);

/**
 * @brief  AGC模块AGC模块噪声Gate功能使能
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  NoiseGateEnable： 0:禁止噪声Gate功能; 1:使能噪声Gate功能
 * @return 无
 */
void AudioADC_AGCNoiseGateEnable(AUDIOADC_MODULE ADCModule, bool NoiseGateEnable);

/**
 * @brief  AGC模块AGC模块噪声Gate模式选择
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  NoiseGateMode 	0:  当检查到过零信号时，PCM数据mute/unmute
 *							1:  PCM数据立即mute/unmute
 * @return 无
 */
void AudioADC_AGCNoiseGateMode(AUDIOADC_MODULE ADCModule, uint8_t NoiseGateMode);


/**
 * @brief  AGC模块AGC模块噪声保持时间设置。
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @param  NoiseHoldFrames	噪声持续FrameTime*NoiseHoldFrames ms后，噪声相关算法开始执行。单位：FrameTime的倍数。范围【0 ~ 31】
 * @return 无
 */
void AudioADC_AGCNoiseHoldFrames(AUDIOADC_MODULE ADCModule, uint8_t NoiseHoldFrames);

/**
 * @brief  AGC模块获取AGC增益
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @return AGC增益
 * @note   返回的值并不是AGC的实际增益值，AGC实际增益值与寄存器值
 * 		        之间的转换见AGC实际增益值与寄存器值对应表
 */
uint8_t AudioADC_AGCGainGet(AUDIOADC_MODULE ADCModule);

/**
 * @brief  AGC模块获取AGC模块静音信息
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @return AGC模块静音标志
 */
uint8_t AudioADC_AGCMuteGet(AUDIOADC_MODULE ADCModule);

/**
 * @brief  AGC模块获取AGC模块更新标志位
 * @param  ADCModule    0,ADC0模块; 1,ADC1模块
 * @return AGC模块更新标志位
 */
uint8_t AudioADC_AGCUpdateFlagGet(AUDIOADC_MODULE ADCModule);

/**
 * @brief  AGC模块清除AGC模块更新标志位
 * @param  ADCModule 	0,ADC0模块; 1,ADC1模块
 * @return 无
 */
void AudioADC_AGCUpdateFlagClear(AUDIOADC_MODULE ADCModule);

/**
 * @brief  BIAS上电
 * @param  MICBIASMODULE 	0,MICBIAS1模块; 1,MICBIAS2模块
 * @return 无
 * @note   ECO0、ECO1芯片不支持使用micbias
 */
void AudioADC_BIASPowerOn(MICBIAS_MODULE MICBIASMODULE);

/**
 * @brief  BIAS掉电
 * @param  MICBIASMODULE 	0,MICBIAS1模块; 1,MICBIAS2模块
 * @return 无
 * @note   ECO0、ECO1芯片不支持使用micbias
 */
void AudioADC_BIASPowerOff(MICBIAS_MODULE MICBIASMODULE);

/**
 * @brief  PGA PD控制
 * @param  ADCModule 	0,ADC0模块; 1,ADC1模块
 * @param  IsLeftEn 左声道PGAPowerUp; IsRightEn 右声道PGAPowerUp
 * @return 无
 */
void AudioADC_PGAPowerUp(AUDIOADC_MODULE ADCModule, bool IsLeftEn, bool IsRightEn);

/**
 * @brief  ADC PowerUp
 * @param  ADCModule 	0,ADC0模块; 1,ADC1模块
 * @param  IsLeftEn 左声道PowerUp; IsRightEn 右声道PowerUp
 * @return 无
 */
void AudioADC_PowerUp(AUDIOADC_MODULE ADCModule, bool IsLeftEn, bool IsRightEn);

/**
 * @brief  absmute 强制静音，放在内部,B5只有MIC有ABSMute
 * @param  ADCModule 	0,ADC0模块; 1,ADC1模块
 * @param  IsLeftEn 左声道PGAAbsMute; IsRightEn 右声道PGAAbsMutep
 * @return 无
 */
void AudioADC_PGAAbsMute(AUDIOADC_MODULE ADCModule, bool IsLeftEn, bool IsRightEn);


/**
 * @brief  PGA电流控制信号
 * @param  ADCModule 	0,ADC0模块; 1,ADC1模块
 * @param  LeftIBSEL 左声道电流值; IsRightEn 右声道电流值
 * @return 无
 */
void AudioADC_PGAIBiasSet(AUDIOADC_MODULE ADCModule,  uint8_t LeftIBSEL,  uint8_t RightIBSEL);

/**
 * @brief  PGA MUTE信号
 * @param  ADCModule 	0,ADC0模块; 1,ADC1模块
 * @param  IsLeftEn 左声道PGAMute使能; IsRightEn 左声道PGAMute使能
 * @return 无
 */
void AudioADC_PGAMute(AUDIOADC_MODULE ADCModule, bool IsLeftEn, bool IsRightEn);


void AudioADC_AnaPowerup(AUDIOADC_MODULE ADCModule, AUDIOADC_EnergyModel EnergyModel);

/**
 * @brief  vmid初始化
 * @param  无
 * @return 无
 * @note   只使用audio_adc,不使用dac的时候,可以使用AudioADC_VMIDInit替代AudioDAC_AllPowerOn来对vmid进行上电。
 * @note   AudioADC_VMIDInit和AudioDAC_AllPowerOn不要重复使用
 */
void AudioVMIDInit(void);

/**
 * @brief  vmid关闭
 * @param  无
 * @return 无
 */
void AudioVMIDDeInit(void);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif //__AUDIO_ADC_H__

/**
 * @}
 * @}
 */
