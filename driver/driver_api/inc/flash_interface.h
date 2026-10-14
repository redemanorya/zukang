/**
 **************************************************************************************
 * @file    flash_interface.h
 * @brief   flash interface
 *
 * @author  jiaozhenqiang
 * @version V1.0.0
 *
 * $Created: 2025-10-24 13:50$
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#ifndef __FLASH_INTERFACE_H__
#define __FLASH_INTERFACE_H__

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

#include "spi_flash.h"

#ifdef CFG_APP_CONFIG
	#include "app_config.h"
#endif

#ifdef CFG_DBUS_ACCESS_EN
	#define	SPiFlashReadData(addr,buf,len)	(NULL == memcpy((void *)(buf),(void *)(addr),len))
#else
	#define	SPiFlashReadData(addr,buf,len)	(FLASH_NONE_ERR != SpiFlashRead(addr,(uint8_t *)(buf),len,1))
#endif

/**
 * @brief  read data from flash
 * @param  StartAddr the data addr need to read for flash
 * @param  Buffer the data addr need to save for MCU's mem
 * @param  Length data length
 * @param  TimeOut read data but fifo always empty
 * @return SPI_FLASH_ERR_CODE
 *         @arg  FIFO_ALWAYS_EMPTY_ERR  function execute time out
 *         @arg  DATALEN_LESS_THEN_ZERO_ERR    read data but data len is < 0
 *         @arg  NONE_ERR
 */
SPI_FLASH_ERR_CODE SpiFlashReadWpr( uint32_t StartAddr, uint8_t* Buffer, uint32_t Length, uint32_t TimeOut);

/**
 * @brief  write data into flash
 * @param  Addr the data addr need to write for flash
 * @param  Buffer the data addr need to read for MCU's mem
 * @param  Length data length
 * @param  TimeOut read data but fifo always empty
 * @return SPI_FLASH_ERR_CODE
 *         @arg  FIFO_ALWAYS_FULL_ERR  function execute time out
 *         @arg  DATALEN_LESS_THEN_ZERO_ERR    read data but data len is < 0
 *         @arg  NONE_ERR
 */
SPI_FLASH_ERR_CODE SpiFlashWriteWpr( uint32_t	Addr, uint8_t	*Buffer, uint32_t 	Length, uint32_t TimeOut);

/**
 * @brief  flash erase
 * @param  EraseType:	see ERASE_TYPE_ENUM type
 *										CHIP_ERASE:		Erase whole chip
 *										SECTOR_ERASE:	Erase One sector
 *										BLOCK_ERASE:	Erase One block
 *				  					for CHIP_ERASE type, param 2 & param 3 are NOT cared.
 * @param  Index:
 *										for SECTOR_ERASE type: 	means sector number
 *										for BLOCK_ERASE type: 	means block number
 * @param  IsSuspend: during flash write or sector/block erase need suspend set it to 1
 * @return NONE
 */
void SpiFlashEraseWpr(ERASE_TYPE_ENUM erase_type, uint32_t index, bool IsSuspend);

/**
 * @brief	擦除flash
 * @param	Offset	 擦除地址，必须4KB对齐
 * @param	Size	擦除size，必须4KB对齐
 * @return	ERASE_FLASH_ERR-擦除失败	FLASH_NONE_ERR-擦出成功
 * @note
 */
int32_t FlashEraseWpr(uint32_t Offset,uint32_t Size);


/**
 * @brief  get spi flash information
 * @param  none
 * @return return the SPI_FLASH_INFO pointer
 * @note this function should be called after SpiFlashInit
 */
SPI_FLASH_INFO* SpiFlashInfoGetWpr(void);

/**
 * @brief	读取flash 唯一序列号到内存缓冲区
 * @param	Buffer    内存缓冲区地址
 * @param	BufLen    缓冲区长度
 * @return	DATALEN_LESS_THEN_ZERO_ERR-缓冲区长度错误       ; FLASH_NONE_ERR-成功
 * @note
 */
SPI_FLASH_ERR_CODE SpiFlashReadUniqueIDWpr(uint8_t* Buffer, uint8_t BufLen);

/**
 * @brief   flash擦除扩展
 * @param   Offset      擦除地址, 必须4KB对齐
 * @param   Size        擦除size, 必须4KB对齐
 * @param   IsSuspend   擦除过程中，是否使用suspend/resume机制响应中断
 * @return  ERASE_FLASH_ERR-擦除失败	; LASH_NONE_ERR-擦出成功
 * @note    此API会检查Offset/Size是否4KB对齐, 不对齐则返回错误
 */
SPI_FLASH_ERR_CODE FlashEraseExt(uint32_t Offset, uint32_t Size, bool IsSuspend);


/************************ Flash 擦写安全检查API ***************/

typedef struct
{
	int32_t  type;			/* A 32-bit positive integer used to represent a setting item, must
								be unique within the table. If <0 indicates the last record. */
	uint32_t addrStart;		/* Area staring address */
	uint32_t length;		/* Area length */
}TS_FA;


/* Return code definition */
typedef enum
{
	E_FOP_NORMAL = FLASH_NONE_ERR,
	E_FOP_ERASE_ADDR_NOT_ALIGN,
	E_FOP_ERASE_LEN_NOT_ALIGN,
	E_FOP_ERASE_VERIFY_FAILED,
	E_FOP_WRITE_ADDRESS_OVERLAP,
	E_FOP_WRITE_VERIFY_FAILED,
	E_FOP_AREA_UNKOWN_TYPE,
	E_FOP_AREA_BEYOND,
	E_FOP_EXCEEED_FLASH_CAPACITY,
} TE_FLASHOP_CODE;


/**
 * @brief  Flash erase operation with safety check.
 * @param  type: Area type code
 * @param  startAddr: The starting address for flash erase，must be 4K aligned.
 * @param  length: Erase length, must be 4K aligned.
 * @param  isSuspend: 1 - Use suspend/resume mechanism during the erase process. 0 - Do not use suspend/resume mechanism.
 * @param  doVerity: 1 - After erase, it will check and confirm that all the data within the erasure range is 0xFF.
 *                   0 - Do not do verify after erase.
 * @return TE_FLASHOP_CODE
 *         @arg  E_FOP_NORMAL
 *         @arg  E_FOP_ERASE_ADDR_NOT_ALIGN
 *         @arg  E_FOP_ERASE_LEN_NOT_ALIGN
 *         @arg  E_FOP_AREA_UNKOWN_TYPE
 *         @arg  E_FOP_AREA_BEYOND
 *         @arg  E_FOP_EXCEEED_FLASH_CAPACITY
 *         @arg  E_FOP_ERASE_VERIFY_FAILED
 *         @arg  others: see SPI_FLASH_ERR_CODE
 */
TE_FLASHOP_CODE FLASHOP_Erase(uint32_t type, uint32_t startAddr, uint32_t length, uint32_t isSuspend, uint32_t doVerity);

/**
 * @brief  Flash write operation with safety check.
 * @param  type: Area type code
 * @param  flashAddr: The starting address for flash write.
 * @param  pBuffer: Pointer of source data.
 * @param  length: Data length.
 * @param  isSuspend: 1 - Use suspend/resume mechanism during the write process. 0 - Do not use suspend/resume mechanism.
 * @param  doVerity: 1 - After write, it will compare and verify the correctness of the data.
 *                   0 - Do not do verify after write.
 * @return TE_FLASHOP_CODE
 *         @arg  E_FOP_NORMAL
 *         @arg  E_FOP_AREA_UNKOWN_TYPE
 *         @arg  E_FOP_AREA_BEYOND
 *         @arg  E_FOP_WRITE_ADDRESS_OVERLAP
 *         @arg  E_FOP_EXCEEED_FLASH_CAPACITY
 *         @arg  E_FOP_WRITE_VERIFY_FAILED
 *         @arg  others: see SPI_FLASH_ERR_CODE
 */
TE_FLASHOP_CODE FLASHOP_Write(uint32_t type, uint32_t flashAddr, uint8_t *pBuffer, uint32_t length, uint32_t isSuspend, uint32_t doVerity);

/**
 * @brief  Print out the contents of gFlashRWTable.
 * @param  printCallback: The callback function used to do the print.
 * @return None
 */
void FLASHOP_ListFlashRWTable(int (*printCallback)(const char *fmt, ...));

/**
 * @brief  Based on the return code, print out the corresponding string information.
 * @param  nRet: Return code.
 * @param  printCallback: The callback function used to do the print.
 * @return None
 */
void FLASHOP_ParseRetCode(int nRet, int (*printCallback)(const char *fmt, ...));

#ifdef __cplusplus
}
#endif // __cplusplus

#endif//__FLASH_INTERFACE_H__
