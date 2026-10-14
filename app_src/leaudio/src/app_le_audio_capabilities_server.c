/**
 ****************************************************************************************
 *
 * @file app_le_audio_capabilities_server.c
 *
 * @brief Applet - Audio Capabilities Server
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
#include "bb_api.h"
#include "app_le_audio_capabilities_server.h"
#include "debug.h"
#include "gaf.h"
#include "bap_capa_srv.h"
#include "bap_uc_cfg.h"
#include "tmap.h"
#include "le_audio_config.h"
/*
 * DEFINES
 ****************************************************************************************
 */

// Maximum number of records per PAC characteristic - Shall be higher than 0
#define NB_RECORDS_PER_PAC_MAX         (2U)

/*
 * MACROS
 ****************************************************************************************
 */

// Compute number of PAC characteristics based on number of records
#define NB_PAC(nb_records) \
            ((nb_records + NB_RECORDS_PER_PAC_MAX - 1) / NB_RECORDS_PER_PAC_MAX)

/*
 * TYPES DEFINITION
 ****************************************************************************************
 */

// Bond Data structure
typedef struct app_le_audio_capabilities_server_bond_data
{
    // Client characteristic configuration bit field
    uint8_t cli_cfg_bf;
    // Client characteristic configuration bit field for Sink/Source PAC characteristics
    uint8_t pac_cli_cfg_bf;
} app_le_audio_capabilities_server_bond_data_t;

// Environment structure
typedef struct app_le_audio_capabilities_server_env
{
//    // Registration structure for events
//    utils_control_intf_t control_intf;
//    #if (APP_DEBUG)
//    // Pointer to Output Provider interface
//    const output_prov_intf_t* p_output_intf;
//    #endif //(APP_DEBUG)
//    // Pointer to Bond Data Provider interface
//    const bond_data_prov_intf_t* p_bond_data_intf;
    // Codec Capabilities structure (Sink direction)
    bap_capa_t* p_capa_sink;
    // Codec Capabilities structure (Source direction)
    bap_capa_t* p_capa_src;
    // Number of records for Sink direction
    uint8_t nb_records_sink;
    // Number of records for Source direction
    uint8_t nb_records_src;
    // Number of Sink PAC characteristics
    uint8_t nb_pacs_sink;
    // Number of Source PAC characteristics
    uint8_t nb_pacs_src;
} app_le_audio_capabilities_server_env_t;

/*
 * LOCAL VARIABLES
 ****************************************************************************************
 */

// Pointer to environment
__STATIC app_le_audio_capabilities_server_env_t* p_env;
// extern bool utils_is_paired(uint8_t conidx);
/*
 * CALLBACK FUNCTIONS
 ****************************************************************************************
 */

__STATIC void app_le_audio_capabilities_server_cb_bond_data(uint8_t conidx, uint8_t cli_cfg_bf,
                                                            uint16_t pac_cli_cfg_bf)
{
    APP_DBG("PACS Bond Data updated (conidx = %d, cli_cfg_bf = 0x%02X, pac_cli_cfg_bf = 0x%02X)\n", conidx,
                 cli_cfg_bf, pac_cli_cfg_bf);

    // if (utils_is_paired(conidx))
    // {
    //     app_le_audio_capabilities_server_bond_data_t bond_data = {
    //         .cli_cfg_bf = cli_cfg_bf,
    //         .pac_cli_cfg_bf = pac_cli_cfg_bf,
    //     };
    // }
}

__STATIC void app_le_audio_capabilities_server_cb_location(uint8_t conidx, uint8_t direction, uint32_t location_bf)
{
    APP_DBG("app_le_audio_capabilities_server_cb_location, direction 0x%x, location_bf 0x%lx\n",
                direction, location_bf);
}

// Set of callback functions for communication with BAP Capabilities Server module
__STATIC const bap_capa_srv_cb_t cbs_bap_capa_srv = {
    .cb_bond_data = app_le_audio_capabilities_server_cb_bond_data,
    .cb_location = app_le_audio_capabilities_server_cb_location,
};

/*
 * LOCAL FUNCTIONS
 ****************************************************************************************
 */

 __STATIC void app_le_audio_capabilities_server_prepare_records_sink(const app_le_audio_capabilities_server_cfg_t* p_cfg)
{
if (p_cfg->location_bf_sink != 0)
    {
        // Following codec capabilities settings are mandatory for Sink direction
        //     -> 16_2 : LC3 / 16kHz / 10ms / 40 bytes
        //     -> 24_2 : LC3 / 24kHz / 10ms / 60 bytes
        // Following codec capabilities are mandatory when Unicast Media Receiver role is supported
        //     -> 48_1 : LC3 / 48kHz / 7.5ms / 75 bytes
        //     -> 48_2 : LC3 / 48kHz / 10ms / 100 bytes
        //     -> 48_3 : LC3 / 48kHz / 7.5ms / 90 bytes
        //     -> 48_4 : LC3 / 48kHz / 10ms / 120 bytes
        //     -> 48_5 : LC3 / 48kHz / 7.5ms / 117 bytes
        //     -> 48_6 : LC3 / 48kHz / 10ms / 155 bytes
        // Following codec capabilities are mandatory when Call Terminal role is supported
        //     -> 32_1 : LC3 / 24kHz / 7.5ms / 60 bytes
        //     -> 32_2 : LC3 / 24kHz / 10ms / 80 bytes
        // Following codec capability is mandatory when Call Gateway role is supported
        //     -> 32_2 : LC3 / 24kHz / 10ms / 80 bytes
        bap_capa_t* p_capa;
        utils_audio_le_cfg_t* p_le_audio_cfg = utils_audio_le_get_cfg_local();

        // At least 2 record are used, one for 16kHz, one for 24kHz
        p_env->nb_records_sink = 2;
        p_le_audio_cfg->bap_uc_cfg_bf_sink = CO_BIT(BAP_UC_CFG_16_2_1_IDX) | CO_BIT(BAP_UC_CFG_16_2_2_IDX)
                                                | CO_BIT(BAP_UC_CFG_24_2_1_IDX) | CO_BIT(BAP_UC_CFG_24_2_2_IDX);

        if (CHECKB(p_le_audio_cfg->tmap_role_bf, TMAP_ROLE_UMR))
        {
            p_env->nb_records_sink += 1; // One for all supported 48kHz capabilities
            p_le_audio_cfg->bap_uc_cfg_bf_sink |= (CO_BIT(BAP_UC_CFG_48_1_1_IDX) | CO_BIT(BAP_UC_CFG_48_2_1_IDX)
                                                    | CO_BIT(BAP_UC_CFG_48_1_1_IDX) | CO_BIT(BAP_UC_CFG_48_2_1_IDX)
                                                    | CO_BIT(BAP_UC_CFG_48_3_1_IDX) | CO_BIT(BAP_UC_CFG_48_4_1_IDX)
                                                    | CO_BIT(BAP_UC_CFG_48_5_1_IDX) | CO_BIT(BAP_UC_CFG_48_6_1_IDX)
                                                    | CO_BIT(BAP_UC_CFG_48_1_2_IDX) | CO_BIT(BAP_UC_CFG_48_2_2_IDX)
                                                    | CO_BIT(BAP_UC_CFG_48_3_2_IDX) | CO_BIT(BAP_UC_CFG_48_4_2_IDX)
                                                    | CO_BIT(BAP_UC_CFG_48_5_2_IDX) | CO_BIT(BAP_UC_CFG_48_6_2_IDX));
        }

        if ((CHECKB(p_le_audio_cfg->tmap_role_bf, TMAP_ROLE_BMR)) && (!CHECKB(p_le_audio_cfg->tmap_role_bf, TMAP_ROLE_UMR))) //MV define
        {
            p_env->nb_records_sink += 1; // 48_2, 48-4
            p_le_audio_cfg->bap_uc_cfg_bf_sink |=  (CO_BIT(BAP_UC_CFG_48_2_1_IDX) | CO_BIT(BAP_UC_CFG_48_2_2_IDX)
                                                    | CO_BIT(BAP_UC_CFG_48_4_1_IDX) | CO_BIT(BAP_UC_CFG_48_4_2_IDX));
        }

        if (CHECKB(p_le_audio_cfg->tmap_role_bf, TMAP_ROLE_CT)
                || CHECKB(p_le_audio_cfg->tmap_role_bf, TMAP_ROLE_CG))
        {
            p_env->nb_records_sink += 1; // One for all supported 32kHz capabilities
            p_le_audio_cfg->bap_uc_cfg_bf_sink |= (CO_BIT(BAP_UC_CFG_32_2_1_IDX) | CO_BIT(BAP_UC_CFG_32_2_2_IDX));

            if (CHECKB(p_le_audio_cfg->tmap_role_bf, TMAP_ROLE_CT))
            {
                p_le_audio_cfg->bap_uc_cfg_bf_sink |= (CO_BIT(BAP_UC_CFG_32_1_1_IDX) | CO_BIT(BAP_UC_CFG_32_1_2_IDX));
            }
        }

        p_capa = (bap_capa_t*)ke_malloc_user(CO_ALIGN4_HI(sizeof(bap_capa_t)) * p_env->nb_records_sink,
                                             KE_MEM_PROFILE);
        ASSERT_ERR(p_capa != NULL);
        p_env->p_capa_sink = p_capa;

        p_capa->param.sampling_freq_bf = BAP_SAMPLING_FREQ_16000HZ_BIT;
        p_capa->param.frame_dur_bf = BAP_FRAME_DUR_10MS_BIT;
        p_capa->param.chan_cnt_bf = p_cfg->channel_cnt_bf;
        p_capa->param.frame_octet_min = 40;
        p_capa->param.frame_octet_max = 40;
        p_capa->param.max_frames_sdu = p_cfg->nb_frames_per_sdu_max;
        p_capa->add_capa.len = 0;

        p_capa = (bap_capa_t*)((uint32_t)p_capa + CO_ALIGN4_HI(sizeof(bap_capa_t)));

        p_capa->param.sampling_freq_bf = BAP_SAMPLING_FREQ_24000HZ_BIT;
        p_capa->param.frame_dur_bf = BAP_FRAME_DUR_10MS_BIT;
        p_capa->param.chan_cnt_bf = p_cfg->channel_cnt_bf;
        p_capa->param.frame_octet_min = 60;
        p_capa->param.frame_octet_max = 60;
        p_capa->param.max_frames_sdu = p_cfg->nb_frames_per_sdu_max;
        p_capa->add_capa.len = 0;

        p_capa = (bap_capa_t*)((uint32_t)p_capa + CO_ALIGN4_HI(sizeof(bap_capa_t)));

        if (CHECKB(p_le_audio_cfg->tmap_role_bf, TMAP_ROLE_UMR))
        {
            p_capa->param.sampling_freq_bf = BAP_SAMPLING_FREQ_48000HZ_BIT;
            p_capa->param.frame_dur_bf = BAP_FRAME_DUR_7_5MS_BIT | BAP_FRAME_DUR_10MS_BIT;
            p_capa->param.chan_cnt_bf = p_cfg->channel_cnt_bf;
            p_capa->param.frame_octet_min = 75;
            p_capa->param.frame_octet_max = 155;
            p_capa->param.max_frames_sdu = p_cfg->nb_frames_per_sdu_max;
            p_capa->add_capa.len = 0;

            p_capa = (bap_capa_t*)((uint32_t)p_capa + CO_ALIGN4_HI(sizeof(bap_capa_t)));
        }

        if (CHECKB(p_le_audio_cfg->tmap_role_bf, TMAP_ROLE_CT))
        {
            p_capa->param.sampling_freq_bf = BAP_SAMPLING_FREQ_32000HZ_BIT;
            p_capa->param.frame_dur_bf = BAP_FRAME_DUR_7_5MS_BIT | BAP_FRAME_DUR_10MS_BIT;
            p_capa->param.chan_cnt_bf = p_cfg->channel_cnt_bf;
            p_capa->param.frame_octet_min = 60;
            p_capa->param.frame_octet_max = 80;
            p_capa->param.max_frames_sdu = p_cfg->nb_frames_per_sdu_max;
            p_capa->add_capa.len = 0;

            p_capa = (bap_capa_t*)((uint32_t)p_capa + CO_ALIGN4_HI(sizeof(bap_capa_t)));
        }

        p_env->nb_pacs_sink = NB_PAC(p_env->nb_records_sink);
    }
    APP_DBG("nb_pacs_sink 0x%x,  chan_cnt_bf 0x%X\n", p_env->nb_pacs_sink, p_cfg->channel_cnt_bf);
}

__STATIC void app_le_audio_capabilities_server_prepare_records_src(const app_le_audio_capabilities_server_cfg_t* p_cfg)
{
    if (p_cfg->location_bf_src != 0)
    {
        // Following codec capabilities settings are mandatory for Source direction
        //     -> 16_2 : LC3 / 16kHz / 10ms / 40 bytes
        // Following codec capabilities are mandatory when Unicast Media Sender role is supported
        //     -> 48_2 : LC3 / 48kHz / 10ms / 100 bytes
        //     At least one of:
        //         -> 48_4 : LC3 / 48kHz / 10ms / 120 bytes
        //         -> 48_6 : LC3 / 48kHz / 10ms / 155 bytes
        // Following codec capabilities are mandatory when Call Terminal role is supported
        //     -> 32_2 : LC3 / 32kHz / 10ms / 80 bytes
        // Following codec capability is mandatory when Call Gateway role is supported
        //     -> 32_2 : LC3 / 32kHz / 10ms / 80 bytes
        bap_capa_t* p_capa;
        utils_audio_le_cfg_t* p_le_audio_cfg = utils_audio_le_get_cfg_local();

        // At least 1 record is used, for 16kHz
        p_env->nb_records_src = 1;
        p_le_audio_cfg->bap_uc_cfg_bf_src = CO_BIT(BAP_UC_CFG_16_2_1_IDX) | CO_BIT(BAP_UC_CFG_16_2_2_IDX);

        if (CHECKB(p_le_audio_cfg->tmap_role_bf, TMAP_ROLE_UMS))
        {
            p_env->nb_records_src += 1; // One for all supported 48kHz capabilities
            p_le_audio_cfg->bap_uc_cfg_bf_src |= (CO_BIT(BAP_UC_CFG_48_2_1_IDX) | CO_BIT(BAP_UC_CFG_48_2_2_IDX));
            p_le_audio_cfg->bap_uc_cfg_bf_src |= (CO_BIT(BAP_UC_CFG_48_6_1_IDX) | CO_BIT(BAP_UC_CFG_48_6_2_IDX));
        }

        if (CHECKB(p_le_audio_cfg->tmap_role_bf, TMAP_ROLE_CT)
                || CHECKB(p_le_audio_cfg->tmap_role_bf, TMAP_ROLE_CG))
        {
            p_env->nb_records_src += 1; // One for all supported 32kHz capabilities
            p_le_audio_cfg->bap_uc_cfg_bf_src |= (CO_BIT(BAP_UC_CFG_32_2_1_IDX) | CO_BIT(BAP_UC_CFG_32_2_2_IDX));
        }

        p_capa = (bap_capa_t*)ke_malloc_user(CO_ALIGN4_HI(sizeof(bap_capa_t)) * p_env->nb_records_src,
                                             KE_MEM_PROFILE);
        ASSERT_ERR(p_capa != NULL);
        p_env->p_capa_src = p_capa;

        p_capa->param.sampling_freq_bf = BAP_SAMPLING_FREQ_16000HZ_BIT;
        p_capa->param.frame_dur_bf = BAP_FRAME_DUR_10MS_BIT;
        p_capa->param.chan_cnt_bf = 0x01;
        p_capa->param.frame_octet_min = 40;
        p_capa->param.frame_octet_max = 40;
        p_capa->param.max_frames_sdu = 1;
        p_capa->add_capa.len = 0;

        p_capa = (bap_capa_t*)((uint32_t)p_capa + CO_ALIGN4_HI(sizeof(bap_capa_t)));

        if (CHECKB(p_le_audio_cfg->tmap_role_bf, TMAP_ROLE_CT))
        {
            p_capa->param.sampling_freq_bf = BAP_SAMPLING_FREQ_32000HZ_BIT;
            p_capa->param.frame_dur_bf = BAP_FRAME_DUR_7_5MS_BIT | BAP_FRAME_DUR_10MS_BIT;
            p_capa->param.chan_cnt_bf = 0x01;
            p_capa->param.frame_octet_min = 60;
            p_capa->param.frame_octet_max = 80;
            p_capa->param.max_frames_sdu = 1;
            p_capa->add_capa.len = 0;

            p_capa = (bap_capa_t*)((uint32_t)p_capa + CO_ALIGN4_HI(sizeof(bap_capa_t)));
        }

        p_capa = (bap_capa_t*)((uint32_t)p_capa + CO_ALIGN4_HI(sizeof(bap_capa_t)));

        if (CHECKB(p_le_audio_cfg->tmap_role_bf, TMAP_ROLE_UMS))
        {
            p_capa->param.sampling_freq_bf = BAP_SAMPLING_FREQ_48000HZ_BIT;
            p_capa->param.frame_dur_bf = BAP_FRAME_DUR_10MS_BIT;
            p_capa->param.chan_cnt_bf = 0x1;
            p_capa->param.frame_octet_min = 120;
            p_capa->param.frame_octet_max = 155;
            p_capa->param.max_frames_sdu = 1;
            p_capa->add_capa.len = 0;

            p_capa = (bap_capa_t*)((uint32_t)p_capa + CO_ALIGN4_HI(sizeof(bap_capa_t)));
        }

        p_env->nb_pacs_src = NB_PAC(p_env->nb_records_src);
    }
}

__STATIC void app_le_audio_capabilities_server_configure_bap_capa_srv(const app_le_audio_capabilities_server_cfg_t* p_cfg)
{
    bap_capa_srv_cfg_t capa_srv_cfg;

    capa_srv_cfg.nb_pacs_sink = p_env->nb_pacs_sink;
    capa_srv_cfg.nb_pacs_src = p_env->nb_pacs_src;
    capa_srv_cfg.cfg_bf = BAP_CAPA_SRV_CFG_PAC_NTF_BIT | BAP_CAPA_SRV_CFG_LOC_NTF_BIT | BAP_CAPA_SRV_CFG_LOC_SUPP_BIT
                                | BAP_CAPA_SRV_CFG_SUPP_CONTEXT_NTF_BIT | BAP_CAPA_SRV_CFG_LOC_WR_BIT;
    capa_srv_cfg.pref_mtu = 0;
    capa_srv_cfg.shdl = 0;
    capa_srv_cfg.location_bf_sink = p_cfg->location_bf_sink;
    capa_srv_cfg.location_bf_src = p_cfg->location_bf_src;

    if (capa_srv_cfg.nb_pacs_sink != 0)
    {
        capa_srv_cfg.supp_context_bf_sink = BAP_CONTEXT_TYPE_UNSPECIFIED_BIT | p_cfg->supp_context_bf_sink;
    }
    else
    {
        capa_srv_cfg.supp_context_bf_sink = 0;
    }

    if (capa_srv_cfg.nb_pacs_src != 0)
    {
        capa_srv_cfg.supp_context_bf_src = BAP_CONTEXT_TYPE_UNSPECIFIED_BIT | p_cfg->supp_context_bf_src;
    }
    else
    {
        capa_srv_cfg.supp_context_bf_src = 0;
    }

    uint8_t status = bap_capa_srv_configure(&cbs_bap_capa_srv, &capa_srv_cfg);
    if (status)
    {
        APP_DBG("bap_capa_srv_configure: status 0x%x, sink 0x%X, src 0x%x, loc 0x%lx, 0x%lX\n",
                status, capa_srv_cfg.supp_context_bf_sink, capa_srv_cfg.supp_context_bf_src,
                p_cfg->location_bf_sink, p_cfg->location_bf_src);
    }
}

__STATIC void app_le_audio_capabilities_server_add_records(void)
{
    gaf_codec_id_t codec_id = GAF_CODEC_ID_LC3;
    uint8_t pac_lid = 0;
    uint8_t nb_records = 0;
    bap_capa_t* p_capa;
    uint8_t cnt;
    uint16_t status;

    if (p_env->nb_records_sink != 0)
    {
        p_capa = p_env->p_capa_sink;

        for (cnt = 0; cnt < p_env->nb_records_sink; cnt++)
        {
            if (nb_records == NB_RECORDS_PER_PAC_MAX)
            {
                pac_lid++;
                nb_records = 0;
            }

            status = bap_capa_srv_set_record(pac_lid, cnt + 1, &codec_id, p_capa, NULL);
            if (status)
            {
                APP_DBG("bap_capa_srv_set_record err 0x%X\n", status);
            }

            p_capa = (bap_capa_t*)((uint32_t)p_capa + CO_ALIGN4_HI(sizeof(bap_capa_t)));
            nb_records++;
        }

        pac_lid++;
        nb_records = 0;
    }

    if (p_env->nb_records_src != 0)
    {
        p_capa = p_env->p_capa_src;

        for (cnt = 0; cnt < p_env->nb_records_src; cnt++)
        {
            if (nb_records == NB_RECORDS_PER_PAC_MAX)
            {
                pac_lid++;
                nb_records = 0;
            }

            status = bap_capa_srv_set_record(pac_lid, p_env->nb_records_sink + cnt + 1,
                                                     &codec_id, p_capa, NULL);
            if (status)
            {
                APP_DBG("bap_capa_srv_set_record2 err 0x%X\n", status);
            }
            p_capa = (bap_capa_t*)((uint32_t)p_capa + CO_ALIGN4_HI(sizeof(bap_capa_t)));
            nb_records++;
        }
    }
}

/*
 * FUNCTIONS
 ****************************************************************************************
 */

void app_le_audio_capabilities_server_configure(const app_le_audio_capabilities_server_cfg_t* p_params,
                                                         component_cb_event cb_event)
{
    ASSERT_ERR(p_env == NULL);
    ASSERT_ERR(p_params != NULL);

    p_env = (app_le_audio_capabilities_server_env_t*)ke_malloc_user(sizeof(app_le_audio_capabilities_server_env_t),
                                                                    KE_MEM_PROFILE);
    ASSERT_ERR(p_env != NULL);
    memset(p_env, 0, sizeof(app_le_audio_capabilities_server_env_t));

    // p_env->control_intf.control = app_le_audio_capabilities_server_mediator_intf.control;
    // p_env->control_intf.event_bf = CO_BIT(COMPONENT_EVENT_CONNECTED);
//    utils_register_event(&(p_env->control_intf));

    app_le_audio_capabilities_server_prepare_records_sink(p_params);
    app_le_audio_capabilities_server_prepare_records_src(p_params);
    app_le_audio_capabilities_server_configure_bap_capa_srv(p_params);
    app_le_audio_capabilities_server_add_records();
}

/// \example{lineno} app_le_audio_capabilities_server.c
#endif