#ifndef __PC_ONLINE_PARAM_H__
#define __PC_ONLINE_PARAM_H__
#include "app_config.h"

#ifdef CFG_FUNC_FLASH_PARAM_ONLINE_TUNING_EN

enum
{
	PC_PARAM_CMD 				= 0x20,
	//PC_CRC_CMD 					= 0x21,
	PC_USB_CMD  				= 0x21,
	PC_UPGRADE_CMD				=0x22,
};

extern void FlashSn_Rx(uint8_t *buf,uint16_t buf_len);
extern void FlashCRC_Rx(uint8_t *buf,uint16_t buf_len);
extern void FlashParamUsb_Rx(uint8_t *buf,uint16_t buf_len);

#endif
#endif

