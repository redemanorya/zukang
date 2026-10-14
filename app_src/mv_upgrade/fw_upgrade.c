// fw_upgrade.c
#include "fw_upgrade.h"
#include "type.h"
#include "debug.h"
#include "rom.h"
#include "flash_interface.h"
#include "mcu_circular_buf.h"
#include "watchdog.h"
#include "rtos_api.h"
#include <string.h>
#include <stdlib.h>
#ifdef MVAPP_UPDATA_EN
//=============================================================================
// 宏定义
//=============================================================================
#define UPGRADE_FIFO_SIZE           (63 * 1024)     // 63KB FIFO大小
#define DEFAULT_WRITE_BUFFER_SIZE   4096            // 默认写入缓冲区大小
#define DEFAULT_ERASE_BLOCK_SIZE    65536           // 默认擦除块大小（64KB）
#define DEFAULT_FLASH_OFFSET        0x400000        // 默认Flash偏移地址
#define ERASE_CYCLE_COUNT           15              // 每15个循环擦除一次

//=============================================================================
// 静态变量
//=============================================================================
static upgrade_config_t s_config;
static upgrade_status_t s_status = UPGRADE_STATUS_IDLE;
static upgrade_error_t s_last_error = UPGRADE_ERR_NONE;
static bool s_initialized = false;

static uint8_t *s_fifo_buffer = NULL;
static MCU_CIRCULAR_CONTEXT s_fifo_handle;
static uint8_t *s_write_buffer = NULL;

static uint32_t s_flash_write_addr = 0;
static uint32_t s_flash_erase_addr = 0;
static uint32_t s_block_erase_count = 0;
static uint32_t s_total_size = 0;
static uint32_t s_received_size = 0;
static uint16_t s_calc_crc = 0;
static bool s_fifo_ready = false;
static uint32_t s_erase_timer = 0;


static bool s_task_running = false;
#define UP_SERVICE_STACK_SIZE		2048
#define UP_SERVICE_PRIO			4
xTaskHandle			usb_up_taskHandle;

// CRC16 CCITT 表
static const uint16_t s_crc_ccitt_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

//=============================================================================
// 私有函数声明
//=============================================================================
static void update_status(upgrade_status_t status);
static void set_error(upgrade_error_t error);
static uint16_t crc16_calc(const uint8_t *buf, uint32_t len, uint16_t crc);
static upgrade_error_t flash_prepare(uint32_t total_size);
static upgrade_error_t flash_write_with_verify(uint32_t addr, const uint8_t *data, uint32_t len);
static bool flash_crc_verify(void);
static void flash_erase_next_block(void);
void execute_upgrade_and_reboot(void);
static void upgrade_worker_task(void *param);

//=============================================================================
// 私有函数实现
//=============================================================================

static void update_status(upgrade_status_t status)
{
    s_status = status;
    if (s_config.callback != NULL) {
        uint32_t progress = upgrade_get_progress();
        s_config.callback(status, progress, s_config.user_data);
    }
    APP_DBG("[UPGRADE] Status: %d, Progress: %u%%\n", status, upgrade_get_progress());
}

static void set_error(upgrade_error_t error)
{
    s_last_error = error;
    update_status(UPGRADE_STATUS_ERROR);
    APP_DBG("[UPGRADE] Error: %d\n", error);
}

static uint16_t crc16_calc(const uint8_t *buf, uint32_t len, uint16_t crc)
{
    for (uint32_t i = 0; i < len; i++) {
        crc = (crc << 8) ^ s_crc_ccitt_table[((crc >> 8) ^ buf[i]) & 0x00FF];
    }
    return crc;
}

/**
 * @brief 准备Flash（擦除第一个块）
 */
static upgrade_error_t flash_prepare(uint32_t total_size)
{
    if (total_size == 0) {
        return UPGRADE_ERR_INVALID_FILE;
    }

    // 计算需要擦除的块数
    s_block_erase_count = total_size / s_config.erase_block_size;
    if (total_size % s_config.erase_block_size) {
        s_block_erase_count++;
    }

    s_flash_erase_addr = s_config.flash_offset;
    s_flash_write_addr = s_config.flash_offset;

    // 擦除第一个块
    if (s_block_erase_count > 0) {
        uint32_t block_num = s_flash_erase_addr / s_config.erase_block_size;
        APP_DBG("[UPGRADE] Erasing first block %u at addr 0x%06X\n", block_num, s_flash_erase_addr);

        SpiFlashEraseWpr(BLOCK_ERASE, block_num, 1);

        s_flash_erase_addr += s_config.erase_block_size;
        s_block_erase_count--;
    }

    return UPGRADE_ERR_NONE;
}

/**
 * @brief 写入Flash并验证
 */
static upgrade_error_t flash_write_with_verify(uint32_t addr, const uint8_t *data, uint32_t len)
{
    if (len == 0 || data == NULL) {
        return UPGRADE_ERR_INVALID_PARAM;
    }

    // 写入数据
    if (SpiFlashWriteWpr(addr, (uint8_t*)data, len, 1) != 0) {
        return UPGRADE_ERR_FLASH_WRITE;
    }

    // 可选：验证写入
    if (s_config.enable_flash_verify) {
        uint8_t *verify_buf = s_write_buffer;  // 复用写入缓冲区
        SPiFlashReadData(addr, verify_buf, len);

        if (memcmp(data, verify_buf, len) != 0) {
            APP_DBG("[UPGRADE] Flash verify failed at addr 0x%06X\n", addr);
            return UPGRADE_ERR_FLASH_VERIFY;
        }
    }

    return UPGRADE_ERR_NONE;
}

/**
 * @brief CRC校验
 */
static bool flash_crc_verify(void)
{
    if (!s_config.enable_crc_check) {
        APP_DBG("[UPGRADE] CRC check disabled\n");
        return true;
    }

    if (s_total_size < 4) {
        APP_DBG("[UPGRADE] File too small for CRC\n");
        return false;
    }

    uint32_t addr = s_config.flash_offset;
    uint32_t data_len = s_total_size - 4;  // 最后4字节是CRC
    uint16_t calc_crc = 0;
    uint8_t tmp_byte = 0;
    uint8_t crc_bytes[4];

    APP_DBG("[UPGRADE] Verifying CRC, data_len=%u\n", data_len);

    // 使用 ROM_CRC16 逐字节计算（与旧代码一致）
    for (uint32_t i = 0; i < data_len; i++) {
        SpiFlashRead(addr, &tmp_byte, 1, 0);
        calc_crc = ROM_CRC16((const char*)&tmp_byte, 1, calc_crc);
        addr++;

        // 每64K喂一次狗，防止看门狗复位
        if ((i & 0xFFFF) == 0) {
            WDG_Feed();
        }
    }

    // 读取存储的CRC值（最后4字节）
    SpiFlashRead(addr,   &crc_bytes[0], 1, 0);
    SpiFlashRead(addr+1, &crc_bytes[1], 1, 0);
    SpiFlashRead(addr+2, &crc_bytes[2], 1, 0);
    SpiFlashRead(addr+3, &crc_bytes[3], 1, 0);

    APP_DBG("[UPGRADE] CRC bytes: 0x%02X,0x%02X,0x%02X,0x%02X\n",
            crc_bytes[0], crc_bytes[1], crc_bytes[2], crc_bytes[3]);

    // 按照旧代码的格式解析CRC（小端，且检查 Tmp[2] | Tmp[3] == 0）
    if (!(crc_bytes[2] | crc_bytes[3])) {
        uint16_t stored_crc = crc_bytes[1] << 8 | crc_bytes[0];

        APP_DBG("[UPGRADE] CRC: calc=0x%04X, stored=0x%04X\n", calc_crc, stored_crc);

        if (stored_crc != calc_crc) {
            APP_DBG("[UPGRADE] CRC mismatch!\n");
            return false;
        }
    } else {
        APP_DBG("[UPGRADE] Invalid CRC storage format (high bytes not zero)\n");
        return false;
    }

    APP_DBG("[UPGRADE] CRC verification passed\n");
    return true;
}

/**
 * @brief 擦除下一个Flash块
 */
static void flash_erase_next_block(void)
{
    if (s_block_erase_count == 0) {
        return;
    }

    // 确保擦除地址在写入地址之后
    if (s_flash_erase_addr < s_flash_write_addr) {
        s_flash_erase_addr = s_flash_write_addr;
        // 重新对齐到块边界
        s_flash_erase_addr = (s_flash_erase_addr + s_config.erase_block_size - 1)
                            / s_config.erase_block_size * s_config.erase_block_size;
    }

    uint32_t block_num = s_flash_erase_addr / s_config.erase_block_size;
    APP_DBG("[UPGRADE] Erasing block %u at addr 0x%06X\n", block_num, s_flash_erase_addr);

    SpiFlashEraseWpr(BLOCK_ERASE, block_num, 1);
	s_flash_erase_addr += s_config.erase_block_size;
	s_block_erase_count--;
}

/**
 * @brief 执行升级并重启
 */
void execute_upgrade_and_reboot(void)
{
    update_status(UPGRADE_STATUS_UPGRADING);

    APP_DBG("[UPGRADE] Applying firmware update from offset 0x%X\n", s_config.flash_offset);
    ROM_BankBUpgradeApply(0, s_config.flash_offset);

    osTaskDelay(100);
    ROM_SysReset();
}

/**
 * @brief 升级工作线程
 */
static void upgrade_worker_task(void *param)
{
    (void)param;

    s_erase_timer = 0;
    APP_DBG("[UPGRADE] Worker task started\n");

    while (1) {
        if (s_status != UPGRADE_STATUS_RECEIVING) {
            osTaskDelay(10);
            continue;
        }

        WDG_Feed();

        int fifo_len = MCUCircular_GetDataLen(&s_fifo_handle);

        // 只要有足够的数据就写入
        if (s_fifo_ready && fifo_len >= s_config.write_buffer_size) {
            MCUCircular_GetData(&s_fifo_handle, s_write_buffer, s_config.write_buffer_size);
            flash_write_with_verify(s_flash_write_addr, s_write_buffer, s_config.write_buffer_size);
            s_flash_write_addr += s_config.write_buffer_size;

            if (s_config.callback != NULL) {
                s_config.callback(s_status, upgrade_get_progress(), s_config.user_data);
            }
        }

        // 检查是否所有数据都已接收
        if (s_received_size >= s_total_size) {
            APP_DBG("[UPGRADE] Worker: all data received, deleting task\n");
            break;
        }

        // 定期擦除后续的Flash块
        s_erase_timer++;
        if (s_erase_timer >= ERASE_CYCLE_COUNT && s_block_erase_count > 0) {
            flash_erase_next_block();
            s_erase_timer = 0;
        }

        osTaskDelay(1);
    }

    APP_DBG("[UPGRADE] Worker task deleting itself\n");
    s_task_running = false;
    vTaskDelete(NULL);  // 删除自己
}


//=============================================================================
// 公共函数实现
//=============================================================================

upgrade_error_t upgrade_init(const upgrade_config_t *config)
{
    if (s_initialized) {
        APP_DBG("[UPGRADE] Already initialized\n");
        return UPGRADE_ERR_NONE;
    }

    // 设置默认配置
    memset(&s_config, 0, sizeof(s_config));
    s_config.flash_offset = DEFAULT_FLASH_OFFSET;
    s_config.max_file_size = 0;
    s_config.write_buffer_size = DEFAULT_WRITE_BUFFER_SIZE;
    s_config.erase_block_size = DEFAULT_ERASE_BLOCK_SIZE;
    s_config.enable_crc_check = true;
    s_config.enable_flash_verify = true;
    s_config.enable_auto_reboot = true;
    s_config.callback = NULL;
    s_config.user_data = NULL;

    // 覆盖用户配置
    if (config != NULL) {
        if (config->flash_offset != 0) {
            s_config.flash_offset = config->flash_offset;
        }
        if (config->max_file_size != 0) {
            s_config.max_file_size = config->max_file_size;
        }
        if (config->write_buffer_size != 0) {
            s_config.write_buffer_size = config->write_buffer_size;
        }
        if (config->erase_block_size != 0) {
            s_config.erase_block_size = config->erase_block_size;
        }
        s_config.enable_crc_check = config->enable_crc_check;
        s_config.enable_flash_verify = config->enable_flash_verify;
        s_config.enable_auto_reboot = config->enable_auto_reboot;
        s_config.callback = config->callback;
        s_config.user_data = config->user_data;
    }

    s_status = UPGRADE_STATUS_IDLE;
    s_last_error = UPGRADE_ERR_NONE;
    s_fifo_ready = false;

    // 分配FIFO内存
    if (s_fifo_buffer == NULL) {
        s_fifo_buffer = (uint8_t*)osPortMalloc(UPGRADE_FIFO_SIZE);
        if (s_fifo_buffer != NULL) {
            MCUCircular_Config(&s_fifo_handle, s_fifo_buffer, UPGRADE_FIFO_SIZE);
            s_fifo_ready = true;
        } else {
            APP_DBG("[UPGRADE] Failed to allocate FIFO buffer\n");
            return UPGRADE_ERR_MEM_ALLOC;
        }
    }

    // 分配写入缓冲区
    if (s_write_buffer == NULL) {
        s_write_buffer = (uint8_t*)osPortMalloc(s_config.write_buffer_size);
        if (s_write_buffer == NULL) {
            APP_DBG("[UPGRADE] Failed to allocate write buffer\n");
            return UPGRADE_ERR_MEM_ALLOC;
        }
    }

    s_initialized = true;
    APP_DBG("[UPGRADE] Module initialized, flash offset: 0x%X, write buf: %u, erase block: %u\n",
            s_config.flash_offset, s_config.write_buffer_size, s_config.erase_block_size);

    return UPGRADE_ERR_NONE;
}

void upgrade_deinit(void)
{
    if (!s_initialized) {
        return;
    }

    // 取消正在进行的升级（含停止 worker task）
    upgrade_cancel();

    if (s_fifo_buffer != NULL) {
        osPortFree(s_fifo_buffer);
        s_fifo_buffer = NULL;
    }

    if (s_write_buffer != NULL) {
        osPortFree(s_write_buffer);
        s_write_buffer = NULL;
    }

    s_fifo_ready = false;
    s_initialized = false;
    s_status = UPGRADE_STATUS_IDLE;

    APP_DBG("[UPGRADE] Module deinitialized\n");
}

upgrade_error_t upgrade_start(uint32_t total_size)
{
    if (!s_initialized) {
        APP_DBG("[UPGRADE] Not initialized\n");
        return UPGRADE_ERR_NOT_INIT;
    }

    if (s_status != UPGRADE_STATUS_IDLE) {
        APP_DBG("[UPGRADE] Busy, current status: %d\n", s_status);
        return UPGRADE_ERR_BUSY;
    }

    if (total_size == 0) {
        APP_DBG("[UPGRADE] Invalid file size: 0\n");
        return UPGRADE_ERR_INVALID_FILE;
    }

    if (s_config.max_file_size > 0 && total_size > s_config.max_file_size) {
        APP_DBG("[UPGRADE] File too large: %u > %u\n", total_size, s_config.max_file_size);
        return UPGRADE_ERR_FILE_TOO_LARGE;
    }

    // 重置状态
    s_total_size = total_size;
    s_received_size = 0;
    s_calc_crc = 0;
    s_flash_write_addr = s_config.flash_offset;
    s_flash_erase_addr = s_config.flash_offset;
    s_block_erase_count = 0;

    // 清空FIFO
    if (s_fifo_ready) {
        MCUCircular_Config(&s_fifo_handle, s_fifo_buffer, UPGRADE_FIFO_SIZE);
    }

    APP_DBG("[UPGRADE] Starting upgrade, total size: %u bytes\n", total_size);

    // 暂停音频服务
    AudioCoreServicePause();

    // 准备Flash
    upgrade_error_t err = flash_prepare(total_size);
    if (err != UPGRADE_ERR_NONE) {
        set_error(err);
        return err;
    }

    // 启动工作线程
    if (!s_task_running) {
        s_task_running = true;
        xTaskCreate(upgrade_worker_task, "upgrade_worker",UP_SERVICE_STACK_SIZE,NULL,UP_SERVICE_PRIO,&usb_up_taskHandle);
        if (usb_up_taskHandle == 0) {
            s_task_running = false;
            APP_DBG("[UPGRADE] Failed to create worker task\n");
            return UPGRADE_ERR_MEM_ALLOC;
        }
    }

    update_status(UPGRADE_STATUS_RECEIVING);

    return UPGRADE_ERR_NONE;
}

upgrade_error_t upgrade_receive_data(const uint8_t *data, uint32_t len)
{
    if (!s_initialized) {
        return UPGRADE_ERR_NOT_INIT;
    }

    if (data == NULL || len == 0) {
        return UPGRADE_ERR_INVALID_PARAM;
    }

    // 检查状态
    if (s_status != UPGRADE_STATUS_RECEIVING) {
        APP_DBG("[UPGRADE] Cannot receive data, invalid status: %d\n", s_status);
        return UPGRADE_ERR_INVALID_PARAM;
    }

    // 检查FIFO空间
    if (!s_fifo_ready || MCUCircular_GetSpaceLen(&s_fifo_handle) < len) {
        APP_DBG("[UPGRADE] FIFO full or not ready, space=%d, need=%u\n",
                MCUCircular_GetSpaceLen(&s_fifo_handle), len);
        return UPGRADE_ERR_MEM_ALLOC;
    }

    // 放入FIFO
    MCUCircular_PutData(&s_fifo_handle, data, len);
    s_received_size += len;

    // 计算实时CRC（不包括尾部CRC字段）
    if (s_config.enable_crc_check && s_received_size <= s_total_size - 4) {
        s_calc_crc = crc16_calc(data, len, s_calc_crc);
    }

    return UPGRADE_ERR_NONE;
}

upgrade_error_t upgrade_finish(void)
{
    if (!s_initialized) {
        return UPGRADE_ERR_NOT_INIT;
    }

    if (s_status != UPGRADE_STATUS_RECEIVING) {
        APP_DBG("[UPGRADE] Cannot finish, not in receiving state: %d\n", s_status);
        return UPGRADE_ERR_INVALID_PARAM;
    }

    APP_DBG("[UPGRADE] Finishing upgrade, total received: %u, total size: %u, write_addr=0x%06X\n",
            s_received_size, s_total_size, s_flash_write_addr);

    // 等待工作线程删除自己
    int timeout = 500;  // 500ms
    while (s_task_running && timeout > 0) {
        osTaskDelay(10);
        timeout -= 10;
    }

    update_status(UPGRADE_STATUS_VERIFYING);

    // 直接处理FIFO中剩余的数据
    if (s_fifo_ready) {
        uint32_t remaining = MCUCircular_GetDataLen(&s_fifo_handle);
        uint32_t written_so_far = s_flash_write_addr - s_config.flash_offset;
        uint32_t needed = s_total_size - written_so_far;

        APP_DBG("[UPGRADE] FIFO remaining: %d, written: %d, needed: %d\n",
                remaining, written_so_far, needed);

        if (remaining > 0 && needed > 0) {
            uint32_t bytes_to_write = (remaining < needed) ? remaining : needed;

            if (bytes_to_write > 0) {
                APP_DBG("[UPGRADE] Writing final %d bytes at addr 0x%06X\n",
                        bytes_to_write, s_flash_write_addr);

                MCUCircular_GetData(&s_fifo_handle, s_write_buffer, bytes_to_write);
                upgrade_error_t err = flash_write_with_verify(s_flash_write_addr, s_write_buffer, bytes_to_write);
                if (err != UPGRADE_ERR_NONE) {
                    set_error(err);
                    return err;
                }
                s_flash_write_addr += bytes_to_write;
                APP_DBG("[UPGRADE] Final write completed at addr 0x%06X\n", s_flash_write_addr);
            }
        }

        // 清空FIFO
        while (MCUCircular_GetDataLen(&s_fifo_handle) > 0) {
            uint8_t dummy[64];
            MCUCircular_GetData(&s_fifo_handle, dummy, 64);
        }
    }

    // 验证写入长度
    uint32_t final_written = s_flash_write_addr - s_config.flash_offset;
    if (final_written != s_total_size) {
        APP_DBG("[UPGRADE] Warning: written %d, expected %d, diff=%d\n",
                final_written, s_total_size, s_total_size - final_written);
    }

    // CRC校验
    bool crc_ok = flash_crc_verify();

    if (!crc_ok) {
        set_error(UPGRADE_ERR_CRC_FAIL);
        return UPGRADE_ERR_CRC_FAIL;
    }

    update_status(UPGRADE_STATUS_SUCCESS);
    APP_DBG("[UPGRADE] Firmware verification passed\n");

    // if (s_config.enable_auto_reboot) {
    //     execute_upgrade_and_reboot();
    // }

    return UPGRADE_ERR_NONE;
}

void upgrade_cancel(void)
{
    if (!s_initialized) {
        return;
    }

    APP_DBG("[UPGRADE] Upgrade cancelled\n");

    // 停止 worker task
    s_task_running = false;
    if (usb_up_taskHandle != NULL) {
        vTaskDelete(usb_up_taskHandle);
        usb_up_taskHandle = NULL;
        APP_DBG("[UPGRADE] Worker task deleted\n");
    }

    update_status(UPGRADE_STATUS_IDLE);
}

upgrade_status_t upgrade_get_status(void)
{
    return s_status;
}

uint32_t upgrade_get_progress(void)
{
    if (s_total_size == 0) {
        return 0;
    }
    return (s_received_size * 100) / s_total_size;
}

uint32_t upgrade_get_received_size(void)
{
    return s_received_size;
}

uint32_t upgrade_get_total_size(void)
{
    return s_total_size;
}

upgrade_error_t upgrade_get_last_error(void)
{
    return s_last_error;
}

bool upgrade_is_busy(void)
{
    return (s_status == UPGRADE_STATUS_RECEIVING || s_status == UPGRADE_STATUS_START);
}
#endif