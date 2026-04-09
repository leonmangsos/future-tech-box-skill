#include <Arduino.h>
#include "PS2X_lib.h"
#include "pins.h"

// ==================== PS2 手柄对象 ====================
PS2X ps2x;
int ps2Error = 1;

// ==================== 电机速度配置 ====================
// 参照参考代码的 PWM 速度等级表 (0~8 共9档)
#define PWM_MAX 9
int pwm_value[PWM_MAX] = {0, 150, 160, 170, 190, 210, 220, 230, 240};

// 8个电机 PWM 输出值（每个电机2个引脚：正转+反转）
// motor_pwm[0]=M1正转, [1]=M1反转, [2]=M2反转, [3]=M2正转,
// motor_pwm[4]=M3正转, [5]=M3反转, [6]=M4反转, [7]=M4正转
int motor_pwm[8] = {0, 0, 0, 0, 0, 0, 0, 0};

// 电机引脚编号映射：[电机序号][0=正转引脚索引, 1=反转引脚索引]
// 参照参考代码：前左M1(0,1), 前右M2(3,2), 后左M3(4,5), 后右M4(7,6)
int motor_pwm_num[4][2] = {{0,1}, {3,2}, {4,5}, {7,6}};

// 摇杆死区
const int DEADZONE = 4;

// PS2 定时器
hw_timer_t *time_ps2 = NULL;

// 蜂鸣器提示音
void beepOnce(int freq = 1000, int duration = 100) {
  tone(BUZZER_PIN, freq, duration);
}

// ==================== 电机全停 ====================
void carStop() {
  for (int i = 0; i < 8; i++) {
    motor_pwm[i] = 0;
  }
}

// ==================== 左摇杆控制（前后+差速转向）====================
void motor_change(int lx_value, int ly_value) {
  for (int i = 0; i < 4; i++) {
    int pwm_num0 = motor_pwm_num[i][0];
    int pwm_num1 = motor_pwm_num[i][1];

    if (ly_value > PWM_MAX + 1) {
      // 左摇杆往下推 → 后退方向
      motor_pwm[pwm_num0] = 0;
      motor_pwm[pwm_num1] = pwm_value[ly_value - PWM_MAX - 1];
      // X轴差速转向
      if (lx_value > PWM_MAX) {
        if (i % 2)  // 右侧电机减速
          motor_pwm[pwm_num1] = motor_pwm[pwm_num1] - (lx_value - PWM_MAX - 1) * 4;
      } else if (lx_value < PWM_MAX) {
        if (!(i % 2))  // 左侧电机减速
          motor_pwm[pwm_num1] = motor_pwm[pwm_num1] - (PWM_MAX - lx_value) * 4;
      }
    } else if (ly_value < PWM_MAX) {
      // 左摇杆往上推 → 前进方向
      motor_pwm[pwm_num0] = 0;
      motor_pwm[pwm_num1] = 0;

      if (lx_value > PWM_MAX) {
        // 右转（以前轮为轴）
        if (i == 2)
          motor_pwm[pwm_num1] = pwm_value[lx_value - PWM_MAX - 1];
        else if (i == 3)
          motor_pwm[pwm_num0] = pwm_value[lx_value - PWM_MAX - 1];
      } else if (lx_value < PWM_MAX) {
        // 左转（以前轮为轴）
        if (i == 2)
          motor_pwm[pwm_num0] = pwm_value[PWM_MAX - lx_value - 2];
        else if (i == 3)
          motor_pwm[pwm_num1] = pwm_value[PWM_MAX - lx_value - 2];
      } else {
        // 纯前进
        motor_pwm[pwm_num0] = pwm_value[PWM_MAX - ly_value - 1];
        motor_pwm[pwm_num1] = 0;
        // X轴差速转向
        if (lx_value > PWM_MAX) {
          if (i % 2)
            motor_pwm[pwm_num0] = motor_pwm[pwm_num0] - (lx_value - PWM_MAX - 1) * 4;
        } else if (lx_value < PWM_MAX) {
          if (!(i % 2))
            motor_pwm[pwm_num0] = motor_pwm[pwm_num0] - (PWM_MAX - lx_value) * 4;
        }
      }
    } else {
      // Y在中间，不动
      motor_pwm[pwm_num0] = 0;
      motor_pwm[pwm_num1] = 0;
    }
  }
}

// ==================== 右摇杆控制（原地旋转+横移）====================
void motor_change1(int rx_value, int ry_value) {
  for (int i = 0; i < 4; i++) {
    int pwm_num0 = motor_pwm_num[i][0];
    int pwm_num1 = motor_pwm_num[i][1];

    if (ry_value > PWM_MAX + 1) {
      // 右摇杆下推 → 原地顺时针旋转
      if (i % 2) {
        motor_pwm[pwm_num1] = pwm_value[ry_value - PWM_MAX - 1];
        motor_pwm[pwm_num0] = 0;
      } else {
        motor_pwm[pwm_num0] = pwm_value[ry_value - PWM_MAX - 1];
        motor_pwm[pwm_num1] = 0;
      }
    } else if (ry_value < PWM_MAX - 1) {
      // 右摇杆上推 → 原地逆时针旋转
      if (i % 2) {
        motor_pwm[pwm_num0] = pwm_value[PWM_MAX - ry_value - 1];
        motor_pwm[pwm_num1] = 0;
      } else {
        motor_pwm[pwm_num1] = pwm_value[PWM_MAX - ry_value - 1];
        motor_pwm[pwm_num0] = 0;
      }
    } else {
      // RY在中间 → 检查 RX（左右横移）
      if (rx_value > PWM_MAX) {
        // 右横移
        if (i == 0 || i == 3) {
          motor_pwm[pwm_num0] = pwm_value[rx_value - PWM_MAX - 1];
          motor_pwm[pwm_num1] = 0;
        } else if (i == 1 || i == 2) {
          motor_pwm[pwm_num0] = 0;
          motor_pwm[pwm_num1] = pwm_value[rx_value - PWM_MAX - 1];
        }
      } else if (rx_value < PWM_MAX) {
        // 左横移
        if (i == 0 || i == 3) {
          motor_pwm[pwm_num1] = pwm_value[PWM_MAX - rx_value - 1];
          motor_pwm[pwm_num0] = 0;
        } else if (i == 1 || i == 2) {
          motor_pwm[pwm_num1] = 0;
          motor_pwm[pwm_num0] = pwm_value[PWM_MAX - rx_value - 1];
        }
      } else {
        motor_pwm[pwm_num0] = 0;
        motor_pwm[pwm_num1] = 0;
      }
    }
  }
}

// ==================== PS2 定时器中断回调 ====================
void IRAM_ATTR func_ps2() {
  byte vibrate = 0;
  ps2x.read_gamepad(false, vibrate);

  // 读取摇杆原始值
  int ly_value = ps2x.Analog(PSS_LY);
  int lx_value = ps2x.Analog(PSS_LX);
  int ry_value = ps2x.Analog(PSS_RY);
  int rx_value = ps2x.Analog(PSS_RX);

  // 映射到 0 ~ PWM_MAX*2 (即 0~18)，中间值为 PWM_MAX(9)
  ly_value = map(ly_value, 0, 255, 0, PWM_MAX * 2);
  lx_value = map(lx_value, 0, 255, 0, PWM_MAX * 2);
  ry_value = map(ry_value, 0, 255, 0, PWM_MAX * 2);
  rx_value = map(rx_value, 0, 255, 0, PWM_MAX * 2);

  // ==================== 按键功能 ====================

  // START 键：急停
  if (ps2x.ButtonPressed(PSB_START)) {
    carStop();
    return;  // 立即停止，不处理摇杆
  }

  // SELECT 键：（保留，可自定义）

  // △ 三角键：全速前进（按住生效）
  if (ps2x.Button(PSB_GREEN)) {
    // 四轮全速前进
    for (int i = 0; i < 4; i++) {
      int pwm_num0 = motor_pwm_num[i][0];
      int pwm_num1 = motor_pwm_num[i][1];
      motor_pwm[pwm_num0] = pwm_value[PWM_MAX - 1];
      motor_pwm[pwm_num1] = 0;
    }
    return;
  }

  // × 叉键：全速后退（按住生效）
  if (ps2x.Button(PSB_BLUE)) {
    for (int i = 0; i < 4; i++) {
      int pwm_num0 = motor_pwm_num[i][0];
      int pwm_num1 = motor_pwm_num[i][1];
      motor_pwm[pwm_num0] = 0;
      motor_pwm[pwm_num1] = pwm_value[PWM_MAX - 1];
    }
    return;
  }

  // □ 方块键：左横移（按住生效）
  if (ps2x.Button(PSB_PINK)) {
    int spd = pwm_value[6];  // 中等速度横移
    // 麦克纳姆轮左横移：M1反转,M2正转,M3正转,M4反转
    // 即：前左反、前右正、后左正、后右反
    motor_pwm[motor_pwm_num[0][0]] = 0;
    motor_pwm[motor_pwm_num[0][1]] = spd;
    motor_pwm[motor_pwm_num[1][0]] = spd;
    motor_pwm[motor_pwm_num[1][1]] = 0;
    motor_pwm[motor_pwm_num[2][0]] = spd;
    motor_pwm[motor_pwm_num[2][1]] = 0;
    motor_pwm[motor_pwm_num[3][0]] = 0;
    motor_pwm[motor_pwm_num[3][1]] = spd;
    return;
  }

  // ○ 圆圈键：右横移（按住生效）
  if (ps2x.Button(PSB_RED)) {
    int spd = pwm_value[6];
    // 麦克纳姆轮右横移：M1正转,M2反转,M3反转,M4正转
    motor_pwm[motor_pwm_num[0][0]] = spd;
    motor_pwm[motor_pwm_num[0][1]] = 0;
    motor_pwm[motor_pwm_num[1][0]] = 0;
    motor_pwm[motor_pwm_num[1][1]] = spd;
    motor_pwm[motor_pwm_num[2][0]] = 0;
    motor_pwm[motor_pwm_num[2][1]] = spd;
    motor_pwm[motor_pwm_num[3][0]] = spd;
    motor_pwm[motor_pwm_num[3][1]] = 0;
    return;
  }

  // L1 键：原地左旋（按住生效）
  if (ps2x.Button(PSB_L1)) {
    int spd = pwm_value[5];
    // 原地左旋：左侧反转，右侧正转
    for (int i = 0; i < 4; i++) {
      int pwm_num0 = motor_pwm_num[i][0];
      int pwm_num1 = motor_pwm_num[i][1];
      if (i % 2) {
        // 右侧电机：正转
        motor_pwm[pwm_num0] = spd;
        motor_pwm[pwm_num1] = 0;
      } else {
        // 左侧电机：反转
        motor_pwm[pwm_num0] = 0;
        motor_pwm[pwm_num1] = spd;
      }
    }
    return;
  }

  // R1 键：原地右旋（按住生效）
  if (ps2x.Button(PSB_R1)) {
    int spd = pwm_value[5];
    for (int i = 0; i < 4; i++) {
      int pwm_num0 = motor_pwm_num[i][0];
      int pwm_num1 = motor_pwm_num[i][1];
      if (i % 2) {
        // 右侧电机：反转
        motor_pwm[pwm_num0] = 0;
        motor_pwm[pwm_num1] = spd;
      } else {
        // 左侧电机：正转
        motor_pwm[pwm_num0] = spd;
        motor_pwm[pwm_num1] = 0;
      }
    }
    return;
  }

  // L2 键：左前斜移（按住生效）
  if (ps2x.Button(PSB_L2)) {
    int spd = pwm_value[5];
    // 麦克纳姆轮左前：前右和后左转，前左和后右不动
    motor_pwm[motor_pwm_num[0][0]] = 0;
    motor_pwm[motor_pwm_num[0][1]] = 0;
    motor_pwm[motor_pwm_num[1][0]] = spd;
    motor_pwm[motor_pwm_num[1][1]] = 0;
    motor_pwm[motor_pwm_num[2][0]] = spd;
    motor_pwm[motor_pwm_num[2][1]] = 0;
    motor_pwm[motor_pwm_num[3][0]] = 0;
    motor_pwm[motor_pwm_num[3][1]] = 0;
    return;
  }

  // R2 键：右前斜移（按住生效）
  if (ps2x.Button(PSB_R2)) {
    int spd = pwm_value[5];
    // 麦克纳姆轮右前：前左和后右转，前右和后左不动
    motor_pwm[motor_pwm_num[0][0]] = spd;
    motor_pwm[motor_pwm_num[0][1]] = 0;
    motor_pwm[motor_pwm_num[1][0]] = 0;
    motor_pwm[motor_pwm_num[1][1]] = 0;
    motor_pwm[motor_pwm_num[2][0]] = 0;
    motor_pwm[motor_pwm_num[2][1]] = 0;
    motor_pwm[motor_pwm_num[3][0]] = spd;
    motor_pwm[motor_pwm_num[3][1]] = 0;
    return;
  }

  // 方向十字键
  if (ps2x.Button(PSB_PAD_UP)) {
    int spd = pwm_value[5];
    for (int i = 0; i < 4; i++) {
      motor_pwm[motor_pwm_num[i][0]] = spd;
      motor_pwm[motor_pwm_num[i][1]] = 0;
    }
    return;
  }
  if (ps2x.Button(PSB_PAD_DOWN)) {
    int spd = pwm_value[5];
    for (int i = 0; i < 4; i++) {
      motor_pwm[motor_pwm_num[i][0]] = 0;
      motor_pwm[motor_pwm_num[i][1]] = spd;
    }
    return;
  }
  if (ps2x.Button(PSB_PAD_LEFT)) {
    // 原地左转
    int spd = pwm_value[4];
    for (int i = 0; i < 4; i++) {
      int pwm_num0 = motor_pwm_num[i][0];
      int pwm_num1 = motor_pwm_num[i][1];
      if (i % 2) {
        motor_pwm[pwm_num0] = spd;
        motor_pwm[pwm_num1] = 0;
      } else {
        motor_pwm[pwm_num0] = 0;
        motor_pwm[pwm_num1] = spd;
      }
    }
    return;
  }
  if (ps2x.Button(PSB_PAD_RIGHT)) {
    // 原地右转
    int spd = pwm_value[4];
    for (int i = 0; i < 4; i++) {
      int pwm_num0 = motor_pwm_num[i][0];
      int pwm_num1 = motor_pwm_num[i][1];
      if (i % 2) {
        motor_pwm[pwm_num0] = 0;
        motor_pwm[pwm_num1] = spd;
      } else {
        motor_pwm[pwm_num0] = spd;
        motor_pwm[pwm_num1] = 0;
      }
    }
    return;
  }

  // ==================== 摇杆控制 ====================
  // 如果所有摇杆都在死区内 → 停车
  if (abs(ly_value - PWM_MAX) <= DEADZONE && abs(lx_value - PWM_MAX) <= DEADZONE &&
      abs(ry_value - PWM_MAX) <= DEADZONE && abs(rx_value - PWM_MAX) <= DEADZONE) {
    carStop();
  } else {
    // 右摇杆不在中间位置 → 右摇杆优先（原地旋转/横移）
    if ((ry_value >= PWM_MAX - 1) && (ry_value <= PWM_MAX + 1) &&
        (rx_value >= PWM_MAX - 1) && (rx_value <= PWM_MAX + 1)) {
      // 右摇杆在中间 → 使用左摇杆（前后+差速转向）
      motor_change(lx_value, ly_value);
    } else {
      // 右摇杆活跃 → 使用右摇杆（旋转/横移）
      motor_change1(rx_value, ry_value);
    }
  }
}

// ==================== setup ====================
void setup() {
  Serial.begin(115200);
  delay(2000);  // USB-Serial/JTAG 枚举时间

  Serial.println("========================================");
  Serial.println("  PS2 手柄遥控小车 v2");
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
    byte type = ps2x.readType();
    switch (type) {
      case 0: Serial.println("手柄类型: 未知"); break;
      case 1: Serial.println("手柄类型: DualShock"); break;
      case 2: Serial.println("手柄类型: GuitarHero"); break;
      case 3: Serial.println("手柄类型: 无线 DualShock"); break;
    }
    Serial.println("\nPS2 手柄连接成功！");
    Serial.println("------------------------------------");
    Serial.println("操控说明：");
    Serial.println("  左摇杆 = 前后移动 + 差速转向");
    Serial.println("  右摇杆 Y = 原地旋转");
    Serial.println("  右摇杆 X = 左右横移");
    Serial.println("  三角 = 全速前进  叉叉 = 全速后退");
    Serial.println("  方块 = 左横移  圆圈 = 右横移");
    Serial.println("  L1 = 原地左旋  R1 = 原地右旋");
    Serial.println("  L2 = 左前斜移  R2 = 右前斜移");
    Serial.println("  十字键 = 方向控制");
    Serial.println("  START = 急停");
    Serial.println("------------------------------------");

    // 连接成功蜂鸣提示
    beepOnce(1000, 100);
    delay(150);
    beepOnce(1500, 100);

    // 配置硬件定时器中断，每 100ms 读取一次 PS2 手柄
    time_ps2 = timerBegin(1, 80, true);
    timerAttachInterrupt(time_ps2, &func_ps2, true);
    timerAlarmWrite(time_ps2, 100000, true);  // 100ms
    timerAlarmEnable(time_ps2);
  } else {
    Serial.println("\nPS2 手柄连接失败！");
    Serial.println("请检查：");
    Serial.println("  1. 手柄接收器是否插好");
    Serial.println("  2. 手柄是否开机配对");
    Serial.println("  3. 接线: CLK=41, CMD=9, CS=42, DAT=10");
    beepOnce(500, 300);
  }
}

// ==================== loop ====================
void loop() {
  // 手柄连接失败时，每秒重试
  if (ps2Error != 0) {
    delay(1000);
    ps2Error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_CS, PS2_DAT, false, false);
    if (ps2Error == 0) {
      Serial.println("PS2 手柄重新连接成功！");
      beepOnce(1000, 100);
      // 启动定时器
      time_ps2 = timerBegin(1, 80, true);
      timerAttachInterrupt(time_ps2, &func_ps2, true);
      timerAlarmWrite(time_ps2, 100000, true);
      timerAlarmEnable(time_ps2);
    }
    return;
  }

  // 将 motor_pwm 数组的值输出到实际电机引脚
  // 引脚顺序: GPIO11(M1正), GPIO12(M1反), GPIO13(M2反), GPIO14(M2正),
  //           GPIO15(M3正), GPIO16(M3反), GPIO17(M4反), GPIO18(M4正)
  analogWrite(M1_FWD, motor_pwm[0]);
  analogWrite(M1_REV, motor_pwm[1]);
  analogWrite(M2_REV, motor_pwm[2]);
  analogWrite(M2_FWD, motor_pwm[3]);
  analogWrite(M3_FWD, motor_pwm[4]);
  analogWrite(M3_REV, motor_pwm[5]);
  analogWrite(M4_REV, motor_pwm[6]);
  analogWrite(M4_FWD, motor_pwm[7]);

  delay(100);
}
