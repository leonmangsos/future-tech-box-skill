/**
 * 三轴加速度计倾斜 → LED 矩阵显示
 * 
 * 功能说明：
 * - 使用 LIS3DHTR 三轴加速度计检测倾斜方向
 * - 3×3 LED 矩阵显示倾斜位置
 * - 水平时中间 LED 亮，倾斜时对应方向的 LED 亮
 * 
 * LED 矩阵布局（物理视角）：
 *      左    中    右
 * 上    1     2     3    （前方）
 * 中    4     5     6
 * 下    7     8     9    （后方）
 * 
 * 倾斜映射：
 * - 水平：中间(5)亮
 * - 向前倾：上排(1,2,3)亮
 * - 向后倾：下排(7,8,9)亮
 * - 向左倾：左列(1,4,7)亮
 * - 向右倾：右列(3,6,9)亮
 * - 斜向倾：对应角落亮
 */

#include <Arduino.h>
#include <Wire.h>
#include "LIS3DHTR.h"
#include "pins.h"

// ==================== 全局对象 ====================
LIS3DHTR<TwoWire> accel;

// LED 矩阵行列引脚数组
const int ROW_PINS[3] = {LED_ROW1, LED_ROW2, LED_ROW3};  // 33, 34, 35
const int COL_PINS[3] = {LED_COL1, LED_COL2, LED_COL3};  // 36, 37, 38

// LED 状态数组：1=点亮, 0=熄灭
// ledState[0] = GPIO33 行（物理下方）
// ledState[1] = GPIO34 行（物理中间）
// ledState[2] = GPIO35 行（物理上方）
int ledState[3][3] = {
  {0, 0, 0},  // 下排 LED 7,8,9
  {0, 0, 0},  // 中排 LED 4,5,6
  {0, 0, 0}   // 上排 LED 1,2,3
};

// 倾斜检测阈值（单位：g，范围 0-1）
const float TILT_THRESHOLD = 0.25;  // 倾斜超过这个值才触发

// 非阻塞定时
unsigned long lastUpdateTime = 0;
const unsigned long UPDATE_INTERVAL = 50;  // 每 50ms 更新一次倾斜状态

// 传感器状态
bool sensorFound = false;
uint8_t sensorAddr = 0;

// ==================== LED 控制函数 ====================

/**
 * 根据 LED 编号（1-9）设置 LED 状态
 * 
 * LED 编号对应位置：
 *   1  2  3  (上排, GPIO35)
 *   4  5  6  (中排, GPIO34)
 *   7  8  9  (下排, GPIO33)
 */
void setLED(int num, int state) {
  if (num < 1 || num > 9) return;
  int idx = num - 1;
  int row = 2 - (idx / 3);  // LED 1-3→row 2, LED 4-6→row 1, LED 7-9→row 0
  int col = idx % 3;
  ledState[row][col] = state;
}

// 熄灭所有 LED
void clearAllLEDs() {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      ledState[i][j] = 0;
    }
  }
}

// 点亮指定行（row: 0=下, 1=中, 2=上）
void setRow(int row, int state) {
  for (int col = 0; col < 3; col++) {
    ledState[row][col] = state;
  }
}

// 点亮指定列（col: 0=左, 1=中, 2=右）
void setCol(int col, int state) {
  for (int row = 0; row < 3; row++) {
    ledState[row][col] = state;
  }
}

// LED 矩阵扫描显示函数 - 必须在 loop() 中持续调用
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

// ==================== I2C 扫描 ====================

uint8_t scanI2C() {
  Serial.println("正在扫描 I2C 设备...");
  uint8_t foundAddr = 0;
  
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("  找到设备: 0x");
      Serial.println(addr, HEX);
      
      // 检查是否为 LIS3DHTR
      if (addr == 0x18 || addr == 0x19) {
        foundAddr = addr;
      }
    }
  }
  
  return foundAddr;
}

// ==================== 倾斜检测与 LED 映射 ====================

/**
 * 根据加速度值更新 LED 显示
 * 
 * X 轴：正值=右倾，负值=左倾
 * Y 轴：正值=前倾，负值=后倾
 * 
 * 注意：根据传感器安装方向可能需要调整正负号
 */
void updateLEDFromTilt(float x, float y) {
  // 先清空所有 LED
  clearAllLEDs();
  
  // 确定前后方向（Y轴）
  int yDir = 0;  // 0=中间, 1=前, -1=后
  if (y > TILT_THRESHOLD) {
    yDir = 1;   // 向前倾
  } else if (y < -TILT_THRESHOLD) {
    yDir = -1;  // 向后倾
  }
  
  // 确定左右方向（X轴）
  int xDir = 0;  // 0=中间, 1=右, -1=左
  if (x > TILT_THRESHOLD) {
    xDir = 1;   // 向右倾
  } else if (x < -TILT_THRESHOLD) {
    xDir = -1;  // 向左倾
  }
  
  // 根据方向点亮对应的 LED
  // 计算行和列索引
  // 行：yDir=1（前）→ row=2（上），yDir=-1（后）→ row=0（下），yDir=0 → row=1（中）
  // 列：xDir=-1（左）→ col=0，xDir=1（右）→ col=2，xDir=0 → col=1（中）
  
  int targetRow = 1 + yDir;  // 0, 1, 2
  int targetCol = 1 + xDir;  // 0, 1, 2
  
  // 点亮目标位置
  ledState[targetRow][targetCol] = 1;
  
  // 如果倾斜角度较大，可以点亮整行或整列增强效果
  float tiltMagnitude = sqrt(x*x + y*y);
  
  if (tiltMagnitude > TILT_THRESHOLD * 2) {
    // 大幅度倾斜：点亮整行和整列
    if (yDir != 0) {
      setRow(targetRow, 1);  // 点亮整行
    }
    if (xDir != 0) {
      setCol(targetCol, 1);  // 点亮整列
    }
  }
}

// ==================== 初始化 ====================

void setup() {
  Serial.begin(115200);
  delay(2000);  // 等待串口稳定
  
  Serial.println();
  Serial.println("========================================");
  Serial.println("  三轴加速度计倾斜 → LED 矩阵显示");
  Serial.println("========================================");
  Serial.println();
  
  // 初始化 LED 矩阵引脚
  Serial.println("初始化 LED 矩阵...");
  for (int i = 0; i < 3; i++) {
    pinMode(ROW_PINS[i], OUTPUT);
    pinMode(COL_PINS[i], OUTPUT);
    digitalWrite(ROW_PINS[i], LOW);   // 行默认低电平（不选中）
    digitalWrite(COL_PINS[i], HIGH);  // 列默认高电平（熄灭）
  }
  Serial.println("  行引脚: GPIO33, GPIO34, GPIO35");
  Serial.println("  列引脚: GPIO36, GPIO37, GPIO38");
  
  // 初始化 I2C
  Serial.println();
  Serial.println("初始化 I2C...");
  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.print("  SDA: GPIO");
  Serial.print(I2C_SDA);
  Serial.print(", SCL: GPIO");
  Serial.println(I2C_SCL);
  
  // 扫描 I2C 设备
  Serial.println();
  sensorAddr = scanI2C();
  
  if (sensorAddr == 0) {
    Serial.println();
    Serial.println("⚠️ 未检测到 LIS3DHTR 加速度计!");
    Serial.println("  可能原因:");
    Serial.println("    1. 传感器未连接到 Grove I2C 接口");
    Serial.println("    2. 接线错误");
    Serial.println();
    Serial.println("将进入 LED 演示模式...");
    sensorFound = false;
  } else {
    Serial.println();
    Serial.print("使用 I2C 地址: 0x");
    Serial.println(sensorAddr, HEX);
    
    // 初始化加速度计
    accel.begin(Wire, sensorAddr);
    delay(100);
    
    if (!accel) {
      Serial.println("❌ 传感器初始化失败!");
      sensorFound = false;
    } else {
      // 配置加速度计
      accel.setOutputDataRate(LIS3DHTR_DATARATE_50HZ);  // 50Hz 数据率
      accel.setFullScaleRange(LIS3DHTR_RANGE_2G);       // ±2g 范围
      accel.setHighSolution(true);                       // 高分辨率模式
      
      Serial.println("✅ LIS3DHTR 初始化成功!");
      sensorFound = true;
    }
  }
  
  Serial.println();
  Serial.println("========================================");
  if (sensorFound) {
    Serial.println("倾斜板子，观察 LED 矩阵显示：");
    Serial.println("  - 水平：中间 LED 亮");
    Serial.println("  - 前倾：上排 LED 亮");
    Serial.println("  - 后倾：下排 LED 亮");
    Serial.println("  - 左倾：左列 LED 亮");
    Serial.println("  - 右倾：右列 LED 亮");
  } else {
    Serial.println("演示模式: LED 会自动循环显示");
  }
  Serial.println("========================================");
  Serial.println();
  
  // 开机点亮中间 LED
  setLED(5, 1);
}

// ==================== 主循环 ====================

// 演示模式计数器
int demoStep = 0;
unsigned long demoTimer = 0;

void loop() {
  unsigned long now = millis();
  
  if (sensorFound) {
    // 正常模式：读取传感器并更新 LED
    if (now - lastUpdateTime >= UPDATE_INTERVAL) {
      lastUpdateTime = now;
      
      // 读取加速度值
      float x = accel.getAccelerationX();
      float y = accel.getAccelerationY();
      float z = accel.getAccelerationZ();
      
      // 更新 LED 显示
      updateLEDFromTilt(x, y);
      
      // 串口输出调试信息
      Serial.print("X: ");
      Serial.print(x, 2);
      Serial.print("  Y: ");
      Serial.print(y, 2);
      Serial.print("  Z: ");
      Serial.print(z, 2);
      
      // 显示方向
      Serial.print("  → ");
      if (abs(x) < TILT_THRESHOLD && abs(y) < TILT_THRESHOLD) {
        Serial.print("水平");
      } else {
        if (y > TILT_THRESHOLD) Serial.print("前");
        if (y < -TILT_THRESHOLD) Serial.print("后");
        if (x < -TILT_THRESHOLD) Serial.print("左");
        if (x > TILT_THRESHOLD) Serial.print("右");
      }
      Serial.println();
    }
  } else {
    // 演示模式：循环点亮每个 LED
    if (now - demoTimer >= 300) {
      demoTimer = now;
      
      clearAllLEDs();
      
      // 循环点亮 1-9 号 LED
      setLED(demoStep + 1, 1);
      
      demoStep = (demoStep + 1) % 9;
    }
  }
  
  // 持续扫描 LED 矩阵（非阻塞）
  scanDisplay();
}
