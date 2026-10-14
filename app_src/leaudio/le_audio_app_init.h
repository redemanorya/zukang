/**
 *@file le_audio_app_init.h
 * @author Cole (cole@mvsilicon.com)
 * @brief
 * @version 0.1
 * @date 2025-03-24
 *
 * @copyright Copyright (c) 2025
 *
 */
#ifndef LE_AUDIO_APP_INIT_H
#define LE_AUDIO_APP_INIT_H
#include "gapm.h"

typedef struct gapm_all_cfg {
    uint32_t metainfo;
    const gapm_config_t* p_cfg;
    gapm_callbacks_t* p_cbs;
    gapm_proc_cmp_cb cmp_cb;
} gapm_all_cfg_t;

extern gapm_callbacks_t app_gap_cbs;
extern gapm_callbacks_t tws_link_cbs;
extern gapm_callbacks_t ble_gs_gap_cbs;
extern const gapm_config_t ble_gs_configuration;
extern void udsc_configuration_cb_gapm_cmp(uint32_t event, uint16_t status);
extern void tws_configuration_cb_gapm_cmp(uint32_t event, uint16_t status);
extern void ble_gs_configuration_cb_gapm_cmp(uint32_t event, uint16_t status);
/**
 *@brief
 *
*/
extern void le_core_init(void);
/**
 * @brief USER DATA SERVICE SERVER
 *
 */
void app_udss_init(void);
/**
 * @brief
 *
 */
uint16_t le_bis_source_init(void);
/**
 * @brief
 *
 */
uint16_t le_bis_sink_init(void);
/**
 * @brief USER DATA SERVICE CLIENT
 *
 * @return uint16_t
 */
uint16_t app_le_udsc_init(void);
/**
 * @brief
 *
 * @return uint16_t
 */
uint16_t le_bis_assis_init(void);
/**
 * @brief ble stack initialization
 *
 */
uint16_t le_stack_init(void);

#endif/* LE_AUDIO_APP_INIT_H */
