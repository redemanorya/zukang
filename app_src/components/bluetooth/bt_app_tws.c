/**
 **************************************************************************************
 * @file    bluetooth_tws.c
 * @brief   
 *
 * @author  KK
 * @version V1.0.0
 *
 * $Created: 2021-4-18 18:00:00$
 *
 * @Copyright (C) Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */



#include "type.h"
#include "debug.h"
#include "clk.h"
#include "bt_config.h"
#include "stdlib.h"
#include "bt_manager.h"
#include "bt_interface.h"
#include "bt_tws_api.h"
#include "mcu_circular_buf.h"
#include "app_config.h"
#include "bt_play_mode.h"
#include "main_task.h"
#include "audio_core_api.h"
#include "ctrlvars.h"
#include "bt_avrcp_api.h"
#include "dac_interface.h"
#include "adc_interface.h"
#include "sbcenc_api.h"
#include "uarts.h"
#include "dma.h"
#include "i2s.h"
#include "audio_adc.h"
#include "i2s_interface.h"
#include "irqn.h"
#include "audio_vol.h"
#include "bt_app_ddb_info.h"
#include "remind_sound.h"
#include "bt_app_connect.h"
#include "bt_app_tws_connect.h"
#include "bt_app_sniff.h"
#include "bt_app_tws.h"
#include "bt_tws_mode.h"
#include "main_task.h"
#include "rtos_api.h"
#include "media_play_mode.h"
#include "bt_stack_service.h"
#include "remind_sound.h"
#include "audio_core_service.h"
#include "bt_app_common.h"
#include "le_audio_config.h"
#if (BT_TWS_SUPPORT == ENABLE)

//tws cmd
typedef enum
{
	CMD_TWS_RESET = 0,
	CMD_TWS_SNIFF,
	CMD_TWS_REPAIR,
	CMD_TWS_VOL,
	CMD_TWS_MUTE,
	CMD_TWS_HFP_STATE,
	CMD_TWS_A2DP_STATE,
	CMD_TWS_EQ,
	CMD_TWS_MUSIC_TREB_BASS,
	CMD_TWS_REMIND,
	CMD_TWS_KEY_MSG,
	CMD_TWS_REMINDCLEAR_MSG,

	CMD_TWS_USER_MSG,
	CMD_TWS_MAX,
}TWS_CMD_TYPE;

const char * const TWS_CommonCmds[CMD_TWS_MAX] =
{
	"CMD:TWS_RESET",
	"CMD:TWS_SNIFF",
	"CMD:TWS_REPAIR",
	"CMD:TWS_VOL",
	"CMD:MUTE",
	"CMD:HFP_STATE",
	"CMD:A2DP_STATE",
	"CMD:TWS_EQ",
	"CMD:TWS_MUSIC_TREB_BASS",
	"CMD:REMIND",
	"CMD:TWS_KEY_MSG",
	"CMD:TWS_REMINDCLEAR_MSG",
	"CMD:CMD_TWS_USER_MSG",
};

typedef struct
{
	TWS_CMD_TYPE type;
	uint8_t		 para_len;
	uint8_t	*	 para_buf;
}TwsCmdPara;

#include "dac.h"
BT_TWS_PLAY tws_play = BT_TWS_PLAY_IDLE;
uint32_t tws_delay = BT_TWS_DELAY_48;//96;	//Tony20221109 for delay

static uint32_t error_count = 0;
#define TWS_CONNECT_MUTE		30
#define TWS_DISCONNECT_MUTE		50
TIMER TwsMuteTimer;
TIMER TwsDacOnTimer;
extern uint32_t gBtTwsSniffLinkLoss;
		
extern void AudioMusicVol(uint8_t musicVol);
extern void TwsSlaveFifoMuteTimeSet(void);
extern void TwsSlaveFifoUnmuteSet(void);
extern void mv_lmp_send(uint32_t cmd1,uint32_t cmd2,uint32_t cmd3);
extern void tws_device_close(void);

TWS_CMD_TYPE GetTwsRecCmd(uint8_t * cmd_string ,uint8_t *para_offset)
{
	uint8_t i;
	uint8_t len;
	for(i = 0;i < CMD_TWS_MAX; i++)
	{
		len = strlen(TWS_CommonCmds[i]);
		if(memcmp(cmd_string, TWS_CommonCmds[i],len) == 0)
		{
			*para_offset = len + 1;
			return i;
		}
	}
	return CMD_TWS_MAX;
}

void TwsSendCmd(TwsCmdPara *para)
{
	bool    ret;
	uint8_t *p;
	uint8_t len;
	uint8_t i;
	static uint8_t temp[BT_TWS_USER_CMD_MAX_LEN+30]; //常驻，TWS发送会用到这个buf
	// uint8_t *temp;

	extern uint8_t *TwsMem;
	if(TwsMem == NULL)
		return;

	if(para == NULL ||
	   para->type >= CMD_TWS_MAX ||
	   btManager.twsState < BT_TWS_STATE_CONNECTED )
		return;

	{
		//复用bis_src_sink_buffer,这个数组前面500字节已经复用在bt_tws->l2cap_buf上面
		//512之后复用在 gTwsSlavePlayCt
		// extern uint8_t bis_src_sink_buffer[1024];
		// temp = bis_src_sink_buffer + 512 + 256;
	}

	ret = GIE_STATE_GET();
	p  	= (uint8_t *)TWS_CommonCmds[para->type];
	len = strlen(TWS_CommonCmds[para->type]) + 1; //拷贝字符串结束符 '\0'

	GIE_DISABLE();

	memcpy(temp,p,len);
	for(i=0; i < para->para_len; i++)
	{
		temp[len++] = para->para_buf[i];
	}

	if(btManager.twsRole == BT_TWS_MASTER)
	{
		tws_master_send(temp, len);
	}
	else if(btManager.twsRole == BT_TWS_SLAVE)
	{
		tws_slave_send(temp, len);
	}

	if(ret)
	{
		GIE_ENABLE();
	}
}

/**************************************************************************
 *  tws cmd send
 *************************************************************************/
void tws_vol_send(uint16_t Vol,bool MuteFlag)
{
	TwsCmdPara para;
	uint8_t	   buf[2];

	if(!sys_parameter.TwsVolSyncEnable)
		return;

	buf[0] 			= Vol;
	buf[1] 			= MuteFlag;

	para.type 		= CMD_TWS_VOL;
	para.para_len   = 2;
	para.para_buf   = buf;
	TwsSendCmd(&para);
//	if(btManager.twsRole == BT_TWS_MASTER)
//	{
//		APP_DBG("master send cmd_vol_mute:%u %u\n",mainAppCt.MusicVolume,MuteFlag);
//	}
//	else if(btManager.twsRole == BT_TWS_SLAVE)
//	{
//		APP_DBG("slave send cmd_vol_mute:%u %u\n",mainAppCt.MusicVolume,MuteFlag);
//	}
}

void tws_user_cmd_send(uint8_t *data,uint8_t len)
{
	TwsCmdPara para;

	if(len > BT_TWS_USER_CMD_MAX_LEN) len = BT_TWS_USER_CMD_MAX_LEN;

	para.type 		= CMD_TWS_USER_MSG;
	para.para_len   = len;
	para.para_buf   = data;
	TwsSendCmd(&para);

	APP_DBG("tws_user_cmd_send %d\n", len);
}

void tws_send_key_msg(uint16_t key_data)
{
	TwsCmdPara para;
	uint8_t	   buf[2];

	buf[0] = (uint8_t)(key_data>>8);
	buf[1] = (uint8_t)(key_data&0xff);
	
	para.type 		= CMD_TWS_KEY_MSG;
	para.para_len   = 2;
	para.para_buf   = buf;
	TwsSendCmd(&para);

	APP_DBG("send key msg: 0x%x\n", key_data);
}

void tws_slave_tws_reset_send(void)
{
	TwsCmdPara para;
	if(btManager.twsRole != BT_TWS_SLAVE)
		return;

	para.type 		= CMD_TWS_RESET;
	para.para_len   = 0;
	para.para_buf   = NULL;
	TwsSendCmd(&para);

	APP_DBG("slave send cmd_reset!!!\n");
}

/**************************************************************************
 *  
 *************************************************************************/
void tws_master_vol_send(uint16_t Vol,bool MuteFlag)
{
	if(btManager.twsRole == BT_TWS_MASTER)
	{
		tws_vol_send(Vol,MuteFlag);
	}
}

/**************************************************************************
 *  
 *************************************************************************/
#if (BT_HFP_SUPPORT == ENABLE)
void tws_master_hfp_send(void)
{
	TwsCmdPara para;
	uint8_t	   buf[2];

	if(btManager.twsRole == BT_TWS_SLAVE)
		return;
	
	buf[0] = (uint8_t)GetHfpState(BtCurIndex_Get());

	para.type 		= CMD_TWS_HFP_STATE;
	para.para_len   = 1;
	para.para_buf   = buf;
	TwsSendCmd(&para);

	APP_DBG("master send cmd_hfp_state:%d\n", buf[0]);
}
#endif

/**************************************************************************
 *  
 *************************************************************************/
void tws_master_a2dp_send(void)
{
	TwsCmdPara para;
	uint8_t	   buf[2];

	if(btManager.twsRole == BT_TWS_SLAVE)
		return;

	buf[0] = (uint8_t)GetBtPlayState();

	para.type 		= CMD_TWS_A2DP_STATE;
	para.para_len   = 1;
	para.para_buf   = buf;
	TwsSendCmd(&para);

	APP_DBG("master send cmd_a2dp_state:%d\n", buf[0]);
}

/**************************************************************************
 *  
 *************************************************************************/
void tws_master_mute_send(bool MuteFlag)
{
	TwsCmdPara para;
	uint8_t	   buf[2];

	if(btManager.twsRole == BT_TWS_SLAVE)
		return;

	buf[0] = MuteFlag;

	para.type 		= CMD_TWS_MUTE;
	para.para_len   = 1;
	para.para_buf   = buf;
	TwsSendCmd(&para);
//	osTaskDelay(CFG_CMD_DELAY);
	APP_DBG("master send cmd_dac_mute: %u\n",MuteFlag);

}


void tws_master_remindclear_send(bool isclear)
{
	TwsCmdPara para;
	uint8_t	   buf[2];

	if(btManager.twsRole == BT_TWS_SLAVE)
		return;

	buf[0] = isclear;

	para.type 		= CMD_TWS_REMINDCLEAR_MSG;
	para.para_len   = 1;
	para.para_buf   = buf;
	TwsSendCmd(&para);
//	osTaskDelay(CFG_CMD_DELAY);
	APP_DBG("master send tts send: %d\n",isclear);

}

/**************************************************************************
 *  
 *************************************************************************/
#ifdef CFG_FUNC_MUSIC_EQ_EN	 
void tws_eq_mode_send(uint16_t eq_mode)
{
	TwsCmdPara para;
	uint8_t	   buf[2];

	buf[0] = eq_mode;

	para.type 		= CMD_TWS_EQ;
	para.para_len   = 1;
	para.para_buf   = buf;
	TwsSendCmd(&para);

	if(btManager.twsRole == BT_TWS_MASTER)
	{
		APP_DBG("master send cmd_eq_mode:%u\n",eq_mode);
	}
	else if(btManager.twsRole == BT_TWS_SLAVE)
	{
		APP_DBG("slave send cmd_eq_mode:%u\n",eq_mode);
	}
}

/**************************************************************************
 *  
 *************************************************************************/
void tws_master_eq_mode_send(uint16_t eq_mode)
{
	if(btManager.twsRole == BT_TWS_MASTER)
	{
		tws_eq_mode_send(eq_mode);
	}
}
#endif

/**************************************************************************
 *  
 *************************************************************************/
#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
void tws_music_bass_treb_send(uint16_t bass_vol,uint16_t treb_vol)
{
	TwsCmdPara para;
	uint8_t	   buf[2];

	buf[0] = bass_vol;
	buf[1] = treb_vol;

	para.type 		= CMD_TWS_MUSIC_TREB_BASS;
	para.para_len   = 2;
	para.para_buf   = buf;
	TwsSendCmd(&para);
	
	if(btManager.twsRole == BT_TWS_MASTER)
	{
		APP_DBG("master send cmd_music_bass_treb:%u %u\n",bass_vol,treb_vol);
	}
	else if(btManager.twsRole == BT_TWS_SLAVE)
	{
		APP_DBG("slave send cmd_music_bass_treb:%u %u\n",bass_vol,treb_vol);
	}
}

/**************************************************************************
 *  
 *************************************************************************/
void tws_master_music_bass_treb_send(uint16_t bass_vol,uint16_t treb_vol)
{
	if(btManager.twsRole == BT_TWS_MASTER)
	{
		tws_music_bass_treb_send(bass_vol,treb_vol);
	}
}
#endif

#ifdef CFG_FUNC_REMIND_SOUND_EN
void tws_master_remind_sound_send(char * remind,uint32_t IsBlock)
{
	TwsCmdPara para;
	uint8_t	   buf[10];

	if(btManager.twsRole != BT_TWS_MASTER)
		return;

	buf[0] = IsBlock;
	memcpy(&buf[1],remind,8);

	para.type 		= CMD_TWS_REMIND;
	para.para_len   = 9;
	para.para_buf   = buf;
	TwsSendCmd(&para);

	APP_DBG("tws_master_remind_sound_send: %s\n",remind);
}
#endif
/**************************************************************************
 *  
 *************************************************************************/
void tws_peer_repair(void)
{
	TwsCmdPara para;

	para.type 		= CMD_TWS_REPAIR;
	para.para_len   = 0;
	para.para_buf   = NULL;
	TwsSendCmd(&para);

	APP_DBG("send cmd_repair\n");
}

/**************************************************************************
 * tws master callback
 *************************************************************************/
void BtTws_Master_Connected(BT_TWS_CALLBACK_PARAMS * param)
{
	APP_DBG("TWS_MASTER_CONNECTED:\n");
	{
		MessageContext		msgSend;
		msgSend.msgId		= MSG_BT_TWS_MASTER_CONNECTED;
		MessageSend(GetMainMessageHandle(), &msgSend);
	}
	memcpy(btManager.btTwsDeviceAddr, param->params.bd_addr, param->paramsLen);
	APP_DBG("master connect: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x \n",
		btManager.btTwsDeviceAddr[0],
		btManager.btTwsDeviceAddr[1],
		btManager.btTwsDeviceAddr[2],
		btManager.btTwsDeviceAddr[3],
		btManager.btTwsDeviceAddr[4],
		btManager.btTwsDeviceAddr[5]
		);

	//改善延时的代码
	{
		extern void set_slaver_cmd(uint8_t* addr);
		set_slaver_cmd(param->params.bd_addr);
	}

	btManager.twsState = BT_TWS_STATE_CONNECTED;
	btManager.twsRole = BT_TWS_MASTER;
	BtDdb_UpgradeTwsInfor(btManager.btTwsDeviceAddr);
	btManager.twsFlag = 1;
	btManager.btReconnectedFlag = 1;

	if(btManager.btConStateProtectCnt)
		btManager.btConStateProtectCnt=0;

#ifdef TWS_AUDIO_BUFFER_DYNAMIC_MALLOC
	if(!tws_mem_check())
	{
		return ;
	}
#endif

	tws_effect_switch(1);//切换到tws 框图

	BtReconnectTwsStop();

#ifdef TWS_AUDIO_BUFFER_DYNAMIC_MALLOC
	TWS_Params_Init();
#endif
	extern void TwsAuidoSyncInit(void);
	TwsAuidoSyncInit();

//蓝牙可见性状态更新
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
	BtTwsExitPairingMode();

	BtStackServiceMsgSend(MSG_BTSTACK_ACCESS_MODE_SET);
	
#elif ((TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)||(TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER))
	BtTwsExitSimplePairingMode();

	BtStackServiceMsgSend(MSG_BTSTACK_ACCESS_MODE_SET);
#elif (TWS_PAIRING_MODE == CFG_TWS_BLE_PAIR)
	BtTwsExitPeerPairingMode();

	BtStackServiceMsgSend(MSG_BTSTACK_ACCESS_MODE_SET);
#else //CFG_TWS_PEER_SLAVE/CFG_TWS_PEER_MASTER
	BtTwsExitPeerPairingMode();

#ifdef CFG_REMID_SOUND_DECODEING_REUSE
	DecoderRemindPlayFlagSet(1);
#endif

	BtStackServiceMsgSend(MSG_BTSTACK_ACCESS_MODE_SET);

#endif

#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER && defined(BT_SNIFF_ENABLE))
	if(Bt_sniff_sniff_start_state_get())
	{
		//再次使从进入sniff
		//printf("send sniff cmd to slave again\n");
		BTSetRemDevIntoSniffMode(btManager.btTwsDeviceAddr);
		return;
	}
	else
	{
		tws_master_active_cmd();
	}
#endif
	
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)
	DisableAdvertising();
#endif

//	tws_master_vol_send(mainAppCt.MusicVolume, IsAudioPlayerMute());
	tws_vol_send(mainAppCt.MusicVolume, IsAudioPlayerMute());

	if(GetSystemMode() == ModeIdle)
	{
		printf("sleep disconnect tws\n");
		BtTwsDeviceDisconnectExt();
	}
#ifdef CFG_FUNC_MUSIC_EQ_EN
	tws_master_eq_mode_send(mainAppCt.EqMode);
#endif

#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
	tws_master_music_bass_treb_send(mainAppCt.MusicBassStep, mainAppCt.MusicTrebStep);
#endif
}

/**************************************************************************
 *  
 *************************************************************************/
void BtTws_Master_Disconnected(BT_TWS_CALLBACK_PARAMS * param)
{
	btManager.twsMode=0;
	APP_DBG("TWS_MASTER_DISCONNECT:\n");
	btManager.twsState = BT_TWS_STATE_NONE;
	tws_effect_switch(0);//切出tws框图
//蓝牙可见性配置
	BtStackServiceMsgSend(MSG_BTSTACK_ACCESS_MODE_SET);

#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)
	if(Bt_sniff_sniff_start_state_get() == 0)
		ble_advertisement_data_update();
	else
	{
		gBtTwsSniffLinkLoss = 1;
	}
#elif ((TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER)||(TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)||(TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)||(TWS_PAIRING_MODE == CFG_TWS_BLE_PAIR) )
	BtStackServiceMsgSend(MSG_BTSTACK_RECONNECT_REMOTE_PAGE_TIMEOUT);
#endif

	BtReconnectTwsStop();

	//TWS组网,发现对方LINKKEY不符合预期断开;则清除自身的TWS信息,再次发起组网;
	if(gBtTwsAppCt->btTwsPairingStart)
	{
		memset(&btManager.btLinkDeviceInfo[0], 0, sizeof(BT_LINK_DEVICE_INFO));
//		BtTwsPairingTimeout();
	}
}

/**************************************************************************
 *  
 *************************************************************************/
void BtTwsRecvCmdData(BT_TWS_CALLBACK_PARAMS * param)
{
	uint8_t para_offset;
	uint8_t ret = 0;
	if(param == NULL)
		return;
	if(param->role == BT_TWS_SLAVE && (param->params.twsData)[0] == 'd')
		return;

	switch(GetTwsRecCmd(param->params.twsData,&para_offset))
	{
	default:
		break;
	case CMD_TWS_RESET:
		APP_DBG("%s:cmd_reset\n\n",(param->role == BT_TWS_SLAVE)?"Slave":"Master");
#ifdef TWS_CODE_BACKUP
		g_tws_need_init = 1;
#endif
		break;
	case CMD_TWS_SNIFF:
#ifdef BT_SNIFF_ENABLE
		APP_DBG("%s:cmd_gotodeepsleep\n\n",(param->role == BT_TWS_SLAVE)?"Slave":"Master");
		send_sniff_msg();
#endif
		break;
	case CMD_TWS_REPAIR:
		APP_DBG("%s:cmd_repair\n\n",(param->role == BT_TWS_SLAVE)?"Slave":"Master");
		BtTwsPeerEnterPairingMode();
		break;
	case CMD_TWS_VOL:

		ret = GIE_STATE_GET();
		GIE_DISABLE();

		uint8_t MuteFlag =  param->params.twsData[para_offset + 1];
		uint8_t Volume = param->params.twsData[para_offset];

		if(Volume > AudioVol_GetMaxLevel()) Volume = AudioVol_GetMaxLevel();

		AudioMusicVolSet(Volume);

		if(ret)
		{
			GIE_ENABLE();
		}

	#if (BT_AVRCP_VOLUME_SYNC == ENABLE)
				//add volume sync(bluetooth play mode)
				if(GetSystemMode() == ModeBtAudioPlay)
				{
					extern void SetBtSyncVolume(uint8_t volume);
					SetBtSyncVolume(Volume);
					SendSysModeMsg(MSG_BT_PLAY_VOLUME_SET);
				}
	#endif
//
//		APP_DBG("%s rcv cmd_vol_mute:%u %u\n",(param->role == BT_TWS_SLAVE)?"Slave":"Master",
//				mainAppCt.gSysVol.AudioSourceVol[APP_SOURCE_NUM], mainAppCt.gSysVol.MuteFlag);
//		{
//			bool ret = GIE_STATE_GET();
//			GIE_DISABLE();
//			uint8_t MuteFlag 	  = param->params.twsData[para_offset + 1];
//			mainAppCt.MusicVolume = param->params.twsData[para_offset];
//			if(ret)
//			{
//				GIE_ENABLE();
//			}
//			APP_DBG("%s rcv cmd_vol_mute:%u %u\n",(param->role == BT_TWS_SLAVE)?"Slave":"Master",
//												   mainAppCt.MusicVolume, MuteFlag);
//			if(MuteFlag)
//			{
//				if(IsAudioPlayerMute() == FALSE)
//				{
//					HardWareMuteOrUnMute();
//				}
//				if(param->role == BT_TWS_SLAVE)
//				{
//					TwsSlaveFifoMuteTimeSet();
//				}
//			}
//			else
//			{
//				if(param->role == BT_TWS_SLAVE)
//				{
//					TwsSlaveFifoUnmuteSet();
//					AudioMusicVol(mainAppCt.MusicVolume);
//					SystemVolSet();
//					break;
//				}
//	#if (BT_AVRCP_VOLUME_SYNC == ENABLE)
//				//add volume sync(bluetooth play mode)
//				if(GetSystemMode() == ModeBtAudioPlay)
//				{
//					extern void SetBtSyncVolume(uint8_t volume);
//					SetBtSyncVolume(mainAppCt.MusicVolume);
//					SendSysModeMsg(MSG_BT_PLAY_VOLUME_SET);
//				}
//	#endif
//	#ifdef TWS_CODE_BACKUP
//	#ifdef CFG_RES_AUDIO_DAC0_EN
//				AudioDAC_DigitalMute(DAC0, FALSE, FALSE);
//	#endif
//
//	#ifdef CFG_RES_AUDIO_DACX_EN
//				AudioDAC_DigitalMute(DAC1, FALSE, FALSE);
//	#endif
//	#endif
//	#ifdef CFG_FUNC_REMIND_SOUND_EN
//				{
//					extern void master_remind_sound_mute_release(void);
//					master_remind_sound_mute_release();
//				}
//	#endif
//				APP_DBG("\n--Unmute\n");
//			}
//		}
		break;
	case CMD_TWS_MUTE:
		if(param->role == BT_TWS_SLAVE)
		{
			uint8_t MuteFlag 	  = param->params.twsData[para_offset];
			if(MuteFlag)
			{
				if(!IsAudioPlayerMute())
				{
					HardWareMuteOrUnMute();
				}
				TwsSlaveFifoMuteTimeSet();
			}
			else
			{
				TwsSlaveFifoUnmuteSet();
			}
		}
		break;
	case CMD_TWS_REMINDCLEAR_MSG:
		if(param->role == BT_TWS_SLAVE)
		{
			uint8_t isclear 	  = param->params.twsData[para_offset];
			if(isclear)
			{
#ifdef CFG_FUNC_REMIND_SOUND_EN
				 RemindSoundClearSlavePlay();
#endif
			}
			else
			{
				//nothing to do
			}
		}
		break;
#if (BT_HFP_SUPPORT == ENABLE)
	case CMD_TWS_HFP_STATE:
		if(param->role == BT_TWS_SLAVE)
		{
			bool ret = GIE_STATE_GET();
			GIE_DISABLE();
			uint8_t val = param->params.twsData[para_offset];
			if(ret)
			{
				GIE_ENABLE();
			}

			APP_DBG("slave rcv cmd_hfp_state = %d\n", val);
		}
		break;
#endif
	case CMD_TWS_A2DP_STATE:
		if(param->role == BT_TWS_SLAVE)
		{
			bool ret = GIE_STATE_GET();
			GIE_DISABLE();
			uint8_t val = param->params.twsData[para_offset];
			if(ret)
			{
				GIE_ENABLE();
			}

			APP_DBG("slave rcv cmd_a2dp_state = %d\n", val);
		}
		break;
#ifdef CFG_FUNC_MUSIC_EQ_EN
	case CMD_TWS_EQ:
		{
			bool ret = GIE_STATE_GET();
			GIE_DISABLE();
			mainAppCt.EqMode = param->params.twsData[para_offset];
			if(ret)
			{
				GIE_ENABLE();
			}
			APP_DBG("%s rcv cmd_eq_mode = %u\n", (param->role == BT_TWS_SLAVE)?"Slave":"Master",mainAppCt.EqMode);
		#ifndef CFG_FUNC_EQMODE_FADIN_FADOUT_EN
			//EqModeSet(mainAppCt.EqMode);
			AudioEffect_EqMode_Set(&EffectParambin, mainAppCt.EqMode);
		#endif
		}
		break;
#endif
#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
	case CMD_TWS_MUSIC_TREB_BASS:
		{
			bool ret = GIE_STATE_GET();
			GIE_DISABLE();
			mainAppCt.MusicBassStep = param->params.twsData[para_offset];
			mainAppCt.MusicTrebStep = param->params.twsData[para_offset + 1];
			if(ret)
			{
				GIE_ENABLE();
			}
			APP_DBG("%s rcv cmd_music_bass_treb = %u %u\n",	(param->role == BT_TWS_SLAVE)?"Slave":"Master",
															mainAppCt.MusicBassStep,mainAppCt.MusicTrebStep);
			//MusicBassTrebAjust(mainAppCt.MusicBassStep, mainAppCt.MusicTrebStep);
			AudioBassTrebAjust(&EffectParambin, EFFECT_MUSICEQ, PRESET_EQ_BASS_VOL);
			AudioBassTrebAjust(&EffectParambin, EFFECT_MUSICEQ, PRESET_EQ_TREB_VOL);
		}
		break;
#endif
#ifdef CFG_FUNC_REMIND_SOUND_EN
	case CMD_TWS_REMIND:
		{
			uint8_t buf[10];

			bool ret = GIE_STATE_GET();
			GIE_DISABLE();
			memcpy(&buf[0],&param->params.twsData[para_offset],9);
			buf[9] = 0;
			if(ret)
			{
				GIE_ENABLE();
			}

			APP_DBG("%s rcv cmd_remind_sound = %x %s\n", (param->role == BT_TWS_SLAVE)?"Slave":"Master",
														 buf[0],&buf[1]);
			RemindSoundServiceItemRequest((char *)(&buf[1]),(uint32_t)buf[0]);
		}
		break;
#endif
	case CMD_TWS_KEY_MSG:
		{
			MessageContext msgSend;

			bool ret = GIE_STATE_GET();
			GIE_DISABLE();

			msgSend.msgId = param->params.twsData[para_offset];
			msgSend.msgId <<= 8;
			msgSend.msgId |= param->params.twsData[para_offset + 1];

			if(ret)
			{
				GIE_ENABLE();
			}

			DBG("%s rev key msg: 0x%x\n",(param->role == BT_TWS_SLAVE)?"Slave":"Master",msgSend.msgId);
			if(GetSystemMode() == ModeBtHfPlay && param->role == BT_TWS_MASTER)
			{
				if(msgSend.msgId != MSG_POWERDOWN) //通话下响应从机关机命令
				break;
			}
			MessageSend(GetMainMessageHandle(), &msgSend);
		}
		break;

	case CMD_TWS_USER_MSG:
		printf("CMD_TWS_USER_MSG %d \n",param->paramsLen);
		uint8_t cnt = para_offset;
		for(;cnt<param->paramsLen;cnt++)
		{
			printf("0x%x,", param->params.twsData[cnt]);
		}
		break;
	}
}

/**************************************************************************
 * tws slave callback
 *************************************************************************/
void BtTws_Slave_Connected(BT_TWS_CALLBACK_PARAMS * param)
{
	APP_DBG("TWS_SLAVE_CONNECTED:\n");
	{
		MessageContext		msgSend;
		msgSend.msgId		= MSG_BT_TWS_SLAVE_CONNECTED;
		MessageSend(GetMainMessageHandle(), &msgSend);
	}
	memcpy(btManager.btTwsDeviceAddr, param->params.bd_addr, param->paramsLen);
	APP_DBG("addr: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x \n",
		btManager.btTwsDeviceAddr[0],
		btManager.btTwsDeviceAddr[1],
		btManager.btTwsDeviceAddr[2],
		btManager.btTwsDeviceAddr[3],
		btManager.btTwsDeviceAddr[4],
		btManager.btTwsDeviceAddr[5]
		);
	btManager.twsState = BT_TWS_STATE_CONNECTED;
	btManager.twsRole = BT_TWS_SLAVE;
	BtDdb_UpgradeTwsInfor(btManager.btTwsDeviceAddr);
	btManager.twsFlag = 1;
	btManager.btReconnectedFlag = 1;
	
	BtReconnectDevStop();
	BtReconnectTwsStop();
	
	if(btManager.btConStateProtectCnt)
		btManager.btConStateProtectCnt=0;
	
//蓝牙可见性配置
	//slave在tws组网成功后，进入不可被搜索不可被连接状态
//	BtSetAccessMode_NoDisc_NoCon();
	Vis_Set(BtAccessModeNotAccessible,__FUNCTION__);
	
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)
	tws_slave_simple_pairing_end();
#ifdef BT_SNIFF_ENABLE
	if(Bt_sniff_sniff_start_state_get() && (gBtTwsSniffLinkLoss==0))
		Bt_sniff_sniff_stop();
#endif
#endif

#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
	BtTwsExitPairingMode();
#elif ((TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)||(TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER))
	BtTwsExitSimplePairingMode();
#else //CFG_TWS_PEER_SLAVE/CFG_TWS_PEER_MASTER
	BtTwsExitPeerPairingMode();
#endif

	if(GetSystemMode() == ModeIdle)
	{
		printf("sleep disconnect tws\n");
		BtTwsDeviceDisconnectExt();
	}

#ifdef CFG_AUTO_ENTER_TWS_SLAVE_MODE
	TwsSlaveModeEnter();
#endif
}

/**************************************************************************
 *  
 *************************************************************************/
void BtTws_Slave_Disconnected(BT_TWS_CALLBACK_PARAMS * param)
{
	btManager.twsMode=0;
	APP_DBG("TWS_SLAVE_DISCONNECT:\n");
	btManager.twsState = BT_TWS_STATE_NONE;
	//tws_effect_switch(0);//dac sink api要快速更新
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)
#ifdef BT_SNIFF_ENABLE
	if(Bt_sniff_sniff_start_state_get())
	{
		//BleScanParamConfig_Sniff();
		BtTwsConnectApi();
		gBtTwsSniffLinkLoss = 1;
	}
	else
#endif
	if(!btManager.twsSbSlaveDisable)
	{
		tws_slave_simple_pairing_ready();
	}
#elif ((TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER)||(TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)||(TWS_PAIRING_MODE == CFG_TWS_BLE_PAIR))//||(TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)
	if(tws_reconnect_flag_get())
	{
		extern void BtTwsLinkLoss(void);
		APP_DBG("tws link loss, reconnect...\n");
		BtTwsLinkLoss();
	}
	
	BtStackServiceMsgSend(MSG_BTSTACK_RECONNECT_REMOTE_PAGE_TIMEOUT);
#elif (TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)
	TwsSlaveModeExit();
#endif

#ifdef CFG_AUTO_ENTER_TWS_SLAVE_MODE
	TwsSlaveModeExit();
#endif

	BtReconnectTwsStop();

}

/**************************************************************************
 *  tws audio
 *************************************************************************/
uint32_t tws_sync_instant = 0;
bool tws_state_audiocore_assort(TWS_SYNC_STATE State, TWS_SYNC_STATE LastState)
{
	bool ret = FALSE;

#ifdef TWS_AUDIO_BUFFER_DYNAMIC_MALLOC
	if(!TWS_TwsEnableGet())
	{
		return 0;
	}
#endif

	if(State == LastState )
	{
		if(IsTimeOut(&TwsMuteTimer))
		{
			ret = TRUE;
			if(State == TWS_CONNECT_JITTER && tws_get_role() == BT_TWS_MASTER)
			{
				if(btManager.twsState == BT_TWS_STATE_CONNECTED)
				{
					extern uint32_t tws_master_sync_instant_cal(uint8_t value);
					extern void tws_sync_alarm_set(uint32_t instant);
					
					tws_sync_instant = tws_master_sync_instant_cal(15);
					
					mv_lmp_send(TWS_CMD_DEVICE_OPEN, tws_sync_instant, 0);
					TimeOutSet(&TwsDacOnTimer, 200);
					btManager.TwsDacNeedTimeout = TRUE;

					tws_sync_alarm_set(tws_sync_instant);
				}
				else
				{
					tws_device_open_isr();
//					if(AudioCoreSinkIsInit(TWS_SINK_NUM))
//					{
//						AudioCoreSinkDepthChange(TWS_SINK_NUM, 2 * 512);
//					}
//					AudioCoreSourceUnmute(TWS_SOURCE_NUM, TRUE, TRUE);//for test
				}
			}
			else if(btManager.twsState == BT_TWS_STATE_NONE)
			{
				printf("TwsMuteTimer timeout, tws source channle unmute %d\n", State);
//				AudioCoreSourceUnmute(TWS_SOURCE_NUM, TRUE, TRUE);//for test
			}
		}
	}
	else
	{
		switch(State)
		{
			case TWS_CONNECT_JITTER:
				tws_device_close();//reset fifo;
			case TWS_DISCONNECT_JITTER:
//				tws_master_vol_send(mainAppCt.MusicVolume, IsAudioPlayerMute());
//				AudioCoreSourceMute(TWS_SOURCE_NUM, TRUE, TRUE);
				TimeOutSet(&TwsMuteTimer, TWS_CONNECT_MUTE);
				{
					printf("tws disconnect: %x-%x\n", State, LastState );
				}
				break;

			case TWS_HW_INIT:
//				if(AudioCoreSinkIsInit(TWS_SINK_NUM))
//				{
//					AudioCoreSinkDepthChange(TWS_SINK_NUM, TWS_STATRT_PLAY_FRAM * 128);
//				}
//				AudioCoreSourceUnmute(TWS_SOURCE_NUM, TRUE, TRUE);//for test
				break;

			case TWS_DISCONNECT:
//				if(AudioCoreSinkIsInit(TWS_SINK_NUM))
//				{
//					AudioCoreSinkDepthChange(TWS_SINK_NUM, 2 * 512);
//				}
//				AudioCoreSourceUnmute(TWS_SOURCE_NUM, TRUE, TRUE);//for test
				break;

			case TWS_AUDIO:
#ifdef CFG_FUNC_REMIND_SOUND_EN
				if(tws_get_role() == BT_TWS_MASTER && SoftFlagGet(SoftFlagTwsRemind))
				{
					RemindSoundServiceItemRequest(SOUND_REMIND_LEFTCHAN, REMIND_PRIO_PARTNER);//仅主机播放，从机mute播放，local audio识别。
					RemindSoundServiceItemRequest(SOUND_REMIND_RIGHTCHA, REMIND_PRIO_PARTNER | REMIND_ATTR_MUTE);//mute主机播放，local audio识别。
				}
				SoftFlagDeregister(SoftFlagTwsRemind);
#endif
				tws_dac_mute_flag_set(0);
				break;

			default:
				break;
		}
	}
	return ret;
}

/**************************************************************************
 *  
 *************************************************************************/
uint32_t tws_audio_adjust(uint32_t m_len,uint32_t s_len)
{
	int tt = m_len-s_len;
	printf("%d\n",tt);
	if(abs(tt) > 150)
	{
		error_count++;
		if(error_count > 5)
		{
			tws_slave_tws_reset_send();
		}
	}
	else
	{
		error_count = 0;;
	}
	return 1;
}


bool is_tws_slave(void)
{
	if((btManager.twsRole == BT_TWS_SLAVE)&&(btManager.twsState == BT_TWS_STATE_CONNECTED))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

bool BtTwsStatusGet(void)
{
#if !(BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT)
	return 1;
#endif
	if(btManager.twsState >= BT_TWS_STATE_CONNECTING || tws_link_state_get() >= BT_TWS_STATE_CONNECTING)
		return 1;
	else
		return 0;
}
#define TWS_BIS_SWITCH_TWS		0x01
#define TWS_BIS_SWITCH_BIS_SRC	0x02
#define TWS_BIS_SWITCH_BIS_SINK	0x04
#define TWS_BIS_SWITCH_BIS		0x08

#define TWS_BIS_SWITCH_DELAY_TIME	1000//2000//2s

uint32_t tws_bis_switch_delay_count = 0;
uint8_t tws_bis_switch_status = 0;

uint16_t tws_inquiry_delay_count = 0;
uint8_t tws_exit_bis_src_enter(void)
{
	if(BtTwsStatusGet())
	{
		BtTwsDeviceDisconnectExt();

		tws_bis_switch_status = TWS_BIS_SWITCH_BIS_SRC;
		return 0;
	}
	else
	{
		if(gBtTwsAppCt->btTwsPairingStart)
		{
			BtTwsExitPeerPairingMode();
		}

		return 1;
	}
}

uint8_t tws_exit_bis_sink_enter(void)
{
	if(BtTwsStatusGet())
	{
		BtTwsDeviceDisconnectExt();

		tws_bis_switch_status = TWS_BIS_SWITCH_BIS_SINK;

		return 0;
	}
	else
	{
		if(gBtTwsAppCt->btTwsPairingStart)
		{
			BtTwsExitPeerPairingMode();
		}

		return 1;
	}

}

uint8_t tws_exit_bis_enter(void)
{
	if(BtTwsStatusGet())
	{
		BtTwsDeviceDisconnectExt();

		tws_bis_switch_status = TWS_BIS_SWITCH_BIS;

		return 0;
	}
	else
	{
		if(gBtTwsAppCt->btTwsPairingStart)
		{
			BtTwsExitPeerPairingMode();
		}

		return 1;
	}

}

uint8_t bis_exit_tws_enter(void)
{
	MessageContext		msgSend;

	if((tws_bis_switch_status ==  TWS_BIS_SWITCH_BIS_SRC || tws_bis_switch_status ==  TWS_BIS_SWITCH_BIS_SINK)
	&& tws_bis_switch_delay_count)
	{
		return 0;//切换中不生效
	}

#if (BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT)
	if(Get_bis_role()!=BIS_NONE)
	{
		printf("bis_exit_tws_enter Get_bis_role() %d\n",Get_bis_role());
		if(Get_bis_role() == BIS_SRC)
			msgSend.msgId = MSG_LE_BIS_SRC_STOP;
		else if(Get_bis_role() == BIS_SINK)
			msgSend.msgId = MSG_LE_BIS_SNK_STOP;

		MessageSend(GetMainMessageHandle(), &msgSend);

		tws_bis_switch_status = TWS_BIS_SWITCH_TWS;

		return 0;
	}
	else
#endif
		return 1;
}

uint8_t tws_mem_check(void)
{
#if (BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT)
	if(Get_bis_role()!=BIS_NONE)
	{
		printf("#Get_bis_role %d\n",Get_bis_role());
		BtTwsDeviceDisconnectExt();
		return 0;
	}
#endif
	return 1;
}


void tws_bis_switch_process(void)
{
	MessageContext		msgSend;

	if(tws_bis_switch_status)
	{
		tws_bis_switch_delay_count++;
	}
	else
		tws_bis_switch_delay_count = 0;

	if(tws_bis_switch_delay_count>=TWS_BIS_SWITCH_DELAY_TIME)
	{
		tws_bis_switch_delay_count = 0;
		msgSend.msgId = MSG_NONE;

		switch(tws_bis_switch_status)
		{
			case TWS_BIS_SWITCH_TWS:
				msgSend.msgId = MSG_BT_TWS_PAIRING;
				break;
			case TWS_BIS_SWITCH_BIS_SRC:
				msgSend.msgId = MSG_LE_BIS_SRC_START;
				break;
			case TWS_BIS_SWITCH_BIS_SINK:
				msgSend.msgId = MSG_LE_BIS_SNK_START;
				break;

			case TWS_BIS_SWITCH_BIS:
				msgSend.msgId = MSG_LE_BIS;
				break;
		}
		tws_bis_switch_status = 0;

		MessageSend(GetMainMessageHandle(), &msgSend);
	}

	if(tws_inquiry_delay_count)
	{
		tws_inquiry_delay_count--;
		if(!tws_inquiry_delay_count)
		{
			CheckBtTwsPairing();
		}
	}
}

void tws_inquiry_delay(void)//底层调用，连接蓝牙时不触发inquiry，延时调用
{
	tws_inquiry_delay_count = 5100;//5.1s
}

#else

bool tws_state_audiocore_assort(TWS_SYNC_STATE State, TWS_SYNC_STATE LastState)
{
	return 0;
}

bool is_tws_slave(void)
{
	return 0;
}

#endif

