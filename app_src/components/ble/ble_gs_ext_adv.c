/**
 * @file ble_gs_ext_adv.c
 * @author Cole (cole@mvsilicon.com)
 * @brief BLE扩展广播和周期性广播测试用例实现
 * @version 0.1
 * @date 2025-12-25
 * 
 * @copyright Copyright (c) 2025
 */
#include "bt_config.h"
#if (BLE_SUPPORT)

#include "ble_gs_ext_adv.h"
#include "ble_gs_service.h"
#include <string.h>
#include <stdio.h>

static uint8_t gs_ext_adv_buf[200];
static uint8_t gs_ext_scan_rsp_buf[100];
static uint16_t gs_ext_adv_len = 0;
static uint16_t gs_ext_scan_rsp_len = 0;


static uint8_t gs_periodic_ext_buf[64];      // 扩展广播数据 (用于发现)
static uint8_t gs_periodic_data_buf[200];    // 周期性广播数据 (主要数据)
static uint16_t gs_periodic_ext_len = 0;
static uint16_t gs_periodic_data_len = 0;

static uint32_t sensor_counter = 0;

static void adv_status_callback(bool started, uint16_t status)
{
    if (started)
    {
        printf("[ADV_TEST] 广播已启动!\n");
    }
    else
    {
        printf("[ADV_TEST] 广播已停止, status=0x%04X\n", status);
    }
}

static uint16_t build_ext_adv_data(void)
{
    uint16_t off = 0;
    
    // 1. 设备名称 (Complete Local Name)
    const char* name = "MVS_ExtAdv_Test";
    uint8_t name_len = strlen(name);
    gs_ext_adv_buf[off++] = name_len + 1;
    gs_ext_adv_buf[off++] = 0x09;  // AD Type: Complete Local Name
    memcpy(&gs_ext_adv_buf[off], name, name_len);
    off += name_len;
    
    // 2. 服务UUID (16-bit)
    uint16_t svc_uuid = 0xAB00;
    gs_ext_adv_buf[off++] = 3;
    gs_ext_adv_buf[off++] = 0x03;  // AD Type: Complete List of 16-bit UUIDs
    gs_ext_adv_buf[off++] = svc_uuid & 0xFF;
    gs_ext_adv_buf[off++] = (svc_uuid >> 8) & 0xFF;
    
    // 3. TX Power Level
    gs_ext_adv_buf[off++] = 2;
    gs_ext_adv_buf[off++] = 0x0A;  // AD Type: TX Power Level
    gs_ext_adv_buf[off++] = 10;    // 10 dBm
    
    // 4. 厂商数据 (Manufacturer Specific Data) - 演示放入更多数据
    uint8_t mfr_data[] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
        0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
    };
    gs_ext_adv_buf[off++] = sizeof(mfr_data) + 3;
    gs_ext_adv_buf[off++] = 0xFF;  // AD Type: Manufacturer Specific Data
    gs_ext_adv_buf[off++] = 0x01;  // Company ID Low
    gs_ext_adv_buf[off++] = 0x02;  // Company ID High
    memcpy(&gs_ext_adv_buf[off], mfr_data, sizeof(mfr_data));
    off += sizeof(mfr_data);
    
    printf("[EXT_ADV] 广播数据长度: %d 字节 (Legacy最大31字节)\n", off);
    
    return off;
}

/**
 * @brief 构建扩展扫描响应数据
 */
static uint16_t build_ext_scan_rsp_data(void)
{
    uint16_t off = 0;
    
    // Appearance
    gs_ext_scan_rsp_buf[off++] = 3;
    gs_ext_scan_rsp_buf[off++] = 0x19;  // AD Type: Appearance
    gs_ext_scan_rsp_buf[off++] = 0x00;
    gs_ext_scan_rsp_buf[off++] = 0x00;
    
    // 更多厂商数据
    uint8_t extra_data[] = "Extended Scan Response Data - More Info Here!";
    gs_ext_scan_rsp_buf[off++] = sizeof(extra_data) + 2;
    gs_ext_scan_rsp_buf[off++] = 0xFF;
    gs_ext_scan_rsp_buf[off++] = 0x01;
    gs_ext_scan_rsp_buf[off++] = 0x02;
    memcpy(&gs_ext_scan_rsp_buf[off], extra_data, sizeof(extra_data) - 1);
    off += sizeof(extra_data) - 1;
    
    printf("[EXT_ADV] 扫描响应长度: %d 字节\n", off);
    
    return off;
}

static uint16_t build_periodic_ext_adv_data(void)
{
    uint16_t off = 0;
    
    // 设备名称 (用于被扫描端发现)
    const char* name = "MVS_Periodic";
    uint8_t name_len = strlen(name);
    gs_periodic_ext_buf[off++] = name_len + 1;
    gs_periodic_ext_buf[off++] = 0x09;  // Complete Name
    memcpy(&gs_periodic_ext_buf[off], name, name_len);
    off += name_len;
    
    return off;
}


static uint16_t build_periodic_data(void)
{
    uint16_t off = 0;
    
    // 自定义数据格式: 模拟传感器数据广播
    // Header: 类型 + 长度
    gs_periodic_data_buf[off++] = 0xAA;  // 数据类型标识
    gs_periodic_data_buf[off++] = 0;     // 数据长度 (稍后填充)
    uint16_t len_pos = off - 1;
    
    gs_periodic_data_buf[off++] = 0x01;
    uint32_t timestamp = sensor_counter++;
    gs_periodic_data_buf[off++] = (timestamp >> 0) & 0xFF;
    gs_periodic_data_buf[off++] = (timestamp >> 8) & 0xFF;
    gs_periodic_data_buf[off++] = (timestamp >> 16) & 0xFF;
    gs_periodic_data_buf[off++] = (timestamp >> 24) & 0xFF;
    
    int16_t temperature = 2350;  // 23.50°C
    uint16_t humidity = 6500;    // 65.00%
    uint32_t pressure = 101325;  // 101.325 kPa
    
    // 温度
    gs_periodic_data_buf[off++] = (temperature >> 0) & 0xFF;
    gs_periodic_data_buf[off++] = (temperature >> 8) & 0xFF;
    
    // 湿度
    gs_periodic_data_buf[off++] = (humidity >> 0) & 0xFF;
    gs_periodic_data_buf[off++] = (humidity >> 8) & 0xFF;
    
    // 气压
    gs_periodic_data_buf[off++] = (pressure >> 0) & 0xFF;
    gs_periodic_data_buf[off++] = (pressure >> 8) & 0xFF;
    gs_periodic_data_buf[off++] = (pressure >> 16) & 0xFF;
    gs_periodic_data_buf[off++] = (pressure >> 24) & 0xFF;
    
    // 额外数据
    const char* extra = "Periodic ADV can carry more data than legacy!";
    memcpy(&gs_periodic_data_buf[off], extra, strlen(extra));
    off += strlen(extra);
    
    // 更新长度字段
    gs_periodic_data_buf[len_pos] = off - 2;
    
    return off;
}

void ble_gs_test_ext_adv_start(void)
{
    
    ble_gs_set_adv_callback(adv_status_callback);
    
    gs_ext_adv_len = build_ext_adv_data();
    
    ble_gs_ext_adv_data_t adv_data = {
        .p_adv_data = gs_ext_adv_buf,
        .adv_data_len = gs_ext_adv_len,
        .p_scan_rsp = NULL,         
        .scan_rsp_len = 0
    };
    
    ble_gs_err_t ret = ble_gs_ext_adv_set_data(&adv_data);
    printf("[EXT_ADV] 设置数据: %s\n", (ret == BLE_GS_OK) ? "成功" : "失败");
    
    ble_gs_ext_adv_param_t param = {
        .adv_type = BLE_GS_EXT_ADV_CONN_NOSCAN,  // 可连接，不可扫描
        .adv_intv_min = 160,    // 100ms
        .adv_intv_max = 240,    // 150ms
        .ch_map = 0x07,
        .max_tx_pwr = 10,
        .primary_phy = BLE_GS_PHY_1M,
        .secondary_phy = BLE_GS_PHY_2M,
        .adv_sid = 0
    };
    
    printf("[EXT_ADV] 模式: 可连接/不可扫描 (Extended Only)\n");
    printf("[EXT_ADV] 参数: primary_phy=1M, secondary_phy=2M\n");
    printf("[EXT_ADV] 间隔: %d~%d (%.1f~%.1fms)\n", 
           param.adv_intv_min, param.adv_intv_max,
           param.adv_intv_min * 0.625, param.adv_intv_max * 0.625);
    
    // 5. 启动扩展广播
    ret = ble_gs_ext_adv_start(&param);
}

void ble_gs_test_ext_adv_scannable(void)
{
    
    ble_gs_set_adv_callback(adv_status_callback);
    
    // 构建广播数据和扫描响应数据
    gs_ext_adv_len = build_ext_adv_data();
    gs_ext_scan_rsp_len = build_ext_scan_rsp_data();
    
    ble_gs_ext_adv_data_t adv_data = {
        .p_adv_data = gs_ext_adv_buf,
        .adv_data_len = gs_ext_adv_len,
        .p_scan_rsp = gs_ext_scan_rsp_buf,
        .scan_rsp_len = gs_ext_scan_rsp_len
    };
    
    ble_gs_err_t ret = ble_gs_ext_adv_set_data(&adv_data);

    ble_gs_ext_adv_param_t param = {
        .adv_type = BLE_GS_EXT_ADV_CONN_SCAN,   // 可连接 + 可扫描
        .adv_intv_min = 160,
        .adv_intv_max = 240,
        .ch_map = 0x07,
        .max_tx_pwr = 10,
        .primary_phy = BLE_GS_PHY_1M,
        .secondary_phy = BLE_GS_PHY_1M,  // 可扫描模式通常用1M
        .adv_sid = 0
    };
    ret = ble_gs_ext_adv_start(&param);
}

void ble_gs_test_ext_adv_2m(void)
{
    ble_gs_set_adv_callback(adv_status_callback);
    
    gs_ext_adv_len = build_ext_adv_data();
    
    ble_gs_ext_adv_data_t adv_data = {
        .p_adv_data = gs_ext_adv_buf,
        .adv_data_len = gs_ext_adv_len,
        .p_scan_rsp = NULL,
        .scan_rsp_len = 0
    };
    ble_gs_ext_adv_set_data(&adv_data);
    
    ble_gs_ext_adv_param_t param = {
        .adv_type = BLE_GS_EXT_ADV_NONCONN_NOSCAN, 
        .adv_intv_min = 160,
        .adv_intv_max = 320,
        .ch_map = 0x07,
        .max_tx_pwr = 10,
        .primary_phy = BLE_GS_PHY_1M,     
        .secondary_phy = BLE_GS_PHY_CODED, 
        .adv_sid = 1
    };
    
    ble_gs_err_t ret = ble_gs_ext_adv_start(&param);
    printf("[EXT_ADV] 启动: %s\n", (ret == BLE_GS_OK) ? "成功" : "失败");
}

void ble_gs_test_ext_adv_stop(void)
{
    printf("[EXT_ADV] 停止广播...\n");
    ble_gs_ext_adv_stop();
}

void ble_gs_test_adv_compare(void)
{

}

void ble_gs_test_periodic_adv_start(void)
{
    
    // 1. 设置广播状态回调
    ble_gs_set_adv_callback(adv_status_callback);
    
    // 2. 构建广播数据
    gs_periodic_ext_len = build_periodic_ext_adv_data();
    gs_periodic_data_len = build_periodic_data();
    
    printf("[PERIODIC] 扩展广播数据: %d 字节 (用于发现)\n", gs_periodic_ext_len);
    printf("[PERIODIC] 周期性数据: %d 字节 (传感器数据)\n", gs_periodic_data_len);
    
    // 3. 设置周期性广播数据
    ble_gs_periodic_adv_data_t adv_data = {
        .p_ext_adv_data = gs_periodic_ext_buf,
        .ext_adv_data_len = gs_periodic_ext_len,
        .p_periodic_data = gs_periodic_data_buf,
        .periodic_data_len = gs_periodic_data_len
    };
    
    ble_gs_err_t ret = ble_gs_periodic_adv_set_data(&adv_data);
    printf("[PERIODIC] 设置数据: %s\n", (ret == BLE_GS_OK) ? "成功" : "失败");
    
    // 4. 配置周期性广播参数
    ble_gs_periodic_adv_param_t param = {
        .adv_intv_min = 160,        // 100ms
        .adv_intv_max = 240,        // 150ms
        .period_intv_min = 80,      // 100ms (80 * 1.25ms)
        .period_intv_max = 120,     // 150ms
        .ch_map = 0x07,
        .max_tx_pwr = 10,
        .primary_phy = BLE_GS_PHY_1M,
        .secondary_phy = BLE_GS_PHY_2M,
        .adv_sid = 0
    };
    
    printf("[PERIODIC] 周期间隔: %.1f~%.1fms\n", 
           param.period_intv_min * 1.25, param.period_intv_max * 1.25);
    ret = ble_gs_periodic_adv_start(&param);
}

void ble_gs_test_periodic_adv_stop(void)
{
    printf("[PERIODIC] 停止周期性广播...\n");
    ble_gs_periodic_adv_stop();
}

void ble_gs_test_periodic_update_data(void)
{
    gs_periodic_data_len = build_periodic_data();
    
    ble_gs_periodic_adv_data_t adv_data = {
        .p_ext_adv_data = NULL,
        .ext_adv_data_len = 0,
        .p_periodic_data = gs_periodic_data_buf,
        .periodic_data_len = gs_periodic_data_len
    };
    
    ble_gs_periodic_adv_set_data(&adv_data);
    printf("[PERIODIC] 数据已更新, counter=%lu\n", (unsigned long)sensor_counter);
}

void ble_gs_test_all_adv_modes(void)
{
    ble_gs_test_ext_adv_start();
}

#endif /* BLE_SUPPORT */
