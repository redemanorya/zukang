#include <string.h>
#include "main_task.h"
#include "remind_sound.h"
#include "powercontroller.h"
#include "irqn.h"
#include "reset.h"
#include "rtc.h"
#include "audio_core_service.h"
#include "watchdog.h"
#include "bt_config.h"
#include "bb_api.h"
#include "bt_manager.h"
#include "bt_stack_service.h"
#include "audio_effect.h"
#include "audio_vol.h"


//#ifdef CFG_APP_SAFE_MODE_EN

bool SafeModeInit(void)
{
	DBG("Safe Mode Init\n");
#if (defined(CFG_APP_BT_MODE_EN) && (BT_HFP_SUPPORT))
	if((GetA2dpState(BtCurIndex_Get()) >= BT_A2DP_STATE_CONNECTED)
		|| (GetHfpState(BtCurIndex_Get()) >= BT_HFP_STATE_CONNECTED)
		|| (GetAvrcpState(BtCurIndex_Get()) >= BT_AVRCP_STATE_CONNECTED))
	{
		//手动断开
		BtStackServiceMsgSend(MSG_BTSTACK_MSG_BT_DISCONNECT_DEV_CTRL);
	}
#endif

	PauseAudioCore();
	DBG("Safe Mode run\n");
#ifdef CFG_APP_BT_MODE_EN
	if(sys_parameter.bt_BackgroundType == BT_BACKGROUND_POWER_ON)
		BtFastPowerOff();
#endif
	return TRUE;
}

bool SafeModeDeinit(void)
{
	DBG("Safe Mode Deinit\n");

	if(IsAudioPlayerMute() == FALSE)
	{
		HardWareMuteOrUnMute();
	}	
	
	PauseAudioCore();
	//注意：AudioCore父任务调整到mainApp下，此处只关闭AudioCore通道，不关闭任务
	AudioCoreProcessConfig((void*)AudioNoAppProcess);
	ModeCommonDeinit();//通路全部释放

#ifdef CFG_APP_BT_MODE_EN
	if(sys_parameter.bt_BackgroundType != BT_BACKGROUND_DISABLE)
	{
		BtStackServiceMsgSend(MSG_BTSTACK_RUN_START);
		if(sys_parameter.bt_BackgroundType == BT_BACKGROUND_POWER_ON)
			BtFastPowerOn();
	}
#endif
	return TRUE;
}

void SafeModeRun(uint16_t msgId)
{
	switch(msgId)
	{
		default:
			CommonMsgProccess(msgId);
			break;
	}

}

//#endif


