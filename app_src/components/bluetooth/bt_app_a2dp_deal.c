/**
 **************************************************************************************
 * @file    bluetooth_a2dp_deal.c
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
#include "debug.h"
#include "app_config.h"
#include "bt_config.h"
#include "bt_interface.h"
#include "bt_app_a2dp_deal.h"
#include "bt_manager.h"
#include "main_task.h"
#include "bt_app_common.h"
#include "bt_app_connect.h"
#include "bt_play_api.h"
#include "bt_stack_service.h"
#include "remind_sound.h"
#include "audio_vol.h"
#include "mode_task.h"
#include "bt_play_mode.h"
#include "bt_app_ddb_info.h"

#if (BT_SUPPORT == ENABLE)

#if (BT_TWS_SUPPORT == DISABLE)
extern uint32_t gSpecificDevice;
#endif

#if (BT_LINK_DEV_NUM == 2)
static uint8_t sPlayingIndex = 0xff;
#endif

/*****************************************************************************************
* A2DP连接成功
****************************************************************************************/
void BtA2dpConnectedDev(BT_A2DP_CALLBACK_PARAMS * param)
{
	uint8_t ConnectIndex = param->index;
	if(ConnectIndex >= BT_LINK_DEV_NUM)
	{
		APP_DBG("a2dp connected index = %d error\n", ConnectIndex);
		return;
	}
	
#if(BT_LINK_DEV_NUM == 2)
	if((btManager.btLinked_env[0].a2dpState == BT_A2DP_STATE_STREAMING)
		|| (btManager.btLinked_env[1].a2dpState == BT_A2DP_STATE_STREAMING)
	  )
	{
		//第1个手机播放音乐,第2个手机连接,只更新连接状态;
		if(btManager.btLinked_env[ConnectIndex].a2dp_index >= BT_LINK_DEV_NUM)//没有被占用
			btManager.btLinked_env[ConnectIndex].a2dp_index = param->index;
		SetA2dpState(ConnectIndex, BT_A2DP_STATE_CONNECTED);
		SetBtConnectedProfile(ConnectIndex, BT_CONNECTED_A2DP_FLAG);
		if(!btManager.btReconnectTimer.timerFlag)
		{
			//Remote Device主动连接BP10,A2DP连接成功,AVRCP未连接上,主动发起1次avrcp连接
			if(GetAvrcpState(ConnectIndex) < BT_AVRCP_STATE_CONNECTED)
			{
				//注册延时连接AVRCP
				BtStack_BtAvrcpConRegister(param->index);
			}
		}
		BtLinkStateConnect(0, ConnectIndex);
		BtStackServiceMsgSend(MSG_BTSTACK_RECONNECT_REMOTE_PROFILE);
		return; //有手机在播放了 不能初始化解码器防止出现断音
	}
	else
#endif
	{
		a2dp_sbc_decoder_init();
	}

#if (BT_TWS_SUPPORT == ENABLE)
	if((btManager.twsState == BT_TWS_STATE_CONNECTED)&&(btManager.twsRole == BT_TWS_SLAVE))
	{
		BTHciDisconnectCmd(param->params.bd_addr);
		return;
	}
#endif

	APP_DBG("param->index:%d,A2dp Connected : bt address = %02x:%02x:%02x:%02x:%02x:%02x\n",param->index,
			(param->params.bd_addr)[0],
			(param->params.bd_addr)[1],
			(param->params.bd_addr)[2],
			(param->params.bd_addr)[3],
			(param->params.bd_addr)[4],
			(param->params.bd_addr)[5]);

#if (BT_LINK_DEV_NUM == 2)
	if(param->index >= BT_LINK_DEV_NUM)
	{
		APP_DBG("a2dp link full\n");
		BTHciDisconnectCmd(param->params.bd_addr);//连接出现异常直接断开蓝牙
		if(btManager.btLinked_env[0].btLinkState)
		{
			btManager.btLinked_env[1].avrcpState = BT_AVRCP_STATE_NONE;
			btManager.btLinked_env[1].a2dpState = BT_A2DP_STATE_NONE;
			btManager.btLinked_env[1].hfpState = BT_HFP_STATE_NONE;
			btManager.btLinked_env[1].btLinkState = 0;
			APP_DBG("BtLink[1] DisConnect\n");
		}
		else
		{
			btManager.btLinked_env[0].avrcpState = BT_AVRCP_STATE_NONE;
			btManager.btLinked_env[0].a2dpState = BT_A2DP_STATE_NONE;
			btManager.btLinked_env[0].hfpState = BT_HFP_STATE_NONE;
			btManager.btLinked_env[0].btLinkState = 0;
			APP_DBG("BtLink[0] DisConnect\n");
		}
		return;
	}
	else
	{
		if((btManager.btLinked_env[param->index].btLinkedProfile & BT_CONNECTED_A2DP_FLAG)
				&& (btManager.btLinked_env[param->index].btLinkedProfile & BT_CONNECTED_AVRCP_FLAG)
		#if(BT_HFP_SUPPORT)
				&& (btManager.btLinked_env[param->index].btLinkedProfile & BT_CONNECTED_HFP_FLAG)
		#endif
			)
		{
			APP_DBG("a2dp link error \n");
			return;
		}

		ConnectIndex = param->index;
	}
#endif

	if((param->params.bd_addr)[0] || (param->params.bd_addr)[1] || (param->params.bd_addr)[2]
	|| (param->params.bd_addr)[3] || (param->params.bd_addr)[4] || (param->params.bd_addr)[5])
	{
		memcpy(GetBtManager()->remoteAddr, param->params.bd_addr, 6);
		memcpy(GetBtManager()->btLinked_env[ConnectIndex].remoteAddr, param->params.bd_addr, 6);
	}
	
	if(btManager.btLinked_env[ConnectIndex].a2dp_index >= BT_LINK_DEV_NUM)//没有被占用
		btManager.btLinked_env[ConnectIndex].a2dp_index = param->index;
	SetA2dpState(ConnectIndex, BT_A2DP_STATE_CONNECTED);

	SetBtConnectedProfile(ConnectIndex, BT_CONNECTED_A2DP_FLAG);

	if(!btManager.btReconnectTimer.timerFlag)
	{
		//Remote Device主动连接BP10,A2DP连接成功,AVRCP未连接上,主动发起1次avrcp连接
		if(GetAvrcpState(ConnectIndex) < BT_AVRCP_STATE_CONNECTED)
		{
			//注册延时主动连接AVRCP
			BtStack_BtAvrcpConRegister(param->index);
		}
	}

	//还未连接手机,更新index
	if(btManager.linkedNumber == 0)
	{
		//btManager.cur_index = param->index;
		BtCurIndex_Set(ConnectIndex);
	}

	BtLinkStateConnect(0, ConnectIndex);

	BtStackServiceMsgSend(MSG_BTSTACK_RECONNECT_REMOTE_PROFILE);
	
}


/*****************************************************************************************
* A2DP断开连接
****************************************************************************************/
void BtA2dpDisconnectedDev(BT_A2DP_CALLBACK_PARAMS * param)
{
	APP_DBG("A2dp disconnect,index:%d\n",param->index);
	uint8_t index = GetBtManagerA2dpIndex(param->index);
	if(index >= BT_LINK_DEV_NUM)
	{
		APP_DBG("a2dp disconnected index = %d error\n", index);
		return;
	}
	//SetA2dpState(param->index, BT_A2DP_STATE_NONE);
	//SetBtDisconnectProfile(BT_CONNECTED_A2DP_FLAG);
	//重新更新蓝牙decoder相关参数
//	if(RefreshSbcDecoder)
//		RefreshSbcDecoder();

	if(index < BT_LINK_DEV_NUM)
	//if((param->params.bd_addr!=NULL)&&(memcmp(param->params.bd_addr,btManager.btLinked_env[i].remoteAddr,BT_ADDR_SIZE) == 0))
	{
		SetA2dpState(index, BT_A2DP_STATE_NONE);
		SetBtDisconnectProfile(index,BT_CONNECTED_A2DP_FLAG);
		BtLinkStateDisconnect(index);
	}
	
	//回连流程,HFP已经连接成功,A2DP被拒绝,再发起一次连接,如还是被拒绝,则认为系统连接成功
	if((BtReconnectDevIsExcute()))//remote terminated disconnect
	{
		uint8_t ConnectIndex = 0;
		for(ConnectIndex = 0; ConnectIndex < BT_LINK_DEV_NUM; ConnectIndex++)
		{
			if(memcmp(btManager.btReconPhoneSt.RemoteDevAddr,btManager.btLinked_env[ConnectIndex].remoteAddr,BT_ADDR_SIZE) == 0)
			{
				break;
			}
		}
#if (defined(CFG_APP_BT_MODE_EN) && (BT_HFP_SUPPORT))
		if((GetHfpState(ConnectIndex) >= BT_HFP_STATE_CONNECTED))
		{
			if(btManager.btReconExcuteSt->TryCount)
			{
				//延时1s再次尝试;
				btManager.btReconExcuteSt->TryCount = 0;
				BtReconnectDevAgain(1000);//延时1s再次发起连接
				APP_DBG("============ delay 1000ms, reconnect again\n");
			}
			else
			{
				BtReconnectDevStop();
				BtLinkStateConnect(1, param->index);
				APP_DBG("=============hfp cannot connect, finished..\n");
			}
		}
#endif
	}
	

	//A2DP断开后，开启检测AVRCP断开机制(3S超时)
	/*if(IsAvrcpConnected())
	{
		btEventListB0Count = btEventListCount;
		btEventListB0Count += 5000;//延时5s
		btCheckEventList |= BT_EVENT_AVRCP_DISCONNECT;
	}*/
	BtStack_BtAvrcpDisconRegister(param->index);
	
	SetA2dpState(param->index,BT_A2DP_STATE_NONE);
}

/*****************************************************************************************
* A2DP连接超时处理
****************************************************************************************/
void BtA2dpConnectTimeout(BT_A2DP_CALLBACK_PARAMS * param)
{
	APP_DBG("A2dp connect timeout,index:%d\n",param->index);
	uint8_t index = GetBtManagerA2dpIndex(param->index);
	if(index >= BT_LINK_DEV_NUM)
	{
		APP_DBG("a2dp connect timeout index = %d error\n", index);
		return;
	}

	if(index < BT_LINK_DEV_NUM && GetA2dpState(index) > BT_A2DP_STATE_NONE)
	{
		SetA2dpState(index, BT_A2DP_STATE_NONE);
		SetBtDisconnectProfile(index,BT_CONNECTED_A2DP_FLAG);

		//重新更新蓝牙decoder相关参数
		a2dp_sbc_decoder_init();

		BtLinkStateDisconnect(index);

		//A2DP断开后，开启检测AVRCP断开机制(5S超时)
		/*if(IsAvrcpConnected())
		{
			btEventListB0Count = btEventListCount;
			btEventListB0Count += 5000;//延时5s
			btCheckEventList |= BT_EVENT_AVRCP_DISCONNECT;
		}*/
	}
	SetA2dpState(param->index,BT_A2DP_STATE_NONE);
}

/*****************************************************************************************
* A2DP Stream 开始播放
****************************************************************************************/
extern uint32_t		a2dp_unmute_delay_cnt;
uint32_t a2dp_pause_delay_cnt = 0; //delay pause
void BtA2dpStreamStart(BT_A2DP_CALLBACK_PARAMS * param)
{
	APP_DBG("A2dp streaming %d...\n", param->index);

#if (BT_LINK_DEV_NUM == 2)
	uint8_t index = GetBtManagerA2dpIndex(param->index);
	if(index >= BT_LINK_DEV_NUM)
	{
		APP_DBG("a2dp streaming index = %d error\n", index);
		return;
	}

	if(GetSystemMode() == ModeBtHfPlay && (index < BT_LINK_DEV_NUM))//通话模式中 其他手机正在播放 挂断后跳转到正在播放的手机
	{
		if(btManager.cur_index != param->index)
		{
			if(btManager.HfpCurIndex != 0xff)
			{
				APP_DBG("talking now,arvcp pause index %d\n",index);
				//AvrcpCtrlPause(index);
				a2dp_pause_delay_cnt = 500;
			}
		}
		
		//APP_DBG("ModeBtHfPlay param->index[%d]->>>>> BT_A2DP_STATE_STREAMING %d \n",index);
		SetA2dpState(index, BT_A2DP_STATE_STREAMING);
		return ;
	}
#endif
#if (BT_HFP_SUPPORT)
	if(GetSystemMode() != ModeBtAudioPlay)
	{
		if(GetSystemMode() == ModeBtHfPlay)
		{
			extern bool GetDelayExitBtHfMode(void);
			APP_DBG("--A2dp streaming unusual set status is %d,%d\n", GetSysModeState(ModeBtHfPlay),GetDelayExitBtHfMode());

			if((GetSysModeState(ModeBtHfPlay) != ModeStateSusend) && !GetDelayExitBtHfMode())
			{
				return; // 退出BTCALL之后先将MODE挂起但是此时还没有进入BTAUDIO
			}
		}else{
			return;
		}
	}
#endif
	if(IsAudioPlayerMute() == FALSE
#ifdef CFG_FUNC_REMIND_SOUND_EN
		&& RemindSoundIsPlay() <= 1
#endif
	)
	{
#if(BT_LINK_DEV_NUM == 2)
	if((btManager.btLinked_env[0].a2dpState == BT_A2DP_STATE_STREAMING)
		|| (btManager.btLinked_env[1].a2dpState == BT_A2DP_STATE_STREAMING)
	  )
		; //有手机在播放了 不能mute防止出现断音
	else
#endif
		{
//			HardWareMuteOrUnMute();
			a2dp_unmute_delay_cnt = 0;
		}
	}

#if (BT_LINK_DEV_NUM == 2)
	if(index < BT_LINK_DEV_NUM)
	{
		SetA2dpState(index, BT_A2DP_STATE_STREAMING);
	}

//#if (BT_LINK_DEV_NUM == 2)
	//if(FirstTalkingPhoneIndexGet()!=0xff)
	if(btManager.HfpCurIndex != 0xff)
	{
		APP_DBG("talking now,arvcp pause index %d\n",index);
		AvrcpCtrlPause(index);
		return;
	}
//#endif

	APP_DBG("A2dp streaming...cur %d, index %d, ,param->index %d\n",btManager.cur_index,index,param->index);


	if(btManager.cur_index == index)
	{
		extern uint32_t AvrcpStateSuspendCount;
		AvrcpStateSuspendCount = 0;
	}
	else
	{
		//if(btManager.cur_index)
		//APP_DBG("111\n");
#ifdef LAST_PLAY_PRIORITY
		extern uint32_t AvrcpStateSuspendCount;
		AvrcpStateSuspendCount++;
#endif


		if(GetA2dpState(btManager.cur_index) != BT_A2DP_STATE_STREAMING)
		{
			//APP_DBG("222\n");
			//btManager.cur_index = param->index;
			BtCurIndex_Set(index);
			BT_DBG("channel[%d]:start playing\n", btManager.cur_index);
		}
		else
		{
			return;
		}
	}
#endif

//#if(BT_LINK_DEV_NUM == 2)
	//if(GetAvrcpState() != BT_AVRCP_STATE_CONNECTED)
		BtMidMessageSend(MSG_BT_MID_PLAY_STATE_CHANGE, 1);
//#endif


#if (BT_TWS_SUPPORT == DISABLE)
#if (BT_HFP_SUPPORT)
	extern uint32_t gSpecificDevice;
	if(gSpecificDevice)
	{
		extern void SpecialDeviceFunc(uint8_t index);
		SpecialDeviceFunc(param->index);
	}
#endif
#endif

#if(BT_LINK_DEV_NUM == 2)
	if((btManager.btLinked_env[0].a2dpState == BT_A2DP_STATE_STREAMING)
		|| (btManager.btLinked_env[1].a2dpState == BT_A2DP_STATE_STREAMING)
	  )
		; //有手机在播放了 不能初始化解码器防止出现断音
	else
#endif
	a2dp_sbc_decoder_init();
	SetA2dpState(param->index,BT_A2DP_STATE_STREAMING);
}

/*****************************************************************************************
* A2DP Stream 播放暂停
****************************************************************************************/
void BtA2dpStreamSuspend(BT_A2DP_CALLBACK_PARAMS * param)
{
	uint8_t index = GetBtManagerA2dpIndex(param->index);
	if(index >= BT_LINK_DEV_NUM)
	{
		APP_DBG("a2dp suspend index = %d error\n", index);
		return;
	}

	APP_DBG("%d,%d,A2dp suspend\n", param->index, index);
	if(GetSystemMode() == ModeBtHfPlay && index < BT_LINK_DEV_NUM)//通话时 其他手机的暂停状态更新
	{
		APP_DBG("ModeBtHfPlay param->index[%d]->>>>> BT_A2DP_STATE_CONNECTED \n",index);
		SetA2dpState(index, BT_A2DP_STATE_CONNECTED);
		return ;
	}
	if(GetSystemMode() != ModeBtAudioPlay)
		return;

#if (BT_LINK_DEV_NUM == 2)
	if( (btManager.btLinked_env[0].btLinkState == 1 && btManager.btLinked_env[1].btLinkState == 0)
	   || (btManager.btLinked_env[0].btLinkState == 0 && btManager.btLinked_env[1].btLinkState == 1) )//防止回连错乱的现象
	{
		sPlayingIndex = 0xff;
	}
#endif

	if(index < BT_LINK_DEV_NUM)
		SetA2dpState(index, BT_A2DP_STATE_CONNECTED);

	if(IsAudioPlayerMute() == FALSE
#ifdef CFG_FUNC_REMIND_SOUND_EN
		&& RemindSoundIsPlay() <= 1
#endif
	)
	{
#if (BT_LINK_DEV_NUM == 2)
		if( (GetA2dpState(0) != BT_A2DP_STATE_STREAMING) && (GetA2dpState(1) != BT_A2DP_STATE_STREAMING) )//防止抢播可能会出现的断音
#endif
		{
//			HardWareMuteOrUnMute();
			a2dp_unmute_delay_cnt = 0;
		}
	}

#if (BT_TWS_SUPPORT == ENABLE)
	if(IsBtAudioMode())
	{
#if (BT_LINK_DEV_NUM == 2)
		if( GetA2dpState(0) != BT_A2DP_STATE_STREAMING && GetA2dpState(1) != BT_A2DP_STATE_STREAMING )//防止抢播可能会出现的断音
#endif
			AudioCoreSourceDisable(1);
	}
#endif

	if(btManager.cur_index != index)
		return;
	
	{
		//a2dp pause后 500ms进行切换
		extern uint32_t AvrcpStateSuspendCount;
		if(AvrcpStateSuspendCount<22)	//延时220ms,防止出现抢播现象,22只是一个经验值,需要根据实际情况调整
			AvrcpStateSuspendCount = 22;
	}

	BtMidMessageSend(MSG_BT_MID_PLAY_STATE_CHANGE, 2);
	SetA2dpState(param->index,BT_A2DP_STATE_CONNECTED);
}

/*****************************************************************************************
* A2DP Stream 接收到媒体数据
****************************************************************************************/
void BtA2dpStreamDataRecevied(BT_A2DP_CALLBACK_PARAMS * param)
{
#if (BT_LINK_DEV_NUM == 2)
	uint8_t index = GetBtManagerA2dpIndex(param->index);
	if(index >= BT_LINK_DEV_NUM)
	{
		APP_DBG("a2dp data receive index = %d error\n", index);
		return;
	}
#endif

#if (BT_LINK_DEV_NUM == 2)
	if (btManager.cur_index >= BT_LINK_DEV_NUM)
		return;
	if(btManager.cur_index != index)
		return;

	if(GetSystemMode() == ModeBtHfPlay && index < BT_LINK_DEV_NUM)//通话可能会有音乐播放 需要加此条件
	{
		AvrcpCtrlPause(index);
	}
	//播放数据源有更换
	if((btManager.btLinked_env[btManager.cur_index].btLinkState == 1)&&(sPlayingIndex != btManager.cur_index)&&(btManager.btDisConnectingFlag == 0)/*&&(GetSystemMode() == AppModeBtAudioPlay)???*/)
	{
		sPlayingIndex = btManager.cur_index;
		APP_DBG("playing index:%d\n",sPlayingIndex);
		btManager.btDdbLastProfile = btManager.btLinked_env[sPlayingIndex].btLinkedProfile;
		//SpiFlashEraseWpr(SECTOR_ERASE, (BTDB_ALIVE_RECORD_ADDR) /4096 , 1);
		//BtDdb_UpgradeLastBtAddr(GetBtManager()->btLinked_env[sPlayingIndex].remoteAddr, btManager.btDdbLastProfile);	//cancel the recording of playback devices
		//BtDdb_UpgradeLastBtProfile(GetBtManager()->btLinked_env[sPlayingIndex].remoteAddr, btManager.btDdbLastProfile);
		if(GetBtPlayState() != BT_PLAYER_STATE_PLAYING)
		{
			SetBtPlayState(BT_PLAYER_STATE_PLAYING);
		}
		a2dp_sbc_decoder_init();

#if(BT_AVRCP_SONG_TRACK_INFOR == ENABLE)
		BTCtrlGetMediaInfor(BtCurIndex_Get());	//主动获取歌曲信息,用于显示歌曲信息
#endif
	}
#endif

	a2dp_sbc_save(param->params.a2dpStreamParams.a2dpStreamData,param->params.a2dpStreamParams.a2dpStreamDataLen);
}

/*****************************************************************************************
* A2DP Stream 数据格式
****************************************************************************************/
void BtA2dpStreamDataType(BT_A2DP_CALLBACK_PARAMS * param)
{
	APP_DBG("A2dp stream %d type:",param->index);
	if(param->index < BT_LINK_DEV_NUM)
	{
		if(param->params.a2dpStreamDataType)
		{
			APP_DBG("AAC\n");
			btManager.a2dpStreamType[param->index] = BT_A2DP_STREAM_TYPE_AAC;
		}
		else
		{
			APP_DBG("SBC\n");
			btManager.a2dpStreamType[param->index] = BT_A2DP_STREAM_TYPE_SBC;
		}
		if(btManager.cur_index == GetBtManagerA2dpIndex(param->index))
			a2dp_sbc_decoder_init();
	}
}

#endif

