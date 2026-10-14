/**
 ****************************************************************************************
 *
 * @file le_udss.h
 *
 * @brief 
 *
 * Copyright (C) RivieraWaves 2009-2024
 * Release Identifier: c73db0ce
 *
 ****************************************************************************************
 */

#ifndef LE_UDSS_H_
#define LE_UDSS_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
typedef struct app_le_udss_cb
{
    void (*cb_read_req)(uint8_t conidx, uint8_t uds_val_id);
    void (*cb_write_req)(uint8_t conidx, uint8_t val_id, uint32_t* p_value);
} app_le_udss_cb_t;
 
uint16_t udss_adv_start(uint16_t timeout_s, const bap_bc_adv_param_t* p_adv_param,
                            uint16_t add_adv_data_len, const uint8_t* p_add_adv_data);

uint16_t udss_adv_stop(void);
uint16_t le_udss_configure(app_le_udss_cb_t* p_cbs_udss);
#endif // LE_UDSS_H_
