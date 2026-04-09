/**
 * 未来科技盒 2.0 - 小车前进，按键A停止
 * 
 * 功能描述：
 * - 启动后小车自动前进
 * - 按下按键A时小车停止
 * - 再次按下按键A时小车继续前进
 */

#include <Arduino.h>
#include "pins.h"

// ============ 状态变量 ============
bool carRunning = true;           // 小车运行状态
bool lastKeyA = HIGH;             // 按键A上次状态
unsigned long lastKeyTime = 0;    // 按键消抖时间

// ============ 电机控制函数 ============

/**
 * 设置单个电机速度
 * @param fwdPin 正转引脚
 * @param revPin 反转引脚
 * @param speed 速度 (-255 ~ 255)，正数前进，负数后退
 */
void setMotor(int fwdPin, int revPin, int speed) {
  if (speed > 0) {
    // 正转
    analogWrite(fwdPin, speed);
    analogWrite(revPin, 0);
  } else if (speed < 0) {
    // 反转
    analogWrite(fwdPin, 0);
    analogWrite(revPin, -speed);
  } else {
    // 停止
    analogWrite(fwdPin, 0);
    analogWrite(revPin, 0);
  }
}

/**
 * 设置所有电机 (左侧/右侧)
 * @param leftSpeed 左侧电机速度 (-255 ~ 255)
 * @param rightSpeed 右侧电机速度 (-255 ~ 255)
 */
void setMotors(int leftSpeed, int rightSpeed) {
  // 左侧电机 (M1, M3)
  setMotor(M1_FWD, M1_REV, leftSpeed);
  setMotor(M3_FWD, M3_REV, leftSpeed);
  
  // 右侧电机 (M2, M4)
  setMotor(M2_FWD, M2_REV, rightSpeed);
  setMotor(M4_FWD, M4_REV, rightSpeed);
}

/**
 * 小车前进
 * @param speed 速度 (0-255)，默认 180
 */
void forward(int speed = 180) {
  setMotors(speed, speed);
  Serial.println("🚗 小车前进");
}

/**
 * 小车停止
 */
void stop() {
  setMotors(0, 0);
  Serial.println("🛑 小车停止");
}

// ============ 初始化 ============
void setup() {
  // 串口初始化
  Serial.begin(115200);
  delay(100);
  Serial.println("\n=== 未来科技盒 2.0 ===");
  Serial.println("功能：小车前进，按键A停止/继续");
  
  // 按键初始化 (内部上拉)
  pinMode(KEY_A, INPUT_PULLUP);
  
  // 电机引脚初始化
  pinMode(M1_FWD, OUTPUT);
  pinMode(M1_REV, OUTPUT);
  pinMode(M2_FWD, OUTPUT);
  pinMode(M2_REV, OUTPUT);
  pinMode(M3_FWD, OUTPUT);
  pinMode(M3_REV, OUTPUT);
  pinMode(M4_FWD, OUTPUT);
  pinMode(M4_REV, OUTPUT);
  
  // 启动时小车前进
  forward();
  carRunning = true;
  
  Serial.println("按下按键A可以停止/继续");
  Serial.println("========================\n");
}

// ============ 主循环 ============
void loop() {
  // 读取按键A状态
  bool currentKeyA = digitalRead(KEY_A);
  
  // 边沿检测 + 非阻塞消抖
  if (lastKeyA == HIGH && currentKeyA == LOW) {
    if (millis() - lastKeyTime > 50) {  // 50ms 消抖
      // 切换小车状态
      carRunning = !carRunning;
      
      if (carRunning) {
        forward();
      } else {
        stop();
      }
      
      lastKeyTime = millis();
    }
  }
  
  lastKeyA = currentKeyA;
  
  // 小延时，避免过度占用 CPU
  delay(10);
}
