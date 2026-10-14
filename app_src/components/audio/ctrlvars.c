/**
 **************************************************************************************
 * @file    ctrlvars.c
 * @brief   Control Variables Definition
 * 
 * @author  Cecilia Wang
 * @version V1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include "nds32_intrinsic.h"
#include "type.h"
#include <string.h>
#include <stdint.h>
#include "clk.h"
#include "spi_flash.h"
#include "timeout.h"
#include "delay.h"
#include "dac.h"

#include "app_config.h"
#include "debug.h"
#include "ctrlvars.h"
#include "breakpoint.h"
#include "audio_adc.h"
#include "main_task.h"

ControlVariablesContext gCtrlVars;

#ifdef CFG_I2S_SLAVE_TO_SPDIFOUT_EN
SyncModuleContext gSyncModule;
#endif

const int16_t DigVolTab_16[16] =
{
	0,  	332,	460,	541,	636,	748,	880,	1035,  
	1218,	1433,	1686,	1984,	2434,	2946,	3500,	4096
};

const int16_t DigVolTab_32[32] =
{
	0,		31,		36,		42,		49,		58,		67,		78,
	91,		107,	125,	147,	173,	204,	240,	282,
	332,	391,	460,	541,	636,	748,	880,	1035,
	1218,	1433,	1686,	1984,	2434,	2946,	3500,	4096
};

static const uint32_t SupportSampleRateList[13]={
	8000,
    11025,
    12000,
    16000,
    22050,
    24000,
    32000,
    44100,
    48000,
//////i2s//////////////////
    88200,
    96000,
    176400,
    192000,
};

const uint16_t HPCList[3]={
	0xffe, //  48k 20Hz  -1.5db
	0xFFC, //  48k 40Hz  -1.5db
	0xFFD, //  32k 40Hz  -1.5db
};

//EQ style: Classical
const unsigned char EqMode_data_1[104] = {
    0x00, 0x00,/*pregain*/
	0x00, 0x00,/*calculation_type*/
    0x01, 0x00,/*filter1 enable*/
    0x01, 0x00,/*filter1 type*/
    0x0A, 0x01,/*filter1 f0*/
    0xB7, 0x00,/*filter1 Q*/
    0x28, 0x05,/*filter1 Gain*/
    0x01, 0x00,/*filter2 enable*/
    0x00, 0x00,/*filter2 type*/
    0x7F, 0x02,/*filter2 f0*/
    0x42, 0x03,/*filter2 Q*/
    0x88, 0xFB,/*filter2 Gain*/
    0x01, 0x00,/*filter3 enable*/
    0x00, 0x00,/*filter3 type*/
    0xEE, 0x06,/*filter3 f0*/
    0xD5, 0x02,/*filter3 Q*/
    0x2D, 0xFE,/*filter3 Gain*/
    0x00, 0x00,/*filter4 enable*/
    0x00, 0x00,/*filter4 type*/
    0xE8, 0x03,/*filter4 f0*/
    0xD4, 0x02,/*filter4 Q*/
    0x00, 0x00,/*filter4 Gain*/
    0x01, 0x00,/*filter5 enable*/
    0x02, 0x00,/*filter5 type*/
    0xB2, 0x0E,/*filter5 f0*/
    0x25, 0x01,/*filter5 Q*/
    0x91, 0x04,/*filter5 Gain*/
    0x00, 0x00,/*filter6 enable*/
    0x00, 0x00,/*filter6 type*/
    0xE8, 0x03,/*filter6 f0*/
    0xD4, 0x02,/*filter6 Q*/
    0x00, 0x00,/*filter6 Gain*/
    0x00, 0x00,/*filter7 enable*/
    0x00, 0x00,/*filter7 type*/
    0xE8, 0x03,/*filter7 f0*/
    0xD4, 0x02,/*filter7 Q*/
    0x00, 0x00,/*filter7 Gain*/
    0x00, 0x00,/*filter8 enable*/
    0x00, 0x00,/*filter8 type*/
    0xE8, 0x03,/*filter8 f0*/
    0xD4, 0x02,/*filter8 Q*/
    0x00, 0x00,/*filter8 Gain*/
    0x00, 0x00,/*filter9 enable*/
    0x00, 0x00,/*filter9 type*/
    0xE8, 0x03,/*filter9 f0*/
    0xD4, 0x02,/*filter9 Q*/
    0x00, 0x00,/*filter9 Gain*/
    0x00, 0x00,/*filter10 enable*/
    0x00, 0x00,/*filter10 type*/
    0xE8, 0x03,/*filter10 f0*/
    0xD4, 0x02,/*filter10 Q*/
    0x00, 0x00,/*filter10 Gain*/
};
//EQ style: Vocal Booster
const unsigned char EqMode_data_5[104] = {
    0x00, 0x00,/*pregain*/
	0x00, 0x00,/*calculation_type*/
    0x00, 0x00,/*filter1 enable*/
    0x01, 0x00,/*filter1 type*/
    0x0A, 0x01,/*filter1 f0*/
    0xB7, 0x00,/*filter1 Q*/
    0x28, 0x05,/*filter1 Gain*/
    0x01, 0x00,/*filter2 enable*/
    0x00, 0x00,/*filter2 type*/
    0x60, 0x00,/*filter2 f0*/
    0xFA, 0x01,/*filter2 Q*/
    0x0C, 0xFB,/*filter2 Gain*/
    0x01, 0x00,/*filter3 enable*/
    0x00, 0x00,/*filter3 type*/
    0x1F, 0x02,/*filter3 f0*/
    0x4D, 0x01,/*filter3 Q*/
    0x1A, 0x04,/*filter3 Gain*/
    0x01, 0x00,/*filter4 enable*/
    0x00, 0x00,/*filter4 type*/
    0xA3, 0x09,/*filter4 f0*/
    0x06, 0x03,/*filter4 Q*/
    0x17, 0x01,/*filter4 Gain*/
    0x01, 0x00,/*filter5 enable*/
    0x02, 0x00,/*filter5 type*/
    0xE3, 0x29,/*filter5 f0*/
    0xCD, 0x02,/*filter5 Q*/
    0x44, 0xFE,/*filter5 Gain*/
    0x00, 0x00,/*filter6 enable*/
    0x00, 0x00,/*filter6 type*/
    0xE8, 0x03,/*filter6 f0*/
    0xD4, 0x02,/*filter6 Q*/
    0x00, 0x00,/*filter6 Gain*/
    0x00, 0x00,/*filter7 enable*/
    0x00, 0x00,/*filter7 type*/
    0xE8, 0x03,/*filter7 f0*/
    0xD4, 0x02,/*filter7 Q*/
    0x00, 0x00,/*filter7 Gain*/
    0x00, 0x00,/*filter8 enable*/
    0x00, 0x00,/*filter8 type*/
    0xE8, 0x03,/*filter8 f0*/
    0xD4, 0x02,/*filter8 Q*/
    0x00, 0x00,/*filter8 Gain*/
    0x00, 0x00,/*filter9 enable*/
    0x00, 0x00,/*filter9 type*/
    0xE8, 0x03,/*filter9 f0*/
    0xD4, 0x02,/*filter9 Q*/
    0x00, 0x00,/*filter9 Gain*/
    0x00, 0x00,/*filter10 enable*/
    0x00, 0x00,/*filter10 type*/
    0xE8, 0x03,/*filter10 f0*/
    0xD4, 0x02,/*filter10 Q*/
    0x00, 0x00,/*filter10 Gain*/
};
//EQ style: Flat
const unsigned char EqMode_data_0[104] = {
    0x00, 0x00,/*pregain*/
	0x00, 0x00,/*calculation_type*/
    0x00, 0x00,/*filter1 enable*/
    0x01, 0x00,/*filter1 type*/
    0x0A, 0x01,/*filter1 f0*/
    0xB7, 0x00,/*filter1 Q*/
    0x28, 0x05,/*filter1 Gain*/
    0x00, 0x00,/*filter2 enable*/
    0x00, 0x00,/*filter2 type*/
    0x60, 0x00,/*filter2 f0*/
    0xFA, 0x01,/*filter2 Q*/
    0x0C, 0xFB,/*filter2 Gain*/
    0x00, 0x00,/*filter3 enable*/
    0x00, 0x00,/*filter3 type*/
    0x1F, 0x02,/*filter3 f0*/
    0x4D, 0x01,/*filter3 Q*/
    0x1A, 0x04,/*filter3 Gain*/
    0x00, 0x00,/*filter4 enable*/
    0x00, 0x00,/*filter4 type*/
    0xA3, 0x09,/*filter4 f0*/
    0x06, 0x03,/*filter4 Q*/
    0x17, 0x01,/*filter4 Gain*/
    0x00, 0x00,/*filter5 enable*/
    0x02, 0x00,/*filter5 type*/
    0xE3, 0x29,/*filter5 f0*/
    0xCD, 0x02,/*filter5 Q*/
    0x44, 0xFE,/*filter5 Gain*/
    0x00, 0x00,/*filter6 enable*/
    0x00, 0x00,/*filter6 type*/
    0xE8, 0x03,/*filter6 f0*/
    0xD4, 0x02,/*filter6 Q*/
    0x00, 0x00,/*filter6 Gain*/
    0x00, 0x00,/*filter7 enable*/
    0x00, 0x00,/*filter7 type*/
    0xE8, 0x03,/*filter7 f0*/
    0xD4, 0x02,/*filter7 Q*/
    0x00, 0x00,/*filter7 Gain*/
    0x00, 0x00,/*filter8 enable*/
    0x00, 0x00,/*filter8 type*/
    0xE8, 0x03,/*filter8 f0*/
    0xD4, 0x02,/*filter8 Q*/
    0x00, 0x00,/*filter8 Gain*/
    0x00, 0x00,/*filter9 enable*/
    0x00, 0x00,/*filter9 type*/
    0xE8, 0x03,/*filter9 f0*/
    0xD4, 0x02,/*filter9 Q*/
    0x00, 0x00,/*filter9 Gain*/
    0x00, 0x00,/*filter10 enable*/
    0x00, 0x00,/*filter10 type*/
    0xE8, 0x03,/*filter10 f0*/
    0xD4, 0x02,/*filter10 Q*/
    0x00, 0x00,/*filter10 Gain*/
};
//EQ style: Pop
const unsigned char EqMode_data_2[104] = {
    0x00, 0x00,/*pregain*/
	0x00, 0x00,/*calculation_type*/
    0x01, 0x00,/*filter1 enable*/
    0x01, 0x00,/*filter1 type*/
    0xCC, 0x00,/*filter1 f0*/
    0x26, 0x02,/*filter1 Q*/
    0xD7, 0xFD,/*filter1 Gain*/
    0x01, 0x00,/*filter2 enable*/
    0x00, 0x00,/*filter2 type*/
    0xCA, 0x02,/*filter2 f0*/
    0xE8, 0x01,/*filter2 Q*/
    0xF9, 0x03,/*filter2 Gain*/
    0x00, 0x00,/*filter3 enable*/
    0x00, 0x00,/*filter3 type*/
    0x1F, 0x02,/*filter3 f0*/
    0x4D, 0x01,/*filter3 Q*/
    0x1A, 0x04,/*filter3 Gain*/
    0x00, 0x00,/*filter4 enable*/
    0x00, 0x00,/*filter4 type*/
    0xA3, 0x09,/*filter4 f0*/
    0x06, 0x03,/*filter4 Q*/
    0x17, 0x01,/*filter4 Gain*/
    0x01, 0x00,/*filter5 enable*/
    0x02, 0x00,/*filter5 type*/
    0x33, 0x12,/*filter5 f0*/
    0x8C, 0x02,/*filter5 Q*/
    0x0E, 0xFE,/*filter5 Gain*/
    0x00, 0x00,/*filter6 enable*/
    0x00, 0x00,/*filter6 type*/
    0xE8, 0x03,/*filter6 f0*/
    0xD4, 0x02,/*filter6 Q*/
    0x00, 0x00,/*filter6 Gain*/
    0x00, 0x00,/*filter7 enable*/
    0x00, 0x00,/*filter7 type*/
    0xE8, 0x03,/*filter7 f0*/
    0xD4, 0x02,/*filter7 Q*/
    0x00, 0x00,/*filter7 Gain*/
    0x00, 0x00,/*filter8 enable*/
    0x00, 0x00,/*filter8 type*/
    0xE8, 0x03,/*filter8 f0*/
    0xD4, 0x02,/*filter8 Q*/
    0x00, 0x00,/*filter8 Gain*/
    0x00, 0x00,/*filter9 enable*/
    0x00, 0x00,/*filter9 type*/
    0xE8, 0x03,/*filter9 f0*/
    0xD4, 0x02,/*filter9 Q*/
    0x00, 0x00,/*filter9 Gain*/
    0x00, 0x00,/*filter10 enable*/
    0x00, 0x00,/*filter10 type*/
    0xE8, 0x03,/*filter10 f0*/
    0xD4, 0x02,/*filter10 Q*/
    0x00, 0x00,/*filter10 Gain*/
};
//EQ style: Rock
const unsigned char EqMode_data_3[104] = {
    0x00, 0x00,/*pregain*/
	0x00, 0x00,/*calculation_type*/
    0x01, 0x00,/*filter1 enable*/
    0x01, 0x00,/*filter1 type*/
    0xC4, 0x00,/*filter1 f0*/
    0x84, 0x02,/*filter1 Q*/
    0xD1, 0x04,/*filter1 Gain*/
    0x01, 0x00,/*filter2 enable*/
    0x00, 0x00,/*filter2 type*/
    0xC3, 0x03,/*filter2 f0*/
    0x33, 0x01,/*filter2 Q*/
    0xB9, 0xFD,/*filter2 Gain*/
    0x00, 0x00,/*filter3 enable*/
    0x00, 0x00,/*filter3 type*/
    0x1F, 0x02,/*filter3 f0*/
    0x4D, 0x01,/*filter3 Q*/
    0x1A, 0x04,/*filter3 Gain*/
    0x00, 0x00,/*filter4 enable*/
    0x00, 0x00,/*filter4 type*/
    0xA3, 0x09,/*filter4 f0*/
    0x06, 0x03,/*filter4 Q*/
    0x17, 0x01,/*filter4 Gain*/
    0x01, 0x00,/*filter5 enable*/
    0x02, 0x00,/*filter5 type*/
    0x8F, 0x0D,/*filter5 f0*/
    0x14, 0x02,/*filter5 Q*/
    0x40, 0x04,/*filter5 Gain*/
    0x00, 0x00,/*filter6 enable*/
    0x00, 0x00,/*filter6 type*/
    0xE8, 0x03,/*filter6 f0*/
    0xD4, 0x02,/*filter6 Q*/
    0x00, 0x00,/*filter6 Gain*/
    0x00, 0x00,/*filter7 enable*/
    0x00, 0x00,/*filter7 type*/
    0xE8, 0x03,/*filter7 f0*/
    0xD4, 0x02,/*filter7 Q*/
    0x00, 0x00,/*filter7 Gain*/
    0x00, 0x00,/*filter8 enable*/
    0x00, 0x00,/*filter8 type*/
    0xE8, 0x03,/*filter8 f0*/
    0xD4, 0x02,/*filter8 Q*/
    0x00, 0x00,/*filter8 Gain*/
    0x00, 0x00,/*filter9 enable*/
    0x00, 0x00,/*filter9 type*/
    0xE8, 0x03,/*filter9 f0*/
    0xD4, 0x02,/*filter9 Q*/
    0x00, 0x00,/*filter9 Gain*/
    0x00, 0x00,/*filter10 enable*/
    0x00, 0x00,/*filter10 type*/
    0xE8, 0x03,/*filter10 f0*/
    0xD4, 0x02,/*filter10 Q*/
    0x00, 0x00,/*filter10 Gain*/
};
//EQ style: Jazz
const unsigned char EqMode_data_4[104] = {
    0x00, 0x00,/*pregain*/
	0x00, 0x00,/*calculation_type*/
    0x01, 0x00,/*filter1 enable*/
    0x01, 0x00,/*filter1 type*/
    0x70, 0x00,/*filter1 f0*/
    0x85, 0x01,/*filter1 Q*/
    0x2C, 0x04,/*filter1 Gain*/
    0x00, 0x00,/*filter2 enable*/
    0x00, 0x00,/*filter2 type*/
    0xC3, 0x03,/*filter2 f0*/
    0x33, 0x01,/*filter2 Q*/
    0xB9, 0xFD,/*filter2 Gain*/
    0x01, 0x00,/*filter3 enable*/
    0x00, 0x00,/*filter3 type*/
    0x93, 0x02,/*filter3 f0*/
    0x6A, 0x03,/*filter3 Q*/
    0xE4, 0xFD,/*filter3 Gain*/
    0x00, 0x00,/*filter4 enable*/
    0x00, 0x00,/*filter4 type*/
    0xA3, 0x09,/*filter4 f0*/
    0x06, 0x03,/*filter4 Q*/
    0x17, 0x01,/*filter4 Gain*/
    0x01, 0x00,/*filter5 enable*/
    0x02, 0x00,/*filter5 type*/
    0xD8, 0x11,/*filter5 f0*/
    0x25, 0x02,/*filter5 Q*/
    0xC2, 0x02,/*filter5 Gain*/
    0x00, 0x00,/*filter6 enable*/
    0x00, 0x00,/*filter6 type*/
    0xE8, 0x03,/*filter6 f0*/
    0xD4, 0x02,/*filter6 Q*/
    0x00, 0x00,/*filter6 Gain*/
    0x00, 0x00,/*filter7 enable*/
    0x00, 0x00,/*filter7 type*/
    0xE8, 0x03,/*filter7 f0*/
    0xD4, 0x02,/*filter7 Q*/
    0x00, 0x00,/*filter7 Gain*/
    0x00, 0x00,/*filter8 enable*/
    0x00, 0x00,/*filter8 type*/
    0xE8, 0x03,/*filter8 f0*/
    0xD4, 0x02,/*filter8 Q*/
    0x00, 0x00,/*filter8 Gain*/
    0x00, 0x00,/*filter9 enable*/
    0x00, 0x00,/*filter9 type*/
    0xE8, 0x03,/*filter9 f0*/
    0xD4, 0x02,/*filter9 Q*/
    0x00, 0x00,/*filter9 Gain*/
    0x00, 0x00,/*filter10 enable*/
    0x00, 0x00,/*filter10 type*/
    0xE8, 0x03,/*filter10 f0*/
    0xD4, 0x02,/*filter10 Q*/
    0x00, 0x00,/*filter10 Gain*/
};

uint16_t SampleRateIndexGet(uint32_t SampleRate)
{
	volatile uint32_t i;
	for(i = 0; i < sizeof(SupportSampleRateList)/sizeof(SupportSampleRateList[0]); i++)
	{
		if(SampleRate == SupportSampleRateList[i])
		{
			break;
		}
	}
	if(i == 13)
	{
		i =0;
	}
	return i;
}

/**
 * parambin协议结构
 * 0x01 len 实际len
 * 0x03 len 实际len
 * 0x04 len 实际len
 * 0x10 len 实际len
 */
static void FindTLV(uint8_t *dataBuf, uint32_t dataSize)
{
	uint8_t *employbuf = dataBuf;
	uint32_t usedSize = 0;
	if(dataBuf == NULL)
	{
		APP_DBG("No hardware parameters\n");
		return;
	}
	while(usedSize < dataSize)
	{
		switch(employbuf[0])
		{
			case 0x03:
				memcpy(&gCtrlVars.HwCt.ADC0PGACt, employbuf, sizeof(gCtrlVars.HwCt.ADC0PGACt));
				break;
			case 0x04:
				memcpy(&gCtrlVars.HwCt.ADC0DigitalCt, employbuf, sizeof(gCtrlVars.HwCt.ADC0DigitalCt));
				break;
			case 0x05:
				memcpy(&gCtrlVars.HwCt.ADC0AGCCt, employbuf, sizeof(gCtrlVars.HwCt.ADC0AGCCt));
				break;
			case 0x06:
				memcpy(&gCtrlVars.HwCt.ADC1PGACt, employbuf, sizeof(gCtrlVars.HwCt.ADC1PGACt));
				break;
			case 0x07:
				memcpy(&gCtrlVars.HwCt.ADC1DigitalCt, employbuf, sizeof(gCtrlVars.HwCt.ADC1DigitalCt));
				break;
			case 0x08:
				memcpy(&gCtrlVars.HwCt.ADC1AGCCt, employbuf, sizeof(gCtrlVars.HwCt.ADC1AGCCt));
				break;
			case 0x09:
				memcpy(&gCtrlVars.HwCt.DAC0Ct, employbuf, sizeof(gCtrlVars.HwCt.DAC0Ct));
				break;
			case 0x0A:
				memcpy(&gCtrlVars.HwCt.DAC1Ct, employbuf, sizeof(gCtrlVars.HwCt.DAC1Ct));
				break;
			case 0x0B:
				memcpy(&gCtrlVars.HwCt.DAC2Ct, employbuf, sizeof(gCtrlVars.HwCt.DAC2Ct));
				break;
			case 0x0C:
				memcpy(&gCtrlVars.HwCt.I2S0Ct, employbuf, sizeof(gCtrlVars.HwCt.I2S0Ct));
				break;
			case 0x0D:
				memcpy(&gCtrlVars.HwCt.I2S1Ct, employbuf, sizeof(gCtrlVars.HwCt.I2S1Ct));
				break;
			case 0x0E:
				memcpy(&gCtrlVars.HwCt.I2S2Ct, employbuf, sizeof(gCtrlVars.HwCt.I2S2Ct));
				break;
			case 0x0F:
				memcpy(&gCtrlVars.HwCt.SPDIF0Ct, employbuf, sizeof(gCtrlVars.HwCt.SPDIF0Ct));
				break;
			case 0x10:
				memcpy(&gCtrlVars.HwCt.SPDIF1Ct, employbuf, sizeof(gCtrlVars.HwCt.SPDIF1Ct));
				break;
		}
		usedSize = usedSize + employbuf[1] + 2;
		employbuf = &dataBuf[usedSize];
	}
}


void AudioCodecVarDefault(void)
{
	//for ADC0 PGA      0x03
	memset(&gCtrlVars.HwCt.ADC0PGACt, 0, sizeof(ADC0PGAContext));
	gCtrlVars.HwCt.ADC0PGACt.MICL_Mode = 1;
	gCtrlVars.HwCt.ADC0PGACt.MICR_Mode = 1;
	gCtrlVars.HwCt.ADC0PGACt.MICL_Single_Gain = 16;
	gCtrlVars.HwCt.ADC0PGACt.MICR_Single_Gain = 16;
	gCtrlVars.HwCt.ADC0PGACt.MICL_DIFF_Gain = 16;
	gCtrlVars.HwCt.ADC0PGACt.MICR_DIFF_Gain = 16;
	gCtrlVars.HwCt.ADC0PGACt.LineinL_Gain = 22;
	gCtrlVars.HwCt.ADC0PGACt.LineinR_Gain = 22;

    //for ADC0 DIGITAL  0x04
	memset(&gCtrlVars.HwCt.ADC0DigitalCt, 0, sizeof(ADCDigitalContext));
	gCtrlVars.HwCt.ADC0DigitalCt.L_Volume = 0;
	gCtrlVars.HwCt.ADC0DigitalCt.R_Volume = 0;
	gCtrlVars.HwCt.ADC0DigitalCt.DC_Block_Enable = 1;

	//for AGC0 ADC0     0x05
	memset(&gCtrlVars.HwCt.ADC0AGCCt, 0, sizeof(ADCAGCContext));
	gCtrlVars.HwCt.ADC0AGCCt.Frame_Time = 10;
	gCtrlVars.HwCt.ADC0AGCCt.Attack_Time = 10;
	gCtrlVars.HwCt.ADC0AGCCt.Decay_Time = 10;

	//for ADC1 PGA      0x06
	memset(&gCtrlVars.HwCt.ADC1PGACt, 0, sizeof(ADC1PGAContext));
	gCtrlVars.HwCt.ADC1PGACt.AUX1L_B18_Gain = 22;
	gCtrlVars.HwCt.ADC1PGACt.AUX1R_B19_Gain = 22;
	gCtrlVars.HwCt.ADC1PGACt.AUX0L_B20_Gain = 17;
	gCtrlVars.HwCt.ADC1PGACt.AUX0R_B21_Gain = 17;
	gCtrlVars.HwCt.ADC1PGACt.MICL_Mode = 0;
	gCtrlVars.HwCt.ADC1PGACt.MICR_Mode = 0;
	gCtrlVars.HwCt.ADC1PGACt.MICL_Single_Gain = 16;
	gCtrlVars.HwCt.ADC1PGACt.MICR_Single_Gain = 16;
	gCtrlVars.HwCt.ADC1PGACt.MICL_DIFF_Gain = 16;
	gCtrlVars.HwCt.ADC1PGACt.MICR_DIFF_Gain = 16;

	//for ADC1 DIGITAL  0x07
	memset(&gCtrlVars.HwCt.ADC1DigitalCt, 0, sizeof(ADCDigitalContext));
	gCtrlVars.HwCt.ADC1DigitalCt.L_Volume = 0;
	gCtrlVars.HwCt.ADC1DigitalCt.R_Volume = 0;
	gCtrlVars.HwCt.ADC1DigitalCt.DC_Block_Enable = 1;

	//for AGC1  ADC1    0x08
	memset(&gCtrlVars.HwCt.ADC1AGCCt, 0, sizeof(ADCAGCContext));
	gCtrlVars.HwCt.ADC1AGCCt.Frame_Time = 10;
	gCtrlVars.HwCt.ADC1AGCCt.Attack_Time = 10;
	gCtrlVars.HwCt.ADC1AGCCt.Decay_Time = 10;

	//for DAC0          0x09
	memset(&gCtrlVars.HwCt.DAC0Ct, 0, sizeof(DAC0_1Context));
	gCtrlVars.HwCt.DAC0Ct.L_Volume = 0;
	gCtrlVars.HwCt.DAC0Ct.R_Volume = 0;
	gCtrlVars.HwCt.DAC0Ct.Scramble_En = 1;
	gCtrlVars.HwCt.DAC0Ct.Scramble_Mode = 2;

	//for DAC1          0x0a
	memset(&gCtrlVars.HwCt.DAC1Ct, 0, sizeof(DAC0_1Context));
	gCtrlVars.HwCt.DAC1Ct.L_Volume = 0;
	gCtrlVars.HwCt.DAC1Ct.R_Volume = 0;
	gCtrlVars.HwCt.DAC1Ct.Scramble_En = 1;
	gCtrlVars.HwCt.DAC1Ct.Scramble_Mode = 2;

	//for DAC2
	memset(&gCtrlVars.HwCt.DAC2Ct, 0, sizeof(DACxContext));
	gCtrlVars.HwCt.DAC2Ct.Volume = 0x1000;
	gCtrlVars.HwCt.DAC2Ct.Scramble_En = 1;
	gCtrlVars.HwCt.DAC2Ct.Scramble_Mode = 2;

	memset(&gCtrlVars.HwCt.I2S0Ct, 0, sizeof(I2S0Context));
	memset(&gCtrlVars.HwCt.I2S1Ct, 0, sizeof(I2S1_2Context));
	memset(&gCtrlVars.HwCt.I2S2Ct, 0, sizeof(I2S1_2Context));
	memset(&gCtrlVars.HwCt.SPDIF0Ct, 0, sizeof(SPDIFContext));
	memset(&gCtrlVars.HwCt.SPDIF1Ct, 0, sizeof(SPDIFContext));

    //system define
	gCtrlVars.sample_rate		= CFG_DEFAULT_SAMPLERATE;
	gCtrlVars.sample_rate_index = SampleRateIndexGet(gCtrlVars.sample_rate);
}

//系统变量初始化
void CtrlVarsInit(void)
{
	APP_DBG("[SYS]: Loading control vars as default\n");

	//音频系统硬件模块变量初始化
	DefaultParamgsInit();
}

//各个模块默认参数设置函数
void DefaultParamgsInit(void)   //在ModeCommonInit函数中会再次被调用
{
	memset(&gCtrlVars.HwCt, 0, sizeof(gCtrlVars.HwCt));
//	memcpy(&gCtrlVars.HwCt, AudioEffect_Parambin_GetCurHardwareConfig(), sizeof(gCtrlVars.HwCt));
#if defined(CFG_FUNC_Parambin_SET_CODEC_EN)
	if(AudioEffect_Parambin_GetCurHardwareConfig(&EffectParambin))
	{
		FindTLV((uint8_t*)EffectParambin.user_codec_parameters, EffectParambin.user_codec_parameters_len);
	}

    //system define
	gCtrlVars.sample_rate		= AudioCoreMixSampleRateGet(DefaultNet);
	gCtrlVars.sample_rate_index = SampleRateIndexGet(gCtrlVars.sample_rate);
	//dac 单端/差分配置不允许动态调整，不跟音效bin走
	#if defined(CHIP_DAC_USE_DIFF)
		gCtrlVars.HwCt.DAC0Ct.DAC_Mode = 1;
	#else
		gCtrlVars.HwCt.DAC0Ct.DAC_Mode = 0;
	#endif
	#if defined(CHIP_DAC1_USE_DIFF)
		gCtrlVars.HwCt.DAC1Ct.DAC_Mode = 1;
	#else
		gCtrlVars.HwCt.DAC1Ct.DAC_Mode = 0;
	#endif
	#if defined(CHIP_DAC2_USE_DIFF)
		gCtrlVars.HwCt.DAC2Ct.DAC_Mode = 1;
	#else
		gCtrlVars.HwCt.DAC2Ct.DAC_Mode = 0;
	#endif
	// DoutMode 默认为编译期宏配置，不可运行时动态调整
#if defined(CFG_RES_DAC_MONITOR_EN)
	gCtrlVars.HwCt.DAC0Ct.Out_Mode = DAC_DOUT_MODE_SEL;
#endif
#if defined(CFG_RES_AUDIO_DAC1_MIX_OUT_EN) && (DAC_OUTPUT_CHANNEL != MONITOR_DAC1)
	gCtrlVars.HwCt.DAC1Ct.Out_Mode = DAC1_DOUT_MODE_SEL;
#endif
	#if defined(CHIP_MIC0_USE_DIFF)
		gCtrlVars.HwCt.ADC0PGACt.MICL_Mode = 1;
		gCtrlVars.HwCt.ADC0PGACt.MICR_Mode = 1;
	#else
		gCtrlVars.HwCt.ADC0PGACt.MICL_Mode = 0;
		gCtrlVars.HwCt.ADC0PGACt.MICR_Mode = 0;
	#endif
	#if defined(CHIP_MIC1_USE_DIFF)
		gCtrlVars.HwCt.ADC1PGACt.MICL_Mode = 1;
		gCtrlVars.HwCt.ADC1PGACt.MICR_Mode = 1;
	#else
		gCtrlVars.HwCt.ADC1PGACt.MICL_Mode = 0;
		gCtrlVars.HwCt.ADC1PGACt.MICR_Mode = 0;
	#endif
#else
	AudioCodecVarDefault();
#endif
	//for system control 0x01
//	gCtrlVars.AutoRefresh = AutoRefresh_ALL_PARA;

	//scramble默认开启，设置成POS_NEG
	//gCtrlVars.HwCt.DAC0Ct.dac_scramble = POS_NEG + 1;
}

void AudioAnaChannelApply(int8_t ana_input_ch) //AudioLineSelSet
{
	//模拟通道先配置为NONE，防止上次配置通道残留，然后再配置需要的模拟通道 

    //AudioADC_AnaInit();
	//AudioADC_VcomConfig(1);//MicBias en
	//AudioADC_MicBias1Enable(1);
	//AudioADC_DynamicElementMatch(ADC0_MODULE, TRUE, TRUE);

	switch(ana_input_ch)
	{
		case ANA_INPUT_CH_LINEIN: //--------------------line -----------------------------------------//
			AudioADC_PGASel(ADC0_MODULE, ADC_LEFT,  ADC_NONE);
			AudioADC_PGASel(ADC0_MODULE, ADC_RIGHT, ADC_NONE);
			if(gCtrlVars.HwCt.ADC0PGACt.LineinL_EN)
			{
				//APP_DBG("LINE 1L En\n");
				AudioADC_PGASel(ADC0_MODULE, ADC_LEFT, ADC0_LineinL);
				AudioADC_PGAGainSet(ADC0_MODULE, ADC_LEFT,  ADC0_LineinL,  31 - gCtrlVars.HwCt.ADC0PGACt.LineinL_Gain);
			}
			if(gCtrlVars.HwCt.ADC0PGACt.LineinR_EN)
			{
				//APP_DBG("LINE 1R En\n");
				AudioADC_PGASel(ADC0_MODULE, ADC_RIGHT, ADC0_LineinR);
				AudioADC_PGAGainSet(ADC0_MODULE, ADC_RIGHT, ADC0_LineinR,  31 - gCtrlVars.HwCt.ADC0PGACt.LineinR_Gain);
			}
			break;

		case ANA_INPUT_CH_AUX0: //--------------------line0 -----------------------------------------//
			AudioADC_PGASel(ADC1_MODULE, ADC_LEFT,  ADC_NONE);
			AudioADC_PGASel(ADC1_MODULE, ADC_RIGHT, ADC_NONE);
			if(gCtrlVars.HwCt.ADC1PGACt.AUX0L_B20_EN)
			{
				//APP_DBG("LINE 2L En\n");
				AudioADC_PGASel(ADC1_MODULE, ADC_LEFT, ADC1_AUX0L_B20);
				AudioADC_PGAGainSet(ADC1_MODULE, ADC_LEFT,  ADC1_AUX0L_B20,  31 - gCtrlVars.HwCt.ADC1PGACt.AUX0L_B20_Gain);
			}
			if(gCtrlVars.HwCt.ADC1PGACt.AUX0R_B21_EN)
			{
				//APP_DBG("LINE 2R En\n");
				AudioADC_PGASel(ADC1_MODULE, ADC_RIGHT, ADC1_AUX0R_B21);
				AudioADC_PGAGainSet(ADC1_MODULE, ADC_RIGHT, ADC1_AUX0R_B21,  31 - gCtrlVars.HwCt.ADC1PGACt.AUX0R_B21_Gain);
			}
			break;

		case ANA_INPUT_CH_AUX1: //--------------------line1-----------------------------------------//
			AudioADC_PGASel(ADC1_MODULE, ADC_LEFT,  ADC_NONE);
			AudioADC_PGASel(ADC1_MODULE, ADC_RIGHT, ADC_NONE);
			if(gCtrlVars.HwCt.ADC1PGACt.AUX1L_B18_EN)
			{
				//APP_DBG("LINE 2L En\n");
				AudioADC_PGASel(ADC1_MODULE, ADC_LEFT, ADC1_AUX1L_B18);
				AudioADC_PGAGainSet(ADC1_MODULE, ADC_LEFT,  ADC1_AUX1L_B18,  31 - gCtrlVars.HwCt.ADC1PGACt.AUX1L_B18_Gain);
			}
			if(gCtrlVars.HwCt.ADC1PGACt.AUX1R_B19_EN)
			{
				//APP_DBG("LINE 2R En\n");
				AudioADC_PGASel(ADC1_MODULE, ADC_RIGHT, ADC1_AUX1R_B19);
				AudioADC_PGAGainSet(ADC1_MODULE, ADC_RIGHT, ADC1_AUX1R_B19,  31 - gCtrlVars.HwCt.ADC1PGACt.AUX1R_B19_Gain);
			}
			break;

		case ANA_INPUT_CH_MIC1:
			AudioADC_PGASel(ADC1_MODULE, ADC_LEFT,  ADC_NONE);
			AudioADC_PGASel(ADC1_MODULE, ADC_RIGHT, ADC_NONE);
			if(gCtrlVars.HwCt.ADC1PGACt.MICL_EN)
			{
				AudioADC_PGASel(ADC1_MODULE, ADC_LEFT, ADC1_MICL);
				AudioADC_PGAMode(ADC1_MODULE, ADC_LEFT, gCtrlVars.HwCt.ADC1PGACt.MICL_Mode);
				if(gCtrlVars.HwCt.ADC1PGACt.MICL_Mode == 0)
				{
					AudioADC_PGAGainSet(ADC1_MODULE, ADC_LEFT,  ADC1_MICL, 31 - gCtrlVars.HwCt.ADC1PGACt.MICL_Single_Gain);//0db bypass
				}
				else
				{
					AudioADC_PGAGainSet(ADC1_MODULE, ADC_LEFT,  ADC1_MICL, 31 - gCtrlVars.HwCt.ADC1PGACt.MICL_DIFF_Gain);//0db bypass
				}
			}
			if(gCtrlVars.HwCt.ADC1PGACt.MICR_EN)
			{
				AudioADC_PGASel(ADC1_MODULE, ADC_RIGHT, ADC1_MICR);
				AudioADC_PGAMode(ADC1_MODULE, ADC_RIGHT, gCtrlVars.HwCt.ADC1PGACt.MICR_Mode);
				if(gCtrlVars.HwCt.ADC1PGACt.MICR_Mode == 0)
				{
					AudioADC_PGAGainSet(ADC1_MODULE, ADC_RIGHT,  ADC1_MICR, 31 - gCtrlVars.HwCt.ADC1PGACt.MICR_Single_Gain);//0db bypass
				}
				else
				{
					AudioADC_PGAGainSet(ADC1_MODULE, ADC_RIGHT,  ADC1_MICR, 31 - gCtrlVars.HwCt.ADC1PGACt.MICR_DIFF_Gain);//0db bypass
				}
			}
			break;

		case ANA_INPUT_CH_MIC0:
			AudioADC_PGASel(ADC0_MODULE, ADC_LEFT,  ADC_NONE);
			AudioADC_PGASel(ADC0_MODULE, ADC_RIGHT, ADC_NONE);
			if(gCtrlVars.HwCt.ADC0PGACt.MICL_EN)
			{
				AudioADC_PGASel(ADC0_MODULE, ADC_LEFT, ADC0_MICL);
				if(gCtrlVars.HwCt.ADC0PGACt.MICL_Mode == 0)
				{
					AudioADC_PGAGainSet(ADC0_MODULE, ADC_LEFT,  ADC0_MICL, 31 - gCtrlVars.HwCt.ADC0PGACt.MICL_Single_Gain);//0db bypass
				}
				else
				{
					AudioADC_PGAGainSet(ADC0_MODULE, ADC_LEFT,  ADC0_MICL, 31 - gCtrlVars.HwCt.ADC0PGACt.MICL_DIFF_Gain);//0db bypass
				}
			}
			if(gCtrlVars.HwCt.ADC0PGACt.MICR_EN)
			{
				AudioADC_PGASel(ADC0_MODULE, ADC_RIGHT, ADC0_MICR);
				if(gCtrlVars.HwCt.ADC0PGACt.MICR_Mode == 0)
				{
					AudioADC_PGAGainSet(ADC0_MODULE, ADC_RIGHT,  ADC0_MICR, 31 - gCtrlVars.HwCt.ADC0PGACt.MICR_Single_Gain);//0db bypass
				}
				else
				{
					AudioADC_PGAGainSet(ADC0_MODULE, ADC_RIGHT,  ADC0_MICR, 31 - gCtrlVars.HwCt.ADC0PGACt.MICR_DIFF_Gain);//0db bypass
				}
			}
			break;
	}
}
#if 0 //Port 替换
void AudioLine3MicSelect(void)
{
	if(gCtrlVars.HwCt.ADC1PGACt.pga_mic_enable)
	{
		AudioADC_PGASel(ADC1_MODULE, CHANNEL_LEFT,MIC_LEFT);
		AudioADC_PGAGainSet(ADC1_MODULE, CHANNEL_LEFT,  MIC_LEFT, 31 - gCtrlVars.HwCt.ADC1PGACt.pga_mic_gain);//0db bypass
//		gCtrlVars.HwCt.ADC1PGACt.pga_mic_mode = Single;
	}
	else
	{
		AudioADC_PGASel(ADC1_MODULE, CHANNEL_LEFT,LINEIN_NONE);
	}
}
#endif
void AudioLineChannelSet(int8_t ana_input_ch)
{
#if defined(CFG_RES_MIC_SELECT) && (MIC_ADC_SEL == ADC0_MODULE)
	gCtrlVars.HwCt.ADC0PGACt.LineinL_EN = 0;
	gCtrlVars.HwCt.ADC0PGACt.LineinR_EN = 0;
	if(ANA_INPUT_CH_LINEIN == ana_input_ch)
	{
		gCtrlVars.HwCt.ADC0PGACt.LineinL_EN = 1;
		gCtrlVars.HwCt.ADC0PGACt.LineinR_EN = 1;
		AudioAnaChannelApply(ana_input_ch);
	}
#endif
#if (defined(CFG_RES_MIC_SELECT) && (MIC_ADC_SEL == ADC1_MODULE))
	gCtrlVars.HwCt.ADC1PGACt.AUX1L_B18_EN = 0;
	gCtrlVars.HwCt.ADC1PGACt.AUX1R_B19_EN = 0;
	gCtrlVars.HwCt.ADC1PGACt.AUX0L_B20_EN = 0;
	gCtrlVars.HwCt.ADC1PGACt.AUX0R_B21_EN = 0;
	if(ANA_INPUT_CH_AUX0 == ana_input_ch)
	{
		gCtrlVars.HwCt.ADC1PGACt.AUX0L_B20_EN = 1;
		gCtrlVars.HwCt.ADC1PGACt.AUX0R_B21_EN = 1;
		AudioAnaChannelApply(ana_input_ch);
	}
	if(ANA_INPUT_CH_AUX1 == ana_input_ch)
	{
		gCtrlVars.HwCt.ADC1PGACt.AUX1L_B18_EN = 1;
		gCtrlVars.HwCt.ADC1PGACt.AUX1R_B19_EN = 1;
		AudioAnaChannelApply(ana_input_ch);
	}
#endif
}

void AudioMicChannelSet(void)
{
#if defined(CFG_RES_MIC_SELECT) && (MIC_ADC_SEL == ADC0_MODULE)
	gCtrlVars.HwCt.ADC0PGACt.MICL_EN = (CFG_RES_MIC_SELECT & 1) ? 1:0;
	gCtrlVars.HwCt.ADC0PGACt.MICR_EN = (CFG_RES_MIC_SELECT & 2) ? 1:0;
	AudioAnaChannelApply(ANA_INPUT_CH_MIC0);
#endif

#if defined(CFG_RES_MIC_SELECT) && (MIC_ADC_SEL == ADC1_MODULE)
	gCtrlVars.HwCt.ADC1PGACt.MICL_EN = (CFG_RES_MIC_SELECT & 1) ? 1:0;
	gCtrlVars.HwCt.ADC1PGACt.MICR_EN = (CFG_RES_MIC_SELECT & 2) ? 1:0;
	AudioAnaChannelApply(ANA_INPUT_CH_MIC1);
#endif
}


// 将 gCtrlVars 中的音频硬件配置全量同步到寄存器
// 涵盖 ADC PGA/AGC/Digital Volume、DAC Volume/Scramble/Dither 等
void AudioCodecHardwareSync(void)
{
//	uint16_t dac_l, dac_r;
	AudioADC_PGAGainSet(ADC0_MODULE, ADC_LEFT,  ADC0_LineinL,  31 - gCtrlVars.HwCt.ADC0PGACt.LineinL_Gain);
	AudioADC_PGAGainSet(ADC0_MODULE, ADC_RIGHT, ADC0_LineinR,  31 - gCtrlVars.HwCt.ADC0PGACt.LineinR_Gain);
	if(gCtrlVars.HwCt.ADC0PGACt.MICL_Mode == 0)
	{
		AudioADC_PGAGainSet(ADC0_MODULE, ADC_LEFT,  ADC0_MICL, 31 - gCtrlVars.HwCt.ADC0PGACt.MICL_Single_Gain);//0db bypass
	}
	else
	{
		AudioADC_PGAGainSet(ADC0_MODULE, ADC_LEFT,  ADC0_MICL, 31 - gCtrlVars.HwCt.ADC0PGACt.MICL_DIFF_Gain);//0db bypass
	}

	if(gCtrlVars.HwCt.ADC0PGACt.MICR_Mode == 0)
	{
		AudioADC_PGAGainSet(ADC0_MODULE, ADC_RIGHT,  ADC0_MICR, 31 - gCtrlVars.HwCt.ADC0PGACt.MICR_Single_Gain);//0db bypass
	}
	else
	{
		AudioADC_PGAGainSet(ADC0_MODULE, ADC_RIGHT,  ADC0_MICR, 31 - gCtrlVars.HwCt.ADC0PGACt.MICR_DIFF_Gain);//0db bypass
	}

	//ADC1 PGA MIC
	if(gCtrlVars.HwCt.ADC1PGACt.MICL_Mode == 0)
	{
		AudioADC_PGAGainSet(ADC1_MODULE, ADC_LEFT,  ADC1_MICL, 31 - gCtrlVars.HwCt.ADC1PGACt.MICL_Single_Gain);
	}
	else
	{
		AudioADC_PGAGainSet(ADC1_MODULE, ADC_LEFT,  ADC1_MICL, 31 - gCtrlVars.HwCt.ADC1PGACt.MICL_DIFF_Gain);
	}

	if(gCtrlVars.HwCt.ADC1PGACt.MICR_Mode == 0)
	{
		AudioADC_PGAGainSet(ADC1_MODULE, ADC_RIGHT,  ADC1_MICR, 31 - gCtrlVars.HwCt.ADC1PGACt.MICR_Single_Gain);
	}
	else
	{
		AudioADC_PGAGainSet(ADC1_MODULE, ADC_RIGHT,  ADC1_MICR, 31 - gCtrlVars.HwCt.ADC1PGACt.MICR_DIFF_Gain);
	}

	//ADC1 PGA AUX line-in
	AudioADC_PGAGainSet(ADC1_MODULE, ADC_LEFT,  ADC1_AUX0L_B20, 31 - gCtrlVars.HwCt.ADC1PGACt.AUX0L_B20_Gain);
	AudioADC_PGAGainSet(ADC1_MODULE, ADC_RIGHT, ADC1_AUX0R_B21, 31 - gCtrlVars.HwCt.ADC1PGACt.AUX0R_B21_Gain);
	AudioADC_PGAGainSet(ADC1_MODULE, ADC_LEFT,  ADC1_AUX1L_B18, 31 - gCtrlVars.HwCt.ADC1PGACt.AUX1L_B18_Gain);
	AudioADC_PGAGainSet(ADC1_MODULE, ADC_RIGHT, ADC1_AUX1R_B19, 31 - gCtrlVars.HwCt.ADC1PGACt.AUX1R_B19_Gain);

#if 1
	//AGC0 param apply
	AudioADC_AGCChannelSel(ADC0_MODULE, (gCtrlVars.HwCt.ADC0AGCCt.AGC_EN & 1), (gCtrlVars.HwCt.ADC0AGCCt.AGC_EN >> 1) & 1);
	AudioADC_AGCMaxLevel(ADC0_MODULE, (uint8_t)gCtrlVars.HwCt.ADC0AGCCt.Max_Level);
	AudioADC_AGCTargetLevel(ADC0_MODULE, (uint8_t)gCtrlVars.HwCt.ADC0AGCCt.Target_Level);
	AudioADC_AGCMaxGain(ADC0_MODULE, (uint8_t)gCtrlVars.HwCt.ADC0AGCCt.Max_Gain);
	AudioADC_AGCMinGain(ADC0_MODULE, (uint8_t)gCtrlVars.HwCt.ADC0AGCCt.Min_Gain);
	AudioADC_AGCGainOffset(ADC0_MODULE, (uint8_t)gCtrlVars.HwCt.ADC0AGCCt.Gain_Offset);
	AudioADC_AGCFrameTime(ADC0_MODULE, gCtrlVars.HwCt.ADC0AGCCt.Frame_Time);
	AudioADC_AGCHoldFrames(ADC0_MODULE, (uint32_t)gCtrlVars.HwCt.ADC0AGCCt.Hold_N_Frame);
	AudioADC_AGCAttackStepTime(ADC0_MODULE, gCtrlVars.HwCt.ADC0AGCCt.Attack_Time);
	AudioADC_AGCDecayStepTime(ADC0_MODULE, gCtrlVars.HwCt.ADC0AGCCt.Decay_Time);
	AudioADC_AGCNoiseThreshold(ADC0_MODULE, (uint8_t)gCtrlVars.HwCt.ADC0AGCCt.NG_Threshold);
	AudioADC_AGCNoiseHoldFrames(ADC0_MODULE, (uint8_t)gCtrlVars.HwCt.ADC0AGCCt.NG_Hold_N_Frame);

	//AGC1 param apply
	AudioADC_AGCChannelSel(ADC1_MODULE, (gCtrlVars.HwCt.ADC1AGCCt.AGC_EN & 1), (gCtrlVars.HwCt.ADC1AGCCt.AGC_EN >> 1) & 1);
	AudioADC_AGCMaxLevel(ADC1_MODULE, (uint8_t)gCtrlVars.HwCt.ADC1AGCCt.Max_Level);
	AudioADC_AGCTargetLevel(ADC1_MODULE, (uint8_t)gCtrlVars.HwCt.ADC1AGCCt.Target_Level);
	AudioADC_AGCMaxGain(ADC1_MODULE, (uint8_t)gCtrlVars.HwCt.ADC1AGCCt.Max_Gain);
	AudioADC_AGCMinGain(ADC1_MODULE, (uint8_t)gCtrlVars.HwCt.ADC1AGCCt.Min_Gain);
	AudioADC_AGCGainOffset(ADC1_MODULE, (uint8_t)gCtrlVars.HwCt.ADC1AGCCt.Gain_Offset);
	AudioADC_AGCFrameTime(ADC1_MODULE, gCtrlVars.HwCt.ADC1AGCCt.Frame_Time);
	AudioADC_AGCHoldFrames(ADC1_MODULE, (uint32_t)gCtrlVars.HwCt.ADC1AGCCt.Hold_N_Frame);
	AudioADC_AGCAttackStepTime(ADC1_MODULE, gCtrlVars.HwCt.ADC1AGCCt.Attack_Time);
	AudioADC_AGCDecayStepTime(ADC1_MODULE, gCtrlVars.HwCt.ADC1AGCCt.Decay_Time);
	AudioADC_AGCNoiseThreshold(ADC1_MODULE, (uint8_t)gCtrlVars.HwCt.ADC1AGCCt.NG_Threshold);
	AudioADC_AGCNoiseHoldFrames(ADC1_MODULE, (uint8_t)gCtrlVars.HwCt.ADC1AGCCt.NG_Hold_N_Frame);
#endif

	//ADC0 digital volume
	AudioADC_VolSetChannel_For_dB(ADC0_MODULE, ADC_LEFT,  (int16_t)gCtrlVars.HwCt.ADC0DigitalCt.L_Volume);
	AudioADC_VolSetChannel_For_dB(ADC0_MODULE, ADC_RIGHT, (int16_t)gCtrlVars.HwCt.ADC0DigitalCt.R_Volume);

	//ADC1 digital volume
	AudioADC_VolSetChannel_For_dB(ADC1_MODULE, ADC_LEFT,  (int16_t)gCtrlVars.HwCt.ADC1DigitalCt.L_Volume);
	AudioADC_VolSetChannel_For_dB(ADC1_MODULE, ADC_RIGHT, (int16_t)gCtrlVars.HwCt.ADC1DigitalCt.R_Volume);

#ifdef CFG_RES_DAC_MONITOR_EN
	AudioDAC_VolGainSet_For_dB(DAC_SEL , (int16_t)DAC_CTRLVAR_SEL.L_Volume, (int16_t)DAC_CTRLVAR_SEL.R_Volume);

	if(DAC_CTRLVAR_SEL.Scramble_En == 0)
	{
		AudioDAC_ScrambleDisable(DAC_SEL);
	}
	else
	{
		AudioDAC_ScrambleEnable(DAC_SEL);
		AudioDAC_ScrambleModeSet(DAC_SEL,(SCRAMBLE_MODULE)DAC_CTRLVAR_SEL.Scramble_Mode);
	}
	if(DAC_CTRLVAR_SEL.Dither_En)
	{
		AudioDAC_DitherEnable(DAC_SEL);
	}
	else
	{
		AudioDAC_DitherDisable(DAC_SEL);
	}
#endif
}


#ifdef CFG_I2S_SLAVE_TO_SPDIFOUT_EN
__attribute__((section(".driver.isr"))) void Timer4Interrupt(void)
{
	gSyncModule.gSyncTimerCnt++;
	Timer_InterruptFlagClear(SYNC_TIMER_INDEX, UPDATE_INTERRUPT_SRC);
	SyncModule_Get();
}

void SyncModule_Init(void)
{
	GIE_ENABLE();
	NVIC_EnableIRQ(Timer4_IRQn);
	gSyncModule.gSyncTimerCnt = 0;
	gSyncModule.gRefreshFlag = 0;
	gSyncModule.gClearDoneFlag = 0;
	gSyncModule.dat = 0;
	gSyncModule.clkRatio = 4;
	Clock_SyncCtrl_Set(I2S_BCLK_SEL, 1);
	Clock_SyncCtrl_Set(MDAC_MCLK_SEL, 1);
	Timer_Config(SYNC_TIMER_INDEX, SYNC_TIMER_OUT_VALUE, 0);
	Timer_InterrputSrcEnable(SYNC_TIMER_INDEX, UPDATE_INTERRUPT_SRC);
	Timer_Start(SYNC_TIMER_INDEX);
	Clock_SyncCtrl_Set(START_EN, 1);
	Clock_SyncCtrl_Set(UPDATE_DONE_CLR, 1);
}

void SyncModule_Reset(void)
{
	uint32_t SampleRate = I2S_SampleRateGet(CFG_RES_I2S_MODULE);
	extern uint8_t mclkFreqNum;

	if(IsSelectMclkClk1(SampleRate))
	{
		gSyncModule.clkRatio = (AUDIO_PLL_CLK1_FREQ * mclkFreqNum / I2S_SampleRateGet(CFG_RES_I2S_MODULE)) / 64;
	}
	else
	{
		gSyncModule.clkRatio = (AUDIO_PLL_CLK2_FREQ * mclkFreqNum / I2S_SampleRateGet(CFG_RES_I2S_MODULE)) / 64;
	}
	DBG("BCLK_MCLK_RATIO = %d\n", gSyncModule.clkRatio);
	Clock_SyncCtrl_Set(I2S_BCLK_SEL, 0);
	Clock_SyncCtrl_Set(MDAC_MCLK_SEL, 0);
	Clock_SyncCtrl_Set(START_EN, 0);
	Clock_SyncCtrl_Set(UPDATE_DONE_CLR, 0);
	gSyncModule.gSyncTimerCnt = 0;
	gSyncModule.gRefreshFlag = 0;
	gSyncModule.gClearDoneFlag = 0;
	gSyncModule.dat = 0;
	Clock_SyncCtrl_Set(I2S_BCLK_SEL, 1);
	Clock_SyncCtrl_Set(MDAC_MCLK_SEL, 1);
	Clock_SyncCtrl_Set(START_EN, 1);
	Clock_SyncCtrl_Set(UPDATE_DONE_CLR, 1);
}

void SyncModule_Get(void)
{
	if(gSyncModule.gRefreshFlag)//上次处理还没有处理完，这次不再使用，准备使用下次
		return;

	//如果再次进来后，还是没清除，则这次结果不用
//	if(SREG_CLK_SYNC_CTRL.CLK_SYNC_CNT_UPDATE_DONE)
//	{
//		gClearDoneFlag = 1;
//		return;
//	}
	Clock_SyncCtrl_Set(UPDATE_EN, 1);
	WaitUs(3);
	gSyncModule.gI2sBclkCnt = Clock_ClkCnt_Get(I2S_BCLK_CNT1);
	gSyncModule.gI2sBclkCnt = (gSyncModule.gI2sBclkCnt << 32)  + Clock_ClkCnt_Get(I2S_BCLK_CNT0);
	gSyncModule.gDacMclkCnt = Clock_ClkCnt_Get(MDAC_MCLK_CNT1);
	gSyncModule.gDacMclkCnt = (gSyncModule.gDacMclkCnt << 32)  + Clock_ClkCnt_Get(MDAC_MCLK_CNT0);
	gSyncModule.gRefreshFlag = 1;
}

void SyncModule_Process(void)
{
	if(gSyncModule.gRefreshFlag || gSyncModule.gClearDoneFlag)
	{
		uint16_t defVal;

		if(gSyncModule.gRefreshFlag)
		{
			gSyncModule.gMClkFromBCLK = gSyncModule.gI2sBclkCnt  * 1000000 / SYNC_TIMER_OUT_VALUE * gSyncModule.clkRatio / gSyncModule.gSyncTimerCnt;
			gSyncModule.gMClkFromDPLL = gSyncModule.gDacMclkCnt  * 1000000 / SYNC_TIMER_OUT_VALUE / gSyncModule.gSyncTimerCnt;
//			DBG("gMClkFromBCLK = %f, gMClkFromDPLL = %f,  ", gSyncModule.gMClkFromBCLK*1.0, gSyncModule.gMClkFromDPLL*1.0);

			if(gSyncModule.gMClkFromBCLK > gSyncModule.gMClkFromDPLL)
			{
				defVal = gSyncModule.gMClkFromBCLK - gSyncModule.gMClkFromDPLL;
				if(defVal > 100)
				{
					gSyncModule.dat = 10;
					*(uint32_t *)0x40026008 += gSyncModule.dat;//defVal/10 * 5 + 1;
				}
				else
				{
					gSyncModule.dat = 5;
					if(defVal > 50)
					{
						if(defVal <= gSyncModule.defVal_bak)
						{
							gSyncModule.dat = 0;
						}
					}
					else if(defVal < 50)
					{
						if(defVal <= gSyncModule.defVal_bak)
						{
							gSyncModule.dat = 0;
						}
						else
						{
							gSyncModule.dat = 1;
						}
					}
					*(uint32_t *)0x40026008 += gSyncModule.dat;
				}

				gSyncModule.defVal_bak = defVal;
				//DBG("+%d, +%d, 0x%08x\n", defVal, defVal > 50?10:1, *(uint32_t *)0x40026008);
//				DBG("+%d, +%d, 0x%08x\n", defVal, gSyncModule.dat, *(uint32_t *)0x40026008);
			}
			else if(gSyncModule.gMClkFromBCLK < gSyncModule.gMClkFromDPLL)
			{
				defVal = gSyncModule.gMClkFromDPLL - gSyncModule.gMClkFromBCLK;
				gSyncModule.dat = 0;
				if(defVal > 100)
				{
					gSyncModule.dat = 10;
					*(uint32_t *)0x40026008 -= gSyncModule.dat;//defVal/10 * 5 + 1;
				}
				else
				{
					gSyncModule.dat = 5;
					if(defVal > 50)
					{
						if(defVal <= gSyncModule.defVal_bak)
						{
							gSyncModule.dat = 0;
						}
					}
					else if(defVal < 50)
					{
						if(defVal <= gSyncModule.defVal_bak)
						{
							gSyncModule.dat = 0;
						}
						else
						{
							gSyncModule.dat = 1;
						}
					}
					*(uint32_t *)0x40026008 -= gSyncModule.dat;
				}
				gSyncModule.defVal_bak = defVal;
				//DBG("-%d, -%d, 0x%08x\n", defVal, defVal > 50?10:1, *(uint32_t *)0x40026008);
//				DBG("-%d, -%d, 0x%08x\n", defVal, gSyncModule.dat, *(uint32_t *)0x40026008);
			}
		}

//		DBG("\n");
		gSyncModule.gRefreshFlag = 0;
		gSyncModule.gClearDoneFlag = 0;
	}
}
#endif
