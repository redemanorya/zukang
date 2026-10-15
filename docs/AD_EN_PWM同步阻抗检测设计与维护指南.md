# AD_EN PWM同步阻抗检测设计与维护指南

## 1. 文档范围

本文说明当前耳机插拔、AD_EN PWM激励、ADC同步采样、阻抗判定及RL串口主动上报的完整代码路径。行号基于2026-10-14当前代码，后续增删代码后可能偏移，应优先按宏名和函数名搜索。

当前硬件连接为：PB9连接`HP_DETC`，低电平表示耳机插入；PB8直接连接`AD_EN/AD_OUT`并提供检测参考电压；PA30/ADC0采右声道；PA31/ADC1采左声道；PB10连接`HP_EN`；DAC0驱动耳机功放。PB8不再在整个检测窗口持续输出直流高电平，而是在采样期间临时切换为50 kHz PWM，采样结束后恢复GPIO低电平。

## 2. 配置入口与资源占用

| 文件与行号 | 配置或资源 | 当前值 | 修改影响 |
| --- | --- | --- | --- |
| `app_src/system_config/app_config.h:92` | `CFG_FUNC_OXS_PROTOCOL_EN` | 已定义 | 开启UART1 RL协议和耳机插拔主动上报。 |
| `app_src/system_config/app_config.h:95` | `CFG_FUNC_HEADPHONE_EN` | 已定义 | 耳机输出、插拔检测和阻抗检测总开关。 |
| `app_src/system_config/app_config.h:96` | `CFG_HP_IMPEDANCE_PWM_SYNC_EN` | 已定义 | 定义时使用PWM同步检测；注释后回退到原直流AD_EN加8次归一化采样分支。 |
| `app_src/components/headphone/headphone.c:52-60` | 引脚、ADC通道和去抖计数 | PB9/PB8/PB10、ADC0/1、5次 | 硬件改版或调整插拔去抖时修改；变更前必须核对原理图与任务轮询周期。 |
| `app_src/components/headphone/headphone.c:63` | `HP_IMP_SPLIT_MV` | 240 mV | 小于阈值判为16 ohm，否则判为32 ohm。应依据PWM差分实测值重新标定。 |
| `headphone.c:65-66` | ADC满量程与参考电压 | 4096、3300 mV | 控制ADC码到毫伏的换算；若实际参考电压不是3.3 V，应校准这里或引入实测系数。 |
| `headphone.c:76` | `HP_SETTLE_MS` | 1000 ms | 插入去抖完成后、开始阻抗检测前的机械稳定等待时间。 |
| `headphone.c:82` | `HP_IMP_ASYM_MV` | 300 mV | 左右差值超过此值时触发一次重测。 |
| `headphone.c:87-91` | PWM硬件资源 | Timer7/PWM2/PB8 | PB8复用为`TIMER7_PWM2_A16_B8`；Timer7不得再分配给其他功能。 |
| `headphone.c:92` | `HP_PWM_FREQ_HZ` | 50000 Hz | 改变激励频率；当前周期20 us。 |
| `headphone.c:93` | `HP_PWM_DUTY_PERCENT` | 50% | PB8高、低电平各约10 us。 |
| `headphone.c:94-95` | 高、低采样相位 | 40%、90% | 当前约在周期8 us和18 us采样。 |
| `headphone.c:96` | `HP_PWM_TOTAL_CYCLES` | 1 | 每次检测脉冲串只运行一个PWM周期。 |
| `headphone.c:97` | `HP_PWM_SAMPLE_CHANNELS` | 2 | 固定保存右、左两个声道的高低电平结果。 |
| `headphone.c:98-99` | ADC轮询与任务超时 | 4096次、5 ms | 防止ADC或中断异常时永久阻塞。 |

单周期方案在40%比较中断内连续完成右、左两个高电平ADC转换，在90%比较中断内连续完成右、左两个低电平ADC转换。`HP_PWM_SAMPLE_CHANNELS`必须保持为2；若要增加周期数或进行多次平均，必须同步重构中断采样、数组容量和结果汇总，不能只修改`HP_PWM_TOTAL_CYCLES`。

## 3. PWM与ADC同步实现

### 3.1 初始化和恢复

- `HeadphoneInit()`位于`headphone.c:1060-1091`。PB9配置为检测输入；PB8默认GPIO输出低；PB10默认关闭耳放；DAC0保持静音。
- `HpPwmGpioRestoreLow()`位于`headphone.c:268-276`。它解除PB8的PWM复用并强制恢复GPIO低电平。
- `HpPwmStopAndRestore()`位于`headphone.c:358-366`。无论成功、超时还是错误，都会关闭Timer7 PWM和比较中断、清标志并恢复PB8低电平。

### 3.2 PWM启动

`HpPwmStart()`位于`headphone.c:368-417`：

1. 根据`Clock_SysClockFreqGet() / HP_PWM_FREQ_HZ`计算Timer7分频。
2. PWM2以50%占空比输出到PB8。
3. PWM0和PWM3不输出到引脚，只把40%和90%位置用作ADC采样比较时刻。
4. 清中断标志，开启比较中断和`TMR7_IRQn`，最后启动PWM。

### 3.3 中断采样时序

`Timer7_Interrupt()`位于`headphone.c:302-356`。一个PWM周期内共执行四次ADC转换：

1. 周期40%位置先读取右声道高电平，再读取左声道高电平。
2. 周期90%位置先读取右声道低电平，再读取左声道低电平。
3. 完成低电平采样后立即停止PWM并置完成标志。

单次PWM脉冲串理论长度为`1 / 50000 = 20 us`，高电平约10 us。如果首次ADC无效、左右分类不同或左右差值过大，主任务会在10 ms后再执行一次脉冲串。50 kHz下40%到90%之间只有10 us，需要容纳两次阻塞式ADC转换；如果转换耗时过长，90%采样可能延迟甚至跨到下一周期，这是当前实验方案的主要风险。

### 3.4 差分值计算

`HpPwmSampleBurst()`位于`headphone.c:419-492`。函数先锁定`SarADCMutex`并启用PA30/PA31模拟输入，然后等待PWM中断完成。有效条件为：

- 中断流程已完成且没有ADC错误；
- 两个声道高电平结果均未达到`0x0FFF`饱和值；
- 两个声道都满足高电平ADC值大于低电平ADC值。

保存到`HP_IMPEDANCE_INFO`中的ADC码不是绝对采样值，而是`high_adc - low_adc`。毫伏值按`delta * 3300 / 4096`计算。结构体说明位于`app_src/components/headphone/headphone.h:43-52`。

## 4. 插拔检测、重测和输出恢复

`HeadphoneTaskPoll()`位于`headphone.c:1113-1258`，完整流程如下：

1. PB9连续5次保持同一状态后通过去抖，当前轮询周期下约为50 ms。
2. 检测到插入后等待`HP_SETTLE_MS`，当前为1000 ms。
3. 调用`HpPwmSampleBurst()`取得左右声道PWM差分值。
4. `HpImpJudge()`在`headphone.c:496-501`按240 mV分别判断16/32 ohm。
5. ADC无效时重测一次；首次左右差值大于300 mV或两边分类不同时也重测一次。两次之间等待10 ms。
6. 保存最终ADC差值、毫伏值和左右阻抗结果。
7. 调用`RL_ApplyHeadphoneOutput()`恢复符合当前音量、显式静音和系统瞬态静音条件的耳机输出，然后发送插入事件。
8. 检测到拔出时关闭有效输出并发送拔出事件。

当前第二次采样不会再次进入第三次重测。若第二次左右分类仍不一致，两个结果会分别保存，串口层最终上报`-1`。

## 5. RL串口主动上报

`RL_HeadphoneImpedanceGet()`位于`headphone.c:661-669`。只有左右两边同时为16 ohm或同时为32 ohm才返回有效值，否则返回有符号32位`-1`。

`RL_SendHeadphoneEvent()`位于`headphone.c:671-691`：

- 插入命令为`0x10`，带4字节小端阻抗：16 ohm为`10 00 00 00`，32 ohm为`20 00 00 00`，无效为`FF FF FF FF`。
- 拔出命令为`0x11`，没有参数。
- 协议尚未结束3秒启动等待时，只保存最后一个待发事件；协议就绪后再主动上报。

部分串口GUI以文本方式保存二进制流时，会把`FF`转换成UTF-8替换字符`EF BF BD`。这不是设备发送了这些字节。判断原始协议必须使用HEX接收或原始二进制抓取。

## 6. 当前实测状态

本轮串口缓存确认`0x10`插入和`0x11`拔出事件均能主动上报，Packet ID正常递增。统计到的插入结果为：16 ohm 6次、32 ohm 0次、无效`-1` 10次；最新一组插入结果依次为`-1、16、-1、16、-1`。因此当前版本已验证事件链路，但阻抗准确率尚未通过验收，尤其是32 ohm尚未被有效识别。

听感方面，普通插拔由原来的两声较明显“咚咚”降低为一声较轻“嗒”；完全断电再上电仍可听到两声较轻“嗒”，但幅度明显降低。两声可能来自PWM重测，也可能分别来自PWM检测和耳放/DAC输出恢复，不能仅凭听感定论。

## 7. 后续调参和诊断顺序

### 7.1 首先获取原始ADC差分值

临时在`app_src/system_config/app_config.h:96`附近增加`#define CFG_FUNC_HEADPHONE_DEBUG_LOG_EN`，观察`headphone.c:477-484`输出的右/左高低ADC、差分码、毫伏值和有效标志。当前工程没有默认定义这个宏。由于UART1同时承载RL二进制协议，调试时应单独烧录诊断固件并暂停协议解析，或把诊断数据扩展成专用RL调试命令，避免文本日志污染协议；测试结束后删除该宏。

分别对16 ohm和32 ohm耳机至少插拔20次，记录左右差分的最小值、最大值、平均值及重试次数，再决定阈值。阈值应放在两组分布的空隙中，而不是继续假定直流方案的240 mV必然适用于PWM差分方案。

### 7.2 参数调整优先级

1. 先用示波器确认PB8确实为50 kHz、50%占空比。单次应为20 us周期、10 us高电平，而不是持续约4至5 ms的单个高电平。
2. 如果仍出现约4至5 ms高电平，优先检查Timer7比较中断是否执行；该时长接近`HP_PWM_TASK_TIMEOUT_MS`，修改周期数不能解决中断未完成的问题。
3. 根据波形稳定区调整`HP_PWM_HIGH_SAMPLE_PERCENT`和`HP_PWM_LOW_SAMPLE_PERCENT`，确保分别落在高、低平台末端且避开边沿。
4. 如果一个周期内四次ADC转换来不及完成，应优先降低PWM频率，或恢复分周期采样；不要只延长任务超时掩盖问题。
5. 最后根据统计分布调整`HP_IMP_SPLIT_MV`和`HP_IMP_ASYM_MV`。

### 7.3 区分上电两声“嗒”的方法

示波器同时观察PB8/AD_EN与PB10/HP_EN：

- PB8出现两个相隔约10 ms的PWM脉冲串，说明触发了阻抗重测。
- PB8只有一个脉冲串，而另一声与PB10上升沿同步，说明第二声来自耳放使能或DAC解除静音。
- 若两者都不对应，应继续观察耳放电源`AMP_VCC`和DAC0输出共模电压。

## 8. 修改后的必测项目

每次修改PWM、采样相位或阈值后，至少完成以下验证：

1. 编译PWM分支以及注释宏后的原直流分支，确认Timer7、ADC和中断无资源冲突。
2. 16 ohm和32 ohm各测试不少于20次，左右声道必须一致，记录有效率和`-1`比例。
3. 验证开机已插入、运行中插入、慢插、快速插拔和拔出事件。
4. 校验RL帧的命令、载荷、Packet ID、长度和CRC。
5. 比较耳机无音源、播放音乐和播放提示音时的“嗒”声大小。
6. 完全断电并等待电源放电后重新上电至少5次，观察PB8、PB10和耳机输出。
7. 确认检测结束后PB8始终恢复低电平，ADC互斥锁已释放，音频任务没有卡顿。
