/**
 *******************************************************************************
 * @file    bt_avrcp_app.c
 * @author  KK
 * @version V1.0.0
 * @date    9-9-2023
 * @brief   Avrcp callback events and actions
 *******************************************************************************
 * @attention
 *
 * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
 * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
 * TIME. AS A RESULT, MVSILICON SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
 * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
 * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
 * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
 */

#include "type.h"
#include "debug.h"
#include "bt_manager.h"
#include "bt_interface.h"
#include "bt_config.h"
#include "string_convert.h"
#include "bt_app_avrcp_deal.h"

#if (BT_SUPPORT == ENABLE)

uint32_t AvrcpStateSuspendCount = 0;

/*****************************************************************
 * @brief  Change Player State Get
 * @param  
 * @return state: TRUE/FALSE
 * @Note   本地的播放状态判断条件
 *****************************************************************/
uint32_t ChangePlayerStateGet(void)
{
	if( AvrcpStateSuspendCount 
		&& ( (btManager.cur_index == 0 && GetA2dpState(1) == BT_A2DP_STATE_STREAMING) || (btManager.cur_index == 1 && GetA2dpState(0) == BT_A2DP_STATE_STREAMING) ) )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/*****************************************************************
 * @brief  bt avrcp callback function
 * @param  event: BT_AVRCP_CALLBACK_EVENT
 * @param  param: BT_AVRCP_CALLBACK_PARAMS
 * @return 
 * @Note   avrcp协议回调事件/参数
 *****************************************************************/
void BtAvrcpCallback(BT_AVRCP_CALLBACK_EVENT event, BT_AVRCP_CALLBACK_PARAMS * param)
{
	switch(event)
	{
		case BT_STACK_EVENT_AVRCP_CONNECTED:
		{
			BtAvrcpConnectedDev(param);
			break;
		}

		case BT_STACK_EVENT_AVRCP_DISCONNECTED:
		{
			BtAvrcpDisconnectedDev(param);
			break;
		}
		case BT_STACK_EVENT_AVRCP_ADV_PLAY_STATUS_CHANGED:
		{
			BtAvrcpPlayStatusChanged(param);
			break;
		}
		case BT_STACK_EVENT_AVRCP_ADV_PLAY_STATUS:
#if (BT_AVRCP_SONG_PLAY_STATE)
			BtAvrcpPlayStatus(param);
#endif
			break;

		case BT_STACK_EVENT_AVRCP_ADV_TRACK_INFO:
			//APP_DBG("TRACK_INFO\n");
#if (BT_AVRCP_SONG_TRACK_INFOR)
			btManager.avrcpMediaInforFlag = 1;
#endif
			break;

		case BT_STACK_EVENT_AVRCP_ADV_ADDRESSED_PLAYERS:
			//APP_DBG("AVRCP Event: BT_STACK_EVENT_AVRCP_ADV_ADDRESSED_PLAYERS\n");
			//APP_DBG("PlayId[%x],UidCount[%x]\n", param->params.avrcpAdv.avrcpAdvAddressedPlayer.playerId, param->params.avrcpAdv.avrcpAdvAddressedPlayer.uidCounter);
			break;

		case BT_STACK_EVENT_AVRCP_ADV_CAPABILITY_COMPANY_ID:

			break;
			
		case BT_STACK_EVENT_AVRCP_ADV_CAPABILITY_EVENTS_SUPPORTED:
			//APP_DBG("AVRCP SUPPORTED FEATRUE = [0x%04x]\n", param->params.avrcpAdv.avrcpAdvEventMask);
			break;

		case BT_STACK_EVENT_AVRCP_ADV_GET_PLAYER_SETTING_VALUE:
#if (BT_AVRCP_PLAYER_SETTING)
			APP_DBG("attrId:%x, ", param->params.avrcpAdv.avrcpAdvPlayerettingValue.attrMask);
			if(param->params.avrcpAdv.avrcpAdvPlayerettingValue.attrMask == AVRCP_ENABLE_PLAYER_EQ_STATUS)
				APP_DBG("EQ:%x\n", param->params.avrcpAdv.avrcpAdvPlayerettingValue.eq);
			else if(param->params.avrcpAdv.avrcpAdvPlayerettingValue.attrMask == AVRCP_ENABLE_PLAYER_REPEAT_STATUS)
				APP_DBG("repeat:%x\n", param->params.avrcpAdv.avrcpAdvPlayerettingValue.repeat);
			else if(param->params.avrcpAdv.avrcpAdvPlayerettingValue.attrMask == AVRCP_ENABLE_PLAYER_SHUFFLE_STATUS)
				APP_DBG("shuffle:%x\n", param->params.avrcpAdv.avrcpAdvPlayerettingValue.shuffle);
			else if(param->params.avrcpAdv.avrcpAdvPlayerettingValue.attrMask == AVRCP_ENABLE_PLAYER_SCAN_STATUS)
				APP_DBG("scan:%x\n", param->params.avrcpAdv.avrcpAdvPlayerettingValue.scan);
#endif
			break;

#if (BT_AVRCP_VOLUME_SYNC)
		case BT_STACK_EVENT_AVRCP_ADV_VOLUME_CHANGE:
			//APP_DBG("BTVOL_CHANGE = [%d]\n", param->params.avrcpAdv.avrcpAdvVolumePercent);
#if (BT_LINK_DEV_NUM == 2)
			if(BtCurIndex_Get() == param->index)
#endif
				BtAvrcpVolumeChanged(param);
			break;
#endif

#if (BT_AVRCP_SONG_TRACK_INFOR)
		case BT_STACK_EVENT_AVRCP_ADV_MEDIA_INFO:
			APP_DBG("AVRCP Event: BT_STACK_EVENT_AVRCP_ADV_MEDIA_INFO\n");
			if(GetMediaInfo)
				GetMediaInfo((void*)param->params.avrcpAdv.avrcpAdvMediaInfo);
			break;
#endif

#if ((BT_AVRCP_VOLUME_SYNC) || (BT_SOURCE_SUPPORT))
		case BT_STACK_EVENT_AVRCP_PANEL_RELEASE:
			BtMidMessageSend(MSG_BT_MID_AVRCP_PANEL_KEY, (uint8_t)param->params.panelKey);
		break;
#endif		

#if (BT_AVRCP_BROWSER_FUNC == ENABLE)
///////////////////////////////////////////////////////////////////////////////////////////
//avrcp browse
		case BT_STACK_EVENT_AVRCP_ADV_AVRCP_BROWSE_CONNECTED:
			APP_DBG("avrcp browse is connected\n");
			//连接成功后重置browse相关寄存器
			TotalNumItems = 0;
			TotalFolderDepth = 1;
			CurUidCount = 0;
			memset(CurUid, 0, 8);
			break;

		case BT_STACK_EVENT_AVRCP_ADV_AVRCP_BROWSE_DISCONNECT:
			APP_DBG("avrcp browse disconnect\n");
			break;
			
		case BT_STACK_EVENT_AVRCP_ADV_AVRCP_SET_BROWSER_PLAYER:
			APP_DBG("UI : BT_UI_CB_STATUS_ADV_AVRCP_SET_BROWSER_PLAYER\n");
			{
				uint16_t listLen=0;
				#define StringMaxLen 30
				uint8_t ConvertStringData[StringMaxLen];
				uint8_t StringData[StringMaxLen]={0};
				
				uint16_t offset=0;
				uint8_t i;
				
				APP_DBG("UID Count:0x%04x, Number of Items:0x%08x, CharSet:0x%04x, Folder Depth:%x\n",
	                    			param->params.brwsPlayer->uidCounter,
	                    			param->params.brwsPlayer->numItems,
	                    			param->params.brwsPlayer->charSet,
	                    			param->params.brwsPlayer->fDepth);
	                    			
				TotalNumItems = param->params.brwsPlayer->numItems;
				TotalFolderDepth = param->params.brwsPlayer->fDepth;
				
	            //显示当前文件夹的名称(最深层次)
	            //1.获取最后1个文件夹 offset
	            offset = 0;
	            if(param->params.brwsPlayer->fDepth>1)
	            {
		            for(i=0; i<(param->params.brwsPlayer->fDepth-1);i++)
		            {
		            	offset += B8toHost16(param->params.brwsPlayer->list+offset) + 2;
		            }
		        }

	            //2.解析文件夹信息
	            listLen = B8toHost16(param->params.brwsPlayer->list+offset); 
	            APP_DBG("Folder Length:%d \nName:", listLen);
	            memcpy(StringData, (param->params.brwsPlayer->list+2), listLen);
	            #ifdef CFG_FUNC_STRING_CONVERT_EN
					StringConvert(ConvertStringData, StringMaxLen, StringData, listLen ,UTF8_TO_GBK);
					APP_DBG("%s\n", ConvertStringData);
				#endif
			}
			break;

		case BT_STACK_EVENT_AVRCP_ADV_AVRCP_GET_FOLDER_ITEMS:
			APP_DBG(" UI : BT_UI_CB_STATUS_ADV_AVRCP_GET_FOLDER_ITEMS\n");
			{
				uint16_t offset=0;
				AvrcpItemType itemType;
				uint8_t i;
				#define StringMaxLen 60
				uint8_t ConvertStringData[StringMaxLen];
				uint8_t StringData[StringMaxLen]={0};
				uint16_t listLen=0;
				
				APP_DBG("uidCount:%x, numItems:%x\n", param->params.fldItems->uidCounter, param->params.fldItems->numItems);

				if(param->params.fldItems->numItems == 0)
					break;
				
				//if(info->numItems == 1)
				{
					//解析数据
					itemType = param->params.fldItems->list[offset++];
				    //itemLen = B8toHost16(info->list+offset);
				    offset += 2;
				    
					if(itemType == AVRCP_ITEM_FOLDER)
					{
						APP_DBG("TYPE:DIRECTORY\n");

						memcpy(CurUid, param->params.fldItems->list+offset, 8);
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
				        listLen = B8toHost16(param->params.fldItems->list+offset);
				        offset += 2;
				        
				        //folder name info
        				memcpy(StringData, (param->params.fldItems->list+offset), listLen);

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
						
						memcpy(CurUid, param->params.fldItems->list+offset, 8);
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
				        listLen = B8toHost16(param->params.fldItems->list+offset);
				        offset += 2;

				        //media element info
        				memcpy(StringData, (param->params.fldItems->list+offset), listLen);

						#ifdef CFG_FUNC_STRING_CONVERT_EN
							StringConvert(ConvertStringData, StringMaxLen, StringData, listLen ,UTF8_TO_GBK);
							APP_DBG("%s\n", ConvertStringData);
						#endif
					}
				}
			}
			break;

		case BT_STACK_EVENT_AVRCP_ADV_AVRCP_CHANGE_PATH:
			APP_DBG("UI : BT_UI_CB_STATUS_ADV_AVRCP_CHANGE_PATH\n");
			APP_DBG("numItems:%x\n", param->params.chPath->numItems);
			break;
			
		case BT_STACK_EVENT_AVRCP_ADV_AVRCP_GET_ITEM_ATTRIBUTES:
			APP_DBG("UI : BT_UI_CB_STATUS_ADV_AVRCP_GET_ITEM_ATTRIBUTES\n");
			APP_DBG("numAttrs:%x\n", param->params.itemAttrs->numAttrs);
			break;
			
		case BT_STACK_EVENT_AVRCP_ADV_AVRCP_SEARCH:
			APP_DBG("UI : BT_UI_CB_STATUS_ADV_AVRCP_SEARCH\n");
			APP_DBG("uidCount:%x, numItems:%x\n", param->params.search->uidCounter, param->params.search->numItems);
			break;
#endif //(BT_AVRCP_BROWSER_FUNC == ENABLE)

///////////////////////////////////////////////////////////////////////////////////////////
		default:
			break;
	}
}


/*****************************************************************
 * @brief  change player
 * @param  
 * @return 
 * @Note   双手机连接切换播放流程
 *****************************************************************/
void ChangePlayer(void)
{
	/*if((PreChangeFlag == TRUE) && IsTimeOut(&tChangePlayer))
	{
		BtCurIndex_Set(btManager.back_index);
		BT_DBG("channel[%d]:start playing\n", btManager.cur_index);
		//if(RefreshSbcDecoder)
		{
			//RefreshSbcDecoder();
			BtMidMessageSend(MSG_BT_MID_PLAY_STATE_CHANGE, AVRCP_ADV_MEDIA_PLAYING);
		}
		PreChangeFlag = FALSE;
	}*/
#if (BT_LINK_DEV_NUM == 2)
	if(AvrcpStateSuspendCount)
	{
		AvrcpStateSuspendCount++;
		if(AvrcpStateSuspendCount >= 1000)
		{
			uint8_t switch_index;
			if (btManager.cur_index >= BT_LINK_DEV_NUM)
			{
				AvrcpStateSuspendCount = 0;
				return;
			}
			switch_index = (btManager.cur_index == 0) ? 1 : 0;

			if(GetA2dpState(switch_index) == BT_A2DP_STATE_STREAMING)
			{
#ifdef LAST_PLAY_PRIORITY
				APP_DBG("ChangePlayer btManager.cur_index Pause %d %d\n",btManager.btLinked_env[btManager.cur_index].avrcp_index,GetA2dpState(btManager.cur_index));
				AvrcpCtrlPause(btManager.btLinked_env[btManager.cur_index].avrcp_index);
				SetA2dpState(btManager.cur_index,BT_A2DP_STATE_CONNECTED);
#endif
				BtCurIndex_Set(switch_index);
				a2dp_sbc_decoder_init();
			}
			AvrcpStateSuspendCount = 0;
		}
	}
#endif

}

/*****************************************************************
 * @brief  avrcp link state set
 * @param  index: 0 or 1
 * @param  state: BT_AVRCP_STATE
 * @Note   
 *****************************************************************/
void SetAvrcpState(uint8_t index, BT_AVRCP_STATE state)
{
	if (index >= BT_LINK_DEV_NUM)
		return;
	btManager.btLinked_env[index].avrcpState = state;
}

/*****************************************************************
 * @brief  avrcp link state get
 * @param  index: 0 or 1
 * @return state: BT_AVRCP_STATE
 * @Note   
 *****************************************************************/
BT_AVRCP_STATE GetAvrcpState(uint8_t index)
{
	if (index >= BT_LINK_DEV_NUM)
		return BT_AVRCP_STATE_NONE;
	return btManager.btLinked_env[index].avrcpState;
}

/*****************************************************************
 * @brief  avrcp current play state set
 * @param  index: 0 or 1
 * @param  state: uint8_t
 * @Note   
 *****************************************************************/
void SetAvrcpCurPlayState(uint8_t index, uint8_t state)
{
	if (index >= BT_LINK_DEV_NUM)
		return;
	btManager.btLinked_env[index].avrcpCurPlayState = state;
}

/*****************************************************************
 * @brief  avrcp current play state get
 * @param  index: 0 or 1
 * @return state: uint8_t
 * @Note   
 *****************************************************************/
uint8_t GetAvrcpCurPlayState(uint8_t index)
{
	if (index >= BT_LINK_DEV_NUM)
		return 0;
	return btManager.btLinked_env[index].avrcpCurPlayState;
}

/*****************************************************************
 * @brief  avrcp link state is connected?
 * @param  index: 0 or 1
 * @return state: 1=connected
 * @Note   
 *****************************************************************/
bool IsAvrcpConnected(uint8_t index)
{
	return (GetAvrcpState(index) == BT_AVRCP_STATE_CONNECTED);
}

/*****************************************************************
 * @brief  avrcp connect
 * @param  index: 0 or 1
 * @param  addr: remote address
 * @Note   avrcp协议连接接口
 *****************************************************************/
void BtAvrcpConnect(uint8_t index, uint8_t *addr)
{
#if (BT_LINK_DEV_NUM == 2)
	uint8_t avrcp_index = GetBtManagerAvrcpIndex(index);

	if( avrcp_index < BT_LINK_DEV_NUM
		&&(btManager.btLinked_env[avrcp_index].avrcpState > BT_AVRCP_STATE_NONE))
	{
		APP_DBG("BtAvrcpConnect:avrcpState index[%d] is %d\n",avrcp_index, btManager.btLinked_env[avrcp_index].avrcpState);
		return;
	}
#endif
	if (addr == NULL)
		return;
	APP_DBG("AvrcpConnect index = %d,addr:%x:%x:%x:%x:%x:%x\n", index, addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]);
	AvrcpConnect(index, addr);
}

#endif
