/**
 **************************************************************************************
 * @file    mv_app_protocol_commands.c
 * @brief   协议命令处理器 
 * @details 实现各具体命令的业务逻辑处理
 *          每个命令处理器负责解析参数、执行硬件操作、返回响应数据
 *
 * @author  Jarvis
 * @version V1.0.0
 *
 * $Created: 2026-01-01
 * $Revision: 1.0
 * $Date: 2026-01-01
 *
 * @note    各命令处理器中的TODO部分需要根据实际硬件平台实现
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include "mv_app_protocol_commands.h"
#include <string.h>
#include <stdio.h>
#include "type.h"
#include "app_message.h"
#include "rtos_api.h"
#include "mvstdio.h"
#include "audio_decoder_api.h"
#include "audio_core_service.h"
#include "decoder.h"
#include "mcu_circular_buf.h"
#include "app_config.h"
#include "core_d1088.h"
#include "debug.h"
#include "main_task.h"
#include "ff.h"
#include "ffpresearch.h"
#include "bt_config.h"
#ifdef MV_APP_CONTROL
/*=============================================================================
 * 调试宏
 *============================================================================*/
#define CMD_DEBUG_ENABLE    1

#if CMD_DEBUG_ENABLE
#include <stdio.h>
#define CMD_DEBUG_LOG(...)   printf("[CMD] " __VA_ARGS__)
#else
#define CMD_DEBUG_LOG(...)
#endif

/*=============================================================================
 * 内部数据结构
 *============================================================================*/
// 设备状态信息（示例，实际项目中根据需求定义）
typedef struct {
    // 常用控制
    uint8_t     powerState;         // 0=关机, 1=开机
    uint8_t     volume;             // 0-32
    uint8_t     muteState;          // 0=正常, 1=静音
    uint8_t     workMode;           // 当前工作模式
    
    // 灯效控制
    uint8_t     ledEnable;          // 0=关, 1=开
    uint8_t     ledMode;            // 灯效模式
    uint8_t     ledR, ledG, ledB;   // RGB值
    uint8_t     ledBrightness;      // 亮度
    uint8_t     ledSpeed;           // 速度
    
    // 麦克风音效
    uint8_t     micVolume;          // 麦克风音量
    uint8_t     micBass;            // 麦克风低音
    uint8_t     micTreble;          // 麦克风高音
    uint8_t     micVoiceEffect;     // 变音效果
    uint8_t     micReverbEnable;    // 混响开关
    uint8_t     micAntiHowlEnable;  // 防啸叫开关
    
    // 音乐音效
    uint8_t     musicVolume;        // 音乐音量
    uint8_t     musicBass;          // 音乐低音
    uint8_t     musicTreble;        // 音乐高音
    uint8_t     freqgain_1;              
    uint8_t     freqgain_2;  
    uint8_t     freqgain_3;              
    uint8_t     freqgain_4;
    uint8_t     freqgain_5;              
    uint8_t     freqgain_6;    
    uint8_t     pregain;         
    uint8_t     musicPresetEffect;  // 预设音效
} DeviceState;

static DeviceState g_deviceState = {
    .powerState = 1,        // 默认开机
    .volume = 15,           // 默认音量15
    .workMode = MODE_BLUETOOTH,
    .ledEnable = 1,
    .ledBrightness = 128,
    .micVolume = 10,
    .musicVolume = 15,
};

/*=============================================================================
 * 内部辅助函数
 *============================================================================*/
static bool IsVolumeValid(uint8_t volume) {
    return (volume >= VOLUME_MIN && volume <= VOLUME_MAX);
}

/*=============================================================================
 * 1. 握手与版本查询命令处理器
 *============================================================================*/
bool HandshakeCommandHandler(uint8_t subCmd, const uint8_t* params, uint16_t paramLen,
                              uint8_t* response, uint16_t* responseLen) {
    switch (subCmd) {
        case HANDSHAKE_QUERY: {
            // 使用预定义的版本信息字符串
            const char* versionInfo = VERSION_INFO;
            uint16_t infoLen = strlen(versionInfo);
            
            if (infoLen > 0) {
                memcpy(response, versionInfo, infoLen);
                *responseLen = infoLen;
            } else {
                *responseLen = 0;
            }
            return true;
        }
        default:
            return false;
    }
}

extern void PowerOffMessage(void);
/*=============================================================================
 * 2. 常用控制命令处理器
 *============================================================================*/
bool CommonControlCommandHandler(uint8_t subCmd, const uint8_t* params, uint16_t paramLen,
                                  uint8_t* response, uint16_t* responseLen) {
    *responseLen = 0;
    MessageContext		msgSend;

    switch (subCmd) {
        case COMMON_POWER: {
            if (paramLen >= 1) {
                uint8_t action = params[0];
                CMD_DEBUG_LOG("Power action=%d\n", action);
                
                switch (action) {
                    case POWER_TOGGLE:
                        g_deviceState.powerState = !g_deviceState.powerState;
                        PowerOffMessage();
                        break;
                    case POWER_ON:
                        g_deviceState.powerState = 1;
                        break;
                    case POWER_OFF:
                        g_deviceState.powerState = 0;
                        break;
                    default:
                        return false;
                }
                // TODO: 调用硬件开关机函数
                return true;
            }
            return false;
        }
        
        case COMMON_PLAY_PAUSE:
            CMD_DEBUG_LOG("Play/Pause\n");
            // TODO: 调用播放/暂停函数
            SendSysModeMsg(MSG_PLAY_PAUSE);
            return true;
            
        case COMMON_PREV:
            CMD_DEBUG_LOG("Previous track\n");
            // TODO: 调用上一曲函数
             SendSysModeMsg(MSG_PRE);
            return true;
            
        case COMMON_NEXT:
            CMD_DEBUG_LOG("Next track\n");
            // TODO: 调用下一曲函数
             SendSysModeMsg(MSG_NEXT);
            return true;
            
        case COMMON_VOL_UP:
            if (g_deviceState.volume < VOLUME_MAX) {
                g_deviceState.volume++;
                CMD_DEBUG_LOG("Volume UP, now=%d\n", g_deviceState.volume);
                // TODO: 调用音量+函数
                msgSend.msgId = MSG_MUSIC_VOLUP;
                MessageSend(GetAudioCoreServiceMsgHandle(), &msgSend);
            }else{
                CMD_DEBUG_LOG("Volume Max\n");
            }
            return true;
            
        case COMMON_VOL_DOWN:
            if (g_deviceState.volume > VOLUME_MIN) {
                g_deviceState.volume--;
                CMD_DEBUG_LOG("Volume DOWN, now=%d\n", g_deviceState.volume);
                // TODO: 调用音量-函数
                msgSend.msgId = MSG_MUSIC_VOLDOWN;
                MessageSend(GetAudioCoreServiceMsgHandle(), &msgSend);
            }else{
                CMD_DEBUG_LOG("Volume Min\n");
            }
            return true;
            
        case COMMON_VOL_SET: {
            if (paramLen >= 1) {
                uint8_t volume = params[0];
                if (IsVolumeValid(volume)) {
                    g_deviceState.volume = volume;
                    CMD_DEBUG_LOG("Volume set=%d\n", volume);
                    // TODO: 调用设置音量函数
                    return true;
                }
            }
            return false;
        }
        
        case COMMON_MUTE: {
            if (paramLen >= 1) {
                uint8_t action = params[0];
                CMD_DEBUG_LOG("Mute action=%d\n", action);
                
                switch (action) {
                    case MUTE_TOGGLE:
                        g_deviceState.muteState = !g_deviceState.muteState;
                        SendSysModeMsg(MSG_MUTE);
                        break;
                    case MUTE_ON:
                        g_deviceState.muteState = 1;
                        break;
                    case MUTE_OFF:
                        g_deviceState.muteState = 0;
                        break;
                    default:
                        return false;
                }
                return true;
            }
            return false;
        }
        
        case COMMON_MODE_SWITCH: {
            if (paramLen >= 1) {
                uint8_t mode = params[0];
                CMD_DEBUG_LOG("Mode switch=%d\n", mode);
                g_deviceState.workMode = mode;
                // TODO: 调用模式切换函数
                msgSend.msgId = MSG_MODE;
                MessageSend(GetMainMessageHandle(), &msgSend);
                return true;
            }
            return false;
        }
        
        default:
            CMD_DEBUG_LOG("Unknown common control subCmd=0x%02X\n", subCmd);
            return false;
    }
}

extern void mv_app_rgb_onoff(uint8_t onoff);
extern void mv_app_get_rgb(uint8_t r,uint8_t g,uint8_t b);
extern void AudioMusicVolSet(uint8_t musicVol);
/*=============================================================================
 * 3. 灯效控制命令处理器
 *============================================================================*/
bool LedEffectCommandHandler(uint8_t subCmd, const uint8_t* params, uint16_t paramLen,
                              uint8_t* response, uint16_t* responseLen) {
    *responseLen = 0;
    
    switch (subCmd) {
        case LED_SWITCH: {
            if (paramLen >= 1) {
                uint8_t action = params[0];
                CMD_DEBUG_LOG("LED switch action=%d\n", action);
                
                switch (action) {
                    case LED_TOGGLE:
                        g_deviceState.ledEnable = !g_deviceState.ledEnable;
#ifdef CFG_DMA_RGB_LED_EN
                        mv_app_rgb_onoff(g_deviceState.ledEnable);
#endif
                        break;
                    case LED_ON:
                        g_deviceState.ledEnable = 1;
                        break;
                    case LED_OFF:
                        g_deviceState.ledEnable = 0;
                        break;
                    default:
                        return false;
                }
                // TODO: 调用灯效开关函数
                return true;
            }
            return false;
        }
        
        case LED_MODE_SWITCH: {
            if (paramLen >= 1) {
                uint8_t mode = params[0];
                CMD_DEBUG_LOG("LED mode switch=%d\n", mode);
                g_deviceState.ledMode = mode;
                // TODO: 调用切换灯效模式函数
                return true;
            }
            return false;
        }
        
        case LED_SET_RGB: {
            if (paramLen >= 3) {
                g_deviceState.ledR = params[0];
                g_deviceState.ledG = params[1];
                g_deviceState.ledB = params[2];
                CMD_DEBUG_LOG("LED RGB: R=%d, G=%d, B=%d\n", 
                              g_deviceState.ledR, g_deviceState.ledG, g_deviceState.ledB);
#ifdef CFG_DMA_RGB_LED_EN
                // TODO: 调用设置RGB颜色函数
                mv_app_get_rgb(g_deviceState.ledR,g_deviceState.ledG,g_deviceState.ledB);
#endif
                return true;
            }
            return false;
        }
        
        case LED_SET_BRIGHTNESS: {
            if (paramLen >= 2) {
                uint8_t brightness = params[0];
                uint8_t speed = params[1];
                g_deviceState.ledBrightness = brightness;
                g_deviceState.ledSpeed = speed;
                CMD_DEBUG_LOG("LED brightness=%d, speed=%d\n", brightness, speed);
                // TODO: 调用设置亮度/速度函数
                return true;
            }
            return false;
        }
        
        default:
            CMD_DEBUG_LOG("Unknown LED effect subCmd=0x%02X\n", subCmd);
            return false;
    }
}

/*=============================================================================
 * 4. 麦克风音效控制命令处理器
 *============================================================================*/
bool MicEffectCommandHandler(uint8_t subCmd, const uint8_t* params, uint16_t paramLen,
                              uint8_t* response, uint16_t* responseLen) {
    *responseLen = 0;
    
    switch (subCmd) {
        case MIC_VOLUME: {
            if (paramLen >= 1) {
                uint8_t volume = params[0];
                if (IsVolumeValid(volume)) {
                    g_deviceState.micVolume = volume;
                    CMD_DEBUG_LOG("Mic volume=%d\n", volume);
                    // TODO: 调用设置麦克风音量函数
                    return true;
                }
            }
            return false;
        }
        
        case MIC_BASS: {
            if (paramLen >= 1) {
                uint8_t bass = params[0];
                g_deviceState.micBass = bass;
                CMD_DEBUG_LOG("Mic bass=%d\n", bass);
                // TODO: 调用设置麦克风低音函数
                return true;
            }
            return false;
        }
        
        case MIC_TREBLE: {
            if (paramLen >= 1) {
                uint8_t treble = params[0];
                g_deviceState.micTreble = treble;
                CMD_DEBUG_LOG("Mic treble=%d\n", treble);
                // TODO: 调用设置麦克风高音函数
                return true;
            }
            return false;
        }
        
        case MIC_EQ_SET: {
            if (paramLen >= 6) {
                uint32_t freq = (params[0] << 24) | (params[1] << 16) | 
                               (params[2] << 8) | params[3];
                int8_t gain = (int8_t)params[5];
                CMD_DEBUG_LOG("Mic EQ: freq=%lu Hz, gain=%d\n", (unsigned long)freq, gain);
                // TODO: 调用设置EQ函数
                return true;
            }
            return false;
        }
        
        case MIC_VOICE_EFFECT: {
            if (paramLen >= 1) {
                uint8_t effect = params[0];
                g_deviceState.micVoiceEffect = effect;
                CMD_DEBUG_LOG("Voice effect=%d\n", effect);
                // TODO: 调用变音效果切换函数
                return true;
            }
            return false;
        }
        
        case MIC_EFFECT_SWITCH: {
            if (paramLen >= 2) {
                uint8_t effectType = params[0];
                uint8_t enable = params[1];
                CMD_DEBUG_LOG("Mic effect: type=%d, enable=%d\n", effectType, enable);
                
                switch (effectType) {
                    case MIC_EFFECT_REVERB:
                        g_deviceState.micReverbEnable = enable;
                        break;
                    case MIC_EFFECT_ANTI_HOWL:
                        g_deviceState.micAntiHowlEnable = enable;
                        break;
                    default:
                        return false;
                }
                // TODO: 调用音效开关函数
                return true;
            }
            return false;
        }
        
        default:
            CMD_DEBUG_LOG("Unknown mic effect subCmd=0x%02X\n", subCmd);
            return false;
    }
}

/*=============================================================================
 * 5. 音乐音效控制命令处理器
 *============================================================================*/
bool MusicEffectCommandHandler(uint8_t subCmd, const uint8_t* params, uint16_t paramLen,
                                uint8_t* response, uint16_t* responseLen) {
    *responseLen = 0;
    const int16_t* music_eq_data;
    int16_t eqgain;
    ROBOEFFECT_ERROR_CODE erro;

    switch (subCmd) {
        case MUSIC_VOLUME: {
            if (paramLen >= 1) {
                uint8_t volume = params[0];
                if (IsVolumeValid(volume)) {
                    g_deviceState.musicVolume = volume;
                    CMD_DEBUG_LOG("Music volume=%d\n", volume);
                    // TODO: 调用设置音乐音量函数
                    AudioMusicVolSet(volume);
                    return true;
                }
            }
            return false;
        }
        
        case MUSIC_BASS: {
            if (paramLen >= 1) {
                int8_t bass = params[0];
                g_deviceState.musicBass = bass;
                CMD_DEBUG_LOG("Music bass=%d\n", bass);
                if (bass > 36) {
                    bass = bass - 256;  
                }

                CMD_DEBUG_LOG("Music bass: gain=%d\n",bass);
                CMD_DEBUG_LOG("Music bass addr:0x%x\n", 
                            AudioEffect_Parambin_GetEffectAddr(&EffectParambin, EFFECT_MUSICEQ));
                
                // 获取当前值（调试用）
                music_eq_data = AudioEffect_ParamGet(EffectParambin.context_memory,
                                AudioEffect_Parambin_GetEffectAddr(&EffectParambin, EFFECT_MUSICEQ),
                                INDEX_EQ_FILTER7_GAIN);
                CMD_DEBUG_LOG("Music bass before:%d (0x%04X)\n", *music_eq_data, *music_eq_data);
                
                // 将 gain (int8_t) 扩展到 int16_t 的高8位
                eqgain = (int16_t)bass << 8;
                CMD_DEBUG_LOG("Music bass set gain:%d (0x%04X)\n", eqgain, (uint16_t)eqgain);
                
                // 设置参数
                erro = AudioEffect_ParamSet(EffectParambin.context_memory,
                                            AudioEffect_Parambin_GetEffectAddr(&EffectParambin, EFFECT_MUSICEQ),
                                            INDEX_EQ_FILTER7_GAIN,
                                            &eqgain);
                return true;
            }
            return false;
        }
        
        case MUSIC_TREBLE: {
            if (paramLen >= 1) {
                int8_t treble = params[0];
                g_deviceState.musicTreble = treble;
                CMD_DEBUG_LOG("Music treble=%d\n", treble);
                if (treble > 36) {
                    treble = treble - 256;  
                }

                CMD_DEBUG_LOG("Music treble: gain=%d\n",treble);
                CMD_DEBUG_LOG("Music treble addr:0x%x\n", 
                            AudioEffect_Parambin_GetEffectAddr(&EffectParambin, EFFECT_MUSICEQ));
                
                // 获取当前值（调试用）
                music_eq_data = AudioEffect_ParamGet(EffectParambin.context_memory,
                                AudioEffect_Parambin_GetEffectAddr(&EffectParambin, EFFECT_MUSICEQ),
                                INDEX_EQ_FILTER8_GAIN);
                CMD_DEBUG_LOG("Music treble before:%d (0x%04X)\n", *music_eq_data, *music_eq_data);
                
                // 将 gain (int8_t) 扩展到 int16_t 的高8位
                eqgain = (int16_t)treble << 8;
                CMD_DEBUG_LOG("Music treble set gain:%d (0x%04X)\n", eqgain, (uint16_t)eqgain);
                
                // 设置参数
                erro = AudioEffect_ParamSet(EffectParambin.context_memory,
                                            AudioEffect_Parambin_GetEffectAddr(&EffectParambin, EFFECT_MUSICEQ),
                                            INDEX_EQ_FILTER8_GAIN,
                                            &eqgain);
                return true;
            }
            return false;
        }
        
        case MUSIC_EQ_SET: {
            if (paramLen >= 6) {
            uint32_t freq = (params[3] << 24) | (params[2] << 16) | 
                        (params[1] << 8) | params[0];
            int8_t gain = (int8_t)params[5];
            
            // 解码：0~36 保持不变，220~255 转为负数
            if (gain > 36) {
                gain = gain - 256;  // 220~255 → -36~-1
            }
            
            CMD_DEBUG_LOG("Music EQ: freq=%lu Hz, gain=%d\n", (unsigned long)freq, gain);
            CMD_DEBUG_LOG("Music EQ addr:0x%x\n", 
                        AudioEffect_Parambin_GetEffectAddr(&EffectParambin, EFFECT_MUSICEQ));
            

            switch (freq)
            {
            case 31:
                    g_deviceState.freqgain_1 = (int8_t)params[5];
                    // 获取当前值（调试用）
                    music_eq_data = AudioEffect_ParamGet(EffectParambin.context_memory,
                                    AudioEffect_Parambin_GetEffectAddr(&EffectParambin, EFFECT_MUSICEQ),
                                    INDEX_EQ_FILTER1_GAIN);
                    CMD_DEBUG_LOG("Music EQ req=%lu Hz before:%d (0x%04X)\n",(unsigned long)freq, *music_eq_data, *music_eq_data);
                    
                    // 将 gain (int8_t) 扩展到 int16_t 的高8位
                    eqgain = (int16_t)gain << 8;
                    CMD_DEBUG_LOG("Music EQ set gain:%d (0x%04X)\n", eqgain, (uint16_t)eqgain);
                    
                    // 设置参数
                    erro = AudioEffect_ParamSet(EffectParambin.context_memory,
                                                AudioEffect_Parambin_GetEffectAddr(&EffectParambin, EFFECT_MUSICEQ),
                                                INDEX_EQ_FILTER1_GAIN,&eqgain);
                break;
                case 62:
                    g_deviceState.freqgain_2 = (int8_t)params[5];
                                // 获取当前值（调试用）
                    music_eq_data = AudioEffect_ParamGet(EffectParambin.context_memory,
                                    AudioEffect_Parambin_GetEffectAddr(&EffectParambin, EFFECT_MUSICEQ),
                                    INDEX_EQ_FILTER2_GAIN);
                    CMD_DEBUG_LOG("Music EQ req=%lu Hz before:%d (0x%04X)\n",(unsigned long)freq, *music_eq_data, *music_eq_data);
                    
                    // 将 gain (int8_t) 扩展到 int16_t 的高8位
                    eqgain = (int16_t)gain << 8;
                    CMD_DEBUG_LOG("Music EQ set gain:%d (0x%04X)\n", eqgain, (uint16_t)eqgain);
                    
                    // 设置参数
                    erro = AudioEffect_ParamSet(EffectParambin.context_memory,
                                                AudioEffect_Parambin_GetEffectAddr(&EffectParambin, EFFECT_MUSICEQ),
                                                INDEX_EQ_FILTER2_GAIN,&eqgain);
                break;
                case 125:
                    g_deviceState.freqgain_3 = (int8_t)params[5];
                                // 获取当前值（调试用）
                    music_eq_data = AudioEffect_ParamGet(EffectParambin.context_memory,
                                    AudioEffect_Parambin_GetEffectAddr(&EffectParambin, EFFECT_MUSICEQ),
                                    INDEX_EQ_FILTER3_GAIN);
                    CMD_DEBUG_LOG("Music EQ req=%lu Hz before:%d (0x%04X)\n",(unsigned long)freq, *music_eq_data, *music_eq_data);
                    
                    // 将 gain (int8_t) 扩展到 int16_t 的高8位
                    eqgain = (int16_t)gain << 8;
                    CMD_DEBUG_LOG("Music EQ set gain:%d (0x%04X)\n", eqgain, (uint16_t)eqgain);
                    
                    // 设置参数
                    erro = AudioEffect_ParamSet(EffectParambin.context_memory,
                                                AudioEffect_Parambin_GetEffectAddr(&EffectParambin, EFFECT_MUSICEQ),
                                                INDEX_EQ_FILTER3_GAIN,&eqgain);
                break;
                    case 500:
                    g_deviceState.freqgain_4 = (int8_t)params[5];
                                // 获取当前值（调试用）
                    music_eq_data = AudioEffect_ParamGet(EffectParambin.context_memory,
                                    AudioEffect_Parambin_GetEffectAddr(&EffectParambin, EFFECT_MUSICEQ),
                                    INDEX_EQ_FILTER4_GAIN);
                    CMD_DEBUG_LOG("Music EQ req=%lu Hz before:%d (0x%04X)\n",(unsigned long)freq, *music_eq_data, *music_eq_data);
                    
                    // 将 gain (int8_t) 扩展到 int16_t 的高8位
                    eqgain = (int16_t)gain << 8;
                    CMD_DEBUG_LOG("Music EQ set gain:%d (0x%04X)\n", eqgain, (uint16_t)eqgain);
                    
                    // 设置参数
                    erro = AudioEffect_ParamSet(EffectParambin.context_memory,
                                                AudioEffect_Parambin_GetEffectAddr(&EffectParambin, EFFECT_MUSICEQ),
                                                INDEX_EQ_FILTER4_GAIN,&eqgain);
                break;
                case 1000:
                        g_deviceState.freqgain_5 = (int8_t)params[5];
                                // 获取当前值（调试用）
                    music_eq_data = AudioEffect_ParamGet(EffectParambin.context_memory,
                                    AudioEffect_Parambin_GetEffectAddr(&EffectParambin, EFFECT_MUSICEQ),
                                    INDEX_EQ_FILTER5_GAIN);
                    CMD_DEBUG_LOG("Music EQ req=%lu Hz before:%d (0x%04X)\n",(unsigned long)freq, *music_eq_data, *music_eq_data);
                    
                    // 将 gain (int8_t) 扩展到 int16_t 的高8位
                    eqgain = (int16_t)gain << 8;
                    CMD_DEBUG_LOG("Music EQ set gain:%d (0x%04X)\n", eqgain, (uint16_t)eqgain);
                    
                    // 设置参数
                    erro = AudioEffect_ParamSet(EffectParambin.context_memory,
                                                AudioEffect_Parambin_GetEffectAddr(&EffectParambin, EFFECT_MUSICEQ),
                                                INDEX_EQ_FILTER5_GAIN,&eqgain);
                break;
                    case 4000:
                    g_deviceState.freqgain_6 = (int8_t)params[5];
                                // 获取当前值（调试用）
                    music_eq_data = AudioEffect_ParamGet(EffectParambin.context_memory,
                                    AudioEffect_Parambin_GetEffectAddr(&EffectParambin, EFFECT_MUSICEQ),
                                    INDEX_EQ_FILTER6_GAIN);
                    CMD_DEBUG_LOG("Music EQ req=%lu Hz before:%d (0x%04X)\n",(unsigned long)freq, *music_eq_data, *music_eq_data);
                    
                    // 将 gain (int8_t) 扩展到 int16_t 的高8位
                    eqgain = (int16_t)gain << 8;
                    CMD_DEBUG_LOG("Music EQ set gain:%d (0x%04X)\n", eqgain, (uint16_t)eqgain);
                    
                    // 设置参数
                    erro = AudioEffect_ParamSet(EffectParambin.context_memory,
                                                AudioEffect_Parambin_GetEffectAddr(&EffectParambin, EFFECT_MUSICEQ),
                                                INDEX_EQ_FILTER6_GAIN,&eqgain);
                break;
            
            default:
                break;
            }


            if(erro != ROBOEFFECT_ERROR_OK)
            {
                CMD_DEBUG_LOG("Music EQ set erro:%d \n", erro);
            }
            
            return true;
            }
            return false;
        }
        
        case MUSIC_PRESET_EFFECT: {
            if (paramLen >= 1) {
                uint8_t effect = params[0];
                g_deviceState.musicPresetEffect = effect;
                CMD_DEBUG_LOG("Preset effect=%d\n", effect);
                // TODO: 调用预设音效切换函数
                return true;
            }
            return false;
        }

        case MUSIC_PRE_GAIN:
            {
                int8_t pregain = (int8_t)params[0];
                g_deviceState.pregain = params[0];                                      
                if (pregain > 18) {
                    pregain = pregain - 256;  
                }

                CMD_DEBUG_LOG("Music Pregain: gain=%d\n",pregain);
                CMD_DEBUG_LOG("Music Pregain addr:0x%x\n", 
                            AudioEffect_Parambin_GetEffectAddr(&EffectParambin, EFFECT_MUSICEQ));
                
                // 获取当前值（调试用）
                music_eq_data = AudioEffect_ParamGet(EffectParambin.context_memory,
                                AudioEffect_Parambin_GetEffectAddr(&EffectParambin, EFFECT_MUSICEQ),
                                INDEX_EQ_PREGAIN);
                CMD_DEBUG_LOG("Music Pregain before:%d (0x%04X)\n", *music_eq_data, *music_eq_data);
                
                // 将 gain (int8_t) 扩展到 int16_t 的高8位
                eqgain = (int16_t)pregain << 8;
                CMD_DEBUG_LOG("Music Pregain set gain:%d (0x%04X)\n", eqgain, (uint16_t)eqgain);
                
                // 设置参数
                erro = AudioEffect_ParamSet(EffectParambin.context_memory,
                                            AudioEffect_Parambin_GetEffectAddr(&EffectParambin, EFFECT_MUSICEQ),
                                            INDEX_EQ_PREGAIN,
                                            &eqgain);

                if(erro != ROBOEFFECT_ERROR_OK)
                {
                    CMD_DEBUG_LOG("Music Pregain set erro:%d \n", erro);
                }
            }
            break;
        
        default:
            CMD_DEBUG_LOG("Unknown music effect subCmd=0x%02X\n", subCmd);
            return false;
    }
}


// 将设备存储的 int16_t 值转换为 app 需要的 uint8_t 格式
uint8_t convert_eqgain_to_protocol(const int16_t* stored_ptr) {
    if (stored_ptr == NULL) {
        return 0;  // 默认值
    }
    
    int16_t stored = *stored_ptr;      // 解引用获取实际值
    int8_t gain = stored >> 8;         // 提取高8位
    
    // 转换为 app 协议格式
    if (gain >= 0) {
        return (uint8_t)gain;          // 0~36 → 0~36
    } else {
        return (uint8_t)(256 + gain);  // -36 → 220, -1 → 255
    }
}

/*=============================================================================
 * 6. 设备状态查询命令处理器
 *============================================================================*/
bool QueryStateCommandHandler(uint8_t subCmd, const uint8_t* params, uint16_t paramLen,
                               uint8_t* response, uint16_t* responseLen) {
    (void)params;
    (void)paramLen;
    
    switch (subCmd) {
        case QUERY_ALL: {
            // 返回所有设备参数
            // 格式：自定义，根据APP约定
            memcpy(response, &g_deviceState, sizeof(g_deviceState));
            *responseLen = sizeof(g_deviceState);
            CMD_DEBUG_LOG("Query all state, len=%d\n", *responseLen);
            return true;
        }
        
        case QUERY_COMMON: {
            // 返回常用控制参数
            uint8_t commonData[4] = {
                g_deviceState.powerState,
                g_deviceState.volume,
                g_deviceState.muteState,
                g_deviceState.workMode
            };
            memcpy(response, commonData, sizeof(commonData));
            *responseLen = sizeof(commonData);
            return true;
        }
        
        case QUERY_LED: {
            // 返回灯效控制参数
            uint8_t ledData[6] = {
                g_deviceState.ledEnable,
                g_deviceState.ledMode,
                g_deviceState.ledR,
                g_deviceState.ledG,
                g_deviceState.ledB,
                g_deviceState.ledBrightness
            };
            memcpy(response, ledData, sizeof(ledData));
            *responseLen = sizeof(ledData);
            return true;
        }
        
        case QUERY_EFFECT: {
            const int16_t *query_data;
            query_data = AudioEffect_ParamGet(EffectParambin.context_memory,
                                AudioEffect_Parambin_GetEffectAddr(&EffectParambin, EFFECT_MUSICEQ),
                                INDEX_EQ_FILTER7_GAIN);
            g_deviceState.musicBass = convert_eqgain_to_protocol(query_data);
            CMD_DEBUG_LOG("query musicBass gain:%d (0x%04X),g_deviceState.musicBass:%d\n", *query_data, *query_data,g_deviceState.musicBass);

            query_data = AudioEffect_ParamGet(EffectParambin.context_memory,
                                AudioEffect_Parambin_GetEffectAddr(&EffectParambin, EFFECT_MUSICEQ),
                                INDEX_EQ_FILTER8_GAIN);
            g_deviceState.musicTreble = convert_eqgain_to_protocol(query_data);
            CMD_DEBUG_LOG("query musicTreble gain:%d (0x%04X),g_deviceState.musicTreble:%d\n", *query_data, *query_data,g_deviceState.musicTreble);

            query_data = AudioEffect_ParamGet(EffectParambin.context_memory,
                                AudioEffect_Parambin_GetEffectAddr(&EffectParambin, EFFECT_MUSICEQ),
                                INDEX_EQ_FILTER1_GAIN);
            g_deviceState.freqgain_1 = convert_eqgain_to_protocol(query_data);
            CMD_DEBUG_LOG("query freqgain_1 gain:%d (0x%04X),g_deviceState.freqgain_1:%d\n", *query_data, *query_data,g_deviceState.freqgain_1);

            query_data = AudioEffect_ParamGet(EffectParambin.context_memory,
                                AudioEffect_Parambin_GetEffectAddr(&EffectParambin, EFFECT_MUSICEQ),
                                INDEX_EQ_FILTER2_GAIN);
            g_deviceState.freqgain_2 = convert_eqgain_to_protocol(query_data);
            CMD_DEBUG_LOG("query freqgain_2 gain:%d (0x%04X),g_deviceState.freqgain_2:%d\n", *query_data, *query_data,g_deviceState.freqgain_2);

            query_data = AudioEffect_ParamGet(EffectParambin.context_memory,
                                AudioEffect_Parambin_GetEffectAddr(&EffectParambin, EFFECT_MUSICEQ),
                                INDEX_EQ_FILTER3_GAIN);
            g_deviceState.freqgain_3 = convert_eqgain_to_protocol(query_data);
            CMD_DEBUG_LOG("query freqgain_3 gain:%d (0x%04X),g_deviceState.freqgain_3:%d\n", *query_data, *query_data,g_deviceState.freqgain_3);

            query_data = AudioEffect_ParamGet(EffectParambin.context_memory,
                                AudioEffect_Parambin_GetEffectAddr(&EffectParambin, EFFECT_MUSICEQ),
                                INDEX_EQ_FILTER4_GAIN);
            g_deviceState.freqgain_4 = convert_eqgain_to_protocol(query_data);
            CMD_DEBUG_LOG("query freqgain_4 gain:%d (0x%04X),g_deviceState.freqgain_4:%d\n", *query_data, *query_data,g_deviceState.freqgain_4);

            query_data = AudioEffect_ParamGet(EffectParambin.context_memory,
                                AudioEffect_Parambin_GetEffectAddr(&EffectParambin, EFFECT_MUSICEQ),
                                INDEX_EQ_FILTER5_GAIN);
            g_deviceState.freqgain_5 = convert_eqgain_to_protocol(query_data);
            CMD_DEBUG_LOG("query freqgain_5 gain:%d (0x%04X),g_deviceState.freqgain_5:%d\n", *query_data, *query_data,g_deviceState.freqgain_5);

            query_data = AudioEffect_ParamGet(EffectParambin.context_memory,
                                AudioEffect_Parambin_GetEffectAddr(&EffectParambin, EFFECT_MUSICEQ),
                                INDEX_EQ_FILTER6_GAIN);
            g_deviceState.freqgain_6 = convert_eqgain_to_protocol(query_data);
            CMD_DEBUG_LOG("query freqgain_6 gain:%d (0x%04X),g_deviceState.freqgain_6:%d\n", *query_data, *query_data,g_deviceState.freqgain_6);

            // 返回音效参数
             query_data = AudioEffect_ParamGet(EffectParambin.context_memory,
                                AudioEffect_Parambin_GetEffectAddr(&EffectParambin, EFFECT_MUSICEQ),
                                INDEX_EQ_PREGAIN);
            g_deviceState.pregain = convert_eqgain_to_protocol(query_data);
            CMD_DEBUG_LOG("query pregain gain:%d (0x%04X),g_deviceState.pregain:%d\n", *query_data, *query_data,g_deviceState.pregain);

            extern uint8_t AudioMusicVolGet(void);
            g_deviceState.musicVolume = AudioMusicVolGet();
            CMD_DEBUG_LOG("query musicVolume g_deviceState.musicVolume:%d\n", g_deviceState.musicVolume);
            // 返回音效参数
            uint8_t effectData[10] = {
                g_deviceState.musicVolume,
                g_deviceState.musicBass,
                g_deviceState.musicTreble,
                g_deviceState.pregain,
                g_deviceState.freqgain_1,
                g_deviceState.freqgain_2,
                g_deviceState.freqgain_3,
                g_deviceState.freqgain_4,
                g_deviceState.freqgain_5,
                g_deviceState.freqgain_6,
            };
            memcpy(response, effectData, sizeof(effectData));
            *responseLen = sizeof(effectData);
            return true;
        }
        
        default:
            CMD_DEBUG_LOG("Unknown query subCmd=0x%02X\n", subCmd);
            return false;
    }
}

/*=============================================================================
 * 7. LE Audio命令处理器
 *============================================================================*/
bool LeAudioCommandHandler(uint8_t subCmd, const uint8_t* params, uint16_t paramLen,
                            uint8_t* response, uint16_t* responseLen) {
    *responseLen = 0;
    
    switch (subCmd) {
        case LE_SCAN_START:
            CMD_DEBUG_LOG("LE Audio scan start\n");
            // TODO: 开始扫描LE Audio SRC
            return true;
            
        case LE_SCAN_STOP:
            CMD_DEBUG_LOG("LE Audio scan stop\n");
            // TODO: 停止扫描
            return true;
            
        case LE_CONNECT: {
            if (paramLen > 0) {
                // params包含要接入的SRC名称
                CMD_DEBUG_LOG("LE Audio connect to: %s\n", (char*)params);
                // TODO: 接入指定SRC
                return true;
            }
            return false;
        }
        
        case LE_SCAN_INFO:
            // SDK主动反馈扫描信息，不是响应APP请求
            CMD_DEBUG_LOG("LE Audio scan info\n");
            return true;
            
        case LE_CONNECT_RESULT:
            // SDK主动反馈接入结果，不是响应APP请求
            CMD_DEBUG_LOG("LE Audio connect result\n");
            return true;
            
        default:
            CMD_DEBUG_LOG("Unknown LE Audio subCmd=0x%02X\n", subCmd);
            return false;
    }
}
#endif