#include <stdio.h>
#include <string.h>
#include "type.h"
#include "app_config.h"
#include "flash_table.h"
#include "spi_flash.h"
#include "flash_interface.h"
#include "debug.h"

/***********************************************************************************
 索引表使用说明（flash_table_init）：
 1、flash容量：
 	1.1、flash地址0xD4位置读取容量信息（4/8/16/32/64/128Mb）。
	1.2、1.1不合法直接读取flash实际容量（8/16/32Mb）。
	1.3、1.2不合法设置容量为8Mb。
	1.4、如修改容量请修改宏 FLASH_CAPACTITY_TYPE
 2、索引表地址：
	2.1、描述1中定义的flash容量减去12KB位置（末尾8KB兼容旧SDK不做改动）。
	2.2、旧SDK没有索引表。
 3、用户数据地址：
	3.1、按名称name在索引表中找到对应地址offset：
		typedef struct _FlashTablePackage
		{
			uint8_t id;
			uint8_t enable_flag;
			uint8_t name[MAX_PACKAGE_NAME_LEN];
			uint8_t offset[4];
			uint8_t check_sum;
			uint8_t	reserve[9];
		} FlashTablePackage;
	3.2、在索引表中没有找到返回0。
		flash_table_read(REMIND_PACKAGE_NAME);
	3.3、8Mb SDK默认使用的数据，没有索引表/未在索引表中找到按 FLASH_DATA_MAP 排列。
		//末尾8KB参量化区域(兼容旧SDK做的保留)
		uint8_t	reserve_1		[1024 * 4];		//size:4KB
		uint8_t	reserve_2		[1024 * 4];		//size:4KB

		uint8_t	flash_table		[1024 * 4];		//size:4KB	索引表
		uint8_t	sys_parameter	[1024 * 4];		//size:4KB	8MB SDK参量化数据
		uint8_t	bt_data			[1024 * 8];		//size:8KB  bt数据 配对记录等
		uint8_t	bp_data			[1024 * 8];		//size:8KB	breakpoint数据
 4、output下文件说明
	4.1、flash.img			flash镜像文件（打包了全部数据），download工具下载
	4.2、BT_Audio_APP.bin	编译器编译产生，烧录代码或者加密使用。
	4.3、main_merge.mva		升级文件包（打包了全部数据）。download工具下载/生产工具使用
***********************************************************************************/

#define MAX_PACKAGE_NAME_LEN		16
//包长度32
#define ONE_PAGEAGE_LEN				32	
typedef struct _FlashTablePackage
{
	uint8_t id;
	uint8_t enable_flag;
	uint8_t name[MAX_PACKAGE_NAME_LEN];
	uint8_t offset[4];
	uint8_t maxlen[4];
	uint8_t	reserve[5];
	uint8_t crc;
} FlashTablePackage;

//加壳字段(32字节)
typedef struct _FlashDataShell
{
	uint8_t Head[4];
	uint8_t Version;
	uint8_t State;
	uint8_t DataCRC[2];
	uint8_t DataLen[3];
	uint8_t MaxLen[3];
	uint8_t name[MAX_PACKAGE_NAME_LEN];
	uint8_t Num;
	uint8_t CheckSum;
} FlashDataShell;

FlashDataShell gDataShell;

struct
{
	uint32_t 	all_crc;			// 总CRC
	uint32_t 	algorithm_crc;		// 算法CRC
	uint32_t 	flashboot_crc;
	uint32_t 	code_crc;
	uint32_t 	flash_map_crc;
	uint32_t 	remind_crc;			// 提示音CRC
	uint32_t	sys_parameter_crc;	// 总CRC
	uint32_t	effect_data_crc;	// 总CRC
	uint32_t	bp_data_crc;		// 总CRC
	uint32_t	bt_data_crc;		// 总CRC
#if defined(FUNC_WIRELESS2_SUPPORT) && WIRELESS_BP_EN
	uint32_t	wireless_bp_crc;	// 总CRC
#endif
#if defined(CFG_FUNC_RECORD_EXTERN_FLASH_EN) && !defined(USE_EXTERN_FLASH_SPACE)
	uint32_t	rec_crc;			// 录音crc 暂时无用
#endif
#ifdef BLE_TUYA_ENABLE
	uint32_t	tuya_crc;			// 涂鸦crc
#endif
}crc_info;

struct
{
	uint32_t 	remind_addr;
#if defined(FUNC_WIRELESS2_SUPPORT) && WIRELESS_BP_EN
	uint32_t	wireless_bp_addr;
#endif
	uint32_t	bp_data_addr;
	uint32_t	bt_data_addr;
	uint32_t	user_config_addr;
	uint32_t	bt_config_addr;
	uint32_t	sys_parameter_addr;
	uint32_t	effect_data_addr;
#ifdef BLE_TUYA_ENABLE
	uint32_t	tuya_data_addr;
#endif
	uint32_t	flash_capacity;
	bool		flash_table_valid;
}flash_table_info;

#ifdef CFG_FUNC_RECORD_EXTERN_FLASH_EN
uint32_t	rec_addr_start;
#endif
#ifdef BLE_TUYA_ENABLE
uint32_t TuyaDataAddrOffset = 0x100000-(8*0x1000);
#endif

static const struct
{
	char *		package_name;	//包名、定义的名称和ini文件中的名称需要完全一致
	uint32_t * 	addr;			//地址
	uint32_t *	crc;			//条目CRC
	uint32_t	offset;			//未找到情况下，默认偏移
}FlashTableReadMap[] =
{
	{"sys_parameter",	&flash_table_info.sys_parameter_addr,	&crc_info.sys_parameter_crc,	SYS_PARA_OFFSET},
	{"bp_data",			&flash_table_info.bp_data_addr,			&crc_info.bp_data_crc,			BP_DATA_OFFSET},
	{"bt_data",			&flash_table_info.bt_data_addr,			&crc_info.bt_data_crc,			BT_DATA_OFFSET},
#if defined(FUNC_WIRELESS2_SUPPORT) && WIRELESS_BP_EN
	{"wireless_bp",		&flash_table_info.wireless_bp_addr,		&crc_info.wireless_bp_crc,		BP_WIRELESS_OFFSET},
#endif
	{"remind",			&flash_table_info.remind_addr,			&crc_info.remind_crc,			0},
#ifdef CFG_EFFECT_PARAMBIN_IN_FLASH_SIZE
	{"effect_data",		&flash_table_info.effect_data_addr,		&crc_info.effect_data_crc,		EFFECT_DATA_OFFSET},
#endif
#if defined(CFG_FUNC_RECORD_EXTERN_FLASH_EN) && !defined(USE_EXTERN_FLASH_SPACE)
	{"rec_data",		&rec_addr_start,						&crc_info.rec_crc,				0},
#endif
#ifdef BLE_TUYA_ENABLE
	{"tuya_data",		&flash_table_info.tuya_data_addr,		&crc_info.tuya_crc,				0},
#endif
};

// 读取音效bin壳信息
void data_shell_read(uint32_t addr)
{
	memset(&gDataShell, 0, sizeof(FlashDataShell));
	if(SpiFlashRead(addr, (uint8_t*)&gDataShell, sizeof(FlashDataShell), 10) != FLASH_NONE_ERR)
	{
		DBG("EffectDataShell read error\n");
	}
}

// 音效bin加壳处理
void data_shell_write(uint32_t addr)
{
	if(SpiFlashWriteWpr(addr, (uint8_t*)&gDataShell, sizeof(FlashDataShell), 0) != FLASH_NONE_ERR)
	{
		DBG("EffectDataShell write error\n");
	}
}


uint32_t flash_table_read(char * name)
{
	uint8_t len;
	uint32_t i;
	uint32_t addr;
	FlashTablePackage package;
	FlashDataShell DataShell;
	uint32_t DataCRC;

	len = strlen(name);
	if(len > MAX_PACKAGE_NAME_LEN)
		len = MAX_PACKAGE_NAME_LEN;

	for(i=0;i<(FLASH_TABLE_SIZE/ONE_PAGEAGE_LEN);i++)
	{
		addr = flash_table_info.flash_capacity - (FLASH_TABLE_OFFSET - sizeof(FlashDataShell)) + (i * ONE_PAGEAGE_LEN);
		if(SPiFlashReadData(addr,(uint8_t*)&package,ONE_PAGEAGE_LEN))
		{
			return 0;
		}
		if(memcmp(package.name,name,len) == 0)//找到
		{
			int32_t addr;
			int32_t maxlen;
			uint8_t check = 0;
			uint8_t *p = (uint8_t*)&package;

			addr = package.offset[3];
			addr <<= 8;
			addr |= package.offset[2];
			addr <<= 8;
			addr |= package.offset[1];
			addr <<= 8;
			addr |= package.offset[0];

			maxlen |= package.maxlen[3];
			maxlen <<= 8;
			maxlen |= package.maxlen[2];
			maxlen <<= 8;
			maxlen |= package.maxlen[1];
			maxlen <<= 8;
			maxlen |= package.maxlen[0];

#if 0
			addr += flash_table_info.flash_capacity;
#endif
			for(i=0;i<31;i++)
			{
				check +=  p[i];
			}

			// APP_DBG("\nflash_table_read: %s\n", package.name);
			// APP_DBG("addr:0x%lx\n", addr);

//			APP_DBG("maxlen:0x%lx\n",maxlen);
//			APP_DBG("id:%x\n",package.id);
//			APP_DBG("enable_flag:%x\n",package.enable_flag);

			if(SPiFlashReadData(addr,(uint8_t*)&DataShell,ONE_PAGEAGE_LEN))
			{
				return 0;
			}
			DataCRC = DataShell.DataCRC[1];
			DataCRC <<= 8;
			DataCRC |= DataShell.DataCRC[0];

			for(i = 0;i < sizeof(FlashTableReadMap)/sizeof(FlashTableReadMap[0]);i++)
			{
				if(memcmp(FlashTableReadMap[i].package_name,name,len) == 0)//找到
				{
					*FlashTableReadMap[i].crc = DataCRC;
//					APP_DBG("DataCRC: 0x%lx\n", FlashTableReadMap[i].crc);
				}
			}

			if (package.enable_flag == 0x11)
			{
				crc_info.all_crc += DataCRC;
			}

			if(addr < 0 || check != package.crc)
			{
				APP_DBG("check:%x  !=  条目crc:%x\n\n", check, package.crc);
				return  0;
			}
			else
			{
//				APP_DBG("check:%x  =  条目crc:%x\n\n", check, package.crc);
				return addr;
			}
		}
	}
	return 0;
}

void flashboot_crc_get(void)
{
	uint32_t addr = 0x0000cc;
	uint8_t crc[4] = {0};
	uint32_t bootCRC;

//	if(SPiFlashReadData(addr,(uint8_t*)&crc,4))
	if(SpiFlashRead(addr, (uint8_t*)&crc, 4, 10) != FLASH_NONE_ERR)
	{
		APP_DBG("bootCRC read is error!\n");
		return;
	}

	bootCRC = crc[1];
	bootCRC <<= 8;
	bootCRC |= crc[0];

	crc_info.flashboot_crc = bootCRC;

	crc_info.all_crc += bootCRC;

//	APP_DBG("flashbootCRC: 0x%lx\n", bootCRC);
}

void code_crc_get(void)
{
	extern char __sdk_code2_start;
	//uint32_t addr = 0x0100cc;
	uint8_t crc[4] = {0};
	uint32_t codeCRC;

//	if(SPiFlashReadData(&__sdk_code2_start + 0xcc,(uint8_t*)&crc,4))
	if(SpiFlashRead((uint32_t)(&__sdk_code2_start + 0xcc), (uint8_t*)&crc, 4, 10) != FLASH_NONE_ERR)
	{
		APP_DBG("codeCRC read is error!\n");
		return;
	}

	codeCRC = crc[1];
	codeCRC <<= 8;
	codeCRC |= crc[0];

	crc_info.code_crc = codeCRC;

	crc_info.all_crc += codeCRC;

//	APP_DBG("codeCRC: 0x%lx\n\n", codeCRC);
}

void algorithm_crc_get(void)
{
	crc_info.algorithm_crc = 0;
}

void flash_table_check(void)
{
	uint32_t addr;
	char Head[4] = "MVFS";
	char name[9] = "flash_map";
	FlashDataShell DataShell;
	uint32_t DataCRC;

	flash_table_info.flash_table_valid = 0;

	addr = flash_table_info.flash_capacity - FLASH_TABLE_OFFSET; // 读到查表壳信息
	if(SPiFlashReadData(addr,(uint8_t*)&DataShell,ONE_PAGEAGE_LEN))
	{
		APP_DBG("flash table read is error!\n");
		return;
	}	

	if(memcmp(DataShell.name, name, sizeof(name)) != 0)//判断名称
	{
		APP_DBG("flash table name is error!\n");
		return;
	}

	if(memcmp(DataShell.Head, Head, sizeof(Head)) != 0)//判断头
	{
		APP_DBG("flash table Head is error!\n");
		return;
	}

	DataCRC = DataShell.DataCRC[1];
	DataCRC <<= 8;
	DataCRC |= DataShell.DataCRC[0];

//	APP_DBG("\nflash_table_read:\n");
//	APP_DBG("\nHead: %s\n", DataShell.Head);
//	APP_DBG("Version: %x\n", DataShell.Version);
//	APP_DBG("State: %x\n", DataShell.State);
//	APP_DBG("DataCRC: 0x%lx\n", DataCRC);
//	APP_DBG("name: %s\n\n\n", DataShell.name);

	{
		uint8_t check = 0;
		uint8_t *p = (uint8_t*)&DataShell;
		uint8_t i;

		for(i=0;i<31;i++){
			check +=  p[i];
		}

		if(check != DataShell.CheckSum)
		{
			APP_DBG("flash table CheckSum is error!  %x  %x \n",check, DataShell.CheckSum);
			return ;
		}
	}	

	crc_info.all_crc = 0;
	crc_info.algorithm_crc = 0;
	crc_info.flashboot_crc = 0;
	crc_info.code_crc = 0;
	crc_info.flash_map_crc = 0;
	crc_info.remind_crc = 0;
	crc_info.sys_parameter_crc = 0;
	crc_info.effect_data_crc = 0;
#if defined(FUNC_WIRELESS2_SUPPORT) && WIRELESS_BP_EN
	crc_info.wireless_bp_crc = 0;
#endif
#if defined(CFG_FUNC_RECORD_EXTERN_FLASH_EN) && !defined(USE_EXTERN_FLASH_SPACE)
	crc_info.rec_crc = 0;
#endif
#ifdef BLE_TUYA_ENABLE
	crc_info.tuya_crc = 0;
#endif
	crc_info.flash_map_crc = DataCRC;

	crc_info.all_crc += DataCRC;

	flashboot_crc_get();

	code_crc_get();

	algorithm_crc_get();

	flash_table_info.flash_table_valid = 1;

	//判断末尾是否是 do not use this
/*	addr = flash_table_info.flash_capacity - FLASH_TABLE_OFFSET + FLASH_TABLE_SIZE - ONE_PAGEAGE_LEN;
	if(SPiFlashReadData(addr,(uint8_t*)&package,ONE_PAGEAGE_LEN))
	{
		return;
	}	

	uint8_t *p = (uint8_t*)&package;
	if(memcmp(p+2,"do not use this",15) == 0)
	{
		flash_table_info.flash_table_valid = 1;
	}*/
}

bool flash_table_is_valid(void)
{
	return flash_table_info.flash_table_valid;
}

void flash_table_init(void)
{
	SPI_FLASH_INFO *ret = SpiFlashInfoGet();
	uint8_t			buf[2],i;
	uint8_t			valid;

	memset(&flash_table_info,0,sizeof(flash_table_info));
	valid = 0;
	
	// if(!SPiFlashReadData(FLASH_CAPACTITY_ADDR, buf, 1))
	if((FLASH_NONE_ERR == SpiFlashRead(FLASH_CAPACTITY_ADDR,(uint8_t *)(buf),1,1)))
	{
		switch(buf[0])
		{
			case 4:   //没有break，这些都是顺序执行
			case 8:
			case 16:
			case 32:
			case 64:
			case 128:
				flash_table_info.flash_capacity = (1024 * 1024 * buf[0])/8;
				valid = 1;
				break;
			default:
				flash_table_info.flash_capacity = ret->Capacity;
				break;
		}
	}
	else
		flash_table_info.flash_capacity = ret->Capacity;

	if( valid == 0 &&
		flash_table_info.flash_capacity != 0x100000 &&
		flash_table_info.flash_capacity != 0x200000 &&
		flash_table_info.flash_capacity != 0x300000 &&
		flash_table_info.flash_capacity != 0x400000
	  )
	{
		flash_table_info.flash_capacity = FLASH_DEFAULT_CAPACTITY;
	}

	flash_table_check();

	APP_DBG("read flash Capacity = 0x%lx 0x%lx,FlashTable: %d\n",ret->Capacity,flash_table_info.flash_capacity,flash_table_info.flash_table_valid);

	for(i = 0;i < sizeof(FlashTableReadMap)/sizeof(FlashTableReadMap[0]);i++)
	{
		*(FlashTableReadMap[i].addr) = flash_table_read(FlashTableReadMap[i].package_name);
		if(*(FlashTableReadMap[i].addr) == 0 && FlashTableReadMap[i].offset != 0)    //这个是不是需要先把所以的addr先清空一下？
		{
			*(FlashTableReadMap[i].addr) = flash_table_info.flash_capacity - FlashTableReadMap[i].offset;
			APP_DBG("no data,use default: %s = 0x%lx\n",FlashTableReadMap[i].package_name,*(FlashTableReadMap[i].addr));
		}
	}

	crc_info.all_crc &= 0xffff;		// 清除高位，只保留低四位
	APP_DBG("\nAllCRC: 0x%lx\n\n\n", crc_info.all_crc);

#ifdef CFG_FUNC_RECORD_EXTERN_FLASH_EN
	#ifdef USE_EXTERN_FLASH_SPACE
		rec_addr_start = CFG_PARA_RECORDS_FLASH_BEGIN_ADDR;
	#else
		APP_DBG("rec_addr_start = 0x%lx\n",rec_addr_start);
	#endif
#endif
#if 0
	flash_table_info.user_config_addr = (flash_table_info.flash_capacity - USER_CONFIG_OFFSET);
	flash_table_info.bt_config_addr = (flash_table_info.flash_capacity - BT_CONFIG_OFFSET);
#else
	flash_table_info.user_config_addr = ret->Capacity - USER_CONFIG_OFFSET;
	flash_table_info.bt_config_addr = ret->Capacity - BT_CONFIG_OFFSET;
#endif

	APP_DBG("user_config_addr = 0x%lx\n",flash_table_info.user_config_addr);
	APP_DBG("bt_config_addr = 0x%lx\n\n",flash_table_info.bt_config_addr);
#ifdef BLE_TUYA_ENABLE
	TuyaDataAddrOffset = flash_table_info.tuya_data_addr + 0x1000;
	APP_DBG("tuya_data_addr = 0x%lx\n",TuyaDataAddrOffset);
#endif
}


void get_crc_data(uint8_t *buf)
{
	buf[6] = crc_info.all_crc & 0x0c; // 高位
	buf[7] = crc_info.all_crc & 0x03; // 低位
	buf[8] = crc_info.algorithm_crc & 0x0c;
	buf[9] = crc_info.algorithm_crc & 0x03;
	buf[10] = crc_info.flashboot_crc & 0x0c;
	buf[11] = crc_info.flashboot_crc & 0x03;
	buf[12] = crc_info.code_crc & 0x0c;
	buf[13] = crc_info.code_crc & 0x03;
	buf[14] = crc_info.flash_map_crc & 0x0c;
	buf[15] = crc_info.flash_map_crc & 0x03;
	buf[16] = crc_info.remind_crc & 0x0c;
	buf[17] = crc_info.remind_crc & 0x03;
	buf[18] = crc_info.sys_parameter_crc & 0x0c;
	buf[19] = crc_info.sys_parameter_crc & 0x03;
	buf[20] = crc_info.effect_data_crc & 0x0c;
	buf[21] = crc_info.effect_data_crc & 0x03;
}

uint32_t get_remind_addr(void)
{
	return flash_table_info.remind_addr + sizeof(FlashDataShell);
}

#if defined(FUNC_WIRELESS2_SUPPORT) && WIRELESS_BP_EN
uint32_t get_wireless_bp_addr(void)
{
	return flash_table_info.wireless_bp_addr + sizeof(FlashDataShell);
}
#endif

uint32_t get_bp_data_addr(void)
{
	return flash_table_info.bp_data_addr + sizeof(FlashDataShell);
}

uint32_t get_bt_data_addr(void)
{
	return flash_table_info.bt_data_addr + sizeof(FlashDataShell);
}

uint32_t get_user_config_addr(void)
{
	return flash_table_info.user_config_addr;
}

uint32_t get_bt_config_addr(void)
{
	return flash_table_info.bt_config_addr;
}

uint32_t get_sys_parameter_addr(void)
{
	return flash_table_info.sys_parameter_addr + sizeof(FlashDataShell);
}

uint32_t get_effect_data_addr(void)
{
	return flash_table_info.effect_data_addr + sizeof(FlashDataShell);
}

uint32_t get_flash_capacity(void)
{
	return flash_table_info.flash_capacity;
}
#ifdef BLE_TUYA_ENABLE
uint32_t get_tuya_data_addr(void)
{
	return TuyaDataAddrOffset;
}
#endif
