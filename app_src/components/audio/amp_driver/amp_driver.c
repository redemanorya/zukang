#include "amp_driver.h"
#include "debug.h"
#include "i2c_interface.h"
#include "type.h"
#include "i2c_host.h"
#include "i2c.h"
#include "i2c_interface.h"
#include "app_message.h"


#define RL_AMP_DEBUG_ENABLE
#ifdef RL_AMP_DEBUG_ENABLE
#define RL_AMP_DEBUG( format ,...)      APP_DBG("\r\n [rl ui touch %d]"format"",__LINE__,##__VA_ARGS__);
#else
#define RL_AMP_DEBUG( format ,...)     
#endif

void* ampI2cHandler = NULL;
#define AMPSCLPortSel				PORT_B
#define AMPSclIndex					7
#define AMPSDAPortSel				PORT_B
#define AMPSdaIndex					6


void amp_driver_init(void)
{
	AMP_Init();
}