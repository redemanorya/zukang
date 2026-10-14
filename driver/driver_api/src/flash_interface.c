/**
 **************************************************************************************
 * @file	flash_interface.c
 * @brief	flash interface
 *
 * @author	jiaozhenqiang
 * @version V1.0.0
 *
 * $Created: 2025-10-24 13:50$
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include <string.h>
#include "flash_interface.h"
#include "watchdog.h"
#include "core_d1088.h"

#ifdef FUNC_OS_EN
#include "rtos_api.h"
osMutexId FlashMutex = NULL;
#endif

SPI_FLASH_ERR_CODE SpiFlashReadWpr( uint32_t StartAddr, uint8_t* Buffer, uint32_t Length, uint32_t TimeOut)
{
	SPI_FLASH_ERR_CODE ret;
#ifdef FUNC_OS_EN
	osMutexLock(FlashMutex);
#endif
	ret = SpiFlashRead(StartAddr, Buffer, Length, TimeOut);
#ifdef FUNC_OS_EN
	osMutexUnlock(FlashMutex);
#endif
	return ret;
}


SPI_FLASH_ERR_CODE SpiFlashWriteWpr( uint32_t	Addr, uint8_t	*Buffer, uint32_t	Length, uint32_t TimeOut)
{
	SPI_FLASH_ERR_CODE ret;
#ifdef FUNC_OS_EN
	osMutexLock(FlashMutex);
#endif
	ret = SpiFlashWrite(Addr, Buffer, Length, TimeOut);
#ifdef FUNC_OS_EN
	osMutexUnlock(FlashMutex);
#endif
	return ret;
}

void SpiFlashEraseWpr(ERASE_TYPE_ENUM erase_type, uint32_t index, bool IsSuspend)
{
#ifdef FUNC_OS_EN
	osMutexLock(FlashMutex);
#endif
	SpiFlashErase(erase_type, index, IsSuspend);
#ifdef FUNC_OS_EN
	osMutexUnlock(FlashMutex);
#endif
}

int32_t FlashEraseWpr(uint32_t Offset,uint32_t Size)
{
	SPI_FLASH_ERR_CODE ret;
#ifdef FUNC_OS_EN
	osMutexLock(FlashMutex);
#endif
	ret = FlashErase(Offset, Size);
#ifdef FUNC_OS_EN
	osMutexUnlock(FlashMutex);
#endif
	return ret;
}

SPI_FLASH_INFO* SpiFlashInfoGetWpr(void)
{
	SPI_FLASH_INFO *ret;
#ifdef FUNC_OS_EN
	osMutexLock(FlashMutex);
#endif
	ret = SpiFlashInfoGet();
#ifdef FUNC_OS_EN
	osMutexUnlock(FlashMutex);
#endif
	return ret;
}

SPI_FLASH_ERR_CODE SpiFlashReadUniqueIDWpr(uint8_t* Buffer, uint8_t BufLen)
{
	SPI_FLASH_ERR_CODE ret;
#ifdef FUNC_OS_EN
	osMutexLock(FlashMutex);
#endif
	ret = SpiFlashReadUniqueID(Buffer, BufLen);
#ifdef FUNC_OS_EN
	osMutexUnlock(FlashMutex);
#endif
	return ret;
}

SPI_FLASH_ERR_CODE FlashEraseExt(uint32_t Offset, uint32_t Size, bool IsSuspend)
{
    SPI_FLASH_ERR_CODE ret = ERASE_FLASH_ERR;
    SPI_FLASH_INFO *pInfo = NULL;
    int32_t i, Round;

#ifdef FUNC_OS_EN
    osMutexLock(FlashMutex);
#endif

    //Check if Offset and Size is at 4K alignment
    if (((Offset & 0xFFF) == 0) && ((Size & 0xFFF) == 0))
    {
        pInfo = SpiFlashInfoGet();

        //Validate input parameters
        if(pInfo->Capacity >= Offset + Size)
        {
            if(Size >= 64 * 1024)
            {
                if(Offset & (0x10000 - 1))
                {
                    Round = (0x10000 - (Offset & 0xFFFF)) >> 12;
                    for(i = 0; i < Round; i++)
                    {
                        WDG_Feed();
                        //4K erase
                        SpiFlashErase(SECTOR_ERASE, Offset / 0x1000, IsSuspend);
                        Size -= 0x1000;
                        Offset += 0x1000;
                    }
                }

                if(Size >= 64 * 1024)
                {
                    Round = Size >> 16;
                    for(i = 0; i < Round; i++)
                    {
                        WDG_Feed();
                        //64K erase
                        SpiFlashErase(BLOCK_ERASE, Offset / 0x10000, IsSuspend);
                        Size -= 0x10000;
                        Offset += 0x10000;
                    }
                }
            }

            Round = Size >> 12;
            for(i = 0; i < Round; i++)
            {
                WDG_Feed();
                //4K erase
                SpiFlashErase(SECTOR_ERASE, Offset / 0x1000, IsSuspend);
                Offset += 0x1000;
            }

            ret = FLASH_NONE_ERR;
        }
    }

#ifdef FUNC_OS_EN
    osMutexUnlock(FlashMutex);
#endif

    return ret;
}

/*==================  Flash Security Operation Check Interface ============================*/

__attribute__((weak)) const TS_FA gFlashRWTable[] =
{
	{0, 0x80000, 0x10000},
	{-1, 0, 0},
};

TE_FLASHOP_CODE FLASHOP_Erase(uint32_t type, uint32_t startAddr, uint32_t length, uint32_t isSuspend, uint32_t doVerity)
{
	int32_t i = 0;
	int32_t findFlag = 0;
	uint32_t u32Addr;
	uint32_t u32Len;
	SPI_FLASH_INFO *pFlashInfo;

	int32_t round;

	//check input
	if (startAddr & 0xFFF)	//not 4K aligned
	{
		return E_FOP_ERASE_ADDR_NOT_ALIGN;
	}

	if (length > 0)
	{
		if (length & 0xFFF)   //not 4K aligned
		{
			return E_FOP_ERASE_LEN_NOT_ALIGN;
		}
	}
	else
	{
		return E_FOP_NORMAL;
	}

	//search table
	i = 0;
	findFlag = 0;
	while(gFlashRWTable[i].type > 0)
	{
		if (gFlashRWTable[i].type == type)
		{
			u32Addr = gFlashRWTable[i].addrStart;
			u32Len = gFlashRWTable[i].length;
			findFlag = 1;

			break;
		}
		i++;
	}

	if (!findFlag)
	{
		return E_FOP_AREA_UNKOWN_TYPE;
	}

	//check address and length
	if ((startAddr < u32Addr) || ((startAddr + length) > (u32Addr + u32Len)))
	{
		return E_FOP_AREA_BEYOND;
	}

#ifdef FUNC_OS_EN
	osMutexLock(FlashMutex);
#endif

	//check flash capacity
	pFlashInfo = SpiFlashInfoGet();
	if ((startAddr + length) > pFlashInfo->Capacity)
	{
#ifdef FUNC_OS_EN
		osMutexUnlock(FlashMutex);
#endif
		return E_FOP_EXCEEED_FLASH_CAPACITY;
	}

	u32Addr = startAddr;
	u32Len = length;

	//now do flash erase
	if(u32Len >= 64 * 1024)
	{
		if (u32Addr & 0xFFFF)
		{
			round = (u32Len & 0xFFFF) >> 12;
			for (i = 0; i < round; i++)
			{
				WDG_Feed();
				SpiFlashErase(SECTOR_ERASE, u32Addr>>12, (isSuspend)?1:0);
				u32Addr += 0x1000;
				u32Len -= 0x1000;
			}
		}

		if (64*1024 < u32Len)
		{
			round = u32Len >> 16;
			for (i = 0; i < round; i++)
			{
				WDG_Feed();
				SpiFlashErase(BLOCK_ERASE, u32Addr>>16, (isSuspend)?1:0);
				u32Addr += 0x10000;
				u32Len -= 0x10000;
			}
		}
	}

	round = u32Len >> 12;
	for (i = 0; i < round; i++)
	{
		WDG_Feed();
		SpiFlashErase(SECTOR_ERASE, u32Addr>>12, (isSuspend)?1:0);
		u32Addr += 0x1000;
		u32Len -= 0x1000;
	}

	//do verity
	if (doVerity)
	{
		uint32_t* pU32Data = NULL;

		DataCacheRangeInvalid(startAddr, length);

		pU32Data = (uint32_t*)startAddr;

		WDG_Feed();
		while((uint32_t)pU32Data < (startAddr + length))
		{
			if (*pU32Data++ != 0xFFFFFFFF)
			{
#ifdef FUNC_OS_EN
				osMutexUnlock(FlashMutex);
#endif

				return E_FOP_ERASE_VERIFY_FAILED;
			}

			if ((((uint32_t)pU32Data) % 0x10000) == 0)
			{
				WDG_Feed();    //feed dog every 64K
			}
		}
	}

#ifdef FUNC_OS_EN
	osMutexUnlock(FlashMutex);
#endif

	return E_FOP_NORMAL;
}

TE_FLASHOP_CODE FLASHOP_Write(uint32_t type, uint32_t flashAddr, uint8_t* pBuffer, uint32_t length, uint32_t isSuspend, uint32_t doVerity)
{
	int32_t i = 0;
	int32_t findFlag = 0;
	uint32_t u32Addr;
	uint32_t u32Len;
	uint8_t *pU8Buf;

	SPI_FLASH_INFO* pFlashInfo;

	//search table
	i = 0;
	findFlag = 0;
	while(gFlashRWTable[i].type > 0)
	{
		if (gFlashRWTable[i].type == type)
		{
			u32Addr = gFlashRWTable[i].addrStart;
			u32Len = gFlashRWTable[i].length;
			findFlag = 1;
			break;
		}
		i++;
	}

	if (!findFlag)
	{
		return E_FOP_AREA_UNKOWN_TYPE;
	}

	//check address and length
	if (length	> 0)
	{
		//check area beyond
		if (((uint32_t)flashAddr < u32Addr) || ((uint32_t)flashAddr + length > (u32Addr + u32Len)))
		{
			return E_FOP_AREA_BEYOND;
		}

		//check buffer and	address overlap
		if (flashAddr < (uint32_t)pBuffer)
		{
			if (flashAddr + length > (uint32_t)pBuffer)
			{
				return E_FOP_WRITE_ADDRESS_OVERLAP;
			}
		}
		else
		{
			if ((uint32_t)pBuffer + length > flashAddr)
			{
				return E_FOP_WRITE_ADDRESS_OVERLAP;
			}
		}
	}
	else
	{
		return E_FOP_NORMAL;
	}

#ifdef FUNC_OS_EN
	osMutexLock(FlashMutex);
#endif

	//check flash capacity
	pFlashInfo = SpiFlashInfoGet();
	if ((flashAddr + length) > pFlashInfo->Capacity)
	{
#ifdef FUNC_OS_EN
		osMutexUnlock(FlashMutex);
#endif
		return E_FOP_EXCEEED_FLASH_CAPACITY;
	}

	//now do flash write
	u32Addr = flashAddr;
	u32Len = length;
	pU8Buf = pBuffer;

	while(u32Len > 0)
	{
		uint32_t tmpLen = 0;
		SPI_FLASH_ERR_CODE eRet;
#define WLEN_ONEROUND	(0x2000)

		WDG_Feed();

		if (u32Len > WLEN_ONEROUND)
		{
			tmpLen = WLEN_ONEROUND;
		}
		else
		{
			tmpLen = u32Len;
		}

		eRet = SpiFlashWrite(u32Addr, pU8Buf, tmpLen, (isSuspend)?1:0);

		if (eRet != FLASH_NONE_ERR)
		{
#ifdef FUNC_OS_EN
			osMutexUnlock(FlashMutex);
#endif
			return eRet;
		}

		pU8Buf += tmpLen;
		u32Addr += tmpLen;
		u32Len -= tmpLen;
	}

	//do verity
	u32Addr = flashAddr;
	u32Len = length;
	pU8Buf = pBuffer;
	if (doVerity)
	{
#define ONE_LINE_SIZE		(0x10000)
		uint32_t round = 0;
		uint32_t bytesInLine = 0;

		round = length / ONE_LINE_SIZE;
		bytesInLine = length % ONE_LINE_SIZE;

		DataCacheRangeInvalid(flashAddr, length);

		WDG_Feed();

		for (i = 0; i < round; i++)
		{
			if (memcmp((void *)u32Addr, pU8Buf, ONE_LINE_SIZE) != 0)
			{
#ifdef FUNC_OS_EN
				osMutexUnlock(FlashMutex);
#endif
				return E_FOP_WRITE_VERIFY_FAILED;
			}
			u32Addr += ONE_LINE_SIZE;
			pU8Buf += ONE_LINE_SIZE;
		}

		if (bytesInLine != 0)
		{
			if (memcmp((void *)u32Addr, pU8Buf, bytesInLine) != 0)
			{
#ifdef FUNC_OS_EN
				osMutexUnlock(FlashMutex);
#endif
				return E_FOP_WRITE_VERIFY_FAILED;
			}
		}
	}

#ifdef FUNC_OS_EN
	osMutexUnlock(FlashMutex);
#endif

	return E_FOP_NORMAL;
}

void FLASHOP_ListFlashRWTable(int (*printCallback)(const char *fmt, ...))
{
	int i = 0;

	while(gFlashRWTable[i].type >= 0)
	{
		if (NULL != printCallback)
		{
			printCallback("Table[%d]: type=%d  addr=0x%08X len=0x%08X\n", i, gFlashRWTable[i].type, gFlashRWTable[i].addrStart, gFlashRWTable[i].length);
		}
		i++;
	}
}

void FLASHOP_ParseRetCode(int nRet, int (*printCallback)(const char *fmt, ...))
{
	if (NULL == printCallback)
	{
		return;
	}

	switch(nRet)
	{
		case E_FOP_NORMAL:
			printCallback("Success\n");
			break;

		case E_FOP_ERASE_ADDR_NOT_ALIGN:
			printCallback("Error: The address must be 4K aligned!\n");
			break;

		case E_FOP_ERASE_LEN_NOT_ALIGN:
			printCallback("Error: The length must be a multiple of 4K!\n");
			break;

		case E_FOP_ERASE_VERIFY_FAILED:
			printCallback("Error: Erase verify failed!\n");
			break;

		case E_FOP_WRITE_ADDRESS_OVERLAP:
			printCallback("Error: The erasing and writing areas overlap each other!\n");
			break;

		case E_FOP_WRITE_VERIFY_FAILED:
			printCallback("Error: Write verify failed!\n");
			break;

		case E_FOP_AREA_UNKOWN_TYPE:
			printCallback("Error: Unknown type of operation area!\n");
			break;

		case E_FOP_AREA_BEYOND:
			printCallback("Error: Operation area exceeds boundaries!\n");
			break;

		case E_FOP_EXCEEED_FLASH_CAPACITY:
			printCallback("Error: Operation area exceeds the flash capacity!\n");
			break;

		default:
			if (nRet < 0)
			{
				printCallback("Error: Please see the definition of SPI_FLASH_ERR_CODE!\n");
			}
			else
			{
				printCallback("Unknown ret code!!\n");
			}
			break;
	}
}


