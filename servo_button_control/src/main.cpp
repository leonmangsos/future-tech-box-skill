/*
 * 未来科技盒 3.0 - AB按键控制舵机S1
 * 
 * 功能说明：
 *   - 按下 KEY_A：舵机角度增加 10°
 *   - 按下 KEY_B：舵机角度减少 10°
 *   - 舵机角度范围：0° ~ 180°
 *   - 初始角度：90°（中间位置）
 * 
 * 硬件连接：
 *   - 舵机 S1 → GPIO47
 *   - KEY_A → GPIO21（内部上拉，按下为 LOW）
 *   - KEY_B → GPIO0 （内部上拉，按下为 LOW）
 */

#include <Arduino.h>
#include <ESP32Servo.h>
#include "pins.h"

Servo servo1;

int currentAngle = 90;       // 当前舵机角度
const int ANGLE_STEP = 10;   // 每次按键改变的角度
const int ANGLE_MIN = 0;     // 最小角度
const int ANGLE_MAX = 180;   // 最大角度

bool lastKeyA = HIGH;
bool lastKeyB = HIGH;

void setup() {
  Serial.begin(115200);
  delay(2000);  // 等待 USB 串口稳定
  
  // 初始化按键（内部上拉）
  pinMode(KEY_A, INPUT_PULLUP);
  pinMode(KEY_B, INPUT_PULLUP);
  
  // 初始化舵机
  servo1.attach(SERVO1_PIN);
  servo1.write(currentAngle);
  
  Serial.println("=== AB按键控制舵机S1 ===");
  Serial.println("KEY_A: 角度增加 10°");
  Serial.println("KEY_B: 角度减少 10°");
  Serial.printf("初始角度: %d°\n", currentAngle);
  Serial.println("========================");
}

void loop() {
  bool currentKeyA = digitalRead(KEY_A);
  bool currentKeyB = digitalRead(KEY_B);
  
  // 检测 KEY_A 按下（下降沿）
  if (lastKeyA == HIGH && currentKeyA == LOW) {
    delay(50);  // 消抖
    if (digitalRead(KEY_A) == LOW) {  // 确认按下
      currentAngle += ANGLE_STEP;
      if (currentAngle > ANGLE_MAX) {
        currentAngle = ANGLE_MAX;
      }
      servo1.write(currentAngle);
      Serial.printf("KEY_A 按下 → 角度: %d°\n", currentAngle);
    }
  }
  
  // 检测 KEY_B 按下（下降沿）
  if (lastKeyB == HIGH && currentKeyB == LOW) {
    delay(50);  // 消抖
    if (digitalRead(KEY_B) == LOW) {  // 确认按下
      currentAngle -= ANGLE_STEP;
      if (currentAngle < ANGLE_MIN) {
        currentAngle = ANGLE_MIN;
      }
      servo1.write(currentAngle);
      Serial.printf("KEY_B 按下 → 角度: %d°\n", currentAngle);
    }
  }
  
  lastKeyA = currentKeyA;
  lastKeyB = currentKeyB;
  
  delay(10);  // 防止循环过快
}
