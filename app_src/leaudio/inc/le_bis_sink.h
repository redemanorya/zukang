#include "le_audio_config.h"
#include "gaf.h"
#include "le_public.h"
#include "bap_bc_src.h"
#include "co_time.h"

// Length of packed codec configuration
#define UTILS_AUDIO_LE_CODEC_CFG_PACKED_LEN    (19U)

typedef struct le_bis_sink_env
{
    // Source advertising identifier
    bap_adv_id_t adv_id;
#ifdef DISCOV_BIS
    // discov advertising identifier
    bap_adv_id_t discov_adv_id;
    // Periodic advertising local index
    uint8_t discov_pa_lid;
    bool sink_enable;
#endif
    // Broadcast identifier
    bap_bcast_id_t broadcast_id;
    /// Codec configuration
    bap_cfg_param_t codec_cfg;
    // Periodic advertising local index
    uint8_t pa_lid;

    // Status bit field (see #applet_broadcast_sink_status_bf enumeration)
    uint8_t status_bf;
    // Synchronization bit field
    uint8_t sync_bf;
    // Group local index
    uint8_t grp_lid;
    /// Meaningful only if #is_encrypted is true
    const gaf_bcast_code_t broadcast_code;
    /// Is encrypted or not ?
    bool is_encrypted;
    /// IS codec_cfg vaild or not
    bool is_codec_cfg_vaild;

#ifdef SINK_CHOOSE_SRC
    co_timer_t scan_timer_user;
    uint8_t src_idx;
#endif
    bap_adv_id_t src_adv_id;
    bap_cfg_metadata_ptr_t bis_metadata;

    co_timer_t timer_user;
    co_timer_t biginfo_timer_user;
} le_bis_sink_env_t;

void set_phone_bcast_code(uint8_t* p_bcast_code);
void le_bis_sink_configuration_cb_gapm_cmp(uint32_t event, uint16_t status);
void le_bis_sink_start(void);

