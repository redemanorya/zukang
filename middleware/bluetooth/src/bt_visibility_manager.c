#include "bt_visibility_manager.h"
#include "bt_stack_service.h"
#include "bt_manager.h"
#include "debug.h"
#include "bt_interface.h"

static VIS_MODE s_current_mode = VIS_MODE_GENERAL;
static char s_last_caller[32] = {0};

// 锁相关
static bool s_is_locked = false;
static VIS_MODE s_locked_mode = VIS_MODE_GENERAL;
static char s_lock_caller[32] = {0};

static const char* mode_to_string(VIS_MODE mode)
{
    switch (mode) {
        case VIS_MODE_DISABLED:     return "DISABLED";
        case VIS_MODE_DISCOVERABLE: return "DISCOVERABLE";
        case VIS_MODE_CONNECTABLE:  return "CONNECTABLE";
        case VIS_MODE_GENERAL:      return "GENERAL";
        default:                    return "UNKNOWN";
    }
}

static void Vis_Apply(VIS_MODE mode)
{
    APP_DBG("[VIS] Apply: %s\n", mode_to_string(mode));
    BTSetAccessMode((uint8_t)mode);
}

void Vis_Init(void)
{
    s_current_mode = VIS_MODE_GENERAL;
    s_last_caller[0] = '\0';
    s_is_locked = false;
    s_locked_mode = VIS_MODE_GENERAL;
    s_lock_caller[0] = '\0';
    APP_DBG("[VIS] Init\n");
    Vis_Apply(s_current_mode);
}

void Vis_Set(VIS_MODE mode, const char *caller)
{
    // 如果被锁定，忽略本次设置
    if (s_is_locked) {
        APP_DBG("[VIS] IGNORED: %s -> %s (by %s) - LOCKED by %s, mode=%s\n",
            mode_to_string(s_current_mode),
            mode_to_string(mode),
            caller ? caller : "unknown",
            s_lock_caller,
            mode_to_string(s_locked_mode));
        return;
    }
  
    APP_DBG("[VIS] %s -> %s (by %s)\n",
        mode_to_string(s_current_mode),
        mode_to_string(mode),
        caller ? caller : "unknown");

    if (s_current_mode == mode && GetBtDeviceConnState() == mode) {
        APP_DBG("[VIS] Current mode is same. No change needed\n");
        return;
    }
    
    s_current_mode = mode;
    if (caller) {
        strncpy(s_last_caller, caller, sizeof(s_last_caller) - 1);
        s_last_caller[sizeof(s_last_caller) - 1] = '\0';
    }
    
    Vis_Apply(s_current_mode);
}

VIS_MODE Vis_Get(void)
{
    return s_current_mode;
}

void Vis_Lock(VIS_MODE locked_mode, const char *caller)
{
    if (s_is_locked) {
        APP_DBG("[VIS] Already locked by %s, ignore new lock request from %s\n",
            s_lock_caller, caller ? caller : "unknown");
        return;
    }
    
    s_is_locked = true;
    s_locked_mode = locked_mode;
    if (caller) {
        strncpy(s_lock_caller, caller, sizeof(s_lock_caller) - 1);
        s_lock_caller[sizeof(s_lock_caller) - 1] = '\0';
    }
    
    APP_DBG("[VIS] LOCKED by %s, force mode=%s\n",
        s_lock_caller,
        mode_to_string(s_locked_mode));
    
    // 锁定后，强制设置为锁定的模式
    if (s_current_mode != s_locked_mode) {
        s_current_mode = s_locked_mode;
        Vis_Apply(s_current_mode);
    }
}

void Vis_Unlock(VIS_MODE mode, const char *caller)
{
    if (!s_is_locked) {
        APP_DBG("[VIS] Not locked, ignore unlock from %s\n", caller ? caller : "unknown");
        return;
    }
    
    APP_DBG("[VIS] UNLOCKED by %s, set mode=%s\n",
        caller ? caller : "unknown",
        mode_to_string(mode));
    
    s_is_locked = false;
    s_lock_caller[0] = '\0';
    
    // 解锁后，直接设置为传入的模式
    if (s_current_mode != mode) {
        s_current_mode = mode;
        Vis_Apply(s_current_mode);
    }
}

bool Vis_IsLocked(void)
{
    return s_is_locked;
}
