//driver、lib
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <nds32_intrinsic.h>
#include "math.h"
#include "dma.h"
#include "clk.h"
#include "timeout.h"
#include "debug.h"
#include "i2s.h"
#include "i2s_interface.h"
#include "audio_adc.h"
#include "dac.h"
#include "spdif.h"
#include "uarts.h"
#include "uarts_interface.h"

//application
#include "app_config.h"
#include "ctrlvars.h"
#include "audio_effect_library.h"
#include "rtos_api.h"
#include "roboeffect_api.h"
#include "communication.h"
#include "audio_core_api.h"
#include "roboeffect_prot.h"
#include "ctrlvars.h"
#include "main_task.h"
#include "audio_effect_parambin.h"
#include "audio_effect_control.h"
#include "user_defined_effect_api.h"
#include "audio_vol.h"
#include "flash_table.h"
#include "flash_interface.h"

#ifdef DUAL_CORE_EN
#include "dualcore_dsp.h"
#endif
#ifdef CFG_FUNC_DEBUG_EN
//#define CFG_FUNC_ROBOEFFECT_DEBUG_EN	//调试AudioAdc module status
#endif
#ifdef CFG_FUNC_USBDEBUG_EN
bool hid_tx_buf_is_used = 0;
#endif
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN

#define  CTL_DATA_SIZE   2

uint8_t  cbuf[8];
uint8_t  tx_buf[256]     = {0xa5, 0x5a, 0x00, 0x00,};

const char AudioLibVer[] = AUDIO_EFFECT_LIBRARY_VERSION;
const char RoboeffectLibVer[] = ROBOEFFECT_LIB_VER;

extern uint8_t  hid_tx_buf[];
extern int16_t cpu_core_clk;
extern uint16_t sizeof_effect_property_for_display(void);

#ifndef ROBOEFFECT_DOUBLE_ENGINE_EN
	extern SubEffect effect_flows[];
#else
	extern SubEffect effect_flows[][2];
#endif

AudioEffectParambinContext *CommunicationChart;  // for Communication
bool g_host_sync_ok = false;  // 上位机同步状态，Deinit后为0，FLOW_INDEX_QUERY完成后为1

static const uint32_t SupportSampleRateList[]={
	8000,
	11025,
	12000,
	16000,
	22050,
	24000,
	32000,
	44100,
	48000,
	//i2s//////////////////
	88200,
	96000,
	176400,
	192000,
};

typedef struct _comm_status_table
{
	uint8_t old_cmd;
	int32_t total_len;
	int32_t remain_len;
	const uint8_t *data;
	uint8_t package_id;
}comm_status_table;

static comm_status_table g_status_machine = {0xff, 0, 0, 0, 0};

static void init_status_matchin(comm_status_table *machine)
{
	memset(machine, 0x00, sizeof(comm_status_table));
	machine->old_cmd = 0xff;
}

static int32_t comm_ret_sample_rate_enum(uint32_t samplerate)
{
	int i;
	for(i=0; i<(sizeof(SupportSampleRateList)/sizeof(uint32_t)); i++)
	{
		if(SupportSampleRateList[i] == samplerate)
			return i;
	}
	return 7;//default 44100
}

uint32_t comm_sample_rate_from_index(uint32_t index)
{
	if(index < (sizeof(SupportSampleRateList)/sizeof(uint32_t)))
		return SupportSampleRateList[index];
	return 44100;//default 44100
}

void HIDUsb_Rx(uint8_t *buf,uint16_t len)
{
	roboeffect_prot_parse_big_block(buf, len);
}

void Communication_Effect_Send(uint8_t *buf, uint32_t len)
{
#if defined(CFG_COMMUNICATION_BY_HS_USB) || defined(CFG_COMMUNICATION_BY_FS_USB)
#ifdef CFG_FUNC_USBDEBUG_EN
	hid_tx_buf_is_used = 1;
#endif
	memcpy(hid_tx_buf, buf, 256);
#endif
#ifdef CFG_COMMUNICATION_BY_UART
	DMA_CircularDataPut(CFG_FUNC_COMMUNICATION_TX_DMA_PORT, buf, len);
#endif
}

uint16_t ota_package_sum = 0;
uint16_t ota_package_num = 0;
uint32_t ota_mva_size = 0;
uint32_t ota_data_offset = 0;
bool just_look = FALSE;

osMutexId Parambin1RWMutex = NULL;
#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
	osMutexId Parambin2RWMutex = NULL;
#endif

typedef enum _UPGRADE_PARAMBIN_ERR_CODE
{
	UPGRADE_PARAMBIN_ERR_CODE_NO_ENOUGH_SPACE = 1, //no enough space
	UPGRADE_PARAMBIN_ERR_CODE_WRONG_PACKAGE_NUM, //wrong package number, maybe lost package
	UPGRADE_PARAMBIN_ERR_CODE_NOT_ALLOWED, //not allowed to update, maybe encrypted chip, or read only data
	UPGRADE_PARAMBIN_ERR_CODE_WRITE_FAILED, //write data to flash failed
	UPGRADE_PARAMBIN_ERR_CODE_INVALID_PARAMBIN, //invalid parambin, check data failed
	UPGRADE_PARAMBIN_ERR_CODE_ERASE_FAILED,
	
	// unknown error code
	UPGRADE_PARAMBIN_ERR_CODE_UNKNOWN,
} UPGRADE_PARAMBIN_ERR_CODE;

#define FLASHBIN_TABLE_PACKAGE_NAME_LEN	16
#define FLASHBIN_TABLE_PACKAGE_LEN		32
#define FLASHBIN_DATA_SHELL_SIZE			32

typedef struct _FLASHBIN_TABLE_PACKAGE
{
	uint8_t id;
	uint8_t enable_flag;
	uint8_t name[FLASHBIN_TABLE_PACKAGE_NAME_LEN];
	uint8_t offset[4];
	uint8_t maxlen[4];
	uint8_t reserve[5];
	uint8_t crc;
} FLASHBIN_TABLE_PACKAGE;

static uint32_t flashbin_u32_le(uint8_t *data)
{
	return ((uint32_t)data[3] << 24) | ((uint32_t)data[2] << 16) | ((uint32_t)data[1] << 8) | data[0];
}

static uint32_t flashbin_effect_data_size_get(void)
{
	SPI_FLASH_INFO *flash_info = SpiFlashInfoGet();
	uint8_t buf[1];
	uint8_t valid = 0;
	uint32_t flash_capacity;
	uint32_t table_addr;
	uint32_t i;
	FLASHBIN_TABLE_PACKAGE package;
	const char package_name[] = "effect_data";
	uint8_t package_name_len = strlen(package_name);

	if(!SPiFlashReadData(FLASH_CAPACTITY_ADDR, buf, 1))
	{
		switch(buf[0])
		{
			case 4:
			case 8:
			case 16:
			case 32:
			case 64:
			case 128:
				flash_capacity = (1024 * 1024 * buf[0]) / 8;
				valid = 1;
				break;
			default:
				flash_capacity = flash_info->Capacity;
				break;
		}
	}
	else
	{
		flash_capacity = flash_info->Capacity;
	}

	if(valid == 0 &&
		flash_capacity != 0x100000 &&
		flash_capacity != 0x200000 &&
		flash_capacity != 0x300000 &&
		flash_capacity != 0x400000)
	{
		flash_capacity = FLASH_DEFAULT_CAPACTITY;
	}

	for(i = 0; i < (FLASH_TABLE_SIZE / FLASHBIN_TABLE_PACKAGE_LEN); i++)
	{
		table_addr = flash_capacity - (FLASH_TABLE_OFFSET - FLASHBIN_DATA_SHELL_SIZE) + (i * FLASHBIN_TABLE_PACKAGE_LEN);
		if(SPiFlashReadData(table_addr, (uint8_t *)&package, FLASHBIN_TABLE_PACKAGE_LEN))
		{
			return 0;
		}

		if(memcmp(package.name, package_name, package_name_len) == 0)
		{
			uint8_t check = 0;
			uint8_t *p = (uint8_t *)&package;
			uint32_t maxlen;
			uint32_t offset;
			uint32_t j;

			for(j = 0; j < (FLASHBIN_TABLE_PACKAGE_LEN - 1); j++)
			{
				check += p[j];
			}

			offset = flashbin_u32_le(package.offset);
			maxlen = flashbin_u32_le(package.maxlen);
			if((int32_t)offset < 0 || check != package.crc || maxlen == 0)
			{
				return 0;
			}

			return maxlen;
		}
	}

	return 0;
}

extern bool RoboeffectOK;
static void flashbin_data_upgrade(uint8_t *buf, uint32_t len)
{
	RoboeffectOK = false;
//	int32_t ret;

	tx_buf[0] = 0xa5;
	tx_buf[1] = 0x5a;
	tx_buf[2] = 0x80;
	tx_buf[3] = 0x06;//length
	tx_buf[4] = 0x04;//flashbin write
	tx_buf[10] = 0x16;
	uint32_t EraseAddr = (AudioEffect_Parambin_GetFlashEffectAddr() - 32);

	PauseAudioCore();

	if(buf[0] == 0x00)//Preparation Packet
	{
		osMutexLock(Parambin1RWMutex);
#if defined(ROBOEFFECT_DOUBLE_ENGINE_EN)
		osMutexLock(Parambin2RWMutex);
#endif
//		SPI_FLASH_INFO *spiflashinfo = SpiFlashInfoGet();
		uint32_t flashSpace = flashbin_effect_data_size_get();
		uint32_t eraseSize;

		if(flashSpace == 0)
		{
			flashSpace = CFG_EFFECT_PARAMBIN_IN_FLASH_SIZE * 1024;
		}

		ota_package_sum = *((uint16_t*)&buf[1]);
		ota_mva_size = *((uint32_t*)&buf[3]);
		DBG("[OTA]: upgrade: %d, %ld\n", ota_package_sum, ota_mva_size);

		data_shell_read(EraseAddr);
		eraseSize = ((ota_mva_size + FLASHBIN_DATA_SHELL_SIZE + 4095) / 4096) * 4096;

		ota_package_num = 0x1234;//to avoid if cmd 0x00 failed, user cannot send cmd 0x01 to force to upgrade
//		if((spiflashinfo->Capacity < UPGRADE_NVM_DATA_ADDR || spiflashinfo->Capacity < ota_mva_size + UPGRADE_NVM_DATA_ADDR))
		if(flashSpace < eraseSize)
		{
			tx_buf[5] = 0x01;
			*((uint32_t*)&tx_buf[6]) = UPGRADE_PARAMBIN_ERR_CODE_NO_ENOUGH_SPACE;
			DBG("[OTA]: flash free space NOT enough! need %dKB\n", (int)ceil(ota_mva_size / 1024.0));
			osMutexUnlock(Parambin1RWMutex);
#if defined(ROBOEFFECT_DOUBLE_ENGINE_EN)
			osMutexUnlock(Parambin2RWMutex);
#endif
		}
		//before erasing, stop all dma and IRQ
		else if(FlashEraseWpr(EraseAddr, eraseSize) == FLASH_NONE_ERR)
		{
			uint32_t ck_sum = 0;

			tx_buf[5] = 0x00;
			ota_data_offset = 0;
			ota_package_num = 1;

			for(int i = 0; i < len; i++)
				ck_sum += buf[i];

			tx_buf[5] = 0x00;
			memcpy(&tx_buf[6], &ck_sum, sizeof(uint32_t));
//			*((uint32_t*)&tx_buf[6]) = ck_sum;
			//DBG("[OTA]: package sum=%d, mva total size=%d\n", ota_package_sum, ota_mva_size);
			// DBG("[OTA]: erase ok, begin to recv ParamBIN\n");
			// APP_DBG("EraseAddr: 0x%lx\n", EraseAddr);

			//todo: de-init roboeffect context and reset all logic
			// GIE_DISABLE();
//			osPortFree(AudioEffectParambin.context_memory);
//			AudioEffectParambin.context_memory = NULL;
		}
		else
		{
			tx_buf[5] = 0x01;
			*((uint32_t*)&tx_buf[6]) = UPGRADE_PARAMBIN_ERR_CODE_ERASE_FAILED;
			DBG("[OTA]: erase flash error!\n");
			osMutexUnlock(Parambin1RWMutex);
#if defined(ROBOEFFECT_DOUBLE_ENGINE_EN)
			osMutexUnlock(Parambin2RWMutex);
#endif
		}
//		DBG("see2\n");
//		for(i=0; i<10; i++)
//			DBG("0x%02X ", tx_buf[i]);
//		DBG("\n");

		just_look = TRUE;

		Communication_Effect_Send(tx_buf, 11);
	}
	else if(buf[0] == 0x01)//Data packet
	{
		uint32_t ck_sum = 0;

		if(ota_package_num == *((uint16_t*)&buf[1]))
		{
			for(int i = 0; i < len; i++)
				ck_sum += buf[i];
			// DBG("W:%d\n", len-3);
			// DataCacheInvalidAll();

			if(just_look)
			{
				//uint8_t *pppp = &buf[3];
				//printf("-->%c, %d, %d, %d\n", pppp[12],pppp[13],pppp[14],pppp[15]);
				just_look = FALSE;
			}

			if(SpiFlashWriteWpr(AudioEffect_Parambin_GetFlashEffectAddr() + ota_data_offset, &buf[3], len-3, 0) == FLASH_NONE_ERR)   //阻塞式写flash
			{
				tx_buf[5] = 0x00;
//				*((uint32_t*)&tx_buf[6]) = ck_sum;
				memcpy(&tx_buf[6], &ck_sum, sizeof(uint32_t));
				ota_data_offset += len-3;
				// DBG("Pkg %d\n", ota_package_num);
				ota_package_num++;
				DBG(".");
			}
			else
			{
				tx_buf[5] = 0x01;
				*((uint32_t*)&tx_buf[6]) = UPGRADE_PARAMBIN_ERR_CODE_WRITE_FAILED;
				DBG("[OTA]: Package (%d) write failed!\n", ota_package_num);
				osMutexUnlock(Parambin1RWMutex);
	#if defined(ROBOEFFECT_DOUBLE_ENGINE_EN)
				osMutexUnlock(Parambin2RWMutex);
	#endif
			}
		}
		else
		{
			tx_buf[5] = 0x01;
			*((uint32_t*)&tx_buf[6]) = UPGRADE_PARAMBIN_ERR_CODE_WRONG_PACKAGE_NUM;
			//DBG("[OTA]: Package num not match.\n");
			osMutexUnlock(Parambin1RWMutex);
	#if defined(ROBOEFFECT_DOUBLE_ENGINE_EN)
			osMutexUnlock(Parambin2RWMutex);
	#endif
		}
		Communication_Effect_Send(tx_buf, 11);
	}
	else if(buf[0] == 0x02)//stop packet
	{
		if(ota_package_num - 1 == ota_package_sum)
		{
			//need check whole mva package's crc16
			DataCacheInvalidAll();

			if(roboeffect_parambin_check_whole_bin((uint8_t *)AudioEffect_Parambin_GetFlashEffectAddr(), NULL) == 0)
			{
				data_shell_write(EraseAddr);
				DBG("\n[OTA]: ParamBIN write finished!\n");
				tx_buf[5] = 0x00;
				mainAppCt.effect_flow_index = 0;
#ifndef ROBOEFFECT_DOUBLE_ENGINE_EN
				mainAppCt.effect_param_mode_index = 0;	//reset all to 0 because flowchart may change
#else
				mainAppCt.effect_param_mode_groupindex[0] = 0;
				mainAppCt.effect_param_mode_groupindex[1] = 0;
#endif
				MessageContext msgSend;
				if(GetSystemMode() == ModeSafe)
				{
					if(GetSysModeState(ModeSafe) == ModeStateReady)
					{
						SetSysModeState(ModeSafe, ModeStateSusend);
					}
					msgSend.msgId = MSG_DEVICE_SERVICE_MODE_OK;
				}
				else
				{
					msgSend.msgId = MSG_EFFECT_REFRESH;
				}
				MessageSend(GetMainMessageHandle(), &msgSend);
			}
			else
			{
				DBG("[OTA]: mva crc error!\n");
				tx_buf[5] = 0x01;
				*((uint32_t*)&tx_buf[6]) = UPGRADE_PARAMBIN_ERR_CODE_INVALID_PARAMBIN;
			}
			//todo: switch roboeffect anyway(if upgrade failed, flashbin-data is broken, run default data)
//			need_switch_mode = SWITCH_MODE_AFTER_DOWNLOAD;//* load all new data, including codec parameters
		}
		else
		{
			//DBG("[OTA]: Package lost???\n");
			tx_buf[5] = 0x01;
			*((uint32_t*)&tx_buf[6]) = UPGRADE_PARAMBIN_ERR_CODE_WRONG_PACKAGE_NUM;
		}

		Communication_Effect_Send(tx_buf, 11);
		osMutexUnlock(Parambin1RWMutex);
#if defined(ROBOEFFECT_DOUBLE_ENGINE_EN)
		osMutexUnlock(Parambin2RWMutex);
#endif
	}
}

bool roboeffect_effect_update_params_entrance(uint8_t addr, uint8_t *buf, uint32_t len)
{
	if(CommunicationChart->context_memory == NULL)
		return FALSE;

	tx_buf[0] = 0xA5;
	tx_buf[1] = 0x5A;
	tx_buf[2] = addr;
	if(len == 0)//require parameters
	{
		const int16_t *params;
		if(((params = roboeffect_get_effect_parameter(CommunicationChart->context_memory, addr, 0xff)) != NULL))
//    		&& (gCtrlVars.AutoRefresh != AutoRefresh_ALL_PARA) && (gCtrlVars.AutoRefresh != AutoRefresh_ALL_EFFECTS_PARA))
		{
			int16_t len = roboeffect_get_effect_parameter_count(CommunicationChart->context_memory, addr);
			if(len >= 0)
			{
				int16_t *pp = (int16_t*)&tx_buf[5];//out parameter 0 is enable/disable

				tx_buf[3] = (len + 1) * 2 + 1;
				tx_buf[4] = 0xff;

				*(pp++) = roboeffect_get_effect_status(CommunicationChart->context_memory, addr);//on/off
				memcpy(pp, params, len * 2);//parameters
				pp += len;

				tx_buf[4 + (len + 1) * 2 + 1] = 0x16;
				Communication_Effect_Send(tx_buf,  tx_buf[3] + 5);
			}
			else
			{
				return FALSE;
			}
		}
		else
		{
//        	if(gCtrlVars.AutoRefresh != AutoRefresh_ALL_EFFECTS_PARA)
//        	{
//        		gCtrlVars.AutoRefresh = AutoRefresh_ALL_PARA;
//    			AudioEffect_CommunicationQueue_Send(gCtrlVars.AutoRefresh);
//        	}
//    		extern void Communication_Effect_0x02(void);
//    		Communication_Effect_0x02();
			return FALSE;
		}
	}
	else//set parameters
	{
//		ROBOEFFECT_ERROR_CODE ret;
		if(buf[0] == 0xff)//all parameters
		{
				int16_t enable = buf[1];//[1] is enable/disable
				int16_t *params = (int16_t*)&buf[3];//[3] is parameters beginning
				AudioEffect_ParamSet(CommunicationChart->context_memory, addr, 0xff, params);
				AudioEffect_Enable(CommunicationChart->context_memory, addr, enable);

				AudioEffect_Parambin_UpdateTempEffectParamsByAddr(CommunicationChart, addr, 0xff, &buf[3]);

				if (addr == (CommunicationChart->user_effect_list->count + 0x81))
				{
					MessageContext msgSend;
					msgSend.msgId = MSG_EFFECT_REFRESH;
					MessageSend(GetMainMessageHandle(), &msgSend);
					gCtrlVars.AutoRefresh = AutoRefresh_ALL_EFFECTS_PARA;
					AudioEffect_CommunicationQueue_Send(gCtrlVars.AutoRefresh);
				}
				else
				{
					gCtrlVars.AutoRefresh = addr;
					AudioEffect_CommunicationQueue_Send(gCtrlVars.AutoRefresh);
				}
		}
		else//one parameter
		{
			if(buf[0] == 0)//only enable/disable
			{
				int16_t enable = buf[1];//[1] is enable/disable
				ROBOEFFECT_ERROR_CODE ret = AudioEffect_Enable(CommunicationChart->context_memory, addr, enable);
				printf("en/dis = %d, %lu:%lu\n", (int)ret, CommunicationChart->user_effect_list->frame_size, \
					roboeffect_recommend_frame_size_upon_effect_change(CommunicationChart->context_memory, CommunicationChart->user_effect_list->frame_size, addr, (enable)?(1):(-1)));
				AudioEffect_Parambin_UpdateTempEffectParamsByAddr(CommunicationChart, addr, 0, &buf[1]);

#if 0   //上位机会自动刷新,SDK不需要在刷一遍
				gCtrlVars.AutoRefresh = addr;
				AudioEffect_CommunicationQueue_Send(gCtrlVars.AutoRefresh);
#endif
			}
			else//other parameter, only one parameter configurated
			{
				int16_t *params = (int16_t*)&buf[1];//[1] is parameter
				int16_t index = buf[0] - 1;//[0] - 1 is parameter index
//				 printf("%02X: %d, %d\n", addr, index, params[0]);
				AudioEffect_ParamSet(CommunicationChart->context_memory, addr, index, params);
				//DBG("addr = %x, index = %d, *params = %x, params = %x\n", addr, index, *params, params);

				AudioEffect_Parambin_UpdateTempEffectParamsByAddr(CommunicationChart, addr, index + 1, &buf[1]);
			}

#if 0
			gCtrlVars.AutoRefresh = addr;
			AudioEffect_CommunicationQueue_Send(gCtrlVars.AutoRefresh);
#endif
		}

		Communication_Effect_Send(&addr, 1);
	}

	return TRUE;
}

void merge_clip_data_init(merge_status *merge_stat)
{
	merge_stat->addr = 0x81;
	merge_stat->src_offset = 0;
	merge_stat->status = 0;
	merge_stat->crc_sum = 0;
}

uint16_t merge_clip_data_fill(uint8_t *context_memory, merge_status *merge_stat, uint8_t *output, uint16_t len_wanted)
{
	uint16_t len_filled = 0, des_offset = 0;
	uint16_t param_max_len = 0;//bytes
	uint8_t *param_src = (uint8_t *)roboeffect_get_effect_parameter(context_memory, merge_stat->addr, 0xff);
	param_max_len = roboeffect_get_effect_parameter_count(context_memory, merge_stat->addr);

	if(param_src == NULL)
		return len_filled;

	while(len_filled < len_wanted)
	{
		// printf("%d, %d\n", len_filled, len_wanted);
		if(merge_stat->status == 0)//addr
		{
			output[des_offset] = merge_stat->addr;
			merge_stat->crc_sum += output[des_offset];
			des_offset ++;len_filled ++;
			merge_stat->status = 1;
		}
		else if(merge_stat->status == 1)//length
		{
			output[des_offset] = 1 + param_max_len * 2;
			merge_stat->crc_sum += output[des_offset];
			des_offset ++;len_filled ++;
			merge_stat->status = 2;
		}
		else if(merge_stat->status == 2)//enable
		{
			output[des_offset] = roboeffect_get_effect_status(context_memory, merge_stat->addr);
			merge_stat->crc_sum += output[des_offset];
			des_offset ++;len_filled ++;
			merge_stat->status = 3;
		}
		else if(merge_stat->status == 3 && merge_stat->src_offset < param_max_len * 2)//param data
		{
			output[des_offset] = param_src[merge_stat->src_offset];
			merge_stat->crc_sum += output[des_offset];
			merge_stat->src_offset ++;
			des_offset ++;len_filled ++;
		}
		else
		if(merge_stat->src_offset >= param_max_len * 2)//swith to next
		{
			merge_stat->addr ++;
			// printf("next: %02X\n", merge_stat->addr);
			param_src = (uint8_t *)roboeffect_get_effect_parameter(context_memory, merge_stat->addr, 0xff);
			param_max_len = roboeffect_get_effect_parameter_count(context_memory, merge_stat->addr);

			if(param_src == NULL)//no other effect node
			{
				// merge_clip_data_init(merge_stat);
				return len_filled;
			}

			merge_stat->status = 0;//reset status
			merge_stat->src_offset = 0;
		}

	}

	return len_filled;
}

merge_status g_merge_statue = {0};
uint8_t graph_cnt = 0;
static uint8_t package_id = 0;
void roboeffect_effect_enquiry_stream(uint8_t *buf, uint32_t tlen)
{
	uint8_t *temp = tx_buf;
	if(buf[0] == V3_PACKAGE_TYPE_PARAM)//enquiry params
	{
		temp[0] = 0xA5;
		temp[1] = 0x5A;
		temp[2] = 0x80;
		temp[3] = 10;//len including EOM
		temp[4] = 0x00;//package id
		temp[5] = 0xFF;
		temp[6] = 0x00;
		temp[7] = V3_PACKAGE_TYPE_PARAM;//type

		temp[8] = 0x01;//graph count

		*(uint16_t *)&temp[9] = AudioCoreFrameSizeGet(DefaultNet);
		*(uint16_t *)&temp[11] = comm_ret_sample_rate_enum(AudioCoreMixSampleRateGet(DefaultNet));
		
		temp[13] = 0x01;//EOM
		temp[14] = 0x16;
		Communication_Effect_Send(temp, temp[3] + 5);

		package_id = 0;
	}
	else if(buf[0] == V3_PACKAGE_TYPE_LIB || buf[0] == V3_PACKAGE_TYPE_STREAM || buf[0] == V3_PACKAGE_TYPE_PARAMBIN_DATA || buf[0] == V3_PACKAGE_TYPE_MODE_ALL_NAME)//enquiry stream
	{
		temp[0] = 0xA5;
		temp[1] = 0x5A;
		temp[2] = 0x80;
		//len later

		if(g_status_machine.old_cmd != buf[0])//if not get EOM, a new cmd is alway the first package
		{
			uint16_t len;

			init_status_matchin(&g_status_machine);
			g_status_machine.old_cmd = buf[0];

			//package type dispatch, no parambin flow-chart
			if(buf[0] == V3_PACKAGE_TYPE_LIB)
			{
				g_status_machine.data = effect_property_for_display;
				g_status_machine.total_len = sizeof_effect_property_for_display();
				// printf("first audio effect info\n");
			}
			else if(buf[0] == V3_PACKAGE_TYPE_STREAM)
			{
				g_status_machine.data = CommunicationChart->user_flow_script;
				g_status_machine.total_len = CommunicationChart->user_flow_script_len;
				// printf("first script\n");
			}
			else if(buf[0] == V3_PACKAGE_TYPE_PARAMBIN_DATA)//! ParamBIN data upload process
			{
				uint32_t tSize = 0;
				if(buf[1] == 0x00)//whole flash bin data
				{
					if(roboeffect_parambin_check_whole_bin((uint8_t *)AudioEffect_Parambin_GetFlashEffectAddr(), &tSize) == ROBOEFFECT_ERROR_OK)
					{
						g_status_machine.data = (uint8_t*)AudioEffect_Parambin_GetFlashEffectAddr();
						g_status_machine.total_len = tSize;
					}
					else {return;}
				}
				else if(buf[1] == 0x01)//flash bin header data
				{
					if(roboeffect_parambin_check_whole_bin((uint8_t *)AudioEffect_Parambin_GetFlashEffectAddr(), &tSize) == ROBOEFFECT_ERROR_OK)
					{
						g_status_machine.data = (uint8_t*)AudioEffect_Parambin_GetFlashEffectAddr();
						g_status_machine.total_len = tSize;
					}
					else {return;}
				}
				else if(buf[1] == 0x02)//flow chart block data
				{
					if(roboeffect_parambin_check_whole_bin((uint8_t *)AudioEffect_Parambin_GetFlashEffectAddr(), &tSize) == ROBOEFFECT_ERROR_OK)
					{
						uint8_t *data_ptr;
						if((data_ptr = roboeffect_parambin_get_flow_by_index((uint8_t *)AudioEffect_Parambin_GetFlashEffectAddr(), buf[2], &tSize)) != NULL)
						{
							g_status_machine.data = data_ptr;
							g_status_machine.total_len = tSize;
						}
						else {return;}
					}
					else {return;}
				}
				else if(buf[1] == 0x03)//sub-type block data
				{
					uint8_t *data_ptr, *flow_data;
					uint32_t flow_size;
					if((flow_data = roboeffect_parambin_get_flow_by_index((uint8_t *)AudioEffect_Parambin_GetFlashEffectAddr(), buf[2], &flow_size)) == NULL)
					{
						return;
					}

					if((data_ptr = roboeffect_parambin_get_sub_type((uint8_t *)AudioEffect_Parambin_GetFlashEffectAddr(), flow_size, *((uint32_t*)&buf[3]), FALSE, &tSize)) != NULL)
					{
						g_status_machine.data = data_ptr;
						g_status_machine.total_len = tSize;
					}
					else {return;}
				}
			}
			else if(buf[0] == V3_PACKAGE_TYPE_MODE_ALL_NAME)//all mode name under current flow-chart
			{
				uint32_t tSize = 0, flow_size;
				uint8_t *data_ptr, *flow_data;

				if((flow_data = roboeffect_parambin_get_flow_by_index((uint8_t *)AudioEffect_Parambin_GetFlashEffectAddr(), mainAppCt.effect_flow_index, &flow_size)) == NULL)
				{
					return;
				}

				if((data_ptr = roboeffect_parambin_get_sub_type((uint8_t *)AudioEffect_Parambin_GetFlashEffectAddr(), flow_size, ROBO_PB_SUBTYPE_PARAMS_MODE_INFO, FALSE, &tSize)) != NULL)
				{
					g_status_machine.data = data_ptr + 4 + 4 + 4;
					g_status_machine.total_len = (int32_t)(data_ptr + 4 + 4 + *((uint32_t*)(data_ptr + 4)));
				}
				else {return;}
			}
			else
			{
				//no such command existed.
				return;
			}

			g_status_machine.remain_len = g_status_machine.total_len;//init remain_len
			if(g_status_machine.remain_len >= STREAM_CLIPS_LEN)
				len = STREAM_CLIPS_LEN;
			else
				len = g_status_machine.remain_len;
			
			temp[3] = len + 7;

			//[package_id, 0xFF, 0x00, type, len_H, len_L, data...data, EOM]
			temp[4] = g_status_machine.package_id;
			temp[5] = 0xFF;
			temp[6] = 0x00;
			temp[7] = buf[0];//message type

			*((uint16_t*)(&temp[8])) = g_status_machine.total_len;

			memcpy(&temp[10], g_status_machine.data, len);

			g_status_machine.remain_len -= len;
			g_status_machine.data += len;

			if(g_status_machine.remain_len > 0)
			{
				temp[10 + len] = 0x00;//not EOM
				g_status_machine.package_id ++;
			}
			else
			{
				temp[10 + len] = 0x01;//EOM
				init_status_matchin(&g_status_machine);//reset for next field
			}
			
			temp[10 + len + 1] = 0x16;

			Communication_Effect_Send(temp, len + 12);
		}
		else
		{
			uint16_t len;
			if(g_status_machine.remain_len >= STREAM_CLIPS_LEN)
				len = STREAM_CLIPS_LEN;
			else
				len = g_status_machine.remain_len;

			temp[3] = len + 2;
			temp[4] = g_status_machine.package_id;
			memcpy(&temp[5], g_status_machine.data, len);

			g_status_machine.remain_len -= len;
			g_status_machine.data += len;

			if(g_status_machine.remain_len > 0)
			{
				temp[5 + len] = 0x00;//not EOM
				g_status_machine.package_id ++;
			}
			else
			{
				temp[5 + len] = 0x01;//EOM
				init_status_matchin(&g_status_machine);//reset for next field

				//switch to next graph
			}
			
			temp[5 + len + 1] = 0x16;

			Communication_Effect_Send(temp, len + 7);
		}
	}
	/*
	在线下载时,如果tlen为1,判定为使用了v2老版本上位机
	*/
	else if((buf[0] == V3_PACKAGE_TYPE_PARAMBIN_UPGRADE) && (tlen != 1))//upgrade parambin data
	{
//		DBG("Flashbin upgrade by ACPWorkbench!\n");
		if(IsAudioPlayerMute() == FALSE)
		{
			HardWareMuteOrUnMute();
		}
		flashbin_data_upgrade(buf + 1, tlen - 1);
		if(IsAudioPlayerMute() == TRUE)
		{
			HardWareMuteOrUnMute();
		}
	}
	else if(buf[0] == V3_PACKAGE_TYPE_FLOW_INDEX_QUERY || buf[0] == V3_PACKAGE_TYPE_MODE_INDEX_QUERY)//query/set flow chart index or parameter mode index
	{
		if(tlen == 1)//query
		{
			if(buf[0] == V3_PACKAGE_TYPE_FLOW_INDEX_QUERY)
			{
#ifndef ROBOEFFECT_DOUBLE_ENGINE_EN
				temp[0] = 0xA5;
				temp[1] = 0x5A;
				temp[2] = 0x80;
				temp[3] = 6+strlen(effect_flow_name[mainAppCt.effect_flow_index]);
				temp[4] = 0x00;//reserved
				temp[5] = 0xFF;
				temp[6] = 0x00;
				temp[7] = buf[0];//type
				temp[8] = 0x01;//package number   //00 index, 01 name
				uint8_t i = strlen(effect_flow_name[mainAppCt.effect_flow_index]);
				memcpy(&temp[9], effect_flow_name[mainAppCt.effect_flow_index], i);
				temp[9+i] = 0x01;// EOM
				temp[10+i] = 0x16;
				Communication_Effect_Send(temp, 11+i);
#else
				uint8_t i = 0;
				temp[0] = 0xA5;
				temp[1] = 0x5A;
				temp[2] = 0x80;
				if(CommunicationChart == &EffectParambin)
				{
					temp[3] = 6+strlen(effect_flow_name[mainAppCt.effect_flow_index][0]);
				}
				else
				{
					temp[3] = 6+strlen(effect_flow_name[mainAppCt.effect_flow_index][1]);
				}

				temp[4] = 0x00;//reserved
				temp[5] = 0xFF;
				temp[6] = 0x00;
				temp[7] = buf[0];//type
				temp[8] = 0x01;//package number   //00 index, 01 name
				if(CommunicationChart == &EffectParambin)
				{
					i = strlen(effect_flow_name[mainAppCt.effect_flow_index][0]);
					memcpy(&temp[9], effect_flow_name[mainAppCt.effect_flow_index][0], i);
				}
				else
				{
					i = strlen(effect_flow_name[mainAppCt.effect_flow_index][1]);
					memcpy(&temp[9], effect_flow_name[mainAppCt.effect_flow_index][1], i);
				}

				temp[9+i] = 0x01;// EOM
				temp[10+i] = 0x16;
				Communication_Effect_Send(temp, 11+i);
#endif
			}
			else if(buf[0] == V3_PACKAGE_TYPE_MODE_INDEX_QUERY)
			{
				temp[0] = 0xA5;
				temp[1] = 0x5A;
				temp[2] = 0x80;
				temp[3] = 7;
				temp[4] = 0x00;//reserved
				temp[5] = 0xFF;
				temp[6] = 0x00;
				temp[7] = buf[0];//type
				temp[8] = 0x00;//package number   //00 index, 01 name
#ifndef ROBOEFFECT_DOUBLE_ENGINE_EN
				temp[9] = mainAppCt.effect_param_mode_index;
#else
				if(CommunicationChart == &EffectParambin)
				{
					temp[9] = mainAppCt.effect_param_mode_groupindex[0];
				}
				else
				{
					temp[9] = mainAppCt.effect_param_mode_groupindex[1];
				}
#endif
				temp[10] = 0x01;// EOM
				temp[11] = 0x16;
				Communication_Effect_Send(temp, temp[3] + 5);
			}
		}
		//else if(tlen == 2)//set
		else
		{
			if(buf[0] == V3_PACKAGE_TYPE_FLOW_INDEX_QUERY)   //切焦点
			{
#ifndef ROBOEFFECT_DOUBLE_ENGINE_EN
	#if 0  //不再支持上位机双击切图
				for(uint8_t i = 0; i < AvailableNUM; i++)
				{
					if(strcmp((char *)(buf + 1), effect_flows[i].name) == 0)
					{
						mainAppCt.effect_flow_index = i;
						mainAppCt.effect_param_mode_index = 0;
						Switch = true;
					}
				}
	#endif
#else
				for(uint8_t i = 0; i < AvailableNUM; i++)
				{
					for(uint8_t j = 0; j < 2; j++)
					{
						if(strcmp((char *)(buf + 1), effect_flows[i][j].name) == 0)
						{
							if(j == 0)
							{
								CommunicationChart = &EffectParambin;
								//mainAppCt.effect_param_mode_index = effect_flows[mainAppCt.effect_flow_index][0].param_mode_index;
							}
							else if(j == 1)
							{
								CommunicationChart = &EffectParambin2;
								//mainAppCt.effect_param_mode_index = effect_flows[mainAppCt.effect_flow_index][1].param_mode_index;
							}
#if 0  //不再支持上位机双击切图
							if(mainAppCt.effect_flow_index != i)
							{
								mainAppCt.effect_flow_index = i;
								mainAppCt.effect_param_mode_groupindex[0] = 0;
								mainAppCt.effect_param_mode_groupindex[1] = 0;
								Switch = true;
							}
#endif
						}
					}
				}
#endif
			}
			else if(buf[0] == V3_PACKAGE_TYPE_MODE_INDEX_QUERY)   //切换音效参数
			{
#ifndef ROBOEFFECT_DOUBLE_ENGINE_EN
				mainAppCt.effect_param_mode_index = buf[1];

#else
				if(CommunicationChart == &EffectParambin)
				{
					mainAppCt.effect_param_mode_groupindex[0] = buf[1];
				}
				else
				{
					mainAppCt.effect_param_mode_groupindex[1] = buf[1];
				}
#endif

				//切换参数需要重新初始化音效，先从Flash里面获取参数,并且可能有帧长和采样率的修改
				MessageContext msgSend;
				msgSend.msgId = MSG_EFFECT_REFRESH;
				MessageSend(GetMainMessageHandle(), &msgSend);
			}

#if 0  //不再支持上位机双击切图
#ifndef ROBOEFFECT_DOUBLE_ENGINE_EN
			DBG("switch flow: %d, mode: %d\n", mainAppCt.effect_flow_index, mainAppCt.effect_param_mode_index);
#else
			DBG("switch flowgroup: %d, mode1: %d\n, mode2: %d\n", mainAppCt.effect_flow_index, mainAppCt.effect_param_mode_groupindex[0], mainAppCt.effect_param_mode_groupindex[1]);
#endif

			if(Switch == true)
			{
				MessageContext msgSend;
				msgSend.msgId = MSG_EFFECT_REFRESH;
				MessageSend(GetMainMessageHandle(), &msgSend);
			}
#endif

			temp[0] = 0x80;
			Communication_Effect_Send(temp, 1);
#if 0   //上位机会自动刷新,SDK不需要在刷一遍
			gCtrlVars.AutoRefresh = AutoRefresh_ALL_EFFECTS_PARA;
			AudioEffect_CommunicationQueue_Send(gCtrlVars.AutoRefresh);
#endif
		}
		g_host_sync_ok = true;  // FLOW_INDEX_QUERY 完成后，标记上位机已同步
	}
	else if(buf[0] == V3_PACKAGE_EFFECT_ALL_PARAM_EFFECT)
	{
		uint16_t len;
		temp[0] = 0xA5;
		temp[1] = 0x5A;
		temp[2] = 0x80;
		//len later

		if(g_status_machine.old_cmd != buf[0])//if not get EOM, a new cmd is alway the first package
		{
			init_status_matchin(&g_status_machine);
			g_status_machine.old_cmd = buf[0];

			temp[4] = g_status_machine.package_id;
			temp[5] = 0xFF;
			temp[6] = 0x00;
			temp[7] = buf[0];//message type

			merge_clip_data_init(&g_merge_statue);
			// g_merge_statue.crc_sum = 0;
			len = merge_clip_data_fill(CommunicationChart->context_memory, &g_merge_statue, &temp[8], STREAM_CLIPS_LEN);

			if(len == STREAM_CLIPS_LEN)
			{
				temp[3] = len + 5;
				temp[8 + len] = 0x00;//not EOM
				g_status_machine.package_id ++;

				temp[8 + len + 1] = 0x16;
				Communication_Effect_Send(temp, len + 10);
			}
			else
			{
				temp[3] = len + 5 + 4;
				*((uint32_t*)(&temp[8 + len])) = g_merge_statue.crc_sum;
				// printf("CRC: %X\n", g_merge_statue.crc_sum);
				temp[8 + len + 4] = 0x01;//EOM
				init_status_matchin(&g_status_machine);//reset for next field

				temp[8 + len + 1 + 4] = 0x16;
				Communication_Effect_Send(temp, len + 10 + 4);
			}
		}
		else
		{
			len = merge_clip_data_fill(CommunicationChart->context_memory, &g_merge_statue, &temp[5], STREAM_CLIPS_LEN);

			temp[4] = g_status_machine.package_id;

			if(len == STREAM_CLIPS_LEN)
			{
				temp[3] = len + 1 + 1;
				temp[5 + len] = 0x00;//not EOM
				g_status_machine.package_id ++;

				temp[5 + len + 1] = 0x16;
				Communication_Effect_Send(temp, len + 7);
			}
			else
			{
				temp[3] = len + 1 + 4 + 1;
				*((uint32_t*)(&temp[5 + len])) = g_merge_statue.crc_sum;
				// printf("CRC: %X\n", g_merge_statue.crc_sum);
				temp[5 + len + 4] = 0x01;//EOM
				init_status_matchin(&g_status_machine);//reset for next field

				temp[5 + len + 4 + 1] = 0x16;
				Communication_Effect_Send(temp, len + 5 + 4 + 1 + 1);
			}

		}
	}
//	else if(buf[0] == V3_PACKAGE_EFFECT_ALL_PARAM_CODEC)
//	{
//		temp[0] = 0xA5;
//		temp[1] = 0x5A;
//		temp[2] = 0x80;
//		//len later
//
//		if(g_status_machine.old_cmd != buf[0])//if not get EOM, a new cmd is alway the first package
//		{
//			uint16_t len;
//
//			init_status_matchin(&g_status_machine);
//			g_status_machine.old_cmd = buf[0];
//
//			g_status_machine.data = (const uint8_t *)&gCtrlVars.HwCt;
//			g_status_machine.total_len = sizeof(HardwareConfigContext);
//			// printf("first audio effect info\n");
//
//			g_status_machine.remain_len = g_status_machine.total_len;//init remain_len
//			if(g_status_machine.remain_len >= STREAM_CLIPS_LEN)
//				len = STREAM_CLIPS_LEN;
//			else
//				len = g_status_machine.remain_len;
//
//			temp[3] = len + 5;
//
//			//[package_id, 0xFF, 0x00, type, len_H, len_L, data...data, EOM]
//			temp[4] = g_status_machine.package_id;
//			temp[5] = 0xFF;
//			temp[6] = 0x00;
//			temp[7] = buf[0];//message type
//
//			memcpy(&temp[8], g_status_machine.data, len);
//
//			g_status_machine.remain_len -= len;
//			g_status_machine.data += len;
//
//			if(g_status_machine.remain_len > 0)
//			{
//				temp[8 + len] = 0x00;//not EOM
//				g_status_machine.package_id ++;
//				temp[8 + len + 1] = 0x16;
//			}
//			else
//			{
//				temp[3] = temp[3] + 4;
//				*((uint32_t*)(&temp[8 + len])) = AudioEffect_GetUserHWCfgCRCLen((uint8_t *)&gCtrlVars.HwCt);
//				temp[8 + len + 4] = 0x01;//EOM
//				temp[8 + len + 4 + 1] = 0x16;
//				init_status_matchin(&g_status_machine);//reset for next field
//			}
//			Communication_Effect_Send(temp, temp[3] + 5);
//		}
//		else
//		{
//			uint16_t len;
//			if(g_status_machine.remain_len >= STREAM_CLIPS_LEN)
//				len = STREAM_CLIPS_LEN;
//			else
//				len = g_status_machine.remain_len;
//
//			temp[3] = len + 2;
//			temp[4] = g_status_machine.package_id;
//			memcpy(&temp[5], g_status_machine.data, len);
//
//			g_status_machine.remain_len -= len;
//			g_status_machine.data += len;
//
//			if(g_status_machine.remain_len > 0)
//			{
//				temp[5 + len] = 0x00;//not EOM
//				g_status_machine.package_id ++;
//				temp[5 + len + 1] = 0x16;
//			}
//			else
//			{
//				temp[3] = temp[3] + 4;
//				*((uint32_t*)(&temp[5 + len])) = AudioEffect_GetUserHWCfgCRCLen((uint8_t *)&gCtrlVars.HwCt);
//				temp[5 + len + 4] = 0x01;//EOM
//				temp[5 + len + 4 + 1] = 0x16;
//				init_status_matchin(&g_status_machine);//reset for next field
//			}
//			Communication_Effect_Send(temp, temp[3] + 5);
//		}
//	}
	else if(buf[0] == V3_PACKAGE_SOURCE_SINK_INFO)
	{
		uint8_t *pp = temp;
		const roboeffect_io_unit *device_node;

		memset(pp, 0x01, 256);//! 0x01 for Unmatched source or sink, 0x00 for Matched

		*pp++ = 0xA5;
		*pp++ = 0x5A;
		*pp++ = 0x80;
		*pp++ = 5 + CommunicationChart->user_effect_steps->des_unit_num + CommunicationChart->user_effect_steps->src_unit_num;//len

		*pp++ = 0x00;//package id
		*pp++ = 0xFF;
		*pp++ = 0x00;
		*pp++ = buf[0];//type

		//source
		device_node = CommunicationChart->source_io;
		for(uint8_t i = 0; i < CommunicationChart->user_effect_steps->src_unit_num; i++, pp++, device_node++)
		{
			for(uint8_t source_index = 0; source_index < AUDIO_CORE_SOURCE_MAX_NUM; source_index++)
			{
				if(AudioEffect_Parambin_GetSourceEnum(roboeffect_parambin_get_io_name(CommunicationChart->context_memory, device_node->io_id)) != -1)
					*pp = 0;
				break;
			}
		}

		//sink
		device_node = CommunicationChart->sink_io;
		for(uint8_t i = 0; i < CommunicationChart->user_effect_steps->des_unit_num; i++, pp++, device_node++)
		{
			for(uint8_t sink_index = 0; sink_index < AUDIO_CORE_SINK_MAX_NUM; sink_index++)
			{
				if(AudioEffect_Parambin_GetSinkEnum(roboeffect_parambin_get_io_name(CommunicationChart->context_memory, device_node->io_id)) != -1)
					*pp = 0;
				break;
			}
		}

		*pp++ = 0x01;//EOM
		*pp++ = 0x16;

		Communication_Effect_Send(temp, temp[3] + 5);
	}
	else if(buf[0] == V3_PACKAGE_CTRLVARS_HIDDEN)
	{
		temp[0] = 0xA5;
		temp[1] = 0x5A;
		temp[2] = 0x80;
#if (GET_PACKAGE(CFG_CHIP_SEL) == GET_PACKAGE(CFG_CHIP_B2640A2))//CFG_CHIP_B2640Ax
		temp[3]  = 43-5; // 数据长度
		temp[4]  = 0x00;
		temp[5]  = 0xFF;
		temp[6]  = 0x00;
		temp[7]  = 0x0C;
		//协议主体
		temp[8] = 0x03;
		temp[9] = 0x08; // 隐藏参数个数
		temp[10] = 0;
		temp[11] = 2;
		temp[12] = 3;
		temp[13] = 4;
		temp[14] = 6;
		temp[15] = 8;
		temp[16] = 10;
		temp[17] = 11;
		temp[18] = 0x04;
		temp[19] = 0x02; // 隐藏参数个数
		temp[20] = 5;
		temp[21] = 7;
		temp[22] = 0x06;
		temp[23] = 0x0C; // 隐藏参数个数
		temp[24] = 0;
		temp[25] = 1;
		temp[26] = 4;
		temp[27] = 5;
		temp[28] = 6;
		temp[29] = 7;
		temp[30] = 8;
		temp[31] = 9;
		temp[32] = 10;
		temp[33] = 11;
		temp[34] = 14;
		temp[35] = 15;
		temp[36] = 0x09; // 模块索引
		temp[37] = 0x00; // 隐藏整个模块
		temp[38] = 0x0B;
		temp[39] = 0x01; // 隐藏参数个数
		temp[40] = 1;
	//
		temp[41] = 1; //EOM
		temp[42] = 0x16;
		Communication_Effect_Send(temp, 43);
#elif (GET_PACKAGE(CFG_CHIP_SEL) == GET_PACKAGE(CFG_CHIP_B2668A2))//CFG_CHIP_B2668Ax
		temp[3]  = 33-5; // 数据长度
		temp[4]  = 0x00;
		temp[5]  = 0xFF;
		temp[6]  = 0x00;
		temp[7]  = 0x0C;
		//协议主体
		temp[8] = 0x03;
		temp[9] = 0x04; // 隐藏参数个数
		temp[10] = 2;
		temp[11] = 3;
		temp[12] = 10;
		temp[13] = 11;
		temp[14] = 0x06;
		temp[15] = 0x08; // 隐藏参数个数
		temp[16] = 0;
		temp[17] = 1;
		temp[18] = 6;
		temp[19] = 7;
		temp[20] = 8;
		temp[21] = 9;
		temp[22] = 10;
		temp[23] = 11;
		temp[24] = 0x0A;
		temp[25] = 0x01; // 隐藏参数个数
		temp[26] = 1;
		temp[27] = 0x0B; // 模块索引
		temp[28] = 0x00; // 隐藏整个模块
		temp[29] = 0x0C; // 模块索引
		temp[30] = 0x00; // 隐藏整个模块

		temp[31] = 1; //EOM
		temp[32] = 0x16;
		Communication_Effect_Send(temp, 33);
#else
		temp[3]  = 5; // 数据长度1 + len * sizeof(int16)
		temp[4]  = 0x00;
		temp[5]  = 0xFF;
		temp[6]  = 0x00;
		temp[7]  = 0x0C;
		temp[8] = 1; //EOM
		temp[9] = 0x16;
		Communication_Effect_Send(temp, 10);
#endif
	}
	else if(buf[0] == V3_PACKAGE_TYPE_MODE_LIB_CRC)
	{
		uint32_t crc32_temp = EFFECT_PROPERTY_BIN_CRC32;
		temp[0] = 0xA5;
		temp[1] = 0x5A;
		temp[2] = 0x80;
		temp[3] = 0x08;//const
		temp[4] = 0x00;//package id
		temp[5] = 0xFF;
		temp[6] = 0x00;//
		temp[7] = 0x0E;

		memcpy(&(temp[8]), &crc32_temp, 4);
		tx_buf[4 + tx_buf[3]] = 0x16;
		Communication_Effect_Send(temp, temp[3] + 5);
	}
	else if(buf[0] == V3_PACKAGE_TYPE_MODE_PARAMBIN_CRC)
	{
		roboeffect_parambin_brief brief_info;

		temp[0] = 0xA5;
		temp[1] = 0x5A;
		temp[2] = 0x80;
		temp[3] = 0x08;//const
		temp[4] = 0x00;//package id
		temp[5] = 0xFF;
		temp[6] = 0x00;//
		temp[7] = V3_PACKAGE_TYPE_MODE_PARAMBIN_CRC;

		roboeffect_parambin_get_brief_info((const uint8_t*) AudioEffect_Parambin_GetFlashEffectAddr(), &brief_info);
		memcpy(&(temp[8]), &brief_info.parambin_crc32, 4);
		tx_buf[4 + tx_buf[3]] = 0x16;

		Communication_Effect_Send(temp, temp[3] + 5);
	}
	else if(buf[0] == V3_PACKAGE_TYPE_PARAMBIN_FLOW_STATUS_UPLOAD)
	{
		if(tlen == 1)//query
		{
			int16_t sum = 0;

			temp[0] = 0xA5;
			temp[1] = 0x5A;
			temp[2] = 0x80;
#ifndef ROBOEFFECT_DOUBLE_ENGINE_EN
			for(uint8_t i = 0; i < AvailableNUM; i++)
			{
				sum = sum + strlen(effect_flows[i].name);
			}
			temp[3] = 0x06 + AvailableNUM*2 + sum;//const
			temp[4] = 0x00;//package id
			temp[5] = 0xFF;
			temp[6] = 0x00;//
			temp[7] = V3_PACKAGE_TYPE_PARAMBIN_FLOW_STATUS_UPLOAD;

			temp[8] = AvailableNUM;

			sum = 9;
			for(uint8_t i = 0; i < AvailableNUM; i++)
			{
				memcpy(&temp[sum], effect_flows[i].name ,strlen(effect_flows[i].name));
				sum = sum + strlen(effect_flows[i].name);
				temp[sum] = 0x3A;//EOM
				temp[sum+1] = effect_flows[i].state;//EOM
				sum = sum+2;
			}

			temp[sum] = 0x01;
			temp[sum+1] = 0x16;//结束

			Communication_Effect_Send(temp, sum+2);
#else
			uint8_t flowSum = 0;
			for(uint8_t i = 0; i < AvailableNUM; i++)
			{
				for(uint8_t j = 0; j < 2; j++)
				{
					if(strlen(effect_flows[i][j].name) != 0)
					{
						flowSum ++;
						sum = sum + strlen(effect_flows[i][j].name);
					}
				}
			}
			temp[3] = 0x06 + flowSum*2 + sum;//const
			temp[4] = 0x00;//package id
			temp[5] = 0xFF;
			temp[6] = 0x00;//
			temp[7] = V3_PACKAGE_TYPE_PARAMBIN_FLOW_STATUS_UPLOAD;

			temp[8] = flowSum;

			sum = 9;
			for(uint8_t i = 0; i < AvailableNUM; i++)
			{
				for(uint8_t j = 0; j < 2; j++)
				{
					if(strlen(effect_flows[i][j].name) != 0)
					{
						memcpy(&temp[sum], effect_flows[i][j].name ,strlen(effect_flows[i][j].name));
						sum = sum + strlen(effect_flows[i][j].name);
						temp[sum] = 0x3A;//EOM
						temp[sum+1] = effect_flows[i][j].state;//EOM
						sum = sum+2;
					}
				}
			}

			temp[sum] = 0x01;
			temp[sum+1] = 0x16;//结束

			Communication_Effect_Send(temp, sum+2);
#endif
		}
		else//set
		{
#ifndef ROBOEFFECT_DOUBLE_ENGINE_EN
			for(uint8_t i = 0; i < AvailableNUM; i++)
			{
				if(strcmp((char *)(buf + 1), effect_flows[i].name) == 0)
				{
					effect_flows[i].state = 0x00;
				}
				else
				{
					effect_flows[i].state = 0x01;
				}
			}
#else
			for(uint8_t i = 0; i < AvailableNUM; i++)
			{
				for(uint8_t j = 0; j < 2; j++)
				{
					if(strcmp((char *)(buf + 1), effect_flows[i][j].name) == 0)
					{
						effect_flows[i][0].state = 0x00;
						effect_flows[i][1].state = 0x00;
						break;
					}
					else
					{
						effect_flows[i][0].state = 0x01;
						effect_flows[i][1].state = 0x01;
					}
				}
			}
#endif
			temp[0] = 0x80;
			Communication_Effect_Send(temp, 1);
		}
	}
}

void Communication_Effect_0x00(void)
{
	char *pp;
    memset(tx_buf, 0, sizeof(tx_buf));
	tx_buf[0]  = 0xa5;
	tx_buf[1]  = 0x5a;
	tx_buf[2]  = 0x00;
	tx_buf[3]  = 0x11;
	tx_buf[4]  = 0x43;//41=B1; 42=B5; 43=B6

	tx_buf[5]  = CFG_SDK_MAJOR_VERSION;
	tx_buf[6]  = CFG_SDK_MINOR_VERSION;
	tx_buf[7]  = CFG_SDK_PATCH_VERSION;

	pp = (char *)AudioLibVer;

	tx_buf[8] = atoi(pp);
	while(pp-AudioLibVer < strlen(AudioLibVer) && *pp != '.'){pp++;}
	tx_buf[9] = atoi(++pp);
	while(pp-AudioLibVer < strlen(AudioLibVer) && *pp != '.'){pp++;}
	tx_buf[10] = atoi(++pp);

	pp = (char *)RoboeffectLibVer;

	tx_buf[11] = atoi(pp);
	while(pp-RoboeffectLibVer < strlen(RoboeffectLibVer) && *pp != '.'){pp++;}
	tx_buf[12] = atoi(++pp);
	while(pp-RoboeffectLibVer < strlen(RoboeffectLibVer) && *pp != '.'){pp++;}
	tx_buf[13] = atoi(++pp);

	tx_buf[14] = 0x02;
	tx_buf[15] = 'B';
	tx_buf[16] = '6';
	tx_buf[17] = 0b11111;

	tx_buf[18]  = PARAMBIN_VER_H;
	tx_buf[19]  = PARAMBIN_VER_M;
	tx_buf[20]  = PARAMBIN_VER_L;
	//-----------------------------------//
	tx_buf[21] = 0x16;
	Communication_Effect_Send(tx_buf, 22);
}

void Communication_Effect_0x01(uint8_t *buf, uint32_t len)
{
	if(len == 0) //ask
	{
		memset(tx_buf, 0, sizeof(tx_buf));
		tx_buf[0]  = 0xa5;
		tx_buf[1]  = 0x5a;
		tx_buf[2]  = 0x01;
		tx_buf[3]  = 1+8*2;
		tx_buf[4]  = 0xff;
		tx_buf[10]  = 0x1;//System Sample Rate Enable
		tx_buf[11] = comm_ret_sample_rate_enum(CommunicationChart->user_effect_list->sample_rate);
		tx_buf[13]  = 0x1;//SDK MCLK为全局
		memcpy(&tx_buf[19], &(CommunicationChart->user_effect_list->frame_size), 2);
		tx_buf[5 + 8*2] = 0x16;
		Communication_Effect_Send(tx_buf, 6 + 8*2);
	}
	//else
	//{
		//do nothing
	//}
}

#pragma pack(1)
typedef struct _roboeffect_0x02_data
{
	//system memory info
	uint8_t sys_mem_info_num;
	uint8_t sys_mem_info_len;
	uint8_t sys_mem_id;
	uint16_t current_mem_used;
	uint16_t max_mem_size;

	//system cpu info
	uint8_t sys_cpu_info_num_1;
	uint8_t sys_cpu_info_len_1;
	uint8_t cpu_id_1;
	uint16_t cpu_mcps_frq_1;
	uint16_t cpu_max_frq_1;
#ifdef DUAL_CORE_EN
	uint8_t sys_cpu_info_num_2;
	uint8_t sys_cpu_info_len_2;
	uint8_t cpu_id_2;
	uint16_t cpu_mcps_frq_2;
	uint16_t cpu_max_frq_2;
#endif

	//roboeffect memory info
	uint8_t robo_mem_info_num_1;
	uint8_t robo_mem_info_len_1;
	uint8_t robo_mem_id_1;//here only one roboeffect
	uint16_t robo_current_mem_1;
	uint16_t robo_max_mem_1;
#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
	uint8_t robo_mem_info_num_2;
	uint8_t robo_mem_info_len_2;
	uint8_t robo_mem_id_2;//here only one roboeffect
	uint16_t robo_current_mem_2;
	uint16_t robo_max_mem_2;
#endif

	//roboeffect error code info
	uint8_t robo_err_code_info_num_1;
	uint8_t robo_err_code_info_len_1;
	uint8_t robo_err_id_1;//here only one roboeffect
	int16_t robo_main_err_code_1;
	uint8_t robo_err_effect_addr_1;
	int16_t robo_err_effect_code_1;

	//refresh info
	uint8_t refresh_code_info_num;
	uint8_t refresh_code_info_len;
	uint8_t reserved;
	uint8_t refresh_mode;
	uint8_t refresh_param_code;

} roboeffect_0x02_data;
#pragma pack()

void Communication_Effect_0x02(void)///systme ram
{
	uint8_t check_sum = 0, *check_ptr;
	ROBOEFFECT_ERROR_CODE audioeffect_err;
	roboeffect_effect_err audioeffect_err_Detail;
	uint16_t UsedRamSize = (CFG_CHIP_RAM_SIZE - osPortRemainMem())/1024;;   //系统级ram使用
	uint16_t CpuMaxFreq = Clock_CoreClockFreqGet() / 1000000;
	uint16_t MaxRamSize = CFG_CHIP_RAM_SIZE/1024;

	audioeffect_err = roboeffect_get_error_code(CommunicationChart->context_memory);
	roboeffect_get_error_code_extended(CommunicationChart->context_memory, &audioeffect_err_Detail);
	roboeffect_0x02_data *data_ptr = (roboeffect_0x02_data*)&tx_buf[6];

	memset(tx_buf, 0, sizeof(tx_buf));

	tx_buf[0]  = 0xa5;
	tx_buf[1]  = 0x5a;
	tx_buf[2]  = 0x02;
	tx_buf[3]  = 1 +  1 + sizeof(roboeffect_0x02_data) + 1 ;
	tx_buf[4]  = 0xfe;
	tx_buf[5]  = 0x05; //Unit num   //初始单元数,后面会修改变化

	data_ptr->sys_mem_info_num = 0x01;
	data_ptr->sys_mem_info_len = 5;
	data_ptr->sys_mem_id = 1;
	data_ptr->current_mem_used = UsedRamSize;
	data_ptr->max_mem_size = MaxRamSize;

	data_ptr->sys_cpu_info_num_1 = 0x02;
	data_ptr->sys_cpu_info_len_1 = 5;
	data_ptr->cpu_id_1 = 1;
	data_ptr->cpu_mcps_frq_1 = (uint16_t)SystemMips;
	data_ptr->cpu_max_frq_1 = CpuMaxFreq;

#ifdef DUAL_CORE_EN
	data_ptr->sys_cpu_info_num_2 = 0x02;
	data_ptr->sys_cpu_info_len_2 = 5;
	data_ptr->cpu_id_2 = 2;
	data_ptr->cpu_mcps_frq_2 = (uint16_t)FrameMcps_Communication();
	data_ptr->cpu_max_frq_2 = CpuMaxFreq;
#endif

	if(RoboeffectOK == 1)
	{
#ifndef ROBOEFFECT_DOUBLE_ENGINE_EN
		data_ptr->robo_mem_info_num_1 = 0x03;
		data_ptr->robo_mem_info_len_1 = 5;
		data_ptr->robo_mem_id_1 = 1;
		data_ptr->robo_current_mem_1 = (EffectParambin.audioeffect_memory_size - roboeffect_get_free_memory_space(EffectParambin.context_memory))/1024;
		data_ptr->robo_max_mem_1 = EffectParambin.audioeffect_memory_size/1024;
#else
		data_ptr->robo_mem_info_num_1 = 0x03;
		data_ptr->robo_mem_info_len_1 = 5;
		data_ptr->robo_mem_id_1 = 1;
		data_ptr->robo_current_mem_1 = (EffectParambin.audioeffect_memory_size - roboeffect_get_free_memory_space(EffectParambin.context_memory))/1024;
		data_ptr->robo_max_mem_1 = EffectParambin.audioeffect_memory_size/1024;
		if(EffectParambin2.context_memory != NULL)
		{
			data_ptr->robo_mem_info_num_2 = 0x03;
			data_ptr->robo_mem_info_len_2 = 5;
			data_ptr->robo_mem_id_2 = 2;
			data_ptr->robo_current_mem_2 = (EffectParambin2.audioeffect_memory_size - roboeffect_get_free_memory_space(EffectParambin2.context_memory))/1024;
			data_ptr->robo_max_mem_2 = EffectParambin2.audioeffect_memory_size/1024;
		}
#endif
	}
	else
	{
#ifndef ROBOEFFECT_DOUBLE_ENGINE_EN
		data_ptr->robo_mem_info_num_1 = 0x03;
		data_ptr->robo_mem_info_len_1 = 5;
		data_ptr->robo_mem_id_1 = 1;
		data_ptr->robo_current_mem_1 = 0;
		data_ptr->robo_max_mem_1 = 0;
#else
		data_ptr->robo_mem_info_num_1 = 0x03;
		data_ptr->robo_mem_info_len_1 = 5;
		data_ptr->robo_mem_id_1 = 1;
		data_ptr->robo_current_mem_1 = 0;
		data_ptr->robo_max_mem_1 = 0;

		data_ptr->robo_mem_info_num_2 = 0x03;
		data_ptr->robo_mem_info_len_2 = 5;
		data_ptr->robo_mem_id_2 = 2;
		data_ptr->robo_current_mem_2 = 0;
		data_ptr->robo_max_mem_2 = 0;
#endif
	}

	data_ptr->robo_err_code_info_num_1 = 0x04;
	data_ptr->robo_err_code_info_len_1 = 6;
	data_ptr->robo_err_id_1 = 1;
	if(RoboeffectOK == 1)
	{
		data_ptr->robo_main_err_code_1 = audioeffect_err;
		data_ptr->robo_err_effect_addr_1 = audioeffect_err_Detail.err_effect_addr;
		data_ptr->robo_err_effect_code_1 = audioeffect_err_Detail.err_effect_code;
	}
	else
	{
		data_ptr->robo_main_err_code_1 = 0;
		data_ptr->robo_err_effect_addr_1 = 0;
		data_ptr->robo_err_effect_code_1 = 0;
	}

	data_ptr->refresh_code_info_num = 0x05;
	data_ptr->refresh_code_info_len = 0x03;
	data_ptr->reserved = 0;
	data_ptr->refresh_mode = AudioEffect_CommunicationQueue_Recv();
	data_ptr->refresh_param_code = 0xFF;   //单一硬件模块或音效的所有详细参数

#ifdef DUAL_CORE_EN
	tx_buf[5]  = tx_buf[5] + 1; //Unit num
#endif

#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
	//双图模式跑单图，数据重新排列
	if(EffectParambin2.context_memory != NULL)
	{
		tx_buf[5]  = tx_buf[5] + 1; //Unit num

	}
	else
	{
		tx_buf[3]  = 1 +  1 + sizeof(roboeffect_0x02_data) + 1 - 7;
		//tx_buf[5]  = 0x05; //Unit num tx_buf[5]不修改
		uint16_t offset = offsetof(roboeffect_0x02_data, robo_mem_info_num_2);
		memmove(&tx_buf[6 + offset], &tx_buf[6 + offset + 7], (1 +  1 + sizeof(roboeffect_0x02_data) + 1));
		//memset(&tx_buf[buf_size - remove_len], 0, remove_len);
	}
#endif

	for(check_ptr = &tx_buf[4]; check_ptr < &tx_buf[4 + tx_buf[3]] - 1; check_ptr++)
	{
		// printf("%02X\n", *check_ptr);
		check_sum += *check_ptr;
	}

	tx_buf[3 + tx_buf[3]] = check_sum;
	tx_buf[4 + tx_buf[3]] = 0x16;

	Communication_Effect_Send(tx_buf, tx_buf[3] + 5);
}

void Comm_PGA0_0x03(uint8_t * buf)
{
	uint16_t TmpData;
	memcpy(&TmpData, &buf[1], 2);

	switch(buf[0])///ADC0 PGA
	{
		case 0:
			gCtrlVars.HwCt.ADC0PGACt.MICL_EN = !!TmpData;
#ifdef CFG_FUNC_ROBOEFFECT_DEBUG_EN
			DBG("ADC0PGACt.MICL_EN = %d\n", gCtrlVars.HwCt.ADC0PGACt.MICL_EN);
#endif
			if(gCtrlVars.HwCt.ADC0PGACt.MICL_EN == 1)
			{
				gCtrlVars.HwCt.ADC0PGACt.LineinL_EN = 0;
				AudioADC_PGASel(ADC0_MODULE, ADC_LEFT, ADC0_MICL);
			}
			else
			{
				if(gCtrlVars.HwCt.ADC0PGACt.LineinL_EN == 1)
				{
					AudioADC_PGASel(ADC0_MODULE, ADC_LEFT, ADC0_LineinL);
				}
				else
				{
					AudioADC_PGASel(ADC0_MODULE, ADC_LEFT, ADC_NONE);
				}
			}
			break;
		case 1:
			gCtrlVars.HwCt.ADC0PGACt.MICR_EN = !!TmpData;
#ifdef CFG_FUNC_ROBOEFFECT_DEBUG_EN
			DBG("ADC0PGACt.MICR_EN = %d\n", gCtrlVars.HwCt.ADC0PGACt.MICR_EN);
#endif
			if(gCtrlVars.HwCt.ADC0PGACt.MICR_EN == 1)
			{
				gCtrlVars.HwCt.ADC0PGACt.LineinR_EN = 0;
				AudioADC_PGASel(ADC0_MODULE, ADC_RIGHT, ADC0_MICR);
			}
			else
			{
				if(gCtrlVars.HwCt.ADC0PGACt.LineinR_EN == 1)
				{
					AudioADC_PGASel(ADC0_MODULE, ADC_LEFT, ADC0_LineinR);
				}
				else
				{
					AudioADC_PGASel(ADC0_MODULE, ADC_RIGHT, ADC_NONE);
				}
			}
			break;
		case 2:
			gCtrlVars.HwCt.ADC0PGACt.LineinL_EN = !!TmpData;
#ifdef CFG_FUNC_ROBOEFFECT_DEBUG_EN
			DBG("ADC0PGACt.LineinL_EN = %d\n", gCtrlVars.HwCt.ADC0PGACt.LineinL_EN);
#endif
			if(gCtrlVars.HwCt.ADC0PGACt.LineinL_EN == 1)
			{
				gCtrlVars.HwCt.ADC0PGACt.MICL_EN = 0;
				AudioADC_PGASel(ADC0_MODULE, ADC_LEFT, ADC0_LineinL);
			}
			else
			{
				if(gCtrlVars.HwCt.ADC0PGACt.MICL_EN == 1)
				{
					AudioADC_PGASel(ADC0_MODULE, ADC_LEFT, ADC0_MICL);
				}
				else
				{
					AudioADC_PGASel(ADC0_MODULE, ADC_LEFT, ADC_NONE);
				}
			}
			break;
		case 3:
			gCtrlVars.HwCt.ADC0PGACt.LineinR_EN = !!TmpData;
#ifdef CFG_FUNC_ROBOEFFECT_DEBUG_EN
			DBG("ADC0PGACt.LineinR_EN = %d\n", gCtrlVars.HwCt.ADC0PGACt.LineinR_EN);
#endif
			if(gCtrlVars.HwCt.ADC0PGACt.LineinR_EN == 1)
			{
				gCtrlVars.HwCt.ADC0PGACt.MICR_EN = 0;
				AudioADC_PGASel(ADC0_MODULE, ADC_RIGHT, ADC0_LineinR);
			}
			else
			{
				if(gCtrlVars.HwCt.ADC0PGACt.MICR_EN == 1)
				{
					AudioADC_PGASel(ADC0_MODULE, ADC_LEFT, ADC0_MICR);
				}
				else
				{
					AudioADC_PGASel(ADC0_MODULE, ADC_RIGHT, ADC_NONE);
				}
			}
			break;
		case 4://MICL_Mode
#if 0  //单端/差分不不许动态更改
			if(gCtrlVars.HwCt.ADC0PGACt.MICL_EN == 1)
			{
				gCtrlVars.HwCt.ADC0PGACt.MICL_Mode = TmpData;
				AudioADC_PGAMode(ADC0_MODULE, ADC_LEFT, gCtrlVars.HwCt.ADC0PGACt.MICL_Mode);
			}
#endif
			break;
		case 5://MICR_Mode
#if 0
			if(gCtrlVars.HwCt.ADC0PGACt.MICR_EN == 1)
			{
				gCtrlVars.HwCt.ADC0PGACt.MICR_Mode = TmpData;
				AudioADC_PGAMode(ADC0_MODULE, ADC_LEFT, gCtrlVars.HwCt.ADC0PGACt.MICR_Mode);
			}
#endif
			break;
		case 6: //MICL_SE_Gain
			if(gCtrlVars.HwCt.ADC0PGACt.MICL_EN == 1)
			{
				if(gCtrlVars.HwCt.ADC0PGACt.MICL_Mode == 0)
				{
					gCtrlVars.HwCt.ADC0PGACt.MICL_Single_Gain = TmpData > 31? 31 : TmpData;
					AudioADC_PGAGainSet(ADC0_MODULE, ADC_LEFT, ADC0_MICL, 31 - gCtrlVars.HwCt.ADC0PGACt.MICL_Single_Gain);
				}
			}
			break;
		case 7: //MICR_SE_Gain
			if(gCtrlVars.HwCt.ADC0PGACt.MICR_EN == 1)
			{
				if(gCtrlVars.HwCt.ADC0PGACt.MICR_Mode == 0)
				{
					gCtrlVars.HwCt.ADC0PGACt.MICR_Single_Gain = TmpData > 31? 31 : TmpData;
					AudioADC_PGAGainSet(ADC0_MODULE, ADC_RIGHT, ADC0_MICR, 31 - gCtrlVars.HwCt.ADC0PGACt.MICR_Single_Gain);
				}
			}
			break;
		case 8: //MICL_DF_Gain
			if(gCtrlVars.HwCt.ADC0PGACt.MICL_EN == 1)
			{
				if(gCtrlVars.HwCt.ADC0PGACt.MICL_Mode == 1)
				{
					gCtrlVars.HwCt.ADC0PGACt.MICL_DIFF_Gain = TmpData > 31? 31 : TmpData;
					AudioADC_PGAGainSet(ADC0_MODULE, ADC_LEFT, ADC0_MICL, 31 - gCtrlVars.HwCt.ADC0PGACt.MICL_DIFF_Gain);
				}
			}
			break;
		case 9: //MICR_DF_Gain
			if(gCtrlVars.HwCt.ADC0PGACt.MICR_EN == 1)
			{
				if(gCtrlVars.HwCt.ADC0PGACt.MICR_Mode == 1)
				{
					gCtrlVars.HwCt.ADC0PGACt.MICR_DIFF_Gain = TmpData > 31? 31 : TmpData;
					AudioADC_PGAGainSet(ADC0_MODULE, ADC_RIGHT, ADC0_MICR, 31 - gCtrlVars.HwCt.ADC0PGACt.MICR_DIFF_Gain);
				}
			}
			break;
		case 10: //LineinL_Gain
			if(gCtrlVars.HwCt.ADC0PGACt.LineinL_EN == 1)
			{
				gCtrlVars.HwCt.ADC0PGACt.LineinL_Gain = TmpData > 31? 31 : TmpData;
				AudioADC_PGAGainSet(ADC0_MODULE, ADC_LEFT, ADC0_LineinL, 31 - gCtrlVars.HwCt.ADC0PGACt.LineinL_Gain);
			}
			break;
		case 11: //LineinR_Gain
			if(gCtrlVars.HwCt.ADC0PGACt.LineinR_EN == 1)
			{
				gCtrlVars.HwCt.ADC0PGACt.LineinR_Gain = TmpData > 31? 31 : TmpData;
				AudioADC_PGAGainSet(ADC0_MODULE, ADC_RIGHT, ADC0_LineinR, 31 - gCtrlVars.HwCt.ADC0PGACt.LineinR_Gain);
			}
			break;
		default:
			break;
	}
}

void Communication_Effect_0x03(uint8_t *buf, uint32_t len)////ADC0 PGA
{
	uint16_t i,k;
	if(len == 0) //ask
	{
		memset(tx_buf, 0, sizeof(tx_buf));
		tx_buf[0]   = 0xa5;
		tx_buf[1]   = 0x5a;
		tx_buf[2]   = 0x03;//cmd
		tx_buf[3]   = 1+12*2;///1 + len * sizeof(int16)
		tx_buf[4]   = 0xff;///all paramgs,

		memcpy(&tx_buf[5],  &gCtrlVars.HwCt.ADC0PGACt.MICL_EN, 2);
		memcpy(&tx_buf[7],  &gCtrlVars.HwCt.ADC0PGACt.MICR_EN, 2);
		memcpy(&tx_buf[9],  &gCtrlVars.HwCt.ADC0PGACt.LineinL_EN, 2);
		memcpy(&tx_buf[11], &gCtrlVars.HwCt.ADC0PGACt.LineinR_EN, 2);
		memcpy(&tx_buf[13], &gCtrlVars.HwCt.ADC0PGACt.MICL_Mode, 2);
		memcpy(&tx_buf[15], &gCtrlVars.HwCt.ADC0PGACt.MICR_Mode, 2);
		memcpy(&tx_buf[17],  &gCtrlVars.HwCt.ADC0PGACt.MICL_Single_Gain, 2);
		memcpy(&tx_buf[19],  &gCtrlVars.HwCt.ADC0PGACt.MICR_Single_Gain, 2);
		memcpy(&tx_buf[21],  &gCtrlVars.HwCt.ADC0PGACt.MICL_DIFF_Gain, 2);
		memcpy(&tx_buf[23], &gCtrlVars.HwCt.ADC0PGACt.MICR_DIFF_Gain, 2);
		memcpy(&tx_buf[25], &gCtrlVars.HwCt.ADC0PGACt.LineinL_Gain, 2);
		memcpy(&tx_buf[27], &gCtrlVars.HwCt.ADC0PGACt.LineinR_Gain, 2);
		tx_buf[5 + 12*2]  = 0x16;
		Communication_Effect_Send(tx_buf, 6 + 12*2);
	}
	else  //set
	{
		switch(buf[0])///ADC0 PGA
		{
			case 0xff:
				buf++;
				for(i = 0; i < 19; i++)
				{
					cbuf[0] = i;////id
					for(k = 0; k < CTL_DATA_SIZE; k++)
					{
						cbuf[ k + 1] = (uint8_t)buf[k];
					}
					Comm_PGA0_0x03(&cbuf[0]);
					buf += 2;
				}
				break;
			default:
				Comm_PGA0_0x03(buf);
				break;
		}
	}
}

void Comm_ADC0_0x04(uint8_t * buf)
{
	uint16_t TmpData;
//	bool LeftEnable;
//	bool RightEnable;
//	bool LeftMute;
//	bool RightMute;

	memcpy(&TmpData, &buf[1], 2);
	switch(buf[0])///adc0 digital
	{
		case 0: //Enable
			gCtrlVars.HwCt.ADC0DigitalCt.Enable = TmpData;
#ifdef CFG_FUNC_ROBOEFFECT_DEBUG_EN
			DBG("ADC0DigitalCt.Enable = %d\n", gCtrlVars.HwCt.ADC0DigitalCt.Enable);
#endif
			if(TmpData == 0)
			{
				AudioADC_LREnable(ADC0_MODULE, 0, 0);
				AudioADC_Disable(ADC0_MODULE);
			}
			else if(TmpData == 1)
			{
				AudioADC_LREnable(ADC0_MODULE, 1, 0);
				AudioADC_Enable(ADC0_MODULE);
			}
			else if(TmpData == 2)
			{
				AudioADC_LREnable(ADC0_MODULE, 0, 1);
				AudioADC_Enable(ADC0_MODULE);
			}
			else if(TmpData == 3)
			{
				AudioADC_LREnable(ADC0_MODULE, 1, 1);
				AudioADC_Enable(ADC0_MODULE);
			}
			break;
		case 1: //Sample_Rate
			break;
		case 2: //CLK_Source;
			break;
		case 3: //Channel_Swap
			gCtrlVars.HwCt.ADC0DigitalCt.Channel_Swap = !!TmpData;
			AudioADC_ChannelSwap(ADC0_MODULE, gCtrlVars.HwCt.ADC0DigitalCt.Channel_Swap);
			break;
		case 4: //DC_Block_Enable
			gCtrlVars.HwCt.ADC0DigitalCt.DC_Block_Enable = !!TmpData;
			AudioADC_HighPassFilterSet(ADC0_MODULE, gCtrlVars.HwCt.ADC0DigitalCt.DC_Block_Enable);
			break;
		case 5: //L_Volume
			gCtrlVars.HwCt.ADC0DigitalCt.L_Volume = TmpData;
			AudioADC_VolSetChannel_For_dB(ADC0_MODULE, ADC_LEFT, (int16_t)gCtrlVars.HwCt.ADC0DigitalCt.L_Volume);
			break;
		case 6: //R_Volume
			gCtrlVars.HwCt.ADC0DigitalCt.R_Volume = TmpData;
			AudioADC_VolSetChannel_For_dB(ADC0_MODULE, ADC_RIGHT, (int16_t)gCtrlVars.HwCt.ADC0DigitalCt.R_Volume);
			break;
		case 7: //L_Mute
			gCtrlVars.HwCt.ADC0DigitalCt.L_Mute = !!TmpData;
			AudioADC_DigitalMute(ADC0_MODULE, gCtrlVars.HwCt.ADC0DigitalCt.L_Mute, gCtrlVars.HwCt.ADC0DigitalCt.R_Mute);
			break;
		case 8: //R_Mute
			gCtrlVars.HwCt.ADC0DigitalCt.R_Mute = !!TmpData;
			AudioADC_DigitalMute(ADC0_MODULE, gCtrlVars.HwCt.ADC0DigitalCt.L_Mute, gCtrlVars.HwCt.ADC0DigitalCt.R_Mute);
		    break;
		 default:
		   break;
	}
}

void Communication_Effect_0x04(uint8_t *buf, uint32_t len)////ADC0 DIGITAL
{
	uint16_t i,k;

	if(len == 0) //ask
	{
		memset(tx_buf, 0, sizeof(tx_buf));
		tx_buf[0]  = 0xa5;
		tx_buf[1]  = 0x5a;
		tx_buf[2]  = 0x04;
		tx_buf[3]  = 1+9*2;///1 + len * sizeof(int16)
		tx_buf[4]  = 0xff;
		memcpy(&tx_buf[5], &gCtrlVars.HwCt.ADC0DigitalCt.Enable, 2);
		memcpy(&tx_buf[7], &gCtrlVars.HwCt.ADC0DigitalCt.Sample_Rate, 2);
		memcpy(&tx_buf[9], &gCtrlVars.HwCt.ADC0DigitalCt.CLK_Source, 2);
		memcpy(&tx_buf[11], &gCtrlVars.HwCt.ADC0DigitalCt.Channel_Swap, 2);
		//memcpy(&tx_buf[13], &gCtrlVars.sample_rate_index, 2);//定义一个全局结构体，用于上传PC
		memcpy(&tx_buf[13], &gCtrlVars.HwCt.ADC0DigitalCt.DC_Block_Enable, 2);
		memcpy(&tx_buf[15], &gCtrlVars.HwCt.ADC0DigitalCt.L_Volume, 2);
		memcpy(&tx_buf[17], &gCtrlVars.HwCt.ADC0DigitalCt.R_Volume, 2);
		memcpy(&tx_buf[19], &gCtrlVars.HwCt.ADC0DigitalCt.L_Mute, 2);
		memcpy(&tx_buf[21], &gCtrlVars.HwCt.ADC0DigitalCt.R_Mute, 2);////adc0 hpc en
		tx_buf[23] = 0x16;
		Communication_Effect_Send(tx_buf, 24);
	}
	else
	{
		switch(buf[0])///ADC0 PGA
		{
			case 0xff:
				buf++;
				for(i = 0; i < 10; i++)
				{
					cbuf[0] = i;////id
					for(k = 0; k < CTL_DATA_SIZE; k++)
					{
						cbuf[ k + 1] = (uint8_t)buf[k];
					}
					Comm_ADC0_0x04(&cbuf[0]);
					buf += 2;
				}
				break;
			default:
				Comm_ADC0_0x04(buf);
				break;
		}
	}
}

//下面参数实际配置不执行操作
void Comm_AGC0_0x05(uint8_t * buf) //ADC0 DIGITAL
{
	uint16_t TmpData;

	memcpy(&TmpData, &buf[1], 2);
	switch(buf[0])
	{
		case 0:
			gCtrlVars.HwCt.ADC0AGCCt.AGC_EN = TmpData;
			if(TmpData == 0)
			{
				AudioADC_AGCChannelSel(ADC0_MODULE, 0, 0);
			}
			else if(TmpData == 1)
			{
				AudioADC_AGCChannelSel(ADC0_MODULE, 1, 0);
			}
			else if(TmpData == 2)
			{
				AudioADC_AGCChannelSel(ADC0_MODULE, 0, 1);
			}
			else if(TmpData == 3)
			{
				AudioADC_AGCChannelSel(ADC0_MODULE, 1, 1);
			}
			break;
		case 1:
			gCtrlVars.HwCt.ADC0AGCCt.Max_Level = TmpData;
			AudioADC_AGCMaxLevel(ADC0_MODULE, (uint8_t)TmpData);
			break;
		case 2: //Target_Level
			gCtrlVars.HwCt.ADC0AGCCt.Target_Level = TmpData;
			AudioADC_AGCTargetLevel(ADC0_MODULE, (uint8_t)TmpData);
			break;
		case 3: //Max_Gain
			gCtrlVars.HwCt.ADC0AGCCt.Max_Gain = TmpData;
			AudioADC_AGCMaxGain(ADC0_MODULE, (uint8_t)TmpData);
			break;
		case 4: //Min_Gain
			gCtrlVars.HwCt.ADC0AGCCt.Min_Gain = TmpData;
			AudioADC_AGCMinGain(ADC0_MODULE, (uint8_t)TmpData);
			break;
		case 5: //Gain_Offset
			gCtrlVars.HwCt.ADC0AGCCt.Gain_Offset = TmpData;
			AudioADC_AGCGainOffset(ADC0_MODULE, (uint8_t)TmpData);
			break;
		case 6://Frame_Time
			gCtrlVars.HwCt.ADC0AGCCt.Frame_Time = TmpData;
			AudioADC_AGCFrameTime(ADC0_MODULE, TmpData);
			break;
		case 7://Hold_N_Frame
			gCtrlVars.HwCt.ADC0AGCCt.Hold_N_Frame = TmpData;
			AudioADC_AGCHoldFrames(ADC0_MODULE, (uint32_t)TmpData);
			break;
		case 8: //Attack_Time
			gCtrlVars.HwCt.ADC0AGCCt.Attack_Time = TmpData;
			AudioADC_AGCAttackStepTime(ADC0_MODULE, TmpData);
			break;
		case 9: //Decay_Time
			gCtrlVars.HwCt.ADC0AGCCt.Decay_Time = TmpData;
			AudioADC_AGCDecayStepTime(ADC0_MODULE, TmpData);
			break;
		case 10: //NG_Threshold
			gCtrlVars.HwCt.ADC0AGCCt.NG_Threshold = TmpData;
			AudioADC_AGCNoiseThreshold(ADC0_MODULE, (uint8_t)TmpData);
			break;
		case 11: //NG_Rref
			gCtrlVars.HwCt.ADC0AGCCt.NG_Rref = TmpData;
			break;
		case 12: //NG_Hold_N_Frame
			gCtrlVars.HwCt.ADC0AGCCt.NG_Hold_N_Frame = TmpData;
			AudioADC_AGCNoiseHoldFrames(ADC0_MODULE, (uint8_t)TmpData);
			break;
		default:
			break;
	}
}

void Communication_Effect_0x05(uint8_t *buf, uint32_t len) //ADC0 DIGITAL
{
	uint16_t i,k;

	if(len == 0) //ask
	{
		memset(tx_buf, 0, sizeof(tx_buf));
		tx_buf[0]  = 0xa5;
		tx_buf[1]  = 0x5a;
		tx_buf[2]  = 0x05;
		tx_buf[3]  = 1+13*2;///1 + len * sizeof(int16)
		tx_buf[4]  = 0xff;
		memcpy(&tx_buf[5], &gCtrlVars.HwCt.ADC0AGCCt.AGC_EN, 2);
		memcpy(&tx_buf[7], &gCtrlVars.HwCt.ADC0AGCCt.Max_Level, 2);
		memcpy(&tx_buf[9], &gCtrlVars.HwCt.ADC0AGCCt.Target_Level, 2);
		memcpy(&tx_buf[11], &gCtrlVars.HwCt.ADC0AGCCt.Max_Gain, 2);
		//memcpy(&tx_buf[13], &gCtrlVars.sample_rate_index, 2);//定义一个全局结构体，用于上传PC
		memcpy(&tx_buf[13], &gCtrlVars.HwCt.ADC0AGCCt.Min_Gain, 2);
		memcpy(&tx_buf[15], &gCtrlVars.HwCt.ADC0AGCCt.Gain_Offset, 2);
		memcpy(&tx_buf[17], &gCtrlVars.HwCt.ADC0AGCCt.Frame_Time, 2);
		memcpy(&tx_buf[19], &gCtrlVars.HwCt.ADC0AGCCt.Hold_N_Frame, 2);
		memcpy(&tx_buf[21], &gCtrlVars.HwCt.ADC0AGCCt.Attack_Time, 2);
		memcpy(&tx_buf[23], &gCtrlVars.HwCt.ADC0AGCCt.Decay_Time, 2);
		memcpy(&tx_buf[25], &gCtrlVars.HwCt.ADC0AGCCt.NG_Threshold, 2);
		memcpy(&tx_buf[27], &gCtrlVars.HwCt.ADC0AGCCt.NG_Rref, 2);
		memcpy(&tx_buf[29], &gCtrlVars.HwCt.ADC0AGCCt.NG_Hold_N_Frame, 2);
		tx_buf[31] = 0x16;
		Communication_Effect_Send(tx_buf, 32);
	}
	else
	{
		switch(buf[0])///ADC0 PGA
		{
			case 0xff:
				buf++;
				for(i = 0; i < 13; i++)
				{
					cbuf[0] = i;////id
					for(k = 0; k < CTL_DATA_SIZE; k++)
					{
						cbuf[ k + 1] = (uint8_t)buf[k];
					}
					Comm_AGC0_0x05(&cbuf[0]);
					buf += 2;
				}
				break;
			default:
				Comm_AGC0_0x05(buf);
				break;
		}
	}
}

void Comm_PGA1_0x06(uint8_t * buf)   //ADC1 PGA
{
	uint16_t TmpData;

	memcpy(&TmpData, &buf[1], 2);
	switch(buf[0])
	{
		case 0:
			gCtrlVars.HwCt.ADC1PGACt.MICL_EN = !!TmpData;
			if(gCtrlVars.HwCt.ADC1PGACt.MICL_EN == 1)
			{
				gCtrlVars.HwCt.ADC1PGACt.AUX0L_B20_EN = 0;
				gCtrlVars.HwCt.ADC1PGACt.AUX1L_B18_EN = 0;
				AudioADC_PGASel(ADC1_MODULE, ADC_LEFT, ADC1_MICL);
			}
			else
			{
				if(gCtrlVars.HwCt.ADC1PGACt.AUX0L_B20_EN == 1)
				{
					AudioADC_PGASel(ADC1_MODULE, ADC_LEFT, ADC1_AUX0L_B20);
				}
				else if(gCtrlVars.HwCt.ADC1PGACt.AUX1L_B18_EN == 1)
				{
					AudioADC_PGASel(ADC1_MODULE, ADC_LEFT, ADC1_AUX1L_B18);
				}
				else
				{
					AudioADC_PGASel(ADC1_MODULE, ADC_LEFT, ADC_NONE);
				}
			}
			break;
		case 1:
			gCtrlVars.HwCt.ADC1PGACt.MICR_EN = !!TmpData;
			if(gCtrlVars.HwCt.ADC1PGACt.MICR_EN == 1)
			{
				gCtrlVars.HwCt.ADC1PGACt.AUX0R_B21_EN = 0;
				gCtrlVars.HwCt.ADC1PGACt.AUX1R_B19_EN = 0;
				AudioADC_PGASel(ADC1_MODULE, ADC_RIGHT, ADC1_MICR);
			}
			else
			{
				if(gCtrlVars.HwCt.ADC1PGACt.AUX0R_B21_EN == 1)
				{
					AudioADC_PGASel(ADC1_MODULE, ADC_RIGHT, ADC1_AUX0R_B21);
				}
				else if(gCtrlVars.HwCt.ADC1PGACt.AUX1R_B19_EN == 1)
				{
					AudioADC_PGASel(ADC1_MODULE, ADC_RIGHT, ADC1_AUX1R_B19);
				}
				else
				{
					AudioADC_PGASel(ADC1_MODULE, ADC_RIGHT, ADC_NONE);
				}
			}
			break;
		case 2: //LineinL_B20_EN
			gCtrlVars.HwCt.ADC1PGACt.AUX0L_B20_EN = !!TmpData;
			if(gCtrlVars.HwCt.ADC1PGACt.AUX0L_B20_EN == 1)
			{
				gCtrlVars.HwCt.ADC1PGACt.MICL_EN = 0;
				AudioADC_PGASel(ADC1_MODULE, ADC_LEFT, ADC1_AUX0L_B20);
			}
			else
			{
				if(gCtrlVars.HwCt.ADC1PGACt.MICL_EN == 1)
				{
					AudioADC_PGASel(ADC1_MODULE, ADC_LEFT, ADC1_MICL);
				}
				else if(gCtrlVars.HwCt.ADC1PGACt.AUX1L_B18_EN == 1)
				{
					AudioADC_PGASel(ADC1_MODULE, ADC_LEFT, ADC1_AUX1L_B18);
				}
				else
				{
					AudioADC_PGASel(ADC1_MODULE, ADC_LEFT, ADC_NONE);
				}
			}
			break;
		case 3: //LineinR_B21_EN
			gCtrlVars.HwCt.ADC1PGACt.AUX0R_B21_EN = !!TmpData;
			if(gCtrlVars.HwCt.ADC1PGACt.AUX0R_B21_EN == 1)
			{
				gCtrlVars.HwCt.ADC1PGACt.MICR_EN = 0;
				AudioADC_PGASel(ADC1_MODULE, ADC_RIGHT, ADC1_AUX0R_B21);
			}
			else
			{
				if(gCtrlVars.HwCt.ADC1PGACt.MICR_EN == 1)
				{
					AudioADC_PGASel(ADC1_MODULE, ADC_RIGHT, ADC1_MICR);
				}
				else if(gCtrlVars.HwCt.ADC1PGACt.AUX1R_B19_EN == 1)
				{
					AudioADC_PGASel(ADC1_MODULE, ADC_RIGHT, ADC1_AUX1R_B19);
				}
				else
				{
					AudioADC_PGASel(ADC1_MODULE, ADC_RIGHT, ADC_NONE);
				}
			}
			break;
		case 4: //LineinL_B18_EN
			gCtrlVars.HwCt.ADC1PGACt.AUX1L_B18_EN = !!TmpData;
			if(gCtrlVars.HwCt.ADC1PGACt.AUX1L_B18_EN == 1)
			{
				gCtrlVars.HwCt.ADC1PGACt.MICL_EN = 0;
				AudioADC_PGASel(ADC1_MODULE, ADC_LEFT, ADC1_AUX1L_B18);
			}
			else
			{
				if(gCtrlVars.HwCt.ADC1PGACt.MICL_EN == 1)
				{
					AudioADC_PGASel(ADC1_MODULE, ADC_LEFT, ADC1_MICL);
				}
				else if(gCtrlVars.HwCt.ADC1PGACt.AUX0L_B20_EN == 1)
				{
					AudioADC_PGASel(ADC1_MODULE, ADC_LEFT, ADC1_AUX0L_B20);
				}
				else
				{
					AudioADC_PGASel(ADC1_MODULE, ADC_LEFT, ADC_NONE);
				}
			}
			break;
		case 5: //LineinR_B19_EN
			gCtrlVars.HwCt.ADC1PGACt.AUX1R_B19_EN = !!TmpData;
			if(gCtrlVars.HwCt.ADC1PGACt.AUX1R_B19_EN == 1)
			{
				gCtrlVars.HwCt.ADC1PGACt.MICR_EN = 0;
				AudioADC_PGASel(ADC1_MODULE, ADC_RIGHT, ADC1_AUX1R_B19);
			}
			else
			{
				if(gCtrlVars.HwCt.ADC1PGACt.MICR_EN == 1)
				{
					AudioADC_PGASel(ADC1_MODULE, ADC_RIGHT, ADC1_MICR);
				}
				else if(gCtrlVars.HwCt.ADC1PGACt.AUX0R_B21_EN == 1)
				{
					AudioADC_PGASel(ADC1_MODULE, ADC_RIGHT, ADC1_AUX0R_B21);
				}
				else
				{
					AudioADC_PGASel(ADC1_MODULE, ADC_RIGHT, ADC_NONE);
				}
			}
			break;
		case 6://MICL_Mode
#if 0
			if(gCtrlVars.HwCt.ADC1PGACt.MICL_EN == 1)
			{
				gCtrlVars.HwCt.ADC1PGACt.MICL_Mode = TmpData;
				AudioADC_PGAMode(ADC1_MODULE, ADC_LEFT, gCtrlVars.HwCt.ADC1PGACt.MICL_Mode);
			}
#endif
			break;
		case 7://MICR_Mode
#if 0
			if(gCtrlVars.HwCt.ADC1PGACt.MICR_EN == 1)
			{
				gCtrlVars.HwCt.ADC1PGACt.MICR_Mode = TmpData;
				AudioADC_PGAMode(ADC1_MODULE, ADC_LEFT, gCtrlVars.HwCt.ADC1PGACt.MICR_Mode);
			}
#endif
			break;
		case 8: //MICL_SE_Gain
			if(gCtrlVars.HwCt.ADC1PGACt.MICL_EN == 1)
			{
				if(gCtrlVars.HwCt.ADC1PGACt.MICL_Mode == 0)
				{
					gCtrlVars.HwCt.ADC1PGACt.MICL_Single_Gain = TmpData > 31? 31 : TmpData;
					AudioADC_PGAGainSet(ADC1_MODULE, ADC_LEFT, ADC1_MICL, 31 - gCtrlVars.HwCt.ADC1PGACt.MICL_Single_Gain);
				}
			}
			break;
		case 9: //MICR_SE_Gain
			if(gCtrlVars.HwCt.ADC1PGACt.MICR_EN == 1)
			{
				if(gCtrlVars.HwCt.ADC1PGACt.MICR_Mode == 0)
				{
					gCtrlVars.HwCt.ADC1PGACt.MICR_Single_Gain = TmpData > 31? 31 : TmpData;
					AudioADC_PGAGainSet(ADC1_MODULE, ADC_RIGHT, ADC1_MICR, 31 - gCtrlVars.HwCt.ADC1PGACt.MICR_Single_Gain);
				}
			}
			break;
		case 10: //MICL_DF_Gain
			if(gCtrlVars.HwCt.ADC1PGACt.MICL_EN == 1)
			{
				if(gCtrlVars.HwCt.ADC1PGACt.MICL_Mode == 1)
				{
					gCtrlVars.HwCt.ADC1PGACt.MICL_DIFF_Gain = TmpData > 31? 31 : TmpData;
					AudioADC_PGAGainSet(ADC1_MODULE, ADC_LEFT, ADC1_MICL, 31 - gCtrlVars.HwCt.ADC1PGACt.MICL_DIFF_Gain);
				}
			}
			break;
		case 11: //MICR_DF_Gain
			if(gCtrlVars.HwCt.ADC1PGACt.MICR_EN == 1)
			{
				if(gCtrlVars.HwCt.ADC1PGACt.MICR_Mode == 1)
				{
					gCtrlVars.HwCt.ADC1PGACt.MICR_DIFF_Gain = TmpData > 31? 31 : TmpData;
					AudioADC_PGAGainSet(ADC1_MODULE, ADC_RIGHT, ADC1_MICR, 31 - gCtrlVars.HwCt.ADC1PGACt.MICR_DIFF_Gain);
				}
			}
			break;
		case 12: //Linein_B20_Gain
			if(gCtrlVars.HwCt.ADC1PGACt.AUX0L_B20_EN == 1)
			{
				gCtrlVars.HwCt.ADC1PGACt.AUX0L_B20_Gain = TmpData > 31? 31 : TmpData;
				AudioADC_PGAGainSet(ADC1_MODULE, ADC_LEFT, ADC1_AUX0L_B20, 31 - gCtrlVars.HwCt.ADC1PGACt.AUX0L_B20_Gain);
			}
			break;
		case 13: //Linein_B21_Gain
			if(gCtrlVars.HwCt.ADC1PGACt.AUX0R_B21_EN == 1)
			{
				gCtrlVars.HwCt.ADC1PGACt.AUX0R_B21_Gain = TmpData > 31? 31 : TmpData;
				AudioADC_PGAGainSet(ADC1_MODULE, ADC_RIGHT, ADC1_AUX0R_B21, 31 - gCtrlVars.HwCt.ADC1PGACt.AUX0R_B21_Gain);
			}
			break;
		case 14: //Linein_B18_Gain
			if(gCtrlVars.HwCt.ADC1PGACt.AUX1L_B18_EN == 1)
			{
				gCtrlVars.HwCt.ADC1PGACt.AUX1L_B18_Gain = TmpData > 31? 31 : TmpData;
				AudioADC_PGAGainSet(ADC1_MODULE, ADC_LEFT, ADC1_AUX1L_B18, 31 - gCtrlVars.HwCt.ADC1PGACt.AUX1L_B18_Gain);
			}
			break;
		case 15: //Linein_B19_Gain
			if(gCtrlVars.HwCt.ADC1PGACt.AUX1R_B19_EN == 1)
			{
				gCtrlVars.HwCt.ADC1PGACt.AUX1R_B19_Gain = TmpData > 31? 31 : TmpData;
				AudioADC_PGAGainSet(ADC1_MODULE, ADC_RIGHT, ADC1_AUX1R_B19, 31 - gCtrlVars.HwCt.ADC1PGACt.AUX1R_B19_Gain);
			}
			break;
		default:
			break;
	}
}

void Communication_Effect_0x06(uint8_t *buf, uint32_t len) //ADC1 PGA
{
	uint16_t i,k;

	if(len == 0) //ask
	{
		memset(tx_buf, 0, sizeof(tx_buf));
		tx_buf[0]  = 0xa5;
		tx_buf[1]  = 0x5a;
		tx_buf[2]  = 0x06;//cmd
		tx_buf[3]  = 1 + 16*2;///1 + len * sizeof(int16)
		tx_buf[4]  = 0xff;///all paramgs
		memcpy(&tx_buf[5], &gCtrlVars.HwCt.ADC1PGACt.MICL_EN, 2);
		memcpy(&tx_buf[7], &gCtrlVars.HwCt.ADC1PGACt.MICR_EN, 2);
		memcpy(&tx_buf[9], &gCtrlVars.HwCt.ADC1PGACt.AUX0L_B20_EN, 2);
		memcpy(&tx_buf[11], &gCtrlVars.HwCt.ADC1PGACt.AUX0R_B21_EN, 2);
		memcpy(&tx_buf[13], &gCtrlVars.HwCt.ADC1PGACt.AUX1L_B18_EN, 2);
		memcpy(&tx_buf[15], &gCtrlVars.HwCt.ADC1PGACt.AUX1R_B19_EN, 2);
		memcpy(&tx_buf[17], &gCtrlVars.HwCt.ADC1PGACt.MICL_Mode, 2);
		memcpy(&tx_buf[19], &gCtrlVars.HwCt.ADC1PGACt.MICR_Mode, 2);
		memcpy(&tx_buf[21], &gCtrlVars.HwCt.ADC1PGACt.MICL_Single_Gain, 2);
		memcpy(&tx_buf[23], &gCtrlVars.HwCt.ADC1PGACt.MICR_Single_Gain, 2);
		memcpy(&tx_buf[25], &gCtrlVars.HwCt.ADC1PGACt.MICL_DIFF_Gain, 2);
		memcpy(&tx_buf[27], &gCtrlVars.HwCt.ADC1PGACt.MICR_DIFF_Gain, 2);
		memcpy(&tx_buf[29], &gCtrlVars.HwCt.ADC1PGACt.AUX0L_B20_Gain, 2);
		memcpy(&tx_buf[31], &gCtrlVars.HwCt.ADC1PGACt.AUX0R_B21_Gain, 2);
		memcpy(&tx_buf[33], &gCtrlVars.HwCt.ADC1PGACt.AUX1L_B18_Gain, 2);
		memcpy(&tx_buf[35], &gCtrlVars.HwCt.ADC1PGACt.AUX1R_B19_Gain, 2);
		tx_buf[5 + 16*2]  = 0x16;
		Communication_Effect_Send(tx_buf, 6 + 16*2);
	}
	else
	{
		switch(buf[0])///adc1 digital set
		{
			case 0xff:
				buf++;
				for(i = 0; i < 16; i++)
				{
					cbuf[0] = i;////id
					for(k = 0; k < CTL_DATA_SIZE; k++)
					{
						cbuf[ k + 1] = (uint8_t)buf[k];
					}
					Comm_PGA1_0x06(&cbuf[0]);
					buf += 2;
				}
				break;
			default:
				Comm_PGA1_0x06(buf);
				break;
		}
	}
}

void Comm_ADC1_0x07(uint8_t * buf)
{
	uint16_t TmpData;
//	bool LeftEnable;
//	bool RightEnable;
//	bool LeftMute;
//	bool RightMute;

	memcpy(&TmpData, &buf[1], 2);
	switch(buf[0])///adc1 digital channel en
	{
		case 0: //ADC1 en
			gCtrlVars.HwCt.ADC1DigitalCt.Enable = TmpData;
			if(TmpData == 0)
			{
				AudioADC_LREnable(ADC1_MODULE, 0, 0);
				AudioADC_Disable(ADC1_MODULE);
			}
			else if(TmpData == 1)
			{
				AudioADC_LREnable(ADC1_MODULE, 1, 0);
				AudioADC_Enable(ADC1_MODULE);
			}
			else if(TmpData == 2)
			{
				AudioADC_LREnable(ADC1_MODULE, 0, 1);
				AudioADC_Enable(ADC1_MODULE);
			}
			else if(TmpData == 3)
			{
				AudioADC_LREnable(ADC1_MODULE, 1, 1);
				AudioADC_Enable(ADC1_MODULE);
			}
			break;
		case 1: //Sample_Rate
			break;
		case 2: //CLK_Source
			break;
		case 3: //Channel_Swap
			gCtrlVars.HwCt.ADC1DigitalCt.Channel_Swap = !!TmpData;
			AudioADC_ChannelSwap(ADC1_MODULE, gCtrlVars.HwCt.ADC1DigitalCt.Channel_Swap);
			break;
		case 4: //DC_Block_Enable
			gCtrlVars.HwCt.ADC1DigitalCt.DC_Block_Enable = !!TmpData;
			AudioADC_HighPassFilterSet(ADC1_MODULE, gCtrlVars.HwCt.ADC1DigitalCt.DC_Block_Enable);
			break;
		case 5: //L_Volume
			gCtrlVars.HwCt.ADC1DigitalCt.L_Volume = TmpData;
			AudioADC_VolSetChannel_For_dB(ADC1_MODULE, ADC_LEFT, (int16_t)gCtrlVars.HwCt.ADC1DigitalCt.L_Volume);
			break;
		case 6: //R_Volume
			gCtrlVars.HwCt.ADC1DigitalCt.R_Volume = TmpData;
			AudioADC_VolSetChannel_For_dB(ADC1_MODULE, ADC_RIGHT, (int16_t)gCtrlVars.HwCt.ADC1DigitalCt.R_Volume);
			break;
		case 7: //L_Mute
			gCtrlVars.HwCt.ADC1DigitalCt.L_Mute = !!TmpData;
			AudioADC_DigitalMute(ADC1_MODULE, gCtrlVars.HwCt.ADC1DigitalCt.L_Mute, gCtrlVars.HwCt.ADC1DigitalCt.R_Mute);
			break;
		case 8: //R_Mute
			gCtrlVars.HwCt.ADC1DigitalCt.R_Mute = !!TmpData;
			AudioADC_DigitalMute(ADC1_MODULE, gCtrlVars.HwCt.ADC1DigitalCt.L_Mute, gCtrlVars.HwCt.ADC1DigitalCt.R_Mute);
		    break;
		 default:
		   break;
	}
}

void Communication_Effect_0x07(uint8_t *buf, uint32_t len)///ADC1 DIGITAL
{
	uint16_t i,k;

	if(len == 0) //ask
	{
		memset(tx_buf, 0, sizeof(tx_buf));
		tx_buf[0]  = 0xa5;
		tx_buf[1]  = 0x5a;
		tx_buf[2]  = 0x07;
		tx_buf[3]  = 1+9*2;///1 + len * sizeof(int16)
		tx_buf[4]  = 0xff;
		memcpy(&tx_buf[5], &gCtrlVars.HwCt.ADC1DigitalCt.Enable, 2);
		memcpy(&tx_buf[7], &gCtrlVars.HwCt.ADC1DigitalCt.Sample_Rate, 2);
		memcpy(&tx_buf[9], &gCtrlVars.HwCt.ADC1DigitalCt.CLK_Source, 2);
		memcpy(&tx_buf[11], &gCtrlVars.HwCt.ADC1DigitalCt.Channel_Swap, 2);
		//memcpy(&tx_buf[13], &gCtrlVars.sample_rate_index, 2);
		memcpy(&tx_buf[13],  &gCtrlVars.HwCt.ADC1DigitalCt.DC_Block_Enable, 2);
		memcpy(&tx_buf[15], &gCtrlVars.HwCt.ADC1DigitalCt.L_Volume, 2);
		//tx_buf[19] = 5;
		memcpy(&tx_buf[17], &gCtrlVars.HwCt.ADC1DigitalCt.R_Volume, 2);
		memcpy(&tx_buf[19], &gCtrlVars.HwCt.ADC1DigitalCt.L_Mute, 2);
		memcpy(&tx_buf[21], &gCtrlVars.HwCt.ADC1DigitalCt.R_Mute, 2);
		tx_buf[23] = 0x16;
		Communication_Effect_Send(tx_buf, 24);
	}
	else
	{
		switch(buf[0])///ADC0 PGA
		{
			case 0xff:
				buf++;
				for(i = 0; i < 10; i++)
				{
					cbuf[0] = i;////id
					for(k = 0; k < CTL_DATA_SIZE; k++)
					{
						cbuf[ k + 1] = (uint8_t)buf[k];
					}
					Comm_ADC1_0x07(&cbuf[0]);
					buf += 2;
				}
				break;
			default:
				Comm_ADC1_0x07(buf);
				break;
		}
	}
}

//AGC1
void Comm_AGC1_0x08(uint8_t * buf)
{
	uint16_t TmpData;

	memcpy(&TmpData, &buf[1], 2);
	switch(buf[0])
	{
		case 0:
			gCtrlVars.HwCt.ADC1AGCCt.AGC_EN = TmpData;
			if(TmpData == 0)
			{
				AudioADC_AGCChannelSel(ADC1_MODULE, 0, 0);
			}
			else if(TmpData == 1)
			{
				AudioADC_AGCChannelSel(ADC1_MODULE, 1, 0);
			}
			else if(TmpData == 2)
			{
				AudioADC_AGCChannelSel(ADC1_MODULE, 0, 1);
			}
			else if(TmpData == 3)
			{
				AudioADC_AGCChannelSel(ADC1_MODULE, 1, 1);
			}
			break;
		case 1:
			gCtrlVars.HwCt.ADC1AGCCt.Max_Level = TmpData;
			AudioADC_AGCMaxLevel(ADC1_MODULE, (uint8_t)TmpData);
			break;
		case 2: //Target_Level
			gCtrlVars.HwCt.ADC1AGCCt.Target_Level = TmpData;
			AudioADC_AGCTargetLevel(ADC1_MODULE, (uint8_t)TmpData);
			break;
		case 3: //Max_Gain
			gCtrlVars.HwCt.ADC1AGCCt.Max_Gain = TmpData;
			AudioADC_AGCMaxGain(ADC1_MODULE, (uint8_t)TmpData);
			break;
		case 4: //Min_Gain
			gCtrlVars.HwCt.ADC1AGCCt.Min_Gain = TmpData;
			AudioADC_AGCMinGain(ADC1_MODULE, (uint8_t)TmpData);
			break;
		case 5: //Gain_Offset
			gCtrlVars.HwCt.ADC1AGCCt.Gain_Offset = TmpData;
			AudioADC_AGCGainOffset(ADC1_MODULE, (uint8_t)TmpData);
			break;
		case 6://Frame_Time
			gCtrlVars.HwCt.ADC1AGCCt.Frame_Time = TmpData;
			AudioADC_AGCFrameTime(ADC1_MODULE, TmpData);
			break;
		case 7://Hold_N_Frame
			gCtrlVars.HwCt.ADC1AGCCt.Hold_N_Frame = TmpData;
			AudioADC_AGCHoldFrames(ADC1_MODULE, (uint32_t)TmpData);
			break;
		case 8: //Attack_Time
			gCtrlVars.HwCt.ADC1AGCCt.Attack_Time = TmpData;
			AudioADC_AGCAttackStepTime(ADC1_MODULE, TmpData);
			break;
		case 9: //Decay_Time
			gCtrlVars.HwCt.ADC1AGCCt.Decay_Time = TmpData;
			AudioADC_AGCDecayStepTime(ADC1_MODULE, TmpData);
			break;
		case 10: //NG_Threshold
			gCtrlVars.HwCt.ADC1AGCCt.NG_Threshold = TmpData;
			AudioADC_AGCNoiseThreshold(ADC1_MODULE, (uint8_t)TmpData);
			break;
		case 11: //NG_Rref
			gCtrlVars.HwCt.ADC1AGCCt.NG_Rref = TmpData;
			break;
		case 12: //NG_Hold_N_Frame
			gCtrlVars.HwCt.ADC1AGCCt.NG_Hold_N_Frame = TmpData;
			AudioADC_AGCNoiseHoldFrames(ADC1_MODULE, (uint8_t)TmpData);
			break;
		default:
			break;
	}
}

void Communication_Effect_0x08(uint8_t *buf, uint32_t len)////ADC1 AGC
{
	uint16_t i,k;

	if(len == 0) //ask
	{
		memset(tx_buf, 0, sizeof(tx_buf));
		tx_buf[0]  = 0xa5;
		tx_buf[1]  = 0x5a;
		tx_buf[2]  = 0x08;
		tx_buf[3]  = 1+13*2;///1 + len * sizeof(int16)
		tx_buf[4]  = 0xff;
		memcpy(&tx_buf[5], &gCtrlVars.HwCt.ADC1AGCCt.AGC_EN, 2);
		memcpy(&tx_buf[7], &gCtrlVars.HwCt.ADC1AGCCt.Max_Level, 2);
		memcpy(&tx_buf[9], &gCtrlVars.HwCt.ADC1AGCCt.Target_Level, 2);
		memcpy(&tx_buf[11], &gCtrlVars.HwCt.ADC1AGCCt.Max_Gain, 2);
		//memcpy(&tx_buf[13], &gCtrlVars.sample_rate_index, 2);//定义一个全局结构体，用于上传PC
		memcpy(&tx_buf[13], &gCtrlVars.HwCt.ADC1AGCCt.Min_Gain, 2);
		memcpy(&tx_buf[15], &gCtrlVars.HwCt.ADC1AGCCt.Gain_Offset, 2);
		memcpy(&tx_buf[17], &gCtrlVars.HwCt.ADC1AGCCt.Frame_Time, 2);
		memcpy(&tx_buf[19], &gCtrlVars.HwCt.ADC1AGCCt.Hold_N_Frame, 2);
		memcpy(&tx_buf[21], &gCtrlVars.HwCt.ADC1AGCCt.Attack_Time, 2);
		memcpy(&tx_buf[23], &gCtrlVars.HwCt.ADC1AGCCt.Decay_Time, 2);
		memcpy(&tx_buf[25], &gCtrlVars.HwCt.ADC1AGCCt.NG_Threshold, 2);
		memcpy(&tx_buf[27], &gCtrlVars.HwCt.ADC1AGCCt.NG_Rref, 2);
		memcpy(&tx_buf[29], &gCtrlVars.HwCt.ADC1AGCCt.NG_Hold_N_Frame, 2);
		tx_buf[31] = 0x16;
		Communication_Effect_Send(tx_buf, 32);
	}
	else
	{
		switch(buf[0])
		{
			case 0xff:
				buf++;
				for(i = 0; i < 13; i++)
				{
					cbuf[0] = i;////id
					for(k = 0; k < CTL_DATA_SIZE; k++)
					{
						cbuf[ k + 1] = (uint8_t)buf[k];
					}
					Comm_AGC1_0x08(&cbuf[0]);
					buf += 2;
				}
				break;
			default:
				Comm_AGC1_0x08(buf);
				break;
		}
	}
}

//DAC0
void Comm_DAC0_0x09(uint8_t * buf)
{
    uint16_t TmpData;
    uint32_t val;

	memcpy(&TmpData, &buf[1], 2);
	switch(buf[0]) //DAC0 set
	{
		case 0: //DAC0 en
			break;
        case 1: //DAC_Mode
			break;
        case 2: //Sample_Rate
			gCtrlVars.HwCt.DAC0Ct.Sample_Rate = TmpData;
			break;
        case 3: //CLK_Source
			gCtrlVars.HwCt.DAC0Ct.CLK_Source = TmpData;
			break;
		case 4: //Dither_En
			gCtrlVars.HwCt.DAC0Ct.Dither_En = TmpData;
			if(TmpData == 0)
			{
				AudioDAC_DitherDisable(DAC0_MODULE);
			}
			if(TmpData == 1)
			{
				AudioDAC_DitherEnable(DAC0_MODULE);
			}
			break;
		case 5: //Dither_Value_H15
			gCtrlVars.HwCt.DAC0Ct.Dither_ValueH15 = TmpData;
			val = AudioDAC_DitherPowGet(DAC0_MODULE);
			val = (val & 0x00FF) | ((uint32_t)(TmpData & 0x7FFF) << 8);
			AudioDAC_DitherPowSet(DAC0_MODULE, val);
			break;
		case 6: //Dither_Value_L8
			gCtrlVars.HwCt.DAC0Ct.Dither_ValueL8 = TmpData;
			val = AudioDAC_DitherPowGet(DAC0_MODULE);
			val = (val & 0xFFFFFF00) | TmpData;
			AudioDAC_DitherPowSet(DAC0_MODULE, val);
			break;
        case 7: //Scramble_En
            gCtrlVars.HwCt.DAC0Ct.Scramble_En = !!TmpData;
            if(gCtrlVars.HwCt.DAC0Ct.Scramble_En)
            {
            	AudioDAC_ScrambleEnable(DAC0_MODULE);
            }
            else
            {
            	AudioDAC_ScrambleDisable(DAC0_MODULE);
            }
            break;
		case 8: //Scramble_Mode
            gCtrlVars.HwCt.DAC0Ct.Scramble_Mode = TmpData > 2? 2 : TmpData;
			AudioDAC_ScrambleModeSet(DAC0_MODULE, (SCRAMBLE_MODULE)gCtrlVars.HwCt.DAC0Ct.Scramble_Mode);
            break;
        case 9: //Out_Mode (静态配置参数，不可运行时调整)
            // gCtrlVars.HwCt.DAC0Ct.Out_Mode = TmpData > 3? 3 : TmpData;
            // AudioDAC_DoutModeSet 已移至 AUDIODAC_ParamCt.DoutMode 在 init 时通过宏配置
            break;
        case 10: //L_Volume
			gCtrlVars.HwCt.DAC0Ct.L_Volume = TmpData;
			AudioDAC_VolGainSet_For_dB(DAC0_MODULE, (int16_t)gCtrlVars.HwCt.DAC0Ct.L_Volume, (int16_t)gCtrlVars.HwCt.DAC0Ct.R_Volume);
	        break;
        case 11: //R_Volume
			gCtrlVars.HwCt.DAC0Ct.R_Volume = TmpData;
			AudioDAC_VolGainSet_For_dB(DAC0_MODULE, (int16_t)gCtrlVars.HwCt.DAC0Ct.L_Volume, (int16_t)gCtrlVars.HwCt.DAC0Ct.R_Volume);
	        break;
		case 12: //L_Mute
            gCtrlVars.HwCt.DAC0Ct.L_Mute = !!TmpData;
            AudioDAC_DigitalMute(DAC0_MODULE, gCtrlVars.HwCt.DAC0Ct.L_Mute, gCtrlVars.HwCt.DAC0Ct.R_Mute);
            break;
		case 13: //R_Mute
            gCtrlVars.HwCt.DAC0Ct.R_Mute = !!TmpData;
            AudioDAC_DigitalMute(DAC0_MODULE, gCtrlVars.HwCt.DAC0Ct.L_Mute, gCtrlVars.HwCt.DAC0Ct.R_Mute);
            break;
		default:
			break;
	}
}

void Communication_Effect_0x09(uint8_t *buf, uint32_t len)///DAC0
{
	uint16_t i,k;

	if(len == 0) //ask
	{
		memset(tx_buf, 0, sizeof(tx_buf));
		tx_buf[0]  = 0xa5;
		tx_buf[1]  = 0x5a;
		tx_buf[2]  = 0x09;
		tx_buf[3]  = 1 + 14*2;///1 + len * sizeof(int16)
		tx_buf[4]  = 0xff;
		memcpy(&tx_buf[5], &gCtrlVars.HwCt.DAC0Ct.Enable, 2);
        memcpy(&tx_buf[7], &gCtrlVars.HwCt.DAC0Ct.DAC_Mode, 2);
		memcpy(&tx_buf[9], &gCtrlVars.HwCt.DAC0Ct.Sample_Rate, 2);
		memcpy(&tx_buf[11], &gCtrlVars.HwCt.DAC0Ct.CLK_Source, 2);
		memcpy(&tx_buf[13], &gCtrlVars.HwCt.DAC0Ct.Dither_En, 2);
		memcpy(&tx_buf[15], &gCtrlVars.HwCt.DAC0Ct.Dither_ValueH15, 2);
		memcpy(&tx_buf[17], &gCtrlVars.HwCt.DAC0Ct.Dither_ValueL8, 2);
		memcpy(&tx_buf[19], &gCtrlVars.HwCt.DAC0Ct.Scramble_En, 2);
		memcpy(&tx_buf[21], &gCtrlVars.HwCt.DAC0Ct.Scramble_Mode, 2);
		memcpy(&tx_buf[23], &gCtrlVars.HwCt.DAC0Ct.Out_Mode, 2);
		memcpy(&tx_buf[25], &gCtrlVars.HwCt.DAC0Ct.L_Volume, 2);
		memcpy(&tx_buf[27], &gCtrlVars.HwCt.DAC0Ct.R_Volume, 2);
		memcpy(&tx_buf[29], &gCtrlVars.HwCt.DAC0Ct.L_Mute, 2);
		memcpy(&tx_buf[31], &gCtrlVars.HwCt.DAC0Ct.R_Mute, 2);
		tx_buf[33] = 0x16;
		Communication_Effect_Send(tx_buf, 34);
	}
	else
	{
		switch(buf[0])///dac0
		{
			case 0xff:
				buf++;
				for(i = 0; i < 14; i++)
				{
					cbuf[0] = i;////id
					for(k = 0; k < CTL_DATA_SIZE; k++)
					{
						cbuf[ k + 1] = (uint8_t)buf[k];
					}
					Comm_DAC0_0x09(&cbuf[0]);
					buf += 2;
				}
				break;
			default:
				Comm_DAC0_0x09(buf);
				break;
		}
	}
}

//DAC1
void Comm_DAC1_0x0A(uint8_t * buf)
{
    uint16_t TmpData;
    uint32_t val;

	memcpy(&TmpData, &buf[1], 2);
	switch(buf[0]) //DAC1 set
	{
		case 0: //DAC1 en
			break;
        case 1: //DAC_Mode
			break;
        case 2: //Sample_Rate
			gCtrlVars.HwCt.DAC1Ct.Sample_Rate = TmpData;
			break;
        case 3: //CLK_Source
			gCtrlVars.HwCt.DAC1Ct.CLK_Source = TmpData;
			break;
		case 4: //Dither_En
			gCtrlVars.HwCt.DAC1Ct.Dither_En = TmpData;
			if(TmpData == 0)
			{
				AudioDAC_DitherDisable(DAC1_MODULE);
			}
			if(TmpData == 1)
			{
				AudioDAC_DitherEnable(DAC1_MODULE);
			}
			break;
		case 5: //Dither_Value_H15
			gCtrlVars.HwCt.DAC1Ct.Dither_ValueH15 = TmpData;
			val = AudioDAC_DitherPowGet(DAC1_MODULE);
			val = (val & 0x00FF) | ((uint32_t)(TmpData & 0x7FFF) << 8);
			AudioDAC_DitherPowSet(DAC1_MODULE, val);
			break;
		case 6: //Dither_Value_L8
			gCtrlVars.HwCt.DAC1Ct.Dither_ValueL8 = TmpData;
			val = AudioDAC_DitherPowGet(DAC1_MODULE);
			val = (val & 0xFFFFFF00) | TmpData;
			AudioDAC_DitherPowSet(DAC1_MODULE, val);
			break;
        case 7: //Scramble_En
            gCtrlVars.HwCt.DAC1Ct.Scramble_En = !!TmpData;
            if(gCtrlVars.HwCt.DAC1Ct.Scramble_En)
            {
            	AudioDAC_ScrambleEnable(DAC1_MODULE);
            }
            else
            {
            	AudioDAC_ScrambleDisable(DAC1_MODULE);
            }
            break;
		case 8: //Scramble_Mode
            gCtrlVars.HwCt.DAC1Ct.Scramble_Mode = TmpData > 2? 2 : TmpData;
			AudioDAC_ScrambleModeSet(DAC1_MODULE, (SCRAMBLE_MODULE)gCtrlVars.HwCt.DAC1Ct.Scramble_Mode);
            break;
        case 9: //Out_Mode (静态配置参数，不可运行时调整)
            // gCtrlVars.HwCt.DAC1Ct.Out_Mode = TmpData > 3? 3 : TmpData;
            // AudioDAC_DoutModeSet 已移至 AUDIODAC_ParamCt.DoutMode 在 init 时通过宏配置
            break;
        case 10: //L_Volume
			gCtrlVars.HwCt.DAC1Ct.L_Volume = TmpData;
			AudioDAC_VolGainSet_For_dB(DAC1_MODULE, (int16_t)gCtrlVars.HwCt.DAC1Ct.L_Volume, (int16_t)gCtrlVars.HwCt.DAC1Ct.R_Volume);
	        break;
        case 11: //R_Volume
			gCtrlVars.HwCt.DAC1Ct.R_Volume = TmpData;
			AudioDAC_VolGainSet_For_dB(DAC1_MODULE, (int16_t)gCtrlVars.HwCt.DAC1Ct.L_Volume, (int16_t)gCtrlVars.HwCt.DAC1Ct.R_Volume);
	        break;
		case 12: //L_Mute
            gCtrlVars.HwCt.DAC1Ct.L_Mute = !!TmpData;
            AudioDAC_DigitalMute(DAC1_MODULE, gCtrlVars.HwCt.DAC1Ct.L_Mute, gCtrlVars.HwCt.DAC1Ct.R_Mute);
            break;
		case 13: //R_Mute
            gCtrlVars.HwCt.DAC1Ct.R_Mute = !!TmpData;
            AudioDAC_DigitalMute(DAC1_MODULE, gCtrlVars.HwCt.DAC1Ct.L_Mute, gCtrlVars.HwCt.DAC1Ct.R_Mute);
            break;
		default:
			break;
	}
}

void Communication_Effect_0x0A(uint8_t *buf, uint32_t len)///DAC0
{
	uint16_t i,k;

	if(len == 0) //ask
	{
		memset(tx_buf, 0, sizeof(tx_buf));
		tx_buf[0]  = 0xa5;
		tx_buf[1]  = 0x5a;
		tx_buf[2]  = 0x0A;
		tx_buf[3]  = 1 + 14*2;///1 + len * sizeof(int16)
		tx_buf[4]  = 0xff;
		memcpy(&tx_buf[5], &gCtrlVars.HwCt.DAC1Ct.Enable, 2);
        memcpy(&tx_buf[7], &gCtrlVars.HwCt.DAC1Ct.DAC_Mode, 2);
		memcpy(&tx_buf[9], &gCtrlVars.HwCt.DAC1Ct.Sample_Rate, 2);
		memcpy(&tx_buf[11], &gCtrlVars.HwCt.DAC1Ct.CLK_Source, 2);
		memcpy(&tx_buf[13], &gCtrlVars.HwCt.DAC1Ct.Dither_En, 2);
		memcpy(&tx_buf[15], &gCtrlVars.HwCt.DAC1Ct.Dither_ValueH15, 2);
		memcpy(&tx_buf[17], &gCtrlVars.HwCt.DAC1Ct.Dither_ValueL8, 2);
		memcpy(&tx_buf[19], &gCtrlVars.HwCt.DAC1Ct.Scramble_En, 2);
		memcpy(&tx_buf[21], &gCtrlVars.HwCt.DAC1Ct.Scramble_Mode, 2);
		memcpy(&tx_buf[23], &gCtrlVars.HwCt.DAC1Ct.Out_Mode, 2);
		memcpy(&tx_buf[25], &gCtrlVars.HwCt.DAC1Ct.L_Volume, 2);
		memcpy(&tx_buf[27], &gCtrlVars.HwCt.DAC1Ct.R_Volume, 2);
		memcpy(&tx_buf[29], &gCtrlVars.HwCt.DAC1Ct.L_Mute, 2);
		memcpy(&tx_buf[31], &gCtrlVars.HwCt.DAC1Ct.R_Mute, 2);
		tx_buf[33] = 0x16;
		Communication_Effect_Send(tx_buf, 34);
	}
	else
	{
		switch(buf[0])///dac0
		{
			case 0xff:
				buf++;
				for(i = 0; i < 14; i++)
				{
					cbuf[0] = i;////id
					for(k = 0; k < CTL_DATA_SIZE; k++)
					{
						cbuf[ k + 1] = (uint8_t)buf[k];
					}
					Comm_DAC1_0x0A(&cbuf[0]);
					buf += 2;
				}
				break;
			default:
				Comm_DAC1_0x0A(buf);
				break;
		}
	}
}

//DAC2
void Comm_DAC2_0x0B(uint8_t * buf)
{
    uint16_t TmpData;
    uint32_t val;

	memcpy(&TmpData, &buf[1], 2);
	switch(buf[0]) //DAC2 set
	{
		case 0: //DAC2 en
			break;
        case 1: //DAC_Mode
			break;
        case 2: //Sample_Rate
			gCtrlVars.HwCt.DAC2Ct.Sample_Rate = TmpData;
			break;
        case 3: //CLK_Source
			gCtrlVars.HwCt.DAC2Ct.CLK_Source = TmpData;
			break;
		case 4: //Dither_En
			gCtrlVars.HwCt.DAC2Ct.Dither_En = TmpData;
			if(TmpData == 0)
			{
				AudioDAC_DitherDisable(DAC2_MODULE);
			}
			if(TmpData == 1)
			{
				AudioDAC_DitherEnable(DAC2_MODULE);
			}
			break;
		case 5: //Dither_Value_H15
			gCtrlVars.HwCt.DAC2Ct.Dither_ValueH15 = TmpData;
			val = AudioDAC_DitherPowGet(DAC2_MODULE);
			val = (val & 0x00FF) | ((uint32_t)(TmpData & 0x7FFF) << 8);
			AudioDAC_DitherPowSet(DAC2_MODULE, val);
			break;
		case 6: //Dither_Value_L8
			gCtrlVars.HwCt.DAC2Ct.Dither_ValueL8 = TmpData;
			val = AudioDAC_DitherPowGet(DAC2_MODULE);
			val = (val & 0xFFFFFF00) | TmpData;
			AudioDAC_DitherPowSet(DAC2_MODULE, val);
			break;
        case 7: //Scramble_En
            gCtrlVars.HwCt.DAC2Ct.Scramble_En = !!TmpData;
            if(gCtrlVars.HwCt.DAC2Ct.Scramble_En)
            {
            	AudioDAC_ScrambleEnable(DAC2_MODULE);
            }
            else
            {
            	AudioDAC_ScrambleDisable(DAC2_MODULE);
            }
            break;
		case 8: //Scramble_Mode
            gCtrlVars.HwCt.DAC2Ct.Scramble_Mode = TmpData > 2? 2 : TmpData;
			AudioDAC_ScrambleModeSet(DAC2_MODULE, (SCRAMBLE_MODULE)gCtrlVars.HwCt.DAC2Ct.Scramble_Mode);
            break;
        case 9: //L_Volume
			gCtrlVars.HwCt.DAC2Ct.Volume = TmpData;
			AudioDAC_VolGainSet_For_dB(DAC2_MODULE, (int16_t)gCtrlVars.HwCt.DAC2Ct.Volume, (int16_t)gCtrlVars.HwCt.DAC2Ct.Volume);
	        break;
		case 10: //L_Mute
            gCtrlVars.HwCt.DAC2Ct.Mute = !!TmpData;
            AudioDAC_DigitalMute(DAC2_MODULE, gCtrlVars.HwCt.DAC2Ct.Mute, gCtrlVars.HwCt.DAC2Ct.Mute);
            break;
		default:
			break;
	}
}

void Communication_Effect_0x0B(uint8_t *buf, uint32_t len)///DAC0
{
	uint16_t i,k;

	if(len == 0) //ask
	{
		memset(tx_buf, 0, sizeof(tx_buf));
		tx_buf[0]  = 0xa5;
		tx_buf[1]  = 0x5a;
		tx_buf[2]  = 0x0B;
		tx_buf[3]  = 1 + 11*2;///1 + len * sizeof(int16)
		tx_buf[4]  = 0xff;
		memcpy(&tx_buf[5], &gCtrlVars.HwCt.DAC2Ct.Enable, 2);
        memcpy(&tx_buf[7], &gCtrlVars.HwCt.DAC2Ct.DAC_Mode, 2);
		memcpy(&tx_buf[9], &gCtrlVars.HwCt.DAC2Ct.Sample_Rate, 2);
		memcpy(&tx_buf[11], &gCtrlVars.HwCt.DAC2Ct.CLK_Source, 2);
		memcpy(&tx_buf[13], &gCtrlVars.HwCt.DAC2Ct.Dither_En, 2);
		memcpy(&tx_buf[15], &gCtrlVars.HwCt.DAC2Ct.Dither_ValueH15, 2);
		memcpy(&tx_buf[17], &gCtrlVars.HwCt.DAC2Ct.Dither_ValueL8, 2);
		memcpy(&tx_buf[19], &gCtrlVars.HwCt.DAC2Ct.Scramble_En, 2);
		memcpy(&tx_buf[21], &gCtrlVars.HwCt.DAC2Ct.Scramble_Mode, 2);
		memcpy(&tx_buf[23], &gCtrlVars.HwCt.DAC2Ct.Volume, 2);
		memcpy(&tx_buf[25], &gCtrlVars.HwCt.DAC2Ct.Mute, 2);
		tx_buf[27] = 0x16;
		Communication_Effect_Send(tx_buf, 28);
	}
	else
	{
		switch(buf[0])///dac0
		{
			case 0xff:
				buf++;
				for(i = 0; i < 11; i++)
				{
					cbuf[0] = i;////id
					for(k = 0; k < CTL_DATA_SIZE; k++)
					{
						cbuf[ k + 1] = (uint8_t)buf[k];
					}
					Comm_DAC2_0x0B(&cbuf[0]);
					buf += 2;
				}
				break;
			default:
				Comm_DAC2_0x0B(buf);
				break;
		}
	}
}

void Comm_I2S0_0x0C(uint8_t * buf)
{
	uint16_t TmpData;
	memcpy(&TmpData, &buf[1], 2);
	switch(buf[0])//I2S0
	{
		case 0: //I2S0 TX_Enable
			gCtrlVars.HwCt.I2S0Ct.TX_Enable = TmpData;
			break;
		case 1: //I2S0 RX_Enable
			gCtrlVars.HwCt.I2S0Ct.RX_Enable = TmpData;
			break;
		case 2: //I2S0 Sample_Rate
			gCtrlVars.HwCt.I2S0Ct.Sample_Rate = TmpData;
			break;
		case 3: //I2S0 CLK_Source
			gCtrlVars.HwCt.I2S0Ct.CLK_Source = TmpData;
			break;
		case 4: //I2S0 Mode
			gCtrlVars.HwCt.I2S0Ct.Mode = TmpData;
			break;
		case 5: //I2S0 Format
			gCtrlVars.HwCt.I2S0Ct.Format = TmpData;
			break;
		case 6: //I2S0 Wordlth
			gCtrlVars.HwCt.I2S0Ct.Wordlth = TmpData;
			break;
		case 7: //I2S0 BCLK_INV
			gCtrlVars.HwCt.I2S0Ct.BCLK_INV = TmpData;
			break;
		case 8: //I2S0 LRCLK_INV
			gCtrlVars.HwCt.I2S0Ct.LRCLK_INV = TmpData;
			break;
		case 9: //I2S0 TX_Mute
			gCtrlVars.HwCt.I2S0Ct.TX_Mute = TmpData;
			break;
		default:
			break;
	}
}
void Communication_Effect_0x0C(uint8_t *buf, uint32_t len)//I2S0
{
	uint16_t i,k;
	if(len == 0) //ask
	{
		memset(tx_buf, 0, sizeof(tx_buf));
		tx_buf[0]  = 0xa5;
		tx_buf[1]  = 0x5a;
		tx_buf[2]  = 0x0C;
		tx_buf[3]  = 1 + 10*2;///1 + len * sizeof(int16)
		tx_buf[4]  = 0xff;
#if (defined(CFG_I2S0_MIX_OUT_EN))
		//memcpy(&tx_buf[5], &gCtrlVars.HwCt.I2S0Ct.TX_Enable, 2);
		tx_buf[5] = 1;
#else
		tx_buf[5] = 0;
#endif
#if (defined(CFG_APP_I2SIN_MODE_EN) && (CFG_RES_I2S_MODULE == 0)) || (defined(CFG_I2S0_MIX_IN_EN))
		//memcpy(&tx_buf[7], &gCtrlVars.HwCt.I2S0Ct.RX_Enable, 2);
		tx_buf[7] = 1;
#else
		tx_buf[7] = 0;
#endif
		//memcpy(&tx_buf[9], &gCtrlVars.HwCt.I2S0Ct.Sample_Rate, 2);
		tx_buf[9] = comm_ret_sample_rate_enum(I2S_SampleRateGet(I2S0_MODULE));
		memcpy(&tx_buf[11], &gCtrlVars.HwCt.I2S0Ct.CLK_Source, 2);
		//memcpy(&tx_buf[13], &gCtrlVars.HwCt.I2S0Ct.Mode, 2);
		tx_buf[13] = AudioI2S_MasterModeGet(I2S0_MODULE);
		//memcpy(&tx_buf[15], &gCtrlVars.HwCt.I2S0Ct.Format, 2);
		tx_buf[15] = 2;
		//memcpy(&tx_buf[17], &gCtrlVars.HwCt.I2S0Ct.Wordlth, 2);
		tx_buf[17] = I2S_WordlengthGet(I2S0_MODULE);
		//memcpy(&tx_buf[19], &gCtrlVars.HwCt.I2S0Ct.BCLK_INV, 2);
		tx_buf[19] = 0;   //默认是0，驱动中没有这个的设置
		//memcpy(&tx_buf[21], &gCtrlVars.HwCt.I2S0Ct.LRCLK_INV, 2);
		tx_buf[21] = 0;
		//memcpy(&tx_buf[23], &gCtrlVars.HwCt.I2S0Ct.TX_Mute, 2);
		tx_buf[23] = I2S_MuteStatusGet(I2S0_MODULE);
		tx_buf[25] = 0x16;
		Communication_Effect_Send(tx_buf, 26);
	}
	else
	{
		switch(buf[0])///I2S0
		{
			case 0xff:
				buf++;
				for(i = 0; i < 10; i++)
				{
					cbuf[0] = i;////id
					for(k = 0; k < CTL_DATA_SIZE; k++)
					{
						cbuf[ k + 1] = (uint8_t)buf[k];
					}
					Comm_I2S0_0x0C(&cbuf[0]);
					buf += 2;
				}
				break;
			default:
				Comm_I2S0_0x0C(buf);
				break;
		}
	}
}

void Comm_I2S1_0x0D(uint8_t * buf)
{
	uint16_t TmpData;
	memcpy(&TmpData, &buf[1], 2);
	switch(buf[0])//I2S0
	{
		case 0: //I2S1 TX_Enable
			gCtrlVars.HwCt.I2S1Ct.TX_Enable = TmpData;
			break;
		case 1: //I2S1 RX_Enable
			gCtrlVars.HwCt.I2S1Ct.RX_Enable = TmpData;
			break;
		case 2: //I2S1 Sample_Rate
			gCtrlVars.HwCt.I2S1Ct.Sample_Rate = TmpData;
			break;
		case 3: //I2S1 CLK_Source
			gCtrlVars.HwCt.I2S1Ct.CLK_Source = TmpData;
			break;
		case 4: //I2S1 Mode
			gCtrlVars.HwCt.I2S1Ct.Mode = TmpData;
			break;
		case 5: //I2S1 Format
			gCtrlVars.HwCt.I2S1Ct.Format = TmpData;
			break;
		case 6: //I2S1 TDM_Type
			gCtrlVars.HwCt.I2S1Ct.TDM_Type = TmpData;
			break;
		case 7: //I2S1 TDM_Amount_Cnt
			gCtrlVars.HwCt.I2S1Ct.TDM_Amount_Cnt = TmpData;
			break;
		case 8: //I2S1 Wordlth
			gCtrlVars.HwCt.I2S1Ct.Wordlth = TmpData;
			break;
		case 9: //I2S1 BCLK_INV
			gCtrlVars.HwCt.I2S1Ct.BCLK_INV = TmpData;
			break;
		case 10: //I2S1 LRCLK_INV
			gCtrlVars.HwCt.I2S1Ct.LRCLK_INV = TmpData;
			break;
		case 11: //I2S1 Dual_Mode_En
			gCtrlVars.HwCt.I2S1Ct.Dual_Mode_En = TmpData;
			break;
		case 12: //I2S1 Quad_Mode_En
			gCtrlVars.HwCt.I2S1Ct.Quad_Mode_En = TmpData;
			break;
		case 13: //I2S1 TX_Mute
			gCtrlVars.HwCt.I2S1Ct.TX_Mute = TmpData;
			break;
		default:
			break;
	}
}

void Communication_Effect_0x0D(uint8_t *buf, uint32_t len)//I2S1
{
	uint16_t i,k;
	if(len == 0) //ask
	{
		memset(tx_buf, 0, sizeof(tx_buf));
		tx_buf[0]  = 0xa5;
		tx_buf[1]  = 0x5a;
		tx_buf[2]  = 0x0D;
		tx_buf[3]  = 1 + 14*2;///1 + len * sizeof(int16)
		tx_buf[4]  = 0xff;
#if (defined(CFG_RES_AUDIO_I2S1_MIX_OUT_EN))
		tx_buf[5] = 1;
#else
		tx_buf[5] = 0;
#endif
#if (defined(CFG_APP_I2SIN_MODE_EN) && (CFG_RES_I2S_MODULE == 1)) || (defined(CFG_RES_AUDIO_I2S1_MIX_IN_EN))
		tx_buf[7] = 1;
#else
		tx_buf[7] = 0;
#endif
		tx_buf[9] = comm_ret_sample_rate_enum(I2S_SampleRateGet(I2S1_MODULE));
		memcpy(&tx_buf[11], &gCtrlVars.HwCt.I2S1Ct.CLK_Source, 2);
		tx_buf[13] = AudioI2S_MasterModeGet(I2S1_MODULE);
		tx_buf[15] = 2;
		memcpy(&tx_buf[17], &gCtrlVars.HwCt.I2S1Ct.TDM_Type, 2);
		memcpy(&tx_buf[19], &gCtrlVars.HwCt.I2S1Ct.TDM_Amount_Cnt, 2);
		tx_buf[21] = I2S_WordlengthGet(I2S1_MODULE);
		//memcpy(&tx_buf[23], &gCtrlVars.HwCt.I2S1Ct.BCLK_INV, 2);
		tx_buf[23] = 0;   //默认是0，驱动中没有这个的设置
		tx_buf[25] = 0; 
		//memcpy(&tx_buf[27], &gCtrlVars.HwCt.I2S1Ct.Dual_Mode_En, 2);
		tx_buf[27] = 0;
		//memcpy(&tx_buf[29], &gCtrlVars.HwCt.I2S1Ct.Quad_Mode_En, 2);
		tx_buf[29] = 0;
		tx_buf[31] = I2S_MuteStatusGet(I2S1_MODULE);
		tx_buf[33] = 0x16;
		Communication_Effect_Send(tx_buf, 34);
	}
	else
	{
		switch(buf[0])///I2S1
		{
			case 0xff:
				buf++;
				for(i = 0; i < 14; i++)
				{
					cbuf[0] = i;////id
					for(k = 0; k < CTL_DATA_SIZE; k++)
					{
						cbuf[ k + 1] = (uint8_t)buf[k];
					}
					Comm_I2S1_0x0D(&cbuf[0]);
					buf += 2;
				}
				break;
			default:
				Comm_I2S1_0x0D(buf);
				break;
		}
	}
}

void Comm_I2S2_0x0E(uint8_t * buf)
{
	uint16_t TmpData;
	memcpy(&TmpData, &buf[1], 2);
	switch(buf[0])
	{
		case 0:
			gCtrlVars.HwCt.I2S2Ct.TX_Enable = TmpData;
			break;
		case 1:
			gCtrlVars.HwCt.I2S2Ct.RX_Enable = TmpData;
			break;
		case 2:
			gCtrlVars.HwCt.I2S2Ct.Sample_Rate = TmpData;
			break;
		case 3:
			gCtrlVars.HwCt.I2S2Ct.CLK_Source = TmpData;
			break;
		case 4:
			gCtrlVars.HwCt.I2S2Ct.Mode = TmpData;
			break;
		case 5:
			gCtrlVars.HwCt.I2S2Ct.Format = TmpData;
			break;
		case 6:
			gCtrlVars.HwCt.I2S2Ct.TDM_Type = TmpData;
			break;
		case 7:
			gCtrlVars.HwCt.I2S2Ct.TDM_Amount_Cnt = TmpData;
			break;
		case 8:
			gCtrlVars.HwCt.I2S2Ct.Wordlth = TmpData;
			break;
		case 9:
			gCtrlVars.HwCt.I2S2Ct.BCLK_INV = TmpData;
			break;
		case 10:
			gCtrlVars.HwCt.I2S2Ct.LRCLK_INV = TmpData;
			break;
		case 11:
			gCtrlVars.HwCt.I2S2Ct.Dual_Mode_En = TmpData;
			break;
		case 12:
			gCtrlVars.HwCt.I2S2Ct.Quad_Mode_En = TmpData;
			break;
		case 13:
			gCtrlVars.HwCt.I2S2Ct.TX_Mute = TmpData;
			break;
		default:
			break;
	}
}

void Communication_Effect_0x0E(uint8_t *buf, uint32_t len)//SPDIF
{
	uint16_t i,k;
	if(len == 0) //ask
	{
		memset(tx_buf, 0, sizeof(tx_buf));
		tx_buf[0]  = 0xa5;
		tx_buf[1]  = 0x5a;
		tx_buf[2]  = 0x0E;
		tx_buf[3]  = 1 + 14*2;///1 + len * sizeof(int16)
		tx_buf[4]  = 0xff;
#if (defined(CFG_RES_AUDIO_I2S2_MIX_OUT_EN))
		tx_buf[5] = 1;
#else
		tx_buf[5] = 0;
#endif
#if (defined(CFG_APP_I2SIN_MODE_EN) && (CFG_RES_I2S_MODULE == 2)) || (defined(CFG_RES_AUDIO_I2S2_MIX_IN_EN))
		tx_buf[7] = 1;
#else
		tx_buf[7] = 0;
#endif
		tx_buf[9] = comm_ret_sample_rate_enum(I2S_SampleRateGet(I2S2_MODULE));
		memcpy(&tx_buf[11], &gCtrlVars.HwCt.I2S2Ct.CLK_Source, 2);
		tx_buf[13] = AudioI2S_MasterModeGet(I2S2_MODULE);
		tx_buf[15] = 2;
		memcpy(&tx_buf[17], &gCtrlVars.HwCt.I2S2Ct.TDM_Type, 2);
		memcpy(&tx_buf[19], &gCtrlVars.HwCt.I2S2Ct.TDM_Amount_Cnt, 2);
		tx_buf[21] = I2S_WordlengthGet(I2S2_MODULE);
		tx_buf[23] = 0;   //默认是0，驱动中没有这个的设置
		tx_buf[25] = 0; 
		tx_buf[27] = 0;
		tx_buf[29] = 0;
		tx_buf[31] = I2S_MuteStatusGet(I2S2_MODULE);
		tx_buf[33] = 0x16;
		Communication_Effect_Send(tx_buf, 34);
	}
	else
	{
		switch(buf[0])///I2S1
		{
			case 0xff:
				buf++;
				for(i = 0; i < 14; i++)
				{
					cbuf[0] = i;////id
					for(k = 0; k < CTL_DATA_SIZE; k++)
					{
						cbuf[ k + 1] = (uint8_t)buf[k];
					}
					Comm_I2S2_0x0E(&cbuf[0]);
					buf += 2;
				}
				break;
			default:
				Comm_I2S2_0x0E(buf);
				break;
		}
	}
}

void Comm_SPDIF0_0x0F(uint8_t * buf)
{
	uint16_t TmpData;
	memcpy(&TmpData, &buf[1], 2);
	switch(buf[0])
	{
		case 0:
			gCtrlVars.HwCt.SPDIF0Ct.Enable = TmpData;
			break;
		case 1:
			gCtrlVars.HwCt.SPDIF0Ct.Sample_Rate = TmpData;
			break;
		case 2:
			gCtrlVars.HwCt.SPDIF0Ct.Channle_Mode = TmpData;
			break;
		case 3:
			gCtrlVars.HwCt.SPDIF0Ct.IO_Select = TmpData;
			break;
		case 4:
			gCtrlVars.HwCt.SPDIF0Ct.Lock_Status = TmpData;
			break;
		default:
			break;
	}
}

void Communication_Effect_0x0F(uint8_t *buf, uint32_t len)//SPDIF
{
	uint16_t i,k;
	if(len == 0) //ask
	{
		memset(tx_buf, 0, sizeof(tx_buf));
		tx_buf[0]  = 0xa5;
		tx_buf[1]  = 0x5a;
		tx_buf[2]  = 0x0F;
		tx_buf[3]  = 1 + 5*2;///1 + len * sizeof(int16)
		tx_buf[4]  = 0xff;
		memcpy(&tx_buf[5], &gCtrlVars.HwCt.SPDIF0Ct.Enable, 2);
		memcpy(&tx_buf[7], &gCtrlVars.HwCt.SPDIF0Ct.Sample_Rate, 2);
		memcpy(&tx_buf[9], &gCtrlVars.HwCt.SPDIF0Ct.Channle_Mode, 2);
		memcpy(&tx_buf[11], &gCtrlVars.HwCt.SPDIF0Ct.IO_Select, 2);
		memcpy(&tx_buf[13], &gCtrlVars.HwCt.SPDIF0Ct.Lock_Status, 2);
		tx_buf[15] = 0x16;
		Communication_Effect_Send(tx_buf, 16);
	}
	else
	{
		switch(buf[0])///I2S1
		{
			case 0xff:
				buf++;
				for(i = 0; i < 5; i++)
				{
					cbuf[0] = i;////id
					for(k = 0; k < CTL_DATA_SIZE; k++)
					{
						cbuf[ k + 1] = (uint8_t)buf[k];
					}
					Comm_SPDIF0_0x0F(&cbuf[0]);
					buf += 2;
				}
				break;
			default:
				Comm_SPDIF0_0x0F(buf);
				break;
		}
	}
}

void Comm_SPDIF1_0x10(uint8_t * buf)
{
	uint16_t TmpData;
	memcpy(&TmpData, &buf[1], 2);
	switch(buf[0])
	{
		case 0:
			gCtrlVars.HwCt.SPDIF1Ct.Enable = TmpData;
			break;
		case 1:
			gCtrlVars.HwCt.SPDIF1Ct.Sample_Rate = TmpData;
			break;
		case 2:
			gCtrlVars.HwCt.SPDIF1Ct.Channle_Mode = TmpData;
			break;
		case 3:
			gCtrlVars.HwCt.SPDIF1Ct.IO_Select = TmpData;
			break;
		case 4:
			gCtrlVars.HwCt.SPDIF1Ct.Lock_Status = TmpData;
			break;
		default:
			break;
	}
}

void Communication_Effect_0x010(uint8_t *buf, uint32_t len)//SPDIF
{
	uint16_t i,k;
	if(len == 0) //ask
	{
		memset(tx_buf, 0, sizeof(tx_buf));
		tx_buf[0]  = 0xa5;
		tx_buf[1]  = 0x5a;
		tx_buf[2]  = 0x10;
		tx_buf[3]  = 1 + 5*2;///1 + len * sizeof(int16)
		tx_buf[4]  = 0xff;
		memcpy(&tx_buf[5], &gCtrlVars.HwCt.SPDIF1Ct.Enable, 2);
		memcpy(&tx_buf[7], &gCtrlVars.HwCt.SPDIF1Ct.Sample_Rate, 2);
		memcpy(&tx_buf[9], &gCtrlVars.HwCt.SPDIF1Ct.Channle_Mode, 2);
		memcpy(&tx_buf[11], &gCtrlVars.HwCt.SPDIF1Ct.IO_Select, 2);
		memcpy(&tx_buf[13], &gCtrlVars.HwCt.SPDIF1Ct.Lock_Status, 2);
		tx_buf[15] = 0x16;
		Communication_Effect_Send(tx_buf, 16);
	}
	else
	{
		switch(buf[0])///I2S1
		{
			case 0xff:
				buf++;
				for(i = 0; i < 5; i++)
				{
					cbuf[0] = i;////id
					for(k = 0; k < CTL_DATA_SIZE; k++)
					{
						cbuf[ k + 1] = (uint8_t)buf[k];
					}
					Comm_SPDIF1_0x10(&cbuf[0]);
					buf += 2;
				}
				break;
			default:
				Comm_SPDIF1_0x10(buf);
				break;
		}
	}
}

void Communication_Effect_0x80(uint8_t *buf, uint32_t len)
{
//	printf("see 0x80 :%02X\n", buf[0]);
	if(CommunicationChart->context_memory || buf[0] == V3_PACKAGE_TYPE_LIB || buf[0] == V3_PACKAGE_TYPE_PARAM
			|| buf[0] == V3_PACKAGE_TYPE_PARAMBIN_UPGRADE)
	{
		roboeffect_effect_enquiry_stream(buf, len);
	}
}

void Communication_Effect_0xfb(uint8_t *buf, uint32_t len)
{
	uint8_t num = buf[0], *addr = &(buf[1]), out_num = 0;
	uint32_t *ptr_size = (uint32_t *)&(tx_buf[5]);

	tx_buf[0] = 0xa5;
	tx_buf[1] = 0x5a;
	tx_buf[2] = 0xfb;

	for(int i = 0; i < num; i++)
	{
		uint32_t t_size;
		roboeffect_get_effect_size(CommunicationChart->context_memory, addr[i], &t_size);
		// printf("-->0x%02X: %d\n", addr[i], t_size);
		if(t_size < 0) break;//not enough nodes to get
		*ptr_size = t_size;
		ptr_size++;
		*ptr_size = roboeffect_recommend_frame_size_upon_effect_change(CommunicationChart->context_memory, CommunicationChart->user_effect_list->frame_size, addr[i], 1);
		ptr_size++;
		out_num++;
	}
	tx_buf[3] = 1 + out_num * 4 * 2;
	tx_buf[4] = out_num;
	tx_buf[tx_buf[3] + 4] = 0x16;

	// printf("send %d: ", tx_buf[3] + 5);
	// for(int i = 0; i<tx_buf[3] + 5; i++)
	// {
	// 	printf("%02X ", tx_buf[i]);
	// }
	// printf("\n");

	Communication_Effect_Send(tx_buf, tx_buf[3] + 5);
}

void Communication_Effect_0xff(uint8_t *buf, uint32_t len)
{
    uint32_t TmpData;
	memset(tx_buf, 0, sizeof(tx_buf));
	tx_buf[0] = 0xa5;
	tx_buf[1] = 0x5a;
	tx_buf[2] = 0xFF;
	tx_buf[3] = 3;
	if(len == 0)
	{
		tx_buf[4] = 0;//index0
		tx_buf[5] = CFG_COMMUNICATION_CRYPTO;
	}
	else
	{
		memcpy(&TmpData, &buf[0], 4);
		tx_buf[4] = 1;//index1
		if(CFG_COMMUNICATION_PASSWORD != TmpData)
		{
			tx_buf[5] = 0;///passwrod err
		}
		else
		{
		    tx_buf[5] = 1;///passwrod ok
		}
	}
	tx_buf[7] = 0x16;
	Communication_Effect_Send(tx_buf, 8);
}

void Communication_Effect_Config(uint8_t Control, uint8_t *buf, uint32_t len)
{
	// 未同步状态下，仅允许 0x00(版本查询)、0x02(系统信息)、0x80(调音查询)、0xFF(加密验证) 通过
	if(!g_host_sync_ok && Control != 0x00 && Control != 0x02 && Control != 0x80 && Control != 0xFF)
	{
		return;
	}

//	printf("see:%02X\n", Control);
	switch(Control)
	{
		case 0x00:
			Communication_Effect_0x00();
			break;
		case 0x01:
			Communication_Effect_0x01(buf, len);
			break;
		case 0x02:
			Communication_Effect_0x02();
			break;
		case 0x03:
			Communication_Effect_0x03(buf, len);
			break;
		case 0x04:
			Communication_Effect_0x04(buf, len);
			break;
		case 0x05:
			Communication_Effect_0x05(buf, len);
			break;
		case 0x06:
			Communication_Effect_0x06(buf, len);
			break;
		case 0x07:
			Communication_Effect_0x07(buf, len);
			break;
		case 0x08:
			Communication_Effect_0x08(buf, len);
			break;
		case 0x09: //DAC0
			Communication_Effect_0x09(buf, len);
			break;
		case 0x0A: //DAC1
			Communication_Effect_0x0A(buf, len);
			break;
		case 0x0B: //DAC2
			Communication_Effect_0x0B(buf, len);
			break;
		case 0x0C: //I2S0  //I2S只支持显示,不支持调整
			Communication_Effect_0x0C(buf, len);
			break;
		case 0x0D: //I2S1  //I2S只支持显示,不支持调整
			Communication_Effect_0x0D(buf, len);
			break;
		case 0x0E: //I2S2  //I2S只支持显示,不支持调整
			Communication_Effect_0x0E(buf, len);
			break;
		case 0x0F: //SPDIF0
			Communication_Effect_0x0F(buf, len);
			break;
		case 0x010: //SPDIF1
			Communication_Effect_0x010(buf, len);
			break;
		case 0x80:
			Communication_Effect_0x80(buf, len);
			break;
		case 0xfb:
			 Communication_Effect_0xfb(buf, len);
			break;
//		case 0xfc://user define tag
//			 Communication_Effect_0xfc(buf, len);
//			break;
//		case 0xfd://user define tag
//			 Communication_Effect_0xfd(buf, len);
//			break;
		case 0xff:
			Communication_Effect_0xff(buf, len);
			break;
		default:
			if(((Control >= 0x30) && (Control <= 0x7f)) || ((Control >= 0x81) && (Control <= 0xef)))
			{
				roboeffect_effect_update_params_entrance(Control, buf, len);
			}
			else
			{

			}
			break;
	}
	//-----Send ACK ----------------------//
	if(Control > 0xf0)///有效控制寄存器范围   这句解决加调音参数加密问题
	{
		return;
	}
	if(Control != 0x80 && package_id != 0)
	{
		package_id = 0;		//重新开始计数;
		gCtrlVars.AutoRefresh = AutoRefresh_ALL_PARA;
		AudioEffect_CommunicationQueue_Send(gCtrlVars.AutoRefresh);
	}
	if((Control > 0)&&(Control != 0x80))
	{
		if(len > 0)// if(len = 0) {polling all parameter}
		{
			memset(tx_buf, 0, sizeof(tx_buf));
			tx_buf[0] = Control;
			Communication_Effect_Send(tx_buf, 1);
		}
	}
}

void AudioEffect_CommunicationQueue_Init()
{
	uint8_t queueNum = 20;
	gCtrlVars.RefreshMsgHandle = MessageRegister(queueNum);
}

void AudioEffect_CommunicationQueue_Deinit()
{
	if(gCtrlVars.RefreshMsgHandle)
	{
		MessageDeregister(gCtrlVars.RefreshMsgHandle);
		gCtrlVars.RefreshMsgHandle = NULL;
	}
}

bool AudioEffect_CommunicationQueue_Send(uint32_t sendMsgType)
{
	MessageContext sendMsg;
	sendMsg.msgId = sendMsgType;

	if(gCtrlVars.RefreshMsgHandle == NULL || sendMsgType == 0)
	{
		return  FALSE;
	}

	if(sendMsgType == AutoRefresh_ALL_PARA || sendMsgType == AutoRefresh_ALL_EFFECTS_PARA || sendMsgType == 0x80)
	{
		MessageClear(gCtrlVars.RefreshMsgHandle);
	}

	if (MessageSend(gCtrlVars.RefreshMsgHandle, &sendMsg) != pdTRUE)
	{
		return FALSE;
	}

	return TRUE;
}

uint32_t AudioEffect_CommunicationQueue_Recv()
{
	MessageContext recvMsg;
	recvMsg.msgId = AutoRefresh_NONE;

	if(gCtrlVars.RefreshMsgHandle == NULL)
	{
		return recvMsg.msgId;
	}

	MessageRecv(gCtrlVars.RefreshMsgHandle, &recvMsg, 1);
	return recvMsg.msgId;
}

#ifdef CFG_COMMUNICATION_BY_UART
#define ROBO_UART_BUF_MAX_LEN 512
uint8_t robo_uart_recv_buf[ROBO_UART_BUF_MAX_LEN] = {0};
MemHandle uart_recv_handle;

static uint8_t uart_tx_buf[512];
static uint8_t uart_rx_buf[512];
static uint8_t s_rx_buf[512];
static uint8_t uart_buf[4096];
void uart_data_init(void)
{
	if(GET_UARTIO_PORT(CFG_FUNC_COMMUNICATION_RX_PORT) == RES_PORT_A)
		GPIO_PortAModeSet(GET_UARTIO_PIN(CFG_FUNC_COMMUNICATION_RX_PORT), GET_UARTIO_MODE(CFG_FUNC_COMMUNICATION_RX_PORT));
	else
		GPIO_PortBModeSet(GET_UARTIO_PIN(CFG_FUNC_COMMUNICATION_RX_PORT), GET_UARTIO_MODE(CFG_FUNC_COMMUNICATION_RX_PORT));

	if(GET_UARTIO_PORT(CFG_FUNC_COMMUNICATION_TX_PORT) == RES_PORT_A)
		GPIO_PortAModeSet(GET_UARTIO_PIN(CFG_FUNC_COMMUNICATION_TX_PORT), GET_UARTIO_MODE(CFG_FUNC_COMMUNICATION_TX_PORT));
	else
		GPIO_PortBModeSet(GET_UARTIO_PIN(CFG_FUNC_COMMUNICATION_TX_PORT), GET_UARTIO_MODE(CFG_FUNC_COMMUNICATION_TX_PORT));

	UARTS_Init(GET_UARTIO_UARTX(CFG_FUNC_COMMUNICATION_TX_PORT), CFG_FUNC_COMMUNICATION_UART_BAUDRATE, 8, 0, 1);

	UART_IOCtl(GET_UARTIO_UARTX(CFG_FUNC_COMMUNICATION_TX_PORT), UART_IOCTL_DMA_TX_EN, 1);
	UART_IOCtl(GET_UARTIO_UARTX(CFG_FUNC_COMMUNICATION_TX_PORT), UART_IOCTL_DMA_RX_EN, 1);

	DMA_CircularConfig(CFG_FUNC_COMMUNICATION_RX_DMA_PORT, sizeof(uart_tx_buf)/2, uart_rx_buf, sizeof(uart_tx_buf));
	DMA_CircularConfig(CFG_FUNC_COMMUNICATION_TX_DMA_PORT, sizeof(uart_tx_buf)/2, uart_tx_buf, sizeof(uart_tx_buf));
	DMA_ChannelEnable(CFG_FUNC_COMMUNICATION_RX_DMA_PORT);
	DMA_ChannelEnable(CFG_FUNC_COMMUNICATION_TX_DMA_PORT);

	mv_mopen(&uart_recv_handle, robo_uart_recv_buf, ROBO_UART_BUF_MAX_LEN - 1, NULL);
}

void uart_data_entry(void)
{
	uint32_t len;
	uint8_t rxdata;

	if(DMA_CircularDataLenGet(CFG_FUNC_COMMUNICATION_RX_DMA_PORT) > 0)
	{
		memset(uart_buf, 0x00, sizeof(uart_buf));
		DMA_CircularDataGet(CFG_FUNC_COMMUNICATION_RX_DMA_PORT, uart_buf, 1);
		rxdata = uart_buf[0];
		mv_mwrite(&rxdata, 1, 1, &uart_recv_handle);
	}

	len = mv_msize(&uart_recv_handle);
	if(len > 0)
	{
		mv_mread(s_rx_buf, 1, len, &uart_recv_handle);
		roboeffect_prot_parse_big_block(s_rx_buf, len);
	}
}
#endif

#endif//#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
