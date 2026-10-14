/**
 **************************************************************************************
 * @file    bluetooth_connect.c
 * @brief   bluetooth 连接/回连  相关函数功能接口
 *
 * @author  KK
 * @version V1.0.0
 *
 * $Created: 2019-10-28 18:00:00$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include <string.h>
#include "type.h"

//application
#include "app_config.h"
#include "bt_config.h"
#include "main_task.h"
#include "bt_manager.h"
#include "debug.h"
#include "bt_app_common.h"
#include "bt_app_connect.h"
#include "bt_app_ddb_info.h"
#include "bt_engine_utility.h"
#include "bt_stack_service.h"

#if (BT_SUPPORT == ENABLE)

extern bool IsBtReconnectReady(void);
extern uint8_t BB_link_state_is_used(uint8_t *addr);
extern int32_t lm_env_link_state(uint8_t *addr);
#ifdef BT_UNIFIED_CONTROL
extern bool BtIsSystemOn(void); // 查询是否允许回连
#endif

static uint32_t g_bt_discon_fail_cnt = 0; //传统的断开方式,连续4次断开不成功,则直接调用HCI断开

/*****************************************************************************************
 * 
 ****************************************************************************************/
void BtConnectCtrl(void)
{
	BT_DBG("bt connect ctrl\n");
	
	BtDdb_GetLastBtAddr(GetBtManager()->btDdbLastAddr, &GetBtManager()->btDdbLastProfile);

	if(!BtAddrIsValid(btManager.btDdbLastAddr))
	{
		BT_DBG("Last Addr is NULL\n");
		return ;
	}
	
	//reconnect remote device
	BT_DBG("Reconnect:");
	{
		uint8_t i;
		for(i=0; i<6; i++)
		{
			BT_DBG("%x ", btManager.btDdbLastAddr[i]);
		}
	}
	BT_DBG("\n");

	//主动连接2次
	BtReconnectDevCreate(btManager.btDdbLastAddr, 2, 3, 0, btManager.btDdbLastProfile);
}

/*****************************************************************************************
 * OnlyDisconnect = TRUE, 直接断开，不判断条件,不终止回连行为
 ****************************************************************************************/
void BtDisconnectCtrl(bool OnlyDisconnect)
{
	uint8_t i;

#ifdef BT_PROFILE_BQB_ENABLE
	BTDisconnect(0);
#endif

	if(OnlyDisconnect)
	{
		//直接断开，不判断条件,不终止回连行为
		for(i=0; i<BT_LINK_DEV_NUM;i++)
		{
			BTDisconnect(i);
	#ifdef BT_MULTI_LINK_SUPPORT
			btManager.btLinked_env[i].btLinkDisconnFlag = 1;//防止双手机时，回连异常
	#endif
		}
		return;
	}

	BT_DBG("bt disconnect ctrl\n");

	//1.终止回连行为
	BtReconnectDevStop();

	//2.断开已连接上的profile
	for(i=0;i<BT_LINK_DEV_NUM;i++)
	{
		if(btManager.btLinked_env[i].btLinkState)
		{
#ifdef BT_MULTI_LINK_SUPPORT
			btManager.btLinked_env[i].btLinkDisconnFlag = 1;//防止双手机时，回连异常
#endif
			BTDisconnect(btManager.btLinked_env[i].a2dp_index);//根据连接ID来断连
		}
	}
	
//#if (BT_LINK_DEV_NUM == 2) //目前代码secondindex暂未使用
//	SecondTalkingPhoneIndexSet(0xff);//断开连接后清标志
//#endif
}

/*****************************************************************************************
 * 
 ****************************************************************************************/
void BtCancelConnect(void)
{
	//此函数内不能加打印信息
	//BtReconnectDevStop();	
	//正在处理中,立即停止
	if(btManager.btReconExcuteSt == (&btManager.btReconPhoneSt))
	{
		btManager.btReconExcuteSt = NULL;
	}
	else
	{
		//从链表中移除
		btstack_list_remove(&btManager.btReconHandle, &btManager.btReconPhoneSt.item);
	}
	btManager.btReconPhoneSt.excute = NULL;
	btManager.btReconPhoneSt.ConnectionTimer.timerFlag = TIMER_UNUSED;
	btManager.btReconPhoneSt.profile = 0;
	if (GetBtManager()->btAccessModeEnable != BT_ACCESSBLE_GENERAL)
	{
		SetBtUserState(BT_USER_STATE_NONE);
	}
	else
	{
		SetBtUserState(BT_USER_STATE_PREPAIR);
	}	
}

/*****************************************************************************************
 * 
 ****************************************************************************************/
//此函数底层调用
void BtCancelReconnect(void)
{
	//BtReconnectDevStop();
	
	//正在处理中,超时退出
	if(btManager.btReconExcuteSt == (&btManager.btReconPhoneSt))
	{
	//	btManager.btReconExcuteSt = NULL;
	//	btManager.btReconPhoneSt.excute = NULL;
	//	btManager.btReconPhoneSt.ConnectionTimer.timerFlag = TIMER_UNUSED;
		btManager.btReconPhoneSt.TryCount = 0;
	}
	else
	{
		//从链表中移除
		btstack_list_remove(&btManager.btReconHandle, &btManager.btReconPhoneSt.item);
		btManager.btReconPhoneSt.excute = NULL;
		btManager.btReconPhoneSt.ConnectionTimer.timerFlag = TIMER_UNUSED;
		btManager.btReconPhoneSt.profile = 0;
	}
}

/*********************************************************************
 * 注册、注销 回连profile优先级顺序
 * default: hfp -> a2dp -> avrcp
 *********************************************************************/
void BtReconProfilePrioRegister(uint32_t profile)
{
	btManager.btReconProfilePriority |= profile;
}

void BtReconProfilePrioDeregister(uint32_t profile)
{
	btManager.btReconProfilePriority &= ~profile;
}

uint32_t BtReconProfilePrioFlagGet(void)
{
	return btManager.btReconProfilePriority;
}

/*****************************************************************************************
 * 
 ****************************************************************************************/
void BtReconnectDevExcute(void)
{
	BT_DBG("BtReconnectDevExcute, %d\n", btManager.btReconPhoneSt.TryCount);
	uint8_t i;

#if BT_SOURCE_SUPPORT
	if(BtSourceReconnectDevExcute())
		return;
#endif
	
	for(i=0;i<BT_LINK_DEV_NUM;i++)
	{
		if(memcmp(btManager.btDdbLastAddr, btManager.btLinked_env[i].remoteAddr, 6)==0)
			break;
	}

	if(i<BT_LINK_DEV_NUM)
	{
			if((GetA2dpState(i) == BT_A2DP_STATE_NONE)&&(GetAvrcpState(i) == BT_AVRCP_STATE_NONE)
#if (BT_HFP_SUPPORT)
			&&(GetHfpState(i) == BT_HFP_STATE_NONE)
#endif
			)
	    {
	        //当前链路存在,则应用层停止发起新的连接,等待超时后再发起连接
	        if(lm_env_link_state(btManager.btDdbLastAddr))
	        {
		        BT_DBG("link exist, delay2s reconnect device...\n");
	            BtReconnectDevStop();
	            BtReconnectDevCreate(btManager.btDdbLastAddr, sys_parameter.bt_ReconnectionTryCounts, sys_parameter.bt_ReconnectionInternalTime, 2000, btManager.btDdbLastProfile);
	            return;
	        }
	    }
	}
	
	btManager.btReconPhoneSt.profile &= (BT_PROFILE_SUPPORTED_HFP | BT_PROFILE_SUPPORTED_A2DP | BT_PROFILE_SUPPORTED_AVRCP);

	//当前链路已被使用,并且不是当前的设备,则停止回连流程
	if(BB_link_state_is_used(btManager.btDdbLastAddr))
	{
		btManager.btReconPhoneSt.profile &= ~(BT_PROFILE_SUPPORTED_HFP | BT_PROFILE_SUPPORTED_A2DP | BT_PROFILE_SUPPORTED_AVRCP);
		BT_DBG("!!!  lm link used, stop reconnect...\n");
		BtStackServiceMsgSend(MSG_BTSTACK_RECONNECT_REMOTE_STOP);
		return;
	}

	//修改蓝牙协议回连顺序
	if(btManager.btReconProfilePriority)
	{
		if((btManager.btReconProfilePriority & BT_PROFILE_SUPPORTED_A2DP)&&(GetA2dpState(0) >= BT_A2DP_STATE_CONNECTED))
			BtReconProfilePrioDeregister(BT_PROFILE_SUPPORTED_A2DP);
		
		if((btManager.btReconProfilePriority & BT_PROFILE_SUPPORTED_AVRCP)&&(GetAvrcpState(0) >= BT_AVRCP_STATE_CONNECTED))
			BtReconProfilePrioDeregister(BT_PROFILE_SUPPORTED_AVRCP);
		
		if((btManager.btReconProfilePriority & BT_PROFILE_SUPPORTED_A2DP))
		{
			BT_DBG("A2DP connect priority...\n");
			BtA2dpConnect(0, btManager.btDdbLastAddr);
			return;
		}
		else if((btManager.btReconProfilePriority & BT_PROFILE_SUPPORTED_AVRCP))
		{
			BT_DBG("AVRCP connect priority...\n");
			BtAvrcpConnect(0, btManager.btDdbLastAddr);
			return;
		}
	}

	//在回连手机时,打开手机蓝牙功能;安卓手机会自动回连;
	//某些安卓手机回连非常快,会导致sdp channel复用,导致异常;
	//在发起回连时,确定sdp server是不是被使用,未使用则开始进行回连
#if (BT_HFP_SUPPORT)
	if((btManager.btReconPhoneSt.profile & BT_PROFILE_SUPPORTED_HFP)&&(GetHfpState(BtCurIndex_Get()) < BT_HFP_STATE_CONNECTED))
	{
		signed char status = 0;
		/*if(!IsBtReconnectReady())
		{
			BtReconnectDelay();
			return;
		}*/
		BT_DBG("connect hfp\n");
		status = BtHfpConnect(0,btManager.btDdbLastAddr);
		if(status > 2)
		{
	        BT_DBG("link exist, delay2s reconnect device...\n");
            BtReconnectDevStop();
            BtReconnectDevCreate(btManager.btDdbLastAddr, sys_parameter.bt_ReconnectionTryCounts, sys_parameter.bt_ReconnectionInternalTime, 2000, btManager.btDdbLastProfile);
            return;
		}
	}
	else 
#endif
	if((btManager.btReconPhoneSt.profile & BT_PROFILE_SUPPORTED_A2DP)&&(GetA2dpState(BtCurIndex_Get()) < BT_A2DP_STATE_CONNECTED))
	{
		btManager.btReconPhoneSt.profile &= ~(BT_PROFILE_SUPPORTED_HFP);
		
		/*if(!IsBtReconnectReady())
		{
			BtReconnectDelay();
			return;
		}*/
		BT_DBG("connect a2dp\n");
		BtA2dpConnect(0,btManager.btDdbLastAddr);
	}
	else if((btManager.btReconPhoneSt.profile & BT_PROFILE_SUPPORTED_AVRCP)&&(GetAvrcpState(BtCurIndex_Get()) != BT_AVRCP_STATE_CONNECTED))
	{
		btManager.btReconPhoneSt.profile &= ~(BT_PROFILE_SUPPORTED_HFP | BT_PROFILE_SUPPORTED_A2DP);
		
		/*if(!IsBtReconnectReady())
		{
			BtReconnectDelay();
			return;
		}*/
		BT_DBG("connect avrcp\n");
		BtAvrcpConnect(0,btManager.btDdbLastAddr);
	}
	else
	{
		btManager.btReconPhoneSt.profile &= ~(BT_PROFILE_SUPPORTED_HFP | BT_PROFILE_SUPPORTED_A2DP | BT_PROFILE_SUPPORTED_AVRCP);
		//btManager.btReconPhoneSt.ConnectionTimer.timerFlag = TIMER_UNUSED;
		BT_DBG("connect end\n");
		BtStackServiceMsgSend(MSG_BTSTACK_RECONNECT_REMOTE_STOP);
	}
}

/*****************************************************************************************
 * @brief : bt reconnect device 
 * @params: profile
 * @params: addr
 * @params: try count
 * @params: interval time
 * @params: delay time
 * @note  : 1.在开机的时候，需要回连连接过的设备
 *          2.在BB连接丢失(设备拿远后)，需要回连设备
 ****************************************************************************************/
void BtReconnectDevCreate(uint8_t *addr, uint8_t tryCount, uint8_t interval, uint32_t delayMs, uint32_t profile)
{
#ifdef BT_UNIFIED_CONTROL
	// 检查是否允许回连（蓝牙关闭时不允许）
	if (!BtIsSystemOn())
	{
		BT_DBG("BtReconnectDevCreate blocked: BT power off\n");
		return;
	}
#endif

	if(btManager.btReconExcuteSt == (&btManager.btReconPhoneSt))
	{
		BT_DBG("BtReconnectDev is running..., wait\n");
		//BtReconnectDevStop();
		return;
	}

    if(tryCount == 0)
    {
		BT_DBG("BtReconnectDevCreate fail, count = 0\n");
		return;
    }

	if(((addr[0]==0)&&(addr[1]==0)&&(addr[2]==0)&&(addr[3]==0)&&(addr[4]==0)&&(addr[5]==0))
		||
		((addr[0]==0xff)&&(addr[1]==0xff)&&(addr[2]==0xff)&&(addr[3]==0xff)&&(addr[4]==0xff)&&(addr[5]==0xff)))
	{
		BT_DBG("addr == null\n");
		return;
	}
	
	BT_DBG("BtReconnectDevCreate\n");
	SetBtUserState(BT_USER_STATE_RECON);
	btManager.btReconPhoneSt.ConnectionTimer.timerFlag = TIMER_UNUSED;
	memcpy(btManager.btReconPhoneSt.RemoteDevAddr, addr, BT_ADDR_SIZE);
	btManager.btReconPhoneSt.TryCount = tryCount;
	btManager.btReconPhoneSt.IntervalTime = interval;
	btManager.btReconPhoneSt.DelayStartTime = delayMs;
	btManager.btReconPhoneSt.profile = profile;
	btManager.btReconPhoneSt.excute = (FUNC_RECONNECTION_EXCUTE)BtReconnectDevExcute;

	btstack_list_add_tail(&btManager.btReconHandle, &btManager.btReconPhoneSt.item);

	btManager.btReconStopDelay = 5;
}

/*****************************************************************************************
 * 
 ****************************************************************************************/
//1.在连接设备成功后，需要停止回连
//2.在连接次数超时后，需要停止回连
//3.在有其他设备连接成功后，停止回连
void BtReconnectDevStop(void)
{
	if(btManager.btReconStopDelay > 0)
	{
		return;
	}
	BT_DBG("BtReconnectDevStop\n");
	BtReconProfilePrioDeregister(BT_PROFILE_SUPPORT_GENERAL);
	
	//正在处理中,立即停止
	if(btManager.btReconExcuteSt == (&btManager.btReconPhoneSt))
	{
		btManager.btReconExcuteSt = NULL;
	}
	else
	{
		//从链表中移除
		btstack_list_remove(&btManager.btReconHandle, &btManager.btReconPhoneSt.item);
	}
	btManager.btReconPhoneSt.excute = NULL;
	btManager.btReconPhoneSt.ConnectionTimer.timerFlag = TIMER_UNUSED;
	btManager.btReconPhoneSt.profile = 0;
}

/*****************************************************************************************
 * 
 ****************************************************************************************/
void BtReconnectDevice(void)
{
	btManager.BtPowerOnFlag = 1;

	//reconnect last bluetooth device
	BtDdb_GetLastBtAddr(GetBtManager()->btDdbLastAddr, &GetBtManager()->btDdbLastProfile);
	if(!BtAddrIsValid(btManager.btDdbLastAddr))
	{
		BT_DBG("Last Addr is NULL\n");
		return ;
	}
	
	//reconnect remote device
	BT_DBG("Reconnect:");
	{
		uint8_t i;
		for(i=0; i<6; i++)
		{
			BT_DBG("%x ", btManager.btDdbLastAddr[i]);
		}
	}
	BT_DBG("\n");

    if(GetBtCurConnectFlag())
    {
        BtReconnectDevCreate(btManager.btDdbLastAddr, sys_parameter.bt_ReconnectionTryCounts, sys_parameter.bt_ReconnectionInternalTime, 2000, btManager.btDdbLastProfile);
    }
    else
    {
        BtReconnectDevCreate(btManager.btDdbLastAddr, sys_parameter.bt_ReconnectionTryCounts, sys_parameter.bt_ReconnectionInternalTime, 1000, btManager.btDdbLastProfile);
    }
}

uint8_t BtReconnectDevIsUsed(void)
{
	if(btManager.btReconPhoneSt.excute)
		return 1;
	else
		return 0;
}

uint8_t BtReconnectDevIsExcute(void)
{
	if(btManager.btReconExcuteSt == (&btManager.btReconPhoneSt))
		return 1;
	else
		return 0;
}

//延时后再次发起协议连接
void BtReconnectDevAgain(uint32_t delay)
{
	if(btManager.btReconExcuteSt == (&btManager.btReconPhoneSt))
	{
		TimeOutSet(&btManager.btReconExcuteSt->ConnectionTimer.timerHandle, delay);
		btManager.btReconExcuteSt->ConnectionTimer.timerFlag = (TIMER_USED | TIMER_WAITING);
	}
}

uint32_t delay_time = 0;
/***********************************************************************************
 * @breif  : bt reconnect process
 * @note   : 蓝牙回连流程处理函数
 **********************************************************************************/
void BtReconnectProcess(void)
{
	if(btManager.btReconStopDelay > 0)
		btManager.btReconStopDelay--;
	//执行当前回连的流程 phone
	if(btManager.btReconExcuteSt)
	{
		if(btManager.btReconExcuteSt->ConnectionTimer.timerFlag == TIMER_UNUSED)
		{
			//当前的回连执行进程结束
			if(!btstack_list_empty(&btManager.btReconHandle))
			{
				//等待执行下一个链表的回连流程,将当前的回连流程插入队列
				if(btManager.btReconExcuteSt->TryCount)
				{
					btManager.btReconExcuteSt->TryCount--;
					btstack_list_add_tail(&btManager.btReconHandle, &btManager.btReconExcuteSt->item);
					//BT_DBG("reconnect: insert list\n");
				}
				/*else
				{
					BT_DBG("reconnect: end\n");
				}*/
				btManager.btReconExcuteSt = NULL;
			}
			else
			{
				//链表中无其他回连等待事件,则超时等待下一次连接到来
				if(btManager.btReconExcuteSt->TryCount)
				{
					btManager.btReconExcuteSt->TryCount--;
					TimeOutSet(&btManager.btReconExcuteSt->ConnectionTimer.timerHandle, (btManager.btReconExcuteSt->IntervalTime * 1000));
					btManager.btReconExcuteSt->ConnectionTimer.timerFlag = (TIMER_USED | TIMER_WAITING);
					//BT_DBG("reconnect: end -> waiting\n");
				}
				else
				{
					btManager.btReconExcuteSt = NULL;
					if (GetBtManager()->btAccessModeEnable != BT_ACCESSBLE_GENERAL)
					{
						SetBtUserState(BT_USER_STATE_NONE);
					}else{
						SetBtUserState(BT_USER_STATE_PREPAIR);
					}
				}
			}
		}
		else if(btManager.btReconExcuteSt->ConnectionTimer.timerFlag & TIMER_STARTED)
		{
			btManager.btReconExcuteSt->ConnectionTimer.timerFlag &= ~TIMER_STARTED;
			
			if(btManager.btReconExcuteSt->DelayStartTime)
			{
				//延时处理
				TimeOutSet(&btManager.btReconExcuteSt->ConnectionTimer.timerHandle, (btManager.btReconExcuteSt->DelayStartTime));
				btManager.btReconExcuteSt->DelayStartTime = 0;
				btManager.btReconExcuteSt->ConnectionTimer.timerFlag |= TIMER_WAITING;
				//BT_DBG("reconnect: start -> waiting\n");
			}
			else if(btManager.btReconExcuteSt->excute)
			{
				//回连执行函数
				btManager.btReconExcuteSt->excute();
				//BT_DBG("reconnect: excute\n");
			}
			else
			{
				//异常退出
				btManager.btReconExcuteSt->ConnectionTimer.timerFlag = TIMER_UNUSED;
				//BT_DBG("reconnect: excute is null, start -> unused\n");
			}
		}
		else if(btManager.btReconExcuteSt->ConnectionTimer.timerFlag & TIMER_WAITING)
		{
			//延时等待处理
			if(IsTimeOut(&btManager.btReconExcuteSt->ConnectionTimer.timerHandle))
			{
				btManager.btReconExcuteSt->ConnectionTimer.timerFlag &= ~TIMER_WAITING;
				btManager.btReconExcuteSt->ConnectionTimer.timerFlag |= TIMER_STARTED;
				//BT_DBG("reconnect: waiting -> start\n");
			}
		}
	}
	//从回连管理链表中获取需要执行的流程
	else if(!btstack_list_empty(&btManager.btReconHandle))
	{
		if (delay_time < 100)
		{
			delay_time++;
		}
		else
		{
			delay_time = 0;
			btManager.btReconExcuteSt = (BT_RECONNECT_ST*)btstack_list_pop(&btManager.btReconHandle);
			//BT_DBG("reconnect: pop list...\n");
		
			if(btManager.btReconExcuteSt)
			{
				btManager.btReconExcuteSt->ConnectionTimer.timerFlag = (TIMER_USED | TIMER_STARTED);
				//BT_DBG("reconnect: start...\n");
			}
		}
	}
}

/***********************************************************************************
 * 快速开关蓝牙
 * 断开蓝牙连接，蓝牙进入不可被搜索，不可被连接状态
 **********************************************************************************/
void BtScanPageStateSet(BT_SCAN_PAGE_STATE state)
{
	btManager.btScanPageState = state;
}

BT_SCAN_PAGE_STATE BtScanPageStateGet(void)
{
	return btManager.btScanPageState;
}

void BtScanPageStateCheck(void)
{
	static uint32_t bt_disconnect_count = 0;
	switch(btManager.btScanPageState)
	{
		case BT_SCAN_PAGE_STATE_CLOSING:
			//BT_DBG("BT_SCAN_PAGE_STATE_CLOSING\n");
			// If there is a reconnectiong process, stop it
			BtReconnectDevStop();

#if  (BT_TWS_SUPPORT == ENABLE) && (CFG_TWS_ONLY_IN_BT_MODE == ENABLE)
			BtReconnectTwsStop();

			BtTwsDeviceDisconnectExt();
#endif

			if(sys_parameter.bt_BackgroundType == BT_BACKGROUND_FAST_POWER_ON_OFF)
			{
				// If there is a bt link, disconnect it
				if(GetBtCurConnectFlag())
				{
					BtDisconnectCtrl(FALSE);
					BtScanPageStateSet(BT_SCAN_PAGE_STATE_DISCONNECTING);
					g_bt_discon_fail_cnt = 0;
					break;
				}
			}

#if (BT_TWS_SUPPORT == ENABLE)
#if (CFG_TWS_ONLY_IN_BT_MODE == DISABLE)
			if((btManager.twsFlag) && (btManager.twsState == BT_TWS_STATE_NONE))
			{
				if(IsIdleModeReady())
					Vis_Set(BtAccessModeNotAccessible,__FUNCTION__);//BtSetAccessMode_NoDisc_NoCon();
				else
					Vis_Set(BtAccessModeConnectableOnly,__FUNCTION__);//BtSetAccessMode_NoDisc_Con();
			}
			else
#endif
#endif
			{
				Vis_Set(BtAccessModeNotAccessible,__FUNCTION__);
			}
			BtScanPageStateSet(BT_SCAN_PAGE_STATE_DISABLE);
			break;
			
		case BT_SCAN_PAGE_STATE_DISCONNECTING:
			if(bt_disconnect_count > 500)	// wait about 200ms
			{
				/*if(GetBtDeviceConnState() != BT_DEVICE_CONNECTION_MODE_NONE)
				{
					BtStackServiceMsgSend(MSG_BTSTACK_ACCESS_MODE_SET);
					BtScanPageStateSet(BT_SCAN_PAGE_STATE_DISABLE);
				}*/
				if(GetBtCurConnectFlag() == 0)
                {
					BtStackServiceMsgSend(MSG_BTSTACK_ACCESS_MODE_SET);
					BtScanPageStateSet(BT_SCAN_PAGE_STATE_DISABLE);

					g_bt_discon_fail_cnt = 0;
                }
				else
				{
					if(GetBtCurConnectFlag())
					{
						//前4次,断开流程走协议断开方式,超过后,直接发送HCI断开CMD
						g_bt_discon_fail_cnt++;
						if(g_bt_discon_fail_cnt >= 5)
							BTHciDisconnectCmd(btManager.btDdbLastAddr);
						else
							BtDisconnectCtrl(FALSE);
						BtScanPageStateSet(BT_SCAN_PAGE_STATE_DISCONNECTING);
					}
				}
				bt_disconnect_count = 0;
			}
			else
				bt_disconnect_count++;
			break;
			
		case BT_SCAN_PAGE_STATE_DISABLE:
			if(sys_parameter.bt_BackgroundType == BT_BACKGROUND_FAST_POWER_ON_OFF)
			{
				// double check wether there is a bt link, if any, disconnect again
				if(GetBtCurConnectFlag())
				{
					BtDisconnectCtrl(FALSE);
					BtScanPageStateSet(BT_SCAN_PAGE_STATE_DISCONNECTING);
				}
			}
			break;

		case BT_SCAN_PAGE_STATE_OPENING:
			BT_DBG("BT_SCAN_PAGE_STATE_OPENING\n");
			BtScanPageStateSet(BT_SCAN_PAGE_STATE_ENABLE);

			btManager.BtPowerOnFlag = 0;

#if (BT_TWS_SUPPORT == ENABLE)
			btManager.TwsPowerOnFlag = 0;
#endif
			
#if ((CFG_TWS_ONLY_IN_BT_MODE == ENABLE) || defined(TWS_SLAVE_MODE_SWITCH_EN))
#if ((TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER)||(TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM) || (TWS_PAIRING_MODE == CFG_TWS_BLE_PAIR))
			if(sys_parameter.bt_TwsReconnectionEnable && btManager.twsFlag)
			{
				btManager.btConStateProtectCnt = 1;
				if(btManager.twsRole == BT_TWS_SLAVE)
				{
					BtReconnectTws_Slave();
				}
				else
				{
					BtReconnectTws();
				}
			}
			if(sys_parameter.bt_ReconnectionEnable)
			{
				BtReconnectDevice();
			}

			BtSetAccessMode_select();
			break;
#endif
#else
#if ((TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)||(TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE))
			if(sys_parameter.bt_ReconnectionEnable)
			{
				//BtReconnectDevice();
				GetBtManager()->btReconnectDelayCount = 1;
			}

#elif (TWS_PAIRING_MODE != CFG_TWS_ROLE_SLAVE)
			if(sys_parameter.bt_TwsReconnectionEnable && btManager.twsFlag)
			{
				if(btManager.twsRole == BT_TWS_SLAVE)
				{
#ifndef CFG_AUTO_ENTER_TWS_SLAVE_MODE
					BtReconnectTws_Slave();
					BtSetAccessMode_select();
					break;
#endif
			
					if(sys_parameter.bt_ReconnectionEnable && GetCurTotaBtRecNum())
					{
						btManager.btAccessModeEnable = BT_ACCESSBLE_GENERAL;
						BtSetAccessMode_select();// tws 从机概率出现可见性异常问题  2024/11/11

						BtReconnectDevice();
						break;
					}
				}
			}
			if(sys_parameter.bt_ReconnectionEnable)
			{
				btManager.btReconnectDelayCount = 1;
			}
#endif
#endif

			//btManager.btAccessModeEnable = BT_ACCESSBLE_GENERAL;
			BtSetAccessMode_select();
			
			break;
			
		case BT_SCAN_PAGE_STATE_ENABLE:
			break;

		case BT_SCAN_PAGE_STATE_OPEN_WAITING:
			//sleep唤醒后延时回连手机
			if(btManager.btExitSniffReconPhone == 0)
			{
				BT_DBG("BT_SCAN_PAGE_STATE_OPEN_WAITING_END\n");
				BtScanPageStateSet(BT_SCAN_PAGE_STATE_OPENING);
			}
			break;

		default:
			break;
	}
}


/*****************************************************************************************
 * TWS
 ****************************************************************************************/
#if (BT_TWS_SUPPORT == ENABLE)
/*****************************************************************************************
 *
 ****************************************************************************************/
void BtReconnectTwsExcute(void)
{
	BT_DBG("BtReconnectTwsExcute\n");

	if(btManager.twsState != BT_TWS_STATE_CONNECTED)
	{
        //当前链路存在,则应用层停止发起新的连接,等待超时后再发起连接
        if(lm_env_link_state(btManager.btReconTwsSt.RemoteDevAddr))
        {
	        printf("tws link exist, delay5s reconnect device...\n");
            BtReconnectTwsStop();
            BtReconnectTwsCreate(btManager.btTwsDeviceAddr, sys_parameter.bt_TwsReconnectionTryCounts, sys_parameter.bt_TwsReconnectionInternalTime, 5000, BT_PROFILE_SUPPORTED_TWS);
            return;
        }
		BT_DBG("tws connect...\n");
		tws_connect(btManager.btReconTwsSt.RemoteDevAddr);
	}
	else
	{
		//btManager.btReconTwsSt.ConnectionTimer.timerFlag = TIMER_UNUSED;
		BtReconnectTwsStop();
	}
}

/*****************************************************************************************
 *
 ****************************************************************************************/
void BtReconnectTwsCreate(uint8_t *addr, uint8_t tryCount, uint8_t interval, uint32_t delayMs, uint32_t profile)
{
#ifdef BT_UNIFIED_CONTROL
	// 检查是否允许回连（蓝牙关闭时不允许）
	if (!BtIsSystemOn())
	{
		BT_DBG("BtReconnectTwsCreate blocked: BT power off\n");
		return;
	}
#endif

	if(btManager.twsState == BT_TWS_STATE_CONNECTED)
		return;

	if(btManager.btReconExcuteSt == (&btManager.btReconTwsSt))
	{
		BT_DBG("BtReconnectTws is running...\n");
		return;
	}

    if(tryCount == 0)
    {
		BT_DBG("BtReconnectTwsCreate fail, count = 0\n");
		return;
    }

	BT_DBG("BtReconnectTwsCreate\n");
	btManager.btReconTwsSt.ConnectionTimer.timerFlag = TIMER_UNUSED;
	memcpy(btManager.btReconTwsSt.RemoteDevAddr, addr, BT_ADDR_SIZE);
	btManager.btReconTwsSt.TryCount = tryCount;
	btManager.btReconTwsSt.IntervalTime = interval;
	btManager.btReconTwsSt.DelayStartTime = delayMs;
	btManager.btReconTwsSt.profile = profile;
	btManager.btReconTwsSt.excute = (FUNC_RECONNECTION_EXCUTE)BtReconnectTwsExcute;

	btstack_list_add_tail(&btManager.btReconHandle, &btManager.btReconTwsSt.item);
}

/*****************************************************************************************
 *
 ****************************************************************************************/
void BtReconnectTws_Slave(void)
{
	if(btManager.twsState == BT_TWS_STATE_CONNECTED)
		return;

	if(btManager.TwsPowerOnFlag)
		return;
	btManager.TwsPowerOnFlag = 1;

	//reconnect remote device
	BT_DBG("tws slave Reconnect:");
	{
		uint8_t i;
		for(i=0; i<6; i++)
		{
			BT_DBG("%x ", btManager.btTwsDeviceAddr[i]);
		}
	}
	BT_DBG("\n");

	BtReconnectTwsCreate(btManager.btTwsDeviceAddr, sys_parameter.bt_TwsReconnectionTryCounts, sys_parameter.bt_TwsReconnectionInternalTime, 5000, BT_PROFILE_SUPPORTED_TWS);
}

/*****************************************************************************************
 *
 ****************************************************************************************/
void BtReconnectTws(void)
{
	if(btManager.twsState == BT_TWS_STATE_CONNECTED)
		return;

	if(btManager.TwsPowerOnFlag)
		return;
	btManager.TwsPowerOnFlag = 1;

	//reconnect remote device
	BT_DBG("tws Reconnect:");
	{
		uint8_t i;
		for(i=0; i<6; i++)
		{
			BT_DBG("%x ", btManager.btTwsDeviceAddr[i]);
		}
	}
	BT_DBG("\n");

	sys_parameter.bt_TwsReconnectionInternalTime += 2;//master 每次回连间隔+2s,跟slave 回连错开
	BtStartReconnectTws(sys_parameter.bt_TwsReconnectionTryCounts, sys_parameter.bt_TwsReconnectionInternalTime);
}

/*****************************************************************************************
 *
 ****************************************************************************************/
//1.在开机的时候，需要回连连接过的设备
//2.在BB连接丢失(设备拿远后)，需要回连设备
void BtStartReconnectTws(uint8_t tryCount, uint8_t interval)
{
	BtReconnectTwsCreate(btManager.btTwsDeviceAddr, tryCount, interval, 900, BT_PROFILE_SUPPORTED_TWS);
}

/*****************************************************************************************
 *
 ****************************************************************************************/
//1.在连接设备成功后，需要停止回连
//2.在连接次数超时后，需要停止回连
//3.在有其他设备连接成功后，停止回连
void BtReconnectTwsStop(void)
{
	BT_DBG("BtReconnectTwsStop\n");

	//正在处理中,立即停止
	if(btManager.btReconExcuteSt == (&btManager.btReconTwsSt))
	{
		btManager.btReconExcuteSt = NULL;
	}
	else
	{
		//从链表中移除
		btstack_list_remove(&btManager.btReconHandle, &btManager.btReconTwsSt.item);
	}
	btManager.btReconTwsSt.excute = NULL;
	btManager.btReconTwsSt.ConnectionTimer.timerFlag = TIMER_UNUSED;
	btManager.btReconTwsSt.profile = 0;
}

void BtTwsCancelReconnect(void)
{
	//此函数内不能加打印信息
	BtReconnectTwsStop();

	BtDdb_ClearTwsDeviceRecord();
}

#endif //#if (BT_TWS_SUPPORT == ENABLE)
#else
void BtCancelConnect(void)
{
}
#endif //#if (BT_SUPPORT == ENABLE)
