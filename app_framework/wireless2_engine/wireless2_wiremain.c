/**
 **************************************************************************************
 * @file    wireless2_wiremain.c
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

#include <string.h>
#include <stdlib.h>
#include "wireless2.h"
#include "irqn.h"
#include "debug.h"
#include "bt_app_init.h"
#include "audio_association.h"
#include "wireless2_wiremain.h"
#include "wireless2_audiomain.h"
#include "mvwireless2_config.h"

#ifdef FUNC_WIRELESS2_SUPPORT

#define CRC_PACKSUB 0

uint32_t CompanyWord = (COMPANY_BYTE3<<24) | (COMPANY_BYTE2<<16) | (WIRELESS_LINK_KEY1<<8) | (WIRELESS_LINK_KEY0);
uint32_t WirelessDeviceId = ((~COMPANY_BYTE3)&0xff) | (((~COMPANY_BYTE2)&0xff)<<8) | (((~WIRELESS_LINK_KEY1)&0xff)<<16) | (((~WIRELESS_LINK_KEY0)&0xff)<<24);
unsigned char audio_init_isready = 0;//射频初始化很快，中断会提前进来，确保音频准备好了。

Flash_param_t flash_dev_param;
#if defined(CFG_LOCK_PAIRED_TXRX)
	uint8_t PairInfoUpdate = 0;//0:None; 1:write; 2：Earse
#endif

uint8_t rfsend_buffer[RFAUDIO_TRANS_LEN*2+10];//tx:Max:6-1=rf_translen+23
Wireless2_param_t	wireless2_config = {
	.npack		 = RFPACK_NAUDIO,
	.rf_pbuffer  = rfsend_buffer,
	.rf_translen = RFAUDIO_TRANS_LEN,//RF_PACKET_LEN,
	.au_audiolen      = RFAUDIO_ENCODE_LEN,
	.sbc_len_backward = RFAUDIO_ENCODE_LEN*2,
};


void PairedDeviceInit(void);
uint8_t KeyRemote(uint8_t device, uint8_t* RecvData);

void ConfigBtBbParams(BtBbParams *params)
{
	if(params == NULL)
		return;

	memset(params, 0 ,sizeof(BtBbParams));

	params->freqTrim = BT_DEFAULT_TRIM;

	//em config
	params->em_start_addr = BB_EM_START_PARAMS;
	params->em_size = BB_EM_SIZE;
}

void WirelessDeviceIdInit(void)
{
	unsigned long long ChipID;

	Chip_IDGet(&ChipID);
	ChipID = (ChipID>>32)&0xffffffff;

	DBG("ChipID:0x%08llx\n", ChipID);
	if(ChipID != 0 && ChipID != 0xffffffff)
	{
		WirelessDeviceId = ChipID;
	}
	else
	{
		WirelessDeviceId &= 0xffffff;
		WirelessDeviceId++;

		DBG("ChipID random:0x%08lx\n", WirelessDeviceId);
	}
	//根据ID随机延时，防止多设备同时启动撞时序
	DelayMs(WirelessDeviceId&0xff);
}

void Wireless_CmdProcess(void)
{
	unsigned char getcmd1,getcmd2;

	if(KeyRemote(0,&getcmd1)!=0xff)
		APP_DBG("dev1 cmd[%d]\n",getcmd1);
	if(KeyRemote(1,&getcmd2)!=0xff)
		APP_DBG("dev2 cmd[%d]\n",getcmd2);
}

uint32_t	em_start_addr = BB_EM_START_PARAMS;
void WirelessInit(void)
{
	BtBbParams			bbParams;
	WirelessDeviceIdInit();
	PairedDeviceInit();

#ifdef WIRELESS2_EFFECTTESTMODE
	return;
#endif

	MVWIRE2_ParamInit(&wireless2_config);
	wireless_AudioParityCntStart_cb = wireless_AudioParityCntStart;
	WIRELESS_FUNCTION(0);
	MVWIRE2_DeviceRoleSet(WIRELESS_SDK_ROLE);

	WDG0_Feed();
	Wireless2_ModeEn(1);
	ConfigBtBbParams(&bbParams);

#ifdef BT_HIGH_RF_TXPOWER_ENABLE
	Bt_select_high_pa_mode(1);
#endif
	set_OOB_band(0);

	Wireless_common_init(&bbParams);
	NVIC_SetPriority(BT_IRQn,0);
	NVIC_SetPriority(USB_HS_IRQn,1);
	NVIC_SetPriority(USB_FS_WAKEUP_IRQn,1);
	NVIC_SetPriority(TMR2_IRQn,1);
	NVIC_SetPriority(IPC_HSEMINT0_IRQn,1);
	NVIC_SetPriority(PLMT_IRQn,1);
	NVIC_SetPriority(SWI_IRQn,1);
	NVIC_SetPriority(TMR4_DMAC1TINT_IRQn,1);
}

void MVWIRE2_ConnectedCB(unsigned char id,unsigned char role)
{
	if((audio_init_isready)&&(id==0)&&(device1.ConStatus != CONNECT_AUDIO))
	{
		device1.ConStatus = CONNECT_AUDIO;
		device1.handle = 0x80;
		device1.RecvNum = 0;
	}
	if((audio_init_isready)&&(id==1)&&(device2.ConStatus != CONNECT_AUDIO))
	{
		device2.ConStatus = CONNECT_AUDIO;
		device2.handle = 0x81;
		device2.RecvNum = 0;
	}
}

void MVWIRE2_DisconnectedCB(unsigned char id,unsigned char role)
{
	if((id==0)&&(device1.ConStatus != CONNECT_NONE))
	{
		device1.handle = HANDLE_NONE;
		device1.ConStatus = CONNECT_NONE;

		WirelessAudioDevice1RxSyncReset();
	}

	if((id==1)&&(device2.ConStatus != CONNECT_NONE))
	{
		device2.handle = HANDLE_NONE;
		device2.ConStatus = CONNECT_NONE;

		WirelessAudioDevice2RxSyncReset();
	}

	if((device1.ConStatus == CONNECT_NONE)&&(device2.ConStatus == CONNECT_NONE)){
		wireless_AudioParityCntReset();
	}
}


char MVWIRE2_GetDevice1ConnState(void)
{
	return device1.ConStatus != CONNECT_NONE;
}


char MVWIRE2_GetDevice2ConnState(void)
{
#if	(RFAUDIO_ENCODE_CH==1)
	return FALSE;
#endif
	return device2.ConStatus != CONNECT_NONE;
}

void MVWIRE2_ConnStateDisplay(void)
{
	static char change_f1=0;
#ifndef	WIRELESS_TX_DEVICE
	static char change_f2=0;
#endif

	if(change_f1!=MVWIRE2_GetDevice1ConnState())
	{
		if(MVWIRE2_GetDevice1ConnState())
		{
			printf("device1_conn\r\n");
		}
		else
		{
			printf("device1_disconn\r\n");
		}
		change_f1 = MVWIRE2_GetDevice1ConnState();
	}
#ifndef	WIRELESS_TX_DEVICE
	if(change_f2!=MVWIRE2_GetDevice2ConnState())
	{
		if(MVWIRE2_GetDevice2ConnState())
		{
			printf("device2_conn\r\n");
		}
		else
		{
			printf("device2_disconn\r\n");
		}
		change_f2 = MVWIRE2_GetDevice2ConnState();
	}
#endif
}

void WirelessInfo_Process(void)
{
	MVWIRE2_ConnStateDisplay();
	Wireless_CmdProcess();
#if defined(CFG_LOCK_PAIRED_TXRX)
	PairedInfoSetFlash();
#endif
}


void audio_PackBuild(unsigned char* audio_encodeframe, uint8_t Cnt, uint8_t Cmd)
{
	unsigned short sbc_crc16;
#if (RFAUDIO_ENCODE_CH==1)
	audio_encodeframe[RFAUDIO_ENCODE_LEN + 0] = Cnt;//40
	audio_encodeframe[RFAUDIO_ENCODE_LEN + 1] = Cmd;
	sbc_crc16=wireless_CRC16_2(&audio_encodeframe[0], RFAUDIO_ENCODE_LEN + PACKET_CNT_LEN);
	audio_encodeframe[RFAUDIO_ENCODE_LEN + PACKET_CNT_LEN + 0] = sbc_crc16 & 0xff;
	audio_encodeframe[RFAUDIO_ENCODE_LEN + PACKET_CNT_LEN + 1] = sbc_crc16>>8;
#elif (RFAUDIO_ENCODE_CH==2)
	audio_encodeframe[RFAUDIO_ENCODE_LEN*2 + 0] = Cnt;//40
	audio_encodeframe[RFAUDIO_ENCODE_LEN*2 + 1] = Cmd;
	sbc_crc16=wireless_CRC16_2(&audio_encodeframe[0], RFAUDIO_ENCODE_LEN*2 + PACKET_CNT_LEN);
	audio_encodeframe[RFAUDIO_ENCODE_LEN*2 + PACKET_CNT_LEN + 0] = sbc_crc16 & 0xff;
	audio_encodeframe[RFAUDIO_ENCODE_LEN*2 + PACKET_CNT_LEN + 1] = sbc_crc16>>8;
#endif
}

unsigned char PairedDeviceFlagGet(void)
{
	unsigned char DeviceMask = 0;

	if(flash_dev_param.PairedDev1Info != NOPAIR_WORD)
		DeviceMask |= DEVICE1_MASK;
#if	( defined(WIRELESS_TURNKEY4_1)&&(!defined(WIRELESS_TX_DEVICE)) )
	if(flash_dev_param.PairedDev2Info != NOPAIR_WORD)
		DeviceMask |= DEVICE2_MASK;
#endif
	 return DeviceMask;
}

uint32_t PairInfoGet(unsigned char Device)
{
	if(Device & DEVICE1_MASK && PairedDeviceFlagGet() & DEVICE1_MASK)
	{
		return flash_dev_param.PairedDev1Info;
	}
#if	( defined(WIRELESS_TURNKEY4_1)&&(!defined(WIRELESS_TX_DEVICE)) )
	else if(Device & DEVICE2_MASK)
	{
		return flash_dev_param.PairedDev2Info;
	}
#endif
	else
	{
		return NOPAIR_WORD;
	}
}

bool PairedInfoSetRematch(uint32_t Info, unsigned char Device)
{
	if(Device & DEVICE1_MASK)
	{
		if(flash_dev_param.PairedDev1Info != Info)
		{
			flash_dev_param.PairedDev1Info = Info;
		}
	}
	if(Device & DEVICE2_MASK)
	{
		if(flash_dev_param.PairedDev2Info != Info)
		{
			flash_dev_param.PairedDev2Info = Info;
		}
	}
#if (defined(CFG_LOCK_PAIRED_TXRX) || defined(CFG_LOCK_PAIRED_RX_EN)) && !defined(CFG_LOCK_PAIRED_RX_SAVE_NVM)
	SpiFlashErase(SECTOR_ERASE, CONINF_FLASH_ADDR/4096, 1);
	SpiFlashWrite(CONINF_FLASH_ADDR, (uint8_t*)&flash_dev_param, sizeof(flash_dev_param),1);
#endif
	return TRUE;
}

bool PairedInfoSet(uint32_t Info, unsigned char Device)
{
	bool NeedErase = FALSE;
#if defined(CFG_LOCK_PAIRED_TXRX)
	if(Device & DEVICE1_MASK)
	{
		if(flash_dev_param.PairedDev1Info != Info)
		{
			if(flash_dev_param.PairedDev1Info != NOPAIR_WORD)
			{
				NeedErase = TRUE;
#if defined(CFG_LOCK_PAIRED_RX_EN) && !defined(CFG_LOCK_PAIRED_RX_SAVE_NVM)
				if(flash_dev_param.PairedDev2Info == Info)
				{
					flash_dev_param.PairedDev2Info = flash_dev_param.PairedDev1Info;
					DBG("LR  change\n");
				}
#endif
			}
			flash_dev_param.PairedDev1Info = Info;
		}
	}
#endif
#if	( defined(WIRELESS_TURNKEY4_1)&&(!defined(WIRELESS_TX_DEVICE)) )
	if(Device & DEVICE2_MASK)
	{
		if(flash_dev_param.PairedDev2Info != Info)
		{
			if(flash_dev_param.PairedDev2Info != NOPAIR_WORD)
			{
				NeedErase = TRUE;
			}
			flash_dev_param.PairedDev2Info = Info;
		}
	}
#endif

#if defined(CFG_LOCK_PAIRED_TXRX) || defined(CFG_LOCK_PAIRED_RX_EN)
	if(NeedErase)
	{
		PairInfoUpdate = 2;
	}
	else
		PairInfoUpdate = 1;

#ifdef CFG_LOCK_PAIRED_RX_SAVE_NVM
	return FALSE;
#else
	return TRUE;
#endif
#else
	return FALSE;
#endif
}


void PairedDeviceInit(void)
{
	PairedFlagGetFunc = PairedDeviceFlagGet;
	PairInfoGetFunc = PairInfoGet;
	PairedInfoSetFunc = PairedInfoSet;

#if defined(CFG_LOCK_PAIRED_TXRX) || defined(CFG_LOCK_PAIRED_RX_EN)
	SpiFlashRead(CONINF_FLASH_ADDR,(uint8_t*)&flash_dev_param,sizeof(flash_dev_param),100);
#else
	flash_dev_param.PairedDev1Info = NOPAIR_WORD;
	#if	( defined(WIRELESS_TURNKEY4_1)&&(!defined(WIRELESS_TX_DEVICE)) )
	flash_dev_param.PairedDev2Info = NOPAIR_WORD;
	#endif
#endif
	if(PairedDeviceFlagGet() & DEVICE1_MASK)
	{
		printf("Paired 0x%08x\n", flash_dev_param.PairedDev1Info);

	}
	else
	{
		printf("Null device1 paired!\n");
	}
#if	( defined(WIRELESS_TURNKEY4_1)&&(!defined(WIRELESS_TX_DEVICE)) )
	if(PairedDeviceFlagGet() & DEVICE2_MASK)
	{
		printf("Paired device2 0x%08x\n", flash_dev_param.PairedDev2Info);
	}
	else
	{
		printf("Null device2 paired!\n");
	}
#endif
}

#if defined(CFG_LOCK_PAIRED_TXRX)
void PairedInfoSetFlash(void)
{
	if(PairInfoUpdate == 2)
	{
		PairInfoUpdate--;
	#ifndef CFG_LOCK_PAIRED_RX_SAVE_NVM
		SpiFlashErase(SECTOR_ERASE, CONINF_FLASH_ADDR/4096, 1);
	#endif
	}
	else if(PairInfoUpdate == 1)
	{
	#ifndef CFG_LOCK_PAIRED_RX_SAVE_NVM
		SpiFlashWrite(CONINF_FLASH_ADDR, (uint8_t*)&flash_dev_param, sizeof(flash_dev_param),1);
	#endif
		PairInfoUpdate--;
	}
}
#endif//CFG_LOCK_PAIRED_TXRX

#else
uint32_t CompanyWord ;
uint32_t WirelessDeviceId ;
unsigned char audio_init_isready ;

void MVWIRE2_ConnectedCB(unsigned char id,unsigned char role)
{
}

void MVWIRE2_DisconnectedCB(unsigned char id,unsigned char role)
{
}
char MVWIRE2_GetDevice1ConnState(void)
{
	return FALSE;
}


#endif
