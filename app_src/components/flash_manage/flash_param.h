#ifndef __SYS_PARAMETER_H__
#define __SYS_PARAMETER_H__

typedef enum _SYS_PARAMETER_ID_
{
	//BT 部分
	BT_PARA_BT_NAME_ID 	= 0,		//蓝牙名称
	BT_PARA_BLE_NAME_ID,			//BLE蓝牙名称
	BT_PARA_RF_TX_LEVEL_ID,			//蓝牙正常工作时发射功率
	BT_PARA_RF_PAGE_LEVEL_ID,		//蓝牙回连发射功率
	BT_PARA_TRIM_VAL_ID,			//trim值
	BT_PARA_CallinRingType_ID,		//bt 铃声设置
	BT_PARA_BackgroundType_ID,		//BT 后台设置
	BT_PARA_SimplePairingEnable_ID,	// SIMPLEPAIRING 开启/关闭
	BT_PARA_PinCode_ID,				// Pin code设置
	BT_PARA_ReconnectionEnable_ID, 	//参数1 BT自动重连(开机或者切换模式)
									//参数2  自动重连尝试次数
									//参数3  自动重连每两次间隔时间(in seconds)
	 								//参数4 BB Lost之后自动重连 1-> 打开/0->关闭
	 								//参数5 BB Lost 尝试重连次数
	 								//参数6 BB Lost 重连每两次间隔时间(in seconds)
	BLE_PARA_RF_TX_LEVEL_ID,		//BLE正常工作时发射功率									

	//tws部分
	TWS_PARA_TWS_VOL_SYNC_ID = 0x100,		//tws 主从之间音量控制同步
	TWS_PARA_ReconnectionEnable_ID, 		//参数1 tws自动重连(开机或者切换模式)
											//参数2 tws自动重连尝试次数
											//参数3 tws自动重连每两次间隔时间(in seconds)
	 										//参数4 twsBB Lost之后自动重连 1-> 打开/0->关闭
	 										//参数5 twsBB Lost 尝试重连次数
	 										//参数6 twsBB Lost 重连每两次间隔时间(in seconds)
	TWS_PARA_TwsPairingWhenPhoneConnectedSupport_ID,	//1 -> 表示手机连接时，该音箱可以发起组队
														//0 -> 表示手机连接时，该音箱无法发起组队
	TWS_PARA_TwsConnectedWhenActiveDisconSupport_ID,	//1 -> 表示用户主动断开TWS配对后，下次开机不能自动回连
														//0 -> 表示用户主动断开TWS配对后，下次开机能再回连

	//系统部分
	SYS_PARA_VER_ID = 0x200,


	//user
	USER_PARA_DEFAULT_ID = 0x1000,

}SYS_PARAMETER_ID;


typedef struct _BT_PARA_BT_NAME_
{
	uint16_t id;
	uint8_t  len;
	uint8_t  name[BT_NAME_SIZE];
}__attribute__((packed)) BT_PARA_BT_NAME;

typedef struct _BT_PARA_BT_PINCODE_
{
	uint16_t id;
	uint8_t  len;
	uint8_t  code[BT_PIN_CODE_LEN];
}__attribute__((packed)) BT_PARA_BT_PINCODE;

typedef struct _BT_PARA_Reconnection_
{
	uint16_t id;
	uint8_t  len;
	uint8_t  para[6];
}__attribute__((packed)) BT_PARA_Reconnection;


typedef struct _COMMON_PARA_TYPE_
{
	uint16_t id;
	uint8_t  len;
	uint16_t para_val;
}__attribute__((packed)) COMMON_PARA;


typedef struct _FLASH_PARAMETER_
{
	BT_PARA_BT_NAME 	SysVer;

	BT_PARA_BT_NAME 	BtName;
	BT_PARA_BT_NAME 	BleName;

	COMMON_PARA			bt_TxPowerLevel;
	COMMON_PARA			bt_PagePowerLevel;

	COMMON_PARA			BtTrim;
	COMMON_PARA			bt_CallinRingType;
	COMMON_PARA			bt_BackgroundType;
	COMMON_PARA			bt_SimplePairingEnable;

	BT_PARA_BT_PINCODE	bt_PinCode;


	BT_PARA_Reconnection bt_Reconnection;


	COMMON_PARA			TwsVolSyncEnable;
	BT_PARA_Reconnection bt_TwsReconnection;
	COMMON_PARA			bt_TwsPairingWhenPhoneConnectedSupport;
	COMMON_PARA			bt_TwsConnectedWhenActiveDisconSupport;

	COMMON_PARA			ble_TxPowerLevel;
}__attribute__((packed)) FLASH_PARAMETER;

#endif
