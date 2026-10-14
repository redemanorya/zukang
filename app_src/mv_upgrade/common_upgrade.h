/**
 **************************************************************************************
 * @file    common_upgrade.h
 * @brief   通用固件升级模块头文件（支持USB/SPP/BLE等任意数据通道）
 *
 * @author  Jarvis
 * @version V1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __COMMON_UPGRADE_H__
#define __COMMON_UPGRADE_H__

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "bt_config.h"
#include "fw_upgrade.h"
#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#ifdef MVAPP_UPDATA_EN

//=============================================================================
// 协议命令定义
//=============================================================================
#define UP_CMD_HANDSHAKE        0x80    // 握手命令
#define UP_CMD_HANDSHAKE_ACK    0x81    // 握手应答
#define UP_CMD_HANDSHAKE_NAK    0x82    // 握手否定应答

#define UP_CMD_WRITE_DATA       0x30    // 写数据命令
#define UP_CMD_WRITE_DATA_ACK   0x31    // 写数据应答
#define UP_CMD_WRITE_DATA_NAK   0x32    // 写数据错误应答

#define UP_CMD_END              0x40    // 结束命令
#define UP_CMD_END_ACK          0x41    // 结束应答

#define UP_CMD_QUERY_STATUS     0x50    // 查询状态
#define UP_CMD_QUERY_STATUS_ACK 0x51    // 查询状态应答

//=============================================================================
// 升级状态枚举
//=============================================================================
typedef enum
{
    UP_STATE_IDLE = 0,          // 空闲状态
    UP_STATE_READY,             // 就绪状态（握手完成）
    UP_STATE_BUSY,              // 忙状态（正在接收数据）
    UP_STATE_VERIFYING,         // 校验中
    UP_STATE_SUCCESS,           // 成功状态
    UP_STATE_FAILED             // 失败状态
} UPGRADE_STATE;

//=============================================================================
// 升级错误码
//=============================================================================
typedef enum
{
    UP_ERR_NONE = 0,            // 无错误
    UP_ERR_CRC,                 // CRC校验错误
    UP_ERR_FLASH_ERASE,         // Flash擦除错误
    UP_ERR_FLASH_WRITE,         // Flash写入错误
    UP_ERR_FLASH_VERIFY,        // Flash验证错误
    UP_ERR_INVALID_SIZE,        // 无效的文件大小
    UP_ERR_TIMEOUT,             // 超时错误
    UP_ERR_MEMORY,              // 内存错误
    UP_ERR_NOT_READY,           // 未就绪
    UP_ERR_BUSY                 // 忙
} UPGRADE_ERROR;

//=============================================================================
// 升级信息结构体
//=============================================================================
typedef struct
{
    uint32_t total_size;        // 文件总大小
    uint32_t received_size;     // 已接收大小
    uint32_t flash_offset;      // Flash起始地址
    uint16_t crc;               // CRC校验值
    uint8_t status;             // 当前状态
    uint8_t error_code;         // 错误码
} UPGRADE_INFO;

//=============================================================================
// 回调函数类型
//=============================================================================
typedef void (*upgrade_status_callback_t)(uint8_t status, uint32_t progress, void *user_data);

//=============================================================================
// 数据发送接口（需要外部实现）
//=============================================================================
/**
 * @brief 数据发送函数指针类型
 * @param data 数据缓冲区
 * @param len 数据长度
 * @return 0:成功, 负值:失败
 */
typedef int (*upgrade_send_func_t)(const uint8_t *data, uint16_t len);

/**
 * @brief 设置数据发送接口
 * @param send_func 发送函数指针
 */
void upgrade_set_send_func(upgrade_send_func_t send_func);

//=============================================================================
// API函数声明
//=============================================================================

/**
 * @brief 初始化升级模块
 * @param config 配置参数（NULL则使用默认配置）
 * @return 0:成功, 负值:失败
 */
int upgrade_module_init(const upgrade_config_t *config);

/**
 * @brief 反初始化升级模块
 */
void upgrade_module_deinit(void);

/**
 * @brief 升级异常时恢复到初始化状态（保留send_func/callback）
 */
void upgrade_reset_to_idle(void);

/**
 * @brief 处理接收到的数据（由USB/SPP/BLE等数据通道调用）
 * @param buf 数据缓冲区
 * @param len 数据长度
 */
void upgrade_process_data(const uint8_t *buf, uint16_t len);

/**
 * @brief 发送应答包
 * @param cmd 命令字
 * @param data 应答数据
 * @param len 数据长度
 */
void upgrade_send_ack(uint8_t cmd, const uint8_t *data, uint16_t len);

/**
 * @brief 获取当前升级状态
 * @return 当前状态
 */
UPGRADE_STATE upgrade_get_state(void);

/**
 * @brief 获取升级信息
 * @return 升级信息结构体指针
 */
UPGRADE_INFO* upgrade_get_info(void);

/* 注: upgrade_get_last_error / upgrade_cancel / upgrade_get_progress / upgrade_is_busy
   由 fw_upgrade.h 提供声明，本模块不再重复声明 */

/**
 * @brief 设置状态回调
 * @param callback 回调函数
 * @param user_data 用户数据
 */
void upgrade_set_callback(upgrade_status_callback_t callback, void *user_data);

#endif /* MVAPP_UPDATA_EN */

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__COMMON_UPGRADE_H__