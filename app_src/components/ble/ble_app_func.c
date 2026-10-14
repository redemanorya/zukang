
#include "type.h"
#include <string.h>
#include "ble_api.h"
#include "ble_app_func.h"
#include "bt_manager.h"
#include "bt_config.h"
#include "le_audio_config.h"
#include "gatt.h"

#include "debug.h"
#if (BLE_SUPPORT)
extern BT_CONFIGURATION_PARAMS		*btStackConfigParams;
extern void user_set_ble_bd_addr(uint8_t* local_addr);

extern void ble_gs_app_init(void);

#define BLE_DFLT_DEVICE_NAME			(sys_parameter.ble_LocalDeviceName)
#define BLE_DFLT_DEVICE_NAME_LEN		(strlen(BLE_DFLT_DEVICE_NAME))

#endif

