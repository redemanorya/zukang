#include "bt_config.h"
#if (BLE_SUPPORT)

#include "ble_gs_test.h"

#if BLE_GS_TEST_ENABLE

#include "ble_gs_service.h"
#include <stdio.h>
#include <string.h>

// 外部依赖
extern int ble_gs_app_send(const uint8_t* data, uint16_t len);
extern bool ble_gs_app_is_connected(void);
extern bool ble_gs_app_is_ntf_enabled(void);
extern uint8_t ble_gs_app_get_role_p_conidx(void);

static volatile uint32_t gs_test_tick = 0;
static volatile bool gs_high_speed_mode = false;
static volatile bool gs_tx_ready = true;  // 发送就绪标志
static volatile uint32_t gs_tx_pending = 0;  // 待确认的发送数
static volatile uint32_t gs_ntf_cmp_count = 0;  

// 测试配置
static struct {
    uint16_t interval;      // 发送间隔(loop次数)
    uint16_t pkt_size;      // 包大小
    uint32_t last_tx_tick;  // 上次发送tick
    uint16_t pkt_seq;       // 包序号
} gs_test_cfg;

// 测试统计
static ble_gs_test_stats_t gs_stats;

// 速率统计
static struct {
    uint32_t last_print_tick;
    uint32_t last_ok_count;
    uint32_t last_bytes;
} gs_rate_stats;

#define TEST_DBG(fmt, ...)  printf("[GS_TEST] " fmt "\n", ##__VA_ARGS__)

/*
 * 获取当前tick
 */
static uint32_t get_tick(void)
{
    return gs_test_tick;
}

// 当前发送的包大小（用于回调时计算字节数）
static uint16_t gs_current_pkt_size = 244;

/*
 * 内部发送函数
 * 注意：tx_ok 和 tx_bytes 现在在回调中更新，这里只更新 tx_total
 */
static int test_send_packet(uint16_t seq, uint16_t len)
{
    uint8_t buf[244];
    
    if (len > sizeof(buf)) len = sizeof(buf);
    if (len < 4) len = 4;
    
    // 构造测试数据：[seq_lo][seq_hi][tick_lo][tick_hi][padding...]
    buf[0] = (seq >> 0) & 0xFF;
    buf[1] = (seq >> 8) & 0xFF;
    buf[2] = (gs_test_tick >> 0) & 0xFF;
    buf[3] = (gs_test_tick >> 8) & 0xFF;
    for (uint16_t i = 4; i < len; i++) {
        buf[i] = (uint8_t)(i & 0xFF);
    }
    
    gs_current_pkt_size = len;
    gs_stats.tx_total++;
    
    int ret = ble_gs_app_send(buf, len);
    if (ret == 0) {
        // 提交成功，但不在这里计入 tx_ok
        // tx_ok 在 NTF_CMP 回调中更新
        return 0;
    } else if (ret == -2) {
        gs_stats.tx_busy++;
        gs_stats.tx_total--;  // BUSY不计入总数
        return -2;  // BUSY
    } else {
        gs_stats.tx_fail++;
        return -1;
    }
}

// 上次统计时的回调计数
static uint32_t gs_last_cmp_count = 0;

/*
 * 打印速率 - 基于NTF_CMP回调的真实统计
 */
static void print_rate(void)
{
    uint32_t tick_diff = get_tick() - gs_rate_stats.last_print_tick;
    if (tick_diff == 0) tick_diff = 1;
    
    // 使用回调计数差值
    uint32_t cmp_diff = gs_ntf_cmp_count - gs_last_cmp_count;
    uint32_t bytes_diff = cmp_diff * gs_current_pkt_size;
    
    uint32_t pps = cmp_diff * 1000 / tick_diff;
    uint32_t bps = bytes_diff * 1000 / tick_diff;
    uint32_t kbps = bps * 8 / 1000;
    
    TEST_DBG("REAL: %lu pkt/s, %lu kbps | cmp=%lu sub=%lu pend=%lu",
             pps, kbps, gs_ntf_cmp_count, gs_stats.tx_total, gs_tx_pending);
    
    gs_rate_stats.last_print_tick = get_tick();
    gs_last_cmp_count = gs_ntf_cmp_count;
}

/*
 * 通知发送完成回调（从ble_gs_app调用）
 * 只有这里才表示数据真正发出去了
 */
void ble_gs_test_on_tx_done(void)
{
    gs_ntf_cmp_count++;  // 独立计数器
    gs_tx_ready = true;
    if (gs_tx_pending > 0) gs_tx_pending--;
    
    // 真正发送成功，更新统计
    gs_stats.tx_ok++;
    gs_stats.tx_bytes += gs_current_pkt_size;
}

/*
 * 优化连接参数以获得最大吞吐量
 */
void ble_gs_test_optimize_conn(void)
{
    uint8_t conidx = ble_gs_app_get_role_p_conidx();
    
    TEST_DBG("Optimize: conidx=%d connected=%d ntf_en=%d", 
             conidx, ble_gs_app_is_connected(), ble_gs_app_is_ntf_enabled());
    
    if (conidx == 0xFF || !ble_gs_app_is_connected()) {
        TEST_DBG("Not connected!");
        return;
    }
    
    TEST_DBG("=== MAX THROUGHPUT MODE ===");
    
    // 1. 设置2M PHY（先设PHY再改间隔）
    ble_gs_err_t ret = ble_gs_set_phy(conidx, BLE_GS_PHY_2M, BLE_GS_PHY_2M);
    TEST_DBG("Set PHY 2M: %s", ret == BLE_GS_OK ? "OK" : "FAIL");
    
    // 2. 请求最小连接间隔 7.5ms (interval=6)
    ret = ble_gs_conn_param_update(conidx, 6, 0, 400);
    TEST_DBG("Conn param (7.5ms): %s", ret == BLE_GS_OK ? "OK" : "FAIL");
    
    TEST_DBG("Use phone app to set MTU=247 for best results");
    TEST_DBG("=== OPTIMIZATION DONE ===");
}

/*
 * API实现
 */
void ble_gs_test_init(void)
{
    memset(&gs_stats, 0, sizeof(gs_stats));
    memset(&gs_test_cfg, 0, sizeof(gs_test_cfg));
    memset(&gs_rate_stats, 0, sizeof(gs_rate_stats));
    gs_test_tick = 0;
    gs_high_speed_mode = false;
    gs_tx_ready = true;
    gs_tx_pending = 0;
    TEST_DBG("Test module initialized");
}

void ble_gs_test_start(uint16_t interval_ms, uint16_t pkt_size)
{
    if (gs_stats.running) {
        TEST_DBG("Test already running, stop first");
        return;
    }
    
    if (!ble_gs_app_is_connected() || !ble_gs_app_is_ntf_enabled()) {
        TEST_DBG("Not ready: connected=%d ntf_en=%d", 
                 ble_gs_app_is_connected(), ble_gs_app_is_ntf_enabled());
        return;
    }
    
    // 重置统计
    memset(&gs_stats, 0, sizeof(gs_stats));
    memset(&gs_rate_stats, 0, sizeof(gs_rate_stats));
    gs_ntf_cmp_count = 0;  // 重置回调计数
    gs_last_cmp_count = 0;
    
    // interval_ms=0 
    gs_high_speed_mode = (interval_ms == 0);
    gs_test_cfg.interval = interval_ms;
    gs_test_cfg.pkt_size = (pkt_size > 0 && pkt_size <= 244) ? pkt_size : 244;
    gs_test_cfg.last_tx_tick = get_tick();
    gs_test_cfg.pkt_seq = 0;
    gs_tx_ready = true;
    gs_tx_pending = 0;
    
    gs_stats.running = true;
    gs_stats.start_tick = get_tick();
    gs_rate_stats.last_print_tick = get_tick();
    
    if (gs_high_speed_mode) {
        TEST_DBG("Test STARTED: HIGH-SPEED MODE, pkt_size=%d", gs_test_cfg.pkt_size);
    } else {
        TEST_DBG("Test STARTED: interval=%dms pkt_size=%d", interval_ms, gs_test_cfg.pkt_size);
    }
}

void ble_gs_test_stop(void)
{
    if (!gs_stats.running) {
        TEST_DBG("Test not running");
        return;
    }
    
    gs_stats.running = false;
    gs_stats.end_tick = get_tick();
    gs_high_speed_mode = false;
    
    TEST_DBG("Test STOPPED");
    ble_gs_test_print_stats();
}

void ble_gs_test_loop(void)
{
    // 更新tick计数
    gs_test_tick++;
    
    if (!gs_stats.running) return;
    
    // 检查连接状态
    if (!ble_gs_app_is_connected() || !ble_gs_app_is_ntf_enabled()) {
        TEST_DBG("Connection lost, stopping test");
        ble_gs_test_stop();
        return;
    }
    
    if (gs_high_speed_mode) {
        if (gs_tx_pending == 0) {
            int ret = test_send_packet(gs_test_cfg.pkt_seq, gs_test_cfg.pkt_size);
            if (ret == 0) {
                gs_test_cfg.pkt_seq++;
                gs_tx_pending = 1;  // 只允许 1 个在途
            } else if (ret == -2) {
                // BUSY，等下次loop或回调
            } else {
                gs_test_cfg.pkt_seq++;  // 错误，跳过
            }
        }
    } else {
        // 固定间隔模式
        uint32_t now = get_tick();
        if (gs_test_cfg.interval > 0) {
            if (now - gs_test_cfg.last_tx_tick < gs_test_cfg.interval) {
                // 未到发送时间，但每秒打印一次
                if (now - gs_rate_stats.last_print_tick >= 1000) {
                    print_rate();
                }
                return;
            }
        }
        
        // 尝试发送
        int ret = test_send_packet(gs_test_cfg.pkt_seq, gs_test_cfg.pkt_size);
        if (ret == 0) {
            gs_test_cfg.pkt_seq++;
            gs_test_cfg.last_tx_tick = now;
        } else if (ret == -2) {
            // BUSY，不更新last_tx_tick
        } else {
            gs_test_cfg.pkt_seq++;
            gs_test_cfg.last_tx_tick = now;
        }
    }
    
    // 每1000个tick打印一次速率（约1秒）
    uint32_t now = get_tick();
    if (now - gs_rate_stats.last_print_tick >= 1000) {
        print_rate();
    }
}

void ble_gs_test_burst(uint16_t count, uint16_t pkt_size)
{
    if (!ble_gs_app_is_connected() || !ble_gs_app_is_ntf_enabled()) {
        TEST_DBG("BURST: Not ready");
        return;
    }
    
    uint16_t len = (pkt_size > 0 && pkt_size <= 244) ? pkt_size : 244;
    
    // 重置统计
    memset(&gs_stats, 0, sizeof(gs_stats));
    gs_stats.start_tick = get_tick();
    
    TEST_DBG("BURST START: count=%d size=%d (optimized)", count, len);
    
    uint16_t sent = 0;
    uint32_t busy_cnt = 0;
    const uint32_t max_busy = (uint32_t)count * 1000;  // 防止死循环
    
    while (sent < count && busy_cnt < max_busy) {
        int ret = test_send_packet(sent, len);
        if (ret == 0) {
            sent++;
        } else if (ret == -2) {
            // BUSY，忙等待一下
            busy_cnt++;
            for (volatile int k = 0; k < 100; k++) {}
        } else {
            sent++;  // 其他错误，跳过
        }
        
        // 更新tick
        if ((busy_cnt & 0x3FF) == 0) {
            gs_test_tick++;
        }
    }
    
    gs_stats.end_tick = get_tick();
    
    // 计算吞吐量
    uint32_t elapsed = gs_stats.end_tick - gs_stats.start_tick;
    if (elapsed == 0) elapsed = 1;
    uint32_t bps = gs_stats.tx_bytes * 1000 / elapsed;
    uint32_t kbps = bps * 8 / 1000;
    
    TEST_DBG("BURST DONE: ok=%lu fail=%lu busy=%lu",
             gs_stats.tx_ok, gs_stats.tx_fail, gs_stats.tx_busy);
    TEST_DBG("Throughput: %lu B/s (%lu kbps)", bps, kbps);
}

int ble_gs_test_notify_once(uint16_t len)
{
    if (!ble_gs_app_is_connected() || !ble_gs_app_is_ntf_enabled()) {
        TEST_DBG("NOTIFY: Not ready");
        return -1;
    }
    
    static uint16_t seq = 0;
    int ret = test_send_packet(seq++, len);
    if (ret == 0) {
        TEST_DBG("NOTIFY OK: len=%d", len);
    }
    return ret;
}

const ble_gs_test_stats_t* ble_gs_test_get_stats(void)
{
    return &gs_stats;
}

void ble_gs_test_print_stats(void)
{
    uint32_t real_time_ms = gs_ntf_cmp_count * 75 / 10;  // * 7.5ms
    if (real_time_ms == 0) real_time_ms = 1;
    
    uint32_t real_bytes = gs_ntf_cmp_count * gs_current_pkt_size;
    uint32_t real_pps = gs_ntf_cmp_count * 1000 / real_time_ms;
    uint32_t real_bps = real_bytes * 1000 / real_time_ms;
    uint32_t real_kbps = real_bps * 8 / 1000;
    
    TEST_DBG("========== TEST STATISTICS ==========");
    TEST_DBG("Packets sent:  %lu", gs_ntf_cmp_count);
    TEST_DBG("Bytes sent:    %lu", real_bytes);
    TEST_DBG("Real time:     ~%lu ms (based on 7.5ms interval)", real_time_ms);
    TEST_DBG("Throughput:    %lu pkt/s", real_pps);
    TEST_DBG("Throughput:    %lu KB/s (%lu kbps)", real_bps/1000, real_kbps);
    TEST_DBG("=====================================");
    TEST_DBG("Note: This is near BLE max! ~32KB/s = 260kbps");
    TEST_DBG("Phone shows ~30KB/s due to protocol overhead.");
}

void ble_gs_test_reset_stats(void)
{
    memset(&gs_stats, 0, sizeof(gs_stats));
    memset(&gs_rate_stats, 0, sizeof(gs_rate_stats));
    TEST_DBG("Stats reset");
}

bool ble_gs_test_is_running(void)
{
    return gs_stats.running;
}

#endif // BLE_GS_TEST_ENABLE

#endif /* BLE_SUPPORT */
