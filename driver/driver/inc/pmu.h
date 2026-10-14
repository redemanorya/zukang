/**
 **************************************************************************************
 * @file    pmu.h
 * @brief	pmu module driver interface
 *
 * @author  tony
 * @version V1.0.7
 *
 * @Created: 201-4-16
 *
 * @Copyright (C) 2014, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
 
 /**
 * @addtogroup PMU
 * @{
 * @defgroup pmu pmu.h
 * @{
 */

#ifndef __PMU_H__
#define __PMU_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "type.h"
#include "gpio.h"

/* Typedef -------------------------------------------------------------------*/

typedef enum _OSC_CLK_SEL
{
	LOSC_ANA_MODE = 0,
	LOSC_DIG_MODE = 1
}OSC_CLK_SEL;

typedef enum _POWERUP_EVENT_SEL
{
	GPIO_C012_EVENT     = (4),
	POWERKEY_EVENT      = (8)
}POWERUP_EVENT_SEL;

typedef enum _POWERKEY_PULLUP_SEL
{
	SEL_22K   = (0),
	SEL_650K  = (1),
}POWERKEY_PULLUP_SEL;

typedef enum _OSC_DIRECT_INPUT_SEL
{
	 OSC_USE = 0,
	 EXTERNAL_DIRECT_INPUT_USE = 1
}OSC_DIRECT_INPUT_SEL;

typedef enum _POWERKEY_SWITCH_MODE
{
	HARD_MODE = 0,
	SOFT_MODE = 1
}POWERKEY_SWITCH_MODE;

typedef enum _POWER_LONG_RESET_MODE
{
	LONGR_RST_MODE_TIMEOUT_KEYUP = 0,
	LONGR_RST_MODE_TIMEOUT = 1
}POWER_LONG_RESET_MODE;

typedef enum _HARD_TRIGGER_MODE
{
	LEVEL_TRIGGER = 0,
	EDGE_TRIGGER = 1
}HARD_TRIGGER_MODE;

typedef enum _POWERKEY_ACTIVE_LEVEL
{
	LOW_INDICATE_POWERON = 0,
	HIGH_INDICATE_POWERON = 1
}POWERKEY_ACTIVE_LEVEL;

typedef enum _POWERKEY_LONGORSHORT_PRESS_SEL
{
	POWERKEY_LONG_PRESS_MODE = 0,
	POWERKEY_SHORT_PRESS_MODE = 1
}POWERKEY_LONGORSHORT_PRESS_SEL;

typedef enum _POWERKEY_INT_SEL
{
	POWERKEY_TRIGLE_INT_NONE  = 0,
	POWERKEY_TRIGLE_INT 	  = 1,
	POWERKEY_SHORT_TRIGLE_INT = 2,
	POWERKEY_LONG_TRIGLE_INT  = 3
}POWERKEY_INT_SEL;

typedef enum _POWERKEY_RES_TUNNING
{
	PULLUP_22K,
	PULLUP_650K
}POWERKEY_RES_TUNNING;

typedef enum _GPIOC_POWERUP_PIN
{
	GPIOC_POWERUP_PIN_C0 = 0,
	GPIOC_POWERUP_PIN_C1,
	GPIOC_POWERUP_PIN_C2
}GPIOC_POWERUP_PIN;

typedef enum _GPIOC_POWERUP_TYPE
{
	GPIOC_POWERUP_TYPE_LEVEL = 0,//高唤醒
	GPIOC_POWERUP_TYPE_RISING_EDGE,
	GPIOC_POWERUP_TYPE_FALING_EDGE,
	GPIOC_POWERUP_TYPE_BOTH_EDGE
}GPIOC_POWERUP_TYPE;


/* Define --------------------------------------------------------------------*/


/* Variables -----------------------------------------------------------------*/


/* API -----------------------------------------------------------------------*/
/**
 * @brief   PMU register write enable
 * @param   None
 * @return  None
 * @note
 */
void PMU_WriteEnable(void);

/**
 * @brief   PMU register write disable
 * @param   None
 * @return  None
 * @note
 */
void PMU_WriteDisable(void);

/**
 * @brief   Trigger system entering power down
 * @param   None
 * @return  None
 * @note
 */
void PMU_SystemPowerDown(void);

/**
 * @brief   Enable automatic power up from passive power off state when power ready
 * @param   None
 * @return  None
 * @note
 */
void PMU_AutoPowerOnEnable(void);

/**
 * @brief   Disable automatic power up from passive power off state when power ready
 * @param   None
 * @return  None
 * @note
 */
void PMU_AutoPowerOnDisable(void);

/**
 * @brief   Check setting state of automatic power up mode
 * @param   None
 * @return  true:Enable; false:Disable;
 * @note
 */
bool PMU_IsAutoPowerOnEnable(void);

/**
 * @brief   Get first Power Up flag from register
 * @param   None
 * @return  true: Flag is set; false:Flag is not set;
 * @note
 */
bool PMU_GetRegFirstPowerUpFlag(void);


/**
 * @brief   Clear the first Power Up flag in register
 * @param   None
 * @return  None
 * @note
 */
void PMU_ClearRegFirstPowerUpFlag(void);

/**
 * @brief   Clean NVM mem when first power on
 * @param   None
 * @return  None
 * @note    This function should be called before PMU_NvmRead and PMU_NvmWrite.
 */
void PMU_NVMInit(void);

/**
 * @brief   Read NVM data
 * @param   Nvmoffset: the offset range is 0 - 127
 * @param   Buf: the memory address to store NVM data
 * @param   Length: data length, Nvmoffset + Length <= 128
 * @return  true:read success; false: read failed because of parameter error or PMU_NVMInit not be called firstly;
 * @note    
 */
bool PMU_NvmRead(uint8_t Nvmoffset, uint8_t* Buf, uint8_t Length);

/**
 * @brief   Write NVM data
 * @param   Nvmoffset: the offset range is 0 - 127
 * @param   Buf: the memory address reading data from
 * @param   Length: data length, Nvmoffset + Length <= 128
 * @return  true:write success; false: write failed because of parameter error or PMU_NVMInit not be called firstly;
 * @note    
 */
bool PMU_NvmWrite(uint8_t Nvmoffset, uint8_t* Buf, uint8_t Length);

/**
 * @brief   Get powerup event flag
 * @param   None
 * @return  CHARGE_IN_OUT_EVENT: charge insert or extract
 *          GPIO_C012_EVENT:     GPIO_C0/C1/C2
 *          POWERKEY_EVENT:      valid POWERKEY event
 * @note    
 */
POWERUP_EVENT_SEL PMU_PowerupEventGet(void);

/**
 * @brief   Clear powerup event flag
 * @param   None
 * @return  None
 * @note
 */
void PMU_PowerupEventClr(void);


/**
 * @brief   Enable Powerkey to SarADC path, Powerkey can be used as ADCKey
 * @param   None
 * @return  None
 * @note
 */
void PMU_PowerkeySarADCEn(void);

/**
 * @brief   Disable Powerkey to SarADC path
 * @param   None
 * @return  None
 * @note
 */
void PMU_PowerkeySarADCDis(void);

/**
 * @brief   Check if Powerkey to SarADC path is enabled
 * @param   None
 * @return  1:enabled; 0:disabled;
 * @note
 */
uint8_t PMU_PowerkeySarADCEnGet(void);

/**
 * @brief   Set Powerkey pullup through 110K resistance
 * @param   None
 * @return  None
 * @note
 */
void PMU_PowerkeyPullup110K(void);


/**
 * @brief   Set Powerkey pullup through 22K resistance
 * @param   None
 * @return  None
 * @note
 */
void PMU_PowerkeyPullup22K(void);

/**
 * @brief   Get Powerkey pullup type
 * @param   None
 * @return  SEL_650K: through 650K resistance; SEL_22K: through 22K resistance;
 * @note
 */
POWERKEY_PULLUP_SEL PMU_PowerkeyPullupOhmGet(void);

void PMU_GpioC0DebugModeSet(uint8_t set_sel);

void PMU_GpioC1DebugModeSet(uint8_t set_sel);

void PMU_GpioC2DebugModeSet(uint8_t set_sel);

/**
 * @brief   Clear Powerkey press trigger status flag
 * @param   None
 * @return  None
 * @note
 */
void PMU_PowerKeyStateClear(void);

/**
 * @brief   Clear Powerkey short press trigger status flag
 * @param   None
 * @return  None
 * @note
 */
void PMU_PowerKeyShortPressStateClear(void);

/**
 * @brief   Clear Powerkey long press trigger status flag
 * @param   None
 * @return  None
 * @note
 */
void PMU_PowerKeyLongPressStateClear(void);

/**
 * @brief   Set Powerkey switch mode
 * @param   PowerKeyMode: HARD_MODE or SOFT_MODE
 * @return  None
 * @note
 */
void PMU_PowerKeyModeSet(POWERKEY_SWITCH_MODE PowerKeyMode);

/**
 * @brief   Get Powerkey switch mode
 * @param   None
 * @return  HARD_MODE or SOFT_MODE
 * @note
 */
POWERKEY_SWITCH_MODE PMU_PowerKeyModeGet(void);

/**
 * @brief   Set Powerkey trigger type of hard switch mode
 * @param   HardTriggerMode: 0 - LEVEL_TRIGGER; 1 - EDGE_TRIGGER;
 * @return  None
 * @note
 */
void PMU_PowerKeyHardModeSet(HARD_TRIGGER_MODE HardTriggerMode);

/**
 * @brief   Get Powerkey trigger type settings of hard switch mode
 * @param   None
 * @return  LEVEL_TRIGGER or EDGE_TRIGGER
 * @note
 */
HARD_TRIGGER_MODE PMU_PowerKeyHardModeGet(void);

/**
 * @brief   Set Powerkey active level 
 * @param   PowerkeyActiveLevel: LOW_INDICATE_POWERON or HIGH_INDICATE_POWERON;
 * @return  None
 * @note
 */
void PMU_PowerKeyActiveLevelSet(POWERKEY_ACTIVE_LEVEL PowerkeyActiveLevel);

/**
 * @brief   Get settings of Powerkey active level 
 * @param   None
 * @return  LOW_INDICATE_POWERON or HIGH_INDICATE_POWERON
 * @note
 */
POWERKEY_ACTIVE_LEVEL PMU_PowerKeyActiveLevelGet(void);

/**
 * @brief   Set the startup trigger type of Powerkey when in powerdown status
 * @param   mode_set: POWERKEY_LONG_PRESS_MODE  - long press trigger startup
 *                    POWERKEY_SHORT_PRESS_MODE - short press trigger startup
 * @return  None
 * @note
 */
void PMU_PowerKeyLongOrShortPressSet(POWERKEY_LONGORSHORT_PRESS_SEL mode_set);

/**
 * @brief   Get settings of Powerkey startup trigger type
 * @param   None
 * @return  POWERKEY_LONG_PRESS_MODE or POWERKEY_SHORT_PRESS_MODE
 * @note
 */
POWERKEY_LONGORSHORT_PRESS_SEL PMU_PowerKeyLongOrShortPressGet(void);

/**
 * @brief   Get the Powerkey trig state
 * @param   None
 * @return  Bit 0:POWERKEY_TRG
            Bit 1:POWERKEY_TRG_S_PRESS
            Bit 2:POWERKEY_TRG_L_PRESS
            Bit 3:POWERKEY_ST
 * @note    
 */
uint8_t PMU_PowerKeyStateGet(void);


/**
 * @brief   Get the flag of Powerkey trigger state
 * @param   None
 * @return  true: Powerkey trigger flag is set; false: Powerkey trigger flag is not set;
 * @note    This flag indicates that the time Powerkey pressed has lasted for short press time
 */
bool PMU_PowerKeyTrigStateGet(void);

/**
 * @brief   Get the flag of Powerkey short press state
 * @param   None
 * @return  true: Powerkey short press flag is set; false: Powerkey short press flag is not set;
 * @note    This flag indicates that the time Powerkey pressed is more than short press time and less than long press time
 */
bool PMU_PowerKeyShortPressTrigStateGet(void);

/**
 * @brief   Get the flag of Powerkey long press state
 * @param   None
 * @return  true: Powerkey long press flag is set; false: Powerkey long press flag is not set;
 * @note    This flag indicates that the time Powerkey pressed has lasted for long press time in soft switch mode
 */
bool PMU_PowerKeyLongPressTrigStateGet(void);

/**
 * @brief   Get current Powerkey pin state
 * @param   None
 * @return  1: Powerkey pin is high level; 0: Powerkey pin is low level;
 * @note
 */
bool PMU_PowerKeyPinStateGet(void);

/**
 * @brief   Enable Powerkey module
 * @param   None
 * @return  None
 * @note
 */
void PMU_PowerKeyEnable(void);

/**
 * @brief   Disable Powerkey module
 * @param   None
 * @return  None
 * @note
 */
void PMU_PowerKeyDisable(void);

/**
 * @brief   Get current Powerkey enable state
 * @param   None
 * @return  ture: enabled; false: disabled;
 * @note
 */
bool PMU_PowerKeyEnGet(void);

/**
 * @brief   Setting Powerkey long press reset mode.
 * @param   PowerLongResetMode: 0:reset release until press release if press time is longer than 8s
                                1:reset release when press time reached 8s no matter when press release
 * @return  None
 * @note
 */
void PMU_PowerLongResetModeSet(POWER_LONG_RESET_MODE PowerLongResetMode);

/**
 * @brief   Get Powerkey long press reset mode.
 * @param   None
 * @return  0:reset release until press release if press time is longer than 8s
            1:reset release when press time reached 8s no matter when press release
 * @note
 */
POWER_LONG_RESET_MODE PMU_PowerLongResetModeGet(void);

/**
 * @brief   Set Powerkey short press time
 * @param   cnt: 1~63, unit:8ms; 
 * @return  None
 * @note    1) Powerkey short press time is: cnt*8ms + NoiseFilter*4ms;
 *          2) cnt should not be zero
 */
void PMU_PowerKeyShortPressTrigMaxCntSet(uint8_t cnt);

/**
 * @brief   Get settings of Powerkey short press time
 * @param   None
 * @return  setting value
 * @note
 */
uint8_t PMU_PowerKeyShortPressTrigMaxCntGet(void);

/**
 * @brief   Set Powerkey long press time
 * @param   cnt: 1~63, unit:64ms; 
 * @return  None
 * @note    Powerkey long press time is: cnt*64ms + NoiseFilter*4ms;
 */
void PMU_PowerKeyLongPressTrigMaxCntSet(uint8_t cnt);

/**
 * @brief   Get settings of Powerkey long press time
 * @param   None
 * @return  setting value
 * @note    shuld confirm: long press time > short press time
 */
uint8_t PMU_PowerKeyLongPressTrigMaxCntGet(void);

/**
 * @brief   Set Powerkey long press reset time 
 * @param   cnt: 0~15, unit:1.024s; 
 * @return  None
 * @note    Powerkey long press time is: cnt*1.024s + NoiseFilter*4ms;
 */
void PMU_PowerKeyResetTrigMaxCntSet(uint8_t cnt);

/**
 * @brief   Get settings of Powerkey long press reset time 
 * @param   None
 * @return  setting value
 * @note
 */
uint8_t PMU_PowerKeyResetTrigMaxCntGet(void);

/**
 * @brief   Set Powerkey noise filter time
 * @param   cnt: 0~63, unit:4ms; 
 * @return  None
 * @note
 */
void PMU_PowerKeyNoiseFilterMaxCntSet(uint8_t cnt);

/**
 * @brief   Get settings of Powerkey noise filter time
 * @param   None
 * @return  setting value
 * @note
 */
uint8_t PMU_PowerKeyNoiseFilterMaxCntGet(void);

/**
 * @brief   Calibrate pmu rc clock to 32K Hz
 * @param   None
 * @return  None
 * @note
 */
void pmu_rc32k_calibrate();


/**
 * @brief  get bool to determine if the PMU was first powered on.
 * @param  None
 * @return true:first time power on.
 *         false:not the first time.
 * @note   This function should be called before PMU_PowerKeyEnable()
 */
bool PMU_FristPowerOnFlagGet(void);

/**
 * @brief   Set reserve register one bit to 1
 * @param   BitMask register bit mask[bit0--bit7]
 * @return  None
 * @note    bit0,bit1 is used by system
 */
void PMU_ResRegOneBitSet(uint8_t BitMask);

/**
 * @brief   Clear reserve register one bit to 0
 * @param   BitMask register bit mask[bit0--bit7]
 * @return  None
 * @note    bit0,bit1 is used by system
 */
void PMU_ResRegOneBitClear(uint8_t BitMask);

/**
 * @brief   Get reserve register value
 * @param   None
 * @return  Register value
 * @note
 */
uint8_t PMU_ResRegGet(void);

//*************************   GPIOC设置  ***************************** //
/**
 * @brief      使能GPIOC端口一bit的中断功能
 * @param[in]  GpioIndex: bit位，例如GPIO_INDEX1
 * @param[in]  TrigType: 触发极性，例如GPIO_NEG_EDGE_TRIGGER触发
 * @return     无
 */
void GPIO_C_INTEnable( GPIO_INDEX GpioIndex, GPIO_TriggerType TrigType);

/**
 * @brief      清除GPIOC端口的中断状态
 * @param[in]  GpioIndex: GPIO bit位,例如GPIO_INDEX1
 * @return     无
 */
void GPIO_C_INTFlagClear(GPIO_INDEX GpioIndex);

/**
 * @brief      获取GPIOC端口的中断状态
 * @param[in]  NONE
 * @return     一组GPIO的中断状态
 */
uint32_t GPIO_C_INTStatusGet(void);

/**
 * @brief      GPIOC端口读取 1bit数据
 * @param[in]  GpioIndex：GPIO bit位，例如：GPIO_INDEX1
 * @return     1 or 0
 */
bool  GPIO_C_OneBitGet(GPIO_INDEX GpioIndex);

/**
 * @brief      读取一组GPIO数据
 * @param[in]  NONE
 * @return     GPIO值
 */
uint32_t GPIO_C_RegGet(void);

/**
 * @brief  配置GPIOC0 IE,OE,DS,PD状态
 * @param  ie: 输入使能 0-不使能 1-使能
 * @param  oe: 输出使能 0-不使能 1-使能
 * @param  ds: 0-2mA 1-4mA
 * @param  pd: 是否拉低
 * @return void
 * @note   none
 */
void PMU_C0GPIOCtrlSet(uint8_t ie, uint8_t oe, uint8_t ds, uint8_t pd);

/**
 * @brief  配置GPIOC1 IE,OE,DS,PD状态
 * @param  ie: 输入使能 0-不使能 1-使能
 * @param  oe: 输出使能 0-不使能 1-使能
 * @param  ds: 0-2mA 1-4mA
 * @param  pd: 是否拉低
 * @return void
 * @note   none
 */
void PMU_C1GPIOCtrlSet(uint8_t ie, uint8_t oe, uint8_t ds, uint8_t pd);

/**
 * @brief  配置GPIOC2 IE,OE,DS,PD状态
 * @param  ie: 输入使能 0-不使能 1-使能
 * @param  oe: 输出使能 0-不使能 1-使能
 * @param  ds: 0-2mA 1-4mA
 * @param  pd: 是否拉低
 * @return void
 * @note   none
 */
void PMU_C2GPIOCtrlSet(uint8_t ie, uint8_t oe, uint8_t ds, uint8_t pd);

/**
* @brief  GPIOC0 输出高电平
* @return void
* @note   需要外置上拉电阻，建议10K
*/
void PMU_C0GPIOOutSetHight(void);

/**
* @brief  翻转GPIOC0输出电平
* @return void
* @note   需要外置上拉电阻，建议10K
*/
void PMU_C0GPIOOutSetTGL(void);

/**
* @brief  GPIOC0输出低电平
* @return void
* @note   none
*/
void PMU_C0GPIOOutSetLow(void);

/**
* @brief  GPIOC1 输出高电平
* @return void
* @note   需要外置上拉电阻，建议10K
*/
void PMU_C1GPIOOutSetHight(void);

/**
* @brief  翻转GPIOC1输出电平
* @return void
* @note   需要外置上拉电阻，建议10K
*/
void PMU_C1GPIOOutSetTGL(void);

/**
* @brief  GPIOC1输出低电平
* @return void
* @note   none
*/
void PMU_C1GPIOOutSetLow(void);

/**
* @brief  GPIOC2 输出高电平
* @return void
* @note   none
*/
void PMU_C2GPIOOutSetHight(void);

/**
* @brief  翻转GPIOC2输出电平
* @return void
* @note   需要外置上拉电阻，建议10K
*/
void PMU_C2GPIOOutSetTGL(void);

/**
* @brief  GPIOC2输出低电平
* @return void
* @note   需要外置上拉电阻，建议10K
*/
void PMU_C2GPIOOutSetLow(void);


/**
* @brief  GPIOC0/1/2 Powerup 类型设置
* @param	 ePin 按键
* @param	 eType 类型
* @return void
*/
void PMU_GPIOCSetPowerupType(GPIOC_POWERUP_PIN ePin, GPIOC_POWERUP_TYPE eType);


#ifdef __cplusplus
}
#endif

#endif

/**
 * @}
 * @}
 */
