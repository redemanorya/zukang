/**
 *******************************************************************************
 * @file    bt_hid_app.h
 * @author  KK
 * @version V1.0.0
 * @date    27-Nov-2025
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
 * <h2><center>&copy; COPYRIGHT 2025 MVSilicon </center></h2>
 */

#include "type.h"
#include "debug.h"
#include "bt_config.h"
#include "bt_manager.h"
#include "bt_hid_api.h"

#if ((BT_HID_SUPPORT)&&(BT_SUPPORT == ENABLE))
#include "rtos_api.h"

// bool hid_connect = FALSE;

/***********************************************************************************
 * BtHidCallback
 **********************************************************************************/
void BtHidCallback(BT_HID_CALLBACK_EVENT event, BT_HID_CALLBACK_PARAMS * param)
{
	switch(event)
	{
		case BT_STACK_EVENT_HID_CONNECTED:
			APP_DBG("BT_STACK_EVENT_HID_CONNECTED\n");
			btManager.hidState = BT_HID_STATE_CONNECTED;
			break;
			
		case BT_STACK_EVENT_HID_DISCONNECTED:
			APP_DBG("BT_STACK_EVENT_HID_DISCONNECTED\n");
			btManager.hidState = BT_HID_STATE_NONE;
			break;
			
		case BT_STACK_EVENT_HID_DATA_RECEIVED:
			APP_DBG("BT_STACK_EVENT_HID_DATA_RECEIVED\n");
			{
				uint32_t i;
				APP_DBG("data len: %d:", param->params.hidData.dataLen);
				for(i=0;i<param->params.hidData.dataLen;i++)
				{
					APP_DBG("0x%x ", param->params.hidData.data[i]);
				}
				APP_DBG("\n");
			}
			break;
			
		case BT_STACK_EVENT_HID_DATA_SENT:
			break;
			
	}
}

BT_HID_STATE GetHidState(void)
{
	return btManager.hidState;
}

/***********************************************************************************
 * hid command
 **********************************************************************************/
#define HID_CMD_RELEASE 0x00
#define HID_VOL_UP      BIT(0) 
#define HID_VOL_DN      BIT(1)
#define HID_NEXT        BIT(2) 
#define HID_PREV        BIT(3) 
#define HID_MUTE        BIT(4)
#define HID_STOP        BIT(7) 

#define HID_Camera      BIT(1) //用于自拍

void BtHidUserKey(uint8_t cmd)
{
	HidData sHidData;
	
	sHidData.channel = HidIntrChannel;
	btManager.hidUserCmd[0] = 0xa1;
	btManager.hidUserCmd[1] = cmd;
	sHidData.data = &btManager.hidUserCmd[0];
	sHidData.dataLen = 2;

	HidSendData(&sHidData);
}

void HidCmd_VolUp(void)
{
	BtHidUserKey(HID_VOL_UP);
	osTaskDelay(100);
	BtHidUserKey(HID_CMD_RELEASE);
}

void HidCmd_VolDn(void)
{
	BtHidUserKey(HID_VOL_DN);
	osTaskDelay(100);
	BtHidUserKey(HID_CMD_RELEASE);
}

void HidCmd_Next(void)
{
	BtHidUserKey(HID_NEXT);
	osTaskDelay(500);
	BtHidUserKey(HID_CMD_RELEASE);
}

void HidCmd_Prev(void)
{
	BtHidUserKey(HID_PREV);
	osTaskDelay(500);
	BtHidUserKey(HID_CMD_RELEASE);
}

void HidCmd_Mute(void)
{
	BtHidUserKey(HID_MUTE);
	osTaskDelay(500);
	BtHidUserKey(HID_CMD_RELEASE);
}

void HidCmd_Stop(void)
{
	BtHidUserKey(HID_STOP);
	osTaskDelay(500);
	BtHidUserKey(HID_CMD_RELEASE);
}

//自拍
void HidCmd_Camera(void)
{
	BtHidUserKey(HID_Camera);
	osTaskDelay(500);
	BtHidUserKey(HID_CMD_RELEASE);
}

/*
 * Note: 自定义HID描述符、自定义HID服务名称、自定义HID服务描述、自定义HID服务提供者名称
 */
//用户自定义HID报告描述符
static const U8 UserHidDescriptorList[] = {
	0x35, 39, 	//Data Element Sequence, 39 bytes
	0x35, 37, 	//Data Element Sequence, 37 bytes
	0x08, 22, 	//HID Descriptor List
	0x25, 33, 	//report descriptor

	//HID报告描述符
	0x05, 0x0C, //Usage Page (Consumer)
	0x09, 0x01, //Usage (Consumer Control)
	0xA1, 0x01, //Collection (Application)
	0x15, 0x00, //Logical minimum (0)
	0x25, 0x01, //Logical maximum (1)
	0x09, 0xE9,	//VOL+
	0x09, 0xEA, //VOL-
	0x09, 0xB5, //Scan Next Track
	0x09, 0xB6, //Scan Previous Track
	0x09, 0xE2, //Mute
	0x09, 0xB3, //Fast Forward OOC 15.7
	0x09, 0xCD, //Play/Pause
	0x09, 0xB7, //Stop
	0x75, 0x01, //Report Size (1)
	0x95, 0x08, //Report Count (8)
	0x81, 0x42,
	0xC0
};

static const U8 UserHidServiceName[] = {
	0x25,9,				/* service name length */ 
	'K', 'e', 'y','b', 'o','a', 'r','d','\0'
};

static const U8 UserHidServiceDescription[] = {
	0x25,12,			/* service description length */ 
	'B', 'l', 'u', 'e','t','o','o','t','h','K','B', '\0'
};

static const U8 UserHidProviderName[] = {
	0x25,5,		   /* provider name length */ 
	'M', 'V', 'B', 'T', '\0'
};


void UserHidDescriptorListRegister(void)
{
	//注册自定义HID报告描述符
	// BTSetUserDefinedHidDescriptorList((uint8_t *)UserHidDescriptorList, sizeof(UserHidDescriptorList));
	//注册自定义HID服务名称
	// BTSetUserDefinedHidServiceName((uint8_t *)UserHidServiceName, sizeof(UserHidServiceName));
	//注册自定义HID服务描述
	// BTSetUserDefinedHidServiceDescription((uint8_t *)UserHidServiceDescription, sizeof(UserHidServiceDescription));
	//注册自定义HID提供者名称
	//BTSetUserDefinedHidProviderName((uint8_t *)UserHidProviderName, sizeof(UserHidProviderName));
}

#endif

