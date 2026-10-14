# B6_Audio_SDK 开发者参考手册

> **芯片**: 山景 (Mountain View Silicon) B6/B62 系列  
> **SDK版本**: V0.3.16 (2026-08-13)  
> **内核**: Andes NDS32 + FreeRTOS  
> **适用人群**: 初次接触山景芯片 SDK 的嵌入式开发者

---

## 手册导航

本手册分为四个部分，建议按顺序阅读，也可根据需要直接查阅对应章节：

| 部分 | 文档 | 内容概要 | 建议阅读顺序 |
|------|------|----------|--------------|
| **快速上手** | [01_快速上手指南.md](file:///d:/MVS_err/workspace/B6_Audio_SDK/docs/01_快速上手指南.md) | 环境搭建、编译烧录、基本配置、启动流程 | 第1天 |
| **模块深入剖析** | [02_模块深入剖析.md](file:///d:/MVS_err/workspace/B6_Audio_SDK/docs/02_模块深入剖析.md) | 各层架构详解、核心数据结构、调用链、AudioCore/蓝牙/模式引擎深入分析 | 第2-3天 |
| **外设配置指南** | [03_外设配置指南.md](file:///d:/MVS_err/workspace/B6_Audio_SDK/docs/03_外设配置指南.md) | GPIO/UART/I2C/DMA/ADC/PWM/Timer等外设的配置方法、驱动API、典型应用 | 按需查阅 |
| **功能开发实战** | [04_功能开发实战.md](file:///d:/MVS_err/workspace/B6_Audio_SDK/docs/04_功能开发实战.md) | 常见功能的完整开发流程：新增模式、修改音效、添加按键、低功耗配置等 | 第4-5天 |

---

## 快速索引

### 按功能查找

| 我想... | 参考章节 |
|---------|----------|
| 搭建开发环境并编译烧录 | [01_快速上手指南.md](file:///d:/MVS_err/workspace/B6_Audio_SDK/docs/01_快速上手指南.md) |
| 理解 SDK 整体架构 | [02_模块深入剖析.md](file:///d:/MVS_err/workspace/B6_Audio_SDK/docs/02_模块深入剖析.md) |
| 配置 GPIO/按键/LED | [03_外设配置指南.md](file:///d:/MVS_err/workspace/B6_Audio_SDK/docs/03_外设配置指南.md) |
| 新增一个音频模式 | [04_功能开发实战.md](file:///d:/MVS_err/workspace/B6_Audio_SDK/docs/04_功能开发实战.md) |
| 修改音效参数 | [04_功能开发实战.md](file:///d:/MVS_err/workspace/B6_Audio_SDK/docs/04_功能开发实战.md) |
| 配置蓝牙功能 | [03_外设配置指南.md](file:///d:/MVS_err/workspace/B6_Audio_SDK/docs/03_外设配置指南.md) |
| 实现低功耗 | [04_功能开发实战.md](file:///d:/MVS_err/workspace/B6_Audio_SDK/docs/04_功能开发实战.md) |
| 调试技巧 | [01_快速上手指南.md](file:///d:/MVS_err/workspace/B6_Audio_SDK/docs/01_快速上手指南.md) |

### 按文件查找

| 配置文件 | 用途 |
|----------|------|
| `app_src/system_config/app_config.h` | 功能模式总开关 |
| `app_src/system_config/chip_config.h` | 芯片型号选择 |
| `app_src/system_config/bt_config.h` | 蓝牙名称/地址/协议 |
| `app_src/system_config/gpio_config.h` | GPIO 引脚分配 |
| `app_src/system_config/clock_config.h` | 时钟频率配置 |
| `app_src/system_config/flow_config.h` | 音效图 Source/Sink 定义 |
| `app_src/main.c` | 程序入口、硬件初始化 |
| `app_src/main_task.c` | 主任务、消息循环 |

---

## 核心概念速览

### 五层架构

```
app_src        → 应用层（模式、组件、配置）
app_framework  → 框架引擎层（模式/音频/蓝牙引擎）
middleware     → 中间件层（RTOS/音频算法/蓝牙栈/文件系统）
driver_api     → 驱动API层（OS集成封装）
driver         → 硬件驱动层（寄存器级驱动）
```

### 配置宏命名规范

| 前缀 | 含义 | 示例 |
|------|------|------|
| `CFG_APP_*` | 音频模式开关 | `CFG_APP_BT_MODE_EN` |
| `CFG_FUNC_*` | 功能开关 | `CFG_FUNC_MIC_EN` |
| `CFG_PARA_*` | 参数配置 | `CFG_PARA_SYS_VOLUME_DEFAULT` |
| `CFG_RES_*` | 硬件资源开关 | `CFG_RES_AUDIO_DAC2_MIX_OUT_EN` |

### 模式生命周期

```
模式切换触发 → 旧模式 Deinit → 新模式 Init → 新模式 Run
```

### 音频通路模型

```
Source (输入源) → Effect Flow (音效图) → Sink (输出端)
```

---

## 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| V1.0 | 2026-09-16 | 初始版本 |
| V1.1 | 2026-09-22 | 基于成功编译结果修订构建系统/编译流程/产物清单/烧录说明，修正文档内文件链接路径 |

---

> **提示**: 建议结合 `B6_Audio_SDK_架构分析文档.md` 一起阅读，该文档提供了 SDK 的整体架构概览。
