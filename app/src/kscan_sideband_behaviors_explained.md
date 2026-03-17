# ZMK `kscan_sideband_behaviors.c` 源文件详解

## 1. 概述

`kscan_sideband_behaviors.c` 是 ZMK 固件中的一个**kscan 包装驱动**（wrapper driver）。它的核心作用是：

> **在底层 kscan 设备（如矩阵键盘扫描器、GPIO 按键等）和 ZMK 的 behavior 系统之间架起一座桥梁，使得特定的按键事件可以"旁路"（sideband）绕过正常的 keymap 处理流程，直接触发指定的 behavior。**

"Sideband"（旁路）意味着这些 behavior 的触发不经过 ZMK 的正常 keymap 层级处理管线，而是在 kscan 回调阶段立即执行。

---

## 2. 设备树绑定（DTS Binding）

对应的 devicetree compatible 字符串是 `zmk,kscan-sideband-behaviors`。

DTS binding 文件 (`zmk,kscan-sideband-behaviors.yaml`) 定义了如下结构：

```yaml
compatible: "zmk,kscan-sideband-behaviors"
include: kscan.yaml   # 它自身也是一个 kscan 设备

properties:
  auto-enable:         # 是否在系统启动时自动启用
    type: boolean
  kscan:               # 引用底层真正的 kscan 设备（phandle）
    type: phandle
    required: true

child-binding:         # 子节点：每个子节点定义一个旁路 behavior
  properties:
    row:               # 匹配的矩阵行号（默认0）
      type: int
      default: 0
    column:            # 匹配的矩阵列号
      type: int
      required: true
    bindings:          # 要触发的 behavior 绑定
      type: phandle-array
      required: true
```

**使用示例（概念性）**：
```dts
sideband_behaviors: kscan-sideband-behaviors {
    compatible = "zmk,kscan-sideband-behaviors";
    kscan = <&inner_kscan>;
    auto-enable;

    power_button: power_button {
        row = <0>;
        column = <3>;
        bindings = <&kp C_POWER>;
    };
};
```

这表示：当底层 `inner_kscan` 报告 row=0, column=3 的按键事件时，直接触发 `&kp C_POWER` 这个 behavior，而不经过 keymap 处理。

---

## 3. Zephyr KSCAN API 详解

这个驱动的核心是**实现了 Zephyr 的 `kscan_driver_api` 接口**，使自身也成为一个合法的 kscan 设备。

### 3.1 `kscan_driver_api` 结构体

定义在 `zephyr/include/zephyr/drivers/kscan.h` 中：

```c
typedef void (*kscan_callback_t)(const struct device *dev, uint32_t row,
                                 uint32_t column, bool pressed);

typedef int (*kscan_config_t)(const struct device *dev, kscan_callback_t callback);
typedef int (*kscan_disable_callback_t)(const struct device *dev);
typedef int (*kscan_enable_callback_t)(const struct device *dev);

__subsystem struct kscan_driver_api {
    kscan_config_t config;              // 配置回调函数
    kscan_disable_callback_t disable_callback;  // 禁用回调
    kscan_enable_callback_t enable_callback;    // 启用回调
};
```

这个 API 定义了三个核心操作：

| API 函数 | 系统调用 | 作用 |
|----------|---------|------|
| `kscan_config()` | `api->config(dev, callback)` | 注册一个回调函数，当按键事件发生时被调用 |
| `kscan_enable_callback()` | `api->enable_callback(dev)` | 启用回调，开始报告按键事件 |
| `kscan_disable_callback()` | `api->disable_callback(dev)` | 禁用回调，停止报告按键事件 |

### 3.2 `kscan_callback_t` 回调

```c
typedef void (*kscan_callback_t)(const struct device *dev,
                                 uint32_t row, uint32_t column, bool pressed);
```

- `dev`：产生事件的 kscan 设备指针
- `row`：按键所在行
- `column`：按键所在列
- `pressed`：`true` = 按下，`false` = 释放

---

## 4. 核心数据结构

### 4.1 `struct ksbb_entry` — 旁路行为条目

```c
struct ksbb_entry {
    struct zmk_behavior_binding binding;  // ZMK behavior 绑定（如 &kp A）
    uint8_t row;                          // 匹配的行号
    uint8_t column;                       // 匹配的列号
};
```

每个条目对应 DTS 中的一个子节点，定义了 "当 (row, column) 按键事件发生时，触发哪个 behavior"。

### 4.2 `struct ksbb_config` — 驱动配置（只读）

```c
struct ksbb_config {
    const struct device *kscan;     // 底层真正的 kscan 设备
    bool auto_enable;               // 是否自动启用
    struct ksbb_entry *entries;     // 旁路行为条目数组
    size_t entries_len;             // 条目数量
};
```

### 4.3 `struct ksbb_data` — 驱动运行时数据

```c
struct ksbb_data {
    kscan_callback_t callback;  // 上层注册的回调（用于透传非旁路事件）
    bool enabled;               // 当前是否启用
};
```

---

## 5. 关键函数逐一解析

### 5.1 静态设备数组 `ksbbs[]`

```c
static const struct device *ksbbs[] = {DT_INST_FOREACH_STATUS_OKAY(GET_KSBB_DEV)};
```

**为什么需要这个？** 因为 Zephyr 的 `kscan_callback_t` 回调签名中没有"上下文"参数（void *user_data），所以当底层 kscan 触发回调时，无法直接知道是哪个 KSBB 实例在使用它。解决方案是维护一个静态全局数组，在回调中遍历查找。

### 5.2 `find_ksbb_for_inner()` — 查找包装器

```c
const struct device *find_ksbb_for_inner(const struct device *inner_dev);
```

给定一个底层 kscan 设备指针，遍历 `ksbbs[]` 数组，找到包装（wrap）了它的 KSBB 设备。

### 5.3 `find_sideband_behavior()` — 查找旁路行为

```c
struct ksbb_entry *find_sideband_behavior(const struct device *dev,
                                           uint32_t row, uint32_t column);
```

在 KSBB 的条目列表中，按 (row, column) 查找匹配的旁路行为条目。

### 5.4 `ksbb_inner_kscan_callback()` — ★ 核心回调

```c
void ksbb_inner_kscan_callback(const struct device *dev,
                                uint32_t row, uint32_t column, bool pressed);
```

这是注册到底层 kscan 设备上的回调函数，是整个驱动的**核心逻辑**：

```
底层 kscan 按键事件
        │
        ▼
ksbb_inner_kscan_callback()
        │
        ├── 1. 通过 find_ksbb_for_inner() 找到对应的 KSBB 设备
        │
        ├── 2. 通过 find_sideband_behavior() 检查此 (row,col) 是否有旁路行为
        │       │
        │       ├── 有 → 直接调用 behavior_keymap_binding_pressed/released()
        │       │        （position = INT32_MAX，表示非 keymap 位置）
        │       │
        │       └── 无 → 不触发旁路行为
        │
        └── 3. 如果 KSBB 已启用且上层注册了回调 → 向上透传事件
                （这样 KSBB 的使用者也能收到所有按键事件）
```

**关键细节**：
- `position = INT32_MAX`：旁路行为不对应 keymap 中的任何真实位置，所以使用一个特殊值
- 旁路行为的触发**独立于**事件的向上透传——即使上层没有注册回调，旁路行为依然会触发
- 事件的向上透传受 `data->enabled` 和 `data->callback` 双重控制

### 5.5 `ksbb_configure()` — 实现 `kscan_config`

```c
static int ksbb_configure(const struct device *dev, kscan_callback_t callback);
```

仅保存上层传入的回调函数指针，不做其他操作。这个回调会在 `ksbb_inner_kscan_callback()` 中用于向上透传事件。

### 5.6 `ksbb_enable()` — 实现 `kscan_enable_callback`

```c
static int ksbb_enable(const struct device *dev);
```

流程：
1. 设置 `data->enabled = true`
2. 处理电源管理（PM）：
   - PM Runtime 模式：获取底层设备的引用计数
   - PM Device 模式：唤醒设备并恢复运行
3. 调用 `kscan_config(config->kscan, &ksbb_inner_kscan_callback)` — 将自己的回调注册到底层 kscan
4. 调用 `kscan_enable_callback(config->kscan)` — 启用底层 kscan 的回调

### 5.7 `ksbb_disable()` — 实现 `kscan_disable_callback`

```c
static int ksbb_disable(const struct device *dev);
```

流程与 enable 相反：
1. 设置 `data->enabled = false`
2. 调用 `kscan_disable_callback(config->kscan)` — 禁用底层 kscan 回调
3. 处理电源管理：释放引用计数或挂起设备

### 5.8 `ksbb_init()` — 设备初始化

```c
static int ksbb_init(const struct device *dev);
```

1. 检查底层 kscan 设备是否就绪
2. 如果未设置 `auto_enable` 且启用了 PM，则将设备标记为挂起状态

### 5.9 `ksbb_pm_action()` — 电源管理动作

```c
static int ksbb_pm_action(const struct device *dev, enum pm_device_action action);
```

将 PM 的 `SUSPEND` / `RESUME` 动作映射到 `ksbb_disable()` / `ksbb_enable()`。

---

## 6. API 实例注册

```c
static const struct kscan_driver_api ksbb_api = {
    .config = ksbb_configure,
    .enable_callback = ksbb_enable,
    .disable_callback = ksbb_disable,
};
```

这使得 KSBB 设备自身也是一个合法的 kscan 设备，可以被上层（如 ZMK 的核心 kscan 处理）使用。

---

## 7. 宏实例化（`KSBB_INST`）

`KSBB_INST(n)` 宏为每个 DTS 中声明的 `zmk,kscan-sideband-behaviors` 节点生成：

1. **自动启用函数**（如果 `auto-enable` 为 true）：
   - 通过 `SYS_INIT()` 在 `APPLICATION` 阶段自动调用
   - 自动配置底层 kscan 回调并启用

2. **条目数组** `entries_n[]`：
   - 使用 `DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP` 遍历所有子节点
   - 每个子节点通过 `ENTRY(e)` 宏提取 row, column, binding

3. **配置结构体** `ksbb_config_n`

4. **数据结构体** `ksbb_data_n`

5. **PM 设备定义** `PM_DEVICE_DT_INST_DEFINE`

6. **设备定义** `DEVICE_DT_INST_DEFINE`，初始化优先级为 `CONFIG_ZMK_KSCAN_SIDEBAND_BEHAVIORS_INIT_PRIORITY`（默认 95，高于普通 kscan 的 90，确保底层 kscan 先初始化）

---

## 8. 架构示意图

```
┌─────────────────────────────────────────────┐
│           ZMK Keymap / 上层系统              │
│   （通过 kscan API 使用 KSBB 作为 kscan）     │
└────────────────────┬────────────────────────┘
                     │ kscan_config(ksbb, callback)
                     │ kscan_enable_callback(ksbb)
                     ▼
┌─────────────────────────────────────────────┐
│         KSBB (kscan_sideband_behaviors)      │
│                                              │
│  ┌──────────┐   ksbb_inner_kscan_callback() │
│  │ entries[] │──► 匹配 (row,col)?            │
│  │ row=0,c=3 │    ├─ YES → behavior 直接触发 │
│  │ binding=… │    └─ NO  → 仅透传            │
│  └──────────┘                                │
│                     │ 透传给上层 callback      │
└────────────────────┬────────────────────────┘
                     │ kscan_config(inner, ksbb_cb)
                     │ kscan_enable_callback(inner)
                     ▼
┌─────────────────────────────────────────────┐
│           底层 KSCAN 设备                     │
│     （GPIO矩阵、直连按键、EC11编码器等）       │
└─────────────────────────────────────────────┘
```

---

## 9. 典型使用场景

1. **直连按键触发系统行为**：如硬件电源按键、复位按键，不需要经过 keymap 映射，直接绑定 behavior
2. **编码器旁路**：编码器的旋转事件直接触发音量调节等行为
3. **调试/特殊按键**：某些按键需要在 keymap 系统之外独立工作
4. **混合模式**：部分按键通过旁路处理，其余按键正常透传到 keymap 系统

---

## 10. 注意事项

根据 DTS binding 文档的警告：

> 只应使用**基础系统行为**（basic system behaviors）。不要使用 tap-hold、sticky keys 等复杂行为作为旁路行为，因为不经过 keymap 处理流程。

这是因为旁路行为的 `position` 被设为 `INT32_MAX`，不对应 keymap 中的真实位置，复杂行为可能依赖位置信息和 keymap 上下文来正常工作。

---

## 11. Kconfig 配置

```kconfig
config ZMK_KSCAN_SIDEBAND_BEHAVIORS
    bool
    default y
    depends on DT_HAS_ZMK_KSCAN_SIDEBAND_BEHAVIORS_ENABLED
    select KSCAN

config ZMK_KSCAN_SIDEBAND_BEHAVIORS_INIT_PRIORITY
    int "Keyboard scan sideband behaviors driver init priority"
    default 95   # 高于普通 kscan 的 90
```

- 当 DTS 中存在 `zmk,kscan-sideband-behaviors` 节点时自动启用
- 自动 `select KSCAN` 确保 kscan 子系统可用
- 初始化优先级 95 保证底层 kscan 设备（优先级 90）已经就绪
