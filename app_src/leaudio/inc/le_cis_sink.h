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

#ifndef LE_CIS_SINK_H_
#define LE_CIS_SINK_H_
#include "gap.h"
#include "le_audio_config.h"
/*
 * ENUMERATIONS
 ****************************************************************************************
 */


/*
 * TYPES DEFINITION
 ****************************************************************************************
 */
#if 1//def LE_CIS_SINK

#define CIS_SINK_NB_MEM  (2)

/// Parameters structure for Mediator configuration
typedef struct le_cis_sink_cfg
{
    /// Number of Sink ASEs
    uint8_t nb_ases_sink;
    /// Number of Source ASEs
    uint8_t nb_ases_src;
} le_cis_sink_cfg_t;


void le_cis_sink_init(uint8_t cis_app, uint8_t sink_loc);
void le_cis_sink_configuration_cb_gapm_cmp(uint32_t event, uint16_t status);
#endif
#endif // LE_CIS_SINK_H_
