#include <string.h>
#include "type.h"
#include "gpio.h"
#include "dma.h"
#include "delay.h"
#include "timeout.h"
#include "reset.h"
#include "irqn.h"
#include "debug.h"
#include "pwm.h"
#include "app_config.h"
#include "pwm_dma_led.h"

#ifdef CFG_DMA_RGB_LED_EN
TIMER gLedSwitchTime;
TIMER gRGBTestTime;
TIMER gRGBScanTime;
#ifdef CFG_MORE_GPIO_RGB_CTRL_EN
TIMER gRGBScanTime1;
uint8_t DataChangeFlag;
#endif

uint8_t  DmaTestBuffer[LED_BUFFER_LEN]={0};

/*
*********************************************************
*				IO口和DMA通道初始化
*********************************************************
*/
void Led_Dma_Proc_Init(void)
{
	DMA_CONFIG DMAConfig;
	uint32_t DmaTestLen;
	DMA_ERROR error;
	memset(DmaTestBuffer,0,sizeof(DmaTestBuffer));

	DmaTestLen = LED_BUFFER_LEN;//LED_BUFFER_LEN*4;
	DMAConfig.Dir = DMA_CHANNEL_DIR_MEM2PERI;
	DMAConfig.Mode = DMA_BLOCK_MODE; //DMA_CIRCULAR_MODE;
	DMAConfig.SrcAddress = (uint32_t)DmaTestBuffer;
	DMAConfig.SrcAddrIncremental = DMA_SRC_AINCR_SRC_WIDTH;
	DMAConfig.DstAddress = TIMER_ADDR;
	DMAConfig.DataWidth =  DMA_DWIDTH_BYTE;
	DMAConfig.DstAddrIncremental = DMA_DST_AINCR_NO;
	DMAConfig.BufferLen = DmaTestLen;
	error = DMA_TimerConfig(DMA_MASK, &DMAConfig);DBG("error0 = %d",error);
	error = DMA_BlockConfig(DMA_MASK);DBG("error1 = %d",error);
	error = DMA_BlockBufSet(DMA_MASK,DmaTestBuffer,DmaTestLen);DBG("error2 = %d",error);
	DMA_ChannelEnable(DMA_MASK);
	Timer_Config(TIMER_ID, 1, 0);
	Timer_Start(TIMER_ID);
	TimeOutSet(&gRGBScanTime,20);

	PWM_StructInit	PWMParam;
	PWMParam.CounterMode			= PWM_COUNTER_MODE_UP;
	PWMParam.OutputType 			= PWM_OUTPUT_SINGLE_1;
	PWMParam.DMAReqEnable			= PWM_REQ_DMA_MODE;//PWM_REQ_DMA_MODE;
	PWMParam.FreqDiv				= LED_T; //120M系统频率下 12000 = 100us
	PWMParam.Duty[0]				= 50;
	PWMParam.Duty[1]				= 50;
	PWMParam.PWMMultiEnable			= (PWM_OUT_CH == PWM_CHANNEL0)?0:1;
	PWMParam.MasterModeSel          = 0;
	PWMParam.MasterSlaveSel         = 0;
	PWMParam.TriggerInSrc           = 0;
	PWMParam.CcxmAndCccUpdataSel    = 0;

	PWM_GpioConfig(PWM_CHANNEL,PWM_INDEX,PWM_IO_MODE_OUT,PWM_OUT_CH);
	PWM_Config(TIMER_ID,&PWMParam);
	PWM_Enable(TIMER_ID);

	TimeOutSet(&gLedSwitchTime, 2000);
}

/*
******************************************************
*				将数据传送到DMA通道中
******************************************************
*/
void DmaDataChange(void)
{
	uint16_t j;
	uint8_t i;
	for(i=0;i<DMA_RGB_LED_NUM;i++)
	{
		for(j=0;j<8;j++)
		{
			if((DMA_RGB_G.buf[i]<<j)&0x80)
			{
				DmaTestBuffer[8*3*i+j+4] = HT;
			}
			else
			{
				DmaTestBuffer[8*3*i+j+4] = LT;
			}
		}

		for(j=0;j<8;j++)
		{
			if((DMA_RGB_B.buf[i]<<j)&0x80)
			{
				DmaTestBuffer[8*3*i+8+j+4] = HT;
			}
			else
			{
				DmaTestBuffer[8*3*i+8+j+4] = LT;
			}
		}

		for(j=0;j<8;j++)
		{
			if((DMA_RGB_R.buf[i]<<j)&0x80)
			{
				DmaTestBuffer[8*3*i+16+j+4] = HT;
			}
			else
			{
				DmaTestBuffer[8*3*i+16+j+4] = LT;
			}
		}
	}
}

#ifdef CFG_MORE_GPIO_RGB_CTRL_EN
void DmaDataChange1(void)
{
	uint32_t TempData;
	uint16_t j;
	uint8_t i;

	for(i=0;i<(DMA_RGB_LED_NUM_1*3);i++)
	{
		for(j=0;j<32;j++)
		{
			TempData = 0;
			if(dma_rgb_buf1[i] &  BIT((31-j)))
			{
				TempData = LED_GPIO2;//TempData = LED_PORT;
			}
			DmaTestBuffer1[ i*32+j] =(uint8_t) TempData;
		}
	}
}
#endif

/*
****************************************************
*			刷新DMA通道数据
****************************************************
*/
void LedDmaDataMode(void)
{
	uint32_t DmaTestLen;
	DmaTestLen = LED_BUFFER_LEN;
	if(IsTimeOut(&gLedSwitchTime))
	{
		TimeOutSet(&gLedSwitchTime, 80);
		if(DMA_InterruptFlagGet(DMA_MASK, DMA_DONE_INT))
		{
			DMA_InterruptFlagClear(DMA_MASK, DMA_DONE_INT);
			DmaDataChange();
			DMA_BlockBufSet(DMA_MASK,DmaTestBuffer,DmaTestLen);
			DMA_ChannelEnable(DMA_MASK);
		}
	}
}

#ifdef MV_APP_CONTROL
//APP控制
void mv_app_get_rgb(uint8_t r,uint8_t g,uint8_t b)
{
	DmaRgbFifo.rgb_mode = 4;
	DmaRgbFifo.r_duty = b;
	DmaRgbFifo.g_duty = g;
	DmaRgbFifo.b_duty = r;
}

void mv_app_rgb_onoff(uint8_t onoff)
{
	if(onoff)
	{
		DmaRgbFifo.rgb_mode = 4;
	}else{
		DmaRgbFifo.rgb_mode = 3;
	}
}
#endif
/*
//////////////////////////////////////////////////////////////////
//					RGB闪烁模式处理								//
//////////////////////////////////////////////////////////////////
*/
void RgbModeRun(void)
{
	uint8_t i;
	switch(DmaRgbFifo.rgb_mode)
	{
		case 0:					//只做所有灯呼吸灯渐变
			if(IsTimeOut(&gRGBScanTime))
			{
				TimeOutSet(&gRGBScanTime,10);
				if((DmaRgbFifo.r_duty < 255)&&(DmaRgbFifo.g_duty == 0)&&(DmaRgbFifo.b_duty == 0))
				{
					DmaRgbFifo.r_duty++;
				}
				else if((DmaRgbFifo.r_duty > 0)&&(DmaRgbFifo.g_duty < 255)&&(DmaRgbFifo.b_duty == 0))
				{
					DmaRgbFifo.r_duty--;
					DmaRgbFifo.g_duty++;
				}
				else if((DmaRgbFifo.r_duty == 0)&&(DmaRgbFifo.g_duty > 0)&&(DmaRgbFifo.b_duty < 255))
				{
					DmaRgbFifo.g_duty--;
					DmaRgbFifo.b_duty++;
				}
				else if((DmaRgbFifo.r_duty < 255)&&(DmaRgbFifo.g_duty == 0)&&(DmaRgbFifo.b_duty > 0))
				{
					DmaRgbFifo.b_duty--;
					DmaRgbFifo.r_duty++;
				}
				for(i=0;i<DMA_RGB_LED_NUM;i++)		//此处将每次颜色渐变的数据传输到
				{
					DMA_RGB_R.buf[i] = DmaRgbFifo.r_duty;
					DMA_RGB_G.buf[i] = DmaRgbFifo.g_duty;
					DMA_RGB_B.buf[i] = DmaRgbFifo.b_duty;
				}
			}
			break;
		case 1:						//灯圈一边颜色渐变一边环绕亮灯
			if(IsTimeOut(&gRGBScanTime))
			{
				TimeOutSet(&gRGBScanTime,80);
				if(DmaRgbFifo.rgb_duty_add1>0)
				{
					DMA_RGB_R.buf[DmaRgbFifo.rgb_duty_add1-1] = 0;
					DMA_RGB_G.buf[DmaRgbFifo.rgb_duty_add1-1] = 0;
					DMA_RGB_B.buf[DmaRgbFifo.rgb_duty_add1-1] = 0;
				}
				if((DmaRgbFifo.r_duty < 255)&&(DmaRgbFifo.g_duty == 0)&&(DmaRgbFifo.b_duty == 0))
				{
					DmaRgbFifo.r_duty+=15;
				}
				else if((DmaRgbFifo.r_duty > 0)&&(DmaRgbFifo.g_duty < 255)&&(DmaRgbFifo.b_duty == 0))
				{
					DmaRgbFifo.r_duty-=15;
					DmaRgbFifo.g_duty+=15;
				}
				else if((DmaRgbFifo.r_duty == 0)&&(DmaRgbFifo.g_duty > 0)&&(DmaRgbFifo.b_duty < 255))
				{
					DmaRgbFifo.g_duty-=15;
					DmaRgbFifo.b_duty+=15;
				}
				else if((DmaRgbFifo.r_duty < 255)&&(DmaRgbFifo.g_duty == 0)&&(DmaRgbFifo.b_duty > 0))
				{
					DmaRgbFifo.b_duty-=15;
					DmaRgbFifo.r_duty+=15;
				}
				if(DmaRgbFifo.rgb_duty_add1 < DMA_RGB_LED_NUM)			//一个灯圈中灯的总数
				{
					DmaRgbFifo.rgb_duty_add1++;
				}
				else
					DmaRgbFifo.rgb_duty_add1 = 0;
				for(i=0;i<3;i++)
				{
					if((DmaRgbFifo.rgb_duty_add1+i)>=DMA_RGB_LED_NUM)
					{
						DMA_RGB_R.buf[DmaRgbFifo.rgb_duty_add1+i-DMA_RGB_LED_NUM] = DmaRgbFifo.r_duty;
						DMA_RGB_G.buf[DmaRgbFifo.rgb_duty_add1+i-DMA_RGB_LED_NUM] = DmaRgbFifo.g_duty;
						DMA_RGB_B.buf[DmaRgbFifo.rgb_duty_add1+i-DMA_RGB_LED_NUM] = DmaRgbFifo.b_duty;
					}
					else
					{
						DMA_RGB_R.buf[DmaRgbFifo.rgb_duty_add1+i] = DmaRgbFifo.r_duty;
						DMA_RGB_G.buf[DmaRgbFifo.rgb_duty_add1+i] = DmaRgbFifo.g_duty;
						DMA_RGB_B.buf[DmaRgbFifo.rgb_duty_add1+i] = DmaRgbFifo.b_duty;
					}
				}
			}
			break;
		case 2:
			if(IsTimeOut(&gRGBScanTime))
			{
				TimeOutSet(&gRGBScanTime,80);
				for(i=0; i<DMA_RGB_LED_NUM; i++)
				{
					DMA_RGB_R.buf[i] = 0;
					DMA_RGB_G.buf[i] = 0;
					DMA_RGB_B.buf[i] = 0;
				}
				for(i=0;i<2;i++)
				{
					if((DmaRgbFifo.rgb_duty_add2+i)>=DMA_RGB_LED_NUM)
					{
						DMA_RGB_R.buf[DmaRgbFifo.rgb_duty_add2+i-DMA_RGB_LED_NUM] = DmaRgbFifo.r_duty;
						DMA_RGB_G.buf[DmaRgbFifo.rgb_duty_add2+i-DMA_RGB_LED_NUM] = DmaRgbFifo.g_duty;
						DMA_RGB_B.buf[DmaRgbFifo.rgb_duty_add2+i-DMA_RGB_LED_NUM] = DmaRgbFifo.b_duty;
					}
					else
					{
						DMA_RGB_R.buf[DmaRgbFifo.rgb_duty_add2+i] = DmaRgbFifo.r_duty;
						DMA_RGB_G.buf[DmaRgbFifo.rgb_duty_add2+i] = DmaRgbFifo.g_duty;
						DMA_RGB_B.buf[DmaRgbFifo.rgb_duty_add2+i] = DmaRgbFifo.b_duty;
					}
					if((DmaRgbFifo.rgb_duty_add1-i)<=0)
					{
						DMA_RGB_R.buf[DmaRgbFifo.rgb_duty_add1-i+(DMA_RGB_LED_NUM/2)] = DmaRgbFifo.r_duty;
						DMA_RGB_G.buf[DmaRgbFifo.rgb_duty_add1-i+(DMA_RGB_LED_NUM/2)] = DmaRgbFifo.g_duty;
						DMA_RGB_B.buf[DmaRgbFifo.rgb_duty_add1-i+(DMA_RGB_LED_NUM/2)] = DmaRgbFifo.b_duty;
					}
					else
					{
						DMA_RGB_R.buf[DmaRgbFifo.rgb_duty_add1-i] = DmaRgbFifo.r_duty;
						DMA_RGB_G.buf[DmaRgbFifo.rgb_duty_add1-i] = DmaRgbFifo.g_duty;
						DMA_RGB_B.buf[DmaRgbFifo.rgb_duty_add1-i] = DmaRgbFifo.b_duty;
					}
				}
				if(!DmaRgbFifo.rgb_add_turn)
				{
					if(DmaRgbFifo.rgb_duty_add1 > 0)
					{
						DmaRgbFifo.rgb_duty_add1--;
						DmaRgbFifo.rgb_duty_add2++;
					}
					else
					{
						DmaRgbFifo.rgb_duty_add1=0;
						DmaRgbFifo.rgb_duty_add2=DMA_RGB_LED_NUM-1;
						DmaRgbFifo.rgb_add_turn = 1;
					}
				}
				else
				{
					if(DmaRgbFifo.rgb_duty_add1 < ((DMA_RGB_LED_NUM/2)-1))
					{
						DmaRgbFifo.rgb_duty_add1++;
						DmaRgbFifo.rgb_duty_add2--;
					}
					else
					{
						DmaRgbFifo.rgb_duty_add1=((DMA_RGB_LED_NUM/2)-1);
						DmaRgbFifo.rgb_duty_add2=DMA_RGB_LED_NUM/2;
						DmaRgbFifo.rgb_add_turn = 0;
					}
				}
			}
			break;
		case 3:
			if(IsTimeOut(&gRGBScanTime))
			{
				TimeOutSet(&gRGBScanTime,80);
				for(i=0; i<DMA_RGB_LED_NUM; i++)
				{
					DMA_RGB_R.buf[i] = 0;
					DMA_RGB_G.buf[i] = 0;
					DMA_RGB_B.buf[i] = 0;
				}
			}
			break;

		case 4:
			if(IsTimeOut(&gRGBScanTime))
			{
				TimeOutSet(&gRGBScanTime,80);
				for(i=0; i<DMA_RGB_LED_NUM; i++)
				{
					DMA_RGB_R.buf[i] = DmaRgbFifo.r_duty;
					DMA_RGB_G.buf[i] = DmaRgbFifo.g_duty;
					DMA_RGB_B.buf[i] = DmaRgbFifo.b_duty;
				}
			}
			break;
	}
}

#ifdef CFG_MORE_GPIO_RGB_CTRL_EN
void ModeTest1(void)
{
	uint8_t i;
	DmaRgbFifo.rgb_mode1 = 1;
	switch(DmaRgbFifo.rgb_mode1)
	{
		case 0:					//只做所有灯呼吸灯渐变
			if(IsTimeOut(&gRGBScanTime1))
			{
				TimeOutSet(&gRGBScanTime1,10);
				if((DmaRgbFifo.r_duty1 < 255)&&(DmaRgbFifo.g_duty1 == 0)&&(DmaRgbFifo.b_duty1 == 0))
				{
					DmaRgbFifo.r_duty1++;
				}
				else if((DmaRgbFifo.r_duty1 > 0)&&(DmaRgbFifo.g_duty1 < 255)&&(DmaRgbFifo.b_duty1 == 0))
				{
					DmaRgbFifo.r_duty1--;
					DmaRgbFifo.g_duty1++;
				}
				else if((DmaRgbFifo.r_duty1 == 0)&&(DmaRgbFifo.g_duty1 > 0)&&(DmaRgbFifo.b_duty1 < 255))
				{
					DmaRgbFifo.g_duty1--;
					DmaRgbFifo.b_duty1++;
				}
				else if((DmaRgbFifo.r_duty1 < 255)&&(DmaRgbFifo.g_duty1 == 0)&&(DmaRgbFifo.b_duty1 > 0))
				{
					DmaRgbFifo.b_duty1--;
					DmaRgbFifo.r_duty1++;
				}
				for(i=0;i<DMA_RGB_LED_NUM_1;i++)
				{
					DMA_RGB_R.buf1[i] = DmaRgbFifo.r_duty1;
					DMA_RGB_G.buf1[i] = DmaRgbFifo.g_duty1;
					DMA_RGB_B.buf1[i] = DmaRgbFifo.b_duty1;
				}
			}
			break;
		case 1:
			if(IsTimeOut(&gRGBScanTime1))
			{
				TimeOutSet(&gRGBScanTime1,100);
				if(DmaRgbFifo.rgb_duty_add1>0)
				{
					DMA_RGB_R.buf1[DmaRgbFifo.rgb_duty_add1-1] = 0;
					DMA_RGB_G.buf1[DmaRgbFifo.rgb_duty_add1-1] = 0;
					DMA_RGB_B.buf1[DmaRgbFifo.rgb_duty_add1-1] = 0;
				}
				if((DmaRgbFifo.r_duty1 < 255)&&(DmaRgbFifo.g_duty1 == 0)&&(DmaRgbFifo.b_duty1 == 0))
				{
					DmaRgbFifo.r_duty1+=15;
				}
				else if((DmaRgbFifo.r_duty1 > 0)&&(DmaRgbFifo.g_duty1 < 255)&&(DmaRgbFifo.b_duty1 == 0))
				{
					DmaRgbFifo.r_duty1-=15;
					DmaRgbFifo.g_duty1+=15;
				}
				else if((DmaRgbFifo.r_duty1 == 0)&&(DmaRgbFifo.g_duty1 > 0)&&(DmaRgbFifo.b_duty1 < 255))
				{
					DmaRgbFifo.g_duty1-=15;
					DmaRgbFifo.b_duty1+=15;
				}
				else if((DmaRgbFifo.r_duty1 < 255)&&(DmaRgbFifo.g_duty1 == 0)&&(DmaRgbFifo.b_duty1 > 0))
				{
					DmaRgbFifo.b_duty1-=15;
					DmaRgbFifo.r_duty1+=15;
				}
				if(DmaRgbFifo.rgb_duty_add1 < DMA_RGB_LED_NUM_1)
				{
					DmaRgbFifo.rgb_duty_add1++;
				}
				else
					DmaRgbFifo.rgb_duty_add1 = 0;
				for(i=0;i<3;i++)
				{
					if((DmaRgbFifo.rgb_duty_add1+i)>=DMA_RGB_LED_NUM_1)
					{
						DMA_RGB_R.buf1[DmaRgbFifo.rgb_duty_add1+i-DMA_RGB_LED_NUM_1] = DmaRgbFifo.r_duty1;
						DMA_RGB_G.buf1[DmaRgbFifo.rgb_duty_add1+i-DMA_RGB_LED_NUM_1] = DmaRgbFifo.g_duty1;
						DMA_RGB_B.buf1[DmaRgbFifo.rgb_duty_add1+i-DMA_RGB_LED_NUM_1] = DmaRgbFifo.b_duty1;
					}
					else
					{
						DMA_RGB_R.buf1[DmaRgbFifo.rgb_duty_add1+i] = DmaRgbFifo.r_duty1;
						DMA_RGB_G.buf1[DmaRgbFifo.rgb_duty_add1+i] = DmaRgbFifo.g_duty1;
						DMA_RGB_B.buf1[DmaRgbFifo.rgb_duty_add1+i] = DmaRgbFifo.b_duty1;
					}
				}
			}
			break;
	}
}
#endif


void RgbLed1MsInterrupt(void)
{
#ifdef CFG_MORE_GPIO_RGB_CTRL_EN
	ModeTest1();
#else
	RgbModeRun();
#endif
	LedDmaDataMode();
}

void RgbModeSwitch(void)
{
	DmaRgbFifo.rgb_mode++;
	if(DmaRgbFifo.rgb_mode > 3)
		DmaRgbFifo.rgb_mode = 0;
	DBG("RGB Mode %d!\n",DmaRgbFifo.rgb_mode);
}

void SetRgbLedMode(uint8_t rgb_mode)
{
	DmaRgbFifo.rgb_mode = rgb_mode;
}

uint8_t GetRgbLedMode(void)
{
	return DmaRgbFifo.rgb_mode;
}

#endif
