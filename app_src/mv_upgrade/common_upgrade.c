/**
 *******************************************************************************
 * @file    common_upgrade.c
 * @author  jarvis
 * @version V0.0.1
 * @date    2026/01/01
 * @brief   通用固件升级模块（支持USB/SPP/BLE等任意数据通道）
 *******************************************************************************
 */

#include "common_upgrade.h"
#include "type.h"
#include "debug.h"
#include "app_message.h"
#include <string.h>
#include "rtos_api.h"
#include "app_config.h"
#include "fw_upgrade.h"
#include "timeout.h"
#include "flash_table.h"
#ifdef MVAPP_UPDATA_EN
#define USB_SEND
//#define SPP_SEND


//=============================================================================
// 宏定义 - 协议格式
//=============================================================================
// 协议格式: [0xA5][0x5A][0x22][帧长度][命令][数据][CRC]
#define FRAME_HEADER0           0xA5
#define FRAME_HEADER1           0x5A
#define FRAME_CTRL_WORD         0x22

// 包格式偏移量
#define HEADER_SIZE             3       // 帧头长度
#define OFFSET_CTRL             2       // 控制字位置 buf[2]
#define OFFSET_LEN              3       // 帧长度位置 buf[3]
#define OFFSET_CMD              4       // 命令位置 buf[4]
#define OFFSET_DATA             5       // 数据起始位置 buf[5]

#define MAX_DATA_LEN            250     // 最大数据长度

//#define SPP_SEND
#define USB_SEND

//=============================================================================
// 类型定义
//=============================================================================
typedef struct {
    uint8_t len;            // 帧长度
    uint8_t cmd;            // 命令
    uint8_t cmd_data[255];  // 数据定义为协议最大的帧长
} UpgradeCmd;

//=============================================================================
// 静态变量
//=============================================================================
static bool s_is_upgrading = false;
static UPGRADE_STATE s_state = UP_STATE_IDLE;
static UPGRADE_ERROR s_last_error = UP_ERR_NONE;
static upgrade_send_func_t s_send_func = NULL;
static upgrade_status_callback_t s_callback = NULL;
static void *s_callback_user_data = NULL;
static UPGRADE_INFO s_upgrade_info = {0};
static TIMER s_upgrade_end_time_out;
static bool s_upgrade_end_time_flag = FALSE;
static TIMER s_upgrade_ing_time_out;
static bool s_upgrade_ing_time_flag = FALSE;

//=============================================================================
// 外部函数声明
//=============================================================================
extern void Union_Effect_Send(uint8_t *buf, uint32_t len);
extern bool SppSendData(uint8_t * data, uint16_t dataLen);


//=============================================================================
// 私有函数声明
//=============================================================================
static bool send_ack(uint8_t cmd, const uint8_t *data, uint16_t len);
static void on_upgrade_status(upgrade_status_t status, uint32_t progress, void *user_data);
static void update_state(UPGRADE_STATE state);
static void set_error(UPGRADE_ERROR error);

//=============================================================================
// 私有函数实现
//=============================================================================

/**
 * @brief 更新状态
 */
static void update_state(UPGRADE_STATE state)
{
    s_state = state;
    s_upgrade_info.status = (uint8_t)state;
}

/**
 * @brief 设置错误码
 */
static void set_error(UPGRADE_ERROR error)
{
    s_last_error = error;
    s_upgrade_info.error_code = (uint8_t)error;
}

/**
 * @brief 升级状态回调（来自底层升级模块）
 */
static void on_upgrade_status(upgrade_status_t status, uint32_t progress, void *user_data)
{
    (void)user_data;
    
    switch (status) {
        case UPGRADE_STATUS_SUCCESS:
            APP_DBG("[UPGRADE] Success, rebooting...\n");
            update_state(UP_STATE_SUCCESS);
            if (s_callback) {
                s_callback(UP_STATE_SUCCESS, 100, s_callback_user_data);
            }
            s_is_upgrading = false;
            break;
            
        case UPGRADE_STATUS_ERROR:
            APP_DBG("[UPGRADE] Error: %d\n", upgrade_get_last_error());
            update_state(UP_STATE_FAILED);
            set_error((UPGRADE_ERROR)upgrade_get_last_error());
            if (s_callback) {
                s_callback(UP_STATE_FAILED, upgrade_get_progress(), s_callback_user_data);
            }
            s_is_upgrading = false;
            break;
            
        case UPGRADE_STATUS_VERIFYING:
            APP_DBG("[UPGRADE] Verifying firmware...\n");
            update_state(UP_STATE_VERIFYING);
            if (s_callback) {
                s_callback(UP_STATE_VERIFYING, progress, s_callback_user_data);
            }
            break;
            
        case UPGRADE_STATUS_RECEIVING:
            s_upgrade_info.received_size = progress;
            if (s_callback) {
                s_callback(UP_STATE_BUSY, upgrade_get_progress(), s_callback_user_data);
            }
            break;
            
        default:
            break;
    }
}

/**
 * @brief 发送应答包
 * @param cmd 命令
 * @param data 应答数据
 * @param len 数据长度
 */
static bool send_ack(uint8_t cmd, const uint8_t *data, uint16_t len)
{
    uint8_t tx_buf[260];
    uint16_t i;
    uint8_t crc = 0;
    uint8_t frame_len;
    
    if (s_send_func == NULL) {
        APP_DBG("[UPGRADE] Send func not set!\n");
        return;
    }
    
    if (len > MAX_DATA_LEN) {
        APP_DBG("[UPGRADE] Ack data too long: %d\n", len);
        return;
    }
    
    // 帧长度 = 命令(1) + 数据(len) + CRC(1)
    frame_len = 1 + len + 1;
    
    // 构建应答包: 帧头(3) + 帧长度(1) + 命令(1) + 数据(len) + CRC(1)
    tx_buf[0] = FRAME_HEADER0;
    tx_buf[1] = FRAME_HEADER1;
    tx_buf[2] = FRAME_CTRL_WORD;
    tx_buf[3] = frame_len;
    tx_buf[4] = cmd;
    
    if (data != NULL && len > 0) {
        memcpy(&tx_buf[5], data, len);
    }
    
    // 计算CRC（从头到数据结束）
    uint16_t total_len = HEADER_SIZE + 1 + 1 + len;  // 头3 + 长度1 + 命令1 + 数据
    for (i = 0; i < total_len; i++) {
        crc += tx_buf[i];
    }
    tx_buf[total_len] = crc;

    // 通过设置的发送接口发送
    s_send_func(tx_buf, total_len + 1);
}

//=============================================================================
// 公共函数实现
//=============================================================================

/**
 * @brief 设置数据发送接口
 */
void upgrade_set_send_func(upgrade_send_func_t send_func)
{
    s_send_func = send_func;
    APP_DBG("[UPGRADE] Send func set: %p\n", send_func);
}


/**
 * @brief 初始化升级模块
 */
int upgrade_module_init(const upgrade_config_t *config)
{
    upgrade_config_t default_config;
    const upgrade_config_t *p_config = config;

    if (p_config == NULL) {
        // 使用默认配置
        memset(&default_config, 0, sizeof(default_config));
        default_config.flash_offset = FLASH_DEFAULT_CAPACTITY/2;
        default_config.max_file_size = 0;
        default_config.write_buffer_size = 4096;
        default_config.erase_block_size = 65536;
        default_config.enable_crc_check = true;
        default_config.enable_flash_verify = true;
        default_config.enable_auto_reboot = true;
        default_config.callback = NULL;
        default_config.user_data = NULL;
        p_config = &default_config;
    }

    if (upgrade_init(p_config) != UPGRADE_ERR_NONE) {
        APP_DBG("[UPGRADE] Init failed\n");
        set_error(UP_ERR_MEMORY);
        return -1;
    }

    // 设置回调
    upgrade_set_callback(on_upgrade_status, NULL);

    memset(&s_upgrade_info, 0, sizeof(s_upgrade_info));
    s_upgrade_info.flash_offset = p_config->flash_offset;

    s_is_upgrading = false;
    update_state(UP_STATE_IDLE);
    set_error(UP_ERR_NONE);
#ifdef SPP_SEND
    upgrade_set_send_func(SppSendData);
#else
    upgrade_set_send_func(Union_Effect_Send);
#endif

    APP_DBG("[UPGRADE] Init success, flash_offset:0x%08X\n", p_config->flash_offset);
    return 0;
}

/**
 * @brief 反初始化升级模块
 */
void upgrade_module_deinit(void)
{
    upgrade_deinit();
    s_is_upgrading = false;
    update_state(UP_STATE_IDLE);
    s_send_func = NULL;
    s_callback = NULL;
    APP_DBG("[UPGRADE] Deinit\n");
}

/**
 * @brief 升级异常时恢复到初始化状态
 *        保留 send_func 和 callback，仅重置升级流程状态
 */
void upgrade_reset_to_idle(void)
{
    // 停止 worker task + 取消升级流程（保留 init 时的全局配置）
    upgrade_cancel();

    s_is_upgrading = false;
    s_upgrade_end_time_flag = FALSE;
    s_upgrade_ing_time_flag = FALSE;
    memset(&s_upgrade_info, 0, sizeof(s_upgrade_info));
    update_state(UP_STATE_IDLE);
    set_error(UP_ERR_NONE);
    APP_DBG("[UPGRADE] Reset to idle (error recovery)\n");
}

/**
 * @brief 设置状态回调
 */
void upgrade_set_callback(upgrade_status_callback_t callback, void *user_data)
{
    s_callback = callback;
    s_callback_user_data = user_data;
}

/**
 * @brief 处理接收到的数据
 */
void upgrade_process_data(const uint8_t *buf, uint16_t len)
{
    UpgradeCmd cmd_info;
    uint8_t frame_len = 0;
    uint8_t cmd = 0;
    uint8_t data_len = 0;
    uint8_t crc = 0;
    uint16_t i;
    uint8_t status = 1;
    
    if (buf == NULL || len < OFFSET_DATA) {
        return;
    }
    
    // 按照协议格式解析
    // buf[0] = 0xA5
    // buf[1] = 0x5A
    // buf[2] = 0x22 (控制字)
    // buf[3] = 帧长度
    // buf[4] = 命令
    // buf[5] = 数据开始
    
    // 验证帧头
    if (buf[0] != FRAME_HEADER0 || buf[1] != FRAME_HEADER1 || buf[2] != FRAME_CTRL_WORD) {
        APP_DBG("[UPGRADE] Invalid header: %02X %02X %02X\n", buf[0], buf[1], buf[2]);
        return;
    }
    
    frame_len = buf[OFFSET_LEN];
    cmd = buf[OFFSET_CMD];
    data_len = frame_len - 1 - 1;  // 减去命令(1)和CRC(1)
    
    if (data_len > 0 && data_len <= sizeof(cmd_info.cmd_data)) {
        memcpy(cmd_info.cmd_data, &buf[OFFSET_DATA], data_len);
    }
    
    // CRC校验（从头到数据结束）
    uint16_t total_header = HEADER_SIZE + 1 + 1 + data_len;  // 头3 + 长度1 + 命令1 + 数据
    for (i = 0; i < total_header; i++) {
        crc += buf[i];
    }
    
    if (crc != buf[total_header]) {
        APP_DBG("[UPGRADE] CRC error: calc=%d, recv=%d\n", crc, buf[total_header]);
        return;
    }
    
    // 处理命令
    switch (cmd) {
        case UP_CMD_HANDSHAKE:
            // 握手命令，获取文件大小（数据4字节）
            if (data_len >= 4) {
                uint32_t total_size = cmd_info.cmd_data[0] |
                                      (cmd_info.cmd_data[1] << 8) |
                                      (cmd_info.cmd_data[2] << 16) |
                                      (cmd_info.cmd_data[3] << 24);
                
                APP_DBG("[UPGRADE] Handshake, total size: %u bytes\n", total_size);
                s_upgrade_info.total_size = total_size;
                s_upgrade_info.received_size = 0;
                
                // 开始升级
                if (upgrade_start(total_size) == UPGRADE_ERR_NONE) {
                    s_is_upgrading = true;
                    update_state(UP_STATE_READY);
                    send_ack(UP_CMD_HANDSHAKE_ACK, cmd_info.cmd_data, 4);
                    TimeOutSet(&s_upgrade_ing_time_out,2000);
                    s_upgrade_ing_time_flag = TRUE;
                } else {
                    status = 0;
                    update_state(UP_STATE_FAILED);
                    send_ack(UP_CMD_HANDSHAKE_NAK, &status, 1);
                }
            } else {
                APP_DBG("[UPGRADE] Handshake invalid len: %d\n", data_len);
            }
            break;
            
        case UP_CMD_WRITE_DATA:
            // 接收数据
            if (data_len > 0 && s_is_upgrading) {
                if (upgrade_receive_data((uint8_t*)cmd_info.cmd_data, data_len) == UPGRADE_ERR_NONE) {
                    s_upgrade_info.received_size += data_len;
                    send_ack(UP_CMD_WRITE_DATA_ACK, &status, 1);
                } else {
                    status = 0;
                    set_error(UP_ERR_FLASH_WRITE);
                    send_ack(UP_CMD_WRITE_DATA_NAK, &status, 1);
                }
            } else {
                APP_DBG("[UPGRADE] Write data invalid, len=%d, upgrading=%d\n", 
                        data_len, s_is_upgrading);
            }
            TimeOutSet(&s_upgrade_ing_time_out,2000);
            break;
            
        case UP_CMD_END:
            // 结束升级
            APP_DBG("[UPGRADE] End command received\n");
            if (s_is_upgrading) {
                s_is_upgrading = false;
                s_upgrade_ing_time_flag = FALSE;
                if (upgrade_finish() == UPGRADE_ERR_NONE) {
                    status = 1;
                    send_ack(UP_CMD_END_ACK, &status, 1); 
                    TimeOutSet(&s_upgrade_end_time_out,1000);
                    s_upgrade_end_time_flag = TRUE;
                } else {
                    status = 0;
                    set_error(UP_ERR_FLASH_VERIFY);
                }
            } else {
                status = 0;
                set_error(UP_ERR_NOT_READY);
            }
            break;
            
        case UP_CMD_QUERY_STATUS:
            // 查询状态
            {
                uint8_t status_data[2];
                status_data[0] = (uint8_t)s_state;
                status_data[1] = (uint8_t)upgrade_get_progress();
                send_ack(UP_CMD_QUERY_STATUS_ACK, status_data, 2);
            }
            break;
            
        default:
            APP_DBG("[UPGRADE] Unknown cmd: 0x%02X\n", cmd);
            break;
    }
}

/**
 * @brief 发送应答（外部调用接口）
 */
void upgrade_send_ack(uint8_t cmd, const uint8_t *data, uint16_t len)
{
    send_ack(cmd, data, len);
}

/**
 * @brief 获取升级状态
 */
UPGRADE_STATE upgrade_get_state(void)
{
    return s_state;
}

/**
 * @brief 获取升级信息
 */
UPGRADE_INFO* upgrade_get_info(void)
{
    return &s_upgrade_info;
}

/**
 * @brief timeout 之后重启
 */
void upgrade_end_reboot(void)
{
    if(s_upgrade_end_time_flag)
    {
        if(IsTimeOut(&s_upgrade_end_time_out))
        {
            execute_upgrade_and_reboot(); 
        }   
    }   
}


/**
 * @brief 监控升级逻辑
 */
void upgrade_monitor(void)
{
    if(s_upgrade_ing_time_flag)
    {
        if(IsTimeOut(&s_upgrade_ing_time_out))
        {
            send_ack(UP_CMD_WRITE_DATA_NAK, NULL, 1);
            upgrade_reset_to_idle();
        }
    }
}


#endif /* MVAPP_UPDATA_EN */