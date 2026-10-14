/**
 *******************************************************************************
 * @file    bt_user_spp_api.h
 * @author  KK
 * @version V1.0.0
 * @date    2021-1-4
 * @brief   
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
* @defgroup 
* @{
*/


#ifndef __BT_USER_SPP_H__
#define __BT_USER_SPP_H__

typedef enum
{
	BT_USER_SPP_STATE_NONE,
	BT_USER_SPP_STATE_CONNECTING,
	BT_USER_SPP_STATE_CONNECTED,
} BT_USER_SPP_STATE;

typedef enum{
	BT_STACK_EVENT_USER_SPP_NONE = 0,

	BT_STACK_EVENT_USER_SPP_CONNECTED,

	BT_STACK_EVENT_USER_SPP_DISCONNECTED,

	BT_STACK_EVENT_USER_SPP_DATA_RECEIVED,

	BT_STACK_EVENT_USER_SPP_DATA_SENT,

}BT_USER_SPP_CALLBACK_EVENT;

typedef struct _BT_USER_SPP_CALLBACK_PARAMS
{
	uint16_t					paramsLen;
	bool						status;
	uint16_t					errorCode;

	union
	{
		uint8_t					*bd_addr;
		uint8_t		*userSppReceivedData;
		uint16_t	 userSppReceivedDataLen;
	}params;
} BT_USER_SPP_CALLBACK_PARAMS;

typedef void (*BTUserSppCallbackFunc)(BT_USER_SPP_CALLBACK_EVENT event, BT_USER_SPP_CALLBACK_PARAMS * param);

void BtUserSppCallback(BT_USER_SPP_CALLBACK_EVENT event, BT_USER_SPP_CALLBACK_PARAMS * param);

/**
 * @brief
 *	Connect a remote with Custom Profile channel
 *
 * @param
 *	addr - the address of remote device
 *
 * @return
 *	True for this command has been sent to bluetooth stack successful
 *
 * @note
 *	It will receive the event BT_STACK_EVENT_USER_SPP_CONNECTED when
 *	it connected to remote device.
 */

bool UserSppConnect(uint8_t* addr);


/**
 * @brief
 *	Disconnect the Custom Profile channel
 *
 * @param
 *	NONE
 *
 * @return
 *	True for this command has been sent to bluetooth stack successful
 *
 * @note
 *	It will receive the event BT_STACK_EVENT_USER_SPP_DISCONNECTED if the
 *	channel is disconnected.
 */

bool UserSppDisconnect(void);

/**
 * @brief
 *	Send data to the remote device via Custom Profile channel
 *
 * @param
 *	data -	the data to be sent. And the pointer of the data will
 *			be received in BT_STACK_EVENT_USER_SPP_DATA_SENT. So the
 *			memory of the data should be hold until the event
 *			BT_STACK_EVENT_USER_SPP_DATA_SENT received. So the variable
 *			data should be created by "malloc" method or  "global"
 *			variables
 *
 *	dataLen - the length of the data.
 *
 * @return
 *	True for this command has been sent to bluetooth stack successful.
 *
 * @note
 *	It will receive the event BT_STACK_EVENT_USER_SPP_DATA_SENT when the data
 *	has been sent to RF controller successful. If the data is created by
 *	"malloc" method, the "release" can be called in that event.
 */
bool UserSppSendData(uint8_t * data, uint16_t dataLen);


/**
 * @brief  send data via Custom Profile channel is ready?
 * @param  NONE
 * @return 1=ready, 0=not ready
 * @Note
 *		
 */
bool UserSppIsReady(void);


/**
 * @brief  custom profile initial (app)
 * @param  callback
 * @return 1=success
 * @Note
 *		
 */
bool UserSppAppInitLocal(BTUserSppCallbackFunc callback);

/**
 * @brief  custom profile initial (bt_lib)
 * @param  callback
 *         uuid
 * @return 1=success
 * @Note
 *		
 */
bool UserSppAppInit(BTUserSppCallbackFunc callback, uint8_t* classId, uint8_t classIdLen, uint8_t* sdpList, uint8_t sdpLen);

#endif /*__BT_USER_SPP_H__*/

/**
 * @}
 * @}
 */
 

