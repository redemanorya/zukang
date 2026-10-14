/******************************************************************************
 * @file    dprod.h
 * @author  Mike
 * @version V1.2
 * @date    2025-09-28
 * @brief   dprod function implement
 ******************************************************************************
 */
 /**
 * @addtogroup DPROD
 * @{
 * @defgroup dprod dprod.h
 * @{
 */
#include "type.h"

typedef enum _DPROD_READ_MODE
{
	SINGLE = 0,
	BURST = 1
}DPROD_READ_MODE;

typedef enum _DPROD_SYMBOL
{
    S8_S8 = 0,
	S8_U8 = 1,
	U8_S8 = 2,
	U8_U8 = 3,
}DPROD_SYMBOL;

/**
 * @brief 配置 A 组寄存器并启动计算
 * @param[in] SIZE 向量长度
 * @param[in] ADDA 输入数据 A 起始地址,地址是字节单位,但必须是4字节对齐
 * @param[in] ADDB 输入数据 B 起始地址,地址是字节单位,但必须是4字节对齐
 */
void dprod_configurateA(int SIZE, void *ADDA, void *ADDB);

/**
 * @brief 配置 B 组寄存器并启动计算
 * @param[in] SIZE 向量长度
 * @param[in] ADDA 输入数据 A 起始地址,地址是字节单位,但必须是4字节对齐
 * @param[in] ADDB 输入数据 B 起始地址,地址是字节单位,但必须是4字节对齐
 */
void dprod_configurateB(int SIZE, void *ADDA, void *ADDB);

/**
 * @brief 设置 A 组输入数据的符号模式
 * @param[in] SYMBOL S8_S8, S8_U8, U8_S8, U8_U8
 */
void Dprod_Symbol_SetA(DPROD_SYMBOL SYMBOL);

/**
 * @brief 设置 B 组输入数据的符号模式
 * @param[in] SYMBOL S8_S8, S8_U8, U8_S8, U8_U8
 */
void Dprod_Symbol_SetB(DPROD_SYMBOL SYMBOL);

/**
 * @brief 等待 A 组计算完成并返回结果（有符号）
 * @return 计算结果
 */
int32_t dprod_s8_wait_for_result_A(void);

/**
 * @brief 等待 B 组计算完成并返回结果（有符号）
 * @return 计算结果
 */
int32_t dprod_s8_wait_for_result_B(void);

/**
 * @brief 等待 A 组计算完成并返回结果（无符号）
 * @return 计算结果
 */
uint32_t dprod_u8_wait_for_result_A(void);

/**
 * @brief 等待 B 组计算完成并返回结果（无符号）
 * @return 计算结果
 */
uint32_t dprod_u8_wait_for_result_B(void);

/**
 * @brief 设置加速器读取数据的模式
 * param  Mode：SINGLE or BURST
 * @Note  常规使用SINGLE Mode.
 */
void dprod_set_read_mode(DPROD_READ_MODE Mode);

/**
 * @brief  A 配置是否完成
 * @return 1-完成，0-未完成
 */
uint32_t dprod_is_result_ready_A(void);

/**
 * @brief B 配置是否完成
 * @return 1-完成，0-未完成
 */
uint32_t dprod_is_result_ready_B(void);

/**
 * @}
 * @}
 */
 