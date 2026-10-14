/**
 **************************************************************************************
 * @file    recorder_service.c
 * @brief   
 *
 * @author  Pi
 * @version V1.0.0
 *
 * $Created: 2018-04-26 13:06:47$
 *
 * @Copyright (C) 2018, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include <string.h>
#include "type.h"
#include "rtos_api.h"
#include "app_message.h"
#include "app_config.h"
#include "debug.h"
#include "gpio.h"
#include "mp3enc_api.h"
#include "mp2enc_api.h"
//#include "mode_switch_api.h"
#include "media_play_api.h"
#include "audio_core_api.h"
#include "mcu_circular_buf.h"
#include "recorder_service.h"
#include "timeout.h"
#ifdef CFG_FUNC_RECORD_SD_UDISK
#include "ff.h"
#include "sd_card.h"
#include "otg_host_hcd.h"
#include "otg_host_standard_enum.h"
#include "otg_host_udisk.h"
#elif defined(CFG_FUNC_RECORD_FLASHFS)
#include "file.h"
#endif
#include "main_task.h"
#include "remind_sound.h"
#include <nds32_intrinsic.h>
#include "device_detect.h"
#ifdef	CFG_FUNC_RECORDER_EN
#include "rom.h"

//#define CFG_FUNC_RECORDER_ADD_FADOUT_EN				//录音结束增加Fadout处理

//static uint32_t sRecSamples    = 0;
//#define	MEDIA_RECORDER_FILE_SECOND			30	//自动截断文件再建立新文件，屏蔽后录成单个文件。
bool MediaRecorderServiceInit(MessageHandle parentMsgHandle);
#ifdef CFG_FUNC_RECORD_EXTERN_FLASH_EN
static struct
{
	REC_EXTFLASH_HEAD	RecHead;
	uint8_t 			RecIndex;
	uint32_t			RecAddr;
}ExFlashRec = {0};

#endif

#if MEDIA_RECORDER_FORMAT == RECORD_FORMAT_WAV
	#define ENCODER_OUT_BUF_SIZE			0
#elif MEDIA_RECORDER_FORMAT == RECORD_FORMAT_MP2
	#define ENCODER_OUT_BUF_SIZE			MP2_ENCODER_OUTBUF_CAPACITY//((RecorderCt->SamplePerFrame * MEDIA_RECORDER_BITRATE * 1000 / 8) / CFG_PARA_SAMPLE_RATE + 1100)//单次编码帧（576/1152)输出的size，8为防溢出。
#elif MEDIA_RECORDER_FORMAT == RECORD_FORMAT_MP3
	#define ENCODER_OUT_BUF_SIZE			MP3_ENCODER_OUTBUF_CAPACITY
#endif

#if FILE_WRITE_FIFO_LEN <= 2048
#define FILE_WRITE_BUF_LEN					(512)
#else
#define FILE_WRITE_BUF_LEN					(FILE_WRITE_FIFO_LEN / 4)
#endif


#define MEDIA_RECORDER_TASK_STACK_SIZE		512// 1024
#define MEDIA_RECORDER_TASK_PRIO			2
#define MEDIA_RECORDER_RECV_MSG_TIMEOUT		1

#define MEDIA_RECORDER_NUM_MESSAGE_QUEUE	10

typedef enum
{
	RecorderStateNone = 0,
	RecorderStateEncode,	//编码
	RecorderStateWriteFile,	//写盘
	RecorderStateSaveFlie,	//文件处理，存盘或者关闭文件

}RecorderState;

#ifdef CFG_FUNC_RECORDER_ADD_FADOUT_EN
typedef enum
{
	RecorderFadoutNone = 0,
	RecorderFadoutBegin,
	RecorderFadoutFrame,	//fadout处理
	RecorderFadoutMute,		//
	RecorderFadoutWaitSaveFile
}RecorderFadoutState;
#endif

typedef struct _MediaRecorderContext
{
	xTaskHandle 		taskHandle;
	MessageHandle		parentMsgHandle;

	TaskState			state;
	RecorderState		RecState;			

	uint16_t			*Sink1Fifo;//sink和recorder之间缓冲。
	MCU_CIRCULAR_CONTEXT	Sink1FifoCircular;


	/* for Encoder */
	uint16_t 			RemainSample;//编码buf当前采样点数
	int16_t				*EncodeBuf;
	uint8_t				*EncodeOutBuf;
#if MEDIA_RECORDER_FORMAT == RECORD_FORMAT_WAV
#elif MEDIA_RECORDER_FORMAT == RECORD_FORMAT_MP2
	MP2EncoderContext	*Mp2EncCon;
#elif MEDIA_RECORDER_FORMAT == RECORD_FORMAT_MP3
	MP3EncoderContext	*Mp3EncCon;
#endif
	int32_t				SamplePerFrame;
	
	/* for file */
	MCU_CIRCULAR_CONTEXT FileWCircularBuf;
	int8_t				*FileWFifo;
	int8_t				*WriteBuf; 	//写盘buf
	osMutexId			FifoMutex;//录音buf 推拉流 锁
	
	bool				RecorderOn;
	bool				EncodeOn;
	uint32_t 			SampleRate; //带提示音时，如果不重采样，此处设成提示音采样率一致。
	uint32_t			sRecSamples;
	uint32_t			sRecordingTime ; // unit ms
	uint32_t			sRecordingTimePre ;
	uint32_t			SyncFileMs;//上次同步的录音长度。避免拔出设备引起 文件长度为0。


#ifdef CFG_FUNC_RECORD_SD_UDISK
	FIL					RecordFile;

	FATFS				MediaFatfs;
	uint8_t 			DiskVolume;//设备录音和回放 卷
	uint16_t			FileIndex; //录音文件编号
	uint16_t			FileNum;//录音文件计数器，限制数量以便回放排序。
	//文件操作
	FIL 				FileHandle;
	DIR 				Dir;
	FILINFO 			FileInfo;
#elif defined(CFG_FUNC_RECORD_FLASHFS)
	FFILE				*RecordFile;

#endif

	uint16_t 			SendAgainID;
#ifdef CFG_FUNC_RECORDER_ADD_FADOUT_EN
	uint16_t				fadout_delay_cnt;
	RecorderFadoutState		fadout_state;
#endif
}MediaRecorderContext;

#if MEDIA_RECORDER_FORMAT == RECORD_FORMAT_WAV
static WavFileHeader wfh;
WavInfo_t WavInfo;
#endif
static  MediaRecorderContext*		RecorderCt = NULL;

const char* gMediaVolumePaths[] = {
    MEDIA_VOLUME_STR_C,  // 下标0 → MEDIA_VOLUME_C
    MEDIA_VOLUME_STR_U,  // 下标1 → MEDIA_VOLUME_U
    MEDIA_VOLUME_STR_UH  // 下标2 → MEDIA_VOLUME_UH
};

void MediaRecorderStopProcess(void);

#ifdef CFG_FUNC_RECORD_SD_UDISK
uint16_t RecFileIndex(char *string)
{
	uint32_t Index;
	if(string[5] != '.')
	{
		//APP_DBG("fileName error\n");
		return 0;
	}
	if(string[0] > '9' || string[0] < '0'|| string[1] > '9' || string[1] < '0' || string[2] > '9' || string[2] < '0' || string[3] > '9' || string[3] < '0' || string[4] > '9' || string[4] < '0')
	{
		//APP_DBG("%c%c%c%c%c", string[0], string[1], string[2], string[3], string[4]);
		return 0;
	}
#if MEDIA_RECORDER_FORMAT == RECORD_FORMAT_WAV
	if(string[6] !=  'w' || string[7] !=  'a' || string[8] !=  'v')
	{
		//APP_DBG("not wav");
		return 0;
	}
#elif MEDIA_RECORDER_FORMAT == RECORD_FORMAT_MP2 || MEDIA_RECORDER_FORMAT == RECORD_FORMAT_MP3
	if(string[6] !=  'M' || string[7] !=  'P' || string[8] !=  '3')
	{
		//APP_DBG("not mp3");
		return 0;
	}
#endif
	Index = (string[0] - '0') * 10000 + (string[1] - '0') * 1000	+ (string[2] - '0') * 100 + (string[3] - '0') * 10 + (string[4] - '0');
	if(Index > FILE_NAME_MAX)
	{
		APP_DBG("File index too large\n");
	}
	return Index;
}

void IntToStrMP3Name(char *string, uint16_t number)
{
	if(number > FILE_NAME_MAX)
	{
		return ;
	}
	string[0] = number / 10000 + '0';
	string[1] = (number % 10000) / 1000 + '0';
	string[2] = (number % 1000) / 100 + '0';
	string[3] = (number % 100) / 10 + '0';
	string[4] = number % 10 + '0';
	string[5] = '.';
#if MEDIA_RECORDER_FORMAT == RECORD_FORMAT_WAV
	string[6] = 'w';
	string[7] = 'a';
	string[8] = 'v';
#elif  MEDIA_RECORDER_FORMAT == RECORD_FORMAT_MP2 || MEDIA_RECORDER_FORMAT == RECORD_FORMAT_MP3
	string[6] = 'M';
	string[7] = 'P';
	string[8] = '3';
#endif
	string[9] = '\0';
}

//注意Vol 为卷号，0 或1；
static void FilePathStringGet(char *FilePath, uint32_t FileIndex, uint8_t Vol)
{
	strcpy(FilePath, gMediaVolumePaths[Vol]);

	strcat(FilePath, CFG_PARA_RECORDS_FOLDER);

	strcat(FilePath + strlen(FilePath), "/");

	IntToStrMP3Name(FilePath + strlen(FilePath), FileIndex);
}
void DelRecFileRecSevice(uint32_t current_rec_index);// 
void DelRecFile(void);
//文件序号初始化，需要先初始化卷号。
static bool FileIndexInit(void)
{
#ifdef CFG_FUNC_RECORD_SD_UDISK
	uint32_t Index = 0;
	char FilePath[FILE_PATH_LEN];
	uint16_t RecFileList = 0;

	strcpy(FilePath, gMediaVolumePaths[RecorderCt->DiskVolume]);
	f_chdrive(gMediaVolumePaths[RecorderCt->DiskVolume]);

	strcat(FilePath, CFG_PARA_RECORDS_FOLDER);
	RecorderCt->FileIndex = 0;
	RecorderCt->FileNum = 0;
	if(FR_OK != f_opendir(&RecorderCt->Dir, FilePath))
	{
		APP_DBG("f_opendir failed: %s\n", FilePath);
		if(f_mkdir((TCHAR*)FilePath) != FR_OK)
		{
			MediaRecorderStopProcess();
			return FALSE;
		}
		else
		{
			RecorderCt->FileIndex = 1;
			RecorderCt->FileNum = 0;
			return TRUE;
		}
	}
	else
	{
		while(((f_readdir(&RecorderCt->Dir, &RecorderCt->FileInfo)) == FR_OK) && RecorderCt->FileInfo.fname[0])
		{
			if(RecorderCt->FileInfo.fattrib & AM_ARC)
			{
				Index = RecFileIndex(RecorderCt->FileInfo.fname);
				if(Index > 0 && Index <= FILE_NAME_MAX)
				{
					RecorderCt->FileNum++;
					if(Index > RecFileList)
					{
						RecFileList = Index;//找已存在的录音文件最大编号
					}
				}
			}
		}
		if(RecFileList < FILE_NAME_MAX && RecorderCt->FileNum < FILE_INDEX_MAX)
		{
			RecorderCt->FileIndex = RecFileList + 1;//最大编号下一个编号作为新录音文件编号
			return TRUE;
		}
#ifdef AUTO_DEL_REC_FILE_FUNCTION
		else
		{
			uint32_t i_count=FILE_INDEX_MAX;
			while(i_count>=1)
			{
				APP_DBG("del i_count=%d\n",i_count);
				DelRecFileRecSevice(i_count);
				i_count--;
			}
			RecorderCt->FileIndex = 1;
			RecorderCt->FileNum = 0;
			return TRUE;
		}
#endif
	}
	return FALSE;
#endif
}

#ifdef MEDIA_RECORDER_FILE_SECOND
static bool MediaRecorderNextFileIndex()
{
	bool Ret = FALSE;
	char FilePath[FILE_PATH_LEN + 1];
	
	while(!Ret)
	{
		RecorderCt->FileIndex++;
		if(RecorderCt->FileIndex > FILE_NAME_MAX || RecorderCt->FileNum >= FILE_INDEX_MAX)
		{
			return FALSE;
		}
		FilePathStringGet(FilePath, RecorderCt->FileIndex, RecorderCt->DiskVolume);
		if((f_open(&RecorderCt->FileHandle, (TCHAR*)FilePath, FA_READ | FA_OPEN_EXISTING)) == FR_OK)
		{
			f_close(&RecorderCt->FileHandle);
		}
		else
		{
			Ret = TRUE;
		}
	}
	return Ret;
}
#endif
#endif //CFG_FUNC_RECORD_SD_UDISK

//name must be a short name!!!
static bool MediaRecorderOpenDataFile(void)//uint8_t SongType)
{
#ifdef CFG_FUNC_RECORD_SD_UDISK
	char FilePath[FILE_PATH_LEN + 1];
	if(RecorderCt->FileIndex > FILE_NAME_MAX)
	{
		return FALSE;
	}
	FilePathStringGet(FilePath, RecorderCt->FileIndex, RecorderCt->DiskVolume);
	APP_DBG("Name:%s\n", FilePath);
	FRESULT ress = f_open(&RecorderCt->RecordFile, (TCHAR*)FilePath, FA_CREATE_ALWAYS | FA_WRITE);
//	if((f_open(&RecorderCt->RecordFile, (TCHAR*)FilePath, FA_CREATE_ALWAYS | FA_WRITE)) != FR_OK)
	if(ress != FR_OK)
	{
		DBG("ress: %d\n", ress);
		RecorderCt->RecorderOn = FALSE;
		APP_DBG("Open file error!\n");
		return FALSE;
	}
	else
	{
		APP_DBG("Open File ok!\n");
#if MEDIA_RECORDER_FORMAT == RECORD_FORMAT_WAV
		if(MCUCircular_GetSpaceLen(&RecorderCt->FileWCircularBuf) < sizeof(WavFileHeader))
		{
			APP_DBG("FileWCircularBuf:fifo Error!\n");
		}
		osMutexLock(RecorderCt->FifoMutex);
		MCUCircular_PutData(&RecorderCt->FileWCircularBuf, &wfh, sizeof(WavFileHeader));
		osMutexUnlock(RecorderCt->FifoMutex);
		APP_DBG("File PutData Header ok! %d\n",sizeof(WavFileHeader));
#endif

		RecorderCt->FileNum++;//以文件创建为基准，增加文件计数器
		return TRUE;
	}
#elif defined(CFG_FUNC_RECORD_FLASHFS)
	if((RecorderCt->RecordFile = Fopen(CFG_PARA_FLASHFS_FILE_NAME, "w")) == NULL)
	{
		APP_DBG("Open file error!\n");
		return FALSE;
	}
	else
	{
		APP_DBG("Open File ok!\n");
		return TRUE;
	}
#endif
#ifdef CFG_FUNC_RECORD_EXTERN_FLASH_EN
	return TRUE;
#endif
}

#if MEDIA_RECORDER_FORMAT == RECORD_FORMAT_WAV
void wav_write_header(FIL* f, WavInfo_t* wi)
{
	uint32_t file_size;
	UINT write_size;
	if (f==NULL)
		return;

	file_size = f->obj.objsize;
	f_lseek(f, 0);

	wfh.chunk_id[0]='R'; wfh.chunk_id[1]='I'; wfh.chunk_id[2]='F'; wfh.chunk_id[3]='F';
	wfh.chunk_size = file_size - 8;
	wfh.format[0]='W'; wfh.format[1]='A'; wfh.format[2]='V'; wfh.format[3]='E';
	wfh.subchunk1_id[0]='f'; wfh.subchunk1_id[1]='m'; wfh.subchunk1_id[2]='t'; wfh.subchunk1_id[3]=' ';
	wfh.subchunk1_size = 16;
	wfh.audio_format = 1;
	wfh.num_channels = wi->num_channels;
	wfh.sample_rate = wi->sample_rate;
	wfh.byte_rate = wi->sample_rate*wi->num_channels*wi->bits_per_sample/8;
	wfh.block_align = wi->num_channels*wi->bits_per_sample/8;
	wfh.bits_per_sample = wi->bits_per_sample; // usually 16-bit
	wfh.subchunk2_id[0]='d'; wfh.subchunk2_id[1]='a'; wfh.subchunk2_id[2]='t'; wfh.subchunk2_id[3]='a';
	wfh.subchunk2_size = (wi->bits_per_sample/8)*wi->num_channels*wi->num_samples;

	f_write(f, &wfh, sizeof(wfh), &write_size);
}
#endif

uint16_t MediaRecorderDataSpaceLenGet(void)
{
	return MCUCircular_GetSpaceLen(&RecorderCt->Sink1FifoCircular) / (MEDIA_RECORDER_CHANNEL* 2);
}

//Len: Unit: SampleS
uint16_t MediaRecorderDataSet(void* Buf, uint16_t Len)
{

	int16_t* pBuf = Buf;
	if(Len == 0)
	{
		return 0;
	}
	if(RecorderCt->RecorderOn)
	{
		if(MCUCircular_GetSpaceLen(&RecorderCt->Sink1FifoCircular) / (MEDIA_RECORDER_CHANNEL* 2) <= Len)
			APP_DBG("*");
#if (MEDIA_RECORDER_CHANNEL == 1)
		uint16_t i;
		for(i=0; i<Len; i++)
		{
			pBuf[i] = __nds32__clips((int32_t)pBuf[2 * i] + pBuf[2 * i + 1], (16)-1);//__SSAT((((int32_t)pBuf[2 * i] + (int32_t)pBuf[2 * i + 1]) / 2), 16);
		}
#endif

		MCUCircular_PutData(&RecorderCt->Sink1FifoCircular, pBuf, Len * MEDIA_RECORDER_CHANNEL * 2);

	}
	return 0;
}

bool encoder_init(int32_t num_channels, int32_t sample_rate, int32_t *samples_per_frame)
{
	*samples_per_frame = (sample_rate > 32000)?(1152):(576);
#if MEDIA_RECORDER_FORMAT  == RECORD_FORMAT_WAV
	*samples_per_frame = ENCODER_WAV_SAMPLE_SIZE;
#elif MEDIA_RECORDER_FORMAT  == RECORD_FORMAT_MP2
	return mp2_encoder_initialize(RecorderCt->Mp2EncCon, num_channels, sample_rate, MEDIA_RECORDER_BITRATE);
#elif MEDIA_RECORDER_FORMAT  == RECORD_FORMAT_MP3
	if(sample_rate < 24000)
	{
		return mp3_encoder_initialize(RecorderCt->Mp3EncCon, num_channels, sample_rate, MEDIA_RECORDER_BITRATE, 0);
	}
	else
	{
		return mp3_encoder_initialize(RecorderCt->Mp3EncCon, num_channels, sample_rate, MEDIA_RECORDER_BITRATE, 12000);
	}
#endif
}

bool encoder_encode(int16_t *pcm_in, uint8_t *data_out, uint32_t *plength)
{
#if MEDIA_RECORDER_FORMAT == RECORD_FORMAT_WAV
	#if WAV_BIT_PER_SAMPLE == 24
		int32_t i;
		for(i=(*plength-1);i>=0;i--)
		{
			data_out[i*3+2] = data_out[i*2+1];     // 第三个字节取自16bit的高8位
			data_out[i*3+1] = data_out[i*2];       // 第二个字节取自16bit的低8位
			data_out[i*3] = 0x80;                // 24bit数据的第一个字节
		}
		*plength *= 3;
	#else
		*plength *= 2;
	#endif
	return 0;
#elif MEDIA_RECORDER_FORMAT  == RECORD_FORMAT_MP2
	return mp2_encoder_encode(RecorderCt->Mp2EncCon, pcm_in, data_out, plength);
#elif MEDIA_RECORDER_FORMAT  == RECORD_FORMAT_MP3
	return mp3_encoder_encode(RecorderCt->Mp3EncCon, pcm_in, data_out, plength);
#endif
}

void MediaRecorderEncode(void)
{
	uint32_t Len;
	if(RecorderCt != NULL && RecorderCt->RecorderOn && AudioCoreSinkIsEnable(AUDIO_RECORDER_SINK_NUM))
	{
		RecorderCt->EncodeOn = TRUE;
		Len = MCUCircular_GetDataLen(&RecorderCt->Sink1FifoCircular);
		if(Len / (MEDIA_RECORDER_CHANNEL * 2) + RecorderCt->RemainSample  >= RecorderCt->SamplePerFrame)
		{

			MCUCircular_GetData(&RecorderCt->Sink1FifoCircular, 
							RecorderCt->EncodeBuf + RecorderCt->RemainSample * MEDIA_RECORDER_CHANNEL,
							(RecorderCt->SamplePerFrame - RecorderCt->RemainSample) * MEDIA_RECORDER_CHANNEL * 2);
#ifdef CFG_FUNC_RECORDER_ADD_FADOUT_EN
			if(RecorderCt->fadout_state == RecorderFadoutBegin || RecorderCt->fadout_state == RecorderFadoutFrame)
			{
				uint32_t i;
				uint16_t Vol, VolStep = 4096 / (RecorderCt->SamplePerFrame) + 1;
				if(RecorderCt->fadout_state == RecorderFadoutFrame)
				{
					Vol = 0;
					RecorderCt->fadout_state = RecorderFadoutMute;
				}
				else
				{
					Vol = 4096;
					RecorderCt->fadout_state = RecorderFadoutFrame;
				}
				for(i=0;i< RecorderCt->SamplePerFrame * MEDIA_RECORDER_CHANNEL;i++)
				{
					if(i!= 0 && (i%MEDIA_RECORDER_CHANNEL) == 0)
					{
						if(Vol > VolStep)
							Vol -= VolStep;
						else
							Vol = 0;
					}
					RecorderCt->EncodeBuf[i]  = __nds32__clips(((((int32_t)RecorderCt->EncodeBuf[i]) * Vol + 2048) >> 12), (16)-1);
				}
			}
#endif
#if MEDIA_RECORDER_FORMAT == RECORD_FORMAT_WAV
			Len = RecorderCt->SamplePerFrame * MEDIA_RECORDER_CHANNEL;
			if(!encoder_encode(NULL, (uint8_t *)RecorderCt->EncodeBuf, &Len))
#elif MEDIA_RECORDER_FORMAT == RECORD_FORMAT_MP2 || MEDIA_RECORDER_FORMAT == RECORD_FORMAT_MP3
			if(!encoder_encode(RecorderCt->EncodeBuf, RecorderCt->EncodeOutBuf, &Len))
#endif
			{
				if(MCUCircular_GetSpaceLen(&RecorderCt->FileWCircularBuf) < Len)
				{
					APP_DBG("Encoder:fifo Error!\n");//兼容性错误警告。录音丢数据了，FILE_WRITE_FIFO_LEN不适配。
				}
				osMutexLock(RecorderCt->FifoMutex);
#ifdef CFG_FUNC_RECORDER_ADD_FADOUT_EN
				if(RecorderCt->fadout_state < RecorderFadoutWaitSaveFile)
#endif
				{
					#if MEDIA_RECORDER_FORMAT == RECORD_FORMAT_WAV
					MCUCircular_PutData(&RecorderCt->FileWCircularBuf, RecorderCt->EncodeBuf, Len);
					#elif MEDIA_RECORDER_FORMAT == RECORD_FORMAT_MP2 || MEDIA_RECORDER_FORMAT == RECORD_FORMAT_MP3
					MCUCircular_PutData(&RecorderCt->FileWCircularBuf, RecorderCt->EncodeOutBuf, Len);
					#endif
				}
				osMutexUnlock(RecorderCt->FifoMutex);
				RecorderCt->sRecSamples += RecorderCt->SamplePerFrame;

#ifdef CFG_FUNC_RECORDER_ADD_FADOUT_EN
				if(RecorderCt->fadout_state == RecorderFadoutMute)
					RecorderCt->fadout_state = RecorderFadoutWaitSaveFile;
#endif
				//APP_DBG("RecorderCt->RecSampleS = %d\n", RecorderCt->sRecSamples);
			}
			else
			{
				APP_DBG("Encoder Error!\n");
				RecorderCt->EncodeOn = FALSE;
				return ;//编码帧错误，丢弃。
			}
			RecorderCt->RemainSample = 0;
		}
		else if(Len >= MEDIA_RECORDER_CHANNEL * 2)
		{
			MCUCircular_GetData(&RecorderCt->Sink1FifoCircular, 
							RecorderCt->EncodeBuf + RecorderCt->RemainSample * MEDIA_RECORDER_CHANNEL,
							Len);
			RecorderCt->RemainSample += Len /(MEDIA_RECORDER_CHANNEL * 2);
		}
		RecorderCt->EncodeOn = FALSE;
	}
}


#ifdef CFG_FUNC_RECORD_EXTERN_FLASH_EN
extern uint32_t	rec_addr_start;
void ExFlashRecorderSaveHead(void)
{
	uint32_t addr;
	if(ExFlashRec.RecIndex <= CFG_PARA_RECORDS_INDEX && ExFlashRec.RecIndex > 0)
	{
		addr = rec_addr_start + CFG_PARA_RECORDS_MAX_SIZE * (ExFlashRec.RecIndex - 1);
		DBG("[HHH]ExFlashRecorderSaveHead: FileSize=%d,Crc=%x\n", ExFlashRec.RecHead.RecSize,ExFlashRec.RecHead.RecCrc);
		SpiWrite(addr, &ExFlashRec.RecHead, sizeof(REC_EXTFLASH_HEAD));
	}
	memset(&ExFlashRec,0,sizeof(ExFlashRec));
}

bool IsRecorderRunning(void)
{
	return RecorderCt != NULL;
}

void DelExFlashRecFile(uint8_t index)
{
	uint32_t size_4k;
	if(index <= CFG_PARA_RECORDS_INDEX && index > 0)
	{
		size_4k = rec_addr_start+ CFG_PARA_RECORDS_MAX_SIZE * (index - 1);
#ifndef CFG_PARA_RECORDS_ERASE_ALL
		uint32_t rec_max_len = 0;
		rec_max_len = rec_max_len + ((size_4k/(64*1024) + 1)*(64*1024) - size_4k);

		FlashEraseExt(size_4k, rec_max_len, 1);
#else
		FlashEraseExt(size_4k, CFG_PARA_RECORDS_MAX_SIZE, 1); //全部擦除
#endif
	}
}

void ExFlashRecorderStartIndex(MessageHandle parentMsgHandle,uint8_t index)
{
#ifndef USE_EXTERN_FLASH_SPACE
	extern char __data_lmastart;
	extern bool flash_table_is_valid(void);

	if(!flash_table_is_valid() ||		//flash中没有搜索到索引表
		rec_addr_start < (uint32_t)&__data_lmastart) //起始地址在代码区
	{
		DBG("[HHH] rec_addr_start Error!  0x%lx\n",rec_addr_start);
		return;
	}
#endif
	DBG("index %d \n",index);
	if(index <= CFG_PARA_RECORDS_INDEX && index > 0)
	{
		DBG("[HHH]ExFlash Recorder the %d voice\n",index);
		memset(&ExFlashRec,0,sizeof(ExFlashRec));
		DelExFlashRecFile(ExFlashRec.RecIndex);	// 录音前删除之前的录音文件
		ExFlashRec.RecIndex = index;
		MediaRecorderServiceInit(parentMsgHandle);
	}
}
#endif

void MediaRecorderStopProcess(void)
{
	MessageContext	msgSend;
	uint32_t 		Times;
	Times = ((uint64_t)RecorderCt->sRecSamples * 1000) / RecorderCt->SampleRate;
	while(RecorderCt->EncodeOn)
	{
		osTaskDelay(1);				//让录音服务得到执行。
	}
	AudioCoreSinkDisable(AUDIO_RECORDER_SINK_NUM);
	if(RecorderCt->RecorderOn)
	{
		APP_DBG("Close\n");
#if MEDIA_RECORDER_FORMAT == RECORD_FORMAT_WAV
		WavInfo.bits_per_sample = WAV_BIT_PER_SAMPLE;
		WavInfo.num_channels = MEDIA_RECORDER_CHANNEL;
		WavInfo.sample_rate = RecorderCt->SampleRate;
		WavInfo.num_samples = RecorderCt->sRecSamples;
		wav_write_header(&RecorderCt->RecordFile,&WavInfo);
#endif
#ifdef CFG_FUNC_RECORD_SD_UDISK
		f_close(&RecorderCt->RecordFile);
#elif defined(CFG_FUNC_RECORD_FLASHFS) 
		if(RecorderCt->RecordFile)
		{
			Fclose(RecorderCt->RecordFile);
			RecorderCt->RecordFile = NULL;
		}
#endif
	}
#ifdef CFG_FUNC_RECORDS_MIN_TIME
	if(Times < CFG_FUNC_RECORDS_MIN_TIME && RecorderCt->RecorderOn)
	{
#ifdef CFG_FUNC_RECORD_SD_UDISK
		char FilePath[FILE_PATH_LEN];
		FilePathStringGet(FilePath, RecorderCt->FileIndex, RecorderCt->DiskVolume);
		f_unlink(FilePath);
		RecorderCt->FileNum--;
#elif defined(CFG_FUNC_RECORD_FLASHFS) 
		Remove(CFG_PARA_FLASHFS_FILE_NAME);
#endif
		APP_DBG("Delete file , too short!\n");
	}
//#ifdef CFG_FUNC_RECORD_SD_UDISK
//	else //录盘条件下，文件没有删，需切到下一首，准备好录下一首。
//	{
//		
//	}
//#endif //#ifdef CFG_FUNC_RECORD_SD_UDISK
#endif //#ifdef CFG_FUNC_RECORDS_MIN_TIME

	Times /= 1000;//转换为秒
	RecorderCt->RecorderOn = FALSE;

	if(Times)
	{
		APP_DBG("Recorded %d M %d S\n", (int)(Times / 60), (int)(Times % 60));
	}
	RecorderCt->sRecSamples = 0;
	RecorderCt->RemainSample = 0;
	RecorderCt->SyncFileMs = 0;
	RecorderCt->state = TaskStateStopping;

	while(RecorderCt->state != TaskStateStopping)
	{
		osTaskDelay(1);				//等待录音服务写卡操作执行完成。
	}
#ifdef CFG_FUNC_RECORD_EXTERN_FLASH_EN
	ExFlashRecorderSaveHead();
#endif
	msgSend.msgId = MSG_MEDIA_RECORDER_STOPPED;
	MessageSend(RecorderCt->parentMsgHandle, &msgSend);
//	SoftFlagDeregister(SoftFlagRecording);//清理
}

#ifdef CFG_FUNC_RECORD_SD_UDISK
bool RecordUDiskDiskMount(void)
{
#ifdef CFG_FUNC_HOST_HS_DETECT
	if(GetUdiscHsState() == DETECT_STATE_IN)
	{
		if(f_mount(&RecorderCt->MediaFatfs, MEDIA_VOLUME_STR_UH, 1) == 0)
		{
			RecorderCt->DiskVolume = MEDIA_VOLUME_UH;
			APP_DBG("U盘卡挂载到 %d:/--> 成功\n",RecorderCt->DiskVolume);
			return TRUE;
		}
		else
		{
			APP_DBG("U盘卡挂载到 %d:/--> 失败\n",RecorderCt->DiskVolume);
			return FALSE;
		}
	}
	else
#endif
#ifdef CFG_FUNC_HOST_FS_DETECT
		if(GetUdiscState() == DETECT_STATE_IN)
	{
		if(f_mount(&RecorderCt->MediaFatfs, MEDIA_VOLUME_STR_U, 1) == 0)
		{
			RecorderCt->DiskVolume = MEDIA_VOLUME_U;
			APP_DBG("U盘卡挂载到 %d:/--> 成功\n",RecorderCt->DiskVolume);
			return TRUE;
		}
		else
		{
			APP_DBG("U盘卡挂载到 %d:/--> 失败\n",RecorderCt->DiskVolume);
			return FALSE;
		}
	}
#endif
	return FALSE;
}

bool RecordSDCardDiskMount(void)
{
#ifdef CFG_FUNC_CARD_DETECT
	if(GetCardState() == DETECT_STATE_IN)
	{
		if(f_mount(&RecorderCt->MediaFatfs, MEDIA_VOLUME_STR_C, 1) == 0)
		{
			RecorderCt->DiskVolume = MEDIA_VOLUME_C;
			APP_DBG("SD mount 0:/--> ok\n");
			if(GetSystemMode() == ModeBtAudioPlay || GetSystemMode() == ModeBtHfPlay)
			{
				SD_CARD *sd = SDCard_GetCardInfo();
				osMutexLock(SDIOMutex);
				SDIO_ClkDisable();
				SDIO_ClkSet(sd->MaxTransSpeed + 1);//降低clk
				osMutexUnlock(SDIOMutex);
				APP_DBG("SDCard MaxTransSpeed: %d\n",sd->MaxTransSpeed + 1);
			}
			return TRUE;
		}
		else
		{
			APP_DBG("SD mount 0:/--> fail\n");
			return FALSE;
		}
	}
#endif
	return FALSE;
}

bool RecordDiskMount(void)
{
#ifdef CFG_FUNC_RECORD_UDISK_FIRST
    return RecordUDiskDiskMount() || RecordSDCardDiskMount();
#else
    return RecordSDCardDiskMount() || RecordUDiskDiskMount();
#endif
}
#endif

void ShowRecordingTime(void);
#ifdef MEDIAPLAYER_REC_DISK_PERF_OPTIMIZE
uint32_t RecFileBuf[1024];
#endif
static bool MediaRecorderDataProcess(void)
{
	uint32_t LenFile, Len = 0;

	switch(RecorderCt->RecState)
	{
		case RecorderStateWriteFile:
			//写卡处理
			osMutexLock(RecorderCt->FifoMutex);
#ifdef MEDIAPLAYER_REC_DISK_PERF_OPTIMIZE
			if(MCUCircular_GetDataLen(&RecorderCt->FileWCircularBuf) < sizeof(RecFileBuf))
			{
				osMutexUnlock(RecorderCt->FifoMutex);
				break;
			}
			Len = MCUCircular_GetData(&RecorderCt->FileWCircularBuf,RecFileBuf,sizeof(RecFileBuf));
			RecorderCt->WriteBuf = (int8_t*)RecFileBuf;
#else
			if(RecorderCt->FileWCircularBuf.W < RecorderCt->FileWCircularBuf.R)// 读指针在buf末尾
			{
				RecorderCt->WriteBuf = &RecorderCt->FileWCircularBuf.CircularBuf[RecorderCt->FileWCircularBuf.R];
				Len = RecorderCt->FileWCircularBuf.BufDepth - RecorderCt->FileWCircularBuf.R;
			}
#ifdef CFG_FUNC_RECORDER_ADD_FADOUT_EN
			else if(RecorderCt->fadout_state == RecorderFadoutWaitSaveFile)
			{
				RecorderCt->WriteBuf = &RecorderCt->FileWCircularBuf.CircularBuf[RecorderCt->FileWCircularBuf.R];
				Len = RecorderCt->FileWCircularBuf.W - RecorderCt->FileWCircularBuf.R;
			}
#endif
			else if((LenFile = MCUCircular_GetDataLen(&RecorderCt->FileWCircularBuf)) < FILE_WRITE_BUF_LEN)
			{
				osMutexUnlock(RecorderCt->FifoMutex);
				break;
			}
			else
			{
				RecorderCt->WriteBuf = &RecorderCt->FileWCircularBuf.CircularBuf[RecorderCt->FileWCircularBuf.R];
				Len = ((RecorderCt->FileWCircularBuf.W - RecorderCt->FileWCircularBuf.R) / 512) * 512;
			}
#endif
			osMutexUnlock(RecorderCt->FifoMutex);

			unsigned int RetLen;
#ifdef CFG_FUNC_RECORD_SD_UDISK			
			//录音媒介 兼容性测试 请分析此行执行阻塞情况。
			FRESULT Ret = f_write(&RecorderCt->RecordFile,RecorderCt->WriteBuf, Len, &RetLen);
			if((Ret != FR_OK) || (Ret == FR_OK && Len != RetLen))
#elif defined(CFG_FUNC_RECORD_FLASHFS)
			if(!Fwrite(RecorderCt->WriteBuf, Len, 1, RecorderCt->RecordFile))
#elif defined(CFG_FUNC_RECORD_EXTERN_FLASH_EN)
			{
				#ifndef CFG_PARA_RECORDS_ERASE_ALL
				uint16_t SECTOR1,SECTOR2;

				SECTOR1 = ExFlashRec.RecAddr / (64*1024);
				SECTOR2 = (ExFlashRec.RecAddr+Len) / (64*1024);

				for(SECTOR1 += 1; SECTOR1 <= SECTOR2; SECTOR1++)
				{
					//APP_DBG("SECTOR1:%d\n", SECTOR1);
					SpiErase(SECTOR1);
				}	
				#endif
			}
			//APP_DBG("Len:%d\n", Len);
			if(FLASH_NONE_ERR != SpiWrite(ExFlashRec.RecAddr, RecorderCt->WriteBuf, Len))
			{
				//APP_DBG("FLASH_NONE_ERR!\n");
			}
			ExFlashRec.RecHead.RecCrc = ROM_CRC16(RecorderCt->WriteBuf, Len,ExFlashRec.RecHead.RecCrc);
			ExFlashRec.RecAddr += Len;
			ExFlashRec.RecHead.RecSize += Len;
			if(0)
#endif
			{
				APP_DBG("File Write Error!\n");
#ifndef MEDIAPLAYER_REC_DISK_PERF_OPTIMIZE
				RecorderCt->FileWCircularBuf.R = (RecorderCt->FileWCircularBuf.R + RetLen) % RecorderCt->FileWCircularBuf.BufDepth;
#endif
#ifdef CFG_FUNC_RECORD_SD_UDISK
				if(RecorderCt->MediaFatfs.free_clst <= 1) //fat缺省0xFFFFFFFF, 当系统更新此参数有效时，输出log
				{
					APP_DBG("No disk space for record\n");
				}
#endif
				MessageContext		msgSend;
				msgSend.msgId		= MSG_MEDIA_RECORDER_ERROR;
				MessageSend(RecorderCt->parentMsgHandle, &msgSend);
				break;
				//return FALSE;//写入错误，停止，应该加上磁盘信息。比如空间满。
			}
#ifndef MEDIAPLAYER_REC_DISK_PERF_OPTIMIZE
			RecorderCt->FileWCircularBuf.R = (RecorderCt->FileWCircularBuf.R + Len) % RecorderCt->FileWCircularBuf.BufDepth;
#endif
			RecorderCt->RecState = RecorderStateSaveFlie;
//			break;
		case RecorderStateSaveFlie:
			
			RecorderCt->sRecordingTime=((uint64_t)RecorderCt->sRecSamples * 1000) / RecorderCt->SampleRate;
			//if(GetRecEncodeState()!=TaskStatePausing)
			ShowRecordingTime();
#ifdef CFG_FUNC_RECORD_EXTERN_FLASH_EN

			uint32_t rec_max_len = (CFG_PARA_RECORDS_MAX_SIZE-CFG_PARA_RECORDS_INFO_SIZE-ENCODER_OUT_BUF_SIZE*2);
#ifndef CFG_PARA_RECORDS_ERASE_ALL
			if((rec_addr_start%(64*1024)) != 0)	//录音起始地址没有64k对齐则去掉尾部非64k部分
			{
				rec_max_len = rec_max_len - (rec_addr_start - rec_addr_start/(64*1024)*(64*1024));
			}
#endif
			if(((RecorderCt->sRecordingTime / 1000) > EXTERN_FLASH_RECORDER_FILE_SECOND)
				|| ExFlashRec.RecHead.RecSize >= rec_max_len)
			{
				DBG("ExFlash size out!\n");
				MessageContext		msgSend;
				msgSend.msgId		= MSG_MEDIA_RECORDER_ERROR;
				MessageSend(RecorderCt->parentMsgHandle, &msgSend);
				return FALSE;//写入错误，停止，应该加上磁盘信息。比如空间满。
			}
#endif

#ifdef MEDIA_RECODER_SYNC_FILE_MS
			//存盘处理
			//此处设置2秒钟同步一次文件，以防突然拔卡操作/断电。
			if(((uint64_t)RecorderCt->sRecSamples * 1000) / RecorderCt->SampleRate - RecorderCt->SyncFileMs >= MEDIA_RECODER_SYNC_FILE_MS || RecorderCt->SyncFileMs == 0 )
			{
				//同步是写入fat表和文件长度信息。媒介兼容性测试时，需关注此行执行阻塞情况。
			#ifdef CFG_FUNC_RECORD_SD_UDISK
				if(f_sync(&RecorderCt->RecordFile) == FR_OK)
			#elif defined(CFG_FUNC_RECORD_FLASHFS)
				Fflush( RecorderCt->RecordFile);
			#endif
				{
					RecorderCt->SyncFileMs = ((uint64_t)RecorderCt->sRecSamples * 1000) / RecorderCt->SampleRate;
					//APP_DBG(" File Ms = %d\n", (int)RecorderCt->SyncFileMs);
				}
			}
#endif

#if defined(MEDIA_RECORDER_FILE_SECOND) && defined(CFG_FUNC_RECORD_SD_UDISK)
			uint32_t Times;
			Times = (RecorderCt->sRecSamples) / RecorderCt->SampleRate;

			if(Times >= MEDIA_RECORDER_FILE_SECOND) //文件长度取决于预设时长。
			{
				APP_DBG("File Recorded %d M %d S\n", (int)(Times / 60), (int)(Times % 60));
				RecorderCt->sRecSamples = 0;
				RecorderCt->SyncFileMs = 0;
				f_close(&RecorderCt->RecordFile);
				if(MediaRecorderNextFileIndex())
				{
					if(MediaRecorderOpenDataFile())
					{
						return TRUE;//打开成功则继续录音
					}
				}
				return FALSE;
			}
#endif
			RecorderCt->RecState = RecorderStateWriteFile;
			break;
		default:
			break;
	}
	return TRUE;
}

static void MediaRecorderServiceStopProcess(void)
{
	if(RecorderCt->state != TaskStateStopping
			&& RecorderCt->state != TaskStateStopped
			&& RecorderCt->state != TaskStateNone)
	{
		if(RecorderCt->state != TaskStatePaused)
		{
			MediaRecorderStopProcess();
		}
//		RecorderCt->state = TaskStateStopping;
	}
}

/**
 * @func        MediaRecorder_Init
 * @brief       MediaRecorder模式参数配置，资源初始化
 * @param       MessageHandle parentMsgHandle
 * @Output      None
 * @return      int32_t
 * @Others      任务块、Adc、Dac、AudioCore配置
 * @Others      数据流从Adc到audiocore配有函数指针，audioCore到Dac同理，由audiocoreService任务按需驱动
 * Record
 */
static bool MediaRecorder_Init(MessageHandle parentMsgHandle)
{
	bool ret = TRUE;
	AudioCoreIO AudioIOSet;
	uint8_t RecordSinkFlowNet = AudioCoreGetNetBySinkChannelNUM(AUDIO_RECORDER_SINK_NUM);
	RecorderCt = (MediaRecorderContext*)osPortMalloc(sizeof(MediaRecorderContext));
	if(RecorderCt == NULL)
	{
		return FALSE;
	}
	//Task config & Service para
	memset(RecorderCt, 0, sizeof(MediaRecorderContext));

	APP_DBG("R:%d\n", (int)xPortGetFreeHeapSize());
	RecorderCt->parentMsgHandle = parentMsgHandle;
	RecorderCt->state = TaskStateCreating;
	//para
	RecorderCt->SampleRate = AudioCoreMixSampleRateGet(RecordSinkFlowNet); //注意采样率同步，从sink1获取。
	RecorderCt->RecorderOn = FALSE;
	RecorderCt->EncodeOn = FALSE;

	//编码控制 句柄
#if MEDIA_RECORDER_FORMAT == RECORD_FORMAT_WAV
#elif MEDIA_RECORDER_FORMAT == RECORD_FORMAT_MP2
	RecorderCt->Mp2EncCon = (MP2EncoderContext*)osPortMalloc(sizeof(MP2EncoderContext));
	if(RecorderCt->Mp2EncCon  == NULL)
	{
		return FALSE;
	}
#elif MEDIA_RECORDER_FORMAT == RECORD_FORMAT_MP3
	RecorderCt->Mp3EncCon = (MP3EncoderContext*)osPortMalloc(sizeof(MP3EncoderContext));
	if(RecorderCt->Mp3EncCon  == NULL)
	{
		return FALSE;
	}
#endif

	encoder_init(MEDIA_RECORDER_CHANNEL, RecorderCt->SampleRate, &(RecorderCt->SamplePerFrame));
	if(RecorderCt->SamplePerFrame > MEDIA_ENCODER_SAMPLE_MAX)
	{
		APP_DBG("Encoder frame error!!!");//保护性检测
	}

	RecorderCt->Sink1Fifo = (uint16_t*)osPortMalloc(MEDIA_RECORDER_FIFO_LEN);
	if(RecorderCt->Sink1Fifo  == NULL)
	{
		return FALSE;
	}
	MCUCircular_Config(&RecorderCt->Sink1FifoCircular, RecorderCt->Sink1Fifo, MEDIA_RECORDER_FIFO_LEN);


	AudioCoreSinkDisable(AUDIO_RECORDER_SINK_NUM);
	memset(&AudioIOSet, 0, sizeof(AudioCoreIO));
	AudioIOSet.Depth = AudioCore.NetCt[RecordSinkFlowNet].FrameSize * 2 ;

	if(!AudioCoreSinkIsInit(AUDIO_RECORDER_SINK_NUM))
	{
		AudioIOSet.Adapt = STD;
		AudioIOSet.Sync = TRUE;
		AudioIOSet.Channels = 2;
		AudioIOSet.Net = AudioCoreGetNetBySinkChannelNUM(AUDIO_RECORDER_SINK_NUM);
		AudioIOSet.DataIOFunc = MediaRecorderDataSet;
		AudioIOSet.LenGetFunc = MediaRecorderDataSpaceLenGet;
		AudioIOSet.PcmWidth	= WIDTH_16BIT;
		if(!AudioCoreSinkInit(&AudioIOSet, AUDIO_RECORDER_SINK_NUM))
		{
			DBG("REC init error");
			return FALSE;
		}
	}


	//编码输入buf
	if(RecorderCt->EncodeBuf == NULL) 
	{
		RecorderCt->EncodeBuf = (int16_t*)osPortMalloc(RecorderCt->SamplePerFrame * MEDIA_RECORDER_CHANNEL * 3);
		if(RecorderCt->EncodeBuf == NULL)
		{
			return FALSE;
		}
	}
	//编码输出buf
#if MEDIA_RECORDER_FORMAT == RECORD_FORMAT_WAV
#elif MEDIA_RECORDER_FORMAT == RECORD_FORMAT_MP3 || MEDIA_RECORDER_FORMAT == RECORD_FORMAT_MP2
	RecorderCt->EncodeOutBuf = osPortMalloc(ENCODER_OUT_BUF_SIZE);
	if(RecorderCt->EncodeOutBuf == NULL)
	{
		return FALSE;
	}
#endif

	if(RecorderCt->FileWFifo == NULL)
	{
		RecorderCt->FileWFifo = (int8_t*)osPortMalloc(FILE_WRITE_FIFO_LEN);
		if(RecorderCt->FileWFifo == NULL)//MP3 和pcm size 需求不同
		{
			return FALSE;
		}
	}
	MCUCircular_Config(&RecorderCt->FileWCircularBuf, RecorderCt->FileWFifo, FILE_WRITE_FIFO_LEN);

	if((RecorderCt->FifoMutex = xSemaphoreCreateMutex()) == NULL)
	{
		return FALSE;
	}
#ifdef CFG_FUNC_RECORD_SD_UDISK
	RecorderCt->FileNum = 0;
#endif
	return ret;
}

static void MediaRecorderEntrance(void * param)
{
	APP_DBG("MediaRecorder Service\n");
	SoftFlagRegister(SoftFlagRecording);//登记录音状态，用于屏蔽后插先播
#ifdef CFG_FUNC_RECORD_EXTERN_FLASH_EN
	if(ExFlashRec.RecIndex <= CFG_PARA_RECORDS_INDEX && ExFlashRec.RecIndex > 0)
	{
		ExFlashRec.RecAddr = rec_addr_start + CFG_PARA_RECORDS_INFO_SIZE + CFG_PARA_RECORDS_MAX_SIZE * (ExFlashRec.RecIndex - 1);
		DBG("[HHH]DelExFlashRecFile:%d,addr:0x%x\n",ExFlashRec.RecIndex,ExFlashRec.RecAddr);
		DelExFlashRecFile(ExFlashRec.RecIndex);	// 录音前删除之前的录音文件
	}
#endif
	while(1)
	{
#ifdef CFG_FUNC_RECORDER_ADD_FADOUT_EN
		if(RecorderCt->fadout_state !=RecorderFadoutNone)
		{
			RecorderCt->fadout_delay_cnt++;
			if(RecorderCt->fadout_delay_cnt > 100	//超时退出
			 || (RecorderCt->fadout_state == RecorderFadoutWaitSaveFile && MCUCircular_GetDataLen(&RecorderCt->FileWCircularBuf) < 10))
			{
				RecServierToParent(MSG_STOP_REC);
				RecorderCt->fadout_delay_cnt 	= 0;
				RecorderCt->fadout_state 		= RecorderFadoutNone;
			}
		}
#endif
		if(RecorderCt->state <= TaskStateRunning)
		{
			if(!RecorderCt->RecorderOn)
			{
	#ifdef CFG_FUNC_RECORD_SD_UDISK
				//bool	Result = FALSE;
				if(RecordDiskMount())
				{
					if(RecorderCt->MediaFatfs.free_clst != 0xFFFFFFFF) //fat系统 缺省值，即存在有效值时，输出log
					{
						APP_DBG("Space Remain: %u (Clst) * %u Sector\n", (unsigned int)RecorderCt->MediaFatfs.free_clst, (unsigned int)RecorderCt->MediaFatfs.csize);
					}

					if(FileIndexInit() == FALSE)
					{
						RecorderCt->RecorderOn = FALSE;
						RecorderCt->RecState = RecorderStateNone;
						APP_DBG("Rec Number Full!\n");
						continue;
					}
				}

	#endif//注意：flashfs dev初始化上电时进行。
				RecorderCt->RecorderOn = TRUE;
				RecorderCt->state	= TaskStateRunning;
				RecorderCt->RecState = RecorderStateWriteFile;//RecorderStateEncode;//sam add
				if(MediaRecorderOpenDataFile())
				{
					//MCUCircular_Config(&RecorderCt->CircularHandle, RecorderCt->SinkRecorderFIFO, MEDIA_RECORDER_FIFO_LEN);
					AudioCoreSinkEnable(AUDIO_RECORDER_SINK_NUM);// BKD CHANGE ,DO not change again please
					APP_DBG("Audio core sink 1 enable\n");
				}else{
					RecorderCt->state = TaskStateStopping;
					MediaRecorderStopProcess();
					APP_DBG("MediaRecorder Init buf & Disk Space error!\n");
				}
			}
			if(RecorderCt->RecorderOn)
			{
				if(MediaRecorderDataProcess() == FALSE)
				{
					if(RecorderCt->state != TaskStatePaused)
					{
						RecorderCt->state = TaskStateStopping;
						MediaRecorderStopProcess();
					}
				}
			}
		}
		if(RecorderCt->state == TaskStatePaused)
		{
			RecorderCt->state = TaskStateStopping;
		}
		osTaskDelay(1);
	}
}

bool MediaRecorderServiceKill(void);
void RecServierToParent(uint16_t id)
{
	MessageContext		msgSend;

	// Send message to main app
	msgSend.msgId		= id;
	MessageSend(GetMainMessageHandle(), &msgSend);
}

void RecServierToParentAgain(uint16_t id)
{
	RecServierToParent(MSG_STOP_REC);
	if(RecorderCt)
		RecorderCt->SendAgainID = id;
}

#ifdef CFG_FUNC_RECORDER_ADD_FADOUT_EN
bool RecServierFadoutBegin(void)
{
	if(RecorderCt && RecorderCt->fadout_state == RecorderFadoutNone)
	{
		RecorderCt->fadout_delay_cnt = 0;
		RecorderCt->fadout_state = RecorderFadoutBegin;
		return TRUE;
	}
	return FALSE;
}
#endif
/***************************************************************************************
 *
 * APIs
 *
 */
bool MediaRecorderServiceInit(MessageHandle parentMsgHandle)
{
	bool		ret = TRUE;
	APP_DBG("MediaRecorderServiceCreate\n");
	ret = MediaRecorder_Init(parentMsgHandle);
	if(ret)
	{
		RecorderCt->taskHandle = NULL;
		xTaskCreate(MediaRecorderEntrance,
					"Recorder",
					MEDIA_RECORDER_TASK_STACK_SIZE,
					NULL,
					MEDIA_RECORDER_TASK_PRIO,
					&RecorderCt->taskHandle);
		if(RecorderCt->taskHandle == NULL)
		{
			ret = FALSE;
		}
	}
	APP_DBG("R:%d\n", (int)xPortGetFreeHeapSize());
	if(!ret)
	{
		MediaRecorderServiceKill();
		APP_DBG("Media Recorder service: create fail!\n");
	}
	return ret;
}

bool MediaRecorderServiceKill(void)
{
	APP_DBG("MediaRecorderServiceKill\n");
	SoftFlagDeregister(SoftFlagRecording);//清理
	//Kill used services
	if(RecorderCt == NULL)
	{
		APP_DBG("RecorderCt NULL\n");
		return FALSE;
	}
	AudioCoreSinkDisable(AUDIO_RECORDER_SINK_NUM);
	//task
	if(RecorderCt->taskHandle != NULL)
	{
		vTaskDelete(RecorderCt->taskHandle);
		RecorderCt->taskHandle = NULL;
	}
#ifdef CFG_FUNC_RECORD_SD_UDISK
	f_unmount(gMediaVolumePaths[RecorderCt->DiskVolume]);
#endif

	AudioCoreSinkDeinit(AUDIO_RECORDER_SINK_NUM);
	//PortFree
	if(RecorderCt->EncodeBuf != NULL)
	{
		osPortFree(RecorderCt->EncodeBuf);
		RecorderCt->EncodeBuf = NULL;
	}
	if(RecorderCt->EncodeOutBuf != NULL)
	{
		osPortFree(RecorderCt->EncodeOutBuf);
		RecorderCt->EncodeOutBuf = NULL;
	}
#if MEDIA_RECORDER_FORMAT == RECORD_FORMAT_WAV
#elif MEDIA_RECORDER_FORMAT == RECORD_FORMAT_MP2
	if(RecorderCt->Mp2EncCon != NULL)
	{
		osPortFree(RecorderCt->Mp2EncCon);
		RecorderCt->Mp2EncCon = NULL;
	}
#elif MEDIA_RECORDER_FORMAT == RECORD_FORMAT_MP3
	if(RecorderCt->Mp3EncCon != NULL)
	{
		osPortFree(RecorderCt->Mp3EncCon);
		RecorderCt->Mp3EncCon = NULL;
	}
#endif
//	if(RecorderCt->WriteBuf != NULL)
//	{
//		osPortFree(RecorderCt->WriteBuf);
//		RecorderCt->WriteBuf = NULL;
//	}
	if(RecorderCt->FifoMutex != NULL)
	{
		vSemaphoreDelete(RecorderCt->FifoMutex);
		RecorderCt->FifoMutex = NULL;
	}
	if(RecorderCt->FileWFifo != NULL)
	{
		osPortFree(RecorderCt->FileWFifo);
		RecorderCt->FileWFifo = NULL;
	}
	if(RecorderCt->Sink1Fifo != NULL)
	{
		osPortFree(RecorderCt->Sink1Fifo);
		RecorderCt->Sink1Fifo = NULL;
	}
	if(RecorderCt->SendAgainID)
		RecServierToParent(RecorderCt->SendAgainID);
	osPortFree(RecorderCt);
	RecorderCt = NULL;
	return TRUE;
}

bool MediaRecorderServiceDeinit(void)
{
	MediaRecorderServiceStopProcess();
	//MediaRecorderServiceKill();
	return TRUE;
}

xTaskHandle GetMediaRecorderTaskHandle(void)
{
	if(RecorderCt != NULL && RecorderCt->taskHandle != NULL)
	{
//		APP_DBG("Rec Service Run ...\n");
		return RecorderCt->taskHandle;
	}else{
//		APP_DBG("Rec Service Null ...\n");
		return NULL;
	}
}

TaskState GetMediaRecorderState(void)
{
	if(RecorderCt != NULL)
	{
//		APP_DBG("Rec State = %d\n",RecorderCt->state);
		return RecorderCt->state;
	}else{
//		APP_DBG("Rec State Task StateNone\n");
		return TaskStateNone;
	}
}

void ShowRecordingTime(void)
{
#ifdef CFG_FUNC_RECORD_SD_UDISK
	if((RecorderCt->sRecordingTime / 1000 - RecorderCt->sRecordingTimePre / 1000 >= 1))
	{
	#if MEDIA_RECORDER_FORMAT == RECORD_FORMAT_WAV
		APP_DBG("%u.wav Recording(%lds)\n",RecorderCt->FileIndex,RecorderCt->sRecordingTime / 1000);//can use IntToStrMP3Name()
	#elif MEDIA_RECORDER_FORMAT == RECORD_FORMAT_MP2 || MEDIA_RECORDER_FORMAT == RECORD_FORMAT_MP3
		APP_DBG("%u.mp3 Recording(%lds)\n",RecorderCt->FileIndex,RecorderCt->sRecordingTime / 1000);//can use IntToStrMP3Name()
	#endif
		RecorderCt->sRecordingTimePre = RecorderCt->sRecordingTime;
	}
#endif
#ifdef CFG_FUNC_RECORD_EXTERN_FLASH_EN
 	 if((RecorderCt->sRecordingTime / 1000 - RecorderCt->sRecordingTimePre / 1000 >= 1))
	 {
 		 APP_DBG("ExFlashRecording %u(%lds)\n",ExFlashRec.RecIndex,RecorderCt->sRecordingTime / 1000);//can use IntToStrMP3Name()
 		 RecorderCt->sRecordingTimePre = RecorderCt->sRecordingTime;
	 }
#endif 
}

bool IsRecoding(void)
{
	if(RecorderCt != NULL && RecorderCt->RecorderOn)
		return TRUE;
	else
		return FALSE;

}

#define MALLOC_REAL_SIZE(a)		((a) % 32 ? ((a) / 32 + 2) * 32 : ((a) / 32 + 1) * 32)
bool MediaRecordHeapEnough(void)
{
	uint32_t TotalSize;
	uint8_t RecordSinkFlowNet = AudioCoreGetNetBySinkChannelNUM(AUDIO_RECORDER_SINK_NUM);
	/******OS********/
	TotalSize = (256 + 192);//TCB 和 mailbox，注意前者也可能是128.
	TotalSize += MALLOC_REAL_SIZE(MEDIA_RECORDER_TASK_STACK_SIZE * 4);//Stack
	/*********PCM FIFO************/
	TotalSize += MALLOC_REAL_SIZE(MEDIA_RECORDER_FIFO_LEN);
	/**********Encode buf****************/
	TotalSize += MALLOC_REAL_SIZE((((AudioCoreMixSampleRateGet(RecordSinkFlowNet)) > 32000)?(1152):(576)) * 2 * MEDIA_RECORDER_CHANNEL);
	/************Encoder**************************/
	TotalSize += MALLOC_REAL_SIZE(sizeof(MP3EncoderContext));
	/**************Encode out*********************/
	TotalSize += MALLOC_REAL_SIZE(ENCODER_OUT_BUF_SIZE);
	/**************MP3 Data FIFO for write************************/
	TotalSize += MALLOC_REAL_SIZE(FILE_WRITE_FIFO_LEN);
	/*************Recoder********************/
	TotalSize += MALLOC_REAL_SIZE(sizeof(MediaRecorderContext));
	/*************Sink Frame**************************/
	TotalSize += 4 * SOURCEFRAME(0) + 32;
	if(xPortGetFreeHeapSize() < TotalSize)
	{
		APP_DBG("Error: Need %d， ram not enough!\n", (int)TotalSize);
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}


void DelRecFileRecSevice(uint32_t current_rec_index)// 
{
#ifdef CFG_FUNC_RECORD_SD_UDISK
	char FilePath[FILE_PATH_LEN];
	strcpy(FilePath, gMediaVolumePaths[RecorderCt->DiskVolume]);
	strcat(FilePath, CFG_PARA_RECORDS_FOLDER);
	strcat(FilePath,"/");
	IntToStrMP3Name(FilePath + strlen(FilePath),current_rec_index);
	if(f_unlink(FilePath))
	{
		APP_DBG("%ldREC.mp3 DEL failure\n",current_rec_index);
	}
#endif	
}

void RecServicePause(void)
{
	if(RecorderCt->state != TaskStateStopping
			&& RecorderCt->state != TaskStateStopped
			&& RecorderCt->state != TaskStateNone)
	{
		if(IsRecoding())
		{
			if(AudioCoreSinkIsEnable(AUDIO_RECORDER_SINK_NUM))
			{
				AudioCoreSinkDisable(AUDIO_RECORDER_SINK_NUM);
				APP_DBG("rec pasue \n");
			}
			else
			{
				AudioCoreSinkEnable(AUDIO_RECORDER_SINK_NUM);
				APP_DBG("rec GO \n");
			}
		}
	}
}

void RecServierMsg(uint16_t msg)
{
//	DBG("msg: %d\n", msg);
	if (GetSystemMode() == ModeIdle) // idle模式不进录音
	{
		return;
	}

	switch(msg)
	{
#ifdef CFG_FUNC_RECORDER_EN
		case MSG_STOP_REC:
			if(GetMediaRecorderState() == TaskStateRunning)//录音 停止
			{
				APP_DBG("MSG_STOP_REC\n");
				MediaRecorderServiceDeinit();
			}
			break;
		case MSG_REC:
			APP_DBG("MSG_REC\n");
			if(IsNormalPlay()||IsNormalPlayback())
				break;
			osTaskDelay(100);
#ifdef	CFG_FUNC_RECORD_SD_UDISK
			if(GetSysModeState(ModeCardAudioPlay)!=ModeStateSusend || GetSysModeState(ModeUDiskAudioPlay)!=ModeStateSusend || GetSysModeState(ModeUDiskHsAudioPlay)!=ModeStateSusend)
			{
				if(GetMediaRecorderState() == TaskStateNone)
				{
					if(!MediaRecordHeapEnough())
					{
						break;
					}
					MediaRecorderServiceInit(GetSysModeMsgHandle());
#ifdef CFG_FUNC_REMIND_SOUND_EN
					RemindSoundServiceItemRequest(SOUND_REMIND_LUYIN, REMIND_PRIO_NORMAL);
//					osTaskDelay(350);//即“录音”提示音本身时长
#endif
				}
				else if(GetMediaRecorderState() == TaskStateRunning)//再按录音键 停止
				{
#ifdef CFG_FUNC_RECORDER_ADD_FADOUT_EN
					if(RecServierFadoutBegin())
					{
						APP_DBG("RecServierFadoutBegin!!\n");
						break;
					}
#endif
					APP_DBG("SOUND_REMIND_STOPREC\n");
#ifdef CFG_FUNC_REMIND_SOUND_EN
					RemindSoundServiceItemRequest(SOUND_REMIND_STOPREC, REMIND_PRIO_NORMAL);
#endif
					MediaRecorderServiceDeinit();
				}
			}
			else
#endif
			{//flashfs录音 待处理
				APP_DBG("rec:error, no disk!!!\n");
			}

		break;

#ifdef CFG_FUNC_RECORD_EXTERN_FLASH_EN
		case MSG_REC1:
		case MSG_REC2:
			APP_DBG("MSG_REC1!!!\n");
			if(!IsRecorderRunning() && (GetMediaRecorderState() == TaskStateNone))
			{
				ExFlashRecorderStartIndex(GetSysModeMsgHandle(), msg - MSG_REC1 + 1);
			}
			else if(GetMediaRecorderState() == TaskStateRunning)//再按录音键 停止
			{
				MediaRecorderServiceDeinit();
			}
			break;
#endif
		case MSG_REC_PLAYBACK:
			APP_DBG("MSG_REC_PLAYBACK\n");
			if(GetSysModeState(ModeUDiskHsPlayBack) == ModeStateRunning)
			{
				RecServierToParent(MSG_DEVICE_SERVICE_U_DISK_HS_BACK_OUT);
				break;
			}
			if(GetSysModeState(ModeUDiskPlayBack) == ModeStateRunning)
			{
				RecServierToParent(MSG_DEVICE_SERVICE_U_DISK_BACK_OUT);
				break;
			}
			if(GetSysModeState(ModeCardPlayBack) == ModeStateRunning)
			{
				RecServierToParent(MSG_DEVICE_SERVICE_CARD_BACK_OUT);
				break;
			}
#ifdef CFG_FUNC_RECORD_SD_UDISK
#ifdef CFG_FUNC_RECORD_UDISK_FIRST
			if(GetSysModeState(ModeUDiskHsAudioPlay) != ModeStateSusend)
			{
				SetSysModeState(ModeUDiskHsPlayBack,ModeStateReady);
				RecServierToParent(MSG_DEVICE_SERVICE_U_DISK_HS_BACK_IN);
				break;
			}
			if(GetSysModeState(ModeUDiskAudioPlay) != ModeStateSusend)
			{
				SetSysModeState(ModeUDiskPlayBack,ModeStateReady);
				RecServierToParent(MSG_DEVICE_SERVICE_U_DISK_BACK_IN);
				break;
			}
			if(GetSysModeState(ModeCardAudioPlay) != ModeStateSusend)
			{
				SetSysModeState(ModeCardPlayBack,ModeStateReady);
				RecServierToParent(MSG_DEVICE_SERVICE_CARD_BACK_IN);
				break;
			}
#else
			if(GetSysModeState(ModeCardAudioPlay) != ModeStateSusend)
			{
				SetSysModeState(ModeCardPlayBack,ModeStateReady);
				SysModeEnter(ModeCardPlayBack);
				break;
			}
			if(GetSysModeState(ModeUDiskHsAudioPlay) != ModeStateSusend)
			{
				SetSysModeState(ModeUDiskHsPlayBack,ModeStateReady);
				RecServierToParent(MSG_DEVICE_SERVICE_U_DISK_HS_BACK_IN);
				break;
			}
			if(GetSysModeState(ModeUDiskAudioPlay) != ModeStateSusend)
			{
				SetSysModeState(ModeUDiskPlayBack,ModeStateReady);
				SysModeEnter(ModeUDiskPlayBack);
				break;
			}

#endif
#endif
			break;
#ifdef CFG_FUNC_RECORD_EXTERN_FLASH_EN
			case MSG_REC1_PLAYBACK:
			case MSG_REC2_PLAYBACK:
				RemindServiceItemRequestExt(msg - MSG_REC1_PLAYBACK + 1);
			break;

			case MSG_DEL_ALL_REC:
			{
				if(RemindServiceItemReplaying() || (GetMediaRecorderState() == TaskStateRunning))
				{
					APP_DBG("replaying not clear\n");
				}else{
					APP_DBG("clear all rec file\n");
					uint8_t i;
					extern void DelExFlashRecFile(uint8_t index);
					for(i = 1;i <= CFG_PARA_RECORDS_INDEX; i++)
						DelExFlashRecFile(i);
				}
				
			}
			break;
#endif
#ifdef DEL_REC_FILE_EN
		case MSG_REC_FILE_DEL:
			if(IsNormalPlayback())// || GetSystemMode() == AppModeFlashFsPlayBack)
			{
#if defined(CFG_APP_USB_PLAY_MODE_EN) || defined(CFG_APP_USB_HS_PLAY_MODE_EN) || defined(CFG_APP_CARD_PLAY_MODE_EN)
				if(GetMediaPlayerState() == PLAYER_STATE_PLAYING)
				{
					MediaPlayerStop();
					DelRecFile();
					MediaPlayerPlayPause();
				}
				else
#endif
				{
					APP_DBG("not playback mode playing state\n");
				}
			}
			else
			{
				APP_DBG("not playback mode \n");
			}
			break;
#endif
		case MSG_PLAY_PAUSE:
			if(GetMediaRecorderTaskHandle() != NULL)
			{
//				APP_DBG("RECORDER_GO_PAUSED\n");
				RecServicePause();
				msg = MSG_NONE;
			}
			break;
		case MSG_MEDIA_RECORDER_STOPPED:
			MediaRecorderServiceKill();
			break;

		case MSG_MEDIA_RECORDER_ERROR:
			MediaRecorderServiceDeinit();
			break;
#endif
	}
}
#endif //ifdef CFG_FUNC_RECORDER_EN
