
#include "gapm.h"
#include "utils_cb_gap.h"
#include "le_audio_config.h"
#include "debug.h"
#include "le_udss.h"
#include "rtos_api.h"
/*
 * TYPES DEFINITION
 ****************************************************************************************
 */
uint8_t conn_update = 0;
#ifdef LE_UDSS

#define CONFIGURATION                                                                   \
    {                                                                                   \
        .role = GAP_ROLE_LE_PERIPHERAL | GAP_ROLE_LE_BROADCASTER | GAP_ROLE_LE_OBSERVER | GAP_ROLE_LE_CENTRAL, \
        .pairing_mode = GAPM_PAIRING_MODE_ALL,                                          \
        .privacy_cfg = 0,                                                               \
        .renew_dur = 1500,                                                              \
        .private_identity.addr = {0, 0, 0, 0, 0, 0},                                    \
        .irk.key = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},                    \
        .gap_start_hdl = 0,                                                             \
        .gatt_start_hdl = 0,                                                            \
        .att_cfg = 0,                                                                   \
        .sugg_max_tx_octets = GAP_LE_MAX_OCTETS,                                        \
        .sugg_max_tx_time = GAP_LE_MAX_TIME,                                            \
        .tx_pref_phy = GAP_PHY_ANY,                                                     \
        .rx_pref_phy = GAP_PHY_ANY,                                                     \
        .tx_path_comp = 0,                                                              \
        .rx_path_comp = 0,                                                              \
        .class_of_device = 0x200408,                                                    \
        .dflt_link_policy = 0,                                                          \
    }

__STATIC void app_udss_configuration_state(uint16_t event, uint16_t status, const void* p_params);                                           
/*
 * FUNCTIONS
 ****************************************************************************************
 */
uint16_t app_udss_start_adv(char* p_device_name, uint8_t name_len, uint16_t add_adv_data_len, const uint8_t* p_add_adv_data)
{    
    APP_DBG("app_udss_start_adv\n");     
    bap_bc_adv_param_t adv_param = {
        .adv_intv_min_slot = 192,
        .adv_intv_max_slot = 192,
        .ch_map = ADV_ALL_CHNLS_EN,
        .phy_prim = GAPM_PHY_TYPE_LE_1M,
        .phy_second = GAPM_PHY_TYPE_LE_2M,
        .adv_sid = 1,
        .own_addr_type = /*GAPM_STATIC_ADDR,*/GAPM_GEN_RSLV_ADDR,
    };

    uint16_t size = (add_adv_data_len + 2)+ (name_len + 2);
    uint16_t idx = 0;
    uint8_t* adv_data = (uint8_t *)osPortMalloc((add_adv_data_len + 2)+ (name_len + 2));
    if (adv_data == NULL)
    {
        printf("malloc err\n");
        return;
    }

    adv_data[idx++] = name_len + 1;
    adv_data[idx++] = GAP_AD_TYPE_COMPLETE_NAME;
    strcpy(&adv_data[idx], p_device_name);
    idx +=  name_len;

    adv_data[idx++] = add_adv_data_len + 1;
    adv_data[idx++] = GAP_AD_TYPE_MANU_SPECIFIC_DATA;
    memcpy(&adv_data[idx], p_add_adv_data, add_adv_data_len);
    idx += add_adv_data_len;

    if (size == idx)
    {
        printf("size matched\n");
    }
    else
    {
        printf("size err %d, %d\n", size, idx);
    }
    
    uint16_t status = udss_adv_start(0, &adv_param, size, adv_data);
    if (status)
    {
        APP_DBG("app_udss_start_adv: 000 failed status 0x%x\n", status);     
    }

    if (adv_data)
    {
        osPortFree(adv_data);
    }

    return status;
}

uint16_t app_udss_adv_stop(void)
{    
    uint16_t status = udss_adv_stop();
    if (status)
    {
        APP_DBG("app_udss_adv_stop: failed status 0x%x\n", status);     
    }
    return status;
}

void app_udss_configuration_cb_gapm_cmp(uint32_t event, uint16_t status)
{
    app_udss_configuration_state(event, status, NULL);
}

__STATIC void app_udss_configuration_state(uint16_t event, uint16_t status, const void* p_params)
{
    bool is_finished = false;
    APP_DBG("app_udss_configuration_state event: 0x%x\n", event);

	switch (event)
	{
		case EVENT_CONFIGURATION_DONE:
			gapm_set_name(EVENT_CONFIGURATION_NAME_SET, strlen(BT_NAME), (const uint8_t*)BT_NAME,
											app_udss_configuration_cb_gapm_cmp);
		 break;

		case EVENT_CONFIGURATION_NAME_SET:
			gapm_get_controller_bdaddr(EVENT_CONFIGURE_READ_CONTROLLER_ADDRESS,
										app_udss_configuration_cb_gapm_cmp);
			break;

		case EVENT_CONFIGURE_READ_CONTROLLER_ADDRESS:
			is_finished = true;
			break;

		default:  break;
	}

    if ( is_finished )
    {
#ifdef LE_UDSS
        uint8_t vendor_data[4] = {1, 2, 3, 4};
        status = app_udss_start_adv(BLE_NAME, sizeof(BLE_NAME), sizeof(vendor_data), vendor_data);
        if (status)
        {
            APP_DBG("app_udss_configuration_state: app_udss_start_adv error 0x%x\n", status);
        }
#ifdef LE_UDSC
        app_le_udsc_start_scan();
#endif
#endif
    }
}

uint16_t app_start_le_adv()
{
    uint8_t vendor_data[4] = {1, 2, 3, 4};
    uint16_t status = app_udss_start_adv(BLE_NAME, sizeof(BLE_NAME), sizeof(vendor_data), vendor_data);
    if (status)
    {
        APP_DBG("app_udss_configuration_state: app_udss_start_adv error 0x%x\n", status);
    }
    return status;
}

void app_send_value_to_phone(uint8_t val_id, uint32_t value)
{
    uint8_t conidx = get_conidx_from_role(ROLE_SLAVE);
    if (conidx == 0xff)
    {
        APP_DBG("get invaid condix\n");
        return;
    }
    le_udss_send_notify(conidx, value);
}

static void app_le_udss_cb_read_req(uint8_t conidx, uint8_t val_id)
{
    APP_DBG("app_le_udss_cb_read_req: condix %d\n", conidx);
    if (!conn_update)
    {
        app_update_conn_para(conidx);
    }
}

static void app_le_udss_cb_write_req(uint8_t conidx, uint8_t val_id, uint32_t value)
{
    APP_DBG("app_le_udss_cb_write_req, value 0x%x 0x%x\n", value, val_id);
    app_get_value_from_phone(conidx, val_id, value);
}

void app_get_value_from_phone(uint8_t conidx, uint8_t val_id, uint32_t value)
{
    APP_DBG("app_get_value_from_phone, value 0x%x 0x%x\n", value, val_id);
}

__STATIC const app_le_udss_cb_t app_le_cbs_udss = {
    .cb_read_req = app_le_udss_cb_read_req,
    .cb_write_req = app_le_udss_cb_write_req,
};

__STATIC void app_udss_configure(app_le_udss_cb_t* p_cbs_udss)
{
    app_le_link_configure();
    le_udss_configure(p_cbs_udss);
}

void app_udss_init(void)
{
	app_udss_configure(&app_le_cbs_udss);
    // app_vendor_s_init();
}
#endif


