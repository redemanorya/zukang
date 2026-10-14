#include <string.h>
#include "type.h"
#include "sys_param.h"
#include "spi_flash.h"
#include "flash_interface.h"
#include "flash_table.h"
#include "flash_param.h"
#include "rtos_api.h"
#include "debug.h"
#include "bt_config.h"
#include "app_config.h"


SYS_PARAMETER sys_parameter;

#if ((BT_SUPPORT == ENABLE) || (BLE_SUPPORT == ENABLE))



static const SYS_PARAMETER default_parameter = 
{
	.bt_LocalDeviceName 	= BT_NAME,
	.ble_LocalDeviceName	= BLE_NAME,
	.bt_TxPowerLevel		= BT_TX_POWER_LEVEL,
	.bt_PagePowerLevel		= BT_PAGE_TX_POWER_LEVEL,
	.BtTrim					= BT_DEFAULT_TRIM,
	.TwsVolSyncEnable		= TWS_VOL_SYNC_SUPPORT,	//主从之间音量控制同步
	.bt_CallinRingType		= SYS_DEFAULT_RING_TYPE,
	.bt_BackgroundType		= SYS_BT_BACKGROUND_TYPE,
	.bt_SimplePairingEnable	= BT_SIMPLEPAIRING_FLAG,
	.bt_PinCode				= BT_PINCODE,
	.bt_ReconnectionEnable			= BT_RECONNECTION_SUPPORT,
	.bt_ReconnectionTryCounts		= BT_POR_TRY_COUNTS,
	.bt_ReconnectionInternalTime	= BT_POR_INTERNAL_TIME,
	.bt_BBLostReconnectionEnable	= BT_BLR_RECONNECTION_SUPPORT,
	.bt_BBLostTryCounts				= BT_BLR_TRY_COUNTS,
	.bt_BBLostInternalTime			= BT_BLR_INTERNAL_TIME,
	.bt_TwsReconnectionEnable		= TWS_RECONNECTION_SUPPORT,
	.bt_TwsReconnectionTryCounts	= TWS_POR_TRY_COUNTS,
	.bt_TwsReconnectionInternalTime	= TWS_POR_INTERNAL_TIME,
	.bt_TwsBBLostReconnectionEnable	= TWS_BLR_RECONNECTION_SUPPORT,
	.bt_TwsBBLostTryCounts			= TWS_BLR_TRY_COUNTS,
	.bt_TwsBBLostInternalTime		= TWS_BLR_INTERNAL_TIME,
	.bt_TwsConnectedWhenActiveDisconSupport = TWS_CONNECTED_WHEN_ACTIVE_DISCONNECTION_SUPPORT,
	.bt_TwsPairingWhenPhoneConnectedSupport	= TWS_PAIRING_WHEN_PHONE_CONNECTED_SUPPORT,
	.ble_TxPowerLevel = BLE_TX_POWER_LEVEL,
};

//flash中参数读取
//从flash中读取参数成功，将参数 拷贝到dest_para
//读取不成功，拷贝默认参数default_para到dest_para
static const struct
{
	SYS_PARAMETER_ID 	id;				//参数ID
	void * 				dest_para;		//参数地址
	void * 				default_para;	//默认参数地址
	uint8_t				len;			//参数长度
}FlashParamReadMap[] =
{
	{BT_PARA_BT_NAME_ID,				(void *)sys_parameter.bt_LocalDeviceName,		(void *)default_parameter.bt_LocalDeviceName,		BT_NAME_SIZE},
	{BT_PARA_BLE_NAME_ID,				(void *)sys_parameter.ble_LocalDeviceName,		(void *)default_parameter.ble_LocalDeviceName,		BLE_NAME_SIZE},
	{BT_PARA_RF_TX_LEVEL_ID,			(void *)&sys_parameter.bt_TxPowerLevel,			(void *)&default_parameter.bt_TxPowerLevel,			1},
	{BT_PARA_RF_PAGE_LEVEL_ID,			(void *)&sys_parameter.bt_PagePowerLevel,		(void *)&default_parameter.bt_PagePowerLevel,		1},
	{BT_PARA_TRIM_VAL_ID,				(void *)&sys_parameter.BtTrim,					(void *)&default_parameter.BtTrim,					1},
	{BT_PARA_CallinRingType_ID,			(void *)&sys_parameter.bt_CallinRingType,		(void *)&default_parameter.bt_CallinRingType,		1},
	{BT_PARA_BackgroundType_ID,			(void *)&sys_parameter.bt_BackgroundType,		(void *)&default_parameter.bt_BackgroundType,		1},
	{BT_PARA_SimplePairingEnable_ID,	(void *)&sys_parameter.bt_SimplePairingEnable,	(void *)&default_parameter.bt_SimplePairingEnable,	1},
	{BT_PARA_PinCode_ID,				(void *)sys_parameter.bt_PinCode,				(void *)default_parameter.bt_PinCode,				BT_PIN_CODE_LEN},
	{BT_PARA_ReconnectionEnable_ID,		(void *)&sys_parameter.bt_ReconnectionEnable,	(void *)&default_parameter.bt_ReconnectionEnable,	6},

	{TWS_PARA_TWS_VOL_SYNC_ID,			(void *)&sys_parameter.TwsVolSyncEnable,		(void *)&default_parameter.TwsVolSyncEnable,		1},
	{TWS_PARA_ReconnectionEnable_ID,	(void *)&sys_parameter.bt_TwsReconnectionEnable,(void *)&default_parameter.bt_TwsReconnectionEnable,6},
	{TWS_PARA_TwsPairingWhenPhoneConnectedSupport_ID,(void *)&sys_parameter.bt_TwsPairingWhenPhoneConnectedSupport,(void *)&default_parameter.bt_TwsPairingWhenPhoneConnectedSupport,1},
	{TWS_PARA_TwsConnectedWhenActiveDisconSupport_ID,(void *)&sys_parameter.bt_TwsConnectedWhenActiveDisconSupport,(void *)&default_parameter.bt_TwsConnectedWhenActiveDisconSupport,1},
	
	{BLE_PARA_RF_TX_LEVEL_ID,			(void *)&sys_parameter.ble_TxPowerLevel,			(void *)&default_parameter.ble_TxPowerLevel,			1},
};

uint8_t flash_parameter_read(SYS_PARAMETER_ID id,uint8_t * buf)
{
	uint32_t addr = get_sys_parameter_addr();
	uint8_t * offset;
	uint8_t  len;
	SYS_PARAMETER_ID read_id;

	//参数区最大4K
	for(offset = (uint8_t *)addr; offset < (uint8_t *)(addr + 4096); offset += (2 + 1 + len))
	{
		read_id = offset[1];
		read_id <<= 8;
		read_id |= offset[0];
		len = offset[2];

		if(read_id == 0xffff && len == 0xff)
			break;
//		{
//			uint8_t i;
//			printf("ID: %x,len = %d\n",read_id,len);
//			for(i=0;i<len;i++)
//				printf("%02x ",offset[3+i]);
//			printf("\n");
//		}
		if(read_id == id)
		{
			memcpy(buf,offset+3,len);
			return len;
		}
	}

	return 0;
}

void sys_parameter_init(void)
{
	uint8_t * buf,len,i;

	memset(&sys_parameter,0,sizeof(sys_parameter));
	memcpy(&sys_parameter,&default_parameter,sizeof(SYS_PARAMETER));

	buf = osPortMalloc(256);
	if(flash_table_is_valid() && buf)
	{
		for(i=0;i<sizeof(FlashParamReadMap)/sizeof(FlashParamReadMap[0]);i++)
		{
			if((len = flash_parameter_read(FlashParamReadMap[i].id,buf)) > 0)
				memcpy(FlashParamReadMap[i].dest_para,buf,FlashParamReadMap[i].len);
			else
				memcpy(FlashParamReadMap[i].dest_para,FlashParamReadMap[i].default_para,FlashParamReadMap[i].len);
		}

		//读取参数版本
		if((len = flash_parameter_read(SYS_PARA_VER_ID,buf)) > 0)
		{
			DBG("flash_parameter_ver: %s\n",buf);
		}

		//检查值是否有效
		if(sys_parameter.BtTrim > 0x1f)
			sys_parameter.BtTrim = default_parameter.BtTrim;
		if(sys_parameter.bt_TxPowerLevel > 13)
			sys_parameter.bt_TxPowerLevel = default_parameter.bt_TxPowerLevel;
		if(sys_parameter.bt_PagePowerLevel > 13)
			sys_parameter.bt_PagePowerLevel = default_parameter.bt_PagePowerLevel;
	}

#ifdef BT_PROFILE_BQB_ENABLE
	sys_parameter.bt_ReconnectionTryCounts = 0;
	sys_parameter.bt_ReconnectionInternalTime = 1;
	sys_parameter.bt_BBLostTryCounts = 3;
#endif

	if(buf)
		osPortFree(buf);
}



#endif
void sys_parameter_update(SYS_PARAMETER_ID id,void *dest_para,uint8_t lenght)
{
	uint8_t * buf;

	if(flash_table_is_valid())
	{
		buf = osPortMalloc(4096);
		if(buf)
		{
			uint32_t  offset;
			SYS_PARAMETER_ID read_id;
			uint8_t  len;
			uint32_t addr = get_sys_parameter_addr();

			data_shell_read(addr - 32);
			//读取所有4K数据
			SPiFlashReadData(addr,buf, (4096 - 32));

			for(offset=0,len=0; offset< (4096 - 32); offset += (2 + 1 + len))
			{
				read_id = buf[offset + 1];
				read_id <<= 8;
				read_id |= buf[offset + 0];
				len = buf[offset + 2];

				if(read_id == 0xffff && len == 0xff)
					break;
				//找到对应ID，并且长度相等，更新数据
				if(read_id == id && lenght == len)
				{
					//更新对应位置的数据
					memcpy(buf + offset + 3,dest_para,lenght);

					SpiFlashEraseWpr(SECTOR_ERASE, ((addr-32)/4096), 1);
					SpiFlashWriteWpr(addr, buf, (4096 - 32), 1);

					DBG("sys_parameter_update: %x %d\n",id,lenght);
					break;
				}
			}
			data_shell_write(addr - 32);
			osPortFree(buf);
		}
	}
}
