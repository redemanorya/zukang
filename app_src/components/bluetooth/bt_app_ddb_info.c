
/**
 **************************************************************************************
 * @file    bluetooth_ddb_info.c
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

#include <string.h>
#include "app_config.h"
#include "type.h"
#include "spi_flash.h"
#include "flash_table.h"
#include "bt_app_ddb_info.h"
#include "debug.h"
#include "sys_param.h"
#include "rtos_api.h"
#include "app_message.h"
#include "flash_param.h"
#include "prf_types.h"
#include "bap_uc_cli.h"
#include "gapc_sec.h"
#include "flash_interface.h"

#ifdef CFG_FUNC_OPEN_SLOW_DEVICE_TASK
extern void SlowDevice_MsgSend(uint16_t msgId);
#endif


#ifdef CFG_APP_BT_MODE_EN

////////////////////////////////////////////////////////////////////
extern BT_CONFIGURATION_PARAMS		*btStackConfigParams;

////////////////////////////////////////////////////////////////////
static uint32_t FlshGetPairingInfoWriteOffset(uint32_t StartOffset, uint32_t Step);

#ifdef PRINT_RECORD_INFOR
/**********************************************************************************************/
/* 打印相应的信息 */
/**********************************************************************************************/
//static void PrintRecordInfor(const BT_DB_RECORD * record)
//{
//	uint8_t len=0;
//	BT_DBG("---\n");
//	BT_DBG("RemoteAddr:");
//	for(len=0; len<BT_ADDR_SIZE; len++)
//	{
//		BT_DBG("%02x ",record->bdAddr[len]);
//	}
//	BT_DBG("\nlinkkey:");
//	for(len=0; len<BT_LINK_KEY_LEN; len++)
//	{
//		BT_DBG("%02x ",record->linkKey[len]);
//	}
//	BT_DBG("\n---\n");
//}
/*****************************************************************************************
 * 
 ****************************************************************************************/
static void ShowOneDevMemoryPairedInfo(int RecId,unsigned char* Info, int Size)
{
    int i,t=0;
    BT_DBG("MemID = %d: ",RecId);
    for(i = 0 ; i < Size ; i ++)
    {
        t = Info[i];
        t &= 0xFF;
        BT_DBG("%02x ",t);
    }
    BT_DBG("\n");
}

/*****************************************************************************************
 * 
 ****************************************************************************************/
static void ShowOneDevSavedPairingInfo(int RecId,int AddrOffset, int Size)
{
    int i,t=0;
    uint8_t C;
    BT_DBG("RecID = %d: ",RecId);
    for(i = 0; i < Size ; i ++)
    {
    	SPiFlashReadData(AddrOffset + i, &C, 1);
        t = C; 
        t &= 0xFF;
        BT_DBG("%02x ",t);
    }
    BT_DBG("\n");
}
/*****************************************************************************************
 * 
 ****************************************************************************************/
static void ShowFlashLoadResult(int RecIdxFromZero, unsigned char *BtAddr,unsigned char *LinkKey,unsigned char *Property, 
								unsigned char *Role, unsigned char *Profile)
{
    int i,t=0;
    BT_DBG("FlhID = %d: ",RecIdxFromZero);
    for(i = 0 ; i < BT_ADDR_SIZE ; i ++)
    {
        t = BtAddr[i]; 
        t &= 0xFF;
        BT_DBG("%02x ",t);
    }
    
    for(i = 0 ; i < BT_LINK_KEY_LEN ; i ++)
    {
        t = LinkKey[i]; 
        t &= 0xFF;
        BT_DBG("%02x ",t);
    }
    
    t = Property[0]; 
        t &= 0xFF;
        BT_DBG("%02x ",t);
		
    t = Role[0]; 
        t &= 0xFF;
        BT_DBG("%02x\n",t);
		
    t = Profile[0]; 
        t &= 0xFF;
        BT_DBG("%02x\n",t);
}
#endif //show bt pairing info


/****************************************************************************
 * DDB list重新分配新设备的位置
 * 第一个信息是TWS信息 
 * list满了，则移除第二个设备信息
 ****************************************************************************/
static uint32_t DbdAllocateRecord(void)
{
    uint32_t count;
	
	//find the available item if any
	//从[1]开始保存device地址信息
	for(count = BT_DEVICE_LIST_START ; count < MAX_BT_DEVICE_NUM ; count ++)
	{
		if(btManager.btLinkDeviceInfo[count].UsedFlag == 0)
			return count;
	}

	//now it is full, I will remove the first record
    for(count = (BT_DEVICE_LIST_START+1) ; count < MAX_BT_DEVICE_NUM ; count ++)
    {
        memcpy(&(btManager.btLinkDeviceInfo[count-1]),&(btManager.btLinkDeviceInfo[count]),sizeof(BT_LINK_DEVICE_INFO));
    }

    btManager.btLinkDeviceInfo[MAX_BT_DEVICE_NUM - 1].UsedFlag = 0;

    return MAX_BT_DEVICE_NUM - 1;
}

/****************************************************************************
 *  获取到list中已记录的蓝牙个数
 ****************************************************************************/
uint32_t GetCurTotaBtRecNum(void)
{
    uint32_t count;
    for(count = BT_DEVICE_LIST_START ; count < MAX_BT_DEVICE_NUM ; count ++)
    {
        if(!(btManager.btLinkDeviceInfo[count].UsedFlag))
			break;
    }
    
    return (count);
}

/*****************************************************************************************
 * 
 ****************************************************************************************/
uint32_t DdbFindRecord(const uint8_t *bdAddr)
{
    uint32_t count;

	if(bdAddr == NULL)
		return DDB_NOT_FOUND;

	for (count = 0; count < MAX_BT_DEVICE_NUM; count++) 
	{
		if(btManager.btLinkDeviceInfo[count].UsedFlag == 0)continue;
		if (memcmp(bdAddr, btManager.btLinkDeviceInfo[count].device.bdAddr, BT_ADDR_SIZE) == 0)
		{
            return count;
        }
    }
    return DDB_NOT_FOUND;
}

/*****************************************************************************************
 * 
 ****************************************************************************************/
void DdbDeleteRecord(uint8_t index)
{
	uint32_t count = 0;

	if(index >= MAX_BT_DEVICE_NUM)
		return;

	if(index != (MAX_BT_DEVICE_NUM-1))
	{
		for(count = (index+1) ; count < MAX_BT_DEVICE_NUM ; count ++)
	    {
	        memcpy(&(btManager.btLinkDeviceInfo[count-1]),&(btManager.btLinkDeviceInfo[count]),sizeof(BT_LINK_DEVICE_INFO));
	    }

	    btManager.btLinkDeviceInfo[MAX_BT_DEVICE_NUM - 1].UsedFlag = 0;
	}
	else
	{
		memset(&btManager.btLinkDeviceInfo[MAX_BT_DEVICE_NUM-1], 0, sizeof(BT_LINK_DEVICE_INFO));
		btManager.btLinkDeviceInfo[MAX_BT_DEVICE_NUM-1].remote_profile = 0xff;
	}
}


/****************************************************************************
 *  从flash中加载保存的设备信息
 *  input:  OneFullRecBlockSize - 每块保存数据的大小
 *          RecIdxFromZero - 数据索引
 *          BtAddr - 读取的蓝牙地址信息
 *          Linkkey - 读取的蓝牙秘钥
 *          Property - 包含 trust, keytype, pinlen信息
 *  return: 0:失败;    1:成功
 ****************************************************************************/
static int FlshLoad1of8Dev(int OneFullRecBlockSize, int RecIdxFromZero, uint8_t* BtAddr, uint8_t* LinkKey, uint8_t* Property, 
							uint8_t* Role, uint8_t* Profile)
{
	uint8_t Tmp[MVBT_DB_FLAG_SIZE];
	uint32_t i;
	uint32_t StartOffset = BTDB_TOTAL_RECORD_ADDR;

    OneFullRecBlockSize += MVBT_DB_FLAG_SIZE;

	//get the data
	for(i = StartOffset; i <= StartOffset + BTDB_TOTAL_RECORD_MEM_SIZE - OneFullRecBlockSize; i += OneFullRecBlockSize)
	{
		SPiFlashReadData(i, Tmp, MVBT_DB_FLAG_SIZE);
		if((Tmp[0] == 'M') && (Tmp[1] == 'V') && (Tmp[2] == 'B') && (Tmp[3] == 'T'))
		{
			SPiFlashReadData(i + MVBT_DB_FLAG_SIZE, Tmp, 1); //rec count

			if(RecIdxFromZero >= (uint32_t)Tmp[0])
			{
				return 0;//not found
			}
			i = i + RecIdxFromZero * BT_REC_INFO_LEN + MVBT_DB_FLAG_SIZE + 1;
			//get addr
			SPiFlashReadData(i, BtAddr, BT_ADDR_SIZE);

			//key
			SPiFlashReadData(i + BT_ADDR_SIZE, LinkKey, BT_LINK_KEY_LEN);

			//property
			SPiFlashReadData(i + (BT_ADDR_SIZE + BT_LINK_KEY_LEN), Property, 1);

			//tws_role
			SPiFlashReadData(i + (BT_ADDR_SIZE + BT_LINK_KEY_LEN + 1), Role, 1);

			//profile
			SPiFlashReadData(i + (BT_ADDR_SIZE + BT_LINK_KEY_LEN + 2), Profile, 1);

#ifdef PRINT_RECORD_INFOR
            ShowFlashLoadResult(RecIdxFromZero, BtAddr,LinkKey,Property, Role, Profile);
#endif

			return 1;
		}
	}

	return 0;
}

/**********************************************************************************************
 * 加载flash中保存的蓝牙配对记录信息
 **********************************************************************************************/
uint8_t BtDdb_Open(const uint8_t * localBdAddr)
{
	uint32_t 		count;
	
	/* See if the device database file exists. If it does not exist then
	 * create a new one.
	 *
	 * For some reasons, I just clear the info.
	 */
	for(count = 0 ; count < MAX_BT_DEVICE_NUM ; count ++)
	{
		uint8_t Property;
		btManager.btLinkDeviceInfo[count].UsedFlag = FlshLoad1of8Dev(1 + BT_REC_INFO_LEN * MAX_BT_DEVICE_NUM/*one rec block size*/,
													 count,btManager.btLinkDeviceInfo[count].device.bdAddr,
														   btManager.btLinkDeviceInfo[count].device.linkKey,
														   &Property,
														   &btManager.btLinkDeviceInfo[count].tws_role,
														   &btManager.btLinkDeviceInfo[count].remote_profile
														   );

		if(btManager.btLinkDeviceInfo[count].UsedFlag)
		{
//			  Record->trusted = (RecNum & 0x80) ? 1 : 0; //bit 7
//			  Record->keyType = (RecNum>>4) & 0x07; 	 //bit 4.5.6
//			  Record->pinLen  = (RecNum & 0x0F) ;		 //bit 3,2,1,0			  
			btManager.btLinkDeviceInfo[count].device.trusted = (Property & 0x80) ? 1 : 0; //bit 7
			btManager.btLinkDeviceInfo[count].device.keyType = (Property>>4) & 0x07;	  //bit 4.5.6
			btManager.btLinkDeviceInfo[count].device.pinLen  = (Property & 0x0F) ;		  //bit 3,2,1,0 			 
		}
	}
	return 0;
}

#ifdef FLASH_SAVE_REMOTE_BT_NAME
void BtDdb_PrintRecord(void)
{
	uint32_t i;
	uint8_t k;
	uint8_t cnt;	//表示这张表存储了几个设备
	uint8_t Tmp[4];
	uint32_t Step = (4 + BT_REC_INFO_LEN * MAX_BT_DEVICE_NUM + 1);
	uint32_t StartOffset = BTDB_TOTAL_RECORD_ADDR;	// 保存的时候是从高到低，读出的时候从低到高
	BT_LINK_DEVICE_INFO info[8];

	BT_DBG("print the remote name: 0x%x\n", StartOffset);
	for(i = StartOffset; i < (BTDB_TOTAL_RECORD_ADDR + BTDB_TOTAL_RECORD_MEM_SIZE - 4); i += Step)
	{
		SPiFlashReadData(i, Tmp, 4);
		if((Tmp[0] == 'M') && (Tmp[1] == 'V') && (Tmp[2] == 'B') && (Tmp[3] == 'T'))
		{
			SPiFlashReadData(i + 4, &cnt, 1);
			BT_DBG("%d remote devices are saved\n", cnt);
			for(k = 0; k < cnt; k++)
			{
				SPiFlashReadData((i + 5 + k * BT_REC_INFO_LEN), info[k].device.bdAddr, BT_ADDR_SIZE);
				
#ifdef FLASH_SAVE_REMOTE_BT_NAME
#ifdef BT_TWS_SUPPORT
				SPiFlashReadData((i + 30 + k * BT_REC_INFO_LEN), info[k].device.bdName, BT_NAME_SIZE);
#else
				SPiFlashReadData((i + 28 + k * BT_REC_INFO_LEN), info[k].device.bdName, BT_NAME_SIZE);
#endif
#endif

#ifdef FLASH_SAVE_REMOTE_BT_NAME
				BT_DBG("%x:  Remote device [%d] name: %s  addr:%02X:%02X:%02X:%02X:%02X:%02X\n", i, k, info[k].device.bdName, 
					info[k].device.bdAddr[0],
					info[k].device.bdAddr[1],
					info[k].device.bdAddr[2],
					info[k].device.bdAddr[3],
					info[k].device.bdAddr[4],
					info[k].device.bdAddr[5]);
#else
				BT_DBG("%x:  Remote device [%d]  addr:%02X:%02X:%02X:%02X:%02X:%02X\n", i, k, 
					info[k].device.bdAddr[0],
					info[k].device.bdAddr[1],
					info[k].device.bdAddr[2],
					info[k].device.bdAddr[3],
					info[k].device.bdAddr[4],
					info[k].device.bdAddr[5]);
#endif
			}
			break;
		}
		else if((Tmp[0] != 0xFF) || (Tmp[1] != 0xFF) || (Tmp[2] != 0xFF) || (Tmp[3] != 0xFF)) //some error data found, then skip it
		{
			BT_DBG("some error data found, then skip it\n");
			continue;	// 不需要跳出循环，继续寻找下一个509B的数据
		}
	}

	return;
}
#endif

/**********************************************************************************************
 * 新增新的蓝牙配对信息
 * input: record
 **********************************************************************************************/
bool BtDdb_AddOneRecord(const BT_DB_RECORD * record)
{
	uint32_t count;

#ifdef FLASH_SAVE_REMOTE_BT_NAME
	uint8_t nameLen = (btManager.remoteNameLen <= BT_NAME_SIZE) ? btManager.remoteNameLen : BT_NAME_SIZE;
	memset(record->bdName, 0, BT_NAME_SIZE);
	memcpy(record->bdName, btManager.remoteName, nameLen);
#endif

	uint32_t totalCount;
	
	if(btManager.btDutModeEnable)
		return FALSE;

#if (BT_SOURCE_SUPPORT)
	if(gSwitchSourceAndSink == A2DP_SET_SOURCE)
	{
		if((memcmp(btManager.btLinkDeviceInfo[0].device.bdAddr, record->bdAddr, BT_ADDR_SIZE) != 0)
			||(memcmp(btManager.btLinkDeviceInfo[0].device.linkKey, record->linkKey, BT_LINK_KEY_LEN) != 0)
			)
		{
			memset(&(btManager.btLinkDeviceInfo[0].device), 0, sizeof(BT_DB_RECORD));
			memcpy(&(btManager.btLinkDeviceInfo[0].device),record,sizeof(BT_DB_RECORD));
			btManager.btLinkDeviceInfo[0].UsedFlag = 1;
			btManager.ddbUpdate = 1;
		}
	}
	else
#endif
	{
		count = DdbFindRecord((const uint8_t*)&(record->bdAddr));
		if (count == DDB_NOT_FOUND)
		{
			count = DbdAllocateRecord();

			btManager.btLinkDeviceInfo[count].UsedFlag = 1;
			memcpy(&(btManager.btLinkDeviceInfo[count].device),record,sizeof(BT_DB_RECORD));
			btManager.btLinkDeviceInfo[count].remote_profile = 0xff;

			btManager.ddbUpdate = 1;
		}
		else if(count == 0)
		{
			//tws 配对记录，不更新
			if(memcmp(btManager.btLinkDeviceInfo[count].device.linkKey, record->linkKey, BT_LINK_KEY_LEN) != 0)
			{
				memcpy(&(btManager.btLinkDeviceInfo[count].device),record,sizeof(BT_DB_RECORD));
				btManager.btLinkDeviceInfo[count].UsedFlag = 1;
				btManager.ddbUpdate = 1;
			}
		}
		else
		{
			totalCount = GetCurTotaBtRecNum();
			//确认更新的设备信息在列表中的序列
			if((count+1) != totalCount)
			{
				//从列表中先删除之前的记录
				DdbDeleteRecord(count);
				//将记录更新到列表最后

				memcpy(&(btManager.btLinkDeviceInfo[totalCount-1].device),record,sizeof(BT_DB_RECORD));
				btManager.btLinkDeviceInfo[totalCount-1].UsedFlag = 1;
				btManager.btLinkDeviceInfo[totalCount-1].remote_profile = 0xff;
				btManager.ddbUpdate = 1;
			}
			else
			{
				if(memcmp(btManager.btLinkDeviceInfo[count].device.linkKey, record->linkKey, BT_LINK_KEY_LEN) != 0)
				{
					memcpy(&(btManager.btLinkDeviceInfo[count].device),record,sizeof(BT_DB_RECORD));
					btManager.btLinkDeviceInfo[count].UsedFlag = 1;
					btManager.ddbUpdate = 1;
					btManager.btLinkDeviceInfo[count].remote_profile = 0xff;
					btManager.btDdbLastProfile = 0; //在同一个连接设备的link key发生变化时,同步需要清除连接过的profile参数
				}
			}
		}
	}
	return TRUE;
}

/**********************************************************************************************
 * 更新最后一次连接过的蓝牙设备地址
 * input:  BtLastAddr - 蓝牙地址
 * output: 1=success;  0=fail;
 **********************************************************************************************/
bool BtDdb_UpgradeLastBtAddr(uint8_t *BtLastAddr, uint8_t BtLastProfile)
{
	uint32_t count, totalCount;
	BT_DB_RECORD record;
	uint8_t tempprofile;

	if(btManager.btDutModeEnable)
		return 0;
	
	if(btManager.btLastAddrUpgradeIgnored)
		return 0;
		
	//BT_DBG("BtDdb_UpgradeLastBtAddr\n");
	count = DdbFindRecord((const uint8_t*)BtLastAddr);
	if (count == DDB_NOT_FOUND)
	{
		return 0;
	}
	else if(count == 0)
	{
		return 0;
	}
	else
	{
		//地址一致,不需要重复保存
		if(memcmp(btManager.btDdbLastAddr, BtLastAddr, BT_ADDR_SIZE) == 0)
		{
			if(btManager.btDdbLastProfile == BtLastProfile)
			{
				//断开的手机的情况下忽略再重新连接会导致linkkey不一样，需要在这里更新一下remote_profile的信息
				tempprofile = ~BtLastProfile;
				if(tempprofile != btManager.btLinkDeviceInfo[count].remote_profile)
				{
					BT_DBG(" -- cur remote profile is not match need updata\n");
					btManager.btLinkDeviceInfo[count].remote_profile = ~BtLastProfile;
				}
				return 0;
			}
		}
		btManager.btDdbLastProfile = BtLastProfile;
		memcpy(btManager.btDdbLastAddr, BtLastAddr, BT_ADDR_SIZE);

		totalCount = GetCurTotaBtRecNum();
		//确认更新的设备信息在列表中的序列
		if((count+1) != totalCount)
		{
			//保存记录
			memcpy(&record,&(btManager.btLinkDeviceInfo[count].device),sizeof(BT_DB_RECORD));
			
			//从列表中先删除之前的记录
			DdbDeleteRecord(count);
			
			//将记录更新到列表最后
			memcpy(&(btManager.btLinkDeviceInfo[totalCount-1].device),&record,sizeof(BT_DB_RECORD));
			btManager.btLinkDeviceInfo[totalCount-1].UsedFlag = 1;
			btManager.ddbUpdate = 1;
			btManager.btLinkDeviceInfo[totalCount-1].remote_profile = ~BtLastProfile;
		}
		else
		{
			btManager.btLinkDeviceInfo[totalCount-1].UsedFlag = 1;
			btManager.ddbUpdate = 1;
			btManager.btLinkDeviceInfo[totalCount-1].remote_profile = ~BtLastProfile;
		}
	}

	if(btManager.ddbUpdate)
	{
		btManager.ddbUpdate = 0;
		//save total device info to flash
#ifdef CFG_FUNC_OPEN_SLOW_DEVICE_TASK
		{
			extern void SlowDevice_MsgSend(uint16_t msgId);
			SlowDevice_MsgSend(MSG_DEVICE_BT_LINKED_INFOR_WRITE);
		}
#else
		SaveTotalDevRec2Flash(1 + BT_REC_INFO_LEN * MAX_BT_DEVICE_NUM/*one total rec block size*/,
				GetCurTotaBtRecNum());
#endif
	}

	return 0;
}

bool BtDdb_UpLastPorfile(uint8_t BtLastProfile)
{
	btManager.btDdbLastProfile = BtLastProfile;
#ifdef CFG_FUNC_OPEN_SLOW_DEVICE_TASK
	{
		extern void SlowDevice_MsgSend(uint16_t msgId);
		SlowDevice_MsgSend(MSG_DEVICE_BT_LINKED_PROFILE_UPDATE);
	}
#else
		BtDdb_UpgradeLastBtProfile(btManager.btDdbLastAddr, btManager.btDdbLastProfile);
#endif
	return 0;
}
/*****************************************************************************************
 * 
 ****************************************************************************************/
//SlowDeviceEventProcess
bool BtDdb_UpgradeLastBtProfile(uint8_t *BtLastAddr, uint8_t BtLastProfile)
{
	uint8_t Tmp[BT_REC_INFO_LEN];
	uint32_t Step = (MVBT_DB_FLAG_SIZE + 1 + BT_REC_INFO_LEN * MAX_BT_DEVICE_NUM);
	uint8_t TmpProfile = 0xff;
	uint32_t StartOffset = BTDB_TOTAL_RECORD_ADDR;
	uint32_t TotalNumber = 0;

	if(btManager.btDutModeEnable)
		return 0;
		
	if(btManager.btLastAddrUpgradeIgnored)
		return 0;
		
	StartOffset = FlshGetPairingInfoWriteOffset(StartOffset, Step);
	//printf("offset=0x%x\n", StartOffset);
	StartOffset += Step;
	StartOffset += 4;//flag
	StartOffset += 1;//total number

	TotalNumber = GetCurTotaBtRecNum();
	StartOffset += ((TotalNumber-1)*BT_REC_INFO_LEN);

	SPiFlashReadData(StartOffset, Tmp, BT_REC_INFO_LEN);

	if(memcmp(BtLastAddr, &Tmp[0], BT_ADDR_SIZE) == 0)
	{
		TmpProfile &= ~BtLastProfile;
		SpiFlashWriteWpr(StartOffset + BT_REC_INFO_LEN - 1, &TmpProfile, 1, 1);	// profile(1)
		//BT_DBG("BtDdb_UpgradeLastBtProfile: 0x%02x\n", TmpProfile);
		
		btManager.btLinkDeviceInfo[TotalNumber-1].remote_profile = ~BtLastProfile;
		return 1;
	}
	return 0;
}
#ifdef FLASH_SAVE_REMOTE_BT_NAME
/****************************************************************************
 *  获取蓝牙名之后再更新一下flash中的设备信息
 ****************************************************************************/
void BtDdb_UpdateDeviceName(uint8_t *bdAddr, uint8_t *bdName)
{
	uint32_t count;
	bool need_save = FALSE;
	
	for (count = 0; count < MAX_BT_DEVICE_NUM; count++) 
	{
		if(btManager.btLinkDeviceInfo[count].UsedFlag == 0)continue;
		if (memcmp(bdAddr, btManager.btLinkDeviceInfo[count].device.bdAddr, BT_ADDR_SIZE) == 0 && memcmp(&btManager.btLinkDeviceInfo[count].device.bdName[0],bdName,40)!= 0)
		{
            memcpy(&btManager.btLinkDeviceInfo[count].device.bdName[0],bdName,40);
			need_save = TRUE;
        }
    }
	if(need_save)
	{
		SaveTotalDevRec2Flash(1 + BT_REC_INFO_LEN * MAX_BT_DEVICE_NUM/*one total rec block size*/,
							 	GetCurTotaBtRecNum());
	}
}
#endif
/**********************************************************************************************
 * 加载最后一次连接过的蓝牙设备地址
 * input:  BtLastAddr - 蓝牙地址
 * output: 1=success;  0=fail;
 **********************************************************************************************/
bool BtDdb_GetLastBtAddr(uint8_t *BtLastAddr, uint8_t* profile)
{
#if (BT_SOURCE_SUPPORT)
	if(gSwitchSourceAndSink == A2DP_SET_SOURCE)
	{
		if(btManager.btLinkDeviceInfo[0].UsedFlag) //source paired infor
		{
			//从配对列表中获取index-0设备的地址
			memcpy(BtLastAddr,btManager.btLinkDeviceInfo[0].device.bdAddr,BT_ADDR_SIZE);
			*profile = ~btManager.btLinkDeviceInfo[0].remote_profile;
			return 1;
		}
		else
		{
			memset(BtLastAddr, 0, BT_ADDR_SIZE);
			return 0;
		}
	}
	else
#endif
	{
		uint32_t totalCount = GetCurTotaBtRecNum();

		if((totalCount>BT_DEVICE_LIST_START)&&(totalCount<=MAX_BT_DEVICE_NUM))
		{
			//从配对列表中获取最后1-2个设备的地址
			memcpy(BtLastAddr,btManager.btLinkDeviceInfo[totalCount-1].device.bdAddr,BT_ADDR_SIZE);
			*profile = ~btManager.btLinkDeviceInfo[totalCount-1].remote_profile;
			return 1;
		}
		else
		{
			memset(BtLastAddr, 0, BT_ADDR_SIZE);
			return 0;
		}
	}
}

/****************************************************************************************/
/* 清除所有的配对记录 */
/****************************************************************************************/
void BtDdb_EraseBtLinkInforMsg(void)
{
#ifdef CFG_FUNC_OPEN_SLOW_DEVICE_TASK
	SlowDevice_MsgSend(MSG_DEVICE_BT_LINKED_INFOR_ERASE);
#else
	BtDdb_Erase();
#endif
}

//SlowDeviceEventProcess
bool BtDdb_Erase(void)
{

	BT_DBG("flash erase: bt record\n");

	//配对设备信息
	SpiFlashEraseWpr(SECTOR_ERASE, BTDB_TOTAL_RECORD_ADDR /4096 , 1);
	SpiFlashEraseWpr(SECTOR_ERASE, (BTDB_TOTAL_RECORD_ADDR + 4*1024) /4096 , 1);

	memset(&btManager.btLinkDeviceInfo[0], 0, sizeof(btManager.btLinkDeviceInfo));
	return 1;
}

//此函数谨慎使用(此区域是受保护区域，不能随意擦除)
bool BtDdb_Erase_BtConfig(void)
{
	BT_DBG("flash erase: bt config\n");

	SpiFlashEraseWpr(SECTOR_ERASE, BTDB_CONFIGDATA_START_ADDR /4096 , 1);
	
	return 1;
}



/****************************************************************************************/
/* bt/ble 名称保存 */
/* name_type == 1 ble名称*/
/* name_type == 0 bt名称*/
/****************************************************************************************/
//SlowDeviceEventProcess
int32_t BtDeviceSaveNameToFlash(char* deviceName, uint8_t deviceLen,uint8_t name_type)
{
	uint32_t addr = get_sys_parameter_addr();

	if(addr > 0)
	{
		extern void sys_parameter_update(SYS_PARAMETER_ID id,void *dest_para,uint8_t lenght);

		if(name_type == 1)
		{
			if(sys_parameter.ble_LocalDeviceName != deviceName)
			{
				if(deviceLen > BLE_NAME_SIZE)
					deviceLen = BLE_NAME_SIZE;
				memset(sys_parameter.ble_LocalDeviceName,0,BLE_NAME_SIZE);
				memcpy(sys_parameter.ble_LocalDeviceName,deviceName,deviceLen);
			}
			sys_parameter_update(BT_PARA_BLE_NAME_ID,sys_parameter.ble_LocalDeviceName,BLE_NAME_SIZE);
		}
		else
		{
			if(sys_parameter.bt_LocalDeviceName != deviceName)
			{
				if(deviceLen > BT_NAME_SIZE)
					deviceLen = BT_NAME_SIZE;
				memset(sys_parameter.bt_LocalDeviceName,0,BT_NAME_SIZE);
				memcpy(sys_parameter.bt_LocalDeviceName,deviceName,deviceLen);
			}
			sys_parameter_update(BT_PARA_BT_NAME_ID,sys_parameter.bt_LocalDeviceName,BT_NAME_SIZE);
		}

		return 0;
	}

	return -1;
}

/*****************************************************************************************
 * 
 ****************************************************************************************/
int8_t BtDdb_SaveBtConfigurationParams(BT_CONFIGURATION_PARAMS *params)
{
	SPI_FLASH_ERR_CODE ret=0;
	uint8_t		*backupPtr = NULL;
	
	if(params == NULL)
	{
		BT_DBG("write error:params is null\n");
		return -1;
	}

	//1.alloc backup memory
	backupPtr = (uint8_t*)osPortMalloc(4096);
	if(backupPtr == NULL)
	{	
		return -1;//
	}
	memset(backupPtr, 0, 4096);
	//2.read all data
	SPiFlashReadData(BTDB_CONFIG_ADDR, backupPtr, 4096);

	//3.update params
	memcpy(backupPtr, params, sizeof(BT_CONFIGURATION_PARAMS));

	//4.erase area
	SpiFlashEraseWpr(SECTOR_ERASE, (BTDB_CONFIG_ADDR/4096), 1);

	//5.update params
	ret = SpiFlashWriteWpr(BTDB_CONFIG_ADDR, backupPtr, 4096, 1);
	if(ret != FLASH_NONE_ERR)
	{
		//release memory
		if(backupPtr)
		{
			osPortFree(backupPtr);
			backupPtr = NULL;
		}
		BT_DBG("write error:%d\n", ret);
		return -2;
	}

	BT_DBG("write success\n");
	//release memory
	if(backupPtr)
	{
		osPortFree(backupPtr);
		backupPtr = NULL;
	}
	return 0;
}



/****************************************************************************
 *  查询flash中保存的蓝牙数据的offset
 ****************************************************************************/
static uint32_t FlshGetPairingInfoWriteOffset(uint32_t StartOffset, uint32_t Step)
{
	uint32_t i;
	uint8_t Tmp[MVBT_DB_FLAG_SIZE];
	if(Step == 0)
	{
		BT_DBG("ERROR: Step cannot be zero\n");
		return StartOffset;
	}

	for(i = StartOffset; i <= StartOffset + BTDB_TOTAL_RECORD_MEM_SIZE - Step - 4/* 4 byte Magic Number */; i += Step)
	{
		SPiFlashReadData(i, Tmp, MVBT_DB_FLAG_SIZE);
		if((Tmp[0] == 'M') && (Tmp[1] == 'V') && (Tmp[2] == 'B') && (Tmp[3] == 'T'))
		{
			if(i != StartOffset)
			{
				return i - Step;
			}
			else
			{
				break; //"data is full, need to erase Flash
			}
		}
		else
		{
			if((Tmp[0] != 0xFF) || (Tmp[1] != 0xFF) || (Tmp[2] != 0xFF) || (Tmp[3] != 0xFF)) //some error data found, then skip it
			{
				if(i != StartOffset)
				{
					return i - Step;
				}
				else
				{
					break; //data is full
				}
			}
		}
	}
	
	if( i > StartOffset + BTDB_TOTAL_RECORD_MEM_SIZE - Step - 4) /* 4 means 4 byte Magic Number */
	{
		//empty DATA aera, for first usage
		return (StartOffset + BTDB_TOTAL_RECORD_MEM_SIZE - 4 - ((BTDB_TOTAL_RECORD_MEM_SIZE - 4) % Step) - Step);
	}
		

	//erase & write magic number
	SpiFlashEraseWpr(SECTOR_ERASE, (StartOffset) /4096 , 1);
	SpiFlashEraseWpr(SECTOR_ERASE, (StartOffset+4096) /4096 , 1);
	Tmp[0] = 'P';
	Tmp[1] = 'R';
	Tmp[2] = 'I';
	Tmp[3] = 'F';
	SpiFlashWriteWpr(StartOffset + BTDB_TOTAL_RECORD_MEM_SIZE - 4, Tmp, 4, 1);	//write magic number

	return (StartOffset + BTDB_TOTAL_RECORD_MEM_SIZE - 4 - ((BTDB_TOTAL_RECORD_MEM_SIZE - 4) % Step) - Step);

}


/****************************************************************************
 *  获取list中对应index的设备信息
 ****************************************************************************/
static uint32_t Get1of8RecInfo(uint8_t RecIdx/*from 0*/, uint8_t *Data/*size must be no less than 23B*/)
{
    if(RecIdx >= GetCurTotaBtRecNum())return 0; //not found
    
    memcpy(Data,btManager.btLinkDeviceInfo[RecIdx].device.bdAddr,BT_ADDR_SIZE);
    memcpy(Data+BT_ADDR_SIZE,btManager.btLinkDeviceInfo[RecIdx].device.linkKey,BT_LINK_KEY_LEN);
    Data[BT_ADDR_SIZE + BT_LINK_KEY_LEN] = (((btManager.btLinkDeviceInfo[RecIdx].device.trusted & 0x01) << 7) |
                ((btManager.btLinkDeviceInfo[RecIdx].device.keyType & 0x07) << 4) |
                ((btManager.btLinkDeviceInfo[RecIdx].device.pinLen  & 0x0F)));
#if (BT_TWS_SUPPORT == ENABLE)
    if(RecIdx == 0)
	{
		Data[23] = btManager.twsRole;
	}
	else
#endif
	{
		Data[23] = 0;
	}
	Data[24] = btManager.btLinkDeviceInfo[RecIdx].remote_profile;

#ifdef FLASH_SAVE_REMOTE_BT_NAME
	memcpy(Data + 25, btManager.btLinkDeviceInfo[RecIdx].device.bdName, BT_NAME_SIZE);
#endif
    return 1;
}

/****************************************************************************
 *  保存List中所有的蓝牙设备信息到flash
 ****************************************************************************/
void SaveTotalDevRec2Flash(int OneFullRecBlockSize, int TotalRecNum)
{
	uint8_t i, Tmp[5];
	uint32_t StartOffset = BTDB_TOTAL_RECORD_ADDR;
	uint8_t OneBtRec[BT_REC_INFO_LEN];

    OneFullRecBlockSize += MVBT_DB_FLAG_SIZE;//including 3Bytes of sync data

	StartOffset = FlshGetPairingInfoWriteOffset(StartOffset, OneFullRecBlockSize);
	//BT_DBG("New 0x%lx\n", StartOffset);
	Tmp[0] = 'M';
	Tmp[1] = 'V';
	Tmp[2] = 'B';
	Tmp[3] = 'T';
	//total num
	Tmp[4] = TotalRecNum & 0xFF;
	SpiFlashWriteWpr(StartOffset, Tmp, 5, 1);
	//BT_DBG("TOTAL NUM:%d\n", TotalRecNum);
	StartOffset += 5;
	
	for(i = 0; i < TotalRecNum; i++)
	{
		//get data
		if(Get1of8RecInfo(i, OneBtRec))
		{
#ifdef PRINT_RECORD_INFOR            
			ShowOneDevMemoryPairedInfo(i,OneBtRec,BT_REC_INFO_LEN);
#endif
			if(i == (TotalRecNum - 1))
			{
				//BT_DBG("Update the profile!!!\n");
				OneBtRec[24] = ~btManager.btDdbLastProfile;	//新接入的remote device更新profile
			}
			SpiFlashWriteWpr(StartOffset, OneBtRec, BT_REC_INFO_LEN, 1);
			StartOffset += BT_REC_INFO_LEN;
#ifdef PRINT_RECORD_INFOR            
			ShowOneDevSavedPairingInfo(i,StartOffset - BT_REC_INFO_LEN,BT_REC_INFO_LEN);
#endif
		}
	}
}

#else
void update_btDdb(uint8_t addr)
{
}


#endif



/*****************************************************************************************
 *
 ****************************************************************************************/
#if (BT_TWS_SUPPORT == ENABLE)
static void BtDdb_GetTwDeviceLinkKey(uint8_t *LinkKey)
{
	uint32_t Step = (MVBT_DB_FLAG_SIZE + 1 + BT_REC_INFO_LEN * MAX_BT_DEVICE_NUM);

	uint32_t StartOffset = BTDB_TOTAL_RECORD_ADDR;

	StartOffset = FlshGetPairingInfoWriteOffset(StartOffset, Step);
	StartOffset += Step;
	StartOffset += 4;//flag
	StartOffset += 1;//total number

	SPiFlashReadData(StartOffset + BT_ADDR_SIZE, LinkKey, BT_LINK_KEY_LEN);
}

/****************************************************************************
 *  更新TWS配对信息
 ****************************************************************************/
uint32_t BtDdb_UpgradeTwsInfor(uint8_t *BtTwsAddr)
{
	uint32_t count;//, totalCount
	BT_DB_RECORD record;
	uint8_t LinkKey[BT_LINK_KEY_LEN];

	count = DdbFindRecord((const uint8_t*)BtTwsAddr);
	if (count == DDB_NOT_FOUND)
	{
		return 0;
	}
	else if(count == 0)
	{
		if(!btManager.twsFlag)
		{
			BT_DBG("tws infor update\n");
			btManager.btLinkDeviceInfo[0].tws_role = btManager.twsRole;
			btManager.btLinkDeviceInfo[0].remote_profile = 0xff;
			btManager.ddbUpdate = 1;
		}
		else if(btManager.btLinkDeviceInfo[0].tws_role != btManager.twsRole)
		{
			BT_DBG("[Warm!]tws role changed: %d -> %d\n", btManager.btLinkDeviceInfo[0].tws_role, btManager.twsRole);
			btManager.btLinkDeviceInfo[0].tws_role = btManager.twsRole;
			btManager.btLinkDeviceInfo[0].remote_profile = 0xff;
			btManager.ddbUpdate = 1;
		}
		else
		{
			BtDdb_GetTwDeviceLinkKey(&LinkKey[0]);
			if(memcmp(&LinkKey[0], &(btManager.btLinkDeviceInfo[count].device.linkKey[0]), BT_LINK_KEY_LEN) != 0)
			{
				BT_DBG("[Warm!]must update the linkkey\n");
				btManager.ddbUpdate = 1;
			}
			else
			{
				return 0;
			}
		}
	}
	else
	{
		//totalCount = GetCurTotaBtRecNum();
		//确认更新的设备信息在列表中的序列
		//if((count+1) != totalCount)
		{
			//保存记录
			memcpy(&record,&(btManager.btLinkDeviceInfo[count].device),sizeof(BT_DB_RECORD));

			//从列表中先删除之前的记录
			DdbDeleteRecord(count);

			//将记录更新到列表最后
			memcpy(&(btManager.btLinkDeviceInfo[0].device),&record,sizeof(BT_DB_RECORD));
			btManager.btLinkDeviceInfo[0].UsedFlag = 1;
			btManager.btLinkDeviceInfo[0].tws_role = btManager.twsRole;
			btManager.btLinkDeviceInfo[0].remote_profile = 0xff;
			btManager.ddbUpdate = 1;
		}
		/*else
		{
			return 0;
		}*/
	}

	if(btManager.ddbUpdate)
	{
		btManager.ddbUpdate = 0;
		//save total device info to flash
#ifdef CFG_FUNC_OPEN_SLOW_DEVICE_TASK
		SlowDevice_MsgSend(MSG_DEVICE_BT_LINKED_INFOR_WRITE);
#else
		SaveTotalDevRec2Flash(1 + BT_REC_INFO_LEN * MAX_BT_DEVICE_NUM/*one total rec block size*/,
						GetCurTotaBtRecNum());
#endif

// #ifdef PRINT_RECORD_INFOR
// 		printf("tws infor update\n");
// 		PrintRecordInfor((const BT_DB_RECORD*)&(btManager.btLinkDeviceInfo[0].device));
// #endif
		btManager.twsFlag = 1;
		memcpy(btManager.btTwsDeviceAddr, &(btManager.btLinkDeviceInfo[0].device.bdAddr), BT_ADDR_SIZE);
	}
	return 0;
}

/*****************************************************************************************
 *
 ****************************************************************************************/
bool BtDdb_GetTwsDeviceAddr(uint8_t *BtTwsAddr)
{
	bool ret = 0;
	BT_DBG("--get tws info keyType :%d,pinLen:%d,remote_profile:%d,tws_role:%d\n",btManager.btLinkDeviceInfo[0].device.keyType,btManager.btLinkDeviceInfo[0].device.pinLen,
	btManager.btLinkDeviceInfo[0].remote_profile,btManager.btLinkDeviceInfo[0].tws_role);

	if((btManager.btLinkDeviceInfo[0].device.keyType)||(btManager.btLinkDeviceInfo[0].device.pinLen))
	{
		if((btManager.btLinkDeviceInfo[0].remote_profile == 0xff)&&(btManager.btLinkDeviceInfo[0].tws_role != 0xff))
		{
			ret = 1;
			memcpy(BtTwsAddr,btManager.btLinkDeviceInfo[0].device.bdAddr,BT_ADDR_SIZE);
			btManager.twsRole = btManager.btLinkDeviceInfo[0].tws_role;
		}
	}
	return ret;
}

/*****************************************************************************************
 *
 ****************************************************************************************/
//清除tws infor 相关寄存器的记录,不清除flash的内容
bool BtDdb_ClearTwsDeviceRecord(void)
{
	memset(&btManager.btLinkDeviceInfo[0].device, 0, sizeof(BT_DB_RECORD));
	btManager.btLinkDeviceInfo[0].UsedFlag = 0;
	btManager.btLinkDeviceInfo[0].tws_role = BT_TWS_UNKNOW;
	btManager.btLinkDeviceInfo[0].remote_profile = 0xff;

	btManager.twsRole = 0xff;
	btManager.twsFlag = 0;
	return TRUE;
}

/*****************************************************************************************
 *
 ****************************************************************************************/
//清除DDB中TWS的配对信息
void BtDdb_ClrTwsDevInfor(void)
{
	uint32_t StartOffset = BTDB_TOTAL_RECORD_ADDR;
	uint32_t Step = (MVBT_DB_FLAG_SIZE + 1 + BT_REC_INFO_LEN * MAX_BT_DEVICE_NUM);
	uint8_t tmp[BT_REC_INFO_LEN];

	BtDdb_ClearTwsDeviceRecord();

	StartOffset = FlshGetPairingInfoWriteOffset(StartOffset, Step);
	StartOffset += Step;
	StartOffset += 4;//flag
	StartOffset += 1;//total number

	memset(tmp, 0, BT_REC_INFO_LEN);
	SpiFlashWriteWpr(StartOffset, tmp, BT_REC_INFO_LEN, 1);
	BT_DBG("clear tws device infor\n");

	btManager.twsFlag = 0;
	btManager.twsRole = 0xff;
}

/*****************************************************************************************
 *
 ****************************************************************************************/
//清除tws infor flag(仅仅只是将最后1bytes置0)
bool BtDdb_ClrTwsDevInforFlag(void)
{
	uint8_t Tmp[BT_REC_INFO_LEN];
	uint32_t Step = (MVBT_DB_FLAG_SIZE + 1 + BT_REC_INFO_LEN * MAX_BT_DEVICE_NUM);
	uint8_t TmpProfile = 0xff;

	uint32_t StartOffset = BTDB_TOTAL_RECORD_ADDR;

	StartOffset = FlshGetPairingInfoWriteOffset(StartOffset, Step);
	StartOffset += Step;
	StartOffset += 4;//flag
	StartOffset += 1;//total number

	SPiFlashReadData(StartOffset, Tmp, BT_REC_INFO_LEN);

	TmpProfile = 0;
	SpiFlashWriteWpr(StartOffset + BT_REC_INFO_LEN - 1, &TmpProfile, 1, 1);
	BT_DBG("clear tws device infor flag\n");

	btManager.twsFlag = 0;
	btManager.twsRole = 0xff;

	return 1;
}

/*****************************************************************************************
 *
 ****************************************************************************************/
bool BtDdb_ClearTwsDeviceAddrList(void)
{
	memset(btManager.btTwsDeviceAddr, 0, BT_ADDR_SIZE);
	BtDdb_ClrTwsDevInforFlag();

	return 1;
}

#endif

#if defined(CFG_APP_BT_MODE_EN) || defined(CFG_APP_BLE_MODE_EN)
/************************************************************************************************************/
/* bt stack 初始化时加载flash中的蓝牙参数时不对flash中的 bt_ConfigHeader及bt_trimValue进行操作*/
/* 保留原始配置参数中的 bt_ConfigHeader 和 bt_trimValue */
/************************************************************************************************************/
int8_t BtDdb_InitBtConfigurationParams(BT_CONFIGURATION_PARAMS *params)
{
	SPI_FLASH_ERR_CODE ret=0;
	BT_CONFIGURATION_PARAMS		*btStackConfigParamsBack = NULL;
	uint8_t		*backupPtr = NULL;

	if(params == NULL)
	{
		BT_DBG("write error:params is null\n");
		return -1;
	}

	//1.alloc backup memory
	btStackConfigParamsBack = (BT_CONFIGURATION_PARAMS*)osPortMalloc(sizeof(BT_CONFIGURATION_PARAMS));
	if(btStackConfigParamsBack == NULL)
	{
		return -2;//
	}
	memset(btStackConfigParamsBack, 0, sizeof(BT_CONFIGURATION_PARAMS));

	backupPtr = (uint8_t*)osPortMalloc(4096);
	if(backupPtr == NULL)
	{
		if(btStackConfigParamsBack)
		{
			osPortFree(btStackConfigParamsBack);
			btStackConfigParamsBack = NULL;
		}
		return -1;//
	}
	memset(backupPtr, 0, 4096);

	//2.read bt stack config data back
	ret = SPiFlashReadData(BTDB_CONFIG_ADDR, (uint8_t*)btStackConfigParamsBack, sizeof(BT_CONFIGURATION_PARAMS));
	if(ret != FLASH_NONE_ERR)
	{
		ret = SPiFlashReadData(BTDB_CONFIG_ADDR, (uint8_t*)btStackConfigParamsBack, sizeof(BT_CONFIGURATION_PARAMS));
		if(ret != FLASH_NONE_ERR)
		{
			BT_DBG("bt data read error!\n");
			if(btStackConfigParamsBack)
			{
				osPortFree(btStackConfigParamsBack);
				btStackConfigParamsBack = NULL;
			}
			return -3;	//flash 读取错误
		}
	}
	memcpy(btStackConfigParamsBack->bt_LocalDeviceAddr, params->bt_LocalDeviceAddr, BT_ADDR_SIZE);
	memcpy(btStackConfigParamsBack->ble_LocalDeviceAddr, params->ble_LocalDeviceAddr, BT_ADDR_SIZE);

	//3.read all data from flash
	SPiFlashReadData(BTDB_CONFIG_ADDR, backupPtr, 4096);

	//4.merge: update params bt_LocalDeviceAddr and ble_LocalDeviceAddr
	memcpy(backupPtr, btStackConfigParamsBack, sizeof(BT_CONFIGURATION_PARAMS));

	//5.erase
	SpiFlashEraseWpr(SECTOR_ERASE, (BTDB_CONFIG_ADDR/4096), 1);

	//6.update params
	ret = SpiFlashWriteWpr(BTDB_CONFIG_ADDR, (uint8_t*)backupPtr, 4096, 1);
	if(ret != FLASH_NONE_ERR)
	{
		BT_DBG("write error:%d\n", ret);
		if(btStackConfigParamsBack)
		{
			osPortFree(btStackConfigParamsBack);
			btStackConfigParamsBack = NULL;
		}
		if(backupPtr)
		{
			osPortFree(backupPtr);
			backupPtr = NULL;
		}
		return -2;
	}

	if(btStackConfigParamsBack)
	{
		osPortFree(btStackConfigParamsBack);
		btStackConfigParamsBack = NULL;
	}
	if(backupPtr)
	{
		osPortFree(backupPtr);
		backupPtr = NULL;
	}

	BT_DBG("write success\n");
	return 0;
}
/****************************************************************************************/
/* 对蓝牙参数配置表数据进行读操作 */
/****************************************************************************************/
int8_t BtDdb_LoadBtConfigurationParams(BT_CONFIGURATION_PARAMS *params)
{
	SPI_FLASH_ERR_CODE ret=0;

	if(params == NULL)
	{
		BT_DBG("read error:params is null\n");
		return -1;
	}

	ret = SPiFlashReadData(BTDB_CONFIG_ADDR, (uint8_t*)params, sizeof(BT_CONFIGURATION_PARAMS));

	memcpy(params->bt_LocalDeviceName,sys_parameter.bt_LocalDeviceName,BT_NAME_SIZE);
	memcpy(params->ble_LocalDeviceName,sys_parameter.ble_LocalDeviceName,BLE_NAME_SIZE);

	if(ret != FLASH_NONE_ERR)
		return -3;	//flash 读取错误

	//数据读取后，外面进行处理
	return 0;

}

/****************************************************************************************/
/* 对flash最后4K数据按照蓝牙参数结构体进行读操作 */
/* 适用于只想读取蓝牙参数配置表的场景，避免了对flash中其他数据的影响 */
/****************************************************************************************/
int8_t BtDdb_LoadLast4KBtConfigParams(BT_CONFIGURATION_PARAMS *params)
{
	SPI_FLASH_ERR_CODE ret=0;

	if(params == NULL)
	{
		BT_DBG("read error:params is null\n");
		return -1;
	}

	ret = SPiFlashReadData(BTDB_CONFIG_ADDR, (uint8_t*)params, sizeof(BT_CONFIGURATION_PARAMS));
	APP_DBG("BtDdb_LoadLast4KBtConfigParams-addr:0x%lx: read ret=%d\n", BTDB_CONFIG_ADDR, ret);

	if(ret != FLASH_NONE_ERR)
		return -3;	//flash 读取错误

	//数据读取后，外面进行处理
	return 0;

}
#endif

