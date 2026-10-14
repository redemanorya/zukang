/**
 * @file ble_gs_ext_adv.h
 * @author Cole (cole@mvsilicon.com)
 * @brief BLE扩展广播和周期性广播测试用例
 * @version 0.1
 * @date 2025-12-25
 * 
 * @copyright Copyright (c) 2025
 * 
 * 本文件包含三种广播模式的测试示例:
 *   1. Legacy Advertising     - 传统广播 (31字节)
 *   2. Extended Advertising   - 扩展广播 (254字节, BLE 5.0+)
 *   3. Periodic Advertising   - 周期性广播 (定时发送, BLE 5.0+)
 */
#ifndef _BLE_GS_EXT_ADV_H_
#define _BLE_GS_EXT_ADV_H_

#include "bt_config.h"
#if (BLE_SUPPORT)

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 ****************************************************************************************
 * 扩展广播测试 (Extended Advertising)
 * 
 * 特点:
 *   - 最大254字节广播数据 (Legacy仅31字节)
 *   - 支持多种PHY: 1M/2M/Coded
 *   - 可连接或不可连接
 *   - 需要BLE 5.0+设备
 ****************************************************************************************
 */

/**
 * @brief 启动扩展广播测试 (2M PHY，可连接)
 * 
 * 验证方法:
 *   1. 打开 nRF Connect APP
 *   2. 扫描找到 'MVS_ExtAdv_Test'
 *   3. 查看设备详情:
 *      - PHY: LE 2M (如果手机支持)
 *      - 厂商数据: 36+ 字节
 */
void ble_gs_test_ext_adv_start(void);

/**
 * @brief 扩展广播测试 - 可扫描模式 (带扫描响应)
 * 
 * 特点:
 *   - 可连接 + 可扫描
 *   - 支持扫描响应数据 (额外数据)
 *   - nRF Connect会显示更多信息
 */
void ble_gs_test_ext_adv_scannable(void);

/**
 * @brief 启动远距离扩展广播 (1M Primary + Coded Secondary)
 * 
 * 特点:
 *   - Primary PHY=1M: 普通手机可扫描
 *   - Secondary PHY=Coded: 远距离数据传输 (约4倍于1M PHY)
 * 
 * 注意: 需要在 rwip_config.h 中定义 CFG_CSSA 宏
 */
void ble_gs_test_ext_adv_2m(void);

/**
 * @brief 停止扩展广播
 */
void ble_gs_test_ext_adv_stop(void);

/**
 * @brief Legacy与Extended广播对比
 */
void ble_gs_test_adv_compare(void);

/*
 ****************************************************************************************
 * 周期性广播测试 (Periodic Advertising)
 * 
 * 特点:
 *   - 按固定时间间隔发送数据 (可配置 7.5ms~81.9s)
 *   - 扫描端可同步接收，不会漏收
 *   - 不可连接 (仅广播)
 *   - 适合传感器数据、信标、音频同步等场景
 *   - 需要BLE 5.0+设备且启用BLE_PER_ADV
 ****************************************************************************************
 */

/**
 * @brief 启动周期性广播测试
 * 
 * 验证方法:
 *   1. 打开 nRF Connect APP
 *   2. 扫描找到 'MVS_Periodic'
 *   3. 点击设备详情
 *   4. 找到 'Periodic Advertising' 标签
 *   5. 点击 'Sync' 同步
 *   6. 查看周期性数据 (模拟传感器数据)
 */
void ble_gs_test_periodic_adv_start(void);

/**
 * @brief 停止周期性广播
 */
void ble_gs_test_periodic_adv_stop(void);

/**
 * @brief 更新周期性广播数据
 * 
 * 模拟传感器数据更新，接收端会自动收到最新数据
 * 可定时调用此函数更新传感器数据
 */
void ble_gs_test_periodic_update_data(void);

/*
 ****************************************************************************************
 * 快速测试入口
 ****************************************************************************************
 */

/**
 * @brief 运行所有广播测试 (演示用)
 * 
 * 依次启动:
 *   1. 扩展广播 5秒
 *   2. 远距离广播 5秒
 *   3. 周期性广播 (持续)
 */
void ble_gs_test_all_adv_modes(void);

#ifdef __cplusplus
}
#endif

#endif /* BLE_SUPPORT */
#endif /* _BLE_GS_EXT_ADV_H_ */
