/**
 * @file ble_gs_app.h
 * @author Cole (cole@mvsilicon.com)
 * @brief BLE Generic Service Application Header
 * @version 0.1
 * @date 2025-12-25
 * 
 * @copyright Copyright (c) 2025
 */

#ifndef _BLE_GS_APP_H_
#define _BLE_GS_APP_H_

#include "bt_config.h"
#if (BLE_SUPPORT)

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
enum le_phy_prop
{
    /// Scan connectable advertisements on the LE 1M PHY. Connection parameters for the LE 1M PHY are provided
    PROP_1M_BIT = (1 << 0),
    /// Connection parameters for the LE 2M PHY are provided
    PROP_2M_BIT = (1 << 1),
    /// Scan connectable advertisements on the LE Coded PHY. Connection parameters for the LE Coded PHY are provided
    PROP_CODED_BIT = (1 << 2),
};

void ble_gs_app_ble_init(void);
void ble_gs_app_disconnect(uint8_t conidx);

#ifdef __cplusplus
}
#endif

#endif /* BLE_SUPPORT */
#endif /* _BLE_GS_APP_H_ */



