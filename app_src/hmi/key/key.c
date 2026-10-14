/**
 **************************************************************************************
 * @file    Key.c
 * @brief   
 *
 * @author  pi
 * @version V1.0.0
 *
 * $Created: 2018-01-11 17:30:47$
 *
 * @Copyright (C) 2018, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#include "adc_levels.h"
#include "app_config.h"
#include "app_message.h"
#include "debug.h"
#include "key.h"
#include "ctrlvars.h"
#include "timeout.h"
#include "beep.h"
#include "mode_task.h"
#include "bt_config.h"
#ifdef CFG_RES_ADC_KEY_SCAN
#include "adc_key.h"
#endif
#ifdef CFG_RES_IR_KEY_SCAN
#include "ir_key.h"
#endif
#ifdef CFG_RES_CODE_KEY_USE
#include "code_key.h"
#endif
#ifdef CFG_RES_IO_KEY_SCAN
#include "io_key.h"
#endif
#ifdef CFG_APP_IDLE_MODE_EN
#include "idle_mode.h"
#endif
#include "pmu_powerkey.h"
/*************************************************
* ADC KEY按键属性对应消息列表
*
*  注：用户需要根据功能定义重新修改此表！！！
*  按键调整某一音效：默认调整EffectParambin(主图)
***************************************************/
static const uint16_t ADKEY_TAB[][5] =
{
	/*KEY_PRESSED					SHORT_RELEASE                  	LONG_PRESS                  KEY_HOLD                    LONG_PRESS_RELEASE*/

	/*power adc key*/
#if defined(CFG_IDLE_MODE_POWER_KEY) && (POWERKEY_MODE == POWERKEY_MODE_PUSH_BUTTON)
	{MSG_NONE,						MSG_PLAY_PAUSE,					MSG_POWERDOWN,  			MSG_NONE,					MSG_NONE},
#else
	{MSG_NONE,						MSG_PLAY_PAUSE,					MSG_NONE,  					MSG_NONE,					MSG_NONE},
#endif
	{MSG_HID_KEY1_DOWN,				MSG_HID_KEY_UP,					MSG_NONE,					MSG_NONE,					MSG_HID_KEY_UP},
	{MSG_HID_KEY2_DOWN,				MSG_HID_KEY_UP,					MSG_NONE,					MSG_NONE,					MSG_HID_KEY_UP},
	{MSG_HID_KEY3_DOWN,				MSG_HID_KEY_UP,					MSG_NONE,					MSG_NONE,					MSG_HID_KEY_UP},
	{MSG_NONE,						MSG_EFFECTPARAM_SWITCH,			MSG_NONE,					MSG_NONE,					MSG_NONE},
	{MSG_NONE,						MSG_MUSIC_BASS_UP,				MSG_MUSIC_BASS_DW,			MSG_MUSIC_BASS_DW,			MSG_NONE},
	{MSG_NONE,						MSG_MUSIC_TREB_UP,				MSG_MUSIC_TREB_DW,			MSG_MUSIC_TREB_DW,			MSG_NONE},
	{MSG_NONE,						MSG_MIC_VOLDOWN,				MSG_MIC_VOLDOWN,			MSG_MIC_VOLDOWN,			MSG_NONE},
	{MSG_NONE,						MSG_MIC_VOLUP,					MSG_MIC_VOLUP,				MSG_MIC_VOLUP,				MSG_NONE},
#if MSG_EFFECTFLOW_SWITCH_EN
	{MSG_NONE,						MSG_EFFECTFLOW_SWITCH,			MSG_EFFECTPARAM_SWITCH,		MSG_NONE,					MSG_NONE},
#else
	{MSG_NONE,						MSG_MIC_TREB_UP,				MSG_MIC_TREB_DW, 			MSG_MIC_TREB_DW,			MSG_NONE},
#endif
	{MSG_NONE,						MSG_MIC_BASS_UP,				MSG_MIC_BASS_DW, 			MSG_MIC_BASS_DW,			MSG_NONE},

	/*adc1 key*/
#if(defined(BT_SNIFF_ENABLE))
	{MSG_NONE,						MSG_PLAY_PAUSE,   				MSG_BT_SNIFF,      			MSG_NONE,                   MSG_NONE},
#else
	{MSG_NONE,						MSG_PLAY_PAUSE,   				MSG_DEEPSLEEP,      		MSG_NONE,                   MSG_NONE},
#endif
	{MSG_NONE,						MSG_PRE,          				MSG_FB_START,               MSG_FB_START,               MSG_FF_FB_END},
	{MSG_NONE,						MSG_NEXT,	        			MSG_FF_START,               MSG_FF_START,               MSG_FF_FB_END},
	{MSG_NONE,						MSG_MUSIC_VOLDOWN,				MSG_MUSIC_VOLDOWN,   		MSG_MUSIC_VOLDOWN,			MSG_NONE},
	{MSG_NONE,						MSG_MUSIC_VOLUP,				MSG_MUSIC_VOLUP,     		MSG_MUSIC_VOLUP,     		MSG_NONE},
	{MSG_NONE,						MSG_EFFECTPARAM_EQ,	        	MSG_EFFECTPARAM_EQ,        	MSG_NONE,                   MSG_NONE},
	{MSG_NONE,						MSG_MUTE,         				MSG_MUTE,        			MSG_NONE,                   MSG_NONE},
	{MSG_NONE,						MSG_NONE,   					MSG_NONE, 					MSG_NONE,                   MSG_NONE},
#if (BT_SOURCE_SUPPORT )
	{MSG_NONE,						MSG_BT_SOURCE_DISCONNECT,/*MSG_REPEAT,*/MSG_REPEAT_AB,     	MSG_NONE,                   MSG_NONE},
	{MSG_NONE,						MSG_BT_SOURCE_INQUIRY,/*MSG_REC,*/MSG_REC_PLAYBACK,      	MSG_NONE,                   MSG_NONE},
#else	
	#ifdef CFG_FUNC_RECORD_EXTERN_FLASH_EN
	{MSG_NONE,						MSG_REC1,  						MSG_REC1_PLAYBACK,			MSG_NONE,                   MSG_NONE},
	{MSG_NONE,						MSG_REC2,     					MSG_REC2_PLAYBACK,			MSG_NONE,                   MSG_NONE},
	#else
	{MSG_NONE,						MSG_REPEAT,  					MSG_REPEAT_AB,              MSG_NONE,                   MSG_NONE},
	{MSG_NONE,						MSG_REC,     					MSG_REC_PLAYBACK,           MSG_NONE,                   MSG_NONE},
	#endif
#endif//#if (BT_SOURCE_SUPPORT )
#ifdef CFG_FUNC_RECORD_EXTERN_FLASH_EN
	{MSG_NONE,						MSG_MODE,  						MSG_DEL_ALL_REC, 			MSG_NONE,                   MSG_NONE},
#else
	{MSG_NONE,						MSG_MODE,  						MSG_BT_TWS_PAIRING, 		MSG_NONE,                   MSG_NONE},
#endif

	/*adc2 key*/
	{MSG_NONE,						MSG_LE_BIS,  					MSG_BT_HF_CALL_REJECT,     	MSG_NONE,                   MSG_NONE},
#if BLE_SUPPORT && BT_LE_AUDIO_SUPPORT
	{MSG_NONE,						MSG_LE_BIS_SRC_START,    		MSG_LE_BIS_SRC_STOP,      	MSG_NONE,                   MSG_NONE},
	{MSG_NONE,						MSG_LE_BIS_SNK_START,	 		MSG_LE_BIS_SNK_STOP,  		MSG_NONE,                   MSG_NONE},
#else
	{MSG_NONE,						MSG_FOLDER_PRE,    				MSG_BROWSE,      			MSG_NONE,                   MSG_NONE},
	{MSG_NONE,						MSG_FOLDER_NEXT,	 			MSG_REC_FILE_DEL,  			MSG_NONE,                   MSG_NONE},
#endif
	{MSG_NONE,						MSG_RTC_SET_TIME,	     		MSG_RTC_DISP_TIME,          MSG_NONE,                   MSG_NONE},
	{MSG_NONE,						MSG_RTC_SET_ALARM,	     		MSG_NONE,              		MSG_NONE,                   MSG_NONE},
	{MSG_NONE,						MSG_RTC_UP,	     		    	MSG_ADJUST_ONEPARAM,		MSG_NONE,                   MSG_NONE},
	{MSG_NONE,						MSG_RTC_DOWN,       		    MSG_BT_HF_TRANS_CHANGED,    MSG_NONE,                   MSG_NONE},
	{MSG_NONE,						MSG_BT_CONNECT_CTRL, 			MSG_BT_ENTER_DUT_MODE,      MSG_NONE,                   MSG_NONE},
	{MSG_NONE,						MSG_BT_CONNECT_MODE,          	MSG_UPDATE,                 MSG_NONE,                   MSG_BT_XM_AI_STOP},
	{MSG_NONE,						MSG_LE_SCAN_START,   			MSG_NONE, 					MSG_NONE,  					MSG_NONE},
	{MSG_NONE,						MSG_NONE,     				MSG_NONE,   				MSG_NONE,    				MSG_NONE},

};

/*************************************************
* GPIO按键属性对应消息列表
*
*  注：用户需要根据功能定义重新修改此表！！！
***************************************************/
static const uint16_t IOKEY_TAB[][5] =
{
    /*KEY_PRESSED					SHORT_RELEASE                 	LONG_PRESS                  KEY_HOLD                    LONG_PRESS_RELEASE*/
	{MSG_NONE,						MSG_EFFECTFLOW_SWITCH,        	MSG_SOFT_POWER,  			MSG_NONE,                   MSG_NONE},
	{MSG_NONE,						MSG_MUTE,          			  	MSG_RGB_MODE,     		  	MSG_NONE,    				MSG_NONE},
	{MSG_NONE,						MSG_NONE,	    			  	MSG_NONE,    			    MSG_NONE,   				MSG_NONE},
};

/*************************************************
* code按键属性对应消息列表
*
*  注：用户需要根据功能定义重新修改此表！！！
***************************************************/
static const uint16_t CODEKEY_TAB[][5] =
{
	/*KEY_PRESSED					SHORT_RELEASE                 	LONG_PRESS                  KEY_HOLD                    LONG_PRESS_RELEASE*/
	{MSG_NONE,						MSG_MUSIC_VOLDOWN,        		MSG_MUSIC_VOLDOWN,   		MSG_MUSIC_VOLDOWN,  		MSG_NONE},
	{MSG_NONE,						MSG_MUSIC_VOLUP,          		MSG_MUSIC_VOLUP,     		MSG_MUSIC_VOLUP,    		MSG_NONE},
};

/*************************************************
* 遥控按键属性对应消息列表
*
*  注：用户需要根据功能定义重新修改此表！！！
***************************************************/
static const uint16_t IRKEY_TAB[][5] =
{
	/*KEY_PRESSED					SHORT_RELEASE                 	LONG_PRESS                  KEY_HOLD                    LONG_PRESS_RELEASE*/

#if(defined(BT_SNIFF_ENABLE))
	{MSG_NONE,						MSG_BT_SNIFF,    				MSG_BT_SNIFF,  				MSG_NONE,                   MSG_NONE},
#else
	{MSG_NONE,						MSG_DEEPSLEEP,    				MSG_DEEPSLEEP,  			MSG_NONE,                   MSG_NONE},
#endif
	{MSG_NONE,						MSG_MODE,  						MSG_NONE,                   MSG_NONE,                   MSG_NONE},
	{MSG_NONE,						MSG_MUTE,	        			MSG_NONE,                   MSG_NONE,                   MSG_NONE},
	{MSG_NONE,						MSG_PLAY_PAUSE,   		    	MSG_NONE,                   MSG_NONE,                   MSG_NONE},
	{MSG_NONE,						MSG_PRE,          				MSG_FB_START,         		MSG_FB_START,               MSG_FF_FB_END},
	{MSG_NONE,						MSG_NEXT,	        			MSG_FF_START,         		MSG_FF_START,               MSG_FF_FB_END},
#ifdef CFG_FUNC_MUSIC_EQ_EN
	{MSG_NONE,						MSG_EFFECTPARAM_EQ,	        	MSG_EFFECTPARAM_EQ,        				MSG_NONE,                   MSG_NONE},
#else
	{MSG_NONE,						MSG_NONE,	        			MSG_NONE,        				MSG_NONE,                   MSG_NONE},
#endif
	{MSG_NONE,						MSG_MUSIC_VOLUP,	    		MSG_MUSIC_VOLUP,      		MSG_MUSIC_VOLUP,     		MSG_NONE},
	{MSG_NONE,						MSG_MUSIC_VOLDOWN,	    		MSG_MUSIC_VOLDOWN,    		MSG_MUSIC_VOLDOWN,   		MSG_NONE},
	{MSG_NONE,						MSG_REPEAT,                   	MSG_NONE,                   MSG_NONE,                   MSG_NONE},
	{MSG_NONE,						MSG_RADIO_PLAY_SCAN,          	MSG_NONE,                   MSG_NONE,                   MSG_NONE},
	{MSG_NONE,						MSG_NUM_1,            			MSG_NONE,                   MSG_NONE,                   MSG_NONE},
	{MSG_NONE,						MSG_NUM_2,            			MSG_NONE,                   MSG_NONE,                   MSG_NONE},
	{MSG_NONE,						MSG_NUM_3,	        			MSG_NONE,                   MSG_NONE,                   MSG_NONE},
	{MSG_NONE,						MSG_NUM_4,	        			MSG_NONE,                   MSG_NONE,                   MSG_NONE},
	{MSG_NONE,						MSG_NUM_5,	        			MSG_NONE,                   MSG_NONE,                   MSG_NONE},
	{MSG_NONE,						MSG_NUM_6,	        			MSG_NONE,                   MSG_NONE,                   MSG_NONE},
	{MSG_NONE,						MSG_NUM_7,            			MSG_NONE,                   MSG_NONE,                   MSG_NONE},
	{MSG_NONE,						MSG_NUM_8,            			MSG_NONE,                   MSG_NONE,                   MSG_NONE},
	{MSG_NONE,						MSG_NUM_9,            			MSG_NONE,                   MSG_NONE,                   MSG_NONE},
	{MSG_NONE,						MSG_NUM_0,            			MSG_NONE,                   MSG_NONE,                   MSG_NONE},
};

static const struct
{
	void 		(*KeyScanInit)(void);
	KeyScanMsg (*AdcKeyScan)(void);
	uint16_t 	* KeyMsgTab;
}KeyScanMap[] =
{
#ifdef CFG_RES_ADC_KEY_SCAN
	{AdcKeyInit, 	AdcKeyScan, 	(uint16_t *)ADKEY_TAB},
#endif
#ifdef CFG_RES_IO_KEY_SCAN
	{IOKeyInit, 	IOKeyScan, 		(uint16_t *)IOKEY_TAB},
#endif
#ifdef CFG_RES_IR_KEY_SCAN
	{IRKeyInit, 	IRKeyScan, 		(uint16_t *)IRKEY_TAB},
#endif

#ifdef CFG_RES_CODE_KEY_USE
	{CodeKeyInit, 	CodeKeyScan, 	(uint16_t *)CODEKEY_TAB},
#endif

#ifdef CFG_ADC_LEVEL_KEY_EN
	{ADCLevelsKeyInit, AdcLevelKeyProcess, 		NULL},
#endif
};

#if (defined(CFG_RES_ADC_KEY_SCAN) ||defined(CFG_RES_IO_KEY_SCAN)|| defined(CFG_RES_IR_KEY_SCAN) || defined(CFG_RES_CODE_KEY_USE) ||  defined(CFG_ADC_LEVEL_KEY_EN))
#ifdef CFG_FUNC_DBCLICK_MSG_EN
/*************************************************
* 按键双击初始化处理函数，只支持单击释放的双击
* dbclick_msg 变量如果定义成数组，则可实现多个双击功能
*
***************************************************/
#define  CFG_PARA_CLICK_MSG             MSG_PLAY_PAUSE //单击执行消息
#define  CFG_PARA_DBCLICK_MSG           MSG_BT_HF_REDAIL_LAST_NUM   //双击执行消息
#define  CFG_PARA_DBCLICK_DLY_TIME      20

KEYBOARD_MSG 	dbclick_msg;
void DbclickInit(void)
{
	dbclick_msg.dbclick_en            = 1;
	dbclick_msg.dbclick_timeout       = 0;

	dbclick_msg.KeyMsg                = CFG_PARA_CLICK_MSG;////Single click msg
	dbclick_msg.dbclick_msg           = CFG_PARA_DBCLICK_MSG;//double  click msg
}

MessageId DbclickProcess(MessageId Msg)
{
	if(!dbclick_msg.dbclick_en)
		return Msg;

	if(dbclick_msg.KeyMsg == Msg)
	{
		if(dbclick_msg.dbclick_timeout == 0)
		{
			dbclick_msg.dbclick_timeout =  CFG_PARA_DBCLICK_DLY_TIME;///4ms*20=80ms
			APP_DBG("start double click_msg \n");
		}
		else
		{
			dbclick_msg.dbclick_timeout = 0;
			APP_DBG("double click_msg \n");
			return dbclick_msg.dbclick_msg;
		}
	}
	else
	{
		if(Msg != MSG_NONE)
		{
			dbclick_msg.dbclick_timeout = 0;
			return Msg;
		}
	}

	if(dbclick_msg.dbclick_timeout)
	{
		dbclick_msg.dbclick_timeout--;
		if(dbclick_msg.dbclick_timeout == 0)
		{
			dbclick_msg.dbclick_timeout = 0;
			APP_DBG("shot click_msg \n");
			return dbclick_msg.KeyMsg;
		}
	}

	return MSG_NONE;
}
#endif

MessageId GetGlobalKeyMessageId(KeyScanMsg 	msg)
{
	if(msg.index != KEY_MSG_INDEX_EMPTY && msg.type >= KEY_PRESSED)
	{
		switch(msg.source)
		{
		case ADC_KEY_SOURCE:
			return ADKEY_TAB[msg.index][msg.type - KEY_PRESSED];
			break;
		case IR_KEY_SOURCE:
			return IRKEY_TAB[msg.index][msg.type - KEY_PRESSED];
			break;
		case IO_KEY_SOURCE:
			break;
		case ADC_LEVEL_KEY_SOURCE:
			break;
		case CODE_KEY_SOURCE:
			break;
		default:
			break;
		}
	}
	return MSG_NONE;
}

void KeyInit(void)
{
	uint8_t i;

	for(i=0;i<sizeof(KeyScanMap)/sizeof(KeyScanMap[0]);i++)
		KeyScanMap[i].KeyScanInit();

#ifdef CFG_FUNC_DBCLICK_MSG_EN
	DbclickInit();
#endif
}

/**
 * @func        KeyScan
 * @brief       KeyScan,根据键值和事件类型查表，输出消息值
 * @param       None  
 * @Output      None
 * @return      MessageId
 * @Others      
 * Record
 * 1.Date        : 20180123
 *   Author      : pi.wang
 *   Modification: Created function
*/
MessageId KeyScan(void)
{
	MessageId 	KeyMsg = MSG_NONE;
	KeyScanMsg 	msg;

	uint8_t i;

	for(i=0;i<sizeof(KeyScanMap)/sizeof(KeyScanMap[0]);i++)
	{
		msg = KeyScanMap[i].AdcKeyScan();
		if(msg.index != KEY_MSG_INDEX_EMPTY && msg.type >= KEY_PRESSED)
		{
#ifdef CFG_FUNC_BEEP_EN
			BeepEnable();
#endif
			if(KeyScanMap[i].KeyMsgTab)
				KeyMsg = KeyScanMap[i].KeyMsgTab[msg.index * 5 + (msg.type - KEY_PRESSED)];
			else
				KeyMsg = msg.index;
			//#ifdef CFG_KEY_MSG_PRINT_EN
			//APP_DBG("KeyMsg(%d, %d) = %d %d\n",msg.source,KeyMsg,msg.index,msg.type);
			//#endif
			if(KeyMsg != MSG_NONE)
				break;
		}
	}

#ifdef CFG_IDLE_MODE_POWER_KEY
#if (POWERKEY_MODE == POWERKEY_MODE_SLIDE_SWITCH_LPD) || (POWERKEY_MODE == POWERKEY_MODE_SLIDE_SWITCH_HPD)
	{
		extern uint16_t GetSystemPowerKeyMsg(void);
		MessageId 	Msg = GetSystemPowerKeyMsg();

		if(Msg != MSG_NONE)
		{
			KeyMsg = Msg;
		}
	}
#endif
#endif

#ifdef CFG_FUNC_DBCLICK_MSG_EN
	KeyMsg = DbclickProcess(KeyMsg);
#endif
	return KeyMsg;
}

#endif


