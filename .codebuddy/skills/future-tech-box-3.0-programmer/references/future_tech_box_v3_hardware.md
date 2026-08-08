# 未来科技盒 3.0 硬件规格文档

> **目标硬件**：未来科技盒 3.0（基于 ESP32-S3）
> **更新日期**：2026-04-23
> **文档版本**：v0.1.0（初始版本）

---

## 主板概述

未来科技盒 3.0 相比 2.0 版本的主要变化：

| 对比项 | 2.0 版本 (XIAO ESP32S3) | 3.0 版本 (ESP32-S3) |
|--------|------------------------|---------------------|
| **LED 矩阵** | 9个单色LED (3×3行列扫描) | **3×3 RGB LED 矩阵** (WS2812/NeoPixel) |
| **语音模块** | ❌ 无 | ✅ UART2 语音模块 |
| **视觉模块** | ❌ 无 | ✅ I2C2 摄像头AI板 |
| **Grove 接口** | 有限 | 6个 Grove 接口 |
| **Flash** | 板载 | 外挂 8M Flash |
| **排针引出** | 有限 | 多组排针引出 |
| **PS2 手柄** | ✅ | ✅ (引脚有变化) |
| **电机** | 4路 (8 GPIO) | 4路 (8 GPIO) |
| **舵机** | 2路 | 2路 |
| **按键** | 2个 (KEY_A + KEY_B/BOOT) | 2个 (KEY_A + KEY_B/BOOT) |

---

## 完整引脚映射表

### GPIO 总览

| GPIO | 功能模块 | 详细说明 | 备注 |
|------|----------|----------|------|
| GPIO0 | Key | 按键B (BOOT) | 按下LOW，兼启动模式选择 |
| GPIO1 | Grove 接口6 | 通用IO | |
| GPIO2 | Grove 接口6 | 通用IO | |
| GPIO3 | Grove 接口3 | 通用IO | |
| GPIO4 | Grove 接口3 | 通用IO | |
| GPIO5 | Grove 接口5 | 通用IO | |
| GPIO6 | Grove 接口5 | 通用IO | |
| GPIO7 | Grove 接口2 | 通用IO | |
| GPIO8 | Grove 接口2 | 通用IO | |
| GPIO9 | PS2 | 私有接口协议 (CMD) | |
| GPIO10 | PS2 | 私有接口协议 (DAT) | |
| GPIO11 | Motor | 前左电机 (M1-A) | |
| GPIO12 | Motor | 前左电机 (M1-B) | |
| GPIO13 | Motor | 前右电机 (M2-A) | |
| GPIO14 | Motor | 前右电机 (M2-B) | |
| GPIO15 | Motor | 后左电机 (M3-A) | |
| GPIO16 | Motor | 后左电机 (M3-B) | |
| GPIO17 | Motor | 后右电机 (M4-A) | |
| GPIO18 | Motor | 后右电机 (M4-B) | |
| GPIO19 | 排针引出 | 串口TX | |
| GPIO20 | 排针引出 | 串口RX | |
| GPIO21 | Key | 按键A | 按下LOW |
| GPIO26 | Grove 接口4 | 通用IO | ⚠️ 2.0中此引脚为蜂鸣器，3.0 无板载蜂鸣器 |
| GPIO27-32 | Flash | 8M外挂Flash | 不可用作通用IO |
| **GPIO33** | **RGB LED** | **LED控制总线** | ⭐ 3.0新特性：RGB灯控制 |
| **GPIO34** | **UART2** | **语音模块通信 (TX)** | ⭐ 3.0新增 |
| **GPIO35** | **UART2** | **语音模块通信 (RX)** | ⭐ 3.0新增 |
| **GPIO36** | **I2C2** | **摄像头AI板通信 (SDA)** | ⭐ 3.0新增 |
| **GPIO37** | **I2C2** | **摄像头AI板通信 (SCL)** | ⭐ 3.0新增 |
| GPIO38 | Grove 接口4 | 通用IO | |
| GPIO39 | UART0 | 程序下载/调试串口 TX | |
| GPIO40 | UART0 | 程序下载/调试串口 RX | |
| GPIO41 | PS2 | 私有接口协议 (CLK) | |
| GPIO42 | PS2 | 私有接口协议 (CS) | |
| GPIO43 | 排针引出 | TX2 | |
| GPIO44 | 排针引出 | RX2 | |
| GPIO45 | 排针引出 | Flash电压选择IO | 谨慎使用 |
| GPIO46 | 排针引出 | 启动模式选择IO | 谨慎使用 |
| GPIO47 | Servo | 舵机S1 | |
| GPIO48 | Servo | 舵机S2 | |

---

## 模块详细说明

### 1. ⭐ RGB LED 矩阵 (3×3 WS2812)（3.0 新特性）

**控制引脚**：GPIO33（单总线控制）

**与 2.0 版本的区别**：
- 2.0：9个单色LED，行列扫描方式，需要6个GPIO控制
- 3.0：9个RGB全彩LED（WS2812/NeoPixel），仅需**1个GPIO**控制总线

**LED 编号布局**（面对主板，待确认）：
```
     LED0    LED1    LED2     ← 第一行
     LED3    LED4    LED5     ← 第二行
     LED6    LED7    LED8     ← 第三行
```

**依赖库**：`Adafruit NeoPixel` 或 `FastLED`

**基础代码模板**：
```cpp
#include <Adafruit_NeoPixel.h>

#define RGB_PIN     33    // RGB LED 数据引脚
#define NUM_LEDS    9     // 3×3 = 9个LED
#define BRIGHTNESS  50    // 亮度 (0-255)，建议不超过100避免电流过大

Adafruit_NeoPixel strip(NUM_LEDS, RGB_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();  // 初始化全灭
}

// 设置单个LED颜色
void setLED(int index, uint8_t r, uint8_t g, uint8_t b) {
  if (index >= 0 && index < NUM_LEDS) {
    strip.setPixelColor(index, strip.Color(r, g, b));
  }
}

// 设置所有LED为同一颜色
void setAllLEDs(uint8_t r, uint8_t g, uint8_t b) {
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.show();
}

// LED编号(1-9) 转 数组索引(0-8)
int ledNumToIndex(int num) {
  return constrain(num - 1, 0, NUM_LEDS - 1);
}
```

**常用颜色定义**：
```cpp
#define COLOR_RED     strip.Color(255, 0, 0)
#define COLOR_GREEN   strip.Color(0, 255, 0)
#define COLOR_BLUE    strip.Color(0, 0, 255)
#define COLOR_YELLOW  strip.Color(255, 255, 0)
#define COLOR_CYAN    strip.Color(0, 255, 255)
#define COLOR_MAGENTA strip.Color(255, 0, 255)
#define COLOR_WHITE   strip.Color(255, 255, 255)
#define COLOR_ORANGE  strip.Color(255, 128, 0)
#define COLOR_OFF     strip.Color(0, 0, 0)
```

**⚠️ 关键优势**：
1. 不再需要行列扫描 → **不需要 `scanDisplay()` 函数**
2. 不需要非阻塞编程来维护LED显示 → **可以安全使用 `delay()`**
3. 支持全彩色显示，可以做出更丰富的视觉效果
4. 仅占用1个GPIO，释放了大量引脚

**PlatformIO 库依赖**：
```ini
lib_deps = 
    adafruit/Adafruit NeoPixel
```

---

### 2. ⭐ 语音模块 (3.0 新增)

**通信接口**：UART2
| 引脚 | GPIO | 说明 |
|------|------|------|
| TX | GPIO34 | 语音模块 UART TX |
| RX | GPIO35 | 语音模块 UART RX |

**初始化代码**：
```cpp
// 使用 Serial1 或 HardwareSerial
HardwareSerial VoiceSerial(1);  // UART1

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  // 初始化语音模块串口
  VoiceSerial.begin(9600, SERIAL_8N1, 35, 34);  // RX=GPIO35, TX=GPIO34
  // 注意：波特率需要根据具体语音模块确认
  
  Serial.println("语音模块初始化完成");
}
```

> ⚠️ **待确认事项**：
> - 语音模块具体型号和协议
> - 通信波特率
> - 支持的指令集
> - 是否支持语音识别/语音播放/TTS

---

### 3. ⭐ 视觉模块 / 摄像头AI板 (3.0 新增)

**通信接口**：I2C2
| 引脚 | GPIO | 说明 |
|------|------|------|
| SDA | GPIO36 | I2C2 数据线 |
| SCL | GPIO37 | I2C2 时钟线 |

**初始化代码**：
```cpp
#include <Wire.h>

// 使用第二个I2C总线
TwoWire Wire1 = TwoWire(1);

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  // 初始化视觉模块I2C
  Wire1.begin(36, 37);  // SDA=GPIO36, SCL=GPIO37
  
  Serial.println("视觉模块I2C初始化完成");
}
```

> ⚠️ **待确认事项**：
> - AI板具体型号
> - I2C 地址
> - 支持的视觉识别功能（颜色/人脸/物体/二维码等）
> - 通信协议和数据格式

---

### 4. 电机控制 (M1-M4)

**引脚分配**（与 2.0 相同）：

| 电机 | 位置 | GPIO-A | GPIO-B | 说明 |
|------|------|--------|--------|------|
| M1 | 前左 | GPIO11 | GPIO12 | |
| M2 | 前右 | GPIO13 | GPIO14 | |
| M3 | 后左 | GPIO15 | GPIO16 | |
| M4 | 后右 | GPIO17 | GPIO18 | |

**电机布局**（俯视图）：
```
        前方
    ┌─────────┐
    │  M1  M2 │   M1=前左(GPIO11/12)  M2=前右(GPIO13/14)
    │         │
    │  M3  M4 │   M3=后左(GPIO15/16)  M4=后右(GPIO17/18)
    └─────────┘
        后方
```

> ⚠️ **待确认**：3.0 版本中右侧电机 (M2/M4) 的正反转GPIO顺序是否与2.0一样相反。
> 2.0 中：M2_FWD=GPIO14, M2_REV=GPIO13（与M1的顺序相反）

**电机控制代码模板**：
```cpp
#define M1_A 11
#define M1_B 12
#define M2_A 13
#define M2_B 14
#define M3_A 15
#define M3_B 16
#define M4_A 17
#define M4_B 18

void setMotor(int pinA, int pinB, int speed) {
  speed = constrain(speed, -255, 255);
  if (speed > 0) {
    analogWrite(pinA, speed);
    analogWrite(pinB, 0);
  } else if (speed < 0) {
    analogWrite(pinA, 0);
    analogWrite(pinB, -speed);
  } else {
    analogWrite(pinA, 0);
    analogWrite(pinB, 0);
  }
}

// ⚠️ 注意：正反转方向待实测确认，可能需要调整
void setMotors(int leftSpeed, int rightSpeed) {
  setMotor(M1_A, M1_B, leftSpeed);
  setMotor(M2_A, M2_B, rightSpeed);   // 可能需要反向
  setMotor(M3_A, M3_B, leftSpeed);
  setMotor(M4_A, M4_B, rightSpeed);   // 可能需要反向
}

void carStop()                 { setMotors(0, 0); }
void carForward(int s = 180)  { setMotors(s, s); }
void carBackward(int s = 180) { setMotors(-s, -s); }
void carTurnLeft(int s = 180) { setMotors(-s, s); }
void carTurnRight(int s = 180){ setMotors(s, -s); }
```

---

### 5. PS2 手柄

**引脚分配**（引脚有变化）：

| 功能 | GPIO | 说明 |
|------|------|------|
| CLK | GPIO41 | 时钟 |
| CMD | GPIO9 | 命令 |
| CS | GPIO42 | 片选 |
| DAT | GPIO10 | 数据 |

**依赖库**：`Arduino-PS2X-ESP32-master`（已提供在 libraries 目录）

---

### 6. 按键

| 按键 | GPIO | 说明 |
|------|------|------|
| KEY_A | GPIO21 | 按下时 LOW（内部上拉） |
| KEY_B | GPIO0 | 按下时 LOW（兼 BOOT 模式） |

**代码模板**：
```cpp
#define KEY_A 21
#define KEY_B 0

void setup() {
  pinMode(KEY_A, INPUT_PULLUP);
  pinMode(KEY_B, INPUT_PULLUP);
}

// 边沿检测
bool lastKeyA = HIGH;
void checkKeyA() {
  bool current = digitalRead(KEY_A);
  if (lastKeyA == HIGH && current == LOW) {
    // 按键A按下事件
    onKeyAPressed();
  }
  lastKeyA = current;
}
```

---

### 7. 舵机

| 舵机 | GPIO | 说明 |
|------|------|------|
| S1 | GPIO47 | 舵机通道1 |
| S2 | GPIO48 | 舵机通道2 |

**代码模板**：
```cpp
#include <ESP32Servo.h>

Servo servo1;
Servo servo2;

void setup() {
  servo1.attach(47);
  servo2.attach(48);
}

void setServo(Servo &servo, int angle) {
  angle = constrain(angle, 0, 180);
  servo.write(angle);
}
```

---

### 8. Grove 接口

3.0 版本提供 **6个 Grove 接口**，可连接各类 Grove 传感器/执行器：

| 接口编号 | GPIO-1 | GPIO-2 | 可用功能 |
|---------|--------|--------|----------|
| 接口2 | GPIO7 | GPIO8 | 数字IO / I2C / 模拟 |
| 接口3 | GPIO3 | GPIO4 | 数字IO / 循迹传感器 |
| 接口4 | GPIO26 | GPIO38 | 数字IO |
| 接口5 | GPIO5 | GPIO6 | 数字IO |
| 接口6 | GPIO1 | GPIO2 | 数字IO |

> ⚠️ **注意**：GPIO26 在 2.0 版本中用于蜂鸣器，**3.0 无板载蜂鸣器模块**（已确认），GPIO26 变为 Grove 接口4 的一部分。如需声音反馈，3.0 可改用语音模块（GPIO34/35）或外接喇叭。

**可连接的 Grove 传感器（已提供库）**：
- 超声波测距传感器 (Grove_Ultrasonic_Ranger)
- 温湿度传感器 DHT20 (Grove_Temperature_And_Humidity_Sensor)
- 三轴加速度计 LIS3DHTR (Grove-3-Axis-Digital-Accelerometer)
- 颜色传感器 VEML6040 (VEML6040)
- LED Bar (Grove_LED_Bar)

---

### 9. 排针引出引脚

| GPIO | 功能 | 说明 |
|------|------|------|
| GPIO19 | 串口TX | 可用于外接串口设备 |
| GPIO20 | 串口RX | 可用于外接串口设备 |
| GPIO43 | TX2 | 备用串口 |
| GPIO44 | RX2 | 备用串口 |
| GPIO45 | Flash电压选择IO | ⚠️ 谨慎使用 |
| GPIO46 | 启动模式选择IO | ⚠️ 谨慎使用 |

---

### 10. UART0 下载/调试串口

| 引脚 | GPIO | 说明 |
|------|------|------|
| TX | GPIO39 | 程序下载 & 调试串口 |
| RX | GPIO40 | 程序下载 & 调试串口 |

---

## 2.0 → 3.0 引脚变化对比

| 功能 | 2.0 引脚 | 3.0 引脚 | 变化说明 |
|------|---------|---------|---------|
| LED 矩阵 | GPIO33-38 (6个GPIO行列扫描) | **GPIO33** (单总线RGB) | ⭐ 大幅简化 |
| 蜂鸣器 | GPIO26 | **❌ 无** | 3.0 无板载蜂鸣器；GPIO26 变为 Grove 接口4 |
| I2C 传感器 | SDA=GPIO39, SCL=GPIO40 | **待确认** | 可能改用 Grove 接口 |
| 语音模块 | ❌ 无 | GPIO34(TX), GPIO35(RX) | ⭐ 新增 |
| 视觉模块 | ❌ 无 | GPIO36(SDA), GPIO37(SCL) | ⭐ 新增 |
| 电机 | GPIO11-18 | GPIO11-18 | ✅ 相同 |
| PS2 手柄 | CLK=41,CMD=9,CS=42,DAT=10 | CLK=41,CMD=9,CS=42,DAT=10 | ✅ 相同 |
| 按键 | A=21, B=0 | A=21, B=0 | ✅ 相同 |
| 舵机 | S1=47, S2=48 | S1=47, S2=48 | ✅ 相同 |

---

## 已提供的库文件清单

| 库名称 | 文件 | 用途 |
|--------|------|------|
| Arduino-PS2X-ESP32 | Arduino-PS2X-ESP32-master.zip | PS2 手柄控制 |
| Grove LED Bar | Grove_LED_Bar.zip | LED Bar 灯条 |
| Grove Temperature And Humidity Sensor | Grove_Temperature_And_Humidity_Sensor.zip | DHT20 温湿度 |
| Grove Ultrasonic Ranger | Grove_Ultrasonic_Ranger.zip | 超声波测距 |
| LIS3DHTR | Grove-3-Axis-Digital-Accelerometer-2g-to-16g-LIS3DHTR.zip | 三轴加速度计 |
| VEML6040 | VEML6040.zip | RGBW 颜色传感器 |

## 已提供的示例程序

| 示例 | 文件 | 说明 |
|------|------|------|
| followline | followline.7z | 循迹小车示例 |
| ps_mode | ps_mode.7z | PS2 手柄控制示例 |

---

## ⚠️ 待确认事项清单

以下内容需要在实际测试后补充完善：

1. **RGB LED 具体型号**：WS2812B / SK6812 / 其他？颜色顺序 GRB/RGB？
2. ~~**蜂鸣器**：3.0 是否还有板载蜂鸣器？~~ ✅ 已确认：3.0 无板载蜂鸣器
3. **语音模块**：具体型号、通信协议、波特率、支持的功能
4. **视觉模块/AI板**：具体型号、I2C地址、支持的识别功能
5. **电机正反转方向**：GPIO-A/B 哪个是正转？右侧电机是否需要反转？
6. **I2C 传感器总线**：Grove 接口是否支持 I2C？使用哪组引脚？
7. **Grove 接口类型**：各接口支持的通信协议（数字/模拟/I2C/UART）
8. **LED 物理排列顺序**：LED索引0-8 对应主板上的物理位置
9. **Flash 芯片**：GPIO27-32 外挂 Flash 的具体用途
10. **USB 接口类型**：USB-C？与2.0的下载方式是否相同？
