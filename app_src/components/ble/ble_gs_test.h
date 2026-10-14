#ifndef _BLE_GS_TEST_H_
#define _BLE_GS_TEST_H_

#include "bt_config.h"
#if (BLE_SUPPORT)

#include <stdint.h>
#include <stdbool.h>

// 测试模块开关
#define BLE_GS_TEST_ENABLE  1

#if BLE_GS_TEST_ENABLE

/**
 * @brief 测试统计结构
 */
typedef struct {
    uint32_t tx_total;      // 总发送次数
    uint32_t tx_ok;         // 成功次数
    uint32_t tx_fail;       // 失败次数
    uint32_t tx_busy;       // BUSY重试次数
    uint32_t tx_bytes;      // 成功发送字节数
    uint32_t start_tick;    // 开始时间(tick)
    uint32_t end_tick;      // 结束时间(tick)
    bool running;           // 测试是否运行中
} ble_gs_test_stats_t;

void ble_gs_test_init(void);
void ble_gs_test_optimize_conn(void);
void ble_gs_test_on_tx_done(void);
void ble_gs_test_start(uint16_t interval_ms, uint16_t pkt_size);
void ble_gs_test_stop(void);
void ble_gs_test_loop(void);
void ble_gs_test_burst(uint16_t count, uint16_t pkt_size);
int ble_gs_test_notify_once(uint16_t len);
const ble_gs_test_stats_t* ble_gs_test_get_stats(void);
void ble_gs_test_print_stats(void);
void ble_gs_test_reset_stats(void);

bool ble_gs_test_is_running(void);

#endif // BLE_GS_TEST_ENABLE

#endif /* BLE_SUPPORT */
#endif // _BLE_GS_TEST_H_
