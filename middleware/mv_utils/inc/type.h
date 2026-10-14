////////////////////////////////////////////////////////////////////////////////
//                   Mountain View Silicon Tech. Inc.
//		Copyright 2011, Mountain View Silicon Tech. Inc., ShangHai, China
//                   All rights reserved.
//
//		Filename	:types.h
//		Description	:re-define language C basic data type for porting conveniently
//						over platforms
//		Changelog	:
//					2012-02-21	Create basic data type re-definition for uniform
//						platform by Robert
///////////////////////////////////////////////////////////////////////////////

#ifndef		__TYPES_H__
#define		__TYPES_H__

#include<stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#ifndef NULL	
#define	NULL			((void*)0)
#endif

#define	FALSE			(0)
#define	TRUE			(1)

typedef	void(*FPCALLBACK)(void);

//typedef unsigned char   		bool;
typedef bool(*TerminateFunc)(void);

//#ifndef int8_t
#if !__int8_t_defined
typedef signed char				int8_t;
#endif

//#ifndef uint8_t
#if !__int8_t_defined
typedef unsigned char			uint8_t;
#endif

//#ifndef int16_t
#if !__int16_t_defined
typedef signed short			int16_t;
#endif

//#ifndef uint16_t
#if !__int16_t_defined
typedef unsigned short			uint16_t;
#endif

#if !__int32_t_defined
typedef signed int				int32_t;
#endif

//#ifndef uint32_t
#if !__int32_t_defined
typedef unsigned int			uint32_t;
#endif

//typedef	unsigned int			size_t;


#if defined(__ICCARM__)    // For IAR compiler
#define __ATTRIBUTE__(keyword)  keyword
#define BITBAND
#define AT(address)             @ address
#define SECTION(name)           @ name
#define WEAK                    __weak
#define UNUSED
#define __NO_INIT__             __no_init
#elif defined(__CC_ARM)    // For Keil compiler
#define __ATTRIBUTE__(keyword)  __attribute__((keyword))
#define BITBAND                 bitband
#define AT(address)             at(address)
#define SECTION(name)           section(name)
#define WEAK                    weak
#define UNUSED                  unused
#define __NO_INIT__
#else                      // For other compiler
#define __ATTRIBUTE__(keyword)  keyword
#define BITBAND
#define AT(address)
#define SECTION(name)
#define WEAK
#define UNUSED
#define __NO_INIT__
#endif
	

//大小端转换宏，ARM CPU为小端模式
#define Le16ToCpu(Val)		(Val)
#define Le32ToCpu(Val)		(Val)
#define CpuToLe16(Val)		(Val)
#define CpuToLe32(Val)		(Val)

#define Be16ToCpu(Val)		(((Val) << 8) | ((Val) >> 8))
#define Be32ToCpu(Val)		(((Val) << 24) | (((Val) << 8) & 0x00FF0000) | (((Val) >> 8) & 0x0000FF00) | ((Val) >> 24))
#define CpuToBe16(Val)		(((Val) << 8) | ((Val) >> 8))
#define CpuToBe32(Val)		(((Val) << 24) | (((Val) << 8) & 0x00FF0000) | (((Val) >> 8) & 0x0000FF00) | ((Val) >> 24))

#define Le16Byte0(Val)		((Val) 0xff)
#define Le16Byte1(Val)		(((Val)>>8) 0xff)
#define ByteToLe16(Byte0, Byte1) 	\
							((((uint16_t)(Byte1))<<8) | (Byte0))
#define Le32Byte0(Val)		((Val) 0xff)
#define Le32Byte1(Val)		(((Val)>>8) 0xff)
#define Le32Byte2(Val)		(((Val)>>16) 0xff)
#define Le32Byte3(Val)		(((Val)>>24) 0xff)
#define ByteToLe32(Byte0, Byte1, Byte2, Byte3) \
							((((((((uint32_t)(Byte3))<<8) | (Byte2))<<8) | (Byte1))<<8) | (Byte0))

#define BIT(n)  			(1 << n)
#define BITS2(m,n)          (BIT(m) | BIT(n) )

/* bits range: for example BITS(16,23) = 0xFF0000
 *   ==>  (BIT(m)-1)   = 0x0000FFFF     ~(BIT(m)-1)   => 0xFFFF0000
 *   ==>  (BIT(n+1)-1) = 0x00FFFFFF
 */
#define BITS(m,n)           (~(BIT(m)-1) & ((BIT(n) - 1) | BIT(n)))


/****PCM Width value @Little Endian*****/
#define WIDTH_16BIT					2
#define WIDTH_24BIT					3	//存储3字节
#define WIDTH_32BIT					4
#define WIDTH_L24BIT				0x14 //存储word 低24bit有效 ,高字节符号
#define WIDTH_H24BIT				0x24 //存储word 高24bit有效

#define	PCM_BYTE(WIDTH)				((WIDTH) & 0xF)
#define SAMPLE_BYTE(WIDTH, CHN)		(((WIDTH) & 0xF) * (CHN))

/**PCM Value Le位宽 带符号 转换**/
#define WIDTH_L24To16(Val)			((int16_t)( (Val)>>8 ))
//#define WIDTH_L24To24(Val)
#define WIDTH_16ToL24(Val)			(( (int32_t)( (int16_t)(Val) ) )<<8)
//#define WIDTH_16To24(Val)			(( (uint32_t)(Val) )<<8)
//#define WIDTH_24To16(Val)			((int16_t)( (Val)>>8 ))
#define WIDTH_24ToL24(Val)			((int32_t)((Val) & 0x800000) ? ((Val) | 0xFF000000) : ((Val) & 0xFFFFFF))

/**********************************************
Convert pcm width type(WIDTH_L24BIT/WIDTH_24BIT/WIDTH_16BIT@ LittleEndian   WIDTH_32BIT/WIDTH_H24BIT需API内启用 )
Param:
    SrcPcm    - pcm to be converted
    SrcType   - SrcPcm's type
    DstPcm    - converted pcm buffer,can be the same as SrcPcm
    DstType   - DstPcm's type
    PcmNums   - Num(SampleNum * Channels) to be converted
Return:
    Converted pcm num, 0 means no conversion
**********************************************/
uint32_t PcmWidthConvert(void* SrcPcm, uint8_t SrcType,
                       void* DstPcm, uint8_t DstType, uint32_t PcmNums);

const unsigned char *GetLibVersionDriver(void);

#ifdef __cplusplus
}
#endif//__cplusplus

#endif	//__TYPE_H__



