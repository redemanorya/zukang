/**
**************************************************************************************
* @file    keyscan.h
* @brief   keyscan.h
*
* @author  Long
* @version V1.0.0
*
* @Created: 2024/05/15 
*
* @copyright Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
**************************************************************************************
*/
#ifndef __KEYSCAN_H__
#define __KEYSCAN_H__ 
#include "type.h"

/**
* @addtogroup KeyScan
* @{
* @defgroup keyscan keyscan.h
* @{
*/

/**
 * @note KeyScan Key Definition:
 * KEYSCAN_KEY_0-KEYSCAN_KEY_31 value from KeyScan_GetValue0 [0:31]
 * KEYSCAN_KEY_32-KEYSCAN_KEY_35 value from KeyScan_GetValue1 [0:3]
 **/

/**
Key Matrix：(IO_pads=9, offset=0) key numbers would be 36

KeyScan[8] A18     -----|
                        |
KeyScan[7] A17    KEYSCAN_KEY_35 ------|
                        |                 |
KeyScan[6] A16    KEYSCAN_KEY_34---KEYSCAN_KEY_27 --------|
                        |                 |                 |
KeyScan[5] A15    KEYSCAN_KEY_33---KEYSCAN_KEY_26---KEYSCAN_KEY_20 ---------|
                        |                 |                 |                 |
KeyScan[4] A14    KEYSCAN_KEY_32---KEYSCAN_KEY_25---KEYSCAN_KEY_19---KEYSCAN_KEY_14 ----------|
                        |                 |                 |                 |                 |
KeyScan[3] A13    KEYSCAN_KEY_31---KEYSCAN_KEY_24---KEYSCAN_KEY_18---KEYSCAN_KEY_13---KEYSCAN_KEY_9 -----------|
                        |                 |                 |                 |                 |                |
KeyScan[2] A12    KEYSCAN_KEY_30---KEYSCAN_KEY_23---KEYSCAN_KEY_17---KEYSCAN_KEY_12---KEYSCAN_KEY_8---KEYSCAN_KEY_5 ------------|
                        |                 |                 |                 |                 |                |                |
KeyScan[1] A11    KEYSCAN_KEY_29---KEYSCAN_KEY_22---KEYSCAN_KEY_16---KEYSCAN_KEY_11---KEYSCAN_KEY_7---KEYSCAN_KEY_4---KEYSCAN_KEY_2 ------------|
                        |                 |                 |                 |                 |                |                |               |
KeyScan[0] A10    KEYSCAN_KEY_28---KEYSCAN_KEY_21---KEYSCAN_KEY_15---KEYSCAN_KEY_10---KEYSCAN_KEY_6---KEYSCAN_KEY_3---KEYSCAN_KEY_1---KEYSCAN_KEY_0 |
--------------------------------------------------------------------------------------------------------------------------------------------------
Key Matrix：(IO_pads=4, offset=2)   key numbers would be 6
KeyScan[5] A15(2)   ----|
                       |
KeyScan[4] A14(2)   KEYSCAN_KEY_5 -------|
                       |                |
KeyScan[3] A13(2)   KEYSCAN_KEY_4---KEYSCAN_KEY_2 --------|
                       |                  |              |
KeyScan[2] A12(2)   KEYSCAN_KEY_3---KEYSCAN_KEY_1---KEYSCAN_KEY_0 |
--------------------------------------------------------------------------------------------------------------------------------------------------
Key Matrix：(IO_pads=6, offset=0)  key numbers would be 15

KeyScan[5] A15(2)    ----|
                        |
KeyScan[4] A14(2)   KEYSCAN_KEY_14 -------|
                        |                 |
KeyScan[3] A13(2)    KEYSCAN_KEY_13---KEYSCAN_KEY_9 --------|
                        |                 |                |
KeyScan[2] A12(2)    KEYSCAN_KEY_12---KEYSCAN_KEY_8---KEYSCAN_KEY_5 ---------|
                        |                 |                |                |
KeyScan[1] A11(4)    KEYSCAN_KEY_11---KEYSCAN_KEY_7---KEYSCAN_KEY_4---KEYSCAN_KEY_2 ----------|
                        |                 |                |                |                |
KeyScan[0] A10(3)    KEYSCAN_KEY_10---KEYSCAN_KEY_6---KEYSCAN_KEY_3---KEYSCAN_KEY_1---KEYSCAN_KEY_0 |
其他情况参照上面示例类推。

**/

typedef struct _KeyScanCntxt
{
	uint8_t IO_pads;    // 2-9，numbers of IO used for keyscan，注意：2 <= offset + IO_pads <=9
	uint8_t offset;     // 0-7，Keysan的起始offset, 0--KeyScan[0] A1 1---KeyScan[1] A7 以此类推


	uint8_t strobe_cycle; // the value decide shake eliminating time,time unit = one scan cycle = (interval+
	uint8_t interver_cycle; // the value decide shake eliminating time,time unit = one scan cycle = (interval+

	uint16_t shake_time;

	uint32_t idle_cycle;

	uint8_t div_num;//max15

    bool rx_int_en;		//enable/disable interrupt；
    bool int_mode;//

} KeyScanCntxt;


/**
 * @brief   initialize keyscan module
 * @param  cntx: keyscan context @detail see KeyScanCntxt
 * @retval TRUE  : success
 * @retval FALSE : fail
 * @note   none
 */
bool KeyScan_Init(KeyScanCntxt *cntx);


/**
* @brief   enable keyscan module
* @param  void
* @return void
* @note   none
*/
void KeyScan_Enable(void);


/**
* @brief   disable keyscan module
* @param  void
* @return void
* @note   none
*/
void KeyScan_Disable(void);


/**
* @brief   clear keyscan interrupt flag
* @param  void
* @return void
* @note   none
*/
void KeyScan_ClrIntFlg(void);


/**
* @brief  interrupt pending flag
* @param  void
* @return interrupt pending flag
* @note   none
*/
bool KeyScan_GetIntFlg(void);


/**
* @brief   get keyscan value[31:0]
* @param  void
* @return uint32_t
* @note   none
*/
uint32_t KeyScan_GetValue0(void);


/**
* @brief   get keyscan value[35:32]
* @param  void
* @return uint32_t  4bit for value[35:32]
* @note   none
*/
uint32_t KeyScan_GetValue1(void);

#endif

/**
 * @}
 * @}
 */
 