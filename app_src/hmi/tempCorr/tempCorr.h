
#ifndef __TEMPCORR_H__
#define __TEMPCORR_H__

typedef struct _TMPCORR_
{
	int8_t TMP;
	uint16_t SarADCTrim;
}TMPCORR;

void TemperatureCorrInit(void);

uint8_t TemperatureGet(void);

#endif
