/**
 **************************************************************************************
 * @file    dualcore_dsp.h
 * @brief
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2025-12-25 13:06:47$
 *
 * @Copyright (C) 2025, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __DUALCORE_DSP_H__
#define __DUALCORE_DSP_H__

#include "mvwireless2_config.h"

typedef enum
{
	E_REFRESH = 0,
	E_UART_DEBUG,
	E_PING,
	E_PING2,
	E_CORE2_READY,
	E_SYSTEM_TICK_SYNC,
	E_PING_REPLY,
	E_PING_REPLY2,
	E_PRINT_NOHSEM,
	E_PRINT_HSEMQUERY,
	E_PRINT_HSEMINT,
	E_TRANS_DPS_CT,		//传输DSP处理数据结构
	E_EFFECT_DSP_SET,	//
	E_EFFECT_DSP_EN,	//
	E_WIRELESS2_ENCODER_INIT,
	E_WIRELESS2_ENCODER_APPLY,
	E_EFFECT_DSP_SET_DONE,	//
	E_EFFECT_DSP_EN_DONE,	//

} TE_IPCCMD;


typedef struct {
	uint32_t cmd             :8;
	uint32_t param           :24;
} TS_IPCITEM;

typedef struct _RoboEffectParam_CT_{
	uint32_t dsp_ct_addr; //ram ct地址
	uint32_t addr;
	uint32_t param_index;
	uint32_t param_input_addr;//buf addr
	uint32_t enable;
} RoboEffectParam_CT;

typedef struct _RoboEffect_CT_{

	uint32_t dsp_fuc_addr; //dsp执行函数
	uint32_t dsp_effect_param_set_fun;//函数地址
	uint32_t dsp_effect_enable_fun;//函数地址
	//图1
	uint32_t dsp_ct_addr; //单图下ram结构体地址
	uint32_t dsp_buf_addr;//单图下ram结构体地址
	uint32_t dsp_buf_len;//单图下ram结构体地址
	uint32_t dsp_params_pool_addr;//参数变量区域
	uint32_t dsp_params_len;
	//图2，如果双图使能
	uint32_t dsp_ct2_addr; //双图下图2的ram结构体地址
	uint32_t dsp_buf2_addr;//双图下图2的ram结构体地址
	uint32_t dsp_buf2_len;//双图下图2的ram结构体地址
	uint32_t dsp_params_pool_addr2;
	uint32_t dsp_params_len2;

	RoboEffectParam_CT dspParamCt;//在线调音
} RoboEffect_CT;

typedef struct _WirelessEncoder_CT_{
	uint8_t	 dsp_encoder_id;
	uint16_t dsp_encoder_framelen;
	uint32_t dsp_encoder_len;
	uint32_t dsp_output_addr;
	uint32_t dsp_input_addr;
	uint32_t dsp_apply_addr;

}WirelessEncoder_CT;

typedef struct _WirelessEncoderBuffer_CT_{
#if (RFAUDIO_ENCODE_CH==1)
	uint8_t  dsp_output_addr[RFAUDIO_ENCODE_LEN];
	uint32_t dsp_input_addr[ONE_FRAME];
	uint32_t dsp_output_size;
#elif (RFAUDIO_ENCODE_CH==2)
	uint8_t  dsp_output_addr[RFAUDIO_ENCODE_LEN];
	uint8_t  dsp_output_addr_R[RFAUDIO_ENCODE_LEN];
	int32_t dsp_input_addr[ONE_FRAME];
	int32_t dsp_input_addr_R[ONE_FRAME];
	uint32_t dsp_output_size;
#endif
}WirelessEncoderBuffer_CT;


extern RoboEffect_CT DspEffectCt;

extern WirelessEncoderBuffer_CT wireless_encoderbuff_ct;

#define 	CFG_CORE2_FLASH_RUNNING_ADDR	0x10000

typedef int (*roboeffect_apply_core2)(void *);

void IsCore2Ready(void);

void DualCoreDspAddrInit(void);

bool DualCoreDspAddrIsInitOk(void);

void DualCoreCore2UARTInit(void);

void DualCoreCore2DSPEffectSet(uint8_t addr, uint8_t param_index, int16_t *param_input);

void DualCoreCore2DSPEffectEnable(uint8_t addr, uint8_t enable);

void DualCoreDspRoboeffectApply(uint32_t item);
/*
 * param_input 参数必须为全局变量地址
 */
void DualCoreDspEffectSet(void *main_context, uint8_t addr, uint8_t param_index, int16_t *param_input);

void DualCoreDspEffectEnable(void *main_context, uint8_t addr, uint8_t enable);

#endif /*__DUALCORE_DSP_H__*/
