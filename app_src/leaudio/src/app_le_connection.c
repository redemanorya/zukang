/**
 ****************************************************************************************
 *
 * @file app_connection.c
 *
 * @brief Module that handle connection events
 *
 * Copyright (C) RivieraWaves 2009-2024
 * Release Identifier: c73db0ce
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "bt_config.h"
#if BLE_SUPPORT
#define BLE_HOST_PRESENT 1
#include <string.h>
#include "debug.h"
#include "co_math.h"
#include "gapm.h"
#include "gaf.h"
#include "gapc.h"
#include "gapc_sec.h"
#include "le_audio_config.h"
#include "le_udsc.h"
#include "le_udss.h"
#include "bap_capa_cli.h"

// 函数声明

#if (BLE_HOST_PRESENT)
#include "gapc_le.h"
#endif //(BLE_HOST_PRESENT)
#include "gapc_sec.h"
#include "bt_app_ddb_info.h"

#include "hl_error.h"
#include "app_int2.h"
#include "le_bis_assis.h"
#include "le_cis_src.h"
#include "le_cis_sink.h"
#include "app_le_coordinated_set_coordinator.h"
#include "app_le_volume_controller.h"
#include "app_le_audio_capabilities_client.h"
#include "app_le_media_client.h"
#include "app_le_call_client.h"
#include "bap_uc_cli.h"
#include "bt_app_ddb_info.h"
#include "aes_int.h"
#include "utils_audio.h"

/*
 * DEFINES
 ****************************************************************************************
 */

// Mark a variable unused to remove compilation warning
// (for variable defined but not used by SW in Debug mode)
#define APP_UNUSED(x) (void)(sizeof(x))

// Maximum number of connections
#define APP_CON_NB_MAX          (2)

// Authenticated payload timeout (LE Ping) in multiple of 10 milliseconds
#define AUTHENTICATED_PAYLOAD_TIMEOUT_10MS      (1000U)

/*
 * ENUMERATIONS
 ****************************************************************************************
 */

// Status bit field meaning
enum app_con_status
{
    // Indicate if pairing has been successfully performed
    APP_CON_STATUS_PAIRED_POS = 0,
    APP_CON_STATUS_PAIRED_BIT = CO_BIT(APP_CON_STATUS_PAIRED_POS),
};

// Metainfo for GAPC procedures
enum app_le_le_connection_gapc_metainfo
{
    // Disconnect
    GAPC_METAINFO_DISCONNECT = 0U,
    // Set Authentication Payload Timeout (LE Ping)
    GAPC_METAINFO_LE_PING,
    // Get list of peer supported features
    GAPC_METAINFO_PEER_FEATURES,
    // Update subrating
    GAPC_METAINFO_UPDATE_SUBRATING,
    // Use Periodic Advertising Sync Transfer
    GAPC_METAINFO_PAST,
};

/// Event values for Service Discovery state machine
enum app_event_service_discovery
{
    // State machine start has been required
    EVENT_SERVICE_DISCOVERY_START = 0U,
    // Presence of a service has been discovered
    EVENT_SERVICE_DISCOVERY_DISCOVERED,
    // General service discovery has been completed
    EVENT_SERVICE_DISCOVERY_GENERAL_END,
    // Content of a service has been discovered
    EVENT_SERVICE_DISCOVERY_CONTENT_DISCOVERED,
    // // Content of a service has been discovered
    // EVENT_SERVICE_DISCOVERY_CONTENT_COMPLETED,
    // LE Audio Discovery has been completed
    EVENT_SERVICE_DISCOVERY_COMPLETED,
};

/*
 * TYPE DEFINITIONS
 ****************************************************************************************
 */
typedef struct security_test_h7_result
{
    /// see enum #security_test_type
    uint8_t  test_type;
    /// AES result
    uint8_t  result[AES_BLOCK_SIZE];
} security_test_h7_result_t;

/// H7 Parameters
typedef struct security_test_h7_param
{
    /// see enum #security_test_type
    uint8_t  test_type;
    /// W
    uint8_t  w[AES_BLOCK_SIZE];
    /// Key ID
    uint8_t  key_id[AES_BLOCK_SIZE];
} security_test_h7_param_t;

/* SALT = 0x00000000000000000000000000000000746D7031 */
const uint8_t salt[16] = { 0x31, 0x70, 0x6d, 0x74 };
uint8_t central_add[6];
typedef void (*security_test_result_cb)(uint16_t result_size, const void* p_result);
uint8_t lebr[4]={'r','b','e','l'};
uint8_t tmp1[4]={'1','p','m','t'};

extern uint8_t le_linkkey[16];
extern bool linkkey_2_ltk_flag;
// Application bond data - should be a smarter key manager
/// Bond data
typedef struct app_con_bond_data_
{
    // Bond data for specific address
    gap_bdaddr_t addr; //!< address
    // Corresponding keys
    gapc_pairing_keys_t keys; //!< keys

    uint8_t pairing_level;
    bool enc_key_present;

    gapc_bond_data_t bond_data;
} app_con_bond_data_t;

// Application connection information
/// Connection information
typedef struct app_con_info_
{
    // Peer device address
    gap_bdaddr_t addr; //!< address
    // Connection index
    uint8_t conidx; //!< connection index
    // Status (see app_con_status enumeration)
    uint8_t status_bf; //!< status bit field
    //Master or slave role
    uint8_t role;
    //peer databse hash
    uint8_t service_hash[16];
    uint8_t local_auth;
    uint8_t remote_auth;
    uint8_t audio_loc
} app_con_info_t;

/// Parameters structure for #COMPONENT_EVENT_ENCRYPTION_STARTED event type
typedef struct component_event_encryption_started
{
    /// Connection index
    uint8_t conidx;
} component_event_encryption_started_t;

/// Parameters structure for #COMPONENT_EVENT_PAIRED event type
typedef struct component_event_paired
{
    /// Connection index
    uint8_t conidx;
    /// Pairing level
    uint8_t pairing_level;
} app_event_paired_t;

/// Parameters structure for #COMPONENT_CONTROL_START mediator control type
typedef struct app_gaf_cli_mediator_start
{
    /// Connection index
    uint8_t conidx;
} app_gaf_cli_mediator_start_t;

/// Parameters structure for #COMPONENT_EVENT_SERVICE_DISCOVERED event type
typedef struct app_event_service_discovered
{
    /// Connection index
    uint8_t conidx;
    /// UUID
    uint16_t uuid;
    /// Service information
    prf_svc_t svc;
} app_event_service_discovered_t;

typedef struct app_control_start_discovery
{
    /// Connection index
    uint8_t conidx;
    /// Pointer to service information\n
    /// May be NULL
    const prf_svc_t* p_svc;
} app_control_start_discovery_t;

typedef struct app_gaf_cli_mediator_stopped
{
    /// Connection index
    uint8_t conidx;
} app_gaf_cli_mediator_stopped_t;

// Environment structure
typedef struct app_env
{
    uint8_t actv_idx;
    /// Connection index
    uint8_t conidx;

    // Published Audio Capabilities Service information
    prf_svc_t uuid_ab04;
    prf_svc_t pacs;
    prf_svc_t udss;
    prf_svc_t bass;
    prf_svc_t ascs;
    prf_svc_t mcs;
    prf_svc_t ots;
    prf_svc_t tbs;
    prf_svc_t vcs;
    prf_svc_t csis;
    prf_svc_t cas;
    prf_svc_t tmas;

    uint8_t device_num;

    // Number of Coordinated Devices remaining to discover
    uint8_t nb_set_members_remaining;
    // Indicate if connection is being established with other Set members
    bool is_connecting_members;    
} app_env_t;
/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
// Pointer to environment
__STATIC app_env_t* p_env = NULL;

// Bond data
__STATIC app_con_bond_data_t app_con_bond_data[APP_CON_NB_MAX];
// Application connection information
__STATIC app_con_info_t app_con_info[APP_CON_NB_MAX];

extern void le_cis_src_cfg_lc3(uint8_t conidx);

/*
 * EXTERNAL FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */

#if (BLE_HOST_PRESENT)
extern uint16_t app_get_appearance(void);
extern const char* app_get_dev_name(uint8_t* p_name_len);
#endif //(BLE_HOST_PRESENT)
uint8_t sink_need_restart_udss_adv = 0;
extern uint8_t conn_update;
uint8_t linkkey[16];

extern void media_player_transition_group(uint8_t event, uint16_t status, uint8_t conidx);
extern void mediator_phone_start_group(void);
extern void load_ascs_info(uint8_t conidx, uint8_t* p_ascs_info);
// static uint8_t need_reconnect = 0;
/*
 * APPLICATION STATE MACHINES
 ****************************************************************************************
 */
void H6KeyInvert_2(uint16_t result_size,const void* p_result)
{
    printf("H6KeyInvert_2\n");
}

__STATIC void security_test_h6_result_cb_1(uint8_t status, const uint8_t* aes_res, security_test_result_cb cb_result)
{
	printf("status2=%x\n",status);
	memcpy(linkkey, aes_res, 16);
    uint8_t key_type = 8;

    LeSaveLinkey(app_con_bond_data[0].keys.irk.identity.addr, key_type, aes_res);
    uint8_t i;
    for(i = 0; i< 16; i++)
        printf(" %02x ",aes_res[i]);
    printf("save linkkey\n");

#if zsq
		uint8_t i;
		for(i = 0; i< 16; i++){

			linkkey[i]=aes_res[15-i];
			printf(" %02x ",linkkey[i]);
		}
		printf("save linkkey\n");
		LeSaveLinkey(central_add, linkkey, 0x08);
#endif
}

void H6KeyInvert(uint16_t result_size,const void* p_result)
{
    printf("H6KeyInvert\n");
}

__STATIC void security_test_h6_result_cb(uint8_t status, const uint8_t* aes_res, security_test_result_cb cb_result)
{
    printf("security_test_h6_result_cb\n");
	aes_h6(aes_res, lebr, security_test_h6_result_cb_1, H6KeyInvert_2);
}

void H7KeyInvert(uint16_t result_size, const void* p_result)
{
	printf("h7 key invert ok: result_size %d\n", result_size);
    for (int i = 0; i < result_size; i++)
    {
        printf("0x%x ", *(uint8_t*)(p_result+i));
    }
    printf("\n");
}
extern void SecHandleLinkKeyNotify(uint8_t* addr , uint8_t keyType, uint8_t* linkkey);
__STATIC void security_test_h7_result_cb(uint8_t status, const uint8_t* aes_res, security_test_result_cb cb_result)
{
    printf("security_test_h7_result_cb\n");

    security_test_h7_result_t result;
    result.test_type = 11;//SECURITY_TEST_H7;
    memcpy(result.result, aes_res, AES_BLOCK_SIZE);
    aes_h6(aes_res,lebr,/*security_test_h6_result_cb*/security_test_h6_result_cb_1, H6KeyInvert);
    cb_result(sizeof(result), &result);

	printf("DDB_AddRecord: 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n",
				app_con_info[0].addr.addr[0], app_con_info[0].addr.addr[1], app_con_info[0].addr.addr[2], 
				app_con_info[0].addr.addr[3], app_con_info[0].addr.addr[4], app_con_info[0].addr.addr[5]);
}


void ble_save_bond_data(void)
{
	// SaveTotalBleDevRec2Flash(1 + BLE_REC_INFO_LEN * MAX_BLE_DEVICE_NUM,
	// 					p_env->device_num);
}

bool set_role_with_conidx(uint8_t role, uint8_t conidx)
{
    if (app_con_info[0].conidx == conidx)
    {
        app_con_info[0].role = role;
        return true;
    }
    else if (app_con_info[1].conidx == conidx)
    {
        app_con_info[1].role = role;     
        return true;
    }
    else
    {
        return false;
    }
}

uint8_t get_role_from_conidx(uint8_t conidx)
{
    uint8_t role = 0xff;

    while (1)
    {
        if (app_con_info[0].conidx == conidx)
        {
            role = app_con_info[0].role;
            break;
        }
        else if (app_con_info[1].conidx == conidx)
        {
            role = app_con_info[1].role;
            break;
        }
        else
        {
            role = 0xff;
            break;
        }
    }
    APP_DBG("get_role_from_conidx: conidx 0x%x, role 0x%x\n", conidx, role);
    return role;
}

bool set_audio_loc_with_conidx(uint8_t audio_loc, uint8_t conidx)
{
    if (app_con_info[0].conidx == conidx)
    {
        app_con_info[0].audio_loc = audio_loc;
        return true;
    }
    else if (app_con_info[1].conidx == conidx)
    {
        app_con_info[1].audio_loc = audio_loc;     
        return true;
    }
    else
    {
        return false;
    }
}

uint8_t get_audio_loc_from_conidx(uint8_t conidx)
{
    uint8_t audio_loc = 0xff;

    while (1)
    {
        if (app_con_info[0].conidx == conidx)
        {
            audio_loc = app_con_info[0].audio_loc;
            break;
        }
        else if (app_con_info[1].conidx == conidx)
        {
            audio_loc = app_con_info[1].audio_loc;
            break;
        }
        else
        {
            audio_loc = 0xff;
            break;
        }
    }
    APP_DBG("get_audio_loc_from_conidx: conidx 0x%x, audio_loc 0x%x\n", conidx, audio_loc);
    return audio_loc;
}


uint8_t get_conidx_from_role(uint8_t role)
{
    while (1)
    {
        if (app_con_info[0].role == role)
        {
            return app_con_info[0].conidx;
        }
        else if (app_con_info[1].role == role)
        {
            return app_con_info[1].conidx;
        }
        else
        {
            return 0xff;
        }
    }
}

/// LE Connection parameter procedure completed handler
__STATIC void gapc_le_update_params_cmp_cb(uint8_t conidx, uint32_t dest_id, uint16_t status)
{
    conn_update = 1;
    APP_DBG("gapc_le_update_params_cmp_cb: status 0x%x\n", status);
    if (get_role_from_conidx(conidx) == ROLE_SLAVE)
    {
        // uint32_t value = 0x13245768;
        // app_send_value_to_phone(0, value);
    }
}

void app_le_audio_discovery(uint8_t event, uint8_t conidx, uint16_t status, const void* p_params)
{
    APP_DBG("app_le_audio_discovery: event 0x%x\n", event);
    switch (event)
    {
        case EVENT_SERVICE_DISCOVERY_START:
        {
            app_gaf_cli_mediator_start_t params_start = {
                .conidx = conidx,
            };

            APP_DBG("Start discovery (conidx = %d)\n", params_start.conidx);
            status = gaf_cli_discover(params_start.conidx, 0);
            if (status)
            {
                APP_DBG("gaf_cli_discover error 0x%x\n", status);
            }
        } break;

        case EVENT_SERVICE_DISCOVERY_DISCOVERED:
        {
            const app_event_service_discovered_t params_service_discovered
                                                = *(const app_event_service_discovered_t*)p_params;
            prf_svc_t* p_svc;

            switch (GATT_UUID_16_LSB(params_service_discovered.uuid))
            {
                case GATT_SVC_AUDIO_STREAM_CTRL: { p_svc = &(p_env->ascs); } break;
                case GATT_SVC_BCAST_AUDIO_SCAN: { p_svc = &(p_env->bass); } break;
                case GATT_SVC_GENERIC_MEDIA_CONTROL: { p_svc = &(p_env->mcs); } break;
                case GATT_SVC_GENERIC_TELEPHONE_BEARER: { p_svc = &(p_env->tbs); } break;
                case GATT_SVC_VOLUME_CONTROL: { p_svc = &(p_env->vcs); } break;
                case GATT_SVC_OBJECT_TRANSFER: { p_svc = &(p_env->ots); } break;
                case GATT_SVC_PUBLISHED_AUDIO_CAPA: { p_svc = &(p_env->pacs); } break;
                case GATT_SVC_USER_DATA: { p_svc = &(p_env->udss); } break;
                case GATT_LALI_USER_UUID: { p_svc = &(p_env->uuid_ab04); } break;
                case GATT_SVC_COMMON_AUDIO: { p_svc = &(p_env->cas); } break;
                case GATT_SVC_TELEPHONY_MEDIA_AUDIO: { p_svc = &(p_env->tmas); } break;
                case GATT_SVC_COORD_SET_IDENTIFICATION: { p_svc = &(p_env->csis); } break;

                default: 
                { 
                    APP_DBG("other uuid 0x%x\n", GATT_UUID_16_LSB(params_service_discovered.uuid));
                    p_svc = NULL; 
                } break;
            }

            if (p_svc != NULL)
            {
                memcpy(p_svc, &(params_service_discovered.svc), sizeof(prf_svc_t));
            }
        } break;

        case EVENT_SERVICE_DISCOVERY_GENERAL_END:
        {
            if (status != GAF_ERR_NO_ERROR)
            {
                break;
            }
        }
        // no break;

        case EVENT_SERVICE_DISCOVERY_CONTENT_DISCOVERED:
        {
            app_control_start_discovery_t params_start_discovery = {
                .conidx = conidx,
            };
            APP_DBG("EVENT_SERVICE_DISCOVERY_CONTENT_DISCOVERED bass 0x%x, pacs 0x%x, vcs 0x%x\n", p_env->bass.shdl, p_env->pacs.shdl, p_env->vcs.shdl);
#if (BT_LE_AUDIO_SUPPORT == LE_CIS_SUPPORT)
            if (p_env->pacs.shdl != GATT_INVALID_HDL)
            {
                params_start_discovery.p_svc = &(p_env->pacs);
   
                app_le_audio_capabilities_client_transition_service_discovery(EVENT_SERVICE_DISCOVERY_START, params_start_discovery.conidx,
                                                                                GAF_ERR_NO_ERROR, params_start_discovery.p_svc);
                p_env->pacs.shdl = GATT_INVALID_HDL;
                break;
            }

            if (p_env->mcs.shdl != GATT_INVALID_HDL)
            {
                params_start_discovery.p_svc = &(p_env->mcs);

                app_le_media_client_state_service_discovery(EVENT_SERVICE_DISCOVERY_START, GAF_ERR_NO_ERROR, 
                                                        params_start_discovery.conidx, params_start_discovery.p_svc);
                p_env->mcs.shdl = GATT_INVALID_HDL;
                break;
            }

            if (p_env->vcs.shdl != GATT_INVALID_HDL)
            {
                params_start_discovery.p_svc = &(p_env->vcs);
                app_le_volume_controller_state_service_discovery(EVENT_SERVICE_DISCOVERY_START,  params_start_discovery.conidx,
                													GAF_ERR_NO_ERROR, params_start_discovery.p_svc);
                p_env->vcs.shdl = GATT_INVALID_HDL;
                break;
            }

            if (p_env->tbs.shdl != GATT_INVALID_HDL)
            {
                params_start_discovery.p_svc = &(p_env->tbs);
                app_le_call_client_state_service_discovery(EVENT_SERVICE_DISCOVERY_START, 0, GAF_ERR_NO_ERROR);
                p_env->tbs.shdl = GATT_INVALID_HDL;
                break;
            }
            if (p_env->csis.shdl != GATT_INVALID_HDL)
            {
                app_le_coordinated_set_coordinator_start_discovery_t csis_params_start_discovery;
                csis_params_start_discovery.basic.p_svc = &(p_env->csis);
                csis_params_start_discovery.basic.conidx = params_start_discovery.conidx;
                csis_params_start_discovery.p_sirk = NULL;

                app_le_coordinated_set_coordinator_state_service_discovery(EVENT_SERVICE_DISCOVERY_START, GAF_ERR_NO_ERROR,
                															&csis_params_start_discovery);
                p_env->csis.shdl = GATT_INVALID_HDL;
                break;
            }
#endif
#ifdef LE_CIS_SRC
            if (p_env->ascs.shdl != GATT_INVALID_HDL)
            {
                params_start_discovery.p_svc = &(p_env->ascs);

                le_cis_src_state_service_discovery(EVENT_SERVICE_DISCOVERY_START, params_start_discovery.conidx, GAF_ERR_NO_ERROR, params_start_discovery.p_svc);
                p_env->ascs.shdl = GATT_INVALID_HDL;
                break;
            }
#endif
#ifdef LE_BIS_ASSIS
            if (p_env->bass.shdl != GATT_INVALID_HDL)
            {
                params_start_discovery.p_svc = &(p_env->bass);

                le_bis_assis_state_service_discovery(EVENT_SERVICE_DISCOVERY_START, 0, &params_start_discovery);
                p_env->bass.shdl = GATT_INVALID_HDL;
                break;
            }
#endif
#ifdef LE_UDSC
            if (p_env->udss.shdl != GATT_INVALID_HDL)
            {
                params_start_discovery.p_svc = &(p_env->udss);

                status = le_udsc_state_service_discovery(EVENT_SERVICE_DISCOVERY_START, params_start_discovery.conidx, GAP_ERR_NO_ERROR, GATT_UUID_16_LSB(GATT_SVC_USER_DATA), p_env->udss.shdl,p_env->udss.ehdl);
                if (status)
                {
                    APP_DBG("le_udsc_state_service_discovery: failed 0x%x\n", status);
                }

                p_env->udss.shdl = GATT_INVALID_HDL;
                break;
                
            }

            if (p_env->uuid_ab04.shdl != GATT_INVALID_HDL)
            {
                params_start_discovery.p_svc = &(p_env->uuid_ab04);

                status = le_udsc_state_service_discovery(EVENT_SERVICE_DISCOVERY_START, params_start_discovery.conidx, GAP_ERR_NO_ERROR,0xab04,p_env->uuid_ab04.shdl,p_env->uuid_ab04.ehdl);
                if (status)
                {
                    APP_DBG("le_udsc_state_service_discovery: failed 0x%x\n", status);
                }

                p_env->uuid_ab04.shdl = GATT_INVALID_HDL;
                break;

            }
#endif
        }
        break;

        case EVENT_SERVICE_DISCOVERY_COMPLETED:
        {
#ifdef LE_CIS_SRC
            le_cis_src_cfg_lc3(conidx);
#endif
            ble_save_bond_data();
#ifdef LE_CIS_SRC
            // if (get_cis_scene() == CIS_APP_MEDIA_PLAYER)
            {
                media_player_transition_group(EVENT_GROUP_START, GAF_ERR_NO_ERROR, conidx);
            }
            // else if (get_cis_scene() == CIS_APP_PHONE)
            // {
            //     mediator_phone_start_group();
            // }

            app_update_conn_para(conidx);

            utils_audio_le_cfg_t* p_audio_cfg = utils_audio_le_get_cfg(conidx);

            printf("baikal: utils_audio_le_get_cfg11: nb_set_members %d\n", p_audio_cfg->nb_set_members);
            if (p_audio_cfg->nb_set_members > 1)
            {
                if (p_env->is_connecting_members == true)
                {
                    // Another member has been found
                    p_env->nb_set_members_remaining--;
                    if (p_env->nb_set_members_remaining == 0)
                    {
                        // All members have been found
                        p_env->is_connecting_members = false;

                        // //stop scan TODO:
                        // status = app_le_udsc_stop_scan();
                        // if (status)
                        // {
                        //     APP_DBG("scan stop failed: 0x%x\n", status);
                        // }
                    }
                }
                else
                {
                    p_env->is_connecting_members = true;
                    p_env->nb_set_members_remaining = p_audio_cfg->nb_set_members - 1;
                }

                printf("baikal: is_connecting_members: 0x%x\n", p_env->is_connecting_members);
                if (p_env->is_connecting_members == true)
                {
                    // Seek for Set Members
#ifdef LE_UDSC
                    app_le_udsc_start_scan();
#endif
                    break;
                }
            }
#endif
#ifdef LE_BIS_ASSIS
            le_bis_assis_start_audio();
#endif
        } break;

        default: { } break;
    }
}

__STATIC void app_gaf_client_cb_cmp_evt(uint8_t cmd_type, uint16_t status, uint8_t conidx)
{
    ASSERT_WARN(status == GAF_ERR_NO_ERROR, status, cmd_type);

    APP_DBG("Discovery completed (conidx = %d)\n", conidx);

    app_le_audio_discovery(EVENT_SERVICE_DISCOVERY_GENERAL_END, conidx, status, NULL);
}

__STATIC void app_gaf_client_cb_svc_found(uint8_t conidx, uint16_t uuid, uint16_t shdl, uint16_t ehdl)
{
    app_event_service_discovered_t params_service_discovered = {
        .conidx = conidx,
        .uuid = uuid,
        .svc = {
            .shdl = shdl,
            .ehdl = ehdl,
        },
    };

    APP_DBG("Service found: UUID = 0x%04X, SHDL=0x%04X (conidx = %d), %d\n", uuid, shdl, conidx, params_service_discovered.svc.shdl);
    app_le_audio_discovery(EVENT_SERVICE_DISCOVERY_DISCOVERED, conidx, GAF_ERR_NO_ERROR, &params_service_discovered);
}

// Set of callback function for communication with GAF Client module
__STATIC const gaf_cli_cb_t cbs_gaf_cli = {
    .cb_cmp_evt = app_gaf_client_cb_cmp_evt,
    .cb_svc_found = app_gaf_client_cb_svc_found, 
};

uint16_t app_gaf_client_configure(void)
{
    if (!p_env)
    {
        p_env = (app_env_t*)ke_malloc_user(sizeof(app_env_t), KE_MEM_PROFILE);
        ASSERT_ERR(p_env != NULL);
        memset(p_env, 0, sizeof(app_env_t));
        p_env->conidx = GAP_INVALID_CONIDX;

        app_con_info[0].conidx = GAP_INVALID_CONIDX;
        app_con_info[1].conidx = GAP_INVALID_CONIDX;
    }

    // APP_DBG("app_gaf_client_configure: 0x%x\n", p_env->udss);
    uint16_t status = gaf_cli_configure(&cbs_gaf_cli);
    return status;
}

void app_le_link_configure(void)
{
    if (!p_env)
    {
        p_env = (app_env_t*)ke_malloc_user(sizeof(app_env_t), KE_MEM_PROFILE);
        ASSERT_ERR(p_env != NULL);
        memset(p_env, 0, sizeof(app_env_t));
        p_env->conidx = GAP_INVALID_CONIDX;

        app_con_info[0].conidx = GAP_INVALID_CONIDX;
        app_con_info[1].conidx = GAP_INVALID_CONIDX;
    }

    APP_DBG("app_le_link_configure: 0x%x\n", p_env->udss);
}

void app_update_conn_para(uint8_t conidx)
{
    gapc_le_con_param_nego_with_ce_len_t param;
    param.hdr.interval_max = 384;//192;
    param.hdr.interval_min = 384;//192;
    param.hdr.latency = 0;
    param.hdr.sup_to = 1000;  
    param.ce_len_max = 10;
    param.ce_len_min = 5;

    uint16_t status = gapc_le_update_params(conidx, 0, &param, gapc_le_update_params_cmp_cb);
    if (status)
    {
        APP_DBG("conn update error, status 0x%x\n", status);
    }
}

uint16_t app_secure_req(uint8_t conidx)
{
    uint8_t auth;
    uint16_t status;
#ifdef LINK_WITH_PHONE
    auth = GAP_AUTH_REQ_SEC_CON_BOND | GAP_AUTH_CT2, //GAP_AUTH_REQ_MITM_BOND;
#else
    auth = GAP_AUTH_REQ_NO_MITM_BOND;
#endif
    status = gapc_le_request_security(conidx, auth);
    if (status)
    {
        APP_DBG("gapc_le_request_security error 0x%x\n", status);
    }
    return status;
}

__STATIC void app_le_connection_cb_gapc_peer_features(uint8_t conidx, uint32_t metainfo, uint16_t status,
                                                         const uint8_t* p_features)
{
    ASSERT_WARN(status == GAP_ERR_NO_ERROR, status, metainfo);

    if (status == GAP_ERR_NO_ERROR)
    {
        // APP_DBG("Peer features (conidx = %d) - %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\n", conidx,
        //              p_features[0], p_features[1], p_features[2], p_features[3], p_features[4], p_features[5],
        //              p_features[6], p_features[7]);
    }
}


/// Callback executed when read attribute database hash procedure is completed.
__STATIC void app_read_database_hash_cmp_cb(uint8_t conidx, uint32_t dest_id, uint16_t status, uint16_t handle,
                                               const uint8_t* p_hash)
{  
    //if hash is not match, then start service discovery again
    if (memcmp(p_hash, app_con_info[conidx].service_hash, 16))
    {
#if defined(LE_UDSC) || defined(LE_BIS_ASSIS) || defined(LE_CIS_SRC)
        if (get_role_from_conidx(conidx) == ROLE_MASTER)
        {
            // Start service discovery
            app_le_audio_discovery(EVENT_SERVICE_DISCOVERY_START, conidx, GAP_ERR_NO_ERROR, NULL);
        }
#endif
#ifdef LE_CIS_SINK
        if (get_role_from_conidx(conidx) == ROLE_SLAVE)
        {
            // Start service discovery
            app_le_audio_discovery(EVENT_SERVICE_DISCOVERY_START, conidx, GAP_ERR_NO_ERROR, NULL);//MCC
        }
#endif        
    }
    else
    {
#ifdef LE_CIS_SRC
        // le_cis_src_start_audio();
#endif
    }
}

bool is_link_with_phone = 0;
__STATIC void app_state_transition_local(uint8_t event, uint16_t status, const void* p_param)
{
    uint16_t ret;

    APP_DBG("app_state_transition_local: event 0x%x, status 0x%x\n", event, status);
    switch (event)
    {
        case APP_LE_CONNECTION_ESTABLISHED: //
        {
            uint8_t conidx = *((uint8_t*)p_param);
            if (get_conidx_from_role(conidx) == ROLE_SLAVE)
            {
                is_link_with_phone = 1;   
            }

#ifdef LE_BIS_ASSIS
            status = bap_bc_assist_stop_scan();
            if (status)
            {
                APP_DBG("scan stop failed; status 0x%x\n", status);
            }
#endif

#ifdef LE_UDSC
            if (1)//(prase_adv_data("Atm SWC", p_adv_data, length))
            {
                status = app_le_udsc_stop_scan();
                if (status)
                {
                    APP_DBG("scan stop failed: 0x%x\n", status);
                }
            }else
            {
                return;
            }
#endif
            ret = gapc_le_get_peer_features(conidx, GAPC_METAINFO_PEER_FEATURES,
                                            app_le_connection_cb_gapc_peer_features);
            if (ret)
            {
                APP_DBG("gapc_le_get_peer_features: error ret 0x%x\n", ret);
            }

            // if (Get_bis_role() == BIS_SINK)
            {
#if defined(LE_UDSC) || defined(LE_CIS_SRC) || defined(LE_BIS_ASSIS)
                if (get_role_from_conidx(conidx) == ROLE_MASTER)
                {
                    ret = app_le_start_pairing(conidx);
                    if (ret)
                    {
                        APP_DBG("app_le_start_pairing: error ret 0x%x\n", ret);
                    }
                }
#endif
            }
// //            else if (Get_bis_role() == BIS_SRC)
//             {
// #ifdef LE_BIS_DELEG
//                 if (get_role_from_conidx(conidx) == ROLE_MASTER)
//                 {
//                     ret = app_le_start_pairing(conidx);
//                     if (ret)
//                     {
//                         APP_DBG("app_le_start_pairing: error ret 0x%x\n", ret);
//                     }                
//                 }
// #endif                
//             }

#if defined(LE_UDSS) && !defined(LINK_WITH_PHONE)   
    APP_DBG("app_state_transition_local: APP_LE_CONNECTION_ESTABLISHED conidx 0x%x\n", conidx);
            // app_secure_req(conidx);         
#endif
        } break;

        case APP_PAIRING_SUCCEED: 
        {
            app_event_paired_t pair_param = *(app_event_paired_t*)p_param;

#if defined(LE_UDSC) || defined(LE_BIS_ASSIS) || defined(LE_CIS_SRC)
                if (get_role_from_conidx(pair_param.conidx) == ROLE_MASTER)
                {
                    // Start service discovery
                    app_le_audio_discovery(EVENT_SERVICE_DISCOVERY_START, pair_param.conidx, GAP_ERR_NO_ERROR, NULL);
                }
#endif
#ifdef LE_CIS_SINK
                if (get_role_from_conidx(pair_param.conidx) == ROLE_SLAVE)
                {
                    // Start service discovery
                    app_le_audio_discovery(EVENT_SERVICE_DISCOVERY_START, pair_param.conidx, GAP_ERR_NO_ERROR, NULL);//MCC
                }
#endif
#ifdef LE_UDSS
                if (get_role_from_conidx(pair_param.conidx) == ROLE_SLAVE)
                {
                    uint16_t db_chg_inc_ccc = 1;
                    uint16_t user_ctrl_pt_ccc = 1;

                    ret = udss_enable(pair_param.conidx, db_chg_inc_ccc, user_ctrl_pt_ccc);
                    if (ret)
                    {
                        APP_DBG("udss_enable failed, ret 0x%x\n", ret);
                    }
                }
#endif
        } break;

        case APP_LE_CONNECTION_ESTABLISHED_PAIRED: //
        {

            uint8_t conidx = *((uint8_t*)p_param);
            // set_ltk_2_controller(conidx);
#ifdef LE_CIS_SRC
            le_cis_src_restore_bond_data(conidx);
#endif
#ifdef LE_CIS_SINK
            if (Get_cis_role() == CIS_SINK)
            {
                le_cis_sink_restore_bond_data(conidx);
            }
#endif
            // if (need_reconnect)
            {
                app_update_conn_para(conidx);
            }

            if ((get_role_from_conidx(conidx) == ROLE_MASTER) || (app_con_info[conidx].role == ROLE_MASTER))
            {
                // Request encryption
                status = app_le_encrypt(conidx);
                if (status)
                {
                    printf("app_le_encrypt: err 0x%x\n", status);
                }
            }
            //read database_hash service
            status = gapc_le_get_peer_db_hash(conidx, conidx, app_read_database_hash_cmp_cb);
        } break;

        case APP_ENCRYPTION_STARTED: //
        {
            // if (need_reconnect)
            {
#ifdef LE_CIS_SRC
                uint8_t conidx = *((uint8_t*)p_param);
                le_cis_src_restore_bond_data(conidx);
                // le_cis_src_start_audio();
#endif
                // need_reconnect = 0;
            }
        } break;

        case APP_DISCONNECTED: //
        {
            ret = gapm_delete_activity(p_env->actv_idx);
            if (ret)
            {
                printf("gapm_delete_activity: err 0x%x\n", ret);
            }

            conn_update = 0;
            // need_reconnect = 1;
            uint8_t conidx = *((uint8_t*)p_param);
            uint8_t role = get_role_from_conidx(conidx);
#if defined(LE_UDSS) && !defined(LE_BIS_DELEG) && !defined(LE_CIS_SINK)
            if (role == ROLE_SLAVE)
            {
                is_link_with_phone = 0;
                app_udss_start_adv();     
            }
#endif
#if !defined(LE_BIS_ASSIS) && defined(LE_UDSC) && !defined(LE_CIS_SRC)
            if (role == ROLE_MASTER)
            {
                app_le_udsc_start_scan();
            }
#endif
#if !defined(LE_UDSS) && defined(LE_BIS_DELEG) && !defined(LE_CIS_SINK)
            if (role == ROLE_SLAVE)
            {
                is_link_with_phone = 0;
                le_bis_deleg_start_solicite();     
            }
#endif
#if defined(LE_BIS_ASSIS) && !defined(LE_UDSC) && !defined(LE_CIS_SRC)
            if (role == ROLE_MASTER)
            {
                status = bap_bc_assist_start_scan(0);

                if (status != GAF_ERR_NO_ERROR)
                {
                    APP_DBG("bap_bc_assist_start_scan status: %d\n",status);
                }
            }
#endif
#if !defined(LE_UDSS) && !defined(LE_BIS_DELEG) && defined(LE_CIS_SINK)
            if (role == ROLE_SLAVE)
            {
                is_link_with_phone = 0;
                app_le_adv_start(NULL);     
            }
#endif
#if !defined(LE_BIS_ASSIS) && !defined(LE_UDSC) && defined(LE_CIS_SRC)
            if (role == ROLE_MASTER)
            {
                app_le_scan_start(NULL);
            }
#endif
            //clear conidx
            if (app_con_info[0].conidx == conidx)
            {
                app_con_info[0].conidx = GAP_INVALID_CONIDX;
            }

            if (app_con_info[1].conidx == conidx)
            {
                app_con_info[1].conidx = GAP_INVALID_CONIDX;
            } 
        } break;

        default: { } break;
    }
}
/*
 * CONNECTION MANAGEMENT
 ****************************************************************************************
 */

///! [APP_LE_CON_CB]
#if (BLE_HOST_PRESENT)
__STATIC void app_con_le_connection_req(uint8_t conidx, uint32_t metainfo, uint8_t actv_idx, uint8_t role,
                                        const gap_bdaddr_t* p_peer_addr, const gapc_le_con_param_t* p_con_params,
                                        uint8_t clk_accuracy)
{
    gapc_bond_data_t bond_data;
    uint16_t status;
    uint8_t device_lid = (app_con_info[0].conidx == GAP_INVALID_CONIDX) ? 0 : 1;

    APP_DBG("LE connection created: 0x%02X:%02X:%02X:%02X:%02X:%02X, device_lid 0x%x, conidx 0x%x, role 0x%x\n",
             p_peer_addr->addr[5], p_peer_addr->addr[4], p_peer_addr->addr[3], p_peer_addr->addr[2],
             p_peer_addr->addr[1], p_peer_addr->addr[0], device_lid, app_con_info[0].conidx, role);

    utils_add_peer(conidx, (const gap_addr_t*)(p_peer_addr->addr), p_peer_addr->addr_type, true);

    p_env->actv_idx = actv_idx;
    app_con_info[device_lid].conidx = conidx;
    memcpy(&(app_con_info[device_lid].addr), p_peer_addr, sizeof(gap_addr_t));
    // app_con_info[device_lid].addr.addr_type = GAP_ADDR_PUBLIC;
    memcpy(central_add, p_peer_addr->addr, 6);
    printf("addr=%02x %02x %02x %02x %02x %02x\n",central_add[0],central_add[1],central_add[2],central_add[3],central_add[4],central_add[5], p_peer_addr->addr_type);

    //baikal add 
    if (linkkey_2_ltk_flag)
    {
        SETB(app_con_info[device_lid].status_bf, APP_CON_STATUS_PAIRED, 1);
    }

    bool is_vaild = set_role_with_conidx(role, conidx);
    if (!is_vaild)
    {
        APP_DBG("err role for releted conidx!!!!\n");
    }

#if 0
    ble_link_bond_data_t ble_link_bond_data;
    ascs_bond_data_t ascs_bond_data;
    int ret = FlshBleLoad1of8Dev(1 + BLE_REC_INFO_LEN * MAX_BLE_DEVICE_NUM, conidx, &ble_link_bond_data, &ascs_bond_data);

    if (!ret)
    {
        memset(&ble_link_bond_data, 0, sizeof(ble_link_bond_data_t));
        ble_link_bond_data.ble_role = 0xff;
    }
    printf("addr 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x, ltk 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x, role 0x%x 0x%x: paired 0x%x, 0x%x\n",
               ble_link_bond_data.addr.addr[0], ble_link_bond_data.addr.addr[1], ble_link_bond_data.addr.addr[2], ble_link_bond_data.addr.addr[3], ble_link_bond_data.addr.addr[4], 
               ble_link_bond_data.addr.addr[5], ble_link_bond_data.ltk.key.key[0], ble_link_bond_data.ltk.key.key[1], ble_link_bond_data.ltk.key.key[2], 
               ble_link_bond_data.ltk.key.key[3], ble_link_bond_data.ltk.key.key[4], ble_link_bond_data.ltk.key.key[5], ble_link_bond_data.ltk.key.key[6], 
               ble_link_bond_data.ltk.key.key[7], ble_link_bond_data.ltk.key.key[8], ble_link_bond_data.ltk.key.key[9], ble_link_bond_data.ltk.key.key[10], 
               ble_link_bond_data.ltk.key.key[11], ble_link_bond_data.ltk.key.key[12], ble_link_bond_data.ltk.key.key[13], ble_link_bond_data.ltk.key.key[14], 
               ble_link_bond_data.ltk.key.key[15], ble_link_bond_data.ble_role, role, ble_link_bond_data.is_paired, ret);

    // printf("irk 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x, num 0x%x, is_paired 0x%x\n", 
    //             irk[0], irk[1], irk[2], irk[3], irk[4], irk[5], irk[6], irk[7], irk[8],
    //              irk[9], irk[10], irk[11], irk[12], irk[13], irk[14], irk[15], p_env->device_num, is_paired);
    // printf("hash 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n", 
    //             hash[0], hash[1], hash[2], hash[3], hash[4], hash[5], hash[6], hash[7], hash[8],
    //              hash[9], hash[10], hash[11], hash[12], hash[13], hash[14], hash[15]);

    if ((ble_link_bond_data.ble_role == role) && (ble_link_bond_data.is_paired == 1))
    {
    	//check ble dev list
    	if (!memcmp(&ble_link_bond_data.addr, p_peer_addr, sizeof(gap_bdaddr_t)))
    	{
            // pairing_level = GAP_PAIRING_BOND_SECURE_CON;
            // enc_key_present = true;

    		printf("found paired device: pairing_level 0x%X, 0x%X\n", 
                        ble_link_bond_data.pairing_lvl, ble_link_bond_data.enc_key_present);
            SETB(app_con_info[device_lid].status_bf, APP_CON_STATUS_PAIRED, 1);

            bond_data.local_sign_counter = ble_link_bond_data.bond_data.local_sign_counter;
            bond_data.remote_sign_counter = ble_link_bond_data.bond_data.peer_sign_counter;
            bond_data.gatt_start_hdl = ble_link_bond_data.bond_data.gatt_start_hdl;
            bond_data.gatt_end_hdl = ble_link_bond_data.bond_data.gatt_end_hdl;
            bond_data.svc_chg_hdl = ble_link_bond_data.bond_data.svc_chg_hdl;
            bond_data.cli_info = ble_link_bond_data.bond_data.cli_info;
            bond_data.cli_feat = ble_link_bond_data.bond_data.cli_feat;
            bond_data.srv_feat = ble_link_bond_data.bond_data.srv_feat;

            memcpy(app_con_info[conidx].service_hash, ble_link_bond_data.hash, 16);
            memcpy(&app_con_bond_data[device_lid].keys.ltk, &ble_link_bond_data.ltk, sizeof(gapc_ltk_t));
            memcpy(&app_con_bond_data[device_lid].keys.irk, &ble_link_bond_data.irk, sizeof(gapc_irk_t));

            //load security info
            app_con_bond_data[conidx].pairing_level = ble_link_bond_data.pairing_lvl;
            app_con_bond_data[conidx].enc_key_present = ble_link_bond_data.enc_key_present;
            app_con_info[device_lid].role = role;

            utils_set_paired(conidx, ble_link_bond_data.pairing_lvl);
            if (ble_link_bond_data.enc_key_present)
            {
                utils_set_encryption_key_known(conidx);
            }

#ifdef LE_CIS_SRC
            load_ascs_src_info(conidx, &ascs_bond_data);
#endif
#ifdef LE_CIS_SINK
            load_ascs_sink_info(conidx, &ascs_bond_data);
#endif
    	}
    }
#endif

    if (CHECKB(app_con_info[device_lid].status_bf, APP_CON_STATUS_PAIRED))
    {
        bond_data.enc_key_present = true;
#ifdef LINK_WITH_PHONE
        bond_data.pairing_lvl = GAP_PAIRING_BOND_SECURE_CON;//GAP_PAIRING_SECURE_CON;
#else
        bond_data.pairing_lvl = GAP_PAIRING_BOND_PRESENT_BIT;
#endif
    }


    if (role == ROLE_MASTER)
    {
#if defined(LE_UDSC) && !defined(LE_CIS_SRC)
        bond_data.pairing_lvl = GAP_PAIRING_BOND_PRESENT_BIT;     
#endif

    }

    // Automatically accept connection
    status = gapc_le_connection_cfm(conidx, 0,
                           GETB(app_con_info[device_lid].status_bf, APP_CON_STATUS_PAIRED) ? &bond_data : NULL);

    printf("gapc_le_connection_cfm:  0x%x, status 0x%x\n",
                 CHECKB(app_con_info[device_lid].status_bf, APP_CON_STATUS_PAIRED), status);
//    if ((ble_link_bond_data.ble_role == role) && (1 == ble_link_bond_data.is_paired))
//    {
//    	//check ble dev list
//    	if (!memcmp(&ble_link_bond_data.addr, p_peer_addr, sizeof(gap_bdaddr_t)))
//    	{
//    		printf("found paired device\n");
//        	app_state_transition_local(APP_LE_CONNECTION_ESTABLISHED_PAIRED, GAP_ERR_NO_ERROR, &conidx);
//    	}
//    }
//    else
    {
        if (CHECKB(app_con_info[device_lid].status_bf, APP_CON_STATUS_PAIRED))
        {
            
        	app_state_transition_local(APP_LE_CONNECTION_ESTABLISHED_PAIRED, GAP_ERR_NO_ERROR, &conidx);
        }
        else
        {
        	app_state_transition_local(APP_LE_CONNECTION_ESTABLISHED, GAP_ERR_NO_ERROR, &conidx);
        }
    }
}

#if (!(0))
// Callback that handle connection establishment
__STATIC const gapc_connection_req_cb_t app_connection_req_cb_itf =
{
    .le_connection_req          = app_con_le_connection_req,
};
#endif // (!(0))
#endif // (BLE_HOST_PRESENT)
///! [APP_LE_CON_CB]


// Link disconnection
__STATIC void app_cb_gapc_disconnected(uint8_t conidx, uint32_t metainfo, uint16_t reason)
{
    APP_DBG("LINK DISCONNECTED %d, 0x%04X\n", conidx, reason);   

    app_state_transition_local(APP_DISCONNECTED, GAP_ERR_NO_ERROR, &conidx);
}

#if (BLE_HOST_PRESENT)
__STATIC void app_cb_gapc_name_get(uint8_t conidx, uint32_t metainfo, uint16_t token, uint16_t offset,
                                   uint16_t max_length)
{
    printf("app_cb_gapc_name_get\n");
}

__STATIC void app_cb_gapc_appearance_get(uint8_t conidx, uint32_t metainfo, uint16_t token)
{
//    gapc_le_get_appearance_cfm(conidx, token, GAP_ERR_NO_ERROR, app_get_appearance());
}
#endif //(BLE_HOST_PRESENT)

__STATIC void app_cb_cli_hash_info(uint8_t conidx, uint32_t dest_id, uint16_t handle, const uint8_t* p_hash)
{
    // printf("app_cb_cli_hash_info: handle 0x%x, p_hash 0x%x\n", handle, p_hash);
    uint8_t hash[16];
    memcpy(hash, p_hash, 16);

    // printf("app_cb_cli_hash_info 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n", 
    //             hash[0], hash[1], hash[2], hash[3], hash[4], hash[5], hash[6], hash[7], hash[8],
    //              hash[9], hash[10], hash[11], hash[12], hash[13], hash[14], hash[15]);
    memcpy(app_con_info[conidx].service_hash, p_hash, 16);

}
/// Callback executed when connection bond data is updated.
bool send_ltk = 0;
__STATIC void app_bond_data_updated(uint8_t conidx, uint32_t dest_id, const gapc_bond_data_updated_t* p_data)
{
    printf("app_bond_data_updated: 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x, size 0x%x, conidx 0x%x\n", 
            p_data->local_sign_counter, p_data->peer_sign_counter, p_data->gatt_start_hdl, 
            p_data->gatt_end_hdl, p_data->svc_chg_hdl, p_data->cli_info, p_data->cli_feat, 
            p_data->srv_feat, sizeof(gapc_bond_data_updated_t), conidx);
    
    memcpy(&app_con_bond_data[conidx].bond_data, p_data, sizeof(gapc_bond_data_updated_t));
}

// Callback that handle connection information
__STATIC const gapc_connection_info_cb_t app_connection_info_cb_itf =
{
    .disconnected         = app_cb_gapc_disconnected,
    .bond_data_updated    = app_bond_data_updated,
    .auth_payload_timeout = NULL,
    .no_more_att_bearer   = NULL,
    #if (!(0))
    .cli_hash_info        = app_cb_cli_hash_info,
    #endif //(!(0))
    #if (BLE_HOST_PRESENT)
    .name_get             = app_cb_gapc_name_get,
    .appearance_get       = app_cb_gapc_appearance_get,
    #else //(BLE_HOST_PRESENT)
    .name_get             = NULL,
    .appearance_get       = NULL,
    #endif //(BLE_HOST_PRESENT)
    .slave_pref_param_get = NULL,
    .name_set             = NULL,
    .appearance_set       = NULL,
};

__STATIC void app_le_le_connection_cb_gapc_cmp_evt(uint8_t conidx, uint32_t metainfo, uint16_t status)
{
    APP_DBG("app_le_le_connection_cb_gapc_cmp_evt: conidx 0x%x, status 0x%x\n", conidx, status);
}

// Link authentication information
__STATIC void app_con_cb_gapc_sec_auth_info(uint8_t conidx, uint32_t metainfo, uint8_t sec_lvl, bool encrypted,
                                            uint8_t key_size)
{
    uint16_t status;
    APP_DBG("AUTH INFO %d, %d-%d\n", conidx, sec_lvl, encrypted);

#ifdef LE_CIS_SINK
    if (Get_cis_role() == CIS_SINK)
    {
        le_cis_sink_restore_bond_data(conidx);
    }
#endif

    if (encrypted)
    {
        status = gapc_set_ping_to(conidx, GAPC_METAINFO_LE_PING,
                                                      AUTHENTICATED_PAYLOAD_TIMEOUT_10MS,
                                                      app_le_le_connection_cb_gapc_cmp_evt);
        if (status)
        {
            APP_DBG("gapc_set_ping_to err 0x%x\n", status);
        }
    }
}

// Handle numeric comparison request
__STATIC void app_con_cb_gapc_sec_numeric_compare_req(uint8_t conidx, uint32_t metainfo, uint32_t value)
{
    APP_DBG("PAIRING USER VAL CFM %d %d\n", conidx, value);
    // Automatically confirm
    gapc_pairing_numeric_compare_rsp(conidx, true);
}

// Informed that pairing succeed
__STATIC void app_con_cb_gapc_sec_pairing_succeed(uint8_t conidx, uint32_t metainfo, uint8_t pairing_level,
                                                  bool enc_key_present, uint8_t key_type)
{
    uint8_t device_lid = (app_con_info[0].conidx == conidx) ? 0 : 1;
    APP_DBG("PAIRING SUCCEED %d, device_id 0x%x, pairing_level 0x%x, 0x%x\n", conidx, device_lid, pairing_level, enc_key_present);
    
    SETB(app_con_info[device_lid].status_bf, APP_CON_STATUS_PAIRED, 1);

    app_event_paired_t pair_param;
    pair_param.conidx = conidx;
    pair_param.pairing_level = pairing_level;
    app_con_bond_data[conidx].pairing_level = pairing_level;
    app_con_bond_data[conidx].enc_key_present = enc_key_present;
    
    app_state_transition_local(APP_PAIRING_SUCCEED, GAP_ERR_NO_ERROR, &pair_param);
}

// Informed that pairing failed
__STATIC void app_con_cb_gapc_sec_pairing_failed(uint8_t conidx, uint32_t metainfo, uint16_t reason)
{
    APP_DBG("app_con_cb_gapc_sec_pairing_failed: PAIRING FAILED %d, 0x%04X\n", conidx, reason);
}

// Handle Security request (IOCAP / Pincode / Passkey)
__STATIC void app_con_cb_gapc_sec_info_req(uint8_t conidx, uint32_t metainfo, uint8_t exp_info)
{
    APP_DBG("app_con_cb_gapc_sec_info_req: 0x%x, 0x%x\n", exp_info, metainfo);
    uint16_t status;
    switch(exp_info)
    {
        case GAPC_INFO_TK_DISPLAYED:
        {
            gap_sec_key_t sec_key = { .key = {0x40, 0xE2, 0x1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
            status = gapc_le_pairing_provide_tk(conidx,  true, &sec_key);
            if (status)
            {
                printf("gapc_le_pairing_provide_tk: err 0x%x\n", status);
            }
        } break;

        case GAPC_INFO_BT_PASSKEY:
        {
            APP_DBG("PAIRING PASSKEY GET %d\n", conidx);
            // Force 123456 PASS KEY
            status = gapc_pairing_provide_passkey(conidx, true, 123456);
            if (status)
            {
                printf("GAPC_INFO_BT_PASSKEY: err 0x%x\n", status);
            }
        } break;

        case GAPC_INFO_PASSKEY_DISPLAYED:
        {
            APP_DBG("GAPC_INFO_PASSKEY_DISPLAYED GET %d\n", conidx);
            // Force 123456 as PASS KEY
            status = gapc_pairing_provide_passkey(conidx, true, 123456);
            if (status)
            {
                printf("GAPC_INFO_PASSKEY_DISPLAYED: err 0x%x\n", status);
            }
        } break;

        case GAPC_INFO_PASSKEY_ENTERED:
        {
            APP_DBG("GAPC_INFO_PASSKEY_ENTERED GET %d\n", conidx);
            status = gapc_pairing_provide_passkey(conidx, true, 123456);
            if (status)
            {
                printf("GAPC_INFO_PASSKEY_ENTERED: err 0x%x\n", status);
            }
        }  break;

        case GAPC_INFO_IRK:
        {
            uint8_t cnt;

            for (cnt = 0; cnt < GAP_KEY_LEN; cnt++)
            {
                // Generate a new IRK
//                p_env->bond_data.local_irk.key[cnt] = (uint8_t)co_rand_word();
            }

            printf("    IRK generated\n");
            // status = gapc_le_pairing_provide_irk(conidx, &(p_env->bond_data.local_irk));
            // if (status)
            // {
            //     printf("GAPC_INFO_IRK: err 0x%x\n", status);
            // }
        } break;
        default: { /* Ignore */ } break;
    }
}

#if (BLE_HOST_PRESENT)
extern uint8_t le_linkkey[16];
__STATIC void app_con_cb_gapc_sec_le_encrypt_req(uint8_t conidx, uint32_t metainfo, uint16_t ediv, const gap_le_random_nb_t* p_rand)
{
    uint16_t status = 0;
    uint8_t device_lid = (app_con_info[0].conidx == conidx) ? 0 : 1;
    APP_DBG("ENCRYPT REQUEST conidx: %d, device_lid %d\n", conidx, device_lid);

    if (linkkey_2_ltk_flag)
    {
        memcpy(app_con_bond_data[device_lid].keys.ltk.key.key, le_linkkey, 16);
        app_con_bond_data[device_lid].keys.ltk.key_size = 16;
    }

    status = gapc_le_encrypt_req_reply(conidx, true, (const struct gapc_ltk*)&(app_con_bond_data[device_lid].keys.ltk.key),
                              app_con_bond_data[device_lid].keys.ltk.key_size);

    // gap_sec_key_t tmp_key;
    // memcpy(tmp_key.key, le_linkkey, 16);

    // for (int i = 0; i < 16; i++)
    // {
    //     printf("0x%x ", tmp_key.key[i]);
    // }
    // printf("\n");

    // status = gapc_le_encrypt_req_reply(conidx, true, &tmp_key, 16);    
    if (status)
    {
        APP_DBG("gapc_le_encrypt_req_reply error 0x%x\n", status);
    }
}
#endif //(BLE_HOST_PRESENT)

/****************************************************************************
 *  获取ble list中对应index的设备信息
 ****************************************************************************/
uint32_t GetBle1of8RecInfo(uint8_t device_lid/*from 0*/, uint8_t *Data/*size must be no less than 23B*/)
{
    // if(RecIdx >= GetCurTotaBtRecNum()) return 0; //not found
    uint8_t cnt = 0;
    memcpy(Data, &app_con_bond_data[device_lid].bond_data, sizeof(gapc_bond_data_updated_t));
    cnt += sizeof(gapc_bond_data_updated_t);

    memcpy(Data + cnt, &(app_con_bond_data[device_lid].keys.ltk), sizeof(gapc_ltk_t));
    cnt += sizeof(gapc_ltk_t);

    memcpy(Data + cnt, &(app_con_bond_data[device_lid].keys.irk), sizeof(gapc_irk_t));
    cnt += sizeof(gapc_irk_t);   

    memcpy(Data + cnt, &(app_con_bond_data[device_lid].addr), sizeof(gap_bdaddr_t));    
    cnt += sizeof(gap_bdaddr_t);  

    memcpy(Data + cnt, &(app_con_bond_data[device_lid].pairing_level), 1);
    cnt++; 

    memcpy(Data + cnt, &(app_con_bond_data[device_lid].enc_key_present), 1);    
    cnt++; 

    uint8_t is_paired = app_is_paired(device_lid);
    memcpy(Data + cnt, &is_paired, 1);
    cnt++; 

    memcpy(Data + cnt, app_con_info[device_lid].service_hash, 16);    
    cnt += 16; 

    memcpy(Data + cnt, &(app_con_info[device_lid].role), 1);
    cnt++; 
#ifdef LE_CIS_SINK
    app_bond_data_t ascs_bond_data;
    uint8_t nb_ases_sink, nb_ases_src;
    memset(&ascs_bond_data, 0, sizeof(ascs_bond_data));
    memcpy(Data + cnt, &ascs_bond_data.svc_info, sizeof(prf_svc_t));
    cnt += sizeof(prf_svc_t);

    memcpy(Data + cnt, &nb_ases_sink, 1); 
    cnt++;

    memcpy(Data + cnt, &nb_ases_src, 1); 
    cnt++;

    uint16_t size = 4 * sizeof(bap_uc_cli_ascs_char_t);
    bap_uc_cli_ascs_char_t char_info[4];
    memcpy(Data + cnt, &char_info[0], size); 
    cnt += size;
#else
#ifdef LE_CIS_SRC
    app_bond_data_t* p_ascs_bond_data = get_ascs_src_bond_data();
    memcpy(Data + cnt, &p_ascs_bond_data->svc_info, sizeof(prf_svc_t));
    cnt += sizeof(prf_svc_t);

    memcpy(Data + cnt, &p_ascs_bond_data->p_ascs->nb_ases_sink, 1); 
    cnt++;

    memcpy(Data + cnt, &p_ascs_bond_data->p_ascs->nb_ases_src, 1); 
    cnt++;

    uint8_t nb_chars = p_ascs_bond_data->p_ascs->nb_ases_sink + p_ascs_bond_data->p_ascs->nb_ases_src + 1;
    uint16_t size = nb_chars * sizeof(bap_uc_cli_ascs_char_t);
    memcpy(Data + cnt, &p_ascs_bond_data->p_ascs->char_info[0], size); 
    cnt += size;
#endif
#endif

    app_sink_bond_data_t* p_ascs_sink_bond_data = get_ascs_sink_bond_data();

    memcpy(Data + cnt, &p_ascs_sink_bond_data->cli_cfg_bf, 1); 
    cnt++;

    memcpy(Data + cnt, &p_ascs_sink_bond_data->ase_cli_cfg_bf, 2); 
    cnt += 2;    

    printf("GetBle1of8RecInfo: cnt %d\n", cnt);

    return 1;
}

// Callback used to indicate pairing keys that must be stored on a non-volatile memory.
__STATIC void app_con_cb_gapc_sec_key_received(uint8_t conidx, uint32_t metainfo, const gapc_pairing_keys_t* p_keys)
{
    if (conidx == 0)
    {
        p_env->device_num = 1;
    }
    else if (conidx ==  1)
    {
        p_env->device_num = 2;        
    }

    uint8_t device_lid = (app_con_info[0].conidx == conidx) ? 0 : 1;
    APP_DBG("STORE KEY RECEIVED AS BOND DATA: conidx %d, device_lid 0x%x\n", conidx, device_lid);
    app_con_bond_data[device_lid].keys = *p_keys;
    app_con_bond_data[device_lid].addr = app_con_info[device_lid].addr;
    // TODO check if IRK present, if yes store the identity present with the IRK

    APP_DBG("BOND Address : 0x%02X:%02X:%02X:%02X:%02X:%02X Valid_Key 0x%02X  Pairing_lvl = 0x%02X\n",
             app_con_bond_data[device_lid].addr.addr[5], app_con_bond_data[device_lid].addr.addr[4],
             app_con_bond_data[device_lid].addr.addr[3], app_con_bond_data[device_lid].addr.addr[2],
             app_con_bond_data[device_lid].addr.addr[1], app_con_bond_data[device_lid].addr.addr[0],
             app_con_bond_data[device_lid].keys.valid_key_bf, app_con_bond_data[device_lid].keys.pairing_lvl);

    if(p_keys->valid_key_bf & GAP_KDIST_IDKEY)
    {
        const uint8_t* p_key = &(p_keys->irk.key.key[0]);
        const uint8_t* p_addr = &(p_keys->irk.identity.addr[0]);
        APP_UNUSED(p_key);
        APP_UNUSED(p_addr);
        APP_DBG("IRK (%s 0x%02X%02X%02X%02X%02X%02X) 0x%02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X\n",
                p_keys->irk.identity.addr_type == GAP_ADDR_PUBLIC ? "Public" : "Private",
                p_addr[5],p_addr[4],p_addr[3],p_addr[2],p_addr[1],p_addr[0],
                p_key[15], p_key[14], p_key[13], p_key[12], p_key[11], p_key[10], p_key[9], p_key[8], p_key[7],
                p_key[6], p_key[5], p_key[4], p_key[3], p_key[2], p_key[1], p_key[0]);

        app_con_bond_data[device_lid].keys.irk.identity = p_keys->irk.identity;
    }

    if(p_keys->valid_key_bf & GAP_KDIST_ENCKEY)
    {
        const uint8_t* p_key = &(p_keys->ltk.key.key[0]);
        APP_UNUSED(p_key);
        APP_DBG("LTK (%d) 0x%02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X: 0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x:0x%x\n", p_keys->ltk.key_size,
                p_key[15], p_key[14], p_key[13], p_key[12], p_key[11], p_key[10], p_key[9], p_key[8], p_key[7],
                p_key[6], p_key[5], p_key[4], p_key[3], p_key[2], p_key[1], p_key[0], 
                p_keys->ltk.ediv, p_keys->ltk.key_size, p_keys->ltk.randnb.nb[0], p_keys->ltk.randnb.nb[1], p_keys->ltk.randnb.nb[2], 
                p_keys->ltk.randnb.nb[3], p_keys->ltk.randnb.nb[4], p_keys->ltk.randnb.nb[5]);

        // //baikal add
        // uint8_t tmp_key[16];
        // for (int i = 0; i < 16; i++)
        // {
        //     tmp_key[i] = p_keys->ltk.key.key[15-i];
        // }
        if (1)//((app_con_info[conidx].local_auth & GAP_AUTH_CT2) && (app_con_info[conidx].remote_auth & GAP_AUTH_CT2))
        {
        	aes_h7(salt, p_keys->ltk.key.key, (aes_func_result_cb) security_test_h7_result_cb, (uint32_t)H7KeyInvert);
        	// aes_h7(salt, tmp_key, (aes_func_result_cb) security_test_h7_result_cb, (uint32_t)H7KeyInvert);
        }
        else
        {
        	aes_h6(p_keys->ltk.key.key, tmp1, security_test_h6_result_cb, H6KeyInvert);    
        }
    }

}

#if (BLE_HOST_PRESENT)
__STATIC void app_con_cb_gapc_sec_ltk_req(uint8_t conidx, uint32_t metainfo, uint8_t key_size)
{
    uint8_t device_lid = (app_con_info[0].conidx == conidx) ? 0 : 1;
    APP_DBG("LTK REQUEST %d\n", conidx, device_lid);

    if (!CHECKB(app_con_info[device_lid].status_bf, APP_CON_STATUS_PAIRED))
    {
        uint8_t cnt;

        app_con_bond_data[device_lid].keys.ltk.key_size = GAP_KEY_LEN;
        app_con_bond_data[device_lid].keys.ltk.ediv = (uint16_t)co_rand_word();

        for (cnt = 0; cnt < RAND_NB_LEN; cnt++)
        {
            app_con_bond_data[device_lid].keys.ltk.key.key[cnt] = (uint8_t)co_rand_word();
            app_con_bond_data[device_lid].keys.ltk.randnb.nb[cnt] = (uint8_t)co_rand_word();
        }

        for (cnt = RAND_NB_LEN; cnt < GAP_KEY_LEN; cnt++)
        {
            app_con_bond_data[device_lid].keys.ltk.key.key[cnt] = (uint8_t)co_rand_word();
        }
    }

    gapc_le_pairing_provide_ltk(conidx, &(app_con_bond_data[device_lid].keys.ltk));
}
#endif //(BLE_HOST_PRESENT)

__STATIC void app_con_cb_gapc_sec_auth_req(uint8_t conidx, uint32_t metainfo, uint8_t auth_level)
{
    APP_DBG("app_con_cb_gapc_sec_auth_req: auth_level 0x%x\n", auth_level);
}

__STATIC void app_con_cb_gapc_sec_key_pressed(uint8_t conidx, uint32_t metainfo, uint8_t notification_type)
{
        APP_DBG("app_con_cb_gapc_sec_key_pressed: conidx 0x%x\n", conidx);
}

__STATIC void app_con_cb_gapc_sec_pairing_req(uint8_t conidx, uint32_t metainfo, uint8_t auth_level)
{
    APP_DBG("PAIRING REQ %d, auth_level 0x%x\n", conidx, auth_level);
    app_con_info[conidx].remote_auth = auth_level;

    #if(BLE_HOST_PRESENT)
    uint16_t status;
#ifdef LINK_WITH_PHONE
    uint8_t sec_req_level = GAP_NO_SEC;//GAP_SEC1_SEC_CON_PAIR_ENC;
#else
    uint8_t sec_req_level = GAP_SEC1_NOAUTH_PAIR_ENC;
#endif
    gapc_pairing_t pairing_info =
    {
#if defined(LINK_WITH_PHONE)
        .iocap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT,//GAP_IO_CAP_NO_INPUT_NO_OUTPUT,//GAP_IO_CAP_DISPLAY_ONLY,
#else
        .iocap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT,
#endif
        .oob = GAP_OOB_AUTH_DATA_NOT_PRESENT,

#ifdef LINK_WITH_PHONE
        .auth = GAP_AUTH_REQ_SEC_CON_BOND | GAP_AUTH_CT2, //GAP_AUTH_REQ_MITM_BOND,
#else
        .auth = GAP_AUTH_BOND,
#endif
        .key_size = GAP_KEY_LEN,
        .ikey_dist = GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY | GAP_KDIST_LINKKEY,
        .rkey_dist = GAP_KDIST_ENCKEY | GAP_KDIST_IDKEY | GAP_KDIST_LINKKEY,
    };
    
    app_con_info[conidx].local_auth = pairing_info.auth;

    gapm_le_configure_security_level(sec_req_level);
    status = gapc_le_pairing_accept(conidx, true, &pairing_info, 0);
    if(status != GAP_ERR_NO_ERROR)
    {
        APP_DBG("app_con_cb_gapc_sec_pairing_req: failed 0x%x\n", status);
    }
    #endif // (BLE_HOST_PRESENT)
}

__STATIC void app_le_connection_cb_gapc_param_update_req(uint8_t conidx, uint32_t metainfo,
                                                            const gapc_le_con_param_nego_t* p_param)
{
    APP_DBG("app_le_connection_cb_gapc_param_update_req: conidx 0x%x, intv %d %d\n", 
                conidx, p_param->interval_min, p_param->interval_max);
    uint16_t status = gapc_le_update_params_cfm(conidx, true, 5, 10);
    if (status)
    {
        APP_DBG("gapc_le_update_params_cfm err 0x%x\n", status);
    }
}

__STATIC void app_le_connection_cb_gapc_param_updated(uint8_t conidx, uint32_t metainfo,
                                                         const gapc_le_con_param_t* p_param)
{
    APP_DBG("Connection parameters updated (conidx = %d, interval = %d x 1.25ms)\n", conidx, p_param->interval);
}

__STATIC void app_le_configuration_cb_hw_error(uint32_t metainfo, uint8_t code)
{
    printf("Hardware Error (0x%02X)\n", code);
}

// Callback that handle securitys
__STATIC const gapc_security_cb_t app_security_cb_itf =
{
    .auth_info           = app_con_cb_gapc_sec_auth_info,
    .pairing_succeed     = app_con_cb_gapc_sec_pairing_succeed,
    .pairing_failed      = app_con_cb_gapc_sec_pairing_failed,
    .info_req            = app_con_cb_gapc_sec_info_req,
    .numeric_compare_req = app_con_cb_gapc_sec_numeric_compare_req,
    .key_received        = app_con_cb_gapc_sec_key_received,
    .pairing_req         = app_con_cb_gapc_sec_pairing_req,
    #if (BLE_HOST_PRESENT)
    .le_encrypt_req      = app_con_cb_gapc_sec_le_encrypt_req,
    #endif //(BLE_HOST_PRESENT)
    .auth_req            = app_con_cb_gapc_sec_auth_req,
    .key_pressed         = app_con_cb_gapc_sec_key_pressed,
    #if (BLE_HOST_PRESENT)
    .ltk_req             = app_con_cb_gapc_sec_ltk_req,
    #endif // (BLE_HOST_PRESENT)
    .repeated_attempt    = NULL,
};
///! [APP_BT_SEC_CB]

#if (BLE_HOST_PRESENT)
__STATIC const gapc_le_config_cb_t app_le_config_cb_itf =
{
    .param_update_req = app_le_connection_cb_gapc_param_update_req,
    .param_updated = app_le_connection_cb_gapc_param_updated,
    .packet_size_updated = NULL,
    .phy_updated = NULL,
};
#endif //(BLE_HOST_PRESENT)

// List of callbacks that will handle GAP events
gapm_callbacks_t app_gap_cbs =
{
    .p_con_req_cbs   = &app_connection_req_cb_itf,
    .p_sec_cbs       = &app_security_cb_itf,
    .p_info_cbs      = &app_connection_info_cb_itf,
    .p_le_config_cbs = &app_le_config_cb_itf,
    .p_bt_config_cbs = NULL,
    .p_gapm_cbs = app_le_configuration_cb_hw_error,
};


// Disconnect procedure function handler
__STATIC void app_disconnect_cmp(uint8_t conidx, uint32_t metainfo, uint16_t status)
{
    APP_DBG("DISCONNECT CMD COMPLETED %d, 0x%04X\n", conidx, status);
}

#if (BLE_HOST_PRESENT)
// Encryption procedure function handler
__STATIC void app_encrypt_cmp(uint8_t conidx, uint32_t metainfo, uint16_t status)
{
    APP_DBG("ENCRYPTION CMD COMPLETED %d, 0x%04X\n", conidx, status);
    if (status)
    {
        return;
    }
    app_state_transition_local(APP_ENCRYPTION_STARTED, GAP_ERR_NO_ERROR, &conidx);
}
#endif //(BLE_HOST_PRESENT)

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************************
 */


// Terminate connection
uint16_t app_disconnect(void)
{
    uint8_t g_conidx = 0;
    return gapc_disconnect(g_conidx, 0, LL_ERR_REMOTE_USER_TERM_CON, app_disconnect_cmp);
}

#if (BLE_HOST_PRESENT)

uint16_t app_le_start_pairing(uint8_t conidx)
{
    APP_DBG("app_le_start_pairing\n");
    uint8_t sec_req_level;
#ifdef LINK_WITH_PHONE
    sec_req_level = GAP_NO_SEC;//GAP_SEC1_SEC_CON_PAIR_ENC;
#else
    sec_req_level = GAP_SEC1_NOAUTH_PAIR_ENC;
#endif

    gapc_pairing_t pairing_info =
    {
#if defined(LINK_WITH_PHONE)
        .iocap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT,//GAP_IO_CAP_KB_DISPLAY,//GAP_IO_CAP_DISPLAY_ONLY,
#else
        .iocap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT,
#endif
        .oob = GAP_OOB_AUTH_DATA_NOT_PRESENT,

#ifdef LINK_WITH_PHONE
        .auth = GAP_AUTH_REQ_SEC_CON_BOND, //GAP_AUTH_REQ_MITM_BOND,
#else
        .auth = GAP_AUTH_BOND,
#endif
        .key_size = GAP_KEY_LEN,
        .ikey_dist = GAP_KDIST_ENCKEY,
        .rkey_dist = 0,
    };

    if (get_role_from_conidx(conidx) == ROLE_MASTER)
    {
#if defined(LE_UDSC) && !defined(LE_CIS_SRC)
    sec_req_level = GAP_SEC1_NOAUTH_PAIR_ENC;
    pairing_info.iocap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
    pairing_info.auth = GAP_AUTH_BOND;
#endif
    }
    else if (get_role_from_conidx(conidx) == 0xff)
    {
        APP_DBG("err get role from the releated conidx!!!!\n");
    }

    gapm_le_configure_security_level(sec_req_level);
    return gapc_le_bond(conidx, &pairing_info, 0);
}

uint16_t app_le_encrypt(uint8_t conidx)
{
    uint8_t device_lid = (app_con_info[0].conidx == conidx) ? 0 : 1;
    APP_DBG("app_le_encrypt: device_lid: 0x%x\n", device_lid);
    gapc_ltk_t ltk_info;
    memcpy(ltk_info.key.key, app_con_bond_data[device_lid].keys.ltk.key.key, 16);
    ltk_info.key_size = 16;
    for (int i = 0; i < 16; i++)
    {
        printf("0x%x ", ltk_info.key.key[i]);
    }
    printf("\n");
    return (gapc_le_encrypt(conidx, 0, &ltk_info, app_encrypt_cmp));
}

bool app_is_paired(uint8_t conidx)
{
    return (GETB(app_con_info[0].status_bf, APP_CON_STATUS_PAIRED));
}
#endif //(BLE_HOST_PRESENT)

// Get GAP callback interface
gapm_callbacks_t* app_gap_cbs_itf_get(void)
{
    return &app_gap_cbs;
}

void app_set_con_le_config_cb_itf(const gapc_le_config_cb_t* p_le_config_cb_itf)
{
    app_gap_cbs.p_le_config_cbs = p_le_config_cb_itf;
}

void app_set_con_security_cb_itf(const gapc_security_cb_t* p_sec_cb_itf)
{
    app_gap_cbs.p_sec_cbs = p_sec_cb_itf;
}

void app_set_con_info_itf(const gapc_connection_info_cb_t* p_info_cb_itf)
{
    app_gap_cbs.p_info_cbs = p_info_cb_itf;
}

/// \example{lineno} app_connection.c
#endif
