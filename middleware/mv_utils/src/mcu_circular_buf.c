 /**
 **************************************************************************************
 * @file    mcu_circular_buf.c
 * @brief   mcu
 *
 * @author  Sam
 * @version V1.1.0
 *
 * $Created: 2015-11-02 15:56:11$
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include <string.h>
#include "type.h"
#include "mcu_circular_buf.h"
#include "debug.h"


void MCUCircular_Config(MCU_CIRCULAR_CONTEXT* CircularBuf, void* Buf, uint32_t Len)
{
    CircularBuf->CircularBuf = Buf;
    CircularBuf->BufDepth = Len;
    CircularBuf->R = 0;
    CircularBuf->W = 0;
}

__attribute__((section(".ilm_section")))
int32_t MCUCircular_GetSpaceLen(MCU_CIRCULAR_CONTEXT* CircularBuf)
{
	//读写指针重合，只发生在MCU 循环fifo 初始化或者播空的时候（此时异常，系统强制行为）
	if(CircularBuf->R == CircularBuf->W)
	{
		return CircularBuf->BufDepth;
	}
	else
	{
		return (CircularBuf->BufDepth + CircularBuf->R - CircularBuf->W) % CircularBuf->BufDepth;
	}   
}

__attribute__((section(".ilm_section")))
void MCUCircular_PutData(MCU_CIRCULAR_CONTEXT* CircularBuf, void* InBuf, uint32_t Len)
{
    if(Len == 0)
	{
		return;
	}
	if(CircularBuf->W + Len <= CircularBuf->BufDepth)
    {
        memcpy((void *)&CircularBuf->CircularBuf[CircularBuf->W], InBuf, Len);
    }
    else
    {
        memcpy((void *)&CircularBuf->CircularBuf[CircularBuf->W], InBuf, CircularBuf->BufDepth - CircularBuf->W);
        memcpy((void *)&CircularBuf->CircularBuf[0], (uint8_t*)InBuf + CircularBuf->BufDepth - CircularBuf->W, CircularBuf->W + Len - CircularBuf->BufDepth);
    }
    CircularBuf->W = (CircularBuf->W + Len) % CircularBuf->BufDepth;    
}

void MCUCircular_PutData_Printf(MCU_CIRCULAR_CONTEXT* CircularBuf, void* InBuf, uint32_t Len)
{
    if(Len == 0||CircularBuf->CircularBuf == NULL)
	{
		return;
	}
	if(CircularBuf->W + Len <= CircularBuf->BufDepth)
    {
      CircularBuf->CircularBuf[CircularBuf->W]= *((uint8_t*)InBuf);
    }
    else
    {
        //memcpy((void *)&CircularBuf->CircularBuf[CircularBuf->W], InBuf, CircularBuf->BufDepth - CircularBuf->W);
        CircularBuf->CircularBuf[0] =*((uint8_t*)InBuf);
    }
    CircularBuf->W = (CircularBuf->W + Len) % CircularBuf->BufDepth;    
}

__attribute__((section(".ilm_section")))
uint32_t MCUCircular_GetDataLen(MCU_CIRCULAR_CONTEXT* CircularBuf)
{
	uint32_t R, W;
	uint32_t Len;
	if(CircularBuf->CircularBuf == NULL)
	{
		return 0;
	}

	R = CircularBuf->R;
	W = CircularBuf->W;

	if(R == W)
	{
	//	DBG("e\n");
		return 0;
	}
	else if(R < W)
	{
		Len = W - R;
	}
	else
	{
		Len = CircularBuf->BufDepth + W - R;
	}

	return Len;
}

__attribute__((section(".ilm_section")))
int32_t MCUCircular_GetData(MCU_CIRCULAR_CONTEXT* CircularBuf, void* OutBuf, uint32_t MaxLen)
{
    uint32_t R;//, W;
    uint32_t Len;
    

	if(MaxLen == 0)
	{
		return 0;
	}

	R = CircularBuf->R;
//	W = CircularBuf->W;

	Len = MCUCircular_GetDataLen(CircularBuf);

    if(Len > MaxLen)
    {
        Len = MaxLen;
    }
    
    if(Len + R > CircularBuf->BufDepth)
    {
        memcpy(OutBuf, (void *)&CircularBuf->CircularBuf[CircularBuf->R], CircularBuf->BufDepth - CircularBuf->R);
        memcpy((uint8_t* )OutBuf + CircularBuf->BufDepth - R, (void *)&CircularBuf->CircularBuf[0], Len + R - CircularBuf->BufDepth);
    }
    else
    {
        memcpy(OutBuf, (void *)&CircularBuf->CircularBuf[CircularBuf->R], Len);
    }
    
    R = (R + Len) % CircularBuf->BufDepth;
    
    CircularBuf->R = R;
        
    return Len;
}

int32_t MCUCircular_GetData_Printf(MCU_CIRCULAR_CONTEXT* CircularBuf, void* OutBuf, uint32_t MaxLen)
{
    uint32_t R;//, W;
    uint32_t Len;
    
	if(CircularBuf->CircularBuf == NULL)
	{
		return 0;
	}

	if(MaxLen == 0)
	{
		return 0;
	}

	R = CircularBuf->R;
//	W = CircularBuf->W;

	Len = MCUCircular_GetDataLen(CircularBuf);

    if(Len > MaxLen)
    {
        Len = MaxLen;
    }
    
    if(Len + R > CircularBuf->BufDepth)
    {
        //memcpy(OutBuf, (void *)&CircularBuf->CircularBuf[CircularBuf->R], CircularBuf->BufDepth - CircularBuf->R);
       // memcpy((uint8_t* )OutBuf + CircularBuf->BufDepth - R, (void *)&CircularBuf->CircularBuf[0], Len + R - CircularBuf->BufDepth);
		*((uint8_t* )OutBuf) = CircularBuf->CircularBuf[0];
    }
    else
    {
       // memcpy(OutBuf, (void *)&CircularBuf->CircularBuf[CircularBuf->R], Len);
       *((uint8_t* )OutBuf) = CircularBuf->CircularBuf[CircularBuf->R];
    }
    
    R = (R + Len) % CircularBuf->BufDepth;
    
    CircularBuf->R = R;
        
    return Len;
}

int32_t MCUCircular_ReadData(MCU_CIRCULAR_CONTEXT* CircularBuf, void* OutBuf, uint32_t MaxLen)
{
    uint32_t R;//, W;
    uint32_t Len;

	if(CircularBuf->CircularBuf == NULL)
	{
		return 0;
	}

	if(MaxLen == 0)
	{
		return 0;
	}

	R = CircularBuf->R;
//	W = CircularBuf->W;

	Len = MCUCircular_GetDataLen(CircularBuf);

    if(Len > MaxLen)
    {
        Len = MaxLen;
    }

    if(Len + R > CircularBuf->BufDepth)
    {
        memcpy(OutBuf, (void *)&CircularBuf->CircularBuf[CircularBuf->R], CircularBuf->BufDepth - CircularBuf->R);
        memcpy((uint8_t* )OutBuf + CircularBuf->BufDepth - R, (void *)&CircularBuf->CircularBuf[0], Len + R - CircularBuf->BufDepth);
    }
    else
    {
        memcpy(OutBuf, (void *)&CircularBuf->CircularBuf[CircularBuf->R], Len);
    }

//    R = (R + Len) % CircularBuf->BufDepth;
//
//    CircularBuf->R = R;

    return Len;
}


int32_t MCUCircular_AbortData(MCU_CIRCULAR_CONTEXT* CircularBuf, uint32_t MaxLen)
{
    uint32_t R;//, W;
    uint32_t Len;

	if(CircularBuf->CircularBuf == NULL)
	{
		return 0;
	}

	if(MaxLen == 0)
	{
		return 0;
	}

	R = CircularBuf->R;

	Len = MCUCircular_GetDataLen(CircularBuf);

    if(Len > MaxLen)
    {
        Len = MaxLen;
    }

    R = (R + Len) % CircularBuf->BufDepth;

    CircularBuf->R = R;

    return Len;
}




void MCUDCircular_Config(MCU_DOUBLE_CIRCULAR_CONTEXT* CircularBuf, void* Buf, uint32_t Len)
{
    CircularBuf->CircularBuf = Buf;
    CircularBuf->BufDepth = Len;
    CircularBuf->R1 = 0;
    CircularBuf->R2 = 0;
    CircularBuf->W = 0;
}

int32_t MCUDCircular_GetSpaceLen(MCU_DOUBLE_CIRCULAR_CONTEXT* CircularBuf)
{
	//?áD′??????o?￡???·￠éú?úMCU ?-?·fifo 3?ê??ˉ?ò??2￥??μ?ê±oò￡¨′?ê±òì3￡￡??μí3????DD?a￡?
	if(CircularBuf->R1 == CircularBuf->W)
	{
		return CircularBuf->BufDepth;
	}
	else
	{
		return (CircularBuf->BufDepth + CircularBuf->R1 - CircularBuf->W) % CircularBuf->BufDepth;
	}
}

void MCUDCircular_PutData(MCU_DOUBLE_CIRCULAR_CONTEXT* CircularBuf, void* InBuf, uint32_t Len)
{
    if(Len == 0)
	{
		return;
	}
	if(CircularBuf->W + Len <= CircularBuf->BufDepth)
    {
        memcpy((void *)&CircularBuf->CircularBuf[CircularBuf->W], InBuf, Len);
    }
    else
    {
        memcpy((void *)&CircularBuf->CircularBuf[CircularBuf->W], InBuf, CircularBuf->BufDepth - CircularBuf->W);
        memcpy((void *)&CircularBuf->CircularBuf[0], (uint8_t*)InBuf + CircularBuf->BufDepth - CircularBuf->W, CircularBuf->W + Len - CircularBuf->BufDepth);
    }
    CircularBuf->W = (CircularBuf->W + Len) % CircularBuf->BufDepth;
}

uint32_t MCUDCircular_Get1To2Len(MCU_DOUBLE_CIRCULAR_CONTEXT* CircularBuf)
{
	uint32_t R1, R2;
	uint32_t Len;

	R1 = CircularBuf->R1;
	R2 = CircularBuf->R2;

	if(R1 == R2)
	{
	//	DBG("e\n");
		return 0;
	}
	else if(R1 < R2)
	{
		Len = R2 - R1;
	}
	else
	{
		Len = CircularBuf->BufDepth + R2 - R1;
	}

	return Len;
}

uint32_t MCUDCircular_GetData1Len(MCU_DOUBLE_CIRCULAR_CONTEXT* CircularBuf)
{
	uint32_t R1, W;
	uint32_t Len;

	R1 = CircularBuf->R1;
	W = CircularBuf->W;

	if(R1 == W)
	{
	//	DBG("e\n");
		return 0;
	}
	else if(R1 < W)
	{
		Len = W - R1;
	}
	else
	{
		Len = CircularBuf->BufDepth + W - R1;
	}

	return Len;
}


int32_t MCUDCircular_GetData1(MCU_DOUBLE_CIRCULAR_CONTEXT* CircularBuf, void* OutBuf, uint32_t MaxLen)
{
    uint32_t R1;//, W;
    uint32_t Len;


	if(MaxLen == 0)
	{
		return 0;
	}

	R1 = CircularBuf->R1;
//	W = CircularBuf->W;

	Len = MCUDCircular_GetData1Len(CircularBuf);

    if(Len > MaxLen)
    {
        Len = MaxLen;
    }

    if(Len + R1 > CircularBuf->BufDepth)
    {
        memcpy(OutBuf, (void *)&CircularBuf->CircularBuf[CircularBuf->R1], CircularBuf->BufDepth - CircularBuf->R1);
        memcpy((uint8_t* )OutBuf + CircularBuf->BufDepth - R1, (void *)&CircularBuf->CircularBuf[0], Len + R1 - CircularBuf->BufDepth);
    }
    else
    {
        memcpy(OutBuf, (void *)&CircularBuf->CircularBuf[CircularBuf->R1], Len);
    }

    R1 = (R1 + Len) % CircularBuf->BufDepth;
    if(Len > MCUDCircular_Get1To2Len(CircularBuf))
    {
    	CircularBuf->R2 = R1;
    }
    CircularBuf->R1 = R1;

    return Len;
}



int32_t MCUDCircular_ReadData1(MCU_DOUBLE_CIRCULAR_CONTEXT* CircularBuf, void* OutBuf, uint32_t MaxLen)
{
    uint32_t R1;//, W;
    uint32_t Len;


	if(MaxLen == 0)
	{
		return 0;
	}

	R1 = CircularBuf->R1;
//	W = CircularBuf->W;

	Len = MCUDCircular_GetData1Len(CircularBuf);

    if(Len > MaxLen)
    {
        Len = MaxLen;
    }

    if(Len + R1 > CircularBuf->BufDepth)
    {
        memcpy(OutBuf, (void *)&CircularBuf->CircularBuf[CircularBuf->R1], CircularBuf->BufDepth - CircularBuf->R1);
        memcpy((uint8_t* )OutBuf + CircularBuf->BufDepth - R1, (void *)&CircularBuf->CircularBuf[0], Len + R1 - CircularBuf->BufDepth);
    }
    else
    {
        memcpy(OutBuf, (void *)&CircularBuf->CircularBuf[CircularBuf->R1], Len);
    }
    return Len;
}






uint32_t MCUDCircular_GetData2Len(MCU_DOUBLE_CIRCULAR_CONTEXT* CircularBuf)
{
	uint32_t R2, W;
	uint32_t Len;

	R2 = CircularBuf->R2;
	W = CircularBuf->W;

	if(R2 == W)
	{
	//	DBG("e\n");
		return 0;
	}
	else if(R2 < W)
	{
		Len = W - R2;
	}
	else
	{
		Len = CircularBuf->BufDepth + W - R2;
	}

	return Len;
}


int32_t MCUDCircular_GetData2(MCU_DOUBLE_CIRCULAR_CONTEXT* CircularBuf, void* OutBuf, uint32_t MaxLen)
{
    uint32_t R2;//, W;
    uint32_t Len;

	if(MaxLen == 0)
	{
		return 0;
	}

	R2 = CircularBuf->R2;
//	W = CircularBuf->W;

	Len = MCUDCircular_GetData2Len(CircularBuf);

    if(Len > MaxLen)
    {
        Len = MaxLen;
    }

    if(Len + R2 > CircularBuf->BufDepth)
    {
        memcpy(OutBuf, (void *)&CircularBuf->CircularBuf[CircularBuf->R2], CircularBuf->BufDepth - CircularBuf->R2);
        memcpy((uint8_t* )OutBuf + CircularBuf->BufDepth - R2, (void *)&CircularBuf->CircularBuf[0], Len + R2 - CircularBuf->BufDepth);
    }
    else
    {
        memcpy(OutBuf, (void *)&CircularBuf->CircularBuf[CircularBuf->R2], Len);
    }

    R2 = (R2 + Len) % CircularBuf->BufDepth;

    CircularBuf->R2 = R2;

    return Len;
}
