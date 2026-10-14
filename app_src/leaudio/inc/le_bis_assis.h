/**
 ****************************************************************************************
 *
 * @file applet_broadcast_assistant.h
 *
 * @brief Applet - Broadcast Assistant - Header file
 *
 * Copyright (C) RivieraWaves 2009-2024
 * Release Identifier: c73db0ce
 *
 ****************************************************************************************
 */

#ifndef LE_BIS_ASSIS_H_
#define LE_BIS_ASSIS_H_
#include "gap.h"

typedef struct applet_solicatation_found
{
    /// Pointer to device address
    const gap_bdaddr_t addr;
    #if (BT_53)
    /// Use subrating ?
    bool use_subrating;
    #endif //(BT_53)   
} applet_solicatation_found_t;

void le_bis_assis_state_service_discovery(uint8_t event, uint16_t status, void* params);
void bis_assis_configuration_cb_gapm_cmp(uint32_t event, uint16_t status);
#endif // LE_BIS_ASSIS_H_
