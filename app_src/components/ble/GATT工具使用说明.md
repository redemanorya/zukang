# GATT 工具使用说明

## 工具简介

`gatt_tools_b6.exe` 读取当前目录下的 `input.gatt` 文件，自动生成两个 C 文件：

| 输出文件 | 内容 |
|----------|------|
| `ble_app_gatt.h` | ATT DB 静态数组 + handle 枚举 + extern 声明 |
| `ble_app_gatt.c` | Profile 注册表 + size 常量 |

---

## 使用方法

将 `gatt_tools_b6.exe` 放入 `input.gatt` 所在目录，**双击运行**即可。

成功时终端显示：
`
[OK] Generated: ble_app_gatt.h and ble_app_gatt.c
`

---

## input.gatt 编写格式

### 基本语法

`
PRIMARY_SERVICE, <服务UUID>
CHARACTERISTIC, <特征UUID>, <权限>
`

- 每个 PRIMARY_SERVICE 后跟若干 CHARACTERISTIC
- 服务之间用**空行**分隔
- `#` 开头的行为注释

### UUID 格式

| 类型 | 格式 | 示例 |
|------|------|------|
| 16-bit 标准服务 | 名称常量 | `GAP_SERVICE`, `GATT_SERVICE` |
| 16-bit 自定义服务 | 4位十六进制 | `AB00` |
| 16-bit 标准特征 | 名称常量 | `GAP_DEVICE_NAME`, `GAP_APPEARANCE` |
| 16-bit 自定义特征 | 4位十六进制 | `AB01`, `2A2B` |
| 128-bit UUID | 标准连字符格式 | `0000FD50-0000-1000-8000-00805F9B34FB` |

### 权限关键字

| 关键字 | 说明 |
|--------|------|
| `READ` | 可读 |
| `WRITE` | 可写（带响应） |
| `WRITE_WITHOUT_RESPONSE` | 可写（无响应） |
| `NOTIFY` | 通知（自动添加 CCCD 描述符） |
| `INDICATE` | 指示（自动添加 CCCD 描述符） |
| `DYNAMIC` | 动态值（叠加使用，生成时忽略） |

多个权限用 `|` 分隔，例如：`READ | WRITE | DYNAMIC`

### 示例

`
# GAP 服务
PRIMARY_SERVICE, GAP_SERVICE
CHARACTERISTIC, GAP_DEVICE_NAME, READ | WRITE | DYNAMIC,
CHARACTERISTIC, GAP_APPEARANCE, READ | DYNAMIC,

# GATT 服务
PRIMARY_SERVICE, GATT_SERVICE
CHARACTERISTIC, 2A2B, READ | DYNAMIC

# 自定义服务
PRIMARY_SERVICE, AB00
CHARACTERISTIC, AB01, READ | WRITE | DYNAMIC,
CHARACTERISTIC, AB02, NOTIFY | DYNAMIC,
CHARACTERISTIC, AB03, NOTIFY | DYNAMIC,
`

---

## 生成文件说明

### ble_app_gatt.h

- 包含所有 ATT DB 数组（static，多文件 include 不会重复定义）
- 末尾有 extern 声明供其他 .c 文件引用

### ble_app_gatt.c

- 包含 `profile_att_handle_t` 枚举（各特征的 handle 编号）
- 包含 `mv_profile1[]` Profile 注册表
- 包含 `MV_profile_db_size` 大小常量
- **首次使用时需将此文件加入项目编译列表**

---

## 注意事项

- 生成文件顶部标注 `DO NOT EDIT MANUALLY`，每次运行都会覆盖，请勿手动修改
- 工具必须在 `input.gatt` 所在目录下运行
