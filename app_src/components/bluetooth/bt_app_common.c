/**
 **************************************************************************************
 * @file    bluetooth_common.c
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
#include "delay.h"
#include "debug.h"
#include "app_config.h"
#include "bt_config.h"

#include "chip_info.h"

#include "main_task.h"
#include "bt_interface.h"
#include "bt_manager.h"
#include "bt_common_api.h"
#include "bt_app_ddb_info.h"
#include "bt_a2dp_api.h"
#include "bt_avrcp_api.h"
#include "bt_config.h"
#include "bb_api.h"
#include "rtos_api.h"
#include "bt_app_common.h"
#include "bt_app_connect.h"
//#include "bt_app_sniff.h"
#include "mode_task.h"
#include "bt_stack_service.h"
#include "bt_play_mode.h"
#if (BT_TWS_SUPPORT == ENABLE)
#include "bt_app_tws_connect.h"
#endif

#if ((BT_SUPPORT == ENABLE) || (BLE_SUPPORT == ENABLE))
extern BT_CONFIGURATION_PARAMS		*btStackConfigParams;
#endif
#if (BT_SUPPORT == ENABLE)

//蓝牙被连接条件状态
#define BT_CON_REJECT		0
#define BT_CON_MASTER		1
#define BT_CON_SLAVE		2
#define BT_CON_UNKNOW		3



//extern api function
extern void Set_rwip_sleep_enable(bool flag);

extern uint8_t is_tws_device(uint8_t *remote_addr);
/*****************************************************************************************
 * 连接条件判断
 * 场景1：双手机+TWS
 ****************************************************************************************/ 
#if ((BT_DEVICE_NUMBER == 2)&&(BT_TWS_LINK_NUM == 1))
static uint32_t BtRemoteDeviceConnecting_DualPhoneAndTws(uint8_t *addr)
{
	if(btManager.btLastAddrUpgradeIgnored
#if (BT_HFP_SUPPORT == ENABLE)
		|| (GetHfpState(BtCurIndex_Get()) > BT_HFP_STATE_CONNECTED)
#endif
		)
	{
		BT_DBG("****** mv test box is connected ***\n");
		return BT_CON_REJECT; //1. 测试盒连接上设备后,不允许其他设备再次连接进行测试 2. 主机在进行来电响铃或通话，拒绝其他设备连接
	}

//对箱主/角色随机
#if ((TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)||(TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER) || (TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER) || (TWS_PAIRING_MODE == CFG_TWS_BLE_PAIR))
	if(is_tws_device(addr))
	{
		if(btManager.twsState == BT_TWS_STATE_CONNECTED)
		{
			BT_DBG("tws state is connected, reject the new tws device\n");
			return BT_CON_REJECT;
		}

		if((btManager.btLinkState == 1)
			&&(btManager.linkedNumber >= 2))
		{
			BT_DBG("2phones are connected, reject the new tws device\n");
			return BT_CON_REJECT;
		}

		if((btManager.btLinkState == 1)
			||(GetA2dpState(BtCurIndex_Get()) >= BT_A2DP_STATE_CONNECTED)
			||(GetAvrcpState(BtCurIndex_Get()) >= BT_AVRCP_STATE_CONNECTED)
#if (BT_HFP_SUPPORT == ENABLE)
			|| (GetHfpState(BtCurIndex_Get()) >= BT_HFP_STATE_CONNECTED)
#endif
			)
		{
			if((memcmp(addr, btManager.btTwsDeviceAddr, 6) == 0)&&(btManager.twsFlag))
			{
				if(btManager.twsRole == BT_TWS_MASTER)
				{
					BT_DBG("phone connected, tws Role:Master\n");
					return BT_CON_MASTER;
				}
				else
				{
					BT_DBG("role not match, reject\n");
					return BT_CON_REJECT;
				}
			}
			else
			{
				BT_DBG("phone connected, reject\n");
				return BT_CON_REJECT;
			}
		}

		if((btManager.twsFlag)&&(btManager.twsRole == BT_TWS_MASTER))
		{
			if(memcmp(addr, btManager.btTwsDeviceAddr, 6) == 0)
			{
				BT_DBG("BT_TWS_MASTER: tws device connecting, Cur_Role:Master\n");
				return BT_CON_MASTER;
			}
			else
			{
				BT_DBG("BT_TWS_MASTER: tws device connecting, Cur_Role:Slave\n");
				return BT_CON_SLAVE;
			}
		}
		else if((btManager.twsFlag)&&(btManager.twsRole == BT_TWS_SLAVE))
		{
			BT_DBG("BT_TWS_SLAVE: tws device connecting, Cur_Role:Slave\n");
			BtReconnectTwsStop();
			return BT_CON_SLAVE;
		}
		else
		{
			BT_DBG("BT_TWS_SLAVE: tws device connecting, Cur_Role:unknow\n");
			return BT_CON_UNKNOW;
		}
	}
	else
	{
		if(!IsBtAudioMode() && sys_parameter.bt_BackgroundType == BT_BACKGROUND_FAST_POWER_ON_OFF)
		{
			return BT_CON_REJECT;
		}
		else
		if((btManager.twsState == BT_TWS_STATE_CONNECTED))
		{
			if(btManager.twsRole == BT_TWS_SLAVE)
			{
				return BT_CON_REJECT;
			}

			if(btManager.btLinkState == 1)
			{
				return BT_CON_REJECT;
			}

			return BT_CON_MASTER;
		}
		else
		{
			if((btManager.btLinkState == 1)
				&&(btManager.linkedNumber >= 2)
				)
			{
				//双手机已连上,拒绝新手机连接
				return BT_CON_REJECT;
			}
			else
			{
				return BT_CON_MASTER;
			}
		}
	}

#elif(TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)
	if(is_tws_device(addr))
	{
		if(btManager.twsState == BT_TWS_STATE_CONNECTED)
		{
			BT_DBG("tws state is connected, reject the new tws device\n");
			return BT_CON_REJECT;
		}
		else if(btManager.btLinkState == 1)
		{
			BT_DBG("phone connected, reject tws connect\n");
			return BT_CON_REJECT;
		}
		else
		{
			BT_DBG("tws device connecting\n");
			return BT_CON_SLAVE;
		}
	}
	else
	{
		if(!IsBtAudioMode() && sys_parameter.bt_BackgroundType == BT_BACKGROUND_FAST_POWER_ON_OFF)
		{
			return BT_CON_REJECT;
		}
		else if(btManager.twsState == BT_TWS_STATE_CONNECTED)
		{
			BT_DBG("tws state is connected, reject the new tws device\n");
			return BT_CON_REJECT;
		}
		else
		{
			if((btManager.btLinkState == 1)
				&&(btManager.linkedNumber >= 2)
				)
			{
				return BT_CON_REJECT;
			}
			else
			{
				return BT_CON_MASTER;
			}
		}
	}
#else
	return BT_CON_SLAVE;
#endif
}
#endif

/*****************************************************************************************
 * 连接条件判断
 * 场景2：单手机+TWS
 ****************************************************************************************/ 
#if ((BT_DEVICE_NUMBER == 1)&&(BT_TWS_LINK_NUM == 1))
static uint32_t BtRemoteDeviceConnecting_SinglePhoneAndTws(uint8_t *addr)
{
	if(btManager.btLastAddrUpgradeIgnored
#if (BT_HFP_SUPPORT == ENABLE)
		|| (GetHfpState(BtCurIndex_Get()) > BT_HFP_STATE_CONNECTED)
#endif
		)
	{
		BT_DBG("****** mv test box is connected ***\n");
		return BT_CON_REJECT; //1. 测试盒连接上设备后,不允许其他设备再次连接进行测试 2. 主机在进行来电响铃或通话，拒绝其他设备连接
	}

//	if(is_tws_device(addr) && Get_bis_role()!=BIS_NONE)
//	{
//		BT_DBG("****** bis role = %d ***\n",Get_bis_role());
//		return BT_CON_REJECT;
//	}

//对箱主/角色随机
#if ((TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)||(TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER) || (TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER) || (TWS_PAIRING_MODE == CFG_TWS_BLE_PAIR))
	if(is_tws_device(addr))
	{
		if(btManager.twsState == BT_TWS_STATE_CONNECTED)
		{
			BT_DBG("tws state is connected, reject the new tws device\n");
			return BT_CON_REJECT;
		}
		if((btManager.btLinkState == 1)
			||(GetA2dpState(BtCurIndex_Get()) >= BT_A2DP_STATE_CONNECTED)
			||(GetAvrcpState(BtCurIndex_Get()) >= BT_AVRCP_STATE_CONNECTED)
#if (BT_HFP_SUPPORT == ENABLE)
			|| (GetHfpState(BtCurIndex_Get()) >= BT_HFP_STATE_CONNECTED)
#endif
			)
		{
			if((memcmp(addr, btManager.btTwsDeviceAddr, 6) == 0)&&(btManager.twsFlag))
			{
				if(btManager.twsRole == BT_TWS_MASTER)
				{
					BT_DBG("phone connected, tws Role:Master\n");
					return BT_CON_MASTER;
				}
				else
				{
					#if (CFG_TWS_PAIR_DOUBLEKEY && (TWS_PAIRING_MODE != CFG_TWS_ROLE_RANDOM))
					BT_DBG("role not match, reject\n");
					return BT_CON_REJECT;
					#else
					BT_DBG("phone connected2, tws Role:Master\n");
					return BT_CON_MASTER;
					#endif
				}
			}
			else
			{
				#if (CFG_TWS_PAIR_DOUBLEKEY && (TWS_PAIRING_MODE != CFG_TWS_ROLE_RANDOM))
				BT_DBG("role not match, reject\n");
				return BT_CON_REJECT;
				#else
				BT_DBG("phone connected2, tws Role:Master\n");
				return BT_CON_MASTER;
				#endif
			}
		}

		if((btManager.twsFlag)&&(btManager.twsRole == BT_TWS_MASTER))
		{
			if(memcmp(addr, btManager.btTwsDeviceAddr, 6) == 0)
			{
				BT_DBG("BT_TWS_MASTER: tws device connecting, Cur_Role:Master\n");
				return BT_CON_MASTER;
			}
			else
			{
				BT_DBG("BT_TWS_MASTER: tws device connecting, Cur_Role:Slave\n");
				return BT_CON_SLAVE;
			}
		}
		else if((btManager.twsFlag)&&(btManager.twsRole == BT_TWS_SLAVE))
		{
			BT_DBG("BT_TWS_SLAVE: tws device connecting, Cur_Role:Slave\n");
			BtReconnectTwsStop();
			return BT_CON_SLAVE;
		}
		else
		{
			BT_DBG("BT_TWS_SLAVE: tws device connecting, Cur_Role:unknow\n");
			return BT_CON_UNKNOW;
		}
	}
	else
	{
		if(!IsBtAudioMode() && sys_parameter.bt_BackgroundType == BT_BACKGROUND_FAST_POWER_ON_OFF)
		{
			return BT_CON_REJECT;
		}
		else
		{
			extern unsigned char get_tws_device_number(void);
			extern uint8_t lc_link_number(void);

			if((lc_link_number()>=2) && (get_tws_device_number()==0))
			{
				printf("2 phone connected, disconnect current link req. \n");
				return BT_CON_REJECT;
			}
		}

		if((btManager.twsState == BT_TWS_STATE_CONNECTED))
		{
			if(btManager.twsRole == BT_TWS_SLAVE)
			{
				return BT_CON_REJECT;
			}

			if(btManager.btLinkState == 1)
			{
				return BT_CON_REJECT;
			}

			return BT_CON_MASTER;
		}
		else
		{
			if(btManager.btLinkState == 1)
			{
				//手机已连上,拒绝新手机连接
				return BT_CON_REJECT;
			}
			else
			{
				return BT_CON_MASTER;
			}
		}
	}

#elif(TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)
	if(is_tws_device(addr))
	{
		if(btManager.twsState == BT_TWS_STATE_CONNECTED)
		{
			BT_DBG("tws state is connected, reject the new tws device\n");
			return BT_CON_REJECT;
		}
		else if(btManager.btLinkState == 1)
		{
			BT_DBG("phone connected, reject tws connect\n");
			return BT_CON_REJECT;
		}
		else
		{
			BT_DBG("tws device connecting\n");
			return BT_CON_MASTER;//BT_CON_SLAVE;
		}
	}
	else
	{
		if(!IsBtAudioMode() && sys_parameter.bt_BackgroundType == BT_BACKGROUND_FAST_POWER_ON_OFF)
		{
			return BT_CON_REJECT;
		}
		else
			if(btManager.twsState == BT_TWS_STATE_CONNECTED)
		{
			BT_DBG("tws state is connected, reject the new tws device\n");
			return BT_CON_REJECT;
		}
		else
		{
			if(btManager.btLinkState == 1)
			{
				return BT_CON_REJECT;
			}
			else
			{
				return BT_CON_MASTER;
			}
		}
	}

#else
	return BT_CON_SLAVE;
#endif
}

#endif

/*****************************************************************************************
 * 连接条件判断
 * 场景3：双手机
 ****************************************************************************************/ 
#if ((BT_DEVICE_NUMBER == 2)&&(BT_TWS_LINK_NUM == 0))
static uint32_t BtRemoteDeviceConnecting_DualPhone(uint8_t *addr)
{
	if(btManager.btLastAddrUpgradeIgnored 
#if (BT_HFP_SUPPORT == ENABLE)
		|| (GetHfpState(BtCurIndex_Get()) > BT_HFP_STATE_CONNECTED)
#endif
		)
	{
		BT_DBG("****** mv test box is connected ***\n");
		return BT_CON_REJECT; //1. 测试盒连接上设备后,不允许其他设备再次连接进行测试 2. 主机在进行来电响铃或通话，拒绝其他设备连接
	}
	
	if(!IsBtAudioMode() && sys_parameter.bt_BackgroundType == BT_BACKGROUND_FAST_POWER_ON_OFF)
	{
		return BT_CON_REJECT;
	}
	else
	{
		if((btManager.btLinkState == 1)
			&&(btManager.linkedNumber>=2)
			)
		{
			return BT_CON_REJECT;
		}
		else 
		{
#ifdef FUNC_BIS_SUPPORT
			extern uint8_t lc_link_number(void);
			extern uint8_t BtMultiLinkCheck(void);
			if((lc_link_number()>=2) && !BtMultiLinkCheck())
				return BT_CON_REJECT;
#endif
			return BT_CON_MASTER;
		}
	}
}
#endif

/*****************************************************************************************
 * 连接条件判断
 * 场景4：单手机
 ****************************************************************************************/ 
#if ((BT_DEVICE_NUMBER == 1)&&(BT_TWS_LINK_NUM == 0))
static uint32_t BtRemoteDeviceConnecting_SinglePhone(uint8_t *addr)
{
	if(btManager.btLastAddrUpgradeIgnored 
#if (BT_HFP_SUPPORT == ENABLE)
		|| (GetHfpState(BtCurIndex_Get()) > BT_HFP_STATE_CONNECTED)
#endif
		)
	{
		BT_DBG("****** mv test box is connected ***\n");
		return BT_CON_REJECT; //1. 测试盒连接上设备后,不允许其他设备再次连接进行测试 2. 主机在进行来电响铃或通话，拒绝其他设备连接
	}
	
	if(!IsBtAudioMode() && sys_parameter.bt_BackgroundType == BT_BACKGROUND_FAST_POWER_ON_OFF)
	{
		return BT_CON_REJECT;
	}
	else
	{
		if((btManager.btLinkState == 1)
			||(GetA2dpState(BtCurIndex_Get()) > BT_A2DP_STATE_NONE)
			||(GetAvrcpState(BtCurIndex_Get()) > BT_AVRCP_STATE_NONE)
#if (BT_HFP_SUPPORT)
			||(GetHfpState(BtCurIndex_Get()) > BT_HFP_STATE_NONE)
#endif
			)
		{
			return BT_CON_REJECT;
		}
		else 
		{
			return BT_CON_SLAVE;
		}
	}
}
#endif

/***********************************************************************************
 * 被连接时,确认连接条件
 * return: 
 * 0=reject
 * 1=master
 * 2=slave
 **********************************************************************************/
uint32_t BtRemoteLinkConReq(uint8_t *addr)
{
#if ((BT_DEVICE_NUMBER == 2)&&(BT_TWS_LINK_NUM == 1))
	return BtRemoteDeviceConnecting_DualPhoneAndTws(addr);
#elif ((BT_DEVICE_NUMBER == 1)&&(BT_TWS_LINK_NUM == 1))
	return BtRemoteDeviceConnecting_SinglePhoneAndTws(addr);
#elif ((BT_DEVICE_NUMBER == 2)&&(BT_TWS_LINK_NUM == 0))
	return BtRemoteDeviceConnecting_DualPhone(addr);
#else //((BT_DEVICE_NUMBER == 1)&&(BT_TWS_LINK_NUM == 0))
	return BtRemoteDeviceConnecting_SinglePhone(addr);
#endif
}




/*****************************************************************************************
 * 反馈当前蓝牙可见性
 ****************************************************************************************/ 
void BtAccessModeUpdate(BtAccessMode accessMode)
{
	BT_DBG("$$$ access mode %d->%d\n", btManager.deviceConMode, accessMode);
	SetBtDeviceConnState(accessMode);
}

/*****************************************************************************************
 * 根据实际的应用条件，重新配置蓝牙可见性
 * 场景1：双手机
 ****************************************************************************************/ 
#if ((BT_DEVICE_NUMBER == 2)&&(BT_TWS_LINK_NUM == 0))
static void BtAccessMode_DualPhone(void)
{
	BT_DBG("BtAccessModeSetting, DualPhone \n");

	if(btManager.btLinkState)
	{
		if(IsIdleModeReady())
		{
			Vis_Set(BtAccessModeNotAccessible,__FUNCTION__);
		}
		else if(btManager.linkedNumber >= 2)
		{
			Vis_Set(BtAccessModeNotAccessible,__FUNCTION__);
		}
		else
		{
#ifdef BT_LINK_2DEV_ACCESS_DIS_CON
			BtSetAccessMode_select();
#else
			Vis_Set(BtAccessModeConnectableOnly,__FUNCTION__);
#endif
		}
	}
	else
	{
		if(IsIdleModeReady())
		{
			Vis_Set(BtAccessModeNotAccessible,__FUNCTION__);
		}
		else if(sys_parameter.bt_BackgroundType == BT_BACKGROUND_FAST_POWER_ON_OFF)
		{
			if(IsBtAudioMode())
			{
				BtSetAccessMode_select();
			}
			else
			{
				Vis_Set(BtAccessModeNotAccessible,__FUNCTION__);
			}
		}
		else
		{
			BtSetAccessMode_select();
		}
	}
}
#endif
/*****************************************************************************************
 * 根据实际的应用条件，重新配置蓝牙可见性
 * 场景2：单手机
 ****************************************************************************************/ 
#if ((BT_DEVICE_NUMBER == 1)&&(BT_TWS_LINK_NUM == 0))
static void BtAccessMode_SinglePhone(void)
{
	BT_DBG("BtAccessModeSetting, SinglePhone \n");
	
	if(btManager.btLinkState)
	{
		Vis_Set(BtAccessModeNotAccessible,__FUNCTION__);
	}
	else
	{
		if(IsIdleModeReady())
		{
			Vis_Set(BtAccessModeNotAccessible,__FUNCTION__);
		}
		else if(sys_parameter.bt_BackgroundType == BT_BACKGROUND_FAST_POWER_ON_OFF)
		{
			if(IsBtAudioMode())
			{
				BtSetAccessMode_select();
			}
			else
			{
				Vis_Set(BtAccessModeNotAccessible,__FUNCTION__);
			}
		}
		else
		{
			BtSetAccessMode_select();
		}
	}
}
#endif

/*****************************************************************************************
 * 根据实际的应用条件，重新配置蓝牙可见性
 * 场景3：双手机+TWS
 ****************************************************************************************/
#if ((BT_DEVICE_NUMBER == 2)&&(BT_TWS_LINK_NUM == 1))
static void BtAccessMode_DualPhoneAndTws(void)
{
	printf("BtAccessModeSetting, DualPhoneAndTws \n");
	if(btManager.twsState == BT_TWS_STATE_CONNECTED)
	{
		if(btManager.btLinkState)
		{
			//BtSetAccessMode_NoDisc_NoCon();
			Vis_Set(BtAccessModeNotAccessible,__FUNCTION__);
		}
		else
		{
			if(sys_parameter.bt_BackgroundType == BT_BACKGROUND_FAST_POWER_ON_OFF)
			{
				if(IsBtAudioMode())
				{
					BtSetAccessMode_select();
				}
				else
				{
					//BtSetAccessMode_NoDisc_NoCon();
					Vis_Set(BtAccessModeNotAccessible,__FUNCTION__);
				}
			}
			else
				BtSetAccessMode_select();
		}
	}
	else
	{
		if(btManager.btLinkState)
		{
			if(IsIdleModeReady())
			{
				//BtSetAccessMode_NoDisc_NoCon();
				Vis_Set(BtAccessModeNotAccessible,__FUNCTION__);
			}
			else if(btManager.linkedNumber >= 2)
			{
				//BtSetAccessMode_NoDisc_NoCon();
				Vis_Set(BtAccessModeNotAccessible,__FUNCTION__);
			}
			else
			{
			#ifdef BT_LINK_2DEV_ACCESS_DIS_CON
				BtSetAccessMode_select();
			#else
				//BtSetAccessMode_NoDisc_Con();
				Vis_Set(BtAccessModeConnectableOnly,__FUNCTION__);
			#endif
			}
		}
		else
		{
			if(IsIdleModeReady())
			{
				//BtSetAccessMode_NoDisc_NoCon();
				Vis_Set(BtAccessModeNotAccessible,__FUNCTION__);
			}
			else if(sys_parameter.bt_BackgroundType == BT_BACKGROUND_FAST_POWER_ON_OFF)
			{
				if(IsBtAudioMode())
				{
					BtSetAccessMode_select();
				}
				else if(IsBtTwsSlaveMode())
				{
					//BtSetAccessMode_NoDisc_NoCon();
					Vis_Set(BtAccessModeNotAccessible,__FUNCTION__);
				}
				else
				{
					#if (CFG_TWS_ONLY_IN_BT_MODE == ENABLE)
					//BtSetAccessMode_NoDisc_NoCon();
					Vis_Set(BtAccessModeNotAccessible,__FUNCTION__);
					#else
					BtSetAccessMode_select();
					#endif
				}
			}
			else
			{
				BtSetAccessMode_select();
			}
		}
	}
}
#endif
/*****************************************************************************************
 * 根据实际的应用条件，重新配置蓝牙可见性
 * 场景4：单手机+TWS
 ****************************************************************************************/
#if ((BT_DEVICE_NUMBER == 1)&&(BT_TWS_LINK_NUM == 1))
static void BtAccessMode_SinglePhoneAndTws(void)
{
	printf("BtAccessModeSetting, SinglePhoneAndTws \n");
	if(btManager.twsState == BT_TWS_STATE_CONNECTED)
	{
		if(btManager.btLinkState)
		{
			//BtSetAccessMode_NoDisc_NoCon();
			Vis_Set(BtAccessModeNotAccessible,__FUNCTION__);
		}
		else
		{
			if(sys_parameter.bt_BackgroundType == BT_BACKGROUND_FAST_POWER_ON_OFF)
			{
				if(IsBtAudioMode())
				{
					BtSetAccessMode_select();
				}
				else
				{
					//BtSetAccessMode_NoDisc_NoCon();
					Vis_Set(BtAccessModeNotAccessible,__FUNCTION__);
				}
			}
			else
				BtSetAccessMode_select();
		}
	}
	else
	{
		if(btManager.btLinkState)
		{
			if(IsIdleModeReady())
			{
				//BtSetAccessMode_NoDisc_NoCon();
				Vis_Set(BtAccessModeNotAccessible,__FUNCTION__);
			}
#if 0	//当前应用场景 从机断开组网后，被手机连接可见性要设为2
			else if((btManager.twsFlag) && (btManager.twsRole == BT_TWS_SLAVE))
			{
				//BtSetAccessMode_NoDisc_NoCon();
				Vis_Set(BtAccessModeNotAccessible,__FUNCTION__);
				BtReconnectTwsStop();
			}
#endif
			else
			{
				//BtSetAccessMode_NoDisc_Con();
				Vis_Set(BtAccessModeConnectableOnly,__FUNCTION__);
			}
		}
		else
		{
			if(IsIdleModeReady())
			{
				//BtSetAccessMode_NoDisc_NoCon();
				Vis_Set(BtAccessModeNotAccessible,__FUNCTION__);
			}
			else if(sys_parameter.bt_BackgroundType == BT_BACKGROUND_FAST_POWER_ON_OFF)
			{
				if(IsBtAudioMode())
				{
					BtSetAccessMode_select();
				}
				else if(IsBtTwsSlaveMode())
				{
					//BtSetAccessMode_NoDisc_NoCon();
					Vis_Set(BtAccessModeNotAccessible,__FUNCTION__);
				}
				else
				{
				#if (CFG_TWS_ONLY_IN_BT_MODE == ENABLE)
					BtSetAccessMode_NoDisc_NoCon();
				#else
					//BtSetAccessMode_NoDisc_Con();
					Vis_Set(BtAccessModeConnectableOnly,__FUNCTION__);
				#endif
				}
			}
			else
			{
				BtSetAccessMode_select();
			}
		}
	}
}
#endif

/*****************************************************************************************
 * 根据实际的应用条件，重新配置蓝牙可见性
 ****************************************************************************************/ 
void BtAccessModeSetting(void)
{
#if ((BT_DEVICE_NUMBER == 2)&&(BT_TWS_LINK_NUM == 1))
	BtAccessMode_DualPhoneAndTws();
#elif ((BT_DEVICE_NUMBER == 1)&&(BT_TWS_LINK_NUM == 1))
	BtAccessMode_SinglePhoneAndTws();
#elif ((BT_DEVICE_NUMBER == 2)&&(BT_TWS_LINK_NUM == 0))
	BtAccessMode_DualPhone();
#else //((BT_DEVICE_NUMBER == 1)&&(BT_TWS_LINK_NUM == 0))
	BtAccessMode_SinglePhone();
#endif
}

/*****************************************************************************************
 * 蓝牙协议栈初始化成功后反馈
 ****************************************************************************************/
void BtStackInitialized(void)
{
	BT_DBG("BT_STACK_EVENT_COMMON_STACK_INITIALIZED\n");
	SetBtDeviceConnState(BT_DEVICE_CONNECTION_MODE_NONE);
	
	Set_rwip_sleep_enable(0);
	
	BtMidMessageSend(MSG_BT_MID_STACK_INIT, 0);
}

/*****************************************************************************************
 * 蓝牙名称反馈函数
 ****************************************************************************************/
void BtGetRemoteName(BT_STACK_CALLBACK_PARAMS * param)
{
	memset(btManager.remoteName, 0, BT_NAME_SIZE);
	btManager.remoteNameLen = 0;
	
	if((param->params.remDevName.nameLen) && (param->params.remDevName.name))
	{
		if(param->params.remDevName.nameLen < BT_NAME_SIZE)
		{
			btManager.remoteNameLen = param->params.remDevName.nameLen;
		}
		else
		{
			btManager.remoteNameLen = BT_NAME_SIZE;
		}
		memcpy(btManager.remoteName, param->params.remDevName.name, btManager.remoteNameLen);
	}
	BT_DBG("\t nameLen = %d , name = %s \n",btManager.remoteNameLen, btManager.remoteName);

#ifdef FLASH_SAVE_REMOTE_BT_NAME
	//if(!sys_parameter.bt_SimplePairingEnable)
		BtDdb_UpdateDeviceName(param->params.remDevName.addr,param->params.remDevName.name);
#endif

#if BT_SOURCE_SUPPORT
	BtSourceGetRemoteName(param);
#endif

	if((param->params.remDevName.name[0] == 'M')
		&& (param->params.remDevName.name[1] == 'V')
		&& (param->params.remDevName.name[2] == '_')
		&& (param->params.remDevName.name[3] == 'B')
		&& (param->params.remDevName.name[4] == 'T')
		&& (param->params.remDevName.name[5] == 'B')
		&& (param->params.remDevName.name[6] == 'O')
		&& (param->params.remDevName.name[7] == 'X')
		)
	{
        if(btManager.btLinkState)
        {
    	    BT_DBG("device is connected, disconnect btbox\n");
    		btManager.btLastAddrUpgradeIgnored = 0;
            BTHciDisconnectCmd(param->params.remDevName.addr);
        }
        else
        {
    		BT_DBG("connect btbox\n");
    		btManager.btLastAddrUpgradeIgnored = 1;
			memset(btManager.btDdbLastAddr, 0, 6);
        }
	}

}

/*****************************************************************************************
 * 蓝牙page连接超时
 ****************************************************************************************/
void BtLinkPageTimeout(BT_STACK_CALLBACK_PARAMS * param)
{
	BtStackServiceMsgSend(MSG_BTSTACK_RECONNECT_REMOTE_PAGE_TIMEOUT);
#if BT_TWS_SUPPORT
	BtStackServiceMsgSend(MSG_BTSTACK_RECONNECT_TWS_PAGE_TIMEOUT);
#endif
}

/*****************************************************************************************
 * 蓝牙连接异常断开
 ****************************************************************************************/
/*void BtDevConnectionAborted(void)
{
	//在L2CAP异常断开所有协议时，开始启动回连机制,30s超时
	btEventListB1State = 0;
	{
		if(GetBtPlayState() == 1)
			btEventListB1State = 1;
	}

	btCheckEventList |= BT_EVENT_L2CAP_LINK_DISCONNECT;
	btEventListB1Count = btEventListCount;
	btEventListB1Count += 30000;
}*/

/*****************************************************************************************
 * 蓝牙回连流程失败
 * 条件: 手机端取消配对记录,手机端反馈认证失败
 * 处理方式: 取消回连流程
 ****************************************************************************************/
void BtPairingFail(void)
{
	//手机端取消配对，系统开机回连，手机端会反馈该msg，回复认证失败
	//可以停止回连，并可选择清除配对记录
	BtReconnectDevStop();
}

/*****************************************************************************************
 * 蓝牙连接丢失
 * 条件: 手机端拉远后
 ****************************************************************************************/
void BtDevConnectionLinkLoss(BT_STACK_CALLBACK_PARAMS * param)
{
#if (BT_TWS_SUPPORT == ENABLE)
	BT_DBG("LINK LOST\n");
#else
	//connection timeout
	if(param->errorCode == 0x08)
	{
		BT_DBG("BB LOST\n");
#endif
	if(!btManager.btLastAddrUpgradeIgnored)
	{
#if (BT_TWS_SUPPORT == ENABLE)
		//tws已经连接,并且为从,则不能回连主机
		if((btManager.twsState == BT_TWS_STATE_CONNECTED)&&(btManager.twsRole == BT_TWS_SLAVE))
			return;
#endif

		if(sys_parameter.bt_BBLostReconnectionEnable)
		{
			//bb lost: reconnect device
			BtReconnectDevCreate(btManager.btDdbLastAddr, sys_parameter.bt_BBLostTryCounts, sys_parameter.bt_BBLostInternalTime, 1000, btManager.btDdbLastProfile);
		}
	}

	btManager.btLastAddrUpgradeIgnored = 0;

#if (BT_TWS_SUPPORT == DISABLE)
	}
#endif //#ifndef BT_TWS_SUPPORT
}

/***********************************************************************************
 * 获取当前系统配置的支持蓝牙协议
 **********************************************************************************/
uint32_t GetSupportProfiles(void)
{
	uint32_t		profiles = 0;
	
#if BT_HFP_SUPPORT
	profiles |= BT_PROFILE_SUPPORTED_HFP;
#endif

#if (BT_A2DP_SUPPORT)
	profiles |= BT_PROFILE_SUPPORTED_A2DP;
#endif

#if (BT_AVRCP_SUPPORT)
	profiles |= BT_PROFILE_SUPPORTED_AVRCP;
#endif

#if (BT_SPP_SUPPORT)
	profiles |= BT_PROFILE_SUPPORTED_SPP;
#endif
	
#if BT_HID_SUPPORT
	profiles |= BT_PROFILE_SUPPORTED_HID;
#endif
	
#if BT_MFI_SUPPORT
	profiles |= BT_PROFILE_SUPPORTED_MFI;
#endif
	
#if BT_OBEX_SUPPORT
	profiles |= BT_PROFILE_SUPPORTED_OBEX;
#endif

#if BT_PBAP_SUPPORT
	profiles |= BT_PROFILE_SUPPORTED_PBAP;
#endif

#if (BT_TWS_SUPPORT)
	profiles |= BT_PROFILE_SUPPORTED_TWS;
#endif

	return profiles;
}
#if 0
/***********************************************************************************
 * 蓝牙已连接协议管理
 * 设置已连接的协议
 **********************************************************************************/
void SetBtConnectedProfile(uint16_t connectedProfile)
{
	btManager.btConnectedProfile |= connectedProfile;

	SetBtCurConnectFlag(1);

	if(btManager.btLastAddrUpgradeIgnored)
		return;
	
	//连接到一个蓝牙设备的一个profile时,就更新最后1次连接的蓝牙记录信息
	if(GetNumOfBtConnectedProfile() == 1)
	{
		//配对地址一致,不需要重复保存
		if(memcmp(btManager.btDdbLastAddr, btManager.remoteAddr, 6) == 0)
		{
			if(btManager.btDdbLastProfile & connectedProfile)
				return ;
		}
		
		btManager.btDdbLastProfile = (uint8_t)btManager.btConnectedProfile;

		//BP10主动回连上次连接的设备，被新的手机连接，发现MAC地址不一致，则停止回连
		BtReconnectDevStop();

		memcpy(btManager.btDdbLastAddr, btManager.remoteAddr, 6);
		BtDdb_UpgradeLastBtAddr(btManager.remoteAddr, btManager.btDdbLastProfile);
	}
	else
	{
		if((btManager.btDdbLastProfile & connectedProfile) == 0 )
		{
			btManager.btDdbLastProfile |= connectedProfile;
#ifdef CFG_FUNC_OPEN_SLOW_DEVICE_TASK
			{
			extern void SlowDevice_MsgSend(uint16_t msgId);
			SlowDevice_MsgSend(MSG_DEVICE_BT_LINKED_PROFILE_UPDATE);
			}
#else
			BtDdb_UpgradeLastBtProfile(btManager.remoteAddr, btManager.btDdbLastProfile);
#endif
		}
	}
}


/***********************************************************************************
 * 获取已断开的协议
 **********************************************************************************/
void SetBtDisconnectProfile(uint16_t disconnectProfile)
{
	btManager.btConnectedProfile &= ~disconnectProfile;

	if(!btManager.btConnectedProfile)
	{
		SetBtCurConnectFlag(0);
	}
}
#endif
/***********************************************************************************
 * 获取已连接的协议信息
 **********************************************************************************/
uint16_t GetBtConnectedProfile(void)
{
	return btManager.btConnectedProfile;
}

/***********************************************************************************
 * 获取已连接的协议个数
 **********************************************************************************/
uint8_t GetNumOfBtConnectedProfile(void)
{
	uint8_t number_of_profile = 0;
	uint8_t i;

	//根据常用的A2DP/AVRCP/HFP三个协议,确认当前已连接的协议状态
	for(i=0;i<3;i++)
	{
		if((btManager.btConnectedProfile >> i)&0x01)
		{
			number_of_profile++;
		}
	}

	return 	number_of_profile;
}

/***********************************************************************************
 * get device name (max 40bytes)
 * 获取本地传统蓝牙设备名称
 **********************************************************************************/
uint8_t* BtDeviceNameGet(void)
{
	return &btStackConfigParams->bt_LocalDeviceName[0];
}

/***********************************************************************************
 * 更新 蓝牙名称到flash
 **********************************************************************************/
int32_t BtDeviceNameSet(void)
{
	BT_DBG("device name set!\n");

	return BtDeviceSaveNameToFlash(sys_parameter.bt_LocalDeviceName,strlen(sys_parameter.bt_LocalDeviceName),0);

}

/***********************************************************************************
 * pin code get (max: 16bytes)
 * 获取pin code信息
 **********************************************************************************/
uint8_t* BtPinCodeGet(void)
{
	return btManager.bt_pinCode;
}

/***********************************************************************************
 * save pin code to flash
 **********************************************************************************/
int32_t BtPinCodeSet(uint8_t *pinCode)
{
	//kk：pin code 保存位置需要调整到系统参数区
	// BT_CONFIGURATION_PARAMS 	*btParams = NULL;
	// int8_t ret=0;
	// BT_DBG("pin code set!\n");

	// //1.是否开启简易配对
	// if(btStackConfigParams->bt_simplePairingFunc)
	// 	return -3;//模式不对

	// //2.申请RAM
	// btParams = (BT_CONFIGURATION_PARAMS*)osPortMalloc(sizeof(BT_CONFIGURATION_PARAMS));
	// if(btParams == NULL)
	// {
	// 	BT_DBG("ERROR: Ram is not enough!\n");
	// 	return -2;//RAM不够
	// }
	// memcpy(btParams, btStackConfigParams, sizeof(BT_CONFIGURATION_PARAMS));
	
	// //3.将pin code更新
	// memcpy(btStackConfigParams->bt_pinCode, pinCode, 4);
	// memcpy(btParams->bt_pinCode, pinCode, 4);

	// //4.将更新数据保存到flash
	// BtDdb_SaveBtConfigurationParams(btParams);
	// memset(btParams, 0, sizeof(BT_CONFIGURATION_PARAMS));

	// //5.重新从flash中读取数据，再次进行对比
	// ret = BtDdb_LoadBtConfigurationParams(btParams);
	// if(ret == -3)
	// {
	// 	//读取异常，read again
	// 	ret = BtDdb_LoadBtConfigurationParams(btParams);
	// 	if(ret == -3)
	// 	{
	// 		BT_DBG("bt database read error!\n");
	// 		osPortFree(btParams);
	// 		return -3;//读取失败
	// 	}
	// }

	// ret = memcmp(btStackConfigParams, btParams,sizeof(BT_CONFIGURATION_PARAMS));
	// if(ret == 0)
	// {
	// 	BT_DBG("save ok!\n");
	// 	osPortFree(btParams);
	// 	return 0;
	// }
	// else
	// {
	// 	BT_DBG("save NG!\n");
	// 	osPortFree(btParams);
	// 	return -4;//保存失败
	// }
	return 0;
}

/***********************************************************************************
 * @brief:  蓝牙事件处理
 **********************************************************************************/
void BtEventFlagRegister(uint32_t SoftEvent, uint32_t SoftTimeOut)
{
	btManager.btEventFlagMask = SoftEvent;
	btManager.btEventFlagCount = SoftTimeOut;
}

void BtEventFlagDeregister(uint32_t SoftEvent)
{
	if(btManager.btEventFlagMask == SoftEvent)
	{
		btManager.btEventFlagMask = BT_EVENT_FLAG_NONE;
		btManager.btEventFlagCount = 0;
	}
}

void BtEventFlagProcess(void)
{
	if(btManager.btEventFlagMask)
	{
		if(BT_EVENT_FLAG_AVRCP_CONNECT == btManager.btEventFlagMask)
		{
			if(btManager.btEventFlagCount)
			{
				btManager.btEventFlagCount--;
				if(btManager.btEventFlagCount == 0)
				{
					if((GetAvrcpState(BtCurIndex_Get()) < BT_AVRCP_STATE_CONNECTED)&&(GetA2dpState(BtCurIndex_Get()) >= BT_A2DP_STATE_CONNECTED))
					{
						BT_DBG("+++ avrcp connect \n");
						BtAvrcpConnect(BtCurIndex_Get(),btManager.remoteAddr);
					}
					btManager.btEventFlagMask = BT_EVENT_FLAG_NONE;
				}
			}
			else
			{
				btManager.btEventFlagCount = 0;
				btManager.btEventFlagMask = BT_EVENT_FLAG_NONE;
			}
		}
		else if(BT_EVENT_FLAG_AVRCP_DISCONNECT == btManager.btEventFlagMask)
		{
			if(btManager.btEventFlagCount)
			{
				btManager.btEventFlagCount--;
				if(btManager.btEventFlagCount == 0)
				{
					if(GetAvrcpState(BtCurIndex_Get()) > BT_AVRCP_STATE_NONE)
					{
						BT_DBG("--- avrcp disconnect \n");
						AvrcpDisconnect(BtCurIndex_Get());
					}
					btManager.btEventFlagMask = BT_EVENT_FLAG_NONE;
				}
			}
			else
			{
				btManager.btEventFlagCount = 0;
				btManager.btEventFlagMask = BT_EVENT_FLAG_NONE;
			}
		}
		else if(BT_EVENT_FLAG_A2DP_DISCONNECT  == btManager.btEventFlagMask)
		{
			if(btManager.btEventFlagCount)
			{
				btManager.btEventFlagCount--;
				if(btManager.btEventFlagCount == 0)
				{
					if(GetA2dpState(BtCurIndex_Get()) > BT_A2DP_STATE_NONE)
					{
						BT_DBG("--- a2dp disconnect \n");
						A2dpDisconnect(BtCurIndex_Get());
					}
					btManager.btEventFlagMask = BT_EVENT_FLAG_NONE;
				}
			}
			else
			{
				btManager.btEventFlagCount = 0;
				btManager.btEventFlagMask = BT_EVENT_FLAG_NONE;
			}
		}
	}
}



/*****************************************************************************************
 * TWS连接丢失
 ****************************************************************************************/
#if (BT_TWS_SUPPORT == ENABLE)
void BtTwsConnectionLinkLoss(void)
{
	BT_DBG("[TWS]LINK LOST\n");

#ifdef BT_SNIFF_ENABLE
	{
		extern void sniff_lmpsend_set(uint8_t set);
		sniff_lmpsend_set(0);
	}
#endif

#if (TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)
	if(GetSystemMode() == ModeTwsSlavePlay)
	{
//		SendSysModeMsg(MSG_BT_TWS_LINKLOSS);
		return;
	}
#endif

#if (TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER)
	if(btManager.twsRole == BT_TWS_SLAVE)
	{
		BtTwsLinkLoss();
	}
#else
	BtTwsLinkLoss();
#endif
}
/*****************************************************************************************
 * TWS连接超时(当次page过程)
 ****************************************************************************************/
void BtTwsPairingTimeout(void)
{
	CheckBtTwsPairing();
}
#endif



#endif

