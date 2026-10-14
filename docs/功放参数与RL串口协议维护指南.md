# 功放参数与RL串口协议维护指南

本文面向后续维护人员，说明两项容易出错的工作：如何把新的ACM8625S调音参数安全导入工程，以及RL串口协议在代码中的接收、解析、执行和回复流程。协议的线上格式以《OXS功放通信协议文档V1.md》为准，实际测试帧参考《RL串口协议功能测试文档.md》。

## 1. 功放参数文件的职责

当前硬件使用两颗ACM8625S：

|通道|用途|I²C地址|工程文件|
|---|---|---:|---|
|TWE|全频喇叭|`0x58`|`app_src/components/audio/amp_driver/ACM8625S_export_TWE.c`|
|WF|低音喇叭|`0x5A`|`app_src/components/audio/amp_driver/ACM8625S_export_WF.c`|

`ACM8625.c` 是两颗功放的公共入口。`ACM8625_PowerOn()`依次调用`ACM86xx_init_TWE()`和`ACM86xx_init_WF()`；`ACM8625_VolSet()`同时调用`TWE_volumeControl()`和`WF_volumeControl()`；`ACM8625_Mute()`同时控制两颗功放。因此，不能只替换其中一颗功放的参数后就认为双路均已更新。

当前参数规模为：TWE初始化表12项、主表1182项；WF初始化表12项、主表1236项。这些数量只用于核对当前版本，新导出参数的主表数量允许变化。

## 2. 新功放参数的安全替换方法

### 2.1 为什么不能整文件覆盖

厂家工具导出的C文件是代码模板，不是本SDK可直接编译的驱动。模板可能包含双层花括号、未定义的`uint8/uint16`、通用函数名、占位I²C函数和重复全局符号。直接放进工程源码目录会产生语法错误或链接重定义。

`new_amp_parameter/`仅用于保存原始导出文件和备份，已经在`.cproject`中排除，不参与AndeSight构建。正式编译的文件始终位于`app_src/components/audio/amp_driver/`。

### 2.2 推荐替换步骤

1. 将厂家原始导出文件复制到`new_amp_parameter/`留档，不修改原件。
2. 备份当前TWE、WF参数文件，备份文件不要放到会参与编译的源码目录。
3. 根据实际功放用途确认文件归属：全频对应TWE/`0x58`，低音对应WF/`0x5A`，不要仅凭文件名猜测。
4. 从厂家文件中提取且仅提取两个寄存器表：`m_reg_tab_initialization[]`和`m_reg_tab[]`。
5. 分别写入正式文件中的`m_reg_tab_initialization_TWE/WF`和`m_reg_tab_TWE/WF`，保持`REG_TAB`的“寄存器地址、数据”顺序完全一致。
6. 保留正式文件中的SDK适配代码，包括头文件、I²C地址、`ACM86xx_Write_REG_*()`、读寄存器、Mute/UnMute、PostEQ、音量和初始化函数。
7. 保留`ACM8625_VolumeCoefficientGet()`及两路`*_volumeControl()`。当前音量是逻辑0～100档：1～100对应约-58.5～0 dB的Q23系数，0由上层执行硬件静音；不要恢复厂家模板中的16档音量表。
8. 检查数组首尾、元素数量、页切换寄存器以及最后的启动/数字开通寄存器，防止复制少一行或多一个括号。
9. C/H文件保存为GB2312/CP936并保留原换行格式。执行AndeSight **Clean Project** 后再 **Build Project**，确认日志中没有编译`new_amp_parameter/`。

### 2.3 替换后的验证

- 冷启动确认两颗功放均完成I²C初始化，没有某一路始终静音。
- 固定正弦音源分别听全频和低音，确认分频、相位、增益和声道方向符合调音目标。
- 通过RL协议测试扬声器音量`0/1/25/50/75/100`和显式Mute/UnMute。
- 音量0必须完全静音；1～100应逐级增大；TWE和WF应同步变化。
- 若新参数改变了PDN时序、I²C地址、寄存器格式或音量寄存器位置，不能仅替换表格，必须单独审查驱动适配层。

## 3. RL串口协议代码结构

### 3.1 入口和运行时序

协议主体位于`app_src/components/headphone/headphone.c`，由`CFG_FUNC_OXS_PROTOCOL_EN`控制。虽然历史宏和部分内部变量仍保留`OXS`命名，对外维护函数统一使用`RL_`前缀。

UART1使用PA14/PA15、115200、8N1。开机最初3秒用于打印有限的启动日志；`RL_ProtocolPoll()`到达截止时间后调用`UartLogEnterProtocolMode()`，清空UART FIFO并启用RX中断。此后UART1只允许二进制协议数据，不应再调用日志打印。Shell默认关闭，USB仅用于升级。

主数据流如下：

```text
UART1 RX中断
  -> 256字节环形缓冲
  -> RL_ProtocolPoll()
  -> RL_ParserFeed()
  -> 帧头/长度/帧尾/CRC校验
  -> RL_ProcessFrame()
  -> 命令处理与硬件应用
  -> RL_SendCommandResult()/RL_SendError()
  -> UART1发送回复
```

中断函数只搬运字节，最多一次读取32字节，不在中断里计算CRC、操作功放或写Flash。所有耗时工作均在主任务轮询中执行。

### 3.2 帧格式和解析限制

- 帧头线上字节固定为`4F 52 45 50`。
- 多字节字段均为小端序，使用`RL_ReadU16/U32()`和`RL_WriteU16/U32()`，不要直接强制转换未对齐指针。
- Packet Length包含完整数据包，最小17字节、最大64字节。
- 帧尾为`5A 5A`，CRC覆盖从帧头到帧尾，CRC低字节先发送。
- 接收环形缓冲为256字节，残帧100 ms超时；错误帧静默丢弃并滑动搜索下一帧头。
- CRC由`RL_Crc16Arc()`实现，参数为CRC-16/IBM/ARC：初值`0x0000`、反射多项式`0xA001`。`middleware/mv_utils/src/crc16.c`使用的是CRC-CCITT，不能直接替换，否则现有上位机帧全部校验失败。

## 4. 关键函数说明

|函数|职责|修改注意事项|
|---|---|---|
|`RL_ProtocolInit()`|恢复持久化状态、配置UART1、建立3秒切换截止时间|默认音量均为50，Mute均关闭|
|`UART_Interrupt()`/`UART1_Interrupt()`|把UART1字节写入环形缓冲|保持短小，不调用硬件控制和Flash接口|
|`RL_ProtocolPoll()`|日志/协议切换、残帧超时、消费环形缓冲、延迟保存|必须在`main_task.c`主循环持续调用|
|`RL_ParserFeed()`|逐字节匹配帧头，检查长度、帧尾和CRC|帧长上限改变时同步检查缓冲区大小|
|`RL_ProcessFrame()`|校验Packet Type并按Cmd分发|新增命令时在这里增加`case`|
|`RL_SetVolume()`|检查Payload、输出枚举、范围和当前模式，应用音量|扬声器和耳机状态相互独立|
|`RL_SetMute()`|设置显式静音标志并应用输出|音量0静音不应篡改显式Mute标志|
|`RL_ApplySpeakerOutput()`|将0～100音量写入两颗ACM8625S并控制Mute|由`ACM8625_VolSet()`同步控制TWE/WF|
|`RL_ApplyHeadphoneOutput()`|计算耳机DAC增益并结合插入状态使能输出|还受USB保持和系统瞬态静音约束|
|`RL_SendPacket()`|组包、写帧尾、计算CRC并通过UART1发送|回复必须沿用请求Packet ID|
|`RL_SendCommandResult()`|按Ack规则返回成功数据包或错误ACK|语义错误即使Ack=0也必须返回错误|
|`RL_SendHeadphoneEvent()`|主动上报插入/拔出及有效阻抗|主动Packet ID从1递增并跳过0|
|`RL_ScheduleSave()`|启动2秒合并保存计时|避免每次调音量立即擦写Flash|

## 5. 修改或新增协议命令

新增命令建议严格按以下顺序进行：

1. 在协议常量区增加唯一Cmd值，并在协议Markdown中定义Payload长度、字段类型、字节序和错误行为。
2. 编写独立的`RL_SetXxx()`或查询函数，先校验长度，再校验枚举/范围/模式，最后修改状态和硬件。
3. 在`RL_ProcessFrame()`的`switch`中增加分发，返回现有`OXS_ERROR_E`错误码；确有需要时再扩展错误码并同步上位机文档。
4. 在`RL_SendCommandResult()`中定义Ack=1时的成功Payload。Ack=0成功不回复，但任何语义错误都返回错误ACK。
5. 如果是设备主动事件，复用`RL_NextEventPacketId()`和`RL_SendPacket()`，Packet Type使用DATA，不要使用请求Packet ID。
6. 若状态需要掉电保存，扩展`BP_SYS_INFO`时必须保持在现有断点分区内，处理旧结构不匹配，并继续使用2秒合并保存。
7. 更新《OXS功放通信协议文档V1.md》和《RL串口协议功能测试文档.md》，给出可直接发送的HEX帧与期望回复。

## 6. 协议回归清单

- 使用HEX发送，不使用ASCII字符串发送。
- 覆盖完整帧、拆包、粘包、错误帧头、非法长度、错误帧尾、错误CRC和100 ms残帧超时。
- 覆盖Ack=0和Ack=1、未知命令、错误Packet Type、错误输出枚举、参数越界及非LineIn模式。
- 核对回复Packet ID、Cmd、Payload、总长度和CRC。
- 测试耳机插拔主动上报、16/32 Ω一致识别和未知阻抗`-1`。
- 测试音量与Mute后等待至少2秒，再断电确认四项状态恢复。
- 确认协议接管后UART1不再混入文本日志；若要观察启动日志，应在上电后3秒内查看。

