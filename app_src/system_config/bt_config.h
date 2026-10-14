///////////////////////////////////////////////////////////////////////////////
//               Mountain View Silicon Tech. Inc.
//  Copyright 2012, Mountain View Silicon Tech. Inc., Shanghai, China
//                       All rights reserved.
//  Filename: bt_config.h
//  maintainer: KK
///////////////////////////////////////////////////////////////////////////////
#ifndef __BT_DEVICE_CFG_H__
#define __BT_DEVICE_CFG_H__

#include "chip_config.h"

#define ENABLE							(1)
#define DISABLE							(0)

/*****************************************************************
 * 蓝牙开关   A系列芯片不支持BT
 *****************************************************************/
#if !defined(CHIP_PIN_RF)
	#define	FUNC_RF_EN					0
#else
	#define	FUNC_RF_EN					1
#endif

//bt功能开关，需要和 FUNC_RF_EN 组合一起配置
#define	FUNC_ENABLE						(ENABLE  && FUNC_RF_EN)
#define	FUNC_DISABLE					DISABLE

/*****************************************************************
 * 蓝牙功能开关
 *****************************************************************/
#define FUNC_BT_SUPPORT					//经典蓝牙功能
//#define FUNC_BLE_SUPPORT				//BLE功能
//#define FUNC_BIS_SUPPORT				//BIS功能（需和BLE功能同时开启）
//#define FUNC_BT_TWS_SUPPORT			//TWS功能
//#define FUNC_BT_MULTI_LINK_SUPPORT	//经典蓝牙双手机连接

/*****************************************************************
 * 蓝牙模块配置(经典蓝牙BT和低功耗蓝牙BLE模块)
 *****************************************************************/
#ifdef FUNC_BT_SUPPORT
	#define BT_SUPPORT			        FUNC_ENABLE
	#if(BT_SUPPORT == FUNC_ENABLE)
	/*****************************************************************
	 * @brief 蓝牙统一控制宏
	 * @note 启用后提供统一的蓝牙开关接口 BtSetPowerState(bool enable)
	 *       同时控制：RF 射频、回连(TWS回连)、可见性
	 * 打开宏提供蓝牙开关接口,调用消息MSG_BT_ENABLED/MSG_BT_DISABLED打开或者关闭
	 *****************************************************************/
	//#define BT_UNIFIED_CONTROL
	#endif
#else
	#define BT_SUPPORT					FUNC_DISABLE
#endif

#ifdef FUNC_BLE_SUPPORT
	#define BLE_SUPPORT					FUNC_ENABLE
	//#define MV_APP_CONTROL
	//#define MVAPP_UPDATA_EN
#else
	#define BLE_SUPPORT					FUNC_DISABLE
#endif

/*****************************************************************
 * 外部射频模块配置\
 * @note 1.外部PA/LNA模块需要在硬件上连接好,并且在软件中配置好对应的GPIO口
 *       2.根据芯片手册，配置合适的芯片发射功率值(BT_TX_POWER_LEVEL/BT_PAGE_TX_POWER_LEVEL),防止射频模块过载
 *****************************************************************/
#define EXTERNAL_PA_LNA					FUNC_DISABLE	

/* "sys_param.h" 蓝牙基础参数*/
/*****************************************************************
 * 蓝牙频偏参数
 *****************************************************************/
#define BT_DEFAULT_TRIM					0x06	//trim范围:0x00~0x0f
#define BT_MIN_TRIM						0x0		//trim最小值
#define BT_MAX_TRIM						0xf		//trim最大值

/*****************************************************************
 * 蓝牙通用功能配置
 *****************************************************************/
//#define BT_SNIFF_ENABLE				//sniff功能开关
//#define BT_SNIFF_DEEPSLEEP_ENABLE		//在sniff模式允许进入deepsleep
#if defined(CHIP_PIN_PA_VDD)
	#define BT_HIGH_RF_TXPOWER_ENABLE		//RF Transmit use high transmit mode
#endif

/*****************************************************************
 * 蓝牙名称注意事项:
 * 1.蓝牙名称支持中文,需要使用URL编码,开启bt_name.h,屏蔽BT_NAME定义
 * 2.BLE的名称修改在ble广播数据中体现(ble_app_func.c)
 *****************************************************************/
//#include "bt_name.h"
#define BT_NAME							"B6_BT"
#define BLE_NAME						"B6_BLE"

#ifdef BT_SUPPORT
//	#define SYS_PARAM_BT_NAME			//BT name 蓝牙名称从flash中获取
#endif

#ifdef BLE_SUPPORT
	//Note:如果需要修改BLE广播包中的设备名称,请打开此宏,并且在sys_parameter.ble_LocalDeviceName中设置需要的名称，长度不能超过31字节
//	#define SYS_PARAM_BLE_NAME			//BLE name 蓝牙名称从flash中获取
#endif

//蓝牙发射功率配置
 /* set rf tx power max level(max: 13 -- 10 or 13dbm according pa_vdd voltage)
  * Rf Tx Power Range, max:10-13 dbm, step:1dbm/step
{   level  dbm
	[13] = 13,  //10 dbm pa-vdd:1.8v; 13 dbm pa-vdd:2.5v
	[12] = 12,
	[11] = 11,
	[10] = 10,
	[9] = 9,
	[8] = 8,
	[7] = 7,
	[6] = 6,
	[5] = 5,
	[4] = 4,
	[3] = 3,
	[2] = 2,
	[1] = 1,
	[0] = 0,
}*/
#if (EXTERNAL_PA_LNA == FUNC_ENABLE)
	//仔细阅读射频芯片手册,根据外部PA/LNA的工作电压和芯片的发射功率配置合适的值,防止射频模块过载
	#define BT_TX_POWER_LEVEL				4		//蓝牙正常工作时发射功率
	#define BT_PAGE_TX_POWER_LEVEL			2		//蓝牙回连发射功率
#else
	#define BT_TX_POWER_LEVEL				13		//蓝牙正常工作时发射功率
	#define BT_PAGE_TX_POWER_LEVEL			8		//蓝牙回连发射功率(减小上电时蓝牙回连的干扰声)
#endif

#define BLE_TX_POWER_LEVEL				13

enum
{
	BT_BACKGROUND_FAST_POWER_ON_OFF = 0,		//0 -> BT后台不能连接手机
	BT_BACKGROUND_POWER_ON = 1,					//1 -> BT后台可以连接手机
	BT_BACKGROUND_DISABLE = 2,					//2 -> 无后台
};
#if BT_SOURCE_SUPPORT
	#define SYS_BT_BACKGROUND_TYPE		BT_BACKGROUND_POWER_ON				//BT 后台设置
#else
	#define SYS_BT_BACKGROUND_TYPE		BT_BACKGROUND_FAST_POWER_ON_OFF		//0 -> BT后台不能连接手机
#endif

//#define FLASH_SAVE_REMOTE_BT_NAME		//是否需要记忆已连设备的蓝牙名称

//蓝牙简易配对功能
#define BT_SIMPLEPAIRING_FLAG		TRUE 	//0:use pin code; 1:simple pairing
#define BT_PINCODE					"0000"	//简易配对设置为0,则通过PINCODE进行连接
// #define BT_NUMERICAL_CONFIRMATION_ENABLE	//数值比对功能开关,开启后,在配对时会有一个数值显示在手机上,需要用户确认是否匹配成功

#define BT_ADDR_SIZE				6
#define BT_LSTO_DFT					8000    //连接超时时间 		//timeout:8000*0.625=5s
#define BT_PAGE_TIMEOUT				8000	//page timeout	//timeout:8000*0.625=5s  

//蓝牙异步时钟采样率微调配置
#define	CFG_PARA_BT_SYNC					//BtPlay 异步时钟 采样点同步
#define CFG_PARA_HFP_SYNC					//通话 异步时钟 采样点同步

/*****************************************************************
 * 经典蓝牙协议宏定义开关
 *****************************************************************/
#define BT_A2DP_SUPPORT				FUNC_ENABLE		//音乐播放
#define BT_AVRCP_SUPPORT			FUNC_ENABLE		//音乐控制
#define BT_HFP_SUPPORT				FUNC_ENABLE		//通话功能
#define BT_SPP_SUPPORT				FUNC_DISABLE	//数据传输
#define BT_OBEX_SUPPORT				FUNC_DISABLE	//文件传输(适用于安卓手机通过文件推送进行OTA升级)
#define BT_PBAP_SUPPORT				FUNC_DISABLE	//电话本
#define BT_HID_SUPPORT				FUNC_DISABLE	//HID
#define BT_MFI_SUPPORT				FUNC_DISABLE	//MFI功能
#define BT_USER_SPP_SUPPORT			FUNC_DISABLE	//自定义UUID服务数据传输功能
#define BT_DID_SUPPORT				FUNC_ENABLE		

#if BT_SOURCE_SUPPORT && BT_HFP_SUPPORT
	#define BT_HFG_SUPPORT				(1)		//语音发射(暂只支持CVSD格式--8KHz)
#else
	#define BT_HFG_SUPPORT				(0)
#endif

/*****************************************************************
 * BLE 功能配置
 * 1. BLE Peripheral 数据传输功能 开关
 * 2. BLE Central And Peripheral (主从一体): 扫描+广播 分时复用
 *****************************************************************/
#if (BLE_SUPPORT == ENABLE)
	#define BLE_LINK_SUPPORT			(1)		//BLE数据传输功能开关,默认开启

	#define BLE_DUAL_ROLE 				DISABLE //BLE主从一体开关

#if (BLE_LINK_SUPPORT)
	//#define BLE_TUYA_ENABLE					//涂鸦功能	请参考B6涂鸦功能说明文档
#endif

#else
	#define BLE_LINK_SUPPORT			(0)
#endif

/*****************************************************************
 * LE AUDIO 功能配置
 *****************************************************************/
#define LE_CIS_SUPPORT					(1)
#define LE_BIS_SUPPORT					(2)

#if BLE_SUPPORT == ENABLE
	#ifdef FUNC_BIS_SUPPORT
		#define BT_LE_AUDIO_SUPPORT		LE_BIS_SUPPORT
	#else
		#define BT_LE_AUDIO_SUPPORT		(0)
	#endif
#else
	#define BT_LE_AUDIO_SUPPORT			(0)
#endif

	#define LE_AUDIO_ROLE				CIS_NONE

/*****************************************************************
 *****************************************************************/
//#define BT_AUTO_ENTER_PLAY_MODE	//在非蓝牙模式下,后台蓝牙连上手机,播放音乐自动切换到播放模式

#define	BT_ACCESSBLE_NONE					0		//0 -> 不可见不可连接
#define	BT_ACCESSBLE_DISCOVERBLEONLY		1		//1 -> 可见不可连接
#define	BT_ACCESSBLE_CONNECTABLEONLY		2		//2 -> 不可见可连接
#define	BT_ACCESSBLE_GENERAL				3		//3 -> 可见可连接

#define POWER_ON_BT_ACCESS_MODE_SET  BT_ACCESSBLE_GENERAL	//经典蓝牙上电开机的可见性配置

#if (POWER_ON_BT_ACCESS_MODE_SET == BT_ACCESSBLE_GENERAL)
//	#define BT_ACCESS_MODE_SET_BY_POWER_ON_TIMEOUT			//开机蓝牙可见性设置延时,按键MSG_BT_OPEN_ACCESS消息打开可见性
 	#define BT_VISIBILITY_DELAY_TIME		10000			//单位:ms  开机蓝牙可见性设置延时时间
#endif

/*****************************************************************
 * 连接链路参数配置
 * 包含作为蓝牙SINK,支持双手机连接配置
 *****************************************************************/
#if (BT_SUPPORT == ENABLE)
	#ifdef FUNC_BT_MULTI_LINK_SUPPORT
		#define BT_MULTI_LINK_SUPPORT			//双手机连接
	#endif

	#ifdef BT_MULTI_LINK_SUPPORT
		#define LAST_PLAY_PRIORITY				//后播放优先
//		#define BT_LINK_2DEV_ACCESS_DIS_CON		//开启后,第一个手机连上后,第二个手机需要能搜索到; 关闭后,第二个手机搜索不到,能回连上
	#endif
#endif

/*****************************************************************
 * 蓝牙协议连接链路参数配置
 * 不要随意修改
 *****************************************************************/
#ifdef BT_MULTI_LINK_SUPPORT
	#define BT_LINK_DEV_NUM				2 	//蓝牙连接手机个数 (1 or 2)
	#define BT_DEVICE_NUMBER			2	//蓝牙ACL连接个数 (1 or 2)
	#define BT_SCO_NUMBER				2	//蓝牙通话链路个数 (1 or 2),BT_SCO_NUMBER必须小于BT_DEVICE_NUMBER
#else
	#define BT_LINK_DEV_NUM				1
	#define BT_DEVICE_NUMBER			1
	#define BT_SCO_NUMBER				1
#endif

/*****************************************************************
 * A2DP profile
 *****************************************************************/
#if BT_A2DP_SUPPORT

	//#define BT_AUDIO_AAC_ENABLE		//AAC解码开关
	//Note1:开启AAC,需要同步开启解码器类型USE_AAC_DECODER(app_config.h)
	//Note2:目前双手机配置跟AAC解码不能同时打开
	#if defined(BT_PROFILE_BQB_ENABLE) && !defined(BT_AUDIO_AAC_ENABLE)
		#define BT_AUDIO_AAC_ENABLE
	#endif

	#if defined(BT_AUDIO_AAC_ENABLE) && !defined(USE_AAC_DECODER)
		#define USE_AAC_DECODER
	#endif
	#if (defined(BT_AUDIO_AAC_ENABLE) && defined(BT_MULTI_LINK_SUPPORT))
		#error Conflict: BT_AUDIO_AAC_ENABLE and BT_MULTI_LINK_SUPPORT setting error
	#endif
#endif /* BT_A2DP_SUPPORT */

/*****************************************************************
 * AVRCP profile
 *****************************************************************/
#if (BT_AVRCP_SUPPORT)

	#define BT_AVRCP_VOLUME_SYNC			(0)		//音量同步功能开关

	//(eg:EQ/repeat mode/shuffle/scan configuration)
	#define BT_AVRCP_PLAYER_SETTING			(0)		//歌曲的循环模式(只支持苹果手机自带播放器)

	#define BT_AVRCP_SONG_PLAY_STATE		(0)		//歌曲播放状态和时间获取

	#define BT_AVRCP_SONG_TRACK_INFOR		(0)		//歌曲和歌词信息获取(尽量和歌曲播放状态同时使用)

	#define BT_AUTO_PLAY_MUSIC				(0)		//AVRCP连接成功后，自动播放歌曲

#endif /* BT_AVRCP_SUPPORT */

/*****************************************************************
 * get remote device time
 * android: map
 * ios:hfp
 *****************************************************************/
//#define BT_GETTIME_SUPPORT

/*****************************************************************
 * HFP profile
 *****************************************************************/
enum
{
	USE_NULL_RING = 0,						//0 -> 不支持来电铃声
	USE_NUMBER_REMIND_RING = 1,				//1 -> 来电报号和铃声
	USE_LOCAL_AND_PHONE_RING = 2,			//2 -> 使用手机铃声，若没有则播本地铃声
	USE_ONLY_LOCAL_RING = 3,				//3 -> 强制使用本地铃声
};
#define SYS_DEFAULT_RING_TYPE				USE_LOCAL_AND_PHONE_RING	//蓝牙通话来电铃声配置

#if BT_HFP_SUPPORT

	#define BT_HFP_SUPPORT_WBS				(1)	//0: only cvsd   1: cvsd + msbc

//	#define BT_HFP_BATTERY_SYNC					//电池电量同步(本地电量同步到手机端显示) (开启需要和 CFG_FUNC_POWER_MONITOR_EN 关联)

	#define BT_REMOTE_AEC_DISABLE				//关闭手机端AEC

	#define BT_HFP_MIC_PGA_GAIN				15  //ADC PGA GAIN +18db(0~31, 0:max, 31:min)

	#define BT_HFP_CALL_DURATION_DISP			//来电通话时长配置选项

//	开启HFP连接，不使能进入HFP相关模式(推送电池电量显示)
//	该应用适用于某些需要连接HFP协议，但是不需要HFP相关功能的场合
//	#define BT_HFP_MODE_DISABLE

#endif /* BT_HFP_SUPPORT */

/*****************************************************************
 * 蓝牙回连参数配置
 *****************************************************************/
#define BT_RECONNECTION_SUPPORT			(1)		// 开启自动重连(开机或者切换模式)
#define BT_POR_TRY_COUNTS				(5)		// 开机重连尝试次数
#define BT_POR_INTERNAL_TIME			(3)		// 开机重连每两次间隔时间(in seconds)

#define BT_BLR_RECONNECTION_SUPPORT		(1)		// 开启BB Lost(连接丢失)重连
#define BT_BLR_TRY_COUNTS				(90)	// BB Lost 尝试重连次数
#define BT_BLR_INTERNAL_TIME			(5)		// BB Lost 重连每两次间隔时间(in seconds)


/*****************************************************************
 *
 * OBEX config (mva通过obex进行升级,升级方式:双bank)
 * 使用说明：
 * 1. chip_config.h中,需要注意flash容量,如芯片使用16Mb,要通过OBEX实现双BANK升级,则需要将CHIP_FLASH_CAPACTITY配置为8Mb
 * 2. OBEX双bank升级,数据缓存ram需要63KB,需要注意下
 * 
 *****************************************************************/
#if BT_OBEX_SUPPORT
// 	#define MVA_BT_OBEX_UPDATE_FUNC_SUPPORT		//Obex协议进行蓝牙双BANK升级功能
#endif /*BT_OBEX_SUPPORT*/

/*****************************************************************
 * TWS 功能配置
 *****************************************************************/
#ifdef FUNC_BT_TWS_SUPPORT
	#define BT_TWS_SUPPORT				FUNC_ENABLE	// TWS function switch
#else
	#define BT_TWS_SUPPORT				DISABLE		// TWS function switch
#endif

#if (BT_TWS_SUPPORT == ENABLE)

	#define TWS_DAC0_OUT						//DAC相位对齐

	#define CFG_TWS_SAMPLE_RATE			44100	//需要和SDK关联,后续待处理

	#define BT_TWS_LINK_NUM				1		//TWS参数仅能为1， 不可修改
	#define CFG_TWS_ONLY_IN_BT_MODE		DISABLE //enable:仅在蓝牙模式下工作, disable:在所有模式下工作
	#define TWS_STATRT_PLAY_FRAM		56*2    //取值范围[56*1 - 56*4]; 对应music通路的delay时间是56*3*[1 - 4] = [168 - 672]ms；此值越大，主副箱距离越远
	#define CFG_CMD_DELAY				10      //ms 主从CMD发送到处理延时5ms左右，信道质量差时更长，考虑被打断会形成7~20mS延时，此处设10ms较合适。


	//TWS组网条件判断
	#define TWS_FILTER_NAME						//过滤名称
//	#define TWS_FILTER_USER_DEFINED				//自定义过滤条件(max:6bytes)
	#ifdef TWS_FILTER_USER_DEFINED
		#define TWS_FILTER_INFOR		"TWS-MV" //(length:6bytes) //默认定义为字符串,假如是定义参数,请移步到配置处自行赋值
	#endif

	#define CFG_TWS_ROLE_RANDOM			0		//适用于双键组网		主从角色在组网前随机
	#define CFG_TWS_PEER_MASTER			3		//对箱主				按键发起配对的机器组网成功后默认就是MASTER
	#define CFG_TWS_PEER_SLAVE			4		//对箱从				按键发起配对的机器组网成功后默认就是SLAVE
	#define CFG_TWS_BLE_PAIR			5		//单键 BLE协助组网		此配置必须打开 BLE_SUPPORT和BLE_DUAL_ROLE
	/*
	 *  对箱方案
	 *  对箱组网配对方式选择
	 *  选择除 CFG_TWS_ROLE_MASTER, CFG_TWS_ROLE_SLAVE 外配置
	 */
	#define TWS_PAIRING_MODE			CFG_TWS_PEER_MASTER
	#if TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER
		#define CFG_TWS_PAIR_DOUBLEKEY			0	//0:双键组网		1:单键组网
	#endif

//	#define TWS_AUDIO_BUFFER_DYNAMIC_MALLOC

	//slave连接成功后自动切换到tws_slave模式
	#if(TWS_PAIRING_MODE != CFG_TWS_PEER_SLAVE)
		#define CFG_AUTO_ENTER_TWS_SLAVE_MODE
	#endif

	/*
	 * 打开表示从机连上TWS后能够切模式 (需同时打开后台蓝牙)
	 * 关闭表示从机连上TWS后不能切模式 (需同时打开后台蓝牙)
	 */
//	#define TWS_SLAVE_MODE_SWITCH_EN

	/*
	 * 打开表示支持TWS主从同步关机；
	 * 关闭表示单箱关机，哪个音箱按键关机就关哪个音箱
	 */
	#define TWS_POWEROFF_MODE_SYNC

#else

	#define BT_TWS_LINK_NUM				0		//不开起TWS功能,只能将此参数配置为0

#endif

/*
 * TWS 主从之间音量控制同步
 */
#define TWS_VOL_SYNC_SUPPORT							(1)

/*
 ENABLE 表示手机连接时，该音箱可以发起组队
 DISABLE 表示手机连接时，该音箱无法发起组队
 */
#define TWS_PAIRING_WHEN_PHONE_CONNECTED_SUPPORT		(1)

/*
 ENABLE 表示用户主动断开TWS配对后，下次开机不能自动回连
 DISABLE 表示用户主动断开TWS配对后，下次开机能再回连
 */
#define TWS_CONNECTED_WHEN_ACTIVE_DISCONNECTION_SUPPORT	(0)

/*****************************************************************
 * TWS回连参数配置
 *****************************************************************/
#define TWS_RECONNECTION_SUPPORT		(0)			// 开启自动重连(开机或者切换模式)
#define TWS_POR_TRY_COUNTS				(3)			// 开机重连尝试次数
#define TWS_POR_INTERNAL_TIME			(3)			// 开机重连每两次间隔时间(in seconds)

#define TWS_BLR_RECONNECTION_SUPPORT	(0)			// 开启tws BB Lost(连接丢失)重连
#define TWS_BLR_TRY_COUNTS				(3)			// tws BB Lost 尝试重连次数
#define TWS_BLR_INTERNAL_TIME			(5)			// tws BB Lost 重连每两次间隔时间(in seconds)


/*****************************************************************
 * 2.4G功能配置
 *****************************************************************/
#if WIRELESS_SUPPORT  && !(BLE_SUPPORT || BT_SOURCE_SUPPORT)
	#include "wireless_config.h" //仅在此引用
	#define  CFG_WIRELESS_EN	//for App config
#endif

/*****************************************************************
 * BLE 地址配置说明
 * 1.BT 关闭，使用BLE固定地址
 * 2.BIS或者CIS功能打开需要ctkd流程，BLE和BT地址配置为一样,ble广播使用随机地址
 * 3.在其他场景下，BLE和BT使用不同的地址，BLE 使用固定的地址广播
 * BIS或者CIS开启后不需要兼容手机客户可修改为ble和BT使用不同地址
 *****************************************************************/
#if ((BLE_SUPPORT && (!BT_LE_AUDIO_SUPPORT)) || (!BT_SUPPORT))
   #define BLE_USE_DIFF_ADDR_THAN_BT
#endif


#endif /*__BT_DEVICE_CFG_H__*/
