---
name: future-tech-box-3.0-programmer
description: This skill should be used when users want to program the Future Tech Box 3.0 (未来科技盒3.0) board based on ESP32-S3. It handles the complete workflow from natural language requirement to code generation, compilation and flashing. Key differences from v2.0 include RGB LED matrix (WS2812), voice module, and vision/AI camera module. Trigger phrases include "编程未来科技盒3.0", "烧录程序到3.0主板", "RGB灯", "语音模块", "视觉模块", or any hardware control request mentioning 未来科技盒3.0.
---

# 未来科技盒 3.0 自动编程烧录（v3.0 Skill）

## 概述
本 skill 实现从用户自然语言需求到代码生成、编译、烧录的完整自动化流程。

**目标硬件**：未来科技盒 3.0（基于 ESP32-S3）  
**开发框架**：PlatformIO + Arduino  
**支持系统**：Windows / macOS / Linux  
**Skill 版本**：v0.1.0（初始版本）

### ⚠️ 与 2.0 版本的关键区别

| 特性 | 2.0 | 3.0 | 影响 |
|------|-----|-----|------|
| LED 矩阵 | 9个单色LED (行列扫描) | **3×3 RGB LED (WS2812)** | 不再需要 scanDisplay()，可用 delay() |
| 语音模块 | ❌ | ✅ UART2 (GPIO34/35) | 新功能 |
| 视觉/AI | ❌ | ✅ I2C2 (GPIO36/37) | 新功能 |
| 蜂鸣器 | GPIO26 板载 | ⚠️ 待确认 | GPIO26 变为 Grove 接口4 |

---

## 🌐 跨平台支持说明

本 SKILL 支持三种操作系统：

| 系统 | 串口命名 | 检测脚本 | 特殊说明 |
|------|----------|----------|----------|
| **Windows** | `COMx` | `detect_port_windows.py` | 使用 pnputil 检测 |
| **macOS** | `/dev/cu.usbmodem*` | `detect_port_macos.py` | 使用 system_profiler/ioreg |
| **Linux** | `/dev/ttyACM*` | `detect_port_linux.py` | 可能需要 dialout 用户组权限 |

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
6. Arduino 框架缓存状态

> 环境检测脚本可复用 2.0 版本的脚本，仅需修改项目模板部分。

---

### Phase 1: 需求理解与代码生成

**输入**：用户自然语言描述  
**处理**：
1. 解析用户意图，识别涉及的硬件模块
2. 读取 `references/pinout_mapping_v3.csv` 获取引脚映射
3. 读取 `references/future_tech_box_v3_hardware.md` 获取硬件约束
4. 生成符合 PlatformIO 结构的代码

---

### Phase 2: 编译

**执行命令**：`pio run -d <project_path>`

---

### Phase 3: 烧录

**主烧录命令**：`pio run -t upload --upload-port <PORT> -d <project_path>`

> 烧录策略与 2.0 版本相同：先烧录，失败后自动重试，仍失败再请用户操作。
> 详细的烧录流程、重试机制参考 2.0 版本 SKILL.md。

---

## 支持的硬件功能

| 功能 | 状态 | 示例指令 |
|------|------|----------|
| **RGB LED 矩阵** | ✅ | "LED 显示彩虹色"、"中间灯亮红色" |
| 按键 | ✅ | "按下按键 A 时..." |
| 电机控制 | ✅ | "让小车前进"、"原地左转" |
| 循迹传感器 | ✅ | "实现循迹小车" |
| 舵机/机械臂 | ✅ | "控制机械臂抓取物体" |
| PS2 手柄 | ✅ | "用手柄遥控小车" |
| I2C 加速度计 | ✅ | "检测倾斜方向" |
| I2C 颜色传感器 | ✅ | "识别颜色" |
| I2C 温湿度 | ✅ | "读取当前温度" |
| 超声波传感器 | ✅ | "测量前方距离" |
| **语音模块** | 🔧 待完善 | "语音播报距离"、"语音控制小车" |
| **视觉模块** | 🔧 待完善 | "识别前方物体"、"检测人脸" |
| 蜂鸣器 | ⚠️ 待确认 | "蜂鸣器响一声" |
| WiFi Web 遥控 | ✅ | "用网页控制小车" |

✅ = 已支持  🔧 = 开发中  ⚠️ = 待确认

---

## ⭐ RGB LED 矩阵控制要点（3.0 核心新特性）

### 控制方式
- **控制引脚**：GPIO33（单总线）
- **LED 数量**：9 个（3×3 矩阵）
- **类型**：WS2812 / NeoPixel（全彩 RGB）
- **推荐库**：`Adafruit NeoPixel`

### ⚠️ 重要：与 2.0 的编程范式差异

**2.0 单色LED（行列扫描）**：
- 需要持续调用 `scanDisplay()` 维持显示
- loop() 中禁止使用 `delay()`
- 蜂鸣器、按键等必须用非阻塞方式

**3.0 RGB LED（WS2812）**：
- 设置颜色后调用 `strip.show()` 即生效，**无需持续扫描**
- ✅ 可以在 loop() 中自由使用 `delay()`
- ✅ 编程大幅简化

### 代码模板

```cpp
#include <Adafruit_NeoPixel.h>

#define RGB_PIN     33
#define NUM_LEDS    9
#define BRIGHTNESS  50

Adafruit_NeoPixel strip(NUM_LEDS, RGB_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
}

// 按编号(1-9)设置LED颜色
void setLED(int num, uint8_t r, uint8_t g, uint8_t b) {
  if (num >= 1 && num <= 9) {
    strip.setPixelColor(num - 1, strip.Color(r, g, b));
    strip.show();
  }
}

// 设置所有LED
void setAllLEDs(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.show();
}

// 关闭所有LED
void clearLEDs() {
  strip.clear();
  strip.show();
}
```

### 常用效果代码

```cpp
// 彩虹效果
void rainbowEffect(int wait = 50) {
  for (long firstPixelHue = 0; firstPixelHue < 65536; firstPixelHue += 256) {
    for (int i = 0; i < NUM_LEDS; i++) {
      int pixelHue = firstPixelHue + (i * 65536L / NUM_LEDS);
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show();
    delay(wait);
  }
}

// 逐个点亮效果
void wipeEffect(uint32_t color, int wait = 100) {
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, color);
    strip.show();
    delay(wait);
  }
}

// 呼吸灯效果（全彩）
void breatheEffect(uint8_t r, uint8_t g, uint8_t b, int speed = 10) {
  for (int brightness = 0; brightness < 255; brightness++) {
    strip.setBrightness(brightness);
    setAllLEDs(r, g, b);
    delay(speed);
  }
  for (int brightness = 255; brightness >= 0; brightness--) {
    strip.setBrightness(brightness);
    setAllLEDs(r, g, b);
    delay(speed);
  }
  strip.setBrightness(BRIGHTNESS);  // 恢复默认亮度
}

// 闪烁效果
void blinkEffect(uint32_t color, int times = 3, int interval = 200) {
  for (int t = 0; t < times; t++) {
    for (int i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, color);
    }
    strip.show();
    delay(interval);
    strip.clear();
    strip.show();
    delay(interval);
  }
}
```

### PlatformIO 配置

```ini
[env:esp32s3]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200

lib_deps = 
    adafruit/Adafruit NeoPixel

; ⚠️ CH343 USB 转串口配置（3.0 必须）
; 3.0 使用外置 CH343 芯片做 USB 转串口，不是 ESP32-S3 内置 USB
; 必须关闭 USB CDC on Boot，否则串口输出会走内置 USB 而非 CH343
build_flags = 
    -DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=0
```

> 🚨 **关键配置说明（已验证）**：
> - 3.0 必须使用 `board = esp32-s3-devkitc-1`，**不能**使用 `seeed_xiao_esp32s3`（那是 2.0 的板型）
> - 必须设置 `ARDUINO_USB_CDC_ON_BOOT=0`，否则 `Serial.print` 输出会走 ESP32-S3 内置 USB 而非 CH343，串口监视器将看不到任何输出
> - CH343 的 VID:PID = `1A86:55D3`，在 `pio device list` 中显示为 `USB-Enhanced-SERIAL CH343`

---

## 📌 完整引脚映射表（3.0 版本）

### Grove 接口

| 接口 | GPIO_1 | GPIO_2 | 常用传感器 |
|------|--------|--------|-----------|
| **接口2** | GPIO7 | GPIO8 | 超声波传感器（SIG=GPIO7，GPIO8=NC） |
| **接口3** | GPIO3 | GPIO4 | 循迹传感器（左循迹=GPIO3, 右循迹=GPIO4） |
| **接口4** | GPIO26 | GPIO38 | 通用 Grove 接口 |
| **接口5** | GPIO5 | GPIO6 | 通用 Grove 接口 |
| **接口6** | GPIO1 | GPIO2 | 通用 Grove 接口 |

> ⚠️ **3.0 循迹传感器引脚与 2.0 不同！**
> - 2.0 循迹传感器：左=GPIO2，右=GPIO1（接口3）
> - 3.0 循迹传感器：左=GPIO3，右=GPIO4（接口3）
> - 生成循迹代码前必须确认目标版本！

### I²C 接口

| I2C 组 | 对应接口 | SDA | SCL | 说明 |
|--------|----------|-----|-----|------|
| I2C1（默认） | 接口1 / 接口8 | GPIO39 | GPIO40 | 大部分传感器默认使用 |
| I2C2 | 接口7 | GPIO37 | GPIO36 | 视觉/AI 摄像头模块 |

### 电机引脚

| 电机 | 正转 GPIO | 反转 GPIO | 位置 |
|------|----------|----------|------|
| M1 | GPIO11 | GPIO12 | 前左 |
| M2 | GPIO14 | GPIO13 | 前右 ⚠️ 正反转 GPIO 顺序相反 |
| M3 | GPIO15 | GPIO16 | 后左 |
| M4 | GPIO18 | GPIO17 | 后右 ⚠️ 正反转 GPIO 顺序相反 |

### 舵机引脚（云台结构）

| 舵机 | GPIO | 位置 | 居中角度 | 最小角度 | 最大角度 | 说明 |
|------|------|------|----------|----------|----------|------|
| S1 | GPIO47 | **云台底部（水平旋转）** | 104° | 0° | 220° | 底部无物理限位，行程较大 |
| S2 | GPIO48 | **云台顶部（俯仰）** | 180° | 90° | 270° | 受结构限制，只有一半行程 |

#### ⚠️ 舵机使用强制规范（写舵机程序前必须确认！）

1. **接线标准**：用户必须保证 **S1 = 云台底部舵机，S2 = 云台顶部舵机**。在编写任何舵机相关程序之前，必须向用户确认接线是否符合此标准。
2. **角度范围不同**：S1 和 S2 的居中位置、最小值、最大值完全不同，不能使用统一的 0~180 范围！
3. **空闲保护**：舵机无操作超过 2~3 秒必须自动 detach()，防止堵转烧毁。
4. **边界保护**：角度必须严格 constrain 在安全范围内，到达边界时停止响应。

**舵机控制代码模板**：
```cpp
#include <ESP32Servo.h>

#define SERVO1_PIN 47  // 云台底部（水平旋转）
#define SERVO2_PIN 48  // 云台顶部（俯仰）

// S1 云台底部参数
#define S1_CENTER 104
#define S1_MIN    0
#define S1_MAX    220

// S2 云台顶部参数
#define S2_CENTER 180
#define S2_MIN    90
#define S2_MAX    270

Servo servo1;
Servo servo2;

void setup() {
  // 注意：ESP32Servo 的 attach 可指定脉宽范围以支持 >180° 舵机
  // 标准舵机 500~2500μs 对应 0~270°
  servo1.attach(SERVO1_PIN, 500, 2500);
  servo2.attach(SERVO2_PIN, 500, 2500);
  servo1.write(S1_CENTER);  // 底部居中 104°
  servo2.write(S2_CENTER);  // 顶部居中 180°
}
```

> **重要提醒**：每次用户要求写舵机/云台相关程序时，必须先确认：
> "请确认您的接线：S1（GPIO47）接的是云台底部（水平旋转）舵机，S2（GPIO48）接的是云台顶部（俯仰）舵机，对吗？"

### 语音模块（UART2）

| 功能 | GPIO | 说明 |
|------|------|------|
| TX | GPIO34 | 语音模块通信 |
| RX | GPIO35 | 语音模块通信 |

**语音模块通信代码模板**：
```cpp
#define VOICE_TX 34
#define VOICE_RX 35

// 使用 Serial1 作为语音模块串口
void setup() {
  Serial.begin(115200);   // USB 调试串口
  delay(2000);
  Serial1.begin(9600, SERIAL_8N1, VOICE_RX, VOICE_TX);  // 语音模块串口
  Serial.println("语音模块串口初始化完成");
}

void loop() {
  // 接收语音模块发来的指令
  if (Serial1.available()) {
    String cmd = Serial1.readStringUntil('\n');
    Serial.print("收到语音指令: ");
    Serial.println(cmd);
    // 根据指令执行相应操作...
  }
}
```

### PS2 手柄引脚

| 功能 | GPIO |
|------|------|
| CLK | GPIO41 |
| CMD | GPIO9 |
| CS | GPIO42 |
| DAT | GPIO10 |

### 按键引脚

| 按键 | GPIO | 说明 |
|------|------|------|
| KEY_A | GPIO21 | 按下时 LOW |
| KEY_B | GPIO0 | 按下时 LOW（兼 BOOT） |

### 排针引出引脚

| GPIO | 功能 |
|------|------|
| GPIO19 | 串口TX |
| GPIO20 | 串口RX |
| GPIO43 | TX2 |
| GPIO44 | RX2 |
| GPIO45 | Flash电压选择IO |
| GPIO46 | 启动模式选择IO |

### RGB LED

| 功能 | GPIO | 说明 |
|------|------|------|
| RGB LED 数据总线 | GPIO33 | WS2812 × 9，单总线控制 |

---

## 按键控制要点

**与 2.0 相同**：按键使用内部上拉电阻，按下时为 LOW。

| 按键 | GPIO | 说明 |
|------|------|------|
| KEY_A | GPIO21 | 按下时 LOW |
| KEY_B | GPIO0 | 按下时 LOW（兼 BOOT） |

**🎉 3.0 简化点**：由于 RGB LED 不需要扫描，按键可以使用简单的 delay() 消抖方式：

```cpp
bool lastKeyA = HIGH;

void loop() {
  bool currentKeyA = digitalRead(KEY_A);
  
  if (lastKeyA == HIGH && currentKeyA == LOW) {
    delay(50);  // ✅ 3.0 可以用 delay 消抖
    onKeyAPressed();
  }
  
  lastKeyA = currentKeyA;
}
```

---

## 电机控制要点

与 2.0 版本引脚相同（但循迹传感器引脚不同，见引脚映射表）。

**⚠️ 重要**：右侧电机 (M2, M4) 的正反转 GPIO 顺序与左侧相反。

**默认速度建议**：180（70%功率），不超过 220。

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

void setMotor(int fwdPin, int revPin, int speed) {
  speed = constrain(speed, -255, 255);
  if (speed > 0) {
    analogWrite(fwdPin, speed);
    analogWrite(revPin, 0);
  } else if (speed < 0) {
    analogWrite(fwdPin, 0);
    analogWrite(revPin, -speed);
  } else {
    analogWrite(fwdPin, 0);
    analogWrite(revPin, 0);
  }
}

void setMotors(int leftSpeed, int rightSpeed) {
  setMotor(M1_FWD, M1_REV, leftSpeed);
  setMotor(M2_FWD, M2_REV, rightSpeed);
  setMotor(M3_FWD, M3_REV, leftSpeed);
  setMotor(M4_FWD, M4_REV, rightSpeed);
}

// 前进: setMotors(180, 180);
// 后退: setMotors(-180, -180);
// 左转: setMotors(-180, 180);
// 右转: setMotors(180, -180);
```

### 循迹传感器（3.0 引脚！）

```cpp
// ⚠️ 3.0 循迹引脚与 2.0 不同！
#define TRACK_LEFT  3   // 接口3-GPIO3（左循迹）
#define TRACK_RIGHT 4   // 接口3-GPIO4（右循迹）

void setup() {
  pinMode(TRACK_LEFT, INPUT);
  pinMode(TRACK_RIGHT, INPUT);
}

// 黑线上 = LOW, 白色地面 = HIGH（根据实际传感器可能需要反转）
```

---

## PS2 手柄控制要点

引脚与 2.0 相同：
| 功能 | GPIO |
|------|------|
| CLK | GPIO41 |
| CMD | GPIO9 |
| CS | GPIO42 |
| DAT | GPIO10 |

**依赖库**：`Arduino-PS2X-ESP32-master`（在 `references/libraries/` 中提供）

> PS2 手柄控制架构与 2.0 版本相同，需要硬件定时器中断读取。

---

## Grove 传感器

3.0 版本有 6 个 Grove 接口，可接入多种传感器。

### 超声波传感器（接口2，GPIO7）

**⚠️ 这是单线超声波模块（SIG 模式）**，只使用 GPIO7 一个引脚完成距离测量。GPIO8 连接的是超声波模块的 NC（空引脚），忽略即可。

**超声波传感器代码模板**：
```cpp
#define ULTRASONIC_PIN 7  // 接口2-GPIO7，单线 SIG 模式

float readUltrasonic() {
  // Step 1: 发送触发脉冲
  pinMode(ULTRASONIC_PIN, OUTPUT);
  digitalWrite(ULTRASONIC_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_PIN, LOW);
  
  // Step 2: 切换为输入，读取回波
  pinMode(ULTRASONIC_PIN, INPUT);
  long duration = pulseIn(ULTRASONIC_PIN, HIGH, 30000);
  
  // Step 3: 计算距离
  if (duration == 0) return -1;
  return duration / 58.0;  // 距离(cm) = 脉冲时间(μs) / 58
}
```

**⚠️ 超声波传感器注意事项**：
1. GPIO8 不需要初始化
2. 测量间隔至少 60ms，推荐 100ms
3. 有效量程约 2cm ~ 400cm
4. 建议使用中值滤波消除跳变（5次采样取中值）

### 温湿度传感器 (DHT20)
**依赖库**：`Grove_Temperature_And_Humidity_Sensor`
**I2C 地址**：0x38
```cpp
#include "DHT.h"
// DHT20 使用 I2C 接口
Wire.begin(39, 40);  // I2C1: SDA=39, SCL=40
```

### 三轴加速度计 (LIS3DHTR)
**依赖库**：`LIS3DHTR`
**I2C 地址**：0x18
```cpp
#include "LIS3DHTR.h"
LIS3DHTR<TwoWire> lis;
Wire.begin(39, 40);  // I2C1: SDA=39, SCL=40
lis.begin(Wire, 0x18);
```

### 颜色传感器 (VEML6040)
**依赖库**：`VEML6040`
**I2C 地址**：0x10

### LED Bar
**依赖库**：`Grove_LED_Bar`

### 🚨 I²C 传感器编码最佳实践

1. **I2C 初始化必须指定引脚**：
```cpp
Wire.begin(39, 40);   // I2C1: 接口1/接口8
// Wire.begin(37, 36); // I2C2: 接口7
```

2. **必须在初始化前进行 I2C 扫描**：
```cpp
void scanI2C() {
  Serial.println("正在扫描 I2C 设备...");
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  找到设备: 0x%02X\n", addr);
    }
  }
}
```

3. **传感器初始化失败必须提供降级模式**：
```cpp
// ❌ 错误：死循环
if (!sensor) { while (1) { 闪灯; } }

// ✅ 正确：降级到演示模式
bool sensorFound = false;
if (!sensor) {
  Serial.println("传感器未检测到，进入演示模式");
  sensorFound = false;
}
```

---

## 引用资源

- **引脚映射**：`references/pinout_mapping_v3.csv`
- **硬件规格**：`references/future_tech_box_v3_hardware.md`
- **主板图片**：`references/3.0主板正面.jpg`、`references/3.0主板背面.jpg`
- **库文件**：`references/libraries/`
  - Arduino-PS2X-ESP32-master.zip
  - Grove_LED_Bar.zip
  - Grove_Temperature_And_Humidity_Sensor.zip
  - Grove_Ultrasonic_Ranger.zip
  - Grove-3-Axis-Digital-Accelerometer-2g-to-16g-LIS3DHTR.zip
  - VEML6040.zip
- **示例程序**：`references/examples/`
  - followline.7z（循迹小车）
  - ps_mode.7z（PS2 手柄控制）

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
└── lib/                # 第三方库（按需复制）
    └── PS2X_lib/       # PS2 手柄库（如需要）
```

---

## 约束与限制

1. 仅支持未来科技盒 3.0（ESP32-S3 QFN56 + CH343）
2. 需要稳定的网络连接（首次下载依赖）
3. USB 必须是数据线（非充电线）
4. **烧录时必须显式指定 `--upload-port`**，禁止依赖 PlatformIO 自动检测
5. **`setup()` 中 `delay()` 必须 ≥ 2000ms**，确保 USB 串口稳定
6. **PlatformIO 命令优先使用 `python -m platformio`**，如果 `pio.exe` 被系统策略阻止则必须使用此方式
7. **3.0 必须使用 `board = esp32-s3-devkitc-1`**，禁止使用 `seeed_xiao_esp32s3`（那是 2.0 板型）
8. **必须设置 `ARDUINO_USB_CDC_ON_BOOT=0`**，否则串口输出走错通道
9. **RGB LED 亮度建议 20-50**，避免 LED 过亮刺眼和电流过大
10. **NeoPixel 亮度调节禁止闪白反馈**：`setBrightness()` 后必须直接用当前颜色 `show()`，不得插入闪白效果
11. **I2C 传感器必须提供降级模式**，初始化失败不能进入死循环
12. **3.0 循迹传感器引脚与 2.0 不同**：3.0 用 GPIO3/GPIO4，2.0 用 GPIO2/GPIO1
13. 语音模块和视觉模块功能待完善（需要更多硬件信息）

---

## 常见问题排查指南

### 问题 1：`pio.exe` 被 Windows 应用控制策略阻止

**症状**：
```
程序"pio.exe"无法运行: An Application Control policy has blocked this file
```

**解决方案（已验证有效）**：使用 `python -m platformio` 代替 `pio` 命令：

```bash
# ❌ 被阻止的写法
pio run

# ✅ 正确写法
python -m platformio run
python -m platformio run -t upload --upload-port COM10
python -m platformio device list
```

**⚠️ SKILL 执行时的强制规则**：所有 PlatformIO 命令一律使用 `python -m platformio` 前缀。

### 问题 2：board 配置错误导致编译失败或串口无输出

**症状**：编译失败，或烧录后串口无输出

**根因**：使用了 2.0 的 `seeed_xiao_esp32s3` 板型

```ini
; ❌ 错误
board = seeed_xiao_esp32s3

; ✅ 正确（3.0 版本）
board = esp32-s3-devkitc-1
```

### 问题 3：串口无输出（USB CDC 配置缺失）

**症状**：程序烧录成功，LED 正常工作，但串口监视器看不到任何输出

**根因**：3.0 使用 CH343 做 USB 转串口，但如果不显式关闭 USB CDC on Boot，串口输出走错通道

**解决方案**：确保 `platformio.ini` 中有：
```ini
build_flags = 
    -DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=0
```

### 问题 4：CH343 驱动未安装

**症状**：USB 连接后设备管理器显示未知设备或黄色感叹号

**解决方案**：
1. 下载 CH343 驱动：https://www.wch.cn/downloads/CH343SER_EXE.html
2. 安装后重新插入 USB
3. 设备管理器应显示 `USB-Enhanced-SERIAL CH343 (COMx)`

### 问题 5：NeoPixel RGB LED 亮度切换时闪烁

**症状**：按键调节亮度时，LED 先闪烁白光，然后才恢复正确亮度

**根因**：代码中在 `setBrightness()` 后插入了闪白反馈效果

**❌ 错误写法**（会闪烁）：
```cpp
void onKeyPressed() {
  strip.setBrightness(newBrightness);
  // ❌ 闪白反馈
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(255, 255, 255));
  }
  strip.show();
  delay(50);
}
```

**✅ 正确写法**（平滑过渡）：
```cpp
void onKeyPressed() {
  strip.setBrightness(newBrightness);
  // ✅ 直接用当前颜色刷新
  strip.show();  // 立即以新亮度显示当前颜色
}
```

### 问题 6：编译错误 - ledcAttach 未定义

**症状**：`error: 'ledcAttach' was not declared in this scope`

**解决方案**：使用 Arduino Core 2.x 兼容 API：
```cpp
ledcSetup(channel, PWM_FREQ, PWM_RESOLUTION);
ledcAttachPin(pin, channel);
ledcWrite(channel, brightness);  // 注意：使用通道号
```

### 问题 7：烧录失败 - 端口不可用

**解决方案**：
1. 先运行 `python -m platformio device list` 确认端口
2. 指定端口烧录：`python -m platformio run -t upload --upload-port COMx`
3. 如仍失败，按 RST 按钮后重试
4. 备用方案：使用 esptool 直接烧录

### 问题 8：烧录显示成功但程序未生效

**解决方案**：
1. 始终显式指定端口（最关键）
2. 代码中 `setup()` 使用 `delay(2000)`
3. 使用 I2C 扫描辅助调试
4. 传感器初始化失败时切换到降级/演示模式，不要死循环

---

## ⚠️ 开发中 / 待补充内容

### 语音模块
- [x] ✅ 通信引脚已确认：UART2 GPIO34(TX)/GPIO35(RX)
- [ ] 确认语音模块具体型号
- [ ] 确认通信协议和波特率（暂用 9600）
- [ ] 编写完整驱动代码模板
- [ ] 测试语音播报/识别功能

### 视觉模块 / AI 摄像头
- [x] ✅ 接口已确认：I2C2 GPIO37(SDA)/GPIO36(SCL)
- [ ] 确认 AI 板型号
- [ ] 确认 I2C 地址
- [ ] 编写通信代码模板
- [ ] 测试视觉识别功能

### 蜂鸣器
- [ ] 确认 3.0 是否有板载蜂鸣器（GPIO26 现为 Grove 接口4）
- [ ] 如有，确认蜂鸣器引脚
- [ ] 适配代码模板

### RGB LED 矩阵
- [x] ✅ 控制引脚已确认：GPIO33
- [x] ✅ 类型已确认：WS2812 / NeoPixel（NEO_GRB + NEO_KHZ800）
- [x] ✅ 亮度安全范围：建议 20-50
- [ ] 确认物理排列顺序（LED 索引对应主板位置）

### 电机方向
- [x] ✅ 引脚映射已确认（与 2.0 相同）
- [x] ✅ 右侧电机 M2/M4 的 GPIO 正反转顺序确认需要反转
- [ ] 实测确认各电机实际正反转方向

### 已确认的硬件信息（与 2.0 对比）
- [x] ✅ 主板型号：ESP32-S3 QFN56（非 XIAO ESP32S3）
- [x] ✅ USB 转串口：CH343（VID:PID = 1A86:55D3）
- [x] ✅ PlatformIO board：`esp32-s3-devkitc-1`
- [x] ✅ 必须设置 `ARDUINO_USB_CDC_ON_BOOT=0`
- [x] ✅ 舵机引脚：S1=GPIO47, S2=GPIO48
- [x] ✅ 循迹传感器引脚：左=GPIO3, 右=GPIO4（与 2.0 不同！）
- [x] ✅ Grove 接口引脚映射（接口2-6 全部确认）
