/*
 * opi_psram.h
 *
 *	Created on: Sep 2, 2025
 *		Author:
 */
/**
 * @addtogroup PSRAM
 * @{
 * @defgroup opi_psram opi_psram.h
 * @{
 */

#ifndef OPI_PSRAM_H_
#define OPI_PSRAM_H_

#include "type.h"

typedef struct
{
	uint32_t VendorID				:  4; //0000b
	uint32_t ColumnAddressBitCount	:  4;
	uint32_t RowAddressBitCount 	:  5;
	uint32_t						:  3; //reserved
} TS_MR0;

typedef union {
	TS_MR0 f;
	uint16_t value;
} SU_OPIPSRAM_MR0;


typedef struct
{
	uint32_t DeviceID				:  4;
	uint32_t						:  3; //reserved
	uint32_t KGD					:  1;
	uint32_t						:  8; //reserved
} TS_MR1;

typedef union {
	TS_MR1 f;
	uint16_t value;
} SU_OPIPSRAM_MR1;

typedef struct
{
	uint32_t BurstLength1			:  2;
	uint32_t BurstType				:  1;
	uint32_t LatencyType			:  1;
	uint32_t LatencyCode			:  4;
	uint32_t BurstLength0			:  1;
	uint32_t						:  3; //reserved
	uint32_t DriverStrength 		:  3;
	uint32_t DPD					:  1; 
} TS_MR2;

typedef union {
	TS_MR2 f;
	uint16_t value;
} SU_OPIPSRAM_MR2;


typedef struct
{
	uint32_t RefreshFlag			:  2;
	uint32_t PASR					:  3;
	uint32_t LPMen					:  1;
	uint32_t						:  2;  //reserved
	uint32_t RefreshFreqSet 		:  2;
	uint32_t ManualRefresh			:  1;
	uint32_t						:  1;
	uint32_t SoftReset				:  4;
} TS_MR3;

typedef union {
	TS_MR3 f;
	uint16_t value;
} SU_OPIPSRAM_MR3;


typedef enum
{
	DR_33_OM   = 0,
	DR_100_OM  = 1,
	DR_66_OM   = 2,
	DR_50_OM   = 3,
	DR_40_OM   = 4,
	DR_25_OM   = 5,
	DR_20_OM   = 6
} TE_DRVSTR;

typedef enum
{
	LT_VARIABLE = 0,
	LT_FIXED    = 1
} TE_LATENCY_TYPE;

typedef enum
{
	LC_84_MHZ  = 0xE,			//1110b
	LC_108_MHZ = 0xF,			//1111b
	LC_133_MHZ = 0x0,			//0000b
	LC_166_MHZ = 0x1,			//0001b
	LC_200_MHZ = 0x2,			//0010b
} TE_LATENCY_CODE;


typedef enum
{
	OPIPSRAM_RET_INVALID_PARAM = -255,		//invalid input parameter
	OPIPSRAM_RET_DQS_CALIBRATE_FAILED, 
	OPIPSRAM_RET_SUCCESS = 0,
}OPIPSRAM_RET_CODE;

typedef enum
{
	SELFREFRESH_FREQ_05 = 0,
	SELFREFRESH_FREQ_1X = 1,
	SELFREFRESH_FREQ_4X = 3
}TE_SELFREFRESH_FREQ;

/**
 * @brief  opi psram initialize
 * @param  none
 * @return OPIPSRAM_RET_SUCCESS - initialize success
 *         OPIPSRAM_RET_DQS_CALIBRATE_FAILED - initialize failed
 * @note this function should be called before use psaram
 */
OPIPSRAM_RET_CODE OPIPSRAM_Init(void);

/**
 * @brief  read opi psram MR0 register
 * @param  none
 * @return the value of MR0 register
 */
uint16_t OPIPSRAM_ReadMR0(void);

/**
 * @brief  read opi psram MR1 register
 * @param  none
 * @return the value of MR1 register
 */
uint16_t OPIPSRAM_ReadMR1(void);

/**
 * @brief  read opi psram MR2 register
 * @param  none
 * @return the value of MR2 register
 */
uint16_t OPIPSRAM_ReadMR2(void);

/**
 * @brief  read opi psram MR3 register
 * @param  none
 * @return the value of MR3 register
 */
uint16_t OPIPSRAM_ReadMR3(void);

/**
 * @brief  write opi psram MR2 register
 * @param  value - the data to write
 * @return none
 */
void OPIPSRAM_WriteMR2(uint16_t value);


/**
 * @brief  write opi psram MR3 register
 * @param  value - the data to write
 * @return none
 */
void OPIPSRAM_WriteMR3(uint16_t value);


/**
 * @brief  set opi psram driver strength
 * @param  eDS - driver strength value
 * @return none
 */
void OPIPSRAM_SetDriverStrength(TE_DRVSTR eDS);

/**
 * @brief  set opi psram latency type
 * @param  eLT - latency type value
 * @return none
 */
void OPIPSRAM_SetLatencyType(TE_LATENCY_TYPE eLT);

/**
 * @brief  set opi psram latency code
 * @param  eLC - latency code value
 * @return none
 */
void OPIPSRAM_SetLatencyCode(TE_LATENCY_CODE eLC);

/**
 * @brief  set opi psram to enter or exit Low Power Mode
 * @param  isEna: true - enter LPM; other - exit LPM;
 * @return none
 */
void OPIPSRAM_SetLPM(bool isEna);

/**
 * @brief  set opi psram to enter or Deep Power Down mode
 * @param  isEna: true - enter DPD; other - exit DPC;
 * @return none
 */
int OPIPSRAM_SetDPD(bool isEna);

/**
 * @brief  execute opi psram global reset
 * @param  none
 * @return none
 */
void OPIPSRAM_GlobalReset(void);

/**
 * @brief  set opi psram self refresh parameter
 * @param  eRefFreq: self refresh parameter;
 * @return none
 */
void OPIPSRAM_SetSelfRefresh(TE_SELFREFRESH_FREQ eRefFreq);

#endif /* OPI_PSRAM_H_ */

/**
 * @}
 * @}
 */