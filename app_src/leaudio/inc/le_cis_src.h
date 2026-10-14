/**
 ****************************************************************************************
 *
 * @file le_cis_src.h
 *
 * @brief Applet - Broadcast Assistant - Header file
 *
 *
 ****************************************************************************************
 */

#ifndef LE_CIS_SRC_H_
#define LE_CIS_SRC_H_
#include "gap.h"
#include "prf_types.h"
#include "bap_uc_cfg.h"
#include "le_audio_config.h"
#include "bap_uc_cli.h"


#ifdef LE_CIS_SRC



// Event values for Group state machine
enum media_player_event_group
{
    // Start Group state machine
    EVENT_GROUP_START = 0U,
    // Play has been required
    EVENT_GROUP_PLAY,
    // Stop has been required
    EVENT_GROUP_STOP,
    // Pause has been required
    EVENT_GROUP_PAUSE,
};

/// Group type values
enum le_cis_src_group_type
{
    /// Media
    APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_MEDIA = 0U,
    /// Call
    APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_CALL,
    /// Ringtone
    APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_RINGTONE,

    /// Media_2
    APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_MEDIA_2,

    APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_MAX,
};

/// Common control type values
enum component_control_type
{
    /// Start
    COMPONENT_CONTROL_START = 0U,
    /// Stop
    COMPONENT_CONTROL_STOP,

    /// Configure
    COMPONENT_CONTROL_CONFIGURE,

    APP_LE_AUDIO_UNICAST_CLIENT_CONTROL_PAUSE,

    APP_LE_AUDIO_UNICAST_CLIENT_CONTROL_ADD,
    /// First available control type value for child Components
    COMPONENT_CONTROL_FIRST,
};


/// Parameters structure for #APP_LE_AUDIO_UNICAST_CLIENT_CONTROL_CONFIGURE_FULL control type
typedef struct le_cis_src_configure_full
{
    /// Group type (see #le_cis_src_group_type enumeration)
    uint8_t type;
    /// Sink or source direction ? - Meaningful only for #APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_CALL type
    bool sink;
    /// Pointer to QoS configuration
    const bap_uc_cfg_t* p_qos_cfg;
    /// Data Path ID
    uint8_t dp_id;
    /// Codec ID
    gaf_codec_id_t codec_id;
} le_cis_src_configure_full_t;

/// Parameters structure for #APP_LE_AUDIO_UNICAST_CLIENT_CONTROL_PAUSE control type
typedef struct le_cis_src_pause
{
    /// Group type (see #le_cis_src_group_type enumeration)
    uint8_t type;
} le_cis_src_pause_t;

/// Parameters structure for #COMPONENT_CONTROL_START control type
typedef struct le_cis_src_start
{
    /// Group type (see #le_cis_src_group_type enumeration)
    uint8_t type;
} le_cis_src_start_t;

/// Parameters structure for #APP_LE_AUDIO_UNICAST_CLIENT_CONTROL_ADD control type
typedef struct le_cis_src_add
{
    /// Group type (see #le_cis_src_group_type enumeration)
    uint8_t type;
    /// Connection index left
    uint8_t conidx_left;
    /// Connection index right - Meaningful only if is_stereo = true
    uint8_t conidx_right;
    /// Is a stereo stream ?
    bool is_stereo;
} le_cis_src_add_t;

/// Parameters structure for #COMPONENT_CONTROL_CONFIGURE control type
typedef struct le_cis_src_configure
{
    /// Group type (see #le_cis_src_group_type enumeration)
    uint8_t type;
    /// Sink or source direction ? - Meaningful only for #APP_LE_AUDIO_UNICAST_CLIENT_GROUP_TYPE_CALL type
    bool sink;
    /// Configuration name
    const char* cfg_name;
    /// Data Path ID
    uint8_t dp_id;
    /// Data Path ID
    uint8_t dp_id_1;
} le_cis_src_configure_t;

/// Parameters structure for #APP_LE_AUDIO_UNICAST_CLIENT_EVENT_GROUP_STOPPED event type
typedef struct le_cis_src_group_stopped
{
    /// Group type (see #le_cis_src_group_type enumeration)
    uint8_t type;
} le_cis_src_group_stopped_t;

/// Parameters structure for #COMPONENT_CONTROL_STOP control type
typedef struct le_cis_src_stop
{
    /// Group type (see #le_cis_src_group_type enumeration)
    uint8_t type;
} le_cis_src_stop_t;
void le_cis_src_state_service_discovery(uint8_t event, uint8_t conidx, uint16_t status,
                                                                  const prf_svc_t* p_svc);

void cis_src_configuration_cb_gapm_cmp(uint32_t event, uint16_t status);
#endif
#endif // LE_CIS_SRC_H_
