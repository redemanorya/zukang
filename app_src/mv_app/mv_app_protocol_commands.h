/**
 **************************************************************************************
 * @file    mv_app_protocol_commands.h
 * @brief   协议命令处理器
 * @details 定义各命令的子命令码、参数结构体及命令处理器函数声明
 *          包含：握手查询、常用控制、灯效控制、麦克风音效、音乐音效、状态查询、LE Audio
 *
 * @author  Jarvis
 * @version V1.0.0
 *
 * $Created: 2026-01-01
 * $Revision: 1.0
 * $Date: 2026-01-01
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#ifndef MV_APP_PROTOCOL_COMMANDS_H
#define MV_APP_PROTOCOL_COMMANDS_H

#include "mv_app_protocol_handler.h"

#ifdef __cplusplus
extern "C" {
#endif
#ifdef MV_APP_CONTROL
/*=============================================================================
 * 1. 握手与版本查询子命令
 *============================================================================*/
typedef enum {
    HANDSHAKE_QUERY      = 0x01,     // APP查询设备版本
} HandshakeSubCmd;

/*=============================================================================
 * 2. 常用控制子命令
 *============================================================================*/
typedef enum {
    COMMON_POWER         = 0x01,     // 开关机
    COMMON_PLAY_PAUSE    = 0x02,     // 播放/暂停
    COMMON_PREV          = 0x03,     // 上一曲
    COMMON_NEXT          = 0x04,     // 下一曲
    COMMON_VOL_UP        = 0x05,     // 音量+
    COMMON_VOL_DOWN      = 0x06,     // 音量-
    COMMON_VOL_SET       = 0x07,     // 音量设置
    COMMON_MUTE          = 0x08,     // 静音/取消静音
    COMMON_MODE_SWITCH   = 0x09,     // 模式切换
} CommonControlSubCmd;

// 开关机动作
typedef enum {
    POWER_TOGGLE        = 0x00,      // 开关机
    POWER_ON            = 0x01,      // 开机
    POWER_OFF           = 0x02,      // 关机
} PowerAction;

// 静音动作
typedef enum {
    MUTE_TOGGLE         = 0x00,      // 静音/取消静音
    MUTE_ON             = 0x01,      // 静音
    MUTE_OFF            = 0x02,      // 取消静音
} MuteAction;

// 工作模式
typedef enum {
    MODE_SEQUENCE       = 0x00,      // 顺序切换
    MODE_BLUETOOTH      = 0x01,      // 蓝牙模式
    MODE_USB            = 0x02,      // U盘模式
    MODE_TF             = 0x03,      // TF卡模式
    MODE_LINEIN         = 0x04,      // Linein模式
    MODE_FM             = 0x05,      // FM模式
    MODE_OPTICAL        = 0x06,      // 光纤模式
    MODE_COAXIAL        = 0x07,      // 同轴模式
    MODE_USB_SOUNDCARD  = 0x08,      // USB声卡模式
} WorkMode;

/*=============================================================================
 * 3. 灯效控制子命令
 *============================================================================*/
typedef enum {
    LED_SWITCH           = 0x01,     // 灯效开关
    LED_MODE_SWITCH      = 0x02,     // 切换灯效模式
    LED_SET_RGB          = 0x03,     // 设置RGB颜色
    LED_SET_BRIGHTNESS   = 0x04,     // 设置亮度/速度
} LedEffectSubCmd;

// 灯效开关动作
typedef enum {
    LED_TOGGLE          = 0x00,      // 开关
    LED_ON              = 0x01,      // 开
    LED_OFF             = 0x02,      // 关
} LedSwitchAction;

/*=============================================================================
 * 4. 麦克风音效控制子命令
 *============================================================================*/
typedef enum {
    MIC_VOLUME           = 0x01,     // 麦克风音量
    MIC_BASS             = 0x02,     // 麦克风低音
    MIC_TREBLE           = 0x03,     // 麦克风高音
    MIC_EQ_SET           = 0x04,     // 固定频点EQ设定
    MIC_VOICE_EFFECT     = 0x05,     // 变音效果切换
    MIC_EFFECT_SWITCH    = 0x06,     // 音效开关
} MicEffectSubCmd;

// 变音效果
typedef enum {
    VOICE_SEQUENCE      = 0x00,      // 顺序切换
    VOICE_NORMAL        = 0x01,      // 正常
    VOICE_BABY          = 0x02,      // 娃娃音
    VOICE_MALE_TO_FEMALE= 0x03,      // 男变女
    VOICE_FEMALE_TO_MALE= 0x04,      // 女变男
} VoiceEffect;

// 麦克风音效类型
typedef enum {
    MIC_EFFECT_REVERB    = 0x00,     // 混响
    MIC_EFFECT_ANTI_HOWL = 0x01,     // 防啸叫
} MicEffectType;

/*=============================================================================
 * 5. 音乐音效控制子命令
 *============================================================================*/
typedef enum {
    MUSIC_VOLUME         = 0x01,     // 音乐音量
    MUSIC_BASS           = 0x02,     // 音乐低音
    MUSIC_TREBLE         = 0x03,     // 音乐高音
    MUSIC_EQ_SET         = 0x04,     // 固定频点EQ设定
    MUSIC_PRESET_EFFECT  = 0x05,     // 预设音效切换
    MUSIC_PRE_GAIN       = 0x06,     // 预设音效切换
} MusicEffectSubCmd;

/*=============================================================================
 * 6. 设备状态查询子命令
 *============================================================================*/
typedef enum {
    QUERY_ALL            = 0x01,     // 查询所有参数状态
    QUERY_COMMON         = 0x02,     // 查询常用控制参数
    QUERY_LED            = 0x03,     // 查询灯效控制参数
    QUERY_EFFECT         = 0x04,     // 查询音效参数
} QueryStateSubCmd;

/*=============================================================================
 * 7. LE Audio子命令
 *============================================================================*/
typedef enum {
    LE_SCAN_START        = 0x01,     // APP：搜索SRC
    LE_SCAN_INFO         = 0x02,     // SDK：反馈搜索信息
    LE_SCAN_STOP         = 0x03,     // APP：停止搜索
    LE_CONNECT           = 0x04,     // APP：接入指定SRC
    LE_CONNECT_RESULT    = 0x05,     // SDK：反馈接入结果
} LeAudioSubCmd;

/*=============================================================================
 * 公共参数定义
 *============================================================================*/
#define VOLUME_MAX              32
#define VOLUME_MIN              0
#define EQ_GAIN_MIN             -12
#define EQ_GAIN_MAX             12
#define BRIGHTNESS_MAX          255
#define BRIGHTNESS_MIN          0
#define SPEED_MAX               255
#define SPEED_MIN               0


#define STRINGIFY(x)               #x
#define STRINGIFY_VALUE(x)         STRINGIFY(x)

// 构建各模块版本字符串
#define SDK_VERSION_STR            "SDK:V" STRINGIFY_VALUE(CFG_SDK_MAJOR_VERSION) "." \
                                   STRINGIFY_VALUE(CFG_SDK_MINOR_VERSION) "." \
                                   STRINGIFY_VALUE(CFG_SDK_PATCH_VERSION)

#define FLASHBOOT_VERSION_STR      "Flashboot:V" STRINGIFY_VALUE(5) "." \
                                   STRINGIFY_VALUE(5) "." \
                                   STRINGIFY_VALUE(0)

#define DECODER_VERSION_STR        "AudioDecoder:V" STRINGIFY_VALUE(1) "." \
                                   STRINGIFY_VALUE(0) "." \
                                   STRINGIFY_VALUE(4)

#define EFFECT_VERSION_STR         "AudioEffect:V" STRINGIFY_VALUE(1) "." \
                                   STRINGIFY_VALUE(0) "." \
                                   STRINGIFY_VALUE(0)

#define ROBOEFFECT_VERSION_STR     "RoboEffect:V" STRINGIFY_VALUE(1) "." \
                                   STRINGIFY_VALUE(0) "." \
                                   STRINGIFY_VALUE(0)

#define DRIVER_VERSION_STR         "Driver:V" STRINGIFY_VALUE(1) "." \
                                   STRINGIFY_VALUE(0) "." \
                                   STRINGIFY_VALUE(0)

#define BT_VERSION_STR             "Bt:V" STRINGIFY_VALUE(1) "." \
                                   STRINGIFY_VALUE(0) "." \
                                   STRINGIFY_VALUE(0)

#define FATFS_VERSION_STR          "Fatfs:V" STRINGIFY_VALUE(1) "." \
                                   STRINGIFY_VALUE(0) "." \
                                   STRINGIFY_VALUE(0)

// 拼接所有版本信息
#define VERSION_INFO               SDK_VERSION_STR ";" \
                                   FLASHBOOT_VERSION_STR ";" \
                                   DECODER_VERSION_STR ";" \
                                   EFFECT_VERSION_STR ";" \
                                   ROBOEFFECT_VERSION_STR ";" \
                                   DRIVER_VERSION_STR ";" \
                                   BT_VERSION_STR ";" \
                                   FATFS_VERSION_STR

/*=============================================================================
 * 命令处理器函数声明
 *============================================================================*/
bool HandshakeCommandHandler(uint8_t subCmd, const uint8_t* params, uint16_t paramLen,
                             uint8_t* response, uint16_t* responseLen);

bool CommonControlCommandHandler(uint8_t subCmd, const uint8_t* params, uint16_t paramLen,
                                  uint8_t* response, uint16_t* responseLen);

bool LedEffectCommandHandler(uint8_t subCmd, const uint8_t* params, uint16_t paramLen,
                              uint8_t* response, uint16_t* responseLen);

bool MicEffectCommandHandler(uint8_t subCmd, const uint8_t* params, uint16_t paramLen,
                              uint8_t* response, uint16_t* responseLen);

bool MusicEffectCommandHandler(uint8_t subCmd, const uint8_t* params, uint16_t paramLen,
                                uint8_t* response, uint16_t* responseLen);

bool QueryStateCommandHandler(uint8_t subCmd, const uint8_t* params, uint16_t paramLen,
                               uint8_t* response, uint16_t* responseLen);

bool LeAudioCommandHandler(uint8_t subCmd, const uint8_t* params, uint16_t paramLen,
                            uint8_t* response, uint16_t* responseLen);
#endif

#ifdef __cplusplus
}
#endif

#endif /* MV_APP_PROTOCOL_COMMANDS_H */