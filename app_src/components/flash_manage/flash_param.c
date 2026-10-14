#include "type.h"
#include "sys_param.h"
#include "flash_param.h"
#include "bt_config.h"

#if (BT_SUPPORT == ENABLE)

const FLASH_PARAMETER SysDefaultParm =
{
	.SysVer = {SYS_PARA_VER_ID,sizeof(SysDefaultParm.SysVer.name),
			"Ver 1.0.0"},  //版本

	.BtName = {BT_PARA_BT_NAME_ID,sizeof(SysDefaultParm.BtName.name),
			BT_NAME},	//蓝牙名称
	.BleName = {BT_PARA_BLE_NAME_ID,sizeof(SysDefaultParm.BleName.name),
			BLE_NAME},	//BLE蓝牙名称
	.bt_TxPowerLevel = {BT_PARA_RF_TX_LEVEL_ID,sizeof(SysDefaultParm.bt_TxPowerLevel.para_val),
			BT_TX_POWER_LEVEL},//蓝牙正常工作时发射功率
	.bt_PagePowerLevel = {BT_PARA_RF_PAGE_LEVEL_ID,sizeof(SysDefaultParm.bt_PagePowerLevel.para_val),
			BT_PAGE_TX_POWER_LEVEL},//蓝牙回连发射功率

	.BtTrim = {BT_PARA_TRIM_VAL_ID,sizeof(SysDefaultParm.BtTrim.para_val),
			BT_DEFAULT_TRIM},//trim值
	.bt_CallinRingType = {BT_PARA_CallinRingType_ID,sizeof(SysDefaultParm.bt_CallinRingType.para_val),
			SYS_DEFAULT_RING_TYPE},
	.bt_BackgroundType = {BT_PARA_BackgroundType_ID,sizeof(SysDefaultParm.bt_BackgroundType.para_val),
			SYS_BT_BACKGROUND_TYPE},

	.bt_SimplePairingEnable = {BT_PARA_SimplePairingEnable_ID,sizeof(SysDefaultParm.bt_SimplePairingEnable.para_val),
			BT_SIMPLEPAIRING_FLAG},// SIMPLEPAIRING 开启
	.bt_PinCode = {BT_PARA_PinCode_ID,sizeof(SysDefaultParm.bt_PinCode.code),
			BT_PINCODE},// Pin code设置

	.bt_Reconnection =  {BT_PARA_ReconnectionEnable_ID,sizeof(SysDefaultParm.bt_Reconnection.para),
			{BT_RECONNECTION_SUPPORT,BT_POR_TRY_COUNTS,BT_POR_INTERNAL_TIME,
			 BT_BLR_RECONNECTION_SUPPORT,BT_BLR_TRY_COUNTS,BT_BLR_INTERNAL_TIME}},
							//参数1 BT自动重连(开机或者切换模式)  --- 1 开启
							//参数2  自动重连尝试次数  --- 5次
							//参数3  自动重连每两次间隔时间(in seconds) --- 间隔3S
							//参数4 BB Lost之后自动重连 1-> 打开/0->关闭  --- 1 打开
							//参数5 BB Lost 尝试重连次数 --- 90次
							//参数6 BB Lost 重连每两次间隔时间(in seconds) --- 间隔5S

	.TwsVolSyncEnable =  {TWS_PARA_TWS_VOL_SYNC_ID,sizeof(SysDefaultParm.TwsVolSyncEnable.para_val),
			TWS_VOL_SYNC_SUPPORT},//开启tws 主从之间音量控制同步

	.bt_TwsReconnection =  {TWS_PARA_ReconnectionEnable_ID,sizeof(SysDefaultParm.bt_TwsReconnection.para),
			{TWS_RECONNECTION_SUPPORT,TWS_POR_TRY_COUNTS,TWS_POR_INTERNAL_TIME,
			 TWS_BLR_RECONNECTION_SUPPORT,TWS_BLR_TRY_COUNTS,TWS_BLR_INTERNAL_TIME}},
							//参数1 tws自动重连(开机或者切换模式) --- 1 开启
							//参数2 tws自动重连尝试次数 --- 3次
							//参数3 tws自动重连每两次间隔时间(in seconds) --- 间隔3S
							//参数4 twsBB Lost之后自动重连 1-> 打开/0->关闭 --- 1 打开
							//参数5 twsBB Lost 尝试重连次数 --- 3次
							//参数6 twsBB Lost 重连每两次间隔时间(in seconds) --- 间隔5S

	.bt_TwsPairingWhenPhoneConnectedSupport =  {TWS_PARA_TwsPairingWhenPhoneConnectedSupport_ID,sizeof(SysDefaultParm.bt_TwsPairingWhenPhoneConnectedSupport.para_val),
			TWS_PAIRING_WHEN_PHONE_CONNECTED_SUPPORT}, //0 -> 手机连接时，该音箱无法发起组队
	.bt_TwsConnectedWhenActiveDisconSupport =  {TWS_PARA_TwsConnectedWhenActiveDisconSupport_ID,sizeof(SysDefaultParm.bt_TwsConnectedWhenActiveDisconSupport.para_val),
			TWS_CONNECTED_WHEN_ACTIVE_DISCONNECTION_SUPPORT}, //0 -> 用户主动断开TWS配对后，下次开机能再回连

	.ble_TxPowerLevel = {BLE_PARA_RF_TX_LEVEL_ID,sizeof(SysDefaultParm.ble_TxPowerLevel.para_val),
			BLE_TX_POWER_LEVEL},//BLE正常工作时发射功率			
};

#endif

