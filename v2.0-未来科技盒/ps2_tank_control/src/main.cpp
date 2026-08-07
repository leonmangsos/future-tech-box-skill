#include <Arduino.h>
#include "PS2X_lib.h"
#include "pins.h"

// ==================== PS2 手柄对象 ====================
PS2X ps2x;
int ps2Error = 1;

// ==================== 电机速度配置 ====================
#define PWM_MAX 9
int pwm_value[PWM_MAX] = {0, 150, 160, 170, 190, 210, 220, 230, 240};

// 8个电机 PWM 输出值
// motor_pwm[0]=M1正转(GPIO11), [1]=M1反转(GPIO12),
// motor_pwm[2]=M2反转(GPIO13), [3]=M2正转(GPIO14),
// motor_pwm[4]=M3正转(GPIO15), [5]=M3反转(GPIO16),
// motor_pwm[6]=M4反转(GPIO17), [7]=M4正转(GPIO18)
int motor_pwm[8] = {0, 0, 0, 0, 0, 0, 0, 0};

// 电机映射：[电机序号][正转索引, 反转索引]
// 前左M1(0,1), 前右M2(3,2), 后左M3(4,5), 后右M4(7,6)
int motor_pwm_num[4][2] = {{0,1}, {3,2}, {4,5}, {7,6}};

// PS2 定时器
hw_timer_t *time_ps2 = NULL;

// ==================== 电机全停 ====================
void carStop() {
  for (int i = 0; i < 8; i++) {
    motor_pwm[i] = 0;
  }
}

// ==================== 坦克式摇杆控制 ====================
// 左摇杆 Y 轴 → 左侧轮子 M1+M3
// 右摇杆 Y 轴 → 右侧轮子 M2+M4
void tank_control(int ly_value, int ry_value) {
  // --- 左侧轮子（M1=电机0, M3=电机2）由左摇杆 Y 控制 ---
  if (ly_value > PWM_MAX) {
    // 左摇杆下推 → 左侧轮子后退
    int spd = pwm_value[ly_value - PWM_MAX - 1];
    // M1 后退
    motor_pwm[motor_pwm_num[0][0]] = 0;
    motor_pwm[motor_pwm_num[0][1]] = spd;
    // M3 后退
    motor_pwm[motor_pwm_num[2][0]] = 0;
    motor_pwm[motor_pwm_num[2][1]] = spd;
  } else if (ly_value < PWM_MAX) {
    // 左摇杆上推 → 左侧轮子前进
    int spd = pwm_value[PWM_MAX - ly_value - 1];
    // M1 前进
    motor_pwm[motor_pwm_num[0][0]] = spd;
    motor_pwm[motor_pwm_num[0][1]] = 0;
    // M3 前进
    motor_pwm[motor_pwm_num[2][0]] = spd;
    motor_pwm[motor_pwm_num[2][1]] = 0;
  } else {
    // 左摇杆居中 → 左侧轮子停止
    motor_pwm[motor_pwm_num[0][0]] = 0;
    motor_pwm[motor_pwm_num[0][1]] = 0;
    motor_pwm[motor_pwm_num[2][0]] = 0;
    motor_pwm[motor_pwm_num[2][1]] = 0;
  }

  // --- 右侧轮子（M2=电机1, M4=电机3）由右摇杆 Y 控制 ---
  if (ry_value > PWM_MAX) {
    // 右摇杆下推 → 右侧轮子后退
    int spd = pwm_value[ry_value - PWM_MAX - 1];
    // M2 后退
    motor_pwm[motor_pwm_num[1][0]] = 0;
    motor_pwm[motor_pwm_num[1][1]] = spd;
    // M4 后退
    motor_pwm[motor_pwm_num[3][0]] = 0;
    motor_pwm[motor_pwm_num[3][1]] = spd;
  } else if (ry_value < PWM_MAX) {
    // 右摇杆上推 → 右侧轮子前进
    int spd = pwm_value[PWM_MAX - ry_value - 1];
    // M2 前进
    motor_pwm[motor_pwm_num[1][0]] = spd;
    motor_pwm[motor_pwm_num[1][1]] = 0;
    // M4 前进
    motor_pwm[motor_pwm_num[3][0]] = spd;
    motor_pwm[motor_pwm_num[3][1]] = 0;
  } else {
    // 右摇杆居中 → 右侧轮子停止
    motor_pwm[motor_pwm_num[1][0]] = 0;
    motor_pwm[motor_pwm_num[1][1]] = 0;
    motor_pwm[motor_pwm_num[3][0]] = 0;
    motor_pwm[motor_pwm_num[3][1]] = 0;
  }
}

// ==================== PS2 定时器中断回调 ====================
void IRAM_ATTR func_ps2() {
  ps2x.read_gamepad(false, 0);

  // 读取摇杆并映射到 0~18，中间值 9
  int ly_value = map(ps2x.Analog(PSS_LY), 0, 255, 0, PWM_MAX * 2);
  int ry_value = map(ps2x.Analog(PSS_RY), 0, 255, 0, PWM_MAX * 2);

  // 坦克式控制：左摇杆→左轮，右摇杆→右轮
  tank_control(ly_value, ry_value);
}

// ==================== setup ====================
void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("========================================");
  Serial.println("  PS2 坦克式双摇杆遥控");
  Serial.println("  左摇杆→左轮  右摇杆→右轮");
  Serial.println("  未来科技盒 2.0 (XIAO ESP32S3)");
  Serial.println("========================================");

  // 初始化电机引脚
  pinMode(M1_FWD, OUTPUT); pinMode(M1_REV, OUTPUT);
  pinMode(M2_FWD, OUTPUT); pinMode(M2_REV, OUTPUT);
  pinMode(M3_FWD, OUTPUT); pinMode(M3_REV, OUTPUT);
  pinMode(M4_FWD, OUTPUT); pinMode(M4_REV, OUTPUT);
  analogWriteFrequency(10000);
  carStop();

  // 初始化蜂鸣器
  pinMode(BUZZER_PIN, OUTPUT);

  // 初始化 PS2 手柄（重试 3 次）
  Serial.println("正在连接 PS2 手柄...");
  int tryNum = 0;
  while (ps2Error != 0 && tryNum < 3) {
    delay(1000);
    ps2Error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_CS, PS2_DAT, false, false);
    tryNum++;
    Serial.print("  尝试第 ");
    Serial.print(tryNum);
    Serial.print(" 次... ");
    if (ps2Error == 0) {
      Serial.println("成功!");
    } else {
      Serial.print("失败 (错误码: ");
      Serial.print(ps2Error);
      Serial.println(")");
    }
  }

  if (ps2Error == 0) {
    Serial.println("\nPS2 手柄连接成功！");
    Serial.println("------------------------------------");
    Serial.println("操控说明：");
    Serial.println("  左摇杆 上/下 = 左侧轮子 前进/后退");
    Serial.println("  右摇杆 上/下 = 右侧轮子 前进/后退");
    Serial.println("  双摇杆同推   = 前进/后退");
    Serial.println("  反向推       = 原地转向");
    Serial.println("------------------------------------");

    // 连接成功蜂鸣提示
    tone(BUZZER_PIN, 1000, 100);
    delay(150);
    tone(BUZZER_PIN, 1500, 100);

    // 启动硬件定时器中断，每 100ms 读取手柄
    time_ps2 = timerBegin(1, 80, true);
    timerAttachInterrupt(time_ps2, &func_ps2, true);
    timerAlarmWrite(time_ps2, 100000, true);
    timerAlarmEnable(time_ps2);
  } else {
    Serial.println("\nPS2 手柄连接失败！");
    Serial.println("请检查：");
    Serial.println("  1. 手柄接收器是否插好");
    Serial.println("  2. 手柄是否开机配对");
    Serial.println("  3. 接线: CLK=41, CMD=9, CS=42, DAT=10");
    tone(BUZZER_PIN, 500, 300);
  }
}

// ==================== loop ====================
void loop() {
  if (ps2Error != 0) {
    delay(1000);
    return;
  }

  // 将 motor_pwm 数组输出到实际电机引脚
  analogWrite(M1_FWD, motor_pwm[0]);  // GPIO11 M1正转
  analogWrite(M1_REV, motor_pwm[1]);  // GPIO12 M1反转
  analogWrite(M2_REV, motor_pwm[2]);  // GPIO13 M2反转
  analogWrite(M2_FWD, motor_pwm[3]);  // GPIO14 M2正转
  analogWrite(M3_FWD, motor_pwm[4]);  // GPIO15 M3正转
  analogWrite(M3_REV, motor_pwm[5]);  // GPIO16 M3反转
  analogWrite(M4_REV, motor_pwm[6]);  // GPIO17 M4反转
  analogWrite(M4_FWD, motor_pwm[7]);  // GPIO18 M4正转

  delay(100);
}
