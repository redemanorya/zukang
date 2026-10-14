///////////////////////////////////////////////////////////////////////////////
//               Mountain View Silicon Tech. Inc.
//  Copyright 2012, Mountain View Silicon Tech. Inc., Shanghai, China
//                       All rights reserved.
//  Filename: breakpoint.c
//  ChangLog :
//			修改bp 模块操作方式2014-9-26 ，lujiangang
///////////////////////////////////////////////////////////////////////////////
#include "pmu.h"

#include "app_config.h"
#include "breakpoint.h"
#include "media_play_api.h"
#include "spi_flash.h"
#include "debug.h"
#include "ffpresearch.h"
#include "rtos_api.h"
#include "main_task.h"
#include "ctrlvars.h"
#include "flash_interface.h"
#ifdef CFG_FUNC_BREAKPOINT_EN


#ifdef BP_SAVE_TO_FLASH // 掉电记忆
static bool LoadBPInfoFromFlash(void);
static bool SaveBPInfoToFlash(void);

//优化 flash保存数据，关闭这个宏不优化，保持和以前一致
//用变量记录上一次保存的地址和bak，下一次保存的时候直接使用
//每一次保存数据不在搜索flash保存区域来确认保存的地址和bak
#define BP_SAVE_TO_FLASH_READ_OPTIMIZE
#ifdef BP_SAVE_TO_FLASH_READ_OPTIMIZE
	static struct
	{
		uint32_t BakId;
		uint8_t  SubBakId;
		uint32_t BakAddr;
	}BpRecordFlash = {0,0,0};
#endif
#endif

#ifdef BP_PART_SAVE_TO_NVM
static bool LoadBPPartInfoFromNVM(void);
static bool SaveBPPartInfoToNVM(void);
#endif
static BP_INFO bp_info;

// global sysInfo default init nvm value
const static BP_SYS_INFO sInitSysInfo =
{
#ifdef CFG_APP_BIS_BT_DUAL_CHIP_MODE_EN
	(uint8_t)ModeLeAudioBT,                // CurModuleId
#else
#ifdef CFG_APP_BT_MODE_EN
	(uint8_t)ModeBtAudioPlay,                // CurModuleId
#else
	(uint8_t)ModeUDiskAudioPlay,             // CurModuleId
#endif
#endif
	CFG_PARA_SYS_VOLUME_DEFAULT,     		  //Music Volume default value
	
	0,                         				   //AudioEffectParambin_flow

	0,											//AudioEffectParambin_paramMode

#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
	0,											//AudioEffectParambin_paramMode[1]
#endif

    12,                           				//Mic Volume default value
    
#ifdef CFG_FUNC_MUSIC_EQ_EN
	0,                                          //Eq mode default value 
#endif

	12,                  						//Reverb step default value

#ifdef CFG_FUNC_MIC_TREB_BASS_EN
	8,											// Mic Bass default value
	8,											// Mic Treb default value
#endif
#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
	8,                                          // Music Bass default value
    8,	                    					// Music Treb default value
#endif
    
#if 0//def CFG_FUNC_REMIND_SOUND_EN
	1,                             				// 1:Enable Alarm Remind;
	1,                            				// 1:Chinese Mode
#endif
#ifdef CFG_APP_BT_MODE_EN
	15,								            //bt hf volume
#endif
#ifdef CFG_DMA_RGB_LED_EN
	0,
#endif
#ifdef CFG_FUNC_OXS_PROTOCOL_EN
	CFG_RL_SPEAKER_VOLUME_DEFAULT,              // OXS speaker volume，全新断点记录的默认值
	CFG_RL_HEADPHONE_VOLUME_DEFAULT,            // OXS headphone volume
	0,                                          // OXS speaker mute
	0,                                          // OXS headphone mute
#endif
};

#if (defined(CFG_APP_USB_PLAY_MODE_EN) || defined(CFG_APP_USB_HS_PLAY_MODE_EN) || defined(CFG_APP_CARD_PLAY_MODE_EN))
const static BP_PLAYER_INFO sInitPlayerInfo =
{
	25, 					// Volume
	PLAY_MODE_REPEAT_ALL,  	// PlayMode
	0, 						// LrcFlag
	{0}, 					// CardPlay
	{0}, 					// UDiskPlay
	{0}, 					// UDiskHsPlay
};

#endif

#ifdef CFG_APP_RADIOIN_MODE_EN // radio module default init nvm value
const static BP_RADIO_INFO sInitRadioInfo =
{
	{0}, 					// 已保存电台列表/*MAX_RADIO_CHANNEL_NUM*/
	10,  					// FM音量
	0,   					// 当前FM波段范围(00B—87.5~108MHz (US/Europe, China)、01B—76~90MHz (Japan)、10B—65.8~73MHz (Russia)、11B—60~76MHz
	0,  					// 已保存电台个数
	0,   					// 当前电台频率
};
#endif

unsigned short CRC16Check(const char* Buf,unsigned long BufLen,unsigned short CRC);
// 计算CRC
uint8_t GetCrc8CheckSum(uint8_t* ptr, uint32_t len)
{
	uint32_t crc = 0;
	uint32_t i;

	while(len--)
	{
		crc ^= *ptr++;
		for(i = 0; i < 8; i++)
		{
			if(crc & 0x01)
			{
				crc = ((crc >> 1) ^ 0x8C);
			}
			else
			{
				crc >>= 1;
			}
		}
	}
	return (uint8_t)crc;
}

// 系统启动时装载, 从Flash加载BP 信息
//如果加载失败则加载默认BP 值
//注意，该函数可能会读取Flash，因此需要在SpiFlashInfoInit()函数之后调用
void BP_LoadInfo(void)
{
	bool ret = FALSE;

	APP_DBG("BP_LoadInfo\n");
#ifdef BP_SAVE_TO_FLASH
	ret = LoadBPInfoFromFlash();
#endif

	//APP_DBG("ret = %d\n", ret);
	//获取BP 信息失败，则赋默认值
	if(!ret)
	{
		APP_DBG("BP load err, user defalut value\n");
		memcpy((void*)&bp_info.SysInfo, (void*)&sInitSysInfo, sizeof(BP_SYS_INFO));
#if (defined(CFG_APP_USB_PLAY_MODE_EN) || defined(CFG_APP_USB_HS_PLAY_MODE_EN) || defined(CFG_APP_CARD_PLAY_MODE_EN))
		memcpy((void*)&bp_info.PlayerInfo, (void*)&sInitPlayerInfo, sizeof(BP_PLAYER_INFO));
#endif

#ifdef CFG_APP_RADIOIN_MODE_EN
		memcpy((void*)&bp_info.RadioInfo, (void*)&sInitRadioInfo, sizeof(BP_RADIO_INFO));
#endif
		BP_SaveInfo(0);
		return;
	}
	
#ifdef BP_PART_SAVE_TO_NVM
	LoadBPPartInfoFromNVM();
#endif
	return;
}

//返回bp_info 中相关模块BP 信息的指针
//当需要更新BP 信息时，直接通过该指针往bp_info 变量中写
void* BP_GetInfo(BP_INFO_TYPE InfoType)
{
	void *pInfo = NULL;
	switch(InfoType)
	{
		case BP_SYS_INFO_TYPE:
			pInfo = (void *)&(bp_info.SysInfo);
			break;
#if (defined(CFG_APP_USB_PLAY_MODE_EN) || defined(CFG_APP_USB_HS_PLAY_MODE_EN) || defined(CFG_APP_CARD_PLAY_MODE_EN))
		case BP_PLAYER_INFO_TYPE:
			pInfo = (void *)&(bp_info.PlayerInfo);
			break;
#endif			
#ifdef CFG_APP_RADIOIN_MODE_EN
		case BP_RADIO_INFO_TYPE:
			pInfo = (void *)&(bp_info.RadioInfo);
			break;
#endif			
		default:
			break;
	}
	return pInfo;
}

//备份BP 信息到NVM ，FLASH
//成功返回TRUE， 失败返回FALSE
//Device == 0;写flash
//Device == 1;写NVM
bool BP_SaveInfo(uint32_t Device)
{
	uint16_t crc = 0;

	//ASSERT(sizeof(bp_info) <= BP_MAX_SIZE);
//	APP_DBG("\n!BP_SaveInfo!\n");

	//在Save到NVM、FLASH之前必须先计算出CRC
	//bp info 有独立使用的情况，所有仍然需要有crc8
	crc = GetCrc8CheckSum((uint8_t*)&bp_info, sizeof(BP_INFO)-1);
	bp_info.Crc = crc;

	if(Device == 0)
	{
#ifdef BP_SAVE_TO_FLASH // 掉电记忆
		if(!SaveBPInfoToFlash())
		{
			APP_DBG("SaveBPInfoToFlash == FALSE\n");
			return FALSE;
		}
#endif
	}

	else if(Device == 1)
	{
#ifdef	BP_PART_SAVE_TO_NVM
		if(!SaveBPPartInfoToNVM())
		{
			return FALSE;
		}
#endif
	}
//	APP_DBG("BP Save success\n");
	return TRUE;
}

#ifdef BP_PART_SAVE_TO_NVM
static bool LoadBPPartInfoFromNVM(void)	
{
#if defined(CFG_APP_USB_PLAY_MODE_EN) || defined(CFG_APP_USB_HS_PLAY_MODE_EN) || defined(CFG_APP_CARD_PLAY_MODE_EN)
	BP_PART_INFO BpPartInfo;
	if(PMU_NvmRead(10, (uint8_t *)&BpPartInfo, sizeof(BP_PART_INFO)) == FALSE)
	{
		APP_DBG("NVM load error!\n");
		return FALSE;
	}
	if(BpPartInfo.CardPlayTime == 0xffff)
	{
		BpPartInfo.CardPlayTime = 0;
	}

	if(BpPartInfo.UDiskPlayTime == 0xffff)
	{
		BpPartInfo.UDiskPlayTime = 0;
	}

	if (BpPartInfo.CardPlayTime)
	{
		bp_info.PlayerInfo.PlayCardInfo.PlayTime = BpPartInfo.CardPlayTime;
	}
	if (BpPartInfo.UDiskPlayTime)
	{
		bp_info.PlayerInfo.PlayUDiskInfo.PlayTime = BpPartInfo.UDiskPlayTime;
	}
	if (BpPartInfo.UDiskHsPlayTime)
	{
		bp_info.PlayerInfo.PlayUDiskHsInfo.PlayTime = BpPartInfo.UDiskHsPlayTime;
	}
	return TRUE;
#endif
	return FALSE;
}

static bool SaveBPPartInfoToNVM(void)
{
#if defined(CFG_APP_USB_PLAY_MODE_EN) || defined(CFG_APP_USB_HS_PLAY_MODE_EN) || defined(CFG_APP_CARD_PLAY_MODE_EN)
	BP_PART_INFO BpPartInfo;

	BpPartInfo.CardPlayTime = bp_info.PlayerInfo.PlayCardInfo.PlayTime;
	BpPartInfo.UDiskPlayTime = bp_info.PlayerInfo.PlayUDiskInfo.PlayTime;
	BpPartInfo.UDiskHsPlayTime = bp_info.PlayerInfo.PlayUDiskHsInfo.PlayTime;
	if(PMU_NvmWrite(10, (uint8_t *)&BpPartInfo, sizeof(BP_PART_INFO)) == FALSE)
	{
		APP_DBG("NVM write error!\n");
		return FALSE;
	}
#endif
	//APP_DBG("NVM write OK\n");
	return TRUE;
}
#endif

#ifdef BP_SAVE_TO_FLASH // 掉电记忆
#include "spi_flash.h"
#include "flash_table.h"

//（镜像格式：NVM Flag(4Bytes "MV26") + NVM BAK ID(4Bytes) + NVM Size(2Bytes) + NVM SUB BAK ID(1Bytes) + CRC(1Bytes) + NVM Data(Sub1/Sub2/...)）
//（镜像包头：
typedef struct __BP_BAK_WRAPPER_
{
	char 		Magic[4];
	uint16_t 	Crc;
	uint16_t 	BPBakId;  //大块，4K size
	uint16_t 	BPBakSubId;//小块
	uint16_t 	BPBakSize;

}BP_BAK_WRAPPER;

#define	MAGIC_STRING		"MVB6" //BKD CHANGE
#define NVM_BAK_OFFSET		(get_bp_data_addr())	// BP 信息在Flash中的起始地址
#define NVM_BAK_CNT			(BP_DATA_FLASH_SIZE/4096)		// BP 信息存储区域个数,小于8
#define NVM_BLOCK_SIZE		(4096)			// BP 信息存数区域大小


// BP 记录信息Size : bp_info 记录大小+ 记录包头
#define NVM_BAK_SIZE     	(sizeof(BP_INFO) + sizeof(BP_BAK_WRAPPER))	
#define NVM_SUB_BAK_CNT  	((NVM_BLOCK_SIZE)/NVM_BAK_SIZE)

#define NVM_BAK_ADDR0	NVM_BAK_OFFSET
//static uint32_t NVM_BAK_ADDR0   = NVM_BAK_OFFSET;	//BP INFO 在Flash中的起始地址

// OutputBakId: 输出NVM BAK ID
// OutputSubBakId: 输出NVM SUB BAK ID
// return: NVM镜像地址,LJG 返回最新NVM镜像所在block的首地址
static uint32_t FindLatestBPBak(uint32_t* OutputBakId, uint8_t* OutputSubBakId, bool IsLoadFlash)
{
	uint32_t i, j;
	int32_t TempIndex = -1;
	uint32_t TempBakId;
	BP_BAK_WRAPPER BpBakWrapper;
	uint8_t  TempSubBakId;
	uint8_t Buf[NVM_BAK_CNT][2];

	for(i = 0; i < NVM_BAK_CNT; i++)
	{
		// read flash to get nvm bak id
		SPiFlashReadData(NVM_BAK_ADDR0 + i * NVM_BLOCK_SIZE, (uint8_t*)&BpBakWrapper, sizeof(BpBakWrapper));
		if(memcmp(BpBakWrapper.Magic, MAGIC_STRING, 4) != 0)
		{
			APP_DBG("no Magic\n");
			Buf[i][0] = 0xFF;
			Buf[i][1] = 0xFF;
			continue;
		}

		TempBakId    = BpBakWrapper.BPBakId;
		TempSubBakId = BpBakWrapper.BPBakSubId;
		for(j = 0; j < (NVM_BLOCK_SIZE / NVM_BAK_SIZE); j++)
		{
			SPiFlashReadData(NVM_BAK_ADDR0 + i * NVM_BLOCK_SIZE + j * NVM_BAK_SIZE, (uint8_t*)&BpBakWrapper, sizeof(BpBakWrapper));
			if(memcmp(BpBakWrapper.Magic, MAGIC_STRING, 4) != 0)
			{
				break;
			}

			TempBakId    = BpBakWrapper.BPBakId;;
			TempSubBakId = BpBakWrapper.BPBakSubId;
			//APP_DBG("j = %d, BckId = %d, subBakId = %d\n", j, TempBakId, TempSubBakId);
		}

		if(*OutputBakId > TempBakId)
		{
			continue;
		}

		TempIndex       = i;
		*OutputBakId    = TempBakId;//第几个块
		*OutputSubBakId = TempSubBakId;//块中的第几个小块
		Buf[i][0] = TempBakId;
		Buf[i][1] = TempSubBakId;
	}

	if(TempIndex == -1)
	{
		DBG("no find\n");
		return 0;//没有有找到，返回
	}
	//获取BP数据时，查找有效块
	for(i=0; i<NVM_BAK_CNT; i++)
	{
		if(Buf[i][1] < NVM_SUB_BAK_CNT - 1)
		{
			TempIndex = i;
			*OutputBakId = Buf[i][0];
			*OutputSubBakId = Buf[i][1];
			break;
		}
	}

	if((TempIndex == NVM_BAK_CNT - 1) 
		&& (*OutputBakId == TempIndex)
		&& (*OutputSubBakId == NVM_SUB_BAK_CNT - 1))
	{
		for(i=0; i<NVM_BAK_CNT; i++)
		{
			if((Buf[i][1] == 0xFF) && (Buf[i][0] == 0xFF))
			{
				if(i == 0)
				{
					TempIndex = NVM_BAK_CNT - 1;
				}
				else
				{
					TempIndex = i - 1;
				}
				*OutputBakId = TempIndex;
				*OutputSubBakId = NVM_SUB_BAK_CNT - 1;
				break;
			}
		}
	}

	return (NVM_BAK_ADDR0 + TempIndex * NVM_BLOCK_SIZE);
}

//修改返回值
//-3: flash读取错误
//-2: crc返回值错误，记忆结构体长度没有发生变化（长度没有变，获取数据理论上应该做合法性检查）
//-1: crc返回值错误，记忆结构体长度发生了变化，需要对记忆区域做一次全部擦除
//0: crc返回值正确
static int32_t LoadBakBPInfo(uint32_t BakNvmAddr, uint8_t SubBakId)
{
	uint8_t Crc8Val = 0;
	uint16_t Crc16Val = 0;
	uint32_t i;
	
	BP_BAK_WRAPPER BpBakWrapper;
	SPiFlashReadData((uint32_t)(BakNvmAddr + SubBakId * NVM_BAK_SIZE), (uint8_t*)&BpBakWrapper, sizeof(BpBakWrapper));
	SPiFlashReadData((uint32_t)(BakNvmAddr + SubBakId * NVM_BAK_SIZE + sizeof(BpBakWrapper)), (uint8_t*)&bp_info, sizeof(bp_info));
	
	Crc8Val = GetCrc8CheckSum((uint8_t*)&bp_info, sizeof(BP_INFO)-1);
	Crc16Val = CRC16Check((char*)&BpBakWrapper.BPBakId, sizeof(BP_BAK_WRAPPER) - 6, Crc16Val);
	Crc16Val = CRC16Check((char*)&bp_info, sizeof(BP_INFO), Crc16Val);
//	APP_DBG("crc16 : %x, %x", Crc16Val, BpBakWrapper.Crc);
//	APP_DBG("crc8 : %x, %x", Crc8Val, bp_info.Crc);
	if((Crc16Val != BpBakWrapper.Crc) || (Crc8Val != bp_info.Crc))
	{
		//历史存储数据和 code中的 bp size不同，全部擦除
		if(BpBakWrapper.BPBakSize != sizeof(BP_INFO))
		{
			APP_DBG("BPBakSize changed, erase all\n");
			for(i = 0; i < NVM_BAK_CNT; i++)
			{
				SpiFlashEraseWpr(SECTOR_ERASE, (NVM_BAK_OFFSET + i * NVM_BLOCK_SIZE) /4096 , 1);
			}
#ifdef BP_SAVE_TO_FLASH_READ_OPTIMIZE			
			BpRecordFlash.BakAddr = NVM_BAK_OFFSET;
			BpRecordFlash.BakId   = 0;
			BpRecordFlash.SubBakId= 0;
#endif			
			return -1;
		}
		return -2;
	}

	return 0;
}
static bool SaveBakBPInfo(uint32_t BakNvmAddr, uint32_t NvmBakId, uint8_t SubBakId)	//SaveBakNVM
{
	BP_BAK_WRAPPER pBakWrapper;
	uint16_t Crc16Val = 0;

	memcpy((void*)&pBakWrapper.Magic, (void*)MAGIC_STRING, 4);
	pBakWrapper.BPBakId = NvmBakId;
	pBakWrapper.BPBakSize = sizeof(BP_INFO);
	pBakWrapper.BPBakSubId = SubBakId;

	//首先设置bp_info内部CRC
	bp_info.Crc = GetCrc8CheckSum((uint8_t*)&bp_info, sizeof(BP_INFO)-1);

	Crc16Val = CRC16Check((const char*)&pBakWrapper.BPBakId, sizeof(BP_BAK_WRAPPER) - 6, Crc16Val);
	pBakWrapper.Crc = CRC16Check((const char*)&bp_info, sizeof(BP_INFO), Crc16Val);

	if(SubBakId == 0)
	{
//		uint8_t i;
		uint8_t buf[4];

		SPiFlashReadData(BakNvmAddr, buf, 4);
		if(buf[0] !=0xFF ||buf[1] !=0xFF ||buf[2] !=0xFF ||buf[3] !=0xFF)
		{
			SpiFlashEraseWpr(SECTOR_ERASE, (NVM_BAK_OFFSET + NvmBakId * NVM_BLOCK_SIZE) /4096 , 1);
#ifdef BP_SAVE_TO_FLASH_READ_OPTIMIZE			
			BpRecordFlash.BakAddr = NVM_BAK_OFFSET;
			BpRecordFlash.BakId   = 0;
			BpRecordFlash.SubBakId= 0;
#endif				
		}
		if(SpiFlashWriteWpr(BakNvmAddr, (uint8_t *)&pBakWrapper, sizeof(pBakWrapper), 1) < 0)
		{
			return FALSE;
		}
		if(SpiFlashWriteWpr(BakNvmAddr + sizeof(pBakWrapper), (uint8_t *)&bp_info, sizeof(BP_INFO), 1) < 0)
		{
			return FALSE;
		}
		return TRUE;
	}

	if(SubBakId == NVM_SUB_BAK_CNT - 1)//写到最后一个小块的时候擦除下一个大块
	{
		uint8_t buf[4];
		NvmBakId += 1;
		if(NvmBakId >= NVM_BAK_CNT)
		{
			NvmBakId = 0;
		}
		SPiFlashReadData(BakNvmAddr, buf, 4);
		if(buf[0] !=0xFF ||buf[1] !=0xFF ||buf[2] !=0xFF ||buf[3] !=0xFF)
		{
			SpiFlashEraseWpr(SECTOR_ERASE, (NVM_BAK_OFFSET + NvmBakId * NVM_BLOCK_SIZE) /4096 , 1);
		}
	}

	if(SpiFlashWriteWpr((uint32_t)(BakNvmAddr + SubBakId * NVM_BAK_SIZE), (uint8_t *)&pBakWrapper, sizeof(pBakWrapper), 1) < 0)
	{
		return FALSE;
	}
	if(SpiFlashWriteWpr((uint32_t)(BakNvmAddr + SubBakId * NVM_BAK_SIZE + sizeof(pBakWrapper)), (uint8_t *)&bp_info, sizeof(BP_INFO), 1) < 0)
	{
		return FALSE;
	}
	return TRUE;
}

static bool LoadBPInfoFromFlash(void)	
{
	uint32_t i;
	uint32_t LoadNvmBakId = 0;
	uint32_t NvmBakAddr;
	uint8_t  NvmSubBakId = 0;
	int32_t BakBPInfoLoadState;

	APP_DBG("Load BP INFO START\n");

	NvmBakAddr = FindLatestBPBak(&LoadNvmBakId, &NvmSubBakId, TRUE);
#ifdef BP_SAVE_TO_FLASH_READ_OPTIMIZE			
	BpRecordFlash.BakAddr = NvmBakAddr;
	BpRecordFlash.BakId   = LoadNvmBakId;
	BpRecordFlash.SubBakId= NvmSubBakId;
#endif	
	
	if(NvmBakAddr == 0)//从来没有保存过，查看一下是否需要擦除flash,一次只擦除4K,一个Blcok
	{
		uint8_t i;
		uint8_t buf[4];

#ifdef BP_SAVE_TO_FLASH_READ_OPTIMIZE			
		BpRecordFlash.BakId   = 0;
		BpRecordFlash.SubBakId= 0;
#endif	

		if(SPiFlashReadData(NVM_BAK_ADDR0, buf, 4))
		{
			return FALSE;//error
		}
		if(buf[0] != 0xFF ||buf[1] != 0xFF || buf[0] != 0xFF || buf[0] != 0xFF )
		{
			for(i=0; i<NVM_BAK_CNT; i++)
			{
				SpiFlashEraseWpr(SECTOR_ERASE, (NVM_BAK_OFFSET + i * NVM_BLOCK_SIZE) /4096 , 1);
				//APP_DBG("Frist PowerOn Erase Flash ok (%d)\n", i);
			}
		}
		return FALSE;
	}

	BakBPInfoLoadState = LoadBakBPInfo(NvmBakAddr, NvmSubBakId);
	if(BakBPInfoLoadState == 0)
	{
		return TRUE;
	}
	else if(BakBPInfoLoadState == -2)
	{
		//APP_DBG("load back err\n");
		//以下代码有漏洞，只导出了每个block中最后一个镜像
		// 如果导出镜像失败，则尝试按先后顺序依次导出其它镜像，直到导出成功或遍历了所有的镜像都失败
		for(i = (NvmBakAddr - NVM_BAK_ADDR0) / NVM_BLOCK_SIZE; i < NVM_BAK_CNT; i++)
		{
			if(LoadBakBPInfo(NVM_BAK_ADDR0 + i * NVM_BLOCK_SIZE, (NVM_SUB_BAK_CNT - 1)) == 0)
			{
				return TRUE;
			}
		}

		for(i = 0; i < (NvmBakAddr - NVM_BAK_ADDR0) / NVM_BLOCK_SIZE; i++)
		{
			if(LoadBakBPInfo(NVM_BAK_ADDR0 + i * NVM_BLOCK_SIZE, (NVM_SUB_BAK_CNT - 1)) == 0)
			{
				return TRUE;
			}
		}
		//遍历了掉电区域所有记忆块，均为查找到有效记忆块，则认为有异常，进行一次掉电记忆区域全部擦除
		{
			APP_DBG("not found, erase all\n");
			for(i = 0; i < NVM_BAK_CNT; i++)
			{
				SpiFlashEraseWpr(SECTOR_ERASE, (NVM_BAK_OFFSET + i * NVM_BLOCK_SIZE) /4096 , 1);
			}
#ifdef BP_SAVE_TO_FLASH_READ_OPTIMIZE			
			BpRecordFlash.BakAddr = NVM_BAK_OFFSET;
			BpRecordFlash.BakId   = 0;
			BpRecordFlash.SubBakId= 0;
#endif			
		}
	}

	APP_DBG("Load BP From Flash Fail\n");
	return FALSE;
}

static bool SaveBPInfoToFlash(void)
{
	int32_t i;
	uint32_t NvmBakAddr;//可以计算出来
	
	uint32_t SaveNvmBakId = 0;
	uint8_t  SaveNvmSubBakId = NVM_SUB_BAK_CNT - 1;

#ifdef BP_SAVE_TO_FLASH_READ_OPTIMIZE
	SaveNvmBakId	=	BpRecordFlash.BakId;
	SaveNvmSubBakId = 	BpRecordFlash.SubBakId;
	NvmBakAddr 		= 	BpRecordFlash.BakAddr;	
#else
	// 查找最近的NVM镜像
	NvmBakAddr = FindLatestBPBak(&SaveNvmBakId, &SaveNvmSubBakId, FALSE);
#endif
	APP_DBG("NvmBakAddr = %lx, SaveId = %lu,%u\n", NvmBakAddr, SaveNvmBakId, SaveNvmSubBakId);

	if(NvmBakAddr == 0)
	{
		NvmBakAddr = NVM_BAK_OFFSET;
		SaveNvmBakId = 0;
		SaveNvmSubBakId = 0;
	}
	else
	{
		SaveNvmSubBakId++;
		if(SaveNvmSubBakId == NVM_SUB_BAK_CNT)
		{
			SaveNvmSubBakId = 0;
			SaveNvmBakId++;
			if(SaveNvmBakId == NVM_BAK_CNT)
			{
				SaveNvmBakId = 0;
				NvmBakAddr = NVM_BAK_OFFSET;
			}
			else
			{
				NvmBakAddr += NVM_BLOCK_SIZE;
			}
		}		
	}

	// 导入NVM镜像
	//APP_DBG("NvmBakAddr = %x, SaveNvmBakId = %d, SaveNvmSubBakId = %d\n", (unsigned int)NvmBakAddr, (int)SaveNvmBakId, SaveNvmSubBakId);
	if(SaveBakBPInfo(NvmBakAddr, SaveNvmBakId, SaveNvmSubBakId))
	{
#ifdef BP_SAVE_TO_FLASH_READ_OPTIMIZE			
		BpRecordFlash.BakAddr = NvmBakAddr;
		BpRecordFlash.BakId   = SaveNvmBakId;
		BpRecordFlash.SubBakId= SaveNvmSubBakId;
#endif			
		return TRUE;
	}
	else
	{
		//APP_DBG("\n!back err\n");
		// 如果导入镜像失败，则尝试按先后顺序依次导入到其它镜像区域，直到导入成功或遍历了所有的镜像区域都失败
		for(i = ((NvmBakAddr - NVM_BAK_ADDR0) / NVM_BLOCK_SIZE - 1); i >= 0; i--)
		{
			if(SaveBakBPInfo(NVM_BAK_ADDR0 + i * NVM_BLOCK_SIZE, SaveNvmBakId, 0))
			{
#ifdef BP_SAVE_TO_FLASH_READ_OPTIMIZE			
				BpRecordFlash.BakAddr = NvmBakAddr;
				BpRecordFlash.BakId   = SaveNvmBakId;
				BpRecordFlash.SubBakId= SaveNvmSubBakId;
#endif
				return TRUE;
			}
		}

		for(i = (NVM_BAK_CNT - 1); i >= (NvmBakAddr - NVM_BAK_ADDR0) / NVM_BLOCK_SIZE; i--)
		{
			if(SaveBakBPInfo(NVM_BAK_ADDR0 + i * NVM_BLOCK_SIZE, SaveNvmBakId, 0))
			{
#ifdef BP_SAVE_TO_FLASH_READ_OPTIMIZE			
				BpRecordFlash.BakAddr = NvmBakAddr;
				BpRecordFlash.BakId   = SaveNvmBakId;
				BpRecordFlash.SubBakId= SaveNvmSubBakId;
#endif
				return TRUE;
			}
		}
	}

	APP_DBG("Save BP To Flash Fail\n");
	return FALSE;
}
#endif

void BackupInfoUpdata(BACKUP_MODE mode)
{
	MessageContext		msgSend;

	switch(mode)
	{
	case BACKUP_SYS_INFO:
		msgSend.msgId	= MSG_DEVICE_SERVICE_BP_SYS_INFO;
		break;
	case BACKUP_PLAYER_INFO:
		msgSend.msgId	= MSG_DEVICE_SERVICE_BP_PLAYER_INFO;
		break;
#ifdef BP_PART_SAVE_TO_NVM
	case BACKUP_PLAYER_INFO_2NVM:
		msgSend.msgId	= MSG_DEVICE_SERVICE_BP_PLAYER_INFO_2NVM;
		break;
#endif
	case BACKUP_RADIO_INFO:
		msgSend.msgId	= MSG_DEVICE_SERVICE_BP_RADIO_INFO;
		break;
	case BACKUP_ALL_INFO:
		msgSend.msgId	= MSG_DEVICE_SERVICE_BP_ALL_INFO;
		break;
	default:
		break;
	}
//	DBG("main msg handle : %x\n",GetMainMessageHandle());
	MessageSend(GetMainMessageHandle(), &msgSend);
}

//#endif/*CFG_FUNC_BREAKPOINT_EN*/

#if defined(FUNC_MATCH_PLAYER_BP) && (defined(CFG_APP_USB_PLAY_MODE_EN) || defined(CFG_APP_USB_HS_PLAY_MODE_EN) || defined(CFG_APP_CARD_PLAY_MODE_EN))

static bool 	IsFindBpSongFlag = FALSE;
static uint32_t PlaySoneNum = 0;
static uint32_t PlayTime = 0;

void DiskSongSearchBPInit(void)
{
	PlaySoneNum = 0;
	PlayTime = 0;
	IsFindBpSongFlag = FALSE;
}


//typedef void (*ff_presearch_callback)(DIR *dir, FILINFO *finfo, ff_acc_node *acc_node);
void DiskSongSearchBP(DIR *dir, FILINFO *finfo, ff_acc_node *acc_node)
{
	static BP_PLAYER_INFO *BpPlayInfo;
	//static BpPlayInfo = BP_GetInfo(BP_PLAYER_INFO_TYPE);

//	PlaySoneNum = 0;
//	PlayTime = 0;
//	IsFindBpSongFlag = FALSE;
	BpPlayInfo = BP_GetInfo(BP_PLAYER_INFO_TYPE);

	if(IsFindBpSongFlag == FALSE)
	{
		if(GetSystemMode() == ModeCardAudioPlay)
		{
			//APP_DBG("\n");
			//APP_DBG("BP file size = %d, sect = %d, clust = %d\n", BpPlayInfo->PlayCardInfo.FileSize, BpPlayInfo->PlayCardInfo.DirSect, BpPlayInfo->PlayCardInfo.FirstClust);
			if((BpPlayInfo->PlayCardInfo.DirSect == (uint32_t)dir->sect)
			&& (BpPlayInfo->PlayCardInfo.FirstClust == (uint32_t)finfo->fcl)
			&& (BpPlayInfo->PlayCardInfo.FileSize == (uint32_t)finfo->fsize))
			{
				IsFindBpSongFlag = TRUE;
				PlaySoneNum = acc_node->prior_files_amount;//only test
				PlayTime = BpPlayInfo->PlayCardInfo.PlayTime;
				APP_DBG("Find BP Song, num = %d\n", (int)PlaySoneNum);
			}
		}
		else if(GetSystemMode() == ModeUDiskAudioPlay)
		{
			//APP_DBG("\n");
			//APP_DBG("BP file size = %d, sect = %d, clust = %d\n", BpPlayInfo->PlayUDiskInfo.FileSize, BpPlayInfo->PlayUDiskInfo.DirSect, BpPlayInfo->PlayUDiskInfo.FirstClust);
			if((BpPlayInfo->PlayUDiskInfo.DirSect == (uint32_t)dir->sect)
			&& (BpPlayInfo->PlayUDiskInfo.FirstClust == (uint32_t)finfo->fcl)
			&& (BpPlayInfo->PlayUDiskInfo.FileSize == (uint32_t)finfo->fsize))
			{
				IsFindBpSongFlag = TRUE;
				PlaySoneNum = acc_node->prior_files_amount;//only test
				PlayTime = BpPlayInfo->PlayUDiskInfo.PlayTime;
				APP_DBG("Find BP Song, num = %d\n", (int)PlaySoneNum);
			}
		}
		else if(GetSystemMode() == ModeUDiskHsAudioPlay)
		{
			//APP_DBG("\n");
			//APP_DBG("BP file size = %d, sect = %d, clust = %d\n", BpPlayInfo->PlayUDiskInfo.FileSize, BpPlayInfo->PlayUDiskInfo.DirSect, BpPlayInfo->PlayUDiskInfo.FirstClust);
			if((BpPlayInfo->PlayUDiskHsInfo.DirSect == (uint32_t)dir->sect)
			&& (BpPlayInfo->PlayUDiskHsInfo.FirstClust == (uint32_t)finfo->fcl)
			&& (BpPlayInfo->PlayUDiskHsInfo.FileSize == (uint32_t)finfo->fsize))
			{
				IsFindBpSongFlag = TRUE;
				PlaySoneNum = acc_node->prior_files_amount;//only test
				PlayTime = BpPlayInfo->PlayUDiskHsInfo.PlayTime;
				APP_DBG("Find BP Song, num = %d\n", (int)PlaySoneNum);
			}
		}
	}
}

uint32_t BPDiskSongNumGet(void)
{
	return PlaySoneNum;
}

uint32_t BPDiskSongPlayTimeGet(void)
{
	return PlayTime;
}

uint32_t BPDiskSongPlayModeGet(void)
{
	BP_PLAYER_INFO *pBpMediaPlayInfo;
	uint8_t PlayMode;

	pBpMediaPlayInfo = (BP_PLAYER_INFO *)BP_GetInfo(BP_PLAYER_INFO_TYPE);

	PlayMode = BP_GET_ELEMENT(pBpMediaPlayInfo->PlayMode);

	if(PlayMode >= PLAY_MODE_SUM)
	{
		PlayMode = PLAY_MODE_REPEAT_ALL;
	}

	return PlayMode;
}



#endif //defined(FUNC_MATCH_PLAYER_BP) && (defined(CFG_APP_USB_PLAY_MODE_EN) || defined(CFG_APP_USB_HS_PLAY_MODE_EN) || defined(CFG_APP_CARD_PLAY_MODE_EN))


#endif

