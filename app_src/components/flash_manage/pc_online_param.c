#include <string.h>
#include "type.h"
#include "sys_param.h"
#include "spi_flash.h"
#include "flash_table.h"
#include "rtos_api.h"
#include "debug.h"
#include "bt_config.h"
//#include "app_config.h"
#include "bt_app_ddb_info.h"
#include "pc_online_param.h"
#include "flash_param.h"
#include "otg_device_hcd.h"
//application
#include "otg_device_audio.h"
#include "otg_device.h"

#ifdef CFG_FUNC_FLASH_PARAM_ONLINE_TUNING_EN
extern void sys_parameter_update(SYS_PARAMETER_ID id,void *dest_para,uint8_t lenght);

#include "otg_device_hcd.h"
//复用调音工具的发送buf
extern uint8_t  hid_tx_buf[];
uint8_t *p_tx_buf = NULL;

#define  CFG_PC_ONLINE_MAJOR_VERSION		(1)
#define  CFG_PC_ONLINE_MINOR_VERSION		(1)
#define  CFG_PC_ONLINE_PATCH_VERSION	    (0)

#define MAGIC_NUMBER_BT_ADDR				0xB1B5B681
#define MAGIC_NUMBER_BT_NAME				0xB1B5B682
#define MAGIC_NUMBER_BLE_ADDR				0xB1B5B683
#define MAGIC_NUMBER_BLE_NAME				0xB1B5B684

enum
{
	PC_ONLINE_CRC 				= 0x02,
	PC_ONLINE_CRC_ACK 			= 0x03,

	PC_ONLINE_HANDSHAKE 		= 0x10,
	PC_ONLINE_HANDSHAKE_ACK 	= 0x11,

	PC_ONLINE_READ_DATA 		= 0x20,
	PC_ONLINE_READ_DATA_ACK 	= 0x21,
	PC_ONLINE_WRITE 			= 0x30,
	PC_ONLINE_WRITE_ACK 		= 0x31,
};

enum
{
	PC_ONLINE_READ_WRITE_SUCCESS = 0x00,

	PC_ONLINE_READ_OFFSET_ERROR = 0x01,
	PC_ONLINE_READ_CMD_LEN_ERROR = 0x02,

	PC_ONLINE_WRITE_FLASH_TABLE_ERROR = 0x03,
	PC_ONLINE_WRITE_OFFSET_ERROR,
	PC_ONLINE_WRITE_CMD_LEN_ERROR,

	PC_ONLINE_MEMORY_ERROR,
};

#define SET_HID_SEND_PC_ONLINE_CMD(tx_buf,cmd1,cmd2)	tx_buf[0] = 0xA5,\
														tx_buf[1] = 0x5A,\
														tx_buf[2] = cmd1,\
														tx_buf[3] = cmd2




static bool  	hid_tx_flag;
void Union_Effect_Send(uint8_t *buf, uint32_t len)
{
	if(len > 256)
		len = 256;
	memcpy(hid_tx_buf, buf, len);
	hid_tx_flag = TRUE;
}

void PcOnlineReadWriteAck(uint8_t cmd1,uint8_t cmd2,uint8_t *buf,uint8_t len,uint8_t error)
{
	uint8_t tx_buf[16];

	SET_HID_SEND_PC_ONLINE_CMD(tx_buf, cmd1, cmd2);

	//len[2B] + offset[4B] + data_len[2B] + data
	tx_buf[4] = 0x00;
	tx_buf[5] = 4+2+1;
	//offset 直接返回原来的offset
	tx_buf[6] = buf[3];
	tx_buf[7] = buf[4];
	tx_buf[8] = buf[5];
	tx_buf[9] = buf[6];
	//data_len[2B]
	tx_buf[10] = 0x00;
	tx_buf[11] = len;
	//error代码[1B]
	tx_buf[12] = error;
	Union_Effect_Send(tx_buf,13);
}

void FlashSn_Rx(uint8_t *buf,uint16_t buf_len)
{
	switch (buf[0])
	{
	case PC_ONLINE_HANDSHAKE:
		if(buf[3] == 0x81)
		{
			uint8_t tx_buf[16];

			SET_HID_SEND_PC_ONLINE_CMD(tx_buf, PC_PARAM_CMD, PC_ONLINE_HANDSHAKE_ACK);

			//"OK"  + CHIPID +  Vxx.xx.xx + 协议版本
			tx_buf[4] = 0x00;
			tx_buf[5] = 2+1+3+3;
			//OK
			tx_buf[6] = 'O';
			tx_buf[7] = 'K';
			//CHIPID
			tx_buf[8] = CFG_SDK_VER_CHIPID;
			//SDK 版本
			tx_buf[9] = CFG_SDK_MAJOR_VERSION;
			tx_buf[10] = CFG_SDK_MINOR_VERSION;
			tx_buf[11] = CFG_SDK_PATCH_VERSION;
			//协议版本
			tx_buf[12] = CFG_PC_ONLINE_MAJOR_VERSION;
			tx_buf[13] = CFG_PC_ONLINE_MINOR_VERSION;
			tx_buf[14] = CFG_PC_ONLINE_PATCH_VERSION;

			Union_Effect_Send(tx_buf,15);
		}
		else
		{
			uint8_t tx_buf[16];

			SET_HID_SEND_PC_ONLINE_CMD(tx_buf,PC_PARAM_CMD,PC_ONLINE_HANDSHAKE_ACK);

			//"NG" + error代码[1B]
			tx_buf[4] = 0x00;
			tx_buf[5] = 2+1;
			//NG
			tx_buf[6] = 'N';
			tx_buf[7] = 'G';
			//error代码[1B]
			tx_buf[8] = 0x01;

			Union_Effect_Send(tx_buf,9);
		}
		break;

	case PC_ONLINE_READ_DATA:
		if(buf[1] == 6)
		{
			p_tx_buf = osPortMalloc(256);
			uint32_t offset = buf[3]<<24 | buf[4]<<16 | buf[5]<<8 | buf[6];
			uint16_t len = buf[7]<<8 | buf[8];

			if(p_tx_buf == NULL)
			{
				PcOnlineReadWriteAck(PC_PARAM_CMD, PC_ONLINE_READ_DATA_ACK, buf, 0, PC_ONLINE_MEMORY_ERROR);
				break;
			}

			SET_HID_SEND_PC_ONLINE_CMD(p_tx_buf,PC_PARAM_CMD,PC_ONLINE_READ_DATA_ACK);
			//len[2B] + offset[4B] + data_len[2B] + data
			p_tx_buf[4] = 0x00;
			p_tx_buf[5] = 4+2;
			//offset 直接返回原来的offset
			p_tx_buf[6] = buf[3];
			p_tx_buf[7] = buf[4];
			p_tx_buf[8] = buf[5];
			p_tx_buf[9] = buf[6];
			if(offset == MAGIC_NUMBER_BT_ADDR || offset == MAGIC_NUMBER_BLE_ADDR)
			{
#if (FUNC_RF_EN == 1)
				p_tx_buf[10] = 0;
				p_tx_buf[11] = BT_ADDR_SIZE;
				if(offset == MAGIC_NUMBER_BT_ADDR)
					memcpy(&p_tx_buf[12],(void*)(BTDB_CONFIG_ADDR), BT_ADDR_SIZE);
				else
					memcpy(&p_tx_buf[12],(void*)(BTDB_CONFIG_ADDR + BT_ADDR_SIZE), BT_ADDR_SIZE);
#else
				PcOnlineReadWriteAck(PC_PARAM_CMD, PC_ONLINE_READ_DATA_ACK, buf, 0, PC_ONLINE_READ_OFFSET_ERROR);
#endif
			}
			else if((offset == MAGIC_NUMBER_BT_NAME) || (offset == MAGIC_NUMBER_BLE_NAME))
			{
#if (FUNC_RF_EN == 1)
				p_tx_buf[10] = 0;
				if(offset == MAGIC_NUMBER_BT_NAME)
				{
					p_tx_buf[11] = BT_NAME_SIZE;
					memcpy(&p_tx_buf[12],sys_parameter.bt_LocalDeviceName,BT_NAME_SIZE);
				}
				else
				{
					p_tx_buf[11] = BLE_NAME_SIZE;
					memcpy(&p_tx_buf[12],sys_parameter.ble_LocalDeviceName,BLE_NAME_SIZE);
				}
#else
				PcOnlineReadWriteAck(PC_PARAM_CMD, PC_ONLINE_READ_DATA_ACK, buf, 0, PC_ONLINE_READ_OFFSET_ERROR);
#endif
			}
			else
			{
				extern char __data_lmastart;
				if((offset < (uint32_t)(&__data_lmastart)) || (len > (256-12)))
				{
					osPortFree(p_tx_buf);
					PcOnlineReadWriteAck(PC_PARAM_CMD, PC_ONLINE_READ_DATA_ACK, buf, 0, PC_ONLINE_READ_OFFSET_ERROR);
					break;
				}
				p_tx_buf[10] = 0;
				p_tx_buf[11] = len;
				memcpy(&p_tx_buf[12],(void*)offset,len);
			}
			p_tx_buf[5] += p_tx_buf[11];
			Union_Effect_Send(p_tx_buf,p_tx_buf[5] + 6);
			osPortFree(p_tx_buf);
		}
		else
		{
			PcOnlineReadWriteAck(PC_PARAM_CMD, PC_ONLINE_READ_DATA_ACK, buf, 0, PC_ONLINE_READ_CMD_LEN_ERROR);
		}
		break;

	case PC_ONLINE_WRITE:
		{
			uint32_t offset = buf[3]<<24 | buf[4]<<16 | buf[5]<<8 | buf[6];
			uint16_t len = buf[7]<<8 | buf[8];
//			DBG("offset: %x  len: %d\n", offset, len);
			if(offset == MAGIC_NUMBER_BT_ADDR || offset == MAGIC_NUMBER_BLE_ADDR)
			{
				DBG("PC: %x %x %x %x %x %x\n", buf[9],buf[10],buf[11],buf[12],buf[13],buf[14]);
#if (FUNC_RF_EN == 1)
				if(len != BT_ADDR_SIZE)
				{
					PcOnlineReadWriteAck(PC_PARAM_CMD, PC_ONLINE_WRITE_ACK, buf, len, PC_ONLINE_WRITE_CMD_LEN_ERROR);
				}
				else
				{
					uint32_t addr = BTDB_CONFIG_ADDR;
					uint8_t *p_buf = osPortMalloc(4096);
					if(p_buf == NULL)
					{
						PcOnlineReadWriteAck(PC_PARAM_CMD, PC_ONLINE_WRITE_ACK, buf, 0, PC_ONLINE_MEMORY_ERROR);
						break;
					}
					SpiFlashRead(addr,p_buf,4096,10);
					SpiFlashErase(SECTOR_ERASE, (addr/4096), 1);
					if(offset == MAGIC_NUMBER_BLE_ADDR)
						memcpy(&p_buf[0+BT_ADDR_SIZE],&buf[9], BT_ADDR_SIZE);
					else
						memcpy(&p_buf[0],&buf[9], BT_ADDR_SIZE);
					SpiFlashWrite(addr, p_buf, 4096, 1);
					PcOnlineReadWriteAck(PC_PARAM_CMD, PC_ONLINE_WRITE_ACK, buf, len, PC_ONLINE_READ_WRITE_SUCCESS);
					osPortFree(p_buf);
				}
#else
				PcOnlineReadWriteAck(PC_PARAM_CMD, PC_ONLINE_WRITE_ACK, buf, 0, PC_ONLINE_WRITE_OFFSET_ERROR);
#endif
			}
			else if(offset == MAGIC_NUMBER_BT_NAME || offset == MAGIC_NUMBER_BLE_NAME)
			{
#if (FUNC_RF_EN == 1)
				uint32_t addr = get_sys_parameter_addr();
				DBG("PC: %s\n", &buf[9]);
				if(addr == 0)
				{
					PcOnlineReadWriteAck(PC_PARAM_CMD, PC_ONLINE_WRITE_ACK, buf, 0, PC_ONLINE_WRITE_FLASH_TABLE_ERROR);
					break;
				}
				if(offset == MAGIC_NUMBER_BT_NAME)
				{
					if(len > BT_NAME_SIZE)
						len = BT_NAME_SIZE;
					memset(sys_parameter.bt_LocalDeviceName,0,BT_NAME_SIZE);
					memcpy(sys_parameter.bt_LocalDeviceName,&buf[9],len);
					sys_parameter_update(BT_PARA_BT_NAME_ID,sys_parameter.bt_LocalDeviceName,BT_NAME_SIZE);
				}
				else
				{
					if(len > BLE_NAME_SIZE)
						len = BLE_NAME_SIZE;
					if(len == BLE_NAME_SIZE)
						len--;

					memset(sys_parameter.ble_LocalDeviceName,0,BLE_NAME_SIZE);
					memcpy(sys_parameter.ble_LocalDeviceName,&buf[9],len);
					sys_parameter_update(BT_PARA_BLE_NAME_ID,sys_parameter.ble_LocalDeviceName,BLE_NAME_SIZE);
				}

		//		data_shell_read(addr - 32);
		//		SpiFlashErase(SECTOR_ERASE, ((addr - 32)/4096), 1);
		//		SpiFlashWrite(addr, (uint8_t*)&sys_parameter, sizeof(SYS_PARAMETER), 1);
		//		data_shell_write(addr - 32);
				PcOnlineReadWriteAck(PC_PARAM_CMD, PC_ONLINE_WRITE_ACK, buf, len, PC_ONLINE_READ_WRITE_SUCCESS);
#else
				PcOnlineReadWriteAck(PC_PARAM_CMD, PC_ONLINE_WRITE_ACK, buf, 0, PC_ONLINE_WRITE_OFFSET_ERROR);
#endif
			}
			else
			{
				extern char __data_lmastart;
				uint8_t *p_buf;

				//offset少于code位置
				if((offset < (uint32_t)(&__data_lmastart)) || (len > (256-12)))  //(offset < (uint32_t)(&__data_lmastart))
				{
					PcOnlineReadWriteAck(PC_PARAM_CMD, PC_ONLINE_READ_DATA_ACK, buf, 0, PC_ONLINE_WRITE_OFFSET_ERROR);
					break;
				}
				if((offset/4096) != ((offset+len)/4096)) //不在flash同一个BLOCK，需要分2次擦除，暂不支持
				{
					PcOnlineReadWriteAck(PC_PARAM_CMD, PC_ONLINE_READ_DATA_ACK, buf, 0, PC_ONLINE_WRITE_CMD_LEN_ERROR);
					break;
				}
				p_buf = osPortMalloc(4096);
				if(p_buf == NULL)
				{
					PcOnlineReadWriteAck(PC_PARAM_CMD, PC_ONLINE_WRITE_ACK, buf, 0, PC_ONLINE_MEMORY_ERROR);
					break;
				}
				SpiFlashRead((offset/4096)*4096,p_buf,4096,10); //4K对齐读取数据
				SpiFlashErase(SECTOR_ERASE, (offset/4096), 1);	//擦除BLOCK
				//写入offset，相对4K对齐位置的偏移
				memcpy(p_buf + (offset - ((offset/4096)*4096)),&buf[9], len);

				SpiFlashWrite((offset/4096)*4096, p_buf, 4096, 1);
				PcOnlineReadWriteAck(PC_PARAM_CMD, PC_ONLINE_WRITE_ACK, buf, len, PC_ONLINE_READ_WRITE_SUCCESS);
				osPortFree(p_buf);
			}
		}
		break;

	default:
		break;
	}
}


#if 0
void FlashCRC_Rx(uint8_t *buf,uint16_t buf_len)
{
	switch (buf[0])
	{
	case PC_ONLINE_HANDSHAKE:
		if(buf[3] == 0x81)
		{
			uint8_t tx_buf[16];

			SET_HID_SEND_PC_ONLINE_CMD(tx_buf, PC_CRC_CMD, PC_ONLINE_HANDSHAKE_ACK);

			//"OK"  + CHIPID +  Vxx.xx.xx + 协议版本
			tx_buf[4] = 0x00;
			tx_buf[5] = 2+1+3+3;
			//OK
			tx_buf[6] = 'O';
			tx_buf[7] = 'K';
			//CHIPID
			tx_buf[8] = CFG_SDK_VER_CHIPID;
			//SDK 版本
			tx_buf[9] = CFG_SDK_MAJOR_VERSION;
			tx_buf[10] = CFG_SDK_MINOR_VERSION;
			tx_buf[11] = CFG_SDK_PATCH_VERSION;
			//协议版本
			tx_buf[12] = CFG_PC_ONLINE_MAJOR_VERSION;
			tx_buf[13] = CFG_PC_ONLINE_MINOR_VERSION;
			tx_buf[14] = CFG_PC_ONLINE_PATCH_VERSION;

			Union_Effect_Send(tx_buf,15);
		}
		else
		{
			uint8_t tx_buf[16];

			SET_HID_SEND_PC_ONLINE_CMD(tx_buf, PC_CRC_CMD, PC_ONLINE_HANDSHAKE_ACK);

			//"NG" + error代码[1B]
			tx_buf[4] = 0x00;
			tx_buf[5] = 2+1;
			//NG
			tx_buf[6] = 'N';
			tx_buf[7] = 'G';
			//error代码[1B]
			tx_buf[8] = 0x01;

			Union_Effect_Send(tx_buf,9);
		}
		break;

	case PC_ONLINE_CRC:
		p_tx_buf = osPortMalloc(256);
		if(p_tx_buf == NULL)
		{
			PcOnlineReadWriteAck(PC_CRC_CMD, PC_ONLINE_CRC_ACK, buf, 0, PC_ONLINE_MEMORY_ERROR);
			break;
		}
		SET_HID_SEND_PC_ONLINE_CMD(p_tx_buf, PC_CRC_CMD, PC_ONLINE_READ_DATA_ACK);

		p_tx_buf[4] = 0x00;
		p_tx_buf[5] = 16;  //len

		//填充各条目crc值
		extern void get_crc_data(uint8_t *buf);
		get_crc_data(p_tx_buf);

		Union_Effect_Send(p_tx_buf, p_tx_buf[5] + 6);
		osPortFree(p_tx_buf);
		break;
/*
	case PC_ONLINE_READ_DATA:
		if(buf[1] == 6)
		{
			p_tx_buf = osPortMalloc(256);
			uint32_t offset = buf[3]<<24 | buf[4]<<16 | buf[5]<<8 | buf[6];
			uint16_t len = buf[7]<<8 | buf[8];

			if(p_tx_buf == NULL)
			{
				PcOnlineReadWriteAck(PC_ONLINE_READ_DATA_ACK,buf,0,PC_ONLINE_MEMORY_ERROR);
				break;
			}

			SET_HID_SEND_PC_ONLINE_CMD(p_tx_buf, PC_CRC_CMD, PC_ONLINE_READ_DATA_ACK);
			//len[2B] + offset[4B] + data_len[2B] + data
			p_tx_buf[4] = 0x00;
			p_tx_buf[5] = 4+2;
			//offset 直接返回原来的offset
			p_tx_buf[6] = buf[3];
			p_tx_buf[7] = buf[4];
			p_tx_buf[8] = buf[5];
			p_tx_buf[9] = buf[6];
			if(offset == MAGIC_NUMBER_BT_ADDR || offset == MAGIC_NUMBER_BLE_ADDR)
			{
				p_tx_buf[10] = 0;
				p_tx_buf[11] = BT_ADDR_SIZE;
				if(offset == MAGIC_NUMBER_BT_ADDR)
					memcpy(&p_tx_buf[12],BTDB_CONFIG_ADDR, BT_ADDR_SIZE);
				else
					memcpy(&p_tx_buf[12],BTDB_CONFIG_ADDR + BT_ADDR_SIZE, BT_ADDR_SIZE);
			}
			else if(offset == MAGIC_NUMBER_BT_NAME)
			{
				p_tx_buf[10] = 0;
				p_tx_buf[11] = BT_NAME_SIZE;
				memcpy(&p_tx_buf[12],sys_parameter.bt_LocalDeviceName,BT_NAME_SIZE);
			}
			else if(offset == MAGIC_NUMBER_BLE_NAME)
			{
				p_tx_buf[10] = 0;
				p_tx_buf[11] = BLE_NAME_SIZE;
				memcpy(&p_tx_buf[12],sys_parameter.ble_LocalDeviceName,BLE_NAME_SIZE);
			}
			else
			{
				extern char __data_lmastart;
				if(offset < &__data_lmastart || len > (256-12))
				{
					osPortFree(p_tx_buf);
					PcOnlineReadWriteAck(PC_CRC_CMD, PC_ONLINE_READ_DATA_ACK,buf,0,PC_ONLINE_READ_OFFSET_ERROR);
					break;
				}
				p_tx_buf[10] = 0;
				p_tx_buf[11] = len;
				memcpy(&p_tx_buf[12],offset,len);
			}
			p_tx_buf[5] += p_tx_buf[11];
			Union_Effect_Send(p_tx_buf,p_tx_buf[5] + 6);
			osPortFree(p_tx_buf);
		}
		else
		{
			PcOnlineReadWriteAck(PC_CRC_CMD, PC_ONLINE_READ_DATA_ACK,buf,0,PC_ONLINE_READ_CMD_LEN_ERROR);
		}
		break;

	case PC_ONLINE_WRITE:
		{
			uint32_t offset = buf[3]<<24 | buf[4]<<16 | buf[5]<<8 | buf[6];
			uint16_t len = buf[7]<<8 | buf[8];
//			DBG("offset: %x  len: %d\n", offset, len);
			if(offset == MAGIC_NUMBER_BT_ADDR || offset == MAGIC_NUMBER_BLE_ADDR)
			{
				DBG("PC: %x %x %x %x %x %x\n", buf[9],buf[10],buf[11],buf[12],buf[13],buf[14]);
				if(len != BT_ADDR_SIZE)
				{
					PcOnlineReadWriteAck(PC_ONLINE_WRITE_ACK,buf,len,PC_ONLINE_WRITE_CMD_LEN_ERROR);
				}
				else
				{
					uint32_t addr = BTDB_CONFIG_ADDR;
					uint8_t *p_buf = osPortMalloc(4096);
					if(p_buf == NULL)
					{
						PcOnlineReadWriteAck(PC_ONLINE_WRITE_ACK,buf,0,PC_ONLINE_MEMORY_ERROR);
						break;
					}
					SpiFlashRead(addr,p_buf,4096,10);
					SpiFlashErase(SECTOR_ERASE, (addr/4096), 1);
					if(offset == MAGIC_NUMBER_BLE_ADDR)
						memcpy(&p_buf[0+BT_ADDR_SIZE],&buf[9], BT_ADDR_SIZE);
					else
						memcpy(&p_buf[0],&buf[9], BT_ADDR_SIZE);
					SpiFlashWrite(addr, p_buf, 4096, 1);
					PcOnlineReadWriteAck(PC_ONLINE_WRITE_ACK,buf,len,PC_ONLINE_READ_WRITE_SUCCESS);
					osPortFree(p_buf);
				}
			}
			else if(offset == MAGIC_NUMBER_BT_NAME || offset == MAGIC_NUMBER_BLE_NAME)
			{
				uint32_t addr = get_sys_parameter_addr();
				DBG("PC: %s\n", &buf[9]);
				if(addr == 0)
				{
					PcOnlineReadWriteAck(PC_ONLINE_WRITE_ACK,buf,0,PC_ONLINE_WRITE_FLASH_TABLE_ERROR);
					break;
				}
				if(offset == MAGIC_NUMBER_BT_NAME)
				{
					if(len > BT_NAME_SIZE)
						len = BT_NAME_SIZE;
					memset(sys_parameter.bt_LocalDeviceName,0,BT_NAME_SIZE);
					memcpy(sys_parameter.bt_LocalDeviceName,&buf[9],len);
				}
				else
				{
					if(len > BLE_NAME_SIZE)
						len = BLE_NAME_SIZE;
					if(len == BLE_NAME_SIZE)
						len--;

					memset(sys_parameter.ble_LocalDeviceName,0,BLE_NAME_SIZE);
					memcpy(sys_parameter.ble_LocalDeviceName,&buf[9],len);
				}

				SpiFlashErase(SECTOR_ERASE, (addr/4096), 1);
				SpiFlashWrite(addr, (uint8_t*)&sys_parameter, sizeof(SYS_PARAMETER), 1);
				PcOnlineReadWriteAck(PC_ONLINE_WRITE_ACK,buf,len,PC_ONLINE_READ_WRITE_SUCCESS);
			}
			else
			{
				extern char __data_lmastart;
				uint8_t *p_buf;

				//offset少于code位置
				if(offset < &__data_lmastart || len > (256-12))
				{
					PcOnlineReadWriteAck(PC_ONLINE_READ_DATA_ACK,buf,0,PC_ONLINE_WRITE_OFFSET_ERROR);
					break;
				}
				if((offset/4096) != ((offset+len)/4096)) //不在flash同一个BLOCK，需要分2次擦除，暂不支持
				{
					PcOnlineReadWriteAck(PC_ONLINE_READ_DATA_ACK,buf,0,PC_ONLINE_WRITE_CMD_LEN_ERROR);
					break;
				}
				p_buf = osPortMalloc(4096);
				if(p_buf == NULL)
				{
					PcOnlineReadWriteAck(PC_ONLINE_WRITE_ACK,buf,0,PC_ONLINE_MEMORY_ERROR);
					break;
				}
				SpiFlashRead((offset/4096)*4096,p_buf,4096,10); //4K对齐读取数据
				SpiFlashErase(SECTOR_ERASE, (offset/4096), 1);	//擦除BLOCK
				//写入offset，相对4K对齐位置的偏移
				memcpy(p_buf + (offset - ((offset/4096)*4096)),&buf[9], len);

				SpiFlashWrite((offset/4096)*4096, p_buf, 4096, 1);
				PcOnlineReadWriteAck(PC_ONLINE_WRITE_ACK,buf,len,PC_ONLINE_READ_WRITE_SUCCESS);
				osPortFree(p_buf);
			}
		}
		break;
*/
	default:
		break;
	}
}
#endif

typedef struct
{
	uint8_t	flash_param_buf[4*1024];	//4K 参数buf
	uint8_t	hid_tx[256];				//hid 发送buf
	uint8_t write_flag;
}FLASH_PARAMETER_TUNING_CTRL;

static FLASH_PARAMETER_TUNING_CTRL *FlashParam = NULL;

#define SET_HID_SEND_CMD(tx_buf,cmd)	tx_buf[0] = 0xA5,\
										tx_buf[1] = 0x5A,\
										tx_buf[2] = 0x21,\
										tx_buf[3] = cmd

enum
{
	FLASH_PARAMETER_TUNING_HANDSHAKE 		= 0x10,
	FLASH_PARAMETER_TUNING_HANDSHAKE_ACK 	= 0x11,

	FLASH_PARAMETER_TUNING_READ 			= 0x20,
	FLASH_PARAMETER_TUNING_READ_DATA 		= 0x21,
	FLASH_PARAMETER_TUNING_WRITE 			= 0x30,
	FLASH_PARAMETER_TUNING_WRITE_ACK 		= 0x31,

	FLASH_PARAMETER_TUNING_END 				= 0x40,
};

void FlashParamUsb_HandshakeACK(bool ack)
{
	if(!ack)
	{
		uint8_t buf[8];
		SET_HID_SEND_CMD(buf,FLASH_PARAMETER_TUNING_HANDSHAKE_ACK);
		buf[4] = 'N';
		buf[5] = 'G';
		buf[6] = 0;
		Union_Effect_Send(buf,8);
	}
	else
	{
		uint32_t addr = get_sys_parameter_addr();
		uint8_t  check,i;

		memset(&FlashParam->hid_tx[0],0,sizeof(FlashParam->hid_tx));

		SET_HID_SEND_CMD(FlashParam->hid_tx,FLASH_PARAMETER_TUNING_HANDSHAKE_ACK);
		FlashParam->hid_tx[4] = 'O';
		FlashParam->hid_tx[5] = 'K';
		FlashParam->hid_tx[6] = 0;

		FlashParam->hid_tx[7] = CFG_SDK_VER_CHIPID;
		FlashParam->hid_tx[8] = addr>>24;
		FlashParam->hid_tx[9] = addr>>16;
		FlashParam->hid_tx[10] = addr>>8;
		FlashParam->hid_tx[11] = addr;

		check = FlashParam->hid_tx[3]; //不算帧头+控制字
		for(i=4;i<sizeof(FlashParam->hid_tx)-1;i++)
			check += FlashParam->hid_tx[i];

		FlashParam->hid_tx[sizeof(FlashParam->hid_tx)-1] = check;
	}
	hid_tx_flag = TRUE;
}

bool  FlashParamUsb_Tx(DeviceCtx_t *ctx)
{
	if(hid_tx_flag)
	{
		if(!FlashParam)
			OTG_DeviceControlInTransfer(ctx->type,hid_tx_buf,256);
		else
			OTG_DeviceControlInTransfer(ctx->type,FlashParam->hid_tx,256);
		hid_tx_flag = FALSE;
		return TRUE;
	}
	return FALSE;
}

void FlashParamUsb_Rx(uint8_t *buf,uint16_t buf_len)
{
	switch(buf[0])
	{
	default:
		break;
	case FLASH_PARAMETER_TUNING_READ:
		if(FlashParam)
		{
			uint32_t offset;
			uint16_t len;
			uint8_t  check,i;

			offset = buf[1];
			offset <<= 8;
			offset |= buf[2];
			offset <<= 8;
			offset |= buf[3];
			offset <<= 8;
			offset |= buf[4];

			len = buf[5];
			len <<= 8;
			len |= buf[6];

			if(offset >= sizeof(FlashParam->flash_param_buf)) //判断offset超过边界
				len = 0;
			else if((offset+len) >= sizeof(FlashParam->flash_param_buf)) //判断offset+len超过边界，修改len的长度
				len = sizeof(FlashParam->flash_param_buf) - offset;

			if(len > (sizeof(FlashParam->hid_tx) - (2+1+1+4+2+1))) //判断len超过发送buf长度
				len = sizeof(FlashParam->hid_tx) - (2+1+1+4+2+1);  //2B 帧头  + 1B 控制字  + 1B 命令 + 4B offset + 2B len + 1B checksum

			SET_HID_SEND_CMD(FlashParam->hid_tx,FLASH_PARAMETER_TUNING_READ_DATA);
			FlashParam->hid_tx[4] = buf[1];//原封不动 返回offset
			FlashParam->hid_tx[5] = buf[2];
			FlashParam->hid_tx[6] = buf[3];
			FlashParam->hid_tx[7] = buf[4];

			FlashParam->hid_tx[8] = len >> 8;
			FlashParam->hid_tx[9] = len;

			memset(&FlashParam->hid_tx[10],0,sizeof(FlashParam->hid_tx) - (2+1+1+4+2+1));
			if(len > 0)
				memcpy(&FlashParam->hid_tx[10],&FlashParam->flash_param_buf[offset],len);

			check = FlashParam->hid_tx[3]; //不算帧头+控制字
			for(i=4;i<sizeof(FlashParam->hid_tx)-1;i++)
				check += FlashParam->hid_tx[i];

			FlashParam->hid_tx[sizeof(FlashParam->hid_tx)-1] = check;
			hid_tx_flag = TRUE;
		}
		break;
	case FLASH_PARAMETER_TUNING_WRITE:
		if(FlashParam)
		{
			uint32_t offset;
			uint16_t len;
			uint8_t  check,i;

			check = buf[0];
			for(i=1;i<buf_len-1;i++)
				check += buf[i];

			offset = buf[1];
			offset <<= 8;
			offset |= buf[2];
			offset <<= 8;
			offset |= buf[3];
			offset <<= 8;
			offset |= buf[4];

			len = buf[5];
			len <<= 8;
			len |= buf[6];

//			if(buf[buf_len-1] != check)
//			{
//				//check sum通不过，不拷贝数据
//				len = 0;
//			}

			if(offset >= sizeof(FlashParam->flash_param_buf)) //判断offset超过边界
				len = 0;
			else if((offset+len) >= sizeof(FlashParam->flash_param_buf)) //判断offset+len超过边界，修改len的长度
				len = sizeof(FlashParam->flash_param_buf) - offset;

			if(len > (buf_len - (1+4+2+1))) //判断len超过接收buf_len长度
				len = buf_len - (1+4+2+1);	//1B 命令 + 4B offset + 2B len + 1B checksum

			memcpy(&FlashParam->flash_param_buf[offset],buf+7,len);

			SET_HID_SEND_CMD(FlashParam->hid_tx,FLASH_PARAMETER_TUNING_WRITE_ACK);
			FlashParam->hid_tx[4] = buf[1]; //原封不动 返回offset
			FlashParam->hid_tx[5] = buf[2];
			FlashParam->hid_tx[6] = buf[3];
			FlashParam->hid_tx[7] = buf[4];

			FlashParam->hid_tx[8] = len >> 8;
			FlashParam->hid_tx[9] = len;

			memset(&FlashParam->hid_tx[10],0,sizeof(FlashParam->hid_tx) - (2+1+1+4+2+1));

			check = FlashParam->hid_tx[3]; //不算帧头+控制字
			for(i=4;i<sizeof(FlashParam->hid_tx)-1;i++)
				check += FlashParam->hid_tx[i];

			FlashParam->hid_tx[sizeof(FlashParam->hid_tx)-1] = check;
			hid_tx_flag = TRUE;

			FlashParam->write_flag = 1;
		}
		break;
	case FLASH_PARAMETER_TUNING_HANDSHAKE:
		if(memcmp(buf+1,"OK?",3) != 0)
			break;
		if(!FlashParam)
		{
			FlashParam = osPortMalloc(sizeof(FLASH_PARAMETER_TUNING_CTRL));
			if(FlashParam)
			{
				uint32_t addr = get_sys_parameter_addr() - 32; //上位机验证偏移32个字节
				memset(FlashParam,0,sizeof(FLASH_PARAMETER_TUNING_CTRL));
				if(addr && flash_table_is_valid())
				{
					SpiFlashRead(addr, FlashParam->flash_param_buf,sizeof(FlashParam->flash_param_buf), 10);
				}
				else
				{
					//没有找到flash参数，释放内存然后给上位机返回NG
					if(FlashParam)
					{
						osPortFree(FlashParam);
						FlashParam = NULL;
					}
					FlashParamUsb_HandshakeACK(0);
					break;
				}
			}
			else
			{
				//内存申请失败然后给上位机返回NG
				FlashParamUsb_HandshakeACK(0);
				break;
			}
		}
		FlashParamUsb_HandshakeACK(1);
		break;
	case FLASH_PARAMETER_TUNING_END:
		{
			uint8_t buf[8];

			SET_HID_SEND_CMD(buf,FLASH_PARAMETER_TUNING_END);
			buf[4] = 'N';
			buf[5] = 'G';
			buf[6] = 0;
			if(FlashParam)
			{
				if(FlashParam->write_flag)	//写参数 结束
				{
					uint32_t addr = get_sys_parameter_addr() - 32;//上位机验证偏移32个字节
					if(addr && flash_table_is_valid())
					{
						SpiFlashErase(SECTOR_ERASE, addr /4096 , 1);

						SpiFlashWrite(addr, FlashParam->flash_param_buf,sizeof(FlashParam->flash_param_buf), 1);

						buf[4] = 'O';
						buf[5] = 'K';
					}
				}
				else  //读参数 结束
				{
					buf[4] = 'O';
					buf[5] = 'K';
				}
				osPortFree(FlashParam);
				FlashParam = NULL;
			}
			Union_Effect_Send(buf,8);
		}
		break;
	}
}


#endif


