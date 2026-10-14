/**
  *****************************************************************************
  * @lib:			character_encoding
  * @author			Lilu
  * @version		V1.0.0
  * @data			2015-8-14
  * @Brief			character_encoding库支持多国语言的ANSI编码与unicode编码的相互转换
  *					输入、输出数据的大小端模式可配置
  *					unicode编码格式支持：UTF-8、UTF-16（以下unicode默认指代UTF-16）
  *					UNICODE指UCS-2，不考虑支持BMP之外的不常用字符
  *					(注:本程序中UNICODE,UTF-16,UCS-2三者含义等同)
  ******************************************************************************
 */
#ifndef __CHARACTER_ENCODING_H__
#define __CHARACTER_ENCODING_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus	
	
#include "type.h"

#define _CODE_PAGE	936
/* This option specifies the OEM code page to be used on the target system.
/  Incorrect setting of the code page can cause a file open failure.
/
/   1    - ASCII (No extended character. Non-LFN cfg. only)
/   437  - U.S.
/   720  - Arabic
/   737  - Greek
/   775  - Baltic
/   850  - Multilingual Latin 1
/   852  - Latin 2
/   855  - Cyrillic
/   857  - Turkish
/   858  - Multilingual Latin 1 + Euro
/   862  - Hebrew
/   866  - Russian
/   874  - Thai
/   932  - Japanese Shift_JIS (DBCS)
/   936  - Simplified Chinese GBK (DBCS)
/   949  - Korean (DBCS)
/   950  - Traditional Chinese Big5 (DBCS)
*/

#define	ANSI_TO_UNICODE		0x01//字符编码转换方向
#define	UNICODE_TO_ANSI		0x00

#define	UTF8				0x02//UNICODE码采用的编码形式，默认为UTF16即UCS-2
#define	UTF16				0x00

#define	BIG_ENDIAN_IN		0x04//输入字符串大小端，默认为小端
#define	LITTLE_ENDIAN_IN	0x00

#define	BIG_ENDIAN_OUT		0x08//输出字符串大小端，默认为小端
#define	LITTLE_ENDIAN_OUT	0x00

/**
 * @brief
 *   字符编码字符串转换，UNICODE与ANSI互转
 * @param	dst			[input]目的字符串指针
 * @param	dst_len		[input]目的字符串长度，byte单位
 * @param	src			[input]源字符串指针
 * @param	dst_len		[input]源字符串长度，byte单位
 * @param	type		[input]转换类型，参见h文件中宏定义
 *
 * @return
 *   目的字符串的实际长度，0表示转换失败
 * @note	转换类型UNICODE_TO_ANSI或ANSI_TO_UNICODE控制转换方向，unicode编码方式及大小端为可选条件
 *			例:	UNICODE_TO_ANSI | UTF8 | BIG_ENDIAN_OUT为utf8编码向ANSI转换，输入、输出均为大端模式
 *				UNICODE_TO_ANSI 默认为UTF16向ANSI转换，输入输出均为小端模式
 *				UTF8编码 没有大小端的区分
 */
unsigned int character_encoding_string_convert(unsigned char* dst, unsigned int dst_len,
							unsigned char* src, unsigned int src_len, unsigned int type);

/**
 * @brief
 *	字符编码转换，UNICODE与ANSI互转
 * @param	chr		源字符
 * @param	dir		转换方向，UNICODE_TO_ANSI或ANSI_TO_UNICODE
 *
 * @return
 *	目的字符，0表示转换失败
 */
unsigned short character_encoding_convert (unsigned short chr, unsigned int	dir);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif/*__CHARACTER_ENCODING_H__*/


