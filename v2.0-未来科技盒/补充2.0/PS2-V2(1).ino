#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include "time.h"
#include "PS2X_lib.h"

// 定义遥感的默认值和调整范围
const int DEFAULT_LX_VALUE = 128;
const int DEFAULT_LY_VALUE = 127;
const int DEFAULT_RX_VALUE = 128;
const int DEFAULT_RY_VALUE = 127;

const int DEADZONE = 4;

const int ANALOG_MIN = 0;
const int ANALOG_MAX = 255;
int lx_min = DEFAULT_LX_VALUE;
int lx_max = DEFAULT_LX_VALUE;
int ly_min = DEFAULT_LY_VALUE;
int ly_max = DEFAULT_LY_VALUE;
int rx_min = DEFAULT_RX_VALUE;
int rx_max = DEFAULT_RX_VALUE;
int ry_min = DEFAULT_RY_VALUE;
int ry_max = DEFAULT_RY_VALUE;

#define MOTOR_PWM1 11
#define MOTOR_PWM2 12
#define MOTOR_PWM3 13
#define MOTOR_PWM4 14
#define MOTOR_PWM5 15
#define MOTOR_PWM6 16
#define MOTOR_PWM7 17
#define MOTOR_PWM8 18

#define PWM_MAX 9
int pwm_value[PWM_MAX] = {0, 150, 160, 170, 190, 210, 220, 230, 240};
int motor_pwm[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int motor_pwm_num[4][2] = {{0,1}, {3,2}, {4,5}, {7,6}}; // 前左，前右，后左，后右

#define PS2_CLK 41
#define PS2_CS 42
#define PS2_CMD 9
#define PS2_DAT 10
#define pressures   false
#define rumble      false

PS2X ps2x; 
int error = 1;
byte type = 0;
hw_timer_t * time_ps2 = NULL;
int tryNum = 1;

int start_flag = 0;

void setup() {
  Serial.begin(9600);
  // put your setup code here, to run once:
  pinMode(MOTOR_PWM1, OUTPUT);
  pinMode(MOTOR_PWM2, OUTPUT);
  pinMode(MOTOR_PWM3, OUTPUT);
  pinMode(MOTOR_PWM4, OUTPUT);
  pinMode(MOTOR_PWM5, OUTPUT);
  pinMode(MOTOR_PWM6, OUTPUT);
  pinMode(MOTOR_PWM7, OUTPUT);
  pinMode(MOTOR_PWM8, OUTPUT);
  analogWriteFrequency(10000); 

  // 初始化遥感的默认值
  lx_min = DEFAULT_LX_VALUE;
  lx_max = DEFAULT_LX_VALUE;
  ly_min = DEFAULT_LY_VALUE;
  ly_max = DEFAULT_LY_VALUE;
  rx_min = DEFAULT_RX_VALUE;
  rx_max = DEFAULT_RX_VALUE;
  ry_min = DEFAULT_RY_VALUE;
  ry_max = DEFAULT_RY_VALUE;

  Serial.println("Server started");

  while (error != 0 && tryNum < 3) {
    delay(1000);// 1 second wait
    //setup pins and settings: GamePad(clock, command, attention, data, Pressures?, Rumble?) check for error
    error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_CS, PS2_DAT, pressures, rumble);
    Serial.print("#try config ");
    Serial.println(tryNum);
    tryNum ++;
  }

  if(error == 0){
    Serial.println(ps2x.Analog(1), HEX);

    type = ps2x.readType();
    switch(type) {
      case 0:
        Serial.println(" Unknown Controller type found ");
        break;
      case 1:
        Serial.println(" DualShock Controller found ");
        break;
      case 2:
        Serial.println(" GuitarHero Controller found ");
        break;
      case 3:
        Serial.println(" Wireless Sony DualShock Controller found ");
        break;
    }
    // 每100ms读取一次PS2设备
    time_ps2 = timerBegin(1, 80, true);
    // timerAttachInterrupt(time_ps2, &func_ps2, false);
    timerAttachInterrupt(time_ps2, &func_ps2, true);
    timerAlarmWrite(time_ps2, 100000, true);
    timerAlarmEnable(time_ps2);
  }
}
int num = 0;
bool motor_direc = true;
int motor_speed = 4;
bool servo_direction = true;
void loop() {
  analogWrite(MOTOR_PWM1, motor_pwm[0]);
  analogWrite(MOTOR_PWM2, motor_pwm[1]);
  analogWrite(MOTOR_PWM3, motor_pwm[2]);
  analogWrite(MOTOR_PWM4, motor_pwm[3]);
  analogWrite(MOTOR_PWM5, motor_pwm[4]);
  analogWrite(MOTOR_PWM6, motor_pwm[5]);
  analogWrite(MOTOR_PWM7, motor_pwm[6]);
  analogWrite(MOTOR_PWM8, motor_pwm[7]);
  delay(100);
}

void func_ps2(){
  byte vibrate = 0;
  ps2x.read_gamepad(false, vibrate);  // read controller and set large motor to spin at 'vibrate' speed

  if (ps2x.Button(PSB_GREEN))
      Serial.println("==================Triangle pressed=====================");
  if (ps2x.ButtonPressed(    PSB_RED))  // will be TRUE if button was JUST pressed
      Serial.println("================Circle just pressed=====================");

  if (ps2x.ButtonReleased(    PSB_PINK))  // will be TRUE if button was JUST released
      Serial.println("================Square just released====================");

  if (ps2x.NewButtonState(PSB_BLUE))  // will be TRUE if button was JUST pressed OR released
      Serial.println("==================X just changed=====================");

  if (ps2x.Button(PSB_L3) || ps2x.Button(PSB_R3))  // print stick values if either is TRUE
  {
      Serial.print("=============Stick Values:");
      Serial.print(ps2x.Analog(PSS_LY), DEC);  // Left stick, Y axis. Other options: LX, RY, RX
      Serial.print(",");
      Serial.print(ps2x.Analog(PSS_LX), DEC);
      Serial.print(",");
      Serial.print(ps2x.Analog(PSS_RY), DEC);
      Serial.print(",");
      Serial.print(ps2x.Analog(PSS_RX), DEC);
      Serial.println("=========================");
  }
  Serial.println("Factory Values: ");
  Serial.print("Left joystick Y-X: ");
  Serial.print(ps2x.Analog(PSS_LY), DEC);
  Serial.print(",");
  Serial.println(ps2x.Analog(PSS_LX), DEC);

  Serial.print("Right joystick Y-X: ");
  Serial.print(ps2x.Analog(PSS_RY), DEC);
  Serial.print(",");
  Serial.println(ps2x.Analog(PSS_RX), DEC);
  Serial.println("=========================");

  // int ly_value = ps2x.Analog(PSS_LY);
  // int lx_value = ps2x.Analog(PSS_LX);
  // ly_value = map(ly_value, 0, 255, 0, PWM_MAX * 2);
  // lx_value = map(lx_value, 0, 255, 0, PWM_MAX * 2);

  // int ry_value = ps2x.Analog(PSS_RY);
  // int rx_value = ps2x.Analog(PSS_RX);
  // ry_value = map(ry_value, 0, 255, 0, PWM_MAX * 2);
  // rx_value = map(rx_value, 0, 255, 0, PWM_MAX * 2);

  int ly_value = ps2x.Analog(PSS_LY);
  int lx_value = ps2x.Analog(PSS_LX);
  int ry_value = ps2x.Analog(PSS_RY);
  int rx_value = ps2x.Analog(PSS_RX);

  // 根据遥感的实际值映射到电机速度
  ly_value = map(ly_value, 0, 255, 0, PWM_MAX * 2);
  lx_value = map(lx_value, 0, 255, 0, PWM_MAX * 2);
  ry_value = map(ry_value, 0, 255, 0, PWM_MAX * 2);
  rx_value = map(rx_value, 0, 255, 0, PWM_MAX * 2);

  // 检查遥感值是否在初始状态附近，如果是，则不移动电机
  
  if (abs(ly_value - PWM_MAX) <= DEADZONE && abs(lx_value - PWM_MAX) <= DEADZONE &&
      abs(ry_value - PWM_MAX) <= DEADZONE && abs(rx_value - PWM_MAX) <= DEADZONE) {
    for (int i = 0; i < 8; i++) {
      motor_pwm[i] = 0;
    }
  } else {
    if ((ry_value >= PWM_MAX - 1) && (ry_value <= PWM_MAX + 1) &&
        (rx_value >= PWM_MAX - 1) && (rx_value <= PWM_MAX + 1)) {
      motor_change(lx_value, ly_value);
    } else {
      motor_change1(rx_value, ry_value);
    }
  }
  
  // if((ry_value >= PWM_MAX - 1) && (ry_value <= PWM_MAX + 1) 
  // && (rx_value >= PWM_MAX - 1) && (rx_value <= PWM_MAX + 1))
  //   motor_change(lx_value, ly_value);
  // else
  //   motor_change1(rx_value, ry_value);
  
}

void motor_change(int lx_value, int ly_value)
{
  Serial.println("You have operated the left joystick Y-X: ");
  Serial.print(ly_value);
  Serial.print(", ");
  Serial.println(lx_value);
  Serial.println("=========================");
  for(int i = 0; i < 4; i++)
  {
    int pwm_num0 = motor_pwm_num[i][0];
    int pwm_num1 = motor_pwm_num[i][1];
    if(ly_value > PWM_MAX + 1){
      motor_pwm[pwm_num0] = 0;
      motor_pwm[pwm_num1] = pwm_value[ly_value - PWM_MAX - 1];
      // 方向控制
      if(lx_value > PWM_MAX)
      {
        if(i % 2)
          motor_pwm[pwm_num1] = motor_pwm[pwm_num1] - (lx_value - PWM_MAX -1) * 4;
        else
          motor_pwm[pwm_num1] = motor_pwm[pwm_num1];// + (lx_value - PWM_MAX -1) * 4;
      }
      else if(lx_value < PWM_MAX)
      {
        if(i % 2)
          motor_pwm[pwm_num1] = motor_pwm[pwm_num1];// + (PWM_MAX - lx_value) * 4;
        else
          motor_pwm[pwm_num1] = motor_pwm[pwm_num1] - (PWM_MAX - lx_value) * 4;
      }
    }
    else if (ly_value < PWM_MAX)
    { // 以前轮为轴转
      motor_pwm[pwm_num0] = 0;
      motor_pwm[pwm_num1] = 0;
      if(lx_value > PWM_MAX)
      {
        if(i == 2)
          motor_pwm[pwm_num1] = pwm_value[lx_value - PWM_MAX - 1];
        else if(i == 3)
          motor_pwm[pwm_num0] = pwm_value[lx_value - PWM_MAX - 1];
      }
      else if(lx_value < PWM_MAX)
      {
        if(i == 2)
          motor_pwm[pwm_num0] = pwm_value[PWM_MAX - lx_value - 2];
        else if(i == 3)
          motor_pwm[pwm_num1] = pwm_value[PWM_MAX - lx_value - 2];
    }
    else
    {
      motor_pwm[pwm_num0] = pwm_value[PWM_MAX - ly_value - 1];
      motor_pwm[pwm_num1] = 0;
      if(lx_value > PWM_MAX)
      {
        if(i % 2)
          motor_pwm[pwm_num0] = motor_pwm[pwm_num0] - (lx_value - PWM_MAX -1) * 4;
        else
          motor_pwm[pwm_num0] = motor_pwm[pwm_num0];// + (lx_value - PWM_MAX -1) * 4;
      }
      else if(lx_value < PWM_MAX)
      {
        if(i % 2)
          motor_pwm[pwm_num0] = motor_pwm[pwm_num0];// + (PWM_MAX - lx_value) * 4;
        else
          motor_pwm[pwm_num0] = motor_pwm[pwm_num0] - (PWM_MAX - lx_value) * 4;
      }
    }

    }
  }
}

void motor_change1(int rx_value, int ry_value)
{
  Serial.println("You have operated the left joystick Y-X: ");
  Serial.print(ry_value);
  Serial.print(", ");
  Serial.print(rx_value);
  Serial.println("=========================");
  for(int i = 0; i < 4; i++)
  {
    int pwm_num0 = motor_pwm_num[i][0];
    int pwm_num1 = motor_pwm_num[i][1];
    if(ry_value > PWM_MAX + 1){
      if(i % 2)
      {
        motor_pwm[pwm_num1] = pwm_value[ry_value - PWM_MAX - 1];
        motor_pwm[pwm_num0] = 0;
      }
      else
      {
        motor_pwm[pwm_num0] = pwm_value[ry_value - PWM_MAX - 1];
        motor_pwm[pwm_num1] = 0;
      }
    }
    else if (ry_value < PWM_MAX - 1)
    { 
      // 原地掉头
      if(i % 2)
      {
        motor_pwm[pwm_num0] = pwm_value[PWM_MAX - ry_value - 1];
        motor_pwm[pwm_num1] = 0;
      }
      else
      {
        motor_pwm[pwm_num1] = pwm_value[PWM_MAX - ry_value - 1];
        motor_pwm[pwm_num0] = 0;
      }
    }
    // 左右横移
    else
    {
      if (rx_value > PWM_MAX)
      {
        if(i == 0 || i == 3)
        {
          motor_pwm[pwm_num0] = pwm_value[rx_value - PWM_MAX -1];
          motor_pwm[pwm_num1] = 0;
        }
        else if(i == 1 || i == 2)
        {
          motor_pwm[pwm_num0] = 0;
          motor_pwm[pwm_num1] = pwm_value[rx_value - PWM_MAX -1];
        }
      }
      else if (rx_value < PWM_MAX)
      {
        if(i == 0 || i == 3)
        {
          motor_pwm[pwm_num1] = pwm_value[PWM_MAX - rx_value - 1];
          motor_pwm[pwm_num0] = 0;
        }
        else if(i == 1 || i == 2)
        {
          motor_pwm[pwm_num1] = 0;
          motor_pwm[pwm_num0] = pwm_value[PWM_MAX - rx_value - 1];
        }
      }
      else
      {
        motor_pwm[pwm_num0] = 0;
        motor_pwm[pwm_num1] = 0;
      }
    }
  }
}
