/**
 ****************************************************************************************
 *
 * @file app_le_audio_capabilities_client.c
 *
 * @brief Applet - Audio Capabilities Client
 *
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "le_audio_config.h"
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
#include "debug.h"
#include "bb_api.h"
#include "app_le_audio_capabilities_client.h"
#include "bap_capa_cli.h"
#include "bap_uc_cfg.h"
#include "utils_audio.h"
#include "le_cis_src.h"
#include "bap_capa_cli.h"

/*
 * ENUMERATIONS
 ****************************************************************************************
 */
#define COMPONENT_EVENT_SERVICE_CONTENT_DISCOVERED   (8)

// Event values for Service Discovery state machine
enum app_le_audio_capabilities_client_event_service_discovery
{
    // State machine start has been required
    EVENT_SERVICE_DISCOVERY_START = 0U,
    // Content of Published Audio Control Service has been discovered
    EVENT_SERVICE_CONTENT_DISCOVERED,
};

/*
 * TYPES DEFINITION
 ****************************************************************************************
 */

// Environment structure
typedef struct app_le_audio_capabilities_client_env
{
    // Callback function for backward communication with Mediator
    component_cb_event cb_mediator_event;
    // Number of Sink PAC characteristic
    uint8_t nb_pacs_sink;
    // Number of Source PAC characteristic
    uint8_t nb_pacs_src;
} app_le_audio_capabilities_client_env_t;

/*
 * LOCAL VARIABLES
 ****************************************************************************************
 */

// Pointer to environment
__STATIC app_le_audio_capabilities_client_env_t* p_env;

/*
 * STATE MACHINES
 ****************************************************************************************
 */

void app_le_audio_capabilities_client_transition_service_discovery(uint8_t event, uint8_t conidx,
                                                                            uint16_t status, const prf_svc_t* p_svc)
{
    APP_DBG("app_le_audio_capabilities_client_transition_service_discovery: evt 0x%x\n", event);
    switch (event)
    {
        case EVENT_SERVICE_DISCOVERY_START:
        {
            APP_DBG("Start PACS discovery (conidx = %d)\n", conidx);

            if (p_svc != NULL)
            {
                status = bap_capa_cli_discover(conidx, p_svc->shdl, p_svc->ehdl);
                if (status)
                {
                    APP_DBG("bap_capa_cli_discover: err 0x%x\n", status);
                }
            }
            else
            {
                status = bap_capa_cli_discover(conidx, 0, GATT_INVALID_HDL);
                if (status)
                {
                    APP_DBG("bap_capa_cli_discover2: err 0x%x\n", status);
                }
            }
        } break;

        case EVENT_SERVICE_CONTENT_DISCOVERED:
        {
            if (status == GAF_ERR_NO_ERROR)
            {
                APP_DBG("PACS discovered (conidx = %d)\n", conidx);
            }
            else
            {
                APP_DBG("Issue during PACS discovery: 0x%04X (conidx = %d)\n", status, conidx);
                // break;
            }

            if (p_env->cb_mediator_event != NULL)
            {
                p_env->cb_mediator_event(COMPONENT_EVENT_SERVICE_CONTENT_DISCOVERED, status, &conidx);
            }
        } break;

        default: { } break;
    }
}

/*
 * CALLBACK FUNCTIONS
 ****************************************************************************************
 */

__STATIC void app_le_audio_capabilities_client_cb_bond_data_prov(uint16_t component_id, uint16_t type,
                                                                 uint8_t conidx, bool found, uint16_t length,
                                                                 const void* p_data)
{
    uint16_t status;

    if (found)
    {
        bap_capa_cli_pacs_t* p_pacs_info = (bap_capa_cli_pacs_t*)p_data;

        status = bap_capa_cli_restore_bond_data(conidx, p_pacs_info);
        if (status)
        {
            APP_DBG("bap_capa_cli_restore_bond_data: err 0x%x\n", status);
        }
        p_env->nb_pacs_sink = p_pacs_info->nb_pacs_sink;
        p_env->nb_pacs_src = p_pacs_info->nb_pacs_src;
    }
}

__STATIC void app_le_audio_capabilities_client_cb_cmp_evt(uint8_t cmd_type, uint16_t status, uint8_t conidx,
                                                          uint8_t param_1, uint8_t pac_lid)
{
    APP_DBG("app_le_audio_capabilities_client_cb_cmp_evt: cmd_type 0x%x, status 0x%x\n", cmd_type, status);
    ASSERT_WARN(status == GAF_ERR_NO_ERROR, status, cmd_type);

    switch (cmd_type)
    {
        case BAP_CAPA_CLI_CMD_TYPE_DISCOVER:
        {
            app_le_audio_capabilities_client_transition_service_discovery(EVENT_SERVICE_CONTENT_DISCOVERED, conidx,
                                                                          status, NULL);
        } break;

        default: { } break;
    }
}

__STATIC void app_le_audio_capabilities_client_cb_bond_data(uint8_t conidx, bap_capa_cli_pacs_t* p_pacs_info)
{
    APP_DBG("Number of PAC Sink: %d / Number of PAC Source: %d (conidx = %d)\n", p_pacs_info->nb_pacs_sink,
                 p_pacs_info->nb_pacs_src, conidx);

    p_env->nb_pacs_sink = p_pacs_info->nb_pacs_sink;
    p_env->nb_pacs_src = p_pacs_info->nb_pacs_src;
}

__STATIC void app_le_audio_capabilities_client_cb_record(uint8_t conidx, uint8_t pac_lid, uint8_t record_lid,
                                                         uint8_t nb_records, const gaf_codec_id_t* p_codec_id,
                                                         const bap_capa_ptr_t* p_capa,
                                                         const bap_capa_metadata_ptr_t* p_metadata)
{
    uint32_t* p_bap_uc_cfg_bf; 
    if (nb_records != 0)
    {
        APP_DBG("PAC %d: Record %d/%d (conidx = %d)\n", pac_lid, record_lid + 1, nb_records, conidx);

        if (p_codec_id->codec_id[GAF_CODEC_ID_FORMAT_POS] == GAPI_CODEC_FORMAT_LC3)
        {
            utils_audio_le_cfg_t* p_le_audio_cfg = utils_audio_le_get_cfg(conidx);
            p_bap_uc_cfg_bf = (pac_lid < p_env->nb_pacs_sink)
                                    ? &(p_le_audio_cfg->bap_uc_cfg_bf_sink) : &(p_le_audio_cfg->bap_uc_cfg_bf_src);

            if (CHECKB(p_capa->param.sampling_freq_bf, BAP_SAMPLING_FREQ_8000HZ))
            {
                APP_DBG("    8kHz supported\n");

                if (CHECKB(p_capa->param.frame_dur_bf, BAP_FRAME_DUR_7_5MS))
                {
                    *p_bap_uc_cfg_bf |= (CO_BIT(BAP_UC_CFG_8_1_1_IDX) | CO_BIT(BAP_UC_CFG_8_1_2_IDX));
                }
                if (CHECKB(p_capa->param.frame_dur_bf, BAP_FRAME_DUR_10MS))
                {
                    *p_bap_uc_cfg_bf |= (CO_BIT(BAP_UC_CFG_8_2_1_IDX) | CO_BIT(BAP_UC_CFG_8_2_2_IDX));
                }
            }

            if (CHECKB(p_capa->param.sampling_freq_bf, BAP_SAMPLING_FREQ_16000HZ))
            {
                APP_DBG("    16kHz supported\n");

                if (CHECKB(p_capa->param.frame_dur_bf, BAP_FRAME_DUR_7_5MS))
                {
                    *p_bap_uc_cfg_bf |= (CO_BIT(BAP_UC_CFG_16_1_1_IDX) | CO_BIT(BAP_UC_CFG_16_1_2_IDX));
                }
                if (CHECKB(p_capa->param.frame_dur_bf, BAP_FRAME_DUR_10MS))
                {
                    *p_bap_uc_cfg_bf |= (CO_BIT(BAP_UC_CFG_16_2_1_IDX) | CO_BIT(BAP_UC_CFG_16_2_2_IDX));
                }
            }

            if (CHECKB(p_capa->param.sampling_freq_bf, BAP_SAMPLING_FREQ_24000HZ))
            {
                APP_DBG("    24kHz supported\n");

                if (CHECKB(p_capa->param.frame_dur_bf, BAP_FRAME_DUR_7_5MS))
                {
                    *p_bap_uc_cfg_bf |= (CO_BIT(BAP_UC_CFG_24_1_1_IDX) | CO_BIT(BAP_UC_CFG_24_1_2_IDX));
                }
                if (CHECKB(p_capa->param.frame_dur_bf, BAP_FRAME_DUR_10MS))
                {
                    *p_bap_uc_cfg_bf |= (CO_BIT(BAP_UC_CFG_24_2_1_IDX) | CO_BIT(BAP_UC_CFG_24_2_2_IDX));
                }
            }

            if (CHECKB(p_capa->param.sampling_freq_bf, BAP_SAMPLING_FREQ_32000HZ))
            {
                APP_DBG("    32kHz supported\n");

                if (CHECKB(p_capa->param.frame_dur_bf, BAP_FRAME_DUR_7_5MS))
                {
                    *p_bap_uc_cfg_bf |= (CO_BIT(BAP_UC_CFG_32_1_1_IDX) | CO_BIT(BAP_UC_CFG_32_1_2_IDX));
                }
                if (CHECKB(p_capa->param.frame_dur_bf, BAP_FRAME_DUR_10MS))
                {
                    *p_bap_uc_cfg_bf |= (CO_BIT(BAP_UC_CFG_32_2_1_IDX) | CO_BIT(BAP_UC_CFG_32_2_2_IDX));
                }
            }

            if (CHECKB(p_capa->param.sampling_freq_bf, BAP_SAMPLING_FREQ_44100HZ))
            {
                APP_DBG("    44.1kHz supported\n");

                if (CHECKB(p_capa->param.frame_dur_bf, BAP_FRAME_DUR_7_5MS))
                {
                    *p_bap_uc_cfg_bf |= (CO_BIT(BAP_UC_CFG_441_1_1_IDX) | CO_BIT(BAP_UC_CFG_441_1_2_IDX));
                }
                if (CHECKB(p_capa->param.frame_dur_bf, BAP_FRAME_DUR_10MS))
                {
                    *p_bap_uc_cfg_bf |= (CO_BIT(BAP_UC_CFG_441_2_1_IDX) | CO_BIT(BAP_UC_CFG_441_2_2_IDX));
                }
            }

            if (CHECKB(p_capa->param.sampling_freq_bf, BAP_SAMPLING_FREQ_48000HZ))
            {
                APP_DBG("    48kHz supported\n");

                if (CHECKB(p_capa->param.frame_dur_bf, BAP_FRAME_DUR_7_5MS))
                {
                    *p_bap_uc_cfg_bf |= (CO_BIT(BAP_UC_CFG_48_1_1_IDX) | CO_BIT(BAP_UC_CFG_48_3_1_IDX)
                                            | CO_BIT(BAP_UC_CFG_48_5_1_IDX) | CO_BIT(BAP_UC_CFG_48_1_2_IDX)
                                            | CO_BIT(BAP_UC_CFG_48_3_2_IDX) | CO_BIT(BAP_UC_CFG_48_5_2_IDX));
                }
                if (CHECKB(p_capa->param.frame_dur_bf, BAP_FRAME_DUR_10MS))
                {
                    *p_bap_uc_cfg_bf |= (CO_BIT(BAP_UC_CFG_48_2_1_IDX) | CO_BIT(BAP_UC_CFG_48_4_1_IDX)
                                            | CO_BIT(BAP_UC_CFG_48_6_1_IDX) | CO_BIT(BAP_UC_CFG_48_2_2_IDX)
                                            | CO_BIT(BAP_UC_CFG_48_4_2_IDX) | CO_BIT(BAP_UC_CFG_48_6_2_IDX));
                }
            }

            APP_DBG("Supported LC3 configurations bit field %d: (%d): 0x%04lX\n",
                         pac_lid, p_env->nb_pacs_sink, *p_bap_uc_cfg_bf);
        }
    }
    else
    {
        APP_DBG( "PAC - Empty\n");
    }

    // if (p_env->cb_mediator_event)
    // {
    //     app_le_audio_capabilities_record_t pac_record;
    //     pac_record.bap_uc_cfg_bf = *p_bap_uc_cfg_bf;
    //     pac_record.codec_id = *p_codec_id;
    //     pac_record.nb_records = nb_records;
    //     pac_record.record_lid = record_lid;
    //     // p_env->cb_mediator_event(EVENT_PAC_RECORD, GAF_ERR_NO_ERROR, &pac_record);
    // }
}

__STATIC void app_le_audio_capabilities_client_cb_location(uint8_t conidx, uint8_t direction,
                                                                        uint32_t location_bf)
{
    APP_DBG("direction: %d location: 0x%04lX (conidx = %d)\n", direction, location_bf, conidx);

    set_audio_loc_with_conidx((uint8_t)location_bf, conidx);
    // if (p_env->cb_mediator_event)
    // {
    //     app_le_audio_capabilities_record_t pac_record;
    //     pac_record.direction = direction;
    //     pac_record.location_bf = location_bf;
    //     // p_env->cb_mediator_event(EVENT_PAC_LOC, GAF_ERR_NO_ERROR, &pac_record);
    // }
}

__STATIC void app_le_audio_capabilities_client_cb_context(uint8_t conidx, uint8_t context_type,
                                                                       uint16_t context_bf_sink,
                                                                       uint16_t context_bf_src)
{
    if (p_env->nb_pacs_sink != 0)
    {
        APP_DBG("context_type 0x%x contexts (Sink): 0x%02X (conidx = %d)", 
                        context_type, context_bf_sink, conidx);

        // if (p_env->cb_mediator_event)
        // {
        //     app_le_audio_capabilities_record_t pac_record;
        //     pac_record.sink_context_type = context_type;
        //     pac_record.context_bf_sink = context_bf_sink;
        //     // p_env->cb_mediator_event(EVENT_PAC_SINK_CONTEXT, GAF_ERR_NO_ERROR, &pac_record);
        // }
    }
    if (p_env->nb_pacs_src != 0)
    {
        APP_DBG("context_type 0x%x contexts (Source): 0x%02X (conidx = %d)", 
                    context_type, context_bf_src,
                     conidx);

        // if (p_env->cb_mediator_event)
        // {
        //     // app_le_audio_capabilities_record_t pac_record;
        //     // pac_record.src_context_type = context_type;
        //     // pac_record.context_bf_src = context_bf_src;
        //     // p_env->cb_mediator_event(EVENT_PAC_SRC_CONTEXT, GAF_ERR_NO_ERROR, &pac_record);
        // }
    }
}

__STATIC void app_le_audio_capabilities_client_cb_svc_changed(uint8_t conidx)
{
}

// Set of callback functions for communication fir BAP Capabilities Client module
__STATIC const bap_capa_cli_cb_t cbs_bap_capa_cli = {
    .cb_cmp_evt = app_le_audio_capabilities_client_cb_cmp_evt,
    .cb_bond_data = app_le_audio_capabilities_client_cb_bond_data,
    .cb_record = app_le_audio_capabilities_client_cb_record,
    .cb_location = app_le_audio_capabilities_client_cb_location,
    .cb_context = app_le_audio_capabilities_client_cb_context,
    .cb_svc_changed = app_le_audio_capabilities_client_cb_svc_changed,
};

/*
 * FUNCTIONS
 ****************************************************************************************
 */

 void app_le_audio_capabilities_client_configure(const void* p_params, component_cb_event cb_event)
{
    uint16_t status;
    bap_capa_cli_cfg_t cfg = {
        .pref_mtu = 64,
    };

    ASSERT_ERR(p_env == NULL);

    p_env = (app_le_audio_capabilities_client_env_t*)ke_malloc_user(sizeof(app_le_audio_capabilities_client_env_t),
                                                                    KE_MEM_PROFILE);
    ASSERT_ERR(p_env != NULL);
    memset(p_env, 0, sizeof(app_le_audio_capabilities_client_env_t));
    p_env->cb_mediator_event = cb_event;

    status = bap_capa_cli_configure(&cbs_bap_capa_cli, &cfg);
    if (status)
    {
        APP_DBG("bap_capa_cli_configure err 0x%x\n", status);
    }
}

/// \example{lineno} app_le_audio_capabilities_client.c
#endif