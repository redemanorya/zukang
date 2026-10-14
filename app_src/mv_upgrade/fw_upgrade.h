// fw_upgrade.h
#ifndef _FW_UPGRADE_H_
#define _FW_UPGRADE_H_

#include <stdint.h>
#include <stdbool.h>
#include "bt_config.h"
#ifdef __cplusplus
extern "C" {
#endif
#ifdef MVAPP_UPDATA_EN
/**
 * @brief 升级状态枚举
 */
typedef enum {
    UPGRADE_STATUS_IDLE = 0,          // 空闲
    UPGRADE_STATUS_START,              // 升级开始（收到头部信息）
    UPGRADE_STATUS_RECEIVING,          // 接收数据中
    UPGRADE_STATUS_COMPLETE,           // 接收完成
    UPGRADE_STATUS_VERIFYING,          // 校验中
    UPGRADE_STATUS_UPGRADING,          // 升级中（重启前）
    UPGRADE_STATUS_SUCCESS,            // 升级成功
    UPGRADE_STATUS_ERROR               // 错误
} upgrade_status_t;

/**
 * @brief 升级错误码
 */
typedef enum {
    UPGRADE_ERR_NONE = 0,
    UPGRADE_ERR_INVALID_PARAM,
    UPGRADE_ERR_MEM_ALLOC,
    UPGRADE_ERR_INVALID_FILE,
    UPGRADE_ERR_FILE_TOO_LARGE,
    UPGRADE_ERR_CRC_FAIL,
    UPGRADE_ERR_FLASH_ERASE,
    UPGRADE_ERR_FLASH_WRITE,
    UPGRADE_ERR_FLASH_VERIFY,
    UPGRADE_ERR_NOT_INIT,
    UPGRADE_ERR_BUSY,
    UPGRADE_ERR_TIMEOUT
} upgrade_error_t;

/**
 * @brief 升级回调函数类型
 * @param status 当前状态
 * @param progress 进度百分比(0-100)
 * @param user_data 用户数据
 */
typedef void (*upgrade_callback_t)(upgrade_status_t status, uint32_t progress, void *user_data);

/**
 * @brief 升级配置结构体
 */
typedef struct {
    uint32_t flash_offset;              // Flash起始偏移地址（默认0x100000）
    uint32_t max_file_size;             // 最大文件大小（0表示不限制）
    uint32_t write_buffer_size;         // 写入缓冲区大小（默认4096）
    uint32_t erase_block_size;          // 擦除块大小（默认65536）
    bool enable_crc_check;              // 是否启用CRC校验（默认true）
    bool enable_flash_verify;           // 是否启用Flash写入校验（默认true）
    bool enable_auto_reboot;            // 校验通过后是否自动重启（默认true）
    upgrade_callback_t callback;        // 状态回调函数
    void *user_data;                    // 用户数据
} upgrade_config_t;

//=============================================================================
// 核心接口
//=============================================================================

/**
 * @brief 初始化升级模块
 * @param config 升级配置（为NULL时使用默认配置）
 * @return 错误码
 */
upgrade_error_t upgrade_init(const upgrade_config_t *config);

/**
 * @brief 反初始化升级模块
 */
void upgrade_deinit(void);

/**
 * @brief 开始升级（收到文件大小时调用）
 * @param total_size 文件总大小
 * @return 错误码
 */
upgrade_error_t upgrade_start(uint32_t total_size);

/**
 * @brief 接收升级数据（传输层收到数据时调用）
 * @param data 数据指针
 * @param len 数据长度
 * @return 错误码
 */
upgrade_error_t upgrade_receive_data(const uint8_t *data, uint32_t len);

/**
 * @brief 完成升级（数据传输完成后调用）
 * @return 错误码
 */
upgrade_error_t upgrade_finish(void);

/**
 * @brief 取消当前升级
 */
void upgrade_cancel(void);

/**
 * @brief 获取当前状态
 */
upgrade_status_t upgrade_get_status(void);

/**
 * @brief 获取当前进度百分比
 */
uint32_t upgrade_get_progress(void);

/**
 * @brief 获取已接收字节数
 */
uint32_t upgrade_get_received_size(void);

/**
 * @brief 获取文件总大小
 */
uint32_t upgrade_get_total_size(void);

/**
 * @brief 获取最后错误码
 */
upgrade_error_t upgrade_get_last_error(void);

/**
 * @brief 检查是否正在升级中
 */
bool upgrade_is_busy(void);
#endif
#ifdef __cplusplus
}
#endif

#endif /* _FW_UPGRADE_H_ */