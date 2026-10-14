/*
 * audio_association.h
 *
 *  Created on: Oct 27, 2023
 *      Author: piwang
 */

#ifndef AUDIO_ASSOCIATION_H_
#define AUDIO_ASSOCIATION_H_

//libaudio_association.a内帧配置
#define AUDIOASSOCIATION_VER	"1.1.1"

#include "type.h"
#include "timeout.h"

/***strans frame packet
 * 编码包 + PacketIndex + 0xFF
 */
#define PACKET_FRAME_LEN(AupackLen) 		(AupackLen + PACKET_CRC_LEN+PACKET_CNT_LEN)
#define PACKET_INDEX_OFFSET(AupackLen)		(AupackLen)
#define PACKET_INFO_OFFSET(AupackLen)		(AupackLen + 1)

#define DEVICE1_MASK	(1<<0)
#define DEVICE2_MASK	(1<<1)

/**
 * audio out samples remain when packet recevice callback funcion
 */
typedef uint16_t (*AudioAssociationSampleRemain_cb_fn) (void);

/**
 * audio out step Offset change callback funcion
 * uint8_t Device:DEVICE1_MASK/DEVICE2_MASK
 */
typedef void (*AudioAssociationSync_cb_fn) (uint8_t Device);

typedef void (*AudioAssociation2T1RDelaySyncReset_cb_fn) (uint8_t id);

typedef enum _FRAME_DEVICE_PACKETS
{
	MONO_ONE_DEVICE = 0, //单声道单设备单包,Dev1:M1,M2,M3...
	STEREO_ONE_DEVICE = 1,//立体声单设备单包,Dev1:LR1,LR2,LR3....
	STEREO_TWO_PACKET = 2, //立体声单设备双包,Dev1:L1,R1,L2,R2,L3,R3....
	STEREO_TWO_DEVICE = 3, //立体声双设备组包, Dev1:L1,L2,L3...;Dev2:R1,R2,R3....
} FRAME_DEVICE_PACKETS;

typedef struct Rx_AudioAssociation_param
{
	FRAME_DEVICE_PACKETS    FrameGroups; //音频数据帧分组设定
	unsigned char*	 	SilencePack;		//Encoder out with Silence Frame
	unsigned char		AddHeaderLen;		//本地恢复包头长度
	unsigned char   	Encoder_out_len;	//bytes
	unsigned short   	wireless_frames;
	unsigned char	 	frame_len;
	unsigned char	 	pack_info_len;

	unsigned char*   	dec_buf_p;			//Frame Decoder inbuf
	short*	         	MicLeftPcmFifo_p;
	short*          	MicRightPcmFifo_p;
	unsigned int	 	MicLeftPcmFifo_len_set;
	unsigned int	 	MicRightPcmFifo_len_set;

	uint8_t				LogEnable;
	AudioAssociationSampleRemain_cb_fn RemainFunc;
	AudioAssociationSync_cb_fn AudioOutSyncFunc;
	AudioAssociation2T1RDelaySyncReset_cb_fn Wireless2_2T1RDelaySyncResetFunc_t;
}Rx_AudioAssociation_param_t;

typedef enum _RECV_STATUS
{
	RECV_NONE,
	RECV_PACKET,
	RECV_FULL,
	RECV_TIMEOUT,
} RECV_STATUS;

typedef enum _CONNECT_STATUS
{
	CONNECT_NONE = 0,
	CONNECT_WIRELESS,
	CONNECT_AUDIO,
}CONNECT_STATUS;

typedef enum _SYNC_PACKET_STATUS
{
	SYNC_PACKET_NONE = 0,
	SYNC_PACKET_START,	//Wait First Packet
	SYNC_PACKET_ID,
	SYNC_PACKET_DECODE,
	SYNC_PACKET_LOST,
}SYNC_PACKET_STATUS;

#define HANDLE_NONE				0xff
#define HANDLE_DEVICE1_FLAG		0x80
typedef struct _DEVICE_CONTEXT
{
	unsigned short handle;
	CONNECT_STATUS ConStatus;
	uint32_t RecvNum;
	uint32_t PlayFrame;
	uint32_t MuteFrame;
	uint16_t RstNum;
} DEVICE_CONTEXT;

typedef struct _DEVICE_INFO
{
	SYNC_PACKET_STATUS AudioSycn;
	bool 		SyncTimerEn;
	TIMER 		SyncTimer;
	uint8_t 	PcmReadyLen;
	uint8_t 	CurID;
	uint8_t 	RecvID;
	uint16_t	RecvTimingOffset;//
	RECV_STATUS	RecvStatus; //收数据包状态登记,应用层可清
	uint32_t 	MissPackCnt;//包连续失期计数
} DEVICE_INFO;

extern DEVICE_CONTEXT device1;
extern DEVICE_CONTEXT device2;
extern DEVICE_INFO Device1Info;
extern DEVICE_INFO Device2Info;

/***
 * AudioAssociation init function
 * audio_config:context buf
 */
void AudioAssociationInit(Rx_AudioAssociation_param_t *audio_config);

/***
 * Recv packets fifo & sync reset
 */
void WirelessAudioRxSyncReset(void);

/***
 * Device1 Recv packets fifo & sync reset
 */
void WirelessAudioDevice1RxSyncReset(void);

/***
 * Device2 Recv packets fifo & sync reset
 */
void WirelessAudioDevice2RxSyncReset(void);

/****
 * Rx packet record wilreless fifo @ Interrupt,关闭API需应用层重构
 * id：1：from TxDevice1; 2:from TxDevice2
 * p：packet data buf
 * len：packet len,  len % PACKET_FRAME_LEN(AupackLen) == 0
 * RecvStatus:
 * data1 timeout  no lc_detach\n :time out for 720 tick
 * data2 timeout  no lc_detach\n
 * L full / R full :channel fifo overflow
 */
void AudioAssociationRec(uint16_t id, uint8_t *p,uint16_t len);

/**
 * 检测数据包是否已接收
 * return TURE：sync 通道有数据包；FALSE： 同步通道数据不足
 * note:需关注Dac节拍和fifo深度
 */
bool AudioAssocationReady(void);

/**
 * Wireless Audio Get one frame Data for Dac
 * PcmBuf：Stereo Audio Buf @STEREO_ONE_DEVICE Or Left channel buf @Others
 * 		   Size = frame_len * 2 * (Stereo ? 2:1)
 * PcmBuf1：NULL @STEREO_ONE_DEVICE Or Right channel buf @Others
 * 		   Size =  (STEREO_ONE_DEVICE ? (0) : (frame_len * 2)
 * return： samples; 0：no sync device; Frame: decode samples (frame_len)
 * note:log:
 * LE:  Left/Dev1 Packet Lost/Err from this Index
 * LD:	Left/Dev1 Packet Decode from this Index
 * RE:	Right/Dev2 Packet Lost/Err from this Index
 * RD:  Right/Dev2 Packet Decode from this Index
 * */
uint16_t AudioAssociationProcess(int16_t* PcmBuf, int16_t* PcmBuf2);

void AudioFadein(int16_t* pcm_in, uint16_t pcm_length, uint16_t ch);

void AudioFadeout(int16_t* pcm_in, uint16_t pcm_length, uint16_t ch);

const unsigned char *GetLibVersionAudioAssociation(void);

//音量淡入淡出
#define MixerFadeVolume(a, b, c)  	\
    if(a > b + c)		    \
	{						\
		a -= c;				\
	}						\
	else if(a + c < b)	   	\
	{						\
		a += c;				\
	}						\
	else					\
	{						\
		a = b;				\
	}

/***************ASS_DBG_EVT**********/
/*应用层实现下列API用于调试**/
void AssDbgRecvPacket_isr(uint8_t Device, uint8_t *Buf);//for ASS_DBG_RECV
void AssDbgPacketGet(uint8_t Device, uint8_t *Buf); 	//for ASS_DBG_ID_GET
void AssDbgPlayIndex(uint8_t Device);					//For ASS_DBG_DEC
void AssDbgMissGet(uint8_t Device);						//For ASS_DBG_ID_MISS


#endif /* AUDIO_ASSOCIATION_H_ */
