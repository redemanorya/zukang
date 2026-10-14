#include "string.h"
#include "type.h"
#include "tempCorr.h"
#include "adc.h"
#include "chip_info.h"
#include "sadc_interface.h"
#include "debug.h"


#define   Temp_K	0.553
TMPCORR TmpCorrTrim;

extern uint32_t get_flash_capacity(void);
//DCDC T_SENSOR
void TemperatureCorrInit(void)
{
	uint32_t tmp_addr = get_flash_capacity() - 2048 + 16;
	uint32_t DCDC_trim_addr = get_flash_capacity() - 2048 + 4;
	uint8_t TMP[3];
	uint16_t DCDCTrim[3];

	memcpy(TMP, tmp_addr, 3);
	memcpy(&DCDCTrim[0], DCDC_trim_addr, 6);
	//memcpy(test, DCDC_trim_addr, 6);
	//DBG("tmp : %x, %x, %x\n", TMP[0],TMP[1], TMP[2]);
	//DBG("dcdcTrim : %x, %x, %x\n", DCDCTrim[0],DCDCTrim[1], DCDCTrim[2]);
	/* Fallback: erased flash (0xff/0xffff) = FT test not done, chip untested, abort */
	if(((TMP[0] == 0xff) && (TMP[1] == 0xff) && (TMP[2] == 0xff)) ||
	   ((DCDCTrim[0] == 0xffff) && (DCDCTrim[1] == 0xffff) && (DCDCTrim[2] == 0xffff)))
	{
		DBG("TemperatureCorrInit: trim erased (FT not done), return!\n");
		return;
	}
	if((TMP[0] == TMP[1]) &&  (TMP[1] == TMP[2]))
	{
		TmpCorrTrim.TMP = TMP[0];
	}
	else
	{
		TmpCorrTrim.TMP = 25;
	}
	if((DCDCTrim[0] == DCDCTrim[1]) &&  (DCDCTrim[1] == DCDCTrim[2]))
	{
		TmpCorrTrim.SarADCTrim = DCDCTrim[0];
	}
	else
	{
		TmpCorrTrim.SarADCTrim = 866;
	}

	Chip_TmpSensorEn();//打开模拟通道
}

//T=25+（Tadc-Tadc_25oC）*K
uint8_t TemperatureGet(void)
{
	uint16_t ADCDat;
	int16_t diff;
	int8_t Temp;

	if((Chip_Version() == CHIP_VERSION_ECO1) || (Chip_Version() == CHIP_VERSION_ECO0))
	{
		ADC_Enable();
		osTaskDelay(1);
		ADCDat = ADC_SingleDataGet(ADC_CHANNEL_T_SEN2);//zsq changes from ADC_CHANNEL_DAC_AVDD to ADC_CHANNEL_A16_MICBIAS
		ADC_SecureSite();
		//DelayUs(4); //需要等待至少4us才能再次采样
	}
	else
		ADCDat = ADC_SingleDataGet(ADC_CHANNEL_T_SEN2);
	//DBG("ADCDat : %x\n", ADCDat);

	diff = (int16_t)ADCDat - (int16_t)TmpCorrTrim.SarADCTrim;

	Temp = TmpCorrTrim.TMP  -  diff * (float)Temp_K;

	//DBG("Temp : %x\n", Temp);

	return Temp;
}
