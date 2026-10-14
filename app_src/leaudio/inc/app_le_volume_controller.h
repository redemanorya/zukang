/**
 ****************************************************************************************
 *
 * @file app_le_volume_controller.h
 *
 * @brief Applet - Volume Controller - Header file
 *
 *
 ****************************************************************************************
 */
#include "le_audio_config.h"
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
#ifndef APP_LE_VOLUME_CONTROLLER_H_
#define APP_LE_VOLUME_CONTROLLER_H_

/**
 ****************************************************************************************
 * @defgroup APP_LE_VOLUME_CONTROLLER Volume Controller
 * @ingroup APP_LE
 * @brief Applet adding support of Volume Control Service as Controller
 *
 * Discovery of Volume Control Service content shall be initiated by using #COMPONENT_CONTROL_START_DISCOVERY Mediator
 * control type.\n
 * Each time an included Volume Offset Control Service or Audio Input Control Service instance is discovered,
 * a #COMPONENT_EVENT_SERVICE_DISCOVERED Mediator event is triggered.\n
 * #COMPONENT_EVENT_SERVICE_CONTENT_DISCOVERED Mediator event is triggered when content of Volume Control Service has
 * been discovered.\n\n
 * Several Input Control are available: Increase/Decrease Volume / Mute / Unmute.
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>
#include "arch.h"
#include "prf_types.h"

/// @addtogroup APP_LE_VOLUME_CONTROLLER
/// @{

/*
 * INTERFACES
 ****************************************************************************************
 */
void app_vcc_volume_increase(void);
void app_vcc_volume_decrease(void);

void app_le_volume_controller_state_service_discovery(uint8_t event, uint8_t conidx, uint16_t status,
                                                               const prf_svc_t* p_svc);
void app_le_volume_controller_configure(const void* p_params, component_cb_event cb_event);
/// @} APP_LE_VOLUME_CONTROLLER

#endif // APP_LE_VOLUME_CONTROLLER_H_
#endif