/**
 **************************************************************************************
 * @file    bt_stack_service.c
 * @brief   
 *
 * @author  KK
 * @version V1.0.0
 *
 * $Created: 2018-2-9 13:06:47$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include "type.h"
#include "app_config.h"
#include "bt_config.h"
#include "app_message.h"
#include "irqn.h"
#include "watchdog.h"
#include "clk.h"
#include "remind_sound.h"
#include "le_audio_config.h"

#include "sys_em_config.h"
#include "bb_api.h"
#include "bt_app_init.h"
#include "bt_stack_service.h"
#include "bt_stack_memory.h"

#include "bt_common_api.h"
#include "bt_manager.h"
#include "main_task.h"
#include "bt_interface.h"
#include "audio_core_service.h"

#include "bt_app_ddb_info.h"
#include "bt_app_connect.h"
#include "bt_app_common.h"
#include "bt_play_mode.h"

#if (BT_HFP_SUPPORT)
#include "bt_hf_mode.h"
#endif
#if (BT_TWS_SUPPORT == ENABLE)
#include "bt_app_tws_connect.h"
#endif

bool sink_stop_flag = 0;  //handle sink_stop key
extern bool sink_wait_starting;
#if (defined (CFG_APP_BT_MODE_EN ) || defined ( CFG_APP_BLE_MODE_EN ))
typedef struct _BtStackServiceContext
{
	xTaskHandle			taskHandle;
	MessageHandle		msgHandle;
//	TaskState			serviceState;

	uint8_t				serviceWaitResume;	//1:蓝牙不在后台运行时,开启通话,退出播放模式,不能kill蓝牙协议栈

	uint8_t				bbErrorMode;
	uint32_t			bbErrorType;

//	uint32_t			btEnterSniffStep;
//	uint32_t			btExitSniffReconPhone;
}BtStackServiceContext;
BT_CONFIGURATION_PARAMS		*btStackConfigParams = NULL;
static BtStackServiceContext	*btStackServiceCt = NULL;
#define BT_STACK_SERVICE_STACK_SIZE		1024+256 //768
#define BT_STACK_SERVICE_PRIO			4
#define BT_STACK_NUM_MESSAGE_QUEUE		20

static void BtStackServiceEntrance(void * param);
static bool BtStackServiceInit(void);

#ifdef MVA_BT_OBEX_UPDATE_FUNC_SUPPORT
#define BT_OBEX_SERVICE_STACK_SIZE		512
#define BT_OBEX_SERVICE_PRIO			4
xTaskHandle			bt_obex_taskHandle;
#endif
#endif

#ifdef CFG_APP_BT_MODE_EN

static uint8_t gBtHostStackMemHeap[BT_STACK_MEM_SIZE];

extern AvrcpAdvMediaStatus sPlayStatus[BT_LINK_DEV_NUM];
#if (BT_LE_AUDIO_SUPPORT)
static uint8_t target_role = 0xff;
uint32_t gBtBisTimeout = 0;
#endif
//BR/EDR STACK SERVICE

static void BtRstStateCheck(void);
extern void rwble_enable_init(void);
extern void app_set_ble_bd_addr(uint8_t* local_addr);//应用可以设置一次ble 地址
void bis_restart_process(void);

extern int8_t ME_CancelInquiry(void);
#ifdef CFG_FUNC_OPEN_SLOW_DEVICE_TASK
extern void SlowDevice_MsgSend(uint16_t msgId);
#endif
uint8_t BtLinkStateGet(void);

#ifdef BT_GETTIME_SUPPORT
uint32_t gBtMapEnable = 1; // TRUE:安卓手机 FALSE:苹果手机 默认使用map协议即安卓手机，部分安卓手机不会返回设备信息。
#endif

#ifdef BT_CONNECTED_DEEPSLEEP
extern uint8_t sniff_flag;
#endif

/***********************************************************************************
 * 蓝牙断开连接流程
 **********************************************************************************/
extern FUNC_BT_DISCONNECT_PROCESS BtDisconnectProcess;
void BtStack_BtDisconnectProcess(void)
{
	static uint32_t btDisconnectCnt = 0;

	btDisconnectCnt++;
	if(btDisconnectCnt>=200) //200ms
	{
		btDisconnectCnt=0;
		if(btManager.linkedNumber)
		{
			BtDisconnectCtrl(TRUE);
			return;
		}
		else
		{
			//注销
			BtAppiFunc_BtDisconnectProcess(NULL);
		}
	}
}

/***********************************************************************************
 * 蓝牙A2DP连接成功后,主动发起一次AVRCP连接
 **********************************************************************************/
extern FUNC_BT_AVRCP_CON_PROCESS BtAvrcpConProcess;
static uint32_t btAvrcpConIndex = 0;
#if (BT_AVRCP_VOLUME_SYNC == 1)
#define BT_AVRCP_RECON_DELAY		1500 //ms	//时间太短,会导致部分手机音量同步功能不生效
#else
#define BT_AVRCP_RECON_DELAY		300 //ms
#endif

void BtStack_BtAvrcpConProcess(void)
{
	static uint32_t btAvrcpConCnt = 0;

	btAvrcpConCnt++;
	if(btAvrcpConCnt>=BT_AVRCP_RECON_DELAY)
	{
		btAvrcpConCnt=0;
		if((btManager.btLinked_env[btAvrcpConIndex].a2dpState >= BT_A2DP_STATE_CONNECTED)
			&&(btManager.btLinked_env[btAvrcpConIndex].avrcpState != BT_AVRCP_STATE_CONNECTED))
		{
			BtAvrcpConnect(btAvrcpConIndex, btManager.btLinked_env[btAvrcpConIndex].remoteAddr);
		}
		
		//注销
		BtAppiFunc_BtAvrcpConProcess(NULL);
	}
}

void BtStack_BtAvrcpConRegister(uint8_t index)
{
	if(btAvrcpConIndex < BT_LINK_DEV_NUM)
	{
		btAvrcpConIndex = index;
		BtAppiFunc_BtAvrcpConProcess(BtStack_BtAvrcpConProcess);
	}
}

/***********************************************************************************
 * 蓝牙A2DP断链后,主动发起一次AVRCP断开
 * A2DP断开后，开启检测AVRCP断开机制(3S超时)
 **********************************************************************************/
extern FUNC_BT_AVRCP_DISCON_PROCESS BtAvrcpDisconProcess;
static uint32_t btAvrcpDisconIndex = 0;
void BtStack_BtAvrcpDisconProcess(void)
{
	static uint32_t btAvrcpDisconCnt = 0;

	btAvrcpDisconCnt++;
	if(btAvrcpDisconCnt>=3000) //3s
	{
		btAvrcpDisconCnt=0;
		if((btManager.btLinked_env[btAvrcpDisconIndex].a2dpState == BT_A2DP_STATE_NONE)
			&&(btManager.btLinked_env[btAvrcpDisconIndex].avrcpState != BT_AVRCP_STATE_NONE))
		{
			AvrcpDisconnect(btAvrcpDisconIndex);
		}
		//注销
		BtAppiFunc_BtAvrcpDisconProcess(NULL);
	}
}

void BtStack_BtAvrcpDisconRegister(uint8_t index)
{
	if(btAvrcpDisconIndex < BT_LINK_DEV_NUM)
	{
		btAvrcpDisconIndex = index;
		BtAppiFunc_BtAvrcpDisconProcess(BtStack_BtAvrcpDisconProcess);
	}
}


/***********************************************************************************
 * 蓝牙测试盒校准频偏完成回调函数
 **********************************************************************************/
void BtFreqOffsetAdjustComplete(unsigned char offset)
{
#ifndef CFG_FUNC_OPEN_SLOW_DEVICE_TASK
	int8_t ret = 0;
#endif
	APP_DBG("++++++[BT_OFFSET]  offset:0x%x ++++++\n", offset);

	btManager.btLastAddrUpgradeIgnored = 1;

	if(btStackConfigParams == NULL)
		return;

	//判断是否和当前默认值一致,不一致更新保存到flash
	if(offset != btStackConfigParams->bt_trimValue)
	{
		btStackConfigParams->bt_ConfigHeader[0]='M';
		btStackConfigParams->bt_ConfigHeader[1]='V';
		btStackConfigParams->bt_ConfigHeader[2]='B';
		btStackConfigParams->bt_ConfigHeader[3]='T';
		
		btStackConfigParams->bt_trimValue = offset;
#ifdef CFG_FUNC_OPEN_SLOW_DEVICE_TASK
		SlowDevice_MsgSend(MSG_DEVICE_BT_FREQ_OFFSET_WRITE);
#else
		//save to flash
		ret = BtDdb_SaveBtConfigurationParams(btStackConfigParams);
		
		if(ret)
			APP_DBG("[BT_OFFSET]update Error!!!\n");
		else
			APP_DBG("$$$[BT_OFFSET] update $$$\n");
#endif
	}

	//清除所有的配对记录
	BtDdb_EraseBtLinkInforMsg();
}

/***********************************************************************************
 * 蓝牙middleware的消息传递入口函数
 **********************************************************************************/
void BtMidMessageManage(BtMidMessageId messageId, uint8_t Param)
{
	MessageContext		msgSend;
	MessageHandle 		msgHandle;

	switch(messageId)
	{
		case MSG_BT_MID_UART_RX_INT:
			//msgHandle = GetBtStackServiceMsgHandle();
			//msgSend.msgId = MSG_BTSTACK_RX_INT;
			//MessageSend(msgHandle, &msgSend);
			BtStackServiceMsgSend(MSG_BTSTACK_RX_INT);
			break;

		case MSG_BT_MID_ACCESS_MODE_IDLE:
			if(sys_parameter.bt_ReconnectionEnable)
				BtReconnectDevice();
			break;

		case MSG_BT_MID_STACK_INIT:
			{
				//APP_DBG("MSG_BT_MID_STACK_INIT\n");
				//此处配置协议栈初始化完成后，是否进入到蓝牙可被搜索可被连接状态;
				//0 -> 不可见不可连接
				//1 -> 可见不可连接
				//2 -> 不可见可连接
				//3 -> 可见可连接
#if	BT_SOURCE_SUPPORT// source
				if(!SetBtSourceDefaultAccessMode())
#endif
				{
					GetBtManager()->btAccessModeEnable = POWER_ON_BT_ACCESS_MODE_SET;
				}
			}
			break;

		case MSG_BT_MID_STATE_CONNECTED:
			{
				//MessageContext		msgSend;
				msgSend.msgId		= MSG_BT_STATE_CONNECTED;
				MessageSend(GetMainMessageHandle(), &msgSend);
				DBG("BtMidMessageManage: MSG_BT_MID_STATE_CONNECTED\n");

				//SetBtPlayState(BT_PLAYER_STATE_STOP);
#if(BT_LINK_DEV_NUM == 2)
				if((btManager.btLinked_env[0].a2dpState != BT_A2DP_STATE_STREAMING)
					&&(btManager.btLinked_env[1].a2dpState != BT_A2DP_STATE_STREAMING)
					)
#endif
				SetBtPlayState(BT_PLAYER_STATE_STOP);

				BtStackServiceMsgSend(MSG_BTSTACK_RECONNECT_REMOTE_SUCCESS);
			}
			break;
		
		case MSG_BT_MID_STATE_DISCONNECT:
			{
				SoftFlagRegister(SoftFlagDiscDelayMask);
				DBG("BtMidMessageManage: MSG_BT_MID_STATE_DISCONNECT\n");
#ifdef BT_CONNECTED_DEEPSLEEP
				if(rwip_sleep_enable())
				{
		        	sniff_flag = 0;
					BtSniffNormalPower();
				}
#endif
				//SetBtPlayState(BT_PLAYER_STATE_STOP);
#if(BT_LINK_DEV_NUM == 2)
				if((btManager.btLinked_env[0].a2dpState != BT_A2DP_STATE_STREAMING)
					&&(btManager.btLinked_env[1].a2dpState != BT_A2DP_STATE_STREAMING)
					)
#endif
				SetBtPlayState(BT_PLAYER_STATE_STOP);
			}
			break;

//////////////////////////////////////////////////////////////////////////////////////////////////
//AVRCP
		case MSG_BT_MID_PLAY_STATE_CHANGE:
			if((Param == BT_PLAYER_STATE_PLAYING)&&(GetA2dpState(BtCurIndex_Get()) == BT_A2DP_STATE_STREAMING))
			{
				msgHandle = GetMainMessageHandle();
				msgSend.msgId		= MSG_BT_A2DP_STREAMING;
				MessageSend(msgHandle, &msgSend);
			}
			
			if(GetBtPlayState() != Param)
				SetBtPlayState(Param);
			
			break;

		case MSG_BT_MID_VOLUME_CHANGE:		
#if (BT_AVRCP_VOLUME_SYNC)
			SetBtSyncVolume(Param);
			// Send message to bt play mode
			msgSend.msgId		= MSG_BT_PLAY_SYNC_VOLUME_CHANGED;
			MessageSend(GetMainMessageHandle(), &msgSend);
#endif
			break;

//////////////////////////////////////////////////////////////////////////////////////////////////
//HFP
#if (BT_HFP_SUPPORT)
		case MSG_BT_MID_HFP_TASK_RESUME:
			BtHfModeRunningResume();
			break;

		//通话数据格式更新
		case MSG_BT_MID_HFP_CODEC_TYPE_UPDATE:
			BtHfCodecTypeUpdate(Param);
			break;

#ifdef CFG_FUNC_REMIND_SOUND_EN
		//通话模式下呼入电话播放来电提示音
		case MSG_BT_MID_HFP_PLAY_REMIND:
			msgHandle = GetBtHfMessageHandle();
			if(msgHandle == NULL)
				break;		
			// Send message to bt play mode
			msgSend.msgId		= MSG_BT_HF_MODE_REMIND_PLAY;
			MessageSend(msgHandle, &msgSend);
			break;
		
		//通话模式下停止播放提示音
		case MSG_BT_MID_HFP_PLAY_REMIND_END:
			msgHandle = GetBtHfMessageHandle();
			if(msgHandle == NULL)
				break;
			RemindSoundClearPlay();
			break;
#endif

#endif
//////////////////////////////////////////////////////////////////////////////////////////////////
		default:
			break;
	}
}

/***********************************************************************************
 * 定时检测和处理蓝牙相关事件
 **********************************************************************************/
static void CheckBtEventTimer(void)
{
#if (BT_AVRCP_SONG_TRACK_INFOR)
	//获取蓝牙歌曲和歌词3信息
	//if(sPlayStatus[BtCurIndex_Get()] == AVRCP_ADV_MEDIA_PLAYING)
	{
		if(btManager.avrcpMediaInforFlag)
		{
			btManager.avrcpMediaInforFlag = 0;
			BTCtrlGetMediaInfor(BtCurIndex_Get());
		}
	}
#endif
#ifdef BT_ACCESS_MODE_SET_BY_POWER_ON_TIMEOUT
	if(btManager.btvisibilityDelayOn)
	{
		btManager.btvisibilityDelayCount++;
		if(btManager.btvisibilityDelayCount >= BT_VISIBILITY_DELAY_TIME)
		{
			Vis_Set(BtAccessModeNotAccessible,__FUNCTION__);
			btManager.btvisibilityDelayCount = 0;
			btManager.btvisibilityDelayOn = FALSE;
			GetBtManager()->btAccessModeEnable = BT_ACCESSBLE_NONE;
		}
	}
#endif
	//获取蓝牙播放状态
	if(btManager.avrcpPlayStatusTimer.timerFlag)
	{
		if(IsTimeOut(&btManager.avrcpPlayStatusTimer.timerHandle))
		{
			BT_A2DP_STATE state = GetA2dpState(BtCurIndex_Get());
			if(state == BT_A2DP_STATE_STREAMING)
			{
				BTCtrlGetPlayStatus(BtCurIndex_Get());
				TimerStart_BtPlayStatus();
			}
			else
			{
				TimerStop_BtPlayStatus();
			}
		}
	}

	if(sys_parameter.bt_ReconnectionEnable)
	{
		BtReconnectProcess();

		if(btManager.btReconnectDelayCount)
		{
			btManager.btReconnectDelayCount++;
			if((btManager.btReconnectDelayCount>200)//&&(btManager.btReconTwsSt.ConnectionTimer.timerFlag == TIMER_UNUSED)
	#ifdef CFG_FUNC_REMIND_SOUND_EN
				&&(RemindSoundIsPlay() <= 1 )
	#endif
				)
			{
				btManager.btReconnectDelayCount = 0;
				BtReconnectDevice();
			}
		}
	}

	BtScanPageStateCheck();

	BtRstStateCheck();

#if	BT_SOURCE_SUPPORT
	BtSourceNameGetCheck();

#ifdef SOURCE_AUTO_INQUIRY_AND_CONNECT
	BtSourceReInquiryCheck();
#endif

#endif

#ifdef BT_GETTIME_SUPPORT
	//时间获取(只在连接后获取1次)//20240509
	if((btManager.btLinkState) && (gBtMapEnable))
	{
		if(gBtMapEnable == 1)//android phone, connect map profile
		{
			MAP_Connect();
		}

		if(gBtMapEnable++ >= 1500) //获取完成之后在map_time_callback中断开MAP协议，保证手机连接第二台设备能获取时间
		{
			start_get_time_cmd();
			gBtMapEnable = 0; 
		}
	}
#endif
}

/**************************************************************************
 *
 **************************************************************************/
/**
 * @brief	Get message receive handle of bt stack manager
 * @param	NONE
 * @return	MessageHandle
 */
xTaskHandle GetBtStackServiceTaskHandle(void)
{
	if(!btStackServiceCt)
		return NULL;
	
	return btStackServiceCt->taskHandle;
}

uint8_t GetBtStackServiceTaskPrio(void)
{
	return BT_STACK_SERVICE_PRIO;
}

MessageHandle GetBtStackServiceMsgHandle(void)
{
	if(!btStackServiceCt)
		return NULL;
	
	return btStackServiceCt->msgHandle;
}

void BtStackServiceMsgSend(uint16_t msgId)
{
	MessageContext		msgSend;
	msgSend.msgId = msgId;
	if(btStackServiceCt)
		MessageSend(btStackServiceCt->msgHandle, &msgSend);
}

/***********************************************************************************
 * 处理蓝牙异常状态
 **********************************************************************************/
/*static void CheckBtErrorState(void)
{
	btEventListCount++;
	if(btCheckEventList)
	{
		//当接收到BT_STACK_EVENT_COMMON_CONNECTION_ABORTED事件,当前链路出现异常,会主动断开手机,需要主动回连手机,并自动播放歌曲
		//超时时间为30s
		if((btCheckEventList&BT_EVENT_L2CAP_LINK_DISCONNECT)&&(btEventListCount == btEventListB1Count))
		{
			APP_DBG("[btCheckEventList]: BT_EVENT_L2CAP_LINK_DISCONNECT\n");
			
			btCheckEventList &= ~BT_EVENT_L2CAP_LINK_DISCONNECT;
			btEventListB1Count = 0;
		}
	}
}
*/
/***********************************************************************************
 * 蓝牙时钟设置
 * 此函数lib中调用，客户可根据自己条件配置
 **********************************************************************************/
void BtCntClkSet(void)
{
#if 0
	//btclk freq set
//	BACKUP_32KEnable(OSC32K_SOURCE);
//	sniff_cntclk_set(1);//sniff cnt clk 32768 Hz default not use
//_____________________________________________

#ifndef BT_SNIFF_RC_CLK
	//HOSC 32K
	Clock_BTDMClkSelect(OSC_32K_MODE);
	Clock_OSC32KClkSelect(LOSC_32K_MODE);
	Clock_32KClkDivSet(Clock_OSCClkDivGet());  //如果这里出现问题，请检查SystemClockInit，不要在这里直接改——Tony
	Clock_BBCtrlHOSCInDeepsleep(0);//禁止baseband进入sniff后硬件自动关闭HOSC 24M
#else
	//RC
	sniff_rc_init_set();//Rc 初始化参数
	//RC 32K
	Clock_BTDMClkSelect(RC_CLK32_MODE);//select rc_clk_32k
	Clock_32KClkDivSet(750);     //set osc_clk_32k = 24M/32K=750

	Clock_RcCntWindowSet(63);//64-1  --  32K/64 = 500

	Clock_RC32KClkDivSet( Clock_RcFreqGet(1) / ((uint32_t)(32*1000)) );
	Clock_RcFreqAutoCntStart();

	Clock_BBCtrlHOSCInDeepsleep(1);//Deepsleep时,BB接管HOSC
#endif
#endif
}

/***********************************************************************************
 * 蓝牙协议栈消息处理
 **********************************************************************************/
static void BtStackMsgProcess(uint16_t msgId)
{
	MessageContext		msgSend;
	MessageHandle 		msgHandle;

	if(btStackServiceCt == NULL)
		return;

	msgHandle = GetMainMessageHandle();
	uint8_t cur_index = BtCurIndex_Get();
	
	switch(msgId)
	{
		case MSG_BTSTACK_BB_ERROR:
			{
				msgSend.msgId = MSG_BTSTACK_BB_ERROR;

				MessageSend(msgHandle, &msgSend);

				if(btStackServiceCt->bbErrorMode == 1)
				{
					APP_DBG("BT ERROR:0x%lx\n", btStackServiceCt->bbErrorType);
				}
				else if(btStackServiceCt->bbErrorMode == 2)
				{
					APP_DBG("BLE ERROR:0x%lx\n", btStackServiceCt->bbErrorType);
				}
                else if(btStackServiceCt->bbErrorMode == 3)
                {
					APP_DBG("BB MSG ERROR:0x%lx\n", btStackServiceCt->bbErrorType);
                }

				BtReconnectDevStop();
			}
			break;

		case MSG_BTSTACK_BB_ERROR_RESTART:
			//蓝牙协议栈任务启动后,发起回连
			if(IsBtAudioMode())
			{
				APP_DBG("[BT_STACK_APP]:MSG_BTSTACK_BB_ERROR_RESTART, reconnect remote device\n");
				BtReconnectDevCreate(btManager.btDdbLastAddr, sys_parameter.bt_ReconnectionTryCounts, sys_parameter.bt_ReconnectionInternalTime, 0, btManager.btDdbLastProfile);
			}
			break;

		case MSG_BTSTACK_LOCAL_DEVICE_NAME_UPDATE:
			{
				extern void BtSetDeviceName(char *name, uint8_t len);
				//蓝牙名称下发到协议栈
				BtSetDeviceName(sys_parameter.bt_LocalDeviceName, strlen(sys_parameter.bt_LocalDeviceName));
				
#ifdef CFG_FUNC_OPEN_SLOW_DEVICE_TASK
				SlowDevice_MsgSend(MSG_DEVICE_BT_NAME_WRITE);
#endif
			}
			break;

		case MSG_BTSTACK_RECONNECT_REMOTE_SUCCESS:
			APP_DBG("[BT_STACK_MSG]:MSG_BTSTACK_RECONNECT_REMOTE_SUCCESS\n");
			break;

		case MSG_BTSTACK_RECONNECT_REMOTE_PROFILE:
			APP_DBG("[BT_STACK_MSG]:MSG_BTSTACK_RECONNECT_REMOTE_PROFILE\n");
			if(btManager.btReconExcuteSt == (&btManager.btReconPhoneSt))
			{
				//蓝牙处于回连等待期,假如手机有主动连接BP10,则需要继续等待;
				if((btManager.btReconExcuteSt->ConnectionTimer.timerFlag & TIMER_WAITING) == 0)
				{
					APP_DBG("restart connect other profiles.\n");
					btManager.btReconExcuteSt->ConnectionTimer.timerFlag |= TIMER_STARTED;
				}
			}
			break;

		case MSG_BTSTACK_RECONNECT_REMOTE_STOP:
			APP_DBG("[BT_STACK_MSG]:MSG_BTSTACK_RECONNECT_REMOTE_STOP\n");
			BtReconnectDevStop();
			break;

		case MSG_BTSTACK_RECONNECT_REMOTE_PAGE_TIMEOUT:
			APP_DBG("[BT_STACK_MSG]:MSG_BTSTACK_RECONNECT_REMOTE_PAGE_TIMEOUT\n");
			if(btManager.btReconExcuteSt == (&btManager.btReconPhoneSt))
			{
				btManager.btReconExcuteSt->ConnectionTimer.timerFlag = TIMER_UNUSED;
			}
			break;

#if BT_TWS_SUPPORT
		case MSG_BTSTACK_RECONNECT_TWS_STOP:
			APP_DBG("[BT_STACK_MSG]:MSG_BTSTACK_RECONNECT_TWS_STOP\n");
			BtReconnectTwsStop();
			break;
		
		case MSG_BTSTACK_RECONNECT_TWS_PAGE_TIMEOUT:
			APP_DBG("[BT_STACK_MSG]:MSG_BTSTACK_RECONNECT_TWS_PAGE_TIMEOUT\n");
			if(btManager.btReconExcuteSt == (&btManager.btReconTwsSt))
			{
				btManager.btReconExcuteSt->ConnectionTimer.timerFlag = TIMER_UNUSED;
			}
			break;
#endif

		case MSG_BTSTACK_RUN_START:
			APP_DBG("[BT_STACK_MSG]:MSG_BTSTACK_RUN_START\n");
			//BtMidMessageSend(MSG_BT_MID_STATE_FAST_ENABLE, 0);
			BtStackServiceMsgSend(MSG_BTSTACK_ACCESS_MODE_SET);
			if(sys_parameter.bt_BackgroundType == BT_BACKGROUND_DISABLE)
				BtScanPageStateSet(BT_SCAN_PAGE_STATE_OPENING);
#if (BT_TWS_SUPPORT == ENABLE)
			if(sys_parameter.bt_TwsReconnectionEnable && btManager.twsFlag)
			{
				if(btManager.twsRole == BT_TWS_SLAVE)
				{
					extern void BtReconnectTws_Slave(void);
					BtReconnectTws_Slave();
				}
				else
				{
					extern void BtReconnectTws(void);
					BtReconnectTws();
				}
			}
#endif
			break;

		case MSG_BTSTACK_ACCESS_MODE_SET:
			APP_DBG("[BT_STACK_MSG]:MSG_BTSTACK_ACCESS_MODE_SET\n");
			BtAccessModeSetting();
			break;
///////////////////////////////////////////////////////////////////
		case MSG_BTSTACK_NUMERICAL_ACCEPT:
		{
			BT_DBG("bt msg: numerical accept\n");
			BtNumericalAccecpt();
		}
		break;

		case MSG_BTSTACK_NUMERICAL_REJECT:
		{
			BT_DBG("bt msg: numerical reject\n");
			BtNumericalReject();
		}
		break;

///////////////////////////////////////////////////////////////////
		//COMMON
		case MSG_BTSTACK_MSG_BT_CONNECT_DEV_CTRL:
			{
				extern uint8_t BB_link_state_get(uint8_t type);
				BT_DBG("bt msg: connect dev");
				
				if(BB_link_state_get(1) == 0)
				{
					BtConnectCtrl();
				}
				else
				{
					BT_DBG(" busy...\n");
				}
			}
			break;

		case MSG_BTSTACK_MSG_BT_DISCONNECT_DEV_CTRL:
			BT_DBG("bt msg: disconnect dev\n");
			
			//BTHciDisconnectCmd(btManager.btDdbLastAddr);
			BtDisconnectCtrl(FALSE);
			break;

		//AVRCP CMD
		case MSG_BTSTACK_MSG_BT_PLAY:
			BT_DBG("bt msg: play\n");
			BTCtrlPlay(cur_index);
			break;
		
		case MSG_BTSTACK_MSG_BT_PAUSE:
			BT_DBG("bt msg: pause\n");
			BTCtrlPause(cur_index);
			break;

		case MSG_BTSTACK_MSG_BT_NEXT:
			BT_DBG("bt msg: next\n");
			BTCtrlNext(cur_index);
			break;
		
		case MSG_BTSTACK_MSG_BT_PREV:
			BT_DBG("bt msg: prev\n");
			BTCtrlPrev(cur_index);
			break;

		case MSG_BTSTACK_MSG_BT_FF_START:
			BT_DBG("bt msg: ff start\n");
			BTCtrlFF(cur_index);
			break;
		
		case MSG_BTSTACK_MSG_BT_FF_END:
			BT_DBG("bt msg: ff end\n");
			BTCtrlEndFF(cur_index);
			break;
		
		case MSG_BTSTACK_MSG_BT_FB_START:
			BT_DBG("bt msg: fb start\n");
			BTCtrlFB(cur_index);
			break;
		
		case MSG_BTSTACK_MSG_BT_FB_END:
			BT_DBG("bt msg: fb end\n");
			BTCtrlEndFB(cur_index);
			break;

#if (BT_AVRCP_VOLUME_SYNC)
		case MSG_BTSTACK_MSG_BT_VOLUME_SET:
			{
				uint8_t VolumePercent = BtLocalVolLevel2AbsVolme(GetBtSyncVolume());
				BT_DBG("bt msg: volume set %d\n", VolumePercent);
				BTCtrlSetVol(cur_index, VolumePercent);
			}
			break;
#endif

#if (BT_HFP_SUPPORT)
		//HFP CMD
		case MSG_BTSTACK_MSG_BT_REDIAL:
			BT_DBG("bt msg: hf redial\n");
			HfpRedialNumber(cur_index);
			break;

		case MSG_BTSTACK_MSG_BT_OPEN_VOICE_RECONGNITION:
			BT_DBG("bt msg: hf open voice recognition\n");
			OpenBtHfpVoiceRecognitionFunc(cur_index);
			break;
		
		case MSG_BTSTACK_MSG_BT_CLOSE_VOICE_RECONGNITION:
			BT_DBG("bt msg: hf close voice recognition\n");
			HfpVoiceRecognition(cur_index, 0);
			break;

		case MSG_BTSTACK_MSG_BT_ANSWER_CALL:
			BT_DBG("bt msg: hf answer call\n");
			HfpAnswerCall(cur_index);
			break;

		case MSG_BTSTACK_MSG_BT_HANGUP:
			BT_DBG("bt msg: hf hang up\n");
			HfpHangup(cur_index);
			break;

		case MSG_BTSTACK_MSG_BT_HF_AUDIO_TRANSFER:
			BT_DBG("bt msg: hf audio transfer\n");
			HfpAudioTransfer(cur_index);
			break;

		case MSG_BTSTACK_MSG_BT_HF_CALL_HOLDCUR_ANSWER_CALL:
			BT_DBG("bt msg: hf call hold: hold current call and answer another call\n");
			HfpCallHold(cur_index, HF_HOLD_HOLD_ACTIVE_CALLS);
			break;
		
		case MSG_BTSTACK_MSG_BT_HF_CALL_HANGUP_ANSWER_CALL:
			BT_DBG("bt msg: hf call hold: hangup current call and answer another call\n");
			HfpCallHold(cur_index, HF_HOLD_RELEASE_ACTIVE_CALLS);
			break;
		
		case MSG_BTSTACK_MSG_BT_HF_CALL_HANGUP_ANOTHER_CALL:
			BT_DBG("bt msg: hf call hold: hangup another call \n");
			HfpCallHold(cur_index, HF_HOLD_RELEASE_HELD_CALLS);
			break;

#ifdef BT_HFP_BATTERY_SYNC
		case MSG_BTSTACK_MSG_BT_HF_SET_BATTERY:
			BT_DBG("bt msg: set battery level:%d\n", btManager.HfpBatLevel);
			HfpSetBatteryState(cur_index, btManager.HfpBatLevel, 0);
			break;
#endif

		case MSG_BTSTACK_MSG_BT_HF_VOLUME_SET:
			HfpSpeakerVolume(cur_index, btManager.volGain);
			break;

		case MSG_BTSTACK_MSG_BT_HF_GET_CUR_CALLNUMBER:
			//BT_DBG("MSG_BTSTACK_MSG_BT_HF_GET_CUR_CALLNUMBER : %d\n", BtCurIndex_Get());
			HfpGetCurrentCalls(cur_index);
			break;

		case MSG_BTSTACK_MSG_BT_HF_DISABLE_NREC:
			BT_DBG("MSG_BTSTACK_MSG_BT_HF_DISABLE_NREC : %d\n", BtCurIndex_Get());
			HfpDisableNREC(cur_index);
			break;

		case MSG_BTSTACK_MSG_BT_HF_SCO_DISCONNECT:
			BT_DBG("MSG_BTSTACK_MSG_BT_HF_SCO_DISCONNECT : %d\n", btManager.hfpScoDiscIndex);
			HfpAudioDisconnect(btManager.hfpScoDiscIndex);
			break;

		case MSG_BTSTACK_MSG_BT_HF_SCO_CONNECT:
			BT_DBG("MSG_BTSTACK_MSG_BT_HF_SCO_CONNECT : %d\n", BtCurIndex_Get());
			HfpAudioConnect(cur_index);
			break;
#endif
#if (BT_LE_AUDIO_SUPPORT)
		case MSG_BTSTACK_MSG_BT_BIS_EVENT:
			printf("BtStackMsg: MSG_BTSTACK_MSG_BT_BIS_EVENT\n");

			//clear sink_wait_starting
			sink_wait_starting = 0;
			
			if(BIS_SRC == Get_bis_role())
			{
				BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_BIS_SRC_STOP);
			}
			else if(BIS_SINK == Get_bis_role())
			{
				BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_BIS_SNK_STOP);
			}
			else if(BIS_NONE == Get_bis_role())
			{
				if (BtLinkStateGet() == 0
					&& ((GetSystemMode() == ModeBtAudioPlay) || (GetSystemMode() == ModeLeAudioSinkPlay))
					)
				{
					BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_BIS_SNK_START);
				}
				else
				{
					BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_BIS_SRC_START);
				}
			}
			break;

		case MSG_BTSTACK_MSG_BT_BIS_SRC_START:
			{
				if (!gBtBisTimeout)
				{
					printf("BtStackMsg: MSG_BTSTACK_MSG_BT_BIS_SRC_START\n");
					if (Get_bis_role() == BIS_NONE)
					{
						Set_bis_role(BIS_SRC);
					}
					printf("BtStackMsg: MSG_BTSTACK_MSG_BT_BIS_SRC_START %d\n", Get_bis_role());
				}
			}
			break;
			
		case MSG_BTSTACK_MSG_BT_BIS_SRC_STOP:
			{
				if (!gBtBisTimeout)
				{
					printf("BtStackMsg: MSG_BTSTACK_MSG_BT_BIS_SRC_STOP\n");
					Set_bis_role(BIS_NONE);
				}
			}
			break;
			
		case MSG_BTSTACK_MSG_BT_BIS_SNK_START:
			{
				printf("BtStackMsg: MSG_BTSTACK_MSG_BT_BIS_SNK_START: 0x%x, 0x%x\n", btManager.btBisState, gBtBisTimeout);
 				le_bis_sink_printf_state();

				if (!gBtBisTimeout)
				{
					if (check_bis_sink_state() == 0)
					{
						if (btManager.btBisState)
						{
							btManager.btBisState = BT_BIS_STATE_NONE;
						}
					}
					if (Get_bis_role() == BIS_NONE)
					{
						Set_bis_role(BIS_SINK);
						target_role = BIS_SINK;
					}
				}
			}
			break;
			
		case MSG_BTSTACK_MSG_BT_BIS_SNK_STOP:
			{
 				le_bis_sink_printf_state();
#ifdef SINK_CHOOSE_SRC
				le_bis_sink_remove_src();
#endif
				printf("BtStackMsg: MSG_BTSTACK_MSG_BT_BIS_SNK_STOP: 0x%x, 0x%x\n", btManager.btBisState, gBtBisTimeout);
				
				sink_stop_flag = 1;
				if (!gBtBisTimeout)
				{
					Set_bis_role(BIS_NONE);	
					if (check_bis_sink_state() == 0)
					{
						if (btManager.btBisState)
						{
							btManager.btBisState = BT_BIS_STATE_NONE;
						}
					}
				}
			}
			break;
#endif //#if (BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT)


#if (BT_TWS_SUPPORT == ENABLE)
		case MSG_BTSTACK_TWS_CONNECT:
			APP_DBG("[BT_STACK_APP]:tws connect: %02x:%02x:%02x:%02x:%02x:%02x\n", 
				btManager.btTwsDeviceAddr[0],
				btManager.btTwsDeviceAddr[1],
				btManager.btTwsDeviceAddr[2],
				btManager.btTwsDeviceAddr[3],
				btManager.btTwsDeviceAddr[4],
				btManager.btTwsDeviceAddr[5]
				);
			
			if((!btManager.btTwsDeviceAddr[0])&&(!btManager.btTwsDeviceAddr[1])&&(!btManager.btTwsDeviceAddr[2])&&
				(!btManager.btTwsDeviceAddr[3])&&(!btManager.btTwsDeviceAddr[4])&&(!btManager.btTwsDeviceAddr[5])
				)
			{
				APP_DBG("addr error!!!\n");
				break;
			}
			
#if (BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT)
			//tws_connect(btManager.btTwsDeviceAddr);
			if(Get_bis_role()==BIS_NONE)//bis src/sink 不连接tws
			{
				BtTwsPairingConnect(btManager.btTwsDeviceAddr);
			}
			else
			{
				APP_DBG("!!!bis role %d\n",Get_bis_role());
			}
#else
			tws_connect(btManager.btTwsDeviceAddr);
#endif
			break;
			
		case MSG_BTSTACK_TWS_DISCONNECT:
			APP_DBG("[BT_STACK_APP]:tws disconnect\n");
			tws_link_disconnect();
			break;

		case MSG_BT_STACK_TWS_PAIRING_START:			
#if (BT_LINK_DEV_NUM == 2)
			if(btManager.btLinked_env[0].btLinkState && btManager.btLinked_env[1].btLinkState)//连接了2台手机 禁止进行组网
			{
				APP_DBG("break : btLinked_env[0].btLinkState = 1, btLinked_env[1].btLinkState = 1\n");
				break;
			}
#endif
			APP_DBG("[BT_STACK_APP]:tws pairing start\n");
			BtTwsPairingStart();
			break;

		case MSG_BT_STACK_TWS_PAIRING_STOP:
			BtTwsExitPeerPairingMode();
			break;

		case MSG_BT_STACK_TWS_PAIRING_START_RESUME:
			if(btManager.twsState == BT_TWS_STATE_CONNECTED ||gBtTwsAppCt->btTwsPairingStart == 1)
			{
				APP_DBG("tws connecting or connected \n");
				break;
			}
			BtTwsPairingStart();
			break;

		case MSG_BT_STACK_TWS_SYNC_POWERDOWN:
			if(btManager.twsState == BT_TWS_STATE_CONNECTED)
			{
				tws_sync_powerdown_cmd();
				btManager.twsState = BT_TWS_STATE_NONE;
			}
			break;

		case MSG_BT_STACK_TWS_SYNC_REINIT:
			{
				if((btManager.twsState == BT_TWS_STATE_CONNECTED)&&(btManager.twsRole == BT_TWS_MASTER))
				{
					APP_DBG("tws master sync reinit \n");
					//master reinit
					tws_sync_reinit();
				}
			}
			break;
#endif	//#if (BT_TWS_SUPPORT == ENABLE)

#ifdef BLE_TUYA_ENABLE
		case MSG_BTSTACK_MSG_BLE_TUYA_DATA_SEND:
			TuyaBleStackSendData();
			break;
#endif
		default:
			break;
		
	}
}

#if (BT_LE_AUDIO_SUPPORT)
/***********************************************************************************
 * 蓝牙协议栈任务处理
 **********************************************************************************/

void app_set_sink_mode(void)
{
	if(GetSystemMode() != ModeLeAudioSinkPlay)
    {
        LeAudioSinkModeEnter();
    }
}

uint8_t app_get_bis_state(void)
{
	printf("app_get_bis_state 0x%x\n", btManager.btBisState);
	return (uint8_t)btManager.btBisState;
}

static void BtBisProcess(void)
{
	uint8_t var_bis_role = Get_bis_role();
	switch(btManager.btBisState)
	{
		case BT_BIS_STATE_NONE:
#if !defined(LE_BIS_ASSIS) && !defined(LE_BIS_DELEG)
			if(BIS_SRC == var_bis_role)
			{
				btManager.btBisState = BT_BIS_STATE_SRC_START;
			}
			else if(BIS_SINK == var_bis_role)
			{
				btManager.btBisState = BT_BIS_STATE_SNK_START;
			}
#endif
			break;

		case BT_BIS_STATE_SRC_START:
			{
				printf("BT_BIS_STATE_SRC_START\n");
				le_bis_src_print_state();
				if (check_bis_src_is_streamed())
				{
					btManager.btBisState = BT_BIS_STATE_SRC_STREAMED;
					gBtBisTimeout = 0;
					break;
				}

				if (check_bis_src_is_in_idle())
				{

#ifdef LE_UDSS
					// app_udss_adv_stop();
#endif
					btManager.btBisState = BT_BIS_STATE_SRC_START_WAITING;
					le_bis_source_play();
					gBtBisTimeout = 1000;

				}
				else
				{
					le_bis_src_handle_stop();
					btManager.btBisState = BT_BIS_STATE_SRC_STOP_WAITING;
					gBtBisTimeout = 0;
				}
			}
			break;

		case BT_BIS_STATE_SRC_START_WAITING:
			if (check_bis_src_is_streamed())
			{
				gBtBisTimeout = 0;
				btManager.btBisState = BT_BIS_STATE_SRC_STREAMED;
				printf("BT_BIS_STATE_SRC_STREAMED\n");
				break;
			}

			if(gBtBisTimeout)
			{
				gBtBisTimeout--;
				if(gBtBisTimeout == 0)
				{
					btManager.btBisState = BT_BIS_STATE_SRC_STOP;
				}
			}
			break;

		case BT_BIS_STATE_SRC_STREAMED:
			if(BIS_SRC != var_bis_role)
			{
				btManager.btBisState = BT_BIS_STATE_SRC_STOP;
			}
			else if (check_bis_src_is_streamed() == 0)
			{
				btManager.btBisState = BT_BIS_STATE_SRC_STOP;
				gBtBisTimeout = 1000;
			}
			break;

		case BT_BIS_STATE_SRC_STOP:
			{
				printf("MSG_LE_BIS_SRC_STOP, 0x%x\n", check_bis_src_is_streamed());
				le_bis_src_print_state();

				if (check_bis_src_is_streamed())
				{
					le_bis_source_stop();
#ifdef LE_UDSS
					// app_disconnect();
					// app_udss_start_adv();
#endif
				}
				else
				{
					le_bis_src_handle_stop();
				}

#ifdef LE_BIS_ASSIS
				// app_disconnect();
#endif
				btManager.btBisState = BT_BIS_STATE_SRC_STOP_WAITING;
				gBtBisTimeout = 1000;
				LeAudioServiceMsgSend(MSG_TASK_STOP);
			}
			break;
		
		case BT_BIS_STATE_SRC_STOP_WAITING:
			{
				if(check_bis_src_is_in_idle())
				{
					btManager.btBisState = BT_BIS_STATE_NONE;
					gBtBisTimeout = 0;
					Set_bis_role( BIS_NONE );
					LeAudioServiceMsgSend(MSG_TASK_STOP);
					break;
				}

				if(gBtBisTimeout)
				{
					gBtBisTimeout--;
					if(gBtBisTimeout == 0)
					{
						btManager.btBisState = BT_BIS_STATE_SRC_STOP;
					}
				}
			}
			break;

		case BT_BIS_STATE_SNK_START:
			{
				check_bis_sink_state();
				printf("MSG_LE_BIS_Sink_START\n");

				if (le_bis_sink_is_synced())
				{
					btManager.btBisState = BT_BIS_STATE_SNK_STEAMED;
					gBtBisTimeout = 0;
					set_sink_sync_lost(0);
					return;
				}

				if ((check_bis_sink_state()) && (check_bis_sink_state() != 8))
				{
					le_bis_sink_handle_stop();
					gBtBisTimeout = 0;
				}
				else
				{
					BtReconnectDevStop();		//stop reconnect remote device
					Vis_Set(BtAccessModeNotAccessible,__FUNCTION__);
					Set_bis_role( BIS_SINK );
					
					if(GetSystemMode() != ModeLeAudioSinkPlay)
					{
						LeAudioSinkModeEnter();
						btManager.btBisState = BT_BIS_STATE_SNK_START_WAITING_SINK_MODE;
						gBtBisTimeout = 2000;
#ifdef CFG_APP_BIS_BT_DUAL_CHIP_MODE_EN
						LeAudioGetCurModeCmd();
#endif
						break;
					}
#ifdef LE_BIS_DELEG
					le_bis_deleg_start_solicite();
#else
#ifdef LE_UDSS
					// app_udss_adv_stop();
#endif				
					if (!get_sink_scan_start_flag())
					{
						le_bis_sink_start();
					}
#endif
					btManager.btBisState = BT_BIS_STATE_SNK_START_WAITING;
					gBtBisTimeout = 2000;
				}
			}
			break;
		case BT_BIS_STATE_SNK_START_WAITING_SINK_MODE:
			if(GetSystemMode() != ModeLeAudioSinkPlay && gBtBisTimeout)
			{
				gBtBisTimeout--;
//				printf("----gBtBisTimeout: %d\n",gBtBisTimeout);
				break;
			}
#ifdef LE_BIS_DELEG
			le_bis_deleg_start_solicite();
#else
			le_bis_sink_start();
#endif
			btManager.btBisState = BT_BIS_STATE_SNK_START_WAITING;
			gBtBisTimeout = 2000;
			break;
			
		case BT_BIS_STATE_SNK_START_WAITING:
			if (le_bis_sink_is_synced())
			{
				gBtBisTimeout = 0;
				btManager.btBisState = BT_BIS_STATE_SNK_STEAMED;
				if (target_role == BIS_SINK)
				{
					Set_bis_role(BIS_SINK);
				}
				printf("BT_BIS_STATE_SNK_STEAMED\n");
				set_sink_sync_lost(0);
				sink_stop_flag = 0;
				break;
			}
			else if (check_bis_sink_state() == 8)
			{
				if ((Get_bis_role() == BIS_NONE) || (Get_bis_role() == BIS_SRC))
				{
					if (check_bis_sink_state() == 8)
					{
						le_bis_sink_stop_scan();		
					}

					if (!sink_wait_starting)
					{
						btManager.btBisState = BT_BIS_STATE_NONE;
						LeAudioSinkModeExit();
						gBtBisTimeout = 0;
					}
				}
			}
			else if ((check_bis_sink_state() != 8) && check_bis_sink_state())
			{
				if (Get_bis_role() == BIS_NONE)
				{
					btManager.btBisState = BT_BIS_STATE_SNK_STOP;					
				}
			}
			else if (!check_bis_sink_state())
			{
				printf("state is idle: 0x%x, 0x%x\n", sink_wait_starting, Get_bis_role());
				if (sink_wait_starting)
				{
					btManager.btBisState = BT_BIS_STATE_NONE;
				}
			}

			// if (gBtBisTimeout < 7000)
			if (gBtBisTimeout < 1000)
			{
				if (check_bis_sink_state() == 8)
				{
					gBtBisTimeout = 0;					
				}
			}

			if(gBtBisTimeout)
			{
				gBtBisTimeout--;
				if(gBtBisTimeout == 0)
				{
					gBtBisTimeout = 1000;
					if (check_bis_sink_state() == 8)
					{
						btManager.btBisState = BT_BIS_STATE_NONE;
					}
					else
					{
						btManager.btBisState = BT_BIS_STATE_SNK_STOP;
					}
				}
			}
			break;

		case BT_BIS_STATE_SNK_STEAMED:
			if(BIS_SINK != var_bis_role)
			{
				btManager.btBisState = BT_BIS_STATE_SNK_STOP;
				
				LeAudioSinkModeExit();
			}
			else if (le_bis_sink_is_synced() == 0)
			{
				btManager.btBisState = BT_BIS_STATE_SNK_STOP;					
				gBtBisTimeout = 100;
			}
			break;

		case BT_BIS_STATE_SNK_STOP:
			{
				printf("MSG_LE_BIS_Sink_STOP, 0x%x, 0x%x, 0x%X, 0x%x\n", le_bis_sink_is_synced(), sink_stop_flag, check_bis_sink_state(), Get_bis_role());
				
				if (check_bis_sink_state() == 0)
				{
					// gBtBisTimeout--;
					// if (!gBtBisTimeout)
					{
						LeAudioServiceMsgSend(MSG_TASK_STOP);
						app_stop_biginfo_timer();
						btManager.btBisState = BT_BIS_STATE_NONE;
						gBtBisTimeout = 0;
						if (sink_stop_flag)
						{
							Set_bis_role( BIS_NONE );
							sink_stop_flag = 0;
						}
					}
					break;
				}

				if (le_bis_sink_is_synced())
				{
					sink_stop_flag = 0;
					Vis_Set(BtAccessModeGeneralAccessible,__FUNCTION__);

					le_bis_sink_stop();
				}
				else if (check_bis_sink_state())
				{
					le_bis_sink_handle_stop();
				}

#if defined(LE_UDSS) || defined(LE_BIS_DELEG)
				// app_disconnect();
#endif
				btManager.btBisState = BT_BIS_STATE_SNK_STOP_WAITING;
				gBtBisTimeout = 1000;

			}
			break;
			
		case BT_BIS_STATE_SNK_STOP_WAITING:
			{
				if(check_bis_sink_state() == 8)
				{
					if (sink_stop_flag)
					{
						le_bis_sink_stop_scan();
						sink_stop_flag = 0;
						break;
					}

					if (target_role == BIS_SINK)
					{
						btManager.btBisState = BT_BIS_STATE_SNK_START_WAITING;
						gBtBisTimeout = 3000;
						break;
					}
				}

				if((check_bis_sink_state() == 0))
				{
					btManager.btBisState = BT_BIS_STATE_NONE;
					gBtBisTimeout = 0;

					if (!sink_wait_starting)
					{
						Set_bis_role( BIS_NONE );
						LeAudioSinkModeExit();
					}

					break;
				}

				if(gBtBisTimeout)
				{
					gBtBisTimeout--;
					if(gBtBisTimeout == 0)
					{
						btManager.btBisState = BT_BIS_STATE_SNK_STOP;
					}
				}
			}
			break;
		default:
			break;
	}
}

#ifdef BIS_SRC_SWITCH_MODES_WITHOUT_REBOOTING
bool BisSrcInsertZeroFlag = 0;
#endif

#define SRC_START_DELAY_TIMER	1000 //ms
static uint32_t src_delay_timer = 0;
extern uint32_t gSysTick;
void LeAudioSrcPause(void)
{
	if(Get_bis_role() != BIS_SRC)
		return;
#ifdef BIS_SRC_SWITCH_MODES_WITHOUT_REBOOTING
	//hfp模式需要断开bis
	if(GetSysModeState(ModeBtHfPlay) != ModeStateInit && GetSysModeState(ModeBtHfPlay) != ModeStateRunning)
	{
		SoftFlagRegister(SoftFlagLeAudioSrcSync);
		BisSrcInsertZeroFlag = 1;
		return;
	}
#endif
	while(!check_bis_src_is_in_idle())
	{
		BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_BIS_SRC_STOP);
		osTaskDelay(20);
	}

	SoftFlagRegister(SoftFlagLeAudioSrcResume);
}

void LeAudioSrcResume(void)
{
#ifdef BIS_SRC_SWITCH_MODES_WITHOUT_REBOOTING
	if(SoftFlagGet(SoftFlagLeAudioSrcSync))
	{
		SoftFlagDeregister(SoftFlagLeAudioSrcSync);
		if(Get_bis_role() == BIS_SRC)
		{
			extern void bis_sync_audio_close(void);
			bis_sync_audio_close();
		}		
		return;
	}
#endif	
	src_delay_timer = gSysTick;
	SoftFlagDeregister(SoftFlagLeAudioSrcUnMuteFlag);
}

void LeAudioSrcResumeProcess(void)
{
	extern bool le_bis_src_is_synced(void);

	if(mainAppCt.SysCurrentMode == ModeBtHfPlay || mainAppCt.SysCurrentMode == ModeIdle)
		return;

	if(SoftFlagGet(SoftFlagLeAudioSrcResume))
	{
		AudioCoreSourceMute(APP_SOURCE_NUM,TRUE,TRUE); 	//恢复SRC期间需要mute
		SoftFlagRegister(SoftFlagLeAudioSrcUnMuteFlag);

		if(
#ifdef CFG_FUNC_REMIND_SOUND_EN
			//等提示音播完
		    (!RemindSoundIsPlay()) &&
#endif
			(src_delay_timer + SRC_START_DELAY_TIMER) <= gSysTick)
		{
			BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_BIS_SRC_START);
			SoftFlagDeregister(SoftFlagLeAudioSrcResume);
		}
	}
	else
	{
		//恢复SRC完成需要unmute
		if(SoftFlagGet(SoftFlagLeAudioSrcUnMuteFlag) && le_bis_src_is_synced())
		{
			SoftFlagDeregister(SoftFlagLeAudioSrcUnMuteFlag);
			AudioCoreSourceUnmute(APP_SOURCE_NUM,TRUE,TRUE);
		}
	}
}
#endif

/************************************************************************************
 * @brief	Start bluetooth stack service.
 * @param	NONE
 * @return	
 ***********************************************************************************/
bool BtStackServiceStart(void)
{
	bool		ret = TRUE;

	if((btStackServiceCt) && (btStackServiceCt->serviceWaitResume))
	{
		btStackServiceCt->serviceWaitResume = 0;
		return ret;
	}

#ifdef	BT_SNIFF_RC_CLK
	sniff_clk_set(0);//sniff use rc
#endif

	memset((uint8_t*)BB_EM_MAP_ADDR, 0, BB_EM_SIZE);//clear em erea
	
	SoftFlagDeregister(SoftFlagDiscDelayMask);
	ClearBtManagerReg();

	SetBtStackState(BT_STACK_STATE_INITAILIZING);
	
	ret = BtStackServiceInit();
	if(ret)
	{
		btStackServiceCt->taskHandle = NULL;

		xTaskCreate(BtStackServiceEntrance, 
					"BtStack", 
					BT_STACK_SERVICE_STACK_SIZE, 
					NULL, 
					BT_STACK_SERVICE_PRIO, 
					&btStackServiceCt->taskHandle);
		if(btStackServiceCt->taskHandle == NULL)
		{
			ret = FALSE;
		}
		
#ifdef MVA_BT_OBEX_UPDATE_FUNC_SUPPORT
		void bt_obex_upgrate(void);
		xTaskCreate(bt_obex_upgrate,
							"bt_obex_upgrate",
							BT_OBEX_SERVICE_STACK_SIZE,
							NULL,
							BT_OBEX_SERVICE_PRIO,
							&bt_obex_taskHandle);
		if(bt_obex_taskHandle == NULL)
		{
			ret = FALSE;
		}
#endif

#ifdef SOFT_WACTH_DOG_ENABLE
		little_dog_adopt(DOG_INDEX3_BtStackTask);
#endif
	}
	if(!ret)
		APP_DBG("BtStack service create fail!\n");
	return ret;
}

/************************************************************************************
 * @brief	Kill bluetooth stack service.
 * @param	NONE
 * @return	
 ***********************************************************************************/
bool BtStackServiceKill(void)
{
#if (BT_SUPPORT)
	int32_t ret = 0;
	if(btStackServiceCt == NULL)
	{
		return FALSE;
	}
	
	//btStackService
	//Msgbox
	if(btStackServiceCt->msgHandle)
	{
		MessageDeregister(btStackServiceCt->msgHandle);
		btStackServiceCt->msgHandle = NULL;
	}
	
	//task
	if(btStackServiceCt->taskHandle)
	{
		vTaskDelete(btStackServiceCt->taskHandle);
		btStackServiceCt->taskHandle = NULL;
	}

	//deregister bt middleware message send interface
	BtAppiFunc_MessageSend(NULL);

	//stack deinit
	ret = BtStackUninit();
	if(!ret)
	{
		APP_DBG("Bt Stack Uninit fail!!!\n");
		return FALSE;
	}

	if(btStackConfigParams)
	{
		osPortFree(btStackConfigParams);
		btStackConfigParams = NULL;
	}
	//
	if(btStackServiceCt)
	{
		osPortFree(btStackServiceCt);
		btStackServiceCt = NULL;
	}
	APP_DBG("!!btStackServiceCt\n");
	
#ifdef SOFT_WACTH_DOG_ENABLE
	little_dog_deadopt(DOG_INDEX3_BtStackTask);
#endif

#endif
	return TRUE;
}

/***********************************************************************************
 * 
 **********************************************************************************/
void BtStackServiceWaitResume(void)
{
	if(btStackServiceCt == NULL)
		return;
	btStackServiceCt->serviceWaitResume = 1;
}

void BtStackServiceWaitClear(void)
{
	if(btStackServiceCt == NULL)
		return;
	btStackServiceCt->serviceWaitResume = 0;
}

/***********************************************************************************
 * BB错误报告
 * 注:需要判断当前是否在中断中，需要调用不同的消息发送函数接口
 **********************************************************************************/
void BBMatchReport(void)
{
	MessageContext		msgSend;
	msgSend.msgId		= MSG_BTSTACK_BB_ERROR;
	MessageSend(mainAppCt.msgHandle, &msgSend);

    APP_DBG("bb match or ke malloc fail\n");
}

void BBErrorReport(uint8_t mode, uint32_t errorType)
{
	MessageContext		msgSend;
	MessageHandle 		msgHandle;
	if(btStackServiceCt == NULL)
		return;
	
	msgHandle = btStackServiceCt->msgHandle;
	msgSend.msgId = MSG_BTSTACK_BB_ERROR;

	btStackServiceCt->bbErrorMode = mode;
	btStackServiceCt->bbErrorType = errorType;

	//isr
	MessageSendFromISR(msgHandle, &msgSend);
}

/***********************************************************************************
 * BB 中断关闭
 **********************************************************************************/
void BT_IntDisable(void)
{
	NVIC_DisableIRQ(DM_IRQn);
	NVIC_DisableIRQ(BT_IRQn);
	NVIC_DisableIRQ(BLE_IRQn);
}

/***********************************************************************************
 * BB 模块关闭
 **********************************************************************************/
//void BT_ModuleClose(bool power_flag)
//{
//#include "reset.h"
//	Reset_FunctionReset(BTDM_FUNC_SEPA|MDM_FUNC_SEPA);
//
//	if(!power_flag)
//		return;
//
//	Clock_Module2Disable(MDM_APLL_CLK_EN|BT32K_CLK_EN|BT_OR_PMU_32K_CLK_EN|PMU_32K_CLK_EN|MDM_DPLL_CLK_EN); //close clock
//	Clock_Module1Disable(BTDM_HCLK_EN|BTDM_24M_CLK_EN|MDM_PLL_CLK_EN);
//	Clock_Module3Disable(BTDM_LP_CLK_EN|MDM_12M_CLK_EN|MDM_24M_CLK_EN|MDM_48M_CLK_EN);
//
//#ifdef CFG_IDLE_MODE_DEEP_SLEEP	//有配置deepsleep唤醒
//	Clock_Module2Enable(BT_OR_PMU_32K_CLK_EN|PMU_32K_CLK_EN);
//#ifdef CFG_PARA_WAKEUP_SOURCE_IR //有配置IR唤醒(IR时钟和BTDM_LP复用)
//	Clock_Module2Enable(BT32K_CLK_EN);
//	Clock_Module3Enable(BTDM_LP_CLK_EN);
//#endif
//#endif
//}


/***********************************************************************************
 * 
 **********************************************************************************/
uint8_t GetBtStackCt(void)
{
	if(btStackServiceCt)
		return 1;
	else
		return 0;
}

/***********************************************************************************
 * 蓝牙恢复出厂设置
 **********************************************************************************/
static void BtRstStateCheck(void)
{
	switch(btManager.btRstState)
	{
		case BT_RST_STATE_NONE:
			break;
			
		case BT_RST_STATE_START:
			APP_DBG("bt reset start\n");
			// If there is a reconnectiong process, stop it
			if(sys_parameter.bt_ReconnectionEnable)
				BtReconnectDevStop();

			// If there is a bt link, disconnect it
			if(GetBtCurConnectFlag())
			{
				BtDisconnectCtrl(TRUE);
			}

			btManager.btRstState = BT_RST_STATE_WAITING;
			btManager.btRstWaitingCount = 0;
			break;
			
		case BT_RST_STATE_WAITING:
			if(btManager.btRstWaitingCount>=3000)
			{
				btManager.btRstWaitingCount = 2000;
				if(GetBtManager()->btConnectedProfile)
				{
					BtDisconnectCtrl(TRUE);
				}
				else if(GetBtDeviceConnState() == BT_DEVICE_CONNECTION_MODE_ALL)
				{
					btManager.btRstState = BT_RST_STATE_FINISHED;
				}
			}
			else
			{
				btManager.btRstWaitingCount++;
			}
			break;
			
		case BT_RST_STATE_FINISHED:
			btManager.btRstWaitingCount = 0;
			memset(btManager.remoteAddr, 0, 6);
			memset(btManager.btDdbLastAddr, 0, 6);
			
			BtDdb_EraseBtLinkInforMsg();
			
			btManager.btRstState = BT_RST_STATE_NONE;
			APP_DBG("bt reset complete\n");
			break;
			
		default:
			btManager.btRstState = BT_RST_STATE_NONE;
			break;
	}
}

extern void BT_RC_CLOSE();
void BtResetAndKill(bool power_flag)
{
	//bb reset
	if(power_flag)
	{
		RF_PowerDownBySw();
		// BT_RC_CLOSE();
	}
	WDG_Feed();
	BT_IntDisable();
	WDG_Feed();
	//Kill bt stack service
	BtStackServiceKill();
	WDG_Feed();
	osTaskDelay(10);
	// RF_PowerDownByHw();
	// BT_ModuleClose(power_flag);
}

/***********************************************************************************
 * 开关蓝牙
 * 断开蓝牙连接，删除蓝牙协议栈任务，关闭蓝牙晶振
 **********************************************************************************/
void BtPowerOff(void)
{
	uint8_t btDisconnectTimeout = 0;
	if(!btStackServiceCt)
		return;
	
	APP_DBG("[Func]:Bt off\n");
	
	if(GetBtStackState() == BT_STACK_STATE_INITAILIZING)
	{
		while(GetBtStackState() == BT_STACK_STATE_INITAILIZING)
		{
			osTaskDelay(10);
			btDisconnectTimeout++;
			if(btDisconnectTimeout>=100)
				break;
		}

		//快速在BT模式和其他模式(共2个模式)切换，需要delay(500);避免蓝牙初始化和反初始化状态未完成导致的错误
		//osTaskDelay(500);
		osTaskDelay(50);
	}

	if(GetBtDeviceConnState() == BT_DEVICE_CONNECTION_MODE_NONE)
	{
		BtDisconnectCtrl(TRUE);
	}

	//在蓝牙回连时,需要先取消蓝牙回连行为
	BtReconnectDevStop();
	osTaskDelay(50);

	//wait for bt disconnect, 2S timeout
	while(GetBtDeviceConnState() == BT_DEVICE_CONNECTION_MODE_NONE)
	{
		osTaskDelay(10);
		btDisconnectTimeout++;
		if(btDisconnectTimeout>=200)
			break;
	}
	
	BtResetAndKill(FALSE);
}

void BtPowerOn(void)
{
	APP_DBG("[Func]:Bt on\n");
    RF_PowerUpBySw();
    osTaskDelay(50);
	Clock_Module2Enable(ALL_MODULE2_CLK_SWITCH);
	osTaskDelay(50);

	//bt stack restart
	BtStackServiceStart();
}


/***********************************************************************************
 * 进入蓝牙DUT模式
 * 退出DUT模式后,最好重启系统
 **********************************************************************************/
void BtEnterDutModeFunc(void)
{
	uint8_t btDisconnectTimeout = 0;
	if(!GetBtStackCt())
	{
		APP_DBG("Enter dut mode fail\n");
		return;
	}

	if(!btManager.btDutModeEnable)
	{
		btManager.btDutModeEnable = 1;
		
		if(GetBtDeviceConnState() == BT_DEVICE_CONNECTION_MODE_NONE)
		{
			BtDisconnectCtrl(TRUE);
		}
		if(sys_parameter.bt_ReconnectionEnable)
			BtReconnectDevStop();
		
		APP_DBG("confirm bt disconnect\n");
		while(GetBtDeviceConnState() == BT_DEVICE_CONNECTION_MODE_NONE)
		{
			//2s timeout
			osTaskDelay(100);
			btDisconnectTimeout++;
			if(btDisconnectTimeout>=50)
				break;
		}

		APP_DBG("clear all pairing list\n");
		BtDdb_EraseBtLinkInforMsg();
		
		APP_DBG("[Enter dut mode]\n");
		Vis_Set(BtAccessModeGeneralAccessible,__FUNCTION__);
		BTEnterDutMode();
	}
}


/***********************************************************************************
 * 快速开关蓝牙
 * 断开蓝牙连接，蓝牙进入不可被搜索，不可被连接状态
 **********************************************************************************/
void BtFastPowerOff(void)
{
	BtScanPageStateSet(BT_SCAN_PAGE_STATE_CLOSING);
}

void BtFastPowerOn(void)
{
	if(btStackServiceCt == NULL)
		return;

	if(btStackServiceCt->serviceWaitResume)
	{
		if(!btManager.btLinkState)
		{
			BtSetAccessMode_select();
		}
		
		btStackServiceCt->serviceWaitResume = 0;
		return;
	}

	if(btManager.btExitSniffReconPhone)
	{
		BtScanPageStateSet(BT_SCAN_PAGE_STATE_OPEN_WAITING);
	}
	else
	{
		BtScanPageStateSet(BT_SCAN_PAGE_STATE_OPENING);
	}
}


/***********************************************************************************
 * 蓝牙名称的更新
 **********************************************************************************/
void BtLocalDeviceNameUpdate(char *deviceName)
{
	MessageContext		msgSend;
	MessageHandle 		msgHandle;
	uint32_t			nameLen;
	if(btStackServiceCt == NULL)
		return;
	if(deviceName == NULL)
		return;
	
	memset(sys_parameter.bt_LocalDeviceName, 0, BT_NAME_SIZE);
	nameLen = strlen(deviceName);
	if(nameLen > (BT_NAME_SIZE - 1))
	{
		nameLen = BT_NAME_SIZE - 1;
	}
	memcpy(sys_parameter.bt_LocalDeviceName, deviceName, nameLen);
	
	msgHandle = btStackServiceCt->msgHandle;
	msgSend.msgId = MSG_BTSTACK_LOCAL_DEVICE_NAME_UPDATE;

	MessageSend(msgHandle, &msgSend);
}

#elif (BLE_SUPPORT == ENABLE)
/************************************************************************************
 * @brief	Start ble stack service.
 * @param	NONE
 * @return
 ***********************************************************************************/
bool BleStackServiceStart(void)
{
	bool		ret = TRUE;
	memset((uint8_t*)BB_EM_MAP_ADDR, 0, BB_EM_SIZE);//clear em erea

	ret = BtStackServiceInit();
	if(ret)
	{
		btStackServiceCt->taskHandle = NULL;

		xTaskCreate(BtStackServiceEntrance,
					"BtStack",
					BT_STACK_SERVICE_STACK_SIZE,
					NULL,
					BT_STACK_SERVICE_PRIO,
					&btStackServiceCt->taskHandle);
		if(btStackServiceCt->taskHandle == NULL)
		{
			ret = FALSE;
		}

#ifdef MVA_BT_OBEX_UPDATE_FUNC_SUPPORT
		void bt_obex_upgrate(void);
		xTaskCreate(bt_obex_upgrate,
							"bt_obex_upgrate",
							BT_OBEX_SERVICE_STACK_SIZE,
							NULL,
							BT_OBEX_SERVICE_PRIO,
							&bt_obex_taskHandle);
		if(bt_obex_taskHandle == NULL)
		{
			ret = FALSE;
		}
#endif


#ifdef SOFT_WACTH_DOG_ENABLE
		little_dog_adopt(DOG_INDEX3_BtStackTask);
#endif
	}
	if(!ret)
		APP_DBG("BtStack service create fail!\n");
	return ret;
}
void BlePowerOn(void)
{
	APP_DBG("[Func]:Bt on\n");
    RF_PowerUpBySw();
    osTaskDelay(50);
	Clock_Module2Enable(ALL_MODULE2_CLK_SWITCH);
	osTaskDelay(50);

	//bt stack restart
	BleStackServiceStart();
}

void BBErrorReport(uint8_t mode, uint32_t errorType)
{
}

void BBMatchReport(void)
{
}

void BtFreqOffsetAdjustComplete(unsigned char offset)
{
}

#endif

#ifdef MV_APP_CONTROL
/*=============================================================================
 * 轮询发送函数（在主循环中调用）
 * 作用：从队列取出消息，真正通过BLE发送出去
 *============================================================================*/
#include "protocol_queue.h"
uint8_t mv_frame[512];
uint16_t mv_frameLen;
extern ProtocolQueue g_sendQueue; 
extern int ble_gs_app_send(const uint8_t* data, uint16_t len);
void Protocol_PollSend(void) {
		// 从队列中取出消息
		if (ProtocolQueue_Dequeue(&g_sendQueue, mv_frame, &mv_frameLen)) {

			if (!ble_gs_app_is_connected() || !ble_gs_app_is_ntf_enabled()) {
			printf("Not ready: connected=%d ntf_en=%d", 
					ble_gs_app_is_connected(), ble_gs_app_is_ntf_enabled());
		}else{
			ble_gs_app_send(mv_frame, mv_frameLen);
		} 
    }
}
#endif

#if (EXTERNAL_PA_LNA == ENABLE)
/*=============================================================================
 * 外部射频PA/LNA使能函数
 * 作用：在蓝牙协议栈初始化完成后，配置外部射频PA/LNA的使能引脚(RF_TXEN/RF_RXEN)，并设置外部PA/LNA的ENABLE和bypass引脚
 * RF_TXEN: A0/A10/B0/B18
 * RF_RXEN: A1/A11/B1/B19
 *============================================================================*/
static void enable_pa_lna(uint8_t enable)
{
	if(enable)
	{
		//1. gpio mode : enable RF_TXEN and RF_RXEN
		GPIO_PortAModeSet(GPIOA10,0x16);//GPIOA10:RF_TXEN
		GPIO_PortAModeSet(GPIOA11,0x15);//GPIOA11:RF_RXEN
		//2. set external PA/LNA enable pin and bypass pin
	}
}
#endif

#if((BT_SUPPORT == ENABLE) || (BLE_SUPPORT == ENABLE))

static void BtStackServiceEntrance(void * param)
{
	BtBbParams			bbParams;
	MessageContext		msgRecv;

	SetBluetoothMode(BLE_SUPPORT|(BT_SUPPORT<<1));

#ifdef LOW_POWER_TEST
	BtSniffReqAllowSet(1);
	BtSniffAttemptMinSet(1);
#endif

	//load bt stack all params
	LoadBtConfigurationParams();

	//BB init
	ConfigBtBbParams(&bbParams);

#ifdef BT_HIGH_RF_TXPOWER_ENABLE
	Bt_select_high_pa_mode(1);
#endif
	Bluetooth_common_init(&bbParams);

#if ( BT_SUPPORT )
	Bt_init((void*)&bbParams);
//	tws_slave_cap = bbParams.freqTrim;
#endif
	Vis_Init();
#if (BLE_SUPPORT)
#ifdef BLE_USE_DIFF_ADDR_THAN_BT
	app_set_ble_bd_addr(btStackConfigParams->ble_LocalDeviceAddr);//在rwble_enable_init初始化之前设置
#endif
	rwble_enable_init();
	uint16_t err = le_stack_init();
	if(err) {
	    uint8_t module_id = err & 0xFF;
	    uint8_t error_code = err >> 8;
	    APP_DBG("ble api 0x%02X init error, error code: 0x%02X\n", module_id, error_code);
	}else
	{
		APP_DBG("Ble Stack Init Success\n");
	}
#elif( (!BLE_SUPPORT) && ( BT_TWS_SUPPORT))
	rwble_enable_init();
#endif

#if (EXTERNAL_PA_LNA)
	enable_pa_lna(1);
#endif

	//host memory init
	SetBtPlatformInterface(&pfiOS, NULL/*&pfiBtDdb*/);
#if ( BT_SUPPORT )
	Name_confirm_Callback_Set(BtConnectDecide);
	//在蓝牙开启后台运行时,host的内存采用数组,避免存在申请/释放带来碎片化的风险
	BTStackMemAlloc(BT_STACK_MEM_SIZE, gBtHostStackMemHeap, 0);
#endif



	APP_DBG("BtStackServiceEntrance.\n");

#if BT_SOURCE_SUPPORT
#ifdef SOURCE_AUTO_INQUIRY_AND_CONNECT
	QuiryDelayTimerSet(5000);
#endif
#endif

#if (BT_SUPPORT)
	//BR/EDR init
	if(!BtStackInit())
	{
		APP_DBG("error init bt device\n");
		//出现初始化异常时,蓝牙协议栈任务挂起
		while(1)
		{
			MessageRecv(btStackServiceCt->msgHandle, &msgRecv, 0xFFFFFFFF);
		}
	}
	else
	{
		APP_DBG("bt device init success!\n");
	}
#endif
#if (BT_SUPPORT)
	SetBtStackState(BT_STACK_STATE_READY);
#endif
#ifdef BT_ACCESS_MODE_SET_BY_POWER_ON_TIMEOUT
	btManager.btvisibilityDelayOn = TRUE;
	btManager.btvisibilityDelayCount = 0;
#endif
	//BtMidMessageSend(MSG_BT_MID_STATE_FAST_ENABLE, 0);
#if (BT_SUPPORT)
	if(sys_parameter.bt_BackgroundType == BT_BACKGROUND_DISABLE)
		BtStackServiceMsgSend(MSG_BTSTACK_RUN_START);
#endif

#if BT_SOURCE_SUPPORT
	if(gSwitchSourceAndSink == A2DP_SET_SOURCE)
	{
		BtConnectCtrl();//sink->source时，增加一次回连
	}
#endif

#if ((BT_DID_SUPPORT == ENABLE) && (BT_SUPPORT == ENABLE))
	//register did service
	{
		extern void DviceID_Register_SdpService(void);
		DviceID_Register_SdpService();
	}
#endif

#ifdef BLE_TUYA_ENABLE
	uint16_t Tuya_adv_stop_count = 2000;
#endif

	while(1)
	{
		MessageRecv(btStackServiceCt->msgHandle, &msgRecv, 1);

		rw_main();

#if(BT_SUPPORT == ENABLE)
		if(msgRecv.msgId)
			BtStackMsgProcess(msgRecv.msgId);
#endif

#if(BT_SUPPORT == ENABLE)
		BTStackRun();
		BtEventFlagProcess();//bt事件处理
#endif

#ifdef LOW_POWER_TEST
#ifdef EXTERNAL_TRIGGER
		BtExtTriggerProcess();
#endif
		if (rwip_sleep_enable())
		{
			continue;
		}
#endif

#if(BT_SUPPORT == ENABLE)
		if(GetAudioCoreServiceState() == TaskStatePaused)
		{
			MessageContext		msgSend;
			msgSend.msgId		= MSG_NONE;
			MessageSend(GetAudioCoreServiceMsgHandle(), &msgSend);
		}
#endif

#if (BT_SUPPORT)

		if(BtDisconnectProcess)
			BtDisconnectProcess();

		if(BtAvrcpConProcess)
			BtAvrcpConProcess();

		if(BtAvrcpDisconProcess)
			BtAvrcpDisconProcess();

		if(BtScoSendProcess)
			BtScoSendProcess();

		//通话过程中执行获取手机通话状态流程
		if(BtHfpGetCurCallStateProcess)
			BtHfpGetCurCallStateProcess();

	#if (BT_LINK_DEV_NUM>=2) && defined(FUNC_BIS_SUPPORT)
		bis_restart_process();
	#endif
#endif

#if (BT_SUPPORT == ENABLE)
		if(btManager.hfpScoDiscDelay)
		{
			btManager.hfpScoDiscDelay--;
			if(btManager.hfpScoDiscDelay == 0)
			{
				BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_HF_SCO_DISCONNECT);
			}
		}
#endif

#if (BT_SUPPORT)
		if(btManager.btConStateProtectCnt)
		{
			btManager.btConStateProtectCnt++;
			if(btManager.btConStateProtectCnt>=10000)
			{
				btManager.btConStateProtectCnt=0;
				APP_DBG("bt connect state protect end...\n");
			}
		}

		CheckBtEventTimer();


		extern uint32_t a2dp_pause_delay_cnt;
		if(a2dp_pause_delay_cnt)
		{
			a2dp_pause_delay_cnt--;
			if(a2dp_pause_delay_cnt == 0)
			{
				APP_DBG("a2dp_pause_delay_cnt  \n");
				uint8_t cfg_index = (btManager.cur_index == 0)? 1 : 0;
				if((GetA2dpState(cfg_index) == BT_A2DP_STATE_STREAMING) && (GetSystemMode() == ModeBtHfPlay))
				{
					APP_DBG("pause....%d...  \n", cfg_index);
					AvrcpCtrlPause(cfg_index);
				}
			}
		}
#endif

#if (BT_HFP_SUPPORT && (BT_SUPPORT == ENABLE))
		extern void BtModeEnterDetect(void);
		BtModeEnterDetect();
#endif

#if (BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT)
		BtBisProcess();
#endif

#ifdef MV_APP_CONTROL
		Protocol_PollSend();
#endif

#ifdef BLE_TUYA_ENABLE
		if(Tuya_adv_stop_count)
		{
			Tuya_adv_stop_count--;
			if(!Tuya_adv_stop_count)
			{
				extern uint8_t ble_set_adv_flag;
				ble_set_adv_flag = 1;
				ble_gs_app_stop_adv();
			}
		}
#endif

#if (BT_TWS_SUPPORT == ENABLE)
		tws_bis_switch_process();

		if((btManager.twsState >= BT_TWS_STATE_CONNECTING)&&(GetSystemMode() != ModeBtHfPlay))
		{
			tws_audio_state_sync();
		}
		BtTwsRunLoop();
		twsRunLoop();

		#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
		//TwsDoubleKeyPairTimeOut();
		#endif
#endif

#ifdef SOFT_WACTH_DOG_ENABLE
		little_dog_feed(DOG_INDEX3_BtStackTask);
#endif

#ifdef LOG_ANALYSIS_DBG_SWITCH
        GPIOX_TOGGLE(3, 1);
        GPIOX_TOGGLE(5, 1);
#endif
#ifdef PAGE_ENTER_DEEPSLEEP
		static uint32_t count = 0;
		count ++;
		if(count == 5000)
		{
#if 1
        	printf("%s, %d, \r\n",__func__, __LINE__);
			BtChangePowerMode(2);
#endif
		}
#endif
	}
}

/************************************************************************************
 * @brief	Start bluetooth stack service initial.
 * @param	NONE
 * @return
 ***********************************************************************************/
static bool BtStackServiceInit(void)
{
	APP_DBG("bluetooth stack service init.\n");
#if((BT_SUPPORT == ENABLE) || ((BLE_SUPPORT == ENABLE)))
	btStackServiceCt = (BtStackServiceContext*)osPortMalloc(sizeof(BtStackServiceContext));
	if(btStackServiceCt == NULL)
	{
		return FALSE;
	}
	memset(btStackServiceCt, 0, sizeof(BtStackServiceContext));
#endif

	btStackConfigParams = (BT_CONFIGURATION_PARAMS*)osPortMalloc(sizeof(BT_CONFIGURATION_PARAMS));
	if(btStackConfigParams == NULL)
	{
		osPortFree(btStackServiceCt);
		btStackServiceCt = NULL;
		return FALSE;
	}
	memset(btStackConfigParams, 0, sizeof(BT_CONFIGURATION_PARAMS));

	btStackServiceCt->msgHandle = MessageRegister(BT_STACK_NUM_MESSAGE_QUEUE);
	if(btStackServiceCt->msgHandle == NULL)
	{
		osPortFree(btStackServiceCt);
		osPortFree(btStackConfigParams);
		btStackServiceCt = NULL;
		btStackConfigParams = NULL;
		return FALSE;
	}
#if(BT_SUPPORT == ENABLE)
	//register bt middleware message send interface
	BtAppiFunc_MessageSend(BtMidMessageManage);
#endif

#if (BT_TWS_SUPPORT == ENABLE)
	BtTwsAppInit();
#endif
	return TRUE;
}

#ifdef FUNC_BIS_SUPPORT
uint32_t bis_enter_msg_temp = 0;
uint32_t bis_src_restart_time = 0;
uint8_t bis_enter_btlink_check(uint32_t msg)
{
	extern uint8_t MeGetActiveLinkNum(void);
	if(BT_LINK_DEV_NUM<2)
	{
		printf("bis_enter_btlink_check BT_LINK_DEV_NUM %d\n",BT_LINK_DEV_NUM);
		return 0;
	}

	if(MeGetActiveLinkNum()>=2)
	{
		printf("bis_enter_btlink_check MeGetActiveLinkNum %d\n",MeGetActiveLinkNum());
		bis_src_restart_time = 0;
		bis_enter_msg_temp = msg;
		BTDisconnect(!btManager.cur_index);//断开未活跃链路
		return 1;
	}

	return 0;
}

void bis_restart_process(void)
{
	MessageContext		msgSend;

	if(bis_enter_msg_temp)
		bis_src_restart_time++;
	else
		bis_src_restart_time = 0;

	if(bis_src_restart_time>=1000)
	{
		msgSend.msgId = bis_enter_msg_temp;
		MessageSend(GetMainMessageHandle(), &msgSend);
		bis_enter_msg_temp = 0;
	}
}

uint8_t BtLinkStateGet(void)
{
	uint8_t ret = 0;
#if (BT_LINK_DEV_NUM < 2)
	ret = btManager.btLinked_env[0].btLinkState;
#else
	ret = btManager.btLinked_env[0].btLinkState + ((btManager.btLinked_env[1].btLinkState)<<1);
#endif
	return ret;
}

uint8_t BtMultiLinkCheck(void)
{
#ifdef FUNC_BIS_SUPPORT
	if(Get_bis_role()!= BIS_NONE)
		return 0;
#endif

	return 1;
}
#endif//FUNC_BIS_SUPPORT

#else
void BtStackServiceMsgSend(uint16_t msgId)
{
//	MessageContext		msgSend;
//	msgSend.msgId = msgId;
//	if(btStackServiceCt)
//		MessageSend(btStackServiceCt->msgHandle, &msgSend);
}
void BtFreqOffsetAdjustComplete(unsigned char offset)
{
}
#endif

