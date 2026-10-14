#ifndef LE_PUBLIC_H
#define LE_PUBLIC_H


#include "gaf.h"
#include "bap.h"
#include "gapm.h"
#include "bap_bc.h"
/// BD address length
#define GAP_BD_ADDR_LEN                         (6)
/// Key length
#define GAP_KEY_LEN                             (16)
/// Length of Broadcast Id
#define BAP_BC_BROADCAST_ID_LEN                     (3)

#define BIS_SRC_DATA_PATH_ID                       (5)
#define BIS_SRC1_DATA_PATH_ID                      (6)
#define BIS_SINK_DATA_PATH_ID                      (7)
#define BIS_SINK1_DATA_PATH_ID                     (8)

#define CIS_SINK_DATA_PATH_ID_2                     3
#define CIS_SINK_DATA_PATH_ID                      2

/// Audio stream configuration settings structure
typedef struct bap_bc_cfg
{
    /// Presentation delay in microseconds
    uint16_t pres_delay_us;
    /// SDU interval in microseconds
    uint16_t sdu_intv_us;
    /// Sampling frequency
    uint8_t samp_freq;
    /// Framing
    uint8_t framing;
    /// Max SDU size
    uint16_t max_sdu;
    /// Number of retransmission (RTN)
    uint8_t rtn;
    /// Maximum transport latency in milliseconds
    uint8_t max_tlatency_ms;
} bap_bc_cfg_t;


void bis_src_send_data(void* buf,uint32_t len);



#endif //LE_PUBLIC_H
