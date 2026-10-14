/**
 ****************************************************************************************
 *
 * @file applet_audio_capabilities_client.h
 *
 * @brief Applet - Audio Capabilities Client - Header file
 *
 *
 ****************************************************************************************
 */
#include "le_audio_config.h"
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
#ifndef APP_LE_AUDIO_CAPABILITIES_CLIENT_H_
#define APP_LE_AUDIO_CAPABILITIES_CLIENT_H_

/**
 ****************************************************************************************
 * @defgroup APP_LE_AUDIO_CAPABILITIES_CLIENT Audio Capabilities Client
 * @ingroup APP_LE
 * @brief Applet adding support of Published Audio Capabilities Service as Client
 *
 * Discovery of audio capabilities can be started using #COMPONENT_CONTROL_START_DISCOVERY Mediator control type.\n
 * Supported LC3 configuration for each direction are stored in Mediator Shared Environment and may be retrieved using
 * utils_audio_le_get_cfg function.\n
 * A #COMPONENT_EVENT_SERVICE_CONTENT_DISCOVERED Mediator event is triggered once discovery has been successfully
 * completed.
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include <stdint.h>
#include "arch.h"
#include "prf_types.h"
#include "gaf.h"

/// @addtogroup APP_LE_AUDIO_CAPABILITIES_CLIENT
/// @{
enum app_le_audio_capabilities_pac_event
{
    EVENT_PAC_RECORD,
    EVENT_PAC_LOC,
    EVENT_PAC_SINK_CONTEXT,
    EVENT_PAC_SRC_CONTEXT,
};

typedef struct app_le_audio_capabilities_record
{
    uint8_t record_lid;
    uint8_t nb_records;
    gaf_codec_id_t codec_id;
    uint32_t bap_uc_cfg_bf;
    uint8_t sink_context_type;
    uint8_t src_context_type;
    uint16_t context_bf_sink;
    uint16_t context_bf_src;
    uint8_t direction;
    uint32_t location_bf;
}app_le_audio_capabilities_record_t;

/*
 * INTERFACES
 ****************************************************************************************
 */
void app_le_audio_capabilities_client_transition_service_discovery(uint8_t event, uint8_t conidx,
                                                                            uint16_t status, const prf_svc_t* p_svc);

 void app_le_audio_capabilities_client_configure(const void* p_params, component_cb_event cb_event);

/// @} APP_LE_AUDIO_CAPABILITIES_CLIENT

#endif // APP_LE_AUDIO_CAPABILITIES_CLIENT_H_
#endif