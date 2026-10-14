/*
 * effect_service.c
 *
 *  Created on: Mar 3, 2026
 *      Author: piwang
 */
//driver、lib
#include <effect_service.h>
#include <string.h>
#include "type.h"
//application
#include "app_config.h"
#include "rtos_api.h"
#include "app_message.h"
#include "debug.h"
#include "main_task.h"
#include "audio_core_service.h"
#include "mvwireless2_config.h"
#ifdef SOFT_WACTH_DOG_ENABLE
#include "soft_watch_dog.h"
#endif

typedef struct _EffecServiceContext
{
	xTaskHandle			taskHandle;
	MessageHandle		msgHandle;

	TaskState			serviceState;
	AudioCoreProcessFunc EffectProcess;
	uint8_t				ProcessNet;
}EffecServiceContext;

#ifdef CFG_FUNC_EFFECT_SERVICE_EN
static EffecServiceContext		EffectServiceCt;
/**
 * @brief	Effect proc servcie init
 * @param	NONE
 * @return	0 for success
 */
static int32_t EffectServiceInit(void)
{
	memset(&EffectServiceCt, 0, sizeof(EffecServiceContext));

	/* register message handle */
	EffectServiceCt.msgHandle = MessageRegister(EFFECT_SERVICE_NUM_MESSAGE_QUEUE);
	if(EffectServiceCt.msgHandle == NULL)
	{
		return -1;
	}
	EffectServiceCt.serviceState = TaskStateCreating;

	return 0;
}

/*static void EffectServiceDeinit(void)
{
	EffectServiceCt.msgHandle = NULL;
	EffectServiceCt.serviceState = TaskStateNone;
}*/

static void EffectServiceEntrance(void * param)
{
	MessageContext		msgRecv;
	MessageContext		msgSend;
#if defined(CFG_PARA_CORE2_NET)
	NetContext *PNetCt = &AudioCore.NetCt[CFG_PARA_CORE2_NET];
#endif
	EffectServiceCt.serviceState = TaskStateReady;

	/* Send message to parent */
	msgSend.msgId		= MSG_EFFECT_SERVICE_CREATED;
	MessageSend(GetAudioCoreServiceMsgHandle(), &msgSend);

	while(1)
	{
#if defined(CFG_FUNC_EFFECT_CORE_DUAL_EN)
	#if defined(FRAME_SYNC_TRIGER_EN)  && defined(CFG_LOW_DELAY_CTRL)
		if(AudioCore.AudioSource[FRAME_SYNC_SOURCE_NUM].InitFlag && (AudioCore.AudioSource[FRAME_SYNC_SOURCE_NUM].Net == CFG_PARA_CORE2_NET) && !AudioCore.AudioFlowRenew)
		{
			MessageRecv(EffectServiceCt.msgHandle, &msgRecv, 10);
		}
		else
	#endif
		{
			MessageRecv(EffectServiceCt.msgHandle, &msgRecv, 1);
		}
#else
		MessageRecv(EffectServiceCt.msgHandle, &msgRecv, EFFECT_SERVICE_TIMEOUT);
#endif
#ifdef SOFT_WACTH_DOG_ENABLE
		little_dog_feed(DOG_INDEX5_EffectTask);
#endif

		switch(msgRecv.msgId)
		{
			case MSG_TASK_START:
				if(EffectServiceCt.serviceState == TaskStateStarting)
				{
					EffectServiceCt.serviceState = TaskStateRunning;

					msgSend.msgId		= MSG_EFFECT_SERVICE_STARTED;
					MessageSend(GetAudioCoreServiceMsgHandle(), &msgSend);
				}
				break;

			case MSG_TASK_STOP:
				//clear msg
				MessageClear(EffectServiceCt.msgHandle);
				//Set state
				EffectServiceCt.serviceState = TaskStateStopped;
				//reply
				msgSend.msgId		= MSG_EFFECT_SERVICE_STOPPED;
				MessageSend(GetAudioCoreServiceMsgHandle(), &msgSend);
				break;
			default:
				break;
		}

#ifdef FUNC_WIRELESS2_SUPPORT
		WireSinkAudioPlayDataCore2_Set();
#endif

		if(EffectServiceCt.serviceState == TaskStateRunning
				&& GetAudioCoreServiceState() == TaskStateRunning
#if defined(CFG_PARA_CORE2_NET)
				&& !(AudioCore.Pause && (AudioCore.NetCt[CFG_PARA_CORE2_NET].State == MIX_STATE_IDLE))
				&& !(AudioCore.AudioFlowRenew && (AudioCore.NetCt[CFG_PARA_CORE2_NET].State == MIX_STATE_IDLE))
				&& AudioCore.AudioEffectProcess != NULL
#else
				&& EffectServiceCt.EffectProcess != NULL
#endif
							)
		{
#ifndef CFG_FUNC_EFFECT_CORE_DUAL_EN
			//Effect 在转模式的时候可能暂停 Mark
			EffectServiceCt.EffectProcess((AudioCoreContext*)&AudioCore, EffectServiceCt.ProcessNet);
			EffectServiceCt.EffectProcess = NULL;
			MessageContext		msgSend;
			msgSend.msgId		= MSG_NONE;
			MessageSend(GetAudioCoreServiceMsgHandle(), &msgSend);
#else
			switch(PNetCt->State)
			{
				case MIX_STATE_IDLE:

					if(AudioCoreSyncing(CFG_PARA_CORE2_NET) == FALSE)
					{
						break;
					}
					AudioCoreSourceMuteApply(CFG_PARA_CORE2_NET);
					PNetCt->State = MIX_STATE_GOT;
				case MIX_STATE_GOT:
					if(AudioCore.AudioEffectProcess != NULL)
					{
						AudioCore.AudioEffectProcess((AudioCoreContext*)&AudioCore, CFG_PARA_CORE2_NET);
					}
					else
						return;

					AudioCoreSinkMuteApply(CFG_PARA_CORE2_NET);
					AudioCoreSynced(CFG_PARA_CORE2_NET);
					PNetCt->State = MIX_STATE_IDLE;
					break;
				default:
					return;
			}

#endif
		}
	}
}


/**
 * @brief	send message to effect service
 * @param	NONE
 * @return	TRUE/FALSE
 */
bool SendEffectServiceMsg(uint16_t msgId)
{
	MessageContext		msgSend;
	msgSend.msgId		= msgId;
	return MessageSend(EffectServiceCt.msgHandle, &msgSend);
}

TaskState GetEffectServiceState(void)
{
	return EffectServiceCt.serviceState;
}

void SetEffectPause(void)
{
	EffectServiceCt.serviceState = TaskStatePaused;
}

/**
 * @brief	Create effect proc service.
 * @param	 NONE
 * @return
 */
int32_t EffectServiceCreate(void)
{
	int32_t		ret = 0;

	ret = EffectServiceInit();
	if(!ret)
	{
		EffectServiceCt.taskHandle = NULL;

		xTaskCreate(EffectServiceEntrance,
					"EffProc",
					EFFECT_SERVICE_SIZE,
					NULL, EFFECT_SERVICE_PRIO,
					&EffectServiceCt.taskHandle);

		if(EffectServiceCt.taskHandle == NULL)
		{
			ret = -1;
		}
	}
	if(ret)
	{
		APP_DBG("EffectService fail!\n");
	}
	else
	{
#ifdef SOFT_WACTH_DOG_ENABLE
		little_dog_adopt(DOG_INDEX5_EffectTask);
#endif
	}
	return ret;
}

/**
 * @brief	Start effect proc service.
 * @param	 NONE
 * @return
 */
int32_t EffectServiceStart(void)
{
	MessageContext		msgSend;

	EffectServiceCt.serviceState = TaskStateStarting;

	msgSend.msgId		= MSG_TASK_START;
	MessageSend(EffectServiceCt.msgHandle, &msgSend);
	return 0;
}

int32_t EffectServicePause(void)
{
	MessageContext		msgSend;

	msgSend.msgId		= MSG_TASK_PAUSE;
	MessageSend(EffectServiceCt.msgHandle, &msgSend);
	return 0;
}

void EffectServiceResume(void)
{
	MessageContext		msgSend;

	msgSend.msgId		= MSG_TASK_RESUME;
	MessageSend(EffectServiceCt.msgHandle, &msgSend);
}


/**
 * @brief	Exit effect proc service.
 * @param	NONE
 * @return
 */
void EffectServiceStop(void)
{
	MessageContext		msgSend;

	msgSend.msgId		= MSG_TASK_STOP;
	MessageSend(EffectServiceCt.msgHandle, &msgSend);
}

int32_t EffectServiceKill(void)
{
	//task	先删任务，再删邮箱，收资源
	if(EffectServiceCt.taskHandle != NULL)
	{
		vTaskDelete(EffectServiceCt.taskHandle);
		EffectServiceCt.taskHandle = NULL;
	}

	//Msgbox
	if(EffectServiceCt.msgHandle != NULL)
	{
		MessageDeregister(EffectServiceCt.msgHandle);
		EffectServiceCt.msgHandle = NULL;
	}
#ifdef SOFT_WACTH_DOG_ENABLE
	little_dog_deadopt(DOG_INDEX5_EffectTask);
#endif
	//PortFree...
	return 0;
}

void EffectProcessConfig(AudioCoreProcessFunc Process, uint8_t FlowNet)
{
	EffectServiceCt.EffectProcess = Process;
	EffectServiceCt.ProcessNet = FlowNet;

	MessageContext		msgSend;
	msgSend.msgId		= MSG_NONE;
	MessageSend(EffectServiceCt.msgHandle, &msgSend);
#ifdef FUNC_WIRELESS2_SUPPORT
	WireAudioPlayDecoderProcess();
#endif
}

AudioCoreProcessFunc GetEffectProcess(void)
{
	return EffectServiceCt.EffectProcess;
}
#endif //CFG_FUNC_EFFECT_SERVICE_EN

