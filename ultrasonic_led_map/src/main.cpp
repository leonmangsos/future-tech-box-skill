/**
 * 未来科技盒 2.0 - 超声波距离映射LED
 * 
 * 功能：超声波传感器检测距离，距离越近，点亮的LED越多
 * 
 * 硬件连接：
 *   - 超声波传感器 → 接口6 (Grove6, GPIO3)
 *   - LED 矩阵 → 板载 3×3 LED
 * 
 * 距离映射规则（可调整）：
 *   - 0-10cm   → 9颗LED全亮
 *   - 10-20cm  → 8颗LED
 *   - 20-30cm  → 7颗LED
 *   - 30-40cm  → 6颗LED
 *   - 40-50cm  → 5颗LED
 *   - 50-60cm  → 4颗LED
 *   - 60-70cm  → 3颗LED
 *   - 70-80cm  → 2颗LED
 *   - 80-100cm → 1颗LED
 *   - >100cm   → 全灭
 */

#include <Arduino.h>

// ========== 超声波传感器配置 ==========
#define ULTRASONIC_PIN 3  // 接口6 - GPIO3

// ========== LED 矩阵配置 ==========
// 行引脚（阳极，HIGH = 选中）
const int ROW_PINS[3] = {33, 34, 35};  // GPIO33=物理下方, GPIO35=物理上方
// 列引脚（阴极，LOW = 点亮）
const int COL_PINS[3] = {36, 37, 38};  // 从左到右

// LED 状态数组：1=点亮, 0=熄灭
// ledState[0] = GPIO33 行（物理下方, LED 7-9）
// ledState[1] = GPIO34 行（物理中间, LED 4-6）
// ledState[2] = GPIO35 行（物理上方, LED 1-3）
int ledState[3][3] = {
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0}
};

// ========== 距离映射配置 ==========
// 每10cm对应少亮1颗灯，可根据需求调整
const int DISTANCE_STEP = 10;      // 每10cm一个档位
const int MAX_DISTANCE = 100;      // 超过此距离全灭
const int MIN_DISTANCE = 3;        // 最小有效距离

// ========== 超声波测距函数 ==========
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
  
  // 范围检查
  if (distance < MIN_DISTANCE || distance > 350) {
    return -1;
  }
  
  return distance;
}

// ========== LED 控制函数 ==========
// 设置指定编号LED状态（1-9）
void setLED(int num, int state) {
  if (num < 1 || num > 9) return;
  int idx = num - 1;
  int row = 2 - (idx / 3);  // LED 1-3→row 2, LED 4-6→row 1, LED 7-9→row 0
  int col = idx % 3;
  ledState[row][col] = state;
}

// 熄灭所有LED
void turnOffAllLEDs() {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      ledState[i][j] = 0;
    }
  }
}

// 点亮指定数量的LED（从LED1开始，1-9）
void setLEDCount(int count) {
  turnOffAllLEDs();
  
  if (count <= 0) return;
  if (count > 9) count = 9;
  
  // 从LED 1到LED count 依次点亮
  for (int i = 1; i <= count; i++) {
    setLED(i, 1);
  }
}

// LED扫描显示函数 - 必须在loop()中持续调用
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

// ========== 距离映射到LED数量 ==========
int distanceToLEDCount(float distance) {
  if (distance < 0) {
    return 0;  // 无效测量，全灭
  }
  
  if (distance >= MAX_DISTANCE) {
    return 0;  // 超出范围，全灭
  }
  
  // 距离越近，LED越多
  // 0-10cm → 9颗, 10-20cm → 8颗, ..., 80-90cm → 1颗
  int count = 9 - (int)(distance / DISTANCE_STEP);
  
  if (count < 0) count = 0;
  if (count > 9) count = 9;
  
  return count;
}

// ========== 非阻塞定时 ==========
unsigned long lastMeasureTime = 0;
const unsigned long MEASURE_INTERVAL = 100;  // 每100ms测量一次

float currentDistance = -1;
int currentLEDCount = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println();
  Serial.println("========================================");
  Serial.println("  未来科技盒 2.0 - 超声波距离映射LED");
  Serial.println("========================================");
  Serial.println("接口: Grove6 (GPIO3)");
  Serial.println("映射规则:");
  Serial.println("  0-10cm  → 9颗LED");
  Serial.println("  10-20cm → 8颗LED");
  Serial.println("  20-30cm → 7颗LED");
  Serial.println("  ...以此类推...");
  Serial.println("  >100cm  → 全灭");
  Serial.println();
  
  // 初始化LED矩阵引脚
  for (int i = 0; i < 3; i++) {
    pinMode(ROW_PINS[i], OUTPUT);
    pinMode(COL_PINS[i], OUTPUT);
    digitalWrite(ROW_PINS[i], LOW);   // 行默认低电平（不选中）
    digitalWrite(COL_PINS[i], HIGH);  // 列默认高电平（熄灭）
  }
  
  // 开机全亮测试
  setLEDCount(9);
  Serial.println("LED测试: 全亮...");
}

void loop() {
  unsigned long now = millis();
  
  // 非阻塞定时测量
  if (now - lastMeasureTime >= MEASURE_INTERVAL) {
    lastMeasureTime = now;
    
    // 测量距离
    currentDistance = measureDistance();
    
    // 映射到LED数量
    int newLEDCount = distanceToLEDCount(currentDistance);
    
    // 只在数量变化时更新LED
    if (newLEDCount != currentLEDCount) {
      currentLEDCount = newLEDCount;
      setLEDCount(currentLEDCount);
    }
    
    // 串口输出
    if (currentDistance > 0) {
      Serial.print("距离: ");
      Serial.print(currentDistance, 1);
      Serial.print(" cm → LED: ");
      Serial.print(currentLEDCount);
      Serial.println(" 颗");
    } else {
      Serial.println("距离: 超出范围 → LED: 0 颗");
    }
  }
  
  // 持续扫描LED显示
  scanDisplay();
}
