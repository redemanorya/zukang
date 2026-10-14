/**
 **************************************************************************************
 * @file   wireless2_audiomain.h
 * @brief
 *
 * @author  Tony
 * @version V1.0.0
 *
 * $Created: 2026-04-07
 *
 * @Copyright (C) 2026, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#ifndef __WIRELESS2_AUDIOMAIN_H__
#define __WIRELESS2_AUDIOMAIN_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


void Wireless_TransBufInit(void);
uint16_t Wireless_TransDataLen(void);
uint16_t Wireless_TransSpaceLen(void);

void wireless_AudioADC0Get(void* Buf, uint16_t Len);
void wireless_AudioDACSet(void* Buf, uint16_t Len);

uint8_t wireless_AudioParityCntStart(void);
void Wireless_TransBufWrite(unsigned char* data,unsigned int len);
unsigned int Wireless_TransBufRead(unsigned char* data,unsigned int len);
void Wireless_AudioAssociationInit(void);
void MVwire2_Device1ConnectStateProcess(unsigned int cnt,uint8_t* data);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif //__WIRELESS2_AUDIOMAIN_H__

