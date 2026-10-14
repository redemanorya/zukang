/**
 ****************************************************************************************
 *
 * @file applet_volume_renderer.h
 *
 * @brief Applet - Volume Renderer - Header file
 *
 * Copyright (C) RivieraWaves 2009-2024
 * Release Identifier: c73db0ce
 *
 ****************************************************************************************
 */
#include "le_audio_config.h"
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)

#ifndef APPLET_VOLUME_RENDERER_H_
#define APPLET_VOLUME_RENDERER_H_

/**
 ****************************************************************************************
 * @defgroup APPLET_VOLUME_RENDERER Volume Renderer
 * @ingroup APPLET
 * @brief Applet adding support of Volume Control Service as Renderer
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <stdint.h>
/// @addtogroup APPLET_VOLUME_RENDERER
/// @{

/*
 * TYPES DEFINITION
 ****************************************************************************************
 */

/// Parameters structure for Mediator configuration
typedef struct applet_volume_renderer_cfg
{
    /// Initial volume (between 0 and 100)
    uint8_t volume;
} applet_volume_renderer_cfg_t;

/*
 * INTERFACES
 ****************************************************************************************
 */


#endif // APPLET_VOLUME_RENDERER_H_
#endif
