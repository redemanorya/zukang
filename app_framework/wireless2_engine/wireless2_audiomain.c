/**
 **************************************************************************************
 * @file    wireless2_audiomain.c
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
#include "app_config.h"
#include "wireless2_audiomain.h"
#include "irqn.h"
#include "type.h"
#include "mcu_circular_buf.h"
#include "mvwireless2_config.h"
#include "audio_association.h"

#ifdef FUNC_WIRELESS2_SUPPORT

/****************Audio association init**************/
unsigned char    dec_buf_f[RFAUDIO_ENCODE_LEN * 2+6];
short	         MicLeftPcmFifo_f[RFAUDIO_ENCODE_LEN * 2 * 20];
short            MicRightPcmFifo_f[RFAUDIO_ENCODE_LEN * 2 * 20];

#define	RFPACK_REQUEST					1
static unsigned char audio_conuter = 0;
static unsigned int  audio_ignorecnt = 0;

//mic fifo delay Control
static unsigned char audio_paritycnt = 0xff;
static unsigned char audio_RFstartcnt = 0;//rf首包不稳多判断几次

int32_t ADC_DeleteSample = 0;//adc rf同步使用;预留
int32_t DAC_DeleteSample = 0;//adc rf同步使用;预留

Rx_AudioAssociation_param_t wirelessaudio_config={
	.FrameGroups       = STEREO_TWO_DEVICE,
	.wireless_frames   = WIRELESS_RECV_FIFO_THRHLD,
	.AddHeaderLen	   = 0,
	.pack_info_len     = PACKET_CNT_LEN,
	.frame_len		   = ONE_FRAME,
	.Encoder_out_len   = RFAUDIO_ENCODE_LEN,			//sizeof(SilenceFrame),
	.dec_buf_p         = dec_buf_f,
	.MicLeftPcmFifo_p  = MicLeftPcmFifo_f,
	.MicRightPcmFifo_p = MicRightPcmFifo_f,
	.LogEnable = TRUE,
//	.Wireless2_2T1RDelaySyncResetFunc_t = Audio_Check1stFrameAllRightCounterResetForAudioAssociation,
};

void Wireless_AudioAssociationInit(void)
{
	wirelessaudio_config.MicLeftPcmFifo_len_set  = sizeof(MicLeftPcmFifo_f);
	wirelessaudio_config.MicRightPcmFifo_len_set = sizeof(MicRightPcmFifo_f);

	AudioAssociationInit(&wirelessaudio_config);
}

/****************Audio to Wireless Declare**************/
#define TRANS_LEN		(RFAUDIO_TRANS_LEN * 10+1)
MCU_CIRCULAR_CONTEXT 	WirelessTransCircularBuf_t;
unsigned char WirelessTransCircularBuf[TRANS_LEN];

void Wireless_TransBufInit(void)
{
	GIE_DISABLE();
	MCUCircular_Config(&WirelessTransCircularBuf_t, WirelessTransCircularBuf, sizeof(WirelessTransCircularBuf));
	GIE_ENABLE();
}

uint16_t Wireless_TransSpaceLen(void)
{
	return MCUCircular_GetSpaceLen(&WirelessTransCircularBuf_t);
}

uint16_t Wireless_TransDataLen(void)
{
	return MCUCircular_GetDataLen(&WirelessTransCircularBuf_t);
}

//write data in manloop, need DISABLE Interrupt
void Wireless_TransBufWrite(unsigned char* data,unsigned int len)
{
	GIE_DISABLE();
	MCUCircular_PutData(&WirelessTransCircularBuf_t, data,len);
	GIE_ENABLE();
}

__attribute__((section(".tcm_section")))
unsigned int Wireless_TransBufRead(unsigned char* data,unsigned int len)
{
	return MCUCircular_GetData(&WirelessTransCircularBuf_t, data,len);
}

/****************Get decoder pcm data**************/
#define PCMSAVELEN		(ONE_FRAME * 8 * 4 + 1)//2 frames
MCU_CIRCULAR_CONTEXT 	Wireless2DcoderPcmCircularBuf_t;
unsigned char Wireless2DcoderPcmCircularBuf[PCMSAVELEN];

void Wireless_DcoderPcmBufInit(void)
{
	MCUCircular_Config(&Wireless2DcoderPcmCircularBuf_t, Wireless2DcoderPcmCircularBuf, sizeof(Wireless2DcoderPcmCircularBuf));
}

uint16_t Wireless_DcoderPcmSpaceLen(void)
{
	return MCUCircular_GetSpaceLen(&Wireless2DcoderPcmCircularBuf_t);
}

uint16_t Wireless_DcoderPcmDataLen(void)
{
	return MCUCircular_GetDataLen(&Wireless2DcoderPcmCircularBuf_t);
}

//write data in manloop, need DISABLE Interrupt
void Wireless_DcoderPcmBufWrite(unsigned char* data,unsigned int len)
{
	MCUCircular_PutData(&Wireless2DcoderPcmCircularBuf_t, data,len);
}

unsigned int Wireless_DcoderPcmBufRead(unsigned char* data,unsigned int len)
{
	return MCUCircular_GetData(&Wireless2DcoderPcmCircularBuf_t, data,len);
}

/****************************
 * Wireless send Packet request in Interrupt
 * trigger No.1
 * Monitor audio Packet stability
 *****************************/
__attribute__((section(".tcm_section")))
bool Wireless_TransPacketCheck(void)
{
#if (RFAUDIO_ENCODE_CH==1)
	unsigned int Translen = RFAUDIO_TRANS_LEN;
#elif (RFAUDIO_ENCODE_CH==2)
	unsigned int Translen = RFAUDIO_BACKTRANS_LEN;
#endif

	if(audio_ignorecnt!=0)
	{
		Wireless_TransBufInit();
		audio_ignorecnt--;
		return 0;
	}

	if((audio_conuter==0))
	{
		unsigned int WirelessTxThrhld = Translen;

		if(MCUCircular_GetDataLen(&WirelessTransCircularBuf_t) >= WirelessTxThrhld)
		{
			audio_conuter = 1;
			return 1;
		}
	}
	else if(audio_conuter==0xff)//双设备同步才进入
	{
		if(MCUCircular_GetDataLen(&WirelessTransCircularBuf_t) >= Translen)
		{
			audio_conuter = RFPACK_REQUEST;
		}
	}
	else if(audio_conuter!=0)
	{
		audio_conuter++;
		if(audio_conuter > RFPACK_REQUEST)
		{
			audio_conuter = 1;
			if(MCUCircular_GetDataLen(&WirelessTransCircularBuf_t) >= Translen)
			{
				return 1;
			}
			//Audio帧<->WirelessSlot失锁--前者慢
			//本地时钟异常，忽略一批音频音频,对端200包重新同步，本地更稳定。
			audio_ignorecnt=300;
			Wireless_TransBufInit();
		}
	}
	return 0;
}

//audio sync to rf master
void wireless_AudioParityCntReset()
{
	audio_paritycnt = 0xff;
	audio_RFstartcnt = 0;
}

unsigned char wireless_AudioParityCntGet()
{
	return audio_paritycnt;
}

//return 0 no sync  1 sync done
uint8_t wireless_AudioParityCntStart()
{
	extern unsigned char audio_init_isready;
	int32_t del_buf[ONE_FRAME];

	if(audio_RFstartcnt >= 5)
	{
		if((wireless_AudioParityCntGet() == 0xff))
		{
			//此函数为中断，230~260太接近ADC取数据了，随机扣除10sample，下一帧在同步
			if((AudioADC0_DataLenGet()>(ONE_FRAME-15))&&(AudioADC0_DataLenGet()<(ONE_FRAME+15)))
			{
				AudioADC0_DataGet(del_buf,10);
				return 0;
			}
			DMA_ChannelDisable(PERIPHERAL_1_ID_AUDIO_ADC0_RX);
			DMA_CircularFIFOClear(PERIPHERAL_1_ID_AUDIO_ADC0_RX);
			DMA_ChannelEnable(PERIPHERAL_1_ID_AUDIO_ADC0_RX);
			ADC_DeleteSample = 20;
			audio_paritycnt =  0;
			audio_conuter = 0;

			DAC_DeleteSample = DAC_LENGET_GET() % ONE_FRAME;
			DAC_DATASET_SEL(del_buf,ONE_FRAME-DAC_DeleteSample);

			Wireless_TransBufInit();
			Wireless_DcoderPcmBufInit();
		}
	}
	else if(audio_init_isready==1)
		audio_RFstartcnt++;

	if(wireless_AudioParityCntGet()!=0xff)
		return 1;

	return 0;
}

void wireless_AudioADC0Get(void* Buf, uint16_t Len)
{
	AudioADC0_DataGet(Buf,Len-ADC_DeleteSample);
	if(ADC_DeleteSample)
	{
		ADC_DeleteSample = 0;
	}
}

int32_t Buffer[ONE_FRAME*2];
void wireless_AudioDACSet(void* Buf, uint16_t Len)
{
#ifdef	WIRELESS2_EFFECTTESTMODE
	return;
#endif
#ifdef	WIRELESSAUDIOSYNCDAC
	memset(Buffer,0,ONE_FRAME*2*4);
	if(Wireless_DcoderPcmDataLen()>=(Len*8))
	{
		unsigned int s = 0;
		int32_t* Buffer_p = Buf;
		Wireless_DcoderPcmBufRead(Buffer,Len*8);
		for(s = 0; s < ONE_FRAME; s++)
		{
			//走框图
			Buffer_p[2 * s + 0] += Buffer[2 * s + 0];
			Buffer_p[2 * s + 1] += Buffer[2 * s + 1];
			//只有wireless过来
//			Buffer_p[2 * s + 0] = Buffer[2 * s + 0];
//			Buffer_p[2 * s + 1] = Buffer[2 * s + 1];
		}
	}
#endif

	if(DAC_DeleteSample)
	{
		if((DAC_LENGET_GET())<=ONE_FRAME)
		{
			DAC_DeleteSample = 0;
		}
		else
			return;
	}
	DAC_DATASET_SEL(Buf,Len);
}

__attribute__((section(".tcm_section")))
bool Wireless_TransPacketIsReady(uint32_t StepNum)
{
	if(wireless_AudioParityCntGet()==0xff)
		return 0;
	if(Wireless_TransPacketCheck()==0)
		return 0;
#if (RFAUDIO_ENCODE_CH==1)
	if(MCUCircular_GetDataLen(&WirelessTransCircularBuf_t) >= RFAUDIO_TRANS_LEN)
	{
		return 1;
	}
#elif (RFAUDIO_ENCODE_CH==2)
	if(MCUCircular_GetDataLen(&WirelessTransCircularBuf_t) >= (RFAUDIO_BACKTRANS_LEN))
	{
		return 1;
	}
#endif

	Wireless_TransBufInit();
	return 0;
}

#endif
