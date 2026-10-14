/**
 ****************************************************************************************
 *
 * @file app_le_udsc.h
 *
 * @brief app le udsc - Header file
 *
 * Copyright (C) RivieraWaves 2009-2024
 * Release Identifier: c73db0ce
 *
 ****************************************************************************************
 */

#ifndef LE_UDSC_H_
#define LE_UDSC_H_
#include "gap.h"

#ifdef LE_UDSC

void app_le_udsc_start_scan(void);
void udsc_configuration_cb_gapm_cmp(uint32_t event, uint16_t status);
#endif
#endif // LE_UDSC_H_
