# 未来科技盒 2.0 主板硬件规格

## 概述
- **主控芯片**：ESP32-S3（XIAO ESP32S3）
- **电源电压**：5V
- **USB接口**：Type-C（用于烧录和供电）
- **板载开关**：有
- **Flash**：8MB（GPIO27-32）

---

## 按键

| 按键名称 | GPIO | 说明 |
|---------|------|------|
| 按键 A | GPIO21 | 用户按键 |
| 按键 B | GPIO0 | BOOT 模式按键（长按进入烧录模式） |

---

## 电机（Motor）- 小车形态

### 电机布局图

```
        前方
    ┌─────────┐
    │  M1  M2 │   M1=左上  M2=右上
    │         │
    │  M3  M4 │   M3=左下  M4=右下
    └─────────┘
        后方
```

### 电机 GPIO 对应关系

| 电机编号 | 位置 | 正转 GPIO | 反转 GPIO | 说明 |
|---------|------|-----------|-----------|------|
| M1 | 左上 | GPIO11 | GPIO12 | 左侧电机 |
| M2 | 右上 | GPIO14 | GPIO13 | 右侧电机（方向相反） |
| M3 | 左下 | GPIO15 | GPIO16 | 左侧电机 |
| M4 | 右下 | GPIO18 | GPIO17 | 右侧电机（方向相反） |

**⚠️ 重要**：右侧电机 (M2, M4) 的正反转 GPIO 顺序与左侧相反，因为物理安装方向相反。

**控制方式**：每个电机使用两个 GPIO 进行 H 桥控制（正反转 + PWM 调速）

### 📌 电机控制代码示例

```cpp
// 电机引脚定义
#define M1_FWD 11  // 左上正转
#define M1_REV 12  // 左上反转
#define M2_FWD 14  // 右上正转（注意：GPIO14是正转，GPIO13是反转）
#define M2_REV 13  // 右上反转
#define M3_FWD 15  // 左下正转
#define M3_REV 16  // 左下反转
#define M4_FWD 18  // 右下正转（注意：GPIO18是正转，GPIO17是反转）
#define M4_REV 17  // 右下反转

void setup() {
  // 初始化所有电机引脚
  pinMode(M1_FWD, OUTPUT); pinMode(M1_REV, OUTPUT);
  pinMode(M2_FWD, OUTPUT); pinMode(M2_REV, OUTPUT);
  pinMode(M3_FWD, OUTPUT); pinMode(M3_REV, OUTPUT);
  pinMode(M4_FWD, OUTPUT); pinMode(M4_REV, OUTPUT);
  
  // 设置PWM频率（推荐10kHz）
  analogWriteFrequency(10000);
}

// 设置单个电机速度：speed > 0 正转，speed < 0 反转，speed = 0 停止
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

// 设置所有电机（左侧速度，右侧速度）
void setMotors(int leftSpeed, int rightSpeed) {
  setMotor(M1_FWD, M1_REV, leftSpeed);   // 左上
  setMotor(M2_FWD, M2_REV, rightSpeed);  // 右上
  setMotor(M3_FWD, M3_REV, leftSpeed);   // 左下
  setMotor(M4_FWD, M4_REV, rightSpeed);  // 右下
}

// 小车运动函数
void carStop() {
  setMotors(0, 0);
}

void carForward(int speed = 180) {
  setMotors(speed, speed);
}

void carBackward(int speed = 180) {
  setMotors(-speed, -speed);
}

void carTurnLeft(int speed = 180) {
  setMotors(-speed, speed);  // 原地左转
}

void carTurnRight(int speed = 180) {
  setMotors(speed, -speed);  // 原地右转
}

void carArcLeft(int speed = 180) {
  setMotors(speed / 2, speed);  // 弧线左转
}

void carArcRight(int speed = 180) {
  setMotors(speed, speed / 2);  // 弧线右转
}

void loop() {
  carForward(180);
  delay(1000);
  carStop();
  delay(500);
  carTurnLeft(150);
  delay(500);
  carStop();
  delay(500);
}
```

### 📌 麦克纳姆轮小车全向移动（如适用）

如果使用麦克纳姆轮，可以实现横移：

```cpp
// 左平移
void carMoveLeft(int speed = 180) {
  setMotor(M1_FWD, M1_REV, -speed);  // 左上反转
  setMotor(M2_FWD, M2_REV, speed);   // 右上正转
  setMotor(M3_FWD, M3_REV, speed);   // 左下正转
  setMotor(M4_FWD, M4_REV, -speed);  // 右下反转
}

// 右平移
void carMoveRight(int speed = 180) {
  setMotor(M1_FWD, M1_REV, speed);   // 左上正转
  setMotor(M2_FWD, M2_REV, -speed);  // 右上反转
  setMotor(M3_FWD, M3_REV, -speed);  // 左下反转
  setMotor(M4_FWD, M4_REV, speed);   // 右下正转
}
```

---

## Grove 接口

| 接口编号 | 类型 | GPIO | 说明 |
|---------|------|------|------|
| 接口 1 | 数字/模拟 | GPIO5, GPIO6 | 通用 IO |
| 接口 2 | I2C | GPIO39(SDA), GPIO40(SCL) | I2C 总线 |
| 接口 3 | 数字/模拟 | GPIO2, GPIO1 | 通用 IO |
| 接口 4 | 数字/模拟 | GPIO7, GPIO8 | 通用 IO |
| 接口 5 | I2C | GPIO39(SDA), GPIO40(SCL) | I2C 总线（与接口2共用） |
| 接口 6 | 数字/模拟 | GPIO3, GPIO4 | 通用 IO |

**注意**：接口 2 和接口 5 共用 I2C 总线

---

## LED 矩阵（3×3）

| 功能 | GPIO | 物理位置 | 说明 |
|-----|------|---------|------|
| 第 1 行 | GPIO33 | **物理下方** | 行控制（阳极/高电平有效）|
| 第 2 行 | GPIO34 | **物理中间** | 行控制（阳极/高电平有效）|
| 第 3 行 | GPIO35 | **物理上方** | 行控制（阳极/高电平有效）|
| 第 1 列 | GPIO36 | 左列 | 列控制（阴极/低电平有效） |
| 第 2 列 | GPIO37 | 中列 | 列控制（阴极/低电平有效） |
| 第 3 列 | GPIO38 | 右列 | 列控制（阴极/低电平有效） |

**⚠️ 重要：LED 点亮/熄灭控制原理**

这是一个**共阴极行列扫描矩阵**，LED 的点亮条件是：
- **行引脚 = HIGH（高电平）**：行作为阳极（正极）
- **列引脚 = LOW（低电平）**：列作为阴极（负极）
- 电流从行流向列，LED 导通点亮

**控制逻辑表**：
| 行状态 | 列状态 | LED 状态 |
|--------|--------|----------|
| HIGH   | LOW    | ✅ 点亮  |
| HIGH   | HIGH   | ❌ 熄灭  |
| LOW    | LOW    | ❌ 熄灭  |
| LOW    | HIGH   | ❌ 熄灭  |

**⚠️ 常见错误**：
- ❌ 错误：把列设为 HIGH 来"点亮"（实际会熄灭）
- ❌ 错误：把行设为 LOW 来"熄灭"（会导致整行熄灭）
- ✅ 正确：点亮 = 行 HIGH + 列 LOW
- ✅ 正确：熄灭 = 列 HIGH（或行 LOW）

**特性**：
- 灯光颜色：红色（固定，不可变）
- 亮度控制：通过 PWM 调节行引脚的占空比（0-255）
- 控制方式：行列扫描矩阵（**必须使用扫描方式，不能同时点亮所有 LED**）

**LED 矩阵位置映射（物理位置 → 数组索引）**：

⚠️ **注意：物理上方是 GPIO35（行3），物理下方是 GPIO33（行1）**

```
物理视角（面对主板看到的 LED 位置）：
     列1(36)  列2(37)  列3(38)
行3(35)  1        2        3      ← 物理上方
行2(34)  4        5        6      ← 物理中间
行1(33)  7        8        9      ← 物理下方

对应数组索引 ledState[row][col]：
     列1(36)  列2(37)  列3(38)
行3(35)  [2,0]    [2,1]    [2,2]  ← LED 1-3
行2(34)  [1,0]    [1,1]    [1,2]  ← LED 4-6
行1(33)  [0,0]    [0,1]    [0,2]  ← LED 7-9
```

**LED 编号 ↔ 数组索引转换：**
| LED 编号 | 物理位置 | 数组索引 | GPIO 行/列 |
|---------|---------|----------|-----------|
| 1 | 左上 | [2,0] | 行35/列36 |
| 2 | 中上 | [2,1] | 行35/列37 |
| 3 | 右上 | [2,2] | 行35/列38 |
| 4 | 左中 | [1,0] | 行34/列36 |
| 5 | 正中 | [1,1] | 行34/列37 |
| 6 | 右中 | [1,2] | 行34/列38 |
| 7 | 左下 | [0,0] | 行33/列36 |
| 8 | 中下 | [0,1] | 行33/列37 |
| 9 | 右下 | [0,2] | 行33/列38 |

**📌 代码示例：点亮/熄灭所有 LED**

```cpp
// 引脚定义
const int ROW_PINS[3] = {33, 34, 35};  // 行（阳极）- GPIO33在物理下方，GPIO35在物理上方
const int COL_PINS[3] = {36, 37, 38};  // 列（阴极）- 从左到右

// LED 状态数组：1=点亮, 0=熄灭
// ledState[0] = GPIO33 行（物理下方）
// ledState[1] = GPIO34 行（物理中间）
// ledState[2] = GPIO35 行（物理上方）
int ledState[3][3] = {
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0}
};

// ⚠️ LED 编号(1-9) 到数组索引的转换
// LED 1-3 在物理上方（GPIO35 行）→ ledState[2][]
// LED 4-6 在物理中间（GPIO34 行）→ ledState[1][]
// LED 7-9 在物理下方（GPIO33 行）→ ledState[0][]
void setLED(int num, int state) {
  if (num < 1 || num > 9) return;
  int idx = num - 1;
  int row = 2 - (idx / 3);  // LED 1-3→row 2, LED 4-6→row 1, LED 7-9→row 0
  int col = idx % 3;
  ledState[row][col] = state;
}

void setup() {
  // 初始化引脚
  for (int i = 0; i < 3; i++) {
    pinMode(ROW_PINS[i], OUTPUT);
    pinMode(COL_PINS[i], OUTPUT);
    digitalWrite(ROW_PINS[i], LOW);   // 行默认低电平（不选中）
    digitalWrite(COL_PINS[i], HIGH);  // 列默认高电平（熄灭）
  }
}

// 扫描显示函数 - 必须在 loop() 中持续调用
void scanDisplay() {
  for (int row = 0; row < 3; row++) {
    // 先关闭所有列（设为 HIGH）
    for (int col = 0; col < 3; col++) {
      digitalWrite(COL_PINS[col], HIGH);
    }
    
    // 激活当前行（设为 HIGH）
    digitalWrite(ROW_PINS[row], HIGH);
    
    // 根据状态设置列
    for (int col = 0; col < 3; col++) {
      if (ledState[row][col]) {
        digitalWrite(COL_PINS[col], LOW);   // 点亮：列设为 LOW
      } else {
        digitalWrite(COL_PINS[col], HIGH);  // 熄灭：列设为 HIGH
      }
    }
    
    delayMicroseconds(1000);  // 显示 1ms
    
    // 关闭当前行
    digitalWrite(ROW_PINS[row], LOW);
  }
}

// 点亮所有 LED
void turnOnAllLEDs() {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      ledState[i][j] = 1;
    }
  }
}

// 熄灭所有 LED
void turnOffAllLEDs() {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      ledState[i][j] = 0;
    }
  }
}

// 点亮指定编号的 LED（1-9）
void turnOnLED(int num) {
  setLED(num, 1);
}

// 熄灭指定编号的 LED（1-9）
void turnOffLED(int num) {
  setLED(num, 0);
}

void loop() {
  scanDisplay();  // 必须持续调用！
}
```

**📌 按键控制点亮/熄灭示例（边沿检测 + 非阻塞消抖）**

⚠️ **重要**：
1. 必须使用**边沿检测**方式检测按键
2. **有 LED 扫描时禁止使用 `delay()`**，必须用 `millis()` 非阻塞消抖

```cpp
const int KEY_A = 21;
const int KEY_B = 0;

// 按键状态记录（用于边沿检测）
bool lastKeyA = HIGH;
bool lastKeyB = HIGH;

// 非阻塞消抖计时
unsigned long lastKeyATime = 0;
unsigned long lastKeyBTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;  // 50ms 消抖

// LED 开关状态
bool ledOn = true;

void setup() {
  // ... LED 初始化 ...
  pinMode(KEY_A, INPUT_PULLUP);
  pinMode(KEY_B, INPUT_PULLUP);
  
  // 开机点亮所有 LED
  turnOnAllLEDs();
}

void loop() {
  // 读取当前按键状态
  bool currentKeyA = digitalRead(KEY_A);
  bool currentKeyB = digitalRead(KEY_B);
  unsigned long now = millis();
  
  // ✅ 正确：检测按键 A 的下降沿 + 非阻塞消抖
  if (lastKeyA == HIGH && currentKeyA == LOW) {
    if (now - lastKeyATime > DEBOUNCE_DELAY) {  // 非阻塞消抖
      // 切换 LED 状态
      if (ledOn) {
        turnOffAllLEDs();
        ledOn = false;
      } else {
        turnOnAllLEDs();
        ledOn = true;
      }
      lastKeyATime = now;
    }
  }
  
  // 检测按键 B 的下降沿 + 非阻塞消抖
  if (lastKeyB == HIGH && currentKeyB == LOW) {
    if (now - lastKeyBTime > DEBOUNCE_DELAY) {  // 非阻塞消抖
      // 切换 LED 状态
      if (ledOn) {
        turnOffAllLEDs();
        ledOn = false;
      } else {
        turnOnAllLEDs();
        ledOn = true;
      }
      lastKeyBTime = now;
    }
  }
  
  // ⚠️ 必须更新状态记录
  lastKeyA = currentKeyA;
  lastKeyB = currentKeyB;
  
  scanDisplay();  // 持续扫描显示，不会被阻塞
}
```

**❌ 错误示例（禁止这样写）**：

```cpp
// 错误1：直接检测电平状态，按住会连续触发
if (digitalRead(KEY_A) == LOW) {
  turnOnAllLEDs();   // 按住时会一直执行！
}

// 错误2：使用 delay() 消抖（会阻塞 LED 扫描）
if (lastKeyA == HIGH && currentKeyA == LOW) {
  turnOnAllLEDs();
  delay(50);  // ⚠️ 阻塞 50ms，LED 会闪烁/卡顿！
}

// 错误3：共享 lastDebounceTime 的消抖逻辑有 BUG
bool readKey(int pin, bool &lastState) {
  if (currentState != lastState) {
    lastDebounceTime = millis();  // 重置时间
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (currentState != lastState) {  // ⚠️ 刚重置完，这里永远为 false！
      // 永远不会执行到这里
    }
  }
}
```

---

## 蜂鸣器（Buzzer）

| 功能 | GPIO | 说明 |
|-----|------|------|
| 蜂鸣器 | GPIO26 | 有源/无源蜂鸣器（BUZ1） |

**控制方式**：
- **简单响声**：使用 `tone()` 和 `noTone()` 函数（推荐）
- **高级控制**：使用 `ledcWriteTone()` 函数

### ⚠️ 蜂鸣器代码要点

**1. 使用 `tone()` 函数（最简单，推荐）**：

```cpp
#define BUZZER_PIN 26

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
}

// 播放蜂鸣器（指定频率和持续时间）
void beep(int freq = 1000, int duration = 100) {
  tone(BUZZER_PIN, freq, duration);  // freq=频率Hz, duration=持续时间ms
}

void loop() {
  beep(1000, 100);  // 1000Hz 响 100ms
  delay(500);
}
```

**2. 使用 `ledcWriteTone()`（ESP32 原生 API）**：

```cpp
#define BUZZER_PIN 26

void setup() {
  // ESP32 Arduino 3.x 新 API：直接设置引脚
  ledcAttach(BUZZER_PIN, 1000, 8);  // 引脚, 初始频率, 分辨率
}

// 播放指定频率
void playTone(int freq) {
  ledcWriteTone(BUZZER_PIN, freq);  // 直接用引脚号，不是通道号
}

// 停止播放
void stopTone() {
  ledcWriteTone(BUZZER_PIN, 0);
}

void loop() {
  playTone(1000);   // 播放 1000Hz
  delay(100);
  stopTone();
  delay(500);
}
```

### ❌ 常见错误（禁止这样写）

```cpp
// 错误1：每次播放都调用 ledcSetup/ledcAttachPin（旧 API 且重复调用）
void playBuzzer() {
  ledcSetup(0, 1000, 8);      // ⚠️ 这是旧 API！
  ledcAttachPin(BUZZER_PIN, 0);  // ⚠️ 不要每次都调用
  ledcWrite(0, 128);
}

// 错误2：在有 LED 扫描的程序中使用 delay 阻塞
if (keyPressed) {
  tone(BUZZER_PIN, 1000);
  delay(100);  // ⚠️ 会阻塞 LED 扫描，导致闪烁！
  noTone(BUZZER_PIN);
}
```

### ✅ 与 LED 扫描配合使用（非阻塞方式）

当程序中有 LED 矩阵扫描时，**必须使用非阻塞方式控制蜂鸣器**：

```cpp
#define BUZZER_PIN 26

// 蜂鸣器状态
unsigned long buzzerEndTime = 0;
bool buzzerActive = false;

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  // ...
}

// 非阻塞播放蜂鸣器
void beepNonBlocking(int duration = 100) {
  tone(BUZZER_PIN, 1000);  // 开始播放
  buzzerEndTime = millis() + duration;
  buzzerActive = true;
}

// 在 loop 中检查是否需要停止（非阻塞）
void updateBuzzer() {
  if (buzzerActive && millis() >= buzzerEndTime) {
    noTone(BUZZER_PIN);
    buzzerActive = false;
  }
}

void loop() {
  // 检测按键
  if (lastKeyA == HIGH && currentKeyA == LOW) {
    beepNonBlocking(100);  // 非阻塞响 100ms
    turnOnAllLEDs();
    // ⚠️ 不要用 delay(50) 消抖！用下面的方式
  }
  lastKeyA = currentKeyA;
  
  updateBuzzer();   // 检查蜂鸣器状态
  scanDisplay();    // LED 扫描不会被阻塞
}
```

---

## 舵机（Servo）- 机械臂控制

| 舵机编号 | GPIO | 用途 |
|---------|------|------|
| S1 | GPIO47 | 机械臂舵机（上下运动） |
| S2 | GPIO48 | 夹子舵机（开合抓取） |

**控制方式**：软件 PWM 定时器中断（50Hz，脉宽 500-2500μs 对应 0-180°）

### 📌 舵机控制代码示例

```cpp
#define SERVO1_PIN 47  // 机械臂（上下）
#define SERVO2_PIN 48  // 夹子（开合）

// 使用软件PWM控制舵机
hw_timer_t* servoTimer = NULL;
volatile uint16_t servo1Value = 128;  // PWM占空比 (约90度)
volatile uint16_t servo2Value = 128;
const uint16_t PWM_PERIOD = 255;

// 定时器中断函数
void IRAM_ATTR servoTimerISR() {
  static uint8_t pwmCount1 = 0;
  static uint8_t pwmCount2 = 0;
  
  // 舵机1 PWM
  if (++pwmCount1 >= servo1Value) {
    digitalWrite(SERVO1_PIN, LOW);
  }
  if (pwmCount1 >= PWM_PERIOD) {
    pwmCount1 = 0;
    digitalWrite(SERVO1_PIN, HIGH);
  }
  
  // 舵机2 PWM
  if (++pwmCount2 >= servo2Value) {
    digitalWrite(SERVO2_PIN, LOW);
  }
  if (pwmCount2 >= PWM_PERIOD) {
    pwmCount2 = 0;
    digitalWrite(SERVO2_PIN, HIGH);
  }
}

void setup() {
  pinMode(SERVO1_PIN, OUTPUT);
  pinMode(SERVO2_PIN, OUTPUT);
  
  // 初始化定时器（周期100us，实现约50Hz舵机控制）
  servoTimer = timerBegin(1, 80, true);  // 定时器1, 分频80, 向上计数
  timerAttachInterrupt(servoTimer, &servoTimerISR, true);
  timerAlarmWrite(servoTimer, 100, true);  // 100us周期
  timerAlarmEnable(servoTimer);
}

// 设置舵机角度（0-180度）
void setServo1Angle(int angle) {
  angle = constrain(angle, 0, 180);
  // 角度映射到PWM值（4-13对应0-180度，根据实际调整）
  servo1Value = map(angle, 0, 180, 4, 13);
}

void setServo2Angle(int angle) {
  angle = constrain(angle, 0, 180);
  servo2Value = map(angle, 0, 180, 4, 13);
}

// 机械臂抓取动作示例
void grabObject() {
  // 1. 机械臂下降
  for (int angle = 40; angle <= 100; angle += 2) {
    setServo1Angle(angle);
    delay(50);
  }
  delay(500);
  
  // 2. 夹子合拢
  for (int angle = 130; angle >= 60; angle -= 2) {
    setServo2Angle(angle);
    delay(50);
  }
  delay(500);
  
  // 3. 机械臂抬起
  for (int angle = 100; angle >= 40; angle -= 2) {
    setServo1Angle(angle);
    delay(50);
  }
}

// 机械臂释放动作
void releaseObject() {
  // 夹子张开
  for (int angle = 60; angle <= 130; angle += 2) {
    setServo2Angle(angle);
    delay(50);
  }
}

void loop() {
  grabObject();
  delay(2000);
  releaseObject();
  delay(2000);
}
```

---

## 循迹传感器（Line Follower）

循迹传感器用于检测黑线，实现循迹小车功能。

| 传感器 | GPIO | 接口 | 说明 |
|--------|------|------|------|
| 左循迹 | GPIO2 | 接口3-PIN1 | 数字输入，检测到黑线=LOW |
| 右循迹 | GPIO1 | 接口3-PIN2 | 数字输入，检测到黑线=LOW |

### 📌 循迹传感器代码示例

```cpp
#define LINE_LEFT  2   // 左循迹传感器
#define LINE_RIGHT 1   // 右循迹传感器

void setup() {
  Serial.begin(115200);
  pinMode(LINE_LEFT, INPUT);
  pinMode(LINE_RIGHT, INPUT);
}

void loop() {
  int left = digitalRead(LINE_LEFT);   // 0=检测到黑线, 1=白色
  int right = digitalRead(LINE_RIGHT);
  
  Serial.print("左: ");
  Serial.print(left ? "白" : "黑");
  Serial.print("  右: ");
  Serial.println(right ? "白" : "黑");
  
  delay(100);
}
```

### 📌 循迹小车完整示例

```cpp
#define LINE_LEFT  2
#define LINE_RIGHT 1

// 电机引脚（参见电机章节）
#define M1_FWD 11
#define M1_REV 12
#define M2_FWD 14
#define M2_REV 13
#define M3_FWD 15
#define M3_REV 16
#define M4_FWD 18
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

void setup() {
  Serial.begin(115200);
  pinMode(LINE_LEFT, INPUT);
  pinMode(LINE_RIGHT, INPUT);
  
  pinMode(M1_FWD, OUTPUT); pinMode(M1_REV, OUTPUT);
  pinMode(M2_FWD, OUTPUT); pinMode(M2_REV, OUTPUT);
  pinMode(M3_FWD, OUTPUT); pinMode(M3_REV, OUTPUT);
  pinMode(M4_FWD, OUTPUT); pinMode(M4_REV, OUTPUT);
  
  analogWriteFrequency(10000);
}

void loop() {
  int left = digitalRead(LINE_LEFT);
  int right = digitalRead(LINE_RIGHT);
  
  if (left == 1 && right == 1) {
    // 两边都是白色，直行
    setMotors(180, 180);
  } else if (left == 0 && right == 1) {
    // 左边检测到黑线，左转
    setMotors(-90, 200);
  } else if (left == 1 && right == 0) {
    // 右边检测到黑线，右转
    setMotors(200, -90);
  } else {
    // 两边都是黑线，停止或后退
    setMotors(-150, -150);
  }
  
  delay(10);
}
```

---

## I²C 传感器

未来科技盒 2.0 支持多种 I²C 传感器，通过 Grove 接口 2 或 5 连接。

| 引脚 | GPIO | 说明 |
|------|------|------|
| SDA | GPIO39 | I²C 数据线 |
| SCL | GPIO40 | I²C 时钟线 |

### 支持的 I²C 传感器

| 传感器型号 | I²C 地址 | 功能 | 推荐库 |
|-----------|----------|------|--------|
| LIS3DHTR | 0x18/0x19 | 三轴加速度计 ±2g~±16g + 温度 | `LIS3DHTR` |
| VEML6040 | 0x10 | RGBW 颜色传感器 + 环境光 | `VEML6040` |
| DHT20 | 0x38 | 温湿度传感器 | `Grove_Temperature_And_Humidity_Sensor` |

### 📌 I²C 初始化（通用）

```cpp
#include <Wire.h>

#define I2C_SDA 39
#define I2C_SCL 40

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);  // 指定SDA和SCL引脚
}
```

---

### LIS3DHTR 三轴加速度计

测量范围：±2g, ±4g, ±8g, ±16g
I²C 地址：0x18（默认）或 0x19

**PlatformIO 库依赖**：
```ini
lib_deps = 
    seeed-studio/Grove-3-Axis-Digital-Accelerometer-2g-to-16g-LIS3DHTR
```

**代码示例**：

```cpp
#include <Wire.h>
#include "LIS3DHTR.h"

LIS3DHTR<TwoWire> accel;

#define I2C_SDA 39
#define I2C_SCL 40

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  
  accel.begin(Wire, 0x18);  // I2C地址0x18
  if (!accel) {
    Serial.println("LIS3DHTR 未检测到!");
    while(1);
  }
  
  accel.setOutputDataRate(LIS3DHTR_DATARATE_50HZ);
  accel.setFullScaleRange(LIS3DHTR_RANGE_2G);
  accel.setHighSolution(true);
  
  Serial.println("LIS3DHTR 初始化成功!");
}

void loop() {
  float x = accel.getAccelerationX();
  float y = accel.getAccelerationY();
  float z = accel.getAccelerationZ();
  
  Serial.print("X: "); Serial.print(x, 2);
  Serial.print("  Y: "); Serial.print(y, 2);
  Serial.print("  Z: "); Serial.println(z, 2);
  
  delay(100);
}
```

**倾斜检测示例**：

```cpp
void loop() {
  float x = accel.getAccelerationX();
  float y = accel.getAccelerationY();
  
  if (x > 0.3) {
    Serial.println("向右倾斜");
  } else if (x < -0.3) {
    Serial.println("向左倾斜");
  }
  
  if (y > 0.3) {
    Serial.println("向前倾斜");
  } else if (y < -0.3) {
    Serial.println("向后倾斜");
  }
  
  delay(100);
}
```

---

### VEML6040 RGBW 颜色传感器

测量：红、绿、蓝、白色光强度 + 色温 + 环境光照度
I²C 地址：0x10

**PlatformIO 库依赖**：
```ini
lib_deps = 
    thewknd/VEML6040
```

**代码示例**：

```cpp
#include <Wire.h>
#include "veml6040.h"

VEML6040 colorSensor;

#define I2C_SDA 39
#define I2C_SCL 40

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  
  if (!colorSensor.begin()) {
    Serial.println("VEML6040 未检测到!");
    while(1);
  }
  
  // 设置配置：320ms积分时间 + 自动模式 + 使能
  colorSensor.setConfiguration(VEML6040_IT_320MS + VEML6040_AF_AUTO + VEML6040_SD_ENABLE);
  
  Serial.println("VEML6040 初始化成功!");
}

void loop() {
  uint16_t red = colorSensor.getRed();
  uint16_t green = colorSensor.getGreen();
  uint16_t blue = colorSensor.getBlue();
  uint16_t white = colorSensor.getWhite();
  uint16_t cct = colorSensor.getCCT();  // 色温
  float lux = colorSensor.getAmbientLight();  // 环境光
  
  Serial.print("R:"); Serial.print(red);
  Serial.print(" G:"); Serial.print(green);
  Serial.print(" B:"); Serial.print(blue);
  Serial.print(" W:"); Serial.print(white);
  Serial.print(" CCT:"); Serial.print(cct);
  Serial.print("K Lux:"); Serial.println(lux);
  
  delay(400);
}
```

**颜色识别示例**：

```cpp
String detectColor() {
  uint16_t r = colorSensor.getRed();
  uint16_t g = colorSensor.getGreen();
  uint16_t b = colorSensor.getBlue();
  uint16_t w = colorSensor.getWhite();
  
  // 光线太暗或太亮时无法识别
  if (w < 500 || w > 5000) {
    return "未知";
  }
  
  // 找出最大分量
  if (r > g && r > b && r > 100) {
    return "红色";
  } else if (g > r && g > b && g > 100) {
    return "绿色";
  } else if (b > r && b > g && b > 100) {
    return "蓝色";
  } else {
    return "未知";
  }
}
```

---

### DHT20 温湿度传感器

测量范围：温度 -40~80°C，湿度 0~100%
I²C 地址：0x38

**PlatformIO 库依赖**：
```ini
lib_deps = 
    seeed-studio/Grove Temperature And Humidity Sensor
```

**代码示例**：

```cpp
#include <Wire.h>
#include "Grove_Temperature_And_Humidity_Sensor.h"

DHT dht(DHT20);  // DHT20 使用I2C，无需指定引脚

#define I2C_SDA 39
#define I2C_SCL 40

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  dht.begin();
  Serial.println("DHT20 初始化成功!");
}

void loop() {
  float data[2] = {0};
  
  if (!dht.readTempAndHumidity(data)) {
    Serial.print("湿度: ");
    Serial.print(data[0], 1);
    Serial.print(" %  温度: ");
    Serial.print(data[1], 1);
    Serial.println(" °C");
  } else {
    Serial.println("读取失败!");
  }
  
  delay(2000);  // DHT20 读取间隔至少2秒
}
```

---

## 超声波测距传感器

超声波测距传感器可用于检测传感器与其正前方障碍物之间的距离。

| 参数 | 值 |
|-----|-----|
| 测量范围 | 3cm ~ 350cm |
| 通信方式 | 单线脉冲 |
| 推荐接口 | 接口1/3/4/6（数字接口） |

**⚠️ 注意**：超声波传感器**不建议连接到接口5（D5）**，以免造成数据冲突（与 I2C 共用引脚）。

### 接口对应 GPIO

| 接口编号 | 信号引脚(PIN1) | 说明 |
|---------|---------------|------|
| 接口 1 | GPIO5 | 推荐 |
| 接口 3 | GPIO2 | 推荐 |
| 接口 4 | GPIO7 | 推荐 |
| 接口 6 | GPIO3 | 推荐 |

### 📌 代码示例：超声波测距

```cpp
// 超声波传感器引脚（根据实际连接修改）
// 接口1=GPIO5, 接口3=GPIO2, 接口4=GPIO7, 接口6=GPIO3
#define ULTRASONIC_PIN 3  // 示例：接口6

// 测量距离函数
float measureDistance() {
  // 发送触发信号
  pinMode(ULTRASONIC_PIN, OUTPUT);
  digitalWrite(ULTRASONIC_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_PIN, LOW);
  
  // 接收回波
  pinMode(ULTRASONIC_PIN, INPUT);
  unsigned long duration = pulseIn(ULTRASONIC_PIN, HIGH, 30000);  // 超时30ms
  
  if (duration == 0) {
    return -1;  // 超时，无有效回波
  }
  
  // 计算距离（声速 340m/s）
  float distance = duration * 0.034 / 2.0;
  
  // 范围检查（有效范围 3-350cm）
  if (distance < 3 || distance > 350) {
    return -1;
  }
  
  return distance;
}

void setup() {
  Serial.begin(115200);
}

void loop() {
  float distance = measureDistance();
  
  if (distance > 0) {
    Serial.print("距离: ");
    Serial.print(distance, 1);
    Serial.println(" cm");
  } else {
    Serial.println("距离: 超出范围");
  }
  
  delay(100);  // 每100ms测量一次
}
```

### 📌 超声波 + LED 矩阵联动示例（非阻塞）

当与 LED 矩阵配合使用时，**必须使用非阻塞方式**：

```cpp
#define ULTRASONIC_PIN 3  // 接口6

// LED 矩阵引脚
const int ROW_PINS[3] = {33, 34, 35};
const int COL_PINS[3] = {36, 37, 38};
int ledState[3][3] = {{0,0,0}, {0,0,0}, {0,0,0}};

// 非阻塞定时
unsigned long lastMeasureTime = 0;
const unsigned long MEASURE_INTERVAL = 100;  // 每100ms测量一次

// 距离映射到LED数量（距离越近，亮灯越多）
int distanceToLEDCount(float distance) {
  if (distance < 0 || distance >= 100) return 0;
  // 0-10cm→9颗, 10-20cm→8颗, ..., 80-90cm→1颗
  return 9 - (int)(distance / 10);
}

// 设置点亮LED数量（1-9）
void setLEDCount(int count) {
  // 先全灭
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      ledState[i][j] = 0;
  
  // 从LED1开始点亮
  for (int n = 1; n <= count && n <= 9; n++) {
    int idx = n - 1;
    int row = 2 - (idx / 3);
    int col = idx % 3;
    ledState[row][col] = 1;
  }
}

void scanDisplay() {
  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 3; col++)
      digitalWrite(COL_PINS[col], HIGH);
    digitalWrite(ROW_PINS[row], HIGH);
    for (int col = 0; col < 3; col++)
      digitalWrite(COL_PINS[col], ledState[row][col] ? LOW : HIGH);
    delayMicroseconds(1000);
    digitalWrite(ROW_PINS[row], LOW);
  }
}

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 3; i++) {
    pinMode(ROW_PINS[i], OUTPUT);
    pinMode(COL_PINS[i], OUTPUT);
    digitalWrite(ROW_PINS[i], LOW);
    digitalWrite(COL_PINS[i], HIGH);
  }
}

void loop() {
  unsigned long now = millis();
  
  // 非阻塞定时测量
  if (now - lastMeasureTime >= MEASURE_INTERVAL) {
    lastMeasureTime = now;
    
    float distance = measureDistance();
    int ledCount = distanceToLEDCount(distance);
    setLEDCount(ledCount);
    
    Serial.print("距离: ");
    if (distance > 0) {
      Serial.print(distance, 1);
      Serial.print(" cm → LED: ");
    } else {
      Serial.print("超出范围 → LED: ");
    }
    Serial.println(ledCount);
  }
  
  // 持续扫描LED
  scanDisplay();
}
```

### ❌ 常见错误

```cpp
// 错误1：在有LED扫描时使用 delay()
void loop() {
  float d = measureDistance();
  Serial.println(d);
  delay(500);      // ⚠️ 会导致LED闪烁！
  scanDisplay();
}

// 错误2：忘记范围检查
float d = measureDistance();
if (d > 0) {  // ⚠️ 应该检查 d > 3
  // 处理距离
}
```

---

## PS2 手柄遥控

| 功能 | GPIO | 说明 |
|-----|------|------|
| PS2_CMD | GPIO9 | 命令信号 |
| PS2_DATA | GPIO10 | 数据信号 |
| PS2_CLK | GPIO41 | 时钟信号 |
| PS2_CS | GPIO42 | 片选信号 |

**推荐库**：`PS2X_lib`（已适配 ESP32）

### 📌 PlatformIO 库配置

在 `platformio.ini` 中添加 PS2X 库（需要手动添加到 lib 目录）：

```ini
lib_deps = 
    ; PS2X_lib 需要手动添加，或使用本地库
```

### 📌 PS2 手柄控制小车完整示例

```cpp
#include <Arduino.h>
#include "PS2X_lib.h"

// PS2 手柄引脚
#define PS2_CLK 41
#define PS2_CMD 9
#define PS2_CS  42
#define PS2_DAT 10

// 电机引脚
#define M1_FWD 11
#define M1_REV 12
#define M2_FWD 14
#define M2_REV 13
#define M3_FWD 15
#define M3_REV 16
#define M4_FWD 18
#define M4_REV 17

PS2X ps2x;
int error = 1;

// 电机速度等级映射表
const int PWM_LEVELS = 9;
int pwmValues[PWM_LEVELS] = {0, 150, 160, 170, 190, 210, 220, 230, 240};

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

void setup() {
  Serial.begin(115200);
  
  // 初始化电机引脚
  pinMode(M1_FWD, OUTPUT); pinMode(M1_REV, OUTPUT);
  pinMode(M2_FWD, OUTPUT); pinMode(M2_REV, OUTPUT);
  pinMode(M3_FWD, OUTPUT); pinMode(M3_REV, OUTPUT);
  pinMode(M4_FWD, OUTPUT); pinMode(M4_REV, OUTPUT);
  analogWriteFrequency(10000);
  
  // 初始化 PS2 手柄（重试3次）
  int tryNum = 0;
  while (error != 0 && tryNum < 3) {
    delay(1000);
    error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_CS, PS2_DAT, false, false);
    Serial.print("PS2 初始化尝试 ");
    Serial.println(++tryNum);
  }
  
  if (error == 0) {
    Serial.println("PS2 手柄连接成功!");
  } else {
    Serial.println("PS2 手柄连接失败!");
  }
}

void loop() {
  if (error != 0) {
    delay(1000);
    return;
  }
  
  ps2x.read_gamepad(false, 0);  // 读取手柄状态
  
  // 获取左摇杆值（0-255，中间值约128）
  int ly = ps2x.Analog(PSS_LY);  // 上下（上=0，下=255）
  int lx = ps2x.Analog(PSS_LX);  // 左右（左=0，右=255）
  
  // 转换为速度值（-255 到 255）
  int forward = map(ly, 0, 255, 255, -255);  // 前进后退
  int turn = map(lx, 0, 255, -200, 200);     // 左右转向
  
  // 死区处理（摇杆回中附近不动作）
  if (abs(forward) < 20) forward = 0;
  if (abs(turn) < 20) turn = 0;
  
  // 计算左右电机速度
  int leftSpeed = forward + turn;
  int rightSpeed = forward - turn;
  
  // 限幅
  leftSpeed = constrain(leftSpeed, -255, 255);
  rightSpeed = constrain(rightSpeed, -255, 255);
  
  setMotors(leftSpeed, rightSpeed);
  
  // 按键检测示例
  if (ps2x.ButtonPressed(PSB_CROSS)) {    // X 按下
    Serial.println("X 键按下");
  }
  if (ps2x.ButtonPressed(PSB_CIRCLE)) {   // ○ 按下
    Serial.println("○ 键按下");
  }
  if (ps2x.ButtonPressed(PSB_TRIANGLE)) { // △ 按下
    Serial.println("△ 键按下");
  }
  if (ps2x.ButtonPressed(PSB_SQUARE)) {   // □ 按下
    Serial.println("□ 键按下");
  }
  
  // L1/R1 肩键
  if (ps2x.Button(PSB_L1)) {
    Serial.println("L1 按住");
  }
  if (ps2x.Button(PSB_R1)) {
    Serial.println("R1 按住");
  }
  
  delay(50);
}
```

### PS2 按键常量

| 按键 | 常量 | 说明 |
|------|------|------|
| × | PSB_CROSS / PSB_BLUE | 蓝色叉 |
| ○ | PSB_CIRCLE / PSB_RED | 红色圈 |
| □ | PSB_SQUARE / PSB_PINK | 粉色方 |
| △ | PSB_TRIANGLE / PSB_GREEN | 绿色三角 |
| L1 | PSB_L1 | 左肩键1 |
| R1 | PSB_R1 | 右肩键1 |
| L2 | PSB_L2 | 左肩键2 |
| R2 | PSB_R2 | 右肩键2 |
| L3 | PSB_L3 | 左摇杆按下 |
| R3 | PSB_R3 | 右摇杆按下 |
| SELECT | PSB_SELECT | 选择键 |
| START | PSB_START | 开始键 |
| ↑ | PSB_PAD_UP | 方向键上 |
| ↓ | PSB_PAD_DOWN | 方向键下 |
| ← | PSB_PAD_LEFT | 方向键左 |
| → | PSB_PAD_RIGHT | 方向键右 |

### PS2 摇杆常量

| 摇杆 | 常量 | 值范围 |
|------|------|--------|
| 左摇杆X | PSS_LX | 0(左)-255(右) |
| 左摇杆Y | PSS_LY | 0(上)-255(下) |
| 右摇杆X | PSS_RX | 0(左)-255(右) |
| 右摇杆Y | PSS_RY | 0(上)-255(下) |

---

## 串口通讯（UART）

| 功能 | GPIO | 说明 |
|-----|------|------|
| TX | GPIO43 | 串口发送 |
| RX | GPIO44 | 串口接收 |

**用途**：与外部设备串口通讯、调试输出

---

## 排针引出引脚

| 引脚 | GPIO | 说明 |
|-----|------|------|
| D9 | GPIO45 | 模拟/数字引脚（启动模式选择） |
| D10 | GPIO46 | 模拟/数字引脚（启动模式选择） |

**注意**：GPIO45/46 在启动时有特殊用途，使用时需注意

---

## 电源引脚

| 类型 | 数量 | 电压 |
|-----|------|------|
| VCC | 4 个 | 5V |
| GND | 4 个 | 0V |

---

## USB 烧录

| 功能 | GPIO | 说明 |
|-----|------|------|
| USB D+ | GPIO20 | USB 2.0 数据正 |
| USB D- | GPIO19 | USB 2.0 数据负 |

---

## 引脚快速查询表

```
GPIO0  - 按键B(BOOT)      GPIO1  - Grove接口3      GPIO2  - Grove接口3
GPIO3  - Grove接口6       GPIO4  - Grove接口6      GPIO5  - Grove接口1
GPIO6  - Grove接口1       GPIO7  - Grove接口4      GPIO8  - Grove接口4
GPIO9  - PS2_CMD          GPIO10 - PS2_DATA        GPIO11 - 电机M1
GPIO12 - 电机M1           GPIO13 - 电机M2          GPIO14 - 电机M2
GPIO15 - 电机M3           GPIO16 - 电机M3          GPIO17 - 电机M4
GPIO18 - 电机M4           GPIO19 - USB D-          GPIO20 - USB D+
GPIO21 - 按键A            GPIO26 - 蜂鸣器          GPIO27~32 - Flash
GPIO33 - LED行1           GPIO34 - LED行2          GPIO35 - LED行3
GPIO36 - LED列1           GPIO37 - LED列2          GPIO38 - LED列3
GPIO39 - I2C_SDA          GPIO40 - I2C_SCL         GPIO41 - PS2_CLK
GPIO42 - PS2_CS           GPIO43 - UART_TX         GPIO44 - UART_RX
GPIO45 - D9(排针)         GPIO46 - D10(排针)       GPIO47 - 舵机S1
GPIO48 - 舵机S2
```

---

## 代码生成约束

在生成代码时需要注意以下约束：

1. **GPIO27-32**：已被 Flash 占用，**禁止使用**
2. **GPIO19-20**：USB 烧录专用，**禁止使用**
3. **GPIO0**：BOOT 按键，上电时保持高电平，否则进入下载模式
4. **GPIO45-46**：启动模式选择，使用时需谨慎
5. **I2C**：接口 2 和接口 5 共用 GPIO39/40
   - 必须使用 `Wire.begin(39, 40)` 指定引脚
   - 可同时连接多个不同地址的 I2C 设备
6. **电机控制**（⚠️ 重要）：
   - PWM 频率推荐 10kHz：`analogWriteFrequency(10000)`
   - **右侧电机方向相反**：M2(GPIO14正转/GPIO13反转)、M4(GPIO18正转/GPIO17反转)
   - 小车前进时左右电机同向，转弯时反向
7. **LED 矩阵**（⚠️ 重要）：
   - **必须使用行列扫描方式**，不能简单 digitalWrite
   - **点亮条件**：行 HIGH + 列 LOW
   - **熄灭条件**：列 HIGH（或行 LOW）
   - **必须在 loop() 中持续调用扫描函数**
   - 不能同时点亮所有 LED（需分时复用）
   - 参考硬件文档中的"LED 矩阵"章节获取完整示例代码
8. **超声波传感器**：
   - **不建议连接接口5（D5）**，会与 I2C 引脚冲突
   - 推荐使用接口1/3/4/6
   - 与 LED 矩阵配合时**必须使用非阻塞定时**（millis()）
   - 测量间隔建议 100ms 以上
9. **I²C 传感器**：
   - LIS3DHTR（加速度计）：地址 0x18，需要 `LIS3DHTR` 库
   - VEML6040（颜色传感器）：地址 0x10，需要 `VEML6040` 库
   - DHT20（温湿度）：地址 0x38，需要 `Grove Temperature And Humidity Sensor` 库
10. **PS2 手柄**：
    - 需要 `PS2X_lib` 库（ESP32 版本）
    - 初始化时需重试机制（最多 3 次）
    - 摇杆中间值约 128，需要死区处理
11. **舵机控制**：
    - 使用软件 PWM 定时器中断实现
    - 角度范围 0-180°，PWM 值范围约 4-13

---

## PlatformIO 库依赖配置

根据使用的功能，在 `platformio.ini` 中添加相应的库依赖：

```ini
[env:seeed_xiao_esp32s3]
platform = espressif32
board = seeed_xiao_esp32s3
framework = arduino

lib_deps = 
    ; I2C 传感器库
    seeed-studio/Grove-3-Axis-Digital-Accelerometer-2g-to-16g-LIS3DHTR@^1.2.4
    thewknd/VEML6040@^0.3.2
    seeed-studio/Grove Temperature And Humidity Sensor@^2.0.2
    
    ; PS2 手柄库（需要手动添加或使用本地库）
    ; Arduino-PS2X-ESP32
```
