/**
 **************************************************************************************
 * @file    bluetooth_ddb_info.h
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

#ifndef __BLUETOOTH_DDB_INFO_H__
#define __BLUETOOTH_DDB_INFO_H__

#include "bt_manager.h"
#include "prf_types.h"
#include "bap_uc_cli.h"
#include "gapc_sec.h"

#if (BLE_SUPPORT == ENABLE)
typedef struct ascs_bond_data
{
    //unicast client
    // Service description
    prf_svc_t svc_info;
    /// Number of discovered Sink ASE characteristics
    uint8_t nb_ases_sink;
    /// Number of discovered Source ASE characteristics
    uint8_t nb_ases_src;
    /// Characteristic information structures
    bap_uc_cli_ascs_char_t char_info[4];

    //unicast server
    // Client configuration bit field for Audio Stream Control Service
    uint8_t cli_cfg_bf;
    // Client configuration bit field for Audio Stream Control Service (Sink and Source ASE characteristics)
    uint16_t ase_cli_cfg_bf;
} ascs_bond_data_t;

// Bond Data structure
typedef struct le_cis_sink_bond_data
{
    // Client configuration bit field for Audio Stream Control Service
    uint8_t cli_cfg_bf;
    // Client configuration bit field for Audio Stream Control Service (Sink and Source ASE characteristics)
    uint16_t ase_cli_cfg_bf;
} app_sink_bond_data_t;

// Bond data structure used for test demonstration, content should be stored in a non volatile memory
typedef struct app_sink_bond_data
{
    // Service description
    prf_svc_t svc_info;
    // Pointer to Audio Stream Capabilities Service content
    bap_uc_cli_ascs_t* p_ascs;
} app_bond_data_t;

typedef struct ble_link_bond_data
{
    gapc_bond_data_updated_t bond_data;
    /// Long term key - for encryption on Low Energy connection
    gapc_ltk_t ltk;
    /// Identity resolving key information - for address resolution
    gapc_irk_t irk;

    gap_bdaddr_t addr;
    /// Pairing security level (see #gap_pairing_lvl enumeration)
    uint8_t pairing_lvl;   
    /// LTK or link key exchanged during pairing.
    bool enc_key_present;   

    bool is_paired;
    uint8_t hash[16];
    uint8_t ble_role; 
} ble_link_bond_data_t;
#endif
//蓝牙参数保存位置:size:4KB(0x1ff000~0x1fffff)
#define BTDB_CONFIGDATA_START_ADDR		(get_bt_config_addr())

//参数配置表:0x1ff000-0x1fffff(4K)
#define BTDB_CONFIG_ADDR				BTDB_CONFIGDATA_START_ADDR

#if ((BT_SUPPORT == ENABLE) || (BLE_SUPPORT))
#define DDB_NOT_FOUND  0xffff


#define BTDB_CONFIG_MEM_SIZE			(BTDB_CONFIGDATA_SIZE)

//蓝牙连接设备地址保存位置:size:12KB(0x1fb000~0x1fdfff)
#define BTDB_USERDATA_START_ADDR		(get_bt_data_addr())

//配对设备信息:0x1fb000-0x1fcfff(8K)
#define BTDB_TOTAL_RECORD_ADDR			(BTDB_USERDATA_START_ADDR)
#define BTDB_TOTAL_RECORD_MEM_SIZE		(BTDB_USERDATA_SIZE)

//最后1次配对设备信息:0x1fd000-0x1fdfff(4K) -- unused

//BT RECORD INFOR
//最后1次配对设备记录将从配对过的9个设备中寻找,找列表中的最后一个设备记录
#define MVBT_DB_FLAG						"MVBT"
#define MVBT_DB_FLAG_SIZE					4


#define MAX_BLE_DEVICE_NUM					2 
#define BLE_REC_INFO_LEN				    200//(6+16+16+1+1) = (addr+linkkey+flag+role+profile)

//保存的连接过的设备信息
#define MAX_BT_DEVICE_NUM					9 //(Source1+device8)
#ifdef FLASH_SAVE_REMOTE_BT_NAME
#define BT_REC_INFO_LEN						65//(6+16+1+1+1=25) + 40 = (addr+linkkey+flag+role+profile) + remotename
#else
#define BT_REC_INFO_LEN						25//(6+16+1+1+1) = (addr+linkkey+flag+role+profile)
#endif
#define BT_DEVICE_LIST_START				1//


/*extern uint8_t KeyEnc;*/
//#define PRINT_RECORD_INFOR

/**
 * @brief  open bt database
 * @param  localBdAddr - bt device address
 * @return offset
 * @Note 
 *
 */
uint8_t BtDdb_Open(const uint8_t * localBdAddr);

/**
 * @brief  close bt database
 * @param  NONE
 * @return TRUE - success
 * @Note 
 *
 */
bool BtDdb_Close(void);

/**
 * @brief  add one record to bt database
 * @param  record - the structure pointer(BT_DB_RECORD)
 * @return TRUE - success
 * @Note 
 *
 */
bool BtDdb_AddOneRecord(const BT_DB_RECORD * record);

/**
 * @brief  clear bt database area
 * @param  NONE
 * @return TRUE - success
 * @Note 
 *
 */
bool BtDdb_Erase(void);
void BtDdb_EraseBtLinkInforMsg(void);


int8_t BtDdb_LoadBtConfigurationParams(BT_CONFIGURATION_PARAMS *params);
int8_t BtDdb_LoadLast4KBtConfigParams(BT_CONFIGURATION_PARAMS *params);

int8_t BtDdb_SaveBtConfigurationParams(BT_CONFIGURATION_PARAMS *params);

int8_t BtDdb_InitBtConfigurationParams(BT_CONFIGURATION_PARAMS *params);

void DdbDeleteRecord(uint8_t index);

uint32_t DdbFindRecord(const uint8_t *bdAddr);

bool BtDdb_GetLastBtAddr(uint8_t *BtLastAddr, uint8_t* profile);

#ifdef FLASH_SAVE_REMOTE_BT_NAME
void BtDdb_UpdateDeviceName(uint8_t *bdAddr, uint8_t *bdName);
#endif

bool BtDdb_ClearTwsDeviceAddrList(void);

void BtDdb_ClrTwsDevInfor(void);

bool BtDdb_GetTwsDeviceAddr(uint8_t *BtTwsAddr);

uint32_t BtDdb_UpgradeTwsInfor(uint8_t *BtTwsAddr);

//清除tws infor 相关寄存器的记录,不清除flash的内容
bool BtDdb_ClearTwsDeviceRecord(void);

bool BtDdb_UpgradeLastBtAddr(uint8_t *BtLastAddr, uint8_t BtLastProfile);

bool BtDdb_UpgradeLastBtProfile(uint8_t *BtLastAddr, uint8_t BtLastProfile);

bool BtDdb_UpLastPorfile(uint8_t BtLastProfile);

void SaveTotalDevRec2Flash(int OneFullRecBlockSize, int TotalRecNum);

uint32_t GetCurTotaBtRecNum(void);

int32_t BtDeviceSaveNameToFlash(char* deviceName, uint8_t deviceLen,uint8_t name_type);

#endif

#endif //__BT_DDB_FLASH_H__


