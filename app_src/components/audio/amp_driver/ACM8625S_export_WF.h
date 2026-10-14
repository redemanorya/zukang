#ifndef __ACM8625S_export_WF_H
#define __ACM8625S_export_WF_H

#include <string.h>
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
#include "i2c_interface.h"
#include "delay.h"
#include "debug.h"
#include "gpio.h"
#include "app_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ACM8625.h"

typedef unsigned char uint8;
typedef unsigned short uint16;



void ACM86xx_init_WF(void);
unsigned char ACM86xx_Write_REG_WF(uint16 data_length, const REG_TAB *reg_access_ptr);
unsigned char ACM86xx_Read_Reg_WF(uint8 RegAddr, uint8 *pData, uint8 len);
void ACM86xx_Mute_WF(void);
void ACM86xx_UnMute_WF(void);
void WF_volumeControl(uint8 vol);

#endif
