#ifndef _ACM8625_H_
#define _ACM8625_H_


#if 1

#define CFG_META_SWITCH (255)
#define CFG_META_DELAY  (254)
#define CFG_META_BURST  (253)
#define ACM8625_VOLUME_MAX_LEVEL (100U)

typedef union _REG_TAB
{
    struct 
    {
		unsigned char Address;
		unsigned char Data;
    };
} REG_TAB;

#endif

//#include "ACM8625_data.h"




void ACM8625_Init(void);

bool ACM8625_PowerOn(void);
void ACM8625_PowerOn_linemode(void);
void ACM8625_EQChang(void);
void Change8625p_EQ(uint8_t eq);
void AUX_MODE_EQ(uint8_t eq,uint8_t IN0rOUT);
void ACM8625_Mute(uint8_t mute);
void ACM8625_PDN(uint8_t pdn);
void ACM8625_VolSet(uint8_t vol);
uint32_t ACM8625_VolumeCoefficientGet(uint8_t vol);

#endif

