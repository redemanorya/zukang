/*
 * Copyright (c) 2017 Actions Semi Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: wanghui<wanghui@actions-semi.com>
 *
 * Change log:
 *	2018/1/6: Created by wanghui.
 */

#include <stdlib.h>
#include "string.h"
#include <nds32_intrinsic.h>
#include "uarts.h"
#include "uarts_interface.h"
#include "type.h"
#include "debug.h"
#include "timeout.h"
#include "clk.h"
#include "dma.h"
#include "timer.h"
#include "i2c.h"
#include "i2c_host.h"
#include "i2c_interface.h"
#include "delay.h"
#include "debug.h"
#include "gpio.h"
#include "app_config.h"
#include "ACM8625.h"
#include "AMP.h"

typedef struct
{
	uint8_t amp_type;
}AmpContext;

/*AMP I2C*/
static AmpContext s_AmpVar;

void* AMP_I2cHandler = NULL;

#define AMP_SclPortSel                  PORT_B
#define AMP_SclIndexSel                 7

#define AMP_SdaPortSel                  PORT_B
#define AMP_SdaIndexSel                 6

#define AMP_SCL_PIN         			GPIOB7
#define AMP_SDA_PIN         			GPIOB6

#define AMP_PDN_GPIO_PIN         		GPIOB3




void AMP_Init(void)
{
	s_AmpVar.amp_type = AMP_NONE;

    GPIO_PortAModeSet(AMP_SCL_PIN, 0x0);
    GPIO_PortAModeSet(AMP_SDA_PIN, 0x0);
    GPIO_PortAModeSet(AMP_PDN_GPIO_PIN, 0x0);
	APP_DBG("\r\n AMP Init");
	ACM8625_PDN(0);
	DelayMs(20);

   AMP_I2cHandler = I2cMasterCreate(AMP_SclPortSel, AMP_SclIndexSel, AMP_SdaPortSel, AMP_SdaIndexSel);

   if(!AMP_I2cHandler)
   {
	  DBG("amp: Create I2C FAIL!  \n");
	  return;
   }

   
   APP_DBG("\r\n AMP_PowerOn\n");
   AMP_PowerOn();
   

}

//void AMP_PowerOn(uint8_t battery_type)
void AMP_PowerOn(void)
{
	if(ACM8625_PowerOn())
	{
		s_AmpVar.amp_type =AMP_ACM8625;
		APP_DBG("\r\n AMP_PowerOn OK ! \n");
	}
	else
	{
		s_AmpVar.amp_type =AMP_NONE;
		DBG("AMP poweron Failed!!!  \n");
	}
	return ;
}

void AMP_PowerOff(void)
{
	#if 1
	AMP_Mute(1);
	DelayMs(50);	
	AMP_PDN(0);
	#endif
	DBG("AMP_PowerOff");
}

void AMP_PDN(uint8_t pdn)
{
	if(s_AmpVar.amp_type ==AMP_ACM8625)
	{
		ACM8625_PDN(pdn);
	}
}

void AMP_Mute(uint8_t mute)
{
	if(s_AmpVar.amp_type ==AMP_ACM8625)
	{
		//ACM8625_Mute(mute);
	}
}

void AMP_VolSet(uint8_t vol)
{
	if(s_AmpVar.amp_type ==AMP_ACM8625)
	{
		ACM8625_VolSet(vol);
	}
}

