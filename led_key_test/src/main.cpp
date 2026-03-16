/**
 * LED 按键控制测试程序
 * 
 * 功能：
 * - 开机时所有 LED 点亮
 * - 按下按键 A：切换 LED 状态（开↔关）
 * - 按下按键 B：切换 LED 状态（开↔关）
 * 
 * 注意：按键按下时为 LOW（内部上拉）
 */

#include <Arduino.h>
#include "pins.h"

// 行列引脚数组
const int ROW_PINS[3] = {LED_ROW1, LED_ROW2, LED_ROW3};
const int COL_PINS[3] = {LED_COL1, LED_COL2, LED_COL3};

// LED 状态数组：1=点亮, 0=熄灭
int ledState[3][3] = {
  {1, 1, 1},  // 默认全部点亮
  {1, 1, 1},
  {1, 1, 1}
};

// LED 总开关状态
bool ledOn = true;

// 按键状态记录（用于边沿检测）
bool lastKeyA = HIGH;
bool lastKeyB = HIGH;

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
      if (ledOn && ledState[row][col]) {
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
  ledOn = true;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      ledState[i][j] = 1;
    }
  }
  Serial.println("LED: ON (全部点亮)");
}

// 熄灭所有 LED
void turnOffAllLEDs() {
  ledOn = false;
  Serial.println("LED: OFF (全部熄灭)");
}

// 切换 LED 状态
void toggleLEDs() {
  if (ledOn) {
    turnOffAllLEDs();
  } else {
    turnOnAllLEDs();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("================================");
  Serial.println("LED 按键控制测试程序");
  Serial.println("================================");
  Serial.println("按键 A (GPIO21): 切换 LED 开/关");
  Serial.println("按键 B (GPIO0):  切换 LED 开/关");
  Serial.println("按键按下时读取为 LOW");
  Serial.println("================================");
  
  // 初始化 LED 引脚
  for (int i = 0; i < 3; i++) {
    pinMode(ROW_PINS[i], OUTPUT);
    pinMode(COL_PINS[i], OUTPUT);
    digitalWrite(ROW_PINS[i], LOW);
    digitalWrite(COL_PINS[i], HIGH);
  }
  
  // 初始化按键引脚（内部上拉）
  pinMode(KEY_A, INPUT_PULLUP);
  pinMode(KEY_B, INPUT_PULLUP);
  
  // 开机时点亮所有 LED
  turnOnAllLEDs();
  
  Serial.println("初始化完成，LED 已点亮");
  Serial.println();
}

void loop() {
  // 读取当前按键状态
  bool currentKeyA = digitalRead(KEY_A);
  bool currentKeyB = digitalRead(KEY_B);
  
  // 检测按键 A 的下降沿（按下瞬间）
  if (lastKeyA == HIGH && currentKeyA == LOW) {
    Serial.println(">>> 检测到按键 A 按下");
    toggleLEDs();
    delay(50);  // 消抖
  }
  
  // 检测按键 B 的下降沿（按下瞬间）
  if (lastKeyB == HIGH && currentKeyB == LOW) {
    Serial.println(">>> 检测到按键 B 按下");
    toggleLEDs();
    delay(50);  // 消抖
  }
  
  // 更新按键状态记录
  lastKeyA = currentKeyA;
  lastKeyB = currentKeyB;
  
  // 持续扫描 LED 显示
  scanDisplay();
}
