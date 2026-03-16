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

## 电机（Motor）

小车模式下电机位置对应关系：

| 电机编号 | 位置 | GPIO（正向） | GPIO（反向） |
|---------|------|-------------|-------------|
| M1 | 左前方 | GPIO11 | GPIO12 |
| M2 | 右前方 | GPIO13 | GPIO14 |
| M3 | 左后方 | GPIO15 | GPIO16 |
| M4 | 右后方 | GPIO17 | GPIO18 |

**控制方式**：每个电机使用两个 GPIO 进行 H 桥控制（正反转 + PWM 调速）

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

## 舵机（Servo）

| 舵机编号 | GPIO | 说明 |
|---------|------|------|
| S1 | GPIO47 | 舵机通道 1 |
| S2 | GPIO48 | 舵机通道 2 |

**控制方式**：PWM（50Hz，脉宽 500-2500μs 对应 0-180°）

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

## PS2 手柄接口

| 功能 | GPIO | 说明 |
|-----|------|------|
| PS2_CMD | GPIO9 | 命令信号 |
| PS2_DATA | GPIO10 | 数据信号 |
| PS2_CLK | GPIO41 | 时钟信号 |
| PS2_CS | GPIO42 | 片选信号 |

**推荐库**：`PS2X_lib`

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
5. **I2C**：接口 2 和接口 5 共用 GPIO39/40，同时只能接一个 I2C 设备地址
6. **电机控制**：使用 PWM 时注意频率一致性（推荐 1kHz）
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
