# B6_Audio_SDK 架构分析与组成说明文档

> **芯片**: 山景 (Mountain View Silicon) B6/B62 系列  
> **SDK版本**: V0.3.16 (2026-08-13)  
> **内核**: Andes NDS32 (AndeSight V3.2.3 工具链)  
> **RTOS**: FreeRTOS  
> **架构特性**: DualCore 双核支持、RoboEffect 音效引擎、多种音频输入输出模式

---

## 1. 总体概述

B6_Audio_SDK 是山景芯片 B6/B62 系列的音频应用 SDK，面向蓝牙音箱、卡拉OK、USB Audio、HDMI ARC、LE Audio 等音频产品开发。SDK 采用 **五层分层架构**，从底层硬件驱动到顶层应用模式，各层职责明确、接口清晰，支持通过宏配置灵活裁剪功能。

### 1.1 架构分层总览

```
┌─────────────────────────────────────────────────────────────────┐
│                     app_src (应用层)                              │
│  main.c / main_task.c / app_mode_* / components / hmi / power   │
│  system_config / startup / tuya_app / mv_app / mv_upgrade        │
├─────────────────────────────────────────────────────────────────┤
│                  app_framework (框架引擎层)                       │
│  mode_engine / audio_engine / bluetooth_engine                   │
│  slow_device_engine / wireless2_engine                            │
├─────────────────────────────────────────────────────────────────┤
│                   middleware (中间件层)                           │
│  rtos / audio / bluetooth / roboeffect / fatfs / otg_host        │
│  dual_core / flashboot / rtc / cec / mv_utils / lrc              │
├─────────────────────────────────────────────────────────────────┤
│                  driver_api (驱动API层)                           │
│  封装driver接口 + 集成OS信号量/队列                                │
├─────────────────────────────────────────────────────────────────┤
│                    driver (硬件驱动层)                            │
│  GPIO / UART / I2C / SPI / DMA / DAC / ADC / I2S / SPDIF         │
│  USB(OTG) / SDIO / Timer / PWM / Watchdog / PMU / IR / Flash     │
└─────────────────────────────────────────────────────────────────┘
```

### 1.2 顶层目录结构

| 目录/文件 | 说明 |
|-----------|------|
| `driver/` | 硬件驱动层：芯片外设寄存器级驱动 |
| `app_framework/` | 应用框架引擎层：模式/音频/蓝牙等服务引擎 |
| `app_src/` | 应用源码层：主入口、各音频模式、功能组件、配置 |
| `middleware/` | 中间件层：RTOS、音频算法、蓝牙协议栈、文件系统等 |
| `audioeffect_res/` | 音效参数资源：audio_effect.bin 等 |
| `remind_res/` | 提示音资源：MP3 提示音文件 |
| `tools/` | 开发工具：烧录、合并、脚本工具 |
| `Release/` | 编译输出目录 |
| `.settings/` `.vscode/` | IDE 配置 |
| `nds32-ae210p.ld` | 链接脚本（每次编译由 .sag 重新生成） |
| `nds32-ae210p.sag` | SAG 链接配置源文件（内存布局定义，prebuild 时被 update_sag.ps1 动态更新） |
| `nds32_template.txt` | nds_ldsag 生成 .ld 所用的模板 |
| `makefile.defs` | 构建系统顶层配置（生成 sys_em.cfg、选择蓝牙库） |
| `docs/` | 本套剖析文档 |
| `log.txt` | 历史启动日志（2026-09-16 抓取，含已进入 Safe Mode 的旧故障记录） |
| `.cproject` `.project` | Eclipse/AndeSight 工程配置 |
| `B6_Audio_SDK_history.txt` | 版本更新历史 |

---

## 2. 硬件驱动层 (driver/)

### 2.1 目录结构

```
driver/
├── driver/
│   └── inc/          ← 芯片外设驱动头文件（预编译库 + 头文件）
└── driver_api/
    ├── inc/          ← 驱动API封装层头文件
    └── src/          ← 驱动API封装层源码
```

### 2.2 driver/driver/inc — 底层寄存器级驱动

该目录包含芯片所有外设的寄存器级驱动头文件，驱动本身以预编译库形式提供。主要模块：

| 头文件 | 功能职责 |
|--------|----------|
| `gpio.h` | GPIO 端口配置（A/B端口，模式/方向/中断） |
| `uarts.h` | UART 串口驱动 |
| `dma.h` | DMA 控制器（DMA0/DMA1 各8通道） |
| `dac.h` | 音频 DAC 驱动（DAC0/DAC1/DAC2） |
| `adc.h` | SAR ADC 驱动（按键检测等） |
| `audio_adc.h` | 音频专用 ADC 驱动（Mic输入） |
| `audio_fifo.h` | 音频 FIFO 管理 |
| `audio_mixer.h` | 音频硬件混音器 |
| `i2s.h` | I2S 音频接口（含 TDM 模式） |
| `spdif.h` | SPDIF 收发驱动 |
| `dsd.h` | DSD 音频支持 |
| `src.h` | 硬件采样率转换 (SRC/SRA) |
| `spi_flash.h` | SPI Flash 驱动（4bit模式，84MHz） |
| `sdio.h` | SDIO 驱动（TF卡接口） |
| `i2c.h` | I2C 主从驱动 |
| `spim.h` `spis.h` | SPI Master/Slave |
| `timer.h` | 硬件定时器（Timer0~5） |
| `pwm.h` | PWM 输出 |
| `pmu.h` | 电源管理单元 |
| `powercontroller.h` | 功耗控制（DCDC/LDO 电压调节） |
| `clk.h` | 时钟控制（DPLL/APLL 配置） |
| `reset.h` | 复位控制 |
| `watchdog.h` | 看门狗 |
| `ir.h` | IR 红外遥控 |
| `keyscan.h` | 按键扫描 |
| `rtc.h` | RTC 实时时钟 |
| `efuse.h` | eFuse 读取 |
| `chip_info.h` | 芯片信息读取 |
| `dualcore.h` | 双核硬件接口 |
| `opi_psram.h` `psram.h` | PSRAM 驱动 |
| `fft.h` | 硬件 FFT 加速 |
| `otg_device_hcd.h` `otg_host_hcd.h` | USB OTG 主机/设备控制器 |
| `otg_detect.h` | USB OTG 检测 |
| `lcdc.h` | LCD 控制器 |
| `touch.h` | 触摸检测 |
| `can.h` | CAN 总线 |
| `remap.h` | 内存 Remap（ILM/TCM 映射） |

### 2.3 driver/driver_api — 驱动API封装层

该层在底层 driver 之上封装了一层 OS 集成接口：
- 封装常用通讯接口（I2C/SPI/UART 等），集成 DMA 传输
- 集成 FreeRTOS 信号量和队列，为上层提供线程安全的驱动调用
- 对上层提供统一 API，屏蔽底层寄存器操作细节

---

## 3. 中间件层 (middleware/)

中间件层提供操作系统、音频算法、通信协议栈等核心服务。

### 3.1 各模块说明

| 子目录 | 职责 | 形式 |
|--------|------|------|
| `rtos/` | RTOS 服务 | 源码 |
| `audio/` | 音频编解码与音效算法库 | 预编译库 + 头文件 |
| `bluetooth/` | 蓝牙协议栈 | 预编译库 + 头文件 |
| `roboeffect/` | RoboEffect 音效引擎库 | 预编译库 + 头文件 |
| `fatfs/` | FatFs 文件系统 | 源码 |
| `otg_host/` | USB OTG 主机栈 | 预编译库 + 头文件 |
| `dual_core/` | 双核通信 (IPC) | 源码 + 库 |
| `flashboot/` | Flash 启动 | 预编译库 + 头文件 |
| `rtc/` | RTC 服务 | 预编译库 (libRTC.a) + 头文件 |
| `cec/` | HDMI CEC | 预编译库 (libcec.a) |
| `mv_utils/` | 通用工具库 | 源码 |
| `lrc/` | 歌词解析 | 源码 |
| `character_encoding/` | 字符编码转换 | 源码/库 |

### 3.2 rtos/ — RTOS 服务

```
middleware/rtos/
├── freertos/          ← FreeRTOS 内核源码
│   ├── include/       ← FreeRTOS 核心头文件
│   └── portable/
│       ├── GCC/NDS32/ ← NDS32 架构移植层 (port.c, portasm.S)
│       └── MemMang/   ← 内存管理 (heap_5.c)
└── rtos_api/          ← RTOS API 封装层
    └── rtos_api.c/h   ← 消息队列/信号量/任务封装
```

- `rtos_api.h` 封装 `MessageSend`/`MessageRecv`/`MessageSendx` 等消息传递 API
- FreeRTOS 配置通过 `FreeRTOSConfig.h` 定制

### 3.3 audio/ — 音频算法库

该目录包含山景自研的音频编解码器和音效算法，以预编译库形式提供。`inc/audio_effect_library.h` 是入口头文件，集中包含所有音效算法。

**主要算法类别**：

| 类别 | 代表算法 | 说明 |
|------|----------|------|
| **均衡器** | `eq.h` `dynamic_eq.h` | 音乐/麦克风 EQ |
| **混响** | `reverb.h` `reverb_plate.h` `reverb_pro.h` `plate_reverb.h` | 多种混响效果 |
| **回声** | `echo.h` `pcm_delay.h` | 回声效果 |
| **变声** | `pitch_shifter.h` `pitch_shifter_pro.h` `voice_changer.h` `voice_changer_pro.h` | 音高/声调变换 |
| **降噪** | `blue_ns.h` `blue_ns_adv.h` `blue_ns_dual.h` `noise_gate.h` | 蓝牙降噪/噪声门 |
| **AEC** | `blue_aec.h` `beamforming.h` | 回声消除/波束成形 |
| **消原唱** | `vocalcut.h` `vocal_remover.h` | 人声消除 |
| **动态处理** | `drc.h` `compander.h` `expander.h` `howling_guard.h` | 压缩/限制/啸叫抑制 |
| **调制效果** | `chorus.h` `flanger.h` `tremolo.h` `vibrato.h` `phaser.h` | 合唱/镶边/颤音 |
| **失真** | `distortion_ds1.h` `overdrive.h` | 失真/过载 |
| **采样率转换** | `resampler.h` `resampler_sinc.h` `resampler_polyphase.h` | 软件重采样 |
| **编解码** | `sbcenc_api.h` `mp3enc_api.h` `lc3enc.h` `opus_codec.h` `adpcm_codec.h` | 音频编解码 |
| **3D/虚拟低音** | `three_d.h` `virtual_bass.h` `virtual_surround.h` `stereo_widener.h` | 空间音效 |
| **其他** | `howling_suppressor.h` `silence_detector.h` `vad.h` `auto_tune.h` `auto_wah.h` | 啸叫抑制/静默检测/自动调音 |

### 3.4 bluetooth/ — 蓝牙协议栈

```
middleware/bluetooth/
├── inc/    ← 协议栈 API 头文件
└── src/    ← 协议栈适配源码
```

**主要 API 模块**：

| 头文件 | 职责 |
|--------|------|
| `bt_manager.h` | 蓝牙总管理器，管理所有蓝牙事件和 API |
| `bt_interface.h` | 蓝牙统一接口 |
| `bt_a2dp_api.h` | A2DP 高级音频分发协议 |
| `bt_avrcp_api.h` | AVRCP 音视频远程控制协议 |
| `bt_hfp_api.h` `bt_hfg_api.h` | HFP 免提通话协议 |
| `bt_tws_api.h` | TWS 真无线立体声 |
| `bt_spp_api.h` `bt_user_spp_api.h` | SPP 串行端口协议 |
| `bt_hid_api.h` | HID 人机接口设备 |
| `bt_obex_api.h` | OBEX 对象交换（用于 OTA 升级） |
| `bt_mfi_api.h` | MFi (Made for iPod/iPhone) |
| `bt_pbap_api.h` | PBAP 电话簿访问 |
| `bt_common_api.h` | 蓝牙通用 API |
| `bb_api.h` | Baseband 基带 API |
| `bt_visibility_manager.h` | 蓝牙可见性管理 |

### 3.5 roboeffect/ — RoboEffect 音效引擎

```
middleware/roboeffect/
├── roboeffect_lib/
│   ├── wraps/        ← 音效封装（motor_wrap, motor_api, motor_data）
│   └── roboeffect_api.h  ← 效果API枚举（EQ/REVERB/PITCH_SHIFTER/AEC等）
├── third_party_effect/  ← 第三方音效
└── documents/        ← 文档
```

RoboEffect 是山景的音效图 (Flow Graph) 引擎，通过 `audio_effect.bin` 配置文件定义音频效果链路（Source → Effect → Sink），支持双引擎模式。

### 3.6 其他中间件

| 模块 | 说明 |
|------|------|
| `fatfs/` | FatFs 文件系统，支持 U盘/TF卡文件读写 |
| `otg_host/` | USB OTG 主机栈，支持 UAC 音频设备 |
| `dual_core/` | 双核 IPC 通信，`dualcore_api.h` 提供 IPC 读写、命令发送、信号量同步 |
| `flashboot/` | Flash 启动引导，支持 USB OTA 升级 |
| `rtc/` | RTC 服务，提供闹钟、农历、定时器功能 (libRTC.a) |
| `cec/` | HDMI CEC 消费电子控制 (libcec.a) |

---

## 4. 应用框架引擎层 (app_framework/)

该层提供服务级引擎，封装中间件能力为可管理的服务任务。

### 4.1 各引擎说明

| 子目录 | 核心文件 | 职责 |
|--------|----------|------|
| `mode_engine/` | `mode_task.c/h` | **模式引擎**：管理系统音频模式切换（BT/USB/LineIn/HDMI等），驱动模式Init/Deinit/Run生命周期 |
| `audio_engine/` | `audio_core_service.h` `effect_service.h` `decoder.h` `recorder_service.h` `remind_sound.h` `communication.h` `beep.h` | **音频引擎**：管理音频核心服务（创建/启动/暂停/恢复/停止）、效果链路、解码器、录音、提示音 |
| `bluetooth_engine/` | `bt_stack_service.c/h` `bt_engine_audio.c/h` `bt_engine_utility.c/h` | **蓝牙引擎**：蓝牙协议栈服务管理、蓝牙音频通路处理 |
| `slow_device_engine/` | `slow_device_task.c/h` | **慢速设备引擎**：处理慢速外设事件（仅在系统模式Running状态下运行） |
| `wireless2_engine/` | `wireless2.h` `wireless2bb_api.h` `audio_association.h` | **无线2引擎**：2.4G无线音频关联管理 |

### 4.2 模式引擎核心机制

模式引擎是 SDK 的核心调度框架：

- `SysModeNumber` 枚举定义所有音频模式（`ModeBtAudioPlay`, `ModeUsbDevicePlay`, `ModeLineAudioPlay`, `ModeHdmiAudioPlay`, `ModeSafe` 等）
- `SysModeStruct` 定义每个模式的 Init/Deinit/Run 回调和初始状态
- `mode_parameter.h` 中的 `SysMode[]` 数组注册所有模式及其函数指针
- 模式切换通过消息驱动：`SysModeGenerate()` → `SysModeChangeTimeoutProcess()`
- 每个 `app_mode_*` 目录实现对应模式的 Init/Deinit/Run 函数

### 4.3 音频引擎核心机制

- `AudioCoreServiceCreate()` / `AudioCoreServiceStart()` — 创建和启动音频核心服务
- `EffectService` — 管理音效处理链路
- `Decoder` — 音频解码管理
- `RecorderService` — 录音服务
- `RemindSound` — 提示音播放
- `Communication` — 在线调试通信（USB HID/UART）

---

## 5. 应用源码层 (app_src/)

### 5.1 目录结构总览

```
app_src/
├── main.c              ← 程序入口：硬件初始化、时钟、电源、RTCM启动
├── main_task.c/h        ← 主任务：系统初始化、消息循环、模式调度
├── app_mode/            ← 模式框架：消息定义、模式参数表、模式API
├── app_mode_bt/         ← 蓝牙音频模式实现
├── app_mode_media/      ← 媒体播放模式（U盘/TF卡）
├── app_mode_usb_audio/  ← USB Audio 设备模式
├── app_mode_i2s/        ← I2S 输入模式
├── app_mode_hdmi/       ← HDMI ARC 输入模式
├── app_mode_spdif/      ← SPDIF 光纤/同轴模式
├── app_mode_radio/      ← FM 收音模式
├── app_mode_linein/     ← LineIn 模式
├── app_mode_idle/       ← 空闲/待机模式
├── app_mode_safe/       ← 安全模式（异常恢复）
├── app_mode_leaudio_sink/ ← LE Audio Sink 模式
├── audio_common/        ← 音频公共功能（低延迟、DAC Mix、USB Mix、SPDIF Out）
├── components/          ← 功能组件库
├── hmi/                 ← 人机交互（按键、显示、IR遥控）
├── power/               ← 电源管理（DeepSleep、PowerKey、功耗监控）
├── mv_app/              ← MVA 应用协议（PC在线调试命令处理）
├── mv_upgrade/          ← 固件升级管理
├── startup/             ← 启动代码（crt0.S, interrupt.c, retarget.c）
├── system_config/       ← 系统配置头文件
├── tuya_app/            ← 涂鸦 BLE 应用层
└── tuya_sdk/            ← 涂鸦 BLE SDK 集成
```

### 5.2 系统配置 (system_config/)

这是 SDK 的配置中心，通过宏定义控制功能裁剪：

| 配置文件 | 职责 |
|----------|------|
| `app_config.h` | **应用配置总入口**：`CFG_APP_*` 模式开关、`CFG_FUNC_*` 功能开关、`CFG_RES_*` 硬件资源开关、音频通路/帧参数/音效配置 |
| `chip_config.h` | 芯片选型配置（`CFG_CHIP_SEL`，封装信息） |
| `bt_config.h` | 蓝牙配置（设备名、地址、协议支持、TWS、LE Audio等） |
| `clock_config.h` | 时钟配置（晶体频率、PLL频率、USB时钟） |
| `gpio_config.h` | GPIO 引脚分配 |
| `flow_config.h` | **音效图配置**：定义音效Flow列表、Source/Sink枚举映射、效果名/预设枚举 |
| `module_config.h` | 模块功能开关聚合 |
| `resource_defs.h` | 资源地址定义（Flash分区地址、FIFO大小等） |
| `sys_em_config.h` | 系统仿真配置 |
| `mvwireless2_config.h` | 2.4G无线配置 |

**配置宏命名规范**：
- `CFG_APP_*` — 模式列表选择（启用/禁用某音频模式）
- `CFG_FUNC_*` — 功能控制开关
- `CFG_PARA_*` — 参数配置
- `CFG_RES_*` — 硬件资源开关

### 5.3 各音频模式说明

| 模式目录 | 模式名 | 核心功能 |
|----------|--------|----------|
| `app_mode_bt/` | BtPlay | 蓝牙A2DP音频播放、HFP免提通话、TWS真无线 |
| `app_mode_media/` | UDisk/CardPlay | U盘/TF卡媒体播放（MP3/WMA/WAV/SBC解码，文件浏览） |
| `app_mode_usb_audio/` | UsbDevicePlay | USB Audio Device（UAC1/UAC2，HS/FS） |
| `app_mode_i2s/` | I2SInPlay | I2S 数字音频输入 |
| `app_mode_hdmi/` | HdmiAudioPlay | HDMI ARC 音频输入 + CEC控制 |
| `app_mode_spdif/` | Optical/CoaxialPlay | SPDIF 光纤/同轴输入 |
| `app_mode_radio/` | RadioPlay | FM 收音（RDA5807/QN8035芯片） |
| `app_mode_linein/` | LinePlay | 模拟 LineIn 输入 |
| `app_mode_idle/` | Idle | 空闲待机/DeepSleep/PowerDown |
| `app_mode_safe/` | Safe | 安全模式（其他模式初始化失败时进入） |
| `app_mode_leaudio_sink/` | LeAudioSinkPlay | BLE LE Audio BIS/CIS Sink |

### 5.4 components/ — 功能组件库

| 子目录 | 职责 |
|--------|------|
| `audio/` | 音频效果控制（`audio_effect_control.h`, `audio_effect_parambin.h`, `audio_vol.h`, `ctrlvars.h`） |
| `ble/` | BLE 应用层（`ble_api.h`, `ble_gs_app.h`, `ble_app_gatt.h`, GATT服务） |
| `bluetooth/` | 蓝牙应用层（A2DP/AVRCP/HFP/TWS处理，连接管理，设备数据库） |
| `dualcore/` | 双核 DSP 通信（`dualcore_dsp.h`） |
| `flash_manage/` | Flash 管理（断点记忆 `breakpoint.h`、Flash参数 `flash_param.h`、Flash分区表 `flash_table.h`、PC在线参数 `pc_online_param.h`） |
| `otg/` | OTG USB 管理 |
| `rtc/` | RTC 应用层 |
| `soft_watchdog/` | 软件看门狗（多任务监控） |
| `upgrade/` | 固件升级组件 |

### 5.5 hmi/ — 人机交互

| 文件 | 功能 |
|------|------|
| `key.h` `adc_key.h` `code_key.h` `io_key.h` `ir_key.h` `ir_nec_key.h` | 按键检测（ADC按键/IO按键/IR遥控/编码器） |
| `adc_levels.h` | ADC 按键电平分级 |
| `display.h` `seg_led_disp.h` `seg_panel.h` | 数码管/段码显示 |
| `pwm_dma_Led.h` | PWM DMA LED 控制 |
| `QN8035.h` `RDA5807.h` `radio_api.h` | FM 收音芯片驱动 |
| `device_detect.h` | 设备检测（USB/SD/LineIn/HDMI） |
| `tempCorr.h` | 温度校正 |

### 5.6 power/ — 电源管理

| 文件 | 功能 |
|------|------|
| `deepsleep.c/h` | DeepSleep 模式管理（低功耗睡眠/唤醒） |
| `pmu_powerkey.c/h` | PMU 电源按键处理 |
| `power_monitor.c/h` | 功耗监控 |
| `soft_power.c` | 软电源开关 |

### 5.7 startup/ — 启动代码

| 文件 | 功能 |
|------|------|
| `crt0.S` | C 运行时启动汇编（复位向量、栈初始化、调用main） |
| `interrupt.c` | 中断控制器初始化 |
| `retarget.c` | printf/scanf 重定向（UART输出） |
| `init-default.c` | 默认初始化 |

---

## 6. 启动流程分析

SDK 启动遵循以下流程。本节按源码逐行整理（基于 [main.c](file:///d:/MVS_err/workspace/B6_Audio_SDK/app_src/main.c) `main()` 与 [main_task.c](file:///d:/MVS_err/workspace/B6_Audio_SDK/app_src/main_task.c) `SystemInit()`），**不省略任何步骤**；标注 `[条件: XXX]` 的步骤仅在对应宏启用时才参与编译。

### 6.1 FlashBoot 阶段（芯片内置固件）

```
上电/复位
  └─ FlashBoot（芯片出厂固化，非本SDK源码）
      ├─ 读取 CHIP_ID、检测 TCM/SRAM
      ├─ check_user_code：CRC 校验用户代码区（log: "Crc2 0x0000C787 Crc 0x0000C787"）
      └─ CRC 通过 → jump_to_code_addr 跳转到用户代码入口（0x10000）
```

### 6.2 crt0.S 运行时初始化

- 位置：[startup/crt0.S](file:///d:/MVS_err/workspace/B6_Audio_SDK/app_src/startup/crt0.S)
- 职责：设置栈指针、清零 .bss、拷贝 .data 初始值、建立中断向量表，最后调用 `main()`

### 6.3 main() 硬件初始化阶段（完整步骤）

| # | 调用 | 作用 | 备注 |
|---|------|------|------|
| 1 | `Reset_FlagGet()` | 读取复位原因（上电/按键/看门狗/LVD等），存入 `RstFlag` | 后续打印和 RTC 初始化使用 |
| 2 | `Chip_Init()` | 芯片基础初始化（系统寄存器、模块复位释放等） | driver 库函数 |
| 3 | `WDG0_Enable(WDG_STEP_4S)` | 使能硬件看门狗，4 秒超时 | 防死机 |
| 4 | `Reset_FlagClear()` | 清除复位标志寄存器 | |
| 5 | `LDO11_Switch2DCDC11()` + `Power_ConfigDCDC11(1V23)` | **第一次核心电压配置**：LDO 切换为 DCDC，先给固定 1.23V | `[条件: CFG_DCDC11_EN]`，否则走 LDO 分支 |
| 6 | `GPIO_PortAModeSet(GPIOA0/A1, 0)` | 将 A0/A1 的 SW 调试功能改为普通 GPIO | `[条件: !CORE2_SW_PORT_EN]` |
| 7 | `Power_LVDThresholdSel(2V5)` | 设置低电压检测（LVD）阈值为 2.5V | 掉电保护 |
| 8 | `Power_AVDD33Enable()` | 使能模拟 3.3V 电源（音频模拟电路供电） | |
| 9 | **`PMU_NVMInit()`** | **NVM（eFuse/保持寄存器区）初始化，供断点等数据保存到 NVM** | `[条件: BP_PART_SAVE_TO_NVM]`，默认未启用 |
| 10 | `SystemClockInit()` | 时钟树初始化：HOSC→DPLL1(336MHz 档)→系统时钟 168MHz，APLL 锁频，各模块时钟门控，USB/UART/SPDIF 时钟选择 | 详见 `SystemClockInit()` |
| 11 | `DBUS_Access_Area_Init(0)` | 配置 DataBus 可访问范围为 code size | `[条件: CFG_DBUS_ACCESS_EN]` |
| 12 | `Remap_InitTcm(start, sram_addr, tcm_kb)` | TCM/ILM 内存重映射；双核时需为 Core2 预留 `BB_EM_SIZE + CORE2_RAM_SIZE` | 双核/单核参数不同 |
| 13 | `SpiFlashInit(84MHz, MODE_4BIT, 0, 1)` | SPI Flash 初始化：84MHz、4bit 模式 | |
| 14 | `SpiFlashIOCtrl(IOCTL_FLASH_PROTECT, FLASH_LOCK_RANGE_ONE_QUARTER)` | **Flash 写保护**：容量 ≤4MB 时锁定前 1/4 区域（保护代码区） | 防止参数写操作误擦代码 |
| 15 | `LogUartConfig(TRUE)` | 调试串口初始化（GPIO 复用 + 波特率） | `[条件: CFG_UART_LOG_EN]` |
| 16 | `usb_hid_printf_init()` | USB HID 通道的 printf 调试输出 | `[条件: CFG_FUNC_USBDEBUG_EN]` |
| 17 | `SarADC_Init()` | SAR ADC 初始化（ADC 按键/电压检测用） | |
| 18 | `prvInitialiseHeap()` | FreeRTOS 堆（heap_5）初始化 | 必须在创建任何 OS 对象之前 |
| 19 | `osSemaphoreMutexCreate()` | 创建日志打印用的 OS 互斥锁 | 注释明确：必须在堆初始化之后、串口打印之前 |
| 20 | **第二次 DCDC 配置** `Power_ConfigDCDC11(DCDCVoltageConfig())` | **按当前实际主频动态查表调压**（`DCDCVoltageConfig()` 根据 408/384/336/288/192MHz 分档返回电压值） | 与第 5 步的区别：第 5 步是"上电保底电压"，本步是"按主频精细调压" |
| 21 | `LDO16_Switch2DCDC16()` + `Power_ConfigDCDC16(1V4/1V8)` | DCDC16/LDO16 配置；有 `CHIP_PIN_PA_VDD` 封装时给 1.4V（供 RF PA），否则 1.8V | `[条件: CFG_DCDC16_EN]` |
| 22 | `SWD_init(5000)` | 软件看门狗初始化（5 秒，监控多任务存活） | `[条件: SOFT_WACTH_DOG_ENABLE]` |
| 23 | `APP_DBG` 打印复位原因 + SDK 版本横幅 + 各库版本（Driver/OtgHost/BtLib） | 启动日志，`Rst:0x2-Power/Pin` 等 | 对应 log 中的版本信息段 |
| 24 | `ILM_Remap32K(&__ilm_lmastart)` | Core1 ILM 32K 重映射使能（log: "Enable ILM at addr 0x00028000"） | `[条件: CORE1_ILM_REMAP_EN]`，当前 B26X128 已启用 |
| 25 | `NVIC_EnableIRQ(SWI_IRQn)` + `GIE_ENABLE()` | 使能软中断 + **开全局中断** | 注释强调：此后才能调用 OS API |
| 26 | `flash_table_init()` | Flash 型号分区表初始化（log: "flash table name is error!" 即此处打印） | |
| 27 | 双核启动序列：`UartInit_Core2()` → `DualCore_ReleaseCore2ToRun(&__sdk_code2_start)` → `DualCoreModuleInit()` → `DualCoreCore2UARTInit()` | 释放 Core2 到指定地址运行，建立双核 IPC | `[条件: DUAL_CORE_EN]` |
| 28 | `IdleModeConfig()` | 空闲/待机模式参数配置 | `[条件: CFG_APP_IDLE_MODE_EN]` |
| 29 | `sys_parameter_init()` | 从 Flash 加载系统参数（sys_parameter 区） | `[条件: BT_SUPPORT \|\| BLE_SUPPORT]` |
| 30 | `bt_em_size_init()` | 计算并校验蓝牙 EM 内存占用（log: "bt em size:20KB"），超界则死循环报错 | `[条件: CFG_APP_BT_MODE_EN \|\| CFG_APP_BLE_MODE_EN]` |
| 31 | `RTC_ServiceInit(RstFlag&0x01)` + `RTC_IntEnable()` + `NVIC_EnableIRQ(RTC_IRQn)` | RTC 服务初始化并使能中断；入参为上电复位标志 | `[条件: CFG_FUNC_RTC_EN]` |
| 32 | `__nds32__mtsr(0, NDS32_SR_PFMC0)` + `__nds32__mtsr(1, NDS32_SR_PFM_CTL)` | **配置 NDS32 性能计数器**，用于 idleTask 中统计 Mcps（log 中 "CPU:14 M" 的来源） | NDS32 架构特有 |
| 33 | Timer3 配置 + `NVIC_EnableIRQ(TMR3_DMAC1DINT_IRQn)` | LED 刷新定时器（优先级 0 最高，跑在 TCM 段） | `[条件: CFG_FUNC_LED_REFRESH]` |
| 34 | `DispInit(0)` | 显示屏初始化 | `[条件: CFG_FUNC_DISPLAY_EN]` |
| 35 | `DbgPortInit()` | 调试 IO 翻转端口初始化（逻辑分析仪测执行时间用） | `[条件: DBG_IO_TGL]` |
| 36 | `UartLogUseTimer(1)` | 日志输出改由 Timer 驱动（减轻中断负担） | `[条件: CFG_FUNC_DEBUG_USE_TIMER]` |
| 37 | `MainAppTaskStart()` | 创建主任务 MainAppTask（栈 640 words，优先级 4）+ Shell 任务（条件 `CFG_FUNC_SHELL_EN`） | |
| 38 | `vTaskStartScheduler()` | 启动 FreeRTOS 调度器，此后不再返回 | |

### 6.4 MainAppTaskEntrance() → SystemInit() 阶段（完整步骤）

| # | 调用 | 作用 | 备注 |
|---|------|------|------|
| 1 | `DelayMsFunc = osTaskDelay` | 延时函数指针挂接到 OS 延时（进入 OS 环境后不用裸 Delay） | 高分辨率无损解码时除外 |
| 2 | `DMA_ChannelAllocTableSet(DMA0/DMA1, map)` | 设置 DMA0/DMA1 通道分配表（常驻设备优先，模式设备靠后） | |
| 3 | `DumpUartConfig(TRUE)` | Dump 串口配置 | `[条件: CFG_DUMP_DEBUG_EN]` |
| 4 | `SarADC_Init()` | 再次初始化 SAR ADC（确保状态） | |
| 5 | `TemperatureCorrInit()` | 温度校正值加载（log: "trim erased (FT not done)" 即此处） | 未过 FT 时直接返回 |
| 6 | `SystemTimerInit()` | 配置 Timer2 为 **1ms 系统心跳**（`Timer2_Interrupt`：OTG 检测、USB 音频 1ms 处理、HFP 1ms、软看门狗喂狗等） | |
| 7 | `IsCore2Ready()` | 等待 Core2 就绪 | `[条件: DUAL_CORE_EN]` |
| 8 | `BP_LoadInfo()` | 从 Flash 加载断点信息（上次关机时的模式/音量/音效等，log: "Load BP INFO START / no Magic"） | `[条件: CFG_FUNC_BREAKPOINT_EN]` |
| 9 | `SysVarInit()` | 系统变量初始化：从断点恢复音量/EQ/混响步进（带 `BP_PARAM_SAFE_MAX` 越界保护）、生成开机模式（`PowerOnModeGenerate`） | |
| 10 | `WirelessInit()` | 2.4G 无线模块初始化 | `[条件: FUNC_WIRELESS2_SUPPORT]` |
| 11 | SoftFlag 清除 | 清除升级成功等软件标志 | |
| 12 | `TWS_Params_Init()` 或 `TWS_TwsEnable(0)` | TWS 参数初始化 | `[条件: BT_TWS_SUPPORT]` |
| 13 | `HDMI_CEC_DDC_Init()` | HDMI CEC/DDC 初始化 | `[条件: CFG_APP_HDMIIN_MODE_EN]` |
| 14 | `memset(&AudioCore, 0, ...)` | 清零 AudioCore 上下文 | |
| 15 | `CtrlVarsInit()` | **音频控制变量初始化 + 音频系统硬件参数初始化**（gain、PGA、DAC/ADC 默认参数等） | |
| 16 | `I2SDpllPolicy_Init()` | I2S DPLL 时钟策略初始化 | `[条件: CFG_FUNC_I2S_DPLL_POLICY_EN]` |
| 17 | `OTG_FS/HS_PortSetDetectMode()` + `NVIC_EnableIRQ(USB_HS_IRQn)` | OTG FS/HS 端口插拔检测模式配置 | 按 `CFG_PARA_HOST/DEVICE_xx_DET` |
| 18 | `UsbHidHardwareint()` | 纯 HID 通信场景（无 detect 无 host）的 USB 硬件初始化 | 条件组合编译 |
| 19 | `AudioCoreServiceCreate(mainAppCt.msgHandle)` | **创建音频核心服务**（音频通路调度的中枢） | 创建成功后回调 `MSG_AUDIO_CORE_SERVICE_CREATED` → `AudioCoreServiceStart()` → `SysModeTaskCreat()` 创建模式引擎任务 |
| 20 | `RemindSoundInit()` | 提示音服务初始化 | `[条件: CFG_FUNC_REMIND_SOUND_EN]` |
| 21 | `BtStackServiceStart()` | 启动蓝牙协议栈服务（log: "bluetooth stack service init."） | 条件：`bt_BackgroundType != BT_BACKGROUND_DISABLE` |
| 22 | `CAN_FuncInit()` | CAN 总线 demo | `[条件: CFG_FUNC_CAN_DEMO_EN]` |
| 23 | `DeviceServiceInit()` | 设备检测服务初始化（U盘/SD/LineIn/HDMI 插拔检测） | |
| 24 | `BlePowerOn()` | BLE 上电 | `[条件: CFG_APP_BLE_MODE_EN]` |

### 6.5 消息主循环 while(1)

```
while(1) {
  ├─ TemperatureMonitor() + vmid_Dynamic_change()  — 每3秒测温，按温度动态调整 VMID（偏压）
  ├─ MessageRecv(mainAppCt.msgHandle, 10ms)         — 接收消息（超时10ms）
  ├─ PublicDetect()                                 — 静音自动关机等公共检测
  ├─ PublicMsgPross(msg)                            — 公共消息处理（电源/蓝牙/升级/无线等）
  ├─ I2SDpllPolicy_Process()                        — I2S 时钟策略轮询 [条件]
  ├─ upgrade_monitor() / upgrade_end_reboot()       — 升级监控 [条件: MVAPP_UPDATA_EN]
  ├─ little_dog_feed() + WDG0_Feed()                — 软件看门狗 + 硬件看门狗喂狗
  ├─ DeviceServicePocess(msgId)                     — 设备插拔处理
  ├─ SysModeGenerate(msgId)                         — 模式切换判定
  │    └─ AudioEffect_Msg_Check() 过滤音效消息 → MessageSend(GetSysModeMsgHandle())
  └─ SysModeChangeTimeoutProcess()                  — 模式切换超时处理
}
```

### 6.6 启动流程要点（设计意图）

1. **FlashBoot 阶段**：芯片内置 FlashBoot 固件先运行，校验用户代码 CRC，通过后跳转到用户代码
2. **两次电压配置**：`main()` 开头先用固定值保底（第 5 步），时钟初始化完成后再按实际主频精细调压（第 20 步）；这是为了在主频未确定前保证系统能稳定跑到 `SystemClockInit()`
3. **顺序敏感**：`prvInitialiseHeap()`（18）→ `osSemaphoreMutexCreate()`（19）→ 日志打印（23），此顺序不可调换；`GIE_ENABLE()`（25）之前不允许调用任何 OS API
4. **双核启动**：Core1 先启动，通过 `DualCore_ReleaseCore2ToRun()` 释放 Core2
5. **模式进入**：`SysVarInit()` 中的 `PowerOnModeGenerate()` 根据断点恢复上次模式（默认 BtPlay）；若模式初始化失败（如 audio_effect.bin 校验失败）则降级进入 Safe 模式
6. **消息驱动**：主任务通过消息队列接收事件，驱动模式切换和功能处理

### 6.7 关于旧版本简图的说明

本节之前的简版流程图（仅列出 Chip_Init / SystemClockInit / SpiFlashInit 等十余步）是**刻意做的"主干提炼"**，省略了条件编译步骤（`PMU_NVMInit`、`DBUS_Access_Area_Init`、`usb_hid_printf_init` 等默认未启用）与细节步骤（两次电压配置、Flash 写保护、性能计数器等），目的是让读者先建立整体印象。上面的 6.3/6.4 两节表格是与源码一一对应的完整版本，分析具体启动问题时请以表格为准。

---

## 7. 构建系统

### 7.1 工具链与IDE

- **IDE**: AndeSight V3.2.3（基于 Eclipse 的 NDS32 开发环境）
- **编译器**: NDS32 GCC（交叉编译前缀 `nds32le-elf-`，Andes 专用 RISC-V 类架构）
- **构建配置**: `.cproject` 文件定义编译选项、包含路径、链接选项
- **构建目录**: `Release/`（makefile 由 IDE 自动生成于该目录）
- **最终烧录文件**: `Release/output/main_merge.MVA.bin`

### 7.2 链接脚本

| 文件 | 作用 |
|------|------|
| `nds32-ae210p.sag` | SAG 源文件：定义 NDS32 内存布局（EILM/EXEC_CODE/ILM_Sec/SDRAM/栈地址/LMA对齐） |
| `nds32-ae210p.ld` | 由 `nds_ldsag` 工具从 .sag 生成的最终链接脚本，声明 `ENTRY(_start)` 和 `SECTIONS` |
| `nds32_template.txt` | `nds_ldsag` 生成 .ld 时使用的模板文件 |
| `tools/update_sag.ps1` | prebuild 时根据 `Release/sys_em.cfg` 中的宏值动态改写 .sag 的 PowerShell 脚本 |

**链接脚本生成链路**：`sys_em_config.h` →（GCC 预处理）→ `sys_em.cfg` →（update_sag.ps1）→ `nds32-ae210p.sag` →（nds_ldsag + 模板）→ `nds32-ae210p.ld`。每次编译都会重新生成，因此修改 `sys_em_config.h` 中的内存相关宏后无需手动维护 .ld。

**内存布局要点**：
- ILM (Instruction Local Memory)：指令局部内存，通过 Remap 映射到 TCM 地址
- TCM_SRAM：紧耦合内存 SRAM（启动日志打印 TCM_SIZE = 8 KB @ 0x20000000）
- Flash 代码起始地址：0x00010000

### 7.3 编译流程（实际验证）

以下流程基于 2026-09-22 成功编译的实际验证（编译环境：AndeSight V3.2.3，Build Configuration = Release）：

```
0. makefile.defs（make 解析阶段最先执行）
   ├─ nds32le-elf-gcc -E -dM -P sys_em_config.h > Release/sys_em.cfg
   │   （对系统配置头文件做宏展开，生成纯宏列表供脚本读取）
   └─ 读取 sys_em.cfg 中的 USE_BT_*_LIB 宏，选择蓝牙协议栈库变体
      （8 选 1：Std/Bis/Tws/Tws_NoBle/BisAndTws/noBle/Ble/Cis）
      当前配置：USE_BT_NO_BLE_LIB → libBtStack_noBle.a 复制为 libBtStack.a

1. prebuild：tools/script.bat
   ├─ 删除旧 nds32-ae210p.ld
   ├─ update_sag.ps1：按 sys_em.cfg 动态更新 nds32-ae210p.sag
   ├─ nds_ldsag：由 nds32_template.txt + .sag 重新生成 nds32-ae210p.ld
   ├─ 提示音构建：ffmpeg.exe 将 remind_res/*.mp3 转为 .mp2
   │   （16K/ 子目录按 16kHz/24kbps/单声道，其余按 48kHz 立体声码率转换），
   │   MergeAudio2BinNew.exe 合并为 tools/remind_script/all.bin，
   │   并生成 sound_remind_item.h；有变化时同步到
   │   app_framework/audio_engine/remind_sound_item.h
   ├─ copyLibs.exe -s tools/script_copy.ini
   │   （注：当前工程未提供 script_copy.ini，此步实际为空操作；
   │    库选择已由 makefile.defs 完成）
   └─ 按 CFG_DOUBLE_KEY_EN 宏尝试生成 BP16x.ld（bootloader 用，
      与应用固件构建无关；当前工程缺对应 .sag，静默跳过）

2. GCC 编译（nds32le-elf-gcc -Og）
   ├─ 编译 driver/driver_api/src
   ├─ 编译 middleware/ 源码 (freertos, rtos_api, fatfs, roboeffect wraps等)
   ├─ 编译 app_framework/ 源码
   └─ 编译 app_src/ 源码
   （各目录参与编译的文件清单见 Release/ 下自动生成的 sources.mk
    与 */subdir.mk）

3. 链接（Andes C Linker）→ Release/B6_Audio_SDK.adx
   ├─ 链接参数：-Og -nostartfiles -static -mcmodel=medium
   │   -T nds32-ae210p.ld -Wl,--gc-sections -mvh
   └─ 链接 25 个预编译库（Release/objects.mk 中 LIBS 变量）：
      -lDriver -lLrc -ltuya_ble_sdk_v2.0.0_B6 -lmp2enc -lmp3enc
      -lAudioDecoderLibrary -lfatfs -lRoboeffect -lAudioEffectLibrary
      -lVocalCut -lBeamforming -lcec -luac -lresampler -lBtStack
      -lwireless2 -laudio_association -lLC3mv -lLC3Enc -lcvsd_plc
      -lsbcenc -lplc128 -lRTC -ldsp -lm

4. post-build（makefile 内嵌 + tools/script_post.bat）
   ├─ objcopy -O binary：B6_Audio_SDK.adx → output/B6_Audio_SDK.bin
   ├─ objcopy：flash_param.o → tools/merge_script/flash_param.bin
   └─ script_post.bat：
       ├─ 读取 app_config.h 中 CFG_EFFECT_PARAM_IN_FLASH_EN、
       │   CFG_FUNC_REMIND_SOUND_EN 两个宏作为 merge.exe 参数
       ├─ merge.exe：按 merge.ini 合并 6 个分区（main 代码、bp_data、
       │   bt_data、remind 提示音、effect_data 音效参数、sys_parameter），
       │   生成整片 Flash 镜像 flash.img（4MB）
       └─ Andes_MVAGenerate.exe：按 MV_SDKConfig.ini 打包生成
           main_merge.MVA 与 main_merge.MVA.bin（烧录/升级文件，
           文件名带 CRC 后缀，如 main_merge_0x8bcd.MVA）

5. secondary-outputs（分析产物，自动生成）
   ├─ nm      → output/symbol.txt    （符号表）
   ├─ readelf → output/readelf.txt   （ELF 信息）
   ├─ objdump → output/objdump.txt   （反汇编）
   └─ size    → output/.PHONY.size   （段尺寸统计）
```

### 7.4 编译产物清单

成功编译后 `Release/` 下的产物（2026-09-22 实际验证）：

| 文件 | 大小 | 说明 |
|------|------|------|
| `Release/B6_Audio_SDK.adx` | ~4.7 MB | ELF 可执行文件（含调试信息，用于 objdump/readelf/调试器） |
| `Release/output/B6_Audio_SDK.bin` | ~1.39 MB | 纯代码段二进制（objcopy 自 .adx），合并工具的输入 |
| `Release/output/main_merge.MVA.bin` | ~1.65 MB | **最终烧录文件**（MVA 格式，含代码+参数+CRC 校验） |
| `Release/output/main_merge_0x8bcd.MVA` | ~1.65 MB | 同上，文件名带 CRC 后缀版本（0x8bcd 随内容变化） |
| `Release/output/flash.img` / `flash.img.bin` | 4 MB | 整片 Flash 镜像（含全部分区，用于烧录器整片烧录） |
| `Release/output/symbol.txt` | ~384 KB | nm 符号表 |
| `Release/output/readelf.txt` | ~850 KB | ELF 头/段信息 |
| `Release/output/objdump.txt` | ~14 MB | 完整反汇编 |
| `Release/output/.PHONY.size` | <1 KB | 段尺寸统计 |

**固件尺寸**（`.PHONY.size` 实测）：

| 段 | 字节数 | 说明 |
|----|--------|------|
| text | 1,441,946 | 代码 948,094 + 只读数据 493,852 |
| data | 3,696 | 已初始化数据 |
| bss | 123,464 | 未初始化数据 |
| 合计 | 1,569,106 (0x17F152) | |

### 7.5 makefile.defs

构建系统顶层配置（在 make 解析阶段执行，先于所有编译动作），负责：
- 用 GCC 预处理 `app_src/system_config/sys_em_config.h` 生成 `Release/sys_em.cfg`（纯宏定义列表，供 update_sag.ps1 和 script_post.bat 读取）
- 根据 `sys_em.cfg` 中的 `USE_BT_*_LIB` 宏，从 8 个蓝牙协议栈库变体中选择其一复制为 `libBtStack.a` 参与链接（当前配置为 `USE_BT_NO_BLE_LIB` → `libBtStack_noBle.a`）

---

## 8. Flash 分区与资源布局

根据 `log.txt` 和配置文件，Flash 4MB 空间分区如下：

| 区域 | 地址 | 说明 |
|------|------|------|
| 用户代码 | 0x10000 | 主固件代码区 |
| ILM | 0x28000 | 指令局部内存映射 |
| bp_data | 0x3E8000 | 断点数据 |
| bt_data | 0x3EA000 | 蓝牙数据 |
| effect_data | 0x3EC000 | 音效参数 (audio_effect.bin) |
| sys_parameter | 0x3FC000 | 系统参数 |
| user_config | 0x3FE000 | 用户配置 |
| bt_config | 0x3FF000 | 蓝牙配置 |

### 音效参数 (audio_effect.bin)

- 由 **ACPWorkbench** 工具生成
- 通过 `tools/merge_script/merge.exe` 合并到固件
- `merge.ini` 配置：路径 `audioeffect_res/audio_effect.bin`，最大长度 0x12000
- 运行时由 `AudioEffect_Parambin_Check()` 校验，失败则进入 Safe Mode

---

## 9. 工具链 (tools/)

| 工具/脚本 | 功能 |
|-----------|------|
| `merge_script/merge.exe` | 分区合并工具（按 merge.ini 合并代码+bp_data+bt_data+提示音+音效参数+sys_parameter，生成 flash.img） |
| `merge_script/sag_config.exe` | SAG 配置生成 |
| `merge_script/Andes_MVAGenerate.exe` | MVA 烧录/升级文件生成工具（按 MV_SDKConfig.ini 生成 main_merge.MVA / main_merge.MVA.bin） |
| `merge_script/MV_SDKConfig.ini` | MVA 打包配置（输出路径、chip_type=0xB6、代码段来源） |
| `merge_script/merge.ini` `parameter.ini` | 分区合并配置（各分区路径/最大长度/对齐）与参数配置 |
| `merge_script/bp_data.bin` `bt_data.bin` `rec_data.bin` | 分区合并用的默认数据文件 |
| `merge_script/flash_param.bin` | 由 postbuild 从 flash_param.o 提取，作为 sys_parameter 分区内容 |
| `remind_script/MergeAudio2BinNew.exe` | 提示音 MP2 合并工具（生成 all.bin 和 sound_remind_item.h） |
| `remind_script/ffmpeg.exe` | 音频格式转换（MP3→MP2） |
| `libs_copy_script/copyLibs.exe` | 库文件复制工具（需 script_copy.ini 配置，当前工程未提供该文件，实际为空操作） |
| `mv_download_v3.13.exe` | 固件下载/烧录工具 |
| `update_sag.ps1` | prebuild 时按 sys_em.cfg 动态更新 nds32-ae210p.sag |
| `script.bat` | 编译预处理脚本（更新链接脚本、构建提示音、库复制） |
| `script_post.bat` | 编译后处理脚本（分区合并 + MVA 打包） |
| `script_MP3.bat` | 仅重建提示音的独立脚本（不编译代码） |
| `flash.bat` | 重新合并固件脚本（重新生成提示音+分区合并+MVA 打包；注意其内部校验的旧固件名 `BT_Audio_APP.bin` 与本工程不符，属历史遗留） |
| `log_error_num.h` | 错误码定义 |

---

## 10. 资源文件

### 10.1 audioeffect_res/

| 文件 | 说明 |
|------|------|
| `audio_effect.bin` | 音效参数二进制文件（由 ACPWorkbench 生成） |
| `roboeffect_library_info_v2.48.1.bin` | RoboEffect 库信息 |
| `audio_effect_for_read/audio_effect.xlsx` | 音效参数可读格式 |
| `audio_effect_for_read/audio_effect.txt` | 音效参数文本说明 |
| `audio_effect_for_read/I2S1MIX_多线.bin` | I2S1 Mix 音效配置 |

### 10.2 remind_res/

提示音 MP3 文件目录，包含开机、关机、模式切换、连接/断开、音量调节等提示音。16K 子目录存放 16kHz 采样率的提示音。

---

## 11. 第三方集成

### 涂鸦 BLE SDK (tuya_sdk/)

- 集成涂鸦 BLE SDK v2.0.0
- `app_src/tuya_sdk/sdk/lib/libtuya_ble_sdk_v2.0.0_B6.a` — 预编译库
- `app_src/tuya_app/` — 涂鸦应用层（产品测试、UART处理、BLE配置）
- 支持 BLE GATT 通信和 OTA 升级

---

## 12. 双核架构 (DualCore)

B62 芯片支持双核运行：

- **Core1**：主核，运行主任务、音频模式、蓝牙协议栈
- **Core2**：副核，运行 DSP 音效处理（`dualcore_dsp.h`）
- **IPC通信**：通过 `dualcore_api.h` 提供双核间消息和共享内存访问
- 启动时 Core1 通过 `DualCore_ReleaseCore2ToRun()` 释放 Core2

---

## 13. 消息驱动架构详解

SDK 采用 **FreeRTOS 消息队列** 作为各模块间的核心通信机制，所有事件（按键、蓝牙、设备插拔、模式切换等）均通过消息驱动。

### 13.1 消息定义体系

消息 ID 定义在 [app_message.h](file:///d:/MVS_err/workspace/B6_Audio_SDK/app_src/app_mode/app_message.h)，按功能分组编号：

| 消息组 | 起始ID | 用途 |
|--------|--------|------|
| `MSG_MAINAPP_*` | 0x0100 | 主应用消息（模式切换等） |
| `MSG_BT_PLAY_*` | 0x0400 | 蓝牙播放模式 |
| `MSG_BT_HF_*` | 0x0500 | 蓝牙通话模式 |
| `MSG_LINE_AUDIO_*` | 0x0700 | LineIn 模式 |
| `MSG_HDMI_AUDIO_*` | 0x0800 | HDMI 模式 |
| `MSG_RADIO_*` | 0x0A00 | FM 收音模式 |
| `MSG_USB_DEVICE_*` | 0x0B00 | USB 设备模式 |
| `MSG_I2SIN_AUDIO_*` | 0x0D00 | I2S 输入模式 |
| `MSG_TWS_SLAVE_*` | 0x0E00 | TWS 从机模式 |
| `MSG_BTSTACK_*` | 0x7900 | 蓝牙协议栈服务 |
| `MSG_MEDIA_RECORDER_*` | 0x7800 | 录音服务 |

### 13.2 核心消息流

```
按键/设备事件 → DeviceService → MSG_DEVICE_SERVICE_xxx
                                    ↓
                           MainAppTask 消息循环
                                    ↓
                    ┌───────────────┼───────────────┐
                    ↓               ↓               ↓
              SysModeGenerate   PublicMsgPross   AudioEffect_Msg_Check
              (模式切换)        (公共消息处理)     (音效消息过滤)
                    ↓               ↓               ↓
              SysModeTask      各模式任务      AudioCore服务
              (模式引擎)       (BtPlay等)      (音频核心)
```

### 13.3 关键消息处理函数

| 函数 | 所在文件 | 职责 |
|------|----------|------|
| `PublicMsgPross()` | main_task.c | 处理主循环公共消息（电源、蓝牙、升级等） |
| `SysModeGenerate()` | mode_engine/mode_task.c | 根据消息生成模式切换 |
| `DeviceServicePocess()` | app_src/hmi/detect/device_detect.c | 处理设备插拔检测 |
| `AudioEffect_Msg_Check()` | app_src/components/audio/audio_effect_control.c | 过滤音效相关消息 |

---

## 14. AudioCore 音频核心数据流

AudioCore 是 SDK 的音频调度核心，管理 Source → Effect → Sink 的完整音频通路。

### 14.1 核心数据结构

定义在 [audio_core_api.h](file:///d:/MVS_err/workspace/B6_Audio_SDK/app_framework/audio_engine/audio_core_api.h)：

```c
typedef struct _AudioCoreSource {
    bool    Enable;        // 通道使能
    bool    Sync;          // 同步标志（TRUE=每帧同步等待）
    uint8_t Channels;      // PCM通道数
    uint8_t PcmWidth;      // PCM位宽
    uint8_t FlowCh;        // Flow通道数
    uint8_t FlowWidth;     // Flow位宽
    bool    Active;        // 活跃状态
    uint8_t Net;           // 网络编号（DefaultNet=0）
    AudioCoreDataGetFunc DataGetFunc;   // 数据获取回调
    AudioCoreDataLenFunc DataLenFunc;   // 数据长度回调
    PCM_DATA_TYPE *PcmInBuf;           // 输入PCM缓冲区
    SRC_ADAPTER *SrcAdapter;           // 采样率转换适配器
    ...
} AudioCoreSource;

typedef struct _AudioCoreSink {
    bool    Enable;        // 通道使能
    bool    Sync;          // 同步标志
    uint8_t Channels;      // PCM通道数
    uint8_t PcmWidth;      // PCM位宽
    AudioCoreDataSetFunc DataSetFunc;   // 数据输出回调
    AudioCoreSpaceLenFunc SpaceLenFunc; // 剩余空间回调
    PCM_DATA_TYPE *PcmOutBuf;          // 输出PCM缓冲区
    ...
} AudioCoreSink;
```

### 14.2 音频通路状态机

```c
typedef enum {
    MIX_STATE_IDLE,   // 空闲
    MIX_STATE_GOT,    // 数据已获取
    MIX_STATE_SYNC,   // 同步中
    MIX_STATE_PROC,   // 处理中
    MIX_STATE_PUT,    // 数据已输出
} MIX_STATE;
```

### 14.3 Source/Sink 编号映射

定义在 [flow_config.h](file:///d:/MVS_err/workspace/B6_Audio_SDK/app_src/system_config/flow_config.h)：

| Source 编号 | 名称 | 用途 |
|-------------|------|------|
| 0 | MIC_SOURCE | 麦克风输入 |
| 1 | APP_SOURCE | 应用音频（蓝牙/媒体） |
| 2 | REMIND_SOURCE | 提示音 |
| 3 | REC_SOURCE | 录音回放 |
| 4-9 | I2S0/1/2_MIX_SOURCE | I2S混音输入 |
| 10-11 | USB_FS/HS_MIX_SOURCE | USB混音输入 |
| 12 | LINEIN_MIX_SOURCE | LineIn混音 |
| 13 | SPDIF_MIX_SOURCE | SPDIF混音 |
| 14 | TWS_SOURCE | TWS源 |

| Sink 编号 | 名称 | 用途 |
|-----------|------|------|
| 0 | DAC_SINK | DAC直接输出 |
| 1-2 | DAC1/2_MIX_SINK | DAC混音输出 |
| 3 | APP_SINK | 应用输出 |
| 4 | REC_SINK | 录音输出 |
| 5-8 | I2S0/1/2_MIX_SINK | I2S混音输出 |
| 9-10 | USB_FS/HS_MIX_SINK | USB混音输出 |
| 11-12 | SPDIF0/1_SINK | SPDIF输出 |
| 13 | LE_SINK | LE Audio输出 |
| 14 | TWS_SINK | TWS输出 |

### 14.4 音频核心服务

定义在 [audio_core_service.h](file:///d:/MVS_err/workspace/B6_Audio_SDK/app_framework/audio_engine/audio_core_service.h)：

```c
// 服务控制
int32_t AudioCoreServiceCreate(MessageHandle parentMsgHandle);
int32_t AudioCoreServiceStart(void);
int32_t AudioCoreServicePause(void);
void    AudioCoreServiceResume(void);
void    AudioCoreServiceStop(void);
int32_t AudioCoreServiceKill(void);

// 消息接口
bool SendAudioCoreServiceMsg(uint16_t msgId);
MessageHandle GetAudioCoreServiceMsgHandle(void);
```

**关键参数**:
- `AUDIO_CORE_SERVICE_SIZE = 768` — 任务栈大小
- `AUDIO_CORE_SERVICE_PRIO = 5` — 任务优先级
- `AUDIO_CORE_SERVICE_TIMEOUT = 1` — 消息超时 1ms

### 14.5 效果服务

定义在 [effect_service.h](file:///d:/MVS_err/workspace/B6_Audio_SDK/app_framework/audio_engine/effect_service.h)：

```c
int32_t EffectServiceCreate(void);
int32_t EffectServiceStart(void);
void    EffectServiceStop(void);
int32_t EffectServiceKill(void);

bool SendEffectServiceMsg(uint16_t msgId);
void EffectProcessConfig(AudioCoreProcessFunc Process, uint8_t FlowNet);
AudioCoreProcessFunc GetEffectProcess(void);
```

**关键参数**:
- `EFFECT_SERVICE_SIZE = 512` — 任务栈大小
- `EFFECT_SERVICE_PRIO = 6` — 任务优先级（高于 AudioCore）
- `EFFECT_SERVICE_TIMEOUT = 1000` — 消息超时 1s

---

## 15. 任务调度与优先级

SDK 使用 FreeRTOS 多任务调度，各核心任务的优先级和栈大小如下：

| 任务名 | 优先级 | 栈大小 | 职责 | 所在文件 |
|--------|--------|--------|------|----------|
| MainAppTask | 4 | 640 words | 主任务：消息循环、模式调度、温度监控 | main_task.c |
| SysModeTask | 未明确 | 未明确 | 模式引擎任务 | mode_engine/mode_task.c |
| AudioCoreService | 5 | 768 bytes | 音频核心服务 | audio_engine/ |
| EffectService | 6 | 512 bytes | 音效处理服务 | audio_engine/ |
| BtStackService | 见bt_stack_service.h | 见bt_stack_service.h | 蓝牙协议栈服务 | bluetooth_engine/ |
| SlowDeviceTask | 未明确 | 未明确 | 慢速设备处理 | slow_device_engine/ |
| ShellTask | 2 | 512 words | Shell 命令处理 | main_task.c |

**优先级说明**: 数值越大优先级越高。EffectService(6) > AudioCoreService(5) > MainAppTask(4) > ShellTask(2)

---

## 16. 蓝牙协议栈服务

### 16.1 蓝牙栈服务接口

定义在 [bt_stack_service.h](file:///d:/MVS_err/workspace/B6_Audio_SDK/app_framework/bluetooth_engine/bt_stack_service.h)：

```c
// 服务控制
bool BtStackServiceStart(void);           // 启动蓝牙栈
void BtResetAndKill(bool power_flag);     // 重置并关闭蓝牙栈
void BtStackServiceWaitResume(void);      // 等待恢复
void BtStackServiceWaitClear(void);       // 等待清除

// 快速开关
void BtFastPowerOn(void);                 // 快速开机（保持栈后台运行）
void BtFastPowerOff(void);                // 快速关机（断开连接但保持栈运行）

// 电源控制
void BtPowerOn(void);
void BtPowerOff(void);

// DUT 模式
void BtEnterDutModeFunc(void);            // 进入 DUT 测试模式

// 消息接口
void BtStackServiceMsgSend(uint16_t msgId);
MessageHandle GetBtStackServiceMsgHandle(void);
```

### 16.2 蓝牙模式管理

| 功能 | 说明 |
|------|------|
| `BtPlayMode` | A2DP 音频播放模式 |
| `BtHfMode` | HFP 免提通话模式 |
| `TwsSlaveMode` | TWS 从机模式 |
| `LeAudioSinkMode` | LE Audio BIS/CIS Sink 模式 |

---

## 17. 关键设计模式与可迁移知识

以下设计理念可迁移到山景其他芯片 SDK：

1. **宏驱动配置**：通过 `CFG_APP_*/CFG_FUNC_*/CFG_RES_*` 宏控制功能裁剪，无需改代码
2. **模式引擎框架**：统一的 Init/Deinit/Run 模式生命周期管理
3. **消息驱动架构**：FreeRTOS 消息队列驱动各模块间通信
4. **音效图 (Flow Graph)**：通过 `audio_effect.bin` 配置音频效果链路，解耦算法与配置
5. **分层设计**：Driver → Driver API → Middleware → Framework → App，层间通过头文件接口
6. **预编译库 + 源码混合**：核心算法/协议栈以库提供，应用层以源码提供
7. **Flash 分区管理**：固定地址分区存储参数/断点/音效/配置
8. **Safe Mode 机制**：异常时自动降级到安全模式，保证基本功能可用

---

> **文档版本**: V1.1  
> **生成日期**: 2026-09-16  
> **更新日期**: 2026-09-22（基于成功编译结果修订第 7 章构建系统、第 9 章工具链，修正产物清单与文件链接路径）  
> **基于SDK版本**: V0.3.16
