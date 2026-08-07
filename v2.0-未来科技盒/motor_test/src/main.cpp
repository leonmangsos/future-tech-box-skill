/**
 * 未来科技盒 2.0 - 电机测试程序
 * 
 * 功能：测试小车的基本运动
 * - 按键A：依次执行前进、后退、左转、右转、停止
 * - 按键B：停止所有电机
 */

#include <Arduino.h>
#include "pins.h"

// 当前测试步骤
int testStep = 0;

// 按键状态
bool lastKeyA = HIGH;
bool lastKeyB = HIGH;

// 设置单个电机速度
// speed > 0: 正转, speed < 0: 反转, speed = 0: 停止
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
  Serial.println("停止");
}

void carForward(int speed = 180) {
  setMotors(speed, speed);
  Serial.print("前进 速度: ");
  Serial.println(speed);
}

void carBackward(int speed = 180) {
  setMotors(-speed, -speed);
  Serial.print("后退 速度: ");
  Serial.println(speed);
}

void carTurnLeft(int speed = 150) {
  setMotors(-speed, speed);  // 原地左转
  Serial.print("左转 速度: ");
  Serial.println(speed);
}

void carTurnRight(int speed = 150) {
  setMotors(speed, -speed);  // 原地右转
  Serial.print("右转 速度: ");
  Serial.println(speed);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n========================================");
  Serial.println("未来科技盒 2.0 - 电机测试程序");
  Serial.println("========================================");
  Serial.println("按键A: 依次测试各种运动");
  Serial.println("按键B: 紧急停止");
  Serial.println("========================================\n");
  
  // 初始化电机引脚
  pinMode(M1_FWD, OUTPUT); pinMode(M1_REV, OUTPUT);
  pinMode(M2_FWD, OUTPUT); pinMode(M2_REV, OUTPUT);
  pinMode(M3_FWD, OUTPUT); pinMode(M3_REV, OUTPUT);
  pinMode(M4_FWD, OUTPUT); pinMode(M4_REV, OUTPUT);
  
  // 初始化按键
  pinMode(KEY_A, INPUT_PULLUP);
  pinMode(KEY_B, INPUT_PULLUP);
  
  // 设置 PWM 频率
  analogWriteFrequency(10000);
  
  // 初始停止
  carStop();
}

void loop() {
  bool currentKeyA = digitalRead(KEY_A);
  bool currentKeyB = digitalRead(KEY_B);
  
  // 按键A：切换测试步骤
  if (lastKeyA == HIGH && currentKeyA == LOW) {
    testStep++;
    if (testStep > 5) testStep = 0;
    
    Serial.print("\n>>> 测试步骤 ");
    Serial.print(testStep);
    Serial.print(": ");
    
    switch (testStep) {
      case 0:
        carStop();
        break;
      case 1:
        carForward(180);
        break;
      case 2:
        carBackward(180);
        break;
      case 3:
        carTurnLeft(150);
        break;
      case 4:
        carTurnRight(150);
        break;
      case 5:
        carStop();
        Serial.println("测试完成！按A键重新开始");
        testStep = -1;  // 下次按会变成0
        break;
    }
    
    delay(200);  // 简单消抖
  }
  
  // 按键B：紧急停止
  if (lastKeyB == HIGH && currentKeyB == LOW) {
    testStep = 0;
    carStop();
    Serial.println("\n!!! 紧急停止 !!!");
    delay(200);
  }
  
  lastKeyA = currentKeyA;
  lastKeyB = currentKeyB;
}
