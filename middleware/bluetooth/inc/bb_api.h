
#ifndef _BB_API_H_
#define _BB_API_H_


/***************************************************
 *
 **************************************************/
void Bluetooth_common_init(void* params);
void Bt_init(void* params);
void Ble_init(void);

/***************************************************
 * BT debug infor
 * 在BtStackServiceEntrance循环前调用
 **************************************************/
void BtDebugEnable(void);

void SetBluetoothMode(unsigned char mode);

/***************************************************
 *
 **************************************************/
void rw_main(void);
void rwip_reset(void);

/***************************************************
 * get frequence offset value
 **************************************************/
signed char GetFreqOffsetValue(void);

/***************************************************
 * set bt frequence trim value
 * input: value (0x00~0x1f)
 **************************************************/
void BtSetFreqTrim(unsigned char value);

/***************************************************
 * set bt link supervision timeout
 * input: timeout: 0x0001~0xffff  (default:0x7D00 -- 20s)
 * link supervision timeout = time * 0.625ms (Range: 0.625ms - 40.9s)
 **************************************************/
void BtSetLinkSupervisionTimeout(unsigned int timeout);

/***************************************************
 * get BIS link rssi value (Broadcast Isochronous Stream)
 * return: dBm (absolute signal strength), 0 : not synced or invalid
 **************************************************/
int8_t GetBisRssi(void);

/***************************************************
 * get BLE connection rssi value
 * return: dBm (absolute signal strength), 0 : not connected or invalid
 **************************************************/
int8_t GetBleConRssi(void);

/***************************************************
 * get BT classic ACL link rssi delta value
 * return: dBm (absolute signal strength), 0 : not connected or invalid
 **************************************************/
int8_t GetBtAclRssi(void);

/***************************************************
 * get the company ID of the remote device 
 **************************************************/
unsigned short GetRemoteCompanyID(void);

/***************************************************
 * set rf tx power
 * Set RF TX max power level and page TX power level.
 *
 * @param level     Normal TX power: 0 ~ 13 (0 ~ 13 dBm, step 1dBm).
 *                  Negative-power range: 0xF2 ~ 0xFF, mapped to
 *                  RF_QL_TX_PW_TBL[1] ~ [14]:
 *                    0xF2 = -12.85dBm, 0xF3 = -11.89dBm,
 *                    0xF4 = -10.89dBm, 0xF5 =  -9.90dBm,
 *                    0xF6 =  -9.14dBm, 0xF7 =  -8.24dBm,
 *                    0xF8 =  -7.37dBm, 0xF9 =  -6.54dBm,
 *                    0xFA =  -5.60dBm, 0xFB =  -4.62dBm,
 *                    0xFC =  -3.66dBm, 0xFD =  -2.70dBm,
 *                    0xFE =  -1.84dBm, 0xFF =  -0.81dBm.
 * @param pageLevel Page/back-connect TX power. Same ranges as level.
**************************************************/
void SetRfTxPwrMaxLevel(unsigned char level, unsigned char pageLevel);

/***************************************************
 * Set BLE TX max power level.
 *
 * @param tx_pwr   Normal range: 0 ~ 13 (0 ~ 13 dBm, step 1dBm).
 *                 Negative-power range: 0xF2 ~ 0xFF, mapped via
 *                 (int)tx_pwr - 256 and looked up by rf_txpwr_cs_get():
 *                   0xF2 = -14dBm → table[1] (-12.85dBm),
 *                   0xF3 = -13dBm → table[2] (-11.89dBm),
 *                   0xF4 = -12dBm → table[3] (-10.89dBm),
 *                   0xF5 = -11dBm → table[4] ( -9.90dBm),
 *                   0xF6 = -10dBm → table[5] ( -9.14dBm),
 *                   0xF7 =  -9dBm → table[6] ( -8.24dBm),
 *                   0xF8 =  -8dBm, 0xF9 = -7dBm, 0xFA = -6dBm,
 *                   0xFB =  -5dBm, 0xFC = -4dBm, 0xFD = -3dBm,
 *                   0xFE =  -2dBm, 0xFF = -1dBm.
 **************************************************/
void SetbleMaxtxpwr(uint8_t tx_pwr);

/***************************************************
 * get the rf chip id
 **************************************************/
unsigned short GetRfChipId(void);


/***************************************************
 * get the rf pll lock state
 * return: -1 = pll unlock(error)
 * 0 = ok
 **************************************************/
signed char GetRfPllLockState(void);

/***************************************************
 * rf power Up/Down
 **************************************************/
void RF_PowerDownByHw(void);
void RF_PowerUpBySw(void);
void RF_PowerDownBySw(void);

/***************************************************
 * sniff use Hosc/RC
 **************************************************/
void sniff_rc_init_set();//rc initial set
void sniff_cntclk_set(signed char set);//0:32K 1:32768Hz -- bluetooth count clk set

//user api
void BtCntClkSet(void);

/***************************************************
 * select high pa mode
 **************************************************/
void Bt_select_high_pa_mode(unsigned char pa_mode);

/***************************************************
 * memory malloc for kernel
 **************************************************/
void *ke_malloc_user(unsigned int size, unsigned char type);

#endif //_BB_API_H_
