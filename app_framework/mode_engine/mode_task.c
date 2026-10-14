/**
 **************************************************************************************
 * @file    sys_mode.c
 * @brief   Program Entry 
 *
 * @author  ken
 * @version V1.0.0
 *
 * $Created: 2021-02-26 
 *
 * @Copyright (C) 2021, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
//Port#include <components/soft_watchdog/soft_watch_dog.h>
//driver、lib
#include "type.h"
#include <string.h>
#include "watchdog.h"

//application
#include "app_config.h"
#include "bt_config.h"
#include "debug.h"
#include "main_task.h"
#include "remind_sound.h"
#include "mode_parameter.h"
#include "idle_mode.h"
#include "breakpoint.h"
#include "rtos_api.h"
//Port#include "device_detect.h"
#include "audio_core_service.h"
#include "audio_vol.h"
#include "recorder_service.h"
#include "media_play_api.h"
//Port#include "can_func.h"
#include "otg_device_standard_request.h"
#include "otg_device_stor_request.h"
//Port#include "uart_cmd.h"

#include "mvwireless2_config.h"
#ifdef MVAPP_UPDATA_EN
#include "fw_upgrade.h"
#endif

#define SYS_MODE_NUM_MESSAGE_QUEUE			20
#define SYS_MODE_MSG_TIMEOUT 				1
#ifdef	FUNC_WIRELESS2_SUPPORT
//wirless2 状态下mode task轮转清，可高速执行。
#define MODE_TASK_PRIO						5
#else
#define MODE_TASK_PRIO						4
#endif
#define MODE_TASK_STACK_SIZE				768//1024

//鍏宠仈CFG_APP_WIRELESSIN_MODE_EN + CFG_WIRELESS_EN
#define	ALWAY_PLUG_OUT_MODE					(BIT(ModeIdle) | BIT(ModeBtHfPlay) | BIT(ModeLeAudioSinkPlay)| BIT(ModeTwsSlavePlay))

#ifdef AUDIO_STREAM_UNINTERRUPT
uint32_t IsResRamReallocate = 0;//正常情况下不需要释放
uint32_t IsResRamRelease = 0;//通话模式需要释放资源，所以需要做处理
#endif

static SysModeContext SysModeCt = {NULL,NULL};
static uint32_t sPlugOutMode = ALWAY_PLUG_OUT_MODE;

extern void IdlePrevModeSet(SysModeNumber mode);
extern void IdleModeEnter(void);
extern void IdleModeExit(void);

osMutexId SysModeMutex = NULL;

static struct
{
	uint16_t 		Msg;
	uint16_t 		cnt;
	uint16_t 		delay;
	SysModeNumber 	EnterMode;
}EventSendAgain = {0,0,0,0};

void StartEventSendAgain(uint16_t Msg,SysModeNumber EnterMode)
{
	if(EventSendAgain.Msg != Msg)
	{
		EventSendAgain.Msg			= Msg; 			//
		EventSendAgain.cnt 			= 10;  			//
		EventSendAgain.delay 		= 200; 			//
		EventSendAgain.EnterMode 	= EnterMode; 	//
	}	
}

bool IsEventSendAgain(void)
{
	return (EventSendAgain.Msg != 0);
}

void EventSendAgainProcess(void)
{
	if(EventSendAgain.Msg != 0)
	{
		if(GetSysModeState(EventSendAgain.EnterMode) == ModeStateInit || 
			GetSysModeState(EventSendAgain.EnterMode) == ModeStateRunning ||
			GetSysModeState(EventSendAgain.EnterMode) == ModeStateSusend
			)
		{
			EventSendAgain.Msg = 0;
		}
		else
		{
			if(EventSendAgain.delay > 0)
			{
				EventSendAgain.delay--;
				if(EventSendAgain.delay == 0)
				{
					if(EventSendAgain.cnt > 0)
					{
						MessageContext		msgSend;
						EventSendAgain.cnt--;

						msgSend.msgId		= EventSendAgain.Msg;
						MessageSend(GetMainMessageHandle(), &msgSend);
						APP_DBG("---------------------------------plug msg again 0x%x\n",EventSendAgain.Msg);
						
						if(EventSendAgain.cnt == 0)
							EventSendAgain.Msg = 0;
						else
							EventSendAgain.delay = 200;
					}
				}
			}
		}
	}
}


/**
 * @brief printf mode string
 * @param Mode:ModeBtAudioPlay etc...
 * @return none
 */
const char* GetModeNameStr(SysModeNumber Mode)
{
	uint32_t i_count = 0;

	for(;i_count < MODE_STR_AND_REMIND_MAX_NUMBER;i_count++)
	{
		if(Mode == (ModeNameStr[i_count]).ModeNumber)
		{
			return (ModeNameStr[i_count]).ModeName;
		}
	}
	return "unknow mode";
	
}

xTaskHandle GetSysModeTaskHandle(void)
{
	return SysModeCt.taskHandle;
}

/**
 * @brief mode task creat
 * @param none
 * @return none
 */
void SysModeTaskCreat(void)
{	
	if(xTaskCreate(SysModeEntrance, "SysMode", MODE_TASK_STACK_SIZE, NULL, MODE_TASK_PRIO, &SysModeCt.taskHandle) != pdTRUE)
	{
		osTaskDelay(50);
		APP_DBG("system error ,mode task create fail 1\n");
		if(xTaskCreate(SysModeEntrance, "SysMode", MODE_TASK_STACK_SIZE, NULL, MODE_TASK_PRIO, &SysModeCt.taskHandle) != pdTRUE)
		{
			while(1)
			{
				osTaskDelay(50);
				APP_DBG("system error ,mode task create fail 2\n");
			}
		}
		else
		{
#ifdef SOFT_WACTH_DOG_ENABLE
		little_dog_adopt(DOG_INDEX1_ModeTask);
#endif
		}
	}
	else
	{
#ifdef SOFT_WACTH_DOG_ENABLE
		little_dog_adopt(DOG_INDEX1_ModeTask);
#endif
	}
#ifdef MVAPP_UPDATA_EN
	extern int upgrade_module_init(const upgrade_config_t *config);
	upgrade_module_init(NULL);
#endif
}

/**
 * @brief mode task delete
 * @param none
 * @return none
 */
static void SysModeTaskKill(void)
{
	MessageHandle		msgHandleTemp = NULL;
	
	if(SysModeCt.taskHandle == NULL)
	{
		return;
	}
	msgHandleTemp =	SysModeCt.msgHandle;
	SysModeCt.msgHandle = NULL;
	if(msgHandleTemp != NULL)
	{
		MessageClear(msgHandleTemp);
	}
	if(ModeInputFunction.UdiskUnlock)
	{
		ModeInputFunction.UdiskUnlock();
	}
	if(ModeInputFunction.UdiskHsUnlock)
	{
		ModeInputFunction.UdiskHsUnlock();
	}
	if(ModeInputFunction.SDCardForceExit)
	{
		ModeInputFunction.SDCardForceExit();
	}
	vTaskDelete(SysModeCt.taskHandle);
	SysModeCt.taskHandle = NULL;
	MessageDeregister(msgHandleTemp);
//Port	while(!TaskDeleteCompleted());
#ifdef SOFT_WACTH_DOG_ENABLE
	little_dog_deadopt(DOG_INDEX1_ModeTask);
#endif
}

/**
 * @brief get mode message handle
 * @param none
 * @return message handle
 */
MessageHandle GetSysModeMsgHandle(void)
{
	return SysModeCt.msgHandle;
}

/**
 * @brief send message to sysmode
 * @param msgId
 * @return
 */
bool SendSysModeMsg(uint16_t msgId)
{
	MessageContext		msgSend;
	msgSend.msgId		= msgId;
	return MessageSend(SysModeCt.msgHandle, &msgSend);
}

/**
 * @brief get the mode index in the SysMode[]
 * @param mode varible pointer
 * @return mode index in the SysMode[]
 */
uint32_t GetModeIndexInModeLoop(SysModeNumber *sys_mode)
{
	uint32_t mode_index=0;
	for(;mode_index < SYS_MODE_MAX_NUMBER;mode_index++)
	{
		if(SysMode[mode_index].ModeNumber == *sys_mode)// find the index in sysmode
			return mode_index;
	}
	
	*sys_mode = SysMode[SYS_MODE_MAX_NUMBER-1].ModeNumber;
	return SYS_MODE_MAX_NUMBER-1;
}

/**
 * @brief get power on mode and set it to mainAppCt.PowerOnMode
 * @param mode varible pointer
 * @return mode index in the SysMode[]
 */
void PowerOnModeGenerate(void *BpSysInfo)
{
	uint32_t i_count = ModeIdle;
	uint32_t default_count = SYS_MODE_MAX_NUMBER-1;
	SysModeNumber	Mode = ModeIdle;

#ifdef CFG_FUNC_BREAKPOINT_EN
	BP_SYS_INFO * pBpSysInfo = BpSysInfo;
	if(pBpSysInfo !=NULL)
	{
		if((pBpSysInfo->CurModuleId > ModeIdle)&&(pBpSysInfo->CurModuleId < SysModeMax))
		{
			Mode = pBpSysInfo->CurModuleId; 
		}
	}
#endif
	for(i_count = 0;i_count < SYS_MODE_MAX_NUMBER;i_count++)
	{
		if(SysMode[i_count].ModeState == ModeStateInit)
		{
			SysMode[i_count].ModeState = ModeStateReady;
			if(default_count == SYS_MODE_MAX_NUMBER-1)
				default_count = i_count;	
		}
	}	

	mainAppCt.SysPrevMode = SysMode[SYS_MODE_MAX_NUMBER-1].ModeNumber;
	
	if(Mode == ModeIdle)// Do not have memory mode, set a default mode 	
	{
		Mode = SysMode[default_count].ModeNumber;
	}

	APP_DBG("power on mode %s\n",GetModeNameStr(Mode));

	if(GetModeDefineState(ModeIdle))
	{
		mainAppCt.SysPrevMode = Mode;
		mainAppCt.SysCurrentMode = ModeIdle;
	}
	else
	{
		mainAppCt.SysCurrentMode = Mode;
	}
	SysMode[GetModeIndexInModeLoop(&mainAppCt.SysCurrentMode)].ModeState = ModeStateInit;

	IdlePrevModeSet(Mode);
}

/**
 * @brief set mode state 
 * @param sys_mode: mode ,sys_mode_state:mode state,example ModeStateReady
 * @return none
 */
void SetSysModeState(SysModeNumber sys_mode,SysModeState sys_mode_state)
{
	SysMode[GetModeIndexInModeLoop(&sys_mode)].ModeState=sys_mode_state;
}

/**
 * @brief get mode state 
 * @param sys_mode: mode 
 * @return mode state,example ModeStateReady
 */
SysModeState GetSysModeState(SysModeNumber sys_mode)
{
	if(!GetModeDefineState(sys_mode)) //没有被定义的模式
		return ModeStateSusend;
	return SysMode[GetModeIndexInModeLoop(&sys_mode)].ModeState;
}

bool GetModeDefineState(SysModeNumber sys_mode)
{
	uint32_t i_count = 0;
	for(;i_count < SYS_MODE_MAX_NUMBER ;i_count++)
	{
		if(SysMode[i_count].ModeNumber == sys_mode)return TRUE;
	}
	return FALSE;
}
/**
 * @brief send mode msg to main task
 * @param none
 * @return none
 */
void SendModeKeyMsg(void)
{
	MessageContext		msgSend;
	msgSend.msgId		= MSG_SOFT_MODE;
	MessageSend(GetMainMessageHandle(), &msgSend);
}

/**
 * @brief mode enter
 * @param SetMode:which mode you want to enter
 * @return none
 * @note : setmode must be ModeStateReady state.if (setmode==ModeStateSusend), please : SetSysModeState(setmode,ModeStateReady)
*/
void SysModeEnter(SysModeNumber SetMode)
{
#ifdef AUDIO_STREAM_UNINTERRUPT
	//ModeNumber = SysModeMax;
	IsResRamReallocate = 0;
	IsResRamRelease = 0;
#endif
	gChangeModeTimeOutTimer = CHANGE_MODE_TIMEOUT_COUNT;
	if(GetSysModeState(mainAppCt.SysCurrentMode) == ModeStateDeinit)
	{
		return;
	}
	if(GetSysModeState(SetMode) == ModeStateSusend)
	{
		SendModeKeyMsg();
		return;
	}
	if(GetSysModeState(SetMode) == ModeStateInit || GetSysModeState(SetMode) == ModeStateDeinit || GetSysModeState(SetMode) == ModeStateRunning )
	{
#ifdef AUDIO_STREAM_UNINTERRUPT
		if(mainAppCt.SysCurrentMode == ModeBtHfPlay)
		{
			IsResRamReallocate = TRUE;
		}
#endif
		return;	
	}

	if(GetSysModeState(mainAppCt.SysCurrentMode) == ModeStateInit || GetSysModeState(mainAppCt.SysCurrentMode) == ModeStateRunning)
	{
		SysMode[GetModeIndexInModeLoop(&mainAppCt.SysCurrentMode)].ModeState = ModeStateDeinit;
	}
	
	SysMode[GetModeIndexInModeLoop(&SetMode)].ModeState = ModeStateInit;
#ifdef AUDIO_STREAM_UNINTERRUPT
	if(SetMode == ModeBtHfPlay)
	{
		IsResRamRelease = TRUE;
	}
#endif
}

/**
 * @brief plug event process
 * @param Msg:plug event msg ,for example MSG_DEVICE_SERVICE_U_DISK_IN
 * @return none
 */
static void SysModeGenerateByPlugEvent(uint16_t Msg)
{
	uint32_t i_count=0;
	uint32_t i;

	for(;i_count < SYS_DEVICE_DETECT_MAX_NUMBER;i_count++)
	{
		if(Msg == DeviceEventMsgTableArray[i_count].MsgID)
		{

			osMutexLock(SysModeMutex);
			for(i=0;i < SYS_MODE_MAX_NUMBER;i++)
			{
				SysModeNumber mode = SysMode[i].ModeNumber;

				//
				if( ((!(BIT(mode) & DeviceEventMsgTableArray[i_count].SupportMode)) &&
					(GetSysModeState(mode) == ModeStateInit || GetSysModeState(mode) == ModeStateRunning))
#ifdef	CFG_FUNC_RECORDER_EN
					|| SoftFlagGet(SoftFlagRecording)	//褰曢煶灞忚斀鎷旀彃浜嬩欢
#endif
					)
				{
					break;
				}
			}

			if((i >= SYS_MODE_MAX_NUMBER) && (!gInsertEventDelayActTimer))
			{
				APP_DBG("plug msg 0x%x and msg total number %ld\n",Msg,SYS_DEVICE_DETECT_MAX_NUMBER);
				if(DeviceEventMsgTableArray[i_count].EnterMode != ENTERR_PREV_MODE)
				{
					SysModeEnter(DeviceEventMsgTableArray[i_count].EnterMode);
					// plug in
					if(GetSysModeState(DeviceEventMsgTableArray[i_count].EnterMode) != ModeStateInit)
					{
						StartEventSendAgain(Msg,DeviceEventMsgTableArray[i_count].EnterMode);
					}
				}
				else
				{
					sPlugOutMode |= DeviceEventMsgTableArray[i_count].SupportMode;//BIT(mainAppCt.SysCurrentMode);
					SysModeEnter(mainAppCt.SysPrevMode);
				}
				osMutexUnlock(SysModeMutex);
				break;
			}
			osMutexUnlock(SysModeMutex);
		}
	}
}

bool IsEnterPreMode(SysModeNumber pre_mode)
{
	if (pre_mode == ModeUDiskAudioPlay)
	{
		#ifdef CFG_FUNC_HOST_FS_DETECT
		if(GetUdiscState() == DETECT_STATE_IN)
		#endif
		{
			return TRUE;
		}
	}
	else if (pre_mode == ModeUDiskHsAudioPlay)
	{
		#ifdef CFG_FUNC_HOST_HS_DETECT
		if(GetUdiscHsState() == DETECT_STATE_IN)
		#endif
		{
			return TRUE;
		}
	}
	else if (pre_mode == ModeCardAudioPlay)
	{
		#ifdef CFG_FUNC_CARD_DETECT
		if (GetCardState() == DETECT_STATE_IN)
		#endif
		{
			return TRUE;
		}
	}
	else
	{
		return TRUE;
	}
	return FALSE;
}


/**
 * @brief all mode generate except PowerOnModeGenerate()
 * @param Msg:plug event msg ,for example MSG_DEVICE_SERVICE_U_DISK_IN
 * @return none
 */
void SysModeGenerate(uint16_t Msg)
{
	uint32_t count=0;
	SysModeNumber mode_search_count=0;
	
#ifdef CFG_FUNC_RECORDER_EN
	extern TaskState GetMediaRecorderState(void);
	extern void RecServierToParentAgain(uint16_t id);
	if(GetMediaRecorderState() == TaskStateRunning &&
	  (Msg == MSG_MODE||Msg ==MSG_SOFT_MODE || Msg == MSG_ENTER_IDLE_MODE || Msg == MSG_QUIT_IDLE_MODE ||
		Msg == MSG_DEVICE_SERVICE_BTHF_IN || Msg == MSG_DEVICE_SERVICE_BTHF_OUT)
	   )//
	{
		RecServierToParentAgain(Msg);
		return;
	}
#endif

	if(Msg == MSG_MODE||Msg ==MSG_SOFT_MODE)
	{
		if(Msg == MSG_MODE)
		{
			if(IsEventSendAgain()) //Plug in
				return;
			for(count = 0;count < MODE_KEY_INVALID_MAX_NUMBER;count++)
			{
				if(GetSysModeState(ModeKeyInvalid[count]) == ModeStateInit || GetSysModeState(ModeKeyInvalid[count]) == ModeStateRunning)
				{
					return;
				}
			}
		}
		osMutexLock(SysModeMutex);
		count = 0;
		mode_search_count = GetModeIndexInModeLoop(&mainAppCt.SysCurrentMode);// find the index in sysmode
		do
		{//find the ready mode
			count++;
			mode_search_count++;
			if(mode_search_count >= SYS_MODE_MAX_NUMBER)
			{
				mode_search_count=0;
			}
		}while((SysMode[mode_search_count].ModeState!=ModeStateReady || SysMode[mode_search_count].ModeNumber == ModeSafe) && count < SYS_MODE_MAX_NUMBER);

		if(count == SYS_MODE_MAX_NUMBER)
		{
			osMutexUnlock(SysModeMutex);
			if(Msg == MSG_SOFT_MODE)
				IdleModeEnter();
			return;// not find mode,not action
		}
	#ifdef CFG_FUNC_RECORDER_EN
		if((mainAppCt.SysPrevMode != ModeBtHfPlay) && (mainAppCt.SysPrevMode != ModeIdle) && IsNormalPlayback())
		{
			APP_DBG("ModeUDiskPlayBack deinit back pre mode!");
			if (!IsEnterPreMode(mainAppCt.SysPrevMode))
			{
				SysModeEnter(SysMode[mode_search_count].ModeNumber);
			}
			else
			{
				SysModeEnter(mainAppCt.SysPrevMode);//
			}
		}else{
			SysModeEnter(SysMode[mode_search_count].ModeNumber);	
		}	
	#else
		SysModeEnter(SysMode[mode_search_count].ModeNumber);
	#endif
		osMutexUnlock(SysModeMutex);
	}
	else if(Msg == MSG_ENTER_IDLE_MODE)
	{
		IdleModeEnter();
	}
	else if(Msg == MSG_QUIT_IDLE_MODE)
	{
		IdleModeExit();
	}
	else
	{
		SysModeGenerateByPlugEvent(Msg);
	}
	
}

/**
 * @brief manage change mode time out 
 * @param none
 * @return none
 */
void SysModeChangeTimeoutProcess(void)
{
	if(SysMode[GetModeIndexInModeLoop(&mainAppCt.SysCurrentMode)].ModeState == ModeStateDeinit)
	{
		if(gChangeModeTimeOutTimer == 0)
		{
			if(SoftFlagGet(SoftFlagMediaModeRead))
			{
				APP_DBG("====SoftFlagMediaModeRead === \n");
				SoftFlagRegister(SoftFlagMediaModeExitTimeOut);
				gChangeModeTimeOutTimer = CHANGE_MODE_TIMEOUT_COUNT;
				return;
			}
			osMutexLock(SysModeMutex);
			SysModeTaskKill();			
			APP_DBG("create mode task again\n");
			SysModeTaskCreat();
			osSemaphoreMutexUnlock();
			gChangeModeTimeOutTimer = CHANGE_MODE_TIMEOUT_COUNT;
		}
	}
	else
	{
		gChangeModeTimeOutTimer = CHANGE_MODE_TIMEOUT_COUNT;
	}		
}

/**
 * @brief mode de initialization
 * @param none
 * @return none
 */
static void SysModeDeinit(void)
{
	uint32_t Deinit_count=0;
	for(;Deinit_count < SYS_MODE_MAX_NUMBER;Deinit_count++)
	{
		if(SysMode[Deinit_count].ModeState == ModeStateDeinit)
		{
			TIMER time_out;
			gChangeModeTimeOutTimer = CHANGE_MODE_SYS_REMIND;
			osMutexLock(SysModeMutex);
			if(ModeInputFunction.RemindRun)
			{
				TimeOutSet(&time_out,1000);
				while(ModeInputFunction.RemindRun(ModeStateDeinit) && (!IsTimeOut(&time_out)));
				if(IsTimeOut(&time_out))
				{
#ifdef SOFT_WACTH_DOG_ENABLE
					little_dog_feed(DOG_INDEX1_ModeTask);
#endif
					WDG0_Feed();

					APP_DBG("RemindRun Deinit time out!\n");
				}
			}

			APP_DBG("--Deinit mode-- %s\n",GetModeNameStr(SysMode[Deinit_count].ModeNumber));
			gChangeModeTimeOutTimer = CHANGE_MODE_TIMEOUT_COUNT*7;
#ifdef CFG_FUNC_RECORDER_EN
			if(GetMediaRecorderTaskHandle() != NULL)
			{
				APP_DBG("media rec task del\n");
				MediaRecorderServiceDeinit();
			}
#endif
			SysMode[Deinit_count].SysModeDeInit();
			if(SysModeCt.msgHandle != NULL)
			{
				MessageClear(SysModeCt.msgHandle);
			}
			if(sPlugOutMode&(BIT(SysMode[Deinit_count].ModeNumber)))
			{
				SysMode[Deinit_count].ModeState = ModeStateSusend;
				sPlugOutMode = ALWAY_PLUG_OUT_MODE;
			}
			else
			{
#ifdef CFG_FUNC_RECORDER_EN
				if(SysMode[Deinit_count].ModeNumber == ModeCardPlayBack
				|| SysMode[Deinit_count].ModeNumber == ModeUDiskPlayBack
				|| SysMode[Deinit_count].ModeNumber == ModeUDiskHsPlayBack)
				{
					SysMode[Deinit_count].ModeState = ModeStateSusend;
				}else
#endif
				{
					SysMode[Deinit_count].ModeState = ModeStateReady;
				}
			}
			mainAppCt.SysPrevMode = SysMode[Deinit_count].ModeNumber;
			gChangeModeTimeOutTimer = CHANGE_MODE_TIMEOUT_COUNT * 2;
#if defined(HWSRC_MODE_CONFIG_EN) && defined(AUDIOCORE_HWSRC_EN)
			HwSrcAppModeClear(SysMode[Deinit_count].ModeNumber);
#endif
			osMutexUnlock(SysModeMutex);
		}
	}
	
}

/**
 * @brief mode  initialization
 * @param none
 * @return none
 */
static void SysModeInit(void)
{
	uint32_t init_count=0;
	uint32_t count;

	for(init_count = 0;init_count < SYS_MODE_MAX_NUMBER;init_count++)
	{
		if(SysMode[init_count].ModeState == ModeStateInit)
		{
			osMutexLock(SysModeMutex);
			//
			for(count = 0;count < SYS_MODE_MAX_NUMBER;count++)
			{
				if(SysMode[count].ModeState == ModeStateDeinit)
				{
					osMutexUnlock(SysModeMutex);
					return;
				}
			}			
			mainAppCt.SysCurrentMode = SysMode[init_count].ModeNumber;
#if defined(HWSRC_MODE_LIST_SCAN_EN) && defined(AUDIOCORE_HWSRC_EN)
			HwSrcAppModePreSet(SysMode[init_count].ModeNumber);
#endif
			if(ModeInputFunction.RemindRun)
			{
				ModeInputFunction.RemindRun(ModeStateInit);
			}

			APP_DBG("\n SysModeInit-----SysPrevMode = %s SysCurrentMode = %s-----\n",GetModeNameStr(mainAppCt.SysPrevMode),GetModeNameStr(mainAppCt.SysCurrentMode));
			if(SysMode[init_count].SysModeInit() == TRUE)
			{
				SysMode[init_count].ModeState = ModeStateRunning;	
				SoftFlagDeregister(SoftFlagAudioCoreSourceIsDeInit);
				if(ModeInputFunction.AudioCoreResume)
				{
					ModeInputFunction.AudioCoreResume();

#ifdef CFG_LOW_DELAY_CTRL
					void lowDelaySyncFlagSet(bool Flag);
					lowDelaySyncFlagSet(1);
#endif
				}
#ifdef CFG_APP_BIS_BT_DUAL_CHIP_MODE_EN
				if(GetBisRole() == LE_BIS_SRC
				   || GetBisRole() == LE_BIS_SINK
				   || mainAppCt.SysCurrentMode == ModeLeAudioLinein
				   || mainAppCt.SysCurrentMode == ModeLeAudioBT)
					Audio_BisDACSwtichChannal(1);
				else
					Audio_BisDACSwtichChannal(0);
#endif
			}
			else
			{				
			    //初始化失败，状态设为挂起，直接进入安全模式
				SysMode[init_count].ModeState = ModeStateSusend;
				SetSysModeState(ModeSafe, ModeStateReady);
				SysModeEnter(ModeSafe);
				osMutexUnlock(SysModeMutex);
				APP_DBG("mode init failure,enter safe mode\n");
				break;
			}
			osMutexUnlock(SysModeMutex);

	#ifdef CFG_APP_BIS_BT_DUAL_CHIP_MODE_EN
			SendCurModeToSlave();
	#endif
		}
	}
}

/**
 * @brief mode  run
 * @param none
 * @return none
 */
static void SysModeRun(uint16_t MsgId)
{
	uint32_t i_count=0;
	for(i_count = 0;i_count < SYS_MODE_MAX_NUMBER;i_count++)
	{
		if(SysMode[i_count].ModeState == ModeStateDeinit)
		{
			return;
		}
	}
	for(i_count = 0;i_count < SYS_MODE_MAX_NUMBER;i_count++)
	{
		if(SysMode[i_count].ModeState == ModeStateRunning)
		{
#if (BT_TWS_SUPPORT == ENABLE)
			if(MsgId == MSG_BT_TWS_EFFECT_SWTICH)
			{
				extern void tws_a2dp_refresh(void);
				SysCurModeReboot();
				tws_a2dp_refresh();
			}
#endif
			SysMode[i_count].SysModeRun(MsgId);
		}
	}
}

/**
 * @brief mode  task
 * @param none
 * @return none
 */
static void SysModeEntrance(void * param)
{
	MessageContext		msg;
	
	SysModeCt.msgHandle = MessageRegister(SYS_MODE_NUM_MESSAGE_QUEUE);
	while(1)
	{
		MessageRecv(SysModeCt.msgHandle, &msg, SYS_MODE_MSG_TIMEOUT);
#ifdef SOFT_WACTH_DOG_ENABLE
		little_dog_feed(DOG_INDEX1_ModeTask);
#endif

#ifdef CFG_FUNC_RECORDER_EN
		void MediaRecorderEncode(void);
		MediaRecorderEncode();
#endif
		WDG0_Feed();
#ifndef	FUNC_WIRELESS2_SUPPORT
		SysModeDeinit();
		SysModeInit();
#endif
		if(ModeInputFunction.RemindRun)
		{
			ModeInputFunction.RemindRun(ModeStateRunning);
		}
#if (BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT)
		{
			extern void LeAudioSrcResumeProcess(void);
			LeAudioSrcResumeProcess();
		}
#endif
#if defined CFG_RES_USB_FS_MIX_AUDIO_EN		//TODO 由FS声卡并无前台模式，暂与MIX合并
		#ifdef CFG_PARA_DEVICE_FS_DET
		if(GetUSBFsDeviceInitState())
		#endif
		{
			OTG_DeviceRequestProcess(USB_DEVICE_FS);
		#ifdef CFG_OTG_MODE_READER_EN
			OTG_DeviceStorProcess(USB_DEVICE_FS);
		#endif
		}
#elif defined CFG_COMMUNICATION_BY_FS_USB
#ifdef	FUNC_WIRELESS2_SUPPORT
		if((GetSystemMode() != ModeUsbFsDevicePlay)
#else
		if((GetSystemMode() != ModeIdle)
		&& (GetSystemMode() != ModeUsbFsDevicePlay)
#endif
		)
		{
			#ifdef CFG_PARA_DEVICE_FS_DET
			if(GetUSBFsDeviceInitState())
			#endif
			{
				OTG_DeviceRequestProcess(USB_DEVICE_FS);
			#ifdef CFG_OTG_MODE_READER_EN
				OTG_DeviceStorProcess(USB_DEVICE_FS);
			#endif
			}
		}
#endif
#ifdef CFG_RES_USB_HS_MIX_AUDIO_EN
		#ifdef CFG_PARA_DEVICE_HS_DET
		if(GetUSBHsDeviceInitState())
		#endif
		{
			OTG_DeviceRequestProcess(USB_DEVICE_HS);
		#ifdef CFG_OTG_MODE_READER_EN
			OTG_DeviceStorProcess(USB_DEVICE_HS);
		#endif
		#ifdef CFG_FUNC_USB_PROTOCOL_AUTO
			OTG_HS_DeviceAudioModeAutoSwitchProcess();
		#endif
		}
#endif
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN//usb path
#ifdef	FUNC_WIRELESS2_SUPPORT
	if((GetSystemMode() != ModeUsbDevicePlay)
#else
	if((GetSystemMode() != ModeIdle)
	&& (GetSystemMode() != ModeUsbDevicePlay)
//		&& (GetSystemMode() != ModeUDiskAudioPlay)
#endif
	)
	{
	#if defined CFG_COMMUNICATION_BY_HS_USB && !defined CFG_RES_USB_HS_MIX_AUDIO_EN && defined CHIP_PIN_HSOTG
		#ifdef CFG_PARA_DEVICE_HS_DET
		if(GetUSBHsDeviceInitState())
		#endif
		{
			OTG_DeviceRequestProcess(USB_DEVICE_HS);
		}
	#endif
	}
#ifdef CFG_COMMUNICATION_BY_UART
		uart_data_entry();
#endif
#ifdef CFG_UPGRADE_BY_UART
		uartupgrade_data_entry();
#endif
#endif
#ifdef CFG_FUNC_RECORDER_EN
		RecServierMsg(msg.msgId);
#endif
#ifndef	FUNC_WIRELESS2_SUPPORT
		SysModeRun(msg.msgId);
#else
		SendWireless2ModeMsg(msg.msgId);
#endif
		EventSendAgainProcess();

#ifdef CFG_FUNC_CAN_DEMO_EN
		CAN_FuncEntrance();
#endif
#ifdef CFG_APP_BIS_BT_DUAL_CHIP_MODE_EN
		UART_FuncEntrance(msg.msgId);
#endif
	}
}

bool SysCurModeReboot(void)
{
	uint32_t mode_index = GetModeIndexInModeLoop(&mainAppCt.SysCurrentMode);
	if(IsAudioPlayerMute() == FALSE)
	{
		HardWareMuteOrUnMute();
	}
	SoftFlagRegister(SoftFlagSysCurModeReboot);
	SysMode[mode_index].SysModeDeInit();
//	AudioCoreMixParamSet(DefaultNet, AudioEffectParambin->user_effect_list->sample_rate, AudioEffectParambin->user_effect_list->frame_size, WIDTH_L24BIT);
	if(!SysMode[mode_index].SysModeInit())
	{
		SoftFlagDeregister(SoftFlagSysCurModeReboot);
		if(IsAudioPlayerMute() == TRUE)
		{
			HardWareMuteOrUnMute();
		}
		return FALSE;
	}
//		SysMode[mode_index].SysModeRun(0);
#ifdef CFG_FUNC_REMIND_SOUND_EN
	RemindSoundClearPlay();
#endif
	SoftFlagDeregister(SoftFlagAudioCoreSourceIsDeInit);
	if(ModeInputFunction.AudioCoreResume)
	{
		ModeInputFunction.AudioCoreResume();
	}
	SoftFlagDeregister(SoftFlagSysCurModeReboot);
	if(IsAudioPlayerMute() == TRUE)
	{
		HardWareMuteOrUnMute();
	}
	return TRUE;
}

/**
 * @brief 核心函数：判断当前是否为回放播放模式（布尔型，业务层直接用）
 * @return bool  true=回放播放，false=非回放播放
 */
bool IsNormalPlayback(void)
{
	if(GetSystemMode() == ModeUDiskPlayBack
	|| GetSystemMode() == ModeUDiskHsPlayBack
	|| GetSystemMode() == ModeCardPlayBack)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
/**
 * @brief 核心函数：判断当前是否为正常播放模式（布尔型，业务层直接用）
 * @return bool  true=正常播放，false=非正常播放
 */
bool IsNormalPlay(void)
{
	if(GetSystemMode() == ModeUDiskAudioPlay
	|| GetSystemMode() == ModeCardAudioPlay
	|| GetSystemMode() == ModeUDiskHsAudioPlay)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
