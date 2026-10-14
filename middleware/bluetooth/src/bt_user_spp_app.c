/**
 *******************************************************************************
 * @file    bt_user_spp_app.c
 * @author  KK
 * @version V1.0.0
 * @date    2021-1-4
 * @brief   user SPP callback events and actions
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
#include "bt_config.h"

#include "bt_user_spp_api.h"
#include "bt_manager.h"

#if (BT_USER_SPP_SUPPORT == ENABLE)

BT_USER_SPP_STATE		userSppState;

const uint8_t UserSppServiceSdpUuid[] = {
    /* Data Element Sequence, 17 bytes */ 
    0x35, 0x11,

    /* UUID for private */ 
	0x1C,
	0x45,0x39,0x94,0xd5,0xd5,0x8b,0x96,0xf9,0x66,0x16,0xb3,0x7f,0x58,0x6b,0xa2,0xec,	//pandora
    
	/* Max number of bytes for attribute is 32 */ 
    0x00, 0x20,   

    0x35, 0x03,
    0x09, 0x00, 0x04
};


const uint8_t UserSppClassId[] = {
    /* Data Element Sequence, 17 bytes */ 
    0x35, 0x11,

    /* UUID for private */ 
	0x1C,
	0x45,0x39,0x94,0xd5,0xd5,0x8b,0x96,0xf9,0x66,0x16,0xb3,0x7f,0x58,0x6b,0xa2,0xec,	//pandora
};

void BtUserSppCallback(BT_USER_SPP_CALLBACK_EVENT event, BT_USER_SPP_CALLBACK_PARAMS * param)
{
	switch(event)
	{
		case BT_STACK_EVENT_USER_SPP_CONNECTED:
			{
				BT_DBG("User Spp Connected : bt address = %02x:%02x:%02x:%02x:%02x:%02x\n",
						(param->params.bd_addr)[0],
						(param->params.bd_addr)[1],
						(param->params.bd_addr)[2],
						(param->params.bd_addr)[3],
						(param->params.bd_addr)[4],
						(param->params.bd_addr)[5]);
			}
			break;

		case BT_STACK_EVENT_USER_SPP_DISCONNECTED:
			{
				BT_DBG("User Spp Disconnected\n");
			}
			break;

		case BT_STACK_EVENT_USER_SPP_DATA_RECEIVED:
			{
				uint16_t		i;

				BT_DBG("BT_STACK_EVENT_USER_SPP_DATA_RECEIVED:\n");
				for(i = 0; i < param->paramsLen; i++)
				{
					BT_DBG("0x%02x ", param->params.userSppReceivedData[i]);
				}
				BT_DBG("\n");
			}
			break;

		case BT_STACK_EVENT_USER_SPP_DATA_SENT:
		default:
			break;
	}
}

static void SetUserSppState(BT_USER_SPP_STATE state)
{
	userSppState = state;
}

BT_USER_SPP_STATE GetUserSppState(void)
{
	return userSppState;
}

bool UserSppAppInitLocal(BTUserSppCallbackFunc callback)
{
	UserSppAppInit(callback, UserSppClassId, sizeof(UserSppClassId), UserSppServiceSdpUuid, sizeof(UserSppServiceSdpUuid));
}

#endif

