/*
 * wireless2.h
 *
 *  Created on: 2023-11-16 15：20
 *      Author: tony
 */

#ifndef WIRELESS2_H_
#define WIRELESS2_H_

#include "type.h"
#include "wireless2bb_api.h"

//#define MVW2_LIB_VERSION	"1.3.86"
#define MVW2_LIB_VERSION	"0.0.1"

#define NOPAIR_WORD					0xFFFFFFFF
#define COMPANY_BYTE3				0x38
#define COMPANY_BYTE2				0x65
//bit0：dev1 paired; bit1: dev2 paired @ DEVICE1_MASK DEVICE2_MASK
typedef unsigned char 	(*PairedFlagGetCallback)(void);
//bit0：dev1 paired; bit1: dev2 paired @ DEVICE1_MASK DEVICE2_MASK
//return Paired: PairedInfo ;unpaired: LinkKey
typedef uint32_t 		(*PairInfoGetCallback)(unsigned char Device);
//Device: 1：Set dev1Info; 2:Set dev2Info @ DEVICE1_MASK DEVICE2_MASK
typedef bool 			(*PairedInfoSetCallback)(uint32_t Info, unsigned char Device);

typedef uint8_t (*PairChipTpyeGetCallback)(unsigned char Device);
typedef bool (*PairedChipTpyeSetCallback)(uint8_t Info, unsigned char Device);

typedef uint32_t (*SecondaryMwordGetCallback)();
typedef bool (*SecondaryMwordSetCallback)(uint32_t SecondaryMword);

typedef unsigned char 	(*Audio_Check1stFrameAllRight_fp)(unsigned char id,unsigned char cnt);
typedef unsigned char 	(*Audio_Check1stFrameAllRightStateGet_fp)(unsigned char id);
typedef void 		  	(*Audio_Check1stFrameAllRightCounterStart_fp)(unsigned char id);
typedef	void 			(*wireless_AudioParityCntStart_fp)(void);
typedef void 			(*wireless_AudioParityCntProc_fp)(void);

// result: 0 -- SUCC; 1-- FAILED
typedef void 			(*wireless_test_mode_xo_cal_result_cb)(unsigned char result, unsigned char mos_xi, unsigned char cap_xi);

// result: 0 -- SUCC; 1-- FAILED
typedef void 			(*wireless_user_cmd_tx_result_cb)(unsigned char result);

// call in app to regiter the rx data callback
typedef void 			(*wireless_user_cmd_rx_data_cb)(unsigned char result,unsigned char* data, unsigned char len);

extern PairedFlagGetCallback PairedFlagGetFunc;
extern PairInfoGetCallback PairInfoGetFunc;
extern PairedInfoSetCallback PairedInfoSetFunc;
extern PairChipTpyeGetCallback PairChipTpyeGetFunc;
extern PairedChipTpyeSetCallback PairedChipTpyeSetFunc;
extern SecondaryMwordGetCallback SecondaryMwordGetFunc;
extern SecondaryMwordSetCallback SecondaryMwordSetFunc;
extern uint32_t CompanyWord;//define @ App
extern uint32_t WirelessDeviceId; //define @App

//recv
extern Audio_Check1stFrameAllRight_fp	            Audio_Check1stFrameAllRight_cb;
extern Audio_Check1stFrameAllRightStateGet_fp		Audio_Check1stFrameAllRightStateGet_cb;
extern Audio_Check1stFrameAllRightCounterStart_fp	Audio_Check1stFrameAllRightCounterStart_cb;
//send
extern wireless_AudioParityCntStart_fp				wireless_AudioParityCntStart_cb;
extern wireless_AudioParityCntProc_fp				wireless_AudioParityCntProc_cb;

typedef struct Wireless2_param
{
	unsigned char  npack;
	unsigned char* rf_pbuffer;//Tx/Rx Buf
	unsigned int   rf_translen;//Tx Packet Len (fifo)
	unsigned int   au_audiolen;//foreward frame len - sublen
	unsigned int   sbc_len_backward;//backward frame len -sublen
	unsigned int   rf_interval;
}Wireless2_param_t;

typedef enum _wireless2_conn_chip_type
{
	WIRELESS2_CHIP_TYPE_B5X,
	WIRELESS2_CHIP_TYPE_G1X,
	WIRELESS2_CHIP_TYPE_B6X,
	WIRELESS2_CHIP_TYPE_UNKNOWN = 0x0f,
} WIRELESS2_CONN_CHIP_TYPE;

typedef enum _ota_mode_state_type
{
    MVWIRE2_OTA_MODE_STATE_IDLE_STATE, //  0
    MVWIRE2_OTA_MODE_STATE_OTA_INFORM, //  1
    MVWIRE2_OTA_MODE_STATE_M_WAIT_OTA_INFORM_REPLY, //  2
    MVWIRE2_OTA_MODE_STATE_M_S_ENTER_OTA_MODE, //  3
    MVWIRE2_OTA_MODE_STATE_M_SEND_START_OTA_CMD, //  4
    MVWIRE2_OTA_MODE_STATE_S_WAIT_START_OTA_CMD, //  5
    MVWIRE2_OTA_MODE_STATE_S_WAIT_APP_CONFIRM_START_OTA, //  6
    MVWIRE2_OTA_MODE_STATE_S_APP_ACCEPT_OTA_TX, //  7
    MVWIRE2_OTA_MODE_STATE_S_APP_REJECT_OTA_TX, //  8
    MVWIRE2_OTA_MODE_STATE_M_SEND_OTA_DATA_TX_CMD, // 9
    MVWIRE2_OTA_MODE_STATE_S_WAIT_OTA_DATA_TX_CMD, // 10
    MVWIRE2_OTA_MODE_STATE_S_OTA_FILE_RX_DONE, // 11
    MVWIRE2_OTA_MODE_STATE_M_OTA_FILE_TX_DONE,// 12
    MVWIRE2_OTA_MODE_STATE_OTA_TRX_TIMEOUT,// 13
} MVWIRE2_OTA_MODE_STATE;

//参数0关闭AFH;1打开AFH
void wireless2_set_afh(uint8_t enable);

/***Packet param set***/
void MVWIRE2_ParamInit(Wireless2_param_t* param);

//speed(0:1M速率;1:2M速率)
void set_swbb_speed(uint8_t speed);

/*SWBB_PHY_1M_PHY / SWBB_PHY_2M_PHY*/
void MVWIRE2_2T1R_Set_TxMode(unsigned char mode);

/*SWBB_MODE_1T1R / SWBB_MODE_2T1R / ....*/
void MVWIRE2_SetWorkMode(unsigned char work_mode);

// en coex mode
void MVWIRE2_en_coex(uint8_t mode);

//set Rf gain
//0 -- maxgain 6dBm
//15-- mingain 0dBm
void set_dig_gain(int gain);

//set pa,xmpcap,dig
void Rf_PaLnaFineGainSet(unsigned char param1,unsigned char param2,unsigned char param3);

// set supported tx chipset type  @WIRELESS2_CONN_CHIP_TYPE
void MVWIRE2_2T1R_Allowed_Conn_Chip(unsigned char type);

void MVWIRE2_2T1R_Set_Chn_Select_Mode(unsigned char mode);

void MVWIRE2_2T1R_Set_AFH_thres(signed char thres);

void MVWIRE2_2T1R_Set_AFH_policy(unsigned char  policy);

void wireless_2T1R_S_Reconnect(void);

uint16_t slave_2t1r_flash_get_m_chip_id(void);

// 1: found pairing device
// 0: not found new pairing device
uint8_t  MVWIRE2_2T1R_Check_Requesting_Device(uint16_t* device_id,int8_t *rssi);
// -1: failed setting allowed pairing device
// 0:  success
int8_t  MVWIRE2_2T1R_Set_Allowed_Pairing_Device(uint16_t device_id);

void  MVWIRE2_2T1R_Cancel_Pairing_Device(void);

// default is auto conn mode
void  MVWIRE2_2T1R_Disable_Auto_Conn_Mode(void);

// 0: -- user select mode,
// 1: -- auto conn mode,default mode
uint8_t  MVWIRE2_2T1R_Get_Conn_Mode(void);

void wireless_1tnr_slave_set_tx_slot(uint8_t idx);

void wireless_1tnr_slave_send_cmd(uint8_t en,uint8_t cmd,uint8_t param);

// 0 --- scan; 1--- paired; 2 --- connected
unsigned char wireless_slave_1tnr_get_conn_state(void);

void wireless_1tnr_master_send_cmd(uint8_t cmd);

void wireless_2t1r_master_send_cmd(uint8_t* cmd_data, uint8_t len, wireless_user_cmd_tx_result_cb cb);

void wireless_2t1r_slave_reg_cmd_data_cb(wireless_user_cmd_rx_data_cb cb);

void wireless_1tnr_master_start_pair(void);
void wireless_1tnr_master_stop_pair(void);

void MVWIRE2_1tnr_set_pair_mode(uint8_t en);

uint8_t MVWIRE2_1tnr_get_pair_mode_flag(void);

void MVWIRE2_1tnr_master_force_use_saved_id(uint8_t en);

uint8_t MVWIRE2_1tnr_master_get_force_use_saved_id_flag(void);

#define MVWIRE2_DEFAULT_ROLE			0
#define	MVWIRE2_MASTER_ROLE				1
#define MVWIRE2_SLAVER_ROLE				2
void MVWIRE2_DeviceRoleSet(unsigned char dev_id);
unsigned char MVWIRE2_DeviceGet(void);

// set test mode's freq offset, valid range is 0-93
// it must be called before wirelesss init function.
void MVWIRE2_Test_Mode_Set_Freq(unsigned char freq_offset);

/******************************************
 * @param id：0/1, Swbb linkID 注意：数值区别于DEVICE1_MASK/DEVICE2_MASK
 * @param role：Swbb master / Swbb slaver @MVWIRE2_DeviceGet()
 *********************************************/
void MVWIRE2_ConnectedCB(unsigned char id,unsigned char role);
void MVWIRE2_DisconnectedCB(unsigned char id,unsigned char role);

void wireless2_test_mode_stop(void);
bool wireless2_Check_Test_mode_Stopped(void);
void wireless2_test_mode_master_start(void);
void wireless2_test_mode_slave_start(wireless_test_mode_xo_cal_result_cb cb, uint32_t BB_Relocate);
void wireless2_test_calibrationread(uint8_t xi, uint8_t xo,uint8_t accurate, uint8_t length);
bool wireless2_test_calibrationget(void);
// enable remote sleep command handling in low layer,
// default it is disabled.
// en: 0 -- disable remote sleep command handling
//     1 -- enable remote sleep command handling
void wireless2_Enable_Remote_Sleep_Cmd(unsigned char en);

/**audiopacket retrans lock status reset*/
void wireless_transpacket_reset(void);

/**
 * @brief  传输包请求,应用层重构
 * @param  StepNum:packet tx重传计数器，
 * 			0x0：	unlock Frame sync；
 * 			1：		packet first request；
 * 			StepNum / RETRANS_CNT: Trans Packet Num
 * 			StepNum % RETRANS_CNT: Packet Retrans
 * @return TRUE: Ready;FALSE: not Ready
 */
bool Wireless_TransPacketIsReady(uint32_t StepNum);

/**
 * @brief  由audio模块有些应用回导致长延时，稳定后才会给到RF
 * @param
 * @return uint32_t: RF interval
 */
unsigned int wireless_AudioIgnorecntGet(void);

/**
 * @brief  Read Packet data,应用层重构
 * @param  data: Read buffer
 * @param  len: Read data len
 * @return unsigned int: Read data len @byte
 */
unsigned int Wireless_TransBufRead(unsigned char* data,unsigned int len);

void audio_PackAddheader(unsigned char* audio_encodeframe);

void audio_Pack2Frames(unsigned char* audio_encodeframe);

/*****
 * @param  BB_Relocate	Non 0：update BB EM addr; 0: not update BB EM addr,
 */
void wireless2_1_X_initfuncset(uint32_t BB_Relocate);

void wireless2_2_X_initfuncset(uint32_t BB_Relocate);

void wireless2_3_X_initfuncset(uint32_t BB_Relocate);

void wireless2_3_3_initfuncset(uint32_t BB_Relocate);

void wireless2_4_X_initfuncset(uint32_t BB_Relocate);

void wireless2_5_1_initfuncset(uint32_t BB_Relocate);

void wireless2_5_2_initfuncset(uint32_t BB_Relocate);

void wireless2_6_1_initfuncset(uint32_t BB_Relocate);

void Wireless8x1InitFunSet(uint32_t BB_Relocate);

void Wireless8x2InitFunSet(uint32_t BB_Relocate);

void wireless2_test_mode_initfuncset(uint32_t BB_Relocate);

void MVWIRE2_Init(void);

/*3-3, 5-2, 6-1,testmode暂不支持sleep/RFstop,4-1, 5-1不支持sleep*/
void wireless2_Sleep(void);
bool wireless2_GetSleep(void);
void wireless2_Active(void);
void wireless2_RfStop(void);
bool wireless2_CheckStopped(void);
void wireless2_Restart(void);
/**************************************/

void MvWireless2AdvModePairModeEn (uint8_t en);
void MvWireless2AdvModePairingScanEn(uint8_t en);
void MvWireless2AdvModeMasterStartPair(void);
void MvWireless2AdvModeMasterStopPair(void);
void MvWireless2AdvModeForceUseSavedId(uint8_t en);
void MvWireless2AdvModeFlashSetTokenId(uint16_t token_id);
uint8_t MvWireless2AdvModeGetConnState(void);
uint16_t MvWireless2AdvModeGenNewTokenId(void);

/**
 * 麦资源请求
 * return  0-- 抢麦失败  1--抢麦线路忙  2-- 抢麦进行中  3 -- 抢麦已成功
 */
uint8_t MvWireless2AdvModeResourceSch(void);

/**
 * 麦资源释放
 */
void MvWireless2AdvModeResourceRelease(void);

/**
 * 允许抢麦开关
 * return 0--不允许抢麦  1--允许抢麦
 */
uint8_t MvWireless2AdvModeResourceSchSwitch(void);

/**
 * 远程关闭接收机
 */
void MvWireless2AdvModeCloseReceiver(void);

/**
 * 获取工作状态
 * *role 1 -- Master  2-- Slave
 * *state 0 -- 初始化/扫描中  1-- 配对中  2-- 单讲工作中  3-- 抢占资源调度中   4-- 双讲工作中
 * return 0 -- 获取失败  1-- 获取成功
 */
uint8_t MvWireless2AdvModeGetWorkState(uint8_t *role,uint8_t *state);

/**
 * 获取双讲状态  0 --单向通话中  1--双向通话中
 */
uint8_t MvWireless2AdvModeGetSchState(void);

uint16_t wireless_CRC16_2(uint8_t * pData, uint16_t wDataLen);

void m_s_1tnr_flash_set_token_id(uint16_t token_id);

void MVWIRE2_1tnr_pairing_scan_en(uint8_t en);

// void wireless_2_x_Master_Pair_RssiHoldSet(uint8_t rssi_value);
// uint8_t wireless_2_x_Master_Pair_GetTxRssi(void);
void wireless_2_x_Master_Connection_GetTxRssi(uint8_t *rssi_value);
void wireless_2_x_MasterRSSIBuf_Set(uint8_t cnt,uint8_t *rssi,uint16_t *id);
void wireless_2_x_MasterGetRecvRSSI(uint8_t cnt,uint8_t *rssi,uint16_t *id);
void wireless_2_x_MasterClrRecvRSSI(void);
void wireless_2_x_MasterSetPairChipId(uint16_t id);
/********************************************
 * 	Debug Api
 *******************************************/
#define	TURNKEY_TAG						4_1
#define MVWIRE2_STR(W, X, Y)			W ##_ ##X ##_ ##Y
#define MVWIRE2_DBG(ROLE, TURNKEY)		MVWIRE2_STR(MVWIRE2_dbg, ROLE, TURNKEY)
#define MVWIRE2_DBG_FUNC				MVWIRE2_DBG(ROLE_TAG, TURNKEY_TAG)

void MVWIRE2_dbg_master_1_x(void);
void MVWIRE2_dbg_slaver_1_x(void);
void MVWIRE2_dbg_master_2_x(void);
void MVWIRE2_dbg_slaver_2_x(void);
void MVWIRE2_dbg_master_3_x(void);
void MVWIRE2_dbg_slaver_3_x(void);
void MVWIRE2_dbg_master_3_3(void);
void MVWIRE2_dbg_slaver_3_3(void);
void MVWIRE2_dbg_master_3_5(void);
void MVWIRE2_dbg_slaver_3_5(void);
void MVWIRE2_dbg_master_4_1(void);
void MVWIRE2_dbg_slaver_4_1(void);
void MVWIRE2_dbg_master_5_1(void);
void MVWIRE2_dbg_slaver_5_1(void);
void MVWIRE2_dbg_master_5_2(void);
void MVWIRE2_dbg_slaver_5_2(void);
void MVWIRE2_dbg_master_6_1(void);
void MVWIRE2_dbg_slaver_6_1(void);
#endif /* WIRELESS2_H_ */
