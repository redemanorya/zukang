/**
 **************************************************************************************
 * @file    lc3p_audiocodec_api.c
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
#include "debug.h"
#include "app_config.h"
#include "mvwireless2_config.h"


#ifdef FUNC_WIRELESS2_SUPPORT
#include "lc3plus.h"

int32_t pcm_audio_input_L[ONE_FRAME];
int32_t pcm_audio_input_L_null[1];
uint8_t decoder_L[12676];
uint8_t scratch_L[20234];
#if defined(AUDIO_24BIT)
int32_t *input16_L[] = {(int32_t *)pcm_audio_input_L, (int32_t *)pcm_audio_input_L_null};
#else
int16_t *input16_L[] = {(int16_t *)pcm_audio_input_L, (int16_t *)pcm_audio_input_L_null};
#endif
#if	(DECODER_CH == 2)
int32_t pcm_audio_input_R[ONE_FRAME];
int32_t pcm_audio_input_R_null[1];
uint8_t decoder_R[12676];
uint8_t scratch_R[20234];
#if defined(AUDIO_24BIT)
int32_t *input16_R[] = {(int32_t *)pcm_audio_input_R, (int32_t *)pcm_audio_input_R_null};
#else
int16_t *input16_R[] = {(int16_t *)pcm_audio_input_R, (int16_t *)pcm_audio_input_R_null};
#endif
#endif
//-----------------
#if (ONE_FRAME==120)
uint8_t encoder1[3550];
uint8_t scratch_en1[2350];
#elif (ONE_FRAME==240)
uint8_t encoder1[3550];
uint8_t scratch_en1[3500];
#endif

int32_t encoder1_pcm_audio_input[ONE_FRAME];
int16_t Enc1_Buf[ONE_FRAME];
int16_t Enc1_Buf_null[1];

#ifdef	AUDIO_24BIT
int32_t *input16_encoder1[] = {(int32_t *)encoder1_pcm_audio_input, (int32_t *)Enc1_Buf_null};
#else
int16_t *input16_encoder1[] = {(int16_t *)Enc1_Buf, (int16_t *)Enc1_Buf_null};
#endif

#if (ONE_FRAME==120)
uint8_t encoder2[3550];
uint8_t scratch_en2[2350];
#elif (ONE_FRAME==240)
uint8_t encoder2[3550];
uint8_t scratch_en2[3500];
#endif

int32_t encoder2_pcm_audio_input[ONE_FRAME];
int16_t Enc2_Buf[ONE_FRAME];
int16_t Enc2_Buf_null[1];

#ifdef	AUDIO_24BIT
int32_t *input16_encoder2[] = {(int32_t *)encoder2_pcm_audio_input, (int32_t *)Enc2_Buf_null};
#else
int16_t *input16_encoder2[] = {(int16_t *)Enc2_Buf, (int16_t *)Enc2_Buf_null};
#endif


void wireless_decoder_init(uint8_t decoder_id,uint8_t ch)
{
	LC3PLUS_Error err;
	uint8_t ep = LC3PLUS_EP_OFF;
	int decoder_size = 0, decoder_scratch_size = 0;
	int delay = 0, nSamples = 0;

	if(decoder_id==1)
	{
		decoder_size = lc3plus_dec_get_size(SAMPLE_RATE, 1, LC3PLUS_PLC_ADVANCED);
		err = lc3plus_dec_init((LC3PLUS_Dec*)decoder_L, SAMPLE_RATE, 1, (LC3PLUS_PlcMode)LC3PLUS_PLC_ADVANCED);
		err = lc3plus_dec_set_frame_dms((LC3PLUS_Dec*)decoder_L, (int) (ONE_FRAME*10000/SAMPLE_RATE));
		err = lc3plus_dec_set_ep_enabled((LC3PLUS_Dec*)decoder_L, ep);
		delay    = lc3plus_dec_get_delay((LC3PLUS_Dec*)decoder_L);
		nSamples = lc3plus_dec_get_output_samples((LC3PLUS_Dec*)decoder_L);
//		printf("dec: dms: [%d]/10ms\n", (ONE_FRAME*10000/SAMPLE_RATE));
//		printf("dec: delay: %d, nSamples: %d\n", delay, nSamples);
		decoder_scratch_size = lc3plus_dec_get_scratch_size((LC3PLUS_Dec*)decoder_L);

		if((decoder_size > sizeof(decoder_L))||(decoder_scratch_size > sizeof(scratch_L)))
		{
			printf("lc3P_L dec mem err!!!\n");
			printf("err_L[%d]\n",err);
			while(1);
		}
	}
#if	(DECODER_CH == 2)
	if(decoder_id==2)
	{
		decoder_size = lc3plus_dec_get_size(SAMPLE_RATE, 1, LC3PLUS_PLC_ADVANCED);
		err = lc3plus_dec_init((LC3PLUS_Dec*)decoder_R, SAMPLE_RATE, 1, (LC3PLUS_PlcMode)LC3PLUS_PLC_ADVANCED);
		err = lc3plus_dec_set_frame_dms((LC3PLUS_Dec*)decoder_R, (int) (ONE_FRAME*10000/SAMPLE_RATE));
		err = lc3plus_dec_set_ep_enabled((LC3PLUS_Dec*)decoder_R, ep);
		delay    = lc3plus_dec_get_delay((LC3PLUS_Dec*)decoder_R);
		nSamples = lc3plus_dec_get_output_samples((LC3PLUS_Dec*)decoder_R);
//		printf("dec: dms: [%d]/10ms\n", (ONE_FRAME*10000/SAMPLE_RATE));
//		printf("dec: delay: %d, nSamples: %d\n", delay, nSamples);
		decoder_scratch_size = lc3plus_dec_get_scratch_size((LC3PLUS_Dec*)decoder_R);

		if((decoder_size > sizeof(decoder_R))||(decoder_scratch_size > sizeof(scratch_R)))
		{
			printf("lc3P_R dec mem err!!!\n");
			printf("err_L[%d]\n",err);
			while(1);
		}
	}
#endif
}

__attribute__((section(".tcm_section")))
int32_t wireless_decoder_apply(uint8_t decoder_id,uint8_t *decoder_buf,uint8_t decoder_size,int16_t *pcm_buf)
{
	int32_t status = -1;

	if(decoder_id==1)
	{
#if defined(AUDIO_24BIT)
		status = lc3plus_dec24((LC3PLUS_Dec*)decoder_L, decoder_buf, decoder_size, input16_L, scratch_L, 0);
		memcpy(pcm_buf,pcm_audio_input_L,ONE_FRAME*4);
#else
		status = lc3plus_dec16((LC3PLUS_Dec*)decoder_L, decoder_buf, decoder_size, input16_L, scratch_L, 0);
		memcpy(pcm_buf,pcm_audio_input_L,ONE_FRAME*2);
#endif

		if((status != 0) && (decoder_size!=0))
		{
			DBG("Deocder Err_L!%d\n", (int)status);
		}
	}
#if	(DECODER_CH == 2)
	if(decoder_id==2)
	{
#if defined(AUDIO_24BIT)
		status = lc3plus_dec24((LC3PLUS_Dec*)decoder_R, decoder_buf, decoder_size, input16_R, scratch_R, 0);
		memcpy(pcm_buf,pcm_audio_input_R,ONE_FRAME*4);
#else
		status = lc3plus_dec16((LC3PLUS_Dec*)decoder_R, decoder_buf, decoder_size, input16_R, scratch_R, 0);
		memcpy(pcm_buf,pcm_audio_input_R,ONE_FRAME*2);
#endif
		if((status != 0) && (decoder_size!=0))
		{
			DBG("Deocder Err_R!%d\n", (int)status);
		}
	}
#endif
    return status;
}

//--------------------------------------------------------

void wireless_encoder_init(uint8_t decoder_id)
{

	LC3PLUS_Error err = -1;
	uint8_t ep = LC3PLUS_EP_OFF;
	int encoder_size = 0, encoder_scratch_size = 0;
	int nBytes_en = 0;

	if(decoder_id==1)
	{
		encoder_size = lc3plus_enc_get_size(SAMPLE_RATE,1);

		err = lc3plus_enc_init((LC3PLUS_Enc*)encoder1, SAMPLE_RATE, 1);
		err = lc3plus_enc_set_frame_dms((LC3PLUS_Enc*)encoder1,(ONE_FRAME*10000/SAMPLE_RATE));
		err = lc3plus_enc_set_ep_mode((LC3PLUS_Enc*)encoder1, ep);
		err = lc3plus_enc_set_lfe((LC3PLUS_Enc*)encoder1,0);
		err = lc3plus_enc_set_bitrate((LC3PLUS_Enc*)encoder1,AUDIO_FRAME_BITRATE);
		encoder_scratch_size = lc3plus_enc_get_scratch_size((LC3PLUS_Enc*)encoder1);

	//	printf("lc3P dms[%d]/10dms\n",(ONE_FRAME*10000/SAMPLE_RATE));
		if((encoder_size > sizeof(encoder1))||(encoder_scratch_size > sizeof(scratch_en1)))
		{
			printf("encoder_size[%d].encoder_scratch_size[%d]\n",encoder_size,encoder_scratch_size);
			printf("lc3P enc mem err[%d]!!!\n",err);
			while(1);
		}

		lc3plus_enc_get_delay((LC3PLUS_Enc*)encoder1);
		lc3plus_enc_get_input_samples((LC3PLUS_Enc*)encoder1);
		lc3plus_enc_get_real_bitrate((LC3PLUS_Enc*)encoder1);
		nBytes_en = lc3plus_enc_get_num_bytes((LC3PLUS_Enc*)encoder1);

		if(nBytes_en!=RFAUDIO_ENCODE_LEN)
		{
			printf("encoder1 len err :%d RFAUDIO_ENCODE_LEN (%d)\n", nBytes_en,RFAUDIO_ENCODE_LEN);
			while(1);
		}
	}

	err = -1;
	ep = LC3PLUS_EP_OFF;
	encoder_size = 0;
	encoder_scratch_size = 0;
	nBytes_en = 0;

	if(decoder_id==2)
	{
		encoder_size = lc3plus_enc_get_size(SAMPLE_RATE,1);

		err = lc3plus_enc_init((LC3PLUS_Enc*)encoder2, SAMPLE_RATE, 1);
		err = lc3plus_enc_set_frame_dms((LC3PLUS_Enc*)encoder2,(ONE_FRAME*10000/SAMPLE_RATE));
		err = lc3plus_enc_set_ep_mode((LC3PLUS_Enc*)encoder2, ep);
		err = lc3plus_enc_set_lfe((LC3PLUS_Enc*)encoder2,0);
		err = lc3plus_enc_set_bitrate((LC3PLUS_Enc*)encoder2,AUDIO_FRAME_BITRATE);
		encoder_scratch_size = lc3plus_enc_get_scratch_size((LC3PLUS_Enc*)encoder2);

	//	printf("lc3P dms[%d]/10dms\n",(ONE_FRAME*10000/SAMPLE_RATE));
		if((encoder_size > sizeof(encoder2))||(encoder_scratch_size > sizeof(scratch_en2)))
		{
			printf("encoder2_size[%d].encoder_scratch_size2[%d]\n",encoder_size,encoder_scratch_size);
			printf("lc3P enc2 mem err[%d]!!!\n",err);
			while(1);
		}

		lc3plus_enc_get_delay((LC3PLUS_Enc*)encoder2);
		lc3plus_enc_get_input_samples((LC3PLUS_Enc*)encoder2);
		lc3plus_enc_get_real_bitrate((LC3PLUS_Enc*)encoder2);
		nBytes_en = lc3plus_enc_get_num_bytes((LC3PLUS_Enc*)encoder2);

		if(nBytes_en!=RFAUDIO_ENCODE_LEN)
		{
			printf("encoder2 len err :%d RFAUDIO_ENCODE_LEN (%d)\n", nBytes_en,nBytes_en);
			while(1);
		}
	}
	printf("EncoderCoreInit Done!!!\n");
//	printf("enc get num bytes :%d \n", nBytes_en);
}

int32_t wireless_encoder_apply(uint8_t encoder_id,uint8_t *encoder_buf,uint32_t* encoder_size,void *pcm_buf)
{
	int32_t status = -1;

	if(encoder_id==1)
	{
#ifdef 	AUDIO_24BIT
		for (int k=0; k<ONE_FRAME; k++)
		{
			int32_t* get_24bit = (int32_t*)pcm_buf;
			encoder1_pcm_audio_input[k] = ((int32_t)get_24bit[k + 0]);
		}
		status = lc3plus_enc24((LC3PLUS_Enc*)encoder1, input16_encoder1, encoder_buf, (int*)encoder_size, scratch_en1);
#else
		status = lc3plus_enc16((LC3PLUS_Enc*)encoder1, input16_encoder1, encoder_buf, (int*)encoder_size, scratch_en1);
#endif
		if((status != 0) && (encoder_size!=0))
		{
			DBG("enocder Err_L!%d\n", (int)status);
		}
	}
	if(encoder_id==2)
	{
#ifdef 	AUDIO_24BIT
		for (int k=0; k<ONE_FRAME; k++)
		{
			int32_t* get_24bit = (int32_t*)pcm_buf;
			encoder2_pcm_audio_input[k] = ((int32_t)get_24bit[k + 0]);
		}
		status = lc3plus_enc24((LC3PLUS_Enc*)encoder2, input16_encoder2, encoder_buf, (int*)encoder_size, scratch_en2);
#else
		status = lc3plus_enc16((LC3PLUS_Enc*)encoder2, input16_encoder2, encoder_buf, (int*)encoder_size, scratch_en2);
#endif
		if((status != 0) && (encoder_size!=0))
		{
			DBG("enocder Err_R!%d\n", (int)status);
		}
	}

    return status;
}

#endif


