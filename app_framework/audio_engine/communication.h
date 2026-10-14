/**
 **************************************************************************************
 * @file    communication.h
 * @brief   
 *
 * @author  Castle Cai
 * @version V1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __COMMUNICATION_H__
#define __COMMUNICATION_H__

#include <stdint.h>
#include <string.h>
#include <nds32_intrinsic.h>
#include "type.h"
#include "roboeffect_api.h"
#include "audio_effect_control.h"

#define STREAM_CLIPS_LEN 240
extern const uint8_t effect_property_for_display[];

//for communication
typedef struct {
	const char *name;
	int state;
	//uint16_t param_mode_index;
} SubEffect;

typedef struct _merge_status
{
	uint8_t addr;
	uint8_t status;//0 for addr, 1 for length, 2 for enable, 3 for param data
	uint16_t src_offset;
	uint32_t crc_sum;
}merge_status;

extern AudioEffectParambinContext *CommunicationChart;  // for Communication
extern bool g_host_sync_ok;  // 上位机同步状态标志，0=未同步，1=已同步

//v3 package 
#define V3_PACKAGE_TYPE_LIB           			0
#define V3_PACKAGE_TYPE_PARAM           		1
#define V3_PACKAGE_TYPE_STREAM          		2
#define V3_PACKAGE_TYPE_PARAMBIN_DATA    		3//parambin such as parambin header, flow block data, sub-type data, etc.
#define V3_PACKAGE_TYPE_PARAMBIN_UPGRADE 		4
#define V3_PACKAGE_TYPE_MODE_ALL_NAME     		5
#define V3_PACKAGE_TYPE_MODE_INDEX_QUERY       	6
#define V3_PACKAGE_TYPE_FLOW_NAME_QUERY     	7
#define V3_PACKAGE_TYPE_FLOW_INDEX_QUERY    	8
#define V3_PACKAGE_EFFECT_ALL_PARAM_EFFECT		9
#define V3_PACKAGE_EFFECT_ALL_PARAM_CODEC 		10
#define V3_PACKAGE_SOURCE_SINK_INFO 			11
#define V3_PACKAGE_CTRLVARS_HIDDEN				12
#define V3_PACKAGE_TYPE_MODE_LIB_CRC            14
#define V3_PACKAGE_TYPE_MODE_PARAMBIN_CRC       16
#define V3_PACKAGE_TYPE_PARAMBIN_FLOW_STATUS_UPLOAD       17

void AudioEffect_CommunicationQueue_Init();

void AudioEffect_CommunicationQueue_Deinit();

bool AudioEffect_CommunicationQueue_Send(uint32_t sendMsgType);

uint32_t AudioEffect_CommunicationQueue_Recv();

void AudioEffect_Communication_Otg_Init(void);

uint32_t comm_sample_rate_from_index(uint32_t index);

#ifdef CFG_COMMUNICATION_BY_UART
void uart_data_init(void);
void uart_data_entry(void);
#endif

#endif/*__COMMUNICATION_H__*/
