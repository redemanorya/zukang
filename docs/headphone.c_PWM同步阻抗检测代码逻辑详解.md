# headphone.c PWM同步阻抗检测代码逻辑详解

> 整理日期：2026-10-14
> 对应文件：`app_src/components/headphone/headphone.c`
> 行号依据：当前工作区版本，后续修改代码后可能发生偏移。
> 当前实际配置：20 kHz、50%占空比、左右声道各采集一次高电平值和一次低电平值、240 mV分界阈值。

## 1. 本次改动范围

本次改动的目标是把原先“PB8输出固定高电平，再异步读取PA30/PA31 ADC”的阻抗检测方式，改成“PB8输出PWM，并在PWM高、低电平的指定相位同步采样”。串口协议、耳机音量和插拔事件上报的对外接口保持不变。

涉及的主要文件如下：

| 文件 | 改动 |
|---|---|
| `app_src/system_config/app_config.h:96` | 启用`CFG_HP_IMPEDANCE_PWM_SYNC_EN`，替代此前的PB8保持低电平诊断宏。 |
| `app_src/components/headphone/headphone.c:31-37` | 增加PWM、Timer、时钟、中断和RTOS相关头文件。 |
| `headphone.c:69-80` | 原8次直流平均采样仅在未启用PWM同步方案时编译。 |
| `headphone.c:84-110` | 增加Timer7/PWM2/PB8资源、时序参数和同步采样状态。 |
| `headphone.c:267-493` | 新增PWM启动、ISR采样、停止、GPIO恢复和结果换算实现。 |
| `headphone.c:1164-1217` | 在耳机插入流程中通过宏选择PWM同步采样或原直流采样。 |
| `app_src/components/headphone/headphone.h:43-50` | 明确PWM方案下`right_adc/left_adc`保存的是高低电平ADC差值。 |

`B6_Audio_SDK_bak`和`S037_SDK`未参与本次改动。

## 2. 硬件信号与软件资源

| 信号 | DSP引脚 | 软件用途 |
|---|---|---|
| `HP_DETC` | PB9 | 耳机插拔检测，低电平表示插入，见`HpDetRead()`。 |
| `AD_EN/AD_OUT` | PB8 | 检测时临时复用为Timer7 PWM2输出；结束后恢复GPIO低电平。 |
| `AD0` | PA30 / ADC0 | 右声道阻抗采样。 |
| `AD1` | PA31 / ADC1 | 左声道阻抗采样。 |
| `HP_EN` | PB10 | 耳机功放使能。 |
| 耳机音频 | DAC0差分输出 | 由`HpDacOutput()`和RL输出状态共同控制。 |

PB8的复用项为`TIMER7_PWM2_A16_B8`，`HP_PWM_GPIO_INDEX=1`选择PB8，而不是PA16。Timer7中断向量由`app_src/startup/crt0.S:96`指向`Timer7_Interrupt`；`init-default.c:34`中的弱默认处理函数会被`headphone.c`内的强定义覆盖。

## 3. 当前参数及推导

参数集中在`headphone.c:87-99`：

| 宏 | 当前值 | 含义 |
|---|---:|---|
| `HP_PWM_FREQ_HZ` | 20000 | PWM频率20 kHz，周期50 us。 |
| `HP_PWM_DUTY_PERCENT` | 50 | PB8理论高电平25 us、低电平25 us。 |
| `HP_PWM_HIGH_SAMPLE_PERCENT` | 40 | 周期开始后约20 us触发高电平ADC采样。 |
| `HP_PWM_LOW_SAMPLE_PERCENT` | 90 | 周期开始后约45 us触发低电平ADC采样。 |
| `HP_PWM_TOTAL_CYCLES` | 1 | 软件目标为一个周期后停止。它不是硬件单脉冲模式。 |
| `HP_PWM_ADC_POLL_LIMIT` | 4096 | 单次ADC轮询最大循环次数，不是明确的微秒超时。 |
| `HP_PWM_TASK_TIMEOUT_MS` | 5 | 主任务等待ISR完成的兜底时间。 |
| `HP_IMP_SPLIT_MV` | 240 | 小于240 mV判16 Ω，否则判32 Ω。 |
| `HP_SETTLE_MS` | 1000 | 插入去抖完成后，再等待1秒才开始阻抗检测。 |

当前`CFG_FUNC_OPEN_SLOW_DEVICE_TASK`已开启，主循环正常约10 ms调用一次`HeadphoneTaskPoll()`。`HP_DEBOUNCE_COUNT=5`因此对应约50 ms去抖。正常插入时，从物理插入到PWM开始约为：

```text
约50 ms去抖 + 1000 ms稳定等待 + 约2 ms ADC模拟通道稳定
```

所以PB8脉冲通常在插入后约1.05秒出现，而不是插入瞬间出现。

## 4. 完整调用时序

```text
SystemInit()
  └─ HeadphoneInit()
      ├─ PB9配置为输入
      ├─ PB8配置为GPIO输出低
      ├─ PB10配置为GPIO输出低
      └─ DAC0静音、耳机功放关闭

MainAppTaskEntrance()循环（约10 ms）
  └─ HeadphoneTaskPoll()
      ├─ HpDetRead()读取PB9
      ├─ 连续5次相同后确认状态
      ├─ 插入后等待HP_SETTLE_MS=1000 ms
      └─ HpPwmSampleBurst()
          ├─ 锁定SarADCMutex
          ├─ PA30/PA31切换为模拟输入
          ├─ ADC设为单次转换模式
          ├─ HpPwmStart()
          │   ├─ PB8由GPIO切换到Timer7/PWM2
          │   ├─ 配置20 kHz、50%占空比
          │   ├─ 开启40%和90%比较中断
          │   └─ 启动PWM
          ├─ Timer7_Interrupt()
          │   ├─ 40%：依次读取右高值、左高值
          │   ├─ 90%：依次读取右低值、左低值
          │   └─ cycle加1并关闭PWM
          ├─ 主任务检测done或等待5 ms超时
          ├─ HpPwmStopAndRestore()强制清理并把PB8拉低
          ├─ 计算high-low差值及毫伏值
          └─ 释放ADC模拟通道和SarADCMutex
      ├─ HpImpJudge()按240 mV判断16/32 Ω
      ├─ 左右差异异常时最多重试一次
      ├─ 保存到s_imp
      ├─ RL_ApplyHeadphoneOutput()恢复允许的耳机输出
      └─ RL_SendHeadphoneEvent(true)主动上报
```

## 5. PWM同步采样函数详解

### 5.1 `HpPwmGpioRestoreLow()`（268-276）

解除PB8的PWM复用，把PB8恢复为普通GPIO输出并拉低。所有成功、失败和超时路径最终都应执行该函数，防止检测参考信号长期存在。

### 5.2 `HpPwmIrqStop()`（278-284）

在Timer7 ISR内关闭PWM和两个比较中断，然后设置`done=true`通知主任务。该函数没有直接恢复PB8 GPIO模式，完整恢复由主任务中的`HpPwmStopAndRestore()`完成。

### 5.3 `HpPwmAdcRead()`（286-300）

启动指定ADC通道的单次转换，并在中断上下文中轮询转换完成。超过4096次循环返回失败。成功时读取12位ADC结果。

注意：这是阻塞式轮询，而且在Timer7中断内最多调用4次。

### 5.4 `Timer7_Interrupt()`（302-357）

- `PWM1_COMP1_INTERRUPT_SRC`到来时清标志，依次采集右、左高电平ADC值。
- 任一高电平采样失败则置`adc_error`并停止PWM。
- `PWM1_COMP4_INTERRUPT_SRC`到来时依次采集右、左低电平ADC值。
- 低电平采样完成后`cycle++`；达到`HP_PWM_TOTAL_CYCLES`后停止PWM。

`Duty[0]=40`和`Duty[3]=90`仅用于产生采样时间点；实际送到PB8的是PWM2，对应`Duty[2]=50`。

### 5.5 `HpPwmStopAndRestore()`（358-366）

这是任务上下文中的最终清理函数：再次关闭PWM、关闭并清除比较中断，最后恢复PB8为GPIO低电平。重复调用是为了覆盖启动失败、ISR失败和任务超时。

### 5.6 `HpPwmStart()`（368-417）

1. 清空PWM参数和同步采样状态。
2. 使用`Clock_SysClockFreqGet()/20000`计算分频值。
3. 配置向上计数、标准单端PWM、多通道比较和中断请求。
4. 将PB8复用为Timer7 PWM2。
5. 配置Timer7，清旧标志，开启两个比较中断及NVIC。
6. 启动PWM；失败则恢复PB8低电平。

这里的`PWM_OUTPUT_SINGLE_1`表示“标准单端PWM输出类型”，不是“只输出一个脉冲”。真正停止输出依赖`Timer7_Interrupt()`调用`PWM_Disable()`。

### 5.7 `HpPwmSampleBurst()`（419-492）

该函数是一次完整同步检测事务：初始化输出参数、独占SAR ADC、启动PWM、等待完成、强制清理、检查结果、换算毫伏并释放资源。

有效性要求为：ISR已完成、没有ADC错误、高电平未饱和且高值大于低值。最终用于阻抗判断的是：

```text
delta_adc = high_adc - low_adc
delta_mv  = delta_adc × 3300 / 4096
```

差值法可以抵消部分直流偏置，但前提是高、低采样确实位于同一个有效PWM周期的正确相位。

## 6. headphone.c其余函数职责

### 6.1 检测、ADC与输出函数

| 函数 | 行号 | 职责 |
|---|---:|---|
| `HpDetRead()` | 184-188 | 读取低有效PB9并转换为`true=已插入`。 |
| `HpAdEnSet()` | 190-200 | 原直流方案直接设置PB8；PWM启动前也用于确保PB8先为低。 |
| `AdcChSampleInit/Deinit()` | 202-212 | 开启/关闭PA30或PA31模拟输入。 |
| `HpAdcSampleOnce()` | 215-223 | 原直流分支单次采样。 |
| `HpAdcSampleNormalized()` | 226-264 | 原直流分支采8次，去掉一个最大值和一个最小值后平均。 |
| `HpImpJudge()` | 496-501 | 按240 mV阈值返回16 Ω或32 Ω。 |
| `HpDacOutput()` | 504-518 | 同时控制DAC0数字静音和PB10耳机功放使能。 |

### 6.2 RL串口协议辅助函数

| 函数 | 行号 | 职责 |
|---|---:|---|
| `RL_ReadU16/U32`、`RL_WriteU16/U32` | 521-544 | 协议小端整数读写。 |
| `RL_Crc16Arc()` | 546-561 | CRC-16/IBM/ARC计算。 |
| `RL_TimeReached()` | 563-566 | 支持系统Tick回绕的截止时间判断。 |
| `RL_NextEventPacketId()` | 568-576 | 生成主动事件ID，回绕时跳过0。 |
| `RL_SendPacket()` | 578-606 | 组装帧头、长度、ID、类型、Ack、命令、负载、帧尾和CRC，并从UART1发送。 |
| `RL_SendError()` | 608-613 | 返回4字节错误码ACK。 |
| `RL_HeadphoneDbGet()` | 615-622 | 将0～100耳机音量映射到-72.2～0 dB。 |
| `RL_ApplySpeakerOutput()` | 624-628 | 应用功放音量和静音。 |
| `RL_ApplyHeadphoneOutput()` | 630-638 | 综合插入状态、USB瞬态保持、显式静音和音量0，设置DAC0增益与开关。 |
| `RL_OutputControlRefresh()` | 640-644 | 同时重新应用扬声器和耳机状态。 |
| `RL_OutputStateExport()` | 646-653 | 导出四项状态用于断点保存。 |
| `RL_ScheduleSave()` | 655-659 | 安排2秒后合并写Flash。 |
| `RL_HeadphoneImpedanceGet()` | 661-669 | 仅左右同为16 Ω或32 Ω时返回有效值，否则返回-1。 |
| `RL_SendHeadphoneEvent()` | 671-691 | 上报插入/拔出；插入负载包含阻抗。协议未就绪时暂存最后一次事件。 |
| `RL_SetVolume()` | 693-729 | 校验输出枚举、范围和系统模式，独立设置音量并安排保存。 |
| `RL_SetMute()` | 731-763 | 独立设置显式静音并安排保存。 |
| `RL_SendCommandResult()` | 765-811 | 根据Ack和执行结果返回数据或错误。 |
| `RL_ProcessFrame()` | 813-851 | 分发设备信息、音量和静音命令。 |
| `RL_ParserResetWithByte()` | 853-863 | 解析失败后利用当前字节尝试重新同步帧头。 |
| `RL_ParserFeed()` | 865-911 | 流式检查帧头、长度、帧尾和CRC，完整后处理。 |
| `UART1_Interrupt()`/`UART_Interrupt()` | 913-939 | RX中断只把最多32字节搬入256字节环形缓冲。 |
| `RL_ProtocolInit()` | 941-995 | 恢复持久化状态、初始化UART1及输出状态。 |
| `RL_ProtocolPoll()` | 997-1055 | 延迟进入协议模式、解析RX、处理残帧超时并执行延迟Flash保存。 |

### 6.3 对外接口

| 函数 | 行号 | 职责 |
|---|---:|---|
| `HeadphoneInit()` | 1060-1091 | 初始化状态及PB9/PB8/PB10，默认关闭耳机输出。 |
| `HeadphoneSetOutput()` | 1093-1101 | 设置系统瞬态保持状态，并重新计算耳机实际输出。 |
| `HeadphoneGetState()` | 1103-1106 | 返回当前插拔状态。 |
| `HeadphoneGetImpInfo()` | 1108-1111 | 返回最近一次左右ADC、电压和阻抗结果。 |
| `HeadphoneTaskPoll()` | 1113-1258 | 完成去抖、插拔状态机、阻抗采样、重试、输出控制及主动上报。 |

## 7. 当前最需要核查的逻辑风险

### 7.1 “一个周期”不是硬件保证

`PWM_OUTPUT_SINGLE_1`不是one-pulse模式。PWM会持续重复，直到ISR或5 ms任务超时调用`PWM_Disable()`。因此`HP_PWM_TOTAL_CYCLES=1`只表示“第一次90%比较中断处理完成后请求停止”。

### 7.2 一个50 us周期内安排了4次阻塞式ADC转换

20 kHz下的关键时间为：

```text
0 us          PWM周期开始
20 us         40%中断，开始右高值+左高值两次ADC转换
25 us         PB8由高变低
45 us         90%中断，开始右低值+左低值两次ADC转换
50 us         下一个PWM周期开始
```

高采样ISR从20 us开始，距离低采样时间只有25 us；低采样ISR从45 us开始，距离下一周期只有5 us。如果两次ADC转换和中断开销超过对应窗口，低比较标志会在ISR执行期间到达，低采样的实际时刻将偏移；PWM也可能已经进入下一个周期才被关闭。

### 7.3 ADC轮询超时不是时间单位

`HP_PWM_ADC_POLL_LIMIT=4096`只是循环次数。如果ADC完成标志异常，单次读取可能长时间占用Timer7 ISR。中断上下文被阻塞时，主任务中的5 ms兜底也不能立即运行，这可能表现为PB8保持PWM包络或异常电平数毫秒。

### 7.4 5 ms超时与此前约7.2 ms包络相符，但尚不能定论

如果Timer7比较中断没有进入、比较源选择错误或`done`未置位，主任务理论上会在约5个OS tick后清理。加上任务调度和示波器欠采样，可能看到约数毫秒包络。但此前2.50 kS/s的波形无法解析20 kHz载波，必须在正确时基下重新验证。

### 7.5 PB8实测高电平偏低

此前示波器显示PB8包络峰值约1.36～1.44 V。如果探头倍率和测点无误，PB8直接连接AD_OUT后可能被检测网络重载，不能达到3.3 V。此问题会让240 mV阈值和“16 Ω约160 mV、32 Ω约320 mV”的假设失效，也可能超过GPIO推荐驱动能力。

### 7.6 其他可维护性问题

- `HeadphoneTaskPoll()`中的`s_detectCnt`为`uint8_t`且持续递增，没有在达到5后饱和，长时间稳定后会周期性溢出到0。它通常不会重复触发插拔，但建议后续改为饱和计数。
- 文件头部仍写有“PB9 1=inserted”和“AD_EN默认开启”等旧说明，与当前低有效检测、PB8默认低电平不一致。
- 启动PWM前没有显式`NVIC_ClearPendingIRQ(TMR7_IRQn)`，也没有为Timer7设置明确优先级。通常不影响首次运行，但不利于排除遗留Pending中断和优先级问题。
- 当前维护文档中曾记录50 kHz，而实际源码为20 kHz；分析和测试必须以源码宏为准。

## 8. 建议的排查顺序（暂不改业务逻辑）

1. **验证测点**：PB8空载、插入32 Ω、插入16 Ω分别测量峰值，确认是否确实被拉低到约1.4 V。
2. **验证PWM外设本身**：临时取消`Timer7_Interrupt()`内4次ADC调用，仅保留比较标志清除、周期计数和停止；正确结果应是20 kHz、50%占空比的单周期，约25 us高电平。
3. **验证ISR是否进入**：增加内存计数器分别记录40%和90%中断次数，检测结束后在任务上下文打印；不要在ISR内打印日志。
4. **验证启动返回值**：记录`PWM_Config()`、`PWM_Enable()`、`pwmDivider`和`Clock_SysClockFreqGet()`，确认外设配置实际成功。
5. **逐步恢复ADC**：先在40%中断只采右声道，再加左声道；之后再恢复90%低值采样，观察每一步是否让PWM停止失效。
6. **评估结构调整**：若4次阻塞转换无法放入50 us周期，应改用ADC硬件触发/DMA，或让PWM连续运行若干周期、每周期只采一个值，而不是在一个ISR周期内完成四次转换。
7. **最后校准阈值**：只有确认PB8幅值、采样相位和左右差值稳定后，再重新评估240 mV阈值。

## 9. 判断示波器结果的方法

示波器应关闭Zoom偏移，水平位置恢复到触发点`T=0`，使用PB8上升沿Single触发。20 kHz、50%占空比下：

| 观察结果 | 可能含义 |
|---|---|
| 约25 us高电平后回低 | PWM启动和一个周期停止基本正常。 |
| 20 kHz方波持续数毫秒 | PWM配置正常，但Timer7 ISR未及时停止或ADC阻塞。 |
| 只有数毫秒连续高电平且看不到载波 | 采样率/时基不足，或PB8没有真正输出PWM。 |
| Single一直处于Ready | PB8未跨过触发电平、测点错误、PWM未启动或插入状态没有形成新事件。 |
| 已触发但窗口无波形 | 水平延迟/Zoom位置偏离触发点。 |

当前示波器排查与代码审查应首先围绕“Timer7比较中断是否真正进入”和“4次ADC转换是否能在50 us时序内完成”展开。
