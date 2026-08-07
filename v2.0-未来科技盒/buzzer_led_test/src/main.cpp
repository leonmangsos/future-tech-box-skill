/**
 * 蜂鸣器 + LED 按键控制测试程序
 * 
 * 功能：
 * - 按键 A：蜂鸣器响一声 + 点亮所有 LED
 * - 按键 B：蜂鸣器响一声 + 熄灭所有 LED
 * 
 * 关键点：
 * - 使用边沿检测检测按键
 * - 使用非阻塞方式控制蜂鸣器，不影响 LED 扫描
 * - 使用 millis() 非阻塞消抖
 */

#include <Arduino.h>
#include "pins.h"

// 行列引脚数组
const int ROW_PINS[3] = {LED_ROW1, LED_ROW2, LED_ROW3};
const int COL_PINS[3] = {LED_COL1, LED_COL2, LED_COL3};

// LED 状态数组：1=点亮, 0=熄灭
int ledState[3][3] = {
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0}
};

// 按键状态记录（用于边沿检测）
bool lastKeyA = HIGH;
bool lastKeyB = HIGH;

// 非阻塞消抖计时
unsigned long lastKeyATime = 0;
unsigned long lastKeyBTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;

// 蜂鸣器非阻塞控制
unsigned long buzzerEndTime = 0;
bool buzzerActive = false;

// ============ LED 控制函数 ============

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
    
    delayMicroseconds(1000);  // 显示 1ms（微秒级延时不影响响应）
    
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
  Serial.println("LED: 全部点亮");
}

// 熄灭所有 LED
void turnOffAllLEDs() {
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      ledState[i][j] = 0;
    }
  }
  Serial.println("LED: 全部熄灭");
}

// ============ 蜂鸣器控制函数（非阻塞）============

// 非阻塞播放蜂鸣器
void beepNonBlocking(int duration = 100) {
  tone(BUZZER_PIN, 1000);  // 播放 1000Hz
  buzzerEndTime = millis() + duration;
  buzzerActive = true;
  Serial.println("蜂鸣器: 响");
}

// 检查蜂鸣器是否需要停止（在 loop 中调用）
void updateBuzzer() {
  if (buzzerActive && millis() >= buzzerEndTime) {
    noTone(BUZZER_PIN);
    buzzerActive = false;
  }
}

// ============ 主程序 ============

void setup() {
  Serial.begin(115200);
  delay(500);
  
  Serial.println("================================");
  Serial.println("蜂鸣器 + LED 按键控制测试");
  Serial.println("================================");
  Serial.println("按键 A: 蜂鸣器响 + 点亮 LED");
  Serial.println("按键 B: 蜂鸣器响 + 熄灭 LED");
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
  
  // 初始化蜂鸣器引脚
  pinMode(BUZZER_PIN, OUTPUT);
  
  Serial.println("初始化完成");
}

void loop() {
  // 读取当前按键状态
  bool currentKeyA = digitalRead(KEY_A);
  bool currentKeyB = digitalRead(KEY_B);
  unsigned long now = millis();
  
  // 检测按键 A 的下降沿（按下瞬间）+ 非阻塞消抖
  if (lastKeyA == HIGH && currentKeyA == LOW) {
    if (now - lastKeyATime > DEBOUNCE_DELAY) {
      Serial.println(">>> 按键 A 按下");
      beepNonBlocking(100);  // 非阻塞响 100ms
      turnOnAllLEDs();
      lastKeyATime = now;
    }
  }
  
  // 检测按键 B 的下降沿（按下瞬间）+ 非阻塞消抖
  if (lastKeyB == HIGH && currentKeyB == LOW) {
    if (now - lastKeyBTime > DEBOUNCE_DELAY) {
      Serial.println(">>> 按键 B 按下");
      beepNonBlocking(100);  // 非阻塞响 100ms
      turnOffAllLEDs();
      lastKeyBTime = now;
    }
  }
  
  // 更新按键状态记录
  lastKeyA = currentKeyA;
  lastKeyB = currentKeyB;
  
  // 检查蜂鸣器状态（非阻塞）
  updateBuzzer();
  
  // 持续扫描 LED 显示（不会被阻塞）
  scanDisplay();
}
