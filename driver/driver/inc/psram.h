/**
 *******************************************************************************
 * @file    psram.h
 * @brief	The driver of psram module
 *
 * @author  shengqi_zhao
 * @version V1.0.0
 *
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 *******************************************************************************
 */

/**
 * @addtogroup PSRAM
 * @{
 * @defgroup psram psram.h
 * @{
 */
 
#include "type.h"

#ifndef   _PSRAM_H_
#define   _PSRAM_H_

/**
 * @brief  The base address of psram
 */
#define PSRAM_BASE  0x60000000


/**
 * @brief The acess mode of Psram
 * @SPI_1BIT_MODE   cmd 1bit addr 1bit data 1bit
 * @SPI_QUAD_MODE   cmd 1bit addr 4bit data 4bit
 * @SPI_QPI_MODE    cmd 4bit addr 4bit data 4bit
*/
typedef enum{
	SPI_1BIT_MODE = 0x01,
	SPI_QUAD_MODE = 0x02,
	SPI_QPI_MODE  = 0x03,
}PSRAM_TYPE;


/**
 * @brief intial the access mode and param of psram, then the psram can be accessed as normal ram
 *
 * @param[in]   mode              speicifiy the cmd/data access length of psram,see  PSRAM_TYPE definiton
 * @param[in]   burstlen,         the psram can be accessed by burst mode,this param set to promote access efficiency
 * @param[in]   read_wait_cycle   set according to the datasheet wrote you use
 * @param[in]   write_wait_cycle  set according to the datasheet wrote you use
 * @param[in]   hold_cycle        set according to the datasheet wrote you use
 * @param[out]  none
 */
void Psram_Init(PSRAM_TYPE mode,             uint16_t burstlen,   uint8_t read_wait_cycle,
		        uint8_t    write_wait_cycle, uint8_t hold_cycle
		        );


/**
 * @brief  exit from quad mode ,then can initial the psram again
 *
 * @param[in]    none
 * @brief[out]   none
 */
void Psram_QuadModeExit(void);

/**
 * @breif read len byte data from psram, use the memcpy funtion direcctly
 *
 * @param[in]   buffer   the buffer start address used to store the data read from psram
 * @param[in]   pos      the offset relative to the psram base address ,see the micro of the psram base definition
 * @param[in]   len      the len acculated by byte should to be read from psram
 * @param[out]  none
 *
 */
#define Psram_Read(buffer,  len, pos)   memcpy (buffer, ( void* )( PSRAM_BASE + pos ), len )



/**
 * @breif write len bytes from buffer to  psram, use the memcpy funtion direcctly
 *
 * @param[in]   buffer   the buffer start address used to store the data write to psram
 * @param[in]   pos      the offset relative to the psram base address ,see the micro of the psram base definition
 * @param[in]   len      the len acculated by byte should to be written from psram
 * @param[out]  none
 *
 */
#define Psram_Write(buffer, len, pos)  memcpy((void*)(PSRAM_BASE + pos), buffer, len)

#endif

/**
 * @}
 * @}
 */
 