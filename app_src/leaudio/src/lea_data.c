/*
 * bis_data.c
 *
 *  Created on: Sep 9, 2024
 *      Author: zhaoshengqi
 */
#include <stdint.h>
#include "le_audio_config.h"
#include "gaf.h"
#include "app_config.h"
#include "breakpoint.h"

#if BT_LE_AUDIO_SUPPORT
static uint32_t	LeAudioSoftwareFlag = 0;


uint8_t g_cis_app = CIS_APP_NONE;
uint8_t g_audio_loc = GAF_LOC_NOT_ALLOWED;
uint8_t g_cis_mode = CIS_MODE_NONE;
uint8_t cis_role = CIS_NONE;//


#define LeAudioFlagRegister(SoftEvent)			(LeAudioSoftwareFlag |= SoftEvent)
#define LeAudioFlagDeregister(SoftEvent)		(LeAudioSoftwareFlag &= ~SoftEvent)
#define LeAudioFlagGet(SoftEvent)				(LeAudioSoftwareFlag & SoftEvent ? TRUE : FALSE)

enum _LEAUDIO_SOFTWARE_FLAG_
{
	LeAudioSinkRole			=	BIT(0),	//sink
	LeAudioSrcRole			= 	BIT(1),	//src
	LeAudioSinkWedApp		=	BIT(2),	//2路sink
	LeAudioSrcWedApp		= 	BIT(3),	//2路src
	LeAudioUseLocalSrc		= 	BIT(4),	//use_local_src
	LeAudioSrc1Sync			=	BIT(5),	//src1 sync
	LeAudioSrc2Sync			= 	BIT(6),	//src2 sync
	LeAudioSinkSyncLost		= 	BIT(7),	//sink sync lost
	LeAudioDecoderInit		= 	BIT(8),	//解码器初始化
	LeAudioEncoderInit		= 	BIT(9),	//编码器初始化

	LeAudioSinkDacInvalid	= 	BIT(10),//sink DAC pcm数据无效标志
	LeAudioDecoderCfgFlag	= 	BIT(11),//解码器Decoder Config更新
	LeAudioDiscovBisSupport = 	BIT(12),
	LeAudioSinkScanStartFlag=	BIT(13),
};

void Set_bis_role(uint8_t role)
{
	if(role == BIS_SINK)
	{
		LeAudioFlagRegister(LeAudioSinkRole);
		LeAudioFlagDeregister(LeAudioSrcRole);
	}
	else if(role == BIS_SRC)
	{
		LeAudioFlagRegister(LeAudioSrcRole);
		LeAudioFlagDeregister(LeAudioSinkRole);
	}
	else
	{
		LeAudioFlagDeregister(LeAudioSrcRole);
		LeAudioFlagDeregister(LeAudioSinkRole);
	}
#ifdef BP_SAVE_TO_FLASH_LE_AUDIO_ROLE
	BackupInfoUpdata(BACKUP_SYS_INFO); //保存Role
#endif	
}

void Set_discov_bis(uint8_t enable)
{
	if(enable)
		LeAudioFlagRegister(LeAudioDiscovBisSupport);
	else
		LeAudioFlagDeregister(LeAudioDiscovBisSupport);
}

uint8_t Get_discov_bis(void)
{
	return LeAudioFlagGet(LeAudioDiscovBisSupport);
}

uint8_t Get_bis_role(void)
{
	if(LeAudioFlagGet(LeAudioSrcRole))
		return BIS_SRC;

	if(LeAudioFlagGet(LeAudioSinkRole))
		return BIS_SINK;

	return BIS_NONE;
}

void Set_local_src(uint8_t is_local_src)
{
	if(is_local_src)
		LeAudioFlagRegister(LeAudioUseLocalSrc);
	else
		LeAudioFlagDeregister(LeAudioUseLocalSrc);
}

uint8_t Get_local_src(void)
{
	return LeAudioFlagGet(LeAudioUseLocalSrc);
}

void set_src_wed_vaild(bool wed_vaild)
{
	if(wed_vaild)
		LeAudioFlagRegister(LeAudioSrcWedApp);
	else
		LeAudioFlagDeregister(LeAudioSrcWedApp);
}

bool get_src_wed_vaild(void)
{
	return LeAudioFlagGet(LeAudioSrcWedApp);
}

void set_sink_wed_vaild(bool wed_vaild)
{
	if(wed_vaild)
		LeAudioFlagRegister(LeAudioSinkWedApp);
	else
		LeAudioFlagDeregister(LeAudioSinkWedApp);
}

bool get_sink_wed_vaild(void)
{
	return LeAudioFlagGet(LeAudioSinkWedApp);
}

void set_sink_sync_lost(bool sync_lost)
{
	if(sync_lost)
		LeAudioFlagRegister(LeAudioSinkSyncLost);
	else
		LeAudioFlagDeregister(LeAudioSinkSyncLost);
}

bool get_sink_sync_lost(void)
{
	return LeAudioFlagGet(LeAudioSinkSyncLost);
}

void set_src_sync(bool src_sync)
{
	if(src_sync)
		LeAudioFlagRegister(LeAudioSrc1Sync);
	else
		LeAudioFlagDeregister(LeAudioSrc1Sync);
}

bool get_src_sync(void)
{
	return LeAudioFlagGet(LeAudioSrc1Sync);
}

void set_decoder_reinit(bool reinit)
{
	if(!reinit)
		LeAudioFlagRegister(LeAudioDecoderInit);
	else
		LeAudioFlagDeregister(LeAudioDecoderInit);
}

bool get_decoder_reinit(void)
{
	return !LeAudioFlagGet(LeAudioDecoderInit);
}

void set_encoder_reinit(bool reinit)
{
	if(!reinit)
		LeAudioFlagRegister(LeAudioEncoderInit);
	else
		LeAudioFlagDeregister(LeAudioEncoderInit);
}

bool get_encoder_reinit(void)
{
	return !LeAudioFlagGet(LeAudioEncoderInit);
}

void set_sink_dac_pcm_invalid(bool flag)
{
	if(flag)
		LeAudioFlagRegister(LeAudioSinkDacInvalid);
	else
		LeAudioFlagDeregister(LeAudioSinkDacInvalid);
}

bool get_sink_dac_pcm_invalid_flag(void)
{
	return LeAudioFlagGet(LeAudioSinkDacInvalid);
}

bool get_wed_vaild(void)
{
//	if(Get_bis_role() == BIS_SINK)
//		return get_sink_wed_vaild();
	return get_src_wed_vaild();
}

bool le_bis_src_is_synced(void)
{
	return get_src_sync();
}

void set_decoder_cfg_flag(bool cfg)
{
	if(cfg)
		LeAudioFlagRegister(LeAudioDecoderCfgFlag);
	else
		LeAudioFlagDeregister(LeAudioDecoderCfgFlag);
}

bool get_decoder_cfg_flag(void)
{
	return LeAudioFlagGet(LeAudioDecoderCfgFlag);
}

void set_sink_scan_start_flag(uint8_t enable)
{
	if(enable)
		LeAudioFlagRegister(LeAudioSinkScanStartFlag);
	else
		LeAudioFlagDeregister(LeAudioSinkScanStartFlag);
}

uint8_t get_sink_scan_start_flag(void)
{
	return LeAudioFlagGet(LeAudioSinkScanStartFlag);
}
#if BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT
void Set_cis_role(uint8_t role)
{
	cis_role = role;
}

uint8_t Get_cis_role(void)
{
	return cis_role;
}

void Set_cis_app(uint8_t cis_app)
{
	g_cis_app = cis_app;
}

uint8_t Get_cis_app(void)
{
	return g_cis_app;
}

void Set_cis_mode(uint8_t cis_mode)
{
	g_cis_mode = cis_mode;
}

uint8_t Get_cis_mode(void)
{
	return g_cis_mode;
}

void Set_sink_audio_loc(uint8_t audio_loc)
{
	g_audio_loc = audio_loc;
}

uint8_t Get_sink_audio_loc(void)
{
	return g_audio_loc;
}

uint8_t cis_app_get_rank(void)
{
    uint8_t rank;

	if (Get_sink_audio_loc() == GAF_LOC_FRONT_LEFT_BIT)
	{
		rank = 1;
	}
	else if (Get_sink_audio_loc() == GAF_LOC_FRONT_RIGHT_BIT)
	{
		rank = 2;
	}
	else
	{
		rank = 0; //invaild value
	}

    return (rank);
}

#if defined(LE_CIS_SRC) || defined(LE_CIS_SINK)
void cis_app_init(uint8_t cis_app, uint8_t sink_loc)
{
	Set_cis_app(cis_app);
	
	switch (cis_app)
	{
#ifdef LE_CIS_SINK
		case CIS_APP_HEADSET:
		{
			le_cis_sink_init(cis_app, sink_loc);
			le_cis_sink_datapath_init();
			le_cis_sink_2_datapath_init();
			le_cis_sink_in_datapath_init();
			Set_cis_role(CIS_SINK);
		} break;

		case CIS_APP_EARBUD:
		{
			le_cis_sink_init(cis_app, sink_loc);
			le_cis_sink_datapath_init();
			le_cis_sink_in_datapath_init();
			Set_cis_role(CIS_SINK);
		} break;
#endif
#ifdef LE_CIS_SRC
		case CIS_APP_PHONE:
		{
			le_cis_src_init(cis_app, 0);
			le_cis_source_datapath_init();
			le_cis_source_datapath_init_2();
			le_cis_src_out_datapath_init();
			Set_cis_role(CIS_SRC);
		} break;

		case CIS_APP_MEDIA_PLAYER:
		{
			le_cis_src_init(cis_app, 0);
			le_cis_source_datapath_init();
			le_cis_source_datapath_init_2();
			le_cis_src_out_datapath_init();
			Set_cis_role(CIS_SRC);
		} break;
#endif
		default:
			break;
	}
}
#endif
#endif

#else
bool get_wed_vaild(void)
{
	return 0;
}

bool get_src_wed_vaild(void)
{
	return 0;
}

bool get_sink_wed_vaild(void)
{
	return 0;
}
#endif
