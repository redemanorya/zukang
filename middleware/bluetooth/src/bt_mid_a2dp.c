/**
 *******************************************************************************
 * @file    bt_a2dp_app.h
 * @author  KK
 * @version V1.0.0
 * @date    9-9-2023
 * @brief   A2dp callback events and actions
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

#include "type.h"
#include "debug.h"
#include "bt_manager.h"
#include "bt_app_a2dp_deal.h"
#include "bt_interface.h"
#include "app_message.h"
#include "rtos_api.h"

#if (BT_SUPPORT == ENABLE)
//#include "le_audio_config.h"
/*****************************************************************
 * @brief  bt a2dp callback function
 * @param  event: BT_A2DP_CALLBACK_EVENT
 * @param  param: BT_A2DP_CALLBACK_PARAMS
 * @return 
 * @Note   a2dp协议回调事件/参数
 *****************************************************************/
void BtA2dpCallback(BT_A2DP_CALLBACK_EVENT event, BT_A2DP_CALLBACK_PARAMS * param)
{
#if (BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT)
	MessageContext msgSend;
#endif//(BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT)

	switch(event)
	{
		case BT_STACK_EVENT_A2DP_CONNECTED:

#if (BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT)
			// Note:上层应用在收到A2DP连接事件后,如果需要进入LE Audio广播模式,需要在此处发送消息通知模式切换,再进入回连流程
			//SendSysModeMsg(MSG_LE_BIS_SRC_START);
#endif	//(BT_LE_AUDIO_SUPPORT == LE_BIS_SUPPORT)

			BtA2dpConnectedDev(param);
			break;

		case BT_STACK_EVENT_A2DP_DISCONNECTED:
			BtA2dpDisconnectedDev(param);
			break;

		case BT_STACK_EVENT_A2DP_CONNECT_TIMEOUT:
			BtA2dpConnectTimeout(param);
			break;

		case BT_STACK_EVENT_A2DP_STREAM_START:
			BtA2dpStreamStart(param);
			break;

		case BT_STACK_EVENT_A2DP_STREAM_SUSPEND:
			BtA2dpStreamSuspend(param);
			break;

		case BT_STACK_EVENT_A2DP_STREAM_DATA_IND:
			BtA2dpStreamDataRecevied(param);
			break;

		case BT_STACK_EVENT_A2DP_STREAM_DATA_TYPE:
			BtA2dpStreamDataType(param);
			break;

		default:
			break;
	}
}

/*****************************************************************
 * @brief  a2dp link state set
 * @param  index: 0 or 1
 * @param  state: BT_A2DP_STATE
 * @return 
 * @Note   a2dp协议连接状态设置
 *****************************************************************/
void SetA2dpState(uint8_t index, BT_A2DP_STATE state)
{
	if (index >= BT_LINK_DEV_NUM)
		return;
	btManager.btLinked_env[index].a2dpState = state;
}

/*****************************************************************
 * @brief  a2dp link state get
 * @param  index: 0 or 1
 * @return state: BT_A2DP_STATE
 * @Note   a2dp协议连接状态获取
 *****************************************************************/
BT_A2DP_STATE GetA2dpState(uint8_t index)
{
	if (index >= BT_LINK_DEV_NUM)
		return BT_A2DP_STATE_NONE;
	return btManager.btLinked_env[index].a2dpState;
}

/*****************************************************************
 * @brief  a2dp connect
 * @param  index: 0 or 1
 * @param  addr: remote address
 * @Note   a2dp协议连接接口
 *****************************************************************/
void BtA2dpConnect(uint8_t index, uint8_t *addr)
{
#if (BT_LINK_DEV_NUM == 2)
	uint8_t i;
	for(i=0;i<BT_LINK_DEV_NUM;i++)
	{
		if((btManager.btLinked_env[i].a2dp_index == index)
			&&(btManager.btLinked_env[i].a2dpState > BT_A2DP_STATE_NONE))
		{
			APP_DBG("BtA2dpConnect:a2dpState index[%d] is %d\n",i,btManager.btLinked_env[i].a2dpState);
			return;
		}
	}
#endif
	if (addr == NULL)
		return;
	APP_DBG("A2dpConnect index = %d,addr:%x:%x:%x:%x:%x:%x\n", index,addr[0],addr[1],addr[2],addr[3],addr[4],addr[5]);
	A2dpConnect(index ,addr);
}

#endif
