/**
 * LED 矩阵顺序测试程序
 * 
 * 功能：LED 从 1 到 9 依次点亮，再依次熄灭，循环执行
 * 
 * LED 编号与物理位置映射（已修正）：
 * 
 *      列1(36)  列2(37)  列3(38)
 * 行3(35)  1        2        3      ← 物理第1行（上）
 * 行2(34)  4        5        6      ← 物理第2行（中）
 * 行1(33)  7        8        9      ← 物理第3行（下）
 * 
 * 点亮条件：行=HIGH, 列=LOW
 * 熄灭条件：列=HIGH
 */

#include <Arduino.h>
#include "pins.h"

// 行列引脚数组（按 GPIO 顺序）
const int ROW_PINS[3] = {LED_ROW1, LED_ROW2, LED_ROW3};  // GPIO33, 34, 35
const int COL_PINS[3] = {LED_COL1, LED_COL2, LED_COL3};  // GPIO36, 37, 38

// LED 状态数组：1=点亮, 0=熄灭
// ledState[0] = GPIO33 行, ledState[1] = GPIO34 行, ledState[2] = GPIO35 行
int ledState[3][3] = {
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0}
};

// LED 编号 (1-9) 到行列的映射（已修正）
// 物理位置：上→下 对应 GPIO35→GPIO34→GPIO33（行3→行2→行1）
// LED 1 = [2,0], LED 2 = [2,1], LED 3 = [2,2]  (GPIO35 行)
// LED 4 = [1,0], LED 5 = [1,1], LED 6 = [1,2]  (GPIO34 行)
// LED 7 = [0,0], LED 8 = [0,1], LED 9 = [0,2]  (GPIO33 行)
void setLED(int num, int state) {
  if (num < 1 || num > 9) return;
  int idx = num - 1;
  // 修正：从底部行开始映射（2→1→0）
  int row = 2 - (idx / 3);  // LED 1-3 → row 2, LED 4-6 → row 1, LED 7-9 → row 0
  int col = idx % 3;
  ledState[row][col] = state;
  
  Serial.printf("LED %d: row=%d(GPIO%d), col=%d(GPIO%d) = %s\n", 
                num, row, ROW_PINS[row], col, COL_PINS[col], state ? "ON" : "OFF");
}

// 扫描显示函数 - 必须持续调用
void scanDisplay() {
  for (int row = 0; row < 3; row++) {
    // 先关闭所有列（设为 HIGH = 熄灭）
    for (int col = 0; col < 3; col++) {
      digitalWrite(COL_PINS[col], HIGH);
    }
    
    // 激活当前行（设为 HIGH）
    digitalWrite(ROW_PINS[row], HIGH);
    
    // 根据状态设置列
    for (int col = 0; col < 3; col++) {
      if (ledState[row][col]) {
        digitalWrite(COL_PINS[col], LOW);   // 点亮：列 = LOW
      } else {
        digitalWrite(COL_PINS[col], HIGH);  // 熄灭：列 = HIGH
      }
    }
    
    delayMicroseconds(2000);  // 显示 2ms
    
    // 关闭当前行
    digitalWrite(ROW_PINS[row], LOW);
  }
}

// 持续扫描一段时间（毫秒）
void scanFor(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    scanDisplay();
  }
}

// 熄灭所有 LED
void turnOffAll() {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      ledState[i][j] = 0;
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=================================");
  Serial.println("LED 矩阵顺序测试程序");
  Serial.println("=================================");
  Serial.println("LED 编号映射（已修正）：");
  Serial.println("     列1(36) 列2(37) 列3(38)");
  Serial.println("行3(35)  1      2      3  ← 物理上方");
  Serial.println("行2(34)  4      5      6  ← 物理中间");
  Serial.println("行1(33)  7      8      9  ← 物理下方");
  Serial.println("=================================");
  Serial.println();
  
  // 初始化引脚
  for (int i = 0; i < 3; i++) {
    pinMode(ROW_PINS[i], OUTPUT);
    pinMode(COL_PINS[i], OUTPUT);
    digitalWrite(ROW_PINS[i], LOW);   // 行默认 LOW
    digitalWrite(COL_PINS[i], HIGH);  // 列默认 HIGH（熄灭）
  }
  
  Serial.println("初始化完成，开始测试...\n");
}

void loop() {
  // === 阶段 1：依次点亮 LED 1-9 ===
  Serial.println(">>> 依次点亮 LED 1-9");
  turnOffAll();
  
  for (int i = 1; i <= 9; i++) {
    Serial.printf("\n点亮 LED %d\n", i);
    setLED(i, 1);  // 点亮
    scanFor(500);  // 保持 500ms，持续扫描显示
  }
  
  Serial.println("\n所有 LED 已点亮，保持 2 秒...\n");
  scanFor(2000);
  
  // === 阶段 2：依次熄灭 LED 1-9 ===
  Serial.println(">>> 依次熄灭 LED 1-9");
  
  for (int i = 1; i <= 9; i++) {
    Serial.printf("\n熄灭 LED %d\n", i);
    setLED(i, 0);  // 熄灭
    scanFor(500);  // 保持 500ms
  }
  
  Serial.println("\n所有 LED 已熄灭，等待 2 秒后重新开始...\n");
  delay(2000);
  
  Serial.println("========== 重新开始测试 ==========\n");
}
