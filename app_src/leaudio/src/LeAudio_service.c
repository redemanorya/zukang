/***************************************************************************
 *
 * File:
 *     $Id: tws.c 1411 2019-09-17 14:10:58Z  $
 *
 * Description:
 *     
 *
 ****************************************************************************/
//driver、lib
#include <nds32_intrinsic.h>
#include <stdlib.h>
#include "gpio.h"
#include "dma.h"
#include "dac_interface.h"
#include "irqn.h"
#include "i2s_interface.h"

//application
#include "app_config.h"
#include "le_audio_config.h"
#include "bt_config.h"
#include "mcu_circular_buf.h"
#include "le_audio_config.h"
#include "audio_core_api.h"
#include "rtos_api.h"
#include "app_message.h"
#include "bt_play_api.h"
#include "mode_task.h"

#if BT_LE_AUDIO_SUPPORT

//配置BIS DAC同步使用的定时器
#define BIS_SYNC_TIMER_ID   	TIMER8
#define BIS_SYNC_TIMER_IRQ  	TMR8_DMAC2TINT_IRQn
#define BIS_SYNC_TIMER_INT_FUNC Timer8_DMA2TINT_Interrupt

#if BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT
	uint8_t cis_src2sink_streaming = 0;//ase0
	uint8_t cis_sink2src_streaming = 0;//ase1
#endif

#ifdef LE_AUDIO_ENC_LC3_2
	#define LEAUDIO_SERVICE_SIZE			1024+256//4*1024 //1024
#else
	#define LEAUDIO_SERVICE_SIZE			1024 //1024
#endif

#define LEAUDIO_SERVICE_PRIO			5
#define LEAUDIO_SERVICE_TIMEOUT			1		/** 1 ms */
#define LEAUDIO_NUM_MESSAGE_QUEUE		20
MessageHandle LeAudioServiceMsgHandle = NULL;

uint16_t lc3_packet_len;

extern bool check_bis_src_is_streamed(void);

extern void LeAudioSrcDataApiRegister(void *set_data,void *get_len);
extern void LeAudioSinkDataApiRegister(void *get_data,void *get_len,void *get_id);
extern void LeAudioLc3BufLR(uint8_t *fifo);

extern void bis_src_send_data(void* buf, uint32_t len);
extern uint16_t bis_src_get_len(void);
extern uint16_t le_bis_sink_data_get(void* buf,uint32_t len);
extern uint32_t le_bis_sink_data_len_get(void);
extern uint8_t le_bis_sink_data_get_ID(void);
extern void cis_src_send_data(void* buf, uint32_t len);
extern uint16_t cis_src_get_len(void);
extern uint16_t le_cis_sink_data_get(void* buf,uint32_t len);
extern uint32_t le_cis_sink_data_len_get(void);

extern uint16_t le_cis_src_out_data_get(void* buf, uint32_t len);
extern uint32_t le_cis_src_out_data_len_get(void);
extern void cis_sink_in_send_data(void* buf, uint32_t len);
extern uint16_t cis_sink_in_get_len(void);
extern uint8_t bis_src_is_mv(void);
extern uint8_t get_if_use_pto(void);



#define LC3_STEREO_DecoderHandle_Len		(25*1024)
#define LC3_STEREO_EncoderHandle_Len		(13*1024 + 512)

#define LC3_2_STEREO_EncoderHandle_Len		((5*1024+512)*LeAudioFifo.channel)

#define LC3_MONO_DecoderHandle_Len			(12*1024 + 512)
#define LC3_MONO_EncoderHandle_Len			(10*1024 + 512)

#define SBC_DecoderHandle_Len				(5*1024)
#define SBC_EncoderHandle_Len				(5*1024)

struct
{
	uint8_t *decoder_fifo;
	uint8_t *decoder_handle;//coder_handle_fifo
	uint8_t *dec_pcm_fifo;//pcm_fifo;

	uint8_t *encoder_handle;
	uint8_t *enc_pcm_fifo;//pcm_fifo;

	uint8_t	CodecFormat;
	uint8_t	channel;
	uint32_t lc3_bitrate;
	uint16_t lc3_frame_len;//双向时必须同参数
	uint16_t SampleRate;
	uint16_t SrcSampleRate;

	uint8_t *buf_16lr;

	uint32_t pres_delay_us;
	uint8_t  pres_delay_flag;

	uint8_t  init_flag;
}LeAudioFifo;


void LeAudioServiceMsgSend(uint16_t msgId)
{
	MessageContext msgSend;
	msgSend.msgId = msgId;

	MessageSend(LeAudioServiceMsgHandle, &msgSend);
}

void LeAudioCodecFormatSet(uint8_t	CodecFormat,uint8_t	channel)
{
	if(CodecFormat != LE_CODEC_FORMAT_NONE)
	{
		if(LeAudioFifo.CodecFormat != CodecFormat)
		{
			LeAudioFifo.CodecFormat = CodecFormat;
			set_decoder_reinit(1); //解码器需要重新初始化
		}
	}

	if(channel == 1 || channel == 2)
	{
		if(LeAudioFifo.channel != channel)
		{
			LeAudioFifo.channel = channel;
			set_decoder_reinit(1); //解码器需要重新初始化
		}
	}
}

uint8_t LeAudioCodecFormatGet(void)
{
	return LeAudioFifo.CodecFormat;
}

void LeAudioLc3DecoderConfig(uint8_t	channel,
							 uint32_t 	lc3_bitrate,
							 uint16_t 	lc3_frame_len,
							 uint16_t 	SampleRate)
{
	if(LeAudioFifo.channel != channel)
	{
		LeAudioFifo.channel = channel;
		set_decoder_reinit(1); //解码器需要重新初始化
	}

	if(LeAudioFifo.lc3_bitrate != lc3_bitrate)
	{
		LeAudioFifo.lc3_bitrate = lc3_bitrate;
		set_decoder_reinit(1); //解码器需要重新初始化
	}

	if(LeAudioFifo.lc3_frame_len != lc3_frame_len)
	{
		LeAudioFifo.lc3_frame_len = lc3_frame_len;
		set_decoder_reinit(1); //解码器需要重新初始化
	}

	if(LeAudioFifo.SampleRate != SampleRate)
	{
		LeAudioFifo.SampleRate = SampleRate;
		set_decoder_reinit(1); //解码器需要重新初始化
	}
	set_decoder_cfg_flag(1);
}

uint16_t LeAudioGetLc3FrameLen(void)
{
	if(get_sink_wed_vaild())
		return LeAudioFifo.lc3_frame_len/2;
	else
		return LeAudioFifo.lc3_frame_len;
}

uint16_t LeAudioGetSrcSampleRate(void)
{
	return LeAudioFifo.SrcSampleRate;
}

void LeAudioSetSrcSampleRate(uint16_t sample_rate)
{
	if (get_src_wed_vaild())
		LeAudioFifo.SrcSampleRate = 48000;	
	else
		LeAudioFifo.SrcSampleRate = sample_rate;
	AudioCoreSinkChange(LE_AUDIO_SINK_NUM,2,LeAudioFifo.SrcSampleRate);
}

void LeAudioSetSrcFramesSdu(uint8_t frames_sdu)
{
	if(get_src_wed_vaild())
	{
    	LeAudioFifo.lc3_frame_len = Get_LC3_FRAME_LEN((LC3_FRAME_US/1000),LC3_BITRATE_DUALBIS);
		LeAudioFifo.lc3_bitrate = LC3_BITRATE_DUALBIS;
		LeAudioFifo.channel = LC3_STEREO;
	}
	else
	{
    	LeAudioFifo.lc3_frame_len = Get_LC3_FRAME_LEN((LC3_FRAME_US/1000),LC3_BITRATE);
		LeAudioFifo.lc3_bitrate = LC3_BITRATE;
		LeAudioFifo.channel = LC3_CHANNEL;
	}
	lc3_packet_len = LeAudioFifo.lc3_frame_len * frames_sdu;
}

extern uint8_t *bis2_src_sink_buffer;
void bis2_sink_src_buffer_free(void)
{
	if(bis2_src_sink_buffer)
		osPortFree(bis2_src_sink_buffer);
	bis2_src_sink_buffer = NULL;
	bis_sink1_data_clr();
}

void bis2_sink_src_buffer_malloc(void)
{
	if(bis2_src_sink_buffer == NULL)
		bis2_src_sink_buffer = osPortMalloc(BIS_SINK_SRC_RXTX_BUF_LEN);
	bis_sink1_data_clr();
}

void  LeAudioSerDecoderInit(void)
{
	if(get_decoder_reinit())
	{
		uint32_t fifo_len1,fifo_len2,fifo_len3;

		LeAudioFifo.init_flag = 0;
#ifdef CIS_APP_HEADSET_LR
		// LeAudioFifo.channel = LC3_STEREO;
		// LeAudioFifo.lc3_bitrate = LC3_BITRATE * 2;
		// LeAudioFifo.lc3_frame_len = LC3_FRAME_LEN * 2;
		// lc3_packet_len  = LC3_FRAME_LEN* 2;
		LeAudioFifo.channel = LC3_STEREO;
		LeAudioFifo.lc3_bitrate = LeAudioFifo.lc3_bitrate * 2;
		LeAudioFifo.lc3_frame_len = LeAudioFifo.lc3_frame_len * 2;
		lc3_packet_len  = lc3_packet_len* 2;
#endif
		APP_DBG("\nLeAudioSerDecoderInit: %lu %lu %lu-%d %d\n", (unsigned long)LeAudioFifo.channel, (unsigned long)LeAudioFifo.lc3_bitrate, (unsigned long)LeAudioFifo.lc3_frame_len,lc3_packet_len,get_sink_wed_vaild());
		if(LeAudioFifo.decoder_handle)
			osPortFree(LeAudioFifo.decoder_handle);
		if(LeAudioFifo.decoder_fifo)
			osPortFree(LeAudioFifo.decoder_fifo);
		if(LeAudioFifo.dec_pcm_fifo)
			osPortFree(LeAudioFifo.dec_pcm_fifo);
		if(LeAudioFifo.buf_16lr)
			osPortFree(LeAudioFifo.buf_16lr);

		LeAudioSetDecode1ch(0);

		if(LeAudioFifo.CodecFormat == LE_AUDIO_LC3_TYPE)
		{
			if(get_sink_wed_vaild())
			{
				LeAudioSetDecode1ch(1);
				if(get_decoder_cfg_flag())
				{
					LeAudioFifo.channel = LC3_STEREO;
					LeAudioFifo.lc3_bitrate *= 2;
					LeAudioFifo.lc3_frame_len *= 2;
				}
				set_decoder_cfg_flag(0);
				LeAudioTypeSet(LE_CODEC_FORMAT_LC3, lc3_packet_len*2, LeAudioFifo.lc3_frame_len);
			}
			else
				LeAudioTypeSet(LE_CODEC_FORMAT_LC3, lc3_packet_len, LeAudioFifo.lc3_frame_len);

			fifo_len1 = LC3_NUM_FRAMES*4*4*2;

			Lc3EncodeFormatSet(LeAudioFifo.channel, LC3_FRAME_MS, LeAudioFifo.lc3_bitrate);

			if(LeAudioFifo.channel == LC3_STEREO)
				fifo_len2 = LC3_STEREO_DecoderHandle_Len;
			else
				fifo_len2 = LC3_MONO_DecoderHandle_Len;

			fifo_len3 = 20*1024;
		}
		else
		{
			LeAudioTypeSet(LE_CODEC_FORMAT_SBC,AUDIO_SBC_PACKET_LEN,AUDIO_SBC_FRAME_LEN);
			fifo_len1 = 10*1024;
			SbcEncodeFormatSet(SBC_BLOCK,SBC_SUBBAND,SBC_BITPOOL);

			fifo_len2 = SBC_DecoderHandle_Len;
			fifo_len3 = 2048+512;
		}
		APP_DBG("LeAudioSerDecoder malloc : %lu,%lu,%lu\n",(unsigned long)fifo_len1,(unsigned long)fifo_len2,(unsigned long)fifo_len3);

		LeAudioFifo.dec_pcm_fifo = osPortMalloc(fifo_len1);
		LeAudioFifo.decoder_handle = osPortMalloc(fifo_len2);
		LeAudioFifo.decoder_fifo = osPortMalloc(fifo_len3);

		LeAudioFifo.buf_16lr  = osPortMalloc(LC3_NUM_FRAMES*3*sizeof(int16_t));
		LeAudioLc3BufLR(LeAudioFifo.buf_16lr);

		if(LeAudioFifo.decoder_handle == NULL
		   || LeAudioFifo.decoder_fifo == NULL
		   || LeAudioFifo.dec_pcm_fifo == NULL)
		{
			APP_DBG("LeAudioSerDecoder malloc error! %p %p %p\n",LeAudioFifo.decoder_handle,LeAudioFifo.decoder_fifo,LeAudioFifo.dec_pcm_fifo);
		}
		LeAudioDecPcmBufInit(LeAudioFifo.dec_pcm_fifo, fifo_len1);
	}
#ifdef LE_CIS_CONVERSATION
	LeAudio_Decoder_init(LeAudioFifo.decoder_handle, LeAudioFifo.decoder_fifo,LC3_SAMPLERATE);
#else
	LeAudio_Decoder_init(LeAudioFifo.decoder_handle, LeAudioFifo.decoder_fifo,LeAudioFifo.SampleRate);
	AudioCoreSourceChange(APP_SOURCE_NUM, 2, LeAudioFifo.SampleRate);
#endif
	set_decoder_reinit(0);
#if 0
	LeAudioServiceMsgSend(MSG_TASK_START);
#endif
	LeAudioFifo.init_flag = 1;
}

void  LeAudioSerDecoderDeinit(void)
{
	LeAudioFifo.init_flag = 0;
	if(LeAudioFifo.decoder_handle)
		osPortFree(LeAudioFifo.decoder_handle);
	if(LeAudioFifo.decoder_fifo)
		osPortFree(LeAudioFifo.decoder_fifo);
	if(LeAudioFifo.dec_pcm_fifo)
		osPortFree(LeAudioFifo.dec_pcm_fifo);
	if(LeAudioFifo.buf_16lr)
		osPortFree(LeAudioFifo.buf_16lr);

	LeAudioFifo.buf_16lr = NULL;
	LeAudioFifo.dec_pcm_fifo = NULL;
	LeAudioFifo.decoder_handle = NULL;
	LeAudioFifo.decoder_fifo = NULL;
	LeAudioLc3BufLR(NULL);
	bis2_sink_src_buffer_free();
}

void  LeAudioSerEncoderDeinit(void)
{
	LeAudioFifo.init_flag = 0;
	if(LeAudioFifo.encoder_handle)
	{
		osPortFree(LeAudioFifo.encoder_handle);
	}
	if(LeAudioFifo.enc_pcm_fifo)
	{
		osPortFree(LeAudioFifo.enc_pcm_fifo);
	}
	if(LeAudioFifo.buf_16lr)
		osPortFree(LeAudioFifo.buf_16lr);

	LeAudioFifo.buf_16lr = NULL;
	LeAudioFifo.enc_pcm_fifo = NULL;
	LeAudioFifo.encoder_handle = NULL;
	LeAudioLc3BufLR(NULL);
	bis2_sink_src_buffer_free();
}

void LeAudioSerEncodeInit()
{
	if(get_encoder_reinit())
	{
		if(LeAudioFifo.encoder_handle == NULL || LeAudioFifo.enc_pcm_fifo == NULL)
		{
			uint32_t fifo_len1,fifo_len2;

			LeAudioFifo.init_flag = 0;
			if(LeAudioFifo.encoder_handle)
				osPortFree(LeAudioFifo.encoder_handle);
			if(LeAudioFifo.enc_pcm_fifo)
				osPortFree(LeAudioFifo.enc_pcm_fifo);
			if(LeAudioFifo.buf_16lr)
				osPortFree(LeAudioFifo.buf_16lr);

		#ifdef LE_AUDIO_ENC_SBC
			LeAudioFifo.CodecFormat = LE_CODEC_FORMAT_SBC;
		#else
			LeAudioFifo.CodecFormat = LE_AUDIO_LC3_TYPE;
		#endif
			if(LeAudioFifo.CodecFormat == LE_AUDIO_LC3_TYPE)
			{
//				fifo_len1 = LC3_NUM_FRAMES*4*4;
				fifo_len1 = (LC3_NUM_FRAMES*2 + AudioCoreFrameSizeGet(DefaultNet))*4;
				if(get_src_wed_vaild())
					LeAudioTypeSet(LE_AUDIO_LC3_TYPE,lc3_packet_len,LeAudioFifo.lc3_frame_len*2);
				else
		#ifdef CIS_APP_HEADSET_LR
					LeAudioTypeSet(LE_AUDIO_LC3_TYPE,LE_CIS_LC3_PACKET_LEN,LC3_FRAME_LEN*2);	
		#else
					LeAudioTypeSet(LE_AUDIO_LC3_TYPE,lc3_packet_len,LeAudioFifo.lc3_frame_len);
		#endif			

		#ifdef LE_AUDIO_ENC_LC3_2
				if(get_src_wed_vaild())
				{
					LeAudioFifo.channel 	= LC3_STEREO;
					Lc3EncodeFormatSet(LeAudioFifo.channel, LC3_FRAME_US, LeAudioFifo.lc3_bitrate);
				}
				else
			#ifdef CIS_APP_HEADSET_LR
				{
					LeAudioFifo.channel 	= LC3_STEREO;
					Lc3EncodeFormatSet(LeAudioFifo.channel, LC3_FRAME_US, LC3_BITRATE);
				}
			#else				
					Lc3EncodeFormatSet(LeAudioFifo.channel,LC3_FRAME_US,LeAudioFifo.lc3_bitrate/LeAudioFifo.channel);
			#endif			
		#else
				Lc3EncodeFormatSet(LeAudioFifo.channel,LC3_FRAME_MS,LC3_BITRATE);
		#endif

				if(LeAudioFifo.channel == LC3_STEREO)
					fifo_len2 = LC3_STEREO_EncoderHandle_Len;
				else
					fifo_len2 = LC3_MONO_EncoderHandle_Len;
		#ifdef LE_AUDIO_ENC_LC3_2
					fifo_len2 = LC3_2_STEREO_EncoderHandle_Len;
		#endif
			}
			else
			{
				LeAudioTypeSet(LE_CODEC_FORMAT_SBC,AUDIO_SBC_PACKET_LEN,AUDIO_SBC_FRAME_LEN);
				fifo_len1 = 10*1024;
				SbcEncodeFormatSet(SBC_BLOCK,SBC_SUBBAND,SBC_BITPOOL);

				fifo_len2 = SBC_EncoderHandle_Len;
			}
			APP_DBG("LeAudioSerEncoder malloc : %lu,%lu %d\n",(unsigned long)fifo_len1,(unsigned long)fifo_len2,LeAudioFifo.SrcSampleRate);

			LeAudioFifo.enc_pcm_fifo = osPortMalloc(fifo_len1);
			LeAudioFifo.encoder_handle = osPortMalloc(fifo_len2);

			LeAudioFifo.buf_16lr  = osPortMalloc(LC3_NUM_FRAMES*3*sizeof(int16_t));
			LeAudioLc3BufLR(LeAudioFifo.buf_16lr);

			if(LeAudioFifo.encoder_handle == NULL
			   || LeAudioFifo.enc_pcm_fifo == NULL)
			{
				APP_DBG("LeAudioSerEncoder malloc error! %p %p\n",LeAudioFifo.encoder_handle,LeAudioFifo.enc_pcm_fifo);
			}
			LeAudioEncPcmBufInit(LeAudioFifo.enc_pcm_fifo, fifo_len1);
		}
#ifdef LE_CIS_CONVERSATION
		LeAudio_Encoder_init(LeAudioFifo.encoder_handle,LC3_SAMPLERATE);
#else
		LeAudio_Encoder_init(LeAudioFifo.encoder_handle,LeAudioFifo.SrcSampleRate);
#endif
		set_encoder_reinit(0);
		LeAudioServiceMsgSend(MSG_TASK_START);
		LeAudioFifo.init_flag = 1;
	}
}

uint16_t LeAudioDecoderPcmDataLenGet(void)
{
	if(get_decoder_reinit())
	{
		return 0;
	}
	
	return LeAudioDec_DataLenGet();
}

uint16_t LeAudioDecoderPcmDataGet(void* Buf, uint16_t Samples)
{
	set_sink_dac_pcm_invalid(0);
//	LeAudioServiceMsgSend(MSG_NONE);
	return LeAudioDec_DataGet(Buf,Samples);
}

static void LeAudioServiceEntrance(void * param)
{
	TaskState			serviceState;
	MessageContext		msg;

	LeAudioServiceMsgHandle = MessageRegister(LEAUDIO_NUM_MESSAGE_QUEUE);

	serviceState = TaskStatePaused;		//默认暂停

	memset(&LeAudioFifo,0,sizeof(LeAudioFifo));
#ifdef LE_AUDIO_ENC_SBC
	LeAudioFifo.CodecFormat = LE_CODEC_FORMAT_SBC;
#else
	LeAudioFifo.CodecFormat = LE_AUDIO_LC3_TYPE;
	LeAudioFifo.channel 	= LC3_CHANNEL;
#endif
    LeAudioFifo.SampleRate = CFG_LC3_SAMPLERATE;
	LeAudioFifo.SrcSampleRate = CFG_LC3_SAMPLERATE;
	if(get_src_wed_vaild())
	{
    	lc3_packet_len = Get_LC3_FRAME_LEN((LC3_FRAME_US/1000),LC3_BITRATE_DUALBIS);
		LeAudioFifo.lc3_bitrate = LC3_BITRATE_DUALBIS;
	}
	else
	{
    	lc3_packet_len = Get_LC3_FRAME_LEN((LC3_FRAME_US/1000),LC3_BITRATE);
		LeAudioFifo.lc3_bitrate = LC3_BITRATE;
	}
	LeAudioFifo.lc3_frame_len = lc3_packet_len;
	LeAudioFifo.channel = LC3_CHANNEL;

#if !defined(LE_CIS_SRC) && !defined(LE_CIS_SINK)
    LeAudioSrcDataApiRegister((void *)bis_src_send_data,	(void *)bis_src_get_len);
	LeAudioSinkDataApiRegister((void *)le_bis_sink_data_get,(void *)le_bis_sink_data_len_get,	(void *)le_bis_sink_data_get_ID);
#else
	#if (LE_AUDIO_ROLE == CIS_SRC)
		LeAudioSrcDataApiRegister((void *)cis_src_send_data,	(void *)cis_src_get_len);
		LeAudioSinkDataApiRegister((void *)le_cis_src_out_data_get,(void *)le_cis_src_out_data_len_get,	NULL); //cis src rx
	
	#elif (LE_AUDIO_ROLE == CIS_SINK)
		LeAudioSrcDataApiRegister((void *)cis_sink_in_send_data,	(void *)cis_sink_in_get_len); //cis sink tx
		LeAudioSinkDataApiRegister((void *)le_cis_sink_data_get,(void *)le_cis_sink_data_len_get,	NULL);
	#endif
#endif
	while(1)
	{
		if(serviceState == TaskStatePaused)
			MessageRecv(LeAudioServiceMsgHandle, &msg, 0xffffffff);	//消息驱动
		else
			MessageRecv(LeAudioServiceMsgHandle, &msg, LEAUDIO_SERVICE_TIMEOUT);

		switch(msg.msgId)
		{
		case MSG_TASK_START:
			APP_DBG("---LeAudioService START--- %d\n",serviceState);
			serviceState = TaskStateRunning;
			break;

		case MSG_TASK_PAUSE:
			APP_DBG("---LeAudioService PAUSE---\n");
			serviceState = TaskStatePaused;
			set_encoder_reinit(1);
			set_decoder_reinit(1);
			break;
		case MSG_TASK_STOP:
			if(serviceState == TaskStatePaused)
			{
				LeAudioSerEncoderDeinit();
			}
			serviceState = TaskStatePaused;
			break;
		default:
			break;
		}

		if(serviceState == TaskStateRunning && LeAudioFifo.init_flag)
		{
#if	 BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT
			if(Get_bis_role() == BIS_SRC )
#elif !defined(LE_CIS_CONVERSATION)

			if(Get_cis_role() == CIS_SRC)
#endif
			{
				if(LeAudio_Encode() == 1)
				{
					// LeAudioServiceMsgSend(MSG_NONE);//new add
				}
			}
#if BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT
			else if( Get_bis_role() == BIS_SINK )
#elif !defined(LE_CIS_CONVERSATION)
			else if( Get_cis_role() == CIS_SINK)
#endif
			{
				if(LeAudio_Decode() == 1)
				{
//					LeAudioServiceMsgSend(MSG_NONE);//new add
                    // MessageContext                msgSend;
                    // msgSend.msgId                = MSG_NONE;
                    // MessageSend(GetAudioCoreServiceMsgHandle(), &msgSend);
				}
			}
#if !defined(LE_CIS_CONVERSATION)
			else
			{
				APP_DBG("---LeAudioService PAUSE 2---\n");
				serviceState = TaskStatePaused;
				set_encoder_reinit(1);
				set_decoder_reinit(1);
			}
#endif
		}
	}
}


void CreatLeAudioServiceTask(void)
{
	if(xTaskCreate(LeAudioServiceEntrance, "LeAudioService", LEAUDIO_SERVICE_SIZE, NULL, LEAUDIO_SERVICE_PRIO, NULL) != pdTRUE)
	{
		osTaskDelay(50);
		APP_DBG("system error ,LeAudioService create fail 1\n");
		if(xTaskCreate(LeAudioServiceEntrance, "LeAudioService", LEAUDIO_SERVICE_SIZE, NULL, LEAUDIO_SERVICE_PRIO, NULL) != pdTRUE)
		{
			while(1)
			{
				osTaskDelay(50);
				APP_DBG("system error ,LeAudioService create fail 2\n");
			}
		}
	}
}

void set_bis_pres_delay_us(uint32_t pres_delay_us)
{
	LeAudioFifo.pres_delay_us = pres_delay_us;
}

__attribute__((section(".ilm_section")))
void bis_sync_audio_close(void)
{
	LeAudioFifo.pres_delay_flag = 1;
	bis2_sink_src_buffer_free();
	if((Get_bis_role() == BIS_SINK && get_sink_wed_vaild())
	   || (Get_bis_role() == BIS_SRC && get_src_wed_vaild()))
		bis2_sink_src_buffer_malloc();

	if(Get_bis_role() == BIS_SINK)	//需要等到解码数据(LeAudioDecoderPcmDataGet)才可以填入DAC
	{
		set_sink_dac_pcm_invalid(1);
	}
#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN	//双图下清空BRIDGE_MUSIC_SOURCE通道buffer
	BridgeSinkSourceBufferClear();
#else	
#ifdef CFG_RES_DAC_MONITOR_EN
	AudioDAC_DigitalMute(DAC_SEL, 1, 1);
	AudioDAC_DataClear(DAC_SEL);
#endif
#ifdef CFG_RES_AUDIO_DAC1_MIX_OUT_EN
	AudioDAC_DigitalMute(DAC1_SEL, 1, 1);
	AudioDAC_DataClear(DAC1_SEL);
#endif
#ifdef CFG_I2S0_MIX_OUT_EN
	AudioI2S_TXDeInit(I2S0_MODULE);
	DMA_CircularWritePtrSet(PERIPHERAL_1_ID_I2S0_TX,0);
#endif
#ifdef CFG_I2S1_MIX_OUT_EN
	AudioI2S_TXDeInit(I2S1_MODULE);
	DMA_CircularWritePtrSet(PERIPHERAL_1_ID_I2S1_TX,0);
	#if (I2S1_MIX_OUT_LINE_CNT > AUDIO_LINE_SINGLE) || defined(CFG_FUNC_I2S1_TDM_MODE_EN)
	{
		extern void LeAudioI2S1MixTxFifoClear(void);
		LeAudioI2S1MixTxFifoClear();
	}
	#endif	
#endif
#ifdef CFG_I2S2_MIX_OUT_EN
	AudioI2S_TXDeInit(I2S2_MODULE);
	DMA_CircularWritePtrSet(PERIPHERAL_1_ID_I2S2_TX,0);
#endif
#endif
	LeAudioEncPcmBufClear();
	LeAudioDecPcmBufClear();
#ifdef FUNC_BIS_SUPPORT
	bis_sink_data_clr();
	bis_src_send_data_clr();
	bis_last_offset_reset();
#endif
#ifdef LE_CIS_SINK
	cis_sink_data_clr();
#endif
#ifdef LE_CIS_SRC
	cis_src_send_data_clr();
#endif
	set_decoder_reinit(1);
	set_encoder_reinit(1);
}

#ifdef LE_CIS_SRC
extern uint8_t cis_streaming;
#endif

uint16_t LeAudioEncoderPcmDataSet(void* Buf, uint16_t Len)
{
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT) && /*defined(LE_CIS_CONVERSATION) &&*/ defined(LE_CIS_SINK)
	if (cis_sink2src_streaming  && LeAudioFifo.enc_pcm_fifo != NULL)
#elif (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT) && defined(LE_CIS_SRC)
	if (cis_src2sink_streaming  && LeAudioFifo.enc_pcm_fifo != NULL)
#else
	if(check_bis_src_is_streamed() && LeAudioFifo.enc_pcm_fifo != NULL)
#endif
	{
		LeAudioEncPcmSet(Buf, Len);
		LeAudioServiceMsgSend(MSG_NONE);
	}

	return 0;
}

uint16_t GetLeAudioEncoderPcmSpaceLength(void)
{
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT) && /*defined(LE_CIS_CONVERSATION) &&*/ defined(LE_CIS_SINK)
	if (cis_sink2src_streaming  && LeAudioFifo.enc_pcm_fifo != NULL)
#elif (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT) && defined(LE_CIS_SRC)
	if (cis_src2sink_streaming  && LeAudioFifo.enc_pcm_fifo != NULL)
#else
	if(check_bis_src_is_streamed() && LeAudioFifo.enc_pcm_fifo != NULL)
#endif
	{
		return GetLeAudioEncPcmSpaceLength();
	}

	return 0;
}

#include "main_task.h"
__attribute__((section(".ilm_section")))
void bis_sync_audio_open(void)
{
#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN	//双图下开启BRIDGE_MUSIC_SOURCE通道
	AudioCoreSourceEnable(BRIDGE_MUSIC_SOURCE);
	BridgeSinkSourceBufferDrop(AudioDAC0_DataLenGet());
#else	
#ifdef CFG_RES_DAC_MONITOR_EN //george
	AudioDAC_Run(DAC_SEL);
	DMA_ChannelEnable(DAC_DMA_PERI_ID_SEL);
	AudioDAC_Enable(DAC_SEL);
	AudioDAC_DigitalMute(DAC_SEL, 0, 0);
#endif
#ifdef CFG_RES_AUDIO_DAC1_MIX_OUT_EN
	AudioDAC_Run(DAC1_SEL);
	DMA_ChannelEnable(DAC1_DMA_PERI_ID_SEL);
	AudioDAC_Enable(DAC1_SEL);
	AudioDAC_DigitalMute(DAC1_SEL, 0, 0);
#endif
#ifdef CFG_I2S0_MIX_OUT_EN
	DMA_ChannelEnable(PERIPHERAL_1_ID_I2S0_TX);
	I2S_ModuleTxEnable(I2S0_MODULE);
#endif
#ifdef CFG_I2S1_MIX_OUT_EN
	DMA_ChannelEnable(PERIPHERAL_1_ID_I2S1_TX);
	I2S_ModuleTxEnable(I2S1_MODULE);
#endif
#ifdef CFG_I2S2_MIX_OUT_EN
	DMA_ChannelEnable(PERIPHERAL_1_ID_I2S2_TX);
	I2S_ModuleTxEnable(I2S2_MODULE);
#endif
#endif
	set_sink_dac_pcm_invalid(0);
#ifdef BIS_SRC_SWITCH_MODES_WITHOUT_REBOOTING
	extern bool BisSrcInsertZeroFlag;
	BisSrcInsertZeroFlag = 0;
#endif
}

uint16_t get_bis_sync_need_samples(uint16_t	Depth)
{
	if(Get_bis_role() == BIS_SINK)
		return Depth;
	else
		return (LC3_NUM_FRAMES*(1+2+2+2+2) + AudioCoreFrameSizeGet(DefaultNet) + 1);
}

extern bool le_bis_sink_is_synced(void);
extern bool le_bis_src_is_synced(void);
__attribute__((section(".ilm_section")))
void BIS_SYNC_TIMER_INT_FUNC(void)
{
	Timer_InterruptFlagClear(BIS_SYNC_TIMER_ID, UPDATE_INTERRUPT_SRC);
	Timer_Pause(BIS_SYNC_TIMER_ID,1);
	NVIC_DisableIRQ(BIS_SYNC_TIMER_IRQ);
	bis_sync_audio_open();
}

__attribute__((section(".ilm_section")))
void bis_pd_set_callback(void)
{
	if(LeAudioFifo.pres_delay_flag)
	{
		if(Get_bis_role() == BIS_SRC && le_bis_src_is_synced())
		{
			LeAudioFifo.pres_delay_flag = 0;
			if(LeAudioFifo.pres_delay_us > 0 && LeAudioFifo.pres_delay_us < 100000)
				Timer_Config(BIS_SYNC_TIMER_ID,LeAudioFifo.pres_delay_us,0);
			else
				Timer_Config(BIS_SYNC_TIMER_ID,40000,0);
			Timer_Start(BIS_SYNC_TIMER_ID);
			Timer_InterruptFlagClear(BIS_SYNC_TIMER_ID, UPDATE_INTERRUPT_SRC);
			NVIC_EnableIRQ(BIS_SYNC_TIMER_IRQ);
		}
		else if(Get_bis_role() == BIS_SINK && le_bis_sink_is_synced())
		{
			LeAudioFifo.pres_delay_flag = 0;
#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
			if(LeAudioFifo.pres_delay_us > 0 && LeAudioFifo.pres_delay_us < 100000)
			{
				Timer_Config(BIS_SYNC_TIMER_ID,LeAudioFifo.pres_delay_us - 5540,0);
			}
			else
				Timer_Config(BIS_SYNC_TIMER_ID,40000 - 5540,0);
#else			
			if(LeAudioFifo.pres_delay_us > 0 && LeAudioFifo.pres_delay_us < 100000)
			{
				if(get_sink_wed_vaild())
					Timer_Config(BIS_SYNC_TIMER_ID,LeAudioFifo.pres_delay_us - 9540,0);
				else
					Timer_Config(BIS_SYNC_TIMER_ID, LeAudioFifo.pres_delay_us + 2070, 0);
			}
			else
				Timer_Config(BIS_SYNC_TIMER_ID,40000 + 2070,0);
#endif				
			Timer_Start(BIS_SYNC_TIMER_ID);
			Timer_InterruptFlagClear(BIS_SYNC_TIMER_ID, UPDATE_INTERRUPT_SRC);
			NVIC_EnableIRQ(BIS_SYNC_TIMER_IRQ);
		}
	}
}

#else
void bis_pd_set_callback(void)
{
}
#endif//#if BT_LE_AUDIO_SUPPORT
