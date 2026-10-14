
/**
 * @file ad_parser.h
 * @author Cole (cole@mvsilicon.com)
 * @brief 
 * @version 0.1
 * @date 2024-03-15
 * 
 * @copyright Copyright (c) 2024
 * 
 */
/**
 * @title AD Data Parser 
 *
 * AD Data (Advertisements and EIR) Parser
 */

#ifndef AD_PARSER_H
#define AD_PARSER_H


#include <stdint.h>

#if defined __cplusplus
extern "C" {
#endif

/* API_START */

typedef struct ad_context {
     const uint8_t * data;
     uint8_t   offset;
     uint8_t   length;
} ad_context_t;

// Advertising or Scan Response data iterator
void ad_iterator_init(ad_context_t *context, uint8_t ad_len, const uint8_t * ad_data);
bool ad_iterator_has_more(const ad_context_t * context);
void ad_iterator_next(ad_context_t * context);

// Access functions
uint8_t         ad_iterator_get_data_type(const ad_context_t * context);
uint8_t         ad_iterator_get_data_len(const ad_context_t * context);
const uint8_t * ad_iterator_get_data(const ad_context_t * context);

// convenience function on complete advertisements
bool ad_data_contains_uuid16(uint8_t ad_len, const uint8_t * ad_data, uint16_t uuid16);
bool ad_data_contains_uuid128(uint8_t ad_len, const uint8_t * ad_data, const uint8_t * uuid128);
/**
 * @brief Get field data_length from event GAP_EVENT_ADVERTISING_REPORT
 * @param event packet
 * @return data_length
 * @note: btstack_type J
 */
static inline uint8_t gap_event_advertising_report_get_data_length(const uint8_t * event){
    return event[11];
}
/**
 * @brief Get field data from event GAP_EVENT_ADVERTISING_REPORT
 * @param event packet
 * @return data
 * @note: btstack_type V
 */
static inline uint8_t * gap_event_advertising_report_get_data( uint8_t * event){
    return &event[12];
}

/* API_END */

#if defined __cplusplus
}
#endif
#endif // AD_PARSER_H
