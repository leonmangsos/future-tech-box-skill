/**
 * 麦克纳姆轮小车 - 左右横移演示
 * 
 * 功能：
 * - 默认模式：小车左右横移循环
 * - 按下按键 A：切换到全方向演示模式
 * 
 * 修复：
 * - 使用非阻塞式按键检测，随时响应按键
 * - 方向切换前先完全停止电机，避免堵转
 */

#include <Arduino.h>
#include "pins.h"

// ==================== 运动状态枚举 ====================
enum MoveState {
  STATE_STOP,
  STATE_LEFT,
  STATE_RIGHT,
  // 全方向演示状态
  STATE_FORWARD,
  STATE_BACKWARD,
  STATE_FRONT_LEFT,
  STATE_BACK_RIGHT,
  STATE_FRONT_RIGHT,
  STATE_BACK_LEFT,
  STATE_TURN_LEFT,
  STATE_TURN_RIGHT
};

// ==================== 全局变量 ====================
bool fullDemoMode = false;      // 是否全方向演示模式
bool lastKeyA = HIGH;           // 上一次按键状态
unsigned long lastKeyTime = 0;  // 防抖时间

MoveState currentState = STATE_STOP;
unsigned long stateStartTime = 0;
int demoStep = 0;               // 全方向演示步骤

const int MOVE_DURATION = 1200;  // 移动持续时间 (ms)
const int STOP_DURATION = 400;   // 停止持续时间 (ms)，增加让电机完全停止
const int MOTOR_SPEED = 180;  // 默认速度 70%（范围 0-255，避免触发电池保护）

// ==================== 电机控制函数 ====================

/**
 * 设置单个电机速度
 */
void setMotor(int fwdPin, int revPin, int speed) {
  speed = constrain(speed, -255, 255);
  if (speed > 0) {
    analogWrite(fwdPin, speed);
    analogWrite(revPin, 0);
  } else if (speed < 0) {
    analogWrite(fwdPin, 0);
    analogWrite(revPin, -speed);
  } else {
    // 完全停止：两个引脚都输出 0
    analogWrite(fwdPin, 0);
    analogWrite(revPin, 0);
  }
}

/**
 * 设置四个电机独立速度（麦克纳姆轮全向控制）
 */
void setMotorsSeparate(int m1Speed, int m2Speed, int m3Speed, int m4Speed) {
  setMotor(M1_FWD, M1_REV, m1Speed);
  setMotor(M2_FWD, M2_REV, m2Speed);
  setMotor(M3_FWD, M3_REV, m3Speed);
  setMotor(M4_FWD, M4_REV, m4Speed);
}

/**
 * 完全停止所有电机
 * 先制动再释放，确保电机完全停止
 */
void carStop() {
  setMotorsSeparate(0, 0, 0, 0);
}

// ==================== 麦克纳姆轮运动函数 ====================

void carForward(int speed) {
  setMotorsSeparate(speed, speed, speed, speed);
}

void carBackward(int speed) {
  setMotorsSeparate(-speed, -speed, -speed, -speed);
}

void carTurnLeft(int speed) {
  setMotorsSeparate(-speed, speed, -speed, speed);
}

void carTurnRight(int speed) {
  setMotorsSeparate(speed, -speed, speed, -speed);
}

// ★ 左横移（麦克纳姆轮特有）
// M1反转, M2正转, M3正转, M4反转
void carMoveLeft(int speed) {
  setMotorsSeparate(-speed, speed, speed, -speed);
}

// ★ 右横移（麦克纳姆轮特有）
// M1正转, M2反转, M3反转, M4正转
void carMoveRight(int speed) {
  setMotorsSeparate(speed, -speed, -speed, speed);
}

// 左前楔形
void carMoveFrontLeft(int speed) {
  setMotorsSeparate(0, speed, speed, 0);
}

// 右前楔形
void carMoveFrontRight(int speed) {
  setMotorsSeparate(speed, 0, 0, speed);
}

// 左后楔形
void carMoveBackLeft(int speed) {
  setMotorsSeparate(-speed, 0, 0, -speed);
}

// 右后楔形
void carMoveBackRight(int speed) {
  setMotorsSeparate(0, -speed, -speed, 0);
}

// ==================== 状态执行函数 ====================

void executeState(MoveState state) {
  switch (state) {
    case STATE_STOP:
      carStop();
      break;
    case STATE_LEFT:
      Serial.println("← 左横移");
      carMoveLeft(MOTOR_SPEED);
      break;
    case STATE_RIGHT:
      Serial.println("→ 右横移");
      carMoveRight(MOTOR_SPEED);
      break;
    case STATE_FORWARD:
      Serial.println("↑ 前进");
      carForward(MOTOR_SPEED);
      break;
    case STATE_BACKWARD:
      Serial.println("↓ 后退");
      carBackward(MOTOR_SPEED);
      break;
    case STATE_FRONT_LEFT:
      Serial.println("↖ 左前楔形");
      carMoveFrontLeft(MOTOR_SPEED);
      break;
    case STATE_BACK_RIGHT:
      Serial.println("↘ 右后楔形");
      carMoveBackRight(MOTOR_SPEED);
      break;
    case STATE_FRONT_RIGHT:
      Serial.println("↗ 右前楔形");
      carMoveFrontRight(MOTOR_SPEED);
      break;
    case STATE_BACK_LEFT:
      Serial.println("↙ 左后楔形");
      carMoveBackLeft(MOTOR_SPEED);
      break;
    case STATE_TURN_LEFT:
      Serial.println("↺ 原地左转");
      carTurnLeft(MOTOR_SPEED);
      break;
    case STATE_TURN_RIGHT:
      Serial.println("↻ 原地右转");
      carTurnRight(MOTOR_SPEED);
      break;
  }
}

// ==================== 切换状态（带安全停止） ====================

void changeState(MoveState newState) {
  // 如果新状态和当前状态相同，不做任何事
  if (newState == currentState) return;
  
  // 先停止当前运动
  carStop();
  delay(50);  // 短暂延时让电机完全停止
  
  // 切换到新状态
  currentState = newState;
  stateStartTime = millis();
  
  // 执行新状态
  executeState(newState);
}

// ==================== 初始化 ====================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=====================================");
  Serial.println("  麦克纳姆轮小车 - 左右横移演示");
  Serial.println("=====================================");
  Serial.println();
  
  // 初始化电机引脚
  pinMode(M1_FWD, OUTPUT); pinMode(M1_REV, OUTPUT);
  pinMode(M2_FWD, OUTPUT); pinMode(M2_REV, OUTPUT);
  pinMode(M3_FWD, OUTPUT); pinMode(M3_REV, OUTPUT);
  pinMode(M4_FWD, OUTPUT); pinMode(M4_REV, OUTPUT);
  
  // 确保所有电机初始停止
  carStop();
  
  // 设置 PWM 频率（10kHz，减少电机噪音）
  analogWriteFrequency(10000);
  
  // 初始化按键
  pinMode(KEY_A, INPUT_PULLUP);
  pinMode(KEY_B, INPUT_PULLUP);
  
  Serial.println("初始化完成！");
  Serial.println();
  Serial.println("当前模式: 左右横移循环");
  Serial.println("★ 随时按下按键 A 可切换到全方向演示模式 ★");
  Serial.println();
  
  delay(500);
  
  // 开始左横移
  currentState = STATE_LEFT;
  stateStartTime = millis();
  executeState(currentState);
}

// ==================== 按键检测（非阻塞） ====================

void checkButton() {
  bool currentKeyA = digitalRead(KEY_A);
  unsigned long now = millis();
  
  // 防抖：200ms 内不重复检测
  if (now - lastKeyTime < 200) {
    lastKeyA = currentKeyA;
    return;
  }
  
  // 检测下降沿（按下瞬间）
  if (lastKeyA == HIGH && currentKeyA == LOW) {
    lastKeyTime = now;
    
    // 切换模式
    fullDemoMode = !fullDemoMode;
    demoStep = 0;
    
    // 停止电机
    carStop();
    
    if (fullDemoMode) {
      Serial.println();
      Serial.println("========================================");
      Serial.println(">>>>>> 切换到: 全方向演示模式 <<<<<<");
      Serial.println("========================================");
      Serial.println();
      
      // 从前进开始
      currentState = STATE_STOP;
      stateStartTime = millis();
    } else {
      Serial.println();
      Serial.println("========================================");
      Serial.println(">>>>>> 切换到: 左右横移模式 <<<<<<");
      Serial.println("========================================");
      Serial.println();
      
      // 从左横移开始
      currentState = STATE_LEFT;
      stateStartTime = millis();
      executeState(currentState);
    }
  }
  
  lastKeyA = currentKeyA;
}

// ==================== 左右横移模式状态机 ====================

void runLateralMode() {
  unsigned long elapsed = millis() - stateStartTime;
  
  switch (currentState) {
    case STATE_LEFT:
      if (elapsed >= MOVE_DURATION) {
        changeState(STATE_STOP);
      }
      break;
      
    case STATE_STOP:
      if (elapsed >= STOP_DURATION) {
        // 判断下一个状态
        static bool goRight = true;
        if (goRight) {
          changeState(STATE_RIGHT);
        } else {
          changeState(STATE_LEFT);
        }
        goRight = !goRight;
      }
      break;
      
    case STATE_RIGHT:
      if (elapsed >= MOVE_DURATION) {
        changeState(STATE_STOP);
      }
      break;
      
    default:
      // 如果意外进入其他状态，重置为左横移
      changeState(STATE_LEFT);
      break;
  }
}

// ==================== 全方向演示模式状态机 ====================

// 演示序列
const MoveState demoSequence[] = {
  STATE_FORWARD,     // 0: 前进
  STATE_STOP,        // 1: 停
  STATE_BACKWARD,    // 2: 后退
  STATE_STOP,        // 3: 停
  STATE_LEFT,        // 4: 左横移
  STATE_STOP,        // 5: 停
  STATE_RIGHT,       // 6: 右横移
  STATE_STOP,        // 7: 停
  STATE_FRONT_LEFT,  // 8: 左前楔形
  STATE_STOP,        // 9: 停
  STATE_BACK_RIGHT,  // 10: 右后楔形（返回）
  STATE_STOP,        // 11: 停
  STATE_FRONT_RIGHT, // 12: 右前楔形
  STATE_STOP,        // 13: 停
  STATE_BACK_LEFT,   // 14: 左后楔形（返回）
  STATE_STOP,        // 15: 停
  STATE_TURN_LEFT,   // 16: 原地左转
  STATE_STOP,        // 17: 停
  STATE_TURN_RIGHT,  // 18: 原地右转
  STATE_STOP         // 19: 停
};
const int demoStepCount = sizeof(demoSequence) / sizeof(demoSequence[0]);

void runFullDemoMode() {
  unsigned long elapsed = millis() - stateStartTime;
  
  // 判断当前步骤应持续的时间
  int duration = (currentState == STATE_STOP) ? STOP_DURATION : MOVE_DURATION;
  
  if (elapsed >= duration) {
    // 进入下一步
    demoStep++;
    if (demoStep >= demoStepCount) {
      demoStep = 0;
      Serial.println();
      Serial.println("--- 全方向演示完成，重新开始 ---");
      Serial.println();
    }
    
    // 切换到下一个状态
    changeState(demoSequence[demoStep]);
  }
}

// ==================== 主循环 ====================

void loop() {
  // 1. 检测按键（随时响应）
  checkButton();
  
  // 2. 根据模式运行状态机
  if (!fullDemoMode) {
    runLateralMode();
  } else {
    runFullDemoMode();
  }
  
  // 短暂延时，防止 loop 过快
  delay(10);
}
