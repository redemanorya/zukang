/*
 * dprod_interface.c
 *
 *  Created on: 2026年3月23日
 *      Author: mike
 */

#include "dprod.h"

int32_t dprod_s8(int8_t *u, int8_t *v, int dim_vec)
{
	Dprod_Symbol_SetA(S8_S8);//s8*s8
	dprod_configurateA(dim_vec, u, v);
	return dprod_s8_wait_for_result_A();
}

uint32_t dprod_u8(uint8_t *u, uint8_t *v, int dim_vec)
{
	Dprod_Symbol_SetA(U8_U8);//u8*u8
	dprod_configurateA(dim_vec, u, v);
	return dprod_u8_wait_for_result_A();
}

int32_t dprod_s8u8(int8_t *u, uint8_t *v, int dim_vec)
{
	Dprod_Symbol_SetA(S8_U8);//s8*u8
	dprod_configurateA(dim_vec, u, v);
	return dprod_s8_wait_for_result_A();
}

int32_t dprod_u8s8(uint8_t *u, int8_t *v, int dim_vec)
{
	Dprod_Symbol_SetA(U8_S8);//u8*s8
	dprod_configurateA(dim_vec, u, v);
	return dprod_s8_wait_for_result_A();
}

void dprod_vm_s8(int8_t *v, int8_t *m, int dim_vec, int num_rows, int32_t *out)  //m行n列*n行1列 = m行1列
{
	Dprod_Symbol_SetA(S8_S8);
	Dprod_Symbol_SetB(S8_S8);
	int i;
    for (i = 0; i < num_rows; i++)
    {
        if (i == 0)
        {
            dprod_configurateA(dim_vec, v, m);

        }
        else // 取上一次的值
        {
        	out[i - 1] = (((i + 1) & 1) == 0) ? dprod_s8_wait_for_result_A() : dprod_s8_wait_for_result_B();
        }
        if (i < num_rows - 1)
        {
            // prepare_data();  //准备数据
            (((i + 1) & 1) == 0) ? dprod_configurateA(dim_vec, v, m + (dim_vec*(i + 1))) : dprod_configurateB(dim_vec, v, m + (dim_vec*(i + 1)));  // 这一部分34个cycle
        }
    }
    out[num_rows - 1] = (((num_rows - 1) & 1) == 0) ? dprod_s8_wait_for_result_A() : dprod_s8_wait_for_result_B();   //取最后一个值
}

void dprod_vm_u8(uint8_t *v, uint8_t *m, int dim_vec, int num_rows, uint32_t *out)
{
	Dprod_Symbol_SetA(U8_U8);//u8*u8
	Dprod_Symbol_SetB(U8_U8);//u8*u8
	int i;
    for (i = 0; i < num_rows; i++)
    {
        if (i == 0)
        {
            dprod_configurateA(dim_vec, v, m);
        }
        else // 取上一次的值
        {
        	out[i - 1] = (((i + 1) & 1) == 0) ? dprod_u8_wait_for_result_A() : dprod_u8_wait_for_result_B();
        }
        if (i < num_rows - 1)
        {
            // prepare_data();  //准备数据
            (((i + 1) & 1) == 0) ? dprod_configurateA(dim_vec, v, m + (dim_vec*(i + 1))) : dprod_configurateB(dim_vec, v, m + (dim_vec*(i + 1))); // 这一部分34个cycle
        }
    }
    out[num_rows - 1] = (((num_rows - 1) & 1) == 0) ? dprod_u8_wait_for_result_A() : dprod_u8_wait_for_result_B();   //取最后一个值
}
