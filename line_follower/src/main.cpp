/**
 * 未来科技盒 2.0 - 循迹小车
 * 
 * 功能：沿黑线行驶
 * - 左右循迹传感器检测黑线
 * - 检测到黑线时传感器输出 LOW
 * - 按键A：启动/停止循迹
 * - 按键B：紧急停止
 */

#include <Arduino.h>
#include "pins.h"

// 运行状态
bool isRunning = false;

// 按键状态
bool lastKeyA = HIGH;
bool lastKeyB = HIGH;

// 电机速度参数
const int BASE_SPEED = 180;   // 基础速度
const int TURN_SPEED = 200;   // 转弯外侧速度
const int TURN_SLOW = 90;     // 转弯内侧速度（反向）

// 设置单个电机
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

// 设置左右电机
void setMotors(int leftSpeed, int rightSpeed) {
  setMotor(M1_FWD, M1_REV, leftSpeed);
  setMotor(M2_FWD, M2_REV, rightSpeed);
  setMotor(M3_FWD, M3_REV, leftSpeed);
  setMotor(M4_FWD, M4_REV, rightSpeed);
}

void carStop() {
  setMotors(0, 0);
}

void carForward() {
  setMotors(BASE_SPEED, BASE_SPEED);
}

void carTurnLeft() {
  // 左转：左轮反转，右轮正转
  setMotors(-TURN_SLOW, TURN_SPEED);
}

void carTurnRight() {
  // 右转：左轮正转，右轮反转
  setMotors(TURN_SPEED, -TURN_SLOW);
}

void carBackward() {
  setMotors(-BASE_SPEED, -BASE_SPEED);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n========================================");
  Serial.println("未来科技盒 2.0 - 循迹小车");
  Serial.println("========================================");
  Serial.println("按键A: 启动/停止循迹");
  Serial.println("按键B: 紧急停止");
  Serial.println("========================================\n");
  
  // 初始化循迹传感器
  pinMode(LINE_LEFT, INPUT);
  pinMode(LINE_RIGHT, INPUT);
  
  // 初始化电机
  pinMode(M1_FWD, OUTPUT); pinMode(M1_REV, OUTPUT);
  pinMode(M2_FWD, OUTPUT); pinMode(M2_REV, OUTPUT);
  pinMode(M3_FWD, OUTPUT); pinMode(M3_REV, OUTPUT);
  pinMode(M4_FWD, OUTPUT); pinMode(M4_REV, OUTPUT);
  
  // 初始化按键
  pinMode(KEY_A, INPUT_PULLUP);
  pinMode(KEY_B, INPUT_PULLUP);
  
  // 设置 PWM 频率
  analogWriteFrequency(10000);
  
  carStop();
  Serial.println("等待启动...");
}

void loop() {
  bool currentKeyA = digitalRead(KEY_A);
  bool currentKeyB = digitalRead(KEY_B);
  
  // 按键A：切换运行状态
  if (lastKeyA == HIGH && currentKeyA == LOW) {
    isRunning = !isRunning;
    if (isRunning) {
      Serial.println(">>> 循迹启动！");
      tone(BUZZER_PIN, 1000, 100);
    } else {
      Serial.println(">>> 循迹停止");
      carStop();
      tone(BUZZER_PIN, 500, 200);
    }
    delay(200);
  }
  
  // 按键B：紧急停止
  if (lastKeyB == HIGH && currentKeyB == LOW) {
    isRunning = false;
    carStop();
    Serial.println("!!! 紧急停止 !!!");
    tone(BUZZER_PIN, 200, 500);
    delay(200);
  }
  
  lastKeyA = currentKeyA;
  lastKeyB = currentKeyB;
  
  // 循迹逻辑
  if (isRunning) {
    int left = digitalRead(LINE_LEFT);   // 0=检测到黑线, 1=白色
    int right = digitalRead(LINE_RIGHT);
    
    if (left == 1 && right == 1) {
      // 两边都是白色，直行
      carForward();
    } else if (left == 0 && right == 1) {
      // 左边检测到黑线，说明偏右了，需要左转
      carTurnLeft();
    } else if (left == 1 && right == 0) {
      // 右边检测到黑线，说明偏左了，需要右转
      carTurnRight();
    } else {
      // 两边都是黑线（可能是十字路口或终点）
      // 策略：继续前进或停止
      carForward();
    }
  }
  
  delay(10);  // 小延时，提高稳定性
}
