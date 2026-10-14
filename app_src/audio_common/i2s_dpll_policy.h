#ifndef __I2S_DPLL_POLICY_H__
#define __I2S_DPLL_POLICY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "type.h"
#include "i2s_interface.h"
#include "app_config.h"

#ifdef CFG_FUNC_I2S_DPLL_POLICY_EN
/*
 * I2S DPLL0 external lock policy
 * Keep disabled by default. Enable these macros in product config when a
 * specific I2S slave path needs to drive DPLL0 from GPIO_I2Sx_BCLK.
 *
 * CFG_I2S_DPLL_REF_MODULE:
 *   0    -> I2S0
 *   1    -> I2S1
 *   2    -> I2S2
 *   0xFF -> disable
 */
#define CFG_I2S_DPLL_REF_MODULE            0
#define CFG_AUDIO_DAC0_FOLLOW_DPLL0_EN     1
#define CFG_AUDIO_DAC1_FOLLOW_DPLL0_EN     0
#define CFG_AUDIO_DAC2_FOLLOW_DPLL0_EN     0
#define CFG_AUDIO_ADC0_FOLLOW_DPLL0_EN     1
#define CFG_AUDIO_ADC1_FOLLOW_DPLL0_EN     0
#define CFG_AUDIO_I2S0_FOLLOW_DPLL0_EN     0
#define CFG_AUDIO_I2S1_FOLLOW_DPLL0_EN     1
#define CFG_AUDIO_I2S2_FOLLOW_DPLL0_EN     0

/**
 * spdif_out因为使用外部时钟，切到APLL会出现杂音，需使用DPLL0整数倍分频，所以开启DPLL0外部时钟定频时默认跟随DPLL0（定频成功后出声、失锁后无声）
 * spdif_in因为使用内部时钟，切到APLL声音正常，所以可以灵活选择是否开启DPLL0外部时钟跟随（定频成功后使用DPLL0、失锁后使用APLL，声音均能正常）
 */
#if defined(CFG_APP_OPTICAL_MODE_EN) || defined(CFG_APP_COAXIAL_MODE_EN)
#define CFG_AUDIO_SPDIF0OUT_FOLLOW_DPLL0_EN 0
#define CFG_AUDIO_SPDIF1OUT_FOLLOW_DPLL0_EN 1
#endif

#define I2S_DPLL_REF_MODULE_NONE 0xFF
#ifndef CFG_I2S_DPLL_REF_MODULE
#define CFG_I2S_DPLL_REF_MODULE I2S_DPLL_REF_MODULE_NONE
#endif

typedef enum _I2S_DPLL_OWNER
{
	I2S_DPLL_OWNER_NONE = 0,
	I2S_DPLL_OWNER_I2SIN_MODE,
	I2S_DPLL_OWNER_I2S0_MIX,
	I2S_DPLL_OWNER_I2S1_MIX,
	I2S_DPLL_OWNER_I2S2_MIX,
	I2S_DPLL_OWNER_MAX,
} I2S_DPLL_OWNER;

typedef enum _I2S_DPLL_STATE
{
	I2S_DPLL_STATE_IDLE = 0,
	I2S_DPLL_STATE_PREPARE,
	I2S_DPLL_STATE_START,
	I2S_DPLL_STATE_WAIT_LOCK,
	I2S_DPLL_STATE_HOLD,
	I2S_DPLL_STATE_WAIT_RELEASE,
	I2S_DPLL_STATE_STABLE,
	I2S_DPLL_STATE_RELOCK,
	I2S_DPLL_STATE_RELEASE,
} I2S_DPLL_STATE;

typedef void (*I2S_DPLL_STREAM_CB)(void);

/* 初始化 I2S DPLL 策略上下文。
 * 一般在系统启动时调用一次，用于清空状态机、记录参考 I2S 模块选择，
 * 并进入 IDLE 状态等待业务模块注册 candidate 和发起 owner 请求。 */
void I2SDpllPolicy_Init(void);

/* 驱动 I2S DPLL 策略状态机周期运行。
 * 需要在主循环或定时任务中持续调用，内部会推进
 * IDLE -> PREPARE -> START -> WAIT_LOCK -> HOLD -> STABLE / RELOCK / RELEASE。 */
void I2SDpllPolicy_Process(void);

/* 注册一个业务模块为 DPLL 候选输入源。
 * 典型调用方是 i2sin_mode / i2s0_mix / i2s1_mix / i2s2_mix。
 * Module 表示对应的 I2S 口，IsSlave 表示当前是否为 slave 模式，
 * IsActive 表示该业务链路当前是否处于有效激活状态。 */
void I2SDpllPolicy_RegisterCandidate(I2S_DPLL_OWNER Owner, I2S_MODULE Module, bool IsSlave, bool IsActive);

/* 注册业务流的开流/关流回调。
 * 当参考 I2S 需要等 DPLL stable 后再真正打开 AudioCoreSource/AudioCoreSink 时，
 * 策略层会在合适时机回调 EnableCb；在 relock/release 时则回调 DisableCb。 */
void I2SDpllPolicy_RegisterStreamCallback(I2S_DPLL_OWNER Owner, I2S_DPLL_STREAM_CB EnableCb, I2S_DPLL_STREAM_CB DisableCb);

/* 请求当前业务模块成为 DPLL owner。
 * 只有被选中的参考 I2S 且工作在 slave 模式时，该请求才会真正生效。
 * SampleRate 为当前检测到的采样率，策略层会据此配置 DPLL 目标频率。 */
bool I2SDpllPolicy_RequestOwner(I2S_DPLL_OWNER Owner, I2S_MODULE Module, uint32_t SampleRate, bool IsSlave);

/* 上报当前业务模块最新的采样率。
 * 当 owner 已经建立完成后，如果采样率发生变化，策略层会据此触发 relock，
 * 重新配置 DPLL 目标频率并重建 follow 模块的时钟关系。 */
void I2SDpllPolicy_ReportSampleRate(I2S_DPLL_OWNER Owner, I2S_MODULE Module, uint32_t SampleRate);

/* 上报参考时钟丢失事件。
 * 当底层驱动或业务层确认外部 I2S BCLK/LRCLK 已经失效时调用，
 * 策略层会进入 RELOCK 流程，先关闭当前业务流，再把 follow 模块切回原始时钟。 */
void I2SDpllPolicy_ReportClockLost(I2S_DPLL_OWNER Owner, I2S_MODULE Module);

/* 释放当前业务模块的 owner 请求。
 * 一般在对应 I2S 业务退出、deinit 或链路被关闭时调用。
 * 如果当前 owner 正是该模块，策略层会进入 RELEASE 流程并清理当前 DPLL 绑定关系。 */
void I2SDpllPolicy_ReleaseOwner(I2S_DPLL_OWNER Owner, I2S_MODULE Module);

/* 查询当前 DPLL 策略是否已经进入稳定态。
 * 返回 TRUE 表示已经完成锁定、hold 释放，业务可以按 stable 逻辑处理。 */
bool I2SDpllPolicy_IsStable(void);

/* 查询当前 I2S 业务是否需要延迟开流。
 * 对被选中的参考 I2S slave 链路，在 DPLL 尚未 stable 前应返回 TRUE，
 * 业务层据此不要立即执行 AudioCoreSourceEnable/AudioCoreSinkEnable。 */
bool I2SDpllPolicy_ShouldDelayStreamEnable(I2S_MODULE Module, bool IsSlave);

/* 查询当前 I2S 业务是否应由 DPLL 策略旁路 SRA/Adapt。
 * 对被选中的参考 I2S slave 链路，一旦启用该策略就返回 TRUE，
 * 业务层应按纯时钟同步链路处理，使用 STD 模式而不是 SRA/Adapt 模式。
 * 真正的开流时机仍由 ShouldDelayStreamEnable() 控制，需要等 stable 后再打开。 */
bool I2SDpllPolicy_ShouldBypassSra(I2S_MODULE Module, bool IsSlave);

/* 获取当前已经挂载到 DPLL0 的 follow 模块掩码。
 * 返回值用于观察当前有哪些 DAC/ADC/I2S/SPDIF 输出正在跟随 DPLL0。 */
uint32_t I2SDpllPolicy_GetFollowMask(void);

#endif

#ifdef __cplusplus
}
#endif

#endif
