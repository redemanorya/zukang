#ifndef _BT_VISIBILITY_H_
#define _BT_VISIBILITY_H_

#include "bt_common_api.h"

typedef enum {
    VIS_MODE_DISABLED = 0x00,       // 不可见不可连接 (BtAccessModeNotAccessible)
    VIS_MODE_DISCOVERABLE = 0x01,   // 仅可发现不可连接 (BtAccessModeDiscoverbleOnly)
    VIS_MODE_CONNECTABLE = 0x02,    // 仅可连接不可发现 (BtAccessModeConnectableOnly)
    VIS_MODE_GENERAL = 0x03,        // 可见可连接 (BtAccessModeGeneralAccessible)
} VIS_MODE;

/**
 * @brief 初始化可见性管理器
 * @note  必须在蓝牙协议栈启动前调用一次
 */
void Vis_Init(void);

/**
 * @brief 设置蓝牙可见性（唯一对外接口）
 * @param mode    - 想要设置的可见性模式
 * @param caller  - 调用者的函数名，传 __FUNCTION__ 即可
 * @note  如果已被 Vis_Lock() 锁定，则本次设置无效，会打印 IGNORED 日志
 */
void Vis_Set(VIS_MODE mode, const char *caller);

/**
 * @brief 获取当前实际生效的可见性模式
 * @return 当前模式
 * @note  如果被锁定，返回的是锁定时的固定模式
 */
VIS_MODE Vis_Get(void);

/**
 * @brief 锁定可见性
 * @param locked_mode - 锁定后固定的可见性模式
 * @param caller      - 调用者标识，建议传函数名或 "app:reason"
 * @note  锁定后，所有 Vis_Set() 调用都会被忽略
 *        只有调用 Vis_Unlock() 才能解除
 */
void Vis_Lock(VIS_MODE locked_mode, const char *caller);

/**
 * @brief 解锁可见性并设置新的模式
 * @param mode   - 解锁后要设置的可见性模式
 * @param caller - 调用者标识
 * @note  解锁后，直接将可见性设置为 mode
 */
void Vis_Unlock(VIS_MODE mode, const char *caller);
/**
 * @brief 检查当前是否被锁定
 * @return true=已锁定，false=未锁定
 */
bool Vis_IsLocked(void);

#endif