/**
 **************************************************************************************
 * @file    bluetooth_avrcp_deal.c
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
#include "bt_app_avrcp_deal.h"
#include "bt_manager.h"
#include "bt_interface.h"
#include "audio_vol.h"
#include "bt_app_common.h"
#include "app_message.h"
#include "bt_stack_service.h"
#include "mode_task.h"
#include "main_task.h"

#if (BT_SUPPORT == ENABLE)

#if (BT_AVRCP_SONG_TRACK_INFOR)
#define StringMaxLen 60
#include "string_convert.h"
#endif

extern uint32_t AvrcpStateSuspendCount;
extern uint32_t gSpecificDevice;
#if (BT_LINK_DEV_NUM == 2)
AvrcpAdvMediaStatus sPlayStatus[BT_LINK_DEV_NUM]={AVRCP_ADV_MEDIA_STOPPED,AVRCP_ADV_MEDIA_STOPPED};
AvrcpAdvMediaStatus sPlayStatusBk[BT_LINK_DEV_NUM]={AVRCP_ADV_MEDIA_STOPPED,AVRCP_ADV_MEDIA_STOPPED};
#else //(BT_LINK_DEV_NUM == 1)
AvrcpAdvMediaStatus sPlayStatus[BT_LINK_DEV_NUM]={AVRCP_ADV_MEDIA_STOPPED};
AvrcpAdvMediaStatus sPlayStatusBk[BT_LINK_DEV_NUM]={AVRCP_ADV_MEDIA_STOPPED};
#endif
/*****************************************************************************************
* AVRCP连接成功
****************************************************************************************/
void BtAvrcpConnectedDev(BT_AVRCP_CALLBACK_PARAMS * param)
{
	uint8_t ConnectIndex = 0;
	APP_DBG("param->index:%d, Avrcp Connected : bt address = %02x:%02x:%02x:%02x:%02x:%02x\n", param->index,
			(param->params.bd_addr)[0],
			(param->params.bd_addr)[1],
			(param->params.bd_addr)[2],
			(param->params.bd_addr)[3],
			(param->params.bd_addr)[4],
			(param->params.bd_addr)[5]);
	
#if (BT_LINK_DEV_NUM == 2)
#if 1
	if(param->index >= BT_LINK_DEV_NUM)
	{
		APP_DBG("avrcp link full\n");
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
			APP_DBG("avrcp link error \n");
			return;
		}

		ConnectIndex = param->index;
	}
#else
	for(ConnectIndex = 0; ConnectIndex < BT_LINK_DEV_NUM; ConnectIndex++)
	{
		if(memcmp(param->params.bd_addr,btManager.btLinked_env[ConnectIndex].remoteAddr,BT_ADDR_SIZE) == 0)
		{
			break;
		}
	}
	if(ConnectIndex >= BT_LINK_DEV_NUM)
	{
		for(ConnectIndex = 0; ConnectIndex < BT_LINK_DEV_NUM; ConnectIndex++)
		{
			if(!(btManager.btLinked_env[ConnectIndex].btLinkedProfile & BT_CONNECTED_A2DP_FLAG)
					&& !(btManager.btLinked_env[ConnectIndex].btLinkedProfile & BT_CONNECTED_AVRCP_FLAG)
			#if(BT_HFP_SUPPORT)
					&& !(btManager.btLinked_env[ConnectIndex].btLinkedProfile & BT_CONNECTED_HFP_FLAG)
			#endif
				)
			{
				break;
			}
		}
	}
			
	if(ConnectIndex >= BT_LINK_DEV_NUM)
	{
		APP_DBG("AVRCP link full\n");
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
#endif
#endif

	btManager.btLinked_env[ConnectIndex].avrcp_index = param->index;
	SetAvrcpState(ConnectIndex, BT_AVRCP_STATE_CONNECTED);
	sPlayStatus[ConnectIndex] = AVRCP_ADV_MEDIA_STOPPED;//刚连接成功,状态为stop

	if((param->params.bd_addr)[0] || (param->params.bd_addr)[1] || (param->params.bd_addr)[2]
		|| (param->params.bd_addr)[3] || (param->params.bd_addr)[4] || (param->params.bd_addr)[5])
	{
		memcpy(GetBtManager()->remoteAddr, param->params.bd_addr, 6);
		memcpy(GetBtManager()->btLinked_env[ConnectIndex].remoteAddr, param->params.bd_addr, 6);
	}


	SetBtConnectedProfile(ConnectIndex, BT_CONNECTED_AVRCP_FLAG);
	BtLinkStateConnect(0, ConnectIndex);

#if (BT_TWS_SUPPORT == ENABLE)
	if((btManager.twsState == BT_TWS_STATE_CONNECTED)&&(btManager.twsRole == BT_TWS_SLAVE))
	{
		//从机已经组网成功,此时被手机连接上,则断开手机
		BTHciDisconnectCmd(param->params.bd_addr);
		return;
	}
#endif

#if (BT_AUTO_PLAY_MUSIC)
	BtAutoPlayMusic();
#endif

#if (BT_AVRCP_VOLUME_SYNC)
	btManager.avrcpSyncEnable = 0;
	btManager.avrcpSyncVol = mainAppCt.gSysVol.AudioSourceVol[APP_SOURCE_NUM]; //初始值
	AvrcpAdvTargetSyncVolumeConfig(ConnectIndex,BtLocalVolLevel2AbsVolme(AudioMusicVolGet()));//蓝牙AVRCP连接成功后，将当前的音量值同步到AVRCP adv.volume默认值
#endif

	BtStackServiceMsgSend(MSG_BTSTACK_RECONNECT_REMOTE_PROFILE);
	
	BtEventFlagDeregister(BT_EVENT_FLAG_AVRCP_CONNECT);

//	if(btManager.linkedNumber == 0)
//	{
//		//btManager.cur_index = param->index;
//		BtCurIndex_Set(ConnectIndex);
//	}
}

/*****************************************************************************************
* AVRCP断开连接
****************************************************************************************/
void BtAvrcpDisconnectedDev(BT_AVRCP_CALLBACK_PARAMS * param)
{
	uint8_t index = GetBtManagerAvrcpIndex(param->index);
	APP_DBG("Avrcp disconnect,index %d\n",param->index);
	//SetAvrcpState(param->index, BT_AVRCP_STATE_NONE);

	if(index < BT_LINK_DEV_NUM)
	{
	//if((param->params.bd_addr!=NULL)&&(memcmp(param->params.bd_addr,btManager.btLinked_env[i].remoteAddr,BT_ADDR_SIZE) == 0))
		SetAvrcpState(index, BT_AVRCP_STATE_NONE);
		SetBtDisconnectProfile(index,BT_CONNECTED_AVRCP_FLAG);
		BtLinkStateDisconnect(index);
		sPlayStatus[index] = AVRCP_ADV_MEDIA_STOPPED;//断开,状态为stop
	}
	
	//AVRCP断开机制清除
	//BtEventFlagDeregister(BT_EVENT_FLAG_AVRCP_DISCONNECT);

	//AVRCP断开后,主动断开A2DP,会导致误触发行为,先关闭
	//BtEventFlagRegister(BT_EVENT_FLAG_A2DP_DISCONNECT, 1000);
}

/*****************************************************************************************
* AVRCP 播放状态改变处理流程
****************************************************************************************/
void BtAvrcpPlayStatusChanged(BT_AVRCP_CALLBACK_PARAMS * param)
{
//	extern uint32_t a2dp_avrcp_connect_flag;
	uint8_t index = GetBtManagerAvrcpIndex(param->index);
	APP_DBG("Remote AVRCP Play State [%d],index = %d\n", param->params.avrcpAdv.avrcpAdvMediaStatus,param->index);

	if(GetSystemMode() != ModeBtAudioPlay)
		return;

	if(index >= BT_LINK_DEV_NUM)
		return;

#if 0//(BT_HFP_SUPPORT)
	if((param->params.avrcpAdv.avrcpAdvMediaStatus == 1)&&(gSpecificDevice))
	{
		extern void SpecialDeviceFunc(uint8_t index);
		SpecialDeviceFunc(param->index);
	}
#endif

	sPlayStatus[index] = param->params.avrcpAdv.avrcpAdvMediaStatus;
	if(sPlayStatusBk[index] == sPlayStatus[index]) return;//某些手机会一直反馈状态，重复的状态通知需要过滤

	if(btManager.cur_index == index)
	{
		BtMidMessageSend(MSG_BT_MID_PLAY_STATE_CHANGE, param->params.avrcpAdv.avrcpAdvMediaStatus);

		if((sPlayStatusBk[index] == AVRCP_ADV_MEDIA_PLAYING)&&(sPlayStatus[index] != AVRCP_ADV_MEDIA_PLAYING))
		{
			AvrcpStateSuspendCount++;
		}
		else if(sPlayStatus[index] == AVRCP_ADV_MEDIA_PLAYING)
		{
			AvrcpStateSuspendCount = 0;
		}
	}
#ifdef LAST_PLAY_PRIORITY
	else if(param->params.avrcpAdv.avrcpAdvMediaStatus == AVRCP_ADV_MEDIA_PLAYING)
	{
		//AvrcpCtrlPause(btManager.btLinked_env[btManager.cur_index].avrcp_index);
		//BtCurIndex_Set(i);
		SetA2dpState(index, BT_A2DP_STATE_STREAMING);//防止抢占失败
		AvrcpStateSuspendCount++;
	}
	else if(param->params.avrcpAdv.avrcpAdvMediaStatus == AVRCP_ADV_MEDIA_PAUSED)
	{
		AvrcpStateSuspendCount = 0;
	}
#endif
	sPlayStatusBk[index] = sPlayStatus[index];


#if (BT_AUTO_PLAY_MUSIC)
	void BtAutoPlaySetAvrcpPlayStatus(uint8_t play_state);
	BtAutoPlaySetAvrcpPlayStatus(param->params.avrcpAdv.avrcpAdvMediaStatus);
#endif

	
#if (BT_HFP_SUPPORT)
	if((param->params.avrcpAdv.avrcpAdvMediaStatus == 1)&&(gSpecificDevice))
	{
		extern void SpecialDeviceFunc(void);
		SpecialDeviceFunc();
	}
#endif
	//bkd del for press key to play,but STREAMING command not received 
	{
		if(btManager.cur_index == index)//防止播放暂停状态错乱
			BtMidMessageSend(MSG_BT_MID_PLAY_STATE_CHANGE, param->params.avrcpAdv.avrcpAdvMediaStatus);
		
		if(CheckTimerStart_BtPlayStatus() == 0)
		{
			if(param->params.avrcpAdv.avrcpAdvMediaStatus == AVRCP_ADV_MEDIA_PLAYING)
			{
				BTCtrlGetPlayStatus(param->index);
				TimerStart_BtPlayStatus();
			}
		}
	}
}

/*****************************************************************************************
* AVRCP 播放状态反馈
****************************************************************************************/
void BtAvrcpPlayStatus(BT_AVRCP_CALLBACK_PARAMS * param)
{
	uint8_t curPlayStatus = param->params.avrcpAdv.avrcpAdvPlayStatus.CurPlayStatus;
	uint32_t curPlayTimes = param->params.avrcpAdv.avrcpAdvPlayStatus.CurPosInMs;
	uint32_t totalPlayTimes = param->params.avrcpAdv.avrcpAdvPlayStatus.TotalLengthInMs;

	uint8_t index = GetBtManagerAvrcpIndex(param->index);
	sPlayStatus[index] = curPlayStatus;
	
	switch(curPlayStatus)
	{
		case AVRCP_ADV_MEDIA_STOPPED:
			APP_DBG("Stopped \n");
			break;
			
		case AVRCP_ADV_MEDIA_PLAYING:
			APP_DBG("Playing > %02d:%02d / %02d:%02d \n",
					((int)curPlayTimes/1000)/60,
					((int)curPlayTimes/1000)%60,
					((int)totalPlayTimes/1000)/60,
					((int)totalPlayTimes/1000)%60);
			break;

		case AVRCP_ADV_MEDIA_PAUSED:
			APP_DBG("Paused || %02d:%02d / %02d:%02d  \n",
					((int)curPlayTimes/1000)/60,
					((int)curPlayTimes/1000)%60,
					((int)totalPlayTimes/1000)/60,
					((int)totalPlayTimes/1000)%60);
			break;

		case AVRCP_ADV_MEDIA_FWD_SEEK:
			APP_DBG("FF >> %02d:%02d / %02d:%02d  \n",
					((int)curPlayTimes/1000)/60,
					((int)curPlayTimes/1000)%60,
					((int)totalPlayTimes/1000)/60,
					((int)totalPlayTimes/1000)%60);
			break;

		case AVRCP_ADV_MEDIA_REV_SEEK:
			APP_DBG("FB << %02d:%02d / %02d:%02d  \n",
					((int)curPlayTimes/1000)/60,
					((int)curPlayTimes/1000)%60,
					((int)totalPlayTimes/1000)/60,
					((int)totalPlayTimes/1000)%60);
			break;

		default:
			break;
	}
}

/*****************************************************************************************
* 定时主动获取播放时间
****************************************************************************************/
void TimerStart_BtPlayStatus(void)
{
	TimeOutSet(&btManager.avrcpPlayStatusTimer.timerHandle, 900);
	btManager.avrcpPlayStatusTimer.timerFlag = 1;
}

void TimerStop_BtPlayStatus(void)
{
	btManager.avrcpPlayStatusTimer.timerFlag = 0;
}

uint8_t CheckTimerStart_BtPlayStatus(void)
{
	return btManager.avrcpPlayStatusTimer.timerFlag;
}

#if (BT_AVRCP_VOLUME_SYNC)
/*****************************************************************************************
* AVRCP 同步音量改变
****************************************************************************************/
void BtAvrcpVolumeChanged(BT_AVRCP_CALLBACK_PARAMS * param)
{
	uint16_t VolumePercent = 0;
	btManager.avrcpSyncEnable = 1;
	VolumePercent = BtAbsVolume2VolLevel(param->params.avrcpAdv.avrcpAdvVolumePercent);
	APP_DBG("BTVOL_CHANGE = [%d]\n", VolumePercent);
	
	BtMidMessageSend(MSG_BT_MID_VOLUME_CHANGE, (uint8_t)VolumePercent);
}

/*****************************************************************************************
* 设置/获取 音量同步值
****************************************************************************************/
void SetBtSyncVolume(uint8_t volume)
{
	btManager.avrcpSyncVol = volume;
}

uint8_t GetBtSyncVolume(void)
{
	return btManager.avrcpSyncVol;
}
#endif

/***********************************************************************************
 * 获取歌曲信息函数
 **********************************************************************************/
#if (BT_AVRCP_SONG_TRACK_INFOR)
void GetBtMediaInfo(void *params)
{
	AvrcpAdvMediaInfo	*CurMediaInfo;
	uint8_t i;
	uint8_t StringData[StringMaxLen];
	uint8_t ConvertStringData[StringMaxLen];
	CurMediaInfo = (AvrcpAdvMediaInfo*)params;

	if((CurMediaInfo)&&(CurMediaInfo->numIds))
	{
		for(i=0;i<CurMediaInfo->numIds;i++)
		{
			memset(StringData, 0, StringMaxLen);
			memset(ConvertStringData, 0, StringMaxLen);
			
			if(CurMediaInfo->property[i].length)
			{
				if(CurMediaInfo->property[i].charSet == 0x006a)
				{
					//APP_DBG("Character Set Id: UTF-8\n");
					
					if(CurMediaInfo->property[i].length > StringMaxLen)
					{
						memcpy(StringData, CurMediaInfo->property[i].string, StringMaxLen);
					#ifdef CFG_FUNC_STRING_CONVERT_EN
						StringConvert(ConvertStringData, 60, StringData, StringMaxLen ,UTF8_TO_GBK);
					#else
						memcpy(ConvertStringData, StringData, StringMaxLen);
					#endif
					}
					else
					{
						memcpy(StringData, CurMediaInfo->property[i].string, CurMediaInfo->property[i].length);
					#ifdef CFG_FUNC_STRING_CONVERT_EN
						StringConvert(ConvertStringData, 60, StringData, CurMediaInfo->property[i].length ,UTF8_TO_GBK);
					#else
						memcpy(ConvertStringData, StringData, StringMaxLen);
					#endif
					}

			//Attribute ID
					if(ConvertStringData[0])// no character ,not dispaly ID3
					{
						switch(CurMediaInfo->property[i].attrId)
						{
							case 1:
								APP_DBG("Title of the media:");
								break;
			
							case 2:
								 APP_DBG("Name of the artist:");
								break;
			
							case 3:
								 APP_DBG("Name of the Album:");
								break;
			
							//当前曲目数:只有在自带播放器才能获取到
							case 4:
								//APP_DBG("Number of the media\n");
								break;
			
							//总共曲目数:只有在自带播放器才能获取到
							case 5:
								//APP_DBG("Totle number of the media\n");
								break;
			
							case 6:
								//APP_DBG("Genre\n");
								break;
			
							case 7:
								//APP_DBG("Playing time in millisecond\n");
								break;
							
							case 8:
								//APP_DBG("Default cover art\n");
								break;
			
							default:
								break;
						}
					}

					#ifdef CFG_FUNC_STRING_CONVERT_EN
					if((CurMediaInfo->property[i].attrId == 1) 
						|| (CurMediaInfo->property[i].attrId == 2) 
						|| (CurMediaInfo->property[i].attrId == 3) )
						APP_DBG("%s\n", ConvertStringData);
					#endif
					
					}
				else
				{
					;//APP_DBG("Other Character Set Id: 0x%x\n", CurMediaInfo->property[i].charSet);
				}
			}
		}
	}
}
#endif


/***********************************************************************************
 * AVRCP Browse
 **********************************************************************************/
#if (BT_AVRCP_BROWSER_FUNC)
//avrcp browse
//可供参考
uint32_t TotalNumItems = 0; //当前目录下文件数量(song+dirctory)
uint8_t TotalFolderDepth = 1; //当前文件夹的深度
uint8_t CurUid[8] = {0};	//当前选定的文件的UID信息(media element/folder)
uint16_t CurUidCount = 0;
uint8_t MediaPlayUid[8];
uint16_t BTFolderIndex=0;

static uint16_t B8toHost16(const uint8_t* ptr)
{
	return (uint16_t)( ((uint16_t) *ptr << 8) | ((uint16_t) *(ptr+1)) ); 
}

/////////////////////////////////////////////////////////////////////////////
//avrcp browser function
//范例:
void BtSetBrowsedPlayer(void)
{
	//1:Media player virtual file system
	AvrcpCtrlAdvAvrcpSetBrowsedPlayer(1);
}

void BtGetFolderItems(void)
{
	//获取文件名称
	AvrcpCtrlAdvAvrcpGetFolderItems(BTFolderIndex,BTFolderIndex);
}

void BtFolderForwardCtrl(void)
{
	//进入下一个目录
	AvrcpCtrlAdvAvrcpChangePath(0x01, CurUid);//CurUid:folder uid

	BTFolderIndex = 0;
}

void BtFolderBackwardCtrl(void)
{
	//返回上一个目录
	if(TotalFolderDepth>1)
	{
		AvrcpCtrlAdvAvrcpChangePath(0, CurUid);
	}
}

void BtPlaySongByUid(void)
{
	//播放指定的音乐文件
	AvrcpCtrlAdvAvrcpPlayItem(CurUid);//CurUid:song uid
}

/////////////////////////////////////////////////////////////////////////////
//avrcp browser callback
void BtAvrcpBrowseConnected(void)
{
	//连接成功后重置browse相关寄存器
	TotalNumItems = 0;
	TotalFolderDepth = 1;
	CurUidCount = 0;
	memset(CurUid, 0, 8);
}

void BtAvrcpBrowseSetPlayer(AvrcpBrwsPlayer *params)
{
	uint16_t listLen=0;
	#define StringMaxLen 30
	uint8_t ConvertStringData[StringMaxLen];
	uint8_t StringData[StringMaxLen]={0};
	
	uint16_t offset=0;
	uint8_t i;
	
	APP_DBG("UID Count:0x%04x, Number of Items:0x%08x, CharSet:0x%04x, Folder Depth:%x\n",
						params->params.brwsPlayer->uidCounter,
						params->params.brwsPlayer->numItems,
						params->params.brwsPlayer->charSet,
						params->params.brwsPlayer->fDepth);
						
	TotalNumItems = params->params.brwsPlayer->numItems;
	TotalFolderDepth = params->params.brwsPlayer->fDepth;
	
	//显示当前文件夹的名称(最深层次)
	//1.获取最后1个文件夹 offset
	offset = 0;
	if(params->params.brwsPlayer->fDepth>1)
	{
		for(i=0; i<(params->params.brwsPlayer->fDepth-1);i++)
		{
			offset += B8toHost16(params->params.brwsPlayer->list+offset) + 2;
		}
	}

	//2.解析文件夹信息
	listLen = B8toHost16(params->params.brwsPlayer->list+offset); 
	APP_DBG("Folder Length:%d \nName:", listLen);
	memcpy(StringData, (params->params.brwsPlayer->list+2), listLen);
	#ifdef CFG_FUNC_STRING_CONVERT_EN
		StringConvert(ConvertStringData, StringMaxLen, StringData, listLen ,UTF8_TO_GBK);
		APP_DBG("%s\n", ConvertStringData);
	#endif
}


void BtAvrcpBrowseGetFolderItems(AvrcpFldrItems *params)
{
	uint16_t offset=0;
	AvrcpItemType itemType;
	uint8_t i;
	#define StringMaxLen 60
	uint8_t ConvertStringData[StringMaxLen];
	uint8_t StringData[StringMaxLen]={0};
	uint16_t listLen=0;
	
	APP_DBG("uidCount:%x, numItems:%x\n", params->params.fldItems->uidCounter, params->params.fldItems->numItems);

	if(params->params.fldItems->numItems == 0)
		return;
	
	//if(info->numItems == 1)
	{
		//解析数据
		itemType = params->params.fldItems->list[offset++];
		//itemLen = B8toHost16(info->list+offset);
		offset += 2;
		
		if(itemType == AVRCP_ITEM_FOLDER)
		{
			APP_DBG("TYPE:DIRECTORY\n");

			memcpy(CurUid, params->params.fldItems->list+offset, 8);
			APP_DBG("FolderUid:");
			for(i=0;i<8;i++)
			{
				APP_DBG("%02x ", CurUid[i]);
			}
			APP_DBG("\n");
			
			offset += 8;
			//mediaItem.item.folder.folderType = info->list[offset++];
			offset++;
			//mediaItem->item.folder.isPlayable = info->list[offset++];
			offset++;
			//mediaItem->item.folder.charSet = BEtoHost16(info->list+offset);
			offset += 2;
			
			//folder name length
			listLen = B8toHost16(params->params.fldItems->list+offset);
			offset += 2;
			
			//folder name info
			memcpy(StringData, (params->params.fldItems->list+offset), listLen);

			//listLen = mediaItem.item.folder.nameLen;
			APP_DBG("Name Len:%x\n", listLen);
			//memcpy(StringData, mediaItem.item.folder.name, listLen);
			#ifdef CFG_FUNC_STRING_CONVERT_EN
				StringConvert(ConvertStringData, StringMaxLen, StringData, listLen ,UTF8_TO_GBK);
				APP_DBG("%s\n", ConvertStringData);
			#endif
			
		}
		else if(itemType == AVRCP_ITEM_MEDIA_ELEMENT)
		{
			APP_DBG("TYPE:SONG\n");
			
			memcpy(CurUid, params->params.fldItems->list+offset, 8);
			APP_DBG("MediaElementUid:");
			for(i=0;i<8;i++)
			{
				APP_DBG("%02x ", CurUid[i]);
			}
			APP_DBG("\n");
			offset += 8;
			//mediaItem->item.element.mediaType = list[offset++];
			offset++;
			//mediaItem->item.element.charSet = BEtoHost16(list+offset);
			offset += 2;

			//media element length
			listLen = B8toHost16(params->params.fldItems->list+offset);
			offset += 2;

			//media element info
			memcpy(StringData, (params->params.fldItems->list+offset), listLen);

			#ifdef CFG_FUNC_STRING_CONVERT_EN
				StringConvert(ConvertStringData, StringMaxLen, StringData, listLen ,UTF8_TO_GBK);
				APP_DBG("%s\n", ConvertStringData);
			#endif
		}
	}
}

#endif


#endif	//#if (BT_SUPPORT == ENABLE)
