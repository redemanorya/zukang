/**
 *******************************************************************************
 * @file    bt_hfg_api.h
 * @author  KK
 * @version V1.0.0
 * @date    30-Dec-2021
 * @brief   HFP related api
 *******************************************************************************
 * @attention
 *
 * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
 * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
 * TIME. AS A RESULT, MVSILICON SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
 * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
 * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
 * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
 */

/**
* @addtogroup Bluetooth
* @{
* @defgroup Bluetooth Bluetooth
* @{
*/
	
#ifndef __BT_HFG_API_H__
#define __BT_HFG_API_H__


#include "type.h"


typedef enum{
	BT_STACK_EVENT_HFG_NONE = 0x00,
	BT_STACK_EVENT_HFG_CONNECTED,
	BT_STACK_EVENT_HFG_DISCONNECTED,
	BT_STACK_EVENT_HFG_FEATURE,
	BT_STACK_EVENT_HFG_SCO_CONNECTED,
	BT_STACK_EVENT_HFG_SCO_STREAM_START,
	BT_STACK_EVENT_HFG_SCO_STREAM_PAUSE,
	BT_STACK_EVENT_HFG_SCO_DISCONNECTED,
	BT_STACK_EVENT_HFG_SCO_DATA_RECEIVED,
	BT_STACK_EVENT_HFG_SCO_DATA_SENT,
	BT_STACK_EVENT_HFG_CALL_CONNECTED,
	BT_STACK_EVENT_HFG_CALL_DISCONNECTED,
	BT_STACK_EVENT_HFG_CALLSETUP_NONE,
	BT_STACK_EVENT_HFG_CALLSETUP_IN,//incoming call
	BT_STACK_EVENT_HFG_CALLSETUP_OUT,//outgoing
	BT_STACK_EVENT_HFG_CALLSETUP_ALERT,
	BT_STACK_EVENT_HFG_RING,
	BT_STACK_EVENT_HFG_CALLER_ID_NOTIFY,//incoming callnumber
	BT_STACK_EVENT_HFG_CURRENT_CALLS,//list current calls' response
	BT_STACK_EVENT_HFG_CALL_WAIT_NOTIFY,//wait incoming call number
	BT_STACK_EVENT_HFG_SPEAKER_VOLUME,
	BT_STACK_EVENT_HFG_BATTERY,
	BT_STACK_EVENT_HFG_SIGNAL,
	BT_STACK_EVENT_HFG_ROAM,
	BT_STACK_EVENT_HFG_VOICE_RECOGNITION,
	BT_STACK_EVENT_HFG_IN_BAND_RING,
	BT_STACK_EVENT_HFG_CODEC_TYPE,
	BT_STACK_EVENT_HFG_MANUFACTORY_INFO,
	BT_STACK_EVENT_HFG_DEVICE_TYPE,
	BT_STACK_EVENT_HFG_CALL_HOLD,
	BT_STACK_EVENT_HFG_ANSWER_CALL,
	BT_STACK_EVENT_HFG_HANG_UP,
	BT_STACK_EVENT_CALL_NUMBER,
	BT_STACK_EVENT_HFG_GENERATE_DTMF,
	BT_STACK_EVENT_HFG_UNSOLICITED_DATA
}BT_HFG_CALLBACK_EVENT;

typedef struct _BT_HFG_CALLBACK_PARAMS
{
	uint16_t			paramsLen;
	bool				status;
	uint16_t			errorCode;

	union
	{
		uint8_t			*bd_addr;
		uint32_t		features;
		uint8_t 		*scoReceivedData;
		uint8_t 		DtmfDate;
	}params;
}BT_HFG_CALLBACK_PARAMS;

typedef void (*BTHfgCallbackFunc)(BT_HFG_CALLBACK_EVENT event, BT_HFG_CALLBACK_PARAMS * param);

void BtHfgCallback(BT_HFG_CALLBACK_EVENT event, BT_HFG_CALLBACK_PARAMS * param);

typedef struct _HfgAppFeatures
{
	BTHfgCallbackFunc	hfgAppCallback;
}HfgAppFeatures;

/**
 * @brief  HFG connect command.
 * @param  addr The address of bt device to connect.
 * @return True for the command implement successful 
 * @Note If the device has been connected with handsfree profile, the event 
 *		BT_STACK_EVENT_HFG_CONNECTED will be received in the callback. Otherwise
 *		BT_STACK_EVENT_PAGE_TIMEOUT will be received.
 *
 */
bool HfgConnect(uint8_t * addr);


/**
 * @brief  HFG disconnect command.
 * @param  None
 * @return True for the command implement successful 
 * @Note If the device has been disconnected. the event BT_STACK_EVENT_HFG_DISCONNECTED
 *		will be received in the callback. 
 */
bool HfgDisconnect(void);

bool HfgAudioConnect(void);

bool HfgAudioDiconnect(void);

void Hfgincoming(void);

void Hfgmicset(uint8_t mic_gain); // set gian : 0-15

void HfgRing(void);

int hfgSetIndicator(uint8_t Ind,uint8_t Value);

void HfgSetCallState(uint8_t dir, uint8_t state);

void HfgSetCallIndex(uint8_t index);

void HfgCallInit(void);
void HfgSetCurrNumber(char *number);

/**
 * @brief Send hfg audio data
 * @param data The audio data to be sent
 * @param dataLen The length of audio data to be sent
 * @return TRUE means the data has been insert to send
 *			queue The event BT_STACK_EVENT_HFG_SCO_DATA_SENT
 *			will be received when the audio data has been
 *			sent successful
 * @Note
 *
 */
bool HfgSendScoData(uint8_t * data, uint16_t dataLen);

#endif /*__BT_HFG_API_H__*/

/**
 * @}
 * @}
 */

