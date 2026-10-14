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

#ifndef LE_UDSS_H_
#define LE_UDSS_H_
#include "gap.h"

#ifdef LE_UDSS

uint16_t app_udss_start_adv(void);
void app_udss_configuration_cb_gapm_cmp(uint32_t event, uint16_t status);
#endif
#endif // LE_UDSS_H_
