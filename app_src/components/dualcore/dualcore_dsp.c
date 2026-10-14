#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "app_config.h"
#include "rtos_api.h"
#include "dualcore_api.h"
#include "dualcore_dsp.h"
#include "debug.h"
#include "audio_core_effect.h"
#include "roboeffect_api.h"
#include "audio_effect_parambin.h"
#include "mvwireless2_config.h"

#ifdef FUNC_WIRELESS2_SUPPORT
#include "lc3p_audiocodec_api.h"
#endif

#ifdef DUAL_CORE_EN

uint32_t* ct1_ram_add = NULL;
uint32_t ct1_ram_len;

uint32_t* ct2_ram_add = NULL;
uint32_t ct2_ram_len;

RoboEffect_CT DspEffectCt;
static bool sDualCoreDspAddrInitOk = FALSE;

#ifdef FUNC_WIRELESS2_SUPPORT
WirelessEncoder_CT wireless_encoder_ct;
WirelessEncoderBuffer_CT wireless_encoderbuff_ct;
#endif

//typedef int (*roboeffect_apply_core2)(void *);
//void * main_context = NULL;//(void *)0x20099ea0;
//roboeffect_apply_core2 fun2 = NULL;//(int*)(0x2d8e4);//(int*)(0x2d894);
__attribute__((section(".dcdriver.isr")))
void IPC_ProcessForISR(TS_IPCITEM* ipcItem, BaseType_t* pxHigherPriorityTaskWoken)
{
//	TS_IPCITEM ipcItem;
//	memcpy(&ipcItem, pipcItem, 4);
	if((ipcItem->cmd == E_PING_REPLY) || (ipcItem->cmd == E_PING_REPLY2) || (ipcItem->cmd == E_CORE2_READY))
	{
		xSemaphoreGiveFromISR(gIPCAppSem, pxHigherPriorityTaskWoken);
	}
	else if((ipcItem->cmd == E_EFFECT_DSP_SET_DONE) || (ipcItem->cmd == E_EFFECT_DSP_EN_DONE))
	{
		xSemaphoreGiveFromISR(gIPCAppDSPSem, pxHigherPriorityTaskWoken);
	}
}

void IsCore2Ready(void)
{
//	TS_IPCITEM Data;

	if(xSemaphoreTake(gIPCAppSem, portMAX_DELAY) == pdTRUE )
	{
		//TS_IPCITEM ipcItem;
		//if (IpcReadOneItem((uint32_t *)&ipcItem))
		{
			TS_IPCITEM Data;
			Data.cmd = E_SYSTEM_TICK_SYNC;
			IpcSendOneItem(*(uint32_t *)(&Data));
		}
	}
}

//DspEffectContext.dsp_nodes_addr, DspEffectContext.dsp_nodes_len
void HandleIPCMsg(TS_IPCITEM ipcItem)
{
	switch (ipcItem.cmd)
	{
		case E_REFRESH:
			break;

		case E_UART_DEBUG:
			break;

		case E_PING:
			IpcSendOneItem(E_PING_REPLY);
			break;
		case E_CORE2_READY:
			DBG("core2 ready\n");
		case E_PING_REPLY:
			//DBG("&1\n");
			DataCacheInvalidAll();
			//DataCacheRangeInvalid((uint32_t)ct1_ram_add, ct1_ram_len);
			//DataCacheRangeInvalid((uint32_t)DspEffectCt.dsp_params_pool_addr, DspEffectCt.dsp_params_len);
			break;
		case E_PING_REPLY2:
			//DBG("&2\n");
			DataCacheInvalidAll();
			//DBG("add %x, len = %d\n", ct1_ram_add, ct1_ram_len);
			//DataCacheRangeInvalid((uint32_t)ct2_ram_add, ct2_ram_len);
			//DataCacheRangeInvalid((uint32_t)DspEffectCt.dsp_params_pool_addr2, DspEffectCt.dsp_params_len2);
			break;

		case E_PRINT_NOHSEM:
			//UartPrintWithoutHsem();
			break;

		case E_PRINT_HSEMQUERY:
			//UartPrintWithHsemQuery();
			break;
		case E_PRINT_HSEMINT:
			//UartPrintWithHsemInt();
			break;
		case E_EFFECT_DSP_SET:
			break;
		case E_EFFECT_DSP_EN:
			break;
		default:
			DBG("%s unkonw command %d\n", __func__, ipcItem.cmd);
			break;
	}
}

bool DualCoreDspAddrIsInitOk(void)
{
	return sDualCoreDspAddrInitOk;
}

void DualCoreDspRoboeffectApply(uint32_t item)
{
	//音效操作完成执行中，flash不能写
//	SpiFlashDevBusyFlagSet(1);
	SpiFlash_EraseWriteCriticalZone_Enter();
	IpcSendOneItem(item);
	if(xSemaphoreTake(gIPCAppSem, portMAX_DELAY) == pdTRUE )
	{
		DataCacheInvalidAll();
	}
	//SpiFlashDevBusyFlagSet(0);//音效操作完成，可以写flash
	SpiFlash_EraseWriteCriticalZone_Leave();
}

void DualCoreDspEffectSet(void *main_context, uint8_t addr, uint8_t param_index, int16_t *param_input)
{
	TS_IPCITEM Data;

	Data.cmd = E_EFFECT_DSP_SET;

	DspEffectCt.dspParamCt.dsp_ct_addr = (uint32_t)main_context;
	DspEffectCt.dspParamCt.addr = addr;
	DspEffectCt.dspParamCt.param_input_addr = (uint32_t)param_input;
	DspEffectCt.dspParamCt.param_index = param_index;

	//音效操作完成执行中，flash不能写
//	SpiFlashDevBusyFlagSet(1);
	SpiFlash_EraseWriteCriticalZone_Enter();
	IpcSendOneItem(*(uint32_t *)(&Data));
	//ACK
	if(xSemaphoreTake(gIPCAppDSPSem, portMAX_DELAY) == pdTRUE )
	{
	}
//	SpiFlashDevBusyFlagSet(0);
	SpiFlash_EraseWriteCriticalZone_Leave();
}

void DualCoreDspEffectEnable(void *main_context, uint8_t addr, uint8_t enable)
{
	TS_IPCITEM Data;

	Data.cmd = E_EFFECT_DSP_EN;

	DspEffectCt.dspParamCt.dsp_ct_addr = (uint32_t)main_context;
	DspEffectCt.dspParamCt.addr = addr;
	DspEffectCt.dspParamCt.enable = enable;

	//音效操作完成执行中，flash不能写
//	SpiFlashDevBusyFlagSet(1);
	SpiFlash_EraseWriteCriticalZone_Enter();
	IpcSendOneItem(*(uint32_t *)(&Data));
	//ACK
	if(xSemaphoreTake(gIPCAppDSPSem, portMAX_DELAY) == pdTRUE )
	{
	}
//	SpiFlashDevBusyFlagSet(0);
	SpiFlash_EraseWriteCriticalZone_Leave();
}

void DualCoreCore2UARTInit(void)
{
	TS_IPCITEM Data;

	Data.cmd = E_UART_DEBUG;
	Data.param = CORE2_UART;//后续跟3个有效数据
	//SendData = *(U32 *)(&Data);
	IpcSendOneItem(*(uint32_t *)(&Data));
	DBG("uart port = %x\n", Data.param);
}

void DualCoreDspAddrInit(void)
{
	TS_IPCITEM Data;

	sDualCoreDspAddrInitOk = FALSE;
	memset(&DspEffectCt, 0, sizeof(RoboEffect_CT));

	DspEffectCt.dsp_fuc_addr = (uint32_t)roboeffect_apply;

	DspEffectCt.dsp_effect_param_set_fun = (uint32_t)roboeffect_set_effect_parameter;
	DspEffectCt.dsp_effect_enable_fun = (uint32_t)roboeffect_enable_effect;
	DBG("dsp_set_effect_parameter add %lx\n", DspEffectCt.dsp_effect_param_set_fun);
	DBG("dsp_enable_effect add %lx\n", DspEffectCt.dsp_effect_enable_fun);

	roboeffect_get_frame_buffer_info((void*)EffectParambin.context_memory, (void*)&ct1_ram_add, (void*)&ct1_ram_len);
	DBG("add %lx, %lu\n", (uint32_t)ct1_ram_add, ct1_ram_len);

	DspEffectCt.dsp_ct_addr = (uint32_t)EffectParambin.context_memory;
	DspEffectCt.dsp_buf_addr = (uint32_t)ct1_ram_add;
	DspEffectCt.dsp_buf_len = ct1_ram_len;

	DspEffectCt.dsp_params_pool_addr = (uint32_t)roboeffect_get_effect_params_pool(EffectParambin.context_memory, &DspEffectCt.dsp_params_len);
	DBG("nodes = %lx, %lu\n", DspEffectCt.dsp_params_pool_addr, DspEffectCt.dsp_params_len);


#ifdef ROBOEFFECT_DOUBLE_ENGINE_EN
	roboeffect_get_frame_buffer_info((void*)EffectParambin2.context_memory, (void*)&ct2_ram_add, (void*)&ct2_ram_len);
	DBG("add22 %lx, %lu\n", (uint32_t)ct2_ram_add, ct2_ram_len);

	DspEffectCt.dsp_ct2_addr = (uint32_t)EffectParambin2.context_memory;
	DspEffectCt.dsp_buf2_addr = (uint32_t)ct2_ram_add;
	DspEffectCt.dsp_buf2_len = ct2_ram_len;

	DspEffectCt.dsp_params_pool_addr2 = (uint32_t)roboeffect_get_effect_params_pool(EffectParambin2.context_memory, &DspEffectCt.dsp_params_len2);
	DBG("nodes22 = %lx, %lu\n", DspEffectCt.dsp_params_pool_addr2, DspEffectCt.dsp_params_len2);
#endif

	Data.cmd = E_TRANS_DPS_CT;
	Data.param = ((uint32_t)&DspEffectCt) & 0x00FFFFFF;;
	DBG("Data.param  = %d\n", Data.param);
	if(IpcSendOneItem(*(uint32_t *)(&Data)))
	{
		sDualCoreDspAddrInitOk = TRUE;
	}
	DBG("Data.cmd = %x\n", Data.cmd);
}

#ifdef FUNC_WIRELESS2_SUPPORT

void DualCoreDspEncoderAddrInit(void)
{
	TS_IPCITEM Data;

	memset(&wireless_encoder_ct, 0, sizeof(WirelessEncoder_CT));

	wireless_encoder_ct.dsp_apply_addr = (uint32_t)wireless_encoder_apply;
	wireless_encoder_ct.dsp_encoder_id = 1;
//	wireless_encoder_ct.dsp_encoder_len =
	wireless_encoder_ct.dsp_input_addr  = (uint32_t)&wireless_encoderbuff_ct.dsp_input_addr;
	wireless_encoder_ct.dsp_output_addr = (uint32_t)&wireless_encoderbuff_ct.dsp_output_addr;


	Data.cmd = E_WIRELESS2_ENCODER_INIT;
	Data.param = ((uint32_t)&wireless_encoder_ct) & 0x00FFFFFF;;
	DBG("Data.param  = %d\n", Data.param);
	IpcSendOneItem(*(uint32_t *)(&Data));
	DBG("Data.cmd = %x\n", Data.cmd);
}

void DualCoreDspEncoderAddrSet(uint8_t encoder_id,uint8_t *encoder_buf,uint32_t* encoder_size,void *pcm_buf)
{
	TS_IPCITEM Data;

	wireless_encoder_ct.dsp_encoder_id = encoder_id;
	wireless_encoder_ct.dsp_encoder_framelen = ONE_FRAME;


	Data.cmd = E_WIRELESS2_ENCODER_APPLY;
	Data.param = ((uint32_t)&wireless_encoder_ct) & 0x00FFFFFF;;
//	DBG("Data.param  = %d\n", Data.param);
	IpcSendOneItem(*(uint32_t *)(&Data));
//	DBG("Data.cmd = %x\n", Data.cmd);
}

uint8_t DualCoreDspEncoderCoreRunDone(void)
{
	if(xSemaphoreTake(gIPCAppSem, portMAX_DELAY) == pdTRUE )
	{
		TS_IPCITEM ipcItem;
//		if (IpcReadOneItem((uint32_t *)&ipcItem))
//		{
//			extern void HandleIPCMsg(TS_IPCITEM ipcItem);
//			HandleIPCMsg(ipcItem);
//		}
		DataCacheInvalidAll();
		return 1;
	}
	return 0;
}

uint32_t DualCoreDspEncoderGetOutdata(uint8_t *encoder_buf,uint8_t *encoder_buf_R)
{
	memcpy(encoder_buf,wireless_encoderbuff_ct.dsp_output_addr,RFAUDIO_ENCODE_LEN);

	if(encoder_buf_R!=NULL)
		memcpy(encoder_buf_R,wireless_encoderbuff_ct.dsp_output_addr+RFAUDIO_ENCODE_LEN,RFAUDIO_ENCODE_LEN);

	return RFAUDIO_ENCODE_LEN;
}
#endif //FUNC_WIRELESS2_SUPPORT
#endif

