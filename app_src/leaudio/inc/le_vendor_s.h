/**
 ****************************************************************************************
 *
 * @file le_vendor_s.h
 *
 * @brief 
 *
 *
 ****************************************************************************************
 */

#ifndef LE_VENDOR_S_H_
#define LE_VENDOR_S_H_


typedef struct app_le_vendor_s_cb
{
     void (*cb_read_req)(uint8_t conidx, uint32_t token, uint8_t val_id, uint16_t offset, uint16_t max_len);
    void (*cb_write_req)(uint8_t conidx, uint8_t val_id, uint32_t* p_value);
    void (*cb_att_val_rcv) (uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t hdl, uint16_t offset,
                               co_buf_t* p_data);
    void (*cb_att_read_data) (uint8_t conidx, uint8_t user_lid, uint16_t token, uint16_t hdl, uint16_t offset,
                              uint16_t max_length);
} app_le_vendor_s_cb_t;

uint16_t vendor_s_adv_start(uint16_t timeout_s, const bap_bc_adv_param_t* p_adv_param,
                              uint16_t add_adv_data_len, const uint8_t* p_add_adv_data);

uint16_t vendor_s_adv_stop(void);
uint16_t le_vendor_s_configure(app_le_vendor_s_cb_t* p_cbs_vendor_s);
#endif // LE_VENDOR_S_H_
