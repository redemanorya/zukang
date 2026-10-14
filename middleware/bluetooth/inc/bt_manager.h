/**
 **************************************************************************************
 * @file    bt_manager.c
 * @brief   management of all bluetooth event and apis
 *
 * @author  Halley
 * @version V1.1.0
 *
 * $Created: 2016-07-18 16:24:11$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef _BT_MANAGER_H_
#define _BT_MANAGER_H_

#include <string.h>
#include "type.h"
#include "bt_config.h"
#include "bt_common_api.h"
#include "timeout.h"
#include "sys_param.h"
//#if (BT_TWS_SUPPORT == ENABLE)
#include "bt_tws_api.h"
//#endif




#if ((BT_SUPPORT == ENABLE) || (BLE_SUPPORT == ENABLE))


//上电时加载FLASH的配置参数
typedef struct _BT_CONFIGURATION_PARAMS
{
	//BT参量表中排序
	uint8_t			bt_LocalDeviceAddr[BT_ADDR_SIZE];//flash mac顺序: NAP-UAP-LAP (此变量用于保存flash的参数,不能随意修改)
	uint8_t			ble_LocalDeviceAddr[BT_ADDR_SIZE];
	uint8_t			bt_LocalDeviceName[BT_NAME_SIZE];
	uint8_t			ble_LocalDeviceName[BLE_NAME_SIZE];	//预留:现BLE name是通过BLE工具生成的广播包中包含
	uint8_t			bt_ConfigHeader[4];
	uint8_t			bt_trimValue;
	//unused param
	// uint8_t			bt_TxPowerValue;
	// uint32_t		bt_SupportProfile;
	// uint8_t			bt_simplePairingFunc;
	// uint8_t			bt_pinCode[17];//max len: 16
	// uint8_t			bt_pinCodeLen;
	// uint8_t			bt_reconFunc;
	// uint8_t			bt_reconDevNum;
	// uint8_t			bt_reconCount;
	// uint8_t			bt_reconIntervalTime;
	// uint8_t			bt_reconTimeout;
	// uint8_t			bt_accessMode;

	#if BT_SOURCE_SUPPORT
	uint32_t		bt_source_SupportProfile;
	#endif

}BT_CONFIGURATION_PARAMS;
typedef struct _BT_DB_RECORD
{
	uint8_t			bdAddr[BT_ADDR_SIZE];
	bool			trusted;
	uint8_t			linkKey[16];
	uint8_t			keyType;
	uint8_t			pinLen;
#ifdef FLASH_SAVE_REMOTE_BT_NAME
	uint8_t			bdName[BT_NAME_SIZE];
#endif
}BT_DB_RECORD;
#endif

#if (BT_SUPPORT == ENABLE)

#define MAX_PHONE_NUMBER_LENGTH		20

#define BT_LINK_KEY_LEN				16

/***********************************************************************************
 *
 **********************************************************************************/
//btManager.btEventFlagMask
#define BT_EVENT_FLAG_NONE					0x00
#define BT_EVENT_FLAG_AVRCP_DISCONNECT		0x01	//在A2DP断开后,设置AVRCP超时断开(5s)
#define BT_EVENT_FLAG_L2CAP_LINK_DISCONNECT	0x02	//在L2CAP处理异常时断开，需要发起重连，并不播放提示音，连接成功后自动播放
#define BT_EVENT_FLAG_AVRCP_CONNECT			0x03	//在连接A2DP后,主动发起AVRCP连接(50ms)
#define BT_EVENT_FLAG_A2DP_DISCONNECT		0x04	//在AVRCP后,超时断开(1s)


/***********************************************************************************
 *
 **********************************************************************************/
typedef uint8_t (*Name_Confirm_Callback_t)(uint8_t *name);
typedef uint8_t (*Other_Confirm_Callback_t)(void);
// 如果需要过滤某些手机不让他们连接我们板子，bt_stack_server初始化时候，设置函数指针给底层去做判定条件
// 具体的判决内容在函数BtConnectDecide中实现
extern uint8_t Name_confirm_Callback_Set(Name_Confirm_Callback_t callback);
// CFG_TWS_ONLY_IN_BT_MODE打开后测试快速插卡有可能导致从机有声音，用下列回调实现过滤
// 具体的判决内容在函数BtConnectConfirm中实现
extern uint8_t Other_confirm_Callback_Set(Other_Confirm_Callback_t callback);

/***********************************************************************************
 * 鉴权相关函数接口
 **********************************************************************************/
//用作鉴权连接的底层函数接口
typedef void (*SEC_Confirm_Callback_t)(uint32_t val);
extern void SEC_Confirm_Callback_Set(SEC_Confirm_Callback_t callback);

// 使用参考:
// 1. 打开鉴权功能，可以在BtStackServiceEntrance()
// BtNumericalDisplayFuncEnable(1)
// 2. 设置callback，可以在BtStackServiceEntrance()
//SEC_Confirm_Callback_Set(BtNumericalVerify);
// 3. 实现回调函数BtNumericalVerify，如下就是打印出远程设备发过来的鉴权号码

//接受远程设备的鉴权请求
extern void BtNumericalAccecpt(void);
//拒绝远程设备的鉴权请求
extern void BtNumericalReject(void);

/***********************************************************************************
 *
 **********************************************************************************/
#define MAX_FLASH_DDB_DEVICE_NUM	9

typedef enum
{
	BT_STACK_STATE_NONE,
	BT_STACK_STATE_INITAILIZING,
	BT_STACK_STATE_READY	
} BT_STACK_STATE;

//bt reset state
typedef enum
{
	BT_RST_STATE_NONE,
	BT_RST_STATE_START,
	BT_RST_STATE_WAITING,
	BT_RST_STATE_FINISHED,
} BT_RST_STATE;

//scan page state 
typedef enum
{
	BT_SCAN_PAGE_STATE_NONE,
	BT_SCAN_PAGE_STATE_CLOSING,
	BT_SCAN_PAGE_STATE_DISCONNECTING,
	BT_SCAN_PAGE_STATE_DISABLE,
	BT_SCAN_PAGE_STATE_OPENING,
	BT_SCAN_PAGE_STATE_ENABLE,
	BT_SCAN_PAGE_STATE_SNIFF,	
	BT_SCAN_PAGE_STATE_OPEN_WAITING,
} BT_SCAN_PAGE_STATE;

/**
* Bt device connectable mode. 
*/
typedef enum
{
	BT_DEVICE_CONNECTION_MODE_NONE,				/* cann't be discovered and connected*/
	BT_DEVICE_CONNECTION_MODE_DISCOVERIBLE, 	/* can be discovered*/
	BT_DEVICE_CONNECTION_MODE_CONNECTABLE,		/* can be connected*/
	BT_DEVICE_CONNECTION_MODE_ALL,				/* can be discovered and connected*/
} BT_DEVICE_CONNECTION_MODE;

typedef enum 
{
	BT_HFP_STATE_NONE,
	BT_HFP_STATE_CONNECTING,
	BT_HFP_STATE_CONNECTED,
	BT_HFP_STATE_INCOMING,
	BT_HFP_STATE_OUTGOING,
	BT_HFP_STATE_ACTIVE,
	BT_HFP_STATE_3WAY_INCOMING_CALL,		//1CALL ACTIVE, 1CALL INCOMING
	BT_HFP_STATE_3WAY_OUTGOING_CALL,		//1CALL ACTIVE, 1CALL OUTGOING
	BT_HFP_STATE_3WAY_ATCTIVE_CALL			//2CALL ACTIVE
} BT_HFP_STATE;

typedef enum
{
	BT_A2DP_STATE_NONE,
	BT_A2DP_STATE_CONNECTING,
	BT_A2DP_STATE_CONNECTED,
	BT_A2DP_STATE_STREAMING,
} BT_A2DP_STATE;

typedef enum
{
	BT_AVRCP_STATE_NONE,
	BT_AVRCP_STATE_CONNECTING,
	BT_AVRCP_STATE_CONNECTED,	
} BT_AVRCP_STATE;

typedef enum
{
	BT_SPP_STATE_NONE,
	BT_SPP_STATE_CONNECTING,
	BT_SPP_STATE_CONNECTED,
} BT_SPP_STATE;

typedef enum
{
	BT_OBEX_STATE_NONE,
	BT_OBEX_STATE_CONNECTING,
	BT_OBEX_STATE_CONNECTED,
} BT_OBEX_STATE;

typedef enum
{
	BT_PBAP_STATE_NONE,
	BT_PBAP_STATE_CONNECTING,
	BT_PBAP_STATE_CONNECTED,
} BT_PBAP_STATE;

typedef enum
{
	BT_HID_STATE_NONE,
	BT_HID_STATE_CONNECTING,
	BT_HID_STATE_CONNECTED,
} BT_HID_STATE;

typedef enum
{
	BT_MFI_STATE_NONE,
	BT_MFI_STATE_CONNECTING,
	BT_MFI_STATE_CONNECTED,
} BT_MFI_STATE;

typedef enum{
	BT_USER_STATE_NONE,
	BT_USER_STATE_RECON,
	BT_USER_STATE_PREPAIR,
	BT_USER_STATE_CONNECTED,
	BT_USER_STATE_DISCONNECTED,
} BT_USER_STATE;
void SetBtUserState(BT_USER_STATE bt_state);
BT_USER_STATE GetBtUserState(void);

typedef enum 
{
	BT_BIS_STATE_NONE,						//IDLE
	BT_BIS_STATE_SRC_START,
	BT_BIS_STATE_SRC_START_WAITING,
	BT_BIS_STATE_SRC_STREAMED,
	BT_BIS_STATE_SRC_STOP,
	BT_BIS_STATE_SRC_STOP_WAITING,
	BT_BIS_STATE_SNK_START,
	BT_BIS_STATE_SNK_START_WAITING,
	BT_BIS_STATE_SNK_STEAMED,
	BT_BIS_STATE_SNK_STOP,
	BT_BIS_STATE_SNK_STOP_WAITING,
	BT_BIS_STATE_SNK_START_WAITING_SINK_MODE,	//等待系统模式 sink mode切换完成
} BT_BIS_STATE;



#if	BT_SOURCE_SUPPORT
typedef struct _BT_SOURCE_INQUIRY_LIST
{
	uint8_t		flag; 					//1=used
	uint8_t		addr[6];
	uint8_t		nameFlag;
	uint8_t		name[40];
	uint8_t		classOfDev[3];
	int8_t		rssi;
}BT_SOURCE_INQUIRY_LIST;
#endif

typedef uint8_t TIMER_FLAG;
#define TIMER_UNUSED					0x00
#define TIMER_USED						0x01
#define TIMER_STARTED					0x02
#define TIMER_WAITING					0x04

typedef struct _BT_TIMER
{
	TIMER_FLAG	timerFlag;
	TIMER		timerHandle;
}BT_TIMER;






typedef struct _BT_LINK_DEVICE_INFO
{
	BT_DB_RECORD	device;
	uint8_t			UsedFlag;

	uint8_t			tws_role;
	uint8_t			remote_profile;

}BT_LINK_DEVICE_INFO;

//a2dp stream type
#define  BT_A2DP_STREAM_TYPE_SBC	0 //defined
#define  BT_A2DP_STREAM_TYPE_AAC	1

/*******************************************************************************
 *  reconnect struct
 *******************************************************************************/
#include "bt_engine_utility.h"
//蓝牙回连执行函数指针
typedef void (*FUNC_RECONNECTION_EXCUTE)(void);
void BtAppiFunc_ReconnectionExcute(FUNC_RECONNECTION_EXCUTE CallbackFunc);
extern FUNC_RECONNECTION_EXCUTE BtReconnectExcute;
typedef struct _BT_RECONNECT_ST
{
	btstack_item_t				item;
	
	FUNC_RECONNECTION_EXCUTE	excute;				//执行函数指针
	BT_TIMER					ConnectionTimer;	//定时器
	uint32_t					profile;			//需要连接的协议
	uint8_t						RemoteDevAddr[BT_ADDR_SIZE];	//连接的地址
	uint8_t						TryCount;			//重复次数
	uint8_t						IntervalTime;		//间隔时间
	uint32_t					DelayStartTime;		//延时开始处理,只有效一次 unit:ms
}BT_RECONNECT_ST;


/*******************************************************************************
 * 蓝牙双手机连接结构体
 *******************************************************************************/
typedef struct _BT_LINKED_ENV_TAG
{
	uint8_t					btLinkState; //0=disconnect; 1=connected
	uint8_t					btLinkedProfile; //已连接的协议
	uint8_t					remoteAddr[BT_ADDR_SIZE];
	uint8_t					remoteName[BT_NAME_SIZE];
	uint8_t					remoteNameLen;
#ifdef BT_MULTI_LINK_SUPPORT
	uint8_t					btLinkDisconnFlag;
#endif

	//A2DP
	BT_A2DP_STATE			a2dpState;
	uint8_t					a2dp_index;

	//AVRCP
	BT_AVRCP_STATE			avrcpState;				//link state
	uint8_t					avrcp_index;
	uint8_t					avrcpCurPlayState;		//play state

	//HFP
	BT_HFP_STATE			hfpState;
	uint8_t					hf_index;
	uint8_t					appleDeviceFlag;		//1=apple device; 0=other device
	
}BT_LINKED_ENV_TAG;

/*******************************************************************************
 * 蓝牙通话列表结构体
 *******************************************************************************/
typedef struct _BT_HF_CALL_LIST_ST
{
	/* Used: 0=unused, 1=used */
	uint8_t				 used;
	
    /* Dir: 0=outgoing, 1=incoming */
    uint8_t              dir;

    /* Call state. */
    uint8_t				state;
}BT_HF_CALL_LIST_ST;



/********************************************************************
 * @brief  Clear Bt Manager Register
 * @param  NONE
 * @return NONE
 * @Note
 *******************************************************************/
void ClearBtManagerReg(void);

/*******************************************************************
 * @brief  Get Bt Manager
 * @param  NONE
 * @return ptr is returned
 * @Note
 *******************************************************************/
#define GetBtManager() (&btManager)

/********************************************************************
 * @brief  Set Bt Stack State
 * @param  state: The state of bt stack.
 * @return NONE
 * @Note
 *******************************************************************/
void SetBtStackState(BT_STACK_STATE state);

/********************************************************************
 * @brief  Get Bt Stack State
 * @param  NONE
 * @return The state of bt stack.
 * @Note
 *******************************************************************/
BT_STACK_STATE GetBtStackState(void);

/********************************************************************
 * @brief  Set Bt Device Connection Mode
 * @param  mode: connection mode
 * @return TRUE: success
 * @Note
 *******************************************************************/
bool SetBtDeviceConnState(BT_DEVICE_CONNECTION_MODE mode);

/********************************************************************
 * @brief  Get Bt Device Connection State
 * @param  NONE
 * @return connection mode
 * @Note
 *******************************************************************/
BT_DEVICE_CONNECTION_MODE GetBtDeviceConnState(void);


/********************************************************************
* Bt device connected profile. 
*******************************************************************/
#define BT_CONNECTED_A2DP_FLAG			0x0001
#define BT_CONNECTED_AVRCP_FLAG			0x0002
#define BT_CONNECTED_HFP_FLAG			0x0004
#define BT_CONNECTED_MFI_FLAG			0x0008
#define BT_CONNECTED_SPP_FLAG			0x0010
#define BT_CONNECTED_OPP_FLAG			0x0020
#define BT_CONNECTED_HID_FLAG			0x0040

/********************************************************************
 * @brief  Set Bt Connected Profile
 * @param  index: 0 or 1
 * @param  connectedProfile: a2dp/avrcp/hfp/and so on
 * @return NONE
 * @Note   设置link已连接的profile
 *******************************************************************/
void SetBtConnectedProfile(uint8_t index, uint16_t connectedProfile);

/********************************************************************
 * @brief  Set Bt Disconnect Profile
 * @param  index: 0 or 1
 * @param  connectedProfile: a2dp/avrcp/hfp/and so on
 * @return NONE
 * @Note   设置link 断开的的profile
 *******************************************************************/
void SetBtDisconnectProfile(uint8_t index, uint16_t disconnectProfile);

/********************************************************************
 * @brief  Get Bt Connected Profile
 * @param
 * @return connectedProfile: a2dp/avrcp/hfp/and so on
 * @Note   获取蓝牙已经连接的profile
 *******************************************************************/
uint16_t GetBtConnectedProfile(void);

/********************************************************************
 * @brief  Get Number Of BtConnected Profile
 * @param
 * @return number
 * @Note   获取已经连接的协议的数量
 *******************************************************************/
uint8_t GetNumOfBtConnectedProfile(void);

/********************************************************************
 * @brief  Set Bt Current Connect Flag
 * @param
 * @return state
 * @Note
 *******************************************************************/
void SetBtCurConnectFlag(uint8_t state);

/********************************************************************
 * @brief  Get Bt Current Connect Flag
 * @param  state
 * @return NONE
 * @Note
 *******************************************************************/
uint8_t GetBtCurConnectFlag(void);

/********************************************************************
 * @brief  Bt Connect Decide
 * @param  addr - the remote address
 * @return flag
 * @Note   被手机主动连接时,通过地址,由应用层决定是否接受连接
 *******************************************************************/
uint8_t BtConnectDecide(uint8_t *Addr);

/********************************************************************
 * @brief	Bt Connect Confirm
 * @param	addr - the remote address
 * @return	flag
 * @Note	被手机主动连接时,通过地址,由应用层决定是否接受连接
 *			和 BtConnectDecide 类似
 *******************************************************************/
uint8_t BtConnectConfirm(void);

/********************************************************************
 * @brief  Bt Hfp Connect
 * @param  index: 0 or 1
 * @param  addr - the remote address
 * @return
 * @Note   连接HFP协议
 *******************************************************************/
signed char  BtHfpConnect(uint8_t index,uint8_t * addr);

/********************************************************************
 * @brief  disconnect the hfp profile
 * @param  index: 0 or 1
 * @return NONE
 * @Note   断开HFP协议
 *******************************************************************/
void BtHfpDisconnect(uint8_t index);

/********************************************************************
 * @brief  set the hfp profile state
 * @param  index: 0 or 1
 * @param  state
 * @return NONE
 * @Note   设置HFP协议连接状态
 *******************************************************************/
void SetHfpState(uint8_t index,BT_HFP_STATE state);

/********************************************************************
 * @brief  get the hfp profile state
 * @param  index: 0 or 1
 * @return state
 * @Note   获取HFP协议连接状态
 *******************************************************************/
BT_HFP_STATE GetHfpState(uint8_t index);

/********************************************************************
 * @brief  get the sco connect flag
 * @param  NONE
 * @return TRUE - connected
 * @Note
 *******************************************************************/
bool GetScoConnectFlag(void);
bool GetScoLinkConnectFlag(void);

/********************************************************************
 * @brief  get callin phone number
 * @param  index: 0 or 1
 * @param  phone number
 * @return state
 * @Note
 *******************************************************************/
int16_t GetBtCallInPhoneNumber(uint8_t index,uint8_t * number);

/********************************************************************
 * @brief  set the voice recognition flag
 * @param  flag
 * @return void
 * @Note
 *******************************************************************/
void SetBtHfpVoiceRecognition(bool flag);

/********************************************************************
 * @brief  open bt hfp voice recognition funciton
 * @param  index: 0 or 1
 * @return 
 * @Note   打开手机的语音助手功能
 *******************************************************************/
void OpenBtHfpVoiceRecognitionFunc(uint8_t index);

/********************************************************************
 * @brief  Get bt hfp voice recognition flag
 * @param  void
 * @return state
 * @Note   获取语音助手功能开启状态
 *******************************************************************/
bool GetBtHfpVoiceRecognition(void);

/********************************************************************
 * @brief  Set Bt Hfp Speaker Volume
 * @param  gain:
 * @return 
 * @Note   设置通话音量
 *******************************************************************/
int16_t SetBtHfpSpeakerVolume(uint8_t gain);

/********************************************************************
 * @brief	Set Sco Connect Flag
 * @param	flag:
 * @return 
 * @Note
 *******************************************************************/
void SetScoConnectFlag(bool flag);
void SetScoLinkConnectFlag(bool flag);

/********************************************************************
 * @brief  get the a2dp profile state
 * @param  index: 0 or 1
 * @return state
 * @Note
 *******************************************************************/
BT_A2DP_STATE GetA2dpState(uint8_t index);

#if	BT_SOURCE_SUPPORT

/********************************************************************
 * @brief  get the source a2dp profile state
 * @param  none
 * @return state
 * @Note
 *******************************************************************/
BT_A2DP_STATE GetSourceA2dpState(void);
#endif

/*********************************************************************
* @brief  SetBtA2dpSbcBitpool() - Set the SBC quality for A2DP streaming.
* @param  value: <= 53 (default = 53)
*			53: high-quality SBC settings(bitpool = 53)
*			35: middle-quality SBC settings(bitpool = 35)
* @return 1=success; 0=fail
* @Note   设置A2DP SBC音质
*********************************************************************/
uint8_t SetBtA2dpSbcBitpool(uint8_t value);

/********************************************************************
 * @brief  connect the a2dp profile 
 * @param  index: 0 or 1
 * @param  addr - the remote address
 * @return NONE
 * @Note
 *******************************************************************/
void BtA2dpConnect(uint8_t index,uint8_t *addr);

/********************************************************************
 * @brief  get the avrcp profile state
 * @param  index: 0 or 1
 * @return state
 * @Note
 *******************************************************************/
BT_AVRCP_STATE GetAvrcpState(uint8_t index);

/********************************************************************
 * @brief  connect the avrcp profile 
 * @param  index: 0 or 1
 * @param  addr - the remote address
 * @return NONE
 * @Note
 *******************************************************************/
void BtAvrcpConnect(uint8_t index, uint8_t *addr);

/********************************************************************
 * @brief  avrcp profile state
 * @param  index: 0 or 1
 * @return 0: avrcp disconnect
 *         1: avrcp is connected
 * @Note
 *******************************************************************/
 bool IsAvrcpConnected(uint8_t index);

/********************************************************************
 * @brief  timer start - get bt audio play status
 * @param  NONE
 * @return NONE
 * @Note
 *******************************************************************/
void TimerStart_BtPlayStatus(void);

/********************************************************************
 * @brief  timer stop - get bt audio play status
 * @param  NONE
 * @return NONE
 * @Note
 *******************************************************************/
void TimerStop_BtPlayStatus(void);

/********************************************************************
 * @brief  check timer start flag - get bt audio play status
 * @param
 * @return TRUE/FALSE
 * @Note
 *******************************************************************/
uint8_t CheckTimerStart_BtPlayStatus(void);

/********************************************************************
 * @brief	Bt Link State Disconnect
 * @param	flag: 1=connect
 * @param	index: 0 or 1
 * @return
 * @note	更新应用层管理蓝牙链路的连接状态
 *******************************************************************/
void BtLinkStateConnect(uint8_t flag, uint8_t index);

/********************************************************************
 * @brief	Bt Link State Disconnect
 * @param	index: 0 or 1
 * @return
 * @note	更新应用层管理蓝牙链路的断开状态
 *******************************************************************/
void BtLinkStateDisconnect(uint8_t index);

/********************************************************************
 * @brief	Get Bt Link State
 * @param	index: 0 or 1
 * @return	1=连接成功; 0=未连接
 * @note	获取应用层管理蓝牙连接状态
 *******************************************************************/
bool GetBtLinkState(uint8_t index);

/********************************************************************
 * @brief	Bt Connect Ctrl
 * @param
 * @return
 * @note	连接蓝牙
 *******************************************************************/
void BtConnectCtrl(void);

/********************************************************************
 * @brief	Bt Disconnect Ctrl
 * @param   0(断开，终止回连行为) or 1 (直接断开，不判断条件,不终止回连行为)
 * @return
 * @note	断开蓝牙
 *******************************************************************/
void BtDisconnectCtrl(bool OnlyDisconnect);

/********************************************************************
 * @brief	BtSetAccessMode_select
 * @param
 * @return
 * @note	开机蓝牙可见性配置：根据客户需求配置选择可见性
 *******************************************************************/
void BtSetAccessMode_select(void);

#ifdef BT_UNIFIED_CONTROL
/********************************************************************
 * @brief	BtSetPowerState
 * @param   enable true=开启蓝牙，false=关闭蓝牙
 * @return  NONE
 *******************************************************************/
void BtSetPowerState(bool enable);

/********************************************************************
 * @brief	BtIsSystemOn
 * @param   NONE
 * @return  true=蓝牙已开启，false=蓝牙已关闭
 *******************************************************************/
bool BtIsSystemOn(void);
#endif

/********************************************************************
 * @brief	Bt Address Is Valid
 * @param	1=valid; 0=invalid
 * @return
 * @note	判断地址是否有效
 *******************************************************************/
uint8_t BtAddrIsValid(uint8_t *addr);

/********************************************************************
 * @brief	get Bt Hfp Voice Recognition
 * @param
 * @return	flag: 1=enable
 * @note	获取当前是否开启语音识别功能
 *******************************************************************/
bool GetBtHfpVoiceRecognition(void);

/********************************************************************
 * @brief	Set Bt Hfp Voice Recognition
 * @param	flag: 1=enable
 * @return
 * @note	打开语音识别功能
 *******************************************************************/
void SetBtHfpVoiceRecognition(bool flag);

/********************************************************************
 * @brief	bt current index get
 * @param
 * @return	index: 0 or 1
 * @note
 *******************************************************************/
uint8_t BtCurIndex_Get(void);

/********************************************************************
 * @brief	bt current index set
 * @param	index: 0 or 1
 * @return
 * @note
 *******************************************************************/
void BtCurIndex_Set(uint8_t index);

/********************************************************************
 * @brief	bt manage initialize
 * @param
 * @return
 * @note
 *******************************************************************/
void BtManageInit(void);

/********************************************************************
 * @brief	first talking phone index set
 * @param	index: 0 or 1
 * @return
 * @note
 *******************************************************************/
void FirstTalkingPhoneIndexSet(uint8_t Index);

/********************************************************************
 * @brief	first talking phone index get
 * @param
 * @return	index: 0 or 1
 * @note
 *******************************************************************/
uint8_t FirstTalkingPhoneIndexGet(void);

/********************************************************************
 * @brief	second talking phone index set
 * @param	index: 0 or 1
 * @return
 * @note
 *******************************************************************/
void SecondTalkingPhoneIndexSet(uint8_t Index);

/********************************************************************
 * @brief	second talking phone index get
 * @param
 * @return	index: 0 or 1
 * @note
 *******************************************************************/
uint8_t SecondTalkingPhoneIndexGet(void);

/********************************************************************
 * @brief	set a2dp state
 * @param	index: 0 or 1
 * @param	state
 * @return
 * @note
 *******************************************************************/
void SetA2dpState(uint8_t index, BT_A2DP_STATE state);

/********************************************************************
 * @brief	set avrcp state
 * @param	index: 0 or 1
 * @param	state
 * @return
 * @note
 *******************************************************************/
void SetAvrcpState(uint8_t index, BT_AVRCP_STATE state);

/********************************************************************
 * @brief	change player
 * @param
 * @return
 * @note	双手机播放音乐,切换流程处理
 *******************************************************************/
void ChangePlayer(void);

/********************************************************************
 * @brief	set avrcp cur play state
 * @param	index: 0 or 1
 * @param	state
 * @return
 * @note
 *******************************************************************/
void SetAvrcpCurPlayState(uint8_t index, uint8_t state);

/********************************************************************
 * @brief	get avrcp cur play state
 * @param	index: 0 or 1
 * @return	state
 * @note
 *******************************************************************/
uint8_t GetAvrcpCurPlayState(uint8_t index);

/********************************************************************
 * @brief	get bt manager a2dp index
 * @param	index: 0 or 1
 * @return	index: a2dp link index
 * @note
 *******************************************************************/
uint8_t GetBtManagerA2dpIndex(uint8_t index);

/********************************************************************
 * @brief	get bt manager avrcp index
 * @param	index: 0 or 1
 * @return	index: avrcp link index
 * @note
 *******************************************************************/
uint8_t GetBtManagerAvrcpIndex(uint8_t index);

/********************************************************************
 * @brief	get bt manager hfp index
 * @param	index: 0 or 1
 * @return	index: hfp link index
 * @note
 *******************************************************************/
uint8_t GetBtManagerHfpIndex(uint8_t index);

void BtStopReconnect(void);


#if BT_SOURCE_SUPPORT
void BtconnectAvrcpStart(void);
void BtSourceInquiryResultParse(uint8_t *addr, uint16_t extRespLen, uint8_t *extResp, int8_t rssi, uint8_t *classOfDevice);
void BtSourceInquiryListClear(void);
uint8_t BtSourceInquiryListInsert(uint8_t *addr, uint16_t nameLen, uint8_t *name, uint8_t *classOfDev, int8_t rssi);
void BtSourceInquiryListNameUpdate(uint8_t *addr, uint16_t nameLen, uint8_t *name);
void BtSourceInquiryListNameFlagClear(uint8_t *addr);
void BtSourceInquiryListPrintf(void);
void BtSourceNameRequest(void);
void BtSourceNameMatch(void);

void BtSourceInquiryStart(void);
void BtSourceInquiryStop(void);

//HFG
void BtHfgCreateCall(void);
void BtHfgHangUp(void);
uint8_t SetPinCode(uint8_t *code,uint8_t Len,uint8_t *addr);
uint8_t RejectPinCode(uint8_t *addr);

BT_HFP_STATE GetSourceHfgState(void);

extern volatile uint32_t gSwitchSourceAndSink;

#endif

#endif
#if ((BT_SUPPORT == ENABLE) || (BLE_SUPPORT == ENABLE))

/*******************************************************************************
 *
 *******************************************************************************/
typedef struct _BT_MANAGER_ST
{
#if(BT_SUPPORT == ENABLE)
	uint8_t					cur_index;//当前使用的链路
	uint8_t					back_index;//备用的链路
	btstack_list_t			btReconHandle;		//回连的链表管理
	BT_RECONNECT_ST*		btReconExcuteSt;	//当前执行回连流程结构体指针
	BT_RECONNECT_ST			btReconPhoneSt;		//回连手机数据结构
	BT_RECONNECT_ST			btReconTwsSt;		//回连TWS数据结构
	uint32_t				btReconStopDelay;	//启动回连 btReconStopDelay时间内不停止回连
#endif

	uint8_t 				ddbUpdate;	//标识remote device information是否需要写入到FLASH
	uint8_t					btDevAddr[BT_ADDR_SIZE];//系统应用的蓝牙mac顺序: LAP-UAP-NAP
#if(BT_SUPPORT == ENABLE)
	BT_LINK_DEVICE_INFO		btLinkDeviceInfo[MAX_FLASH_DDB_DEVICE_NUM];//1(tws)+ 8(device)
	BT_STACK_STATE			stackState;
	BT_DEVICE_CONNECTION_MODE	deviceConMode;

	uint8_t 				btLinkState; //0=disconnect; 1=connected
	uint8_t					fristBtLinkState;	//手机刚连上标志,方便第一次avrcp控制有效
	//多链路
	uint8_t					linkedNumber;
	BT_LINKED_ENV_TAG		btLinked_env[BT_LINK_DEV_NUM];	//2条链路数据结构

	uint8_t					remoteAddr[BT_ADDR_SIZE];
	uint8_t					remoteName[BT_NAME_SIZE];
	uint8_t					remoteNameLen;
	uint8_t					btDdbLastAddr[BT_ADDR_SIZE];
	uint8_t					btDdbLastProfile;
	uint32_t				btDdbLastInfoOffset;

	uint8_t					bt_simplePairingFunc;
	uint8_t					bt_pinCode[17];//max len: 16
	uint8_t					bt_pinCodeLen;

	uint8_t					btAccessModeEnable; //1=初始化后自动进入可被搜索可被连接状态
	uint32_t				btConStateProtectCnt;//10s超时,用于保护当前的搜索、连接状态
	uint8_t					btLastAddrUpgradeIgnored; //1:用于测试盒校准频偏后,不更新最后一次连接设备信息

	//remote device
	uint8_t					btReconnectTryCount;
	uint8_t					btReconnectIntervalTime;
	BT_TIMER				btReconnectTimer;
	bool					BtPowerOnFlag;
	bool					btReconnectedFlag;
	uint32_t				btReconnectDelayCount;//上电后蓝牙初始化,在进入模式后再发起回连
	uint32_t				btReconProfilePriority;//默认回连流程 HFP->A2DP->AVRCP, 此寄存器可以注册修改回连顺序

	uint16_t				btConnectedProfile;

	uint8_t					btCurConnectFlag;
	uint8_t					btDisConnectingFlag;
	BT_SCAN_PAGE_STATE		btScanPageState;

	BT_RST_STATE			btRstState; 		//BT恢复出厂设置
	uint32_t				btRstWaitingCount;

	uint8_t					appleDeviceFlag;	//Note:此标志通过HFP来进行判断;           1=apple device; 0=other device

	//hfp
	bool					scoConnected; //app and mode state
	bool					scoLinkConnected; //sco link state
	uint8_t					phoneNumber[MAX_PHONE_NUMBER_LENGTH];
	bool					callWaitingFlag;
	uint8_t					batteryLevel;
	uint8_t					signalLevel;
	bool					voiceRecognition;
	uint8_t					volGain;
	uint8_t					hfpScoCodecType[BT_LINK_DEV_NUM];
	uint8_t					hfpVoiceState;		//1:非通话,其他应用使用了通话链路传输音频 0:正常通话
	uint8_t					hfpScoDiscIndex;	//sco断链index
	uint16_t				hfpScoDiscDelay;	//sco断链延时处理
#ifdef BT_HFP_BATTERY_SYNC
	uint8_t					HfpBatLevel;
#endif
	uint8_t		 			Esco_RePack[60];
	uint16_t				Esco_RePackLen;

	BT_HF_CALL_LIST_ST		hfpCallListParams[2];	//三方通话,手机反馈当前的通话链路状态
	uint8_t					HfpCurIndex;			//当前通话的index
	uint8_t					IndexSwFlag;			//双手机退出通话后，是否恢复之前活跃链路播放
	//a2dp
	uint8_t					a2dpStreamType[BT_LINK_DEV_NUM];//=sbc; =aac
	uint32_t				aacFrameNumber;			//保存接收到的aac帧数,用于管理decoder

	//avrcp
	uint16_t				avrcpMediaInforFlag;	//应用层主动获取歌词标志
	BT_TIMER				avrcpPlayStatusTimer;
#if (BT_AVRCP_VOLUME_SYNC)
	uint8_t					avrcpSyncEnable;
	uint8_t					avrcpSyncVol; 			//默认值0xff(未同步), 正常值范围(0-32)
#endif

	uint32_t				btDutModeEnable;		//DUT模式

	uint32_t				btEnterSniffStep;
	uint32_t				btExitSniffReconPhone;

	uint32_t				btEventFlagMask;		//连接标志
	uint32_t				btEventFlagCount;		//处理响应时间

	uint8_t					hfp_CallFalg;			//判断是不是通话中连接蓝牙进入的通话模式
	BT_USER_STATE           btuserstate;

#if (BT_SPP_SUPPORT == ENABLE)
	//spp
	BT_SPP_STATE			sppState;
#endif

#if (BT_OBEX_SUPPORT)
	//obex
	BT_OBEX_STATE			obexState;
#endif

#if (BT_HID_SUPPORT == ENABLE)
	BT_HID_STATE			hidState;
	uint8_t					hidUserCmd[2];
#endif

#if (BT_PBAP_SUPPORT)
	//pbap
	BT_PBAP_STATE			pbapState;

	uint8_t					pbapGetContinueFlag;	//接收到数据后,延时发送Get标志
	uint16_t				pbapGetMaxListCount;
	uint16_t				pbapListStartOffset;

#endif

#if (BT_MFI_SUPPORT)
	BT_MFI_STATE			mfiState;
#endif

#if	BT_SOURCE_SUPPORT
	uint8_t					conRemoteAddr[6];
	BT_A2DP_STATE			a2dpSourceState;
	BT_SOURCE_INQUIRY_LIST	SourceInqResultList[A2DP_SCAN_NUMBER];
	BT_HFP_STATE			hfgState;
	uint8_t					GetNameFlag;
#endif

#ifdef BT_ACCESS_MODE_SET_BY_POWER_ON_TIMEOUT
	bool					btvisibilityDelayOn;
	uint32_t 				btvisibilityDelayCount;
#endif

#if 1//(BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT)
	BT_BIS_STATE			btBisState;
#endif

#if (BT_TWS_SUPPORT == ENABLE)
	bool					TwsPowerOnFlag;
	uint8_t					btTwsDeviceAddr[6];
	uint8_t					TwsFilterInfor[7];	//tws组网过滤信息
	BT_TWS_STATE			twsState;
	BT_TWS_ROLE				twsRole;
	uint8_t					twsFlag;//有TWS配对记录
	//uint32_t				twsSimplePairingCfg;
	uint8_t					twsEnterPairingFlag;//tws组网开始标志 //flag:0=normal; 1=pairing
	uint8_t					twsMode;//0=active mode; 2=sniff mode
	uint8_t					twsSbSlaveDisable;//1=从机关闭TWS组网功能;
	uint8_t					twsStopConnect;	// 1 = 停止TWS组网配对
	uint32_t				btTwsPairingStartDelayCnt;	//延时启动配对

	bool                    TwsDacNeedTimeout;//Tws连接处理时 DAC关闭后需要打开的超时检测时间
	bool                    TwsDacStatus; //TRUE 打开  FALSE 关闭
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)
	uint8_t					twsSoundbarSlaveTestFlag; //1 = soundbar slave进入校准测试标志
#endif
#endif

#endif
} BT_MANAGER_ST;

extern BT_MANAGER_ST		btManager;
#endif

#endif

