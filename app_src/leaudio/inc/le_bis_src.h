#include "le_audio_config.h"
#include "gaf.h"
#include "le_public.h"
#include "bap_bc_src.h"

//#ifdef LE_BIS_SOURCE
/// Maximum length of stream configuration string (\0 character included)
#define LE_BIS_SOURCE_CFG_NAME_MAX_LENGTH     (8U)

/// Parameters structure for Mediator configuration
typedef struct le_bis_source_cfg
{
    /// Pointer to Broadcast name\n
    /// Shall not be NULL in order to activate Public Broadcast
    const char* p_broadcast_name;
    /// broadcast name len
    uint8_t bcast_name_len;
    /// Stream configuration name
    char cfg_name[LE_BIS_SOURCE_CFG_NAME_MAX_LENGTH];
    /// Broadcast code\n
    /// Meaningful only if #is_encrypted is true
    gaf_bcast_code_t broadcast_code;
    /// Broadcast ID
    bap_bcast_id_t broadcast_id;
    /// Is encrypted or not ?
    bool is_encrypted;
    /// Number of streams
    uint8_t nb_streams;
} le_bis_source_cfg_t;

void le_bis_source_set_bcast_code(const uint8_t* p_bcast_code);
void le_bis_source_datapath_init();
void bis_src_configuration_cb_gapm_cmp(uint32_t event, uint16_t status);
//#endif
