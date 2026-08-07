/*
 * 未来科技盒 3.0 - PS2手柄控制云台
 * 
 * 云台接线标准（已校准）：
 *   S1 (GPIO48) = 云台底部舵机（水平旋转）
 *   S2 (GPIO47) = 云台顶部舵机（俯仰）
 * 
 * S1 底部: 居中143°, 最小14°, 最大270°
 * S2 顶部: 居中270°, 最小180°, 最大360°
 * 
 * 操作：
 *   左摇杆 左/右 → S1 水平旋转
 *   右摇杆 上/下 → S2 俯仰
 *   十字键 左/右 → S1 精确±3°
 *   十字键 上/下 → S2 精确±3°
 *   L1/R1       → 云台回中
 */

#include <Arduino.h>
#include <ESP32Servo.h>
#include "PS2X_lib.h"
#include "pins.h"

PS2X ps2x;
Servo servo1;  // S1 底部
Servo servo2;  // S2 顶部
int ps2Error = 1;

// ==================== 云台参数（实测校准）====================
// S1 底部水平: GPIO48
#define S1_CENTER 143
#define S1_MIN    14
#define S1_MAX    270

// S2 顶部俯仰: GPIO47
#define S2_CENTER 270
#define S2_MIN    180
#define S2_MAX    360

int s1Angle = S1_CENTER;
int s2Angle = S2_CENTER;

const int DEAD_ZONE = 30;

// 角度→脉宽映射
// 这些舵机行程超过180°，使用 writeMicroseconds
// 假设 500μs=0°, 2500μs=360°（全量程线性）
// 可微调这两个值来校准
#define PWM_MIN_US  500
#define PWM_MAX_US  2500
#define ANGLE_FULL  360   // 脉宽全量程对应的角度

int angleToUs(int angle) {
  return map(angle, 0, ANGLE_FULL, PWM_MIN_US, PWM_MAX_US);
}

// 空闲保护
unsigned long lastMoveTime = 0;
const unsigned long IDLE_TIMEOUT = 3000;
bool servosAttached = true;

// PS2 定时器
hw_timer_t *time_ps2 = NULL;
volatile int leftX = 128;
volatile int rightY = 128;
volatile bool btnL1 = false;
volatile bool btnR1 = false;
volatile bool btnUp = false;
volatile bool btnDown = false;
volatile bool btnLeft = false;
volatile bool btnRight = false;

void IRAM_ATTR func_ps2() {
  ps2x.read_gamepad(false, 0);
  leftX = ps2x.Analog(PSS_LX);
  rightY = ps2x.Analog(PSS_RY);
  btnL1 = ps2x.Button(PSB_L1);
  btnR1 = ps2x.Button(PSB_R1);
  btnUp = ps2x.ButtonPressed(PSB_PAD_UP);
  btnDown = ps2x.ButtonPressed(PSB_PAD_DOWN);
  btnLeft = ps2x.ButtonPressed(PSB_PAD_LEFT);
  btnRight = ps2x.ButtonPressed(PSB_PAD_RIGHT);
}

void ensureAttached() {
  if (!servosAttached) {
    servo1.attach(SERVO1_PIN, PWM_MIN_US, PWM_MAX_US);
    servo2.attach(SERVO2_PIN, PWM_MIN_US, PWM_MAX_US);
    servosAttached = true;
  }
}

void writeServos() {
  servo1.writeMicroseconds(angleToUs(s1Angle));
  servo2.writeMicroseconds(angleToUs(s2Angle));
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("==========================================");
  Serial.println("  PS2 云台控制（校准版）");
  Serial.println("  S1(GPIO48 底部): 中143°, 14°~270°");
  Serial.println("  S2(GPIO47 顶部): 中270°, 180°~360°");
  Serial.println("==========================================");
  
  servo1.attach(SERVO1_PIN, PWM_MIN_US, PWM_MAX_US);
  servo2.attach(SERVO2_PIN, PWM_MIN_US, PWM_MAX_US);
  writeServos();
  lastMoveTime = millis();
  Serial.printf("初始: S1=%d° (%dμs), S2=%d° (%dμs)\n", 
                s1Angle, angleToUs(s1Angle), s2Angle, angleToUs(s2Angle));
  
  // 连接 PS2
  Serial.println("\n连接手柄...");
  int tryNum = 0;
  while (ps2Error != 0 && tryNum < 5) {
    delay(1000);
    ps2Error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_CS, PS2_DAT, false, false);
    tryNum++;
    if (ps2Error == 0) Serial.println("✅ 手柄连接成功！");
    else Serial.printf("  第%d次失败(%d)\n", tryNum, ps2Error);
  }
  
  if (ps2Error == 0) {
    Serial.println("--------------------------------------------");
    Serial.println("  左摇杆 左/右 → S1 水平");
    Serial.println("  右摇杆 上/下 → S2 俯仰");
    Serial.println("  十字键       → 精确±3°");
    Serial.println("  L1/R1       → 回中");
    Serial.println("--------------------------------------------");
    
    time_ps2 = timerBegin(1, 80, true);
    timerAttachInterrupt(time_ps2, &func_ps2, true);
    timerAlarmWrite(time_ps2, 50000, true);
    timerAlarmEnable(time_ps2);
  } else {
    Serial.println("❌ 手柄连接失败！");
  }
}

void loop() {
  if (ps2Error != 0) { delay(1000); return; }
  
  bool updated = false;
  
  // --- 左摇杆 X轴 → S1 水平 ---
  int lx = leftX;
  if (lx < (128 - DEAD_ZONE)) {
    int speed = map(lx, 128 - DEAD_ZONE, 0, 0, 3);
    s1Angle -= speed;
    updated = true;
  } else if (lx > (128 + DEAD_ZONE)) {
    int speed = map(lx, 128 + DEAD_ZONE, 255, 0, 3);
    s1Angle += speed;
    updated = true;
  }
  
  // --- 右摇杆 Y轴 → S2 俯仰 ---
  int ry = rightY;
  if (ry < (128 - DEAD_ZONE)) {
    int speed = map(ry, 128 - DEAD_ZONE, 0, 0, 3);
    s2Angle += speed;
    updated = true;
  } else if (ry > (128 + DEAD_ZONE)) {
    int speed = map(ry, 128 + DEAD_ZONE, 255, 0, 3);
    s2Angle -= speed;
    updated = true;
  }
  
  // --- 十字键 ---
  if (btnLeft)  { s1Angle -= 3; updated = true; }
  if (btnRight) { s1Angle += 3; updated = true; }
  if (btnUp)    { s2Angle += 3; updated = true; }
  if (btnDown)  { s2Angle -= 3; updated = true; }
  
  // --- L1/R1 回中 ---
  if (btnL1 || btnR1) {
    s1Angle = S1_CENTER;
    s2Angle = S2_CENTER;
    updated = true;
    Serial.printf(">>> 回中: S1=%d°, S2=%d°\n", S1_CENTER, S2_CENTER);
  }
  
  // --- 限位 ---
  s1Angle = constrain(s1Angle, S1_MIN, S1_MAX);
  s2Angle = constrain(s2Angle, S2_MIN, S2_MAX);
  
  // --- 写入舵机 ---
  if (updated) {
    ensureAttached();
    writeServos();
    lastMoveTime = millis();
  }
  
  // --- 空闲保护 ---
  if (servosAttached && (millis() - lastMoveTime > IDLE_TIMEOUT)) {
    servo1.detach();
    servo2.detach();
    servosAttached = false;
    Serial.printf("⚡ 空闲分离（S1=%d°, S2=%d°）\n", s1Angle, s2Angle);
  }
  
  // --- 打印 ---
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 500 && updated) {
    Serial.printf("S1: %3d°  |  S2: %3d°\n", s1Angle, s2Angle);
    lastPrint = millis();
  }
  
  delay(30);
}
