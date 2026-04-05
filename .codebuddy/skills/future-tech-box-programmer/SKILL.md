---
name: future-tech-box-programmer
description: This skill should be used when users want to program the Future Tech Box 2.0 (未来科技盒) board based on XIAO ESP32S3. It handles the complete workflow from natural language requirement to code generation, compilation and flashing. Trigger phrases include "编程未来科技盒", "烧录程序到主板", "让LED亮起来", "控制电机", or any hardware control request mentioning 未来科技盒/XIAO ESP32S3.
---

# 未来科技盒 2.0 自动编程烧录

## 概述
本 skill 实现从用户自然语言需求到代码生成、编译、烧录的完整自动化流程。

**目标硬件**：未来科技盒 2.0（基于 Seeed XIAO ESP32S3）  
**开发框架**：PlatformIO + Arduino  
**支持系统**：Windows / macOS / Linux

---

## 🌐 跨平台支持说明

本 SKILL 支持三种操作系统：

| 系统 | 串口命名 | 检测脚本 | 特殊说明 |
|------|----------|----------|----------|
| **Windows** | `COMx` | `detect_port_windows.py` | 使用 pnputil 检测 |
| **macOS** | `/dev/cu.usbmodem*` | `detect_port_macos.py` | 使用 system_profiler/ioreg |
| **Linux** | `/dev/ttyACM*` | `detect_port_linux.py` | 可能需要 dialout 用户组权限 |

### Linux 用户首次使用注意

如果遇到权限问题，需要将用户添加到 `dialout` 组：

```bash
sudo usermod -a -G dialout $USER
# 然后重新登录或重启
```

### macOS 用户注意

- 驱动通常自动安装
- 串口名称格式为 `/dev/cu.usbmodem*`
- 如果未识别到设备，可能需要安装 CH340 驱动（某些版本主板）

---

## ⚠️ 首次执行说明

首次使用本 skill 时，系统需要进行环境准备，包括：

### 需要下载的组件（首次自动下载）

| 组件 | 大小 | 说明 |
|------|------|------|
| **ESP32 平台包** | ~100-200MB | espressif32 平台定义 |
| **Arduino 框架** | ~50-100MB | arduino-esp32 框架 |
| **ESP32-S3 工具链** | ~200-300MB | xtensa-esp32s3-elf-gcc 编译器 |

### 首次编译耗时预估

- **网络良好**：5-10 分钟
- **网络一般**：10-20 分钟
- **后续编译**：10-30 秒（已缓存）

### 首次执行前的用户提示模板

在执行任何编程任务前，如检测到环境未就绪，向用户显示：

```
╔══════════════════════════════════════════════════════════════════╗
║          ⏳ 首次执行环境准备说明                                  ║
╠══════════════════════════════════════════════════════════════════╣
║                                                                  ║
║  检测到这是首次使用未来科技盒编程功能，需要下载以下组件：          ║
║                                                                  ║
║  📦 ESP32 平台包 ................ 约 100-200 MB                 ║
║  📦 Arduino 框架 ................ 约 50-100 MB                  ║
║  📦 ESP32-S3 编译工具链 ......... 约 200-300 MB                 ║
║                                                                  ║
║  ⏱️  预计耗时：5-20 分钟（取决于网络速度）                        ║
║  💾 下载位置：~/.platformio/                                     ║
║                                                                  ║
║  下载完成后，后续编译将在 10-30 秒内完成。                        ║
║                                                                  ║
╠══════════════════════════════════════════════════════════════════╣
║  是否继续？请回复"确认"开始下载，或"取消"终止操作。               ║
╚══════════════════════════════════════════════════════════════════╝
```

---

## 执行流程

### Phase 0: 环境扫描

**执行脚本**：`python scripts/check_environment.py`

**检测项目**：
1. Python 版本（≥3.8）
2. PlatformIO CLI（≥6.0）
3. 串口连接状态
4. ESP32 平台包
5. ESP32-S3 工具链缓存状态
6. **Arduino 框架缓存状态（关键）**

**输出格式**：JSON
```json
{
  "python": {"installed": true, "version": "3.11.9", "ok": true},
  "platformio": {"installed": true, "version": "6.1.19", "ok": true},
  "serial": {"found": true, "port": "COM6", "vid": "303A", "pid": "1001", "ok": true},
  "platform": {"installed": true, "version": "6.13.0", "ok": true},
  "toolchain": {"cached": true, "path": "...", "ok": true},
  "framework": {
    "cached": true,
    "version": "3.20017.241212",
    "ok": true,
    "api_note": "Arduino Core 3.x uses ledcAttach(), 2.x uses ledcSetup()+ledcAttachPin()"
  },
  "ready": true,
  "warnings": [],
  "missing": [],
  "estimated_first_compile": "10-30 seconds"
}
```

> ⚠️ **关键预判点**：如果 `framework.cached` 为 `false`，首次编译将触发 Arduino 框架下载（50-100MB），
> 可能导致编译命令长时间无响应。必须在执行前告知用户。

**环境就绪后向用户显示**：
```
╔══════════════════════════════════════════════════════════════════╗
║            ✅ 环境检测通过                                        ║
╠══════════════════════════════════════════════════════════════════╣
║  [✓] Python 3.11.9                                               ║
║  [✓] PlatformIO 6.1.19                                           ║
║  [✓] 串口 COM6 (XIAO ESP32S3)                                    ║
║  [✓] ESP32-S3 工具链已缓存                                        ║
╠══════════════════════════════════════════════════════════════════╣
║  请输入您想要实现的功能，例如：                                    ║
║  • "让 LED 灯逐一亮起呼吸灯效果"                                  ║
║  • "按下按键 A 时蜂鸣器响一声"                                    ║
║  • "让小车前进 3 秒后停止"                                        ║
╚══════════════════════════════════════════════════════════════════╝
```

---

### Phase 1: 需求理解与代码生成

**输入**：用户自然语言描述  
**处理**：
1. 解析用户意图，识别涉及的硬件模块
2. 读取 `references/pinout_mapping.csv` 获取引脚映射
3. 读取 `references/future_tech_box_v2_hardware.md` 获取硬件约束
4. 生成符合 PlatformIO 结构的代码

**代码生成前向用户显示**：
```
📝 正在生成代码...
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
识别到的硬件模块：
  • LED 矩阵 (GPIO33-38)
  • [其他涉及的模块]

即将生成的文件：
  • src/main.cpp
  • include/pins.h
  • platformio.ini
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

---

### Phase 2: 编译

**执行命令**：`pio run -d <project_path>`

> ⚠️ **重要提示**：由于 CodeBuddy 的命令执行有超时限制，如果编译命令长时间无响应，
> 应引导用户在终端手动执行编译命令。这不是错误，而是正常的超时保护机制。

**编译前向用户显示**：
```
🔨 正在编译程序...
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
目标平台：ESP32-S3 (XIAO ESP32S3)
框架：Arduino
预计耗时：10-30 秒（首次编译可能需要更长时间）

⏳ 编译中，请稍候...
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

**如果编译命令被跳过或超时，向用户显示**：
```
⏱️ 编译命令执行超时
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
这是正常现象，编译过程可能需要较长时间。

请在终端手动执行以下命令完成编译和烧录：

1. 打开终端（按 Ctrl+` 或在 VS Code 菜单选择"终端"）

2. 执行编译：
   cd "<项目完整路径>"
   pio run

3. 编译成功后执行烧录：
   pio run -t upload

4. （可选）查看串口输出：
   pio device monitor
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

**编译成功后向用户显示**：
```
✅ 编译成功！
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
固件大小：xxx KB (占用 xx%)
RAM 使用：xxx KB (占用 xx%)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

**编译失败处理**：
1. 解析错误信息
2. 尝试自动修复（语法错误、缺失头文件等）
3. 最多重试 2 次
4. 仍失败则向用户报告具体错误

---

### Phase 3: 烧录

> ⚠️ **重要：采用「先烧录，失败后再确认」策略**
>
> **不需要**在烧录前弹出确认框让用户点击。直接进行烧录，过程中提醒用户，如果失败再让用户确认重启后继续。

#### 3.1 烧录执行策略

**主烧录命令**：`pio run -t upload -d <project_path>`

**烧录开始时显示**（仅作提醒，不需要等待用户确认）：
```
📤 正在烧录到主板...
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
目标端口：COM6
烧录速度：921600 bps
预计耗时：10-20 秒

⚠️  烧录过程中请勿断开 USB 连接！
💡 如遇烧录失败，可能需要按一下 RST 复位按钮后重试

⏳ 烧录中，请稍候...
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

#### 3.2 烧录失败自动处理流程

**如果主命令失败（错误码非 0 或超时），执行自动重试策略：**

```
烧录失败自动处理流程：
┌─────────────────────────────────────────────────────────┐
│  Step 1: 自动等待 3 秒后重试 pio run -t upload          │
│          最多自动重试 2 次（无需用户确认）               │
└─────────────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────┐
│  Step 2: 如果 2 次自动重试仍失败                        │
│          → 向用户显示提醒，要求确认重启后继续            │
└─────────────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────┐
│  Step 3: 用户确认后，再重试烧录                         │
│          如果仍失败，改用 esptool 直接烧录              │
└─────────────────────────────────────────────────────────┘
```

**自动重试时显示**（简短提示）：
```
⏳ 烧录遇到问题，自动重试中 (第 x 次)...
```

#### 3.3 自动重试失败后的用户交互

**只有在自动重试 2 次仍失败后，才向用户显示确认框：**

```
╔══════════════════════════════════════════════════════════════════╗
║          ⚠️ 烧录失败，需要您的操作                                ║
╠══════════════════════════════════════════════════════════════════╣
║                                                                  ║
║  自动重试 2 次后仍然失败。                                        ║
║  可能是上一个程序正在运行，导致串口被占用。                        ║
║                                                                  ║
║  请执行以下操作后回复"继续"：                                     ║
║                                                                  ║
║  ✅ 【推荐】按一下主板上的 RST（复位）按钮                        ║
║                                                                  ║
║  或者：                                                           ║
║  • 拔插一次 USB 数据线                                            ║
║  • 关闭可能占用串口的程序（串口监视器、Arduino IDE 等）           ║
║                                                                  ║
╚══════════════════════════════════════════════════════════════════╝
```

#### 3.4 备用烧录命令

**如果 pio 烧录始终失败，尝试使用 esptool 直接调用**：
```bash
python %USERPROFILE%\.platformio\packages\tool-esptoolpy\esptool.py ^
  --chip esp32s3 --port {PORT} --baud 921600 ^
  write_flash -z --flash_mode dio --flash_freq 80m --flash_size 8MB ^
  0x0 {PROJECT}\.pio\build\seeed_xiao_esp32s3\bootloader.bin ^
  0x8000 {PROJECT}\.pio\build\seeed_xiao_esp32s3\partitions.bin ^
  0x10000 {PROJECT}\.pio\build\seeed_xiao_esp32s3\firmware.bin
```

#### 3.5 烧录成功提示

**烧录成功后显示**：
```
🎉 烧录成功！
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
程序已写入主板，即将自动运行。

如需查看串口输出，请说"打开串口监视器"。
如需修改程序，请直接描述新的需求。
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

---

## 支持的硬件功能

| 功能 | 状态 | 示例指令 |
|------|------|----------|
| LED 矩阵 | ✅ | "让 LED 呼吸灯效果" |
| 按键 | ✅ | "按下按键 A 时..." |
| 蜂鸣器 | ✅ | "播放一段音乐" |
| 超声波传感器 | ✅ | "测量前方距离" |
| **电机控制** | ✅ | "让小车前进"、"原地左转" |
| **循迹传感器** | ✅ | "实现循迹小车" |
| **舵机/机械臂** | ✅ | "控制机械臂抓取物体" |
| **PS2 手柄** | ✅ | "用手柄遥控小车" |
| **I2C 加速度计** | ✅ | "检测倾斜方向" |
| **I2C 颜色传感器** | ✅ | "识别颜色" |
| **I2C 温湿度** | ✅ | "读取当前温度" |

✅ = 已支持

### 小车形态电机布局

```
        前方
    ┌─────────┐
    │  M1  M2 │   M1=左上(GPIO11/12)  M2=右上(GPIO14/13)
    │         │
    │  M3  M4 │   M3=左下(GPIO15/16)  M4=右下(GPIO18/17)
    └─────────┘
        后方
```

**⚠️ 重要**：右侧电机 (M2, M4) 的正反转 GPIO 顺序与左侧相反：
- M1(左上): GPIO11=正转, GPIO12=反转
- M2(右上): GPIO14=正转, GPIO13=反转 ← 注意顺序
- M3(左下): GPIO15=正转, GPIO16=反转
- M4(右下): GPIO18=正转, GPIO17=反转 ← 注意顺序

**电机控制代码模板**：
```cpp
#define M1_FWD 11
#define M1_REV 12
#define M2_FWD 14  // 注意：不是13
#define M2_REV 13
#define M3_FWD 15
#define M3_REV 16
#define M4_FWD 18  // 注意：不是17
#define M4_REV 17

void setMotors(int leftSpeed, int rightSpeed) {
  // leftSpeed/rightSpeed: -255~255, 正数前进，负数后退
  setMotor(M1_FWD, M1_REV, leftSpeed);
  setMotor(M2_FWD, M2_REV, rightSpeed);
  setMotor(M3_FWD, M3_REV, leftSpeed);
  setMotor(M4_FWD, M4_REV, rightSpeed);
}

// 前进: setMotors(180, 180);
// 后退: setMotors(-180, -180);
// 左转: setMotors(-150, 150);
// 右转: setMotors(150, -150);
```

### I²C 传感器接口

| 传感器 | 型号 | I²C 地址 | 库名称 |
|--------|------|----------|--------|
| 加速度计 | LIS3DHTR | 0x18 | `LIS3DHTR` |
| 颜色传感器 | VEML6040 | 0x10 | `VEML6040` |
| 温湿度 | DHT20 | 0x38 | `Grove Temperature And Humidity Sensor` |

**I²C 初始化必须指定引脚**：
```cpp
#include <Wire.h>
Wire.begin(39, 40);  // SDA=GPIO39, SCL=GPIO40
```

### PS2 手柄引脚

| 功能 | GPIO |
|------|------|
| CLK | GPIO41 |
| CMD | GPIO9 |
| CS | GPIO42 |
| DAT | GPIO10 |

### 循迹传感器引脚

| 传感器 | GPIO | 接口 |
|--------|------|------|
| 左循迹 | GPIO2 | 接口3-PIN1 |
| 右循迹 | GPIO1 | 接口3-PIN2 |

### ⚠️ LED 矩阵控制要点（必读）

**这是 3×3 行列扫描 LED 矩阵，控制逻辑如下：**

| 行引脚状态 | 列引脚状态 | LED 状态 |
|-----------|-----------|----------|
| **HIGH**  | **LOW**   | ✅ 点亮  |
| HIGH      | HIGH      | ❌ 熄灭  |
| LOW       | LOW       | ❌ 熄灭  |
| LOW       | HIGH      | ❌ 熄灭  |

**⚠️ LED 物理位置映射（重要！）**：
```
物理视角（面对主板）：
     列1(36)  列2(37)  列3(38)
行3(35)  1        2        3      ← 物理上方
行2(34)  4        5        6      ← 物理中间
行1(33)  7        8        9      ← 物理下方
```

**LED 编号转数组索引公式**：
```cpp
int row = 2 - ((num - 1) / 3);  // LED 1-3→row 2, LED 4-6→row 1, LED 7-9→row 0
int col = (num - 1) % 3;
```

**生成 LED 控制代码时必须遵循：**
1. 使用 `ledState[3][3]` 数组存储 LED 状态（1=点亮，0=熄灭）
2. 在 `loop()` 中持续调用 `scanDisplay()` 扫描函数
3. 使用 `setLED(num, state)` 函数按编号（1-9）控制 LED
4. 点亮 LED：`setLED(num, 1)` 或 `turnOnLED(num)`
5. 熄灭 LED：`setLED(num, 0)` 或 `turnOffLED(num)`
6. **禁止**直接用 `digitalWrite(列引脚, HIGH/LOW)` 来控制单个 LED

**详细示例代码参见**：`references/future_tech_box_v2_hardware.md` 的 LED 矩阵章节

### ⚠️ 按键控制要点（必读）

**按键使用内部上拉电阻，按下时为 LOW，松开时为 HIGH。**

| 按键 | GPIO | 说明 |
|------|------|------|
| KEY_A | GPIO21 | 按下时 LOW |
| KEY_B | GPIO0 | 按下时 LOW（兼 BOOT） |

**🚨 必须使用边沿检测方式检测按键！**

**场景1：无 LED 矩阵扫描时（可以用 delay 消抖）**
```cpp
bool lastKeyA = HIGH;

void loop() {
  bool currentKeyA = digitalRead(KEY_A);
  
  if (lastKeyA == HIGH && currentKeyA == LOW) {
    doSomething();
    delay(50);  // ✅ 没有 LED 扫描时可以用 delay
  }
  
  lastKeyA = currentKeyA;
}
```

**场景2：有 LED 矩阵扫描时（禁止用 delay！）**
```cpp
bool lastKeyA = HIGH;
unsigned long lastKeyTime = 0;  // 用于非阻塞消抖

void loop() {
  bool currentKeyA = digitalRead(KEY_A);
  
  // 边沿检测 + 非阻塞消抖
  if (lastKeyA == HIGH && currentKeyA == LOW) {
    if (millis() - lastKeyTime > 50) {  // 50ms 消抖
      doSomething();
      lastKeyTime = millis();
    }
  }
  
  lastKeyA = currentKeyA;
  scanDisplay();  // ⚠️ 不能被 delay 阻塞！
}
```

**生成按键控制代码时必须遵循：**
1. 使用边沿检测 `if (lastKey == HIGH && currentKey == LOW)`
2. 必须更新状态 `lastKey = currentKey`
3. **有 LED 扫描时禁止使用 `delay()` 消抖**，改用 `millis()` 非阻塞方式
4. 为每个按键创建独立的状态变量

### ⚠️ 蜂鸣器控制要点（必读）

| 功能 | GPIO | 说明 |
|------|------|------|
| 蜂鸣器 | GPIO26 | 有源/无源蜂鸣器 |

**简单播放方式**：
```cpp
#define BUZZER_PIN 26

void beep(int freq = 1000, int duration = 100) {
  tone(BUZZER_PIN, freq, duration);  // 使用 Arduino tone() 函数
}
```

**🚨 有 LED 矩阵扫描时必须用非阻塞方式！**

```cpp
#define BUZZER_PIN 26
unsigned long buzzerEndTime = 0;
bool buzzerActive = false;

// 非阻塞播放
void beepNonBlocking(int duration = 100) {
  tone(BUZZER_PIN, 1000);
  buzzerEndTime = millis() + duration;
  buzzerActive = true;
}

// 在 loop 中调用检查
void updateBuzzer() {
  if (buzzerActive && millis() >= buzzerEndTime) {
    noTone(BUZZER_PIN);
    buzzerActive = false;
  }
}

void loop() {
  if (lastKeyA == HIGH && currentKeyA == LOW) {
    beepNonBlocking(100);  // ✅ 非阻塞
    turnOnAllLEDs();
  }
  lastKeyA = currentKeyA;
  
  updateBuzzer();   // 检查蜂鸣器
  scanDisplay();    // LED 扫描不会被阻塞
}
```

**❌ 禁止这样写**：
```cpp
// 错误1：使用旧的 ledcSetup API（每次都重新设置）
void playBuzzer() {
  ledcSetup(0, 1000, 8);        // ⚠️ 旧 API，且不应每次都调用
  ledcAttachPin(BUZZER_PIN, 0); // ⚠️ 不应每次都调用
  ledcWrite(0, 128);
}

// 错误2：阻塞式播放（会导致 LED 闪烁/卡顿）
tone(BUZZER_PIN, 1000);
delay(100);  // ⚠️ 阻塞了 LED 扫描！
noTone(BUZZER_PIN);
```

**生成蜂鸣器控制代码时必须遵循：**
1. 使用 `tone(pin, freq, duration)` 或 `tone(pin, freq)` + `noTone(pin)`
2. **有 LED 扫描时必须用非阻塞方式**：`millis()` 计时 + `updateBuzzer()` 检查
3. 禁止使用 `ledcSetup()`/`ledcAttachPin()` 旧 API
4. 禁止在 LED 扫描的程序中使用 `delay()` 等待蜂鸣器

### ⚠️ 非阻塞编程原则（重要！）

**当程序中有 LED 矩阵扫描时，`loop()` 中禁止使用任何阻塞操作！**

```cpp
void loop() {
  // ❌ 禁止：
  delay(50);           // 阻塞 50ms
  while(条件) { ... }  // 可能长时间阻塞
  
  // ✅ 必须：
  scanDisplay();       // 每次 loop 都要调用，不能被阻塞
}
```

**所有需要延时的操作都改用 `millis()` 非阻塞方式：**
- 按键消抖：用 `millis() - lastKeyTime > 50` 代替 `delay(50)`
- 蜂鸣器定时：用 `millis() >= buzzerEndTime` 代替 `delay(duration)`
- 动画效果：用 `millis() - lastFrameTime > interval` 代替 `delay(interval)`

---

## 引用资源

- **引脚映射**：`references/pinout_mapping.csv`
- **硬件规格**：`references/future_tech_box_v2_hardware.md`
- **环境检测**：`scripts/check_environment.py`（自动检测操作系统）
- **烧录脚本**：`scripts/upload_with_retry.py`（带自动重试）
- **串口识别**（根据系统自动选择）：
  - Windows：`scripts/detect_port_windows.py`
  - macOS：`scripts/detect_port_macos.py`
  - Linux：`scripts/detect_port_linux.py`

---

## 项目模板结构

生成的 PlatformIO 项目结构：

```
<project_name>/
├── platformio.ini      # 平台配置
├── include/
│   └── pins.h          # 引脚定义
├── src/
│   └── main.cpp        # 主程序
└── lib/                # 第三方库（按需）
```

---

## 约束与限制

1. 仅支持未来科技盒 2.0（XIAO ESP32S3）
2. 需要稳定的网络连接（首次下载依赖）
3. USB 必须是数据线（非充电线）
4. 某些复杂功能可能需要用户确认细节
5. Linux 用户需要 dialout 用户组权限

---

## 常见问题排查指南

### 问题 1：编译命令长时间卡住（看似无响应）

**症状**：执行 `pio run` 后超过 1 分钟无任何输出

**根因分析**：
| 可能原因 | 概率 | 验证方法 |
|----------|------|----------|
| Arduino 框架未下载 | ⭐⭐⭐ 高 | 运行 `check_environment.py` 检查 `framework.cached` |
| 网络连接问题 | ⭐⭐ 中 | 检查网络，尝试 ping github.com |
| 杀毒软件干扰 | ⭐ 低 | 检查 Windows Defender 活动 |

**解决方案**：
1. 先运行 `python scripts/check_environment.py` 检查框架状态
2. 如果 `framework.cached = false`，告知用户首次编译需要下载依赖
3. 预估时间：网络良好 5-10 分钟，网络一般 10-20 分钟

### 问题 2：编译错误 - ledcAttach 未定义

**症状**：
```
error: 'ledcAttach' was not declared in this scope
```

**根因**：Arduino ESP32 Core 版本 API 差异

| API | Arduino Core 2.x | Arduino Core 3.x |
|-----|------------------|------------------|
| PWM 配置 | `ledcSetup(channel, freq, resolution)` | `ledcAttach(pin, freq, resolution)` |
| 引脚绑定 | `ledcAttachPin(pin, channel)` | (合并到 ledcAttach) |
| 写入 PWM | `ledcWrite(channel, duty)` | `ledcWrite(pin, duty)` |

**解决方案**：

**蜂鸣器控制**：推荐使用 Arduino 标准 `tone()`/`noTone()` 函数（跨版本兼容）：
```cpp
tone(BUZZER_PIN, 1000, 100);  // 播放 1000Hz，持续 100ms
noTone(BUZZER_PIN);            // 停止播放
```

**PWM 控制（如电机调速）**：使用 2.x 兼容 API
```cpp
// 初始化（仅在 setup() 中调用一次）
ledcSetup(channel, PWM_FREQ, PWM_RESOLUTION);
ledcAttachPin(pin, channel);

// 写入
ledcWrite(channel, brightness);  // 注意：使用通道号，不是引脚号
```

**⚠️ 注意**：蜂鸣器请勿使用 `ledcSetup`/`ledcAttachPin`，直接用 `tone()` 更简单可靠。

### 问题 3：烧录失败 - 端口不可用

**症状**：
```
Error: Unable to open port
Error: PermissionError(13, '拒绝访问')
```

**根因分析**：
| 可能原因 | 概率 | 说明 |
|----------|------|------|
| 串口被短暂占用 | ⭐⭐⭐ 高 | 上一次烧录后设备重启，串口短暂锁定 |
| 串口监视器占用 | ⭐⭐ 中 | Arduino IDE/PlatformIO Monitor 等打开着 |
| 驱动问题 | ⭐ 低 | USB 驱动异常 |

**自动处理策略**（SKILL 应实现）：
1. **自动重试**：等待 2-3 秒后重试烧录，最多 3 次
2. **检测端口状态**：在烧录前运行 `detect_port_windows.py` 确认串口可用
3. **使用 esptool 直接烧录**：如果 `pio run -t upload` 失败，改用 esptool 直接调用

**手动排查步骤**：
1. 运行串口检测脚本：`python scripts/detect_port_windows.py`
2. 检查 USB 连接是否稳固
3. 尝试：按住主板 BOOT 按钮 → 插入 USB → 松开 BOOT
4. 检查设备管理器是否显示 COM 端口

**烧录命令备用方案**：
```bash
# 如果 pio run -t upload 失败，使用 esptool 直接烧录：
python %USERPROFILE%\.platformio\packages\tool-esptoolpy\esptool.py ^
  --chip esp32s3 --port COM6 --baud 921600 ^
  write_flash -z --flash_mode dio --flash_freq 80m --flash_size 8MB ^
  0x0 .pio\build\seeed_xiao_esp32s3\bootloader.bin ^
  0x8000 .pio\build\seeed_xiao_esp32s3\partitions.bin ^
  0x10000 .pio\build\seeed_xiao_esp32s3\firmware.bin
```

### 问题 4：CodeBuddy 命令执行超时

**症状**：命令被跳过，显示 "may take a long time"

**根因**：CodeBuddy 有命令执行超时保护（约 60-120 秒）

**这不是 BUG，是正常保护机制。**

**解决方案**：
1. 确保依赖已下载（`framework.cached = true`）
2. 依赖就绪后，编译仅需 8-10 秒，不会触发超时
3. 如果首次使用，引导用户在终端手动执行一次 `pio run` 完成依赖下载

---

## 问题预判检查清单

在执行编程任务前，运行以下检查：

```
✅ 检查项                        命令/方法                           通过条件
──────────────────────────────────────────────────────────────────────────────
[ ] Python 版本                  python --version                   >= 3.8
[ ] PlatformIO CLI               pio --version                      >= 6.0
[ ] 串口连接                      detect_port_windows.py             status=ok
[ ] ESP32 平台                   pio pkg list -g                    有 espressif32
[ ] Arduino 框架（关键）          check_environment.py               framework.cached=true
[ ] 工具链                        check_environment.py               toolchain.cached=true
```

**如果 Arduino 框架未缓存，必须提前告知用户**：
- 首次编译将下载 50-100MB 依赖
- 预计耗时 5-20 分钟
- 下载期间命令可能无响应（正常现象）

---

## 连续烧录场景处理（重要）

当用户在同一会话中多次烧录不同程序时，采用「**先尝试，失败后再确认**」策略。

### 场景识别

以下情况属于"连续烧录场景"：
1. 用户刚完成一个程序的烧录，又提出新的编程需求
2. 用户要求"再跑一个 case" 或 "换个功能试试"
3. 主板上当前正在运行之前烧录的程序

### 处理策略

**✅ 正确做法**：直接开始编译烧录，过程中提醒用户，失败后再让用户确认

**❌ 错误做法**：~~每次烧录前都弹出确认框让用户点击后才继续~~

### 自动处理流程

```
用户提出新需求
       ↓
直接生成代码、编译
       ↓
尝试烧录（过程中提示：如遇失败可能需按 RST）
       ↓
   ┌───┴───┐
   ↓       ↓
 成功    失败
   ↓       ↓
 完成   自动等待 3 秒，重试（最多 2 次）
           ↓
       ┌───┴───┐
       ↓       ↓
     成功    仍失败
       ↓       ↓
     完成   此时向用户显示确认框
              要求按 RST 后回复"继续"
```

### 烧录提示（融入过程中，无需等待确认）

在烧录开始时的提示信息中包含：
```
💡 如遇烧录失败，可能需要按一下 RST 复位按钮后重试
```

### 代码实现参考

在烧录函数中加入自动重试逻辑：

```python
def upload_with_retry(project_path, port, max_retries=2):
    """带自动重试的烧录函数 - 无需用户提前确认"""
    for attempt in range(max_retries + 1):
        if attempt > 0:
            print(f"⏳ 烧录遇到问题，自动重试中 (第 {attempt} 次)...")
            time.sleep(3)  # 等待 3 秒后重试
        
        # 尝试烧录
        result = run_upload_command(project_path)
        if result.success:
            return {"success": True, "attempts": attempt + 1}
        
        # 如果是权限错误，继续重试
        if "PermissionError" in result.error or "拒绝访问" in result.error:
            continue
        
        # 其他错误也继续重试
        continue
    
    # 所有自动重试都失败，此时才需要用户介入
    return {"success": False, "need_user_action": True, "error": result.error}
```
