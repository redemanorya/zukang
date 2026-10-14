#ifndef _SYS_PARAM_H__
#define _SYS_PARAM_H__

#define BT_NAME_SIZE				40
#define BLE_NAME_SIZE				40

#define BT_PIN_CODE_LEN				8

typedef struct _SYS_PARAMETER_
{
	char	bt_LocalDeviceName[BT_NAME_SIZE];
	char	ble_LocalDeviceName[BLE_NAME_SIZE];
	uint8_t	bt_TxPowerLevel;
	uint8_t	bt_PagePowerLevel;
	uint8_t	BtTrim;
	uint8_t	TwsVolSyncEnable;
	uint8_t	bt_CallinRingType;
	uint8_t	bt_BackgroundType;
	uint8_t	bt_SimplePairingEnable;
	char	bt_PinCode[BT_PIN_CODE_LEN];
	uint8_t	bt_ReconnectionEnable;
	uint8_t	bt_ReconnectionTryCounts;
	uint8_t	bt_ReconnectionInternalTime;
	uint8_t	bt_BBLostReconnectionEnable;
	uint8_t	bt_BBLostTryCounts;
	uint8_t	bt_BBLostInternalTime;
	uint8_t	bt_TwsReconnectionEnable;
	uint8_t	bt_TwsReconnectionTryCounts;
	uint8_t	bt_TwsReconnectionInternalTime;
	uint8_t	bt_TwsBBLostReconnectionEnable;
	uint8_t	bt_TwsBBLostTryCounts;
	uint8_t	bt_TwsBBLostInternalTime;
	uint8_t	bt_TwsPairingWhenPhoneConnectedSupport;
	uint8_t	bt_TwsConnectedWhenActiveDisconSupport;
	uint8_t	ble_TxPowerLevel;
}SYS_PARAMETER;

extern SYS_PARAMETER sys_parameter;
extern void sys_parameter_init(void);

#endif


