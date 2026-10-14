
#ifndef _WIRELESS2BB_API_H_
#define _WIRELESS2BB_API_H_

#include "type.h"

typedef unsigned char (*BBSniffNotifyCallback)(void);

typedef struct _WirelessBbParams
{
	uint8_t		*localDevName;
	uint8_t		localDevAddr[6];
	uint8_t		freqTrim;
	uint32_t	em_start_addr;

	//bb agc config
	uint8_t		pAgcDisable;
	uint8_t		pAgcLevel;

	//sniff config
	uint8_t		pSniffNego;
	uint16_t	pSniffDelay;
	uint16_t	pSniffInterval;
	uint16_t	pSniffAttempt;
	uint16_t	pSniffTimeout;

	BBSniffNotifyCallback	bbSniffNotify;
}WirelessBbParams;

void Wireless_common_init(void* params);
void Wireless2_ModeEn(uint8_t set);
uint8_t Wireless2_ModeEnGet(void);


#endif //_WIRELESS2BB_API_H_
