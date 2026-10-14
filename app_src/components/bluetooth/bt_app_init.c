/**
 **************************************************************************************
 * @file    bluetooth_init.c
 * @brief   
 *
 * @author  KK
 * @version V1.0.0
 *
 * $Created: 2021-4-18 18:00:00$
 *
 * @Copyright (C) Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include <stdlib.h>
#include "delay.h"
#include "debug.h"
#include "chip_info.h"
#include "efuse.h"
#include "remap.h"
#include "spi_flash.h"
#include "rtos_api.h"

#include "app_config.h"
#include "sys_em_config.h"
#include "bt_manager.h"
#include "bt_app_init.h"
#include "bt_app_common.h"
#include "bt_app_ddb_info.h"
#include "bb_api.h"
#include "bt_spp_api.h"
#include "bt_common_api.h"
#if BT_OBEX_SUPPORT
#include "bt_obex_api.h"
#endif
#if BT_PBAP_SUPPORT
#include "bt_pbap_api.h"
#endif
#if BT_MFI_SUPPORT
#include "bt_mfi_api.h"
#endif

#if BT_SOURCE_SUPPORT
#include "bt_hfg_api.h"
#endif

#if ((BT_SUPPORT == ENABLE) ||(BLE_SUPPORT == ENABLE))
extern BT_CONFIGURATION_PARAMS		*btStackConfigParams;
extern BT_MANAGER_ST		btManager;

static void Set_BtL2cap_Channel( void );
extern void Bt_Set_L2capChannel(uint8_t channel_num );
extern void Bt_SetDevNum(uint8_t dev_num);
extern void Init_Bt_Config_data(void);

#endif

#if (BT_SUPPORT == ENABLE)

//EM BT HOST CONFIG DATA
const BT_HOST_PARAM App_Bt_Host_config = 
{
	BT_DEVICE_NUMBER,
	BT_SCO_NUMBER
};

#if BT_SOURCE_SUPPORT
/***********************************************************************************
 * 
 **********************************************************************************/
uint32_t GetSourceSupportProfiles(void)
{
	uint32_t		profiles = 0;

#if (BT_A2DP_SUPPORT)
	profiles |= BT_PROFILE_SUPPORTED_A2DP;
#endif

#if (BT_AVRCP_SUPPORT)
	profiles |= BT_PROFILE_SUPPORTED_AVRCP;
#endif

#if BT_HFG_SUPPORT
	profiles |= BT_PROFILE_SUPPORTED_HFG;
#endif

	return profiles;
}

#endif








int8_t CheckBtName(uint8_t* nameString)
{
	if(((nameString[0]==0x00)&&(nameString[1]==0x00)&&(nameString[2]==0x00))
			|| ((nameString[0]==0xff)&&(nameString[1]==0xff)&&(nameString[2]==0xff)))
	{
		//蓝牙名称不符合预期
		DBG("bt name error\n");
		return -1;
	}
	else
	{
		return 0;
	}

}

int8_t CheckBtParamHeader(uint8_t* header)
{
	if((header[0]=='M')&&(header[1]=='V')&&(header[2]=='B')&&(header[3]=='T'))
	{
		//蓝牙参数header符合预期
		return 0;
	}
	else
	{
		DBG("header error\n");
		return -1;
	}

}

/***********************************************************************************
 * 
 **********************************************************************************/
static void prinfBtConfigParams(void)
{
	APP_DBG("**********\nLocal Device Infor:\n");

	//bluetooth name and address
	APP_DBG("Bt Name:%s\n", btStackConfigParams->bt_LocalDeviceName);
	APP_DBG("FlashBtAddre(NAP-UAP-LAP):");
	APP_DBG("%02x:%02x:%02x:%02x:%02x:%02x\n", 
			btStackConfigParams->bt_LocalDeviceAddr[0],
			btStackConfigParams->bt_LocalDeviceAddr[1],
			btStackConfigParams->bt_LocalDeviceAddr[2],
			btStackConfigParams->bt_LocalDeviceAddr[3],
			btStackConfigParams->bt_LocalDeviceAddr[4],
			btStackConfigParams->bt_LocalDeviceAddr[5]
			);
	
	APP_DBG("BtAddr(LAP-UAP-NAP):");
	APP_DBG("%02x:%02x:%02x:%02x:%02x:%02x\n", 
			btManager.btDevAddr[0],
			btManager.btDevAddr[1],
			btManager.btDevAddr[2],
			btManager.btDevAddr[3],
			btManager.btDevAddr[4],
			btManager.btDevAddr[5]
			);
	
	//ble address
	APP_DBG("Ble Name:%s\n", btStackConfigParams->ble_LocalDeviceName);
	APP_DBG("BleAddr:");
	APP_DBG("%02x:%02x:%02x:%02x:%02x:%02x\n", 
			btStackConfigParams->ble_LocalDeviceAddr[0],
			btStackConfigParams->ble_LocalDeviceAddr[1],
			btStackConfigParams->ble_LocalDeviceAddr[2],
			btStackConfigParams->ble_LocalDeviceAddr[3],
			btStackConfigParams->ble_LocalDeviceAddr[4],
			btStackConfigParams->ble_LocalDeviceAddr[5]
			);
	
	APP_DBG("Freq trim:0x%x\n", btStackConfigParams->bt_trimValue);
	APP_DBG("**********\n");
}

/***********************************************************************************
 * 配置HOST的参数
 **********************************************************************************/
static void ConfigBtStackParams(BtStackParams *stackParams)
{
	uint32_t pCod = 0;
	if(stackParams == NULL)
		return ;

	memset(stackParams, 0 ,sizeof(BtStackParams));

	/* Set support profiles */
	stackParams->supportProfiles = GetSupportProfiles();

#if BT_SOURCE_SUPPORT
	stackParams->sourcesupportProfiles = 0 ;
	if(gSwitchSourceAndSink == A2DP_SET_SOURCE)
	{
		stackParams->supportProfiles = 0;
		stackParams->sourcesupportProfiles = GetSourceSupportProfiles();
	}
#endif

	/* Set local device name */
	stackParams->localDevName = (uint8_t *)btStackConfigParams->bt_LocalDeviceName;

	/* Set callback function */
	stackParams->callback = BtStackCallback;
	stackParams->scoCallback = NULL;//GetScoDataFromApp;

	//simple pairing
	stackParams->btSimplePairing = btManager.bt_simplePairingFunc;
	if((btManager.bt_pinCodeLen)&&(btManager.bt_pinCodeLen<17))
	{
		stackParams->btPinCodeLen = btManager.bt_pinCodeLen;
		memcpy(stackParams->btPinCode, btManager.bt_pinCode, stackParams->btPinCodeLen);
	}
	else
	{
		APP_DBG("ERROR:pin code len %d\n", btManager.bt_pinCodeLen);
		stackParams->btSimplePairing = 1;//开启简易配对
	}

	//class of device
	//headset:苹果手机能显示设备的电池电量
	if(sys_parameter.bt_SimplePairingEnable)
	{
		pCod = COD_AUDIO | COD_MAJOR_AUDIO | COD_MINOR_AUDIO_HEADSET | COD_RENDERING;
	}
	else
	{
		//Note:1.苹果手机连BP10的HFP协议,如需要有同步通讯录的弹框,则需要把设备类型配置为handsfree,但不能显示电池电量
		//2.使用pin code的话,必须要将class of device配置为handsfree,否则某些手机不支持
		pCod = COD_AUDIO | COD_MAJOR_AUDIO | COD_MINOR_AUDIO_HANDSFREE | COD_RENDERING;
	}

	//假如不需要HFP,客户不想手机连接时弹出电话本的权限获取,则需要配置为 HIFI audio
	//pCod = COD_AUDIO | COD_MAJOR_AUDIO | COD_MINOR_AUDIO_HIFIAUDIO | COD_RENDERING;

#if (defined(FUNC_CIS_SUPPORT)&&(BLE_SUPPORT == ENABLE))
	pCod |= COD_LE_AUDIO;
#endif
	
#if (BT_HID_SUPPORT)
	pCod |= (COD_MAJOR_PERIPHERAL | COD_MINOR_PERIPH_KEYBOARD);
#endif

#if	BT_SOURCE_SUPPORT
	if(gSwitchSourceAndSink == A2DP_SET_SOURCE)//source
	{
		//pCod &= 0xfffffbff;
		pCod &= 0xfffffb00;
		pCod |= 0x200;
		if(BT_HFG_SUPPORT)
		{
			pCod |= COD_MINOR_AUDIO_HEADSET;
		}
		else
		{
			pCod |= COD_MINOR_AUDIO_HIFIAUDIO;
		}
	}
#endif

	SetBtClassOfDevice(pCod);

#if BT_HFP_SUPPORT
	/* HFP features */
	stackParams->hfpFeatures.wbsSupport = BT_HFP_SUPPORT_WBS;
#if (BT_HFP_SUPPORT_WBS)
	stackParams->hfpFeatures.hfpAudioDataFormat = HFP_AUDIO_DATA_mSBC;
#else
	stackParams->hfpFeatures.hfpAudioDataFormat = HFP_AUDIO_DATA_PCM;
#endif
	stackParams->hfpFeatures.hfpAppCallback = BtHfpCallback;
#else
	stackParams->hfpFeatures.hfpAppCallback = NULL;
#endif
	
#if (BT_A2DP_SUPPORT)
	/* A2DP features */
	//stackParams->a2dpFeatures.a2dpAudioDataFormat = BT_A2DP_AUDIO_DATA;
#if (defined(BT_AUDIO_AAC_ENABLE) && defined(USE_AAC_DECODER))
	stackParams->a2dpFeatures.a2dpAudioDataFormat = (A2DP_AUDIO_DATA_SBC | A2DP_AUDIO_DATA_AAC);
#else
	stackParams->a2dpFeatures.a2dpAudioDataFormat = (A2DP_AUDIO_DATA_SBC);
#endif
	stackParams->a2dpFeatures.a2dpAppCallback = BtA2dpCallback;
#else
	stackParams->a2dpFeatures.a2dpAppCallback = NULL;
#endif

#if (BT_AVRCP_SUPPORT)
	/* AVRCP features */
	stackParams->avrcpFeatures.supportAdvanced = 1;
#if ((BT_AVRCP_PLAYER_SETTING)||(BT_AVRCP_VOLUME_SYNC))
	stackParams->avrcpFeatures.supportTgSide = 1;
#else
	stackParams->avrcpFeatures.supportTgSide = 0;
#endif
	stackParams->avrcpFeatures.supportSongTrackInfo = BT_AVRCP_SONG_TRACK_INFOR;
	stackParams->avrcpFeatures.supportPlayStatusInfo = BT_AVRCP_SONG_PLAY_STATE;
	stackParams->avrcpFeatures.avrcpAppCallback = BtAvrcpCallback;
#else
	stackParams->avrcpFeatures.avrcpAppCallback = NULL;
#endif
	
	#ifdef BT_NUMERICAL_CONFIRMATION_ENABLE
	{
		extern void BtNumericalDisplayFuncEnable(uint8_t val);
		extern void BtNumericalVerify(uint32_t val);
		BtNumericalDisplayFuncEnable(1);
		SEC_Confirm_Callback_Set(BtNumericalVerify);
	}
	#endif

#if	((BT_SOURCE_SUPPORT) && (BT_A2DP_SUPPORT) && (BT_AVRCP_SUPPORT))
	stackParams->a2dpFeatures.a2dpSourceAppCallback = BtA2dpSourceCallback;

	if(gSwitchSourceAndSink == A2DP_SET_SOURCE)//source
	{
		//A2DP
		stackParams->a2dpFeatures.a2dp_sink_source_support = 0;
		//AVRCP
		stackParams->avrcpFeatures.source_or_sink = gSwitchSourceAndSink;//2:source avrcp  1:sink avrcp
		stackParams->avrcpFeatures.supportTgSide  = 1;
	}
	else if(gSwitchSourceAndSink == A2DP_SET_SINK)//sink
	{
		//A2DP
		stackParams->a2dpFeatures.a2dp_sink_source_support = 1;
		//AVRCP
		stackParams->avrcpFeatures.source_or_sink = gSwitchSourceAndSink;//2:source avrcp  1:sink avrcp
		stackParams->avrcpFeatures.supportTgSide  = 0;
	}
	
	APP_DBG("avrcp Features.supportTgSide [%d]\n", stackParams->avrcpFeatures.supportTgSide);
	
#ifdef CAR_AUTHENTICATED_LINK_KEY_FUNC	
	extern void BtNumericalDisplayFuncEnable(uint8_t val);
	extern void BtNumericalVerify(uint32_t val);
	BtNumericalDisplayFuncEnable(1);
	SEC_Confirm_Callback_Set(BtNumericalVerify);
	APP_DBG("Bt Numerical Display Func enable\n");
#else
	APP_DBG("Bt Numerical Display Func disable\n");
	extern void BtNumericalDisplayFuncEnable(uint8_t val);
	BtNumericalDisplayFuncEnable(0);
#endif
	
#endif

#if (BT_HFG_SUPPORT)
	stackParams->hfgFeatures.hfgAppCallback = BtHfgCallback;
#endif
}

/***********************************************************************************
 * 初始化蓝牙HOST
 **********************************************************************************/
bool BtStackInit(void)
{
//	bool ret;
	int32_t retInit=0;

	BtStackParams	stackParams;

#ifdef FUNC_BT_MULTI_LINK_SUPPORT
	extern void bt_role_switch_set(uint8_t flag);
	bt_role_switch_set(0);	//disable role switch for multi-link
#endif

	Bt_SetDevNum(BT_LINK_DEV_NUM);
	Set_BtL2cap_Channel();
	Init_Bt_Config_data();

	BtManageInit();

	//BTStatckSetPageTimeOutValue(BT_PAGE_TIMEOUT); 

	//BTHostLinkNumConfig(BT_DEVICE_NUMBER, BT_SCO_NUMBER);
//	BTHostParamsConfig(&App_Bt_Host_config);

	ConfigBtStackParams(&stackParams);

#if (BT_TWS_SUPPORT == ENABLE)
	stackParams.btDeviceFlag = ( BT_LINK_DEV_NUM | (BT_TWS_SUPPORT<<2) );
#else
	stackParams.btDeviceFlag = BT_LINK_DEV_NUM;
#endif

#if (BT_TWS_SUPPORT == ENABLE)
#ifdef TWS_FILTER_USER_DEFINED
	strcpy((void *)btManager.TwsFilterInfor, TWS_FILTER_INFOR);
#endif
#endif

#ifdef BT_PROFILE_BQB_ENABLE
	stackParams.BQBTestFlag = 1;
#else
	stackParams.BQBTestFlag = 0;
#endif

	BT2_DBG("Bt Stack Init start\n");
	retInit = BTStackRunInit(&stackParams);
	if(retInit != 0)
	{
		APP_DBG("Bt Stack Init ErrCode [%x]\n", (int)retInit);
		return FALSE;
	}

#if (BT_A2DP_SUPPORT)
	BT2_DBG("A2dpAppInit start\n");
	SetBtA2dpSbcBitpool(35);//53:high-quality SBC settings(bitpool = 53) 35:middle-quality SBC settings(bitpool = 35)
	retInit = A2dpAppInit(&stackParams.a2dpFeatures);
    if(retInit == 0)
	{
		APP_DBG("A2dp Init ErrCode [%x]\n", (int)retInit);
		return FALSE;
	}
#endif

#if (BT_AVRCP_SUPPORT)
	BT2_DBG("AvrcpAppInit start\n");
	retInit = AvrcpAppInit(&stackParams.avrcpFeatures);
	if(retInit == 0)
	{
		APP_DBG("Avrcp Init ErrCode [%x]\n", (int)retInit);
		return FALSE;
	}
#endif

#if (BT_SPP_SUPPORT)
	retInit = SppAppInit(BtSppCallback);
	if(retInit == 0)
	{
		APP_DBG("Spp Init Error!\n");
		return FALSE;
	}
#endif

#if (BT_HFP_SUPPORT)
	BT2_DBG("HfpAppInit start\n");
	retInit = HfpAppInit(&stackParams.hfpFeatures);
	if(retInit == 0)
	{
		APP_DBG("Hfp Init ErrCode [%x]\n", (int)retInit);
		return FALSE;
	}
#endif

#if (BT_OBEX_SUPPORT)
	retInit = ObexAppInit(BtObexCallback);
	if(retInit == 0)
	{
		APP_DBG("Obex Init Error!\n");
		return FALSE;
	}
	else
	{
		if(ObexAppMtuSizeSet(0x0f) == 0)//set obex mtu size //mtu-msb default:0x3f
		{
			APP_DBG("Obex mtu size set error!\n");
			return FALSE;
		}
	}
#endif
	
#if (BT_PBAP_SUPPORT)
	retInit = PbapAppInit(BtPbapCallback);
	if(retInit == 0)
	{
		APP_DBG("Pbap Init Error!\n");
		return FALSE;
	}
	else
	{
		if(PBAP_MtuSizeSet(PBAP_MTU_SIZE))
		{
			APP_DBG("pbap mtu size set error!\n");
			return FALSE;
		}
	}
#endif

#if (BT_HFG_SUPPORT)
#include "bt_hfg_api.h"
	retInit = HfgAppInit(&stackParams.hfgFeatures);
	if(retInit == 0)
	{
		APP_DBG("Hfg Init ErrCode [%x]\n", (int)retInit);
		return FALSE;
	}
#endif

#if (BT_HID_SUPPORT)
#include "bt_hid_api.h"
	UserHidDescriptorListRegister(); //注册自定义HID报告描述符等信息
	retInit = HidAppInit(BtHidCallback);
	if(retInit == 0)
	{
		APP_DBG("Hid Init Error!\n");
		return FALSE;
	}
#endif

#if (BT_MFI_SUPPORT)
	retInit = MFiAppInit(BtMfiCallback);
	if(retInit == 0)
	{
		APP_DBG("MFi Init Error!\n");
		return FALSE;
	}
#endif

#if (BT_USER_SPP_SUPPORT)
	#include "bt_user_spp_api.h"
	retInit = UserSppAppInitLocal(BtUserSppCallback);
	if(retInit == 0)
	{
		APP_DBG("User Spp Init Error!\n");
		return FALSE;
	}
#endif
		
#if (BT_TWS_SUPPORT == ENABLE)
	stackParams.twsFeatures.twsAppCallback = BtTwsCallback;
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
	stackParams->twsFeatures.twsSimplePairingCfg = DISABLE;
	//btManager.twsSimplePairingCfg = DISABLE;
#else
	stackParams.twsFeatures.twsSimplePairingCfg = CFG_TWS_PAIR_DOUBLEKEY;
	//btManager.twsSimplePairingCfg = ENABLE;
#endif
	stackParams.twsFeatures.twsRoleCfg = TWS_PAIRING_MODE;

//	AUDIOEFFECT_EFFECT_PARA *para = get_user_effect_parameters(mainAppCt.EffectMode);
	//para->user_effect_list->sample_rate;
	stackParams.twsFeatures.twsSample = TWS_SAMPLERATE;
	retInit = twsInit(&stackParams.twsFeatures);

#ifndef TWS_AUDIO_BUFFER_DYNAMIC_MALLOC
	extern void tws_delay_set(uint8_t cnt);
	tws_delay_set(TWS_DELAY_DEF);
#endif

	if(retInit == 0)
	{
		APP_DBG("Tws Init ErrCode [%x]\n", (int)retInit);
		return FALSE;
	}
#endif//defined(BT_TWS_SUPPORT)

#ifdef BT_GETTIME_SUPPORT
	retInit = BTStackInitMap();
	if(retInit == 0)
	{
		APP_DBG("Map Init Error!\n");
		return FALSE;
	}
#endif

	APP_DBG("Bt Stack  memory init end\n");
	return TRUE;
}

/***********************************************************************************
 * 反初始化蓝牙HOST
 **********************************************************************************/
bool BtStackUninit(void)
{
	int32_t ret=0;
	BTStackRunUninit();
	
	ret = BTStackMemFree();
	if(ret == -1)
	{
		return FALSE;
	}
	return TRUE;
}

/***********************************************************************************
 * 
 **********************************************************************************/
void BtStackRunNotify(void)
{
//	OSQueueMsgSend(MSG_NEED_BT_STACK_RUN, NULL, 0, MSGPRIO_LEVEL_HI, 0);
}


#endif
#if ((BT_SUPPORT == ENABLE) || (BLE_SUPPORT == ENABLE))
/***********************************************************************************
 *
 **********************************************************************************/
static bool GetBtDefaultAddr(uint8_t *devAddr)
{
//	extern char __sdk_code_start;
	if(devAddr == NULL)
		return FALSE;

	//LAP
	devAddr[5] = Efuse_ReadData(6);
	devAddr[4] = Efuse_ReadData(7);
	devAddr[3] = Efuse_ReadData(2);
	//UAP
	devAddr[2] = Efuse_ReadData(3);
	//NAP
	devAddr[1] = Efuse_ReadData(4);
	devAddr[0] = Efuse_ReadData(5);

	if((devAddr[0] == 0)&&(devAddr[1] == 0)&&(devAddr[2] == 0)
		&&(devAddr[3] == 0)&&(devAddr[4] == 0)&&(devAddr[5] == 0))
	{
		DBG("efuse is empty\n");
		//efuse为空,获取flash id
		{
			extern SPI_FLASH_ERR_CODE SpiFlashReadUniqueID(uint8_t* Buffer, uint8_t BufLen);
			uint8_t flashId[16];

			memset(flashId, 0, sizeof(flashId));
			if (SpiFlashReadUniqueID(flashId, sizeof(flashId)) == FLASH_NONE_ERR)
			{
				// DBG("Flash ID:");
				// for (int j=0; j<sizeof(flashId); j++)
				// {
				// 	DBG("%02X ", flashId[j]);
				// }
				// DBG("\r\n");

				//16Bytes -> 6Bytes 哈希（FNV-1a + CRC32）
				//增加蓝牙地址生成随机性
				{
					// FNV-1a 64 位哈希
					uint64_t fnv_hash = 0xcbf29ce484222325ULL;
					for (int i = 0; i < 16; i++) {
						fnv_hash ^= flashId[i];
						fnv_hash *= 0x100000001b3ULL;
					}
					// CRC32 哈希
					uint32_t crc_hash = 0xFFFFFFFF;
					for (int i = 0; i < 16; i++) {
						crc_hash ^= flashId[i];
						for (int j = 0; j < 8; j++) {
							if (crc_hash & 1)
								crc_hash = (crc_hash >> 1) ^ 0xEDB88320;
							else
								crc_hash >>= 1;
						}
					}
					// 组合：FNV 高 3 字节 + CRC32 低 3 字节
					devAddr[0] = (fnv_hash >> 40) & 0xFF;
					devAddr[1] = (fnv_hash >> 32) & 0xFF;
					devAddr[2] = (fnv_hash >> 24) & 0xFF;
					devAddr[3] = (crc_hash >> 16) & 0xFF;
					devAddr[4] = (crc_hash >> 8) & 0xFF;
					devAddr[5] = crc_hash & 0xFF;
				}
				// DBG("==Calculated BT Address:");
				// for (int j=0; j<6; j++)
				// {
				// 	DBG("%02X ", devAddr[j]);
				// }
				// DBG("\r\n");
			}
			else
			{
				DBG("read flash id error\n");
				//test mac address
				devAddr[5] = 0x1C;
				devAddr[4] = 0x2C;
				devAddr[3] = 0xE0;
				devAddr[2] = 0x66;
				devAddr[1] = 0x66;
				devAddr[0] = 0x66;
				return FALSE;
			}
		}

		//test mac address
		// devAddr[5] = 0x1C;
		// devAddr[4] = 0x2C;
		// devAddr[3] = 0xE0;
		// devAddr[2] = 0x66;
		// devAddr[1] = 0x66;
		// devAddr[0] = 0x66;
	}

	return TRUE;

}
/***********************************************************************************
 *
 **********************************************************************************/
int8_t CheckBtAddr(uint8_t* addr)
{
	if(((addr[0]==0x00)&&(addr[1]==0x00)&&(addr[2]==0x00)&&(addr[3]==0x00)&&(addr[4]==0x00)&&(addr[5]==0x00))
			|| ((addr[0]==0xff)&&(addr[1]==0xff)&&(addr[2]==0xff)&&(addr[3]==0xff)&&(addr[4]==0xff)&&(addr[5]==0xff)))
	{
		//蓝牙地址不符合预期
		DBG("bt addr error\n");
		return -1;
	}
	else
	{
		return 0;
	}
}
bool GetBleDefaultAddr(uint8_t *devAddr,uint8_t *BleAddr)
{
	if(devAddr == NULL || BleAddr == NULL)
		return FALSE;

	memcpy(BleAddr, devAddr,6);
	BleAddr[5] |= 0xc0;
	BleAddr[0] |= 0xc0;
	BleAddr[1] = ~BleAddr[1];
	BleAddr[4] = ~BleAddr[4];
	return TRUE;
}
/***********************************************************************************
 * 从flash中Load蓝牙配置参数
 **********************************************************************************/
void LoadBtConfigurationParams(void)
{
	int8_t ret = 0;
	uint8_t paramsUpdate = 0;
	if(btStackConfigParams == NULL)
		return;

	//load parameters
	ret = BtDdb_LoadBtConfigurationParams(btStackConfigParams);
	if(ret == -3)
	{
		//读取异常，read again
		ret = BtDdb_LoadBtConfigurationParams(btStackConfigParams);
		if(ret == -3)
		{
			APP_DBG("bt database read error!\n");
			return;
		}
	}

	//BT ADDR
	ret = CheckBtAddr(btStackConfigParams->bt_LocalDeviceAddr);
	if(ret != 0)
	{
		paramsUpdate = 1;
		GetBtDefaultAddr(btStackConfigParams->bt_LocalDeviceAddr);
	}

#if BT_SOURCE_SUPPORT
	if(gSwitchSourceAndSink == A2DP_SET_SOURCE)
	{
		// source 和 sink使用不同的MAC
		btStackConfigParams->bt_LocalDeviceAddr[5] = ~btStackConfigParams->bt_LocalDeviceAddr[5];
	}
#endif

	//将ADDR进行反序操作:btStackConfigParams->bt_LocalDeviceAddr(NAP-UAP-LAP)->btManager.btDevAddr(LAP-UAP-NAP)
	btManager.btDevAddr[5]=btStackConfigParams->bt_LocalDeviceAddr[0];
	btManager.btDevAddr[4]=btStackConfigParams->bt_LocalDeviceAddr[1];
	btManager.btDevAddr[3]=btStackConfigParams->bt_LocalDeviceAddr[2];
	btManager.btDevAddr[2]=btStackConfigParams->bt_LocalDeviceAddr[3];
	btManager.btDevAddr[1]=btStackConfigParams->bt_LocalDeviceAddr[4];
	btManager.btDevAddr[0]=btStackConfigParams->bt_LocalDeviceAddr[5];

	//BLE ADDR
	ret = CheckBtAddr(btStackConfigParams->ble_LocalDeviceAddr);
	if(ret != 0)
	{
		paramsUpdate = 1;
		GetBleDefaultAddr(btStackConfigParams->bt_LocalDeviceAddr,btStackConfigParams->ble_LocalDeviceAddr);
	}

#ifdef SYS_PARAM_BT_NAME
	//BT name 蓝牙名称从flash中获取
#if (BT_SUPPORT == ENABLE)
	ret = CheckBtName(btStackConfigParams->bt_LocalDeviceName);
	if(ret != 0)
	{
		strcpy((void *)btStackConfigParams->bt_LocalDeviceName, sys_parameter.bt_LocalDeviceName);
	}
#else
	strcpy((void *)btStackConfigParams->bt_LocalDeviceName, sys_parameter.bt_LocalDeviceName);
#endif
#else
	//BT name 蓝牙名称按照BT_NAME
	strcpy((void *)btStackConfigParams->bt_LocalDeviceName, BT_NAME);
#endif

#ifdef SYS_PARAM_BLE_NAME
	//BLE name 蓝牙名称从flash中获取
	ret = CheckBtName(btStackConfigParams->ble_LocalDeviceName);
	if(ret != 0)
	{
		strcpy((void *)btStackConfigParams->ble_LocalDeviceName, sys_parameter.ble_LocalDeviceName);
	}
#else
	//BLE name 蓝牙名称按照BLE_NAME
	strcpy((void *)btStackConfigParams->ble_LocalDeviceName, BLE_NAME);
#endif

	//BT PARAMS
#if(BT_SUPPORT == ENABLE)
	ret = CheckBtParamHeader(btStackConfigParams->bt_ConfigHeader);
	if(ret != 0)
	{
		APP_DBG("used default bt trim value\n");
		//在匹配header异常时,将蓝牙频偏值修改为默认参数
		//note:在使用默认参数时，trimValue一定不能为0xff，否则会导致bb工作不起来；

		btStackConfigParams->bt_trimValue = BT_DEFAULT_TRIM;
	}
#else
	btStackConfigParams->bt_trimValue = BT_DEFAULT_TRIM;
#endif

#if BT_SOURCE_SUPPORT
	btStackConfigParams->bt_source_SupportProfile =0;
	if(gSwitchSourceAndSink == A2DP_SET_SOURCE)
	{
		// btStackConfigParams->bt_SupportProfile = 0;
		// btStackConfigParams->bt_source_SupportProfile = GetSourceSupportProfiles();
	}
#endif

	//蓝牙公共配置参数,暂时按照宏定义默认的参数进行配置 bt_config.h,频偏值保留flash中数据
	//如后续有需要能动态调整,或者上位机工具修改的,就需要保存到flash中进行管理
	// btStackConfigParams->bt_TxPowerValue = sys_parameter.bt_TxPowerLevel;

	// btManager.bt_SupportProfile = GetSupportProfiles();

	//simple Pairing enable
	//当simplePairing=1时,pinCode无效;反之亦然
#if(BT_SUPPORT == ENABLE)
	btManager.bt_simplePairingFunc = sys_parameter.bt_SimplePairingEnable;
	btManager.bt_pinCodeLen = strlen(sys_parameter.bt_PinCode);
	strcpy((char *)btManager.bt_pinCode,sys_parameter.bt_PinCode);
#endif

	if(paramsUpdate)
	{
		APP_DBG("save bt params to flash\n");
		//保存默认的配置参数到flash，但不包括bt_ConfigHeader及bt_trimValue
		BtDdb_InitBtConfigurationParams(btStackConfigParams);
	}
#if (BT_SUPPORT == ENABLE)
    prinfBtConfigParams();
#endif
}
/***********************************************************************************
 * 配置BB的参数
 **********************************************************************************/
void ConfigBtBbParams(BtBbParams *params)
{
	if(params == NULL)
		return;

	memset(params, 0 ,sizeof(BtBbParams));

	params->localDevName = (uint8_t *)btStackConfigParams->bt_LocalDeviceName;
	memcpy(params->localDevAddr, btManager.btDevAddr, BT_ADDR_SIZE);

	if(btStackConfigParams->bt_trimValue > BT_MAX_TRIM)
	{
		APP_DBG("bt trim:%d, overflow!\n", btStackConfigParams->bt_trimValue);
		btStackConfigParams->bt_trimValue = BT_DEFAULT_TRIM;
	}
	params->freqTrim = btStackConfigParams->bt_trimValue;

	//em config
	params->em_start_addr = BB_EM_START_PARAMS;
	params->em_size = BB_EM_SIZE;

	//agc config
	params->pAgcDisable = 0; //0=auto agc;	1=close agc
	params->pAgcLevel = 1;

	//sniff config
	params->pSniffNego = 0;//1=open;  0=close
	params->pSniffDelay = 0;
	params->pSniffInterval = 0x320;//500ms
	params->pSniffAttempt = 0x01;
	params->pSniffTimeout = 0x01;

	//params->bbSniffNotify = NULL;

	SetRfTxPwrMaxLevel(sys_parameter.bt_TxPowerLevel, sys_parameter.bt_PagePowerLevel);
#if (BLE_SUPPORT)
	SetbleMaxtxpwr(sys_parameter.ble_TxPowerLevel);
#endif
	BtSetLinkSupervisionTimeout(BT_LSTO_DFT);
}


/***********************************************************************************
 * 
 **********************************************************************************/
static void Set_BtL2cap_Channel( void )
{
	uint8_t channel_num = 3;	//default =3 SDP CLIENT/SERVER + reserve

	#if  ( BT_A2DP_SUPPORT )
		channel_num += 3;		//AVDTP_STREAM /SIGNAL + AVCTP(avrcp)
	#endif


	#if (BT_HFP_SUPPORT || BT_SPP_SUPPORT)
		channel_num += 1;//hf and spp use rfcomm protocol which uses L2CAP
	#endif


	#if ( BT_HID_SUPPORT )
		channel_num += 1;
	#endif

	#if (BT_OBEX_SUPPORT)
		//obex can use either rfcomm or l2cap
	#endif

	Bt_Set_L2capChannel( channel_num * BT_LINK_DEV_NUM );

}

#endif

