/*
 * dprod_interface.h
 *
 *  Created on: 2026年3月23日
 *      Author: mike
 */

/**
 * @brief 计算输入向量与向量的点积,计算并返回结果
 * @param[in] u 指向输入向量的地址1,有符号数。地址是字节单位,但必须是4字节对齐
 * @param[in] v 指向输入向量的地址2,有符号数。地址是字节单位,但必须是4字节对齐
 * @param[in] dim_vec 向量的长度 ,范围：1-4095(包含1、4095)
 * @return 输出计算结果
 * @note dim_vec值如果超出范围限制,可能会产生未知影响
 * @note dim_vec < 19 建议使用软件方式计算
 */
int32_t dprod_s8(int8_t *u, int8_t *v, int dim_vec);

/**
 * @brief 计算输入向量与向量的点积,计算并返回结果
 * @param[in] u 指向输入向量的地址1,无符号数。地址是字节单位,但必须是4字节对齐
 * @param[in] v 指向输入向量的地址2,无符号数。地址是字节单位,但必须是4字节对齐
 * @param[in] dim_vec 向量的长度 ,范围：1-4095(包含1、4095)
 * @return 输出计算结果
 * @note dim_vec值如果超出范围限制,可能会产生未知影响
 * @note dim_vec < 19 建议使用软件方式计算
 */
uint32_t dprod_u8(uint8_t *u, uint8_t *v, int dim_vec);

/**
 * @brief 计算输入向量与向量的点积,计算并返回结果
 * @param[in] u 指向输入向量的地址1,有符号数。地址是字节单位,但必须是4字节对齐
 * @param[in] v 指向输入向量的地址2,无符号数。地址是字节单位,但必须是4字节对齐
 * @param[in] dim_vec 向量的长度 ,范围：1-4095(包含1、4095)
 * @return 输出计算结果
 * @note dim_vec值如果超出范围限制,可能会产生未知影响
 * @note dim_vec < 19 建议使用软件方式计算
 */
int32_t dprod_s8u8(int8_t *u, uint8_t *v, int dim_vec);

/**
 * @brief 计算输入向量与向量的点积,计算并返回结果
 * @param[in] u 指向输入向量的地址1,无符号数。地址是字节单位,但必须是4字节对齐
 * @param[in] v 指向输入向量的地址2,有符号数。地址是字节单位,但必须是4字节对齐
 * @param[in] dim_vec 向量的长度,范围：1-4095(包含1、4095)
 * @return 输出计算结果
 * @note dim_vec值如果超出范围限制,可能会产生未知影响
 * @note dim_vec < 19 建议使用软件方式计算
 */
int32_t dprod_u8s8(uint8_t *u, int8_t *v, int dim_vec);

/**
 * @brief 计算输入向量与权重矩阵的点积,并将结果存储到输出指定地址中
 * @param[in] v 指向输入向量的地址,有符号数。地址是字节单位,但必须是4字节对齐
 * @param[in] m 指向权重矩阵的指针,有符号数。以行优先方式排列的数组,地址是字节单位,但必须是4字节对齐
 * @param[in] dim_vec 向量的长度,范围：1-4095(包含1、4095),长度必须要是4的倍数
 * @param[in] num_rows 权重矩阵的行数( >0 )
 * @param[out] out 指向输出向量的地址,(类型为int32_t)
 * @note dim_vec值如果超出范围限制,可能会产生未知影响
 * @note dim_vec < 8 建议使用软件方式计算
 */
void dprod_vm_s8(int8_t *v, int8_t *m, int dim_vec, int num_rows, int32_t *out);

/**
 * @brief 计算输入向量与权重矩阵的点积,并将结果存储到输出指定地址中
 * @param[in] v 指向输入向量的地址,无符号数。地址是字节单位,但必须是4字节对齐
 * @param[in] m 指向权重矩阵的指针,无符号数。以行优先方式排列的数组,地址是字节单位,但必须是4字节对齐
 * @param[in] dim_vec 向量的长度 ,范围：1-4095(包含1、4095),长度必须要是4的倍数
 * @param[in] num_rows 权重矩阵的行数( >0 )
 * @param[out] out 指向输出向量的地址,(类型为uint32_t)
 * @note dim_vec值如果超出范围限制,可能会产生未知影响
 * @note dim_vec < 8 建议使用软件方式计算
 */
void dprod_vm_u8(uint8_t *v, uint8_t *m, int dim_vec, int num_rows, uint32_t *out);